#include "ffmpeg_utility.hpp"

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/image/image_types.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
#include <libavformat/avio.h>
#include <libavutil/mem.h>
#include <libavutil/imgutils.h>
}

namespace olib { namespace openmedialib { namespace ml { namespace image {

UtilityAVPixFmtDescriptor *utility_av_pix_fmt_desc_get( int pixfmt )
    
{   
    AVPixelFormat AVpf = static_cast<AVPixelFormat>(pixfmt);
    return (UtilityAVPixFmtDescriptor*)av_pix_fmt_desc_get( AVpf ); 
}

int utility_avpicture_get_size( int pixfmt, int width, int height )
{ return (int)avpicture_get_size( AVPixelFormat(pixfmt), width, height ); }

void utility_av_pix_fmt_get_chroma_sub_sample( int pixfmt, int *chroma_w, int *chroma_h )
{ av_pix_fmt_get_chroma_sub_sample( AVPixelFormat(pixfmt), chroma_w, chroma_h); }

int utility_av_image_get_linesize( int pixfmt, int width, int plane )
{ return (int)av_image_get_linesize( AVPixelFormat(pixfmt), width, plane ); }

void *utility_av_malloc( int bytes )
{ return av_malloc( bytes + 8 ); }

void utility_av_free( void *buf )
{ return av_free( buf ); }


int utility_plane_count( int pixfmt )
{
    //return av_pix_fmt_count_planes( AVpixfmt_ );
    const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get( static_cast<AVPixelFormat>(pixfmt) ); 
    int i, planes[4] = { 0 }, ret = 0;

    for (i = 0; i < desc->nb_components; i++) {
        planes[desc->comp[i].plane] = 1;
    }
    for (i = 0; i < FF_ARRAY_ELEMS(planes); i++) {
        ret += planes[i];
    }
    return ret;
}

int utility_bitdepth( int pixfmt, int index )
{
    const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get( static_cast<AVPixelFormat>(pixfmt) );
    return desc->comp[index].depth_minus1 + 1;
}

int utility_nb_components( int pixfmt )
{
    const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get( static_cast<AVPixelFormat>(pixfmt) );
    return desc->nb_components;
}

int ML_to_AV( MLPixelFormat pixfmt )
{
	if (pixfmt == ML_PIX_FMT_YUV420P )
		return (int)AV_PIX_FMT_YUV420P;
    else if (pixfmt == ML_PIX_FMT_YUV420P10 )
		return (int)AV_PIX_FMT_YUV420P10;
    else if (pixfmt == ML_PIX_FMT_YUV420P16 )
		return (int)AV_PIX_FMT_YUV420P16;
	else if (pixfmt == ML_PIX_FMT_YUVA420P )
		return (int)AV_PIX_FMT_YUVA420P;
	else if (pixfmt == ML_PIX_FMT_YUV422P )
		return (int)AV_PIX_FMT_YUV422P;
	else if (pixfmt == ML_PIX_FMT_YUV422P10LE )
		return (int)AV_PIX_FMT_YUV422P10LE;
	else if (pixfmt == ML_PIX_FMT_YUV422P10 )
		return (int)AV_PIX_FMT_YUV422P10;
	else if (pixfmt == ML_PIX_FMT_YUV422P16 )
		return (int)AV_PIX_FMT_YUV422P16;
	else if (pixfmt == ML_PIX_FMT_YUV422 )
		return (int)AV_PIX_FMT_YUYV422;
	else if (pixfmt == ML_PIX_FMT_YUV444P )
		return (int)AV_PIX_FMT_YUV444P;
	else if (pixfmt == ML_PIX_FMT_YUVA444P )
		return (int)AV_PIX_FMT_YUVA444P;
	else if (pixfmt == ML_PIX_FMT_UYYVYY411 )
		return (int)AV_PIX_FMT_UYYVYY411;
	else if (pixfmt == ML_PIX_FMT_L8 )
		return (int)AV_PIX_FMT_GRAY8;
	else if (pixfmt == ML_PIX_FMT_L8A8 )
		return (int)AV_PIX_FMT_GRAY8A;
	else if (pixfmt == ML_PIX_FMT_L16 )
		return (int)AV_PIX_FMT_GRAY16;
	else if (pixfmt == ML_PIX_FMT_R8G8B8 )
		return (int)AV_PIX_FMT_RGB24;
	else if (pixfmt == ML_PIX_FMT_R8G8B8A8 )
		return (int)AV_PIX_FMT_RGBA;
	else if (pixfmt == ML_PIX_FMT_R10G10B10 )
		return (int)AV_PIX_FMT_RGB48;

	return (int)AV_PIX_FMT_NONE;
}

bool is_pixfmt_rgb( int pixfmt )
{
    const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get( static_cast<AVPixelFormat>(pixfmt) );
    return desc->flags & PIX_FMT_RGB ? true : false;
}

bool is_pixfmt_planar( int pixfmt )
{
    const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get( static_cast<AVPixelFormat>(pixfmt) );
    return desc->flags & PIX_FMT_PLANAR ? true : false;
}

template<typename T>
AVPicture fill_picture( ml::image_type_ptr image )
{
    AVPicture picture;

    boost::shared_ptr< T > image_type = ml::image::coerce< T >( image );
    for ( int i = 0; i < AV_NUM_DATA_POINTERS; i ++ )
    {
        if ( i < image->plane_count( ) )
        {
            picture.data[ i ] = (boost::uint8_t*)image_type->data( i );
            
            picture.linesize[ i ] = image->linesize( i );
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

    AVPicture input;
    AVPicture output;
    if (src->bitdepth( ) > 8 ) 
	    input = fill_picture< ml::image::image_type_16 >( src );	
    else
	    input = fill_picture< ml::image::image_type_8 >( src );	
       
 
    if (dst->bitdepth( ) > 8 ) 
	    output = fill_picture< ml::image::image_type_16 >( dst );	
    else
	    output = fill_picture< ml::image::image_type_8 >( dst );	
	
	ARENFORCE( sws_isSupportedInput( static_cast<AVPixelFormat>( ML_to_AV( src->ml_pixel_format( ) ) ) ) );
	ARENFORCE( sws_isSupportedOutput( static_cast<AVPixelFormat>( ML_to_AV( dst->ml_pixel_format( ) ) ) ) );
	
	struct SwsContext *img_convert;
	img_convert = sws_getContext( src->width( ), 
			src->height( ), 
			static_cast<AVPixelFormat>( ML_to_AV( src->ml_pixel_format( ) ) ),
			dst->width( ),
			dst->height( ),
			static_cast<AVPixelFormat>( ML_to_AV( dst->ml_pixel_format( ) ) ),
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
	
	std::cerr << "dst " << dst_height << " " << dst->width( ) << "x" << dst->height( ) << " " << " src " << src->width( ) << "x" << src->height( ) << std::endl;

	ARENFORCE ( dst_height == src->height( ) );
}

ML_DECLSPEC void rescale_ffmpeg_image( ml::image_type_ptr src, ml::image_type_ptr dst )
{
    AVPicture input = fill_picture< ml::image::image_type_8 >( src );
    AVPicture output = fill_picture< ml::image::image_type_8 >( dst );

    ARENFORCE( sws_isSupportedInput( static_cast<AVPixelFormat>( ML_to_AV( src->ml_pixel_format( ) ) ) ) );
    ARENFORCE( sws_isSupportedOutput( static_cast<AVPixelFormat>( ML_to_AV( dst->ml_pixel_format( ) ) ) ) );


    struct SwsContext *img_convert;
    img_convert = sws_getContext( src->width( ),
            src->height( ),
            static_cast<AVPixelFormat>( ML_to_AV( src->ml_pixel_format( ) ) ),
            dst->width( ),
            dst->height( ),
            static_cast<AVPixelFormat>( ML_to_AV( dst->ml_pixel_format( ) ) ),
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
}

} } } }

