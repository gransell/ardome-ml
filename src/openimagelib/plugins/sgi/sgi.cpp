
// SGI - An SGI plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openimagelib/plugins/sgi/sgi_plugin.hpp>

namespace opl = olib::openpluginlib;
namespace SGI = olib::openimagelib::plugins::SGI;

extern "C"
{
	SGI_DECLSPEC bool openplugin_init( void )
	{
		return true;
	}

	SGI_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}

	SGI_DECLSPEC bool openplugin_create_plugin( const char*, opl::openplugin** plug )
	{
		*plug = new SGI::SGI_plugin;
		return true;
	}
	
	SGI_DECLSPEC void openplugin_destroy_plugin( opl::openplugin* plug )
	{ delete static_cast<SGI::SGI_plugin*>( plug ); }
}
