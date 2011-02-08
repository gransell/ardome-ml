#include "precompiled_headers.hpp"
#include "opencorelib/cl/media_time.hpp"
#include <boost/test/test_tools.hpp>
void test_timecode()
{
    using namespace olib;
    using namespace olib::opencorelib;

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
    BOOST_CHECK( tc1_ntsc_drop.to_string() == _CT("00:00:20;00") );
    BOOST_CHECK( tc1_ntsc_no_drop.to_string() == _CT("00:00:20:00") );

    // Convert back to media time
    BOOST_CHECK( from_time_code( frame_rate::pal, tc2).to_time_code( frame_rate::pal, false ) == tc2 );
    BOOST_CHECK( mt1.to_frame_nr( frame_rate::pal ) == 500 );
    BOOST_CHECK( mt1.to_frame_nr( frame_rate::ntsc ) == 599 );

    media_time mt3( rational_time(3600) );

    BOOST_CHECK( mt3.to_time_code( frame_rate::ntsc, false ) == time_code(1,0,0,0,false)); 
    BOOST_CHECK( mt3.to_time_code( frame_rate::ntsc, true) == time_code(1,0,3,18,true)); 

    BOOST_CHECK( from_time_code( frame_rate::ntsc, time_code(1,0,0,0,false))
                    .to_time_code(frame_rate::ntsc, true) == time_code(1,0,3,18,true));


    // Check the drop frame mechanism around the first minute...
    time_code diff(0,0,59,29,true);
    BOOST_CHECK( from_time_code( frame_rate::ntsc, diff )
                    .to_time_code(frame_rate::ntsc, true) == diff );

    diff.set_uses_drop_frame( false );
    media_time mdiff = from_time_code(frame_rate::ntsc, diff);
    // Just make sure we don't get an assert here, when we create the time code.
    (mdiff + media_time( rational_time(30000,1001))).to_time_code(frame_rate::ntsc, true);

    // Drop over to 00:01:00:00
    mdiff.add_frames(frame_rate::ntsc, 1);

    // This should be displayed as 00:01:00:02 in NTSC with drop frame.
    BOOST_CHECK( mdiff.to_time_code(frame_rate::ntsc, true) == time_code(0,1,0,2,true));

    BOOST_CHECK(from_time_code( frame_rate::ntsc, time_code(0,1,0,2,true))
                    .to_time_code(frame_rate::ntsc, true) == time_code(0,1,0,2,true));

    BOOST_CHECK( from_time_code( frame_rate::ntsc, time_code(0,1,0,2,true))
                    .to_time_code(frame_rate::ntsc, false) == time_code(0,1,0,0,false));

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
