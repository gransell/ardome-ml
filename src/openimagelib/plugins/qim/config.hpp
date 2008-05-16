
// QIM - A QImage plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef QIM_CONFIG_INC_
#define QIM_CONFIG_INC_

#ifdef WIN32
	#ifdef QIM_EXPORTS
		#define QIM_DECLSPEC __declspec( dllexport )
	#else
		#define QIM_DECLSPEC __declspec( dllimport )
	#endif // QIM_EXPORTS
#else
	#ifdef QIM_EXPORTS
		#define QIM_DECLSPEC extern
	#else
		#define QIM_DECLSPEC
	#endif // QIM_EXPORTS
#endif // WIN32

#endif // QIM_CONFIG_INC_
