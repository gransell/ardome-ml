
// TGA - An TGA plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef TGA_CONFIG_INC_
#define TGA_CONFIG_INC_

#ifdef WIN32
	#ifdef TGA_EXPORTS
		#define TGA_DECLSPEC __declspec( dllexport )
	#else
		#define TGA_DECLSPEC __declspec( dllimport )
	#endif // TGA_EXPORTS
#else
	#ifdef TGA_EXPORTS
		#define TGA_DECLSPEC extern
	#else
		#define TGA_DECLSPEC
	#endif // TGA_EXPORTS
#endif // WIN32

#endif // TGA_CONFIG_INC_
