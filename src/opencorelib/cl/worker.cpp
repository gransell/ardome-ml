#include "precompiled_headers.hpp"
#include <boost/bind.hpp>
#include <limits>

#include "worker.hpp"
#include "jobbase.hpp"
#include "utilities.hpp"
#include "guard_define.hpp"
#include "base_exception.hpp"

#include "assert.hpp"
#include "enforce.hpp"
#include "logger.hpp"

#include "assert_defines.hpp"
#include "enforce_defines.hpp"
#include "log_defines.hpp"

using namespace boost::posix_time;

#undef max

namespace olib
{
	namespace opencorelib
	{
		bool worker::job_compare_less::operator()(	boost::shared_ptr<base_job> lhs, 
													boost::shared_ptr<base_job> rhs )
		{
			return *lhs < *rhs;
        }

        bool worker::job_compare_equal::operator()(	boost::shared_ptr<base_job> lhs, 
													boost::shared_ptr<base_job> rhs )
		{
			return *lhs == *rhs;
        }

		worker::worker() : m_thread_running(false), m_stop_thread(false)
		{
		}

		worker::~worker()
		{
			stop(5000);
		}

        namespace
        {
            void increase_priority( const base_job_ptr& job)
            {
                job->set_priority( job->get_priority() + 1 );
            }
        }

		void worker::add_job(const base_job_ptr& job) 
		{
			boost::lock_guard<boost::mutex> lock(m_wake_thread_mtx);
			m_heap.push_back(job);
            std::make_heap( m_heap.begin(), m_heap.end(), job_compare_less() );
			m_wake_thread.notify_one();
		}

        void worker::add_job_with_highest_priority( const base_job_ptr& job) 
        {
			boost::lock_guard<boost::mutex> lock(m_wake_thread_mtx);
            std::for_each( m_heap.begin(), m_heap.end(),  &increase_priority );
            job->set_priority(0);
            m_heap.push_back(job);
            std::make_heap( m_heap.begin(), m_heap.end(), job_compare_less() );
            m_wake_thread.notify_one();
        }

        void worker::add_reoccurring_job( const base_job_ptr& p_job, const time_value& run_interval ) 
        {
			boost::lock_guard<boost::mutex> lock(m_wake_thread_mtx);
            p_job->m_reoccuring_interval = run_interval;
            p_job->m_run_more_than_once = true;
            p_job->m_next_time_to_run = time_value::now() + run_interval;
            m_timed_jobs.push_back( p_job );
			m_wake_thread.notify_one();
        }

        void worker::remove_reoccurring_job( const base_job_ptr& p_job )
        {
			boost::lock_guard<boost::mutex> lock(m_wake_thread_mtx);
            jobcollection::iterator it = std::find( m_timed_jobs.begin(), m_timed_jobs.end(), p_job );
            if( it == m_timed_jobs.end() ) 
            {
                ARLOG_WARN( "Can not remove the specified job, not in this worker.");
                return;
            }
            
            m_timed_jobs.erase( it );
        }

        void worker::add_job( const base_job_ptr& p_job, const time_value& start_after )
        {
			boost::lock_guard<boost::mutex> lock(m_wake_thread_mtx);
            p_job->m_reoccuring_interval = start_after;
            p_job->m_run_more_than_once = false;
            p_job->m_next_time_to_run = time_value::now() + start_after;
            m_timed_jobs.push_back( p_job );
            m_wake_thread.notify_one();
        }

        void worker::move_timed_jobs_to_heap()
        {
			boost::lock_guard<boost::mutex> lock(m_wake_thread_mtx);
            time_value right_now( time_value::now() ); 
            jobcollection::iterator it(m_timed_jobs.begin());
            bool need_heapify(false);
            while( it != m_timed_jobs.end() )
            {
                if( (*it)->m_next_time_to_run <= right_now )
                {
                    need_heapify = true;
                    m_heap.push_back( (*it) );
                    if( (*it)->m_run_more_than_once == false )
                    {
                        it = m_timed_jobs.erase(it);
                    }
                    else
                    {
                        (*it)->m_next_time_to_run = time_value::now() + (*it)->m_reoccuring_interval;
                        it++;
                    }
                }
                else
                {
                    it++;
                }
            }

            if( need_heapify )
            {
                std::make_heap( m_heap.begin(), m_heap.end(), job_compare_less() );
            }
        }

        time_value worker::next_timed_job_start_time() const
        {
            static const boost::int64_t max_wait_time = ARMAKE_I64(1000);
            time_value start_value( max_wait_time, 0 );
            start_value += time_value::now();
            jobcollection::const_iterator it(m_timed_jobs.begin()), eit(m_timed_jobs.end());
            for( ; it != eit; ++it )
            {
                if( (*it)->m_next_time_to_run < start_value )
                {
                    start_value = (*it)->m_next_time_to_run;
                }
            }

            return start_value;
        }

		size_t worker::job_count() const 
		{
			boost::lock_guard<boost::mutex> lock(m_wake_thread_mtx);
			if( m_current_job ) return 1 + m_heap.size();
			return m_heap.size();
		}

		void worker::stop( long time_out )
		{
			if (m_thread_running) 
            {
                cancel_and_clear_jobs();

                {
					boost::lock_guard<boost::mutex> lock(m_wake_thread_mtx);
                    m_stop_thread = true;
                    m_wake_thread.notify_one();
                }

                utilities::timer a_timer;                  
                for(;;)
                {
					boost::unique_lock<boost::mutex> mtx(m_thread_exit_mtx);

					if(!m_thread_running) break;
                    boost::xtime wt = 
                        utilities::add_millsecs_from_now(time_out - a_timer.elapsed_millisecs());
                    if( m_thread_exit_success.timed_wait(mtx, wt))
                    {
                        if( !m_thread_running ) break;
                    }

                    if( a_timer.elapsed_millisecs() > time_out ) break; 
                }
			}

			ARASSERT(!m_thread_running);
            m_thread_running = false;
		}

        void worker::on_thread_started() 
        {
        }

        void worker::on_thread_stopping() 
        {
        }

        void worker::on_empty_heap() 
        {
        }

        bool worker::on_idle(   boost::condition_variable& wake_thread,
                                boost::mutex& wake_thread_mtx  )
        {
			boost::unique_lock<boost::mutex> lock(wake_thread_mtx);
//            boost::recursive_mutex::scoped_lock lock(wake_thread_mtx);
            for(;;)
            {
                if( m_stop_thread ) return false;
                if( !m_heap.empty()) return true;
                time_value tv = next_timed_job_start_time();
                wake_thread.timed_wait(lock, tv);
                move_timed_jobs_to_heap();
            }
        }

        void worker::on_job_starting(boost::shared_ptr<base_job> p_job) 
        {
            ARUNUSED_ALWAYS(p_job);
        }

        void worker::start()
        {
            // This might set m_stop_thread to true
            stop(5000);

            // Make sure we really start the thread here if the worker is
            // restarted for some kind of reason.
            m_stop_thread = false;

            m_worker_thread = boost::shared_ptr< boost::thread>( 
                new boost::thread(boost::bind( &worker::worker_thread_procedure, this)));

            // Give the worker a chance to start.
            boost::thread::yield();

//            boost::recursive_mutex::scoped_lock mtx(m_thread_started_mtx);
			boost::unique_lock<boost::mutex> mtx(m_thread_started_mtx);
            if( m_thread_running ) return;
            boost::xtime wt = utilities::add_millsecs_from_now(5000); 
            ARENFORCE( m_thread_started.timed_wait(mtx, wt) );
            ARASSERT( m_thread_running );
        }

		void worker::worker_thread_procedure()
		{
			try
			{
                on_thread_started();
                
                {
					boost::lock_guard<boost::mutex> mtx(m_thread_started_mtx);
//                    boost::recursive_mutex::scoped_lock mtx(m_thread_started_mtx);
                    m_thread_running = true;
                    m_thread_started.notify_all();
                }
                
				for(;;)
				{
					do_work();
                    if(!on_idle( m_wake_thread, m_wake_thread_mtx )) break;
				}

				on_thread_stopping();
				ARASSERT(m_current_job == 0);
			}
			catch( ... )
			{
				// Just swallow anything, we are just interested in
				// maintaining state.
			}
			
			boost::lock_guard<boost::mutex> mtx(m_thread_exit_mtx);

			m_thread_running = false;
            m_thread_exit_success.notify_all();
		}

		void worker::reset_current_job()
		{
			boost::lock_guard<boost::mutex> lock(m_wake_thread_mtx);
			m_current_job.reset();
		}

        struct done_counter : public boost::noncopyable
        {
            done_counter() 
                : m_nr_of_jobs_done(0), m_nr_of_jobs_to_wait_for(0){}

            ~done_counter()
            {
            }

            void on_done( boost::shared_ptr<base_job> )
            {
				boost::lock_guard<boost::mutex> lck(m_mtx);
                m_nr_of_jobs_done++;
                if( m_nr_of_jobs_done >= m_nr_of_jobs_to_wait_for )
                {
                    m_all_complete.notify_all();
                }
            }

            bool wait_for_all_jobs_completed(long time_out )
            {
				boost::unique_lock<boost::mutex> lck(m_mtx);
                if( m_nr_of_jobs_done >= m_nr_of_jobs_to_wait_for) return true;
                boost::xtime wt = utilities::add_millsecs_from_now(time_out);
                m_all_complete.timed_wait(lck, wt);
                ARASSERT( m_nr_of_jobs_done == m_nr_of_jobs_to_wait_for )
                                (m_nr_of_jobs_done)(m_nr_of_jobs_to_wait_for);
                return m_nr_of_jobs_done >= m_nr_of_jobs_to_wait_for;
            }

            void add_job( boost::shared_ptr< base_job > a_job )
            {
                boost::tuples::tuple< event_connection_ptr, bool>
                    res = a_job->on_job_done(boost::bind(&done_counter::on_done, this, _1));
                if( res.get<1>() ) m_nr_of_jobs_done++;
                m_nr_of_jobs_to_wait_for += 1;
                m_connections.push_back(res.get<0>());
            }

            int m_nr_of_jobs_done;
            int m_nr_of_jobs_to_wait_for;

			boost::mutex m_mtx;
            boost::condition_variable m_all_complete;
            std::vector< event_connection_ptr > m_connections;
        };

        bool worker::wait_for_all_jobs_completed( long time_out )
        {
            done_counter done_counter;

            {
                // Prevent new jobs from being added to this worker:
				boost::lock_guard<boost::mutex> lock(m_wake_thread_mtx);

				boost::shared_ptr< boost::lock_guard<boost::mutex> > curr_lck;
//                boost::shared_ptr< boost::recursive_mutex::scoped_lock > curr_lck; 

                if( m_current_job ) 
                {
                    // Make sure the current job is prevented from terminating
                    // while we're adding it to the wait list.
                    curr_lck = m_current_job->prevent_job_from_terminating();
                    if(curr_lck) done_counter.add_job(m_current_job);
                    else 
					{
						// T_CERR << "Not adding job because it is done...";
					}
                }

                // These locks will prevent the jobs from reporting termination
                // before this scope is ended (when we will wait for them to 
                // terminate )
//                std::vector< boost::shared_ptr< boost::recursive_mutex::scoped_lock > > locks;
                std::vector< boost::shared_ptr< boost::lock_guard<boost::mutex> > > locks;

                jobcollection q = m_heap;
                // T_CERR << "Size of job heap: " << m_heap.size() << std::endl;
                for( jobcollection::iterator i = q.begin(); i != q.end(); ++i )
                {
					boost::shared_ptr< boost::lock_guard<boost::mutex> > lck;
//                    boost::shared_ptr< boost::recursive_mutex::scoped_lock > lck;
                    lck = (*i)->prevent_job_from_terminating();
                    if( !lck ) continue; // This job was already terminated
                    done_counter.add_job( *i );
                    locks.push_back(lck); // Make sure the job doesn't terminate until we are done.
                }

                // T_CERR << "Nr of jobs in worker: " << done_counter.m_nr_of_jobs_to_wait_for << std::endl;
            }

            return done_counter.wait_for_all_jobs_completed(time_out);
        }

		void worker::do_work()
		{
			// Always release the current job when we leave the function.
			ARGUARD( boost::bind(&worker::reset_current_job , this));
			for(;;) 
			{
				{
					boost::lock_guard<boost::mutex> lock(m_wake_thread_mtx);
                    if( m_heap.empty())
                    {
                        on_empty_heap();
                        break;
                    }
					m_current_job = m_heap.back();
					m_heap.pop_back();
				}

				try
				{
					if(!m_current_job) return;
                    on_job_starting(m_current_job);
					m_current_job->do_work_bootstrapper();
                    if( m_stop_thread ) break;
				}
                catch( olib::opencorelib::base_exception& e)
				{
					t_stringstream ss;
                    e.pretty_print_one_line(ss, print::output_default );
                    ARLOG("Exception on worker thread: %s")(ss.str()).alert();
				}
				catch( std::exception& e)
				{
                    ARLOG("Exception on worker thread: %s")(e.what()).alert();
				}
				catch(...)
				{
					ARLOG("unknown Exception on worker thread. catch( ... )").alert();
				}
			}
		}

		bool worker::get_is_running_job() const 
		{
			boost::lock_guard<boost::mutex> lock(m_wake_thread_mtx);
			return (m_current_job != 0);
		}

		void worker::cancel_current_job() 
		{
			boost::lock_guard<boost::mutex> lock(m_wake_thread_mtx);
			if( !m_current_job) return;
			m_current_job->set_should_terminate_job(true);
		}

        bool worker::cancel_current_job(long time_out) 
        {
			boost::lock_guard<boost::mutex> lock(m_wake_thread_mtx);
            if( !m_current_job) return true;
            m_current_job->set_should_terminate_job(true);
            return m_current_job->wait_for_job_done(time_out);
        }

		void worker::cancel_and_clear_jobs() 
		{
			boost::lock_guard<boost::mutex> lock(m_wake_thread_mtx);
			cancel_current_job();
			while(!m_heap.empty())
			{
				m_heap.back()->set_should_terminate_job(true);
				m_heap.pop_back();
			}
		}
	}
}
