
// EXR - An ILM OpenEXR plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef EXR_CONFIG_INC_
#define EXR_CONFIG_INC_

#ifdef WIN32
	#ifdef EXR_EXPORTS
		#define EXR_DECLSPEC __declspec( dllexport )
	#else
		#define EXR_DECLSPEC __declspec( dllimport )
	#endif // EXR_EXPORTS
#else
	#ifdef EXR_EXPORTS
		#define EXR_DECLSPEC
	#else
		#define EXR_DECLSPEC
	#endif // EXR_EXPORTS
#endif // WIN32

#endif // EXR_CONFIG_INC_
