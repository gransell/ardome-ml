#ifndef _CORE_MESSAGE_QUEUE_H_
#define _CORE_MESSAGE_QUEUE_H_

#include <queue>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition.hpp>

namespace olib
{
   namespace opencorelib
    {
        /// Not currently used by amf
        class message_response
        {
        public:
            virtual ~message_response() {}
        };

        /// Not currently used by amf.
        class CORE_API message
        {
        public:
            message() : m_message_processed(false) {}
            virtual ~message() {}
            
            virtual boost::shared_ptr<message_response> dispatch() = 0;
            
            void set_response( boost::shared_ptr<message_response> res ) { m_message_res = res ; }
            boost::shared_ptr<message_response> get_response() const { return m_message_res; }

            bool get_message_processed() const { return m_message_processed; }
            void set_message_processed( bool v ) { m_message_processed = v; }
        private:
            boost::shared_ptr<message_response> m_message_res;
            bool m_message_processed;
        };

        typedef boost::function< void ( boost::shared_ptr<message> ) > post_callback;

        /** A message queue is serviced by one thread and could be used other 
            threads to communicate with the first thread.
            Not currently used by amf.
            @author Mats Lindel&ouml;f */
        class CORE_API message_queue
        {
        public:
            virtual ~message_queue() {}
            
            /// Push a message on the queue and block until it is serviced by the servant thread.
            virtual boost::shared_ptr<message_response> push_and_wait( boost::shared_ptr<message> ) = 0;

            /// Push a message on the queue and return immediately.
            /** No notification when done is issued. */
            virtual void push( boost::shared_ptr<message> msg ) = 0;

            virtual bool dispatch_next_message() = 0;

        };

        /// Not currently used by amf
        class CORE_API basic_message_queue : public message_queue, public boost::noncopyable
        {
        public:
        
            basic_message_queue();

            virtual boost::shared_ptr<message_response> push_and_wait( boost::shared_ptr<message> msg );

            /// Push a message on the queue and return immediately.
            /** The servicing thread will notify the caller via the 
                callback-function when the message has been serviced. */
            virtual void push( boost::shared_ptr<message> msg, post_callback pc );
            virtual void push( boost::shared_ptr<message> msg );

            virtual bool dispatch_next_message();
        
        private:
			boost::recursive_mutex m_queue_mtx;
            boost::condition_variable_any m_message_dispatched;

            typedef std::pair< boost::shared_ptr< message >, post_callback > message_pair;
            typedef std::queue< message_pair > message_container;
            message_container m_messages;

            void on_message_dispatched( boost::shared_ptr< message > );

        };
    }
}


#endif // _CORE_MESSAGE_QUEUE_H_

