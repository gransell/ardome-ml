
// JPG - An JPG plugin to il.

// Copyright (C) 2005 Visual Media FX Ltd.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifdef WIN32
#include <windows.h>
#include <gdiplus.h>
#endif // WIN32

#include <boost/thread/recursive_mutex.hpp>

#include <openimagelib/plugins/jpg/jpg_plugin.hpp>

namespace opl = olib::openpluginlib;
namespace JPG = olib::openimagelib::plugins::JPG;

namespace
{
	void reflib( int init )
	{
		static long refs = 0;
#ifdef WIN32
		static ULONG_PTR gdiplusToken;
#endif // WIN32

		assert( refs >= 0 && L"JPG_plugin::refinit: refs is negative." );
		
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
	
#ifdef WIN32
	ULONG_PTR gdiplusToken;
	
	bool has_jpg_decoder_installed( void )
	{
		UINT num, size;
		Gdiplus::GetImageDecodersSize( &num, &size );
		
		Gdiplus::ImageCodecInfo* decoders = ( Gdiplus::ImageCodecInfo* ) malloc( size );
		Gdiplus::GetImageDecoders( num, size, decoders );
	
		bool found = false;
		for( UINT i = 0; i < num; ++i )
		{
			if( !wcscmp( decoders[ i ].MimeType, L"image/jpeg" ) )
			{
				found = true;

				break;
			}
		}
	
		free( decoders );

		return found;
	}
#endif
}

extern "C"
{
	JPG_DECLSPEC bool openplugin_init( void )
	{
		boost::recursive_mutex mutex;
		
		reflib( 1 );

#	ifdef WIN32
		if( !has_jpg_decoder_installed( ) )
			return false;
#	endif

		return true;
	}

	JPG_DECLSPEC bool openplugin_uninit( void )
	{
		boost::recursive_mutex mutex;

		reflib( -1 );
		
		return true;
	}
	
	JPG_DECLSPEC bool openplugin_create_plugin( const char*, opl::openplugin** plug )
	{
		*plug = new JPG::JPG_plugin;
		
		return true;
	}
	
	JPG_DECLSPEC void openplugin_destroy_plugin( opl::openplugin* plug )
	{ delete static_cast<JPG::JPG_plugin*>( plug ); }
}
