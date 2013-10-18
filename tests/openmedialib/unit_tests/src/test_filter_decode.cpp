#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/filter.hpp>
#include "utils.hpp"
#include <deque>

using namespace olib::openmedialib::ml;

BOOST_AUTO_TEST_SUITE( decode_filter )

typedef std::deque< frame_type_ptr > frames_type;

void create_encoded_frames( boost::uint32_t a_num_frames, frames_type& a_out_frames, const std::wstring& a_profile )
{
	input_type_ptr color_input = create_delayed_input( L"colour:" );
	BOOST_REQUIRE( color_input );
	color_input->property( "width" ) = 1920;
	color_input->property( "height" ) = 1080;
	color_input->property( "interlace" ) = 0;
	color_input->property( "colourspace" ) = std::wstring( L"yuv422p" );
	color_input->property( "r" ) = 128;

	BOOST_REQUIRE( color_input->init() );

	filter_type_ptr encode_filter = create_filter( L"encode" );
	BOOST_REQUIRE( encode_filter );
	encode_filter->property( "profile" ) = a_profile;
	encode_filter->connect( color_input );
	encode_filter->sync();


	while ( a_num_frames-- )
	{
		frame_type_ptr frame = encode_filter->fetch();
		if ( 0 == frame ) // to reduce spam of BOOST_REQUIRE
		{
			BOOST_REQUIRE( frame );
		}


		// trigger the encode
		frame->get_stream( );

		// remove the image
		frame->set_image( olib::openmedialib::ml::image_type_ptr( ), true );

		a_out_frames.push_back( frame );

		encode_filter->seek( 1, true );

	}
}

boost::int32_t check_inner_threads_resolving( boost::int32_t a_threads )
{
	// create pusher input to feed frame with
	input_type_ptr inp = create_input( "pusher:" );
	BOOST_REQUIRE( inp );

	BOOST_REQUIRE( inp->init() );

	// create decode filter
	filter_type_ptr decode_filter = create_filter( L"decode" );
	BOOST_REQUIRE( decode_filter );
	decode_filter->property( "inner_threads" ) = a_threads;
	decode_filter->connect( inp );
	decode_filter->sync();

	// create encoded frame to push through decode filter
	frames_type frames;
	create_encoded_frames( 1, frames, std::wstring( L"vcodecs/mpeg2_iframe50" ) );

	// push a frame into input
	inp->push( frames.back() );

	// pull frame from decoder
	frame_type_ptr frame_out = decode_filter->fetch();
	BOOST_REQUIRE( frame_out );

	return decode_filter->property( "inner_threads" ).value< boost::int32_t >( );

}


BOOST_AUTO_TEST_CASE( test_inner_threads_property_resolving )
{
	int num_threads = boost::thread::hardware_concurrency();

	// check that sending in -1 resolves into number of threads
	BOOST_CHECK_EQUAL( num_threads, check_inner_threads_resolving( -1 ) );

	// check that sending in -BIG_NUMBER resolves into 1 (and not e.g. 0)
	BOOST_CHECK_EQUAL( 1, check_inner_threads_resolving( boost::integer_traits< boost::int32_t >::const_min ) );

	// check that 0 is preserved
	BOOST_CHECK_EQUAL( 0, check_inner_threads_resolving( 0 ) );

	// check that positive thread counts are preserved
	BOOST_CHECK_EQUAL( 1, check_inner_threads_resolving( 1 ) );
	BOOST_CHECK_EQUAL( boost::integer_traits< boost::int32_t >::const_max, check_inner_threads_resolving( boost::integer_traits< boost::int32_t >::const_max ) );
}


// boost::uint32_t time_decode( boost::uint32_t a_num_frames, boost::int32_t a_num_threads )
boost::uint32_t time_decode( frames_type& a_frames, boost::int32_t a_num_threads )
{
	// create pusher input to feed frame with
	input_type_ptr inp = create_input( "pusher:" );
	BOOST_REQUIRE( inp );

	// set length so get_frames returns the number of frames in the queue
	inp->property( "length" ) = -1;

	BOOST_REQUIRE( inp->init() );

	// create decode filter
	filter_type_ptr decode_filter = create_filter( L"decode" );
	BOOST_REQUIRE( decode_filter );
	decode_filter->property( "inner_threads" ) = a_num_threads;
	BOOST_REQUIRE( decode_filter->connect( inp ) );
	decode_filter->sync();


	for ( frames_type::iterator i = a_frames.begin( ), e = a_frames.end( ); i != e; ++i )
	{
		// make sure the frame doesn't have an image on it ( who knows what the decoder decides to do then... )
		(*i)->set_image( olib::openmedialib::ml::image_type_ptr( ), true ); // decoded = true to prevent frame class from removing the stream
		inp->push( *i );
	}

	BOOST_REQUIRE( decode_filter->get_frames( ) == a_frames.size( ) );

	// start the timer
	boost::posix_time::ptime start_time = boost::posix_time::second_clock::local_time();

	int position = 0;
	while ( position < a_frames.size( ) )
	{
		decode_filter->seek( position++ );
		frame_type_ptr frame_out = decode_filter->fetch( );
		frame_out->get_image( ); // trigger the decode

		if ( 0 == frame_out ) // spam prevention
			BOOST_REQUIRE( frame_out );
	}

	return (boost::posix_time::second_clock::local_time() - start_time).seconds();
}

BOOST_AUTO_TEST_CASE( test_inner_threads_performance_improvement )
{
	frames_type frames;
	create_encoded_frames( 5000, frames, std::wstring( L"vcodecs/mpeg2_iframe50" ) );

	boost::uint32_t time_1 = time_decode(frames, 1);

	boost::uint32_t time_auto = time_decode(frames, -1);

	// expect at least 50% improvement when running threaded
	// N.B. This is very dependent on the machine running the test!
	BOOST_CHECK( 1.5 < ( ( 1.0f * time_1 ) / time_auto ) );

}

BOOST_AUTO_TEST_SUITE_END()

