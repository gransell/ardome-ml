
// HDR - An HDR plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef HDR_CONFIG_INC_
#define HDR_CONFIG_INC_

#ifdef WIN32
	#ifdef HDR_EXPORTS
		#define HDR_DECLSPEC __declspec( dllexport )
	#else
		#define HDR_DECLSPEC __declspec( dllimport )
	#endif // HDR_EXPORTS
#else
	#ifdef HDR_EXPORTS
		#define HDR_DECLSPEC extern
	#else
		#define HDR_DECLSPEC
	#endif // HDR_EXPORTS
#endif // WIN32

#endif // HDR_CONFIG_INC_
