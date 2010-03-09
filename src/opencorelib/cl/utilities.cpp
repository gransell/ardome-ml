#include "precompiled_headers.hpp"

#include "string_defines.hpp"

#include "utilities.hpp"
#include "str_util.hpp"


#include <cctype>

#ifdef OLIB_ON_WINDOWS
	#include <wtypes.h>
	#include "msw/enum_process_modules.hpp"
#endif

#include "base_exception.hpp"
#include "enforce.hpp"
#include "enforce_defines.hpp"
#include "assert.hpp"
#include "assert_defines.hpp"
#include "machine_readable_errors.hpp"

// wx_widgets stackwalk implementation
#ifndef OLIB_ON_LINUX
#	include "stack_walker.hpp"
#endif

#include <algorithm>

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
	#pragma warning ( push )
	#pragma warning (disable:4512) // assignment operator could not be generated
	// Stackwalk print causes this
	#pragma warning ( disable:4311 ) // 'type cast' : pointer truncation from 'void *' to 'unsigned long'
#endif

//#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

#ifdef OLIB_ON_MAC
	#include "mac/stack_dump.hpp"
#endif

namespace olib
{
	namespace opencorelib
	{
		library_info::library_info( const t_path& fn) 
			: m_filename(fn)
		{
		}

		const t_path& library_info::get_filename() const
		{
			return m_filename;
		}

		void library_info::set_filename( const t_path& fn )
		{
			m_filename = fn;
		}

		const t_string& library_info::get_version() const
		{
			return m_version;
		}

		void library_info::set_version( const t_string& ver )
		{
			m_version = ver;
		}

		const t_string& library_info::get_company() const
		{
			return m_company;
		}

		void library_info::set_company( const t_string& comp )
		{
			m_company = comp;
		}

		const t_string& library_info::get_product() const
		{
			return m_product;
		}

		void library_info::set_product( const t_string& prod )
		{
			m_product = prod;
		}

		const t_string& library_info::get_build_number() const
		{
			return m_build_nr;
		}

		void library_info::set_build_number( const t_string& build_nr )
		{
			m_build_nr = build_nr;
		}

		CORE_API t_ostream& operator<<( t_ostream& os, const library_info& info )
		{
			os << info.m_version << _CT("\t") << info.m_build_nr << _CT("\t") 
				<< info.m_filename << _CT("\t") << info.m_company << _CT("\t") << info.m_product;
			return os;
		}


		#ifndef OLIB_ON_LINUX
        class stack_dump : public olib::opencorelib::stack_walker
        {
        public:
            stack_dump() { }
            t_string get_stack_trace() const { return m_stack_trace.str(); }

        protected:
            virtual void OnStackFrame(const olib::opencorelib::stack_frame& frame)
            {
                m_stack_trace << ( t_format(_CT("[%02d] ")) % frame.GetLevel() ).str();

                t_string name = frame.GetName();
                if ( !name.empty() )
                {
                    m_stack_trace << ( t_format(_CT("%-40s")) % name ).str();
                }
                else
                {
                    m_stack_trace << (t_format( _CT("0x%08lx")) % 
                        (unsigned long)frame.GetAddress() ).str();
                }

                if ( frame.HasSourceLocation() )
                {
                    m_stack_trace	<< (_CT("\t"))
                        << frame.GetFileName()
                        << (_CT(":"))
                        << (static_cast<int>(frame.GetLine()));
                }

                m_stack_trace << (_CT("\n"));
            }

        private:
            stack_dump( const stack_dump& );
            stack_dump& operator=( const stack_dump& );
            t_stringstream m_stack_trace;
        };
		#endif

		namespace utilities
		{
			#ifdef OLIB_ON_WINDOWS
			t_string handle_system_error(bool show_message_box)
			{
				LPVOID lp_msg_buf;
                DWORD l_error_nr = ::GetLastError();
                if (!::FormatMessage( 
					FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					l_error_nr,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
					(LPTSTR)&lp_msg_buf,
					0,
					NULL ))
				{
					// Handle the error.
					t_stringstream l_strstrm;
					l_strstrm << _CT("handle_system_error:\n Unable to format the error message.\n The error number was: ") 
						<< l_error_nr;
					return l_strstrm.str();
				}

				// Display the string if the caller wants to.
				if(show_message_box)
                    ::MessageBox( NULL, (LPCTSTR)lp_msg_buf, _CT("System Error"), MB_OK | MB_ICONINFORMATION );

				t_string l_ret_val(static_cast<LPCTSTR>(lp_msg_buf));

                ::LocalFree( lp_msg_buf );
				return l_ret_val;
			}

			boost::int32_t get_last_error_from_os()
			{
				boost::int32_t error_nr = ::GetLastError();
				return error_nr;
			}

			t_string hresult_to_string( long hr )
			{
				LPVOID lp_msg_buf;
                if (!::FormatMessage( 
					FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					hr,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
					(LPTSTR)&lp_msg_buf,
					0,
					NULL ))
				{	
					return t_string(_CT(""));
				}

				t_string ret_val(static_cast<LPCTSTR>(lp_msg_buf));

                ::LocalFree( lp_msg_buf );
				return ret_val;
			}
			#endif // OLIB_ON_WINDOWS

			t_string strip_path_from_file( const t_string& path_and_file )
			{
				t_string cpy(path_and_file);
				
				// Remove all forward slashes, replace with backslash.
				boost::replace_all(cpy, _CT("/"), _CT("\\"));
				t_string::size_type f = cpy.find_last_of(_CT("\\"));
				if( f != t_string::npos ) return cpy.erase(0, f);
				return cpy;
			}

            void make_sure_path_exists( const olib::t_path& ph )
            {
		try
		{
                	if (ph.empty() || boost::filesystem::exists(ph)) return; 
                	make_sure_path_exists(ph.parent_path()); 
                	boost::filesystem::create_directory(ph); 
		}
		catch( boost::filesystem::basic_filesystem_error<olib::t_path> &exception)
		{
			std::wcerr << "exists exits: " << exception.what( ) << std::endl;
		}

			}

			t_string get_stack_trace( int skip_levels )
			{
				t_string stack_trace;
                
                #ifdef OLIB_ON_WINDOWS
				    stack_dump dump;
                    #undef max
				    skip_levels = std::max(1, skip_levels);

				    dump.Walk(skip_levels); 
				    stack_trace = dump.get_stack_trace();
                #elif defined( OLIB_ON_MAC )
				    stack_trace = str_util::to_t_string( mac::get_stack_trace() );
                #endif

				return stack_trace;	
			}
             
            CORE_API boost::uint64_t get_current_thread_id() 
            {
                #ifdef OLIB_ON_WINDOWS
                    return ::GetCurrentThreadId();
                #elif OLIB_ON_LINUX
                    return boost::uint64_t(pthread_self());
                #else
                    return reinterpret_cast<boost::uint64_t>(pthread_self());
                #endif
            }

            timer::timer()
            {
                m_start = boost::get_system_time();
            }

            boost::posix_time::time_duration timer::elapsed()
            {
                return boost::get_system_time() - m_start;
            }

            t_string make_smb_path( const t_string& host,
                                    const t_string& base_url, 
                                    const t_string& file_name )
            {
                ARENFORCE_MSG(!base_url.empty(), "empty base url");
                t_string res = host + t_string( _CT("\\") ) + base_url + t_string( _CT("\\") ) + file_name;
                
                t_regex rex(_CT("\\\\+"));
                t_string replace_with(_CT("\\"));
                
                boost::replace_all(res, _CT("/"), _CT("\\")); // forward to backward slash
                res = boost::regex_replace( res, rex, replace_with ); // no double slashes
                boost::erase_all(res, _CT("smb:")); // remove any smb identifier used on mac/unix.
                
                if(res.size() > 0 && res[0] == _CT('\\') ) 
                    res = t_string(_CT("\\")) + res; // Two backward slashes at the start. One already there
                else
                    res = t_string(_CT("\\\\")) + res; // Two backward slashes at the start

                boost::replace_all(res, _CT("\\"), _CT("/"));
                return t_string(_CT("smb:")) + res;

            }

            t_string make_protocol_path(    const t_string& user,
                                            const t_string& password,
                                            const t_string& host,
                                            const t_string& port,
                                            const t_string& base_url, 
                                            const t_string& file_name,
                                            const t_string& protocol_prefix  )
            {
                t_string res; //( base_url + t_string(_CT("/") + file_name ));

                if( user.size() > 0 || password.size() > 0 )
                    res += user + _CT(":") + password + _CT("@");

                res += host;
                if( !port.empty() ) res += _CT(":") + port;
                res += t_string(_CT("/")) + base_url + t_string(_CT("/")) + file_name;

				t_regex rex(_CT("/+"));
				t_string replace_with(_CT("/"));

                boost::replace_all(res, _CT("\\"), _CT("/")); // backward slash to forward
                res = boost::regex_replace( res, rex, replace_with ); // no double slashes

                boost::erase_all(res, protocol_prefix ); // remove any protocol identifier (like http:)

                if(res.size() > 0 && res[0] == _CT('/') ) 
                    return protocol_prefix + t_string(_CT("/")) + res; // Two backward slashes at the start. One already there
                else
                    return protocol_prefix + t_string(_CT("//")) + res; // Two backward slashes at the start
            }

            CORE_API void add_xml_header_utf8( std::ostream& os )
            {
                os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
            }

			
            CORE_API std::vector< library_info_ptr > get_loaded_libraries()
            {
                #ifdef OLIB_ON_WINDOWS
                        return win::enum_process_modules();
                #else
                        return std::vector< library_info_ptr >();
                #endif

            }

			CORE_API multivalue_property_map parse_multivalue_property( const t_string &prop_str)
            {
                typedef boost::char_separator< TCHAR > charsep_type;
                typedef boost::tokenizer< charsep_type, t_string::const_iterator, t_string > tok_type;
                multivalue_property_map result;

                charsep_type pipe_tok_func( _CT("|") );
                tok_type pipe_tokenizer( prop_str, pipe_tok_func );

                tok_type::const_iterator pipe_iter;
                for( pipe_iter=pipe_tokenizer.begin(); pipe_iter!=pipe_tokenizer.end(); pipe_iter++ )
                {
                    size_t equals_pos = pipe_iter->find( _CT("=") );
                    ARENFORCE_MSG_ERR( equals_pos != t_string::npos, 
                        _CT("Missing \"=\". A multivalue property string component must be of the form \"key=value\"." ), 
                        olib::error::parse_error() );
                    ARENFORCE_MSG_ERR( equals_pos > 0, 
                        _CT("Zero-length key. A multivalue property string component must be of the form \"key=value\"." ), 
                        olib::error::parse_error() );
                   
                    t_string key = pipe_iter->substr(0, equals_pos);
                    t_string value = pipe_iter->substr(equals_pos+1);

                    ARENFORCE_MSG_ERR( value.find(_CT("=")) == t_string::npos, 
                        _CT("Multiple \"=\" found.A multivalue property string component must be of the form \"key=value\"." ),
                        olib::error::parse_error() );

                    //Insert the key-value pair into the result map
                    result.insert( make_pair(key, value) );

                    //std::cout << "KV Pair: " << pipe_iter->substr(0, equals_pos) << "=" << pipe_iter->substr(equals_pos+1) << std::endl;
                }

                return result;
            }
		}
	}
}

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
	#pragma warning ( pop )
#endif
