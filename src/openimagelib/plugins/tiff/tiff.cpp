
// TIFF - A TIFF plugin to il.

// Copyright (C) 2005-2006 Visual Media FX Ltd.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openimagelib/plugins/tiff/tiff_plugin.hpp>

namespace opl = olib::openpluginlib;
namespace TIFF = olib::openimagelib::plugins::TIFF;

extern "C"
{
	TIFF_DECLSPEC bool openplugin_init( void )
	{
		return true;
	}

	TIFF_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}

	TIFF_DECLSPEC bool openplugin_create_plugin( const char*, opl::openplugin** plug )
	{
		*plug = new TIFF::TIFF_plugin;
		return true;
	}
	
	TIFF_DECLSPEC void openplugin_destroy_plugin( opl::openplugin* plug )
	{ delete static_cast<TIFF::TIFF_plugin*>( plug ); }
}
