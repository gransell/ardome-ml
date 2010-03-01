// Librsvg AML Filter plugin
//
// Copyright (C) 2009 Vizrt
// Released under the LGPL.
//
// #plugin:rsvg
//
// Provides a plugin for svg use.

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
extern ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_librsvg( const pl::wstring & );

// OML Filter plugins
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_title( const pl::wstring & );

extern void olib_rsvg_init();

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC rsvg_plugin : public ml::openmedialib_plugin
{
public:

	virtual ml::input_type_ptr input( const pl::wstring &resource )
	{
		if ( resource.find( L"svg:" ) == 0 )
			return create_input_librsvg( resource );
		if ( resource.find( L".svg" ) != pl::wstring::npos )
			return create_input_librsvg( resource );
		return ml::input_type_ptr( );
	}

	virtual ml::filter_type_ptr filter( const pl::wstring &resource )
	{
		if ( resource == L"title" )
			return create_title( resource );
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
		aml::openmedialib::olib_rsvg_init( );

		return true;
	}

	ML_PLUGIN_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}
	
	ML_PLUGIN_DECLSPEC bool openplugin_create_plugin( const char*, pl::openplugin** plug )
	{
		*plug = new aml::openmedialib::rsvg_plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< aml::openmedialib::rsvg_plugin * >( plug ); 
	}
}

