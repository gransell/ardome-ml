#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/frame.hpp>

using namespace olib::openmedialib::ml;

#define MEDIA_REPO_PREFIX "http://releases.ardendo.se/media-repository/"

BOOST_AUTO_TEST_SUITE( avformat_plugin )

BOOST_AUTO_TEST_CASE( amf_1864_wrong_mp4_duration )
{
	input_type_ptr inp = create_input( "avformat:" MEDIA_REPO_PREFIX "/amf/RegressionTests/AMF-1864/conformed01064700-01065108.mp4" );

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

BOOST_AUTO_TEST_SUITE_END()
