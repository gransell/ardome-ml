/* -*- mode: C; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openpluginlib/py/python.hpp>
#include <openpluginlib/py/py.hpp>
#include <openpluginlib/pl/string.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>

#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/python/to_python_converter.hpp>

// std
#include <iterator>

namespace opl	= olib::openpluginlib;
namespace py	= boost::python;

namespace olib { namespace openpluginlib { namespace detail {

#if defined( HAVE_FLEX_STRING )

struct flex_string_to_python_str
{
	static PyObject* convert( const opl::string& s )
	{ 
		return py::incref( py::object( std::string( s.c_str( ) ) ).ptr( ) ); 
	}
};

struct flex_string_from_python_str
{
	flex_string_from_python_str( )
	{
		py::converter::registry::push_back( &convertible, &construct, py::type_id<opl::string>( ) );
	}
	
	static void* convertible( PyObject* obj_ptr )
	{
		if( !PyString_Check( obj_ptr ) ) return 0;
		return obj_ptr;
	}

	static void construct( PyObject* obj_ptr, py::converter::rvalue_from_python_stage1_data* data )
	{
		const char* value = PyString_AsString( obj_ptr );
		if( !value )
			py::throw_error_already_set( );
			
		size_t size = PyString_Size( obj_ptr );

		void* storage = ( ( py::converter::rvalue_from_python_storage<opl::string>* ) data )->storage.bytes;
		new ( storage ) opl::string( value, size );
		data->convertible = storage;
	}	
};

struct flex_wstring_to_python_str
{
	static PyObject* convert( const opl::wstring& s )
	{ 
		return py::incref( py::object( std::string( to_string( s ).c_str( ) ) ).ptr( ) ); 
	}
};

struct flex_wstring_from_python_str
{
	flex_wstring_from_python_str( )
	{ py::converter::registry::push_back( &convertible, &construct, py::type_id<opl::wstring>( ) ); }
	
	static void* convertible( PyObject* obj_ptr )
	{
		if( !PyString_Check( obj_ptr ) ) return 0;
		return obj_ptr;
	}

	static void construct( PyObject* obj_ptr, py::converter::rvalue_from_python_stage1_data* data )
	{
		const char* value = PyString_AsString( obj_ptr );
		if( !value )
			py::throw_error_already_set( );

		size_t size = PyString_Size( obj_ptr );

		void* storage = ( ( py::converter::rvalue_from_python_storage<opl::wstring>* ) data )->storage.bytes;
		new ( storage ) opl::wstring( opl::to_wstring( value ).data(), size );
		data->convertible = storage;
	}
};

#endif

struct flex_string_list_to_python_str_list
{
	static PyObject* convert( const opl::string_list& l )
	{ 
		PyObject* list = PyList_New( static_cast<int>( l.size() ) );
		if ( !list )
			py::throw_error_already_set();

		opl::string_list::const_iterator I = l.begin();
		for ( size_t i=0; i<l.size(); ++i, ++I )
		{
#if defined( HAVE_FLEX_STRING )
			PyList_SetItem( list, static_cast<int>( i ), flex_string_to_python_str::convert( *I ) );
#else
			PyList_SetItem( list, i, PyString_FromString( I->c_str() ) );
#endif
		}

		return py::incref( list ); 
	}
};

struct flex_string_list_from_python_str_list
{
	flex_string_list_from_python_str_list( )
	{ 
		py::converter::registry::push_back( &convertible, &construct, py::type_id<opl::string_list>( ) ); 
	}
	
	static void* convertible( PyObject* obj_ptr )
	{
		void* result = NULL;
		if( PyList_Check( obj_ptr ) && ( !PyList_Size( obj_ptr ) || PyString_Check( PyList_GET_ITEM( obj_ptr, 0 ) ) ) )
			result = obj_ptr;

		return result;
	}

	static void construct( PyObject* obj_ptr, py::converter::rvalue_from_python_stage1_data* data )
	{
		PyObject* value = PyList_AsTuple( obj_ptr );
		if( !value )
			py::throw_error_already_set( );

		void* storage = ( ( py::converter::rvalue_from_python_storage<opl::string_list>* ) data )->storage.bytes;
		opl::string_list* l = new ( storage ) opl::string_list;
		for ( int i=0; i<PyTuple_Size( value ); ++i )
			l->push_back( py::extract< opl::string >( PyTuple_GetItem( value, i ) )() );

		data->convertible = storage;
	}
};

struct flex_wstring_list_to_python_str_list
{
	static PyObject* convert( const opl::wstring_list& l )
	{ 
		PyObject* list = PyList_New( static_cast<int>( l.size() ) );
		if ( !list )
			py::throw_error_already_set();

		opl::wstring_list::const_iterator I = l.begin();
		for ( size_t i=0; i<l.size(); ++i, ++I )
		{
#if defined( HAVE_FLEX_STRING )
			PyList_SetItem( list, static_cast<int>( i ), flex_wstring_to_python_str::convert( *I ) );
#else
			PyList_SetItem( list, i, PyString_FromString( opl::to_string( *I ).c_str() ) );
#endif
		}
		return py::incref( list ); 
	}
};

struct flex_wstring_list_from_python_str_list
{
	flex_wstring_list_from_python_str_list( )
	{ 
		py::converter::registry::push_back( &convertible, &construct, py::type_id<opl::wstring_list>( ) ); 
	}
	
	static void* convertible( PyObject* obj_ptr )
	{
		void* result = NULL;
		if( PyList_Check( obj_ptr ) && ( !PyList_Size( obj_ptr ) || PyString_Check( PyList_GET_ITEM( obj_ptr, 0 ) ) ) )
			result = obj_ptr;

		return result;
	}

	static void construct( PyObject* obj_ptr, py::converter::rvalue_from_python_stage1_data* data )
	{
		PyObject* value = PyList_AsTuple( obj_ptr );
		if( !value )
			py::throw_error_already_set( );

		void* storage = ( ( py::converter::rvalue_from_python_storage<opl::wstring_list>* ) data )->storage.bytes;
		opl::wstring_list* l = new ( storage ) opl::wstring_list;
		for ( int i=0; i<PyTuple_Size( value ); ++i )
			l->push_back( py::extract< opl::wstring >( PyTuple_GetItem( value, i ) )() );

		data->convertible = storage;
	}
};


void py_string( )
{
#if defined( HAVE_FLEX_STRING )
	py::to_python_converter<opl::string, flex_string_to_python_str>( );
	flex_string_from_python_str( );

	py::to_python_converter<opl::wstring, flex_wstring_to_python_str>( );	
	flex_wstring_from_python_str( );
#endif

	py::to_python_converter<opl::string_list, flex_string_list_to_python_str_list>( );	
	flex_string_list_from_python_str_list( );

	py::to_python_converter<opl::wstring_list, flex_wstring_list_to_python_str_list>( );	
	flex_wstring_list_from_python_str_list( );
}

} } }
