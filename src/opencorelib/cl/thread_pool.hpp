#ifndef _CORE_THREAD_POOL_H_
#define _CORE_THREAD_POOL_H_

#include <vector>
#include <deque>
#include <boost/bind.hpp>
#include <boost/tuple/tuple.hpp>
#include "worker.hpp"

namespace olib
{
	namespace opencorelib
	{
		/// A pool of workers that will be given as equal load as possible.
		/** Use the thread pool for jobs that take finite time 
			(for infinite jobs, use a single worker!). and when 
			you don't care exactly when they will be done.
			@author Mats Lindel&ouml;f */
        class CORE_API thread_pool : public boost::noncopyable
		{
		public:
			/// Create a new pool.
			/** @param max_nrOf_workers The maximum number of worker threads
					used to do the works added to the pool. 
				@param thread_termination_timeout The time out in milli seconds
					to wait for the worker threads to terminate. */
			thread_pool(   unsigned int max_nrOf_workers,  
                            const boost::posix_time::time_duration& thread_termination_timeout);

			/// Calls terminate_all_threads and dies.
			virtual ~thread_pool();

			/// Terminates all running workers.
            /** @param time_out This is the maximum wait time in 
                        milliseconds to wait for each thread in the pool
                        to terminate. */
            void terminate_all_threads(const boost::posix_time::time_duration& time_out) ;

			/// Add a job to the pool.
			virtual void add_job( boost::shared_ptr< base_job > p_job ) ;

			/// Get the maximum amount of workers in this pool
			unsigned int get_number_of_workers() const ;

			/// Get the currently available number of worker threads in the pool
			/** Available worker threads are these not currently carrying out a 
				job and has no jobs i queue. */
			unsigned int get_number_of_idle_workers() const ;

            /// Wait for all currently added jobs in the pool to terminate.
            /** During the call to this function, which will block until all
                currently added jobs have been executed, no additional jobs
                can be added to the pool
                @param time_out If all jobs are not completed before the time_out,
                        (given in milli seconds) the function will return false.
                @return true if all jobs were completed, false otherwise. */
            bool wait_for_all_jobs_completed( const boost::posix_time::time_duration& time_out );

			/// Callback which occurs when a job is finished - automatically assigns a pending
			/// job if there are any available
			void job_done( worker *, boost::shared_ptr< base_job > );

			/// Reports number of pending jobs
			int jobs_pending( );

			void clear_jobs( );

		protected:

			typedef std::vector< boost::shared_ptr< worker >  > worker_vec;
			std::vector< boost::tuples::tuple< event_connection_ptr, bool> > callbacks_;
			std::deque< boost::shared_ptr< base_job > > jobs_;

			worker_vec m_workers;
			unsigned int m_i_max_workers;
			unsigned int m_idle_workers;
            boost::posix_time::time_duration m_thread_termination_timeout;
            mutable boost::recursive_mutex m_this_mtx;
			bool is_terminating_;

			bool add_to_empty_worker(boost::shared_ptr< base_job > p_job);
			bool add_to_new_worker(boost::shared_ptr< base_job > p_job);
			void add_to_least_loaded_worker(boost::shared_ptr< base_job > p_job);

            /// Override this function to provide classes derived from worker.
            virtual boost::shared_ptr< worker > create_worker();

            /// Condition var to notify a job completion
            boost::condition_variable_any cond_;

			boost::condition_variable_any idle_worker_cond_;
		};
	}
}

#endif // _CORE_THREAD_POOL_H_

