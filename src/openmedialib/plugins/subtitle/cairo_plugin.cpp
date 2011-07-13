// Librsvg AML Filter plugin
//
// Copyright (C) 2009 Vizrt
// Released under the LGPL.
//
// #plugin:subtitle
//
// Provides a plugin for subtitle use.

// TODO REMOVE maybe
#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>
#include <openpluginlib/pl/pcos/isubject.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>

namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;
namespace cl = olib::opencorelib;
namespace pcos = olib::openpluginlib::pcos;

namespace aml { namespace openmedialib { 

// OML Input plugins
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_filter_subtitle( const pl::wstring & );

extern void olib_cairo_init();

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC cairo_plugin : public ml::openmedialib_plugin
{
public:

	virtual ml::filter_type_ptr filter( const pl::wstring &resource )
	{
		if ( resource == L"subtitle" )
			return create_filter_subtitle( resource );
		return ml::filter_type_ptr( );
	}
};

} }


//
// Access methods for openpluginlib
//

extern "C"
{
	ML_PLUGIN_DECLSPEC bool openplugin_init( void )
	{
		aml::openmedialib::olib_cairo_init( );

		return true;
	}

	ML_PLUGIN_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}
	
	ML_PLUGIN_DECLSPEC bool openplugin_create_plugin( const char*, pl::openplugin** plug )
	{
		*plug = new aml::openmedialib::cairo_plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< aml::openmedialib::cairo_plugin * >( plug ); 
	}
}

