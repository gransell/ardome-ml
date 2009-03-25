
// EXR - An ILM OpenEXR plugin to il.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <ImfRgbaFile.h>
#include <ImfCRgbaFile.h>
#include <ImfArray.h>

#include <openimagelib/plugins/exr/exr_plugin.hpp>

namespace fs  = boost::filesystem;
namespace il  = olib::openimagelib::il;
namespace opl = olib::openpluginlib;

namespace olib { namespace openimagelib { namespace plugins { namespace EXR {

namespace
{
	void destroy( il::image_type* im )
	{ delete im; }

	il::image_type_ptr load_exr( const fs::path& path )
	{
		Imf::RgbaInputFile file( path.native_file_string( ).c_str( ) );
		
		Imath::Box2i dw = file.header( ).dataWindow( );
		int width  = dw.max.x - dw.min.x + 1;
		int height = dw.max.y - dw.min.y + 1;
		
		Imf::Array2D<Imf::Rgba> exr_texels;
		exr_texels.resizeErase( height, width );
		
		file.setFrameBuffer( &exr_texels[ 0 ][ 0 ], 1, width );
		file.readPixels( dw.min.y, dw.max.y );

		il::image_type_ptr im = il::allocate( L"r32g32b32f", width, height );
		il::image_type::pointer texels = im->data( );
		
		for( int j = 0; j < height; ++j )
		{
			for( int i = 0; i < width; ++i )
			{
				( ( float* ) texels )[ 0 ] = exr_texels[ j ][ i ].r;
				( ( float* ) texels )[ 1 ] = exr_texels[ j ][ i ].g;
				( ( float* ) texels )[ 2 ] = exr_texels[ j ][ i ].b;

				texels += 3 * sizeof( float );
			}

			texels += ( im->pitch( ) - im->linesize( ) ) * sizeof( float );
		}

		return im;
	}
}

il::image_type_ptr EXR_plugin::load( const fs::path& path )
{ return load_exr( path ); }

bool EXR_plugin::store( const fs::path& /*path*/, const il::image_type_ptr& /*im*/)
{ return false; }

} } } }
