#include <boost/test/unit_test.hpp>
//#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/image/image.hpp>
//#include <openmedialib/ml/image/image_types.hpp>
//#include <openimagelib/il/utility.hpp>
#include <openmedialib/ml/openmedialib_plugin.hpp>
//#include <openmedialib/ml/types.hpp>

namespace ml = olib::openmedialib::ml;

BOOST_AUTO_TEST_SUITE( image_class )

void test_image_r8g8b8( )
{
	ml::image_type_ptr img = ml::image::allocate( ml::image::ML_PIX_FMT_R8G8B8, 1920, 1080 );

	BOOST_REQUIRE( img );
	BOOST_CHECK( img->width( ) == 1920 );
	BOOST_CHECK( img->height( ) == 1080 );
	BOOST_CHECK( img->bitdepth( ) == 8 );
	BOOST_CHECK( img->plane_count( ) == 1 );
	BOOST_CHECK( img->block_size( ) == 3 );
	BOOST_CHECK( img->linesize( 0 ) == 5760 );	
	BOOST_CHECK( img->pitch( 0 ) == 5760 );	
	BOOST_CHECK( img->offset( 0 ) == 0 );	
}

void test_image_yuv420p( )
{
	ml::image_type_ptr img = ml::image::allocate( _CT("yuv420p"), 1920, 1080 );
	BOOST_REQUIRE( img );
	img = ml::image::allocate( ml::image::ML_PIX_FMT_YUV420P, 1920, 1080 );
	BOOST_REQUIRE( img );

	BOOST_CHECK( img->width( ) == 1920 );
	BOOST_CHECK( img->height( ) == 1080 );
	BOOST_CHECK( img->bitdepth( ) == 8 );
	BOOST_CHECK( img->plane_count( ) == 3 );
	BOOST_CHECK( img->block_size( ) == 3 );
	BOOST_CHECK( img->linesize( 0 ) == 1920 );	
	BOOST_CHECK( img->linesize( 1 ) == 960 );	
	BOOST_CHECK( img->linesize( 2 ) == 960 );	
	BOOST_CHECK( img->pitch( 0 ) == 1920 );	
	BOOST_CHECK( img->pitch( 1 ) == 960 );	
	BOOST_CHECK( img->pitch( 2 ) == 960 );	
	BOOST_CHECK( img->offset( 0 ) == 0 );	
	BOOST_CHECK( img->offset( 1 ) == 2073600 );	
	BOOST_CHECK( img->offset( 2 ) == 2592000 );	
}

void test_image_yuva420p( )
{
	ml::image_type_ptr img = ml::image::allocate( ml::image::ML_PIX_FMT_YUVA420P, 1920, 1080);

	BOOST_REQUIRE( img );
	BOOST_CHECK( img->width( ) == 1920 );
	BOOST_CHECK( img->height( ) == 1080 );
	BOOST_CHECK( img->bitdepth( ) == 8 );
	BOOST_CHECK( img->plane_count( ) == 4 );
}

void test_image_yuv422p( )
{
	ml::image_type_ptr img = ml::image::allocate( ml::image::ML_PIX_FMT_YUV422P, 1920, 1080 );

	BOOST_REQUIRE( img );
	BOOST_CHECK( img->width( ) == 1920 );
	BOOST_CHECK( img->height( ) == 1080 );
	BOOST_CHECK( img->bitdepth( ) == 8 );
	BOOST_CHECK( img->plane_count( ) == 3 );
	BOOST_CHECK( img->block_size( ) == 3 );
	BOOST_CHECK( img->linesize( 0 ) == 1920 );	
	BOOST_CHECK( img->linesize( 1 ) == 960 );	
	BOOST_CHECK( img->linesize( 2 ) == 960 );	
	BOOST_CHECK( img->pitch( 0 ) == 1920 );	
	BOOST_CHECK( img->pitch( 1 ) == 960 );	
	BOOST_CHECK( img->pitch( 2 ) == 960 );	
	BOOST_CHECK( img->offset( 0 ) == 0 );
	BOOST_CHECK( img->offset( 1 ) == 2073600 );
	BOOST_CHECK( img->offset( 2 ) == 3110400 );
}

BOOST_AUTO_TEST_CASE( allocate )
{
	test_image_r8g8b8();
	test_image_yuv420p();
	test_image_yuva420p();
	test_image_yuv422p();
}

void convert_yuv420p_to_yuv422p( )
{
	ml::image_type_ptr src = ml::image::allocate( ml::image::ML_PIX_FMT_YUV420P, 1920, 1080 );
	

/*	
	ml::image_type_ptr dst = ml::image::convert( src, ml::image::ML_PIX_FMT_YUV420P );


	BOOST_CHECK( dst->ml_pixel_format( ) == ml::image::ML_PIX_FMT_YUV420P );
	BOOST_CHECK( dst->width( ) == 1920 );
	BOOST_CHECK( dst->height( ) == 1080 );
	*/

}

BOOST_AUTO_TEST_CASE( convert )
{
	convert_yuv420p_to_yuv422p();	

}

BOOST_AUTO_TEST_SUITE_END()

