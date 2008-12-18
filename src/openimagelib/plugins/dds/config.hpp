
// DDS - An DDS plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef DDS_CONFIG_INC_
#define DDS_CONFIG_INC_

#ifdef WIN32
	#ifdef DDS_EXPORTS
		#define DDS_DECLSPEC __declspec( dllexport )
	#else
		#define DDS_DECLSPEC __declspec( dllimport )
	#endif // DDS_EXPORTS
#else
	#ifdef DDS_EXPORTS
		#define DDS_DECLSPEC __attribute__( ( visibility( "default" ) ) )
	#else
		#define DDS_DECLSPEC
	#endif // DDS_EXPORTS
#endif // WIN32

#endif // DDS_CONFIG_INC_
