/*
* ml - A media library representation.
* Copyright (C) 2013 Vizrt
* Released under the LGPL.
*/
#ifndef AML_IMAGE_UTILITIES_H_
#define AML_IMAGE_UTILITIES_H_

#include <openmedialib/ml/types.hpp>
#include <openmedialib/ml/image/image_interface.hpp>
namespace olib { namespace openmedialib { namespace ml { namespace image {


inline image_type_ptr conform( image_type_ptr image, int flags )
{
	if ( image && !image->matching( flags ) )
	    image = ml::image_type_ptr( static_cast<ml::image::image*>( image->clone( flags ) ) );
	return image;
}

extern ML_DECLSPEC int image_depth ( MLPixelFormat pf );
extern ML_DECLSPEC image_type_ptr allocate ( MLPixelFormat pf, int width, int height );
extern ML_DECLSPEC image_type_ptr allocate ( const olib::t_string pf, int width, int height );
extern ML_DECLSPEC image_type_ptr allocate ( const image_type_ptr &img );

extern ML_DECLSPEC image_type_ptr convert( const image_type_ptr &src, const MLPixelFormat pf );
extern ML_DECLSPEC image_type_ptr convert( const image_type_ptr &src, const olib::t_string pf );


enum ML_DECLSPEC rescale_filter { POINT_SAMPLING = 0x10, BILINEAR_SAMPLING = 2, BICUBIC_SAMPLING = 4 };

ML_DECLSPEC image_type_ptr extract_alpha( const image_type_ptr &im );
ML_DECLSPEC image_type_ptr rescale( const image_type_ptr &im, int new_w, int new_h, rescale_filter filter = POINT_SAMPLING );

ML_DECLSPEC image_type_ptr deinterlace( const image_type_ptr &im );

inline void yuv444_to_rgb24( int &r, int &g, int &b, unsigned char y, unsigned char u, unsigned char v )
{
    r = ( ( 1192 * ( y - 16 ) + 1634 * ( v - 128 ) ) >> 10 );
    g = ( ( 1192 * ( y - 16 ) - 832 * ( v - 128 ) - 400 * ( u - 128 ) ) >> 10 );
    b = ( ( 1192 * ( y - 16 ) + 2066 * ( u - 128 ) ) >> 10 );
    r = r < 0 ? 0 : r > 255 ? 255 : r;
    g = g < 0 ? 0 : g > 255 ? 255 : g;
    b = b < 0 ? 0 : b > 255 ? 255 : b;
}

inline void rgb24_to_yuv444( int &y, int &u, int &v, unsigned char r, unsigned char g, unsigned char b )
{
    y = ( ( 263 * r + 516 * g + 100 * b ) >> 10 ) + 16;
    u = ( ( -152 * r - 298 * g + 450 * b ) >> 10 ) + 128;
    v = ( ( 450 * r - 377 * g - 73 * b ) >> 10 ) + 128;
}

ML_DECLSPEC image_type_ptr field( const image_type_ptr &im, int field );

} } } }

#endif
