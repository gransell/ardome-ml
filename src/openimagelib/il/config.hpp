
// il - A image library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef IL_CONFIG_INC_
#define IL_CONFIG_INC_

#ifdef WIN32
	#ifdef IL_EXPORTS
		#define IL_DECLSPEC __declspec( dllexport )
	#else
		#define IL_DECLSPEC __declspec( dllimport )
	#endif // IL_EXPORTS
#else
	#define IL_DECLSPEC
#endif // WIN32

#ifndef OPENIMAGELIB_LIBNAME
#	define OPENIMAGELIB_LIBNAME "openimagelib_il"
#endif

#ifndef OPENIMAGELIB_TOOLSET
#	if defined( _MSC_VER ) && ( _MSC_VER == 1310 ) && !defined( __ICL )
#		define OPENIMAGELIB_TOOLSET "vc71"

#	elif defined( _MSC_VER ) && ( _MSC_VER >= 1400 )
#		define OPENIMAGELIB_TOOLSET "vc80"

#	elif defined( __ICL ) && ( __INTEL_COMPILER >= 900 ) && ( _MSC_VER == 1310 )
#		define OPENIMAGELIB_TOOLSET "iw-vc71"
#	endif
#endif

#ifndef OPENIMAGELIB_DEBUG_SUFFIX
#	ifndef NDEBUG
#		define OPENIMAGELIB_DEBUG_SUFFIX "d"
#	else
#		define OPENIMAGELIB_DEBUG_SUFFIX "r"
#	endif
#endif

#ifndef OPENIMAGELIB_VERSION
#	define OPENIMAGELIB_VERSION "0_4_1"
#endif

// library search record (auto link).
#if defined( WIN32 ) && !defined( OPENIMAGELIB_BUILD )
#	if defined( OPENIMAGELIB_LIBNAME ) && defined( OPENIMAGELIB_TOOLSET ) && defined( OPENIMAGELIB_DEBUG_SUFFIX ) && defined( OPENIMAGELIB_VERSION )
#		pragma comment( lib, OPENIMAGELIB_LIBNAME "-" OPENIMAGELIB_TOOLSET "-" OPENIMAGELIB_DEBUG_SUFFIX "-" OPENIMAGELIB_VERSION ".lib" )
#	endif
#endif

#endif
