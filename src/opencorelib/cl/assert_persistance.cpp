#include "precompiled_headers.hpp"
#include "assert_persistance.hpp"
#include <loki/Singleton.h>

namespace olib
{
	namespace opencorelib
	{
        typedef Loki::SingletonHolder<	assert_persistance > the_internal_assert_persister;

		assert_persistance& the_assert_persister::instance()
		{
			return the_internal_assert_persister::Instance();
		}

		assert_persistance::assert_persistance(void)
			: m_ignore_all(false)
		{
		}

		assert_persistance::~assert_persistance(void)
		{
		}

		assert_persistance::assert_id assert_persistance::make_id( invoke_assert& a)
		{
			boost::shared_ptr<exception_context> ctx = a.context();
			if( !ctx ) return assert_id();
			return assert_id(ctx->filename(), ctx->line_nr());
		}

		void assert_persistance::ignore_assert(	invoke_assert& a, 
												assert_response::action action) 
		{
			boost::mutex::scoped_lock lck(m_Mtx);
			switch(action) 
			{
				case assert_response::ignore_all: 
					m_ignore_all = true;
					break;
				case assert_response::ignore_always:
					m_ignored_asserts.insert(make_id(a));
					break;
				default: break;
			}

		}

		bool assert_persistance::should_ignore( invoke_assert& a)
		{
			boost::mutex::scoped_lock lck(m_Mtx);
			if( m_ignore_all ) return true;
			bool ignore = m_ignored_asserts.find(make_id(a)) != m_ignored_asserts.end();
			return ignore;
		}

		void assert_persistance::ignore_all( bool v)
		{
			boost::mutex::scoped_lock lck(m_Mtx);
			m_ignore_all = v;
		}

		bool assert_persistance::ignore_all() 
		{
			boost::mutex::scoped_lock lck(m_Mtx);
			return m_ignore_all;
		}
	}
}
