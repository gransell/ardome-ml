// ml - A media library representation.

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#include "analyse_mpeg2.hpp"

namespace olib { namespace openmedialib { namespace ml {

analyse_mpeg2::analyse_mpeg2( )
	: key_temporal_offset_ ( olib::openpluginlib::pcos::key::from_string( "temporal_offset" ) )
	, key_temporal_stream_( olib::openpluginlib::pcos::key::from_string( "temporal_stream" ) )
	, key_frame_rate_code_( olib::openpluginlib::pcos::key::from_string( "frame_rate_code" ) )
	, key_bit_rate_value_( olib::openpluginlib::pcos::key::from_string( "bit_rate_value" ) )
	, key_vbv_buffer_size_( olib::openpluginlib::pcos::key::from_string( "vbv_buffer_size" ) )
	, key_chroma_format_( olib::openpluginlib::pcos::key::from_string( "chroma_format" ) )
	, key_temporal_reference_( olib::openpluginlib::pcos::key::from_string( "temporal_reference" ) )
	, key_picture_coding_type_( olib::openpluginlib::pcos::key::from_string( "picture_coding_type" ) )
	, key_vbv_delay_( olib::openpluginlib::pcos::key::from_string( "vbv_delay" ) )
	, key_top_field_first_( olib::openpluginlib::pcos::key::from_string( "top_field_first" ) )
	, key_frame_pred_frame_dct_( olib::openpluginlib::pcos::key::from_string( "frame_pred_frame_dct" ) )
	, key_progressive_frame_( olib::openpluginlib::pcos::key::from_string( "progressive_frame" ) )
	, key_closed_gop_( olib::openpluginlib::pcos::key::from_string( "closed_gop" ) )
	, key_broken_link_( olib::openpluginlib::pcos::key::from_string( "broken_link" ) )
	, key_analysed_( olib::openpluginlib::pcos::key::from_string( "analysed" ) )
{
	memset( &seq_hdr_, 0, sizeof( sequence_header ) );
	memset( &seq_ext_, 0, sizeof( sequence_extension ) );
	memset( &gop_hdr_, 0, sizeof( gop_header ) );
	memset( &pict_hdr_, 0, sizeof( picture_header ) );
	memset( &pict_ext_, 0, sizeof( picture_coding_extension ) );
}

bool analyse_mpeg2::analyse( olib::openmedialib::ml::stream_type_ptr stream )
{
	if( stream->codec( ) != "http://www.ardendo.com/apf/codec/mpeg/mpeg2" && stream->codec( ) != "http://www.ardendo.com/apf/codec/imx/imx" ) return false;

	if ( stream->properties( ).get_property_with_key( key_analysed_ ).valid( ) && 
		 stream->properties( ).get_property_with_key( key_analysed_ ).value< int >( ) )
		return true;

	boost::uint8_t *data = stream->bytes( );
	size_t size = stream->length( );
	boost::uint8_t *end = data + size;
	
	bool done = false;
	mpeg_start_code::type sc = find_start( data, end );
	
	pict_hdr_.found = false;
	
	while( sc != mpeg_start_code::undefined && sc != mpeg_start_code::sequence_end && !done  )
	{
		switch ( sc )
		{
			case mpeg_start_code::picture_start:
				parse_picture_header( data );
				break;
				
			case mpeg_start_code::sequence_header:
				seq_hdr_.found = false;
				seq_ext_.found = false;
				gop_hdr_.found = false;
				pict_hdr_.found = false;
				pict_ext_.found = false;
				parse_sequence_header( data );
				break;
				
			case mpeg_start_code::extension_start:
				{
					mpeg_extension_id::type ext_id = (mpeg_extension_id::type)get( data, 0, 4 );
					if( ext_id == mpeg_extension_id::sequence_extension )
					{
						parse_sequence_extension( data );
					}
					else if( ext_id == mpeg_extension_id::picture_coding_extension )
					{
						parse_picture_coding_extension( data );
					
						// When we get here we should be done
						done = true;
					}
				}
				break;
				
			case mpeg_start_code::group_start:
				parse_gop_header( data );
				break;
				
			case mpeg_start_code::sequence_end:
			case mpeg_start_code::user_data_start:
			case mpeg_start_code::sequence_error:
			default:
				break;
		}
		
		sc = find_start( data, end );
	}
	
	ARENFORCE_MSG( pict_hdr_.found, "No Picture Header found" );
	ARENFORCE_MSG( seq_hdr_.found, "No Sequence Header found" );
	ARENFORCE_MSG( gop_hdr_.found, "No GOP Header found" );
	
	olib::openpluginlib::pcos::property temporal_reference_prop( key_temporal_reference_ );
	stream->properties( ).append( temporal_reference_prop = pict_hdr_.temporal_reference );
				
	olib::openpluginlib::pcos::property picture_coding_type_prop( key_picture_coding_type_ );
	stream->properties( ).append( picture_coding_type_prop = pict_hdr_.picture_coding_type );
	
	olib::openpluginlib::pcos::property vbv_delay_prop( key_vbv_delay_ );
	stream->properties( ).append( vbv_delay_prop = pict_hdr_.vbv_delay );					
	
	olib::openpluginlib::pcos::property frame_rate_code_prop( key_frame_rate_code_ );
	stream->properties( ).append( frame_rate_code_prop = seq_hdr_.frame_rate_code );
			
	olib::openpluginlib::pcos::property bit_rate_value_prop( key_bit_rate_value_ );
	stream->properties( ).append( bit_rate_value_prop = seq_hdr_.bit_rate_value );
		
	olib::openpluginlib::pcos::property vbv_buffer_size_prop( key_vbv_buffer_size_ );
	stream->properties( ).append( vbv_buffer_size_prop = seq_hdr_.vbv_buffer_size_value );					
	
	olib::openpluginlib::pcos::property broken_link_prop( key_broken_link_ );
	stream->properties( ).append( broken_link_prop = gop_hdr_.broken_link );
	
	olib::openpluginlib::pcos::property closed_gop_prop( key_closed_gop_ );
	stream->properties( ).append( closed_gop_prop = gop_hdr_.closed_gop );					
	
	if ( seq_ext_.found )
	{
		olib::openpluginlib::pcos::property chroma_format_prop( key_chroma_format_ );
		stream->properties( ).append( chroma_format_prop = seq_ext_.chroma_format );
	}
	
	if ( pict_ext_.found )
	{
		olib::openpluginlib::pcos::property top_field_first_prop( key_top_field_first_ );
		stream->properties( ).append( top_field_first_prop = pict_ext_.top_field_first );
						
		olib::openpluginlib::pcos::property frame_pred_frame_dct_prop( key_frame_pred_frame_dct_ );
		stream->properties( ).append( frame_pred_frame_dct_prop = pict_ext_.frame_pred_frame_dct );
						
		olib::openpluginlib::pcos::property progressive_frame_prop( key_progressive_frame_ );
		stream->properties( ).append( progressive_frame_prop = pict_ext_.progressive_frame );
	}

	olib::openpluginlib::pcos::property prop_analysed_( key_analysed_ );
	stream->properties( ).append( prop_analysed_ = int( 1 ) );
	
	return true;
}

int analyse_mpeg2::get( boost::uint8_t *p, int bit_offset, int bit_count )
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
	
mpeg_start_code::type analyse_mpeg2::find_start( boost::uint8_t *&p, boost::uint8_t *end )
{
	mpeg_start_code::type result = mpeg_start_code::undefined;
	
	while ( p + 4 < end && result == mpeg_start_code::undefined )
	{
		bool found = false;
	
		while( !found && p + 4 < end )
		{
			p = ( boost::uint8_t * )memchr( p + 2, 1, end - p );
			if ( p )
			{
				found = *( p - 2 ) == 0 && *( p - 1 ) == 0;
				p ++;
			}
			else
			{
				p = end;
				return result;
			}
		}
	
		if ( found )
		{
			switch( *p ++ )
			{
				case 0x00:    result = mpeg_start_code::picture_start; break;
				case 0xb2:    result = mpeg_start_code::user_data_start; break;
				case 0xb3:    result = mpeg_start_code::sequence_header; break;
				case 0xb4:    result = mpeg_start_code::sequence_error; break;
				case 0xb5:    result = mpeg_start_code::extension_start; break;
				case 0xb7:    result = mpeg_start_code::sequence_end; break;
				case 0xb8:    result = mpeg_start_code::group_start; break;
			}
		}
	}
	
	if ( p + 4 > end )
	{
		p = end;
	}
	
	return result;
}
	
void analyse_mpeg2::parse_sequence_header( boost::uint8_t *&buf )
{
	// Size of this header is at least 64 bits == 8 bytes
	int size = 8;
	
	int bit_offset = 0;
	seq_hdr_.found = true;
	seq_hdr_.horizontal_size_value = get( buf, bit_offset, 12 ); bit_offset += 12;
	seq_hdr_.vertical_size_value = get( buf, bit_offset, 12 ); bit_offset += 12;
	seq_hdr_.aspect_ratio_information = get( buf, bit_offset, 4 ); bit_offset += 4;
	seq_hdr_.frame_rate_code = get( buf, bit_offset, 4 ); bit_offset += 4;
	seq_hdr_.bit_rate_value = get( buf, bit_offset, 18 ); bit_offset += 18;
	seq_hdr_.marker_bit = get( buf, bit_offset, 1 ); bit_offset += 1;
	seq_hdr_.vbv_buffer_size_value = get( buf, bit_offset, 10 ); bit_offset += 10;
	seq_hdr_.constrained_parameters_flag = get( buf, bit_offset, 1 ); bit_offset += 1;
	
	bool load_intra_quantiser_matrix = get( buf, bit_offset, 1 ); bit_offset += 1;
	if( load_intra_quantiser_matrix )
	{
		bit_offset += 8*64;
		size += 64;
	}
	
	bool load_non_intra_quantiser_matrix = get( buf, bit_offset, 1 ); bit_offset += 1;
	if( load_non_intra_quantiser_matrix )
	{
		bit_offset += 8*64;
		size += 64;
	}
	
	// Update data pointer
	buf += size;
}

void analyse_mpeg2::parse_sequence_extension( boost::uint8_t *&buf )
{
	// Size of sequence header extension is 48 bits == 6 bytes
	int size = 6;
	
	int bit_offset = 0;
	seq_ext_.found = true;
	seq_ext_.extension_start_code_identifier = get( buf, bit_offset, 4 ); bit_offset += 4;
	seq_ext_.profile_and_level_indication = get( buf, bit_offset, 8 ); bit_offset += 8;
	seq_ext_.progressive_sequence = get( buf, bit_offset, 1 ); bit_offset += 1;
	seq_ext_.chroma_format = get( buf, bit_offset, 2 ); bit_offset += 2;
	seq_ext_.horizontal_size_extension = get( buf, bit_offset, 2 ); bit_offset += 2;
	seq_ext_.vertical_size_extension = get( buf, bit_offset, 2 ); bit_offset += 2;
	seq_ext_.bit_rate_extension = get( buf, bit_offset, 12 ); bit_offset += 12;
	seq_ext_.marker_bit = get( buf, bit_offset, 1 ); bit_offset += 1;
	seq_ext_.vbv_buffer_size_extension = get( buf, bit_offset, 8 ); bit_offset += 8;
	seq_ext_.low_delay = get( buf, bit_offset, 1 ); bit_offset += 1;
	seq_ext_.frame_rate_extension_n = get( buf, bit_offset, 2 ); bit_offset += 2;
	seq_ext_.frame_rate_extension_d = get( buf, bit_offset, 5 ); bit_offset += 5;
	
	buf += size;
}

void analyse_mpeg2::parse_gop_header( boost::uint8_t *&buf )
{
	// Size of sequence header extension is 26 bits so increase pointer with 3
	int size = 3;
	
	int bit_offset = 0;
	gop_hdr_.found = true;
	gop_hdr_.time_code = get( buf, bit_offset, 25 ); bit_offset += 25;
	gop_hdr_.closed_gop = get( buf, bit_offset, 1 ); bit_offset += 1;
	gop_hdr_.broken_link = get( buf, bit_offset, 1 ); bit_offset += 1;
	
	buf += size;
}

void analyse_mpeg2::parse_picture_header( boost::uint8_t *&buf )
{
	int bit_offset = 0;
	pict_hdr_.found = true;
	pict_hdr_.temporal_reference = get( buf, bit_offset, 10 ); bit_offset += 10;
	pict_hdr_.picture_coding_type = get( buf, bit_offset, 3 ); bit_offset += 3;
	pict_hdr_.vbv_delay = get( buf, bit_offset, 16 ); bit_offset += 16;
	if ( pict_hdr_.picture_coding_type == 2 || pict_hdr_.picture_coding_type == 3) 
	{
		pict_hdr_.full_pel_forward_vector = get( buf, bit_offset, 1 ); bit_offset += 1;
		pict_hdr_.forward_f_code = get( buf, bit_offset, 3 ); bit_offset += 3;
	}
	if ( pict_hdr_.picture_coding_type == 3 ) 
	{
		pict_hdr_.full_pel_backward_vector = get( buf, bit_offset, 1 ); bit_offset += 1;
		pict_hdr_.backward_f_code = get( buf, bit_offset, 3 ); bit_offset += 3;
	}
}

void analyse_mpeg2::parse_picture_coding_extension( boost::uint8_t *&buf )
{
	int bit_offset = 0;
	pict_ext_.found = true;
	pict_ext_.extension_start_code_identifier = get( buf, bit_offset, 4 ); bit_offset += 4;
	pict_ext_.f_code_0_0 = get( buf, bit_offset, 4 ); bit_offset += 4;
	pict_ext_.f_code_0_1 = get( buf, bit_offset, 4 ); bit_offset += 4;
	pict_ext_.f_code_1_0 = get( buf, bit_offset, 4 ); bit_offset += 4;
	pict_ext_.f_code_1_1 = get( buf, bit_offset, 4 ); bit_offset += 4;
	pict_ext_.intra_dc_precision = get( buf, bit_offset, 2 ); bit_offset += 2;
	pict_ext_.picture_structure = get( buf, bit_offset, 2 ); bit_offset += 2;
	pict_ext_.top_field_first = get( buf, bit_offset, 1 ); bit_offset += 1;
	pict_ext_.frame_pred_frame_dct = get( buf, bit_offset, 1 ); bit_offset += 1;
	pict_ext_.concealment_motion_vectors = get( buf, bit_offset, 1 ); bit_offset += 1;
	pict_ext_.q_scale_type = get( buf, bit_offset, 1 ); bit_offset += 1;
	pict_ext_.intra_vlc_format = get( buf, bit_offset, 1 ); bit_offset += 1;
	pict_ext_.alternate_scan = get( buf, bit_offset, 1 ); bit_offset += 1;
	pict_ext_.repeat_first_field = get( buf, bit_offset, 1 ); bit_offset += 1;
	pict_ext_.chroma_420_type = get( buf, bit_offset, 1 ); bit_offset += 1;
	pict_ext_.progressive_frame = get( buf, bit_offset, 1 ); bit_offset += 1;
	pict_ext_.composite_display_flag = get( buf, bit_offset, 1 ); bit_offset += 1;
	if ( pict_ext_.composite_display_flag ) 
	{
		pict_ext_.v_axis = get( buf, bit_offset, 1 ); bit_offset += 1;
		pict_ext_.field_sequence = get( buf, bit_offset, 3 ); bit_offset += 3;
		pict_ext_.sub_carrier = get( buf, bit_offset, 1 ); bit_offset += 1;
		pict_ext_.burst_amplitude = get( buf, bit_offset, 7 ); bit_offset += 7;
		pict_ext_.sub_carrier_phase = get( buf, bit_offset, 8 ); bit_offset += 8;
	}
}

} } }
