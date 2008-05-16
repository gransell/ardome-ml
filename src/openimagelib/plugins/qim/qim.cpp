
// QIM - An QImage plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openimagelib/plugins/qim/qim_plugin.hpp>

namespace opl = olib::openpluginlib;
namespace QIM = olib::openimagelib::plugins::QIM;

extern "C"
{
	QIM_DECLSPEC bool openplugin_init( void )
	{
		return true;
	}

	QIM_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}
	
	QIM_DECLSPEC bool openplugin_create_plugin( const char*, opl::openplugin** plug )
	{
		*plug = new QIM::QIM_plugin;
		return true;
	}
	
	QIM_DECLSPEC void openplugin_destroy_plugin( opl::openplugin* plug )
	{ delete static_cast<QIM::QIM_plugin*>( plug ); }
}
