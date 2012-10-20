// wav - A wav plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>

extern "C" {
#include <libavformat/url.h>
}

#include "store_wav.hpp"

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace pcos = olib::openpluginlib::pcos;

static pcos::key key_source_timecode_( pcos::key::from_string( "source_timecode" ) );

namespace olib { namespace openmedialib { namespace ml { namespace wav {

class ML_PLUGIN_DECLSPEC input_wav : public input_type
{
	public:
		// Constructor and destructor
		input_wav( const std::wstring &spec ) 
			: input_type( ) 
			, spec_( spec )
			, prop_fps_num_( pl::pcos::key::from_string( "fps_num" ) )
			, prop_fps_den_( pl::pcos::key::from_string( "fps_den" ) )
			, prop_file_size_( pl::pcos::key::from_string( "file_size" ) )
			, context_( 0 )
			, channels_( 0 )
			, frequency_( 0 )
			, store_bytes_( 0 )
			, bits_( 0 )
			, bytes_( 0 )
			, offset_( 0 )
			, start_tc_valid_( false )
			, start_tc_in_samples_( 0 )
			, size_( 0 )
			, frames_( 0 )
			, expected_( 0 )
		{
			properties( ).append( prop_fps_num_ = 25 );
			properties( ).append( prop_fps_den_ = 1 );
			properties( ).append( prop_file_size_ = boost::int64_t( 0 ) );
		}

		virtual ~input_wav( ) 
		{ 
			if ( context_ )
				ffurl_close( context_ );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// Basic information
		virtual const std::wstring get_uri( ) const { return spec_; }
		virtual const std::wstring get_mime_type( ) const { return L""; }

		// Audio/Visual
		virtual int get_frames( ) const { return frames_; }
		virtual bool is_seekable( ) const { return true; }

	protected:

		bool initialize( )
		{
			std::wstring resource = spec_;

			if ( resource.find( L"wav:" ) == 0 )
				resource = resource.substr( 4 );

			int error = ffurl_open( &context_, olib::opencorelib::str_util::to_string( resource ).c_str( ), AVIO_FLAG_READ, 0, 0 );
			bool found_fmt = false;

			if ( error == 0 )
			{
				std::vector< boost::uint8_t > buffer( 65536 );

				bool error = ffurl_read_complete( context_, &buffer[ 0 ], 8 ) != 8;

				bool rf64_mode = false;
				if( memcmp( &buffer[ 0 ], "RIFF", 4 ) == 0 )
					rf64_mode = false;
				else if( memcmp( &buffer[ 0 ], "RF64", 4 ) == 0 )
					rf64_mode = true;
				else
					error = true;

				error = !error && ( ffurl_read_complete( context_, &buffer[ 0 ], 4 ) != 4 || memcmp( &buffer[ 0 ], "WAVE", 4 ) );
				offset_ += 12;

				while( !error )
				{
					error = ffurl_read_complete( context_, &buffer[ 0 ], 8 ) != 8;
					offset_ += 8;

					if ( error )
					{
						break;
					}

					//Save the chunk ID
					std::vector< boost::uint8_t > chunk_id(4);
					memcpy( &chunk_id[ 0 ], &buffer[ 0 ], 4 );
					
					//The next 4 bytes after the chunk ID is the size of the chunk
					boost::uint32_t n = get_ule32( buffer, 4 );
					if ( n > boost::uint32_t( buffer.size( ) ) || ffurl_read_complete( context_, &buffer[ 0 ], n ) != (int)n ) break;
					offset_ += n;

					if ( memcmp( &chunk_id[ 0 ], "ds64", 4 ) == 0 )
					{
						//DataSize64 chunk, gives the size of the coming "data" chunk
						bytes_ = get_ule64( buffer, 8 );
					}
					else if ( memcmp( &chunk_id[ 0 ], "bext", 4 ) == 0 )
					{
						//BWAV extension chunk

						//Read the start timecode of the audio
						start_tc_in_samples_ = get_ule64( buffer, 338 );
						start_tc_valid_ = true;
					}
					else if ( memcmp( &chunk_id[ 0 ], "fmt ", 4 ) == 0 )
					{
						boost::uint16_t fmt = get_ule16( buffer, 0 );
						if ( fmt == 0xfffe && n == 40 )
						{
							//WAVE_FORMAT_EXTENSIBLE
							fmt = get_ule16( buffer, 24 );
						}
						if ( fmt != 1 ) break;
						channels_ = get_le16( buffer, 2 );
						frequency_ = get_le32( buffer, 4 );
						store_bytes_ = get_le16( buffer, 12 );
						bits_ = get_le16( buffer, 14 );
						found_fmt = true;
					}
					else if ( memcmp( &chunk_id[ 0 ], "data", 4) == 0 ) 
					{
						//If we have an RF64 WAV, we should already have set the number of bytes from the ds64 chunk
						if ( !rf64_mode )
						{
							bytes_ = get_ule32( buffer, 4 );
						}

						break;
					}
					else
					{
						//Unknown chunk, ignore
					}
				}
			}

			if ( error == 0 && found_fmt )
			{
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

			//Set the source timecode on the frame, if specified by the BWAV extension chunk
			//Note that this value is specified in samples, since the framerate of a WAV is
			//its frequency.
			if( start_tc_valid_ )
			{
				pcos::property source_tc_prop( key_source_timecode_ );
				ARENFORCE( frequency_ > 0 );
				ARENFORCE( prop_fps_den_.value< int >( ) > 0 );
				int start_tc_in_frames = 
					static_cast<int>( start_tc_in_samples_ / frequency_ * prop_fps_num_.value< int >( ) / prop_fps_den_.value< int >( ) );

				frame->properties( ).append( source_tc_prop = (int)( start_tc_in_frames + get_position( ) ) );
			}

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
				bytes_ = ffurl_size( context_ ) - offset_;
			else
				bytes_ = std::min< boost::int64_t >( bytes_, ffurl_size( context_ ) - offset_ );
			frames_ = int( 0.5f + float( bytes_ * prop_fps_num_.value< int >( ) ) / ( bits_ / 8 * channels_ * frequency_ * prop_fps_den_.value< int >( ) ) );
			prop_file_size_ = boost::int64_t( ffurl_size( context_ ) );
		}

		void seek_to_position( int position )
		{
			boost::int64_t offset = offset_ + ml::audio::samples_to_frame( position, frequency_, prop_fps_num_.value< int >( ), prop_fps_den_.value< int >( ) ) * channels_ * bits_ / 8;
			ffurl_seek( context_, offset, SEEK_SET );
		}

		ml::audio_type_ptr create( int position )
		{
			ml::audio_type_ptr result;
			int samples = ml::audio::samples_for_frame( position, frequency_, prop_fps_num_.value< int >( ), prop_fps_den_.value< int >( ) );

			std::vector< boost::uint8_t > buffer( samples * bits_ / 8 * channels_ );
			int nread = ffurl_read_complete( context_, &buffer[ 0 ], buffer.size( ) );

			switch( bits_ )
			{
				case 8:
					result = ml::audio::allocate( ml::audio::FORMAT_PCM8, frequency_, channels_, samples, false );
					memcpy( result->pointer( ), &buffer[ 0 ], result->size( ) );
					break;

				case 16:
					result = ml::audio::allocate( ml::audio::FORMAT_PCM16, frequency_, channels_, samples, false );
					memcpy( result->pointer( ), &buffer[ 0 ], result->size( ) );
					break;

				case 24:
					result = ml::audio::allocate( ml::audio::FORMAT_PCM24, frequency_, channels_, samples, false );
					pack_24( result, buffer, samples * channels_ );
					break;

				case 32:
					result = ml::audio::allocate( ml::audio::FORMAT_PCM32, frequency_, channels_, samples, false );
					memcpy( result->pointer( ), &buffer[ 0 ], result->size( ) );
					break;

				default:
					break;
			}

			if (result) {
				boost::int64_t samples = ((boost::int64_t)result->samples() * (boost::int64_t)nread) / (boost::int64_t)result->size();
				result->original_samples( (int)samples );
			}

			return result;
		}

		inline boost::uint64_t get_ule64( const std::vector< boost::uint8_t > &buffer, int o )
		{
			boost::uint64_t result = 0;
			for( int i = 0; i < 8; ++i )
			{
				result |= ( static_cast<boost::uint64_t>( buffer[ o + i ] ) << (8*i) );
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

		inline boost::uint16_t get_ule16( const std::vector< boost::uint8_t > &buffer, int o )
		{
			return boost::uint16_t( buffer[ o ] | buffer[ o + 1 ] << 8 );
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

		std::wstring spec_;
		pl::pcos::property prop_fps_num_;
		pl::pcos::property prop_fps_den_;
        pl::pcos::property prop_file_size_;
		URLContext *context_;

		int channels_;
		int frequency_;
		int store_bytes_;
		int bits_;
		boost::int64_t bytes_;
		boost::int64_t offset_;
		bool start_tc_valid_;
		boost::uint64_t start_tc_in_samples_;

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
	virtual input_type_ptr input( const std::wstring &spec )
	{
		return input_type_ptr( new input_wav( spec ) );
	}

	virtual store_type_ptr store( const std::wstring &spec, const frame_type_ptr &frame )
	{
		store_wav* s = new store_wav( spec );
		s->initializeFirstFrame(frame);
		return store_type_ptr( s );
	}

	virtual filter_type_ptr filter( const std::wstring & )
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
