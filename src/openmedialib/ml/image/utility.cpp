/*
* ml - A media library representation.
* Copyright (C) 2013 Vizrt
* Released under the LGPL.
*/
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/image/image_types.hpp>
#include <openmedialib/ml/image/image_interface.hpp>

#include <openmedialib/ml/image/ffmpeg_image.hpp>
#include <openmedialib/ml/image/ffmpeg_utility.hpp>

#include <boost/assign/list_of.hpp>
#include <map>


namespace cl = olib::opencorelib;
namespace ml = olib::openmedialib::ml;
namespace image = olib::openmedialib::ml::image;

namespace olib { namespace openmedialib { namespace ml { namespace image {

ML_DECLSPEC int image_depth ( MLPixelFormat pf ) 
{
	//XXX Pixfmts with different bitdepth for different components do exist
	return utility_bitdepth( ML_to_AV( pf ), 0 ); 
}

ML_DECLSPEC image_type_ptr allocate ( MLPixelFormat pf, int width, int height )
{
    ARENFORCE_MSG( pf > ML_PIX_FMT_NONE && pf < ML_PIX_FMT_NB, "Invalid picture format")( pf );
	if ( image_depth( pf ) == 8 )
		return ml::image::image_type_8_ptr( new ml::image::image_type_8( pf, width, height ) );
	else if ( image_depth( pf ) > 8 )
		return ml::image::image_type_16_ptr( new ml::image::image_type_16( pf, width, height ) );
	return image_type_ptr( );	
}

ML_DECLSPEC image_type_ptr allocate ( const olib::t_string pf, int width, int height )
{
	MLPixelFormatMap_type::const_iterator i = MLPixelFormatMap.find( pf );
	if ( i == MLPixelFormatMap.end() )
		ARENFORCE_MSG( false, "Invalid picture format");
	return ml::image::allocate( i->second, width, height );
}

ML_DECLSPEC image_type_ptr allocate ( const image_type_ptr &img )
{
	return ml::image::allocate( img->ml_pixel_format( ), img->width( ), img->height( ) );
}

ML_DECLSPEC image_type_ptr convert( const image_type_ptr &src, const MLPixelFormat pf )
{
	image_type_ptr dst = allocate( pf, src->width( ), src->height( ) );
	dst->set_field_order( src->field_order( ) );
	ml::image::convert_ffmpeg_image( src, dst );
	return dst;
}

ML_DECLSPEC image_type_ptr convert( const image_type_ptr &src, const olib::t_string& pf )
{
	MLPixelFormatMap_type::const_iterator i = MLPixelFormatMap.find( pf );
	if ( i == MLPixelFormatMap.end() )
		return image_type_ptr( );
	return convert( src,  i->second );
}

image_type_ptr rescale( const image_type_ptr &im, int new_w, int new_h, rescale_filter filter )
{
    if( im->width( ) == new_w && im->height( ) == new_h )
        return im;

    image_type_ptr new_im = allocate( im->ml_pixel_format( ), new_w, new_h );
    ml::image::rescale_ffmpeg_image( im, new_im, filter ); 
    return new_im;
}

static int locate_alpha_offset( const MLPixelFormat pf )
{
    if ( !is_pixfmt_alpha( pf ) || is_pixfmt_planar( pf ) ) return -1; // Only RGB with alpha supported

    return utility_offset( ML_to_AV( pf ), utility_nb_components( ML_to_AV( pf ) ) - 1 ); //Alpha is always last component
}

template< typename T, enum MLPixelFormat alpha >
ML_DECLSPEC image_type_ptr extract_alpha( const image_type_ptr &im )
{
    image_type_ptr result;

    if ( im )
    {
        boost::shared_ptr< T > im_type = ml::image::coerce< T >( im );
        int offset = locate_alpha_offset( im->ml_pixel_format( ) );
        if ( offset >= 0 )
        {
            result = allocate( alpha, im->width( ), im->height( ) );
            boost::shared_ptr< T > result_type = ml::image::coerce< T >( result );

            const typename T::data_type *src = im_type->data( );
            typename T::data_type *dst = result_type->data( );

            int h = im->height( );

            int src_rem = im->pitch( ) - im->linesize( );
            int dst_rem = result->pitch( ) - result->linesize( );

			const int max_input = ( 1 << im->bitdepth( ) ) - 1;
			const int max_output = ( 1 << result->bitdepth( ) ) - 1;

            while( h -- )
            {
                int w = im->width( );

                while ( w -- )
                {
                    *dst ++ = ( typename T::data_type )( float( src[ offset ] / max_input ) * max_output );
                    src += 4;
                }

                src += src_rem;
                dst += dst_rem;
            }
        }
    }

    return result;
}

ML_DECLSPEC image_type_ptr extract_alpha( const image_type_ptr &im )
{
    image_type_ptr result;
    if ( ml::image::coerce< ml::image::image_type_8 >( im ) )
        result = extract_alpha< ml::image::image_type_8, ML_PIX_FMT_L8 >( im );
    else if ( ml::image::coerce< ml::image::image_type_16 >( im ) )
        result = extract_alpha< ml::image::image_type_16, ML_PIX_FMT_L16LE >( im );
	return result;
}

template< typename T >
ML_DECLSPEC image_type_ptr deinterlace( const image_type_ptr &im )
{
	ARENFORCE_MSG( !is_pixfmt_alpha( ML_to_AV( im->ml_pixel_format( ) ) ), "Image with alpha not supported to deinterlace" )( im->pf( ) );

	if ( im->field_order( ) != progressive )
	{
		im->set_field_order( progressive );
		boost::shared_ptr< T > im_type = ml::image::coerce< T >( im );
		ARENFORCE_MSG( im_type, "Unable to coerce to image type associated to %d" )( im->bitdepth( ) );
		for ( int i = 0; i < im->plane_count( ); i ++ )
		{
			typename T::data_type *dst = im_type->data( i );
			typename T::data_type *src = im_type->data( i ) + im->pitch( i ) / sizeof( typename T::data_type );
			int linesize = im->linesize( i ) / sizeof( typename T::data_type );
			int src_pitch = ( im->pitch( i ) / sizeof( typename T::data_type ) ) - linesize;
			int height = im->height( i ) - 1;
			
			for (int h = height - 1; h >= 0; h -- )
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

	return im;
}

ML_DECLSPEC image_type_ptr deinterlace( const image_type_ptr &im )
{
	image_type_ptr result;
	if ( ml::image::coerce< ml::image::image_type_8 >( im ) )
		result = deinterlace< ml::image::image_type_8 >( im );
	else if ( ml::image::coerce< ml::image::image_type_16 >( im ) )
		result = deinterlace< ml::image::image_type_16 >( im );
	return result;
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
template< typename T >
ML_DECLSPEC image_type_ptr field( const image_type_ptr &im, int field )
{
	// Default our return to the source
	image_type_ptr new_im = im;

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
			boost::shared_ptr< T > im_type = ml::image::coerce< T >( im );
			boost::shared_ptr< T > new_im_type = ml::image::coerce< T >( new_im );
			typename T::data_type *src = im_type->data( i ) + field * im->pitch( i ) / sizeof( typename T::data_type );
			typename T::data_type *dst = new_im_type->data( i );
			int src_pitch = 2 * im->pitch( i ) / sizeof( typename T::data_type );
			int dst_pitch = new_im->pitch( i ) / sizeof( typename T::data_type );
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

	return new_im;
}

ML_DECLSPEC image_type_ptr field( const image_type_ptr &im, int f )
{
	image_type_ptr result;
	if ( ml::image::coerce< ml::image::image_type_8 >( im ) )
		result = field< ml::image::image_type_8 >( im, f );
	else if ( ml::image::coerce< ml::image::image_type_16 >( im ) )
		result = field< ml::image::image_type_16 >( im, f );
	return result;
}

ML_DECLSPEC bool is_pixfmt_planar(  MLPixelFormat pf ) 
{
	return is_pixfmt_planar( ML_to_AV( pf ) );
}

ML_DECLSPEC bool is_pixfmt_rgb(  MLPixelFormat pf ) 
{
	return is_pixfmt_rgb( ML_to_AV( pf ) );
}

ML_DECLSPEC bool is_pixfmt_alpha(  MLPixelFormat pf ) 
{
	return is_pixfmt_alpha( ML_to_AV( pf ) );
}

ML_DECLSPEC int order_of_component( MLPixelFormat pf, int index ) 
{
	//utility_offset returns bytes, but we want the order as an index
	return utility_offset( ML_to_AV( pf ), index ) / ( ( image_depth( pf ) + 7 ) / 8 );
}

} } } }
