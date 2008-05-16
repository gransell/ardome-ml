
// PNG - A PNG plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef PNG_CONFIG_INC_
#define PNG_CONFIG_INC_

#ifdef WIN32
	#ifdef PNG_EXPORTS
		#define PNG_DECLSPEC __declspec( dllexport )
	#else
		#define PNG_DECLSPEC __declspec( dllimport )
	#endif // PNG_EXPORTS
#else
	#ifdef PNG_EXPORTS
		#define PNG_DECLSPEC extern
	#else
		#define PNG_DECLSPEC
	#endif // PNG_EXPORTS
#endif // WIN32

#endif // PNG_CONFIG_INC_
