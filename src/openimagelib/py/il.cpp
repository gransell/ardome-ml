/* -*- tab-width: 4; indent-tabs-mode: t -*- */ 

// il - A image library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <cstdio>

#include <openpluginlib/py/python.hpp>
#include <openimagelib/il/openimagelib_plugin.hpp>
#include <openimagelib/il/utility.hpp>
#include <openimagelib/plugins/tga/tga_plugin.hpp>
#include <openimagelib/py/py.hpp>

namespace il  = olib::openimagelib;
namespace opl = olib::openpluginlib;
namespace py  = boost::python;

namespace olib { namespace openimagelib { namespace il { namespace detail {

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( image_width_overloads, width, 0, 2 );
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( image_height_overloads, height, 0, 2 );
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( image_pitch_overloads, pitch, 0, 2 );
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( image_offset_overloads, offset, 0, 2 );
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( image_linesize_overloads, linesize, 0, 2 );

static std::string to_string( il::image_type_ptr image )
{
	std::string result;

	if ( image )
	{
		int p;

		// Calculate byte size for the full image (all planes)
		int used = image->linesize( 0 ) * image->height( 0 );
		for ( p = 1; p < image->plane_count( ); p ++ )
			used += image->linesize( p ) * image->height( p );

		// Allocate the raster
		unsigned char *dst = new unsigned char[ used ];
		unsigned char *data = dst;

		if ( dst )
		{
			// Copy each plane into the raster
			for ( p = 0; p < image->plane_count( ); p ++ )
			{
				unsigned char *src = image->data( p );
				int height = image->height( p );

				while( height -- )
				{
					memcpy( dst, src, image->linesize( p ) );
					dst += image->linesize( p );
					src += image->pitch( p );
				}
			}

			// Construct the result
			result = std::string( ( char * )data, used );
		}

		delete [] data;
	}

	return result;
}

void py_basic_image( )
{
	py::class_<il::image_type, boost::noncopyable, il::image_type_ptr>( "image", py::no_init )
		.def( "matching", &il::image_type::matching )
		.def( "plane_count", &il::image_type::plane_count )
		.def( "width", &il::image_type::width, image_width_overloads( py::args( "index", "crop" ), "width" ) )
		.def( "height", &il::image_type::height, image_height_overloads( py::args( "index", "crop" ), "height" ) )
		.def( "pitch", &il::image_type::pitch, image_pitch_overloads( py::args( "index", "crop" ), "pitch" ) )
		.def( "offset", &il::image_type::offset, image_offset_overloads( py::args( "index", "crop" ), "offset" ) )
		.def( "linesize", &il::image_type::linesize, image_linesize_overloads( py::args( "index", "crop" ), "linesize" ) )
		.def( "get_crop_x", &il::image_type::get_crop_x )
		.def( "get_crop_y", &il::image_type::get_crop_y )
		.def( "get_crop_w", &il::image_type::get_crop_w )
		.def( "get_crop_h", &il::image_type::get_crop_h )
		.def( "data", &il::image_type::bits )
		.def( "depth", &il::image_type::depth )
		.def( "count", &il::image_type::count )
		.def( "is_flipped", &il::image_type::is_flipped )
		.def( "set_flipped", &il::image_type::set_flipped )
		.def( "is_flopped", &il::image_type::is_flopped )
		.def( "set_flopped", &il::image_type::set_flopped )
		.def( "is_writable", &il::image_type::is_writable )
		.def( "set_writable", &il::image_type::set_writable )
		.def( "pts", &il::image_type::pts )
		.def( "set_pts", &il::image_type::set_pts )
		.def( "position", &il::image_type::position )
		.def( "set_position", &il::image_type::set_position )
		.def( "field_order", &il::image_type::field_order )
		.def( "set_field_order", &il::image_type::set_field_order )
		.def( "is_cubemap", &il::image_type::is_cubemap )
		.def( "is_volume", &il::image_type::is_volume )
		.def( "pf", &il::image_type::pf )
		.def( "size", &il::image_type::size )
		.def( "crop", &il::image_type::crop )
		.def( "crop_clear", &il::image_type::crop_clear )
		.def( "is_cropped", &il::image_type::is_cropped )
		;

	py::def( "to_string", &to_string );
}

il::image_type_ptr rescale( il::image_type_ptr image, int width, int height )
{
	return il::rescale( image, width, height, 1, BILINEAR_SAMPLING );
}

il::image_type_ptr null_image()
{
	return il::image_type_ptr();
}

il::image_type_ptr allocate0( const std::wstring &pf, int width, int height ) { return il::allocate( pf, width, height ); }

void py_utility( )
{
	py::def( "conform", &il::conform );
	py::def( "convert", &il::convert );
	py::def( "allocate", &allocate0 );
	py::def( "field", &il::field );
	py::def( "deinterlace", &il::deinterlace );
	py::def( "rescale", &rescale );
	py::def( "null_image", &null_image );
}

void py_plugin( )
{
	py::def( "load_image", &il::load_image );
	py::def( "store_image", &il::store_image );
}

} } } }
