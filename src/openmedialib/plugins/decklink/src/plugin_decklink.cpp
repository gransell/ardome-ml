// AML DNXHD Filters
//
// Copyright (C) 2010 Vizrt

#include <iostream>

#include <boost/cstdint.hpp>

#include <opencorelib/cl/guard_define.hpp>
#include <openmedialib/ml/openmedialib_plugin.hpp>

#include "decklink_utilities.h"

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
	
extern ml::input_type_ptr create_input_decklink( const pl::wstring& resource );
extern ml::store_type_ptr create_store_decklink( const pl::wstring& resource, const ml::frame_type_ptr& frame );
	
bool is_decklink_device_present( )
{
	decklink::utilities::decklink_iterator_ptr dli( CreateDeckLinkIteratorInstance(), false );
	
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
	virtual ml::input_type_ptr input( const pl::wstring &resource )
	{
		return create_input_decklink( resource );
	}
	
	virtual ml::store_type_ptr store( const pl::wstring &resource, const ml::frame_type_ptr &frame )
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
		if( !amf::openmedialib::is_decklink_device_present( ) ) return false;
		
		*plug = new amf::openmedialib::aml_decklink;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< amf::openmedialib::aml_decklink * >( plug ); 
	}
}

