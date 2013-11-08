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
    return av_pix_fmt_count_planes( static_cast<AVPixelFormat>(pixfmt) );
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

int utility_av_image_alloc( boost::uint8_t *pointers[4], int linesizes[4], int w, int h, int pix_fmt, int align )
{
	return av_image_alloc( pointers, linesizes, w, h, AVPixelFormat( pix_fmt ), align );
}

int utility_offset( int pixfmt, int index )
{
    const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get( static_cast<AVPixelFormat>(pixfmt) );
    return desc->comp[index].offset_plus1 - 1;
}

ML_DECLSPEC int ML_to_AV( MLPixelFormat pixfmt )
{
	switch ( pixfmt )
	{
		case ML_PIX_FMT_NONE:
			return (int)AV_PIX_FMT_NONE;
		case ML_PIX_FMT_YUV420P:
			return (int)AV_PIX_FMT_YUV420P;
		case ML_PIX_FMT_YUV420P10LE:
			return (int)AV_PIX_FMT_YUV420P10LE;
		case ML_PIX_FMT_YUV420P16LE:
			return (int)AV_PIX_FMT_YUV420P16LE;
		case ML_PIX_FMT_YUVA420P:
			return (int)AV_PIX_FMT_YUVA420P;
		case ML_PIX_FMT_YUV411P:
			return (int)AV_PIX_FMT_YUV411P;
		case ML_PIX_FMT_YUV422P:
			return (int)AV_PIX_FMT_YUV422P;
		case ML_PIX_FMT_YUV422P10LE:
			return (int)AV_PIX_FMT_YUV422P10LE;
		case ML_PIX_FMT_YUV422P16LE:
			return (int)AV_PIX_FMT_YUV422P16LE;
		case ML_PIX_FMT_YUV422:
			return (int)AV_PIX_FMT_YUYV422;
		case ML_PIX_FMT_UYV422:
			return (int)AV_PIX_FMT_UYVY422;
		case ML_PIX_FMT_YUV444P:
			return (int)AV_PIX_FMT_YUV444P;
		case ML_PIX_FMT_YUV444P16LE:
			return (int)AV_PIX_FMT_YUV444P16LE;
		case ML_PIX_FMT_YUVA444P:
			return (int)AV_PIX_FMT_YUVA444P;
		case ML_PIX_FMT_YUVA444P16LE:
			return (int)AV_PIX_FMT_YUVA444P16LE;
		case ML_PIX_FMT_L8:
			return (int)AV_PIX_FMT_GRAY8;
		case ML_PIX_FMT_L16LE:
			return (int)AV_PIX_FMT_GRAY16LE;
		case ML_PIX_FMT_R8G8B8:
			return (int)AV_PIX_FMT_RGB24;
		case ML_PIX_FMT_B8G8R8:
			return (int)AV_PIX_FMT_BGR24;
		case ML_PIX_FMT_R8G8B8A8:
			return (int)AV_PIX_FMT_RGBA;
		case ML_PIX_FMT_B8G8R8A8:
			return (int)AV_PIX_FMT_BGRA;
		case ML_PIX_FMT_A8B8G8R8:
			return (int)AV_PIX_FMT_ABGR;
		case ML_PIX_FMT_A8R8G8B8:
			return (int)AV_PIX_FMT_ARGB;
		case ML_PIX_FMT_R16G16B16LE:
			return (int)AV_PIX_FMT_RGB48LE;
		case ML_PIX_FMT_NB: // primarily here to generate compiler warnings on missing case:s
			ARENFORCE_MSG( false, "ML_PIX_FMT_NB passed to ML_to_AV");

	}

	return (int)AV_PIX_FMT_NONE;
}

ML_DECLSPEC MLPixelFormat AV_to_ML( int av_pixfmt )
{
	switch ( av_pixfmt )
	{
		case AV_PIX_FMT_YUV420P:
			return ML_PIX_FMT_YUV420P;
		case AV_PIX_FMT_YUV420P10LE:
			return ML_PIX_FMT_YUV420P10LE;
		case AV_PIX_FMT_YUV420P16LE:
			return ML_PIX_FMT_YUV420P16LE;
		case AV_PIX_FMT_YUVA420P:
			return ML_PIX_FMT_YUVA420P;
		case AV_PIX_FMT_YUV411P:
			return ML_PIX_FMT_YUV411P;
		case AV_PIX_FMT_YUV422P:
			return ML_PIX_FMT_YUV422P;
		case AV_PIX_FMT_YUV422P10LE:
			return ML_PIX_FMT_YUV422P10LE;
		case AV_PIX_FMT_YUV422P16LE:
			return ML_PIX_FMT_YUV422P16LE;
		case AV_PIX_FMT_YUYV422:
			return ML_PIX_FMT_YUV422;
		case AV_PIX_FMT_UYVY422:
			return ML_PIX_FMT_UYV422;
		case AV_PIX_FMT_YUV444P:
			return ML_PIX_FMT_YUV444P;
		case AV_PIX_FMT_YUV444P16LE:
			return ML_PIX_FMT_YUV444P16LE;
		case AV_PIX_FMT_YUVA444P:
			return ML_PIX_FMT_YUVA444P;
		case AV_PIX_FMT_YUVA444P16LE:
			return ML_PIX_FMT_YUVA444P16LE;
		case AV_PIX_FMT_GRAY8:
			return ML_PIX_FMT_L8;
		case AV_PIX_FMT_GRAY16LE:
			return ML_PIX_FMT_L16LE;
		case AV_PIX_FMT_RGB24:
			return ML_PIX_FMT_R8G8B8;
		case AV_PIX_FMT_BGR24:
			return ML_PIX_FMT_B8G8R8;
		case AV_PIX_FMT_RGBA:
			return ML_PIX_FMT_R8G8B8A8;
		case AV_PIX_FMT_BGRA:
			return ML_PIX_FMT_B8G8R8A8;
		case AV_PIX_FMT_ABGR:
			return ML_PIX_FMT_A8B8G8R8;
		case AV_PIX_FMT_ARGB:
			return ML_PIX_FMT_A8R8G8B8;
		case AV_PIX_FMT_RGB48LE:
			return ML_PIX_FMT_R16G16B16LE;
		default:
			return ML_PIX_FMT_NONE;

	}

	return ML_PIX_FMT_NONE;
}

bool is_pixfmt_rgb( int pixfmt )
{
    const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get( static_cast<AVPixelFormat>(pixfmt) );
    return desc->flags & AV_PIX_FMT_FLAG_RGB ? true : false;
}

bool is_pixfmt_planar( int pixfmt )
{
    const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get( static_cast<AVPixelFormat>(pixfmt) );
    return desc->flags & AV_PIX_FMT_FLAG_PLANAR ? true : false;
}

bool pixfmt_has_alpha( int pixfmt )
{
    const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get( static_cast<AVPixelFormat>(pixfmt) );
    return desc->flags & AV_PIX_FMT_FLAG_ALPHA ? true : false;
}

AVPicture fill_picture( ml::image_type_ptr image )
{
    AVPicture picture;

    for ( int i = 0; i < AV_NUM_DATA_POINTERS; i ++ )
    {
        if ( i < image->plane_count( ) )
        {
            picture.data[ i ] = ( boost::uint8_t * )image->ptr( i );
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

int rescale_and_convert_ffmpeg_image( ml::rescale_object_ptr ro, ml::image_type_ptr src, ml::image_type_ptr dst, int flags )
{
	struct SwsContext *context = 0;
	if (ro) {
		context = (struct SwsContext *)ro->get_context( dst->ml_pixel_format( ) );
	}
	
    AVPicture input = fill_picture( src );
    AVPicture output = fill_picture( dst );

    ARENFORCE( sws_isSupportedInput( static_cast<AVPixelFormat>( ML_to_AV( src->ml_pixel_format( ) ) ) ) );
    ARENFORCE( sws_isSupportedOutput( static_cast<AVPixelFormat>( ML_to_AV( dst->ml_pixel_format( ) ) ) ) );
	
    context = sws_getCachedContext( context, 
			src->width( ),
            src->height( ),
            static_cast<AVPixelFormat>( ML_to_AV( src->ml_pixel_format( ) ) ),
            dst->width( ),
            dst->height( ),
            static_cast<AVPixelFormat>( ML_to_AV( dst->ml_pixel_format( ) ) ),
            flags,
            NULL,
            NULL,
            NULL);

    ARENFORCE( context != NULL );
	
    int retval = sws_scale( context,
        input.data,
        input.linesize,
        0,
        src->height( ),
        output.data,
        output.linesize );

	// Free the temporary allocated context, not the rescale_objects context
	if (!ro) {
		sws_freeContext( context );
	} else {
		ro->set_context( dst->ml_pixel_format( ), context );
	}
	
	return retval;
}

} } } }

