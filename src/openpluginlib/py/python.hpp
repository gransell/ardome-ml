
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef PYTHON_INC_
#define PYTHON_INC_

#ifdef _MSC_VER
#pragma warning ( push )
#pragma warning ( disable: 4100 4121 4503 4511 4512 4244 4267 )
#endif

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif

#include <boost/python.hpp>

#ifdef _MSC_VER
#pragma warning ( default: 4100 4121 4503 4511 4512 4244 4267 )
#pragma warning ( pop )
#endif


#endif
