
// DPX - A DPX plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openimagelib/plugins/dpx/dpx_plugin.hpp>

namespace opl = olib::openpluginlib;
namespace DPX = olib::openimagelib::plugins::DPX;

extern "C"
{
	DPX_DECLSPEC bool openplugin_init( void )
	{
		return true;
	}

	DPX_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}

	DPX_DECLSPEC bool openplugin_create_plugin( const char*, opl::openplugin** plug )
	{
		*plug = new DPX::DPX_plugin;
		
		return true;
	}
	
	DPX_DECLSPEC void openplugin_destroy_plugin( opl::openplugin* plug )
	{ delete static_cast<DPX::DPX_plugin*>( plug ); }
}
