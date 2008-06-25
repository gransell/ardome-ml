
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openpluginlib/py/python.hpp>
#include <openpluginlib/py/py.hpp>

namespace opl = olib::openpluginlib;

BOOST_PYTHON_MODULE( openpluginlib )
{
	opl::detail::py_string( );
	opl::detail::py_openpluginlib( );
	opl::detail::py_pcos_key();
	opl::detail::py_pcos_observer(); 
	opl::detail::py_pcos_subject(); 
	opl::detail::py_pcos_property_container(); 
	opl::detail::py_pcos_property(); 
	opl::detail::py_pcos_any(); 
	opl::detail::py_pcos_visitor(); 
}
