
// SGI - A SGI plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef SGI_CONFIG_INC_
#define SGI_CONFIG_INC_

#ifdef WIN32
	#ifdef SGI_EXPORTS
		#define SGI_DECLSPEC __declspec( dllexport )
	#else
		#define SGI_DECLSPEC __declspec( dllimport )
	#endif // SGI_EXPORTS
#else
	#ifdef SGI_EXPORTS
		#define SGI_DECLSPEC extern
	#else
		#define SGI_DECLSPEC
	#endif // SGI_EXPORTS
#endif // WIN32

#endif // SGI_CONFIG_INC_
