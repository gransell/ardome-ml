#include <boost/test/unit_test.hpp>
#include <boost/assign.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/frame.hpp>
#include <opencorelib/cl/str_util.hpp>
#include <openmedialib/ml/filter.hpp>
#include <openmedialib/ml/store.hpp>

using namespace olib::openmedialib::ml;
using namespace olib::openimagelib::il;
using namespace olib::opencorelib::str_util;


BOOST_AUTO_TEST_SUITE( test_swscale_filter )

static std::vector< std::wstring > pixel_formats;

enum image_side
{
	LEFT,
	RIGHT,
	TOP,
	BOTTOM
};

void intialize_test_environment()
{
	// these are taken from create_corrections() function in filter_swscale.cpp. 
	pixel_formats.push_back( L"r8g8b8" );
	pixel_formats.push_back( L"r8g8b8a8" );
	pixel_formats.push_back( L"b8g8r8" );
	pixel_formats.push_back( L"b8g8r8a8" );
	pixel_formats.push_back( L"yuv444p" );
	pixel_formats.push_back( L"yuv422p" );
	pixel_formats.push_back( L"yuv422" );
	pixel_formats.push_back( L"yuv411p" );
	pixel_formats.push_back( L"yuv420p" );
	pixel_formats.push_back( L"uyv422" );
}

input_type_ptr create_input_with_params( const wchar_t *format )
{
	input_type_ptr colour = create_input( L"colour:" );
	BOOST_REQUIRE( colour );
	colour->property( "r" ) = 180;
	colour->property( "width" ) = 400;
	colour->property( "height" ) = 400;
	colour->property( "sar_num" ) = 1;
	colour->property( "sar_den" ) = 1;	
	colour->property( "out" ) = 150;
	colour->property( "colourspace" ) = std::wstring( format );

	return colour;
}

int match_value_with_array( boost::uint8_t *ptr, int array_size, boost::uint8_t value )
{
	for ( int i = 0; i < array_size; i++ )
	{
		if ( *ptr - value )
		{
			return *ptr - value;
		}
	}

	return 0;
}

inline bool check_rectangle_for_colour( boost::uint8_t *ptr, boost::uint8_t value, int width, int pitch, int height )
{
	while( height -- > 0 )
	{
		int comparison_result = match_value_with_array( ptr, width, value );
		if ( comparison_result )
			return false;

		ptr += pitch;
	}

	return true;
}

void test_black_region( image_type_ptr image, image_side side, int region_breadth )
{
	// copied the looping logic from the border() function in filter_swscale.cpp. not so convinced that it is correct for the non-planar yuv formats though.
	int yuv[ 3 ];
	rgb24_to_yuv444( yuv[ 0 ], yuv[ 1 ], yuv[ 2 ], 0, 0, 0 );

	const int iwidth = image->width( );
	const int iheight = image->height( );

	for ( int i = 0; i < image->plane_count( ); i++ )
	{
		const float wps = float( image->linesize( i ) ) / iwidth;
		const float hps = float( image->height( i ) ) / iheight;

		boost::uint8_t *ptr = image->data( i );
		const boost::uint8_t value = is_yuv_planar( image ) ?  yuv[ i ] : 0;

		const int pitch = image->pitch( i );

		if ( side == LEFT )
		{
			if ( !check_rectangle_for_colour( ptr, value, region_breadth * wps, pitch, iheight * hps ) )
				BOOST_FAIL( "Unexpected border color on left region for " << to_string( image->pf() ) );
		}
		else if ( side == RIGHT )
		{
			if ( !check_rectangle_for_colour( ptr + int( ( iwidth - region_breadth ) * wps ), value, region_breadth * wps, pitch, iheight * hps ) )
				BOOST_FAIL( "Unexpected border color on right region for " << to_string( image->pf() ) );
		}
		else if ( side == TOP )
		{
			if ( !check_rectangle_for_colour( ptr, value, iwidth * wps, pitch, region_breadth * hps ) )
				BOOST_FAIL( "Unexpected border color on top region for " << to_string( image->pf() ) );
		}
		else if ( side == BOTTOM )
		{
			if ( !check_rectangle_for_colour( ptr + int( pitch * ( iheight - region_breadth ) * hps ), value, iwidth * wps, pitch, region_breadth * hps ) )
				BOOST_FAIL( "Unexpected border color on bottom region for " << to_string( image->pf() ) );
		}

	}

}

void test_dimensions()
{
	for ( int i = 0; i < pixel_formats.size(); i++ )
	{
		input_type_ptr in = create_input_with_params( pixel_formats[i].c_str() );
		BOOST_REQUIRE( in );
		
		int in_width = 137;
		int in_height = 791;
		int in_sar_num = 3;
		int in_sar_den = 5;

		// connect to the filter
		filter_type_ptr swscale_filter = create_filter( L"swscale" );
		BOOST_REQUIRE( swscale_filter );
		swscale_filter->property( "width" ) = in_width;
		swscale_filter->property( "height" ) = in_height;
		swscale_filter->property( "sar_num" ) = in_sar_num;
		swscale_filter->property( "sar_den" ) = in_sar_den;

		BOOST_REQUIRE( swscale_filter->connect( in ) );
		swscale_filter->sync();
		input_type_ptr out = swscale_filter;

		frame_type_ptr frame = out->fetch( 0 );
		image_type_ptr image = frame->get_image();

		int width = image->width();
		int height = image->height();
		int sar_num = 0, sar_den = 0;
		frame->get_sar( sar_num, sar_den );	
		std::wstring pf = image->pf();

		int expected_width_increase = 0;
		int expected_height_increase = 0;

		if ( pf.length( ) > 4 )
		{
			if ( ( pf.substr( 0, 3 ) == L"yuv" || pf.substr( 0, 3 ) == L"uyv" ) && pf.substr( 3, 3 ) != L"444" )
			{
				expected_width_increase = in_width % 2 != 0 ? 2 - ( in_width % 2 ) : 0;

				if ( pf.substr( 3, 3 ) == L"411" )
					expected_width_increase = in_width % 4 != 0 ? 4 - ( in_width % 4 ) : 0;
				if ( pf.substr( 3, 3 ) == L"420" )
					expected_height_increase = in_height % 2; 
			}
		}

		int expected_width = in_width + expected_width_increase;
		int expected_height = in_height + expected_height_increase;

		if ( width != expected_width )
			BOOST_FAIL( "width check failed for " << to_string( image->pf() ) );
		if ( height != expected_height )		
			BOOST_FAIL( "height check failed for " << to_string( image->pf() ) );
		if ( sar_num != in_sar_num )
			BOOST_FAIL( L"sar_num check failed for " << to_string( image->pf() ) );
		if ( sar_den != in_sar_den )
			BOOST_FAIL( L"sar_den check failed for " << to_string( image->pf() ) );
	}
}

void test_pillarboxing()
{
	for ( int i = 0; i < pixel_formats.size(); i++ )
	{
		input_type_ptr in = create_input_with_params( pixel_formats[i].c_str() );
		BOOST_REQUIRE( in );

		// connect to the filter
		filter_type_ptr swscale_filter = create_filter( L"swscale" );
		BOOST_REQUIRE( swscale_filter );
		swscale_filter->property( "width" ) = 400;
		swscale_filter->property( "height" ) = 400;
		swscale_filter->property( "sar_num" ) = 2;
		swscale_filter->property( "sar_den" ) = 1;

		BOOST_REQUIRE( swscale_filter->connect( in ) );
		swscale_filter->sync();
		input_type_ptr out = swscale_filter;

		frame_type_ptr frame = out->fetch( 0 );
		image_type_ptr image = frame->get_image();

		test_black_region(image, LEFT, 100);
		test_black_region(image, RIGHT, 100);
	}
}

void test_letterboxing()
{	
	for ( int i = 0; i < pixel_formats.size(); i++ )
	{
		input_type_ptr in = create_input_with_params( pixel_formats[i].c_str() );
		BOOST_REQUIRE( in );

		// connect to the filter
		filter_type_ptr swscale_filter = create_filter( L"swscale" );
		BOOST_REQUIRE( swscale_filter );
		swscale_filter->property( "width" ) = 400;
		swscale_filter->property( "height" ) = 400;
		swscale_filter->property( "sar_num" ) = 1;
		swscale_filter->property( "sar_den" ) = 2;

		BOOST_REQUIRE( swscale_filter->connect( in ) );
		swscale_filter->sync();
		input_type_ptr out = swscale_filter;

		frame_type_ptr frame = out->fetch( 0 );
		image_type_ptr image = frame->get_image();

		test_black_region(image, TOP, 100);
		test_black_region(image, BOTTOM, 100);
	}
}

BOOST_AUTO_TEST_CASE( swscale_filter )
{
	intialize_test_environment();
	test_dimensions();
	test_pillarboxing();
	test_letterboxing();

	// TODO: 
	// only tests for some specific values now. should test for some interesting/boundary cases?
	// does not test the aspect ratio calculations?
	// does not test the pf param of the filter.
	// the color of the letter/pillar box is not checked correctly for non-planar yuv formats (to make the test pass because the filter itself does it wrong now? uses y=0, u=0, v=0 instead of the correct values for black)
	// test combinations of dimension change + aspect ratio change + pf change ?
}

BOOST_AUTO_TEST_SUITE_END()
