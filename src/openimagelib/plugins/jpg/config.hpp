
// JPG - An JPG plugin to il.

// Copyright (C) 2005 Visual Media FX Ltd.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef JPG_CONFIG_INC_
#define JPG_CONFIG_INC_

#ifdef WIN32
	#ifdef JPG_EXPORTS
		#define JPG_DECLSPEC __declspec( dllexport )
	#else
		#define JPG_DECLSPEC __declspec( dllimport )
	#endif // JPG_EXPORTS
#else
	#ifdef JPG_EXPORTS
		#define JPG_DECLSPEC extern
	#else
		#define JPG_DECLSPEC
	#endif // JPG_EXPORTS
#endif // WIN32

#endif // JPG_CONFIG_INC_
