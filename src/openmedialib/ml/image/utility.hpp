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

/// Convenience macro to evulate the max value for a given bit depth
#define ML_MAX_BIT_VALUE( b )       ( ( 1 << b ) - 1 )

/// Scale a sample from a one bit depth to another
#define ML_SCALE_SAMPLE( s, m, d )  ( boost::uint32_t( s ) * m / d )

/// Correction/validation mods
enum ML_DECLSPEC correction { nearest, floor, ceil };

/// Corrects the coordinates such that they're valid within the requested pf
extern ML_DECLSPEC void correct( MLPixelFormat pf, int &x, int &y, enum correction type = nearest );

/// Corrects the coordinates such that they're valid within the requested pf
extern ML_DECLSPEC void correct( const olib::t_string pf, int &x, int &y, enum correction type = nearest );

/// Determines if the coordinates are valid within the pf
extern ML_DECLSPEC bool verify( MLPixelFormat pf, int width, int height );

/// Determines if the coordinates are valid within the pf
extern ML_DECLSPEC bool verify( const olib::t_string pf, int x, int y );

/// Current alignment of pitches and plane offsets
inline int alignment( )
{
	return 32;
}

/// Ensures the returned image matches the requested flags (typically used to convert non-writable images to writable by way of a copy)
inline image_type_ptr conform( image_type_ptr image, int flags )
{
	if ( image && !image->matching( flags ) )
	    image = ml::image_type_ptr( static_cast<ml::image::image*>( image->clone( flags ) ) );
	return image;
}

/// Indicates at which byte offset the alpha component is stored in the alpha plane (-1 if not supported in the pf)
extern ML_DECLSPEC int locate_alpha_offset( const MLPixelFormat pf );

/// Indicates which plane holds the alpha component (-1 if not supported in the pf)
extern ML_DECLSPEC int locate_alpha_plane( const MLPixelFormat pf );

/// Convert the enumerated pf to the corresponding string value
extern ML_DECLSPEC const olib::t_string& MLPF_to_string( MLPixelFormat pf ) ;

/// Convert the string pf to the enumerated value
extern ML_DECLSPEC MLPixelFormat string_to_MLPF( const olib::t_string& pf ) ;

/// Return the bit depths of the picture format (?)
extern ML_DECLSPEC int image_depth( MLPixelFormat pf );

/// Allocate an image based of the requested  picture format and dimensions
extern ML_DECLSPEC image_type_ptr allocate ( MLPixelFormat pf, int width, int height );

/// Allocate an image based of the requested  picture format and dimensions
extern ML_DECLSPEC image_type_ptr allocate( const olib::t_string& pf, int width, int height );

/// Allocate an image based on the make up of the incoming image
extern ML_DECLSPEC image_type_ptr allocate( const image_type_ptr &img );

/// Convert the incoming image to the requested pf
extern ML_DECLSPEC image_type_ptr convert( const image_type_ptr &src, MLPixelFormat pf );

/// Convert the incoming image to the requested pf
extern ML_DECLSPEC image_type_ptr convert( const image_type_ptr &src, const olib::t_string& pf );

/// Convert the incoming image to the requested pf and cache the scaling context
extern ML_DECLSPEC image_type_ptr convert( ml::rescale_object_ptr ro, const image_type_ptr &src, const olib::t_string pf );

/// Supported scaling filters
enum ML_DECLSPEC rescale_filter { POINT_SAMPLING = 0x10, BILINEAR_SAMPLING = 2, BICUBIC_SAMPLING = 4 };

/// Support scaling modes
enum ML_DECLSPEC rescale_mode { MODE_FILL, MODE_SMART, MODE_LETTER, MODE_PILLAR, MODE_NATIVE, MODE_DISTORT };

namespace{

/// This really shouldn't be here I think?
typedef std::map<std::wstring, rescale_mode> rescale_mode_map_type; 
rescale_mode_map_type rescale_mode_map = boost::assign::map_list_of
(L"fill",		MODE_FILL)
(L"smart",		MODE_SMART)
(L"letter",		MODE_LETTER)
(L"pillar",		MODE_PILLAR)
(L"native",		MODE_NATIVE)
(L"distort",	MODE_DISTORT);
}

/// Geometry struct for complex scaling and conversion functions
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
	, src_sar_num( -1 )
	, src_sar_den( -1 )
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
	, safe( false )
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
	, src_sar_num( -1 )
	, src_sar_den( -1 )
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
	, safe( false )
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

	// Optional input sar - if invalid, sar will be derived from the image
	int src_sar_num, src_sar_den;

	// Specifies the colour of the border as an 8 bit rgba
	int r, g, b, a;

	// Everything below is calculated by the 'calculate' function

	// Indicates if the input image should be cropped
	bool cropped; 

	// Specifies the geometry of the scaled image
	int x, y, w, h;

	// Specifes the geometry of the crop
	int cx, cy, cw, ch; 

	// Indicates if the operation should be safe
	bool safe;
};

// Determines if the provided sar values are valid
inline bool valid_sar( int num, int den )
{
	return num > 0 && den > 0;
}

/// Convenience function to carry out scaling of an image with minimal arguments and cached scaling context
ML_DECLSPEC image_type_ptr rescale( ml::rescale_object_ptr ro, const image_type_ptr &im, int new_w, int new_h, rescale_filter filter = POINT_SAMPLING, int sar_num = -1, int sar_den = -1 );

/// Convenience function to carry out scaling of an image with minimal arguments
ML_DECLSPEC image_type_ptr rescale( const image_type_ptr &im, int new_w, int new_h, rescale_filter filter = POINT_SAMPLING, int sar_num = -1, int sar_den = -1 );

/// Do conversion of image according to geometry provided
ML_DECLSPEC image_type_ptr rescale_and_convert( ml::rescale_object_ptr ro, const ml::image_type_ptr &im, geometry &shape );

/// Scale im to the requested width and height, ignoring aspect ratio
ML_DECLSPEC image_type_ptr distort( ml::rescale_object_ptr ro, const image_type_ptr &im, int new_w, int new_h, rescale_filter filter );

/// Interpret the shape and image to calculate all unspecified values in the geomtery
ML_DECLSPEC void calculate( geometry &shape, image_type_ptr src );

/// Extracts an alpha component or plane as an l8 or l16 image - width/height can be used to trim scanlines or columns
ML_DECLSPEC image_type_ptr extract_alpha( const image_type_ptr &im, int width = -1, int height = -1 );

/// Merges the alpha l8 or l16 image inplace (does not copy im first)
ML_DECLSPEC image_type_ptr merge_alpha( image_type_ptr im, image_type_ptr alpha );

/// Deinterlaces the image in place (does not copy im first)
ML_DECLSPEC image_type_ptr deinterlace( const image_type_ptr &im );

/// Converts yuv to rgb values
inline void yuv444_to_rgb24( int &r, int &g, int &b, unsigned char y, unsigned char u, unsigned char v )
{
    r = ( ( 1192 * ( y - 16 ) + 1634 * ( v - 128 ) ) >> 10 );
    g = ( ( 1192 * ( y - 16 ) - 832 * ( v - 128 ) - 400 * ( u - 128 ) ) >> 10 );
    b = ( ( 1192 * ( y - 16 ) + 2066 * ( u - 128 ) ) >> 10 );
    r = r < 0 ? 0 : r > 255 ? 255 : r;
    g = g < 0 ? 0 : g > 255 ? 255 : g;
    b = b < 0 ? 0 : b > 255 ? 255 : b;
}

/// Converts rgb to yuv values
inline void rgb24_to_yuv444( int &y, int &u, int &v, unsigned char r, unsigned char g, unsigned char b )
{
    y = ( ( 263 * r + 516 * g + 100 * b ) >> 10 ) + 16;
    u = ( ( -152 * r - 298 * g + 450 * b ) >> 10 ) + 128;
    v = ( ( 450 * r - 377 * g - 73 * b ) >> 10 ) + 128;
}

/// Extract a field from an interlaced image - field is 0 or 1 and return the 1st and 2nd fields resp.
ML_DECLSPEC image_type_ptr field( const image_type_ptr &im, int field );

/// Indicates if a conversion is necessary for compositing
ML_DECLSPEC MLPixelFormat composite_pf( MLPixelFormat pf );

/// Returns true if the pf is a yuv variant
ML_DECLSPEC bool is_pixfmt_planar( MLPixelFormat pf );

/// Returns true if the pf is an rgb variant
ML_DECLSPEC bool is_pixfmt_rgb( MLPixelFormat pf );

/// Returns true if the pf has an alpha plane or component
ML_DECLSPEC bool pixfmt_has_alpha( MLPixelFormat pf );

/// Returns the sample order of the component in the pf where index indicates the component (0=r, 1=g, 2=b, 3=a)
ML_DECLSPEC int order_of_component( MLPixelFormat pf, int index );

/// Arranges the r, g, b and a samples in the order in which they should appear in the pf and scales them accordingly.
/// Returns the number of components actually used in the picture format.
ML_DECLSPEC int arrange_rgb( MLPixelFormat pf, int samples[ 4 ], int r, int g, int b, int a, int ibits = 8 );

} } } }

#endif
