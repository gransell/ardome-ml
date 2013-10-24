// decode - A decode plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include <openmedialib/ml/openmedialib_plugin.hpp>

#include "filter_analyse.hpp"
#include "filter_decode.hpp"
#include "filter_encode.hpp"
#include "filter_field_order.hpp"
#include "filter_lazy.hpp"
#include "filter_map_reduce.hpp"
#include "filter_rescale.hpp"

namespace olib { namespace openmedialib { namespace ml { namespace decode {

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input( const std::wstring & )
	{
		return input_type_ptr( );
	}

	virtual store_type_ptr store( const std::wstring &, const frame_type_ptr & )
	{
		return store_type_ptr( );
	}

	virtual filter_type_ptr filter( const std::wstring &spec )
	{
		if ( spec == L"analyse" )
			return filter_type_ptr( new filter_analyse( ) );
		if ( spec == L"decode" )
			return filter_type_ptr( new filter_decode( ) );
		if ( spec == L"encode" )
			return filter_type_ptr( new filter_encode( ) );
		if ( spec == L"field_order" )
			return filter_type_ptr( new filter_field_order( ) );
		if ( spec.find( L"lazy:" ) == 0 )
			return filter_type_ptr( new filter_lazy( spec ) );
		if ( spec == L"map_reduce" )
			return filter_type_ptr( new filter_map_reduce( ) );
		if ( spec == L"rescale" || spec == L"aml_rescale" )
			return filter_type_ptr( new filter_rescale( ) );
		return ml::filter_type_ptr( );
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
		*plug = new ml::decode::plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< ml::decode::plugin * >( plug ); 
	}
}
