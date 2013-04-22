#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/frame.hpp>
#include <opencorelib/cl/str_util.hpp>
#include <openmedialib/ml/filter.hpp>
#include <openmedialib/ml/audio_block.hpp>
#include <openmedialib/plugins/avformat/avformat_stream.hpp>

using namespace olib::openmedialib::ml;
using namespace olib::opencorelib::str_util;

#define MEDIA_REPO_PREFIX "http://releases.ardendo.se/media-repository/amf/RegressionTests"

BOOST_AUTO_TEST_SUITE( avformat_plugin )

BOOST_AUTO_TEST_CASE( amf_1864_wrong_mp4_duration )
{
	input_type_ptr inp = create_input( "avformat:" MEDIA_REPO_PREFIX "/AMF-1864/conformed01064700-01065108.mp4" );

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
	input_type_ptr inp = create_delayed_input( to_wstring( "avformat:" MEDIA_REPO_PREFIX "/AMF-1922/XDCAMHD_1080i60_6ch_24bit.mov" ) );
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

namespace
{

	// subclassing stream_avformat just to get most of the stream_type implementation. We
	// still need to overload codec() since it can't get set 
	// to http://www.ardendo.com/apf/codec/aes using the stream_avformat class
	class stream_test : public stream_avformat
	{
		public:
			stream_test( size_t length, boost::int64_t position, 
						 boost::int64_t key, int bitrate, int frequency, int channels, 
						 int samples, int sample_size , std::string codec_str)
				: stream_avformat( CODEC_ID_NONE, length, position, key, bitrate, frequency, channels, samples, sample_size)
				  , codec_str_( codec_str )
		{
		}

			const std::string &codec( ) const { return codec_str_; }

		private:

			std::string codec_str_;

	};

	typedef boost::shared_ptr< stream_test > stream_test_ptr ;

}



BOOST_AUTO_TEST_CASE( avformat_decode_AES )
{
	const int frequency = 48000;
	const int samples = 1920;
	const int channels = 8;


	// create the stream
	stream_test_ptr stream = stream_test_ptr( new stream_test( samples * channels * 4, 0, 0, 0, frequency, channels, samples, 3, "http://www.ardendo.com/apf/codec/aes" ) );
	boost::uint32_t *data_in = (boost::uint32_t*) stream->bytes();
	const boost::uint32_t aes_sample = 0x0FFFFFF0;

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
	BOOST_CHECK_EQUAL( samples * channels * 4, audio->size( ) );
	BOOST_CHECK_EQUAL( samples, audio->samples( ) );
	BOOST_CHECK_EQUAL( 4, audio->sample_size( ) );
	BOOST_CHECK_EQUAL( frequency, audio->frequency( ) );

	boost::uint32_t *data = (boost::uint32_t*)audio->pointer();

	const boost::uint32_t pcm_sample = 0xFFFFFF00;
	bool pcm_out_is_FFFFFF00 = true;
	for ( int i = 0; i < samples && pcm_out_is_FFFFFF00 ; ++i, ++data )
	{
		pcm_out_is_FFFFFF00 = ( *data == pcm_sample );
	}

	BOOST_CHECK( pcm_out_is_FFFFFF00 );

}

BOOST_AUTO_TEST_SUITE_END()

