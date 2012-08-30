#include "precompiled_headers.hpp"
#include "enforce.hpp"
#include "base_exception.hpp"
#include "loghandler.hpp"
#include "log_defines.hpp"
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

				if( str_util::env_var_exists(_CT("AML_USE_ASSERT_ARENFORCE")) )
				{
					std::string enforce_msg = excep.what();

					//Don't throw an exception, just assert to break into the debugger
					assert( false && "ARENFORCE is implemented as assert since AML_USE_ASSERT_ARENFORCE is set. Details are available in the enforce_msg string above." );
				}
				else
				{
					the_log_handler::instance().log(excep, _CT("olib::opencorelib::invoke_enforce::~invoke_enforce"));
				}

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

#ifdef BOOST_ENABLE_ASSERT_HANDLER

#include "enforce_defines.hpp"

namespace boost {
void assertion_failed(char const * expr, char const * function, char const * file, long line) {

	char buf[32];
	snprintf(buf, sizeof buf, "%ld }\n", line);

	std::string msg;
	msg.reserve(256);
	msg += "BOOST ASSERTION FAILED\n\ton expression: { ";
	msg += expr;
	msg += " }\n\tin function { ";
	msg += function;
	msg += " }\n\tin file { ";
	msg += file;
	msg += ":";
	msg += buf;
	msg += "Backtrace:\n";
	msg += olib::opencorelib::utilities::get_stack_trace( 4 );

	std::cerr << msg << std::endl;
	ARLOG_CRIT( msg );
}
}

#else
//#	warning ===============================================================
//#	warning Compiling without BOOST_ENABLE_ASSERT_HANDLER will not give
//#	warning explanatory assertion output when a boost expression fails.
//#	warning ===============================================================
#endif // BOOST_ENABLE_ASSERT_HANDLER


