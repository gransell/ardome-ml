#include "precompiled_headers.hpp"
#include "enforce.hpp"
#include "base_exception.hpp"
#include "loghandler.hpp"
#include "utilities.hpp"
#include <iostream>

namespace olib
{
	namespace opencorelib
	{
		#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
			#pragma warning( push )
			#pragma warning( disable:4355) // this used in base member initializer list. Ok here.
		#endif
        #undef ARENFORCE_A
        #undef ARENFORCE_B

		invoke_enforce::invoke_enforce() 
            :   ARENFORCE_A( *this), 
                ARENFORCE_B( *this), 
                m_exception_context(new exception_context())
		{
		}

		invoke_enforce::invoke_enforce( const invoke_enforce & other) 
			:	ARENFORCE_A( *this),
				ARENFORCE_B( *this),
				m_exception_context( other.m_exception_context )
		{
		}

		#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
			#pragma warning( pop ) // C4355
		#endif

		invoke_enforce::~invoke_enforce()
		{
			// The contructor of CBase_exception must NOT
			// trow an exception (which probably could happen
			// if memory is low for instance... Fall back 
			// must be provided.

			try
			{
				base_exception excep(*m_exception_context);
                the_log_handler::instance().log(excep, _CT("olib::opencorelib::invoke_enforce::~invoke_enforce"));
				try
				{
					m_exception_context.reset();
				}
				catch (...) 
				{
					// Just swollow that the exception context couldn't be destroyed.
				}

				throw excep;
			}
			catch(base_exception& )
			{
				throw;
			}
			catch(...)
			{
				m_exception_context.reset();
				throw std::runtime_error("Failed to throw base_exception. ARENFORCE failed.");
			}

		}

		


		invoke_enforce& invoke_enforce::add_context(const char* filename, long linenr, const char* funcdname, 
			const char* exp )
		{
			m_exception_context->add_context(filename, linenr, funcdname, exp );
			m_exception_context->call_stack( utilities::get_stack_trace(2));
			return *this;	
		}

        invoke_enforce& invoke_enforce::msg(const char* m)
        {
            if(!m_exception_context) return *this;
            m_exception_context->message(m);
            return *this;
        }

        invoke_enforce& invoke_enforce::msg(const wchar_t* m)
        {
            if(!m_exception_context) return *this;
            m_exception_context->message(m);
            return *this;
        }

        invoke_enforce& invoke_enforce::msg(const utf8_string& m )
        {
            if(!m_exception_context) return *this;
            m_exception_context->message(m);
            return *this;
        }

        invoke_enforce& invoke_enforce::msg(const utf16_string& m )
        {
            if(!m_exception_context) return *this;
            m_exception_context->message(m);
            return *this;
        }

		invoke_enforce& invoke_enforce::hr( boost::int32_t hr)
		{
			m_exception_context->h_result(hr);
			return *this;
		}

		invoke_enforce& invoke_enforce::reason(const TCHAR* r)
		{
			m_exception_context->error_reason(r);
			return *this;
		}

		bool invoke_enforce::always_fail()
		{
			return false;
		}
	}
}
