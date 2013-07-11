#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/frame.hpp>
#include <opencorelib/cl/str_util.hpp>
#include <openmedialib/ml/filter.hpp>
#include <openmedialib/ml/audio_block.hpp>
#include <openmedialib/plugins/avformat/avformat_stream.hpp>
#include "utils.hpp"
#include <cmath>

using namespace olib::openmedialib::ml;
using namespace olib::opencorelib::str_util;

#define MEDIA_REPO_PREFIX "http://releases.ardendo.se/media-repository/"
#define MEDIA_REPO_REGRESSION_TESTS_PREFIX "http://releases.ardendo.se/media-repository/amf/RegressionTests"

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
	BOOST_REQUIRE_EQUAL( stream->length(), 250000 );

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
	test_24pcm_input( to_wstring( "avformat:" MEDIA_REPO_PREFIX "/MOV/XDCamHD/XDCAMHD_720p50_6ch_24bit.mov" ) );
}

BOOST_AUTO_TEST_CASE( avformat_input_pcm24_25p )
{
	test_24pcm_input( to_wstring( "avformat:" MEDIA_REPO_PREFIX "/MOV/XDCamHD/XDCAMHD_1080p25_6ch_24bit.mov" ) );
}

BOOST_AUTO_TEST_CASE( avformat_input_pcm24_30p )
{
	test_24pcm_input( to_wstring( "avformat:" MEDIA_REPO_PREFIX "/MOV/XDCamHD/XDCAMHD_1080p30_6ch_24bit.mov" ) );
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


BOOST_AUTO_TEST_SUITE_END()

