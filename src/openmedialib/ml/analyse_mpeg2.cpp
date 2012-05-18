// ml - A media library representation.

// Copyright (C) 2011 Vizrt
// Released under the LGPL.

#include "analyse_mpeg2.hpp"

namespace pl = olib::openpluginlib;

namespace olib { namespace openmedialib { namespace ml {

typedef boost::rational< int > rational;

analyse_mpeg2::analyse_mpeg2( )
{
	memset( &seq_hdr_, 0, sizeof( sequence_header ) );
	memset( &seq_ext_, 0, sizeof( sequence_extension ) );
	memset( &gop_hdr_, 0, sizeof( gop_header ) );
	memset( &pict_hdr_, 0, sizeof( picture_header ) );
	memset( &pict_ext_, 0, sizeof( picture_coding_extension ) );
}

bool analyse_mpeg2::valid( const std::string &codec )
{
	return codec == "http://www.ardendo.com/apf/codec/mpeg/mpeg2" || codec == "http://www.ardendo.com/apf/codec/imx/imx";
}

bool analyse_mpeg2::analysis( boost::uint8_t *data, const size_t size )
{
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
					mpeg_extension_id::type ext_id = (mpeg_extension_id::type)get_bits( data, 0, 4 );
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
	
	return pict_hdr_.found && seq_hdr_.found && gop_hdr_.found;
}

static rational calculate_sar( int width, int height, int ar )
{
	double result = height == 512 || height == 608 ? height - 32 : height;

	if ( ar == 2 )
		result = result * 4.0 / 3.0;
	else if ( ar == 3 )
		result = result * 16.0 / 9.0;
	else if ( ar == 4 )
		result = result * 221.0 / 100.0; // ?2.21?

	return rational( int( 3.0 * result + 0.5 ), 3 * width );
}

bool analyse_mpeg2::collect( pl::pcos::property_container &properties )
{
	ARENFORCE_MSG( pict_hdr_.found, "No Picture Header found" );
	ARENFORCE_MSG( seq_hdr_.found, "No Sequence Header found" );
	ARENFORCE_MSG( gop_hdr_.found, "No GOP Header found" );

	properties.append( pl::pcos::property( key_temporal_reference_ ) = pict_hdr_.temporal_reference );
	properties.append( pl::pcos::property( key_picture_coding_type_ ) = pict_hdr_.picture_coding_type );
	properties.append( pl::pcos::property( key_vbv_delay_ ) = pict_hdr_.vbv_delay );

	properties.append( pl::pcos::property( key_frame_rate_code_ ) = seq_hdr_.frame_rate_code );
	properties.append( pl::pcos::property( key_bit_rate_value_ ) = seq_hdr_.bit_rate_value );
	properties.append( pl::pcos::property( key_width_ ) = seq_hdr_.horizontal_size_value );
	properties.append( pl::pcos::property( key_height_ ) = seq_hdr_.vertical_size_value );
	properties.append( pl::pcos::property( key_aspect_ratio_information_ ) = seq_hdr_.aspect_ratio_information );
	properties.append( pl::pcos::property( key_vbv_buffer_size_ ) = seq_hdr_.vbv_buffer_size_value );

	properties.append( pl::pcos::property( key_broken_link_ ) = gop_hdr_.broken_link );
	properties.append( pl::pcos::property( key_closed_gop_ ) = gop_hdr_.closed_gop );

	if ( seq_ext_.found )
		properties.append( pl::pcos::property( key_chroma_format_ ) = seq_ext_.chroma_format );
	
	if ( pict_ext_.found )
	{
		properties.append( pl::pcos::property( key_top_field_first_ ) = pict_ext_.top_field_first );
		properties.append( pl::pcos::property( key_frame_pred_frame_dct_ ) = pict_ext_.frame_pred_frame_dct );
		properties.append( pl::pcos::property( key_progressive_frame_ ) = pict_ext_.progressive_frame );
	}

	rational sar = calculate_sar( seq_hdr_.horizontal_size_value, seq_hdr_.vertical_size_value, seq_hdr_.aspect_ratio_information );

	properties.append( pl::pcos::property( key_sar_num_ ) = sar.numerator( ) );
	properties.append( pl::pcos::property( key_sar_den_ ) = sar.denominator( ) );

	return true;
}

mpeg_start_code::type analyse_mpeg2::find_start( boost::uint8_t *&p, boost::uint8_t *end )
{
	mpeg_start_code::type result = mpeg_start_code::undefined;
	
	while ( p + 4 < end && result == mpeg_start_code::undefined )
	{
		bool found = false;
	
		while( !found && p + 4 < end )
		{
			p = ( boost::uint8_t * )memchr( p + 2, 1, end - ( p + 2 ) );
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
	seq_hdr_.horizontal_size_value = get_bits( buf, bit_offset, 12 ); bit_offset += 12;
	seq_hdr_.vertical_size_value = get_bits( buf, bit_offset, 12 ); bit_offset += 12;
	seq_hdr_.aspect_ratio_information = get_bits( buf, bit_offset, 4 ); bit_offset += 4;
	seq_hdr_.frame_rate_code = get_bits( buf, bit_offset, 4 ); bit_offset += 4;
	seq_hdr_.bit_rate_value = get_bits( buf, bit_offset, 18 ); bit_offset += 18;
	seq_hdr_.marker_bit = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
	seq_hdr_.vbv_buffer_size_value = get_bits( buf, bit_offset, 10 ); bit_offset += 10;
	seq_hdr_.constrained_parameters_flag = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
	
	bool load_intra_quantiser_matrix = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
	if( load_intra_quantiser_matrix )
	{
		bit_offset += 8*64;
		size += 64;
	}
	
	bool load_non_intra_quantiser_matrix = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
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
	seq_ext_.extension_start_code_identifier = get_bits( buf, bit_offset, 4 ); bit_offset += 4;
	seq_ext_.profile_and_level_indication = get_bits( buf, bit_offset, 8 ); bit_offset += 8;
	seq_ext_.progressive_sequence = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
	seq_ext_.chroma_format = get_bits( buf, bit_offset, 2 ); bit_offset += 2;
	seq_ext_.horizontal_size_extension = get_bits( buf, bit_offset, 2 ); bit_offset += 2;
	seq_ext_.vertical_size_extension = get_bits( buf, bit_offset, 2 ); bit_offset += 2;
	seq_ext_.bit_rate_extension = get_bits( buf, bit_offset, 12 ); bit_offset += 12;
	seq_ext_.marker_bit = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
	seq_ext_.vbv_buffer_size_extension = get_bits( buf, bit_offset, 8 ); bit_offset += 8;
	seq_ext_.low_delay = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
	seq_ext_.frame_rate_extension_n = get_bits( buf, bit_offset, 2 ); bit_offset += 2;
	seq_ext_.frame_rate_extension_d = get_bits( buf, bit_offset, 5 ); bit_offset += 5;
	
	buf += size;
}

void analyse_mpeg2::parse_gop_header( boost::uint8_t *&buf )
{
	// Size of sequence header extension is 26 bits so increase pointer with 3
	int size = 3;
	
	int bit_offset = 0;
	gop_hdr_.found = true;
	gop_hdr_.time_code = get_bits( buf, bit_offset, 25 ); bit_offset += 25;
	gop_hdr_.closed_gop = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
	gop_hdr_.broken_link = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
	
	buf += size;
}

void analyse_mpeg2::parse_picture_header( boost::uint8_t *&buf )
{
	int bit_offset = 0;
	pict_hdr_.found = true;
	pict_hdr_.temporal_reference = get_bits( buf, bit_offset, 10 ); bit_offset += 10;
	pict_hdr_.picture_coding_type = get_bits( buf, bit_offset, 3 ); bit_offset += 3;
	pict_hdr_.vbv_delay = get_bits( buf, bit_offset, 16 ); bit_offset += 16;
	if ( pict_hdr_.picture_coding_type == 2 || pict_hdr_.picture_coding_type == 3) 
	{
		pict_hdr_.full_pel_forward_vector = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
		pict_hdr_.forward_f_code = get_bits( buf, bit_offset, 3 ); bit_offset += 3;
	}
	if ( pict_hdr_.picture_coding_type == 3 ) 
	{
		pict_hdr_.full_pel_backward_vector = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
		pict_hdr_.backward_f_code = get_bits( buf, bit_offset, 3 ); bit_offset += 3;
	}
}

void analyse_mpeg2::parse_picture_coding_extension( boost::uint8_t *&buf )
{
	int bit_offset = 0;
	pict_ext_.found = true;
	pict_ext_.extension_start_code_identifier = get_bits( buf, bit_offset, 4 ); bit_offset += 4;
	pict_ext_.f_code_0_0 = get_bits( buf, bit_offset, 4 ); bit_offset += 4;
	pict_ext_.f_code_0_1 = get_bits( buf, bit_offset, 4 ); bit_offset += 4;
	pict_ext_.f_code_1_0 = get_bits( buf, bit_offset, 4 ); bit_offset += 4;
	pict_ext_.f_code_1_1 = get_bits( buf, bit_offset, 4 ); bit_offset += 4;
	pict_ext_.intra_dc_precision = get_bits( buf, bit_offset, 2 ); bit_offset += 2;
	pict_ext_.picture_structure = get_bits( buf, bit_offset, 2 ); bit_offset += 2;
	pict_ext_.top_field_first = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
	pict_ext_.frame_pred_frame_dct = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
	pict_ext_.concealment_motion_vectors = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
	pict_ext_.q_scale_type = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
	pict_ext_.intra_vlc_format = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
	pict_ext_.alternate_scan = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
	pict_ext_.repeat_first_field = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
	pict_ext_.chroma_420_type = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
	pict_ext_.progressive_frame = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
	pict_ext_.composite_display_flag = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
	if ( pict_ext_.composite_display_flag ) 
	{
		pict_ext_.v_axis = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
		pict_ext_.field_sequence = get_bits( buf, bit_offset, 3 ); bit_offset += 3;
		pict_ext_.sub_carrier = get_bits( buf, bit_offset, 1 ); bit_offset += 1;
		pict_ext_.burst_amplitude = get_bits( buf, bit_offset, 7 ); bit_offset += 7;
		pict_ext_.sub_carrier_phase = get_bits( buf, bit_offset, 8 ); bit_offset += 8;
	}
}

} } }
