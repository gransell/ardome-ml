
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <cstdlib>
#include <vector>

#include <openpluginlib/pl/utf8_utils.hpp>
#include "opencorelib/cl/core.hpp"
#include "opencorelib/cl/str_util.hpp"


namespace olib { namespace openpluginlib {

wstring to_wstring( const string& str )
{
    return opencorelib::str_util::to_wstring( str );
}
			
string to_string( const wstring& str )
{
    return opencorelib::str_util::to_string(str);
}

} }
