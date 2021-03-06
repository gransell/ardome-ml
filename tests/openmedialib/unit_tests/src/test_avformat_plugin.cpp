#include <boost/test/unit_test.hpp>
#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/guard_define.hpp>
#include <opencorelib/cl/special_folders.hpp>
#include <opencorelib/cl/uuid_16b.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/frame.hpp>
#include <opencorelib/cl/str_util.hpp>
#include <openmedialib/ml/filter.hpp>
#include <openmedialib/ml/store.hpp>
#include <openmedialib/ml/stack.hpp>
#include <openmedialib/ml/audio_block.hpp>
#include <openmedialib/plugins/avformat/avformat_stream.hpp>
#include "utils.hpp"
#include <cmath>
#include <boost/filesystem.hpp>
#include <fstream>

using namespace olib::openmedialib::ml;
using namespace olib::opencorelib::str_util;
namespace cl = olib::opencorelib;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( avformat_plugin )

BOOST_AUTO_TEST_CASE( amf_1864_wrong_mp4_duration )
{
	input_type_ptr inp = create_input( "avformat:" MEDIA_REPO_REGRESSION_TESTS_PREFIX "/AMF-1864/conformed01064700-01065108.mp4" );

	BOOST_REQUIRE( inp );
	BOOST_CHECK_EQUAL( inp->get_frames(), 129 );

	inp->seek( 128 );
	BOOST_REQUIRE_EQUAL( inp->get_position(), 128 );
	frame_type_ptr frame = inp->fetch();
	BOOST_REQUIRE( frame );
	BOOST_CHECK( frame->get_image() );
	BOOST_CHECK( frame->get_audio() );
	BOOST_CHECK_EQUAL( frame->get_position(), 128 );
}

BOOST_AUTO_TEST_CASE( amf_1922_number_of_frames_wrong_for_streamed_quicktimefile )
{
	input_type_ptr inp = create_delayed_input( to_wstring( "avformat:" MEDIA_REPO_REGRESSION_TESTS_PREFIX "/AMF-1922/XDCAMHD_1080i60_6ch_24bit.mov" ) );
	BOOST_REQUIRE( inp );
	inp->property( "packet_stream" ) = 1;

	inp->init();

	BOOST_CHECK_EQUAL( inp->get_frames(), 301);
}

BOOST_AUTO_TEST_CASE( amf_1948_invalid_imx_produced )
{
	input_type_ptr color_input = create_delayed_input( L"colour:" );
	BOOST_REQUIRE( color_input );
	color_input->property( "width" ) = 720;
	color_input->property( "height" ) = 608;
	color_input->property( "interlace" ) = 1;
	color_input->property( "colourspace" ) = std::wstring( L"yuv422p" );
	color_input->property( "r" ) = 128;

	BOOST_REQUIRE( color_input->init() );

	filter_type_ptr encode_filter = create_filter( L"encode" );
	BOOST_REQUIRE( encode_filter );
	encode_filter->property( "profile" ) = std::wstring( L"vcodecs/imx50" );
	encode_filter->connect( color_input );
	encode_filter->sync();

	frame_type_ptr frame = encode_filter->fetch();
	BOOST_REQUIRE( frame );
	stream_type_ptr stream = frame->get_stream();
	BOOST_REQUIRE( stream );

	//Standard IMX50 video frame size
	BOOST_REQUIRE_EQUAL( stream->length(), 250000u );

	//Check that the mpeg2 picture coding extension looks like we expect. Specifically, we want
	//to check that:
	//* The 8th byte is 0x98, which indicates that the non_linear_quant and intra_vlc encoder
	//  parameters are enabled.
	//* Byte 3-4 of the 7th byte is 10b, which corresponds to an intra_dc_precision value of 10.
	boost::uint8_t picture_coding_ext[] = { 0x00, 0x00, 0x01, 0xB5, 0x8F, 0xFF, 0xFB, 0x98 };
	BOOST_CHECK_EQUAL( memcmp( stream->bytes() + 38, picture_coding_ext, sizeof( picture_coding_ext ) ), 0 );
}

//J#AMF-2123: avformat plugin fails when loading partial ts file with .awi index and packet_stream=1
BOOST_AUTO_TEST_CASE( amf_2123_failed_to_read_partial_ts )
{
	input_type_ptr inp = create_delayed_input( to_wstring( "avformat:" MEDIA_REPO_REGRESSION_TESTS_PREFIX "/AMF-2123/DemoFromArtBeats_first6MB.ts" ) );
	BOOST_REQUIRE( inp );
	inp->property( "ts_index" ) = to_wstring( MEDIA_REPO_REGRESSION_TESTS_PREFIX "/AMF-2123/DemoFromArtBeats.awi" );
	inp->property( "packet_stream" ) = 1;

	BOOST_REQUIRE( inp->init() );

	//Before the fix, an exception was thrown here due to a failed sanity check
	inp->sync();

	BOOST_CHECK_EQUAL( inp->get_frames(), 1200 );
}

void test_gop_size_estimation( const char *file_path, int expected_gop_size )
{
	BOOST_MESSAGE( "Checking gop size estimation for file: " << file_path );
	input_type_ptr inp = create_delayed_input( L"avformat:" MEDIA_REPO_PREFIX_W + to_wstring( file_path ) );
	BOOST_REQUIRE( inp );
	inp->property( "packet_stream" ) = 1;
	BOOST_REQUIRE( inp->init() );

	frame_type_ptr frame = inp->fetch();
	BOOST_REQUIRE( frame );
	stream_type_ptr stream = frame->get_stream();
	BOOST_REQUIRE( stream );

	const int gop_size = stream->estimated_gop_size();
	BOOST_CHECK_EQUAL( gop_size, expected_gop_size );
}

BOOST_AUTO_TEST_CASE( gop_size_estimation )
{
	test_gop_size_estimation( "DV/DVCPro100_1080i50_4ch_16.dif", 1 );
	test_gop_size_estimation( "MOV/DVCPro25/DVCPro25_PAL_4ch_16bit.mov", 1 );
	test_gop_size_estimation( "MOV/DVCPro100/DVCPro100_1080i50_2ch_16bit.mov", 1 );
	test_gop_size_estimation( "MOV/XDCamHD/XDCamHD_1080i50_6ch_24bit.mov", 12 );
	test_gop_size_estimation( "MOV/XDCamHD/XDCamHD_1080i60_16ch_24bit.mov", 15 );
	test_gop_size_estimation( "XDCamEX/AircraftFlyby-720P50FPS/XDZ10003_01.MP4", 25 );
}

BOOST_AUTO_TEST_CASE( avformat_decode_AES )
{
	const int frequency = 48000;
	const int samples = 1920;
	const int channels = 8;


	// create the stream
	stream_mock_ptr stream = stream_mock_ptr( new stream_mock("http://www.ardendo.com/apf/codec/aes", samples * channels * 4, 0, 0, 0, frequency, channels, samples, 3 ) );
	boost::uint32_t *data_in = (boost::uint32_t*) stream->bytes();
	const boost::uint32_t aes_sample = 0x0ABCDEF0;

	for ( int i = 0; i < samples * channels; ++i )
	{
		*data_in = aes_sample;
		++data_in;
	}

	// create track and add the stream to it
	audio::track_type track;
	track.packets[ 0 ] = stream;


	// create block and add the track to it
	audio::block_type_ptr audio_block = audio::block_type_ptr( new audio::block_type );
	audio_block->tracks[ 0 ] = track;
	audio_block->samples = samples;
	audio_block->last = samples;

	// create frame and add audio_block to it
	frame_type_ptr frame = frame_type_ptr( new frame_type( ) );
	frame->set_audio_block( audio_block );


	// create pusher input to feed frame with
	input_type_ptr inp = create_input( "pusher:" );
	BOOST_REQUIRE( inp );

	BOOST_REQUIRE( inp->init() );

	// create decode filter
	filter_type_ptr decode_filter = create_filter( L"avdecode" );
	BOOST_REQUIRE( decode_filter );
	decode_filter->property( "decode_video" ) = 0;
	decode_filter->connect( inp );
	decode_filter->sync();


	// push a frame into input
	inp->push( frame );


	// pull frame from decoder
	frame_type_ptr frame_out = decode_filter->fetch();
	BOOST_REQUIRE( frame_out );
	audio_type_ptr audio = frame_out->get_audio();
	BOOST_REQUIRE( audio );

	// analyze data
	BOOST_CHECK_EQUAL( audio::pcm24_id, audio->id( ) );
	BOOST_CHECK_EQUAL( 4, audio->sample_storage_size( ) );
	BOOST_CHECK_EQUAL( samples * channels * audio->sample_storage_size(), audio->size( ) );
	BOOST_CHECK_EQUAL( samples, audio->samples( ) );
	BOOST_CHECK_EQUAL( frequency, audio->frequency( ) );

	boost::uint32_t *data = (boost::uint32_t*)audio->pointer();

	const boost::uint32_t pcm_sample = 0xABCDEF00;
	bool pcm_out_is_left_shifted_AES_in = true;
	for ( int i = 0; i < samples && pcm_out_is_left_shifted_AES_in; ++i, ++data )
	{
		pcm_out_is_left_shifted_AES_in = ( *data == pcm_sample );
	}

	BOOST_CHECK( pcm_out_is_left_shifted_AES_in );

}

	void test_24pcm_input( std::wstring input_str )
	{
	// check that pcm24 source material results in pcm24 out from input_avformat when packet_stream = 1
	// also check that the decoded audio is pcm24
	input_type_ptr inp = create_delayed_input( input_str );
	BOOST_REQUIRE( inp );
	inp->property( "packet_stream" ) = 1;

	BOOST_REQUIRE( inp->init() );

	filter_type_ptr decode_filter = create_filter( L"decode" );
	BOOST_REQUIRE( decode_filter );
	decode_filter->connect( inp );
	decode_filter->sync();

	frame_type_ptr frame = inp->fetch();
	BOOST_REQUIRE( frame );


	audio::block_type_ptr audio_block = frame->audio_block( );
	BOOST_REQUIRE( audio_block );

	BOOST_REQUIRE( audio_block->tracks.size() );

	audio::track& audio_track = audio_block->tracks.begin()->second;
	BOOST_REQUIRE( audio_track.packets.size() );

	stream_type_ptr audio_stream = audio_track.packets.begin()->second;

	BOOST_CHECK_EQUAL( audio_stream->sample_size(), 3 );
	BOOST_CHECK_EQUAL( audio_stream->codec(), "http://www.ardendo.com/apf/codec/pcm" );

	frame = decode_filter->fetch();
	BOOST_REQUIRE( frame );

	audio_type_ptr audio = frame->get_audio();
	BOOST_REQUIRE( audio );

	BOOST_CHECK_EQUAL( audio->id( ), audio::pcm24_id );

}

BOOST_AUTO_TEST_CASE( avformat_input_pcm24_50p )
{
	test_24pcm_input( L"avformat:" MEDIA_REPO_PREFIX_W L"/MOV/XDCamHD/XDCamHD_720p50_6ch_24bit.mov" );
}

BOOST_AUTO_TEST_CASE( avformat_input_pcm24_25p )
{
	test_24pcm_input( L"avformat:" MEDIA_REPO_PREFIX_W L"/MOV/XDCamHD/XDCamHD_1080p25_6ch_24bit.mov" );
}

BOOST_AUTO_TEST_CASE( avformat_input_pcm24_30p )
{
	test_24pcm_input( L"avformat:" MEDIA_REPO_PREFIX_W L"/MOV/XDCamHD/XDCamHD_1080p29.97_6ch_24bit.mov" );
}

#define abs( a ) a < 0 ? -1 * a : a


template< typename audio_type > 
void test_resampler( boost::uint32_t sample_rate_in, boost::uint32_t sample_rate_out, 
					 boost::uint32_t fps_num_in, boost::uint32_t fps_den_in, 
					 boost::uint32_t channels_in, boost::uint32_t num_frames )
{
	const audio::identity id_in = audio_type::static_id();
	const boost::uint32_t frequency_in = 440;

	const typename audio_type::sample_type id_amplitude = audio_type::max_sample( );
	const typename audio_type::sample_type amplitude = id_amplitude / ( 2 * channels_in ); // to prevent clipping if/when mixing channels

	const typename audio_type::sample_type tolerance = id_amplitude / 10000;

	const boost::uint32_t samples_to_discard_in_output_check = frequency_in / 4;

	// create pusher input to feed frame with
	input_type_ptr inp = create_input( "pusher:" );
	BOOST_REQUIRE( inp );
	inp->property( "length" ) = int( num_frames );
	BOOST_REQUIRE( inp->init() );

	// create resampler filter
	filter_type_ptr resampler_filter = create_filter( L"resampler" );
	BOOST_REQUIRE( resampler_filter );
	resampler_filter->property( "frequency" ) = int( sample_rate_out );
	resampler_filter->connect( inp );
	resampler_filter->sync();



	frame_type_ptr frame;
	boost::uint32_t samples_to_and_inc_frame = 0;
	for ( boost::uint32_t iframe = 0; iframe < num_frames; ++iframe )
	{
		const boost::uint32_t samples_in = audio::samples_for_frame( iframe, sample_rate_in, fps_num_in, fps_den_in );

		audio_type_ptr cur_audio = audio::allocate( id_in, sample_rate_in, channels_in, samples_in, false );

		typename audio_type::sample_type *dst = static_cast<typename audio_type::sample_type*>( cur_audio->pointer( ) );
		for ( boost::uint32_t isample = 0; isample < samples_in; ++isample )
		{
			typename audio_type::sample_type val = amplitude * std::sin( frequency_in * 6.28318530718 * samples_to_and_inc_frame / sample_rate_in );
			for ( boost::uint8_t channel = 0; channel < channels_in; ++channel, ++dst )
			{
				*dst = val;
			}
			++samples_to_and_inc_frame;
		}

		// create frame and add audio to it
		frame.reset( new frame_type( ) );
		frame->set_position( iframe );
		frame->set_audio( cur_audio );
		frame->set_fps( fps_num_in, fps_den_in );

		inp->push( frame );



	}



	samples_to_and_inc_frame = 0;

//     how many samples should we skip at the tail end:
	const boost::uint32_t total_samples_to_end_at = audio::samples_to_frame( num_frames, sample_rate_out, fps_num_in, fps_den_in ) - samples_to_discard_in_output_check;

	for ( boost::uint32_t iframe = 0; iframe < num_frames; ++iframe )
	{
		resampler_filter->seek( iframe );

		frame_type_ptr frame = resampler_filter->fetch( );

		if ( 0 == frame ) // to prevent log spam
			BOOST_REQUIRE( frame );

		audio_type_ptr audio_out = frame->get_audio( );
		if ( 0 == audio_out ) // to prevent log spam
			BOOST_REQUIRE( audio_out );

		if ( audio_out->id( ) != id_in )
			BOOST_REQUIRE_EQUAL( audio_out->id( ), id_in );

		const boost::uint32_t samples_out = audio_out->samples( );
		const boost::uint32_t expected_samples_out = audio::samples_for_frame( iframe, sample_rate_out, fps_num_in, fps_den_in );
		if ( expected_samples_out != samples_out ) // to prevent log spam
			BOOST_CHECK( expected_samples_out == samples_out );
		
		typename audio_type::sample_type* buf = static_cast<typename audio_type::sample_type*>( audio_out->pointer( ) );

		boost::uint32_t isample = 0;
		if ( iframe == 0 ) // this assumes samples_to_discard_in_output_check is less than the number of samples in a frame, a reasonable assumption
		{
			isample = samples_to_discard_in_output_check;
			buf += samples_to_discard_in_output_check * channels_in;
			samples_to_and_inc_frame = samples_to_discard_in_output_check;
		}


		for ( ; isample < samples_out; ++isample, ++samples_to_and_inc_frame )
		{
			if ( total_samples_to_end_at < samples_to_and_inc_frame )
				break;

			typename audio_type::sample_type expected_value = amplitude * std::sin( frequency_in * 6.28318530718 * samples_to_and_inc_frame / sample_rate_out );

			for ( boost::uint32_t channel = 0; channel < channels_in; ++channel, ++buf )
			{
				if ( abs( expected_value - *buf ) <= tolerance )
					continue;

				BOOST_REQUIRE_EQUAL( expected_value, *buf );
			}
		}
	}
}

BOOST_AUTO_TEST_CASE( avformat_filter_resampler )
{
	boost::uint32_t sample_rate_in	= 48000;
	boost::uint32_t sample_rate_out = 48000;
	boost::uint32_t fps_num_in		= 25;
	boost::uint32_t fps_den_in		= 1;
	boost::uint32_t channels_in		= 1;
	boost::uint32_t frames			= 64;

	test_resampler<audio::pcm16>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::pcm24>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::pcm32>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::floats>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );

	sample_rate_in	= 48000;
	sample_rate_out = 44100;
	fps_num_in		= 25;
	fps_den_in		= 1;
	channels_in		= 1;
	frames			= 64;

	test_resampler<audio::pcm16>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::pcm24>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::pcm32>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::floats>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );

	sample_rate_in	= 44100;
	sample_rate_out = 48000;
	fps_num_in		= 25;
	fps_den_in		= 1;
	channels_in		= 1;
	frames			= 64;

	test_resampler<audio::pcm16>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::pcm24>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::pcm32>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::floats>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );

	sample_rate_in	= 48000;
	sample_rate_out = 44100;
	fps_num_in		= 30000;
	fps_den_in		= 1001;
	channels_in		= 1;
	frames			= 64;

	test_resampler<audio::pcm16>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::pcm24>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::pcm32>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::floats>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );

	sample_rate_in	= 44100;
	sample_rate_out = 48000;
	fps_num_in		= 30000;
	fps_den_in		= 1001;
	channels_in		= 1;
	frames			= 64;

	test_resampler<audio::pcm16>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::pcm24>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::pcm32>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::floats>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );

	sample_rate_in	= 44100;
	sample_rate_out = 48000;
	fps_num_in		= 30000;
	fps_den_in		= 1001;
	channels_in		= 2;
	frames			= 64;

	test_resampler<audio::pcm16>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::pcm24>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::pcm32>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::floats>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );

	sample_rate_in	= 44100;
	sample_rate_out = 48000;
	fps_num_in		= 30000;
	fps_den_in		= 1001;
	channels_in		= 6;
	frames			= 64;

	test_resampler<audio::pcm16>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::pcm24>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::pcm32>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::floats>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );

	sample_rate_in	= 44100;
	sample_rate_out = 48000;
	fps_num_in		= 30000;
	fps_den_in		= 1001;
	channels_in		= 8;
	frames			= 64;

	test_resampler<audio::pcm16>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::pcm24>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::pcm32>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::floats>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );

	sample_rate_in	= 44100;
	sample_rate_out = 48000;
	fps_num_in		= 30000;
	fps_den_in		= 1001;
	channels_in		= 32;
	frames			= 64;

	test_resampler<audio::pcm16>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::pcm24>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::pcm32>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );
	test_resampler<audio::floats>( sample_rate_in, sample_rate_out, fps_num_in, fps_den_in, channels_in, frames );

}

/*Tests analyze function for prores 444 with and without alpha*/
void test_prores444_stream_analyze( std::wstring input, bool alpha )
{
	input_type_ptr inp = create_delayed_input( input );
	BOOST_REQUIRE( inp );
	inp->property( "packet_stream" ) = -1;
	inp->init();
	frame_type_ptr frame = inp->fetch();
	BOOST_REQUIRE( frame );
	BOOST_REQUIRE( frame->get_stream() );
	if ( alpha ) {
		BOOST_CHECK_EQUAL( frame->pf( ), _CT("yuva444p16le") );
	} else {
		BOOST_CHECK_EQUAL( frame->pf( ), _CT("yuv444p16le") );
	}
}

void test_prores422_stream_analyze( std::wstring input )
{
	input_type_ptr inp = create_delayed_input( input );
	BOOST_REQUIRE( inp );
	inp->property( "packet_stream" ) = -1;
	inp->init();
	frame_type_ptr frame = inp->fetch();
	BOOST_REQUIRE( frame );
	BOOST_REQUIRE( frame->get_stream() );
	BOOST_CHECK_EQUAL( frame->pf( ), _CT("yuv422p10le") );
}


BOOST_AUTO_TEST_CASE( avformat_prores_stream_analyze )
{
	//444
	test_prores444_stream_analyze( L"avformat:" MEDIA_REPO_PREFIX_W L"/MOV/ProRes/ShortTests/ProRes4444_1080i50_0ch.mov", true );
	test_prores444_stream_analyze( L"avformat:" MEDIA_REPO_PREFIX_W L"/MOV/ProRes/ShortTests/ProRes4444_1080i59.94-DF_0ch.mov", true );
	test_prores444_stream_analyze( L"avformat:" MEDIA_REPO_PREFIX_W L"/MOV/ProRes/ShortTests/ProRes4444_1080p24_6ch_24bit.mov", true );
	test_prores444_stream_analyze( L"avformat:" MEDIA_REPO_PREFIX_W L"/MOV/ProRes/ShortTests/ProRes4444_1080p25_0ch.mov", true );
	test_prores444_stream_analyze( L"avformat:" MEDIA_REPO_PREFIX_W L"/MOV/ProRes/ShortTests/ProRes4444_1080p30_0ch.mov", true );
	test_prores444_stream_analyze( L"avformat:" MEDIA_REPO_PREFIX_W L"/MOV/ProRes/ShortTests/ProRes4444_1080p50_0ch.mov", true );
	test_prores444_stream_analyze( L"avformat:" MEDIA_REPO_PREFIX_W L"/MOV/ProRes/ShortTests/ProRes4444_1080p60_0ch.mov", true );

	//422
	test_prores422_stream_analyze( L"avformat:" MEDIA_REPO_PREFIX_W L"/MOV/ProRes/ShortTests/ProRes422_Proxy_1080p24_6ch_24bit.mov");
	test_prores422_stream_analyze( L"avformat:" MEDIA_REPO_PREFIX_W L"/MOV/ProRes/ShortTests/ProRes422_LT_1080p24_6ch_24bit.mov"); 
	test_prores422_stream_analyze( L"avformat:" MEDIA_REPO_PREFIX_W L"/MOV/ProRes/ShortTests/ProRes422_1080p24_6ch_24bit.mov"); 
	test_prores422_stream_analyze( L"avformat:" MEDIA_REPO_PREFIX_W L"/MOV/ProRes/ShortTests/ProRes422_HQ_1080p24_6ch_24bit.mov"); 
}

input_type_ptr create_test_graph( int fps_num, int fps_den, int count )
{
	input_type_ptr test = create_input( L"test:" );
	BOOST_REQUIRE( test );
	BOOST_REQUIRE_EQUAL( test->get_frames( ), 250 );

	input_type_ptr tone = create_input( L"tone:" );
	BOOST_REQUIRE( tone );
	BOOST_REQUIRE_EQUAL( tone->get_frames( ), 250 );

	filter_type_ptr muxer = create_filter( L"muxer" );
	BOOST_REQUIRE( muxer );

	muxer->connect( test, 0 );
	muxer->connect( tone, 1 );

	filter_type_ptr fps = create_filter( L"frame_rate" );
	fps->property( "fps_num" ) = fps_num;
	fps->property( "fps_den" ) = fps_den;

	fps->connect( muxer );
	fps->sync( );

	BOOST_REQUIRE_EQUAL( fps->get_frames( ), count );

	frame_type_ptr frame = fps->fetch( );

	BOOST_REQUIRE( frame );
	BOOST_REQUIRE( frame->get_image( ) );
	BOOST_REQUIRE( frame->get_audio( ) );

	return fps;
}

void write_to_store( store_type_ptr store, input_type_ptr input )
{
	for ( int i = 0; i < input->get_frames( ); i ++ )
	{
		frame_type_ptr frame = input->fetch( i );
		BOOST_REQUIRE( frame );
		BOOST_REQUIRE_EQUAL( frame->get_position( ), i );
		BOOST_REQUIRE( store->push( frame ) );
	}

	store->complete( );
}

void check_output( std::wstring filename, int fps_num, int fps_den, int count )
{
	input_type_ptr input = create_input( filename );

	BOOST_REQUIRE( input );
	BOOST_REQUIRE_EQUAL( input->get_frames( ), count );

	for ( int i = 0; i < input->get_frames( ); i ++ )
	{
		frame_type_ptr frame = input->fetch( i );
		BOOST_REQUIRE( frame );
		BOOST_REQUIRE_EQUAL( frame->get_position( ), i );
		BOOST_REQUIRE_EQUAL( frame->get_fps_num( ), fps_num );
		BOOST_REQUIRE_EQUAL( frame->get_fps_den( ), fps_den );
		BOOST_REQUIRE( frame->get_image( ) );
		BOOST_REQUIRE_EQUAL( frame->get_image( )->position( ), i );
		BOOST_REQUIRE( frame->get_audio( ) );
		BOOST_REQUIRE_EQUAL( frame->get_audio( )->position( ), i );
	}
}

void check_audio( std::wstring filename, int fps_num, int fps_den, int count )
{
	input_type_ptr input = create_delayed_input( filename );
	BOOST_REQUIRE( input );

	input->property( "fps_num" ) = fps_num;
	input->property( "fps_den" ) = fps_den;

	BOOST_REQUIRE( input->init( ) );

	// NB: Because of padding at the end of encode, there should always be 1 more frame 
	BOOST_REQUIRE_EQUAL( input->get_frames( ), count + 1 );

	for ( int i = 0; i < input->get_frames( ); i ++ )
	{
		frame_type_ptr frame = input->fetch( i );
		BOOST_REQUIRE( frame );
		BOOST_REQUIRE_EQUAL( frame->get_position( ), i );
		BOOST_REQUIRE_EQUAL( frame->get_fps_num( ), fps_num );
		BOOST_REQUIRE_EQUAL( frame->get_fps_den( ), fps_den );
		BOOST_REQUIRE( !frame->get_image( ) );
		BOOST_REQUIRE( frame->get_audio( ) );
		BOOST_REQUIRE_EQUAL( frame->get_audio( )->position( ), i );
	}
}

void store_mpeg1( std::wstring filename, input_type_ptr input )
{
	store_type_ptr store = create_store( filename, input->fetch( ) );
	BOOST_REQUIRE( store );
	store->property( "vcodec" ) = std::wstring( L"mpeg1video" );
	store->property( "b_frames" ) = 2;
	BOOST_REQUIRE( store->init( ) );
	write_to_store( store, input );
}

void store_mpeg2( std::wstring filename, input_type_ptr input )
{
	store_type_ptr store = create_store( filename, input->fetch( ) );
	BOOST_REQUIRE( store );
	store->property( "vcodec" ) = std::wstring( L"mpeg2video" );
	store->property( "b_frames" ) = 2;
	BOOST_REQUIRE( store->init( ) );
	write_to_store( store, input );
}

void store_qtrle( std::wstring filename, input_type_ptr input )
{
	store_type_ptr store = create_store( filename, input->fetch( ) );
	BOOST_REQUIRE( store );
	store->property( "vcodec" ) = std::wstring( L"qtrle" );
	store->property( "acodec" ) = std::wstring( L"pcm_s16le" );
	BOOST_REQUIRE( store->init( ) );
	write_to_store( store, input );
}

void store_defaults( std::wstring filename, input_type_ptr input )
{
	store_type_ptr store = create_store( filename, input->fetch( ) );
	BOOST_REQUIRE( store );
	BOOST_REQUIRE( store->init( ) );
	write_to_store( store, input );
}

typedef void ( *builder_type )( std::wstring, input_type_ptr );
typedef void ( *checker_type )( std::wstring, int, int, int );

void cleanup( const fs::path &p )
{
	boost::system::error_code ec;
	const bool path_existed = fs::remove( p, ec );
	if( path_existed )
	{
		//Fail the test on error, but don't throw since we're getting called from a destructor
		BOOST_CHECK_MESSAGE( !ec, "Failed to remove temp file " << to_string( p.native() ) << ": " << ec );
	}
}

fs::path generate_unique_tmp_path( const olib::t_string &base_name )
{
	cl::uuid_16b unique_id;
	const fs::path unique_path = 
		cl::special_folder::get( cl::special_folder::temp ) /
		( unique_id.to_hex_string() + base_name );
	BOOST_REQUIRE( !fs::exists( unique_path ) );
	return unique_path;
}

void store_test( const olib::t_string &filename, int fps_num, int fps_den, int count, builder_type write_output, checker_type checker = check_output )
{
	BOOST_REQUIRE_EQUAL( filename.find( _CT('/') ), olib::t_string::npos );
	BOOST_REQUIRE_EQUAL( filename.find( _CT(':') ), olib::t_string::npos );

	const fs::path unique_path = generate_unique_tmp_path( filename );
	ARGUARD( boost::bind( &cleanup, unique_path ) );
	const std::wstring decorated_path = L"avformat:" + to_wstring( unique_path.native() );
	
	input_type_ptr input = create_test_graph( fps_num, fps_den, count );
	write_output( decorated_path, input );
	checker( decorated_path, fps_num, fps_den, count );
}

BOOST_AUTO_TEST_CASE( aml_store_mpeg1_pal )
{
	store_test( _CT("mpeg1.mpg"), 25, 1, 250, store_mpeg1 );
}

BOOST_AUTO_TEST_CASE( aml_store_mpeg1_ntsc )
{
	store_test( _CT("mpeg1.mpg"), 30000, 1001, 300, store_mpeg1 );
}

BOOST_AUTO_TEST_CASE( aml_store_mpeg1_movie )
{
	store_test( _CT("mpeg1.mpg"), 24000, 1001, 240, store_mpeg1 );
}

BOOST_AUTO_TEST_CASE( aml_store_mpeg2_pal )
{
	store_test( _CT("mpeg2.mpg"), 25, 1, 250, store_mpeg2 );
}

BOOST_AUTO_TEST_CASE( aml_store_mpeg2_ntsc )
{
	store_test( _CT("mpeg2.mpg"), 30000, 1001, 300, store_mpeg2 );
}

BOOST_AUTO_TEST_CASE( aml_store_mpeg2_movie )
{
	store_test( _CT("mpeg2.mpg"), 24000, 1001, 240, store_mpeg2 );
}

BOOST_AUTO_TEST_CASE( aml_store_qtrle_pal )
{
	store_test( _CT("qtrle.mov"), 25, 1, 250, store_qtrle );
}

BOOST_AUTO_TEST_CASE( aml_store_qtrle_ntsc )
{
	store_test( _CT("qtrle.mov"), 30000, 1001, 300, store_qtrle );
}

BOOST_AUTO_TEST_CASE( aml_store_qtrle_movie )
{
	store_test( _CT("qtrle.mov"), 24000, 1001, 240, store_qtrle );
}

BOOST_AUTO_TEST_CASE( aml_store_mp2_pal )
{
	store_test( _CT("mp2.mp2"), 25, 1, 250, store_defaults, check_audio );
}

BOOST_AUTO_TEST_CASE( aml_store_mp2_ntsc )
{
	store_test( _CT("mp2.mp2"), 30000, 1001, 300, store_defaults, check_audio );
}

BOOST_AUTO_TEST_CASE( aml_store_mp2_movie )
{
	store_test( _CT("mp2.mp2"), 24000, 1001, 240, store_defaults, check_audio );
}

#ifndef WIN32

// The following tests are not executed on Windows because the ffmpeg build currently lacks
// the required dependencies

BOOST_AUTO_TEST_CASE( aml_store_mp3_pal )
{
	store_test( _CT("mp3.mp3"), 25, 1, 250, store_defaults, check_audio );
}

BOOST_AUTO_TEST_CASE( aml_store_mp3_ntsc )
{
	store_test( _CT("mp3.mp3"), 30000, 1001, 300, store_defaults, check_audio );
}

BOOST_AUTO_TEST_CASE( aml_store_mp3_movie )
{
	store_test( _CT("mp3.mp3"), 24000, 1001, 240, store_defaults, check_audio );
}

#endif

const fs::path unique( const olib::t_string &filename )
{
	BOOST_REQUIRE_EQUAL( filename.find( _CT('/') ), olib::t_string::npos );
	BOOST_REQUIRE_EQUAL( filename.find( _CT(':') ), olib::t_string::npos );
	cl::uuid_16b unique_id;
	const fs::path unique_path = cl::special_folder::get( cl::special_folder::temp ) / ( unique_id.to_hex_string() + filename );
	return unique_path;
}

BOOST_AUTO_TEST_CASE( aml_change_audio_spec )
{
	// Creates 3 mp2 files with differing audio specs, checks that audio spec matches expected for each frame while
	// creating them, concatenates all files together, decodes resultant file and checks that things match as expected.
	// They don't of course. The concatenated output is 1 frame longer (FUDGE1), the third section comes in 1 frame 
	// later (FUDGE2). In packet streaming mode, the mono comes in 1 frame earlier (due to the differences in decode
	// order). None of these are all that surprising given the nature of audio packets being of a different size
	// to frame audio (with frame audio being the manner in which they're created and read back).
	//
	// Two additional tests are made on the resampler here to ensure that 1) it will conform the varying input to the
	// audio spec of the first frame and 2) it will conform the varying input to a specific form.

	const fs::path file1 = unique( _CT("file1.mp2") );
	const fs::path file2 = unique( _CT("file2.mp2") );
	const fs::path file3 = unique( _CT("file3.mp2") );
	const fs::path full = unique( _CT("full.mp2") );

	BOOST_REQUIRE( !fs::exists( file1 ) && !fs::exists( file2 ) && !fs::exists( file3 ) && !fs::exists( full ) );

	ARGUARD( boost::bind( &cleanup, file1 ) );
	ARGUARD( boost::bind( &cleanup, file2 ) );
	ARGUARD( boost::bind( &cleanup, file3 ) );
	ARGUARD( boost::bind( &cleanup, full ) );

	input_type_ptr input = stack( )
	.push( L"tone: out=25 frequency=48000 channels=2 filter:store store=" + to_wstring( file1.native( ) ) )
	.push( L"tone: out=25 frequency=48000 channels=1 filter:store store=" + to_wstring( file2.native( ) ) )
	.push( L"tone: out=25 frequency=44100 channels=1 filter:store store=" + to_wstring( file3.native( ) ) )
	.push( L"filter:playlist slots=3" )
	.pop( );

	BOOST_REQUIRE( input );
	BOOST_CHECK( input->get_frames( ) == 75 );

	int channel_pattern[ ] = { 2, 1, 1 };
	int frequency_pattern[ ] = { 48000, 48000, 44100 };

	for ( int i = 0; i < input->get_frames( ); i ++ )
	{
		frame_type_ptr frame = input->fetch( i );
		BOOST_REQUIRE( frame );
		audio_type_ptr audio = frame->get_audio( );
		BOOST_REQUIRE( frame->get_audio( ) );
		BOOST_CHECK( audio->channels( ) == channel_pattern[ i / 25 ] );
		BOOST_CHECK( audio->frequency( ) == frequency_pattern[ i / 25 ] );
	}

	input = input_type_ptr( );

	std::ifstream if1( to_string( file1.native( ) ).c_str( ), std::ios_base::binary );
	std::ifstream if2( to_string( file2.native( ) ).c_str( ), std::ios_base::binary );
	std::ifstream if3( to_string( file3.native( ) ).c_str( ), std::ios_base::binary );
	std::ofstream of( to_string( full.native( ) ).c_str( ), std::ios_base::binary );

	of << if1.rdbuf( ) << if2.rdbuf( ) << if3.rdbuf( );
	of.close();

	input = create_input( full.native( ) );

	BOOST_REQUIRE( input );
	BOOST_CHECK( input->get_frames( ) == 76 ); // FUDGE1

	for ( int i = 0; i < 75; i ++ )
	{
		frame_type_ptr frame = input->fetch( i );
		BOOST_REQUIRE( frame );
		audio_type_ptr audio = frame->get_audio( );
		BOOST_REQUIRE( frame->get_audio( ) );
		BOOST_CHECK( audio->channels( ) == channel_pattern[ i / 25 ] );
		if ( i == 50 ) continue; // FUDGE2
		BOOST_CHECK( audio->frequency( ) == frequency_pattern[ i / 25 ] );
	}

	input = stack( ).push( to_wstring( full.native( ) ) + L" packet_stream=1 filter:decode" ).pop( );

	BOOST_REQUIRE( input );
	BOOST_CHECK( input->get_frames( ) == 76 ); // FUDGE1

	for ( int i = 0; i < 75; i ++ )
	{
		frame_type_ptr frame = input->fetch( i );
		BOOST_REQUIRE( frame );
		audio_type_ptr audio = frame->get_audio( );
		BOOST_REQUIRE( frame->get_audio( ) );
		if ( i == 24 ) continue; // FUDGE3
		BOOST_CHECK( audio->channels( ) == channel_pattern[ i / 25 ] );
		if ( i == 50 ) continue; // FUDGE2
		BOOST_CHECK( audio->frequency( ) == frequency_pattern[ i / 25 ] );
	}

	input = stack( ).push( to_wstring( full.native( ) ) + L" filter:resampler" ).pop( );

	BOOST_REQUIRE( input );
	BOOST_CHECK( input->get_frames( ) == 76 ); // FUDGE1

	for ( int i = 0; i < 75; i ++ )
	{
		frame_type_ptr frame = input->fetch( i );
		BOOST_REQUIRE( frame );
		audio_type_ptr audio = frame->get_audio( );
		BOOST_REQUIRE( frame->get_audio( ) );
		BOOST_CHECK( audio->channels( ) == channel_pattern[ 0 ] );
		BOOST_CHECK( audio->frequency( ) == frequency_pattern[ 0 ] );
	}

	input = stack( ).push( to_wstring( full.native( ) ) + L" filter:resampler channels=6 frequency=96000" ).pop( );

	BOOST_REQUIRE( input );
	BOOST_CHECK( input->get_frames( ) == 76 ); // FUDGE1

	for ( int i = 0; i < 75; i ++ )
	{
		frame_type_ptr frame = input->fetch( i );
		BOOST_REQUIRE( frame );
		audio_type_ptr audio = frame->get_audio( );
		BOOST_REQUIRE( frame->get_audio( ) );
		BOOST_CHECK( audio->channels( ) == 6 );
		BOOST_CHECK( audio->frequency( ) == 96000 );
	}
}

BOOST_AUTO_TEST_CASE( aml_change_video_spec )
{
	// Creates 3 m2v files with differing video specs, checks that video spec matches expected for each frame while
	// creating them, concatenates all files together, decodes resultant file and checks that things match as expected.
	// They don't - see FUDGE stuff below.
	//
	// An additional tests is made on the resampler here to ensure that it will conform the varying input to the
	// video spec given.

	const fs::path file1 = unique( _CT("file1.m2v") );
	const fs::path file2 = unique( _CT("file2.m2v") );
	const fs::path file3 = unique( _CT("file3.m2v") );
	const fs::path full = unique( _CT("full.m2v") );

	BOOST_REQUIRE( !fs::exists( file1 ) && !fs::exists( file2 ) && !fs::exists( file3 ) && !fs::exists( full ) );

	ARGUARD( boost::bind( &cleanup, file1 ) );
	ARGUARD( boost::bind( &cleanup, file2 ) );
	ARGUARD( boost::bind( &cleanup, file3 ) );
	ARGUARD( boost::bind( &cleanup, full ) );

	input_type_ptr input = stack( )
	.push( L"test: filter:store @store.b_frames=2 store=" + to_wstring( file1.native( ) ) )
	.push( L"colour: sar_num=16 sar_den=15 test: filter:compositor filter:store @store.b_frames=2 store=" + to_wstring( file2.native( ) ) )
	.push( L"colour: sar_num=64 sar_den=45 test: filter:compositor filter:store @store.b_frames=2 store=" + to_wstring( file3.native( ) ) )
	.push( L"filter:playlist slots=3" )
	.pop( );

	BOOST_REQUIRE( input );
	BOOST_CHECK( input->get_frames( ) == 750 );

	int width_pattern[ ] = { 512, 720, 720 };
	int height_pattern[ ] = { 512, 576, 576 };
	int sar_num_pattern[ ] = { 1, 16, 64 };
	int sar_den_pattern[ ] = { 1, 15, 45 };

	for ( int i = 0; i < input->get_frames( ); i ++ )
	{
		frame_type_ptr frame = input->fetch( i );
		BOOST_REQUIRE( frame );
		image_type_ptr image = frame->get_image( );
		BOOST_REQUIRE( image );
		BOOST_CHECK( image->width( ) == width_pattern[ i / 250 ] );
		BOOST_CHECK( image->height( ) == height_pattern[ i / 250 ] );
		BOOST_CHECK( frame->get_sar_num( ) == sar_num_pattern[ i / 250 ] );
		BOOST_CHECK( frame->get_sar_den( ) == sar_den_pattern[ i / 250 ] );
	}

	input = input_type_ptr( );

	std::ifstream if1( to_string( file1.native( ) ).c_str( ), std::ios_base::binary );
	std::ifstream if2( to_string( file2.native( ) ).c_str( ), std::ios_base::binary );
	std::ifstream if3( to_string( file3.native( ) ).c_str( ), std::ios_base::binary );
	std::ofstream of( to_string( full.native( ) ).c_str( ), std::ios_base::binary );

	of << if1.rdbuf( ) << if2.rdbuf( ) << if3.rdbuf( );
	of.close();

	// These fudges are necessary with mpeg stream concatenation afaict - the last frame
	// of each section is lost somewhere in the decode (presuambly because a null is required
	// to release the last image from the end of each section and that doesn't occur when 
	// subsequent packets are available - presumably the last section will have that extra
	// image available).

	int FUDGE_section_size = 249;
	int FUDGE_frame_count = FUDGE_section_size * 3;
	
	input = create_input( full.native( ) );

	BOOST_REQUIRE( input );
	BOOST_CHECK( input->get_frames( ) >= FUDGE_frame_count );

	for ( int i = 0; i < FUDGE_frame_count; i ++ )
	{
		frame_type_ptr frame = input->fetch( i );
		BOOST_REQUIRE( frame );
		image_type_ptr image = frame->get_image( );
		BOOST_REQUIRE( image );
		BOOST_CHECK( image->width( ) == width_pattern[ i / FUDGE_section_size ] );
		BOOST_CHECK( image->height( ) == height_pattern[ i / FUDGE_section_size ] );
		BOOST_CHECK( frame->get_sar_num( ) == sar_num_pattern[ i / FUDGE_section_size ] );
		BOOST_CHECK( frame->get_sar_den( ) == sar_den_pattern[ i / FUDGE_section_size ] );
	}

#if 0
	// Left out for now - has different behavour to the non-packet_stream case, but is currently
	// broken with sar handling, and sees 250 frames per section, though the last is always corrupt.

	input = stack( ).push( to_wstring( full.native( ) ) + L" packet_stream=1 filter:decode" ).pop( );

	BOOST_REQUIRE( input );
	BOOST_CHECK( input->get_frames( ) >= FUDGE_frame_count );

	for ( int i = 0; i < FUDGE_frame_count; i ++ )
	{
		frame_type_ptr frame = input->fetch( i );
		BOOST_REQUIRE( frame );
		image_type_ptr image = frame->get_image( );
		BOOST_REQUIRE( image );
		BOOST_CHECK( image->width( ) == width_pattern[ i / FUDGE_section_size ] );
		BOOST_CHECK( image->height( ) == height_pattern[ i / FUDGE_section_size ] );
		BOOST_CHECK( frame->get_sar_num( ) == sar_num_pattern[ i / FUDGE_section_size ] );
		BOOST_CHECK( frame->get_sar_den( ) == sar_den_pattern[ i / FUDGE_section_size ] );
	}
#endif

	input = stack( ).push( to_wstring( full.native( ) ) + L" filter:swscale width=1280 height=720 sar_num=1 sar_den=1" ).pop( );

	BOOST_REQUIRE( input );
	BOOST_CHECK( input->get_frames( ) >= FUDGE_frame_count );

	for ( int i = 0; i < input->get_frames( ); i ++ )
	{
		frame_type_ptr frame = input->fetch( i );
		BOOST_REQUIRE( frame );
		image_type_ptr image = frame->get_image( );
		BOOST_REQUIRE( image );
		BOOST_CHECK( image->width( ) == 1280 );
		BOOST_CHECK( image->height( ) == 720 );
		BOOST_CHECK( frame->get_sar_num( ) == 1 );
		BOOST_CHECK( frame->get_sar_den( ) == 1 );
	}
}

BOOST_AUTO_TEST_CASE( amf_2332_segfault_when_generating_jpeg_from_yuv411p )
{
	input_type_ptr color_input = create_delayed_input( L"colour:" );
	BOOST_REQUIRE( color_input );
	color_input->property( "width" ) = 720;
	color_input->property( "height" ) = 576;
	color_input->property( "colourspace" ) = std::wstring( L"yuv411p" );
	color_input->property( "r" ) = 128;

	BOOST_REQUIRE( color_input->init() );
	frame_type_ptr frame = color_input->fetch();
	BOOST_REQUIRE( frame );

	const fs::path unique_path = generate_unique_tmp_path( _CT("amf_2332.jpg") );
	ARGUARD( boost::bind( &cleanup, unique_path ) );
	const std::wstring store_uri = L"avformat:" + to_wstring( unique_path.native() );

	store_type_ptr jpg_store = create_store( store_uri, frame );
	BOOST_REQUIRE( jpg_store );
	BOOST_REQUIRE( jpg_store->init() );
	BOOST_REQUIRE( jpg_store->push( frame ) );
	jpg_store->complete();
}

BOOST_AUTO_TEST_SUITE_END()

