
// QuickTime - An QuickTime plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef QUICKTIME_CONFIG_INC_
#define QUICKTIME_CONFIG_INC_

#ifdef WIN32
	#ifdef QUICKTIME_EXPORTS
		#define QUICKTIME_DECLSPEC __declspec( dllexport )
	#else
		#define QUICKTIME_DECLSPEC __declspec( dllimport )
	#endif
#else
	#ifdef QUICKTIME_EXPORTS
		#define QUICKTIME_DECLSPEC extern
	#else
		#define QUICKTIME_DECLSPEC
	#endif
#endif

#endif
