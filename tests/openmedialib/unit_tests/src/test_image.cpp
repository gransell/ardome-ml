#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/image/image.hpp>
#include <openmedialib/ml/openmedialib_plugin.hpp>

namespace ml = olib::openmedialib::ml;

BOOST_AUTO_TEST_SUITE( image_class )

struct image_properties
{
	int width;
	int height;
	int bitdepth;
	int plane_count;
	int num_components;
	int linesize[4];
	int pitch[4];
	int offset[4];
};

void test_image_properties( ml::image_type_ptr img, image_properties *p )
{
    BOOST_REQUIRE( img );
    BOOST_CHECK( img->width( ) == p->width );
    BOOST_CHECK( img->height( ) == p->height );
    BOOST_CHECK( img->bitdepth( ) == p->bitdepth );
    BOOST_CHECK( img->plane_count( ) == p->plane_count );
    BOOST_CHECK( img->num_components( ) == p->num_components );	
    for ( int i = 0; i < p->plane_count; i++ ) {
        BOOST_CHECK( img->linesize( i ) == p->linesize[i] );
        BOOST_CHECK( img->pitch( i ) == p->pitch[i] );
        BOOST_CHECK( img->offset( i ) == p->offset[i] );
    }
}

BOOST_AUTO_TEST_CASE( allocate )
{
	/*
	image properties: WIDTH, HEIGHT, BITDEPTH, PLANE_COUNT, BLOCK_SIZE, [ LINESIZE ] , [ PITCH ], [ OFFSET ]
	*/
	image_properties r8g8b8_137_791 = { 137, 791, 8, 1, 3, { 411 }, { 411 }, { 0 } };
	test_image_properties( ml::image::allocate( ml::image::ML_PIX_FMT_R8G8B8, 137, 791 ), &r8g8b8_137_791 );

	// RGB
	image_properties r8g8b8_1920_1080 = { 1920, 1080, 8, 1, 3, { 5760 }, { 5760 }, { 0 } };
	test_image_properties( ml::image::allocate( ml::image::ML_PIX_FMT_R8G8B8, 1920, 1080 ), &r8g8b8_1920_1080 );
	
	// YUV420P
	image_properties yuv420p_1920_1080 = { 1920, 1080, 8, 3, 3, { 1920, 960, 960 }, { 1920, 960, 960 }, { 0, 2073600, 2592000 } };
	test_image_properties( ml::image::allocate( ml::image::ML_PIX_FMT_YUV420P, 1920, 1080 ), &yuv420p_1920_1080 );

	// YUVA420P
	image_properties yuva420p_1920_1080 = { 1920, 1080, 8, 4, 4, { 1920, 960, 960, 1920 }, { 1920, 960, 960, 1920 }, { 0, 2073600, 2592000, 3110400 } };
	test_image_properties( ml::image::allocate( ml::image::ML_PIX_FMT_YUVA420P, 1920, 1080 ), &yuva420p_1920_1080 );
	
	// YUV422P
	image_properties yuv422p_1920_1080 = { 1920, 1080, 8, 3, 3, { 1920, 960, 960 }, { 1920, 960, 960 }, { 0, 2073600, 3110400 } };
	test_image_properties( ml::image::allocate( ml::image::ML_PIX_FMT_YUV422P, 1920, 1080 ), &yuv422p_1920_1080 );

	// YUV422P10LE
	image_properties yuv422p10le_1920_1080 = { 1920, 1080, 10, 3, 3, { 1920, 960, 960 }, { 1920, 960, 960 }, { 0, 2073600, 3110400 } };
	test_image_properties( ml::image::allocate( ml::image::ML_PIX_FMT_YUV422P10LE, 1920, 1080 ), &yuv422p10le_1920_1080 );
}

void convert_yuv420p_to_yuv422p( )
{
	ml::image_type_ptr src = ml::image::allocate( ml::image::ML_PIX_FMT_YUV420P, 1920, 1080 );
	
	ml::image_type_ptr dst = ml::image::convert( src, ml::image::ML_PIX_FMT_YUV422P );

	BOOST_CHECK( dst->ml_pixel_format( ) == ml::image::ML_PIX_FMT_YUV422P );
}

void convert_yuv422p_to_yuv420p( )
{
	ml::image_type_ptr src = ml::image::allocate( ml::image::ML_PIX_FMT_YUV422P, 1920, 1080 );
	
	ml::image_type_ptr dst = ml::image::convert( src, ml::image::ML_PIX_FMT_YUV420P );

	BOOST_CHECK( dst->ml_pixel_format( ) == ml::image::ML_PIX_FMT_YUV420P );
}

BOOST_AUTO_TEST_CASE( convert )
{
	convert_yuv420p_to_yuv422p();	
	convert_yuv422p_to_yuv420p();	

}


void rescale_yuv420p()
{
    ml::image_type_ptr src = ml::image::allocate( ml::image::ML_PIX_FMT_YUV420P, 1280, 720 );
    ml::image_type_ptr dst = ml::image::rescale( src, 1920, 1080, ml::image::BICUBIC_SAMPLING );
}

BOOST_AUTO_TEST_CASE( rescale )
{
    rescale_yuv420p( );
}

BOOST_AUTO_TEST_SUITE_END()

