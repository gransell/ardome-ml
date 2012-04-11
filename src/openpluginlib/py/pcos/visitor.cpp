
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openpluginlib/py/python.hpp>
#include <openpluginlib/py/py.hpp>

#include <openpluginlib/pl/pcos/visitor.hpp>
#include <openpluginlib/pl/pcos/property.hpp>
#include <openpluginlib/pl/pcos/property_container.hpp>

#include <boost/utility.hpp>

#include <iostream>

namespace pcos	= olib::openpluginlib::pcos;
namespace py	= boost::python;

namespace olib { namespace openpluginlib { namespace detail {

class visitor_wrapper : public pcos::visitor, 
                        public py::wrapper< pcos::visitor >
{
public:
    virtual bool visit_property( pcos::property& p )
    {
		if ( py::override o = get_override( "visit_property" ) )
		{
			return py::call<bool>( o.ptr(), p );
		}

		std::cerr << "visitor_wrapper::updated: no override of visit_property() found\n";
		
		return false;
    }

    virtual bool visit_property_container( pcos::property_container& pc )
    {
		if ( py::override o = get_override( "visit_property_container" ) )
		{
			return py::call<bool>( o.ptr(), pc );
		}

		std::cerr << "visitor_wrapper::updated: no override of visit_property_container() found\n";
	
		return false;
    }
};
    
void py_pcos_visitor()
{
    py::class_< visitor_wrapper, boost::noncopyable >( "visitor" )
        .def( "visit_property", py::pure_virtual( &pcos::visitor::visit_property ) )
        .def( "visit_property_container", py::pure_virtual( &pcos::visitor::visit_property_container ) );
    
}

} } }
