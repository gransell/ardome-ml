// AMF Filter plugin
//
// Copyright (C) 2007 Ardendo

#include "precompiled_headers.hpp"
#include "amf_filter_plugin.hpp"
#include <iostream>

namespace aml { namespace openmedialib { 

// OML Input plugins
extern ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_aml( const pl::wstring & );
extern ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_aml_stack( const pl::wstring & );
extern ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_awi( const pl::wstring & );
extern ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_librsvg( const pl::wstring & );
extern ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_silence( const pl::wstring & );
extern ml::input_type_ptr ML_PLUGIN_DECLSPEC create_input_tone( const pl::wstring & );

// OML Store plugins
extern ml::store_type_ptr ML_PLUGIN_DECLSPEC create_store_null( );
extern ml::store_type_ptr ML_PLUGIN_DECLSPEC create_store_ppm( const pl::wstring & );
extern ml::store_type_ptr ML_PLUGIN_DECLSPEC create_store_preview( );

// OML Filter plugins
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_aml( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_audio_convert( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_charcoal( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_chroma_key( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_colour_space( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_compositor( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_extract( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_invert( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_locked_audio( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_loop( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_mono( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_montage( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_muxer( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_mvitc_decode( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_mvitc_write( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_offset( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_pitch( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_sar( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_slots( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_step( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_store( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_tee( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_threader( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_title( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_transport( const pl::wstring & );
extern ml::filter_type_ptr ML_PLUGIN_DECLSPEC create_volume( const pl::wstring & );

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC plugin : public ml::openmedialib_plugin
{
public:

	virtual ml::input_type_ptr input( const pl::wstring &resource )
	{
		if ( resource.find( L"svg:" ) == 0 )
			return create_input_librsvg( resource );
		if ( resource.find( L".svg" ) != pl::wstring::npos )
			return create_input_librsvg( resource );
		if ( resource.find( L".awi" ) != pl::wstring::npos )
			return create_input_awi( resource );
		if ( resource.find( L"aml_stack:" ) == 0 || ( resource.find( L"=" ) == pl::wstring::npos && resource.find( L".aml" ) != pl::wstring::npos ) )
			return create_input_aml_stack( resource );
		if ( resource.find( L"silence:" ) == 0 )
			return create_input_silence( resource );
		if ( resource.find( L"tone:" ) == 0 )
			return create_input_tone( resource );
		return ml::input_type_ptr( );
	}

	virtual ml::store_type_ptr store( const pl::wstring &resource, const ml::frame_type_ptr & )
	{
		if ( resource.find( L".ppm" ) != pl::wstring::npos )
			return create_store_ppm( resource );
		if ( resource == L"null:" )
			return create_store_null( );
		return create_store_preview( );
	}
	
	virtual ml::filter_type_ptr filter( const pl::wstring &resource )
	{
		if ( resource == L"aml" )
			return create_aml( resource );
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
		if ( resource == L"invert" )
			return create_invert( resource );
		if ( resource == L"locked_audio" )
			return create_locked_audio( resource );
		if ( resource == L"loop" )
			return create_loop( resource );
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
		if ( resource == L"sar" )
			return create_sar( resource );
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
		if ( resource == L"title" )
			return create_title( resource );
		if ( resource == L"transport" )
			return create_transport( resource );
		if ( resource == L"volume" )
			return create_volume( resource );
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

