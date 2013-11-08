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

enum ML_DECLSPEC correction { nearest, floor, ceil };

extern ML_DECLSPEC void correct( MLPixelFormat pf, int &width, int &height, enum correction type = nearest );
extern ML_DECLSPEC void correct( const olib::t_string pf, int &width, int &height, enum correction type = nearest );

extern ML_DECLSPEC bool verify( MLPixelFormat pf, int width, int height );
extern ML_DECLSPEC bool verify( const olib::t_string pf, int width, int height );

inline int alignment( )
{
	return 32;
}

inline image_type_ptr conform( image_type_ptr image, int flags )
{
	if ( image && !image->matching( flags ) )
	    image = ml::image_type_ptr( static_cast<ml::image::image*>( image->clone( flags ) ) );
	return image;
}

extern ML_DECLSPEC const olib::t_string& MLPF_to_string( MLPixelFormat pf ) ;
extern ML_DECLSPEC MLPixelFormat string_to_MLPF( const olib::t_string& pf ) ;
extern ML_DECLSPEC int image_depth ( MLPixelFormat pf );
extern ML_DECLSPEC image_type_ptr allocate ( MLPixelFormat pf, int width, int height );
extern ML_DECLSPEC image_type_ptr allocate ( const olib::t_string& pf, int width, int height );
extern ML_DECLSPEC image_type_ptr allocate ( const image_type_ptr &img );

extern ML_DECLSPEC image_type_ptr convert( const image_type_ptr &src, MLPixelFormat pf );
extern ML_DECLSPEC image_type_ptr convert( const image_type_ptr &src, const olib::t_string& pf );
extern ML_DECLSPEC image_type_ptr convert( ml::rescale_object_ptr ro, const image_type_ptr &src, const olib::t_string pf );

enum ML_DECLSPEC rescale_filter { POINT_SAMPLING = 0x10, BILINEAR_SAMPLING = 2, BICUBIC_SAMPLING = 4 };
enum ML_DECLSPEC rescale_mode { MODE_FILL, MODE_SMART, MODE_LETTER, MODE_PILLAR, MODE_NATIVE };

namespace{
typedef std::map<std::wstring, rescale_mode> rescale_mode_map_type; 
rescale_mode_map_type rescale_mode_map = boost::assign::map_list_of
(L"fill",		MODE_FILL)
(L"smart",		MODE_SMART)
(L"letter",		MODE_LETTER)
(L"pillar",		MODE_PILLAR)
(L"native",		MODE_NATIVE);
}

struct geometry 
{
	geometry( )
	: interp( BILINEAR_SAMPLING )
	, mode( MODE_FILL )
	, pf( ml::image::ML_PIX_FMT_NONE )
	, field_order( ml::image::progressive )
	, width( -1 )
	, height( -1 )
	, sar_num( -1 )
	, sar_den( -1 )
	, r( 0 )
	, g( 0 )
	, b( 0 )
	, a( 0 )
	, cropped( false )
	, x( 0 )
	, y( 0 )
	, w( 0 )
	, h( 0 )
	, cx( 0 )
	, cy( 0 )
	, cw( 0 )
	, ch( 0 )
	{ }

	geometry( ml::image_type_ptr im )
	: interp( BILINEAR_SAMPLING )
	, mode( MODE_FILL )
	, pf( im->ml_pixel_format( ) )
	, field_order( im->field_order( ) )
	, width( im->width( ) )
	, height( im->height( ) )
	, sar_num( im->get_sar_num( ) )
	, sar_den( im->get_sar_den( ) )
	, r( 0 )
	, g( 0 )
	, b( 0 )
	, a( 0 )
	, cropped( false )
	, x( 0 )
	, y( 0 )
	, w( 0 )
	, h( 0 )
	, cx( 0 )
	, cy( 0 )
	, cw( 0 )
	, ch( 0 )
	{ }

	// Specifies the interpolation filter
	int interp;

	// Specifies the mode
	rescale_mode mode;

	// Specifies the picture format of the full image
	ml::image::MLPixelFormat pf;

	// Specifies the field order of the output
	ml::image::field_order_flags field_order;

	// Specifies the dimensions of the full image
	int width, height, sar_num, sar_den;

	// Specifies the colour of the border as an 8 bit rgba
	int r, g, b, a;

	// Everything below is calculated by the 'calculate' function

	// Indicates if the input image should be cropped
	bool cropped; 

	// Specifies the geometry of the scaled image
	int x, y, w, h;

	// Specifes the geometry of the crop
	int cx, cy, cw, ch; 
};

ML_DECLSPEC image_type_ptr rescale( ml::rescale_object_ptr ro, const image_type_ptr &im, int new_w, int new_h, rescale_filter filter = POINT_SAMPLING );
ML_DECLSPEC image_type_ptr rescale( const image_type_ptr &im, int new_w, int new_h, rescale_filter filter = POINT_SAMPLING );
ML_DECLSPEC image_type_ptr rescale_and_convert( ml::rescale_object_ptr ro, const ml::image_type_ptr &im, geometry &shape );

ML_DECLSPEC void calculate( geometry &shape, image_type_ptr src );

ML_DECLSPEC image_type_ptr extract_alpha( const image_type_ptr &im );

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

ML_DECLSPEC bool is_pixfmt_planar( MLPixelFormat pf );
ML_DECLSPEC bool is_pixfmt_rgb( MLPixelFormat pf );
ML_DECLSPEC bool pixfmt_has_alpha( MLPixelFormat pf );
ML_DECLSPEC int order_of_component( MLPixelFormat pf, int index );

} } } }

#endif
