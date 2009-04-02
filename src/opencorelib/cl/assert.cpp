#include "precompiled_headers.hpp"

#include "assert.hpp"
#include "assert_persistance.hpp"
#include "loghandler.hpp"
#include "assert_presenter.hpp"
#include "utilities.hpp"

namespace olib
{
	namespace opencorelib 
	{
        #ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
            #pragma warning( push )
            #pragma warning( disable:4355) // this used in base member initializer list. Ok here.
        #endif

        // Make sure the macro magic works :-)
        #undef ARASSERT_A
        #undef ARASSERT_B 
		
        invoke_assert::invoke_assert(void) 
			:	ARASSERT_A(*this), 
				ARASSERT_B(*this),
				m_level( assert_level::warning ),
				m_ignore_this_assert(false),
				m_context( new exception_context())
		{
		}

		invoke_assert::~invoke_assert(void)
		{
			if ( m_ignore_this_assert ) return;
			handle_assert();
		}

		invoke_assert::invoke_assert( const invoke_assert& o ) 
			:ARASSERT_A(*this), 
			 ARASSERT_B(*this),
			 m_level(o.m_level),
			 m_ignore_this_assert(o.m_ignore_this_assert),
			 m_context(o.m_context)
		{
			o.m_ignore_this_assert = true;
		}

        #ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
        #pragma warning( pop ) // C4355
        #endif

		invoke_assert& invoke_assert::operator=(const invoke_assert& o)
		{
			m_level = o.m_level;
			m_context = o.m_context;
			m_ignore_this_assert = o.m_ignore_this_assert;
			o.m_ignore_this_assert = true;
			return *this;
		}


		invoke_assert invoke_assert::make_assert()
		{
			return invoke_assert();
		}

		invoke_assert& invoke_assert::add_context(	const char* filename, long linenr, 
										const char* funcdname, const char* exp )
		{
			m_context->add_context(filename, linenr, funcdname, exp );
			m_context->call_stack( utilities::get_stack_trace(2) );
			return *this;	
		}

		void invoke_assert::handle_assert()
		{

			try
			{

				// Always log the assertion to a file
                the_log_handler::instance().log(*this, str_util::to_t_string("olib::opencorelib::invoke_assert::handle_assert").c_str());
	
				if(the_assert_persister::instance().should_ignore(*this)) return;

				// Present the error to the user in a Dialog
				the_assert_presenter::instance().show(*this);

			}
			catch (...) 
			{
				// swallow any exceptions that occur 
			}
		}

        invoke_assert& invoke_assert::msg(const char* m)
        {
            if( !m_context ) return *this;
            m_context->message(m);
            return *this;
        }

        invoke_assert& invoke_assert::msg(const wchar_t* m)
        {
            if( !m_context ) return *this;
            m_context->message(m);
            return *this;
        }

        invoke_assert& invoke_assert::msg(const utf8_string& m )
        {
            if( !m_context ) return *this;
            m_context->message(m);
            return *this;
        }

        invoke_assert& invoke_assert::msg(const utf16_string& m )
        {
            if( !m_context ) return *this;
            m_context->message(m);
            return *this;
        }

		invoke_assert& invoke_assert::reason( const t_string& r)
		{
			if( !m_context ) return *this;
			m_context->error_reason(r);
			return *this;
		}

		invoke_assert& invoke_assert::level( assert_level::severity lvl , 
								const t_string& str_msg )
		{

			m_level = lvl;
			if( !m_context ) return *this;
			m_context->message(str_msg);
			return *this;
		}

		invoke_assert& invoke_assert::debug( const t_string& str_msg)
		{
			m_level = assert_level::debug;
			if( !m_context ) return *this;
			m_context->message(str_msg);

			return *this;
		}

		invoke_assert& invoke_assert::warn( const t_string& str_msg )
		{
			m_level = assert_level::warning;
			if( !m_context ) return *this;
			m_context->message(str_msg);
			return *this;
		}

		invoke_assert& invoke_assert::error( const t_string& str_msg )
		{
			m_level = assert_level::error;
			if( !m_context ) return *this;
			m_context->message(str_msg);
			return *this;
		}

		invoke_assert& invoke_assert::fatal( const t_string& str_msg )
		{
			m_level = assert_level::fatal;
			if( !m_context ) return *this;
			m_context->message(str_msg);
			return *this;
		}

		// sets the expression
		/*CAssert& CAssert::set_expr( const char* str_expr)
		{
			m_tested_expression = str_util::to_t_string(str_expr);
			return *this;
		}*/

		invoke_assert& invoke_assert::ignore( bool val)
		{
			m_ignore_this_assert = val;
			return *this;
		}

		boost::shared_ptr<exception_context> invoke_assert::context()
		{
			return m_context;
		}

		t_string invoke_assert::assert_level_asstring() const
		{
			return assert_level::to_string(m_level);
		}


		t_ostream& operator<<( t_ostream& ostrm, const invoke_assert& a)
		{
			ostrm << _CT("<assert level='") << a.assert_level_asstring() << _CT("'>");
			if(a.m_context) ostrm << (*a.m_context);
			else ostrm << _CT("<No_further_information/>");
			ostrm << _CT("</assert>");
			return ostrm;
		}

		void invoke_assert::pretty_print( t_ostream& ostrm , print::option print_option)
		{
			if( !m_context ) return;
			ostrm << _CT("Assertion failed [ ") << assert_level_asstring() << _CT(" ]") << std::endl;
			m_context->pretty_print(ostrm, print_option);
		}

		void invoke_assert::pretty_print_one_line( t_ostream& ostrm, print::option print_option )
		{
			if( !m_context ) return;
			ostrm << _CT("Assertion failed [ ") << assert_level_asstring() << _CT(" ] ");
			m_context->pretty_print_one_line(ostrm, print_option);
		}

		assert_level::severity invoke_assert::level() const
		{
			return m_level;
		}

		bool invoke_assert::always_fail()
		{
			#ifdef _DEBUG
				return false;
			#else
				return true;
			#endif
		}
	}
}
