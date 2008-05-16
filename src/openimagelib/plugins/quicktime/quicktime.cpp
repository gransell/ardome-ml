
// QuickTime - An QuickTime plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifdef WIN32
#include <QTML.h>

#include <boost/thread/recursive_mutex.hpp>
#endif

#include <openimagelib/plugins/quicktime/quicktime_plugin.hpp>

namespace opl = olib::openpluginlib;
namespace QT  = olib::openimagelib::plugins::QT;

#ifdef WIN32
namespace
{
	bool reflib( int init )
	{
		static long refs = 0;

		assert( refs >= 0 && L" QuickTime_plugin::refinit: refs is negative." );
		
		if( init > 0 && ++refs == 1 )
		{
			OSStatus err = InitializeQTML( 0L );
			if( err != noErr )
				return false;
		}
		else if( init < 0 && --refs == 0 )
		{
		}
		
		return true;
	}
	
	boost::recursive_mutex mutex;
}
#endif

extern "C"
{
	QUICKTIME_DECLSPEC bool openplugin_init( void )
	{
#ifdef WIN32
		boost::recursive_mutex::scoped_lock lock( mutex );
	
		return reflib( 1 );
#else
		return true;
#endif
	}

	QUICKTIME_DECLSPEC bool openplugin_uninit( void )
	{
#ifdef WIN32
		boost::recursive_mutex::scoped_lock lock( mutex );
		
		return reflib( -1 );
#else
		return true;
#endif
	}

	QUICKTIME_DECLSPEC bool openplugin_create_plugin( const char*, opl::openplugin** plug )
	{
		*plug = new QT::QT_plugin;
		return true;
	}
	
	QUICKTIME_DECLSPEC void openplugin_destroy_plugin( opl::openplugin* plug )
	{ delete static_cast<QT::QT_plugin*>( plug ); }
}
