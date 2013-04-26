// caca - A caca plugin to ml.
//
// Copyright (C) 2005 Visual Media FX Ltd.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <caca0.h>

#include <openmedialib/ml/openmedialib_plugin.hpp>

#ifdef WIN32
#include <windows.h>
#endif // WIN32

#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>

#include <boost/thread/recursive_mutex.hpp>

namespace opl = olib::openpluginlib;
namespace plugin = olib::openmedialib::ml;


namespace olib { namespace openmedialib { namespace ml { 

class ML_PLUGIN_DECLSPEC caca_store : public store_type
{
	public:
		caca_store( ) 
			: store_type( )
			, bitmap_( 0 )
		{
			caca_init( );
			caca_clear( );
		}

		virtual ~caca_store( )
		{
			if ( bitmap_ )
				caca_free_bitmap( bitmap_ );
			caca_end( );
		}

		virtual bool push( frame_type_ptr frame )
		{
			ml::image_type_ptr img = frame->get_image( );
			if ( img != 0 )
			{
				img = ml::image::convert( img, L"r8g8b8a8" );
				int w = img->width( );
				int h = img->height( );
        		unsigned int const event_mask = CACA_EVENT_RESIZE;
				if ( caca_get_event( event_mask ) )
					caca_refresh( );
				if ( bitmap_ == 0 )
					bitmap_ = caca_create_bitmap(32, w, h, 4 * w, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
				caca_draw_bitmap(0, 0, caca_get_width() - 1, caca_get_height() - 1, bitmap_, (char *)img->data( ) );
				caca_refresh( );
			}
			return img != 0;
		}

	private:
		struct caca_bitmap *bitmap_;
};

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC caca_plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input(  const std::wstring & )
	{
		return input_type_ptr( );
	}

	virtual store_type_ptr store( const std::wstring &, const frame_type_ptr & )
	{
		return store_type_ptr( new caca_store( ) );
	}
};

} } }

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
	
	ML_PLUGIN_DECLSPEC bool openplugin_create_plugin( const char*, opl::openplugin** plug )
	{
		*plug = new plugin::caca_plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( opl::openplugin* plug )
	{ 
		delete static_cast< plugin::caca_plugin * >( plug ); 
	}
}
