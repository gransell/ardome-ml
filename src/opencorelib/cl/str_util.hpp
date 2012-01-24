#ifndef _CORE_STRUTIL_H_
#define _CORE_STRUTIL_H_

#include <locale>
#include <boost/any.hpp>
#include "./string_defines.hpp"

#ifdef OLIB_ON_WINDOWS
    class _bstr_t;
#endif


namespace olib
{
	namespace opencorelib
	{
		/// Streams a CString to a t_ostream. Normally the pointer value would only be shown.
		#ifdef __AFXWIN_H__
		inline olib::t_ostream& operator<<(olib::t_ostream& os, const CString& str)
		{
			CString& ref = const_cast<CString&>(str);
			os << olib::t_string(ref.GetBuffer());
			return os;
		}
		#endif

		/// Should quotes be respected or not when parsing a string.
		namespace quote
		{
			enum action {
                            respect,    /**< Respect quotes when parsing */
                            ignore      /**< Ignore quotes when parsing */
                        };
		}


		///A namespace dedicated to useful helper functions for t_strings
	    /** @author Mats Lindel&ouml;f */
		namespace str_util
		{	
			/// Compares two strings case insensitively
			/*! @param s1 The string that will be compared to s2.
				@param s2 The string will that will be compared to s1.
				@return -1 if s1 is lexicographically before s2, 0 if equal and 1 if s1 is after s2 lexicographically.*/
			template <class T>
			static int compare_nocase(const T& s1, const T& s2, const std::locale& loc = std::locale())
			{
				typename T::const_iterator it1 = s1.begin();
				typename T::const_iterator it2 = s2.begin();

				while( it1 != s1.end() && it2 != s2.end() )
				{
					if(std::toupper(*it1, loc) != std::toupper(*it2, loc))
						return std::toupper(*it1, loc) < std::toupper(*it2, loc) ? -1 : 1;
					++it1;
					++it2;
				}

				return (s1.size() == s2.size()) ? 0 : (s1.size() < s2.size()) ? -1 : 1;
			}

			/// Removes leading and trailing characters in chars_toTrim.
			/*!	The second parameter can be omitted and trim then trims leading and trailing whites paces.
				@param str The string that will be manipulated.
				@param chars_toTrim string with characters that will be trimmed from the start and end of str.*/
			CORE_API t_string& trim(t_string& str, const t_string& chars_toTrim = _CT(" \t\n"));

			/// Split an input string into tokens.
			/** @param input The string to tokenize.
				@param delimiter The delimiter between tokens.
				@param results The resulting token array.
				@param qa Should quotes turn the tokenizer of until a new quote is seen?
				@return Do not use the result! */
			CORE_API void split_string(	const t_string& input, 
											const t_string& delimiter, 
											std::vector<t_string>& results, 
											quote::action qa);

			/// Convert a std::string to a t_string (might not be a conversion)
			CORE_API t_string to_t_string( const std::string& source);
			
			/// Convert a std::wstring to a t_string (might not be a conversion)
			CORE_API t_string to_t_string( const std::wstring& source);

			/// Convert an array of shorts to a t_string 
            /** On mac this means converting either to utf-8 (if TCHAR = char ) 
                or utf-32 (TCHAR=wchar_t). On windows or unix it means a no-conversion
                (if TCHAR=wchar_t) or an utf-8 conversion */
		    CORE_API t_string to_t_string( const wchar_t* source, size_t length );
            
            /// Converts a short array to a t_string. The sequence must be null-terminated. 
			CORE_API t_string to_t_string( const wchar_t* source );
			
			/// Convert a char* to a t_string
			CORE_API t_string to_t_string( const char* source);						
  
			/// Convert a std::string to a std::wstring
			CORE_API std::wstring to_wstring(const std::string& source);
			
			/// Does nothing, just a convenience function.
			/** Simplifies writing code that compiles using both unicode and
				multibyte characters. */
			CORE_API const std::wstring& to_wstring(const std::wstring& source);
            
            /// Converts a short array to a std::wstring.
            CORE_API std::wstring to_wstring( const wchar_t* source, size_t length );
            
            /// Converts a null-terminated short array to a std::wstring.
            CORE_API std::wstring to_wstring( const wchar_t* source );
	
			/// Convert a std::wstring to a std::string
			CORE_API std::string to_string(const std::wstring& source);
			
			/// Does nothing, just a convenience function.
			/** Simplifies writing code that compiles using both unicode and
				multibyte characters. */
			CORE_API const std::string& to_string(const std::string& source);
            
            /// Converts a short array to a std::string.
            CORE_API std::string to_string( const wchar_t* source, size_t length );
            
            /// Converts a null terminated short array to a std::string.
            CORE_API std::string to_string( const wchar_t* source );

			/// Converts a boost::any to a string
			/** The value must be one of the following:
				<ul>
				<li>bool
				<li>int
				<li>float
                <li>boost::int32_t
                <li>long
                <li>boost::int64_t
				<li>std::string
				<li>std::wstring
				<li>olib::t_string
				<li>std::vector<double>
				</ul> 
				@throws base_exception if the type is not one of the above.*/
			CORE_API t_ostream& any_to_t_string(t_ostream& os, const boost::any& v);
            
            CORE_API t_ostream& operator<<(t_ostream& os, const boost::any& v);
            
			#ifdef OLIB_ON_WINDOWS
            /// Convert a _bstr_t to a t_string
            CORE_API t_string to_t_string( const _bstr_t& source);
			CORE_API _bstr_t to_bstr( const t_string& source);

            /// Sets the global locale to the locale of the thread that calls this function.
            CORE_API std::locale set_global_locale_to_thread_locale();
			#endif

			/// Expands environment variables in a string
			/** A string like this: "%TEMP%\Ardendo\App" would be expanded 
				to "C:\Documents and Settings\User\Local Settings\Temp\Ardendo\App" */
			CORE_API t_string expand_env_vars( const t_string& to_expand );

            CORE_API bool env_var_exists( const char *env_var_name );

            CORE_API bool env_var_exists( const t_string& env_var_name );
            CORE_API t_string get_env_var( const t_string& env_var_name );
            CORE_API void set_env_var( const t_string& env_var_name, const t_string& val );
            CORE_API void del_env_var( const t_string& env_var_name );

			/// Returns locale::empty() which forwards all calls to the global and finally the classic locale.
			CORE_API std::locale current_locale();

			/// Removes thousands separator output from loc.
			CORE_API void remove_thousands_separator_output( std::locale& loc);

			/// Creates a format object that doesn't throw an exception if the number of arguments is wrong
			CORE_API t_format argument_insensitive_format(const t_string& fstring);

			/// Parses a string and converts it to a 64 bits signed integer.
			/** boost::lexical_cast doesn't seem to work on all platforms, 
				therefore this function is provided. */
			CORE_API boost::int64_t parse_int64( const t_string& to_parse, int base = 10);

			/// Parses a string and converts it to a 64 bits unsigned integer.
			/** boost::lexical_cast doesn't seem to work on all platforms, 
				therefore this function is provided. */
			CORE_API boost::uint64_t parse_unsigned_int64( const t_string& to_parse, int base = 10);
				
		}
	}
}

#endif // _CORE_STRUTIL_H_

