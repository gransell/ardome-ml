// wav - A wav plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/io.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>

#include "store_wav.hpp"

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace io = ml::io;

namespace pcos = olib::openpluginlib::pcos;

// If this function would appear in a later version of avformat,
// then please put it inside preprocessor block to make it conditional on avformat version
int avio_read_complete( AVIOContext *s, unsigned char *buf, int size )
{
	int remaining = size;
	while ( remaining > 0 )
	{
		int result = avio_read( s, buf, remaining );
		if( result < 0 )
			return result;

		remaining -= result;
	}
	return size - remaining; // correct in case of overshoot
}


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
			, fmt_ ( 0 )
		{
			properties( ).append( prop_fps_num_ = 25 );
			properties( ).append( prop_fps_den_ = 1 );
			properties( ).append( prop_file_size_ = boost::int64_t( 0 ) );
		}

		virtual ~input_wav( ) 
		{ 
			if ( context_ )
				io::close_file( context_ );
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

			int error = io::open_file( &context_, olib::opencorelib::str_util::to_string( resource ).c_str( ), AVIO_FLAG_READ );
			bool found_fmt = false;

			if ( error == 0 )
			{
				std::vector< boost::uint8_t > buffer( 65536 );

				bool error = avio_read_complete( context_, &buffer[ 0 ], 8 ) != 8;

				bool rf64_mode = false;
				if( memcmp( &buffer[ 0 ], "RIFF", 4 ) == 0 )
					rf64_mode = false;
				else if( memcmp( &buffer[ 0 ], "RF64", 4 ) == 0 )
					rf64_mode = true;
				else
					error = true;

				error = !error && ( avio_read_complete( context_, &buffer[ 0 ], 4 ) != 4 || memcmp( &buffer[ 0 ], "WAVE", 4 ) );
				offset_ += 12;

				while( !error )
				{
					// Read Chunk ID and Chunk Data Size
					error = avio_read_complete( context_, &buffer[ 0 ], 8 ) != 8;
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

					if ( memcmp( &chunk_id[ 0 ], "data", 4) == 0 ) 
					{
						//If we have an RF64 WAV, we should already have set the number of bytes from the ds64 chunk
						if ( !rf64_mode )
						{
							bytes_ = get_ule32( buffer, 4 );
						}

						break;
					}

					// Read the rest of the chunk if buffer holds sufficient space
					if ( n > boost::uint32_t( buffer.size( ) ) || avio_read_complete( context_, &buffer[ 0 ], n ) != (int)n ) break;
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
						fmt_ = get_ule16( buffer, 0 );
						if ( fmt_ == WAVE_FORMAT_EXTENSIBLE && n == 40 )
						{
							//WAVE_FORMAT_EXTENSIBLE
							fmt_ = get_ule16( buffer, 24 );
						}

						switch ( fmt_ )
						{
							case WAVE_FORMAT_PCM:
							case WAVE_FORMAT_IEEE_FLOAT:
							{
								found_fmt = true;
							}
							break;
							default:
								found_fmt = false;
						}

						if ( found_fmt != true ) break;

						channels_ = get_le16( buffer, 2 );
						frequency_ = get_le32( buffer, 4 );
						store_bytes_ = get_le16( buffer, 12 );
						bits_ = get_le16( buffer, 14 );
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
			boost::int64_t file_size = avio_size( context_ ); // could involve seek, so don't do too often
			ARENFORCE_MSG( file_size > 0, "Could not size the file. Non-seekable I/O source?" )( file_size )( spec_ );
			if ( bytes_ == 0 )
				bytes_ = file_size - offset_;
			else
				bytes_ = std::min< boost::int64_t >( bytes_, file_size - offset_ );
			frames_ = int( ceil( double( bytes_ * prop_fps_num_.value< int >( ) ) / ( bits_ / 8 * channels_ * frequency_ * prop_fps_den_.value< int >( ) ) ) );
			prop_file_size_ = file_size;
		}

		void seek_to_position( int position )
		{
			boost::int64_t offset = offset_ + ml::audio::samples_to_frame( position, frequency_, prop_fps_num_.value< int >( ), prop_fps_den_.value< int >( ) ) * channels_ * bits_ / 8;
			avio_seek( context_, offset, SEEK_SET );
		}

		ml::audio_type_ptr create( int position )
		{
			ml::audio_type_ptr result;
			int samples = ml::audio::samples_for_frame( position, frequency_, prop_fps_num_.value< int >( ), prop_fps_den_.value< int >( ) );

			std::vector< boost::uint8_t > buffer( samples * bits_ / 8 * channels_ );
			int nread = avio_read_complete( context_, &buffer[ 0 ], buffer.size( ) );

			switch( bits_ )
			{
				case 16:
					result = ml::audio::allocate( ml::audio::pcm16_id, frequency_, channels_, samples, false );
					memcpy( result->pointer( ), &buffer[ 0 ], result->size( ) );
					break;

				case 24:
					result = ml::audio::allocate( ml::audio::pcm24_id, frequency_, channels_, samples, false );
					pack_24( result, buffer, samples * channels_ );
					break;

				case 32:
					result = ml::audio::allocate
					(
						fmt_ == WAVE_FORMAT_IEEE_FLOAT ? ml::audio::float_id : ml::audio::pcm32_id,
						frequency_, channels_, samples, false
					);
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
		AVIOContext *context_;

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
		boost::uint16_t fmt_;
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
