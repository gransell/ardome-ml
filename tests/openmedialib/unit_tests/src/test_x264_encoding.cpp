#include <boost/test/unit_test.hpp>
#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/special_folders.hpp>
#include <opencorelib/cl/str_util.hpp>
#include <opencorelib/cl/guard_define.hpp>
#include <opencorelib/cl/uuid_16b.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/filter.hpp>
#include <openmedialib/ml/store.hpp>
#include <openmedialib/ml/frame.hpp>
#include "utils.hpp"

namespace cl = olib::opencorelib;
namespace ml = olib::openmedialib::ml;
using cl::str_util::to_wstring;
using cl::str_util::to_string;

using boost::uint8_t;

namespace
{
	ml::input_type_ptr load_source( int width, int height, int fps_num, int fps_den )
	{
		ml::input_type_ptr colour_input = ml::create_delayed_input( L"colour:" );
		BOOST_REQUIRE( colour_input );
		colour_input->property( "width" ) = width;
		colour_input->property( "height" ) = height;
		colour_input->property( "fps_num" ) = fps_num;
		colour_input->property( "fps_den" ) = fps_den;
		colour_input->property( "interlace" ) = 0;
		colour_input->property( "colourspace" ) = std::wstring( L"yuv420p" );
		colour_input->property( "r" ) = 128;

		const int ten_seconds_worth_of_frames = 10 * fps_num / fps_den;
		colour_input->property( "out" ) = ten_seconds_worth_of_frames;

		colour_input->init();
		BOOST_REQUIRE_EQUAL( colour_input->get_frames(), ten_seconds_worth_of_frames );
		return colour_input;
	}

	void do_encode_step( const std::wstring &path, const ml::input_type_ptr &source )
	{
		ml::frame_type_ptr first_frame = source->fetch();
		BOOST_REQUIRE( first_frame );
		BOOST_REQUIRE( !first_frame->in_error() );

		ml::store_type_ptr avformat_store = ml::create_store( L"avformat:" + path, first_frame );
		BOOST_REQUIRE( avformat_store );
		avformat_store->property( "vcodec" ) = std::wstring( L"libx264" );
		avformat_store->property( "video_bit_rate" ) = 800000;
		avformat_store->property( "gop_size" ) = 16;
		BOOST_REQUIRE( avformat_store->init() );

		for( int i = 0; i < source->get_frames(); ++i )
		{
			source->seek( i );
			ml::frame_type_ptr fr = source->fetch();
			if ( false == avformat_store->push( fr ) )
			{
				BOOST_FAIL( "Failed to push frame to store " );
			}
		}
		avformat_store->complete();
	}

	void test_encoding( int width, int height, int fps_num, int fps_den )
	{
		ml::input_type_ptr source = load_source( width, height, fps_num, fps_den );
		const int num_source_frames = source->get_frames();

		cl::uuid_16b unique_filename;
		const olib::t_path temp_output_path = cl::special_folder::get( 
			cl::special_folder::temp ) / ( unique_filename.to_hex_string() + _CT(".mp4") );
		BOOST_REQUIRE( !boost::filesystem::exists( temp_output_path ) );
		//Exception-safe cleanup in the event of test failure
		ARGUARD( boost::bind( &boost::filesystem::remove, temp_output_path ) );

		do_encode_step( to_wstring( temp_output_path.native() ), source );

		//Read the produced file back and check that the image properties are correct
		ml::input_type_ptr avformat_input = ml::create_input( L"avformat:" + to_wstring( temp_output_path.native() ) );
		BOOST_REQUIRE( avformat_input );
		avformat_input->sync();

		BOOST_CHECK_EQUAL( avformat_input->get_frames(), num_source_frames );

		ml::frame_type_ptr result_frame = avformat_input->fetch();
		BOOST_REQUIRE( result_frame );
		BOOST_CHECK_EQUAL( result_frame->get_fps_num(), fps_num );
		BOOST_CHECK_EQUAL( result_frame->get_fps_den(), fps_den );

		ml::image_type_ptr result_image = result_frame->get_image();
		BOOST_REQUIRE( result_image );
		BOOST_CHECK_EQUAL( result_image->width(), width );
		BOOST_CHECK_EQUAL( result_image->height(), height );
		BOOST_CHECK_EQUAL( to_string( result_image->pf() ), _CT("yuv420p") );
	}
}

BOOST_AUTO_TEST_SUITE( x264_encoding )

//Very basic tests just to verify that the plugin is functional
//FIXME: This needs to be extended to different gop sizes and bitrates

BOOST_AUTO_TEST_CASE( test_320x180_25fps )
{
	test_encoding( 320, 180, 25, 1 );
}

BOOST_AUTO_TEST_CASE( test_320x180_30fps )
{
	test_encoding( 320, 180, 30000, 1001 );
}

BOOST_AUTO_TEST_CASE( test_320x180_50fps )
{
	test_encoding( 320, 180, 50, 1 );
}

BOOST_AUTO_TEST_CASE( test_320x180_60fps )
{
	test_encoding( 320, 180, 60000, 1001 );
}


BOOST_AUTO_TEST_CASE( test_640x360_25fps )
{
	test_encoding( 640, 360, 25, 1 );
}

BOOST_AUTO_TEST_CASE( test_640x360_30fps )
{
	test_encoding( 640, 360, 30000, 1001 );
}

BOOST_AUTO_TEST_CASE( test_640x360_50fps )
{
	test_encoding( 640, 360, 50, 1 );
}

BOOST_AUTO_TEST_CASE( test_640x360_60fps )
{
	test_encoding( 640, 360, 60000, 1001 );
}

BOOST_AUTO_TEST_SUITE_END()

