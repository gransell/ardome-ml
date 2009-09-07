#ifndef _CORE_TIME_HELPERS_H_
#define _CORE_TIME_HELPERS_H_

#include <iostream>
#include <iomanip>

#include <opencorelib/cl/minimal_string_defines.hpp>

#include <boost/operators.hpp>
#include <boost/thread/thread_time.hpp>

#include "./core_enums.hpp"

namespace olib
{
   namespace opencorelib
    {
        
        /// Class used to time execution.
        class CORE_API timer
        {
        public:
            timer();
            ~timer();
            /// Call start to start the timer.
            void start();
            /// Call stop to stop it.
            void stop();	

            /// Call elapsed to get the elapsed time between start and stop.
            boost::posix_time::time_duration elapsed() const;

            bool is_running() const;

        private:
            class impl;
            boost::shared_ptr<impl> m_pimpl;
        };

        /// Class used to put a thread in a hibernated state.
        /** Instead of just calling sleep, using this class enables another 
            thread to wake this one up. */
        class CORE_API thread_sleeper
        {
        public:
            thread_sleeper();
            ~thread_sleeper();

            /// Put the calling thread in a hibernated state for val time.
            /** @param val The maximum amount of time the calling thread is prepared to sleep.
                @param sleep_activity Should the calling thread just block or 
                        or pump its message loop (useful for gui-threads).
                @return A status flag signaling if the thread slept the specified time or not.*/ 
            thread_sleep::result current_thread_sleep(  const boost::posix_time::time_duration& val, 
                                                        thread_sleep_activity::type sleep_activty = thread_sleep_activity::block );

            /// If another thread is sleeping using current_thread_sleep on this object ...
            /** use this function to wake it up. */
            void wake_sleeping_thread();
        private:
            class impl;
            boost::shared_ptr<impl> m_pimpl;
        };
    }
}

#endif // #ifndef _CORE_TIME_HELPERS_H_
