
// HDR - An HDR plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openimagelib/plugins/hdr/hdr_plugin.hpp>

namespace opl = olib::openpluginlib;
namespace HDR = olib::openimagelib::plugins::HDR;

extern "C"
{
	HDR_DECLSPEC bool openplugin_init( void )
	{
		return true;
	}

	HDR_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}

	HDR_DECLSPEC bool openplugin_create_plugin( const char*, opl::openplugin** plug )
	{
		*plug = new HDR::HDR_plugin;
		
		return true;
	}
	
	HDR_DECLSPEC void openplugin_destroy_plugin( opl::openplugin* plug )
	{ delete static_cast<HDR::HDR_plugin*>( plug ); }
}
