#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/filter.hpp>

using namespace olib::openmedialib::ml;
using namespace olib::openpluginlib;

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
	input_type_ptr inp = create_delayed_input( to_wstring("avformat:" MEDIA_REPO_PREFIX "/AMF-1922/XDCAMHD_1080i60_6ch_24bit.mov") );
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

	//Check that the mpeg2 picture coding extension looks like we expect. Specifically, that the
	//8th byte is 0x98, which indicates that the non_linear_quant and intra_vlc encoder parameters
	//are enabled.
	boost::uint8_t picture_coding_ext[] = { 0x00, 0x00, 0x01, 0xB5, 0x8F, 0xFF, 0xF7, 0x98 };
	BOOST_CHECK_EQUAL( memcmp( stream->bytes() + 38, picture_coding_ext, sizeof( picture_coding_ext ) ), 0 );
}

BOOST_AUTO_TEST_SUITE_END()

