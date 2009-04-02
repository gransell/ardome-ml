#ifndef _CORE_MIN_STRING_DEFINES_H_
#define _CORE_MIN_STRING_DEFINES_H_

#include "platform.hpp"
#include "macro_definitions.hpp"
#include "disabled_warnings.hpp"

// Support for UNICODE strings	

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
#pragma warning( push )
// Local variable may be used without having been initialized	
#pragma warning( disable: 4701 ) 
#endif

#include <string>

#ifdef DOXYGEN_PREPROCESSOR_RUNNING
    /** @def TCHAR 
        A char if OLIB_USE_UTF8 is defined, otherwise a wchar_t. */
    /** @def T_COUT 
        std::cout if OLIB_USE_UTF8 is defined, otherwise a std::wcout */
    /** @def T_CERR 
        std::cerr if OLIB_USE_UTF8 is defined, otherwise a std::wcerr */

    #define TCHAR  
    #define T_COUT 
    #define T_CERR 
#endif

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
#pragma warning( pop )
#endif

#include <sstream>
#include <fstream>
#include <boost/filesystem/fstream.hpp>

#ifdef OLIB_ON_WINDOWS
	#include <tchar.h>
#else
	#ifdef OLIB_USE_UTF16
		#define TCHAR wchar_t
		#define _CT(str) L##str
	#elif defined(OLIB_USE_UTF8)
		#define TCHAR char
		#define _CT(str) str
	#else
		#error("Need to define unicode standard, OLIB_USE_UTF8 or OLIB_USE_UTF16")
	#endif
#endif

#include <boost/format.hpp>

namespace olib
{
    /** @file minimal_string_defines.h 
    Mimics MFC's CString and wx_widgets' wx_string behaviour to enable simple 
    usage in both ANSI and UNICODE builds. The idea is to 
    always use a type named t_xxxx. If compiled with OLIB_USE_UTF16 enabled
    this will be expand to wxxxx, in OLIB_USE_UTF8 it will just be xxxx. 
    @author Mats Lindel&ouml;f */

	typedef std::string utf8_string;
	typedef std::wstring utf16_string;
#ifdef OLIB_USE_UTF16
	typedef std::wiostream t_iostream;
	typedef std::wstring t_string;
	typedef std::wstringstream t_stringstream;
	typedef std::wostringstream t_ostringstream;
	typedef std::wistringstream t_istringstream;
	typedef std::wofstream t_ofstream;
	typedef std::wifstream t_ifstream;
	typedef std::wfstream t_fstream;
	typedef std::wostream t_ostream;
	typedef std::wistream t_istream;
    typedef boost::filesystem::wofstream fs_t_ofstream;
    typedef boost::filesystem::wfstream fs_t_fstream;
    typedef boost::filesystem::wifstream fs_t_ifstream;
	/**  
            <bindgen>
                <attribute name="convert" value="yes"></attribute>
            </bindgen>
            */
	typedef boost::filesystem::wpath t_path;
	typedef boost::filesystem::wdirectory_iterator t_directory_iterator;

    #define T_COUT std::wcout
    #define T_CERR std::wcerr

	typedef boost::wformat t_format;

#elif defined(OLIB_USE_UTF8)
	typedef std::iostream t_iostream;
	typedef std::string t_string;
	typedef std::stringstream t_stringstream;
	typedef std::ostringstream t_ostringstream;
	typedef std::istringstream t_istringstream;
	typedef std::ofstream t_ofstream;
	typedef std::ifstream t_ifstream;
	typedef std::fstream t_fstream;
	typedef std::ostream t_ostream;
	typedef std::istream t_istream;
    typedef boost::filesystem::ofstream fs_t_ofstream;
    typedef boost::filesystem::fstream fs_t_fstream;
    typedef boost::filesystem::ifstream fs_t_ifstream;
	/**  
            <bindgen>
                <attribute name="convert" value="yes"></attribute>
            </bindgen>
            */
	typedef boost::filesystem::path t_path;
	typedef boost::filesystem::directory_iterator t_directory_iterator;
	
    #define T_COUT std::cout
    #define T_CERR std::cerr

	typedef boost::format t_format;

#else
    #error("Need to define unicode standard, OLIB_USE_UTF8 or OLIB_USE_UTF16")
#endif
		
#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
    #define I64SUFFIX i64
    #define I64FMTSPEC _CT("I64")
#elif defined(OLIB_COMPILED_WITH_GCC) 
    #define I64SUFFIX ll
    #define I64FMTSPEC _CT("ll")
#else
    #error("Unsupported compiler. Can not create boost::int64_t integers.")
#endif
	
#define ARMAKE_I64(x) ARCONCAT(x, I64SUFFIX )
#define ARMAKE_UI64(x) ARCONCAT(x, ARCONCAT(u, I64SUFFIX))
	
	
}

#endif // _CORE_STING_DEFINES_H_


