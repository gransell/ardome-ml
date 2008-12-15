
// GDI+ - An GDI+ plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef GDI_CONFIG_INC_
#define GDI_CONFIG_INC_

#ifdef WIN32
	#ifdef GDI_EXPORTS
		#define GDI_DECLSPEC __declspec( dllexport )
	#else
		#define GDI_DECLSPEC __declspec( dllimport )
	#endif // GDI_EXPORTS
#else
	#ifdef GDI_EXPORTS
		#define GDI_DECLSPEC __attribute__( ( visibility( "default" ) ) )
	#else
		#define GDI_DECLSPEC
	#endif // GDI_EXPORTS
#endif // WIN32

#endif // GDI_CONFIG_INC_
