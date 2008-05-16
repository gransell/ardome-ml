#include "precompiled_headers.hpp"
#include "base_exception.hpp"
#include "exception_presenter.hpp"

#include <algorithm>
#include <boost/bind.hpp>

namespace olib
{
	namespace opencorelib
	{

		base_exception::base_exception(const olib::opencorelib::exception_context& context)
			: m_context(context)
		{
		}

		base_exception::~base_exception(void) throw()
		{
		}

		const char* base_exception::what( ) const throw( )
		{
			m_std_except_message = str_util::to_string(m_context.as_xml());
			return m_std_except_message.c_str();
		}

		void base_exception::original_exception_type(const t_string& type_name)
		{
			m_original_exception_type = type_name;
		}
		const t_string& base_exception::original_exception_type(void) const
		{
			return m_original_exception_type;
		}

		void base_exception::transformation_function(const t_string& function_name)
		{
			m_transformation_function = function_name;
		}

		const t_string& base_exception::transformation_function(void) const
		{
			return m_transformation_function;
		}

		void base_exception::add_function_to_catch_stack(const t_string& function_name)
		{
			m_catch_stack.push_back(function_name);
		}

		const std::vector<t_string>& base_exception::catch_stack(void) const
		{
			return m_catch_stack;	
		}

		void base_exception::print_catch_stack(t_stringstream& os, const t_string& func_name)
		{
			os << _T("<catch_func>") << func_name << _T("</catch_func>");
		}
	
		t_string base_exception::as_xml() const
		{
			t_stringstream os(_T(""));

			os << _T("<exception_info>");

			m_context.as_xml(os);			

			os << _T("<original_exception_type>") << m_original_exception_type << _T("</original_exception_type>");
			os << _T("<transforming_function>") << m_transformation_function <<  _T("</transforming_function>");
			
			os << _T("<catch_stack>");
			std::for_each(m_catch_stack.begin(), m_catch_stack.end(), 
                boost::bind(&base_exception::print_catch_stack, boost::ref(os), _1));
			os << _T("</catch_stack>");
			
			os << _T("</exception_info>");

			return os.str();
		}

		const exception_context& base_exception::context() const
		{
			return m_context;
		}

		exception_context& base_exception::context()
		{
			return m_context;
		}

		void base_exception::pretty_print( t_ostream& ostrm, print::option print_option ) const
		{
            bool output_empty = (print_option & print::output_empty_fields) ? true : false;
			if(!m_original_exception_type.empty() || output_empty )
				ostrm << _T("Original Exception Type:[") << m_original_exception_type << _T("]\n"); 

			if(!m_transformation_function.empty() || output_empty )
				ostrm << _T("Transformation function:[") << m_transformation_function << _T("]\n");
			
            m_context.pretty_print(ostrm, print_option);

			if( !m_catch_stack.empty() || output_empty )
			{
				ostrm << _T("\nCatch Stack:\n"); 
				std::for_each(m_catch_stack.begin(), m_catch_stack.end(), 
                    boost::bind(&base_exception::pretty_print_catch_stack, boost::ref(ostrm), _1, true));
			}
		}

		void base_exception::pretty_print_catch_stack(t_ostream& os, const t_string& func_name, bool insert_new_line )
		{
			if( insert_new_line )
				os << _T("\t") << func_name << std::endl;
			else
				os << _T("<=") << func_name << _T("=>");
		}

		void base_exception::pretty_print_one_line(t_ostream& ostrm, print::option print_option) const
		{
			bool output_empty = (print_option & print::output_empty_fields) ? true : false;
			ostrm << _T("Exception thrown :: ");
			m_context.pretty_print_one_line(ostrm, print_option);
			if( !m_catch_stack.empty() || output_empty )
			{
				ostrm << _T(" Catch_stack:["); 
				std::for_each(m_catch_stack.begin(), m_catch_stack.end(), 
                    boost::bind(&base_exception::pretty_print_catch_stack, boost::ref(ostrm), _1, false));
				ostrm << _T("]");
			}

			if( !m_original_exception_type.empty() || output_empty )
				ostrm << _T(" Original Exception:[") << m_original_exception_type << _T("]"); 

			if( !m_transformation_function.empty() || output_empty )
				ostrm << _T(" Transformation function:[") << m_transformation_function << _T("]");
		}

		user_reaction::type base_exception::show( const base_exception& e)
		{
			return show(e, _T("base_exception Caught"));
		}

		user_reaction::type base_exception::show( const base_exception& e, const t_string& caption)
		{
			if (the_exception_presenter::instance().show(caption, e) )
			{
				return user_reaction::rethrow;
			}

			return user_reaction::ignore;
		}

	}

}
