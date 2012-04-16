

// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.
#include <openpluginlib/py/python.hpp>
#include <openpluginlib/py/py.hpp>


#include <openpluginlib/pl/pcos/key.hpp>
#include <string>
#include <sstream>

namespace pcos	= olib::openpluginlib::pcos;
namespace py	= boost::python;

namespace olib { namespace openpluginlib { namespace detail {

pcos::key create_key_from_string( const char* ks )
{
    return pcos::key::from_string( ks );
}

std::string get_key_as_string( const pcos::key& k )
{
    std::ostringstream os;
    os << k;
    return os.str();
}

void py_pcos_key( )
{
    py::class_< pcos::key >( "key", py::no_init )
	.def( "__str__", get_key_as_string )
        .def( py::self < py::self )
        .def( py::self == py::self )
        .def( py::self != py::self );

    py::def( "create_key_from_string", create_key_from_string );
}

} } }
