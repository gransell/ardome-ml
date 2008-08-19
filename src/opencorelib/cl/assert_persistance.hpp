#ifndef _CORE_ASSERT_PERSISTANCE_H_
#define _CORE_ASSERT_PERSISTANCE_H_

#include "assert.hpp"
#include "str_util.hpp"

#include <set>

namespace olib
{
	namespace opencorelib
	{
		/// This class (a singleton) enables users to turn on and off assertions.
		/** When an assertion is displayed you can choose between 
			Ignore, ignore_all or Always_ignore. Ignore means that the 
			assertion is ignored that one time, ignore_all means ignore
			all assertions everywhere while Always_ignore means ingore this particular
			assertion for the rest of this execution.
			@author Mats */
		class CORE_API assert_persistance : public boost::noncopyable
		{

		public:
			assert_persistance(void);
			~assert_persistance(void);

            /// Called when the given assert shoudn't be issued anymore.
			void ignore_assert( invoke_assert& a, assert_response::action action );
            /// Called to find out if a certain assert should be ignored or not.
			bool should_ignore( invoke_assert& a);

            /// Makes should_ignore return v for all asserts.
			void ignore_all( bool v);
			bool ignore_all();

		private:

			class assert_id
			{
			private:
				t_string m_file;
				unsigned int m_line_nr;
			public:
				assert_id() : m_file(str_util::to_t_string("")), m_line_nr(0) {}

				assert_id( const t_string& f, unsigned int lnr) : m_file(f), m_line_nr(lnr) {}
				
				bool operator==( const assert_id& o)
				{
					return (m_file == o.m_file && m_line_nr == o.m_line_nr);
				}

				friend bool operator<( const assert_id& o1, const assert_id& o2)
				{
					int l_res = o1.m_file.compare(o2.m_file);
					if( l_res != 0) return l_res < 0 ? true : false;
					return o1.m_line_nr < o2.m_line_nr;
				}
				
			};
			
			assert_id make_id( invoke_assert& a);

			std::set< assert_id > m_ignored_asserts;
			bool m_ignore_all;

			boost::mutex m_Mtx;
		};

		/// Singleton CAssert_persistance object.
		class CORE_API the_assert_persister
		{
		public:
			static assert_persistance& instance();
		};


	}
}

#endif //_CORE_ASSERT_PERSISTANCE_H_

