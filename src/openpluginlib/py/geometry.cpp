
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openpluginlib/pl/geometry.hpp>
#include <openpluginlib/py/python.hpp>
#include <openpluginlib/py/py.hpp>

namespace opl	= olib::openpluginlib;
namespace py	= boost::python;

namespace olib { namespace openpluginlib { namespace detail {

void py_geometry( )
{
#define DEFINE_OPL_OVERLOADED_OPERATORS( )	\
		.def( py::self + py::self )			\
		.def( py::self += py::self )		\
		.def( py::self - py::self )			\
		.def( -py::self )					\
		.def( py::self -= py::self )		\
		.def( py::self * int( ) )			\
		.def( int( ) * py::self )			\
		.def( py::self * float( ) )			\
		.def( float( ) * py::self )			\
		.def( py::self / int( ) )			\
		.def( py::self /= int( ) )			\
		.def( py::self / float( ) )			\
		.def( py::self /= float( ) )

#define DEFINE_OPL_VEC2_TYPE( type, value_type, name )					\
	py::class_<opl::type>( name, py::init<value_type, value_type>( ) )	\
		DEFINE_OPL_OVERLOADED_OPERATORS( )								\
		.def( "get", &opl::type::get )									\
		.def( "set", &opl::type::set )									\
		;

#define DEFINE_OPL_VEC3_TYPE( type, value_type, name )								\
	py::class_<opl::type>( name, py::init<value_type, value_type, value_type>( ) )	\
		DEFINE_OPL_OVERLOADED_OPERATORS( )											\
		.def( "get", &opl::type::get )												\
		.def( "set", &opl::type::set )												\
		;

#define DEFINE_OPL_VEC4_TYPE( type, value_type, name )											\
	py::class_<opl::type>( name, py::init<value_type, value_type, value_type, value_type>( ) )	\
		DEFINE_OPL_OVERLOADED_OPERATORS( )														\
		.def( "get", &opl::type::get )															\
		.def( "set", &opl::type::set )															\
		;

#define DEFINE_OPL_MATRIX_TYPE( type, value_type, name )	\
	py::class_<opl::type>( name, py::init<value_type>( ) )	\
		DEFINE_OPL_OVERLOADED_OPERATORS( )					\
		.def( "get", &opl::type::get )						\
		.def( "set", &opl::type::set )						\
		;

	DEFINE_OPL_VEC2_TYPE( vec2d, double, "vec2d" )
	DEFINE_OPL_VEC2_TYPE( vec2f, float,  "vec2f" )
	DEFINE_OPL_VEC3_TYPE( vec3d, double, "vec3d" )
	DEFINE_OPL_VEC3_TYPE( vec3f, float,  "vec3f" )
	DEFINE_OPL_VEC4_TYPE( vec4d, double, "vec4d" )
	DEFINE_OPL_VEC4_TYPE( vec4f, float,  "vec4f" )

	DEFINE_OPL_MATRIX_TYPE( matrixd, double, "matrixd" )
	DEFINE_OPL_MATRIX_TYPE( matrixf, float,  "matrixf" )

#undef DEFINE_OPL_VEC2_TYPE
#undef DEFINE_OPL_VEC3_TYPE
#undef DEFINE_OPL_VEC4_TYPE
#undef DEFINE_OPL_MATRIX_TYPE

	py::def( "make_identity", make_identity<float> );
	py::def( "make_identity", make_identity<double> );
}

} } }
