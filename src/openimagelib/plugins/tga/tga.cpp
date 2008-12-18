
// TGA - An TGA plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openimagelib/plugins/tga/tga_plugin.hpp>

namespace opl = olib::openpluginlib;
namespace TGA = olib::openimagelib::plugins::TGA;

extern "C"
{
	TGA_DECLSPEC bool openplugin_init( void )
	{
		return true;
	}

	TGA_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}

	TGA_DECLSPEC bool openplugin_create_plugin( const char*, opl::openplugin** plug )
	{
		*plug = new TGA::TGA_plugin;
		return true;
	}
	
	TGA_DECLSPEC void openplugin_destroy_plugin( opl::openplugin* plug )
	{ delete static_cast<TGA::TGA_plugin*>( plug ); }
}
