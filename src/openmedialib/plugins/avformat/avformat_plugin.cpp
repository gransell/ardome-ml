// avformat - A avformat plugin to ml.
//
// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

#include <map>

#ifdef WIN32
#	include <windows.h>
#	undef INT64_C
#	define INT64_C(x)	x ## i64
#   undef max
#	define snprintf _snprintf
#else
#	undef INT64_C
#	define INT64_C(x)	x ## LL
#endif

#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/cstdint.hpp>
#include <boost/algorithm/string.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

namespace plugin = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;
namespace ml = olib::openmedialib::ml;

namespace olib { namespace openmedialib { namespace ml { 

extern input_type_ptr ML_PLUGIN_DECLSPEC create_input_avformat( const pl::wstring & );
extern filter_type_ptr ML_PLUGIN_DECLSPEC create_avdecode( const pl::wstring & );
extern filter_type_ptr ML_PLUGIN_DECLSPEC create_avencode( const pl::wstring & );
extern filter_type_ptr ML_PLUGIN_DECLSPEC create_resampler( const pl::wstring & );
extern store_type_ptr ML_PLUGIN_DECLSPEC create_store_avformat( const pl::wstring &, const frame_type_ptr & );

const std::wstring avformat_to_oil( int fmt )
{
	if ( fmt == PIX_FMT_YUV420P )
		return L"yuv420p";
	else if ( fmt == PIX_FMT_UYYVYY411 )
		return L"yuv411";
	else if ( fmt == PIX_FMT_YUV411P )
		return L"yuv411p";
	else if ( fmt == PIX_FMT_YUYV422 )
		return L"yuv422";
	else if ( fmt == PIX_FMT_YUV422P )
		return L"yuv422p";
	else if ( fmt == PIX_FMT_RGB24 )
		return L"r8g8b8";
	else if ( fmt == PIX_FMT_BGR24 )
		return L"b8g8r8";
	else if ( fmt == PIX_FMT_ARGB )
		return L"a8r8g8b8";
	else if ( fmt == PIX_FMT_ABGR )
		return L"a8b8g8r8";
	else if ( fmt == PIX_FMT_BGRA )
		return L"b8g8r8a8";
	else if ( fmt == PIX_FMT_RGBA )
		return L"b8g8r8a8";
	else if ( fmt == PIX_FMT_RGB32 )
		return L"r8g8b8a8";
	else if ( fmt == PIX_FMT_BGR32 )
		return L"b8g8r8a8";
	else if ( fmt == PIX_FMT_RGB32_1 )
		return L"r8g8b8a8";
	else if ( fmt == PIX_FMT_BGR32_1 )
		return L"b8g8r8a8";

	// Lies, damned lies but statistically won't matter
	else if ( fmt == PIX_FMT_YUVJ420P )
		return L"yuv420p";
	else if ( fmt == PIX_FMT_YUVJ422P )
		return L"yuv422p";

	return L"";
}

std::string avformat_codec_id_to_apf_codec( CodecID codec_id )
{
	switch( codec_id )
	{
		case CODEC_ID_MPEG1VIDEO: return "http://www.ardendo.com/apf/codec/mpeg/mpeg1";
		case CODEC_ID_MPEG2VIDEO: return "http://www.ardendo.com/apf/codec/mpeg/mpeg2";
		case CODEC_ID_MPEG4: return "http://www.ardendo.com/apf/codec/mpeg/mpeg4";
		case CODEC_ID_H264: return "http://www.ardendo.com/apf/codec/h264/h264";
		case CODEC_ID_MP2: return "http://www.ardendo.com/apf/codec/mp2";
		case CODEC_ID_MP3: return "http://www.ardendo.com/apf/codec/mp3";
		case CODEC_ID_AAC: return "http://www.ardendo.com/apf/codec/aac";
		case CODEC_ID_PCM_S16LE: return "http://www.ardendo.com/apf/codec/pcm";
		case CODEC_ID_PCM_S16BE: return "http://www.ardendo.com/apf/codec/aiff";
		case CODEC_ID_PCM_U16LE: return "http://www.ardendo.com/apf/codec/pcm";
		case CODEC_ID_PCM_U16BE: return "http://www.ardendo.com/apf/codec/aiff";
		case CODEC_ID_PCM_S24LE: return "http://www.ardendo.com/apf/codec/pcm";
		case CODEC_ID_PCM_S24BE: return "http://www.ardendo.com/apf/codec/aiff";
		case CODEC_ID_PCM_U24LE: return "http://www.ardendo.com/apf/codec/pcm";
		case CODEC_ID_PCM_U24BE: return "http://www.ardendo.com/apf/codec/aiff";
		case CODEC_ID_DVVIDEO: return "http://www.ardendo.com/apf/codec/dv/dvcpro";
		case CODEC_ID_DNXHD: return "http://www.ardendo.com/apf/codec/vc3/vc3";
		default: return "";
	}

	return "";
}

CodecID stream_to_avformat_codec_id( const stream_type_ptr &stream )
{
	std::string apf_codec_id = stream->codec( );
	const std::string prefix = "http://www.ardendo.com/apf/codec/";
	ARENFORCE( boost::algorithm::starts_with( apf_codec_id, prefix ) );
	apf_codec_id = apf_codec_id.substr( prefix.size() );

	if( apf_codec_id == "mpeg/mpeg1" )
		return CODEC_ID_MPEG1VIDEO;
	else if( apf_codec_id == "mpeg/mpeg2" )
		return CODEC_ID_MPEG2VIDEO;
	else if( apf_codec_id == "h264/h264" )
		return CODEC_ID_H264;
	else if( apf_codec_id == "mp2" )
		return CODEC_ID_MP2;
	else if( apf_codec_id == "mp3" )
		return CODEC_ID_MP3;
	else if( apf_codec_id == "aac" )
		return CODEC_ID_AAC;
	else if( apf_codec_id == "dv/dv" || apf_codec_id == "dv/dv25" || apf_codec_id == "dv/dvcpro" )
		return CODEC_ID_DVVIDEO;
	else if( apf_codec_id == "vc3/vc3" )
		return CODEC_ID_DNXHD;
	else if( apf_codec_id == "pcm" )
	{
		int sample_size = stream->sample_size( );
		if( sample_size == 2 )
			return CODEC_ID_PCM_S16LE;
		else if( sample_size == 3 )
			return CODEC_ID_PCM_S24LE;
	}
	else if( apf_codec_id == "aiff" )
	{
		int sample_size = stream->sample_size( );
		if( sample_size == 2 )
			return CODEC_ID_PCM_S16BE;
		else if( sample_size == 3 )
			return CODEC_ID_PCM_S24BE;
	}
	
	return CODEC_ID_NONE;
}

const PixelFormat oil_to_avformat( const std::wstring &fmt )
{
	if ( fmt == L"yuv420p" )
		return PIX_FMT_YUV420P;
	else if ( fmt == L"yuv411" )
		return PIX_FMT_UYYVYY411;
	else if ( fmt == L"yuv411p" )
		return PIX_FMT_YUV411P;
	else if ( fmt == L"yuv422" )
		return PIX_FMT_YUYV422;
	else if ( fmt == L"yuv422p" )
		return PIX_FMT_YUV422P;
	else if ( fmt == L"r8g8b8" )
		return PIX_FMT_RGB24;
	else if ( fmt == L"b8g8r8" )
		return PIX_FMT_BGR24;
	else if ( fmt == L"r8g8b8a8" )
		return PIX_FMT_RGB32;
	return PIX_FMT_NONE;
}

il::image_type_ptr convert_to_oil( AVFrame *frame, PixelFormat pix_fmt, int width, int height )
{
	il::image_type_ptr image;
	PixelFormat dst_fmt = pix_fmt;
	std::wstring format = avformat_to_oil( pix_fmt );
	int even = width % 4 != 0 ? 4 - ( width % 4 ) : 0;
	int even_h = height % 2;

	if ( format == L"" )
	{
		format = L"b8g8r8a8";
		dst_fmt = PIX_FMT_BGRA;
	}

	AVPicture output;
	image = il::allocate( format, width + even, height + even_h );
	avpicture_fill( &output, image->data( ), dst_fmt, width + even, height + even_h );
	struct SwsContext *img_convert_ = 0;
	img_convert_ = sws_getCachedContext( img_convert_, width, height, pix_fmt, width + even, height + even_h, dst_fmt, SWS_BICUBIC, NULL, NULL, NULL );
	if ( img_convert_ != NULL )
	{
		sws_scale( img_convert_, frame->data, frame->linesize, 0, height + even_h, output.data, output.linesize );
		sws_freeContext( img_convert_ );
	}

	if ( frame->interlaced_frame )
    {
        if( height == 720 )
        {
            //HD 720 progressive
            image->set_field_order( il::progressive );
        }
        else if( height > 720 )
        {
            //HD material, set top field first
            image->set_field_order( il::top_field_first );
        }
        else
        {
            image->set_field_order( frame->top_field_first ? il::top_field_first : il::bottom_field_first );
        }
    }

    image->set_writable( false );

	return image;
}

// Lock manager
static int lockmgr(void **mtx, enum AVLockOp op)
{
	boost::mutex *mutex = static_cast< boost::mutex * >( *mtx );

	switch(op) 
	{
		case AV_LOCK_CREATE:
			*mtx = new boost::mutex( );
			if(!*mtx)
				return 1;
			return 0;
		case AV_LOCK_OBTAIN:
			if ( mutex ) mutex->lock( );
			return 0;
		case AV_LOCK_RELEASE:
			if ( mutex ) mutex->unlock( );
			return 0;
		case AV_LOCK_DESTROY:
			delete mutex;
			return 0;
	}
	return 1;
}

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC avformat_plugin : public openmedialib_plugin
{
public:

	virtual input_type_ptr input( const pl::wstring &resource )
	{
		return create_input_avformat( resource );
	}

	virtual store_type_ptr store( const pl::wstring &resource, const frame_type_ptr &frame )
	{
		return create_store_avformat( resource, frame );
	}
	
	virtual filter_type_ptr filter( const pl::wstring& resource )
	{
		if ( resource == L"avdecode" )
			return create_avdecode( resource );
		else if ( resource == L"avencode" )
			return create_avencode( resource );
		return create_resampler( resource );
	}
};

} } }

//
// Library initialisation mechanism
//

namespace
{
	void reflib( int init )
	{
		static long refs = 0;

		assert( refs >= 0 && L"avformat_plugin::refinit: refs is negative." );
		
		if( init > 0 && ++refs == 1)
		{
			avcodec_register_all();
			av_register_all( );
			avformat_network_init( );
			//for( int i = 0; i < CODEC_TYPE_NB; i ++ )
				//ml::avctx_opts[ i ]= avcodec_alloc_context2( CodecType( i ) );
			//ml::avformat_opts = avformat_alloc_context( );
			if ( getenv( "AML_AVFORMAT_DEBUG" ) == 0 )
				av_log_set_level( -1 );
			else
				av_log_set_level( atoi(  getenv( "AML_AVFORMAT_DEBUG" ) ) );

			av_lockmgr_register( ml::lockmgr );
		}
		else if( init < 0 && --refs == 0 )
		{
		}
	}
	
	boost::recursive_mutex mutex;
}

//
// Access methods for openpluginlib
//

extern "C"
{
	ML_PLUGIN_DECLSPEC bool openplugin_init( void )
	{
		boost::recursive_mutex::scoped_lock lock( mutex );
		reflib( 1 );
		return true;
	}

	ML_PLUGIN_DECLSPEC bool openplugin_uninit( void )
	{
		boost::recursive_mutex::scoped_lock lock( mutex );
		reflib( -1 );
		return true;
	}
	
	ML_PLUGIN_DECLSPEC bool openplugin_create_plugin( const char*, pl::openplugin** plug )
	{
		*plug = new plugin::avformat_plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( pl::openplugin* plug )
	{ 
		delete static_cast< plugin::avformat_plugin * >( plug ); 
	}
}

