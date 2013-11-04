#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/image/image.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/openmedialib_plugin.hpp>

namespace ml = olib::openmedialib::ml;
namespace cl = olib::opencorelib;

BOOST_AUTO_TEST_SUITE( color_input )


template< typename T >
void check_yuv_image_data( ml::image_type_ptr image, int r, int g, int b )
{
	int y, u, v;
	ml::image::rgb24_to_yuv444( y, u, v, r, g, b );

	typename T::data_type y_shifted = static_cast< typename T::data_type >( y << ( image->bitdepth( ) - 8 ) );
	typename T::data_type u_shifted = static_cast< typename T::data_type >( u << ( image->bitdepth( ) - 8 ) );
	typename T::data_type v_shifted = static_cast< typename T::data_type >( v << ( image->bitdepth( ) - 8 ) );

	typename T::data_type planedata[3] = { y_shifted, u_shifted, v_shifted };

	for( int i = 0; i < 3; i++)
	{
		typename T::data_type *data = (typename T::data_type*)image->ptr( i );
		for (int height = image->height( i ); height > 0; height--) {
			for (int width = image->width( i ); width > 0; width--) {
				if (*data != planedata[ i ] )
				{
					BOOST_REQUIRE_EQUAL( *data, planedata[ i ] );
				}
				data++;
			}
			data += image->pitch( i ) / sizeof( typename T::data_type ) - image->linesize( i ) / sizeof( typename T::data_type );
		}
	}
}

template< typename T >
void check_rgb_image_data( ml::image_type_ptr image, int r, int g, int b )
{
	typename T::data_type rs = static_cast< typename T::data_type >( r << ( image->bitdepth( ) - 8 ) );
    typename T::data_type gs = static_cast< typename T::data_type >( g << ( image->bitdepth( ) - 8 ) );
    typename T::data_type bs = static_cast< typename T::data_type >( b << ( image->bitdepth( ) - 8 ) );

	typename T::data_type vals[4];

    // Determine order or R, G and B (RGB or BGR)
    int order_r = ml::image::order_of_component( image->ml_pixel_format( ), 0 ); // R
    int order_g = ml::image::order_of_component( image->ml_pixel_format( ), 1 ); // G
    int order_b = ml::image::order_of_component( image->ml_pixel_format( ), 2 ); // B

    // Check if pixfmt contains alpha channel
    int order_a = -1;
    if ( pixfmt_has_alpha( image->ml_pixel_format( ) ) ) {
        order_a = ml::image::order_of_component( image->ml_pixel_format( ), 3 ); // A
    }

    vals[order_r] = rs;
    vals[order_g] = gs;
    vals[order_b] = bs;	

	typename T::data_type *data = (typename T::data_type*)image->ptr( );
	for (int height = image->height( ); height > 0; height--) {
		for (int width = image->width( ); width > 0; width--) {

			int pos = 0;
			if (order_a == 0) {
				data++;
				pos++;
			}

			for (int i = pos; i < pos + 3; i++) {
				if ( *data ++ != vals[ i ] ) {
					BOOST_REQUIRE_MESSAGE( false, "Failed on component " << i << " for pixfmt " << cl::str_util::to_string( image->pf( ) ));
				}
			}

			if (order_a == 3) data++;
		}
	}
}

void test_colourspace( std::wstring colourspace ) 
{

	int r, g, b;
	r = 64;
	g = 128;
	b = 255;

	ml::input_type_ptr color_input = ml::create_delayed_input( L"colour:" );
	BOOST_REQUIRE( color_input );
	color_input->property( "width" ) = 1920;
	color_input->property( "height" ) = 1080;
	color_input->property( "colourspace" ) = colourspace;
	color_input->property( "r" ) = r;
	color_input->property( "g" ) = g;
	color_input->property( "b" ) = b;
	BOOST_REQUIRE( color_input->init() );

	ml::frame_type_ptr frame = color_input->fetch( );	
	BOOST_REQUIRE( frame );
	ml::image_type_ptr image = frame->get_image( );
	BOOST_REQUIRE( image );

	if (image->is_yuv_planar( ) ) {
		if( image->bitdepth( ) > 8 ) {
			check_yuv_image_data<ml::image::image_type_16>( image, r, g, b );
		} else {
			check_yuv_image_data<ml::image::image_type_8>( image, r, g, b );
		}
	} 
	else // RGB
	{
		if ( image->bitdepth( ) > 8 ) {
			check_rgb_image_data<ml::image::image_type_16>( image, r, g, b );
		} else {
			check_rgb_image_data<ml::image::image_type_8>( image, r, g, b );
		}
	}
}

BOOST_AUTO_TEST_CASE( create_colour_input_for_all_supported_pixelformats )
{
	std::map<olib::t_string, ml::image::MLPixelFormat>::const_iterator it;

	for (it = ml::image::MLPixelFormatMap.begin(); it != ml::image::MLPixelFormatMap.end(); ++it)
	{
		if ( ml::image::is_pixfmt_planar( it->second ) || ml::image::is_pixfmt_rgb( it->second ) ) 
		{
			test_colourspace( cl::str_util::to_wstring( it->first ) );
		}
	}
}

BOOST_AUTO_TEST_SUITE_END()

