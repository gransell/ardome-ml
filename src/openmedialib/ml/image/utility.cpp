/*
* ml - A media library representation.
* Copyright (C) 2013 Vizrt
* Released under the LGPL.
*/
#include <openmedialib/ml/utilities.hpp>
//#include <openmedialib/ml/image/utility.hpp>
//#include <openmedialib/ml/image/image_interface.hpp>
#include <openmedialib/ml/image/image_types.hpp>
#include <openmedialib/ml/image/image_interface.hpp>


#include <openmedialib/ml/image/ffmpeg_image.hpp>
#include <openmedialib/ml/image/ffmpeg_utility.hpp>

//#include <openmedialib/ml/types.hpp>
#include <boost/assign/list_of.hpp>
#include <map>


namespace cl = olib::opencorelib;
namespace ml = olib::openmedialib::ml;
namespace image = olib::openmedialib::ml::image;

namespace olib { namespace openmedialib { namespace ml { namespace image {

static int image_depth ( MLPixelFormat pf ) {
	
	if ( pf == ML_PIX_FMT_YUV422P10LE )
		return 10;


	return 8;
}

ML_DECLSPEC image_type_ptr allocate ( MLPixelFormat pf, int width, int height )
{
	if ( image_depth( pf ) == 8 )
		return ml::image::image_type_8_ptr( new ml::image::image_type_8( pf, width, height ) );
	else if ( image_depth( pf ) == 10 )
		return ml::image::image_type_16_ptr( new ml::image::image_type_16( pf, width, height ) );
	

	return image_type_ptr( );	
}

ML_DECLSPEC image_type_ptr allocate ( const olib::t_string pf, int width, int height )
{
	return ml::image::allocate( MLPixelFormatMap[ pf ], width, height );
}

ML_DECLSPEC image_type_ptr convert( const image_type_ptr &src, const MLPixelFormat pf )
{
	image_type_ptr dst = allocate( pf, src->width( ), src->height( ) );
	ml::image::convert_ffmpeg_image( src, dst );
	return dst;
}

inline unsigned char clamp_sample( const int v ) { return ( unsigned char )( v < 0 ? 0 : v > 255 ? 255 : v ); }

inline void yuv444_to_rgb( unsigned char *&dst, const int y, const int rc, const int gc, const int bc )
{
    *dst ++ = clamp_sample( ( y + rc ) >> 10 );
    *dst ++ = clamp_sample( ( y - gc ) >> 10 );
    *dst ++ = clamp_sample( ( y + bc ) >> 10 );
}

inline void yuv444_to_bgr( unsigned char *&dst, const int y, const int rc, const int gc, const int bc )
{
    *dst ++ = clamp_sample( ( y + bc ) >> 10 );
    *dst ++ = clamp_sample( ( y - gc ) >> 10 );
    *dst ++ = clamp_sample( ( y + rc ) >> 10 );
}



} } } }
