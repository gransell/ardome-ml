
// template - A template plugin to ml.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

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
namespace il = olib::openimagelib::il;

namespace olib { namespace openmedialib { namespace ml { 

class ML_PLUGIN_DECLSPEC template_input : public input_type
{
	public:
		// Constructor and destructor
		template_input( ) : input_type( ) { }
		virtual ~template_input( ) { }

		// Basic information
		virtual const opl::wstring get_uri( ) const { return L"test:"; }
		virtual const opl::wstring get_mime_type( ) const { return L""; }

		// Audio/Visual
		virtual int get_frames( ) const { return 250; }
		virtual bool is_seekable( ) const { return true; }

		// Visual
		virtual void get_fps( int &num, int &den ) const { num = 25; den = 1; }
		virtual void get_sar( int &num, int &den ) const { num = 1; den = 1; }
		virtual int get_video_streams( ) const { return 1; }
		virtual int get_width( ) const { return 512; }
		virtual int get_height( ) const { return 512; }

		// Audio
		virtual int get_audio_streams( ) const { return 0; }

		// Fetch method
		virtual frame_type_ptr fetch( )
		{
			typedef il::image< unsigned char, il::r8g8b8 > r8g8b8_image_type;

			// Construct a frame and populate with basic information
			frame_type *frame = new frame_type( );
			int num, den;
			get_sar( num, den );
			frame->set_sar( num, den );
			frame->set_pts( get_position( ) * 1.0 / 25.0 );
			frame->set_duration( 1.0 / 25.0 );
			frame->set_position( get_position( ) );

			// Generate an image
			int width = get_width( );
			int height = get_height( );
			image_type_ptr image = image_type_ptr( new image_type( r8g8b8_image_type( width, height, 1 ) ) );
			memset( image->data( ), int( 255 * ( double( get_position( ) ) / double( get_frames( ) ) ) ), image->size( ) );
			frame->set_image( image );

			// Return a frame
			return frame_type_ptr( frame );
		}
};

class ML_PLUGIN_DECLSPEC template_store : public store_type
{
	public:
		template_store( ) 
			: store_type( )
		{ }

		virtual ~template_store( )
		{ }

		virtual bool push( frame_type_ptr frame )
		{
			image_type_ptr img = frame->get_image( );
			if ( img != 0 )
			{
				img = il::convert( img, L"r8g8b8" );
				int w = img->width( );
				int h = img->height( );
				fprintf( stdout, "P6\n%d %d\n255\n", w, h );
				fwrite( img->data( ), w * h * 3, 1, stdout );
				fflush( stdout );
			}
			return img != 0;
		}

		virtual frame_type_ptr flush( )
		{ return frame_type_ptr( ); }

		virtual void complete( )
		{ }
};

class ML_PLUGIN_DECLSPEC template_filter : public filter_type
{
	public:
		template_filter( )
			: filter_type( )
			, prop_u_( pcos::key::from_string( "u" ) )
			, prop_v_( pcos::key::from_string( "v" ) )
		{
			properties( ).append( prop_u_ = 128 );
			properties( ).append( prop_v_ = 128 );
		}

		virtual const opl::wstring get_uri( ) const { return L"template:"; }

		inline void fill( image_type_ptr img, size_t plane, unsigned char val )
		{
			unsigned char *ptr = img->data( plane );
			int width = img->width( plane );
			int height = img->height( plane );
			int diff = img->pitch( plane );
			if ( ptr )
			{
				while( height -- )
				{
					memset( ptr, val, width );
					ptr += diff;
				}
			}
		}

		virtual frame_type_ptr fetch( )
		{
			acquire_values( );

			frame_type_ptr result;
			input_type_ptr input = fetch_slot( );

			if ( input )
			{
				input->seek( get_position( ) );
				result = input->fetch( );
				if ( result && result->get_image( ) )
				{
					image_type_ptr img = result->get_image( );
					if ( img )
					{
						img = il::convert( img, L"yuv420p" );
						fill( img, 1, prop_u_.value< int >( ) );
						fill( img, 2, prop_v_.value< int >( ) );
					}
					result->set_image( img );
				}
			}
			
			return result;
		}

	private:
		pcos::property prop_u_;
		pcos::property prop_v_;
};

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC template_plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input(  const opl::wstring & )
	{
		return input_type_ptr( new template_input( ) );
	}

	virtual store_type_ptr store( const opl::wstring &, const frame_type_ptr & )
	{
		return store_type_ptr( new template_store( ) );
	}

	virtual filter_type_ptr filter( const opl::wstring & )
	{
		return filter_type_ptr( new template_filter( ) );
	}
};

} } }

//
// Library initialisation mechanism
//

namespace
{
	void reflib( int init )
	{
		static long refs = 0;

		assert( refs >= 0 && L"template_plugin::refinit: refs is negative." );
		
		if( init > 0 && ++refs )
		{
			// Initialise
		}
		else if( init < 0 && --refs == 0 )
		{
			// Uninitialise
		}
	}
	
	boost::recursive_mutex mutex;
}

//
// Access methods for openpluginlib
//

extern "C"
{
	ML_PLUGIN_DECLSPEC bool openplugin_init( void )
	{
		boost::recursive_mutex::scoped_lock lock( mutex );
		reflib( 1 );
		return true;
	}

	ML_PLUGIN_DECLSPEC bool openplugin_uninit( void )
	{
		boost::recursive_mutex::scoped_lock lock( mutex );
		reflib( -1 );
		return true;
	}
	
	ML_PLUGIN_DECLSPEC bool openplugin_create_plugin( const char*, opl::openplugin** plug )
	{
		*plug = new plugin::template_plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( opl::openplugin* plug )
	{ 
		delete static_cast< plugin::template_plugin * >( plug ); 
	}
}
