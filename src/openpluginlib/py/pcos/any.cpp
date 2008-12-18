/* -*- mode: C; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openpluginlib/pl/string.hpp>
#include <openpluginlib/pl/pcos/any.hpp>
#include <openpluginlib/py/python.hpp>
#include <openpluginlib/py/py.hpp>
#include <openimagelib/il/basic_image.hpp>

namespace opl	= olib::openpluginlib;
namespace pcos	= olib::openpluginlib::pcos;
namespace il	= olib::openimagelib::il;
namespace py	= boost::python;

namespace olib { namespace openpluginlib { namespace detail {

double		       (*double_cast)( const pcos::any& )	   	  = &pcos::any_cast< double >;
int			       (*int_cast)( const pcos::any& )		   	  = &pcos::any_cast< int >;
unsigned int	   (*uint_cast)( const pcos::any& )		   	  = &pcos::any_cast< unsigned int >;
string		  	   (*string_cast)( const pcos::any& )	   	  = &pcos::any_cast< string >;
wstring		  	   (*wstring_cast)( const pcos::any& )	   	  = &pcos::any_cast< wstring >;
string_list		   (*string_list_cast)( const pcos::any& )    = &pcos::any_cast< string_list >;
wstring_list	   (*wstring_list_cast)( const pcos::any& )   = &pcos::any_cast< wstring_list >;
pcos::int_list	   (*int_list_cast)( const pcos::any& )	   	  = &pcos::any_cast< pcos::int_list >;
pcos::uint_list	   (*uint_list_cast)( const pcos::any& )	  = &pcos::any_cast< pcos::uint_list >;
pcos::double_list  (*double_list_cast)( const pcos::any& )    = &pcos::any_cast< pcos::double_list >;
bool			   (*bool_cast)( const pcos::any& )		   	  = &pcos::any_cast< bool >;
il::image_type_ptr (*image_type_ptr_cast)( const pcos::any& ) = &pcos::any_cast< il::image_type_ptr >;

// injected constructor
pcos::any* make_any( py::back_reference<int> x )
{
	if (x.source().ptr()-> ob_type == &PyBool_Type) {
		return new pcos::any(x.get() != 0);
	}
	else {
		return new pcos::any(x.get());
	}
}

void py_pcos_any()
{
	py::class_< pcos::any >( "any", py::init< double >() )
		.def( py::init< unsigned int >() )
		.def( py::init< string >() )
		.def( py::init< wstring >() )
		.def( py::init< string_list >() )
		.def( py::init< wstring_list >() )
		.def( py::init< pcos::int_list >() )
		.def( py::init< pcos::uint_list >() )
		.def( py::init< pcos::double_list >() )
		.def( py::init< il::image_type_ptr >() )
		.def("__init__", py::make_constructor(make_any))	
		.def( "as_double", double_cast )
		.def( "as_int", int_cast )
		.def( "as_uint", uint_cast )
		.def( "as_string", string_cast )
		.def( "as_wstring", wstring_cast )
		.def( "as_string_list", string_list_cast )
		.def( "as_wstring_list", wstring_list_cast )
		.def( "as_int_list", int_list_cast )
		.def( "as_uint_list", uint_list_cast )
		.def( "as_double_list", double_list_cast )
		.def( "as_bool", bool_cast )
		.def( "as_image", image_type_ptr_cast )
		;
}

} } }
