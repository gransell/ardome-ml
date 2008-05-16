
// DDS - An DDS plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openimagelib/plugins/dds/dds_plugin.hpp>

namespace opl = olib::openpluginlib;
namespace DDS = olib::openimagelib::plugins::DDS;

extern "C"
{
	DDS_DECLSPEC bool openplugin_init( void )
	{
		return true;
	}

	DDS_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}

	DDS_DECLSPEC bool openplugin_create_plugin( const char*, opl::openplugin** plug )
	{
		*plug = new DDS::DDS_plugin;
		return true;
	}
	
	DDS_DECLSPEC void openplugin_destroy_plugin( opl::openplugin* plug )
	{ delete static_cast<DDS::DDS_plugin*>( plug ); }
}
