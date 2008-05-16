
// TIFF - A TIFF plugin to il.

// Copyright (C) 2005-2006 Visual Media FX Ltd.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef TIFF_CONFIG_INC_
#define TIFF_CONFIG_INC_

#ifdef WIN32
	#ifdef TIFF_EXPORTS
		#define TIFF_DECLSPEC __declspec( dllexport )
	#else
		#define TIFF_DECLSPEC __declspec( dllimport )
	#endif // TIFF_EXPORTS
#else
	#ifdef TIFF_EXPORTS
		#define TIFF_DECLSPEC extern
	#else
		#define TIFF_DECLSPEC
	#endif // TIFF_EXPORTS
#endif // WIN32

#endif
