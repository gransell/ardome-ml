// avformat - A avformat plugin to ml.
//
// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>

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

// libavformat codec codec context
static AVCodecContext *avctx_opts[ CODEC_TYPE_NB ];
static AVFormatContext *avformat_opts;

std::map< CodecID, std::string > codec_name_lookup_;
std::map< std::string, CodecID > name_codec_lookup_;

static void register_lookup( CodecID codec, std::string name )
{
	codec_name_lookup_[ codec ] = name;
	name_codec_lookup_[ name ] = codec;
}

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

	if ( format == L"" )
	{
		format = L"b8g8r8a8";
		dst_fmt = PIX_FMT_BGRA;
	}

	AVPicture output;
	image = il::allocate( format, width, height );
	avpicture_fill( &output, image->data( ), dst_fmt, width, height );
	struct SwsContext *img_convert_ = 0;
	img_convert_ = sws_getCachedContext( img_convert_, width, height, pix_fmt, width, height, dst_fmt, SWS_BICUBIC, NULL, NULL, NULL );
	if ( img_convert_ != NULL )
	{
		sws_scale( img_convert_, frame->data, frame->linesize, 0, height, output.data, output.linesize );
		sws_freeContext( img_convert_ );
	}

	if ( frame->interlaced_frame )
		image->set_field_order( frame->top_field_first ? il::top_field_first : il::bottom_field_first );

	return image;
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
			for( int i = 0; i < CODEC_TYPE_NB; i ++ )
				ml::avctx_opts[ i ]= avcodec_alloc_context2( CodecType( i ) );
			ml::avformat_opts = avformat_alloc_context( );
			if ( getenv( "AML_AVFORMAT_DEBUG" ) == 0 )
				av_log_set_level( -1 );
			
			ml::register_lookup( CODEC_ID_MPEG1VIDEO, "mpeg1" );
			ml::register_lookup( CODEC_ID_MPEG2VIDEO, "mpeg2" );
			ml::register_lookup( CODEC_ID_MPEG2VIDEO, "mpeg2/30" );
			ml::register_lookup( CODEC_ID_MPEG2VIDEO, "mpeg2/50" );
			ml::register_lookup( CODEC_ID_MPEG2VIDEO, "mpeg2/mpeg2hd_1080i" );
			ml::register_lookup( CODEC_ID_H264, "h264" );
			ml::register_lookup( CODEC_ID_MPEG4, "mpeg4" );
			ml::register_lookup( CODEC_ID_MP2, "mp2" );
			ml::register_lookup( CODEC_ID_MP3, "mp3" );
			ml::register_lookup( CODEC_ID_DVVIDEO, "dv" );
			ml::register_lookup( CODEC_ID_DVVIDEO, "dv25" );
			ml::register_lookup( CODEC_ID_DVVIDEO, "dv50" );
			ml::register_lookup( CODEC_ID_DVVIDEO, "dvcprohd_1080i" );
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

