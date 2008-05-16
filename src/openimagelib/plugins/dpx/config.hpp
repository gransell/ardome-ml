
// dpx - A dpx plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef DPX_CONFIG_INC_
#define DPX_CONFIG_INC_

#ifdef WIN32
	#ifdef DPX_EXPORTS
		#define DPX_DECLSPEC __declspec( dllexport )
	#else
		#define DPX_DECLSPEC __declspec( dllimport )
	#endif // DPX_EXPORTS
#else
	#ifdef DPX_EXPORTS
		#define DPX_DECLSPEC
	#else
		#define DPX_DECLSPEC
	#endif // DPX_EXPORTS
#endif // WIN32

#endif
