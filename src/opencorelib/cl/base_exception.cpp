#include "precompiled_headers.hpp"
#include "base_exception.hpp"
#include "exception_presenter.hpp"
#include "str_util.hpp"

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
			os << _CT("<catch_func>") << func_name << _CT("</catch_func>");
		}
	
		t_string base_exception::as_xml() const
		{
			t_stringstream os(_CT(""));

			os << _CT("<exception_info>");

			m_context.as_xml(os);			

			os << _CT("<original_exception_type>") << m_original_exception_type << _CT("</original_exception_type>");
			os << _CT("<transforming_function>") << m_transformation_function <<  _CT("</transforming_function>");
			
			os << _CT("<catch_stack>");
			std::for_each(m_catch_stack.begin(), m_catch_stack.end(), 
                boost::bind(&base_exception::print_catch_stack, boost::ref(os), _1));
			os << _CT("</catch_stack>");
			
			os << _CT("</exception_info>");

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
				ostrm << _CT("Original Exception Type:[") << m_original_exception_type << _CT("]\n"); 

			if(!m_transformation_function.empty() || output_empty )
				ostrm << _CT("Transformation function:[") << m_transformation_function << _CT("]\n");
			
            m_context.pretty_print(ostrm, print_option);

			if( !m_catch_stack.empty() || output_empty )
			{
				ostrm << _CT("\nCatch Stack:\n"); 
				std::for_each(m_catch_stack.begin(), m_catch_stack.end(), 
                    boost::bind(&base_exception::pretty_print_catch_stack, boost::ref(ostrm), _1, true));
			}
		}

		void base_exception::pretty_print_catch_stack(t_ostream& os, const t_string& func_name, bool insert_new_line )
		{
			if( insert_new_line )
				os << _CT("\t") << func_name << std::endl;
			else
				os << _CT("<=") << func_name << _CT("=>");
		}

		void base_exception::pretty_print_one_line(t_ostream& ostrm, print::option print_option) const
		{
			bool output_empty = (print_option & print::output_empty_fields) ? true : false;
			ostrm << _CT("Exception thrown :: ");
			m_context.pretty_print_one_line(ostrm, print_option);
			if( !m_catch_stack.empty() || output_empty )
			{
				ostrm << _CT(" Catch_stack:["); 
				std::for_each(m_catch_stack.begin(), m_catch_stack.end(), 
                    boost::bind(&base_exception::pretty_print_catch_stack, boost::ref(ostrm), _1, false));
				ostrm << _CT("]");
			}

			if( !m_original_exception_type.empty() || output_empty )
				ostrm << _CT(" Original Exception:[") << m_original_exception_type << _CT("]"); 

			if( !m_transformation_function.empty() || output_empty )
				ostrm << _CT(" Transformation function:[") << m_transformation_function << _CT("]");
		}

		user_reaction::type base_exception::show( const base_exception& e)
		{
			return show(e, _CT("base_exception Caught"));
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
