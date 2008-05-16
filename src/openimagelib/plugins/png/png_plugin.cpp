
// PNG - A PNG plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <cstdio>

#include <cstdlib>
#include <memory>
#include <vector>
#include <string>

#include <png.h>

#include <openimagelib/plugins/png/png_plugin.hpp>

namespace il = olib::openimagelib::il;
namespace fs = boost::filesystem;

namespace olib { namespace openimagelib { namespace plugins { namespace PNG {

typedef il::image<unsigned char, il::surface_format> image_type;

namespace
{
	void destroy( image_type* im )
	{ delete im; }
				
	boost::shared_ptr<image_type> png_pixelformat_to_image_type( int channels, int width, int height )
	{
		typedef il::image<unsigned char, il::r8g8b8>	r8g8b8_image_type;
		typedef il::image<unsigned char, il::r8g8b8a8>	r8g8b8a8_image_type;
		typedef il::image<unsigned char, il::l8>		l8_image_type;

		switch( channels )
		{
			case 4:
				return il::image_type_ptr( new image_type( r8g8b8a8_image_type( width, height, 1 ) ), destroy );

			case 3:
				return il::image_type_ptr( new image_type( r8g8b8_image_type( width, height, 1 ) ), destroy );

			case 1:
				return il::image_type_ptr( new image_type( l8_image_type( width, height, 1 ) ), destroy );
							
			default:
				return il::image_type_ptr( );
		}
	}

	il::image_type_ptr load_png( const fs::path& path )
	{
		// TODO: some proper exception handling is needed.
		FILE* infile = fopen( path.native_directory_string( ).c_str( ), "rb" );
		if( infile == NULL ) {
		    fprintf( stderr, "load_png: failed to open file: %s\n", path.native_directory_string( ).c_str( ) );
		    return il::image_type_ptr( );
		}
		
		png_byte sig[ 8 ];
		fread( sig, 1, 8, infile );
		if( !png_check_sig( sig, 8 ) ) {
		    fprintf( stderr, "load_png: signature check failed\n" );
		    return il::image_type_ptr( ); 
		}
		
		png_structp png_ptr;
		png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
		
		png_infop info_ptr;
		info_ptr = png_create_info_struct( png_ptr );
		
		png_init_io( png_ptr, infile );
		png_set_sig_bytes( png_ptr, 8 );
		png_read_info( png_ptr, info_ptr );
		
		png_uint_32 width, height;
		int bit_depth, color_type;
		png_get_IHDR( png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL );

		png_uint_32 row_bytes = png_get_rowbytes( png_ptr, info_ptr );
		png_uint_32 channels  = png_get_channels( png_ptr, info_ptr );
		
		il::image_type_ptr image = png_pixelformat_to_image_type( channels, width, height );
		if( !image ) return il::image_type_ptr( );
		
		png_bytepp buffer = new png_bytep [ height ];
		for( image_type::size_type i = 0; i < image->height( ); ++i )
			buffer[ i ] = new png_byte [ row_bytes ];
		
		int 			linesize = image->linesize( );
		unsigned char* 	pixels   = image->data( );
		
		png_read_image( png_ptr, buffer );
		
		for( image_type::size_type i = 0; i < image->height( ); ++i )
		{
			memcpy( pixels, buffer[ i ], linesize );
			pixels += image->pitch( );
		}

		for( image_type::size_type i = 0; i < image->height( ); ++i )
			delete[ ] buffer[ i ];
		delete[ ] buffer;

		png_destroy_info_struct( png_ptr, &info_ptr );
		png_destroy_read_struct( &png_ptr, NULL, NULL );
		
		fclose( infile );
		
		return il::image_type_ptr( image );
	}
}

il::image_type_ptr PNG_plugin::load( const fs::path& path )
{ return il::image_type_ptr( load_png( path ) ); }

bool PNG_plugin::store( const fs::path& /*path*/, const il::image_type_ptr& /*image*/ )
{ return false; }

} } } }
