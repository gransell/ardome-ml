#include "precompiled_headers.hpp"
#include "./time_helpers.hpp"
#include "./enforce_defines.hpp"
#include "log_defines.hpp"
#include "logger.hpp"

namespace olib 
{ 
   namespace opencorelib 
    {
        CORE_API bool operator==(const time_value& lhs, const time_value& rhs)
        {
            return lhs.m_micro_secs == rhs.m_micro_secs &&
                lhs.m_secs == rhs.m_secs;
        }

        CORE_API bool operator<(const time_value& lhs, const time_value& rhs)
        {
            if( lhs.m_secs < rhs.m_secs ) return true;
            if( lhs.m_secs > rhs.m_secs ) return false;
            
            // Seconds are equal, check micro-secs:
            if( lhs.m_micro_secs < rhs.m_micro_secs ) return true;
            if( lhs.m_micro_secs > rhs.m_micro_secs ) return false;

            // Values are equal 
            return false;
        }

        time_value::operator boost::xtime () const
        {
            boost::xtime result;
            result.sec = m_secs;
            result.nsec = static_cast<boost::xtime::xtime_nsec_t>( m_micro_secs * 1000 );
            return result;
        }

        time_value time_value::operator+=(const time_value &tc)
        {
            boost::int64_t mic_secs = m_micro_secs + tc.m_micro_secs;
            m_secs = tc.m_secs + m_secs + (mic_secs / 1000000);
            m_micro_secs = mic_secs % 1000000;
            return *this;
        }

        time_value time_value::operator-=(const time_value &tc)
        {
            m_micro_secs = m_micro_secs - tc.m_micro_secs;
            m_secs = m_secs - tc.m_secs;

            if(m_micro_secs < 0)
            {
                m_secs -= 1;
                m_micro_secs += 1000000;
            }

            return *this;
        }

        #ifndef OLIB_ON_WINDOWS

        class timer::impl
        {
        public:
            impl() : m_running(false), m_start_time(0,0), m_duration(0,0)
            {
            }

            ~impl()
            {
            }

            void start() 
            {
                m_start_time = time_value::now();
                m_running = true;
                m_duration = time_value(0,0);
            }

            void stop()
            {
                m_duration = elapsed();                
                m_running = false;
            }

            bool is_running() const { return m_running; }

            time_value elapsed() const
            {
                if( !m_running ) return m_duration;
                return time_value::now() - m_start_time;
            }

        private:
            bool m_running;
            time_value m_start_time, m_duration;
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

            thread_sleep::result current_thread_sleep( const time_value& val, thread_sleep_activity::type sleep_activty ) 
            {
                if( sleep_activty == thread_sleep_activity::handle_incomming_messages )
                {
                    ARLOG_ERR("This implementation does not take care of the handle_incomming_messages mode");
                }
                
                boost::recursive_mutex::scoped_lock lock( m_mtx );
                bool res = m_wait_condition.timed_wait(lock, time_value::now() + val);
                if( !res ) return thread_sleep::interrupted;
                return thread_sleep::slept_full_time;
            }

            void wake_sleeping_thread()
            {
                boost::recursive_mutex::scoped_lock lock( m_mtx );
                m_wait_condition.notify_one();
            }

        private:
            boost::condition m_wait_condition;
            boost::recursive_mutex m_mtx;
            
        };


        // Possible implementation of current_thread_sleep
        //struct timespec s_time;
        //struct timespec rem_time;

        //s_time.tv_sec = val.get_seconds();
        //s_time.tv_nsec = val.get_micro_seconds() * 1000;

        //for(;;)
        //{
        //    int res = nanosleep( &s_time, &rem_time );
        //    if( res == 0 || m_should_wake ) break;
        //    /// Check if we were interrupted, if not, something else is wrong. Bail out.
        //    if( res == -1 && errno != EINTR ) break;

        //    /// We were interrupted but we shouldn't care about it.
        //    // Continue to sleep the remaining time.
        //    s_time = rem_time;
        //}

        // Possible implementation of wake_sleeping_thread
        //m_should_wake = true;
        //// Wake up any nanosleeping threads
        //// They will check their m_should_wake flags and contiue to sleep
        //// if they weren't the intended target.
        //kill(0, SIGUSR1 );



        //typedef rdtsc_default_timer::value_type value_type;
        /*private:
        #if GCC_VERSION >= 40000 && !defined( OLIB_ON_MAC )
        typedef rdtsc_default_timer::value_type value_type;
        rdtsc_default_timer m_timer;
        #else
        typedef gettimeofday_default_timer::value_type value_type;
        gettimeofday_default_timer m_timer;
        #endif
        };*/


        #else // OLIB_ON_WINDOWS

        class timer::impl
        {
        public:
            impl() : m_start_time(0), m_duration(0,0), m_running(false)
            {
            }

            ~impl()
            {

            }

            void start() 
            {
                m_running = true;
                m_duration = time_value(0,0);
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

            time_value elapsed() const
            {
                if(!m_running) return m_duration;
                
                ARENFORCE_WIN_MSG( timeBeginPeriod(1) == TIMERR_NOERROR, "Could not set timer resolution to 5 ms");
                DWORD duration = timeGetTime() - m_start_time;
                timeEndPeriod(1);

                time_value vt(0,0);
                vt.set_seconds( duration / 1000);
                vt.set_micro_seconds(( duration - (vt.get_seconds() * 1000) )* 1000 );
                return vt;
            }

        private:
            bool m_running;
            DWORD m_start_time;
            time_value m_duration;
        };


        class thread_sleeper::impl
        {
        public:
            impl() 
            {
                m_event = ::CreateEvent(NULL, FALSE, FALSE, NULL );
            }

            ~impl()
            {
                if(m_event) CloseHandle(m_event);
            }

            thread_sleep::result current_thread_sleep( const time_value& val, thread_sleep_activity::type sleep_activty ) 
            {
                {
                    boost::recursive_mutex::scoped_lock lck(m_mtx);
                    DWORD to_wait = static_cast<DWORD>( val.get_seconds() * 1000 + val.get_micro_seconds() / 1000 );
                    if( to_wait <= 0 ) return thread_sleep::slept_full_time;
                    timeSetEvent( to_wait , 0, (LPTIMECALLBACK)m_event, 0, TIME_CALLBACK_EVENT_SET );
                }

                timer a_timer;
                a_timer.start();

                if( sleep_activty == thread_sleep_activity::block)
                {
                    ::WaitForSingleObject(m_event, INFINITE);
                }
                else // thread_sleep_activity::handle_incomming_messages
                {
                    const HANDLE handles [] = { m_event };
                    for(;;)
                    {
                        message_pump();
                        DWORD waitResult = ::MsgWaitForMultipleObjects(1, handles ,FALSE, INFINITE, QS_ALLEVENTS);
                        if( waitResult == WAIT_OBJECT_0 ) break; 
                    }
                }
                
                a_timer.stop();
                time_value small_epsilon(0,10000);
                if( a_timer.elapsed() >= ( val - small_epsilon )) return thread_sleep::slept_full_time;
                return thread_sleep::interrupted;
            }

            void wake_sleeping_thread()
            {
                boost::recursive_mutex::scoped_lock lck(m_mtx);
                SetEvent(m_event);
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

            HANDLE m_event;
            boost::recursive_mutex m_mtx;
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

        time_value timer::elapsed() const
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

        thread_sleep::result thread_sleeper::current_thread_sleep( const time_value& val,
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
