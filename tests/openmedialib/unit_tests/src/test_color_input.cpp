#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/image/image.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/openmedialib_plugin.hpp>
#include "utils.hpp"

namespace ml = olib::openmedialib::ml;
namespace cl = olib::opencorelib;
using ml::image::MLPixelFormat;
using cl::str_util::to_string;
using cl::str_util::to_wstring;

BOOST_AUTO_TEST_SUITE( color_input )

void test_pixel_format( const std::wstring &pf ) 
{
	const int r = 64;
	const int g = 128;
	const int b = 255;
	const int a = 20;

	ml::input_type_ptr color_input = ml::create_delayed_input( L"colour:" );
	BOOST_REQUIRE( color_input );
	color_input->property( "width" ) = 1920;
	color_input->property( "height" ) = 1080;
	color_input->property( "colourspace" ) = pf;
	color_input->property( "r" ) = r;
	color_input->property( "g" ) = g;
	color_input->property( "b" ) = b;
	color_input->property( "a" ) = a;
	BOOST_REQUIRE( color_input->init() );

	ml::frame_type_ptr frame = color_input->fetch( );	
	BOOST_REQUIRE( frame );

	BOOST_CHECK_MESSAGE( check_frame( frame, r, g, b, a, 0 ),
		"check_frame failed for pixel format " << to_string( pf ) );
}

BOOST_AUTO_TEST_CASE( create_colour_input_for_all_supported_pixelformats )
{
	std::map<olib::t_string, ml::image::MLPixelFormat>::const_iterator it;

	for( int i = 0; i < ml::image::ML_PIX_FMT_NB; ++i )
	{
		const MLPixelFormat pf = static_cast<MLPixelFormat>( i );
		if ( ml::image::is_pixfmt_planar( pf ) || ml::image::is_pixfmt_rgb( pf ) ) 
		{
			test_pixel_format( to_wstring( MLPF_to_string( pf ) ) );
		}
	}
}

BOOST_AUTO_TEST_SUITE_END()

