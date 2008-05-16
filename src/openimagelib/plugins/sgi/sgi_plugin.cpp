
// SGI - A SGI plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <cassert>
#include <vector>
#include <string>

#include <boost/filesystem/fstream.hpp>

#include <openimagelib/plugins/sgi/sgi_plugin.hpp>

namespace il = olib::openimagelib::il;
namespace fs = boost::filesystem;

namespace olib { namespace openimagelib { namespace plugins { namespace SGI {

namespace
{
	struct SGI_image
	{
		unsigned short magic;
		unsigned short type;
		unsigned short dim;
		unsigned short xsize, ysize, zsize;
		unsigned long rle_end;
		std::vector<unsigned int> row_start;
		std::vector<unsigned int> row_size;
	};

	void destroy( il::image_type* im )
	{ delete im; }
				
	void convert_short( unsigned short* array, long length )
	{
		unsigned char* ptr = reinterpret_cast<unsigned char*>( array );
		
		while( length-- )
		{
			unsigned int t1 = *ptr++;
			unsigned int t2 = *ptr++;
			
			*array++ = ( t1 << 8 ) | t2;
		}
	}

	void convert_int( unsigned int* array, long length )
	{
		unsigned char* ptr = reinterpret_cast<unsigned char*>( array );
		
		while( length-- )
		{
			unsigned int t1 = *ptr++;
			unsigned int t2 = *ptr++;
			unsigned int t3 = *ptr++;
			unsigned int t4 = *ptr++;
			
			*array++ = ( t1 << 24 ) | ( t2 << 16 ) | ( t3 << 8 ) | t4;
		}
	}

	bool Read_s( fs::ifstream& file, char* s, std::streamsize size, std::streamsize max )
	{
#if _MSC_VER >= 1400
		file._Read_s( s, size, max );
#else
		file.read( s, size );
#endif // _MSC_VER >= 1400
			
		return !file.fail( );
	}
		
	il::image_type_ptr sgi_image_type_to_image_type( int /*dimension*/, int channels, int width, int height )
	{
		typedef il::image<unsigned char, il::r8g8b8>	r8g8b8_image_type;
		typedef il::image<unsigned char, il::r8g8b8a8>	r8g8b8a8_image_type;
		typedef il::image<unsigned char, il::l8a8>		l8a8_image_type;
		typedef il::image<unsigned char, il::l8>		l8_image_type;

		switch( channels )
		{
			case 4:
				return il::image_type_ptr( new il::image_type( r8g8b8a8_image_type( width, height, 1 ) ), destroy );

			case 3:
				return il::image_type_ptr( new il::image_type( r8g8b8_image_type( width, height, 1 ) ), destroy );

			case 2:
				return il::image_type_ptr( new il::image_type( l8a8_image_type( width, height, 1 ) ), destroy );

			case 1:
				return il::image_type_ptr( new il::image_type( l8_image_type( width, height, 1 ) ), destroy );

			default:
				break;
		}

		return il::image_type_ptr( );
	}
	
	bool read_row( fs::ifstream& file, SGI_image& di, unsigned char* buf, int y, int z )
	{
		if( ( di.type & 0xFF00 ) == 0x0100 )
		{
			std::vector<unsigned char> tmp;
			tmp.resize( di.row_size[ y + z * di.ysize ] );
			
			file.rdbuf( )->pubseekoff( di.row_start[ y + z * di.ysize ], std::ios::beg );
			if( !Read_s( file, reinterpret_cast<char*>( &tmp[ 0 ] ), di.row_size[ y + z * di.ysize ], di.row_size[ y + z * di.ysize ] ) ) return false;

			unsigned char* s = &tmp[ 0 ];
			unsigned char* t = buf;
			
			for( ;; )
			{
				unsigned char texel = *s++;
				int count = ( int ) ( texel & 0x7F );
				
				if( !count ) break;
				
				if( ( texel & 0x80 ) != 0 )
				{
					while( count-- ) *t++ = *s++;
				}
				else
				{
					texel = *s++;
					while( count-- ) *t++ = texel;
				}
			}
		}
		else
		{
			file.rdbuf( )->pubseekoff( 512 + ( y * di.xsize ) + ( z * di.xsize * di.ysize ), std::ios::beg );
			if( !Read_s( file, reinterpret_cast<char*>( buf ), di.xsize, di.xsize ) ) return false;
		}

		return true;
	}
	
	il::image_type_ptr load_sgi( const fs::path& path )
	{
		SGI_image di;

		union { int i; char c[ 4 ]; } big_endian_test;
		big_endian_test.i = 1;
		bool is_big_endian = big_endian_test.c[ 0 ] == 1;
	
		fs::ifstream file( path, std::ios::in | std::ios::binary );
		if( !file.is_open( ) ) return il::image_type_ptr( );

		if( !Read_s( file, reinterpret_cast<char*>( &di ), 12, 12 ) )
			return il::image_type_ptr( );
			
		if( is_big_endian )
			convert_short( &di.magic, 6 );
			
		il::image_type_ptr image = sgi_image_type_to_image_type( di.dim, di.zsize, di.xsize, di.ysize );
		if( !image )			  
			return il::image_type_ptr( );
					
		// if run-length encoded.
		if( ( di.type & 0xFF00 ) == 0x0100 )
		{
			unsigned int size = di.ysize * di.zsize * sizeof( unsigned int );
			
			di.row_start.resize( size );
			di.row_size.resize( size );
			
			di.rle_end = 512 + ( 2 * size );
			
			file.rdbuf( )->pubseekoff( 512, std::ios::beg );
			
			if( !Read_s( file, reinterpret_cast<char*>( &di.row_start[ 0 ] ), size, size ) )
				return il::image_type_ptr( );
			if( !Read_s( file, reinterpret_cast<char*>( &di.row_size[ 0 ] ), size, size ) )
				return il::image_type_ptr( );
				
			if( is_big_endian )
			{
				convert_int( &di.row_start[ 0 ], size / sizeof( unsigned long ) );
				convert_int( &di.row_size[ 0 ],  size / sizeof( unsigned long ) );
			}
		}
		
		il::image_type::pointer texels = image->data( );
	
		std::vector<unsigned char> r, g, b, a;
		r.resize( di.xsize );
		g.resize( di.xsize );
		b.resize( di.xsize );
		a.resize( di.xsize );
					
		for( int y = 0; y < di.ysize; ++y )
		{
			if( di.zsize >= 4 )
			{
				if( !read_row( file, di, &r[ 0 ], y, 0 ) ) return il::image_type_ptr( );
				if( !read_row( file, di, &g[ 0 ], y, 1 ) ) return il::image_type_ptr( );
				if( !read_row( file, di, &b[ 0 ], y, 2 ) ) return il::image_type_ptr( );
				if( !read_row( file, di, &a[ 0 ], y, 3 ) ) return il::image_type_ptr( );
				
				unsigned char* rptr = &r[ 0 ];
				unsigned char* gptr = &g[ 0 ];
				unsigned char* bptr = &b[ 0 ];
				unsigned char* aptr = &a[ 0 ];
				long n = di.xsize;
				
				while( n-- )
				{
					texels[ 0 ] = *rptr++;
					texels[ 1 ] = *gptr++;
					texels[ 2 ] = *bptr++;
					texels[ 3 ] = *aptr++;
					
					texels += 4;
				}
			}
			else if( di.zsize == 3 )
			{
				if( !read_row( file, di, &r[ 0 ], y, 0 ) ) return il::image_type_ptr( );
				if( !read_row( file, di, &g[ 0 ], y, 1 ) ) return il::image_type_ptr( );
				if( !read_row( file, di, &b[ 0 ], y, 2 ) ) return il::image_type_ptr( );
				
				unsigned char* rptr = &r[ 0 ];
				unsigned char* gptr = &g[ 0 ];
				unsigned char* bptr = &b[ 0 ];
				long n = di.xsize;
				
				while( n-- )
				{
					texels[ 0 ] = *rptr++;
					texels[ 1 ] = *gptr++;
					texels[ 2 ] = *bptr++;
					
					texels += 3;
				}
			}
			else if( di.zsize == 2 )
			{
				if( !read_row( file, di, &r[ 0 ], y, 0 ) ) return il::image_type_ptr( );
				if( !read_row( file, di, &a[ 0 ], y, 1 ) ) return il::image_type_ptr( );
				
				unsigned char* rptr = &r[ 0 ];
				unsigned char* aptr = &a[ 0 ];
				long n = di.xsize;
				
				while( n-- )
				{
					texels[ 0 ] = *rptr++;
					texels[ 1 ] = *aptr++;
					
					texels += 2;
				}
			}
			else
			{
				if( !read_row( file, di, &r[ 0 ], y, 0 ) ) return il::image_type_ptr( );
				
				unsigned char* rptr = &r[ 0 ];
				long n = di.xsize;
				
				while( n-- )
				{
					texels[ 0 ] = *rptr++;
					++texels;
				}
			}
		}

		image->set_flipped( true );

		return image;
	}
}

il::image_type_ptr SGI_plugin::load( const fs::path& path )
{ return load_sgi( path ); }

bool SGI_plugin::store( const fs::path& path, const il::image_type_ptr& )
{ return false; }

} } } }
