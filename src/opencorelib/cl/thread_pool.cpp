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
								 boost::int32_t tto)
									:	m_i_max_workers(max_nrOf_workers),
										m_dw_thread_termination_timeout(tto)
		{
			if(m_i_max_workers <= 0) m_i_max_workers = 5;
		}

		thread_pool::~thread_pool()
		{
			terminate_all_threads(m_dw_thread_termination_timeout);
		}

		void thread_pool::terminate_all_threads(long time_out) 
		{
            boost::recursive_mutex::scoped_lock lock(m_this_mtx);
			std::for_each(m_Workers.begin(), m_Workers.end(), 
                                boost::bind(&worker::stop, _1, time_out ));
		}

        bool thread_pool::wait_for_all_jobs_completed( long time_out )
        {
            boost::recursive_mutex::scoped_lock lock(m_this_mtx);
            Worker_vec::iterator it(m_Workers.begin()), end_it(m_Workers.end());
            long time_waited(0);
            for( ; it != end_it; ++it)
            {
                utilities::timer a_timer;
                if(!(*it)->wait_for_all_jobs_completed( time_out - time_waited )) return false;
                time_waited += a_timer.elapsed_millisecs();
                if( time_waited > time_out ) return false;
            }
            
            return true;
        }

		bool thread_pool::add_to_empty_worker(boost::shared_ptr< base_job > p_job)
		{
			Worker_vec::iterator it(m_Workers.begin()), end_it(m_Workers.end());
			for( ; it != end_it; ++it)
			{
				if((*it)->job_count() == 0 ) 
				{
					(*it)->add_job(p_job);
					return true;
				}
			}
			return false;
		}

		bool thread_pool::add_to_new_worker(boost::shared_ptr< base_job > p_job)
		{
			if( m_Workers.size() >= m_i_max_workers ) return false;
            boost::shared_ptr< worker > worker = create_worker();
			worker->add_job(p_job);
			worker->start();
			m_Workers.push_back(worker);
			return true;
		}

		void thread_pool::add_to_least_loaded_worker(boost::shared_ptr< base_job > p_job)
		{
			Worker_vec::iterator it(m_Workers.begin()), 
				end_it(m_Workers.end()), min_it(m_Workers.end());
			
			for( ; it != end_it; ++it)
			{
				if( min_it == m_Workers.end()) min_it = it;
				else if((*it)->job_count() <  (*min_it)->job_count() ) min_it = it; 
			}

			ARENFORCE( min_it != end_it );
			(*min_it)->add_job(p_job);
		}

        boost::shared_ptr< worker > thread_pool::create_worker() 
        {
            boost::shared_ptr< worker > p_worker( new worker() );
            return p_worker;
        }

		void thread_pool::add_job( boost::shared_ptr< base_job > p_job ) 
		{
            boost::recursive_mutex::scoped_lock lock(m_this_mtx);
			if( add_to_empty_worker(p_job) ) return;
			if( add_to_new_worker(p_job) ) return;
			add_to_least_loaded_worker(p_job);
		}
		
		unsigned int thread_pool::get_number_of_workers() const 
		{
			return m_i_max_workers;
		}

		unsigned int thread_pool::get_number_of_idle_workers() const 
		{
			boost::recursive_mutex::scoped_lock lock(m_this_mtx);
			unsigned int not_created = m_i_max_workers - m_Workers.size();
			unsigned int idle_workers(0);

			Worker_vec::const_iterator it(m_Workers.begin()), end_it(m_Workers.end());

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
