
// HDR - A HDR plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifdef WIN32
#include <windows.h>
#include <gdiplus.h>

#ifdef min
#	undef min
#endif
#endif // WIN32

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>
#include <string>

#include <boost/filesystem/fstream.hpp>

#include <openimagelib/plugins/hdr/hdr_plugin.hpp>

namespace il = olib::openimagelib::il;
namespace fs = boost::filesystem;

namespace olib { namespace openimagelib { namespace plugins { namespace HDR {

namespace
{
	const int RGBE_VALID_PROGRAMTYPE	= 0x01;
	const int RGBE_VALID_GAMMA			= 0x02;
	const int RGBE_VALID_EXPOSURE		= 0x04;

	struct RgbeInfo
	{
		int   valid;
		char  program_type[ 16 ];
		float gamma;
		float exposure;
		char  format[ 16 ];
	};

	void destroy( il::image_type* im )
	{ delete im; }

	bool Read_s( fs::ifstream& file, char* s, std::streamsize size, std::streamsize max )
	{
#if _MSC_VER >= 1400
		file._Read_s( s, size, max );
#else
		file.read( s, size );
#endif
			
		return !file.fail( );
	}
	
	void float2rgbe( float red, float green, float blue, unsigned char rgbe[ 4 ] )
	{
		float v = red;
		if( v < green ) v = green;
		if( v < blue  ) v = blue;
		
		if( v < std::numeric_limits<float>::min( ) )
		{
			rgbe[ 0 ] = rgbe[ 1 ] = rgbe[ 2 ] = rgbe[ 3 ] = 0;
		}
		else
		{
			int exponent;
			v = frexp( v, &exponent );
			
			rgbe[ 0 ] = static_cast<unsigned char>( red   * v );
			rgbe[ 1 ] = static_cast<unsigned char>( green * v );
			rgbe[ 2 ] = static_cast<unsigned char>( blue  * v );
			rgbe[ 3 ] = exponent + 128;
		}	
	}
	
	void rgbe2float( unsigned char rgbe[ 4 ], float& red, float& green, float& blue )
	{
		if( rgbe[ 3 ] )
		{
			float f = ldexp( 1.0f, rgbe[ 3 ] - ( 128 + 8 ) );
			
			red	  = rgbe[ 0 ] * f;
			green = rgbe[ 1 ] * f;
			blue  = rgbe[ 2 ] * f;
		}
		else
		{
			red = green = blue = 0.0f;
		}
	}
	
	bool read_rgbe_pixels_raw( fs::ifstream& file, float* data, int width, int height )
	{
		char rgbe[ 4 ];
		
		int numpixels = width * height;
		while( numpixels-- )
		{
			if( !Read_s( file, rgbe, sizeof( rgbe ), sizeof( rgbe ) ) )
				return false;
				
			rgbe2float( ( unsigned char* ) rgbe, data[ 0 ], data[ 1 ], data[ 2 ] );
			
			data += 3;
		}
		
		return true;
	}
	
	bool read_rgbe_pixels( fs::ifstream& file, il::image_type_ptr im, int width, int height )
	{
		float* data = reinterpret_cast<float*>( im->data( ) );
				
		if( width < 8 || width > 0x7FFF )
			return read_rgbe_pixels_raw( file, data, width, height );
		
		std::vector<unsigned char> line;
		line.resize( width * 4 * sizeof( float ) );
		
		unsigned char rgbe[ 4 ];
		for( int i = 0; i < height; ++i )
		{
			if( !Read_s( file, ( char* ) rgbe, sizeof( rgbe ), sizeof( rgbe ) ) )
				return false;
				
			if( ( rgbe[ 0 ] != 2 ) || ( rgbe[ 1 ] != 2 ) || ( rgbe[ 2 ] & 0x80 ) )
			{
				rgbe2float( rgbe, data[ 0 ], data[ 1 ], data[ 2 ] );
				
				data += 3;
				
				return read_rgbe_pixels_raw( file, data, width, height );
			}
			
			if( ( ( ( int ) rgbe[ 2 ] ) << 8 | rgbe[ 3 ] ) != width )
				return false;
			
			typedef std::vector<unsigned char>::const_pointer	const_pointer;
			typedef std::vector<unsigned char>::pointer			pointer;

			pointer I = &line[ 0 ];
			
			for( int j = 0; j < 4; ++j )
			{				
				const_pointer J = &line[ ( j + 1 ) * width ];

				while( I < J )
				{
					unsigned char buf[ 2 ];
					if( !Read_s( file, ( char* ) buf, 2, 2 ) )
						return false;
						
					if( buf[ 0 ] > 128 )
					{
						int count = buf[ 0 ] - 128;
						if( !count || ( count > J - I ) )
							return false;
						
						while( count-- > 0 ) *I++ = buf[ 1 ];
					}
					else
					{
						int count = buf[ 0 ];
						if( !count || ( count > ( J - I ) ) )
							return false;
						
						*I++ = buf[ 1 ];
						if( --count > 0 )
						{
							if( !Read_s( file, ( char* ) I, count, count ) )
								return false;
						}
						
						I += count;
					}
				}				
			}
			
			for( int k = 0; k < width; ++k )
			{
				rgbe[ 0 ] = line[ k + 0 * width ];
				rgbe[ 1 ] = line[ k + 1 * width ];
				rgbe[ 2 ] = line[ k + 2 * width ];
				rgbe[ 3 ] = line[ k + 3 * width ];
					
				rgbe2float( rgbe, data[ 0 ], data[ 1 ], data[ 2 ] );
					
				data += 3;
			}
			
			data += im->pitch( ) - im->linesize( );
		}
		
		return true;
	}

	il::image_type_ptr rgbe_to_image_type( int width, int height )
	{
		typedef il::image<unsigned char, il::r32g32b32f> r32g32b32f_image_type;

		return il::image_type_ptr( new il::image_type( r32g32b32f_image_type( width, height, 1 ) ), destroy );
	}

	bool read_hdr_header( fs::ifstream& file, RgbeInfo& info, int& width, int& height )
	{
		info.valid = 0;
		memset( info.program_type, 0, sizeof( info.program_type ) );
		info.gamma = 1.0f;
		info.exposure = 1.0f;
		memset( info.format, 0, sizeof( info.program_type ) );

		char buffer[ 128 ];

		file.getline( buffer, 128 );
		if( file.fail( ) )
			return false;
			
		if( buffer[ 0 ] != '#' || buffer[ 1 ] != '?' )
			return false;
			
		info.valid = RGBE_VALID_PROGRAMTYPE;
		for( size_t i = 0; i < sizeof( info.program_type ) - 1; ++i )
		{
			if( buffer[ i + 2 ] == 0 || isspace( buffer[ i + 2 ] ) )
				break;

			info.program_type[ i ] = buffer[ i + 2 ];
		}

		file.getline( buffer, 128 );
		if( file.fail( ) )
			return false;

		for( ;; )
		{
			if( !strcmp( buffer, "FORMAT=32-bit_rle_rgbe\n" ) )
			{
#if _MSC_VER >= 1400
				strncpy_s( info.format, "32-bit_rle_rgbe", strlen( "32-bit_rle_rgbe" ) );
#else
				strncpy( info.format, "32-bit_rle_rgbe", strlen( "32-bit_rle_rgbe" ) );
#endif
			}
#if _MSC_VER >= 1400
			else if( sscanf_s( buffer, "GAMMA=%g", &info.gamma ) == 1 )
#else
			else if( sscanf( buffer, "GAMMA=%g", &info.gamma ) == 1 )
#endif
			{
				info.valid |= RGBE_VALID_GAMMA;
			}
#if _MSC_VER >= 1400
			else if( sscanf_s( buffer, "EXPOSURE=%g", &info.exposure ) == 1 )
#else
			else if( sscanf( buffer, "EXPOSURE=%g", &info.exposure ) == 1 )
#endif
			{
				info.valid |= RGBE_VALID_EXPOSURE;
			}

			file.getline( buffer, 128 );
			if( !strlen( buffer ) )
				break;
		}

		file.getline( buffer, 128 );
		if( file.fail( ) )
			return false;

#if _MSC_VER >= 1400
		if( sscanf_s( buffer, "-Y %d +X %d", &height, &width ) < 2 )
#else
		if( sscanf( buffer, "-Y %d +X %d", &height, &width ) < 2 )
#endif
			return false;
		
		return true;
	}

	il::image_type_ptr load_hdr( const fs::path& path )
	{
		fs::ifstream file( path, std::ios::in | std::ios::binary );
		if( !file.is_open( ) )
			return il::image_type_ptr( );

		int width, height;

		RgbeInfo rgbe_info;
		if( !read_hdr_header( file, rgbe_info, width, height ) )
			return il::image_type_ptr( );

		il::image_type_ptr image = rgbe_to_image_type( width, height );
		if( !image )
			return il::image_type_ptr( );
			
		if( !read_rgbe_pixels( file, image, width, height ) )
			return il::image_type_ptr( );
			
		return image;
	}
}
			
il::image_type_ptr HDR_plugin::load( const fs::path& path )
{ return il::image_type_ptr( load_hdr( path ) ); }

bool HDR_plugin::store( const fs::path&, const il::image_type_ptr& )
{ return false; }

} } } }
