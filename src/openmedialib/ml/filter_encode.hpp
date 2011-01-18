// ml - A media library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef OPENMEDIALIB_FILTER_ENCODE_INC_
#define OPENMEDIALIB_FILTER_ENCODE_INC_

#include <openmedialib/ml/filter_simple.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/log_defines.hpp>

namespace olib { namespace openmedialib { namespace ml {

class ML_DECLSPEC filter_encode_type : public filter_simple
{
	public:
		filter_encode_type( )
			: filter_simple( )
			, prop_force_( olib::openpluginlib::pcos::key::from_string( "force" ) )
			, prop_width_( olib::openpluginlib::pcos::key::from_string( "width" ) )
			, prop_height_( olib::openpluginlib::pcos::key::from_string( "height" ) )
			, prop_fps_num_( olib::openpluginlib::pcos::key::from_string( "fps_num" ) )
			, prop_fps_den_( olib::openpluginlib::pcos::key::from_string( "fps_den" ) )
			, prop_sar_num_( olib::openpluginlib::pcos::key::from_string( "sar_num" ) )
			, prop_sar_den_( olib::openpluginlib::pcos::key::from_string( "sar_den" ) )
			, prop_interlace_( olib::openpluginlib::pcos::key::from_string( "interlace" ) )
			, prop_bit_rate_( olib::openpluginlib::pcos::key::from_string( "bit_rate" ) )
		{
			properties( ).append( prop_force_ = 0 );
			properties( ).append( prop_width_ = -1 );
			properties( ).append( prop_height_ = -1 );
			properties( ).append( prop_fps_num_ = -1 );
			properties( ).append( prop_fps_den_ = -1 );
			properties( ).append( prop_sar_num_ = -1 );
			properties( ).append( prop_sar_den_ = -1 );
			properties( ).append( prop_interlace_ = -1 );
			properties( ).append( prop_bit_rate_ = -1 );
		}

		virtual ~filter_encode_type( )
		{
		}

	protected:
		virtual bool matching( ml::frame_type_ptr &frame )
		{
			// If we're forcing an encode here, we ignore the rest of the stream checks
			if ( prop_force_.value< int >( ) ) return false;

			// Obtain the stream component
			stream_type_ptr stream = frame->get_stream( );

			// If it doesn't exist, there's no match
			if ( stream == 0 ) return false;

			// Check the video_codec
			if ( frame->get_stream( )->codec( ) != video_codec_ ) return false;

			// We default the bit rate if it hasn't been specified
			if ( prop_bit_rate_.value< int >( ) == -1 ) 
			{ 
				ARLOG_DEBUG3( "bit_rate has not been assigned - defaulting to first stream value of %1%" )( stream->bitrate( ) );
				prop_bit_rate_ = stream->bitrate( );
			}
			else if ( prop_bit_rate_.value< int >( ) != stream->bitrate( ) ) return false;

			return true;
		}

		virtual bool valid( ml::frame_type_ptr &frame )
		{
			olib::openimagelib::il::image_type_ptr image = frame->get_image( );

			if ( !image ) 
			{
				ARLOG_DEBUG3( "No image available for %1%" )( frame->get_position( ) );
				return false;
			}

			bool result = true;

			result = default_value( prop_width_, "width", image->width( ) ) && result;
			result = default_value( prop_height_, "height", image->height( ) ) && result;
			result = default_value( prop_fps_num_, "fps_num", frame->get_fps_num( ) ) && result;
			result = default_value( prop_fps_den_, "fps_den", frame->get_fps_den( ) ) && result;
			result = default_value( prop_sar_num_, "sar_den", frame->get_sar_num( ) ) && result;
			result = default_value( prop_sar_den_, "sar_den", frame->get_sar_den( ) ) && result;
			result = default_value( prop_interlace_, "interlace", int( image->field_order( ) ) ) && result;

			return result;
		}

	private:
		bool default_value( olib::openpluginlib::pcos::property &prop, std::string name, int value )
		{
			bool result = true;
			if ( prop.value< int >( ) == -1 )
			{
				ARLOG_DEBUG3( "%1% has not been assigned - defaulting to first stream value of %2%" )( name )( value );
				prop = value;
			}
			else if ( prop.value< int >( ) != value )
			{
				ARLOG_DEBUG3( "%1% of %2% does not match %3%" )( name )( prop.value< int >( ) )( value );
				result = false;
			}
			return result;
		}

	protected:
		olib::openpluginlib::pcos::property prop_force_;
		olib::openpluginlib::pcos::property prop_width_;
		olib::openpluginlib::pcos::property prop_height_;
		olib::openpluginlib::pcos::property prop_fps_num_;
		olib::openpluginlib::pcos::property prop_fps_den_;
		olib::openpluginlib::pcos::property prop_sar_num_;
		olib::openpluginlib::pcos::property prop_sar_den_;
		olib::openpluginlib::pcos::property prop_interlace_;
		olib::openpluginlib::pcos::property prop_bit_rate_;
		std::string video_codec_;

};

} } }

#endif
