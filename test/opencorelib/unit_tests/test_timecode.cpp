#include "precompiled_headers.hpp"
#include "opencorelib/cl/media_time.hpp"
#include <boost/test/test_tools.hpp>
#include <fstream>

using std::ifstream;

void test_timecode()
{
    using namespace olib;
    using namespace olib::opencorelib;

	//Loop through all the frame numbers in a 24 hour span, and test that
	//NTSC drop and non-drop frame timecode conversion is correct.
	int thd=0, tmd=0, tsd=0, tfd=0;
	int th=0, tm=0, ts=0, tf=0;
	time_code reference_drop_tc(0, 0, 0, 0, true);
	time_code reference_tc(0, 0, 0, 0, false);
	for( int reference_frame=0; reference_frame<2589408; ++reference_frame)
	{
		reference_drop_tc.set_frames(tfd);
		reference_drop_tc.set_seconds(tsd);
		reference_drop_tc.set_minutes(tmd);
		reference_drop_tc.set_hours(thd);

		reference_tc.set_frames(tf);
		reference_tc.set_seconds(ts);
		reference_tc.set_minutes(tm);
		reference_tc.set_hours(th);

		//Check that the timecode that we calculate from the frame number is equal to the
		//reference TC.
		time_code drop_tc = from_frame_number( frame_rate::ntsc, reference_frame ).to_time_code(frame_rate::ntsc, true);
		ARENFORCE( drop_tc == reference_drop_tc )( reference_frame )( drop_tc )( reference_drop_tc );

		time_code tc = from_frame_number( frame_rate::ntsc, reference_frame ).to_time_code(frame_rate::ntsc, false);
		ARENFORCE( tc == reference_tc )( reference_frame )( tc )( reference_tc );

		//Check that the frame number that we calculate from the reference TC is equal to the
		//reference frame number.
		int drop_fr = from_time_code(frame_rate::ntsc, reference_drop_tc).to_frame_nr(frame_rate::ntsc);
		ARENFORCE( drop_fr == reference_frame )( reference_frame )( drop_fr );

		int fr = from_time_code(frame_rate::ntsc, reference_tc).to_frame_nr(frame_rate::ntsc);
		ARENFORCE( fr == reference_frame )( reference_frame )( fr );

		//Advance the reference TCs
		tfd++;
		if(tfd == 30)
		{
			tfd = 0;
			tsd++;
			if( tsd == 60 )
			{
				tsd = 0;
				tmd++;

				//Drop 2 frames every minute, except every 10th
				if(tmd % 10!=0)
					tfd=2;

				if( tmd == 60 )
				{
					tmd = 0;
					thd++;
				}
			}
		}

		tf++;
		if(tf == 30)
		{
			tf = 0;
			ts++;
			if( ts == 60 )
			{
				ts = 0;
				tm++;

				if( tm == 60 )
				{
					tm = 0;
					th++;
				}
			}
		}

		//Progress update
		if( reference_frame % 100000 == 0 )
		{
			T_COUT << (reference_frame*100)/2589408 << _CT("%") << std::endl;
		}
	}

    media_time mt1( rational_time(20) );
    media_time mt2( rational_time(400, 7) );
    time_code tc1 = mt1.to_time_code( frame_rate::pal, false );
    time_code tc2 = mt2.to_time_code( frame_rate::pal, false );
    time_code tc1_ntsc_drop = mt1.to_time_code( frame_rate::ntsc, true );
    time_code tc1_ntsc_no_drop = mt1.to_time_code( frame_rate::ntsc, false );
    
    t_stringstream ss;
    ss << tc1;
    t_string s = ss.str();

    BOOST_CHECK( ss.str().compare(_CT("00:00:20:00")) == 0 );
    BOOST_CHECK( tc2 == time_code(0,0,57,3,false));

    //Make sure that we get a semicolon for NTSC drop frame timecodes
	//N.B. we will not have 00:00:20:00, since 20 seconds don't evenly
	//divide into NTSC frames.
    BOOST_CHECK( tc1_ntsc_drop.to_string() == _CT("00:00:19;29") );
    BOOST_CHECK( tc1_ntsc_no_drop.to_string() == _CT("00:00:19:29") );

    // Convert back to media time
    BOOST_CHECK( from_time_code( frame_rate::pal, tc2).to_time_code( frame_rate::pal, false ) == tc2 );
    BOOST_CHECK( mt1.to_frame_nr( frame_rate::pal ) == 500 );
    BOOST_CHECK( mt1.to_frame_nr( frame_rate::ntsc ) == 599 );

	// Check division
	media_time mt4( rational_time( 20 ) );
	mt4 /= media_time(2);
	BOOST_CHECK( mt4.to_time_code( frame_rate::pal, false ) == time_code( 0,0,10,0,false ) );
	
	mt4 = media_time::zero() - media_time( 5 );
	BOOST_CHECK( mt4 == media_time(-5) );
    
    BOOST_CHECK_THROW(mt4.to_time_code( frame_rate::pal, false ), olib::opencorelib::base_exception);

    media_time mt6 = from_frame_number( frame_rate::ntsc, 45 );
    olib::t_string mt6_str = mt6.to_time_code(frame_rate::ntsc, false).to_string();
}
