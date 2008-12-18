
// lightmap3D - An lightmap3D plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openimagelib/plugins/3D_lightmap/3D_lightmap_plugin.hpp>

namespace opl		= olib::openpluginlib;
namespace lightmap	= olib::openimagelib::plugins::lightmap;

extern "C"
{
	LIGHTMAP3D_DECLSPEC bool openplugin_init( void )
	{
		return true;
	}

	LIGHTMAP3D_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}

	LIGHTMAP3D_DECLSPEC bool openplugin_create_plugin( const char*, opl::openplugin** plug )
	{
		*plug = new lightmap::lightmap3D_plugin;
		return true;
	}
	
	LIGHTMAP3D_DECLSPEC void openplugin_destroy_plugin( opl::openplugin* plug )
	{ delete static_cast<lightmap::lightmap3D_plugin*>( plug ); }
}
