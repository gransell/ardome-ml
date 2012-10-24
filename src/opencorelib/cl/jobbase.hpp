#ifndef _CORE_JOBBASE_H_
#define _CORE_JOBBASE_H_

#include <boost/tuple/tuple.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/condition.hpp>
#include "./event_handler.hpp"
#include "./time_helpers.hpp"
#include "./typedefs.hpp"

namespace olib
{
   namespace opencorelib
    {
        // Forward declaration
        class base_exception;

        /// A class that encapsulates the abstract notion of a job
        /** Jobs are carried out by worker objects. You create a job_base (must
            be a derived class since do_work is abstract) and assigns it to a 
            shared_ptr<job_base> which you add to a worker or a CThread_pool.
            If you keep the reference of the job you can wait for it to 
            complete via wait_for_job_done.

            Another way to wait for completion is to register a callback
            that will be notified when the job is done.

            Each job has a priority that will place it before lower prioritized
            jobs in a job queue. The priority is set in the constructor of the 
            job. 

            @sa worker
            @sa thread_pool
            @author Mats Lindelˆf */
        class CORE_API base_job : public boost::noncopyable,
                                    public boost::enable_shared_from_this< base_job >
        {
        public:
            /// Default constructor. 
            /** Set priority to 5 */
            base_job();
            /// Create a job and set its priority.
            base_job( boost::int32_t prio );
            /// Virtual destructor to ensure correct deletion.
            virtual ~base_job();
            
            /// Compare two jobs
            /** If lhs and rhs has equal priorities the job that was 
                created before the other has higher priority. 
                Strict ordering is ensured since all jobs has a unique
                order number.
                @param lhs The left-hand-side job.
                @param rhs The right-hand-side-job
                @return true if the left job has lower priority than the right
                        false if the opposite is true.*/
            CORE_API friend bool operator<(base_job& lhs, base_job& rhs);

            CORE_API friend bool operator==(base_job& lhs, base_job& rhs);

            /// Do the work you want done by the worker thread by overriding this function.
            /** @return This result can be retrieved when the job is done
                        via the Result member.*/
            virtual int do_work() = 0;

            /// Compare this job with another
            /** Override to make your own comparison if you do not want two
            jobs of the same type in the worker queue. The default implementation
            always returns false. */
            virtual bool compare(const base_job& job) const;

            /// Signals if this job should be terminated of not.
            /** Threads running the do_work function should check this
            member frequently and terminate as soon as possible 
            when the answer is 'true' */
            bool get_should_terminate_job() const;

            /// Wait for this job to terminate
            /** @param time_out The time to wait (in milli seconds) for the job to complete.
                @param activity The activity that should be performed by the calling thread during the wait.
                @return false if the wait timed out. true if the job was done
                    before the timeout value. */
            bool wait_for_job_done( const boost::posix_time::time_duration& time_out, 
                                    thread_sleep_activity::type activity = thread_sleep_activity::block );

            /// The int parameter is just a dummy.
            typedef event_handler< base_job_ptr, boost::int32_t > jobdone_signal;

            /// Register a callback that will be invoked once the job is done.
            /** @return The connection together with a bool saying if 
                        the job already was done or not. This is necessary
                        since the job can become completed just before this
                        function is called. */
            boost::tuples::tuple< event_connection_ptr, bool > 
            on_job_done( jobdone_signal::callback_signature listener );

            /// Is set to true when the job is done
            bool get_is_job_done() const;

            /// Get the result from the do_work function.
            int get_result() const { return m_result; }

            /// If do_work threw an exception, this will be true, otherwise false.
            bool get_exception_thrown() const { return m_exception_thrown; }

            /// If do_work threw a CBase_exception, is can be retrieved here.
            boost::shared_ptr< base_exception > get_base_exception() const { return m_base_exception; }

            /// If do_work threw a std::exception, is can be retrieved here.
            boost::shared_ptr< std::exception > get_std_exception() const { return m_std_exception; }

            void set_priority( boost::int32_t prio ){ m_prio = prio; }

            boost::int32_t get_priority() const { return m_prio; }

			/// Indicates if a reoccuring job should be rescheduled or not
			bool get_should_reschedule( ) const { return m_reschedule; }

			/// Modifier to indicate if a reoccuring job should be rescheduled or not
			void set_should_reschedule( const bool value ) { m_reschedule = value; }

        protected:
            // The priority of the job
            boost::int32_t m_prio;
            // The order number of the job. Unique to all jobs.
            long m_dw_number;
            // Counter used to created unique order numbers.
            static long s_dw_counter;

        private:
            /// Signals to a thread that is running a job that it should terminate.
            /** Termination will not happen automatically.
            If the thread running the job isn't polling get_should_terminate_job
            in a frequent manner, termination will fail. */
            void set_should_terminate_job( bool term );

            // Should the job terminate or not
            volatile bool m_terminate_job;

            // Makes it possible to add some bookkeeping code around the real job function do_work.
            void do_work_bootstrapper();

            mutable boost::recursive_mutex m_signal_mtx;
			mutable boost::recursive_mutex m_job_done_mtx;
			
			boost::condition_variable_any m_job_done_condition;
            bool m_job_done, m_exception_thrown, m_run_more_than_once;
            boost::int32_t m_result;
            boost::system_time m_next_time_to_run;
            boost::posix_time::time_duration m_reoccuring_interval;

            boost::shared_ptr< std::exception > m_std_exception;
            boost::shared_ptr< base_exception > m_base_exception;
            thread_sleeper_ptr m_sleeper;

            /// Signal that the job is done. 
            /** Will wake any thread locked in wait_for_job_done. */
            void signal_job_done();

            jobdone_signal m_job_done_signal;

			boost::shared_ptr< boost::recursive_mutex::scoped_lock >
			prevent_job_from_terminating();
            
			bool m_reschedule;

            friend class worker;
        };
    }
}

#endif //_CORE_JOBBASE_H_

