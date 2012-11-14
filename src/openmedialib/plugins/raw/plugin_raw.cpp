// raw - A raw plugin to ml.
//
// Copyright (C) 2010 Vizrt
// Released under the LGPL.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <opencorelib/cl/str_util.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>
#include <limits>

extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavformat/url.h>
}

namespace pl = olib::openpluginlib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace pcos = olib::openpluginlib::pcos;
namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { namespace raw {

std::string avio_gets( AVIOContext *s )
{
	std::string result;
	int c;

	c = avio_r8( s );
	while( c != EOF && c != '\n' && c != '\r' ) 
	{
		result += c;
		c = avio_r8( s );
	}
	return result;
}

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
			, prop_field_order_( pl::pcos::key::from_string( "field_order" ) )
			, prop_header_( pl::pcos::key::from_string( "header" ) )
			, prop_pad_( pl::pcos::key::from_string( "pad" ) )
			, context_( 0 )
			, size_( 0 )
			, bytes_( 0 )
			, extra_( 0 )
			, frames_( ( std::numeric_limits< int >::max )( ) )
			, expected_( 0 )
			, offset_( 0 )
			, parsed_( false )
		{
			properties( ).append( prop_pf_ = pl::wstring( L"yuv422" ) );
			properties( ).append( prop_width_ = 1920 );
			properties( ).append( prop_height_ = 1080 );
			properties( ).append( prop_fps_num_ = 25 );
			properties( ).append( prop_fps_den_ = 1 );
			properties( ).append( prop_sar_num_ = 1 );
			properties( ).append( prop_sar_den_ = 1 );
			properties( ).append( prop_field_order_ = 0 );
			properties( ).append( prop_header_ = 1 );
			properties( ).append( prop_pad_ = 0 );
		}

		virtual ~input_raw( ) 
		{ 
			if ( context_ )
				avio_close( context_ );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return true; }

		// Basic information
		virtual const pl::wstring get_uri( ) const { return spec_; }
		virtual const pl::wstring get_mime_type( ) const { return L""; }

		// Audio/Visual
		virtual int get_frames( ) const { return frames_; }
		virtual bool is_seekable( ) const { return context_ ? context_->seekable : 0; }

	protected:

		bool initialize( )
		{
			pl::wstring spec = spec_;
			if ( spec.find( L"raw:" ) == 0 )
				spec = spec.substr( 4 );

			int error = avio_open( &context_, pl::to_string( spec ).c_str( ), AVIO_FLAG_READ );
			if ( error == 0 && is_seekable( ) )
				error = parse_header( );

			return error == 0;
		}

		int parse_header( )
		{
			int error = 0;

			if ( prop_header_.value< int >( ) )
			{
				std::string header = avio_gets( context_ );
				std::istringstream str( header );
				std::string token;
				while( !str.eof( ) )
				{
					str >> token;
					if ( token.find( "pf=" ) == 0 ) prop_pf_ = pl::to_wstring( token.substr( 3 ) );
					else if ( token.find( "width=" ) == 0 ) prop_width_ = boost::lexical_cast< int >( token.substr( 6 ) );
					else if ( token.find( "height=" ) == 0 ) prop_height_ = boost::lexical_cast< int >( token.substr( 7 ) );
					else if ( token.find( "sar_num=" ) == 0 ) prop_sar_num_ = boost::lexical_cast< int >( token.substr( 8 ) );
					else if ( token.find( "sar_den=" ) == 0 ) prop_sar_den_ = boost::lexical_cast< int >( token.substr( 8 ) );
					else if ( token.find( "fps_num=" ) == 0 ) prop_fps_num_ = boost::lexical_cast< int >( token.substr( 8 ) );
					else if ( token.find( "fps_den=" ) == 0 ) prop_fps_den_ = boost::lexical_cast< int >( token.substr( 8 ) );
					else if ( token.find( "field_order=" ) == 0 ) prop_field_order_ = boost::lexical_cast< int >( token.substr( 12 ) );
					else if ( token.find( "frames=" ) == 0 ) frames_ = boost::lexical_cast< int >( token.substr( 7 ) );
					else if ( token.find( "pad=" ) == 0 ) prop_pad_ = boost::lexical_cast< int >( token.substr( 4 ) );
					else std::cerr << "unrecognised header entry " << token;
				}
				offset_ = avio_tell( context_ );
			}

			ARENFORCE_MSG( !( prop_pad_.value< int >( ) && prop_header_.value< int >( ) ), "Can't specify both pad and header" );

			il::image_type_ptr image = il::allocate( prop_pf_.value< pl::wstring >( ), prop_width_.value< int >( ), prop_height_.value< int >( ) );
			ARENFORCE_MSG( image, "Failed to allocate image.")( prop_pf_.value< pl::wstring >( ) )( prop_width_.value< int >( ) )( prop_height_.value< int >( ) );
			size_ = avio_size( context_ );
			bytes_ = bytes_per_image( image );
			int pad = prop_pad_.value< int >( );
			extra_ = pad && bytes_ % pad ? pad - ( bytes_ % pad ) : 0;

			if ( is_seekable( ) )
			{
				if ( bytes_ != 0 && size_ != 0 && size_ % ( bytes_ + extra_ ) == offset_ )
					frames_ = size_ / ( bytes_ + extra_ );
				else
					error = 1;
			}

			parsed_ = true;

			return error;
		}

		// Fetch method
		void do_fetch( frame_type_ptr &result )
		{
			if ( !parsed_ )
				parse_header( );

			if ( last_frame_ && last_frame_->get_position( ) == get_position( ) )
			{
				result = last_frame_->shallow( );
				return;
			}

			// Seek to requested position when required
			if ( get_position( ) != expected_ && is_seekable( ) )
				avio_seek( context_, offset_ + boost::int64_t( get_position( ) ) * ( bytes_ + extra_ ), SEEK_SET );
			else
				seek( expected_ );
			expected_ = get_position( ) + 1;

			// Construct a frame and populate with basic information
			frame_type *frame = new frame_type( );
			result = frame_type_ptr( frame );
			frame->set_fps( prop_fps_num_.value< int >( ), prop_fps_den_.value< int >( ) );
			frame->set_sar( prop_sar_num_.value< int >( ), prop_sar_den_.value< int >( ) );
			frame->set_pts( double( get_position( ) ) * prop_fps_den_.value< int >( ) / prop_fps_num_.value< int >( ) );
			frame->set_duration( double( prop_fps_den_.value< int >( ) ) / prop_fps_num_.value< int >( ) );
			frame->set_position( get_position( ) );

			// Generate an image
			int width = prop_width_.value< int >( );
			int height = prop_height_.value< int >( );
			il::image_type_ptr image = il::allocate( prop_pf_.value< pl::wstring >( ), width, height );
			image->set_field_order( static_cast< il::field_order_flags >( prop_field_order_.value < int >( ) ) );
			bool error = false;

			if ( image )
			{
				int pad = prop_pad_.value< int >( );

				for ( int p = 0; p < image->plane_count( ); p ++ )
				{
					boost::uint8_t *dst = image->data( p );
					int pitch = image->pitch( p );
					int width = image->linesize( p );
					int height = image->height( p );
					while( height -- )
					{
						error |= avio_read( context_, dst, width ) != width;
						dst += pitch;
					}
				}

				if ( pad )
				{
					int needed = pad - ( bytes_ % pad );
					padding_.resize( pad );
					if ( needed != pad )
						error |= avio_read( context_, &padding_[ 0 ], needed ) != needed;
				}
			}

			if ( !error )
				frame->set_image( image );

			if ( !is_seekable( ) && url_feof( context_ ) )
				frames_ = expected_;

			last_frame_ = result->shallow( );
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
		pl::pcos::property prop_field_order_;
		pl::pcos::property prop_header_;
		pl::pcos::property prop_pad_;
		AVIOContext *context_;
		boost::int64_t size_;
		int bytes_;
		int extra_;
		int frames_;
		int expected_;
		boost::int64_t offset_;
		ml::frame_type_ptr last_frame_;
		bool parsed_;
		std::vector< boost::uint8_t > padding_;
};

class ML_PLUGIN_DECLSPEC input_aud : public input_type
{
	public:
		// Constructor and destructor
		input_aud( const pl::wstring &spec ) 
			: input_type( ) 
			, spec_( spec )
			, prop_af_( pl::pcos::key::from_string( "af" ) )
			, prop_frequency_( pl::pcos::key::from_string( "frequency" ) )
			, prop_channels_( pl::pcos::key::from_string( "channels" ) )
			, prop_fps_num_( pl::pcos::key::from_string( "fps_num" ) )
			, prop_fps_den_( pl::pcos::key::from_string( "fps_den" ) )
			, prop_profile_( pl::pcos::key::from_string( "profile" ) )
			, prop_header_( pl::pcos::key::from_string( "header" ) )
			, prop_stream_( pl::pcos::key::from_string( "stream" ) )
			, context_( 0 )
			, frames_( ( std::numeric_limits< int >::max )( ) )
			, expected_( 0 )
			, offset_( 0 )
			, parsed_( false )
		{
			properties( ).append( prop_af_ = pl::wstring( L"pcm16" ) );
			properties( ).append( prop_frequency_ = 48000 );
			properties( ).append( prop_channels_ = 2 );
			properties( ).append( prop_fps_num_ = 25 );
			properties( ).append( prop_fps_den_ = 1 );
			properties( ).append( prop_profile_ = pl::wstring( L"dv" ) );
			properties( ).append( prop_header_ = 1 );
			properties( ).append( prop_stream_ = 0 );
		}

		virtual ~input_aud( ) 
		{ 
			if ( context_ )
				avio_close( context_ );
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// Basic information
		virtual const pl::wstring get_uri( ) const { return spec_; }
		virtual const pl::wstring get_mime_type( ) const { return L""; }

		// Audio/Visual
		virtual int get_frames( ) const { return frames_; }
		virtual bool is_seekable( ) const { return context_ ? context_->seekable : !prop_stream_.value< int >( ); }

	protected:

		bool initialize( )
		{
			pl::wstring spec = spec_;
			if ( spec.find( L"aud:" ) == 0 )
				spec = spec.substr( 4 );

			int error = avio_open( &context_, pl::to_string( spec ).c_str( ), AVIO_FLAG_READ );
			if ( error == 0 && is_seekable( ) )
				error = parse_header( );

			return error == 0;
		}

		int parse_header( )
		{
			int error = 0;

			if ( prop_header_.value< int >( ) )
			{
				std::string header = avio_gets( context_ );
				std::istringstream str( header );
				std::string token;
				while( !str.eof( ) )
				{
					str >> token;
					if ( token.find( "af=" ) == 0 ) prop_af_ = pl::to_wstring( token.substr( 3 ) );
					else if ( token.find( "frequency=" ) == 0 ) prop_frequency_ = boost::lexical_cast< int >( token.substr( 10 ) );
					else if ( token.find( "channels=" ) == 0 ) prop_channels_ = boost::lexical_cast< int >( token.substr( 9 ) );
					else if ( token.find( "fps_num=" ) == 0 ) prop_fps_num_ = boost::lexical_cast< int >( token.substr( 8 ) );
					else if ( token.find( "fps_den=" ) == 0 ) prop_fps_den_ = boost::lexical_cast< int >( token.substr( 8 ) );
					else if ( token.find( "profile=" ) == 0 ) prop_profile_ = pl::to_wstring( token.substr( 8 ) );
					else if ( token.find( "frames=" ) == 0 ) frames_ = boost::lexical_cast< int >( token.substr( 7 ) );
					else if ( token.find( "stream=" ) == 0 ) prop_stream_ = boost::lexical_cast< int >( token.substr( 7 ) );
					else std::cerr << "unrecognised header entry " << token;
				}
				offset_ = avio_tell( context_ );
			}

			ml::audio_type_ptr audio = ml::audio::allocate( prop_af_.value< pl::wstring >( ), prop_frequency_.value< int >( ), prop_channels_.value< int >( ), 1, false );
			samples_size_ = audio->size( );

			if ( is_seekable( ) )
			{
				int bytes_per_second = samples_size_ * prop_frequency_.value< int >( );
				frames_ = int( ( prop_fps_num_.value< int >( ) * double( avio_size( context_ ) - offset_ ) / bytes_per_second ) / prop_fps_den_.value< int >( ) );
			}

			parsed_ = true;

			return error;
		}

		// Fetch method
		void do_fetch( frame_type_ptr &result )
		{
			if ( !parsed_ )
				parse_header( );

			if ( last_frame_ && last_frame_->get_position( ) == get_position( ) )
			{
				result = last_frame_->shallow( );
				return;
			}

			// Seek to requested position when required
			if ( get_position( ) != expected_ && is_seekable( ) )
			{
				boost::int64_t samples = ml::audio::samples_to_frame( get_position( ), prop_frequency_.value< int >( ), 
					prop_fps_num_.value< int >( ), prop_fps_den_.value< int >( ), 
					ml::audio::locked_profile::from_string( cl::str_util::to_t_string( prop_profile_.value< pl::wstring >( ) ) ) );
				avio_seek( context_, offset_ + samples * samples_size_, SEEK_SET );
			}
			else
			{
				seek( expected_ );
			}
			expected_ = get_position( ) + 1;

			// Construct a frame and populate with basic information
			frame_type *frame = new frame_type( );
			result = frame_type_ptr( frame );
			frame->set_fps( prop_fps_num_.value< int >( ), prop_fps_den_.value< int >( ) );
			frame->set_pts( double( get_position( ) ) * prop_fps_den_.value< int >( ) / prop_fps_num_.value< int >( ) );
			frame->set_duration( double( prop_fps_den_.value< int >( ) ) / prop_fps_num_.value< int >( ) );
			frame->set_position( get_position( ) );

			// Generate an audio object
			bool error = false;
			int samples = 0;

			if ( prop_stream_.value< int >( ) == 0 )
				samples = ml::audio::samples_for_frame( get_position( ), prop_frequency_.value< int >( ), 
					prop_fps_num_.value< int >( ), prop_fps_den_.value< int >( ), 
					ml::audio::locked_profile::from_string( cl::str_util::to_t_string( prop_profile_.value< pl::wstring >( ) ) ) );
			else
				error = avio_read( context_, ( unsigned char * )( &samples ), sizeof( samples ) ) != sizeof( samples );

			ml::audio_type_ptr audio = ml::audio::allocate( prop_af_.value< pl::wstring >( ), prop_frequency_.value< int >( ), prop_channels_.value< int >( ), samples, false );

			if ( audio )
				error = avio_read( context_, static_cast< unsigned char * >( audio->pointer( ) ), audio->size( ) ) != audio->size( );

			if ( !error )
				frame->set_audio( audio );

			if ( !is_seekable( ) && url_feof( context_ ) )
				frames_ = expected_;

			last_frame_ = result->shallow( );
		}

	private:
		pl::wstring spec_;
		pl::pcos::property prop_af_;
		pl::pcos::property prop_frequency_;
		pl::pcos::property prop_channels_;
		pl::pcos::property prop_fps_num_;
		pl::pcos::property prop_fps_den_;
		pl::pcos::property prop_profile_;
		pl::pcos::property prop_header_;
		pl::pcos::property prop_stream_;
		AVIOContext *context_;
		boost::int64_t size_;
		int frames_;
		int expected_;
		boost::int64_t offset_;
		ml::frame_type_ptr last_frame_;
		size_t samples_size_;
		bool parsed_;
};

class ML_PLUGIN_DECLSPEC store_raw : public store_type
{
	public:
		store_raw( const pl::wstring spec, const frame_type_ptr &frame ) 
			: store_type( )
			, prop_header_( pl::pcos::key::from_string( "header" ) )
			, prop_pad_( pl::pcos::key::from_string( "pad" ) )
			, spec_( spec )
			, context_( 0 )
			, valid_( false )
		{
			properties( ).append( prop_header_ = 1 );
			properties( ).append( prop_pad_ = 0 );
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
				field_order_ = image->field_order( );
			}
		}

		virtual ~store_raw( )
		{
			if ( context_ )
				avio_close( context_ );
		}

		bool init( )
		{
			int error = 0;
			if ( valid_ )
			{
				pl::wstring spec = spec_;
				if ( spec.find( L"raw:" ) == 0 )
					spec = spec.substr( 4 );

				error = avio_open( &context_, pl::to_string( spec ).c_str( ), AVIO_FLAG_WRITE );
				if ( error == 0 )
				{
					AVIOContext *aml = 0;
					if( context_->seekable && avio_open( &aml, pl::to_string( spec + L".aml" ).c_str( ), AVIO_FLAG_WRITE ) == 0 )
					{
						std::ostringstream str;
						if ( prop_header_.value< int >( ) == 0 )
							str << pl::to_string( spec_ )
								<< " pf=" << pl::to_string( pf_ )
								<< " width=" << width_ << " height=" << height_ 
								<< " fps_num=" << fps_num_ << " fps_den=" << fps_den_
								<< " sar_num=" << sar_num_ << " sar_den=" << sar_den_
								<< " field_order=" << field_order_
								<< " pad=" << prop_pad_.value< int >( )
								<< " header=" << prop_header_.value< int >( )
								<< std::endl;
						else
							str << pl::to_string( spec_ )
								<< " header=1"
								<< std::endl;

						std::string string = str.str( );

						//url_setbufsize( aml, string.size( ) );
						avio_write( aml, reinterpret_cast< const unsigned char * >( string.c_str( ) ), string.size( ) );
						avio_flush( aml );
						avio_close( aml );
					}

					if ( prop_header_.value< int >( ) )
						write_header( );
				}
			}
			return valid_ && error == 0;
		}

		void write_header( )
		{
			std::ostringstream str;
			str << "pf=" << pl::to_string( pf_ )
				<< " width=" << width_ << " height=" << height_ 
				<< " fps_num=" << fps_num_ << " fps_den=" << fps_den_
				<< " sar_num=" << sar_num_ << " sar_den=" << sar_den_
				<< " field_order=" << field_order_
				<< " pad=" << prop_pad_.value< int >( )
				<< std::endl;
			std::string string = str.str( );
			avio_write( context_, reinterpret_cast< const unsigned char * >( string.c_str( ) ), string.size( ) );
		}

		bool push( frame_type_ptr frame )
		{
			ARENFORCE_MSG( !( prop_pad_.value< int >( ) && prop_header_.value< int >( ) ), "Can't specify both pad and header" );

			il::image_type_ptr image = frame->get_image( );
			bool success = true;
			if ( image != 0 )
			{
				int pad = prop_pad_.value< int >( );

				for ( int p = 0; success && p < image->plane_count( ); p ++ )
				{
					const boost::uint8_t *dst = image->data( p );
					int pitch = image->pitch( p );
					int width = image->linesize( p );
					int height = image->height( p );
					while( success && height -- )
					{
						avio_write( context_, dst, width );
						dst += pitch;
					}
				}

				if ( pad )
				{
					int needed = pad - ( bytes_ % pad );
					padding_.resize( pad );
					if ( needed != pad )
						avio_write( context_, &padding_[ 0 ], needed );
				}

				avio_flush( context_ );
			}
			return success;
		}

		virtual frame_type_ptr flush( )
		{ return frame_type_ptr( ); }

		virtual void complete( )
		{ }

	private:
		pl::pcos::property prop_header_;
		pl::pcos::property prop_pad_;
		pl::wstring spec_;
		AVIOContext *context_;
		pl::wstring pf_;
		int valid_;
		int width_;
		int height_;
		int fps_num_;
		int fps_den_;
		int sar_num_;
		int sar_den_;
		int bytes_;
		il::field_order_flags field_order_;
		std::vector< boost::uint8_t > padding_;
};

class ML_PLUGIN_DECLSPEC store_aud : public store_type
{
	public:
		store_aud( const pl::wstring spec, const frame_type_ptr &frame ) 
			: store_type( )
			, prop_header_( pl::pcos::key::from_string( "header" ) )
			, prop_stream_( pl::pcos::key::from_string( "stream" ) )
			, spec_( spec )
			, context_( 0 )
			, valid_( false )
		{
			properties( ).append( prop_header_ = 1 );
			properties( ).append( prop_stream_ = 0 );
			if ( frame->has_audio( ) )
			{
				valid_ = true;
				ml::audio_type_ptr audio = frame->get_audio( );
				af_ = audio->af( );
				frequency_ = audio->frequency( );
				channels_ = audio->channels( );
				frame->get_fps( fps_num_, fps_den_ );
			}
		}

		virtual ~store_aud( )
		{
			if ( context_ )
				ffurl_close( context_ );
		}

		bool init( )
		{
			int error = 0;
			if ( valid_ )
			{
				pl::wstring spec = spec_;
				if ( spec.find( L"aud:" ) == 0 )
					spec = spec.substr( 4 );

				error = ffurl_open( &context_, pl::to_string( spec ).c_str( ), AVIO_FLAG_WRITE, 0, 0 );
				if ( error == 0 )
				{
					AVIOContext *aml = 0;
					if( !context_->is_streamed && avio_open( &aml, pl::to_string( spec + L".aml" ).c_str( ), AVIO_FLAG_WRITE ) == 0 )
					{
						std::ostringstream str;
						if ( prop_header_.value< int >( ) == 0 )
							str << pl::to_string( spec_ )
								<< " af=" << pl::to_string( af_ )
								<< " frequency=" << frequency_ << " channels=" << channels_
								<< " fps_num=" << fps_num_ << " fps_den=" << fps_den_
								<< " stream=" << prop_stream_.value< int >( )
								<< " header=" << prop_header_.value< int >( )
								<< std::endl;
						else
							str << pl::to_string( spec_ )
								<< " header=1"
								<< std::endl;

						std::string string = str.str( );

						//url_setbufsize( aml, string.size( ) );
						avio_write( aml, reinterpret_cast< const unsigned char * >( string.c_str( ) ), string.size( ) );
						avio_flush( aml );
						avio_close( aml );
					}

					if ( prop_header_.value< int >( ) )
						write_header( );
				}
			}
			return valid_ && error == 0;
		}

		void write_header( )
		{
			std::ostringstream str;
			str << "af=" << pl::to_string( af_ )
				<< " frequency=" << frequency_ << " channels=" << channels_
				<< " fps_num=" << fps_num_ << " fps_den=" << fps_den_
				<< " stream=" << prop_stream_.value< int >( )
				<< std::endl;
			std::string string = str.str( );
			ffurl_write( context_, reinterpret_cast< const unsigned char * >( string.c_str( ) ), string.size( ) );
		}

		bool push( frame_type_ptr frame )
		{
			ml::audio_type_ptr audio = frame->get_audio( );
			bool success = true;
			if ( audio != 0 )
			{
				if ( prop_stream_.value< int >( ) )
				{
					int samples = audio->samples( );
					success = ffurl_write( context_, ( unsigned char * )( &samples ), sizeof( samples ) ) == sizeof( samples );
				}
				if ( success )
					success = ffurl_write( context_, static_cast< unsigned char * >( audio->pointer( ) ), audio->size( ) ) == audio->size( );
			}
			return success;
		}

		virtual frame_type_ptr flush( )
		{ return frame_type_ptr( ); }

		virtual void complete( )
		{ }

	private:
		pl::pcos::property prop_header_;
		pl::pcos::property prop_stream_;
		pl::wstring spec_;
		URLContext *context_;
		pl::wstring af_;
		int valid_;
		int frequency_;
		int channels_;
		int fps_num_;
		int fps_den_;
};

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC plugin : public openmedialib_plugin
{
public:
	virtual input_type_ptr input( const pl::wstring &spec )
	{
		if ( spec.find( L"raw:" ) == 0 || spec.find( L".raw" ) != spec.npos )
			return input_type_ptr( new input_raw( spec ) );
		else
			return input_type_ptr( new input_aud( spec ) );
	}

	virtual store_type_ptr store( const pl::wstring &spec, const frame_type_ptr &frame )
	{
		if ( spec.find( L"raw:" ) == 0 || spec.find( L".raw" ) != spec.npos )
			return store_type_ptr( new store_raw( spec, frame ) );
		else
			return store_type_ptr( new store_aud( spec, frame ) );
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
		*plug = new ml::raw::plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< ml::raw::plugin * >( plug ); 
	}
}
