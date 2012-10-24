#ifndef _CORE_WORKER_H_
#define _CORE_WORKER_H_

#include "typedefs.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition.hpp>
#include "event_handler.hpp"
#include "time_helpers.hpp"

namespace boost
{
	class thread;
}

#undef add_job

namespace olib
{
	namespace opencorelib
	{
        // Forward declaration.
        class base_job;
	
		/// A Worker provides one worker thread that can execute jobs.
		/** Each instance of a worker keeps a job heap. The jobs in 
			the heap are sorted and executed according to priority.
			Once a job has started it can not be interrupted. 

			@sa olib::opencorelib::thread_pool
			@author Mats Lindel&ouml;f*/
        class CORE_API worker : public boost::noncopyable
		{
		public:
			typedef olib::opencorelib::event_handler< worker *, base_job_ptr > job_done_event;

			/// Constructor
            /// @param name An optional name that will be set on the thread that this worker is running 
			worker( const t_string& name = _CT("Worker") );
			/// Calls stop and cleans up the internal state.
			virtual ~worker();

			/// Start the internal worker thread. 
			void start();

			/// Stop the worker thread, empties the job heap.
            void stop( boost::posix_time::time_duration time_out);

			/// Add a new job tho the job heap.
			/** The priority of the job should not be 
				altered after the insertion. */
			void add_job( const base_job_ptr& p_job ) ;

            /// Adds a job to the worker that will be performed every run_interval period.
            /** The job will be run as soon as possible (taking the priority of the
                job into account) and then at the interval provided in this call. */
            void add_reoccurring_job( const base_job_ptr& p_job, const boost::posix_time::time_duration& run_interval ) ;

            /// Removes a previously added job from the worker.
            void remove_reoccurring_job( const base_job_ptr& p_job );

            /// Adds a job to the worker that will start as close as possible to start_after_ms.
            void add_job( const base_job_ptr& p_job, const boost::posix_time::time_duration& start_after);

            /// Add a new job to the heap that will be given the highest priority.
            /** All existing jobs will be downgraded, this job will have prio 0 */
            void add_job_with_highest_priority( const base_job_ptr& job) ;

			/// Get size of the job heap.
			size_t job_count() const ;

			/// Cancel the currently running job, if any is running.
			/** The function sets the currently running job's 
				job_base::set_should_terminate_job to true and blocks
				until the job is cancelled. */
			void cancel_current_job() ;


            /// Same as cancel_current_job but the function blocks until the job is terminated
            /** @param time_out If the job hasn't terminated within time_out ms, the
                        function returns false.
                @return true if the job was canceled within the given timeout,
                        false otherwise. */
            bool cancel_current_job( const boost::posix_time::time_duration& time_out) ;

            /// Cancel a specific job in this worker's job queue. If the job has already been
            /// started, it will be marked for cancellation. If the job is a reocurring job, 
            /// it will not be run again.
            bool cancel_and_remove_job( const base_job_ptr &job_to_cancel, const boost::posix_time::time_duration& time_out );

            /// Remove all added jobs and cancel the currently executing job.
            /** Will block until the currently executing job is cancelled.
                This function is called automatically by the stop function. */
            void cancel_and_clear_jobs() ;

			/// Returns true if a job is currently running, false otherwise.
			bool get_is_running_job() const ;

            /// Will block until all currently added jobs have terminated.
            /** Additional jobs added after the call to 
                this function will not be taken into account.
                @param time_out If the jobs have not terminated before
                        the given timeout value (given in milli seconds)
                        the function will return false.
                @return true if all jobs were completed within the 
                        given time out value, false otherwise. */
            bool wait_for_all_jobs_completed( const boost::posix_time::time_duration& time_out );

			/// Event that's triggered when a job is completed
			/** Note that this event will block when used in the context of a thread pool. */
			job_done_event on_job_done;

		protected:

			/// Invoked (on worker thread) when thread is started
			/** Inherit from this class and override to do 
				some thread specific initialization. */
			virtual void on_thread_started();

			/// Invoked (on worker thread) when thread is about to stop
			/** Inherit from this class and override to do 
				some thread specific termination. */
			virtual void on_thread_stopping();

            /// Invoked when the last job has been removed from the head
			/** Inherit from this class and override to do 
				some thread specific termination. */
            virtual void on_empty_heap();

            /// The worker thread is idle. 
            /** Either sleep or pump the message loop. In windows
                you MUST do the later if you have created any HWND's.
                If you sleep, you must wake up when the passed condition
                is signaled. */
            virtual bool on_idle(	boost::condition_variable_any& wake_thread, 
                                    boost::recursive_mutex& wake_thread_mtx );

            /// Invoked when a new job is about to be executed
			/** Inherit from this class and override to do 
				some thread specific termination. */
            virtual void on_job_starting(boost::shared_ptr<base_job>);

			/// Helper class to compare two jobs.
			class job_compare_less
			{
			public:
				bool operator()(boost::shared_ptr<base_job> lhs, 
                    boost::shared_ptr<base_job> rhs );
			};

            /// Helper class to see if two jobs are equal
            class job_compare_equal
			{
			public:
				bool operator()(boost::shared_ptr<base_job> lhs, 
                    boost::shared_ptr<base_job> rhs );
			};
			
			typedef std::vector< base_job_ptr > jobcollection;

			/// The internal job heap.
			jobcollection m_heap;

            /// Jobs that should be added to the heap after a certain amount of time
            jobcollection m_timed_jobs;

            boost::condition_variable_any m_thread_exit_success, m_thread_started, m_wake_thread;
            mutable boost::recursive_mutex m_wake_thread_mtx;
            mutable boost::recursive_mutex m_thread_exit_mtx;

			/// Flag to keep track of if the worker is running a job_base or not.
			bool m_thread_running;

            /// Stop the worker thread.
            bool m_stop_thread;

			/// The currently running job. 
			boost::shared_ptr<base_job> m_current_job;

			/// The thread that runs all jobs in this worker.
            boost::shared_ptr< boost::thread > m_worker_thread;
		
			/// Member function, works as Thread_function above but we have access to >this<.
			void worker_thread_procedure();

			/// Runs until no more jobs are available.
			/** After finishing, the worker thread waits for
				new jobs to arrive. */
			void do_work();

            /// Will move timed jobs to the actual heap of jobs that will be performed.
            void move_timed_jobs_to_heap();

            boost::system_time next_timed_job_start_time() const;

			void reset_current_job();
			
			// Name to use for this worker. If supported it will be set on the thread
			t_string m_name;
		};
	}
}

#endif // _CORE_WORKER_H_

