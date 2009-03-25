
// EXR - An ILM OpenEXR plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openimagelib/plugins/exr/exr_plugin.hpp>

namespace opl = olib::openpluginlib;
namespace EXR = olib::openimagelib::plugins::EXR;

extern "C"
{
	EXR_DECLSPEC bool openplugin_init( void )
	{
		return true;
	}

	EXR_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}
	
	EXR_DECLSPEC bool openplugin_create_plugin( const char*, opl::openplugin** plug )
	{
		*plug = new EXR::EXR_plugin;
		return true;
	}
	
	EXR_DECLSPEC void openplugin_destroy_plugin( opl::openplugin* plug )
	{ delete static_cast<EXR::EXR_plugin*>( plug ); }
}
