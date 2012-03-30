#include "precompiled_headers.hpp"
#include "opencorelib/cl/span.hpp"
#include "opencorelib/cl/media_time.hpp"
#include "opencorelib/cl/frames.hpp"

using namespace olib::opencorelib;
#include <boost/test/test_tools.hpp>
#include <iostream>
#include <set>
#include <vector>
#include <ostream>

void test_span()
{
    time_span ts( media_time::zero(), media_time(10) );
    
    // 10 seconds should be 250 frames.
    BOOST_REQUIRE( from_time( ts.get_duration(), frame_rate::pal ) == 250 );

    frames f(250);
    BOOST_REQUIRE( to_time( f, frame_rate::pal ) == media_time(10));

    // 250 pal frames = 10 seconds = 299,97 ntsc frames (rounding down, always)
    BOOST_REQUIRE( convert( frames(250), frame_rate::pal, frame_rate::ntsc ) == 299 );
    BOOST_REQUIRE( convert( frames(2500), frame_rate::pal, frame_rate::ntsc ) == 2997 );
    BOOST_REQUIRE( convert( frames(25000), frame_rate::pal, frame_rate::ntsc ) == 29970 );
        
    // 250 frames corresponds to 10 seconds.
    frame_span fs( frames(0), frames(250) );
    BOOST_REQUIRE( to_time( fs.get_duration(), frame_rate::pal ) == media_time(10));

    frame_span fs2( frames(100), frames(300));

    // Test operator ==
    BOOST_REQUIRE( fs != fs2 );
	// Test operator <
	BOOST_REQUIRE( fs < fs2 );

    frame_span sp = span_intersection( fs, fs2) ;
	BOOST_REQUIRE( sp.get_in() == frames( 100 ) );
	BOOST_REQUIRE( sp.get_out() == frames( 250 ) );
	BOOST_REQUIRE( sp.get_duration() == frames( 150 ) );
	
	// We should only get one span
	std::vector< frame_span > union_result = span_union( fs, fs2 );
	BOOST_REQUIRE( union_result.size() == 1 );
	frame_span union_span = union_result[0];
	BOOST_REQUIRE( union_span.get_in() == frames( 0 ) );
	BOOST_REQUIRE( union_span.get_out() == frames( 400 ) );
	BOOST_REQUIRE( union_span.get_duration() == frames( 400 ) );
	
	std::vector< frame_span > sub_result = span_subtraction( fs, fs2 );
	BOOST_REQUIRE( sub_result.size() == 1 );
	frame_span sub_span = sub_result[0];
	BOOST_REQUIRE( sub_span.get_in() == frames(0) );
	BOOST_REQUIRE( sub_span.get_duration() == frames(100) );

    frames to_negate(50);
	
	frame_span fs3( frames(50), frames(10) );
	sub_result = span_subtraction( fs, fs3 );
	BOOST_REQUIRE( sub_result.size() == 2 );
	BOOST_REQUIRE( sub_result[0].get_in() == frames(0) );
	BOOST_REQUIRE( sub_result[1].get_in() == frames(60) );
	
	// Check that we can get two spans from union and make sure they are sorted when returned
	union_result = span_union( fs2, fs3 );
	BOOST_REQUIRE( union_result.size() == 2 );
	BOOST_REQUIRE( union_result[0].get_in() == frames(50) );
	BOOST_REQUIRE( union_result[1].get_in() == frames(100) );
	
	time_span trim_test( media_time(10), media_time(20) );
	// Extend the time span to the left
    trim_test = trim( trim_test, media_time(-5), side::left );
	BOOST_REQUIRE( trim_test.get_duration() == media_time(25) );
	
	// Extend the time span to the right
    trim_test = trim( trim_test, media_time(10), side::right );
	BOOST_REQUIRE( trim_test.get_duration() == media_time(35) );
	BOOST_REQUIRE( trim_test.get_in() == media_time(5) );
	
	frame_span t1( frames(0), frames(50) );
	frame_span t2( frames(40), frames(100) );
	frame_span t3 = span_subtraction( t1, t2 )[0];
	frame_span t4 = span_intersection( t1, t2 );
	frame_span t5 = span_subtraction( t2, t1 )[0];
	
	frame_span t4_t5_union = span_union( t4, t5 )[0];
	frame_span t3_t4_t5_union = span_union( t3, t4_t5_union )[0];
	frame_span t1_t2_union = span_union( t1, t2 )[0];
	BOOST_REQUIRE( t1_t2_union == t3_t4_t5_union  );
	
	
}
