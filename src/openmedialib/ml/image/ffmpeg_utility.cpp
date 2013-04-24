#ifndef FFMPEG_UTILITY_H_
#define FFMPEG_UTILITY_H_

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/image/image_types.hpp>
#include <opencorelib/cl/enforce.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
}

namespace olib { namespace openmedialib { namespace ml { namespace image {

AVPixelFormat ML_to_AV( MLPixelFormat pixfmt )
{
	if (pixfmt == ML_PIX_FMT_YUV420P )
		return AV_PIX_FMT_YUV420P;
	else if (pixfmt == ML_PIX_FMT_YUVA420P )
		return AV_PIX_FMT_YUVA420P;
	else if (pixfmt == ML_PIX_FMT_YUV422P )
		return AV_PIX_FMT_YUV422P;
	else if (pixfmt == ML_PIX_FMT_R8G8B8 )
		return AV_PIX_FMT_RGB24;
	else if (pixfmt == ML_PIX_FMT_R10G10B10 )
		return AV_PIX_FMT_RGB48;

	return AV_PIX_FMT_NONE;
}


// Fill an AVPicture with a potentially cropped aml image
AVPicture fill_picture( ml::image_type_ptr image )
{
    AVPicture picture;

    for ( int i = 0; i < AV_NUM_DATA_POINTERS; i ++ )
    {
        if ( i < image->plane_count( ) )
        {
            picture.data[ i ] = image->data( i );
            picture.linesize[ i ] = image->pitch( i );
        }
        else
        {
            picture.data[ i ] = 0;
            picture.linesize[ i ] = 0;
        }
    }

    return picture;
}

ML_DECLSPEC void convert_ffmpeg_image( ml::image_type_ptr src, ml::image_type_ptr dst)
{
	AVPicture input = fill_picture( src );	
	AVPicture output = fill_picture( dst );	
	
	ARENFORCE( sws_isSupportedInput( ML_to_AV( src->ml_pixel_format( ) ) ) );
	ARENFORCE( sws_isSupportedOutput( ML_to_AV( dst->ml_pixel_format( ) ) ) );
	
	struct SwsContext *img_convert;
	img_convert = sws_getContext( src->width( ), 
			src->height( ), 
			ML_to_AV( src->ml_pixel_format( ) ),
			dst->width( ),
			dst->height( ),
			ML_to_AV( dst->ml_pixel_format( ) ),
			SWS_BICUBIC,
			NULL,
			NULL,
			NULL);
				
	ARENFORCE( img_convert != NULL );

	int dst_height = sws_scale( img_convert, 
		input.data, 
		input.linesize, 
		0, 
		src->height( ), 
		output.data, 
		output.linesize );
	
	ARENFORCE ( dst_height == src->height( ) );
}

} } } }


#endif
