
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.
#include <openpluginlib/py/python.hpp>
#include <openpluginlib/py/py.hpp>

#include <openpluginlib/py/python.hpp>
#include <openpluginlib/py/py.hpp>

#include <openpluginlib/pl/string.hpp>
#include <openpluginlib/pl/pcos/property_container.hpp>
#include <openpluginlib/pl/pcos/property.hpp>
#include <openpluginlib/pl/pcos/subject.hpp>
#include <openpluginlib/pl/pcos/key.hpp>

namespace pcos	= olib::openpluginlib::pcos;
namespace py	= boost::python;

namespace olib { namespace openpluginlib { namespace detail {

std::vector<pcos::key>::iterator ( std::vector<pcos::key>::*begin )( ) = &std::vector<pcos::key>::begin;
std::vector<pcos::key>::iterator ( std::vector<pcos::key>::*end )( ) = &std::vector<pcos::key>::end;

class iproperty_container_wrap : public pcos::iproperty_container, public py::wrapper< pcos::iproperty_container >
{
public:
    virtual pcos::property get_property_with_string( const char* k ) const   { return py::call<pcos::property>( this->get_override( "get_property_with_string" ).ptr(), k ); }
    virtual pcos::property get_property_with_key( const pcos::key& k ) const { return py::call<pcos::property>( this->get_override( "get_property_with_key" ).ptr(), k ); }
    virtual pcos::key_vector get_keys() const                                { return py::call<pcos::key_vector>( this->get_override( "get_keys" ).ptr() ); }
    virtual void append( const pcos::property& p )                           { return py::call<void>( this->get_override( "append" ).ptr(), p ); }
    virtual void remove( const pcos::property& p )                           { py::call<void>( this->get_override( "remove" ).ptr(), p ); }
    virtual void accept( pcos::visitor& v )                                  { py::call<void>( this->get_override( "accept" ).ptr(), v ); }
};

void py_pcos_property_container()
{
    py::class_< iproperty_container_wrap, boost::noncopyable >( "iproperty_container" )
	.def( "get_property", py::pure_virtual( &pcos::iproperty_container::get_property_with_string ) )
	.def( "get_property", py::pure_virtual( &pcos::iproperty_container::get_property_with_key ) )
	.def( "get_keys", py::pure_virtual( &pcos::iproperty_container::get_keys ) )
	.def( "append", py::pure_virtual( &pcos::iproperty_container::append ) )
	.def( "remove", py::pure_virtual( &pcos::iproperty_container::remove ) )
	.def( "accept", py::pure_virtual( &pcos::iproperty_container::accept ) );

    py::register_ptr_to_python< boost::shared_ptr< pcos::iproperty_container > >();

    py::class_< pcos::property_container, py::bases< pcos::isubject, pcos::iproperty_container > >( "property_container" )
        .def( "get_property", &pcos::property_container::get_property_with_string, py::return_value_policy< py::return_by_value >() )
        .def( "get_property", &pcos::property_container::get_property_with_key, py::return_value_policy< py::return_by_value >() )
        .def( "get_keys", &pcos::property_container::get_keys )
        .def( "append", &pcos::property_container::append )
        .def( "remove", &pcos::property_container::remove )
	.def( "accept", &pcos::property_container::accept )
	.def( "clone", &pcos::property_container::clone, py::return_value_policy< py::manage_new_object >() );

    py::class_< std::vector<pcos::key> >( "key_vector", py::init< std::vector<pcos::key> >() )
		.def( "__iter__", py::range( begin, end ) );
}

} } }
