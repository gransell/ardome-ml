#include "precompiled_headers.hpp"

#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/test/test_tools.hpp>

#include "opencorelib/cl/event_handler.hpp"

using namespace olib;
using namespace olib::opencorelib;

namespace
{
    boost::int32_t last_frame = 0;
    boost::int32_t last_frame_2 = 0;
}


class player :  public boost::enable_shared_from_this< player >, 
                public object
{
public:

    player() {}
    virtual ~player() {}

    event_handler< boost::shared_ptr< player >, boost::int32_t > on_new_frame;

    void set_current_frame( boost::int32_t fr )
    {
        on_new_frame( shared_from_this(), fr );
    }

    void other_player_changed_frame( const boost::shared_ptr< player >& other_player, const boost::int32_t& fr )
    {
        last_frame_2 = fr;
    }

private:

};

void player_callback_receiver( const boost::shared_ptr< player >&, boost::int32_t fr )
{
    last_frame = fr;
}


void test_event_handler()
{
    boost::shared_ptr< player > a_player( new player() );
    
    {
        event_connection_ptr conn = a_player->on_new_frame.connect( boost::bind(&player_callback_receiver,_1, _2) );
        a_player->set_current_frame(100);
        BOOST_CHECK_EQUAL( last_frame, 100 );
    }

    // Now the connection should be dead
    // The callback should not be called upon, since conn has gone out-of-scope.
    a_player->set_current_frame(10);
    BOOST_CHECK_EQUAL( last_frame, 100);

    {
        // Test automatic connection handling through the use of an object-derived class.
        // another_player will listen to what a_player is doing.
        boost::shared_ptr< player > another_player = boost::shared_ptr< player >( new player() );
        a_player->on_new_frame.connect( another_player, &player::other_player_changed_frame );
        a_player->set_current_frame(500);

        BOOST_CHECK_EQUAL( last_frame_2, 500);
    }

    // Now, another_player is dead, so last_frame_2 will no longer be updated.
    a_player->set_current_frame(3);
    BOOST_CHECK_EQUAL( last_frame_2, 500);


    
}
