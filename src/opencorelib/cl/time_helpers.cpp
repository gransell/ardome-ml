#include "precompiled_headers.hpp"
#include "./time_helpers.hpp"
#include "./enforce_defines.hpp"
#include "log_defines.hpp"
#include "logger.hpp"

#include <boost/thread/condition.hpp>

namespace olib 
{ 
   namespace opencorelib 
    {      
        #ifndef OLIB_ON_WINDOWS

        class timer::impl
        {
        public:
            impl() : m_running(false), m_start_time(), m_duration()
            {
            }

            ~impl()
            {
            }

            void start() 
            {
                m_start_time = boost::get_system_time();
                m_running = true;
                m_duration = boost::posix_time::seconds(0);
            }

            void stop()
            {
                m_duration = elapsed();                
                m_running = false;
            }

            bool is_running() const { return m_running; }

            boost::posix_time::time_duration elapsed() const
            {
                if( !m_running ) return m_duration;
                return boost::get_system_time() - m_start_time;
            }

        private:
            bool m_running;
            boost::system_time m_start_time;
            boost::posix_time::time_duration m_duration;
        };

        class thread_sleeper::impl
        {
        public:
            impl() 
            {
               
            }

            ~impl()
            {
               
            }

            thread_sleep::result current_thread_sleep( const boost::posix_time::time_duration& val, thread_sleep_activity::type sleep_activty ) 
            {
                if( sleep_activty == thread_sleep_activity::handle_incomming_messages )
                {
                    ARLOG_ERR("This implementation does not take care of the handle_incomming_messages mode");
                }
                
                boost::recursive_mutex::scoped_lock lock( m_mtx );
                bool res = m_wait_condition.timed_wait(lock, boost::get_system_time() + val);
                if( !res ) return thread_sleep::interrupted;
                return thread_sleep::slept_full_time;
            }

            void wake_sleeping_thread()
            {
                boost::recursive_mutex::scoped_lock lock( m_mtx );
                m_wait_condition.notify_one();
            }

        private:
            boost::condition_variable_any m_wait_condition;
            boost::recursive_mutex m_mtx;
            
        };

        #else // OLIB_ON_WINDOWS

        class timer::impl
        {
        public:
            impl() : m_start_time(0), m_running(false)
            {
            }

            ~impl()
            {

            }

            void start() 
            {
                m_running = true;
                m_duration = boost::posix_time::seconds(0);
                ARENFORCE_WIN_MSG( timeBeginPeriod(1) == TIMERR_NOERROR, "Could not set timer resolution to 5 ms" );
                m_start_time = timeGetTime();
                timeEndPeriod(1);
            }

            void stop()
            {
                m_duration = elapsed();
                m_running = false;
            }

            bool is_running() const { return m_running; }

            boost::posix_time::time_duration elapsed() const
            {
                if(!m_running) return m_duration;
                
                ARENFORCE_WIN_MSG( timeBeginPeriod(1) == TIMERR_NOERROR, "Could not set timer resolution to 5 ms");
                DWORD duration = timeGetTime() - m_start_time;
                timeEndPeriod(1);

                boost::posix_time::time_duration vt = boost::posix_time::milliseconds(duration);
                return vt;
            }

        private:
            bool m_running;
            DWORD m_start_time;
            boost::posix_time::time_duration m_duration;
        };


        class thread_sleeper::impl
        {
        public:
            impl()
                : m_semaphore( 0 )
            {
                create_semaphore();
            }

            ~impl()
            {
                destroy_semaphore();
            }

            void create_semaphore()
            {
                m_semaphore = ::CreateSemaphore(NULL, 0, 1, _CT("thread_sleeper win32 semaphore"));
            }

            void destroy_semaphore()
            {
                if(m_semaphore) CloseHandle(m_semaphore);
            }

            thread_sleep::result current_thread_sleep( const boost::posix_time::time_duration& val, thread_sleep_activity::type sleep_activty ) 
            {
                thread_sleep::result sleep_result = thread_sleep::slept_full_time;

                timer a_timer;
                a_timer.start();

                if( sleep_activty == thread_sleep_activity::block)
                {
                    ::WaitForSingleObject(m_semaphore, static_cast<DWORD>(val.total_milliseconds()));
                }
                else // thread_sleep_activity::handle_incomming_messages
                {
                    const HANDLE handles [] = { m_semaphore };
                    for(;;)
                    {
                        message_pump();
                        boost::posix_time::time_duration diff = val - a_timer.elapsed();
                        if( diff.is_negative() )
                            break;

                        DWORD waitResult = ::MsgWaitForMultipleObjects(1, handles ,FALSE, static_cast<DWORD>(diff.total_milliseconds()), QS_ALLEVENTS);
                        if( waitResult == WAIT_OBJECT_0 )
                        {
                            sleep_result = thread_sleep::interrupted;
                            break; 
                        }
                    }
                }
                
                return sleep_result;
            }

            void wake_sleeping_thread()
            {
                ::ReleaseSemaphore(m_semaphore, 1, NULL);
            }

        private:

            void message_pump()
            {
                MSG msg ; 
                while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
                { 
                    ::DispatchMessage(&msg); 
                }
            }

            HANDLE m_semaphore;
        };
        
        #endif // OLIB_ON_WINDOWS


        timer::timer() : m_pimpl( new impl())
        {

        }

        timer::~timer()
        {

        }

        void timer::start() 
        {
            m_pimpl->start();
        }

        void timer::stop()
        {
            m_pimpl->stop();
        }

        boost::posix_time::time_duration timer::elapsed() const
        {
            return m_pimpl->elapsed();
        }

        bool timer::is_running() const
        {
            return m_pimpl->is_running();

        }

        thread_sleeper::thread_sleeper() : m_pimpl(new impl())
        {
        }

        thread_sleeper::~thread_sleeper()
        {
        }

        thread_sleep::result thread_sleeper::current_thread_sleep( const boost::posix_time::time_duration& val,
                                                                    thread_sleep_activity::type sleep_activty ) 
        {
            return m_pimpl->current_thread_sleep( val, sleep_activty );
        }

        void thread_sleeper::wake_sleeping_thread()
        {
            m_pimpl->wake_sleeping_thread();
        }


        
    }
}
