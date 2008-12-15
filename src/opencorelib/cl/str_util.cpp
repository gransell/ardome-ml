
#include "precompiled_headers.hpp"

#include <errno.h>

#include "./str_util.hpp"
#include "./string_conversions.hpp"
#include "./enforce_defines.hpp"
#include "./enforce.hpp"
#include "./machine_readable_errors.hpp"
#include <numeric>
#include <boost/bind.hpp>

#ifdef OLIB_ON_WINDOWS
	#include <comutil.h>
	#include <comdef.h>
#endif

#include "./assert.hpp"
#include "./assert_defines.hpp"

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>

namespace olib
{
	namespace opencorelib
	{
		t_string& str_util::trim(t_string& str, const t_string& chars_toTrim)
		{
			t_string::size_type l_pos;

			if( str.empty() )
				return str;

			l_pos = str.find_last_not_of(chars_toTrim);
			if( l_pos != t_string::npos )
			{
				if( l_pos + 1 < str.size() )
					str.erase(l_pos + 1, str.size() - l_pos - 1);
			}

			l_pos = str.find_first_not_of(chars_toTrim);
			if( l_pos == t_string::npos )
				return str;

			if( l_pos > 0 )
				str.erase(0, l_pos);

			return str;
		}

		bool match( size_t idx, const t_string& input, const t_string& delimiter )
		{
			if( idx + delimiter.size() >= input.size() ) return false;
			return input.compare(idx, delimiter.size(), delimiter) == 0;
		}

		bool escaped_quote( size_t idx, const t_string& input)
		{
			if( idx + 2 >= input.size() ) return false;
			if( input.compare(idx, 2, _T("\\\"")) == 0) return true;
			if( input.compare(idx, 2, _T("\\'")) == 0) return true;
			return false;
		}

		void str_util::split_string(	const t_string& input, 
										const t_string& delimiter, 
										std::vector<t_string>& results, 
										quote::action qa)
		{
			ARASSERT( delimiter.find(_T("\"")) == t_string::npos)(delimiter);
			ARASSERT( delimiter.find(_T("'")) == t_string::npos)(delimiter);

			size_t last_match = 0;
			std::list<TCHAR> quote_stack;
			for(size_t i = 0; i < input.size(); ++i )
			{
				if(  match(i, input, delimiter) && quote_stack.empty() ) 
				{
					if(i > last_match) results.push_back(input.substr(last_match, i - last_match));
					i += delimiter.size();
					last_match = i;
					while( match(i, input, delimiter) )
					{
						i += delimiter.size();
						last_match = i;
					}
				}

				if(qa == quote::respect)
				{
					if( input[i] == _T('"') || input[i] == _T('\'') && !escaped_quote(i, input) ) 
					{
						if(quote_stack.empty()) quote_stack.push_back(input[i]);
						else if( quote_stack.back() == input[i] ) quote_stack.pop_back();
						else quote_stack.push_back(input[i]);
					}
				}
			}

			if( last_match != input.size() ) 
				results.push_back(input.substr(last_match, input.size() - last_match));

			ARASSERT_MSG( quote_stack.empty(),"The stack should be empty here.");
		}

        CORE_API bool str_util::env_var_exists( const t_string& env_var_name )
        {
			#ifdef OLIB_ON_WINDOWS
				// getenv is deprecated on windows. replace with
				// _dupenv_s, for now, just disable warning.
				#pragma warning( push )
				#pragma warning( disable: 4996 )
			#endif
            
			return getenv( to_string(env_var_name).c_str()) != 0;
			
			#ifdef OLIB_ON_WINDOWS
				#pragma warning( pop )
			#endif
        }

        CORE_API t_string str_util::get_env_var( const t_string& env_var_name )
        {
            #ifdef OLIB_ON_WINDOWS
			static const boost::uint32_t env_max_size(32767);
                TCHAR buf [env_max_size];
                DWORD var_size(0);
                ARENFORCE_WIN( var_size = ::GetEnvironmentVariable( env_var_name.c_str(), buf, env_max_size) )(env_var_name); 
                return t_string(buf, var_size);
            #else
                char* e_val; 
                ARENFORCE( ( e_val = getenv( to_string(env_var_name).c_str()) ) )(env_var_name);
                std::string env_val(e_val);
                return to_t_string(env_val);
            #endif
        }

        CORE_API void str_util::set_env_var( const t_string& env_var_name, const t_string& val )
        {
            #ifdef OLIB_ON_WINDOWS
                ARENFORCE_WIN( ::SetEnvironmentVariable(env_var_name.c_str(), val.c_str()));
            #else
                t_format fmt(_T("%s=%s"));
                putenv( const_cast<char*>(to_string((fmt % env_var_name % val).str()).c_str()) );
            #endif
        }

        CORE_API void str_util::del_env_var( const t_string& env_var_name )
        {
            #ifdef OLIB_ON_WINDOWS
                ARENFORCE_WIN( ::SetEnvironmentVariable(env_var_name.c_str(), 0 ))(env_var_name);
            #else
                t_format fmt(_T("%s="));
                putenv( const_cast<char*>(to_string((fmt % env_var_name).str()).c_str()) );
            #endif
        }

		t_string str_util::expand_env_vars( const t_string& to_expand )
		{
			t_regex regex_env_var(_T("%(.*)%"));
			t_string res;

			t_string::const_iterator it_start(to_expand.begin()), it_end(to_expand.end()); 
			t_smatch what; 
			boost::match_flag_type flags = boost::match_default; 
			while(boost::regex_search(it_start, it_end, what, regex_env_var, flags)) 
			{
				res += t_string( it_start, what[0].first );
				t_string env_var_name(what[1]);
				t_string env_val = get_env_var(env_var_name);
				if( env_val.empty() ) res += env_var_name;
				else res += t_string( env_val );
				it_start = what[0].second;
			}

			if(it_start != it_end) res += t_string(it_start, it_end);
			return res;
		}

		#ifdef OLIB_ON_WINDOWS
        t_string str_util::to_t_string( const _bstr_t& source)
        {
            if( source.length() == 0) return t_string();
            return to_t_string(std::wstring(source));
        }

		_bstr_t str_util::to_bstr( const t_string& source)
		{
			return _bstr_t(source.c_str());
		}

        std::locale str_util::set_global_locale_to_thread_locale()
        {
            LCID id = ::GetThreadLocale();

            TCHAR buffer1[MAX_PATH];
            TCHAR buffer2[MAX_PATH];
            ::GetLocaleInfo(id,  LOCALE_SABBREVLANGNAME , buffer1, MAX_PATH);
            ::GetLocaleInfo(id,  LOCALE_SABBREVCTRYNAME , buffer2, MAX_PATH);

            t_string language(buffer1);
            t_string country(buffer2);
            t_string full_name = language + _T("_") + country;

            try
            {
                std::locale loc( to_string(full_name).c_str() );
                std::locale old = std::locale::global(loc);
                /*	We only want to use the global locale when new streams are
                created (by default it is used). But when we call locale::global
                it will call the setlocale c-function below, with the new
                locale. This gives very strange errors, such as outputting 
                600, 000 000 00 for the number 600000000.00 when a 
                swedish locale is used. When this number is reread it will 
                be interpreted as 600.00! So setting the c-locale back to 
                classic mode will ensure that this doesn't happen, at the 
                same time as we get the bennefit of a global locale that 
                will be used correctly in new streams. */
                setlocale(LC_ALL, std::locale::classic().name().c_str());
                return old;
            }
            catch( std::runtime_error re)
            {
                // Return an empty locale if creation fails.
                // The empty locale will forward all calls to the 
                // global and finally the classic locales.
                return  std::locale::empty();
            }
        }
		#endif // OLIB_ON_WINDOWS

		t_string str_util::to_t_string( const std::string& source)
		{
			#ifdef OLIB_USE_UTF16
				return string_conversions::from_utf8_to_utf16(source.c_str(), source.size());
			#else
				return source.c_str();
			#endif
		}

		t_string str_util::to_t_string( const std::wstring& source)
		{
			#ifdef OLIB_USE_UTF16
				return source;
			#else
				return string_conversions::from_utf16_to_utf8(source.c_str(), source.size());
			#endif
		}


		t_string str_util::to_t_string( const wchar_t* source, size_t length )
		{
            #ifdef OLIB_ON_WINDOWS
                if( ! source ) return t_string();
                return to_t_string( std::wstring(source, length) );
            #else
			    std::vector<wchar_t> wide_chars = string_conversions::unpack_packed_wide_string( source,
																							 length );
			    return to_t_string( std::wstring(&wide_chars[0], length) );
            #endif
		}

		t_string str_util::to_t_string( const wchar_t* source )
		{
            #ifdef OLIB_ON_WINDOWS
                if( ! source ) return t_string();
                return to_t_string( std::wstring(source) );
            #else
			    std::vector<wchar_t> wide_chars = string_conversions::
					unpack_packed_wide_string( source, wcslen(source) ) );
			    return to_t_string( std::wstring(&wide_chars[0], wide_chars.size() - 1) );
            #endif
		}

		t_string str_util::to_t_string( const char* source)
		{
			return to_t_string(std::string(source));
		}

        std::wstring str_util::to_wstring( const wchar_t* source, size_t length )
		{
			return std::wstring(source, length);
		}

		std::wstring str_util::to_wstring( const wchar_t* source )
		{
			return std::wstring(source);
		}

		std::wstring str_util::to_wstring(const std::string& source)
		{
			// From String_conversions.h
			return string_conversions::from_utf8_to_utf16(source.c_str(), source.size());
		}

		const std::wstring& str_util::to_wstring(const std::wstring& source)
		{
			return source;
		}

		std::string str_util::to_string(const std::wstring& source)
		{
			// From String_conversions.h
			return string_conversions::from_utf16_to_utf8(source.c_str(), source.size());
		}

		const std::string& str_util::to_string(const std::string& source)
		{
			return source;
		}

        std::string str_util::to_string( const wchar_t* source, size_t length )
		{
			return string_conversions::from_utf16_to_utf8( source, length );
		}

		std::string str_util::to_string( const wchar_t* source )
		{
			return string_conversions::from_utf16_to_utf8( source, wcslen(source) );
		}

		t_ostream& str_util::any_to_t_string(t_ostream& os, const boost::any& v)
		{
			if (const int * vv = boost::any_cast<int>(&v)) 
			{
				os << *vv;
			} 
			else if (const double * vv = boost::any_cast<double>(&v)) 
			{
				os << *vv;
			} 
			else if (const float* vv = boost::any_cast<float>(&v)) 
			{
				os << *vv;
			} 
			else  if (const boost::int32_t * vv = boost::any_cast<boost::int32_t>(&v)) 
            {
                os << *vv;
            }
            else if (const long * vv = boost::any_cast<long>(&v)) 
            {
                os << *vv;
            }
            else if (const boost::int64_t * vv = boost::any_cast<boost::int64_t>(&v)) 
            {
                os << *vv;
            }
            else if (const std::string* vv = boost::any_cast<std::string>(&v)) 
			{
				os << opencorelib::str_util::to_t_string(*vv);
			}
			else if (const std::wstring* vv = boost::any_cast<std::wstring>(&v)) 
			{
				os << opencorelib::str_util::to_t_string(*vv);
			}
			else if (const t_string* vv = boost::any_cast<t_string>(&v)) 
			{
				os << *vv;
			}
			else if (const bool* vv = boost::any_cast<bool>(&v)) 
			{
				os << *vv;
			}
			else if (const std::vector<double> * vv = boost::any_cast<std::vector<double> >(&v)) 
			{
				for( size_t i = 0; i < vv->size(); ++i )
				{
					os << (*vv)[i];
					if( i + 1 !=  vv->size() ) os << _T(",");
				}
			} 
			else 
			{
				ARENFORCE_MSG(false, "Not a convertible type");
			}
			
			return os;
		}

        t_ostream& str_util::operator<<(t_ostream& os, const boost::any& v)
        {
            return any_to_t_string(os, v);
        }

		std::locale str_util::current_locale()
		{
			// Return an empty locale if creation fails.
			// The empty locale will forward all calls to the 
			// global and finally the classic locales.
			return  std::locale();
		}

		t_format str_util::argument_insensitive_format(const t_string& fstring)
		{
			using namespace boost::io;
			t_format fmter(fstring);
			fmter.exceptions( all_error_bits ^ ( too_many_args_bit | too_few_args_bit )  );
			return fmter;
		}

		boost::int64_t str_util::parse_int64( const t_string& to_parse, int base)
		{
			ARENFORCE_ERR( base >= 2 && base <= 36, olib::error::invalid_parameter_value() );

			errno = 0;
			std::string str( to_string(to_parse));
			char* end_of_array = 0;
			
			boost::int64_t v =
			#ifdef OLIB_ON_WINDOWS
					_strtoi64( &str[0], &end_of_array, base );
			#else
					strtoll( &str[0], &end_of_array, base );
			#endif

			ARENFORCE_MSG_ERR( end_of_array == &str[0] + str.size(), 
					"Did not manage to parse whole string: %s. Parsed until: %s ",
								olib::error::parse_error() )(to_parse)(end_of_array);

			ARENFORCE_MSG_ERR(errno != ERANGE, 
				"Parsed value: %i is out-of-range for boost::int64_t",
				olib::error::value_out_of_range() )(to_parse); 

			return v;
		}

		boost::uint64_t str_util::parse_unsigned_int64( const t_string& to_parse, int base)
		{
			ARENFORCE_ERR( base >= 2 && base <= 36, olib::error::invalid_parameter_value() );

			errno = 0;
			std::string str( to_string(to_parse));
			char* end_of_array = 0;

			for( size_t i = 0; i < str.size(); ++i )
				ARENFORCE_MSG_ERR( std::isdigit(str[i], std::locale()),
									"Parameter contains non-digit chars.", 
										olib::error::invalid_parameter_value())(to_parse);
		
			boost::uint64_t v =
			#ifdef OLIB_ON_WINDOWS
				_strtoui64( &str[0], &end_of_array, base );
			#else
				strtoull( &str[0], &end_of_array, base );
			#endif

			ARENFORCE_MSG_ERR( end_of_array == &str[0] + str.size(), 
				"Did not manage to parse whole string: %s. Parsed until: %s ",
								olib::error::parse_error() )(to_parse)(end_of_array);

			ARENFORCE_MSG_ERR(errno != ERANGE, 
								"Parsed value: %i is out-of-range for boost::uint64_t",
								olib::error::value_out_of_range() )(to_parse); 
			
			return v;
		}
	}
}
