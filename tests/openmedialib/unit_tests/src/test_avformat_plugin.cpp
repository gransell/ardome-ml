#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/frame.hpp>
#include <opencorelib/cl/str_util.hpp>

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

BOOST_AUTO_TEST_SUITE_END()

