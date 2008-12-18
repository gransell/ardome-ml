
// PNG - An PNG plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openimagelib/plugins/png/png_plugin.hpp>

namespace opl = olib::openpluginlib;
namespace PNG = olib::openimagelib::plugins::PNG;

extern "C"
{
	PNG_DECLSPEC bool openplugin_init( void )
	{
		return true;
	}

	PNG_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}
	
	PNG_DECLSPEC bool openplugin_create_plugin( const char*, opl::openplugin** plug )
	{
		*plug = new PNG::PNG_plugin;
		return true;
	}
	
	PNG_DECLSPEC void openplugin_destroy_plugin( opl::openplugin* plug )
	{ delete static_cast<PNG::PNG_plugin*>( plug ); }
}
