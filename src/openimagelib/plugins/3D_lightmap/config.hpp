
// 3D_lightmap - A lightmap generator plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef LIGHTMAP3D_CONFIG_INC_
#define LIGHTMAP3D_CONFIG_INC_

#ifdef WIN32
	#ifdef LIGHTMAP3D_EXPORTS
		#define LIGHTMAP3D_DECLSPEC __declspec( dllexport )
	#else
		#define LIGHTMAP3D_DECLSPEC __declspec( dllimport )
	#endif // LIGHTMAP3D_EXPORTS
#else
	#ifdef LIGHTMAP3D_EXPORTS
		#define LIGHTMAP3D_DECLSPEC
	#else
		#define LIGHTMAP3D_DECLSPEC
	#endif // LIGHTMAP3D_EXPORTS
#endif // WIN32

#endif
