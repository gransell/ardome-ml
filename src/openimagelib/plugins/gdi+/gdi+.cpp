
// GDI+ - An GDI+ plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifdef WIN32
#include <windows.h>
#include <gdiplus.h>
#endif // WIN32

#ifndef BOOST_THREAD_DYN_DLL
    #define BOOST_THREAD_DYN_DLL
#endif
#include <boost/thread/recursive_mutex.hpp>

#include <openimagelib/plugins/gdi+/gdi+_plugin.hpp>

namespace opl = olib::openpluginlib;
namespace GDI = olib::openimagelib::plugins::GDI;

namespace
{
	void reflib( int init )
	{
		static long refs = 0;
#ifdef WIN32
		static ULONG_PTR gdiplusToken;
#endif // WIN32

		assert( refs >= 0 && L" GDI+_plugin::refinit: refs is negative." );
		
		if( init > 0 && ++refs )
		{
#	ifdef WIN32
			Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		
			Gdiplus::GdiplusStartup( &gdiplusToken, &gdiplusStartupInput, NULL );
#	endif
		}
		else if( init < 0 && --refs == 0 )
		{
#	ifdef WIN32
			Gdiplus::GdiplusShutdown( gdiplusToken );
#	endif
		}
	}
	
	boost::recursive_mutex mutex;
}

extern "C"
{
	GDI_DECLSPEC bool openplugin_init( void )
	{
		boost::recursive_mutex::scoped_lock lock( mutex );
		
		reflib( 1 );

		return true;
	}

	GDI_DECLSPEC bool openplugin_uninit( void )
	{
		boost::recursive_mutex::scoped_lock lock( mutex );

		reflib( -1 );
		
		return true;
	}
	
	GDI_DECLSPEC bool openplugin_create_plugin( const char*, opl::openplugin** plug )
	{
		*plug = new GDI::GDI_plugin;
		
		return true;
	}
	
	GDI_DECLSPEC void openplugin_destroy_plugin( opl::openplugin* plug )
	{ delete static_cast<GDI::GDI_plugin*>( plug ); }
}
