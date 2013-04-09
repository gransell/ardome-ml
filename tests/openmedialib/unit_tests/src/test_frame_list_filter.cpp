#include <boost/test/unit_test.hpp>
#include <boost/assign.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/frame.hpp>
// #include <openmedialib/il/basic_image.hpp>
#include <opencorelib/cl/str_util.hpp>
#include <openmedialib/ml/filter.hpp>

using namespace olib::openmedialib::ml;
using namespace olib::openimagelib::il;
using namespace olib::opencorelib::str_util;


namespace
{
    const int gk_num_inputs = 8;
    std::vector<int> gk_frame_list = boost::assign::list_of(1)(3)(3)(7);

    int internal_create_redvalue_from_int(int i)
    {
	return ( ( 1 << i ) - 1 );
    }
}

BOOST_AUTO_TEST_SUITE( frame_frame_list_filter )

BOOST_AUTO_TEST_CASE( frame_frame_list_filter_case )
{
    // create playlist filter which merges the inputs
    filter_type_ptr l_playlist_filter = create_filter( L"playlist" );
    
    BOOST_REQUIRE( l_playlist_filter );

    l_playlist_filter->property( "slots" ) = gk_num_inputs;

    // create 8 color inputs which will create 1 frame of specified color
    std::vector<input_type_ptr> l_inputs;

    for (int i = 0; i < gk_num_inputs; ++i)
    {
	input_type_ptr l_color_input = create_delayed_input( L"colour:" );

	BOOST_REQUIRE( l_color_input );

	l_color_input->property( "colourspace" ) = std::wstring( L"r8g8b8" );
	l_color_input->property( "out" ) = 1;

	l_color_input->property( "r" ) = internal_create_redvalue_from_int( i );

	BOOST_REQUIRE( l_color_input->init() );

	l_inputs.push_back(l_color_input);

	BOOST_REQUIRE( l_playlist_filter->connect( l_color_input, i ) );

    }


    // create the frame_list filter
    filter_type_ptr l_frame_list_filter = create_filter( L"frame_list" );

    BOOST_REQUIRE( l_frame_list_filter );

    // configure it with the frame sequence
    l_frame_list_filter->property( "frames" ) = gk_frame_list;

    BOOST_REQUIRE( l_frame_list_filter->connect( l_playlist_filter ) );

    // not sure why this is needed (is it?)
    l_frame_list_filter->sync();

    // start grabbing the frames

    for(std::vector<int>::const_iterator i = gk_frame_list.begin(), 
	e = gk_frame_list.end(); i != e; ++i)
    {
	frame_type_ptr l_frame = l_frame_list_filter->fetch();
	BOOST_REQUIRE( l_frame );

	image_type_ptr l_image = l_frame->get_image();
	BOOST_REQUIRE( l_image );
	BOOST_CHECK_EQUAL( *(l_image->data( 0 ) ), internal_create_redvalue_from_int( *i ) );

	l_frame_list_filter->seek(1, true);


    }

}

BOOST_AUTO_TEST_SUITE_END()

