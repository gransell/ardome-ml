
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef UTF8_UTILS_INC_
#define UTF8_UTILS_INC_

#include <openpluginlib/pl/config.hpp>
#include <openpluginlib/pl/string.hpp>

namespace olib { namespace openpluginlib {

OPENPLUGINLIB_DECLSPEC wstring		to_wstring( const string& str  );
OPENPLUGINLIB_DECLSPEC string		to_string(  const wstring& str );

} }

#endif
