// template - A template plugin to ml.
//
// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.
//
// #input:test:
//
// Provides a basic input which is useful for testing.
//
// #store:packets:
//
// Provides a basic packet writing store for the generation of elementary
// video and audio files.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/stream.hpp>
#include <openmedialib/ml/awi.hpp>

#ifdef WIN32
#include <windows.h>
#define ftello _ftelli64
#endif // WIN32

#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>

#include <boost/thread/recursive_mutex.hpp>

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;

namespace pcos = olib::openpluginlib::pcos;

namespace olib { namespace openmedialib { namespace ml { 

class ML_PLUGIN_DECLSPEC template_input : public input_type
{
	public:
		// Constructor and destructor
		template_input( ) : input_type( ) { }
		virtual ~template_input( ) { }

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return true; }

		// Basic information
		virtual const std::wstring get_uri( ) const { return L"test:"; }
		virtual const std::wstring get_mime_type( ) const { return L""; }

		// Audio/Visual
		virtual int get_frames( ) const { return 250; }
		virtual bool is_seekable( ) const { return true; }

		// Visual
		virtual void get_fps( int &num, int &den ) const { num = 25; den = 1; }
		virtual void get_sar( int &num, int &den ) const { num = 1; den = 1; }
		virtual int get_width( ) const { return 512; }
		virtual int get_height( ) const { return 512; }

	protected:
		// Fetch method
		void do_fetch( frame_type_ptr &result )
		{
			// Construct a frame and populate with basic information
			frame_type *frame = new frame_type( );
			result = frame_type_ptr( frame );
			int num, den;
			get_sar( num, den );
			frame->set_sar( num, den );
			frame->set_pts( get_position( ) * 1.0 / 25.0 );
			frame->set_duration( 1.0 / 25.0 );
			frame->set_position( get_position( ) );

			// Generate an image
			int width = get_width( );
			int height = get_height( );
			ml::image_type_ptr image = ml::image::allocate( "r8g8b8", width, height );
			memset( image->data( ), int( 255 * ( double( get_position( ) ) / double( get_frames( ) ) ) ), image->size( ) );
			frame->set_image( image );
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
			ml::image_type_ptr img = frame->get_image( );
			if ( img != 0 )
			{
				img = ml::image::convert( img, "r8g8b8" );
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

class ML_PLUGIN_DECLSPEC packets_store : public store_type
{
	public:
		packets_store( const std::wstring &name, const frame_type_ptr &frame ) 
			: store_type( )
			, name_( name )
			, output_( 0 )
			, index_( 0 )
			, generator_( 1 )
			, count_( 0 )
		{
			if ( frame && frame->get_stream( ) )
			{
				if ( name_.find( L"packets:" ) == 0 )
					name_ = name_.substr( 8 );
				output_ = fopen( olib::opencorelib::str_util::to_string( name_ ).c_str( ), "wb" );
				if ( frame->get_stream( ) )
					index_ = fopen( olib::opencorelib::str_util::to_string( name_ + L".awi" ).c_str( ), "wb" );
			}
		}

		virtual ~packets_store( )
		{
		}

		virtual bool push( frame_type_ptr frame )
		{
			bool result = false;
			if ( output_ && frame->get_stream( ) )
			{
				if ( index_ && frame->get_stream( )->key( ) == frame->get_stream( )->position( ) )
				{
					generator_.enroll( count_, ftello( output_ ) );
					std::vector< boost::uint8_t > buffer;
					generator_.flush( buffer );
					fwrite( &buffer[ 0 ], buffer.size( ), 1, index_ );
					fflush( output_ );
					fflush( index_ );
				}
				stream_type_ptr pkt = frame->get_stream( );
				result = fwrite( pkt->bytes( ), pkt->length( ), 1, output_ ) == 1;
				count_ ++;
			}
			return result;
		}

		virtual frame_type_ptr flush( )
		{ return frame_type_ptr( ); }

		virtual void complete( )
		{
			if ( index_ )
			{
				generator_.close( count_, ftell( output_ ) );
				std::vector< boost::uint8_t > buffer;
				generator_.flush( buffer );
				fwrite( &buffer[ 0 ], buffer.size( ), 1, index_ );
				fclose( index_ );
			}
			if ( output_ )
				fclose( output_ );
			index_ = 0;
			output_ = 0;
		}

	private:
		std::wstring name_;
		FILE *output_;
		FILE *index_;
		awi_generator_v4 generator_;
		int count_;
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

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return true; }

		virtual const std::wstring get_uri( ) const { return L"template:"; }

		inline void fill( ml::image_type_ptr img, size_t plane, unsigned char val )
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

	protected:
		void do_fetch( frame_type_ptr &result )
		{
			acquire_values( );

			input_type_ptr input = fetch_slot( );

			if ( input )
			{
				input->seek( get_position( ) );
				result = input->fetch( );
				if ( result && result->get_image( ) )
				{
					ml::image_type_ptr img = result->get_image( );
					if ( img )
					{
						img = ml::image::convert( img, "yuv420p" );
						fill( img, 1, prop_u_.value< int >( ) );
						fill( img, 2, prop_v_.value< int >( ) );
					}
					result->set_image( img );
				}
			}
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
	virtual input_type_ptr input(  const std::wstring & )
	{
		return input_type_ptr( new template_input( ) );
	}

	virtual store_type_ptr store( const std::wstring &name, const frame_type_ptr &frame )
	{
		if ( name.find( L"packets:" ) == 0 )
			return store_type_ptr( new packets_store( name, frame ) );
		return store_type_ptr( new template_store( ) );
	}

	virtual filter_type_ptr filter( const std::wstring & )
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
	
	ML_PLUGIN_DECLSPEC bool openplugin_create_plugin( const char*, pl::openplugin** plug )
	{
		*plug = new ml::template_plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< ml::template_plugin * >( plug ); 
	}
}
