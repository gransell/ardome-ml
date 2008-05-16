
// TIFF - A TIFF plugin to il.

// Copyright (C) 2005 Visual Media FX Ltd.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <vector>
#include <string>

#include <tiffio.h>

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>

#include <openimagelib/plugins/tiff/tiff_plugin.hpp>

namespace fs  = boost::filesystem;
namespace il  = olib::openimagelib::il;
namespace opl = olib::openpluginlib;

namespace olib { namespace openimagelib { namespace plugins { namespace TIFF {

namespace
{
	void destroy( il::image_type* im )
	{ delete im; }

	il::image_type_ptr tiff_image_type_to_image_type( unsigned short components, int width, int height )
	{
		typedef il::image<unsigned char, il::r8g8b8>	    r8g8b8_image_type;
		typedef il::image<unsigned char, il::r8g8b8a8>   r8g8b8a8_image_type;

		if( components == 4 )
			return il::image_type_ptr( new il::image_type( r8g8b8a8_image_type( width, height, 1 ) ), destroy );
		else if( components == 3 )
			return il::image_type_ptr( new il::image_type( r8g8b8_image_type( width, height, 1 ) ), destroy );

		return il::image_type_ptr( );
	}

	il::image_type_ptr load_tiff( const fs::path& path )
	{				
		struct tiff* tif = TIFFOpen( path.native_file_string( ).c_str( ), "r" );
		if( tif == NULL )
			return il::image_type_ptr( );

		int width, height, depth;

		TIFFGetField( tif, TIFFTAG_IMAGEWIDTH,  &width );
		TIFFGetField( tif, TIFFTAG_IMAGELENGTH, &height );
		if( !TIFFGetField( tif, TIFFTAG_IMAGEDEPTH, &depth ) ) depth = 1;

		unsigned short config;
		TIFFGetField( tif, TIFFTAG_PLANARCONFIG, &config );
		if( config != PLANARCONFIG_CONTIG )
		{
			TIFFClose( tif );
			return il::image_type_ptr( );
		}

		unsigned short photo;
		TIFFGetField( tif, TIFFTAG_PHOTOMETRIC, &photo );
		if( config != PHOTOMETRIC_MINISBLACK && photo != PHOTOMETRIC_RGB )
		{
			TIFFClose( tif );
			return il::image_type_ptr( );
		}

		unsigned short bpp, components;
		TIFFGetField( tif, TIFFTAG_BITSPERSAMPLE,   &bpp );
		TIFFGetField( tif, TIFFTAG_SAMPLESPERPIXEL, &components );

		il::image_type_ptr im = tiff_image_type_to_image_type( components, width, height );
		if( !im )
			return il::image_type_ptr( );

		il::image_type::pointer data = im->data( );
		int bytes_per_pixel = components * bpp / 8;

		if( TIFFIsTiled( tif ) )
		{
			int tile_width, tile_height, tile_depth;

			TIFFGetField( tif, TIFFTAG_TILEWIDTH,  &tile_width );
			TIFFGetField( tif, TIFFTAG_TILELENGTH, &tile_height );
			if( !TIFFGetField( tif, TIFFTAG_TILEDEPTH,  &tile_depth ) )
				tile_depth = 1;

			std::vector<unsigned char> buf( TIFFTileSize( tif ) );

			for( int k = 0; k < depth; k += tile_depth )
			{
				for( int j = 0; j < height; j += tile_height )
				{
					for( int i = 0; i < width; i += tile_width )
					{
						TIFFReadTile( tif, &buf[ 0 ], i, j, k, 0 );

						for( int l = 0; l < tile_depth; ++l )
						{
							for( int m = 0; m < tile_height; ++m )
								memcpy( data + ( ( ( k + l ) * height + j + m ) * width + i ) * bytes_per_pixel, 
										&buf[ 0 ] + ( l * tile_height + m ) * tile_width * bytes_per_pixel, tile_width * bytes_per_pixel );
						}
					}
				}
			}
		}
		else
		{
			unsigned int rows;
			TIFFGetField( tif, TIFFTAG_ROWSPERSTRIP, &rows );

			unsigned int strip_size = TIFFStripSize( tif );
			for( int i = 0; i < height * depth; i += rows )
			{
				tstrip_t strip = TIFFComputeStrip( tif, i, 0 );
				TIFFReadEncodedStrip( tif, strip, data + i * im->pitch( ), strip_size );
			}
		}

		TIFFClose( tif );

		return im;
	}
}

il::image_type_ptr TIFF_plugin::load( const fs::path& path )
{ return load_tiff( path ); }

bool TIFF_plugin::store( const fs::path& path, const il::image_type_ptr& )
{ return false; }

} } } }
