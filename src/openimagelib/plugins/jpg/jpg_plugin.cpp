
// JPG - A JPG plugin to il.

// Copyright (C) 2005 Visual Media FX Ltd.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <cstdlib>
#include <vector>
#include <string>

#include <openimagelib/plugins/jpg/jpg_plugin.hpp>

extern "C" { 
#include <jpeglib.h> 
}

namespace olib { namespace openimagelib { namespace plugins { namespace JPG {

typedef il::image<unsigned char, il::surface_format> image_type;

namespace
{
	void destroy( image_type* im )
	{ delete im; }
				
	std::wstring to_wstring( const std::string& str )
	{
		std::vector<wchar_t> ws;
		ws.resize( str.size( ) + 1 );

#if _MSC_VER >= 1400
		size_t size;
		mbstowcs_s( &size, &ws[ 0 ], ws.size( ), str.c_str( ), str.size( ) );
#else
		mbstowcs( &ws[ 0 ], str.c_str( ), str.size( ) );
#endif // _MSC_VER >= 0x1400
		
		return std::wstring( ws.begin( ), ws.end( ) );
	}
				
	boost::shared_ptr<image_type> jpeg_pixelformat_to_image_type( int components, int width, int height )
	{
		using namespace olib::openimagelib::il;

		typedef boost::shared_ptr<image_type>	image_type_ptr;
		typedef image<unsigned char, r8g8b8>	r8g8b8_image_type;
		typedef image<unsigned char, l8>		l8_image_type;

		switch( components )
		{
			case 3:
				return image_type_ptr( new image_type( r8g8b8_image_type( width, height, 1 ) ), destroy );
							
			case 1:
				return image_type_ptr( new image_type( l8_image_type( width, height, 1 ) ), destroy );

			default:
				return image_type_ptr( static_cast<image_type*>( 0 ) );
		}
	}

	// TODO: use IFL on IRIX machines.
	JPG_plugin::image_type_ptr load_jpg_unix( const boost::filesystem::path& path )
	{
		typedef boost::shared_ptr<image_type> image_type_ptr;
		
		FILE* infile = fopen( path.native_directory_string( ).c_str( ), "rb" );
		if( infile == NULL ) return JPG_plugin::image_type_ptr( image_type_ptr( ) );
		
		// TODO: some proper exception handling is needed. can't really be bothered
		// with that setjmp thingie...
		struct jpeg_decompress_struct info;
		struct jpeg_error_mgr jerr;

		info.err = jpeg_std_error( &jerr );
		
		jpeg_create_decompress( &info );
		jpeg_stdio_src( &info, infile );
		jpeg_read_header( &info, TRUE );
		jpeg_start_decompress( &info );

		image_type_ptr image = jpeg_pixelformat_to_image_type( info.output_components, info.output_width, info.output_height );
		if( !image ) return JPG_plugin::image_type_ptr( image_type_ptr( ) );
		
		int stride = info.output_width * info.output_components;
		JSAMPARRAY buffer = ( *info.mem->alloc_sarray )( ( j_common_ptr ) &info, JPOOL_IMAGE, stride, 1 );
		
		int linesize = image->linesize( );
		unsigned char* pixels = image->data( );

		while( info.output_scanline < info.output_height )
		{
			jpeg_read_scanlines( &info, buffer, 1 );
			memcpy( pixels, buffer[ 0 ], linesize );
			pixels += image->pitch( );
		}
		
		jpeg_finish_decompress( &info );
		jpeg_destroy_decompress( &info );

		fclose( infile );

		return JPG_plugin::image_type_ptr( image );
	}

	bool store_jpg( const boost::filesystem::path& path, const boost::shared_ptr<image_type>& img )
	{
		int i;
		struct jpeg_compress_struct cinfo;
		struct jpeg_error_mgr jerr;
		unsigned char *line;
		FILE *out = fopen( path.native_directory_string( ).c_str( ), "wb" );

		boost::shared_ptr<image_type> image = il::convert( img, L"r8g8b8" );
		image = il::conform( image, 0 );

		if ( out != NULL ) 
		{
			line = image->data( );
			cinfo.err = jpeg_std_error( &jerr );
			jpeg_create_compress( &cinfo );
			jpeg_stdio_dest( &cinfo, out );
			cinfo.image_width = image->width( );
			cinfo.image_height = image->height( );
			cinfo.input_components = 3;
			cinfo.in_color_space = JCS_RGB;
			cinfo.optimize_coding = 1;
			jpeg_set_defaults( &cinfo );
			jpeg_set_quality( &cinfo, 50, TRUE );
			jpeg_start_compress( &cinfo, TRUE );
			for ( i = 0 ; i < image->height( ); i ++, line += image->pitch( ) )
				jpeg_write_scanlines( &cinfo, &line, 1 );
			jpeg_finish_compress( &cinfo );
			fclose( out );
			jpeg_destroy_compress( &cinfo );
			return true;
		}

		return false;
	}
}
			
JPG_plugin::image_type_ptr JPG_plugin::load( const boost::filesystem::path& path )
{ return JPG_plugin::image_type_ptr( load_jpg_unix( path ) ); }

bool JPG_plugin::store( const boost::filesystem::path& path, const JPG_plugin::image_type_ptr& image )
{ return store_jpg( path, image ); }

} } } }

