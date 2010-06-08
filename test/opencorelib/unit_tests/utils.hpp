#ifndef UTILS_CL_TESTS_H_INCLUDED_
#define UTILS_CL_TESTS_H_INCLUDED_

#include "opencorelib/cl/str_util.hpp"

//Required in order to use wstrings with BOOST_CHECK_EQUAL
namespace std {
	inline std::ostream &operator<<(std::ostream& output_stream, const std::wstring &the_string)
	{
		return output_stream << olib::opencorelib::str_util::to_string(the_string);
	}
}

#endif //#ifndef UTILS_CL_TESTS_H_INCLUDED_

