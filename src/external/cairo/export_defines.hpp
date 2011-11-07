#ifndef CAIRO_EXPORTS_INC_
#define CAIRO_EXPORTS_INC_

#ifdef WIN32
	#ifdef CAIRO_EXPORTS
		#define CAIRO_API __declspec( dllexport )
	#else
		#define CAIRO_API __declspec( dllimport )
	#endif // CAIRO_EXPORTS
#else
	#define CAIRO_API
#endif // WIN32

#endif

