#include "precompiled_headers.hpp"

#ifdef __AUWIN__
#pragma warning (disable:4512)
#endif
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

#include <algorithm>

#include "exception_context.hpp"
#include "utilities.hpp"
#include "str_util.hpp"

namespace
{
    template< typename CharType >
    olib::t_string presentation_string( const CharType *str )
    {
        if (str == NULL)
            return _CT("( NULL )");
        else
            return olib::opencorelib::str_util::to_t_string( str );
    }
}

namespace olib
{
    namespace opencorelib
    {
        void ec_add_value(exception_context& ec, const char* exp,  const char* obj)
        {
            ec.add_value( exp, presentation_string( obj ) );
        }

        void ec_add_value(exception_context& ec, const char* exp,  const wchar_t* obj)
        {
            ec.add_value( exp, presentation_string( obj ) );
        }

        void ec_add_value( exception_context& ec, const char* exp, const std::wstring& obj)
        {
            ec.add_value( exp, str_util::to_t_string(obj));
        }

        void ec_add_value( exception_context& ec, const char* exp, const std::string& obj)
        {
            ec.add_value( exp, str_util::to_t_string(obj));
        }

        void ec_add_value_to_message( exception_context& ec, const char* obj )
        {
            ec.add_value_to_message( presentation_string( obj ) );
        }

        void ec_add_value_to_message( exception_context& ec, const wchar_t* obj )
        {
            ec.add_value_to_message( presentation_string( obj ) );
        }

        void ec_add_value_to_message( exception_context& ec, const std::wstring& obj)
        {
            ec.add_value_to_message( str_util::to_t_string(obj));
        }

        void ec_add_value_to_message( exception_context& ec, const std::string& obj)
        {
            ec.add_value_to_message( str_util::to_t_string(obj));
        }

		exception_context::exception_context() : m_hr(0), m_line_nr(0)
		{

		}

        exception_context::exception_context(  const char* filename, long linenr, 
                                                 const char* funcdname, const char* exp, const TCHAR* msg ) 
                            : m_hr(0), m_message(msg)
        {
            add_context(filename, linenr, funcdname, exp );
        }

		exception_context::exception_context(	const char* filename, long linenr, 
												const t_string& source, const t_string& targetsite,
												const char* exp) 
			: m_hr(0), m_line_nr( linenr ), m_source(source), m_target_site(targetsite)
		{
			m_expression = str_util::to_t_string(exp);
			m_file_name = utilities::strip_path_from_file( str_util::to_t_string(filename) );
		}

		exception_context::exception_context(	const char* filename, long linenr, 
												const t_string& source, const t_string& targetsite, 
												const char* exp, const t_string& message)
            : m_hr(0), m_line_nr( linenr ), m_message(str_util::argument_insensitive_format(message)),
              m_source(source), m_target_site(targetsite)
		{
			m_expression = str_util::to_t_string(exp);
			m_file_name = utilities::strip_path_from_file( str_util::to_t_string(filename) );
		}

		exception_context::exception_context(	const char* filename, long linenr, 
												const t_string& source, const t_string& targetsite, 
                                                const char* exp, const t_string& message,
                                                const t_string& callstack)
            : m_call_stack(callstack), m_hr(0), m_line_nr( linenr ),
              m_message( str_util::argument_insensitive_format(message)),
              m_source(source), m_target_site(targetsite)  
            
		{
			m_expression = str_util::to_t_string(exp);
			m_file_name = utilities::strip_path_from_file( str_util::to_t_string(filename) );
		}

		exception_context::exception_context(	const char* filename, long linenr, 
												const t_string& source, const t_string& targetsite, 
												const char* exp, const TCHAR* message)
			: m_hr(0), m_line_nr( linenr ), m_message(message), m_source(source), m_target_site(targetsite)
		{
			m_expression = str_util::to_t_string(exp);
			m_file_name = utilities::strip_path_from_file( str_util::to_t_string(filename) );
		}


		exception_context::exception_context(	const char* filename, long linenr, 
												const t_string& source, const t_string& targetsite,
												const char* exp, long hresult) 
			: m_hr(hresult), m_line_nr( linenr ), m_source(source), m_target_site(targetsite)
		{
			m_expression = str_util::to_t_string(exp);
			m_file_name = utilities::strip_path_from_file( str_util::to_t_string(filename) );
		}

		exception_context::exception_context(	const char* filename, long linenr, 
												const t_string& source, const t_string& targetsite, 
												const char* exp, long hresult, const t_string& message)
            : m_hr(hresult), m_line_nr( linenr ),
              m_message(str_util::argument_insensitive_format(message)),
              m_source(source), m_target_site(targetsite)
		{
			m_expression = str_util::to_t_string(exp);
			m_file_name = utilities::strip_path_from_file( str_util::to_t_string(filename) );
		}

		exception_context::exception_context(	const char* filename, long linenr, 
												const t_string& source, const t_string& targetsite, 
												const char* exp, long hresult, const TCHAR* message)
            : m_hr(hresult), m_line_nr( linenr ),
              m_message(str_util::argument_insensitive_format(str_util::to_t_string(message))),
              m_source(source), m_target_site(targetsite)
                                                    
		{
			m_expression = str_util::to_t_string(exp);
			m_file_name = utilities::strip_path_from_file( str_util::to_t_string(filename) );
		}


		exception_context::~exception_context()
		{

		}

        exception_context::exception_context( const exception_context& ec)
            : m_call_stack(ec.m_call_stack), m_error_reason( ec.m_error_reason ),
              m_expression( ec.m_expression ), m_file_name( ec.m_file_name), 
              m_hr( ec.m_hr), m_line_nr(ec.m_line_nr), m_message( ec.m_message ),
              m_name_value_vec( ec.m_name_value_vec),
              m_source(ec.m_source), m_target_site(ec.m_target_site)
		{
		}

		exception_context& exception_context::operator=(const exception_context& ec)
		{
			exception_context tmp = exception_context(ec);
			swap(tmp);
			return *this;
		}

		void exception_context::swap( exception_context& other) throw()
		{
			m_name_value_vec.swap(other.m_name_value_vec);
			m_message.swap(other.m_message);
			m_file_name.swap(other.m_file_name);
			m_source.swap(other.m_source);
			m_target_site.swap(other.m_target_site);
			m_expression.swap(other.m_expression);
			m_call_stack.swap(other.m_call_stack);
			m_error_reason.swap(other.m_error_reason);
			std::swap(m_hr, other.m_hr);
			std::swap(m_line_nr, other.m_line_nr);
		}

		void exception_context::add_context( const char* filename, long linenr, 
                                            const char* funcdname, const char* exp)
		{
			try
			{
				
				m_file_name = utilities::strip_path_from_file( str_util::to_t_string(filename) );

                #ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
                    // This regex matches strings like:
                    // ?Test_simple_assert@
                    // produced by Visual C++ compiler defines __FUNCDNAME__.
                    #define REGEX_FUNCTION_NAME_PATTERN _CT("\\?([^?0].*?)@")

                    // Matches repeated NS1@ in NS1@NS2@NS3
                    // Don't match a @ at the start since this means
                    // that we have reached the end of the namespace sequence.
                    #define REGEX_NAMESPACE_PATTERN _CT("^([^@].*?)@")
                    
                    // __FUNCDNAME__ in a constructor looks somewhat different:
                    //  ??0My_dummy_class@@QAE@XZ
                    // This regexp matches that type.
                    #define REGEX_CONSTRUCTOR_PATTERN _CT("\\?\\?0(.*?)@")

                static const t_regex regex_function_name(REGEX_FUNCTION_NAME_PATTERN);
                static const t_regex regex_namespace(REGEX_NAMESPACE_PATTERN);
                static const t_regex regex_constructor(REGEX_CONSTRUCTOR_PATTERN);

                t_smatch what; 
                t_string fd_name(str_util::to_t_string(funcdname));
                t_string::const_iterator it_start(fd_name.begin()), it_end(fd_name.end()); 
                boost::match_flag_type flags = boost::match_default; 
                if( boost::regex_search(it_start, it_end, what, regex_function_name, flags) ||
                    boost::regex_search(it_start, it_end, what, regex_constructor, flags) ) 
                {
                    m_target_site = what[1];
                    it_start = what[0].second;
                    while(boost::regex_search(it_start, it_end, what, regex_namespace, flags)) 
                    {
                        if(m_source.empty()) m_source = what[1];
                        else m_source = what[1] + _CT("::") + m_source;
                        it_start = what[0].second;
                    }
                }
                else
                {
                    // Should never end up here!
                    m_source = fd_name;
                }	
                #elif defined( OLIB_ON_MAC ) || defined( OLIB_ON_LINUX )
				m_source = str_util::to_t_string(funcdname); 
				#endif

				if(exp) m_expression = str_util::to_t_string(exp);
				m_line_nr = linenr;
			}
			catch (...) 
			{
				// Don't do anything, ignore exceptions here
				// since we are going to throw one ourselves 
				// very soon
			}

		}
		
		void exception_context::add_value( const char* exp, const t_string& val )
        {
            try
            {
                t_string expression = str_util::to_t_string(exp);
                m_name_value_vec.push_back(std::make_pair(expression, val));
            }
            catch(...)
            {
                // Don't do anything, just ignore.
            }
        }

		void exception_context::call_stack( const t_string& str)
		{
			// Remove the three first lines and the last line
			m_call_stack = str;
		}

		t_string exception_context::call_stack() const
		{
			return m_call_stack;
		}


		long exception_context::h_result() const
		{
			return m_hr;
		}

		void exception_context::h_result( long hr)
		{
			m_hr = hr;
		}

		void exception_context::error_reason( const t_string& er)
		{
			m_error_reason = er;
		}

		t_ostream& operator<< ( t_ostream& os, exception_context& exp)
		{
			return exp.as_xml(os);
		}

		void exception_context::print_value_pair( const std::pair< t_string, t_string >& p, t_ostream& ss)
		{
			ss << _CT("<name_value_pair name='") << p.first << _CT("' value='") << p.second << _CT("' />");
		}

		void exception_context::pretty_print_value_pair( const std::pair< t_string, t_string >& p, t_ostream& ss)
		{
			ss <<  _CT("\t") << p.first << _CT(" = ") << p.second << std::endl;
		}

		void exception_context::pretty_print_value_pair_one_line( const std::pair< t_string, t_string >& p, t_ostream& ss)
		{
			ss << _CT("[") << p.first << _CT(" = ") << p.second << _CT("]");
		}

		t_ostream& exception_context::as_xml( t_ostream& ss) const
		{
			ss << _CT("<exception_context>"); 
			ss << _CT("<file>") << m_file_name << _CT("</file>");
			ss << _CT("<line_nr>") <<  m_line_nr << _CT("</line_nr>");
			ss << _CT("<expression>") << m_expression << _CT("</expression>");
			ss << _CT("<source>") << m_source << _CT("</source>");
			ss << _CT("<target_site>") << m_target_site << _CT("</target_site>");
			ss << _CT("<message>") << m_message << _CT("</message>");
			ss << _CT("<hresult>") << m_hr << _CT("</hresult>");
			ss << _CT("<machine_error>") << m_error_reason << _CT("</machine_error>");

			ss << _CT("<context_values>");
			std::for_each(m_name_value_vec.begin(), m_name_value_vec.end(), 
                boost::bind(&exception_context::print_value_pair, _1, boost::ref(ss)));
			ss << _CT("</context_values>");
			ss << _CT("<callstack>");
			ss << m_call_stack;
			ss << _CT("</callstack>");

			ss << _CT("</exception_context>");
			return ss;
		}

		t_string exception_context::as_xml() const
		{
			t_stringstream l_ss;
			as_xml(l_ss);
			return l_ss.str();
		}

		t_ostream& exception_context::pretty_print( t_ostream& os, print::option p_option) const
		{
			bool out_empty = p_option == print::output_empty_fields ? true : false;
			
            if((p_option & print::output_message ))
            {
                os << _CT("Message: ") << m_message << _CT("\n\n");
            }

            if((p_option & print::output_expression) && (!m_expression.empty() || out_empty) )
            {
                os << _CT("Tested expression: ") << m_expression << _CT("\n\n");
            }

            if( (p_option & print::output_file_and_line ) )
            {
			    os << _CT("Context :: \n\t_file:\t\t") << m_file_name
                   << _CT("\n\t_line:\t\t") << m_line_nr << _CT("\n");
            }

            if( (p_option & print::output_source ) ) 
            {
                os << _CT("\t_source:\t\t") << m_source;
#ifdef OLIB_ON_WINDOWS
				os << _CT("::") << m_target_site;
#endif
				os << _CT("\n\n");
            }
			
			if( (p_option & print::output_machine_error ) && (!m_error_reason.empty() || out_empty) ) 
            {
                os << _CT("Machine Readable Error: ") << m_error_reason << _CT("\n\n");
            }
            
            #ifdef OLIB_ON_WINDOWS
			if( (p_option & print::output_hr ) && (m_hr != 0 || out_empty )) 
            {
                os << _CT("HRESULT: ") << m_hr << _CT(", ")
                   << utilities::hresult_to_string(m_hr) << _CT("\n\n");
            }
            #endif
			
            if( (p_option & print::output_name_values) && (!m_name_value_vec.empty() || out_empty) ) 
            {
                os << _CT("Recorded name value pairs: ") << _CT("\n");
                std::for_each(m_name_value_vec.begin(), m_name_value_vec.end(), 
                    boost::bind(&exception_context::pretty_print_value_pair, _1, boost::ref(os)));
				os << _CT("\n\n");
            }
		
			if( (p_option & print::output_callstack )&& (!m_call_stack.empty() || out_empty) ) 
            {
                os << _CT("Callstack: ") << _CT("\n") << m_call_stack;
            }

			os.flush();
			return os;
		}
        
		t_string exception_context::pretty_print(print::option p_option) const
		{
			t_stringstream l_ss;
			pretty_print(l_ss, p_option);
			return l_ss.str();
		}

		t_ostream& exception_context::pretty_print_one_line( t_ostream& os, print::option p_option) const
		{
			bool out_empty = (p_option & print::output_empty_fields) ? true : false;
            if( (p_option & print::output_expression) && (!m_expression.empty() || out_empty) )
                os << _CT("< Expr: '") << m_expression << _CT("'>");
            
            if((p_option & print::output_message ))
                os << m_message << _CT(" ");
            
            if( (p_option & print::output_name_values) && (!m_name_value_vec.empty() || out_empty) ) 
			{
				os << _CT("<");
				std::for_each(m_name_value_vec.begin(), m_name_value_vec.end(), 
                    boost::bind(&exception_context::pretty_print_value_pair_one_line, _1, boost::ref(os)));
				os << _CT(">");
			}

            #ifdef OLIB_ON_WINDOWS
            if( (p_option & print::output_hr ) && (m_hr != 0 || out_empty ))
			{
				t_string hr_str = utilities::hresult_to_string(m_hr);
				boost::trim(hr_str);
				os << _CT("< HR: ") << m_hr << _CT(", ") << hr_str << _CT(" >");
			}
            #endif

            if( ((p_option & print::output_callstack)) && (!m_call_stack.empty() || out_empty))
			{
				t_string tmp_callstack = m_call_stack;
				boost::replace_all( tmp_callstack, _CT("\n"), _CT("  "));
				os << _CT(" Callstack:[ ");
				os << tmp_callstack << _CT(" ]");
			}

            if( (p_option & print::output_source ) ) 
            {
                os << _CT("<") << m_source;
#ifdef OLIB_ON_WINDOWS
				os << _CT("::") << m_target_site;
#endif
				os << _CT(">");
            }

            if( (p_option & print::output_file_and_line ) )
			{
                os << _CT("<") << m_file_name << _CT(":") << m_line_nr << _CT(">");
            }

            if( (p_option & print::output_machine_error ) && (!m_error_reason.empty() || out_empty) )
            {   
				os << _CT("<Machine error: ") << m_error_reason << _CT(">");
			}

			return os;
		}

		t_string exception_context::message() const
		{
			return m_message.str();
		}

        void exception_context::message(const std::string& str)
        {
            m_message = str_util::argument_insensitive_format( str_util::to_t_string(str));
        }

        void exception_context::message(const std::wstring& str)
        {
            m_message = str_util::argument_insensitive_format( str_util::to_t_string(str));
        }

        void exception_context::message(const char* str)
        {
            m_message = str_util::argument_insensitive_format( str_util::to_t_string(str));
        }

        void exception_context::message(const wchar_t* str)
        {
            m_message = str_util::argument_insensitive_format( str_util::to_t_string(str));
        }

		const t_string& exception_context::filename() const
		{
			return m_file_name;
		}

		const t_string& exception_context::source() const
		{
			return m_source;
		}

		void exception_context::source( const t_string& s)
		{
			m_source = s;
		}

		const t_string& exception_context::target_site() const
		{
			return m_target_site;
		}

		const t_string& exception_context::expression() const
		{
			return m_expression;
		}

		const t_string exception_context::error_reason() const
		{
			return m_error_reason;
		}

		long exception_context::line_nr() const
		{
			return m_line_nr;
		}

	}

}
