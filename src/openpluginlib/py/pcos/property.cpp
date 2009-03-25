/* -*- mode: C; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openpluginlib/pl/string.hpp>
#include <openpluginlib/pl/pcos/property.hpp>
#include <openpluginlib/pl/pcos/subject.hpp>
#include <openpluginlib/pl/pcos/key.hpp>
#include <openpluginlib/pl/pcos/any.hpp>
#include <openpluginlib/py/python.hpp>
#include <openpluginlib/py/py.hpp>
#include <openimagelib/il/basic_image.hpp>
#include <openmedialib/ml/input.hpp>
#include <boost/cstdint.hpp>

namespace opl	= olib::openpluginlib;
namespace pcos	= olib::openpluginlib::pcos;
namespace il	= olib::openimagelib::il;
namespace ml	= olib::openmedialib::ml;
namespace py	= boost::python;

namespace olib { namespace openpluginlib { namespace detail {

void ( pcos::property::*set_string )( const opl::string& ) = &pcos::property::set<opl::string>;
void ( pcos::property::*set_wstring )( const opl::wstring& ) = &pcos::property::set<opl::wstring>;
void ( pcos::property::*set_string_list )( const opl::string_list& ) = &pcos::property::set<opl::string_list>;
void ( pcos::property::*set_wstring_list )( const opl::wstring_list& ) = &pcos::property::set<opl::wstring_list>;
void ( pcos::property::*set_double )( const double& ) = &pcos::property::set<double>;
void ( pcos::property::*set_int_list )( const pcos::int_list& ) = &pcos::property::set<pcos::int_list>;
void ( pcos::property::*set_double_list )( const pcos::double_list& ) = &pcos::property::set<pcos::double_list>;
void ( pcos::property::*set_image_type_ptr )( const il::image_type_ptr& ) = &pcos::property::set<il::image_type_ptr>;
void ( pcos::property::*set_input_type_ptr )( const ml::input_type_ptr& ) = &pcos::property::set<ml::input_type_ptr>;

void back_reference_set( pcos::property* p, py::back_reference<int> x )
{
	if (x.source().ptr()->ob_type == &PyBool_Type) {
		p->set< bool >( x.get() );
	}
	else {
		p->set< int >( x.get() );
	}
}


wstring			   ( pcos::property::*value_wstring )( ) const = &pcos::property::value<wstring>;
string			   ( pcos::property::*value_string )( ) const = &pcos::property::value<string>;
wstring_list	   ( pcos::property::*value_wstring_list )( ) const = &pcos::property::value<wstring_list>;
string_list		   ( pcos::property::*value_string_list )( ) const = &pcos::property::value<string_list>;
int				   ( pcos::property::*value_int )( ) const = &pcos::property::value<int>;
boost::int64_t	   ( pcos::property::*value_int64 )( ) const = &pcos::property::value<boost::int64_t>;
double			   ( pcos::property::*value_double )( ) const = &pcos::property::value<double>;
pcos::int_list	   ( pcos::property::*value_int_list )( ) const = &pcos::property::value<pcos::int_list>;
pcos::double_list  ( pcos::property::*value_double_list )( ) const = &pcos::property::value<pcos::double_list>;
bool			   ( pcos::property::*value_bool )( ) const = &pcos::property::value<bool>;
il::image_type_ptr ( pcos::property::*value_image_type_ptr )( ) const = &pcos::property::value<il::image_type_ptr>;
ml::input_type_ptr ( pcos::property::*value_input_type_ptr )( ) const = &pcos::property::value<ml::input_type_ptr>;
std::vector< double > ( pcos::property::*value_double_vector )( ) const = &pcos::property::value< std::vector< double > >;

bool			 ( pcos::property::*is_a_wstring )( ) const = &pcos::property::is_a<opl::wstring>;
bool			 ( pcos::property::*is_a_string )( ) const = &pcos::property::is_a<opl::string>;
bool			 ( pcos::property::*is_a_wstring_list )( ) const = &pcos::property::is_a<opl::wstring_list>;
bool			 ( pcos::property::*is_a_string_list )( ) const = &pcos::property::is_a<opl::string_list>;
bool			 ( pcos::property::*is_a_int )( ) const = &pcos::property::is_a<int>;
bool			 ( pcos::property::*is_a_int64 )( ) const = &pcos::property::is_a<boost::int64_t>;
bool			 ( pcos::property::*is_a_uint64 )( ) const = &pcos::property::is_a<boost::uint64_t>;
bool			 ( pcos::property::*is_a_double )( ) const = &pcos::property::is_a<double>;
bool			 ( pcos::property::*is_a_int_list )( ) const = &pcos::property::is_a<pcos::int_list>;
bool			 ( pcos::property::*is_a_double_list )( ) const = &pcos::property::is_a<pcos::double_list>;
bool			 ( pcos::property::*is_a_bool )( ) const = &pcos::property::is_a<bool>;
bool			 ( pcos::property::*is_a_image_type_ptr )( ) const = &pcos::property::is_a<il::image_type_ptr>;
bool			 ( pcos::property::*is_a_input_type_ptr )( ) const = &pcos::property::is_a<ml::input_type_ptr>;
bool			 ( pcos::property::*is_a_double_vector )( ) const = &pcos::property::is_a< std::vector< double > >;

template < typename T > struct PyConvertTrait
{};

template <> struct PyConvertTrait< int >
{
	static PyObject* typeToPy( int v ) { return PyInt_FromLong( v ); }
	static bool checkType( PyObject* obj ) { return PyInt_CheckExact( obj ); }
};

template <> struct PyConvertTrait< double >
{
	static PyObject* typeToPy( double v ) { return PyFloat_FromDouble( v ); }
	static bool checkType( PyObject* obj ) { return PyFloat_CheckExact( obj ); }
};

template < typename LIST, typename TRAIT > struct list_to_python_list
{
	static PyObject* convert( const LIST& l )
	{ 
		PyObject* list = PyList_New( static_cast<int>( l.size() ) );
		if ( !list )
			py::throw_error_already_set();
		
		typename LIST::const_iterator I = l.begin();
		for ( size_t i=0; i<l.size(); ++i, ++I )
		{
			PyList_SetItem( list, i, TRAIT::typeToPy( *I ) );
		}
		return py::incref( list ); 
	}
};

template < typename LIST, typename TRAIT > struct list_from_python_list
{
	list_from_python_list( )
	{ 
		py::converter::registry::push_back( &convertible, &construct, py::type_id< LIST >( ) ); 
	}
	
	static void* convertible( PyObject* obj_ptr )
	{
		void* result = NULL;
		if( PyList_Check( obj_ptr ) && TRAIT::checkType( PyList_GET_ITEM( obj_ptr, 0 ) ) )
			result = obj_ptr;
		
		return result;
	}
	
	static void construct( PyObject* obj_ptr, py::converter::rvalue_from_python_stage1_data* data )
	{
		PyObject* value = PyList_AsTuple( obj_ptr );
		if( !value )
			py::throw_error_already_set( );
	
		void* storage = ( ( py::converter::rvalue_from_python_storage< LIST >* ) data )->storage.bytes;
		LIST* l = new ( storage ) LIST;
		for ( int i=0; i<PyTuple_Size( value ); ++i )
			l->push_back( py::extract< typename LIST::value_type >( PyTuple_GetItem( value, i ) )() );

		data->convertible = storage;
	}
};

void py_pcos_property()
{
	py::to_python_converter< pcos::int_list, list_to_python_list< pcos::int_list, PyConvertTrait< int > > >( );	
	list_from_python_list< pcos::int_list, PyConvertTrait< int > >();
	py::to_python_converter< pcos::uint_list, list_to_python_list< pcos::uint_list, PyConvertTrait< int > > >( );	
	list_from_python_list< pcos::uint_list, PyConvertTrait< int > >();
	py::to_python_converter< pcos::double_list, list_to_python_list< pcos::double_list, PyConvertTrait< double > > >( );	
	list_from_python_list< pcos::double_list, PyConvertTrait< double > >();
	py::to_python_converter< std::vector< double >, list_to_python_list< std::vector< double >, PyConvertTrait< double > > >( );	
	list_from_python_list< std::vector< double >, PyConvertTrait< double > >();

	py::class_< pcos::property, py::bases< pcos::isubject, pcos::iproperty_container >, boost::shared_ptr< pcos::property > >( "property", py::init< pcos::key >() )
		.def( "get_key", &pcos::property::get_key )
		.def( py::self == py::self )
		.def( py::self != py::self )
		.def( "set_always_notify", &pcos::property::set_always_notify )
		.def( "valid", &pcos::property::valid )
		.def( "set", set_double )
		.def( "set", set_double_list )
		.def( "set", set_int_list )
		.def( "set", set_string )
		.def( "set", set_wstring )
		.def( "set", set_string_list )
		.def( "set", set_wstring_list )
		.def( "set", set_image_type_ptr )
		.def( "set", set_input_type_ptr )
		.def( "set", back_reference_set )
		.def( "set_from_string", &pcos::property::set_from_string )
		.def( "value_as_wstring", value_wstring )
		.def( "value_as_string", value_string )
		.def( "value_as_wstring_list", value_wstring_list )
		.def( "value_as_string_list", value_string_list )
		.def( "value_as_int", value_int )
		.def( "value_as_int64", value_int64 )
		.def( "value_as_double", value_double )
		.def( "value_as_int_list", value_int_list )
		.def( "value_as_double_list", value_double_list )
		.def( "value_as_bool", value_bool )
		.def( "value_as_image", value_image_type_ptr )
		.def( "value_as_input", value_input_type_ptr )
		.def( "value_as_double_vector", value_double_vector )
		.def( "is_a_wstring", is_a_wstring )
		.def( "is_a_string", is_a_string )
		.def( "is_a_wstring_list", is_a_wstring_list )
		.def( "is_a_string_list", is_a_string_list )
		.def( "is_a_int", is_a_int )
		.def( "is_a_int64", is_a_int64 )
		.def( "is_a_uint64", is_a_uint64 )
		.def( "is_a_double", is_a_double )
		.def( "is_a_int_list", is_a_int_list )
		.def( "is_a_double_list", is_a_double_list )
		.def( "is_a_bool", is_a_bool )
		.def( "is_a_image", is_a_image_type_ptr )
		.def( "is_a_input", is_a_input_type_ptr )
		.def( "is_a_double_vector", is_a_double_vector )
		.def( "get_property", &pcos::property::get_property_with_string, py::return_value_policy< py::return_by_value >() )
		.def( "get_property", &pcos::property::get_property_with_key, py::return_value_policy< py::return_by_value >() )
		.def( "get_keys", &pcos::property::get_keys )
		.def( "append", &pcos::property::append )
		.def( "remove", &pcos::property::remove )
		.def( "accept", &pcos::property::accept )
		.def( "clone", &pcos::property::clone, py::return_value_policy< py::manage_new_object >() );
}

} } }
