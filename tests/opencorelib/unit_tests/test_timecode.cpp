#include "precompiled_headers.hpp"
#include <boost/test/auto_unit_test.hpp>
#include "opencorelib/cl/media_time.hpp"
#include <boost/foreach.hpp>

using namespace olib;
using namespace olib::opencorelib;
using boost::uint32_t;

BOOST_AUTO_TEST_SUITE( test_timecode )

struct tc_test
{
	frame_rate::type fps;
	bool drop_frame;
};

const tc_test TC_TESTS[7] = {
	{ frame_rate::movie,   false },
	{ frame_rate::pal,     false },
	{ frame_rate::pal_50,  false },
	{ frame_rate::ntsc,    false },
	{ frame_rate::ntsc,    true  },
	{ frame_rate::ntsc_60, false },
	{ frame_rate::ntsc_60, true  }
};

//Loop through all the frame numbers in a 24 hour span, and test that
//conversion to and from timecode is correct.
void test_framecount_tc_conversion_helper( frame_rate::type fps, bool drop_frame )
{
	const std::string fps_str = str_util::to_string( frame_rate::to_string( fps ) );

	const uint32_t frames_per_drop = drop_frame ? ( fps == frame_rate::ntsc ? 2 : 4 ) : 0;
	const uint32_t tc_rate = time_code::get_tc_rate_for_framerate( fps );
	const uint32_t frames_in_a_day = time_code::get_tc_count_in_24_hours( fps, drop_frame );

	int ref_tc_hour = 0;
	int ref_tc_min = 0;
	int ref_tc_sec = 0;
	int ref_tc_frame = 0;

	for( uint32_t reference_frame = 0; reference_frame < frames_in_a_day; ++reference_frame )
	{
		time_code ref_tc( ref_tc_hour, ref_tc_min, ref_tc_sec, ref_tc_frame, drop_frame );

		//Check that the timecode that we calculate from the frame
		//number is equal to the reference TC.
		const time_code tc =
			from_frame_number( fps, reference_frame ).to_time_code( fps, drop_frame );
		if( tc != ref_tc )
		{
			BOOST_REQUIRE_MESSAGE( tc == ref_tc,
				"Failure at frame " << reference_frame << " for TC rate " << fps_str );
		}

		//Check that the frame number that we calculate from the reference TC is equal to the
		//reference frame number.
		const boost::int32_t frame = from_time_code( fps, ref_tc ).to_frame_nr( fps );
		if( frame != reference_frame )
		{
			BOOST_REQUIRE_MESSAGE( frame == reference_frame,
				"Failure at frame " << reference_frame << " for TC rate " << fps_str );
		}

		//Advance the reference TC
		++ref_tc_frame;
		if( ref_tc_frame == tc_rate )
		{
			ref_tc_frame = 0;
			++ref_tc_sec;
			if( ref_tc_sec == 60 )
			{
				ref_tc_sec = 0;
				++ref_tc_min;

				//Drop 2 frames every minute, except every 10th
				if( ref_tc_min % 10 > 0 )
					ref_tc_frame = frames_per_drop;

				if( ref_tc_min == 60 )
				{
					ref_tc_min = 0;
					ref_tc_hour++;
				}
			}
		}
	}

	//This implicitly tests the get_tc_count_in_24_hours() method as well
	BOOST_REQUIRE_EQUAL( ref_tc_frame, 0 );
	BOOST_REQUIRE_EQUAL( ref_tc_sec, 0 );
	BOOST_REQUIRE_EQUAL( ref_tc_min, 0 );
	BOOST_REQUIRE_EQUAL( ref_tc_hour, 24 );
}

BOOST_AUTO_TEST_CASE( test_framecount_tc_conversion )
{
	BOOST_FOREACH( const tc_test &test, TC_TESTS )
	{
		test_framecount_tc_conversion_helper( test.fps, test.drop_frame );
	}
}

BOOST_AUTO_TEST_CASE( test_tc_to_string )
{
	time_code tc( 12, 34, 56, 24, false );
	BOOST_CHECK( tc.to_string() == _CT("12:34:56:24") );
	t_stringstream ss;
	ss << tc;
	BOOST_CHECK( ss.str() == _CT("12:34:56:24") );

	//Test that a drop-frame timecode introduces a semicolon
	time_code tc_df( 12, 34, 56, 24, true );
	BOOST_CHECK( tc_df.to_string() == _CT("12:34:56;24") );
	time_code tc_ndf( 12, 34, 56, 24, false );
	BOOST_CHECK( tc_ndf.to_string() == _CT("12:34:56:24") );
}

BOOST_AUTO_TEST_CASE( test_string_to_tc )
{
	//Semicolon indicates drop frame
	time_code tc_df(_CT("12:34:56;24"));
	BOOST_CHECK_EQUAL( tc_df.get_uses_drop_frame(), true );
	BOOST_CHECK_EQUAL( tc_df.get_hours(), 12 );
	BOOST_CHECK_EQUAL( tc_df.get_minutes(), 34 );
	BOOST_CHECK_EQUAL( tc_df.get_seconds(), 56 );
	BOOST_CHECK_EQUAL( tc_df.get_frames(), 24 );

	//Colon indicates no drop frame
	time_code tc_ndf(_CT("12:34:56:24"));
	BOOST_CHECK_EQUAL( tc_ndf.get_uses_drop_frame(), false );
	BOOST_CHECK_EQUAL( tc_ndf.get_hours(), 12 );
	BOOST_CHECK_EQUAL( tc_ndf.get_minutes(), 34 );
	BOOST_CHECK_EQUAL( tc_ndf.get_seconds(), 56 );
	BOOST_CHECK_EQUAL( tc_ndf.get_frames(), 24 );
}

BOOST_AUTO_TEST_CASE( test_media_time_division )
{
	media_time mt( rational_time( 20 ) );
	mt /= media_time( 2 );
	BOOST_CHECK( mt.to_time_code( frame_rate::pal, false ) == time_code( 0, 0, 10, 0, false ) );
	
	mt = media_time::zero() - media_time( 5 );
	BOOST_CHECK( mt == media_time(-5) );
	
	BOOST_CHECK_THROW( mt.to_time_code( frame_rate::pal, false ), olib::opencorelib::base_exception );
}

void test_tc_midnight_wrap_around_helper( frame_rate::type fps, bool drop_frame )
{
	const uint32_t frames_in_a_day = time_code::get_tc_count_in_24_hours( fps, drop_frame );
	media_time mt( from_frame_number( fps, frames_in_a_day ) );
	try
	{
		mt.to_time_code( fps, drop_frame );
		BOOST_FAIL( "Expected exception to be thrown when converting frame count"
			<< frames_in_a_day << " to TC without midnight wrap-around." );
	}
	catch( const base_exception &exc )
	{
		//Check that the exception message properly mentions the invalid frame number
		const std::string frames_str = boost::lexical_cast< std::string >( frames_in_a_day );
		const std::string exception_msg = exc.what();
		BOOST_CHECK( exception_msg.find( frames_str ) != std::string::npos );
	}

	//Do the same conversion to time code, but with the wrap_around_at_24h flag set. This should succeed.
	BOOST_CHECK( mt.to_time_code( fps, drop_frame, true ) == time_code( 0, 0, 0, 0, drop_frame ) );

	//Check that midnight wrap around works for more than one day as well
	const uint32_t frames_in_two_days = 2 * frames_in_a_day;
	media_time two_days_mt( from_frame_number( fps, frames_in_two_days ) );
	BOOST_CHECK( two_days_mt.to_time_code( fps, drop_frame, true ) == time_code( 0, 0, 0, 0, drop_frame ) );
}

BOOST_AUTO_TEST_CASE( test_tc_midnight_wrap_around )
{
	BOOST_FOREACH( const tc_test &test, TC_TESTS )
	{
		test_tc_midnight_wrap_around_helper( test.fps, test.drop_frame );
	}
}

BOOST_AUTO_TEST_SUITE_END()
