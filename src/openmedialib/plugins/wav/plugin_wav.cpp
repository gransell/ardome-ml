// wav - A wav plugin to ml.
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

namespace olib { namespace openmedialib { namespace ml { namespace wav {

class ML_PLUGIN_DECLSPEC input_wav : public input_type
{
	public:
		// Constructor and destructor
		input_wav( const pl::wstring &spec ) 
			: input_type( ) 
			, spec_( spec )
			, prop_fps_num_( pl::pcos::key::from_string( "fps_num" ) )
			, prop_fps_den_( pl::pcos::key::from_string( "fps_den" ) )
			, context_( 0 )
			, channels_( 0 )
			, frequency_( 0 )
			, store_bits_( 0 )
			, bits_( 0 )
			, bytes_( 0 )
			, offset_( 0 )
			, size_( 0 )
			, frames_( 0 )
			, expected_( 0 )
		{
			properties( ).append( prop_fps_num_ = 25 );
			properties( ).append( prop_fps_den_ = 1 );
		}

		virtual ~input_wav( ) 
		{ 
			if ( context_ )
				url_close( context_ );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// Basic information
		virtual const pl::wstring get_uri( ) const { return spec_; }
		virtual const pl::wstring get_mime_type( ) const { return L""; }

		// Audio/Visual
		virtual int get_frames( ) const { return frames_; }
		virtual bool is_seekable( ) const { return true; }

		// Visual
		virtual int get_video_streams( ) const { return 0; }

		// Audio
		virtual int get_audio_streams( ) const { return 1; }

	protected:

		bool initialize( )
		{
			int error = url_open( &context_, pl::to_string( spec_ ).c_str( ), URL_RDONLY );
			bool found_fmt = false;

			if ( error == 0 )
			{
				std::vector< boost::uint8_t > buffer( 65536 );

				bool error = url_read_complete( context_, &buffer[ 0 ], 8 ) != 8 || memcmp( &buffer[ 0 ], "RIFF", 4 );
				error = !error && ( url_read_complete( context_, &buffer[ 0 ], 4 ) != 4 || memcmp( &buffer[ 0 ], "WAVE", 4 ) );

				while( !error )
				{
					error = url_read_complete( context_, &buffer[ 0 ], 8 ) != 8;
					if ( error )
					{
						break;
					}
					if ( memcmp( &buffer[ 0 ], "fmt ", 4 ) == 0 )
					{
						int n = get_le32( buffer, 4 );
						if ( n < 0 || n > int( buffer.size( ) ) || url_read_complete( context_, &buffer[ 0 ], n ) != n ) break;
						int fmt = get_le16( buffer, 0 );
						if ( fmt == 0xfffe && n == 40 )
							fmt = get_le16( buffer, 24 );
						if ( fmt != 1 ) break;
						channels_ = get_le16( buffer, 2 );
						frequency_ = get_le32( buffer, 4 );
						store_bits_ = get_le16( buffer, 12 );
						bits_ = get_le16( buffer, 14 );
						found_fmt = true;
					}
					else if ( memcmp( &buffer[ 0 ], "data", 4) == 0 ) 
					{
						bytes_ = get_ule32( buffer, 4 );
						break;
					}
					else
					{
						int n = get_le32( buffer, 4 );
						if ( n < 0 || n > int( buffer.size( ) ) || url_read_complete( context_, &buffer[ 0 ], n ) != n ) break;
					}
				}
			}

			if ( error == 0 && found_fmt )
			{
				offset_ = url_seek( context_, 0, SEEK_CUR );
				size_media( );
			}

			return error == 0 && found_fmt;
		}

		// Fetch method
		void do_fetch( frame_type_ptr &result )
		{
			// Construct a frame and populate with basic information
			frame_type *frame = new frame_type( );
			result = frame_type_ptr( frame );
			frame->set_fps( prop_fps_num_.value< int >( ), prop_fps_den_.value< int >( ) );
			frame->set_pts( double( get_position( ) ) * prop_fps_den_.value< int >( ) / prop_fps_num_.value< int >( ) );
			frame->set_duration( double( prop_fps_den_.value< int >( ) ) / prop_fps_num_.value< int >( ) );
			frame->set_position( get_position( ) );

			// Seek to requested position when required
			if ( get_position( ) != expected_ )
				seek_to_position( get_position( ) );

			frame->set_audio( create( get_position( ) ) );

			expected_ = get_position( ) + 1;
		}

	private:

		void size_media( )
		{
			if ( bytes_ == 0 )
				bytes_ = url_filesize( context_ ) - offset_;
			else
				bytes_ = std::min< boost::int64_t >( bytes_, url_filesize( context_ ) - offset_ );
			frames_ = int( 0.5f + float( bytes_ * prop_fps_num_.value< int >( ) ) / ( bits_ / 8 * channels_ * frequency_ * prop_fps_den_.value< int >( ) ) );
		}

		void seek_to_position( int position )
		{
			boost::int64_t offset = offset_ + ml::audio::samples_to_frame( position, frequency_, prop_fps_num_.value< int >( ), prop_fps_den_.value< int >( ) ) * channels_ * bits_ / 8;
			url_seek( context_, offset, SEEK_SET );
		}

		ml::audio_type_ptr create( int position )
		{
			ml::audio_type_ptr result;
			int samples = ml::audio::samples_for_frame( position, frequency_, prop_fps_num_.value< int >( ), prop_fps_den_.value< int >( ) );

			std::vector< boost::uint8_t > buffer( samples * bits_ / 8 * channels_ );
			url_read_complete( context_, &buffer[ 0 ], buffer.size( ) );

			switch( bits_ )
			{
				case 8:
					result = ml::audio::allocate( ml::audio::FORMAT_PCM8, frequency_, channels_, samples );
					memcpy( result->pointer( ), &buffer[ 0 ], result->size( ) );
					break;

				case 16:
					result = ml::audio::allocate( ml::audio::FORMAT_PCM16, frequency_, channels_, samples );
					memcpy( result->pointer( ), &buffer[ 0 ], result->size( ) );
					break;

				case 24:
					result = ml::audio::allocate( ml::audio::FORMAT_PCM24, frequency_, channels_, samples );
					pack_24( result, buffer, samples * channels_ );
					break;

				case 32:
					result = ml::audio::allocate( ml::audio::FORMAT_PCM32, frequency_, channels_, samples );
					memcpy( result->pointer( ), &buffer[ 0 ], result->size( ) );
					break;

				default:
					break;
			}

			return result;
		}

		inline boost::uint32_t get_ule32( const std::vector< boost::uint8_t > &buffer, int o )
		{
			return boost::uint32_t( buffer[ o ] | buffer[ o + 1 ] << 8 | buffer[ o + 2 ] << 16 | buffer[ o + 3 ] << 24 );
		}

		inline int get_le32( const std::vector< boost::uint8_t > &buffer, int o )
		{
			return int( buffer[ o ] | buffer[ o + 1 ] << 8 | buffer[ o + 2 ] << 16 | buffer[ o + 3 ] << 24 );
		}

		inline int get_le16( const std::vector< boost::uint8_t > &buffer, int o )
		{
			return boost::int16_t( buffer[ o ] | buffer[ o + 1 ] << 8 );
		}

		inline void pack_24( ml::audio_type_ptr &result, const std::vector< boost::uint8_t > &buffer, int samples )
		{
			const boost::uint8_t *src = &buffer[ 0 ];
			int *dst = static_cast< int * >( result->pointer( ) );
			while( samples -- )
			{
				*dst ++ = int( src[ 0 ] << 8 | src[ 1 ] << 16 | src[ 2 ] << 24 ) >> 8;
				src += 3;
			}
		}

		pl::wstring spec_;
		pl::pcos::property prop_fps_num_;
		pl::pcos::property prop_fps_den_;
		URLContext *context_;

		int channels_;
		int frequency_;
		int store_bits_;
		int bits_;
		boost::int64_t bytes_;
		boost::int64_t offset_;

		boost::int64_t size_;
		int frames_;
		int expected_;
};

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input( const pl::wstring &spec )
	{
		return input_type_ptr( new input_wav( spec ) );
	}

	virtual store_type_ptr store( const pl::wstring &spec, const frame_type_ptr &frame )
	{
		return store_type_ptr( );
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
		*plug = new ml::wav::plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< ml::wav::plugin * >( plug ); 
	}
}
