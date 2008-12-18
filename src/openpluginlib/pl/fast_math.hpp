
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2007 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef FAST_MATH_INC_
#define FAST_MATH_INC_

#include <openpluginlib/pl/config.hpp>

namespace olib { namespace openpluginlib {

OPENPLUGINLIB_DECLSPEC float fast_log2f( float x );
OPENPLUGINLIB_DECLSPEC float fast_exp2f( float x );
OPENPLUGINLIB_DECLSPEC float fast_powf( float x, float y );
OPENPLUGINLIB_DECLSPEC int	 fast_floorf( float x );

} }

#endif
