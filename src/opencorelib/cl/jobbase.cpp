#include "precompiled_headers.hpp"
#include "jobbase.hpp"
#include "guard_define.hpp"
#include "utilities.hpp"
#include "base_exception.hpp"
#include <boost/bind.hpp>

namespace olib
{
   namespace opencorelib
    {
        long base_job::s_dw_counter = 0;
        base_job::base_job() 
            :   m_prio(5), m_terminate_job(false), m_job_done(false), 
                m_exception_thrown(false), m_result(0)
        {
            m_dw_number = s_dw_counter++;
        }

        base_job::base_job( boost::int32_t prio )
            :   m_prio(prio), m_terminate_job(false), m_job_done(false), 
                m_exception_thrown(false), m_result(0)
        {
            m_dw_number = s_dw_counter++;
        }

        base_job::~base_job() 
        {
        }

        bool base_job::get_should_terminate_job() const
        {
            return m_terminate_job;
        }

        void base_job::set_should_terminate_job( bool term )
        {
            m_terminate_job = term;
        }

        bool base_job::compare(const base_job& job) const 
        {
            ARUNUSED_ALWAYS( job );
            return false;
        }

        void base_job::do_work_bootstrapper()
        {
            m_job_done = false;
            ARGUARD( boost::bind(&base_job::signal_job_done, this) );
            try
            {
                m_result = do_work();
            }
            catch( olib::opencorelib::base_exception& be)
            {
                m_exception_thrown = true;
                m_base_exception = boost::shared_ptr<base_exception>( new base_exception(be));
                m_result = -1;
            }
            catch( std::exception& se)
            {
                m_exception_thrown = true;
                m_std_exception = boost::shared_ptr<std::exception>( new std::exception(se));
                m_result = -1;
            }
            catch(...)
            {
                m_exception_thrown = true;
                m_result = -1;
            }
        }

        bool base_job::wait_for_job_done(long time_out )
        {
            boost::recursive_mutex::scoped_lock lock( m_job_done_mtx );
            if( m_job_done ) return true;
            boost::xtime xt = utilities::add_millsecs_from_now(time_out);
            return m_job_done_condition.timed_wait(lock, xt);
        }

        boost::shared_ptr< boost::recursive_mutex::scoped_lock >
        base_job::prevent_job_from_terminating()
        {
            boost::shared_ptr< boost::recursive_mutex::scoped_lock >
                lck( new boost::recursive_mutex::scoped_lock(m_job_done_mtx ) );

            if( m_job_done ) 
            {
                return boost::shared_ptr< boost::recursive_mutex::scoped_lock >();
            }
            return lck;
        }

        void base_job::signal_job_done()
        {
            boost::recursive_mutex::scoped_lock lock( m_job_done_mtx );
            m_job_done = true;
            m_job_done_condition.notify_all();
            m_job_done_signal(shared_from_this());
        }

        boost::tuples::tuple< boost::signals::connection, bool >
        base_job::on_job_done( base_job::jobdone_signal::slot_function_type listener )
        {
             boost::recursive_mutex::scoped_lock lock( m_job_done_mtx );
             return boost::tuples::make_tuple(m_job_done_signal.connect(listener), m_job_done);
        }

        bool base_job::get_is_job_done() const
        {
            boost::recursive_mutex::scoped_lock lock( m_job_done_mtx );
            return m_job_done; 
        }


        CORE_API bool operator<(base_job& lhs, base_job& rhs)
        {
            return (lhs.m_prio == rhs.m_prio) ? 
                lhs.m_dw_number > rhs.m_dw_number : lhs.m_prio < rhs.m_prio;
        }

        CORE_API bool operator==(base_job& lhs, base_job& rhs)
        {
            return lhs.compare(rhs);
        }
    }
}
