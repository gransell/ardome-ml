// ml - A media library representation.

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#include "analyse_avc.hpp"
#include "analyse_dnxhd.hpp"
#include "analyse_dv.hpp"
#include "analyse_mpeg2.hpp"

namespace pl = olib::openpluginlib;

namespace olib { namespace openmedialib { namespace ml {

pl::pcos::key analyse_type::key_analysed_ = pl::pcos::key::from_string( "analysed" );
pl::pcos::key analyse_type::key_fps_num_ = pl::pcos::key::from_string( "fps_num" );
pl::pcos::key analyse_type::key_fps_den_ = pl::pcos::key::from_string( "fps_den" );
pl::pcos::key analyse_type::key_packet_size_ = pl::pcos::key::from_string( "packet_size" );
pl::pcos::key analyse_type::key_width_ = pl::pcos::key::from_string( "width" );
pl::pcos::key analyse_type::key_height_ = pl::pcos::key::from_string( "height" );
pl::pcos::key analyse_type::key_pf_ = pl::pcos::key::from_string( "pf" );
pl::pcos::key analyse_type::key_field_order_ = pl::pcos::key::from_string( "field_order" );
pl::pcos::key analyse_type::key_sar_num_ = pl::pcos::key::from_string( "sar_num" );
pl::pcos::key analyse_type::key_sar_den_ = pl::pcos::key::from_string( "sar_den" );
pl::pcos::key analyse_type::key_picture_coding_type_ = pl::pcos::key::from_string( "picture_coding_type" );
pl::pcos::key analyse_type::key_bit_rate_value_ = pl::pcos::key::from_string( "bit_rate_value" );
pl::pcos::key analyse_type::key_temporal_offset_ = pl::pcos::key::from_string( "temporal_offset" );
pl::pcos::key analyse_type::key_temporal_stream_ = pl::pcos::key::from_string( "temporal_stream" );
pl::pcos::key analyse_type::key_temporal_reference_ = pl::pcos::key::from_string( "temporal_reference" );

pl::pcos::key analyse_type::key_vbv_delay_ = pl::pcos::key::from_string( "vbv_delay" );
pl::pcos::key analyse_type::key_profile_and_level_ = pl::pcos::key::from_string( "profile_and_level" );
pl::pcos::key analyse_type::key_closed_gop_ = pl::pcos::key::from_string( "closed_gop" );
pl::pcos::key analyse_type::key_broken_link_ = pl::pcos::key::from_string( "broken_link" );
pl::pcos::key analyse_type::key_frame_rate_code_ = pl::pcos::key::from_string( "frame_rate_code" );
pl::pcos::key analyse_type::key_vbv_buffer_size_ = pl::pcos::key::from_string( "vbv_buffer_size" );
pl::pcos::key analyse_type::key_chroma_format_ = pl::pcos::key::from_string( "chroma_format" );
pl::pcos::key analyse_type::key_top_field_first_ = pl::pcos::key::from_string( "top_field_first" );
pl::pcos::key analyse_type::key_frame_pred_frame_dct_ = pl::pcos::key::from_string( "frame_pred_frame_dct" );
pl::pcos::key analyse_type::key_progressive_frame_ = pl::pcos::key::from_string( "progressive_frame" );
pl::pcos::key analyse_type::key_aspect_ratio_information_ = pl::pcos::key::from_string( "aspect_ratio_information" );

// Convenience method for analysing a prebuilt stream type object
bool analyse_type::analyse( stream_type_ptr stream )
{
	bool result = stream && stream->bytes( ) && stream->length( );
	
	if ( result )
	{
		pl::pcos::property_container properties = stream->properties( );
		pl::pcos::property analysed = properties.get_property_with_key( key_analysed_ );

		result = analysed.valid( ) && analysed.value< int >( );

		if ( !result )
		{
			std::string codec = stream->codec( );
			boost::uint8_t *bytes = stream->bytes( );
			size_t length = stream->length( );

			result = valid( codec ) && analysis( bytes, length ) && collect( properties );

			if ( result )
				properties.append( pl::pcos::property( key_analysed_ ) = 1 );
		}
	}

	return result;
}

// Helper function for obtaining int values from bit offsets in packets
int analyse_type::get_bits( boost::uint8_t *p, int bit_offset, int bit_count )
{
	int result = 0;
	p += bit_offset / 8;
	while( bit_count )
	{
		int bits = 8 - bit_offset % 8;
		int pattern = ( 1 << bits ) - 1;
		boost::uint8_t value = ( *p ++ & pattern );
		if ( bits > bit_count )
		{
			value = value >> ( bits - bit_count );
			bits = bit_count;
		}
		result = ( result << bits ) | value;
		bit_offset += bits;
		bit_count -= bits;
	}
	return result;
}

// Create an analyse object for the given codec
analyse_ptr analyse_factory( const std::string &codec )
{
	analyse_ptr result;

	if ( codec == "http://www.ardendo.com/apf/codec/mpeg/mpeg2" )
		result = analyse_ptr( new analyse_mpeg2( ) );
	else if ( codec == "http://www.ardendo.com/apf/codec/imx/imx" )
		result = analyse_ptr( new analyse_mpeg2( ) );
	else if ( codec == "http://www.ardendo.com/apf/codec/vc3/vc3" )
		result = analyse_ptr( new analyse_dnxhd( ) );
	else if ( codec == "http://www.ardendo.com/apf/codec/dv/dv" )
		result = analyse_ptr( new analyse_dv( ) );
	else if ( codec == "http://www.ardendo.com/apf/codec/avc/avc" )
		result = analyse_ptr( new analyse_avc( ) );

	return result;
}

// Create an analyse object for the stream
analyse_ptr analyse_factory( const stream_type_ptr stream )
{
	return stream ? analyse_factory( stream->codec( ) ) : analyse_ptr( );
}

} } }

