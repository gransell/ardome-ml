#include "precompiled_headers.hpp"

#include "thread_pool.hpp"
#include "enforce.hpp"
#include "enforce_defines.hpp"
#include "utilities.hpp"
#include <boost/bind.hpp>

using namespace boost::posix_time;

namespace olib
{
	namespace opencorelib
	{
		thread_pool::thread_pool(	unsigned int max_nrOf_workers,  
								    const boost::posix_time::time_duration& tto)
									: idle_( m_i_max_workers )
									, m_i_max_workers(max_nrOf_workers) 
                                    , m_thread_termination_timeout(tto)
										
		{
			if(m_i_max_workers <= 0) 
				m_i_max_workers = 5;
		}

		thread_pool::~thread_pool()
		{
			terminate_all_threads(m_thread_termination_timeout);
		}

		void thread_pool::terminate_all_threads(const boost::posix_time::time_duration& time_out) 
		{
            boost::recursive_mutex::scoped_lock lock(m_this_mtx);
			std::for_each(m_workers.begin(), m_workers.end(), 
                                boost::bind(&worker::stop, _1, time_out ));
		}

        bool thread_pool::wait_for_all_jobs_completed( const boost::posix_time::time_duration& time_out )
        {
            boost::recursive_mutex::scoped_lock lock(m_this_mtx);
            worker_vec::iterator it(m_workers.begin()), end_it(m_workers.end());
            boost::posix_time::time_duration time_waited;
            for( ; it != end_it; ++it)
            {
                utilities::timer a_timer;
                if(!(*it)->wait_for_all_jobs_completed( time_out - time_waited )) return false;
                time_waited += a_timer.elapsed();
                if( time_waited > time_out ) return false;
            }
            
            return true;
        }

		bool thread_pool::add_to_empty_worker(boost::shared_ptr< base_job > p_job)
		{
			worker_vec::iterator it(m_workers.begin()), end_it(m_workers.end());
			for( ; it != end_it; ++it)
			{
				if((*it)->job_count() == 0 && !(*it)->get_is_running_job() ) 
				{
					(*it)->add_job(p_job);
					return true;
				}
			}
			return false;
		}

		bool thread_pool::add_to_new_worker(boost::shared_ptr< base_job > p_job)
		{
			if( m_workers.size() >= m_i_max_workers ) return false;
            boost::shared_ptr< worker > worker = create_worker();
			worker->add_job(p_job);
			worker->start();
			m_workers.push_back(worker);
			return true;
		}

		void thread_pool::add_to_least_loaded_worker(boost::shared_ptr< base_job > p_job)
		{
			worker_vec::iterator it(m_workers.begin()), 
				end_it(m_workers.end()), min_it(m_workers.end());
			
			for( ; it != end_it; ++it)
			{
				if( min_it == m_workers.end()) min_it = it;
				else if((*it)->job_count() <  (*min_it)->job_count() ) min_it = it; 
			}

			ARENFORCE( min_it != end_it );
			(*min_it)->add_job(p_job);
		}

		void thread_pool::job_done( worker *work, base_job_ptr job )
		{
            boost::recursive_mutex::scoped_lock lock(m_this_mtx);

			while( true )
			{
				if ( jobs_.size( ) == 0 )
				{
					cond_.wait( lock );
				}

				if ( jobs_.size( ) )
				{
					base_job_ptr next = jobs_.front( );
					jobs_.pop_front( );
					work->add_job( next );
					break;
				}
			}
		}

        boost::shared_ptr< worker > thread_pool::create_worker() 
        {
            boost::shared_ptr< worker > p_worker( new worker() );
			boost::tuples::tuple< event_connection_ptr, bool> callback = p_worker->on_job_done.connect( boost::bind( &thread_pool::job_done, this, _1, _2 ) );
			callbacks_.push_back( callback );
            return p_worker;
        }

		void thread_pool::add_job( boost::shared_ptr< base_job > p_job ) 
		{
			if( add_to_new_worker(p_job) ) return;
			//if( add_to_empty_worker(p_job) ) return;
			{
            	boost::recursive_mutex::scoped_lock lock(m_this_mtx);
				jobs_.push_back( p_job );
				cond_.notify_one( );
			}
		}
		
		int thread_pool::jobs_pending( )
		{
            boost::recursive_mutex::scoped_lock lock(m_this_mtx);
			return jobs_.size( );
		}

		void thread_pool::clear_jobs( )
		{
            boost::recursive_mutex::scoped_lock lock(m_this_mtx);
			jobs_.erase( jobs_.begin( ), jobs_.end( ) );
		}

		unsigned int thread_pool::get_number_of_workers() const 
		{
			return m_i_max_workers;
		}

		unsigned int thread_pool::get_number_of_idle_workers() const 
		{
			boost::recursive_mutex::scoped_lock lock(m_this_mtx);
			unsigned int not_created = m_i_max_workers - m_workers.size();
			unsigned int idle_workers(0);

			worker_vec::const_iterator it(m_workers.begin()), end_it(m_workers.end());

			for( ; it != end_it; ++it)
			{
				if((*it)->job_count() == 0 && !(*it)->get_is_running_job() ) 
				{
					idle_workers += 1;
				}
			}

			return not_created + idle_workers;
		}
	}
}
