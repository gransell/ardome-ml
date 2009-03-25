
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openpluginlib/pl/pcos/observer.hpp>
#include <openpluginlib/pl/pcos/subject.hpp>
#include <openpluginlib/py/python.hpp>
#include <openpluginlib/py/py.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

namespace pcos	= olib::openpluginlib::pcos;
namespace py	= boost::python;

namespace olib { namespace openpluginlib { namespace detail {

    class isubject_wrap : public pcos::isubject, public py::wrapper< pcos::isubject >
    {
    public:
	void attach( boost::shared_ptr< pcos::observer > o )  { py::call<void>(this->get_override("attach").ptr(), o ); }
	void detach( boost::shared_ptr< pcos::observer > o )  { py::call<void>(this->get_override("detach").ptr(), o ); }
	void update()                                         { py::call<void>(this->get_override("update").ptr()); }
	void block( boost::shared_ptr< pcos::observer > o )   { py::call<void>(this->get_override("block").ptr(), o ); }
	void unblock( boost::shared_ptr< pcos::observer > o ) { py::call<void>(this->get_override("unblock").ptr(), o ); }
    };

void py_pcos_subject()
{
    py::class_< isubject_wrap, boost::noncopyable >( "isubject" )
	.def( "attach", py::pure_virtual( &pcos::isubject::attach ) )
        .def( "detach", py::pure_virtual( &pcos::isubject::detach ) )
        .def( "update", py::pure_virtual( &pcos::isubject::update ) )
        .def( "block", py::pure_virtual( &pcos::isubject::block ) )
        .def( "unblock", py::pure_virtual( &pcos::isubject::unblock ) );

    py::register_ptr_to_python< boost::shared_ptr< pcos::isubject > >();

    py::class_< pcos::subject, py::bases< pcos::isubject >, boost::shared_ptr< pcos::subject >, boost::noncopyable >( "subject" )
        .def( "attach", &pcos::subject::attach )
        .def( "detach", &pcos::subject::detach )
        .def( "update", &pcos::subject::update )
        .def( "block", &pcos::subject::block )
        .def( "unblock", &pcos::subject::unblock );
}

} } }
