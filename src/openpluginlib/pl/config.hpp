
// openpluginlib - A plugin interface to openlibraries.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef OPENPLUGINLIB_CONFIG_INC_
#define OPENPLUGINLIB_CONFIG_INC_

#ifdef WIN32
	// class 'xxx' needs to have dll-interface to be used by 
	// clients of class 'yyy'
	#pragma warning( disable : 4251 ) 
#	ifdef OPENPLUGINLIB_EXPORTS
#		define OPENPLUGINLIB_DECLSPEC __declspec( dllexport )
#	else
#		define OPENPLUGINLIB_DECLSPEC __declspec( dllimport )
#	endif
#else
#	define OPENPLUGINLIB_DECLSPEC
#endif

#ifndef OPENPLUGINLIB_LIBNAME
#	define OPENPLUGINLIB_LIBNAME "openpluginlib_pl"
#endif

#ifndef OPENPLUGINLIB_TOOLSET
#	if defined( _MSC_VER ) && ( _MSC_VER == 1310 ) && !defined( __ICL )
#		define OPENPLUGINLIB_TOOLSET "vc71"

#	elif defined( _MSC_VER ) && ( _MSC_VER >= 1400 )
#		define OPENPLUGINLIB_TOOLSET "vc80"

#	elif defined( __ICL ) && ( __INTEL_COMPILER >= 900 ) && ( _MSC_VER == 1310 )
#		define OPENPLUGINLIB_TOOLSET "iw-vc71"
#	endif
#endif

#ifndef OPENPLUGINLIB_DEBUG_SUFFIX
#	ifndef NDEBUG
#		define OPENPLUGINLIB_DEBUG_SUFFIX "d"
#	else
#		define OPENPLUGINLIB_DEBUG_SUFFIX "r"		
#	endif
#endif

#ifndef OPENPLUGINLIB_VERSION
#	define OPENPLUGINLIB_VERSION "0_4_1"
#endif

// library search record (auto link).
#if defined( WIN32 ) && !defined( OPENPLUGINLIB_BUILD )
#	if defined( OPENPLUGINLIB_LIBNAME ) && defined( OPENPLUGINLIB_TOOLSET ) && defined( OPENPLUGINLIB_DEBUG_SUFFIX ) && defined( OPENPLUGINLIB_VERSION )
#		pragma comment( lib, OPENPLUGINLIB_LIBNAME "-" OPENPLUGINLIB_TOOLSET "-" OPENPLUGINLIB_DEBUG_SUFFIX "-" OPENPLUGINLIB_VERSION ".lib" )
#	endif
#endif

#endif
