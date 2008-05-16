/* -*- tab-width: 4; indent-tabs-mode: t -*- */ 
// DPX - A DPX plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <sstream>

#include <boost/filesystem/fstream.hpp>

#include <openpluginlib/pl/string.hpp>

#include <openimagelib/plugins/dpx/dpx_plugin.hpp>

namespace il = olib::openimagelib::il;
namespace fs = boost::filesystem;
namespace opl = olib::openpluginlib;

namespace olib { namespace openimagelib { namespace plugins { namespace DPX {

namespace
{
	const unsigned char IMAGE_ELEMENT_DESC_USER_DEFINED		= 0;
	const unsigned char IMAGE_ELEMENT_DESC_RED				= 1;
	const unsigned char IMAGE_ELEMENT_DESC_GREEN			= 2;
	const unsigned char IMAGE_ELEMENT_DESC_BLUE				= 3;
	const unsigned char IMAGE_ELEMENT_DESC_ALPHA			= 4;
	const unsigned char IMAGE_ELEMENT_DESC_LUMA				= 6;
	const unsigned char IMAGE_ELEMENT_DESC_COLOR_DIFFERENCE	= 7;
	const unsigned char IMAGE_ELEMENT_DESC_DEPTH			= 8;
	const unsigned char IMAGE_ELEMENT_DESC_COMPOSITE_VIDEO	= 9;
	const unsigned char IMAGE_ELEMENT_DESC_RGB				= 50;
	const unsigned char IMAGE_ELEMENT_DESC_RGBA				= 51;
	const unsigned char IMAGE_ELEMENT_DESC_ABGR				= 52;
	const unsigned char IMAGE_ELEMENT_DESC_YUV422			= 100;
	const unsigned char IMAGE_ELEMENT_DESC_YUV4224			= 101;
	const unsigned char IMAGE_ELEMENT_DESC_YUV444			= 102;
	const unsigned char IMAGE_ELEMENT_DESC_YUV4444			= 103;
	
	const unsigned short IMAGE_ORIENTATION_LR_TB			= 0;
	const unsigned short IMAGE_ORIENTATION_RL_TB			= 1;
	const unsigned short IMAGE_ORIENTATION_LR_BT			= 2;
	const unsigned short IMAGE_ORIENTATION_RL_BT			= 3;
	const unsigned short IMAGE_ORIENTATION_TB_LR			= 4;
	const unsigned short IMAGE_ORIENTATION_TB_RL			= 5;
	const unsigned short IMAGE_ORIENTATION_BT_LR			= 6;
	const unsigned short IMAGE_ORIENTATION_BT_RL			= 7;
	
	const unsigned short IMAGE_DATA_PACKING_PACKED			= 0;
	const unsigned short IMAGE_DATA_PACKING_METHOD_A		= 1;
	const unsigned short IMAGE_DATA_PACKING_METHOD_B		= 2;
	
#ifdef WIN32
#	pragma pack( push, 1 )
#else
	// TODO: do the equivalent on Linux...
#endif // WIN32

	struct file_information_header
	{
		unsigned int	magic;
		unsigned int	offset;
		char			version[ 8 ];
		unsigned int	filesize;
		unsigned int	ditto;
		unsigned int	generic_header_length;
		unsigned int	industry_header_length;
		unsigned int	user_header_length;
		char			filename[ 100 ];
		char			creation_date[ 24 ];
		char			creator[ 100 ];
		char			project_name[ 200 ];
		char			copyright[ 200 ];
		unsigned int	encryption_key;
		char			reserved[ 104 ];
	};
	
	struct image_element
	{
		unsigned int	sign;
		unsigned int	low_data;
		float			low_quantity;
		unsigned int	high_data;
		float			high_quantity;
		unsigned char	descriptor;
		unsigned char	transfer;
		unsigned char	colorimetric;
		unsigned char	bitdepth;
		unsigned short	packing;
		unsigned short	encoding;
		unsigned int	offset;
		unsigned int	eol_padding;
		unsigned int	eoi_padding;
		char			description[ 32 ];
	};
	
	struct image_information_header
	{
		unsigned short	orientation;
		unsigned short	image_elements;
		unsigned int	pixels_per_line;
		unsigned int	lines_per_element;
		image_element	element[ 8 ];
		char			reserved[ 52 ];
	};

	struct additional_image_orientation_information_header
	{
		float	x_scanned_size;
		float	y_scanned_size;
		char	reserved[ 20 ];
	};
	
	struct image_orientation_information_header
	{
		unsigned int	x_offset;
		unsigned int	y_offset;
		float			x_center;
		float			y_center;
		unsigned int	x_orig;
		unsigned int	y_orig;
		char			filename[ 100 ];
		char			date_time[ 24 ];
		char			input_device_name[ 32 ];
		char			input_device_serial_name[ 32 ];
		unsigned short	border_validity[ 4 ];
		unsigned int	sar[ 2 ];
		additional_image_orientation_information_header additional;
	};
	
	struct motion_picture_film_information_header
	{
		char			id[ 2 ];
		char			type[ 2 ];
		char			offset[ 2 ];
		char			prefix[ 6 ];
		char			count[ 4 ];
		char			format[ 32 ];
		unsigned int	frame_position;
		unsigned int	sequence_length;
		unsigned int	held_count;
		float			frame_rate;
		float			shutter_angle;
		char			frame_id[ 32 ];
		char			slate[ 100 ];
		char			reserved[ 56 ];
	};

	struct television_information_header
	{
		unsigned int	smpte_time_code;
		unsigned int	smpte_user_bits;
		unsigned char	interlace;
		unsigned char	field_number;
		unsigned char	video_signal;
		unsigned char	zero;
		float			horz_sampling_rate;
		float			vert_sampling_rate;
		float			temporal_sampling_rate;
		float			time_offset;
		float			gamma;
		float			black_level;
		float			black_gain;
		float			breakpoint;
		float			reference_white;
		float			integration_time;
		char			reserved[ 76 ];
	};

	struct user_defined_data_header
	{
		char user_identification[ 32 ];
	};

#ifdef WIN32
#	pragma pack( pop )
#else
	// TODO: do the equivalent on Linux...
#endif // WIN32
	
	template<class T>
	void swap_32_bit( T* array, long length )
	{
		unsigned char* ptr = reinterpret_cast<unsigned char*>( array );
		
		while( length-- )
		{
			unsigned int t1 = *ptr++;
			unsigned int t2 = *ptr++;
			unsigned int t3 = *ptr++;
			unsigned int t4 = *ptr++;

			*array++ = static_cast<T>( ( t1 << 24 ) | ( t2 << 16 ) | ( t3 << 8 ) | t4 );
		}
	}

	template<class T>
	void swap_16_bit( T* array, long length )
	{
		unsigned char* ptr = reinterpret_cast<unsigned char*>( array );
		
		while( length-- )
		{
			unsigned short t1 = *ptr++;
			unsigned short t2 = *ptr++;

			*array++ = static_cast<T>( ( t1 << 8 ) | t2 );
		}
	}

	void swap_file_information_header( file_information_header& header )
	{
		swap_32_bit( &header.magic, 1 );
		swap_32_bit( &header.offset, 1 );
		swap_32_bit( &header.filesize, 1 );
		swap_32_bit( &header.ditto, 1 );
		swap_32_bit( &header.generic_header_length, 1 );
		swap_32_bit( &header.industry_header_length, 1 );
		swap_32_bit( &header.user_header_length, 1 );
		swap_32_bit( &header.encryption_key, 1 );
	}

	void swap_image_information_header( image_information_header& header )
	{
		swap_16_bit( &header.orientation, 1 );
		swap_16_bit( &header.image_elements, 1 );
		swap_32_bit( &header.pixels_per_line, 1 );
		swap_32_bit( &header.lines_per_element, 1 );
		
		for( int i = 0; i < 8; ++i )
		{
			swap_32_bit( &header.element[ i ].sign, 1 );
			swap_32_bit( &header.element[ i ].low_data, 1 );
			swap_32_bit( &header.element[ i ].low_quantity, 1 );
			swap_32_bit( &header.element[ i ].high_data, 1 );
			swap_32_bit( &header.element[ i ].high_quantity, 1 );
			swap_16_bit( &header.element[ i ].packing, 1 );
			swap_16_bit( &header.element[ i ].encoding, 1 );
			swap_32_bit( &header.element[ i ].offset, 1 );
			swap_32_bit( &header.element[ i ].eol_padding, 1 );
			swap_32_bit( &header.element[ i ].eoi_padding, 1 );
		}
	}
	
	void swap_image_orientation_information_header( image_orientation_information_header& header )
	{
		swap_32_bit( &header.x_offset, 1 );
		swap_32_bit( &header.y_offset, 1 );
		swap_32_bit( &header.x_center, 1 );
		swap_32_bit( &header.y_center, 1 );
		swap_32_bit( &header.x_orig, 1 );
		swap_32_bit( &header.y_orig, 1 );
		swap_16_bit( &header.border_validity[ 0 ], 4 );
		swap_32_bit( &header.sar[ 0 ], 2 );
	}
	
	void swap_motion_picture_film_information_header( motion_picture_film_information_header& header )
	{
		swap_32_bit( &header.frame_position, 1 );
		swap_32_bit( &header.sequence_length, 1 );
		swap_32_bit( &header.held_count, 1 );
		swap_32_bit( &header.frame_rate, 1 );
		swap_32_bit( &header.shutter_angle, 1 );
	}

	void swap_television_information_header( television_information_header& header )
	{
		swap_32_bit( &header.smpte_time_code, 1 );
		swap_32_bit( &header.smpte_user_bits, 1 );
		swap_32_bit( &header.horz_sampling_rate, 1 );
		swap_32_bit( &header.vert_sampling_rate, 1 );
		swap_32_bit( &header.temporal_sampling_rate, 1 );
		swap_32_bit( &header.time_offset, 1 );
		swap_32_bit( &header.gamma, 1 );
		swap_32_bit( &header.black_level, 1 );
		swap_32_bit( &header.black_gain, 1 );
		swap_32_bit( &header.breakpoint, 1 );
		swap_32_bit( &header.reference_white, 1 );
		swap_32_bit( &header.integration_time, 1 );
	}

	void destroy( il::image_type* im )
	{ delete im; }

	int descriptor_to_samples_per_pixel( unsigned char descriptor )
	{
		switch( descriptor )
		{
			case IMAGE_ELEMENT_DESC_USER_DEFINED:
			case IMAGE_ELEMENT_DESC_RED:
			case IMAGE_ELEMENT_DESC_GREEN:
			case IMAGE_ELEMENT_DESC_BLUE:
			case IMAGE_ELEMENT_DESC_ALPHA:
			case IMAGE_ELEMENT_DESC_LUMA:
			case IMAGE_ELEMENT_DESC_COLOR_DIFFERENCE:
			case IMAGE_ELEMENT_DESC_DEPTH:
			case IMAGE_ELEMENT_DESC_COMPOSITE_VIDEO:
				return 1;
				
			case IMAGE_ELEMENT_DESC_RGB:
			case IMAGE_ELEMENT_DESC_YUV422:
			case IMAGE_ELEMENT_DESC_YUV444:
				return 3;

			case IMAGE_ELEMENT_DESC_RGBA:
			case IMAGE_ELEMENT_DESC_ABGR:
			case IMAGE_ELEMENT_DESC_YUV4224:
			case IMAGE_ELEMENT_DESC_YUV4444:
				return 4;
			
			default:
				break;
		}
		
		return 0;
	}

	// NOTE: the alignment in a DPX file doesn't match what oil currently supports (oil
	// follows OpenGL conventions). Hence the need for this utility function.	
	int bytes_per_line( int bitdepth, int linesize, int packing )
	{
		if( ( bitdepth == 1 ) || ( bitdepth == 8 ) || ( ( bitdepth == 10 || bitdepth == 12 ) && packing == 0 ) )
		{
			return ( ( linesize * bitdepth + 31 ) / 32 ) * sizeof( unsigned int );
		}
		else if( bitdepth == 10 )
		{
			return ( ( ( ( ( linesize + 2 ) / 3 ) * 32 ) + 31 ) / 32 ) * sizeof( unsigned int );
		}
		else if( bitdepth == 12 )
		{
			return ( ( linesize * 16 + 15 ) / 16 ) * sizeof( unsigned short );
		}
		else if( bitdepth == 16 )
		{
			return ( ( linesize * bitdepth + 15 ) / 16 ) * sizeof( unsigned short );
		}
		else if( bitdepth == 32 )
		{
			return linesize * bitdepth * sizeof( unsigned int );
		}

		return 0;
	}
	
	bool Read_s( fs::ifstream& file, char* s, std::streamsize size, std::streamsize max )
	{
#if _MSC_VER >= 1400
		file._Read_s( s, size, max );
#else
		file.read( s, size );
#endif
			
		return !file.fail( );
	}
	
	bool Write_s( fs::ofstream& file, char* s, std::streamsize size )
	{
		return !file.write( s, size ).fail( );
	}
	
	opl::wstring generate_image_pf( const image_information_header& im_header )
	{
		std::wostringstream str;
		for( int i = 0; i < im_header.image_elements; ++i )
		{
			switch( im_header.element[ i ].descriptor )
			{
				case IMAGE_ELEMENT_DESC_USER_DEFINED:
					break;

				case IMAGE_ELEMENT_DESC_RED:
					str << "r"; str << im_header.element[ i ].bitdepth;
					break;

				case IMAGE_ELEMENT_DESC_GREEN:
					str << "g"; str << im_header.element[ i ].bitdepth;
					break;

				case IMAGE_ELEMENT_DESC_BLUE:
					str << "b"; str << im_header.element[ i ].bitdepth;
					break;

				case IMAGE_ELEMENT_DESC_ALPHA:
					str << "a"; str << im_header.element[ i ].bitdepth;
					break;

				case IMAGE_ELEMENT_DESC_LUMA:
					str << "l"; str << im_header.element[ i ].bitdepth;
					break;

				case IMAGE_ELEMENT_DESC_COLOR_DIFFERENCE:
				case IMAGE_ELEMENT_DESC_DEPTH:
				case IMAGE_ELEMENT_DESC_COMPOSITE_VIDEO:
					break;
					
				case IMAGE_ELEMENT_DESC_RGB:
					str << "r"; str << im_header.element[ i ].bitdepth;
					str << "g"; str << im_header.element[ i ].bitdepth;
					str << "b"; str << im_header.element[ i ].bitdepth;
					break;

				case IMAGE_ELEMENT_DESC_RGBA:
					str << "r"; str << im_header.element[ i ].bitdepth;
					str << "g"; str << im_header.element[ i ].bitdepth;
					str << "b"; str << im_header.element[ i ].bitdepth;
					str << "a"; str << im_header.element[ i ].bitdepth;
					break;

				case IMAGE_ELEMENT_DESC_ABGR:
					str << "a"; str << im_header.element[ i ].bitdepth;
					str << "b"; str << im_header.element[ i ].bitdepth;
					str << "g"; str << im_header.element[ i ].bitdepth;
					str << "r"; str << im_header.element[ i ].bitdepth;
					break;

				// CAUTION: implicitly assuming 8 bits.
				case IMAGE_ELEMENT_DESC_YUV422:
					str << "yuv422";
					break;

				case IMAGE_ELEMENT_DESC_YUV4224:
					str << "yuva422";
					break;

				case IMAGE_ELEMENT_DESC_YUV444:
					str << "yuv444";
					break;

				case IMAGE_ELEMENT_DESC_YUV4444:
					str << "yuva444";
					break;

				default:
					break;
			}
		}
		
		if( im_header.element[ im_header.image_elements - 1 ].transfer == 1 )
			str << "log";
		
		if( im_header.image_elements > 1 )
			str << "p";
		
		return str.str( );
	}

	void convert_12_bit_method_b_to_method_a( il::image_type_ptr im, int plane )
	{
		il::image_type::size_type width = im->linesize( plane );
		il::image_type::size_type height = im->height( plane );

		const unsigned short* src = reinterpret_cast<unsigned short*>( im->data( ) );
		il::image_type::size_type src_pitch = im->pitch( );
			
		unsigned short* dst = reinterpret_cast<unsigned short*>( im->data( ) );
		il::image_type::size_type dst_pitch = im->pitch( );

		const unsigned short* sptr = src;
		unsigned short* dptr = dst;

		il::image_type::size_type orig_width = width;

		while( height-- )
		{
			while( width-- )
				*dst++ = *src++ << 4;

			dst = dptr += dst_pitch;
			src = sptr += src_pitch;
			width = orig_width;
		}
	}

	struct stream_state
	{
		const unsigned int* bits;
		unsigned int val;
		unsigned int bits_remaining;
	};

	// NOTE: Based on GraphicsMagick code.
	unsigned int read_n_bits( stream_state& stream, int n, bool swab )
	{
		unsigned int remaining_bits;
		unsigned int bits = 0;

		remaining_bits = n;

		while( remaining_bits > 0 )
		{
			if( stream.bits_remaining == 0 )
			{
				stream.val = *stream.bits;
				
				if( swab )
					swap_32_bit( &stream.val, 1 );

				stream.bits_remaining = 32;
				++stream.bits;
			}

			unsigned int new_bits = remaining_bits;
			if( new_bits > stream.bits_remaining )
				new_bits = stream.bits_remaining;

			bits |= ( ( ( ( stream.val >> ( 32 - stream.bits_remaining ) ) & ( ~( ~0 << new_bits ) ) ) << ( n - remaining_bits ) ) ) >> ( n - 8 );

			remaining_bits -= new_bits;
			stream.bits_remaining -= new_bits;
		}
		
		return bits;
	}

	void convert_packed_sequence_to_oil( const std::vector<unsigned char>& v, il::image_type_ptr im, int bs, int bitdepth, bool swab, int plane )
	{
		int width = im->width( plane );
		int height = im->height( plane );
		
		const unsigned int* src = ( const unsigned int* ) &v[ 0 ];
			
		unsigned short* dst = ( unsigned short* ) im->data( plane );
		il::image_type::size_type dst_pitch = im->pitch( plane );

		unsigned short* dptr = dst;

		il::image_type::size_type orig_width = bs * width;
		width = orig_width;
		
		stream_state stream;
		stream.bits = src;
		stream.bits_remaining = 0;
		stream.val = 0;
		
		while( height-- )
		{
			while( width-- )
				*dst++ = static_cast<unsigned short>( read_n_bits( stream, bitdepth, swab ) << 8 );
			
			stream.bits_remaining = 0;
			stream.val = 0;

			dst = dptr += dst_pitch;
			width = orig_width;
		}
	}

	void convert_10_bit_method_a_b_to_oil( const std::vector<unsigned char>& v, il::image_type_ptr im, bool swab, int packing, int plane )
	{
		int width = im->linesize( plane );
		int height = im->height( plane );

		const unsigned int* src = ( const unsigned int* ) &v[ 0 ];

		unsigned short* dst = ( unsigned short* ) im->data( plane );
		il::image_type::size_type dst_pitch = im->pitch( plane );

		unsigned short* dptr = dst;

		il::image_type::size_type orig_width = width;

		unsigned short shift = 0, orig_shift = 0;

		if( packing == 1 )
			orig_shift = 24;
		else
			orig_shift = 22;

		unsigned int pix = 0;
		for( int i = 0; i < height; ++i )
		{
			for( int j = 0; j < width; ++j )
			{
				if( j % 3 == 0 )
				{
					pix = *src;
					
					if( swab )
						swap_32_bit( &pix, 1 );
					
					++src;
					shift = orig_shift;
				}

				*dst++ = ( static_cast<unsigned short>( ( pix >> shift ) & 0x3FF ) ) << 8; shift -= 10;
			}

			dst = dptr += dst_pitch;
			width = orig_width;
		}
	}

	il::image_type_ptr load_dpx( const fs::path& path )
	{
		fs::ifstream file( path, std::ios::in | std::ios::binary );
		if( !file.is_open( ) )
			return il::image_type_ptr( );

		file_information_header header;
		if( !Read_s( file, ( char* ) &header, sizeof( header ), sizeof( header ) ) )
			return il::image_type_ptr( );
		
		if( header.magic != 0x58504453 && header.magic != 0x53445058 )
			return il::image_type_ptr( );

		image_information_header image_info_header;
		if( !Read_s( file, ( char* ) &image_info_header, sizeof( image_info_header ), sizeof( image_info_header ) ) )
			return il::image_type_ptr( );
			
		image_orientation_information_header image_orientation_info_header;
		if( !Read_s( file, ( char* ) &image_orientation_info_header, sizeof( image_orientation_info_header ), sizeof( image_orientation_info_header ) ) )
			return il::image_type_ptr( );
		
		motion_picture_film_information_header motion_info_header;
		if( !Read_s( file, ( char* ) &motion_info_header, sizeof( motion_info_header ), sizeof( motion_info_header ) ) )
			return il::image_type_ptr( );
		
		television_information_header tv_info_header;
		if( !Read_s( file, ( char* ) &tv_info_header, sizeof( tv_info_header ), sizeof( tv_info_header ) ) )
			return il::image_type_ptr( );
		
		bool swab = header.magic == 0x58504453 ? true : false;
		
		if( swab )
		{
			swap_file_information_header( header );
			swap_image_information_header( image_info_header );
			swap_image_orientation_information_header( image_orientation_info_header );
			swap_motion_picture_film_information_header( motion_info_header );
			swap_television_information_header( tv_info_header );
		}
		
		opl::wstring pf = generate_image_pf( image_info_header );
		
		il::image_type_ptr im = il::allocate( pf, image_info_header.pixels_per_line, image_info_header.lines_per_element );
		if( !im )
			return il::image_type_ptr( );
		
		for( int i = 0; i < image_info_header.image_elements; ++i )
		{
			file.rdbuf( )->pubseekoff( image_info_header.element[ i ].offset, std::ios::beg );
			
			int bitdepth = image_info_header.element[ i ].bitdepth;
			int packing	 = image_info_header.element[ i ].packing;
			int bs = descriptor_to_samples_per_pixel( image_info_header.element[ i ].descriptor );

			int linesize = bytes_per_line( bitdepth, bs * image_info_header.pixels_per_line, packing );
			
			if( bitdepth == 8 || bitdepth == 16 || ( bitdepth == 12 && packing != 0 ) )
			{
				if( bitdepth == 8 )
				{
					il::image_type::pointer data = im->data( i );
					
					for( unsigned int j = 0; j < image_info_header.lines_per_element; ++j )
					{
						Read_s( file, reinterpret_cast<char*>( &data[ 0 ] ), linesize, linesize );
						data += im->pitch( i );
					}
				}
				else
				{
					unsigned short* data = ( unsigned short* ) im->data( i );
					
					std::vector<unsigned char> image_data( linesize );
					for( unsigned int j = 0; j < image_info_header.lines_per_element; ++j )
					{
						Read_s( file, reinterpret_cast<char*>( &image_data[ 0 ] ), linesize, linesize );
						memcpy( &data[ 0 ], &image_data[ 0 ], linesize );
						data += im->pitch( i );
					}
				}
				
				if( bitdepth == 12 && packing == 2 )
					convert_12_bit_method_b_to_method_a( im, i );

				if( swab && bitdepth != 8 )
					il::swab( im, i );
			}
			else if( bitdepth == 10 || ( bitdepth == 12 || !packing ) )
			{
				std::vector<unsigned char> image_data( linesize * image_info_header.lines_per_element );
				Read_s( file, reinterpret_cast<char*>( &image_data[ 0 ] ), linesize * image_info_header.lines_per_element, linesize * image_info_header.lines_per_element );

				if( packing == 0 )
				{
					convert_packed_sequence_to_oil( image_data, im, bs, bitdepth, swab, i );
				}
				else if( packing == 1 || packing == 2 )
				{
					convert_10_bit_method_a_b_to_oil( image_data, im, swab, packing, i );
				}
			}			
		}
		
		return im;
	}

	// IL enum support for colour spaces... and add swizzling support.
	bool element_info( int i, const opl::wstring& pf, unsigned char& bitdepth, unsigned char& descriptor, unsigned char& transfer, unsigned char& colorimetric )
	{
		transfer = 2, colorimetric = 2;
		
		if( pf == L"r8g8b8"       || pf == L"r8g8b8a8"        || pf == L"r10g10b10"	   || pf == L"r10g10b10a10" 
		 || pf == L"r12g12b12"    || pf == L"r12g12b12a12"    || pf == L"r16g16b16"	   || pf == L"r16g16b16a16" 
		 || pf == L"r8g8b8log"	  || pf == L"r8g8b8a8log"     || pf == L"r10g10b10log" || pf == L"r10g10b10a10log" 
		 || pf == L"r12g12b12log" || pf == L"r12g12b12a12log" || pf == L"r16g16b16log" || pf == L"r16g16b16a16log" )
		{
			if( pf == L"r8g8b8" || pf == L"r8g8b8a8" || pf == L"r8g8b8log" || pf == L"r8g8b8a8log" )
				bitdepth = 8;
			else if( pf == L"r10g10b10" || pf == L"r10g10b10a10" || pf == L"r10g10b10log" || pf == L"r10g10b10a10log" )
				bitdepth = 10;
			else if( pf == L"r12g12b12" || pf == L"r12g12b12a12" || pf == L"r12g12b12log" || pf == L"r12g12b12a12log" )
				bitdepth = 12;
			else if( pf == L"r16g16b16" || pf == L"r16g16b16a16" || pf == L"r16g16b16log" || pf == L"r16g16b16a16log" )
				bitdepth = 16;

			if( pf == L"r8g8b8"	   || pf == L"r10g10b10"    || pf == L"r12g12b12"    || pf == L"r16g16b16"
			 || pf == L"r8g8b8log" || pf == L"r10g10b10log" || pf == L"r12g12b12log" || pf == L"r16g16b16log" )
				descriptor = IMAGE_ELEMENT_DESC_RGB;
			else
				descriptor = IMAGE_ELEMENT_DESC_RGBA;
				
			if( pf == L"r8g8b8log"    || pf == L"r8g8b8a8log"     || pf == L"r10g10b10log" || pf == L"r10g10b10a10log" 
			 || pf == L"r12g12b12log" || pf == L"r12g12b12a12log" || pf == L"r16g16b16log" || pf == L"r16g16b16a16log" )
				transfer = 1, colorimetric = 1;
			
			return true;
		}
		else if( pf == L"r8g8b8p"	 || pf == L"r8g8b8a8p"	   || pf == L"r10g10b10p" || pf == L"r10g10b10a10p" 
			  || pf == L"r12g12b12p" || pf == L"r12g12b12a12p" || pf == L"r16g16b16p" || pf == L"r16g16b16a16p" )
		{
			if( pf == L"r8g8b8p" || pf == L"r8g8b8a8p" )
				bitdepth = 8;
			else if( pf == L"r10g10b10p" || pf == L"r10g10b10a10p" )
				bitdepth = 10;
			else if( pf == L"r12g12b12p" || pf == L"r12g12b12a12p" )
				bitdepth = 12;
			else if( pf == L"r16g16b16p" || pf == L"r16g16b16a16p" )
				bitdepth = 16;

			if( i == 0 )
				descriptor	= IMAGE_ELEMENT_DESC_RED;
			else if( i == 1 )
				descriptor	= IMAGE_ELEMENT_DESC_GREEN;
			else if( i == 2 )
				descriptor	= IMAGE_ELEMENT_DESC_BLUE;
			else if( i == 3 )
				descriptor	= IMAGE_ELEMENT_DESC_ALPHA;
				
			return true;
		}
		else if( pf == L"l12a12p" || pf == L"l16a16p" )
		{
		}
		
		return false;
	}

	bool store_dpx( const fs::path& path, const il::image_type_ptr& im )
	{
		fs::ofstream file( path, std::ios::out | std::ios::binary );
		if( !file.is_open( ) ) return false;

		il::image_type::size_type width  = im->width( );
		il::image_type::size_type height = im->height( );
		
		file_information_header header;
		memset( &header, 0, sizeof( header ) );
		header.magic = 0x53445058;
		header.offset = 2080;				// fixed value if no user defined data is present. check the specification.
		strcpy( header.version, "V2.0" );
		header.filesize = 2080 + im->size( );

		image_information_header image_info_header;
		memset( &image_info_header, 0, sizeof( image_info_header ) );
		image_info_header.orientation = 0;	// xxx
		image_info_header.image_elements = static_cast<unsigned short>( im->plane_count( ) );
		image_info_header.pixels_per_line = width;
		image_info_header.lines_per_element = height;
		
		int offset = 2080;
		for( int i = 0; i < image_info_header.image_elements; ++i )
		{	
			unsigned char bitdepth, descriptor, transfer, colorimetric;
			if( element_info( i, im->pf( ), bitdepth, descriptor, transfer, colorimetric ) )
			{
				image_info_header.element[ i ].sign = 0;
				image_info_header.element[ i ].descriptor = descriptor;
				image_info_header.element[ i ].transfer = transfer;
				image_info_header.element[ i ].colorimetric = colorimetric;
				image_info_header.element[ i ].bitdepth = bitdepth == 10 || bitdepth == 12 ? 16 : bitdepth;
				image_info_header.element[ i ].packing = 0;
				image_info_header.element[ i ].encoding = 0;
				
				if( i > 0 )
				{
					if( bitdepth == 8 )
						offset = 2080 + im->offset( i );
					else
						offset += im->linesize( i - 1 ) * height * sizeof( unsigned short );
				}
				
				image_info_header.element[ i ].offset = offset;
			}
		}

		image_orientation_information_header image_orientation_info_header;
		memset( &image_orientation_info_header, 0, sizeof( image_orientation_info_header ) );
		
		motion_picture_film_information_header motion_info_header;
		memset( &motion_info_header, 0, sizeof( motion_info_header ) );
		
		television_information_header tv_info_header;
		memset( &tv_info_header, 0, sizeof( tv_info_header ) );
		
		user_defined_data_header user_defined_header;
		memset( &user_defined_header, 0, sizeof( user_defined_header ) );

		if( !(	Write_s( file, ( char* ) &header, sizeof( header ) ) && 
				Write_s( file, ( char* ) &image_info_header, sizeof( image_info_header ) ) &&
				Write_s( file, ( char* ) &image_orientation_info_header, sizeof( image_orientation_info_header ) ) &&
				Write_s( file, ( char* ) &motion_info_header, sizeof( motion_info_header ) ) &&
				Write_s( file, ( char* ) &tv_info_header, sizeof( tv_info_header ) ) &&
				Write_s( file, ( char* ) &user_defined_header, sizeof( user_defined_header ) ) ) )
			return false;

		for( int i = 0; i < image_info_header.image_elements; ++i )
		{
			if( image_info_header.element[ i ].bitdepth == 8 )
			{
				il::image_type::const_pointer data = im->data( i );
				for( int j = 0; j < height; ++j )
				{
					if( !Write_s( file, ( char* ) data, im->pitch( i ) ) )
						return false;
						
					data += im->pitch( i );
				}
			}
			else
			{
				unsigned short* data = ( unsigned short* ) im->data( i );
				for( int j = 0; j < height; ++j )
				{
					if( !Write_s( file, ( char* ) data, im->linesize( i ) * sizeof( unsigned short ) ) )
						return false;
						
					data += im->pitch( i );
				}
			}
		}
			
		return true;
	}
}

il::image_type_ptr DPX_plugin::load( const fs::path& path )
{ return load_dpx( path ); }

bool DPX_plugin::store( const fs::path& path, const il::image_type_ptr& im )
{ return store_dpx( path, im ); }

} } } }
