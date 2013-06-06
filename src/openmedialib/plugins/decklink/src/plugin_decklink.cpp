// AML DNXHD Filters
//
// Copyright (C) 2010 Vizrt

#include <iostream>

#include <boost/cstdint.hpp>

#include <opencorelib/cl/guard_define.hpp>
#include <openmedialib/ml/openmedialib_plugin.hpp>

#include "decklink_utilities.h"
#include <DeckLinkAPIDispatch.cpp>

namespace cl = olib::opencorelib;
namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;
namespace pcos = olib::openpluginlib::pcos;

void intrusive_ptr_add_ref( IUnknown *p )
{
	p->AddRef();
}

void intrusive_ptr_release( IUnknown *p )
{
	p->Release();
}

namespace amf { namespace openmedialib { 
	
extern ml::input_type_ptr create_input_decklink( const std::wstring& resource );
extern ml::store_type_ptr create_store_decklink( const std::wstring& resource, const ml::frame_type_ptr& frame );

bool is_decklink_device_present( )
{
#ifndef _WIN32
	decklink::utilities::decklink_iterator_ptr dli( CreateDeckLinkIteratorInstance(), false );
#else
	IDeckLinkIterator *deckLinkIterator = NULL;
	CoCreateInstance(CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL, IID_IDeckLinkIterator, (void**)&deckLinkIterator);
	decklink::utilities::decklink_iterator_ptr dli( deckLinkIterator, false );
#endif
	
	if( !dli ) return false;
		
	IDeckLink *deck_link = NULL;
	if( dli->Next( &deck_link ) == S_OK ) return true;
	
	return false;
}
	
//
// Plugin object
//

class ML_PLUGIN_DECLSPEC aml_decklink : public ml::openmedialib_plugin
{
public:
	virtual ml::input_type_ptr input( const std::wstring &resource )
	{
		return create_input_decklink( resource );
	}

	virtual ml::store_type_ptr store( const std::wstring &resource, const ml::frame_type_ptr &frame )
	{
		return create_store_decklink( resource, frame );
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
		ARLOG_INFO( "Decklink plugin loaded." );
#		ifdef WIN32
		HRESULT error = CoInitialize( NULL );
		if ( error != S_OK )
		{
			ARLOG( "COM failed to initialise." ).level( cl::log_level::error );
			return false;
		}
#		endif
		if( !amf::openmedialib::is_decklink_device_present( ) ) 
		{
			ARLOG( "No Decklink device found." ).level( cl::log_level::error );
			return false;
		}
		ARLOG_INFO( "Decklink plugin initialised." );
		*plug = new amf::openmedialib::aml_decklink;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< amf::openmedialib::aml_decklink * >( plug ); 
	}
}

