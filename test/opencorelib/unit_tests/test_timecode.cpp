#include "precompiled_headers.hpp"
#include "opencorelib/cl/media_time.hpp"
#include <boost/test/test_tools.hpp>
void test_timecode()
{
    using namespace olib;
    using namespace olib::opencorelib;

    media_time mt1( rational_time(20) );
    media_time mt2( rational_time(400, 7) );
    time_code tc1 = mt1.to_time_code( frame_rate::pal );
    time_code tc2 = mt2.to_time_code( frame_rate::pal );
    
    t_stringstream ss;
    ss << tc1;
    t_string s = ss.str();

    BOOST_CHECK( ss.str().compare(_CT("00:00:20:00")) == 0 );
    BOOST_CHECK( tc2 == time_code(0,0,57,3));

    // Convert back to media time
    BOOST_CHECK( from_time_code( frame_rate::pal, tc2).to_time_code( frame_rate::pal ) == tc2 );
    BOOST_CHECK( mt1.to_frame_nr( frame_rate::pal ) == 500 );
    BOOST_CHECK( mt1.to_frame_nr( frame_rate::ntsc_drop_frame ) == 599 );
    BOOST_CHECK( mt1.to_frame_nr( frame_rate::ntsc ) == 600 );

    media_time mt3( rational_time(3600) );

    BOOST_CHECK( mt3.to_time_code( frame_rate::ntsc ) == time_code(1,0,0,0)); 
    BOOST_CHECK( mt3.to_time_code( frame_rate::ntsc_drop_frame) == time_code(1,0,3,18)); 

    BOOST_CHECK( from_time_code( frame_rate::ntsc, time_code(1,0,0,0))
                    .to_time_code(frame_rate::ntsc_drop_frame) == time_code(1,0,3,18));


    // Check the drop frame mechanism around the first minute...
    time_code diff(0,0,59,29);
    BOOST_CHECK( from_time_code( frame_rate::ntsc_drop_frame, diff )
                    .to_time_code(frame_rate::ntsc_drop_frame) == diff );

    media_time mdiff = from_time_code(frame_rate::ntsc, diff);
    // Just make sure we don't get an assert here, when we create the time code.
    (mdiff + media_time( rational_time(30000,1001))).to_time_code(frame_rate::ntsc_drop_frame);

    // Drop over to 00:01:00:00
    mdiff.add_frames(frame_rate::ntsc_drop_frame, 1);

    // This should be displayed as 00:01:00:02 in NTSC with drop frame.
    BOOST_CHECK( mdiff.to_time_code(frame_rate::ntsc_drop_frame) == time_code(0,1,0,2));

    BOOST_CHECK(from_time_code( frame_rate::ntsc_drop_frame, time_code(0,1,0,2))
                    .to_time_code(frame_rate::ntsc_drop_frame) == time_code(0,1,0,2));

    BOOST_CHECK( from_time_code( frame_rate::ntsc_drop_frame, time_code(0,1,0,2))
                    .to_time_code(frame_rate::ntsc) == time_code(0,1,0,0));

	// Check division
	media_time mt4( rational_time( 20 ) );
	mt4 /= media_time(2);
	BOOST_CHECK( mt4.to_time_code( frame_rate::pal ) == time_code( 0,0,10,0 ) );
	
	mt4 = media_time::zero() - media_time( 5 );
	BOOST_CHECK( mt4 == media_time(-5) );
    
    BOOST_CHECK_THROW(mt4.to_time_code( frame_rate::pal ), olib::opencorelib::base_exception);
    
    // Make sure that we take into account the drift when using ntsc
    media_time mt5 = from_frame_number( frame_rate::ntsc, 3630 );
    BOOST_CHECK( mt5.to_time_code( frame_rate::pal ) == time_code( 0, 2, 1, 0 ) );
}
