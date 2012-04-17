

// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.b
// For more information, see http://www.openlibraries.org.
#include <openpluginlib/py/python.hpp>
#include <openpluginlib/py/py.hpp>

#include <openpluginlib/pl/pcos/observer.hpp>
#include <openpluginlib/pl/pcos/isubject.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>

namespace pcos	= olib::openpluginlib::pcos;
namespace py	= boost::python;

namespace olib { namespace openpluginlib { namespace detail {

class observer_wrapper : public pcos::observer, 
                         public py::wrapper< pcos::observer >
{
public:
    struct null_deleter
    {
        void operator()( pcos::isubject* ) {}
    };

    virtual void updated( pcos::isubject* s )
    {
        /// this is really a hack...
        /// I am forcing the subject to be converted into a shared_ptr here to ensure
        /// consistent calling into python. However, if we simply made a regular shared_ptr
        /// upon exiting this call \arg s would be deleted. 
	if ( py::override o = get_override( "updated" ) )
	{
	    o( boost::shared_ptr< pcos::isubject >( s, null_deleter() ) );
	    return;
	}

	std::cerr << "observer_wrapper::updated: no override of updated() found\n";
    }
};
    
void py_pcos_observer()
{
    py::class_< observer_wrapper, boost::noncopyable >( "observer" )
        .def( "updated", py::pure_virtual( &pcos::observer::updated ) );
    
}

} } }
