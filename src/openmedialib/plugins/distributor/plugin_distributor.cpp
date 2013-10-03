// Copyright (C) 2010-2013 Vizrt
// Released under the LGPL.

#include <openmedialib/ml/openmedialib_plugin.hpp>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;

namespace olib { namespace openmedialib { namespace ml { namespace distributor {

input_type_ptr create_nudger();

filter_type_ptr create_fork();
filter_type_ptr create_lock();
filter_type_ptr create_distributor();
filter_type_ptr create_voodoo();

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input( const std::wstring &spec )
	{
		if( spec == L"nudger:" )
			return create_nudger();
		else
			return input_type_ptr();
	}

	virtual filter_type_ptr filter( const std::wstring &spec )
	{
		if ( spec == L"fork" )
			return create_fork();
		else if ( spec == L"lock" )
			return create_lock();
		else if ( spec == L"distributor" )
			return create_distributor();
		else if ( spec == L"voodoo" )
			return create_voodoo();
		else
			return filter_type_ptr( );
	}
};

} } } }

//
// Access methods for openpluginlib
//

extern "C"
{
	ML_PLUGIN_DECLSPEC bool openplugin_init( void )
	{
		return true;
	}

	ML_PLUGIN_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}
	
	ML_PLUGIN_DECLSPEC bool openplugin_create_plugin( const char*, pl::openplugin** plug )
	{
		*plug = new ml::distributor::plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< ml::distributor::plugin * >( plug ); 
	}
}
