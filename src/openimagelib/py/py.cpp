
// il - A image library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openpluginlib/py/python.hpp>
#include <openimagelib/py/py.hpp>

namespace il = olib::openimagelib::il;

BOOST_PYTHON_MODULE( openimagelib )
{
	il::detail::py_basic_image( );
	il::detail::py_utility( );
	il::detail::py_plugin( );
}
