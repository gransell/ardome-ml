
// TGA - A TGA plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifdef WIN32
#include <windows.h>
#include <gdiplus.h>
#endif // WIN32

#include <vector>
#include <string>

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>

#include <openimagelib/plugins/tga/tga_plugin.hpp>

namespace fs  = boost::filesystem;
namespace il  = olib::openimagelib::il;
namespace opl = olib::openpluginlib;

namespace olib { namespace openimagelib { namespace plugins { namespace TGA {

namespace
{
	void destroy( il::image_type* im )
	{ delete im; }

	bool Read_s( fs::ifstream& file, char* s, std::streamsize size, std::streamsize max )
	{
#if _MSC_VER >= 1400
		file._Read_s( s, size, max );
#else
		file.read( s, size );
#endif // _MSC_VER >= 1400
			
		return !file.fail( );
	}

	il::image_type_ptr tga_image_type_to_image_type( unsigned char type, unsigned char bpp, int width, int height )
	{
		typedef il::image<unsigned char, il::b8g8r8>	b8g8r8_image_type;
		typedef il::image<unsigned char, il::b8g8r8a8>	b8g8r8a8_image_type;

#	define TGA_TYPE_MAPPED		1
#	define TGA_TYPE_COLOR		2
#	define TGA_TYPE_GRAY		3
#	define TGA_TYPE_MAPPED_RLE	9
#	define TGA_TYPE_COLOR_RLE	10
#	define TGA_TYPE_GRAY_RLE	11

		switch( type )
		{
			case TGA_TYPE_COLOR:
			case TGA_TYPE_COLOR_RLE:
				if( bpp == 32 )
					return il::image_type_ptr( new il::image_type( b8g8r8a8_image_type( width, height, 1 ) ), destroy );
				else if( bpp == 24 )
					return il::image_type_ptr( new il::image_type( b8g8r8_image_type( width, height, 1 ) ), destroy );
							
			default:
				return il::image_type_ptr( );
		}
	}

	il::image_type_ptr load_tga( const fs::path& path )
	{				
#ifdef WIN32
		#pragma pack( push, 1 )
#else
		// TODO: do the equivalent on Linux...
#endif // WIN32

		struct TgaHeader
		{
			unsigned char idLength;
			unsigned char colorMapType;

			unsigned char imageType;

			unsigned char colorMapIndexLo, colorMapIndexHi;
			unsigned char colorMapLengthLo, colorMapLengthHi;
			unsigned char colorMapSize;

			unsigned char xOriginLo, xOriginHi;
			unsigned char yOriginLo, yOriginHi;

			unsigned char widthLo, widthHi;
			unsigned char heightLo, heightHi;

			unsigned char bpp;

			unsigned char descriptor;
		};

		struct TgaFooter
		{
			unsigned int extensionAreaOffset;
			unsigned int developerDirectoryOffset;
			char signature[ 16 ];
			char dot;
			char null;
		};

#ifdef WIN32
#pragma pack( pop )
#else
	// TODO: do the equivalent on Linux...
#endif // WIN32

		fs::ifstream file( path, std::ios::in | std::ios::binary );
		if( !file.is_open( ) )
			return il::image_type_ptr( );

		std::ios::pos_type beg = file.rdbuf( )->pubseekoff( 0, std::ios::beg );
		std::ios::pos_type end = file.rdbuf( )->pubseekoff( 0, std::ios::end );
		file.rdbuf( )->pubseekoff( 0, std::ios::beg );

		std::vector<char> stream_data;
		stream_data.resize( end - beg );
		Read_s( file, &stream_data[ 0 ], end - beg, end - beg );
		
		const char* data = &stream_data[ 0 ];

		TgaHeader head;
		head = *( ( TgaHeader* ) data );
		data += sizeof( TgaHeader );

#define TGA_SIGNATURE "TRUEVISION-XFILE"
#undef  TGA_SIGNATURE

#define TGA_DESC_ABITS		0x0F
#define TGA_DESC_HORIZONTAL	0x10
#define TGA_DESC_VERTICAL	0x20

		int width	= ( head.widthHi  << 8 ) | head.widthLo;
		int height	= ( head.heightHi << 8 ) | head.heightLo;
		int horzrev	= ( head.descriptor & TGA_DESC_HORIZONTAL );
		int vertrev = ( head.descriptor & TGA_DESC_VERTICAL );
					
#undef TGA_DESC_ABITS
#undef TGA_DESC_HORIZONTAL
#undef TGA_DESC_VERTICAL
					
		il::image_type_ptr im = tga_image_type_to_image_type( head.imageType, head.bpp, width, height );
		if( !im )
			return il::image_type_ptr( );
					
		im->set_flipped( vertrev == 0 );
		im->set_flopped( horzrev != 0 );

		il::image_type::pointer t = im->data( );
		int linesize = im->linesize( );

		if( head.imageType == TGA_TYPE_MAPPED_RLE || head.imageType == TGA_TYPE_COLOR_RLE || head.imageType == TGA_TYPE_GRAY_RLE )
		{
			int	size		= im->size( );
			int	accum_count	= 0;
			int	remainder	= 0;
			int	texel_size	= head.bpp / 8;
			
			while( size	> 0 )
			{
				unsigned char c;

				c = *( ( unsigned char* ) data );
				++data;
				
				int count = ( int ) ( c & 0x7F ) + 1;

				size		-= count * texel_size;
				accum_count	+= count * texel_size;
				
				if( accum_count > linesize )
					remainder = ( accum_count - linesize ) / texel_size;

				if( ( c & 0x80 ) != 0 )
				{
					unsigned char texel[ 4 ];
					
					memcpy( texel, data, texel_size );
					data += texel_size;
					
					int siz = count - remainder;
					while( siz-- )
					{
						memcpy( t, texel, texel_size );
						t += texel_size;
					}

					if( remainder )
					{
						t += im->pitch( ) - im->linesize( );
						while( remainder-- )
						{
							memcpy( t, texel, texel_size );
							t += texel_size;
						}
						
						accum_count = 0;
						remainder = 0;
					}
				}
				else
				{
					int siz = count - remainder;

					memcpy( t, data, siz * texel_size );
					data += siz * texel_size;
					t	 += siz * texel_size;

					if( remainder )
					{
						t += im->pitch( ) - im->linesize( );
						
						memcpy( t, data, remainder * texel_size );
						data += remainder * texel_size;						
						t	 += remainder * texel_size;
						
						accum_count = 0;
						remainder = 0;
					}
				}
			}
		}
		else
		{
			for( int i = 0; i < im->height( ); ++i )
			{
				memcpy( t, data, linesize );
				data += linesize;
				t	 += im->pitch( );
			}
		}
	
		return im;
	}

#undef TGA_TYPE_MAPPED
#undef TGA_TYPE_COLOR
#undef TGA_TYPE_GRAY
#undef TGA_TYPE_MAPPED_RLE
#undef TGA_TYPE_COLOR_RLE
#undef TGA_TYPE_GRAY_RLE
}
			
il::image_type_ptr TGA_plugin::load( const fs::path& path )
{ return load_tga( path ); }

bool TGA_plugin::store( const fs::path& path, const il::image_type_ptr& )
{ return false; }

} } } }
