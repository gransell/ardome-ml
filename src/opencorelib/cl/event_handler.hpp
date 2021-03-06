#ifndef _CORE_EVENT_H_
#define _CORE_EVENT_H_

#include "./typedefs.hpp"
#include "./object.hpp"

#include <boost/thread/recursive_mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/bind.hpp>

#include <list>

 
namespace olib
{
    namespace opencorelib
    {
        /// Simple class used to keep a connection alive.
        /** When a connection goes out-of-scope, the event_handler
            that created it will know and no longer call that callback
            <bindgen>
        		<attribute name="visibility" value="private"></attribute>
        	</bindgen>
        */
        class CORE_API event_connection : public object
        {
        public:
            event_connection() {}
            virtual ~event_connection() {}
        private:
        };

        typedef boost::shared_ptr< event_connection > event_connection_ptr;
        typedef boost::weak_ptr< event_connection > weak_event_connection_ptr;

        /// Provides thread-safe event_handler instances.
        /**
        <bindgen>
    		<attribute name="visibility" value="private"></attribute>
    	</bindgen>
    	*/
        class thread_locker
        {
        public:
            /**
            <bindgen>
        		<attribute name="visibility" value="private"></attribute>
        	</bindgen>
        	*/
            class lock
            {
            public:
                lock( const thread_locker& lck ) : m_lck( lck.m_mtx ){}
            private:
                boost::recursive_mutex::scoped_lock m_lck;
            };
        private:
             mutable boost::recursive_mutex m_mtx;
        };

        /// Use this class to get non-thread-safe event_handlers, pass it in as the ThreadLocker argument.
        /** If you know that your event_handle doesn't need thread-safety, 
            use this class as the template argument to ThreadLocker to get a speed increase
            <bindgen>
        		<attribute name="visibility" value="private"></attribute>
        	</bindgen>
        */
        class not_thread_safe
        {
        public:
            /**
            <bindgen>
        		<attribute name="visibility" value="private"></attribute>
        	</bindgen>
        	*/
            class lock
            {
            public:
                lock( const not_thread_safe& lck ) { }
            private:
            };
        };

        /// Thread safe alternative to boost::signals.
        /** Does not require any library files, just a simple header.
            Less generic than boost::signals since it always takes two
            parameters. If you want to pass more parameters, use the 
            second template argument and pass a class-instance there instead. 

            The third template parameter allows you to control if a mutex is going 
            to be locked every time someone access the event_handler. The default
            is that it will, and should normally be used. 

            @author Mats Lindel&ouml;f
            <bindgen>
        		<attribute name="visibility" value="private"></attribute>
        	</bindgen>
        */
        template < class Sender, class EventArgs, class ThreadLocker = thread_locker >
        class event_handler
        {
        public:
 
            /// To make boost::bind happy we need this typedef.
            typedef void result_type;
            
            typedef boost::function< void ( const Sender&, const EventArgs& ) > callback_signature;

            /// Connects a registrant derived from opencorelib::object to this event_handeler.
            /** When the registrant goes out of scope, the connection goes down automatically. 
                Use this version if you receiver derives from object, since you don't have to 
                handle an explicit event_connection object to keep the connection alive.
                The parameter T must derive from opencorelib::object. */
            template< class T >
            void connect( const boost::shared_ptr<T>& registrant, void (T::*CbFunc)(const Sender&, const EventArgs& )  )
            {
                typename ThreadLocker::lock lck( m_locker );
                // Don't save a shared_ptr in the function-object, since that will 
                // destroy the automatic connection management. We store a weak-ptr to 
                // the object and make sure that it is alive before we ever call on the bound object.
                m_callbacks.push_back( std::make_pair(registrant, 
                                boost::bind( CbFunc, registrant.get(), _1, _2 ) ));
            }

            /// Connects a generic callback to this event_handeler.
            /** When the returned object goes out of scope, the connection goes down automatically. */
            event_connection_ptr connect( callback_signature cb  )
            {
                typename ThreadLocker::lock lck( m_locker );
                event_connection_ptr conn( new event_connection() );
                m_callbacks.push_back( std::make_pair(conn, cb) );
                return conn;
            }

            /// Fires the event. 
            /** Will remove dead connections in a maintenance step.
                @param sender The sender of the event.
                @param args The argument provided by the sender.*/
            void operator()( const Sender& sender,  const EventArgs& args ) const
            {
                // Take a copy to avoid locking during the callback phase.
                callback_collection copy_of_callbacks;

                {
                    typename ThreadLocker::lock lck( m_locker );
                    // Clean up dead references:
                    typename callback_collection::iterator it( m_callbacks.begin()), eit(m_callbacks.end());
                    for( ; it != eit; ++it )
                    {
                        object_ptr is_alive = it->first.lock();
                        if(!is_alive) 
                        {
                            it = m_callbacks.erase(it);
							if( it == eit ) break;
                        }
                    }

                    copy_of_callbacks = m_callbacks;
                }

                // Iterate through the copy, this makes it possible to connect to this signal 
                // during the callback-phase.
                typename callback_collection::iterator it( copy_of_callbacks.begin()), eit(copy_of_callbacks.end());
                for( ; it != eit; ++it )
                {
                    object_ptr is_alive = it->first.lock();
                    if( is_alive ) 
                    {
                        it->second(sender, args);
                    }
                }
            }

        private:

            typedef std::pair< weak_object_ptr, callback_signature > registrant_callback_pair;
            typedef std::list< registrant_callback_pair > callback_collection;
            mutable callback_collection m_callbacks;
            ThreadLocker m_locker;
            // mutable boost::recursive_mutex m_mtx;

        };

    }
}

#endif // _CORE_EVENT_H_
