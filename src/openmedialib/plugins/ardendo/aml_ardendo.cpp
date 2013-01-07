// Ardendo AML Filter plugin
//
// Copyright (C) 2007 Ardendo
// Released under the LGPL.
//
// #plugin:ardendo
//
// This plugin provides a number of specific extensions to the AML system.
//
// Principally, it provides various plugins specific to video editing graphs
// such as the filter:compositor and the input:aml_stack:.

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include <iostream>

namespace aml { namespace openmedialib { 

// OML Input plugins
extern ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_aml( const std::wstring & );
extern ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_aml_stack( const std::wstring & );
extern ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_awi( const std::wstring & );
extern ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_silence( const std::wstring & );
extern ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_tone( const std::wstring & );

// OML Store plugins
extern ml::store_type_ptr ML_PLUGIN_DECLSPEC create_store_awi( const std::wstring & );
extern ml::store_type_ptr ML_PLUGIN_DECLSPEC create_store_null( );
extern ml::store_type_ptr ML_PLUGIN_DECLSPEC create_store_ppm( const std::wstring & );
extern ml::store_type_ptr ML_PLUGIN_DECLSPEC create_store_preview( const std::wstring &, const ml::frame_type_ptr & );

// OML Filter plugins
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_aml( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_audio_convert( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_charcoal( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_chroma_key( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_colour_space( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_compositor( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_extract( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_interlace( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_hold( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_invert( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_locked_audio( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_loop( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_mix_matrix( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_mono( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_montage( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_muxer( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_mvitc_decode( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_mvitc_write( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_offset( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_pitch( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_pulldown( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_repeat( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_sar( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_sleep( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_slots( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_step( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_store( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_tee( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_threader( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_transport( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_volume( const std::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_pass( const std::wstring & );

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC plugin : public ml::openmedialib_plugin
{
public:

	virtual ml::input_type_ptr input( const std::wstring &resource )
	{
		if ( resource.find( L".awi" ) != std::wstring::npos )
			return create_input_awi( resource );
		if ( resource.find( L"aml_stack:" ) == 0 || ( resource.find( L"=" ) == std::wstring::npos && resource.find( L".aml" ) != std::wstring::npos ) )
			return create_input_aml_stack( resource );
		if ( resource.find( L"silence:" ) == 0 )
			return create_input_silence( resource );
		if ( resource.find( L"tone:" ) == 0 )
			return create_input_tone( resource );
		return ml::input_type_ptr( );
	}

	virtual ml::store_type_ptr store( const std::wstring &resource, const ml::frame_type_ptr &frame )
	{
		if ( resource.find( L".awi" ) != std::wstring::npos || resource.find( L"awi:" ) == 0 )
			return create_store_awi( resource );
		if ( resource.find( L".ppm" ) != std::wstring::npos || resource.find( L"ppm:" ) == 0  )
			return create_store_ppm( resource );
		if ( resource == L"null:" )
			return create_store_null( );
		return create_store_preview( resource, frame );
	}
	
	virtual ml::filter_type_ptr filter( const std::wstring &resource )
	{
		if ( resource == L"aml" )
			return create_aml( resource );
		if ( resource == L"aml_pitch" )
			return create_pitch( resource );
		if ( resource == L"audio_convert" )
			return create_audio_convert( resource );
		if ( resource == L"charcoal" )
			return create_charcoal( resource );
		if ( resource == L"chroma_key" )
			return create_chroma_key( resource );
		if ( resource == L"colour_space" || resource == L"color_space" )
			return create_colour_space( resource );
		if ( resource == L"compositor" )
			return create_compositor( resource );
		if ( resource == L"extract" )
			return create_extract( resource );
		if ( resource == L"hold" )
			return create_hold( resource );
		if ( resource == L"interlace" )
			return create_interlace( resource );
		if ( resource == L"invert" )
			return create_invert( resource );
		if ( resource == L"locked_audio" )
			return create_locked_audio( resource );
		if ( resource == L"loop" )
			return create_loop( resource );
		if ( resource == L"mix_matrix" )
			return create_mix_matrix( resource );
		if ( resource == L"mono" )
			return create_mono( resource );
		if ( resource == L"montage" )
			return create_montage( resource );
		if ( resource == L"muxer" )
			return create_muxer( resource );
		if ( resource == L"mvitc_decode" )
			return create_mvitc_decode( resource );
		if ( resource == L"mvitc_write" )
			return create_mvitc_write( resource );
		if ( resource == L"offset" )
			return create_offset( resource );
		if ( resource == L"pitch" )
			return create_pitch( resource );
		if ( resource == L"pulldown" )
			return create_pulldown( resource );
		if ( resource == L"repeat" )
			return create_repeat( resource );
		if ( resource == L"sar" )
			return create_sar( resource );
		if ( resource == L"sleep" )
			return create_sleep( resource );
		if ( resource == L"slots" )
			return create_slots( resource );
		if ( resource == L"store" )
			return create_store( resource );
		if ( resource == L"step" )
			return create_step( resource );
		if ( resource == L"tee" )
			return create_tee( resource );
		if ( resource == L"threader" )
			return create_threader( resource );
		if ( resource == L"transport" )
			return create_transport( resource );
		if ( resource == L"volume" )
			return create_volume( resource );
		if ( resource == L"lowpass" )
		  return create_pass( resource );

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
		return true;
	}

	ML_PLUGIN_DECLSPEC bool openplugin_uninit( void )
	{
		return true;
	}
	
	ML_PLUGIN_DECLSPEC bool openplugin_create_plugin( const char*, pl::openplugin** plug )
	{
		*plug = new aml::openmedialib::plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< aml::openmedialib::plugin * >( plug ); 
	}
}

