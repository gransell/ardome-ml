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

ML_DECLSPEC image_type_ptr allocate ( const image_type_ptr &img )
{
	return ml::image::allocate( img->ml_pixel_format( ), img->width( ), img->height( ) );
}

ML_DECLSPEC image_type_ptr convert( const image_type_ptr &src, const MLPixelFormat pf )
{
	image_type_ptr dst = allocate( pf, src->width( ), src->height( ) );
	ml::image::convert_ffmpeg_image( src, dst );
	return dst;
}


ML_DECLSPEC image_type_ptr convert( const image_type_ptr &src, const olib::t_string pf )
{
	return convert( src,  MLPixelFormatMap[ pf ] );
}


image_type_ptr rescale( const image_type_ptr &im, int new_w, int new_h, int new_d, rescale_filter filter )
{
    return im;
}


ML_DECLSPEC image_type_ptr extract_alpha( const image_type_ptr &im )
{
    return im;
}


ML_DECLSPEC image_type_ptr deinterlace( const image_type_ptr &im )
{
/*
    if ( im && im->field_order( ) != progressive )
    {
        im->set_field_order( progressive );
        for ( int i = 0; i < im->plane_count( ); i ++ )
        {
            unsigned char *dst = im->data( i );
            unsigned char *src = im->data( i ) + im->pitch( i );
            int linesize = im->linesize( i );
            int src_pitch = im->pitch( i ) - linesize;
            int height = im->height( i ) - 1;
            while( height -- )
            {
                for ( int w = 0; w < linesize; w ++ )
                {
                    *dst = ( *src ++ + *dst ) >> 1;
                    dst ++;
                }
                dst += src_pitch;
                src += src_pitch;
            }
        }
    }
*/
    return im;
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

// Obtain the fields in presentation order
ML_DECLSPEC image_type_ptr field( const image_type_ptr &im, int field )
{
    // Default our return to the source
    image_type_ptr new_im = im;

/*
    // Only do something if we have an image and it isn't progressive
    if ( im && im->field_order( ) != progressive )
    {
        // Allocate an image which is half the size of the source
        new_im = allocate( im->pf( ), im->width( ), im->height( ) / 2 );

        // The apps field 0 depends on the field order of the image
        if ( im->field_order( ) == top_field_first )
            field = field == 0 ? 0 : 1;
        else
            field = field == 1 ? 0 : 1;

        // Copy every second scan line from each plane to extract the field
        for ( int i = 0; i < im->plane_count( ); i ++ )
        {
            unsigned char *src = im->data( i ) + field * im->pitch( i );
            unsigned char *dst = new_im->data( i );
            int src_pitch = 2 * im->pitch( i );
            int dst_pitch = new_im->pitch( i );
            int linesize = new_im->linesize( i );
            int height = new_im->height( i );
            while( height -- )
            {
                memcpy( dst, src, linesize );
                dst += dst_pitch;
                src += src_pitch;
            }
        }
    }
*/
    return new_im;
}


} } } }
