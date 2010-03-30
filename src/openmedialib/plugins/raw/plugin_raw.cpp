// raw - A raw plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>

extern "C" {
#include <libavformat/avio.h>
}

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace pcos = olib::openpluginlib::pcos;

namespace olib { namespace openmedialib { namespace ml { namespace raw {

int bytes_per_image( const il::image_type_ptr &image )
{
	int result = 0;
	if ( image )
	{
		for ( int p = 0; p < image->plane_count( ); p ++ )
			result += image->linesize( p ) * image->height( p );
	}
	return result;
}

class ML_PLUGIN_DECLSPEC input_raw : public input_type
{
	public:
		// Constructor and destructor
		input_raw( const pl::wstring &spec ) 
			: input_type( ) 
			, spec_( spec )
			, prop_pf_( pl::pcos::key::from_string( "pf" ) )
			, prop_width_( pl::pcos::key::from_string( "width" ) )
			, prop_height_( pl::pcos::key::from_string( "height" ) )
			, prop_fps_num_( pl::pcos::key::from_string( "fps_num" ) )
			, prop_fps_den_( pl::pcos::key::from_string( "fps_den" ) )
			, prop_sar_num_( pl::pcos::key::from_string( "sar_num" ) )
			, prop_sar_den_( pl::pcos::key::from_string( "sar_den" ) )
			, context_( 0 )
			, size_( 0 )
			, bytes_( 0 )
			, frames_( 0 )
			, expected_( 0 )
		{
			properties( ).append( prop_pf_ = pl::wstring( L"yuv422" ) );
			properties( ).append( prop_width_ = 1920 );
			properties( ).append( prop_height_ = 1080 );
			properties( ).append( prop_fps_num_ = 25 );
			properties( ).append( prop_fps_den_ = 1 );
			properties( ).append( prop_sar_num_ = 1 );
			properties( ).append( prop_sar_den_ = 1 );
		}

		virtual ~input_raw( ) 
		{ 
			if ( context_ )
				url_close( context_ );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return true; }

		// Basic information
		virtual const pl::wstring get_uri( ) const { return spec_; }
		virtual const pl::wstring get_mime_type( ) const { return L""; }

		// Audio/Visual
		virtual int get_frames( ) const { return frames_; }
		virtual bool is_seekable( ) const { return true; }

		// Visual
		virtual int get_video_streams( ) const { return 1; }

		// Audio
		virtual int get_audio_streams( ) const { return 0; }

	protected:

		bool initialize( )
		{
			int error = url_open( &context_, pl::to_string( spec_ ).c_str( ), URL_RDONLY );
			if ( error == 0 )
			{
				il::image_type_ptr image = il::allocate( prop_pf_.value< pl::wstring >( ), prop_width_.value< int >( ), prop_height_.value< int >( ) );
				size_ = url_filesize( context_ );
				bytes_ = bytes_per_image( image );

				if ( bytes_ != 0 && size_ != 0 && size_ % bytes_ == 0 )
					frames_ = size_ / bytes_;
				else
					error = 1;
			}
			return error == 0;
		}

		// Fetch method
		void do_fetch( frame_type_ptr &result )
		{
			// Construct a frame and populate with basic information
			frame_type *frame = new frame_type( );
			result = frame_type_ptr( frame );
			frame->set_fps( prop_fps_num_.value< int >( ), prop_fps_den_.value< int >( ) );
			frame->set_sar( prop_sar_num_.value< int >( ), prop_sar_den_.value< int >( ) );
			frame->set_pts( double( get_position( ) ) * prop_fps_den_.value< int >( ) / prop_fps_num_.value< int >( ) );
			frame->set_duration( double( prop_fps_den_.value< int >( ) ) / prop_fps_num_.value< int >( ) );
			frame->set_position( get_position( ) );

			// Seek to requested position when required
			if ( get_position( ) != expected_ )
				url_seek( context_, boost::int64_t( get_position( ) ) * bytes_, SEEK_SET );
			expected_ = get_position( ) + 1;

			// Generate an image
			int width = prop_width_.value< int >( );
			int height = prop_height_.value< int >( );
			il::image_type_ptr image = il::allocate( prop_pf_.value< pl::wstring >( ), width, height );
			if ( image )
			{
				for ( int p = 0; p < image->plane_count( ); p ++ )
				{
					boost::uint8_t *dst = image->data( p );
					int pitch = image->pitch( p );
					int width = image->linesize( p );
					int height = image->height( p );
					while( height -- )
					{
						url_read_complete( context_, dst, width );
						dst += pitch;
					}
				}
			}
			frame->set_image( image );
		}

	private:

		pl::wstring spec_;
		pl::pcos::property prop_pf_;
		pl::pcos::property prop_width_;
		pl::pcos::property prop_height_;
		pl::pcos::property prop_fps_num_;
		pl::pcos::property prop_fps_den_;
		pl::pcos::property prop_sar_num_;
		pl::pcos::property prop_sar_den_;
		URLContext *context_;
		boost::int64_t size_;
		int bytes_;
		int frames_;
		int expected_;
};

class ML_PLUGIN_DECLSPEC store_raw : public store_type
{
	public:
		store_raw( const pl::wstring spec, const frame_type_ptr &frame ) 
			: store_type( )
			, spec_( spec )
			, context_( 0 )
			, valid_( false )
		{
			if ( frame->has_image( ) )
			{
				valid_ = true;
				il::image_type_ptr image = frame->get_image( );
				pf_ = image->pf( );
				width_ = image->width( );
				height_ = image->height( );
				frame->get_sar( sar_num_, sar_den_ );
				frame->get_fps( fps_num_, fps_den_ );
				bytes_ = bytes_per_image( image );
			}
		}

		virtual ~store_raw( )
		{
			if ( context_ )
				url_fclose( context_ );
		}

		bool init( )
		{
			int error = 0;
			if ( valid_ )
			{
				error = url_fopen( &context_, pl::to_string( spec_ ).c_str( ), URL_WRONLY );
				if ( error == 0 )
				{
					url_setbufsize( context_, bytes_ * 2 );

					std::ostringstream str;
					str << pl::to_string( spec_ )
						<< " pf=" << pl::to_string( pf_ )
						<< " width=" << width_ << " height=" << height_ 
						<< " fps_num=" << fps_num_ << " fps_den=" << fps_den_
						<< " sar_num=" << sar_num_ << " sar_den=" << sar_den_
						<< std::endl;
					std::string string = str.str( );

					ByteIOContext *aml = 0;
					if( url_fopen( &aml, pl::to_string( spec_ + L".aml" ).c_str( ), URL_WRONLY ) == 0 )
					{
						url_setbufsize( aml, string.size( ) );
						put_buffer( aml, reinterpret_cast< const unsigned char * >( string.c_str( ) ), string.size( ) );
						put_flush_packet( aml );
						url_fclose( aml );
					}
				}
			}
			return valid_ && error == 0;
		}

		bool push( frame_type_ptr frame )
		{
			il::image_type_ptr image = frame->get_image( );
			if ( image != 0 )
			{
				for ( int p = 0; p < image->plane_count( ); p ++ )
				{
					const boost::uint8_t *dst = image->data( p );
					int pitch = image->pitch( p );
					int width = image->linesize( p );
					int height = image->height( p );
					while( height -- )
					{
						put_buffer( context_, dst, width );
						dst += pitch;
					}
				}
				put_flush_packet( context_ );
			}
			return image != 0;
		}

		virtual frame_type_ptr flush( )
		{ return frame_type_ptr( ); }

		virtual void complete( )
		{ }

	private:
		pl::wstring spec_;
		ByteIOContext *context_;
		pl::wstring pf_;
		int valid_;
		int width_;
		int height_;
		int fps_num_;
		int fps_den_;
		int sar_num_;
		int sar_den_;
		int bytes_;
};

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input( const pl::wstring &spec )
	{
		return input_type_ptr( new input_raw( spec ) );
	}

	virtual store_type_ptr store( const pl::wstring &spec, const frame_type_ptr &frame )
	{
		return store_type_ptr( new store_raw( spec, frame ) );
	}

	virtual filter_type_ptr filter( const pl::wstring & )
	{
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
		ml::create_filter( L"resampler" );
		*plug = new ml::raw::plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< ml::raw::plugin * >( plug ); 
	}
}
