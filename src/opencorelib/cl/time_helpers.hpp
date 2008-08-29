#ifndef _CORE_TIME_HELPERS_H_
#define _CORE_TIME_HELPERS_H_

#include <iostream>
#include <iomanip>

#include <boost/operators.hpp>
#include <boost/thread/xtime.hpp>

#include "./core_enums.hpp"

namespace olib
{
   namespace opencorelib
    {
        /// Represents a duration of time, in seconds and micro seconds.
        class CORE_API time_value : public boost::addable< time_value >,
                           public boost::subtractable< time_value >,
                           public boost::equality_comparable< time_value >,
                           public boost::less_than_comparable< time_value >
        {
        public:
            time_value( ) : m_secs(0), m_micro_secs(0) {}

            time_value( boost::int64_t secs, boost::int64_t micro_secs )
                : m_secs(secs), m_micro_secs(micro_secs)
            {
            }

            explicit time_value(const boost::xtime& xt)
                : m_secs(xt.sec), m_micro_secs(xt.nsec/1000)
            {
            }
            
            operator boost::xtime () const; 
            
            /// Get the number of seconds.
            boost::int64_t get_seconds() const { return m_secs; }
            /// Set the number of seconds.
            void set_seconds( boost::int64_t t ) { m_secs = t; }

            /// Get the number of micro seconds.
            boost::int64_t get_micro_seconds() const { return m_micro_secs; }
            /// Set the number of micro seconds.
            void set_micro_seconds( boost::int64_t t ) { m_micro_secs = t; }

            /// Compares for equality
            CORE_API friend bool operator==(const time_value& lhs, const time_value& rhs);
            CORE_API friend bool operator<(const time_value& lhs, const time_value& rhs);

            /// Enable time calculations
            time_value operator+=(const time_value &tc);
            /// Enable time calculations
            time_value operator-=(const time_value &tc);

            static time_value now() {
                boost::xtime xt;
                boost::xtime_get(&xt, boost::TIME_UTC);
                return time_value(xt);
            }
            
        private:
            boost::int64_t m_secs;
            boost::int64_t m_micro_secs;
        };

        /**
         * Output a time_value to an ostream.  This should work both for both wide and narrow
         * ostreams in both wide and narrow environment.
         * \todo Maybe get the decimal separator from locale of stream, but that's overkill for now.
         */
        // Inline, since it's a template
        template<typename CharT, typename Traits>
        inline std::basic_ostream<CharT, Traits>& operator << (std::basic_ostream<CharT, Traits>& out,
                                                               const time_value& tv)
        {
            out << tv.get_seconds() << CharT('.')
                << std::setw(6) << std::setfill(CharT('0')) << tv.get_micro_seconds();
            return out;
        }
        
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
            time_value elapsed() const;

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
            /** @return A status flag signalling if the thread slept the specified time or not.*/ 
            thread_sleep::result current_thread_sleep( const time_value& val );

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
