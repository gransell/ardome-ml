#include "precompiled_headers.hpp"
#include "message_queue.hpp"
#include <boost/bind.hpp>

namespace olib
{
   namespace opencorelib
    {
        basic_message_queue::basic_message_queue()
        {
        }

        boost::shared_ptr<message_response> 
        basic_message_queue::push_and_wait( boost::shared_ptr<message> new_msg )
        {
			//boost::unique_lock<boost::mutex> lck(m_queue_mtx);
			boost::recursive_mutex::scoped_lock lck( m_queue_mtx );
			post_callback cb( boost::bind(&basic_message_queue::on_message_dispatched, this, _1 ));
            boost::shared_ptr<message_response> empty_response;
            new_msg->set_response(empty_response);
            new_msg->set_message_processed(false);
            m_messages.push( std::make_pair(new_msg, cb));

            for(;;)
            {
                m_message_dispatched.wait(lck);
                if( new_msg->get_message_processed() ) break;
            }

            return new_msg->get_response();
        }

        void basic_message_queue::push( boost::shared_ptr<message> new_msg, post_callback pc )
        {
			
			boost::recursive_mutex::scoped_lock lck(m_queue_mtx);
            boost::shared_ptr<message_response> empty_result;
            new_msg->set_response(empty_result);
            new_msg->set_message_processed(false);
            m_messages.push( std::make_pair(new_msg, pc));
        }

        void basic_message_queue::push( boost::shared_ptr<message> new_msg )
        {
			boost::recursive_mutex::scoped_lock lck(m_queue_mtx);
            boost::shared_ptr<message_response> empty_result;
            new_msg->set_response(empty_result);
            new_msg->set_message_processed(false);
            m_messages.push( std::make_pair(new_msg, post_callback()));
        }

        bool basic_message_queue::dispatch_next_message()
        {
            message_pair to_dispatch;
            
            {
				boost::recursive_mutex::scoped_lock lck(m_queue_mtx);
                if( m_messages.empty() ) return false;
                to_dispatch = m_messages.back();
                m_messages.pop();
            }
            
            if( !to_dispatch.first ) return false;

            boost::shared_ptr<message_response> res = to_dispatch.first->dispatch();
            to_dispatch.first->set_response(res);
            to_dispatch.first->set_message_processed(true);
            if( to_dispatch.second ) to_dispatch.second(to_dispatch.first);

            return true;
        }

        void basic_message_queue::on_message_dispatched(  boost::shared_ptr< message > )
        {
            m_message_dispatched.notify_all();
        }
    }
}
