#ifndef _CORE_STING_DEFINES_H_
#define _CORE_STING_DEFINES_H_

#include "platform.hpp"
#include "macro_definitions.hpp"
#include "./minimal_string_defines.hpp"

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
	#pragma warning( push )
		#pragma warning( disable: 4511 )
		#pragma warning( disable: 4512 )
		#pragma warning( disable: 4267 )
		#pragma warning( disable: 4244 )
#endif

	#include <boost/format.hpp>

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
	#pragma warning( pop )
#endif

#ifndef BOOST_REGEX_DYN_LINK
    #define BOOST_REGEX_DYN_LINK
#endif
#include <boost/regex.hpp>

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
    #pragma warning (push)
    #pragma warning (disable : 4100 )
    #pragma warning (disable : 4127 )
    #pragma warning (disable : 4189 )
    #pragma warning (disable : 4511 )
    #pragma warning (disable : 4512 )
    #pragma warning (disable : 4244 )
#endif
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/conversion.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/gregorian/gregorian_io.hpp>
#include <boost/date_time/local_time/local_time_io.hpp>
#include <boost/filesystem.hpp>
#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
    #pragma warning (pop)
#endif

namespace olib
{
#ifdef OLIB_USE_UTF8
	typedef boost::format t_format;
	typedef boost::regex t_regex;
	typedef boost::smatch t_smatch;
	typedef boost::cmatch t_cmatch;
    typedef boost::local_time::local_time_facet t_local_time_facet;
    typedef boost::gregorian::date_facet t_date_facet;	

#elif defined(OLIB_USE_UTF16)
	typedef boost::wformat t_format;
	typedef boost::wregex t_regex;
	typedef boost::wsmatch t_smatch;
	typedef boost::wcmatch t_cmatch;
    typedef boost::local_time::wlocal_time_facet t_local_time_facet;
    typedef boost::gregorian::wdate_facet t_date_facet;
#else
#error("Need to define unicode standard, OLIB_USE_UTF8 or OLIB_USE_UTF16")
#endif

}

#endif // _CORE_STING_DEFINES_H_


