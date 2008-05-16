#include "precompiled_headers.hpp"

#define LOKI_CLASS_LEVEL_THREADING
#include <loki/Singleton.h>

#include <boost/ref.hpp>
#include <boost/bind.hpp>

#include "logtarget.hpp"
#include "loghandler.hpp"
#include "utilities.hpp"
#include "str_util.hpp"
#include "assert.hpp"
#include "base_exception.hpp"

namespace olib
{
	namespace opencorelib
	{
		typedef Loki::SingletonHolder<	log_handler > the_internal_log_handler;

		log_handler& the_log_handler::instance()
		{
			return the_internal_log_handler::Instance();
		}

		void log_handler::add_target( logtarget_ptr p)
		{
			boost::recursive_mutex::scoped_lock lck( m_Mtx );
			m_log_targets.push_back(p);
		}

		bool log_handler::remove_target( logtarget_ptr p)
		{
			boost::recursive_mutex::scoped_lock lck( m_Mtx );
			for( size_t i = 0; i < m_log_targets.size(); ++i)
			{
				if( m_log_targets[i] == p )
				{
					m_log_targets.erase( m_log_targets.begin() + i );
					return true;
				}
			}

			return false;
		}

        log_handler::log_handler() : m_log(true), m_global_log_level(log_level::warning)
		{
			
		}

		log_handler::~log_handler()
		{
		}

		void log_handler::log( invoke_assert& a, const TCHAR* log_source ) 
		{
			boost::recursive_mutex::scoped_lock lck( m_Mtx );
			if( m_log ) 
			{
				target_vec::iterator it( m_log_targets.begin());
				for( ; it != m_log_targets.end(); ++it)
					(*it)->log(a, log_source);
			}
		}

		void log_handler::log( base_exception& e, const TCHAR* log_source ) 
		{
			boost::recursive_mutex::scoped_lock lck( m_Mtx );
			if( m_log ) 
			{
				target_vec::iterator it( m_log_targets.begin());
				for( ; it != m_log_targets.end(); ++it)
					(*it)->log(e, log_source);
			}
		}

		void log_handler::log( logger& lg, const TCHAR* log_source )
		{
			boost::recursive_mutex::scoped_lock lck( m_Mtx );
			if( m_log ) 
			{
				target_vec::iterator it( m_log_targets.begin());
				for( ; it != m_log_targets.end(); ++it)
					(*it)->log(lg, log_source);
			}
		}

		bool log_handler::get_is_logging() 
		{
			return m_log;
		}

		void log_handler::set_is_logging( bool v)
		{
			boost::recursive_mutex::scoped_lock lck( m_Mtx );
			m_log = v;
		}
	}
}
