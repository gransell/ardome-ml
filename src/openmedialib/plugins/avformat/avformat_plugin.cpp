/* -*- mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */
// avformat - A avformat plugin to ml.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openpluginlib/pl/utf8_utils.hpp>
#include <openpluginlib/pl/pcos/isubject.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>
#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

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

#include <iostream>
#include <cstdlib>
#include <vector>
#include <deque>
#include <map>
#include <string>
#include <cmath>
#include <cstdio>

#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/cstdint.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

}

namespace oml = olib::openmedialib;
namespace opl = olib::openpluginlib;
namespace plugin = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace pcos = olib::openpluginlib::pcos;

namespace olib { namespace openmedialib { namespace ml { 

extern void ML_PLUGIN_DECLSPEC register_dv_decoder( );

// Alternative to Julian's patch?
static const AVRational ml_av_time_base_q = { 1, AV_TIME_BASE };

// libavformat codec codec context
AVCodecContext *avctx_opts[ CODEC_TYPE_NB ];
AVFormatContext *avformat_opts;

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

class ML_PLUGIN_DECLSPEC avformat_store : public store_type
{
	public:
		avformat_store( const opl::wstring &resource, const frame_type_ptr &frame )
			: store_type( )
			, resource_( resource )
			, prop_enable_audio_( pcos::key::from_string( "enable_audio" ) )
			, prop_enable_video_( pcos::key::from_string( "enable_video" ) )
			, oc_( 0 )
			, fmt_( 0 )
			, audio_stream_( 0 )
			, video_stream_( 0 )
			, img_convert_( 0 )
			, audio_input_frame_size_( 0 )
			, prop_show_stats_( pcos::key::from_string( "show_stats" ) )
			, prop_debug_( pcos::key::from_string( "debug" ) )
			, prop_format_( pcos::key::from_string( "format" ) )
			, prop_vcodec_( pcos::key::from_string( "vcodec" ) )
			, prop_acodec_( pcos::key::from_string( "acodec" ) )
			, prop_pix_fmt_( pcos::key::from_string( "pix_fmt" ) )
			, prop_vfourcc_( pcos::key::from_string( "vfourcc" ) )
			, prop_afourcc_( pcos::key::from_string( "afourcc" ) )
			, prop_width_( pcos::key::from_string( "width" ) )
			, prop_height_( pcos::key::from_string( "height" ) )
			, prop_aspect_ratio_( pcos::key::from_string( "aspect_ratio" ) )
			, prop_fps_num_( pcos::key::from_string( "fps_num" ) )
			, prop_fps_den_( pcos::key::from_string( "fps_den" ) )
			, prop_frequency_( pcos::key::from_string( "frequency" ) )
			, prop_channels_( pcos::key::from_string( "channels" ) )
			, prop_audio_bit_rate_( pcos::key::from_string( "audio_bit_rate" ) )
			, prop_video_bit_rate_( pcos::key::from_string( "video_bit_rate" ) )
			, prop_video_bit_rate_tolerance_( pcos::key::from_string( "video_bit_rate_tolerance" ) )
			, prop_gop_size_( pcos::key::from_string( "gop_size" ) )
			, prop_b_frames_( pcos::key::from_string( "b_frames" ) )
			, prop_mb_decision_( pcos::key::from_string( "mb_decision" ) )
			, prop_qscale_( pcos::key::from_string( "qscale" ) )
			, prop_me_method_( pcos::key::from_string( "me_method" ) )
			, prop_mb_cmp_( pcos::key::from_string( "mb_cmp" ) )
			, prop_ildct_cmp_( pcos::key::from_string( "ildct_cmp" ) )
			, prop_sub_cmp_( pcos::key::from_string( "sub_cmp" ) )
			, prop_cmp_( pcos::key::from_string( "cmp" ) )
			, prop_pre_cmp_( pcos::key::from_string( "pre_cmp" ) )
			, prop_pre_me_( pcos::key::from_string( "pre_me" ) )
			, prop_lumi_mask_( pcos::key::from_string( "lumi_mask" ) )
			, prop_dark_mask_( pcos::key::from_string( "dark_mask" ) )
			, prop_scplx_mask_( pcos::key::from_string( "scplx_mask" ) )
			, prop_tcplx_mask_( pcos::key::from_string( "tcplx_mask" ) )
			, prop_p_mask_( pcos::key::from_string( "p_mask" ) )
			, prop_qns_( pcos::key::from_string( "qns" ) )
			, prop_video_qmin_( pcos::key::from_string( "video_qmin" ) )
			, prop_video_qmax_( pcos::key::from_string( "video_qmax" ) )
			, prop_video_lmin_( pcos::key::from_string( "video_lmin" ) )
			, prop_video_lmax_( pcos::key::from_string( "video_lmax" ) )
			, prop_video_mb_qmin_( pcos::key::from_string( "video_mb_qmin" ) )
			, prop_video_mb_qmax_( pcos::key::from_string( "video_mb_qmax" ) )
			, prop_video_qdiff_( pcos::key::from_string( "video_qdiff" ) )
			, prop_video_qblur_( pcos::key::from_string( "video_qblur" ) )
			, prop_video_qcomp_( pcos::key::from_string( "video_qcomp" ) )
			, prop_mux_rate_( pcos::key::from_string( "mux_rate" ) )
			, prop_mux_preload_( pcos::key::from_string( "mux_preload" ) )
			, prop_mux_delay_( pcos::key::from_string( "mux_delay" ) )
			, prop_video_packet_size_( pcos::key::from_string( "video_packet_size" ) )
			, prop_video_rc_max_rate_( pcos::key::from_string( "video_rc_max_rate" ) )
			, prop_video_rc_min_rate_( pcos::key::from_string( "video_rc_min_rate" ) )
			, prop_video_rc_buffer_size_( pcos::key::from_string( "video_rc_buffer_size" ) )
			, prop_video_rc_buffer_aggressivity_( pcos::key::from_string( "video_rc_buffer_aggresivity" ) )
			, prop_video_rc_initial_cplx_( pcos::key::from_string( "video_rc_initial_cplx" ) )
			, prop_video_i_qfactor_( pcos::key::from_string( "video_i_qfactor" ) )
			, prop_video_b_qfactor_( pcos::key::from_string( "video_b_qfactor" ) )
			, prop_video_i_qoffset_( pcos::key::from_string( "video_i_qoffset" ) )
			, prop_video_b_qoffset_( pcos::key::from_string( "video_b_qoffset" ) )
			, prop_video_intra_quant_bias_( pcos::key::from_string( "video_intra_quant_bias" ) )
			, prop_video_inter_quant_bias_( pcos::key::from_string( "video_inter_quant_bias" ) )
			, prop_dct_algo_( pcos::key::from_string( "dct_algo" ) )
			, prop_idct_algo_( pcos::key::from_string( "idct_algo" ) )
			, prop_me_threshold_( pcos::key::from_string( "me_threshold" ) )
			, prop_mb_threshold_( pcos::key::from_string( "mb_threshold" ) )
			, prop_intra_dc_precision_( pcos::key::from_string( "intra_dc_precision" ) )
			, prop_strict_( pcos::key::from_string( "strict" ) )
			, prop_error_rate_( pcos::key::from_string( "error_rate" ) )
			, prop_noise_reduction_( pcos::key::from_string( "noise_reduction" ) )
			, prop_sc_threshold_( pcos::key::from_string( "sc_threshold" ) )
			, prop_me_range_( pcos::key::from_string( "me_range" ) )
			, prop_coder_( pcos::key::from_string( "coder" ) )
			, prop_context_( pcos::key::from_string( "context" ) )
			, prop_predictor_( pcos::key::from_string( "predictor" ) )
			, prop_pass_( pcos::key::from_string( "pass" ) )
			, prop_pass_log_( pcos::key::from_string( "pass_log" ) )
			, prop_ts_index_( pcos::key::from_string( "ts_index" ) )
			, ts_context_( 0 )
			, log_file_( 0 )
			, log_( 0 )
			, frame_pos_( 0 )
			, push_count_( 0 )
			, last_audio_( -1 )
		{
			// Show stats
			properties( ).append( prop_show_stats_ = 100 );
			properties( ).append( prop_debug_ = 0 );

			// Define the audio output buffer
			audio_outbuf_size_ = 10000;
			audio_outbuf_ = ( uint8_t * )av_malloc( audio_outbuf_size_ );

			// Extract rules from the frame
			// TODO: Ensure this doesn't cause a foul up...
			properties( ).append( prop_enable_audio_ = frame->get_audio( ) != 0 ? 1 : 0 );
			properties( ).append( prop_enable_video_ = frame->get_image( ) != 0 ? 1 : 0 );

			// Extract video related info from frame
			properties( ).append( prop_width_ = 0 );
			properties( ).append( prop_height_ = 0 );
			properties( ).append( prop_aspect_ratio_ = 1.0 );
			properties( ).append( prop_fps_num_ = 25 );
			properties( ).append( prop_fps_den_ = 1 );

			if ( prop_enable_video_.value< int >( ) == 1 )
			{
				prop_width_ = frame->get_image( )->width( );
				prop_height_ = frame->get_image( )->height( );
				prop_aspect_ratio_ = frame->aspect_ratio( );
				int num;
				int den;
				frame->get_fps( num, den );
				prop_fps_num_ = num;
				prop_fps_den_ = den;
			}

			// Allocate the video output buffer 
			// TODO: Don't allocate when video is disabled
			video_outbuf_size_ = 256 * 1024 < prop_width_.value< int >( ) * prop_height_.value< int >( ) * 4 ? 
								 prop_width_.value< int >( ) * prop_height_.value< int >( ) * 4 : 256 * 1024;
			video_outbuf_ = ( uint8_t * )av_malloc( video_outbuf_size_ );

			// Extract audio related info from frame
			properties( ).append( prop_frequency_ = 0 );
			properties( ).append( prop_channels_ = 0 );

			if ( prop_enable_audio_.value< int >( ) == 1 )
			{
				prop_frequency_ = frame->get_audio( )->frequency( );
				prop_channels_ = frame->get_audio( )->channels( );
			}

			// All of these need to migrate to application accessible properties
			properties( ).append( prop_format_ = opl::wstring( L"" ) );
			properties( ).append( prop_acodec_ = opl::wstring( L"" ) );
			properties( ).append( prop_vcodec_ = opl::wstring( L"" ) );
			properties( ).append( prop_vfourcc_ = opl::wstring( L"" ) );
			properties( ).append( prop_afourcc_ = opl::wstring( L"" ) );
			properties( ).append( prop_pix_fmt_ = opl::wstring( L"" ) );

			properties( ).append( prop_audio_bit_rate_ = 128000 );
			properties( ).append( prop_video_bit_rate_ = 400000 );
			properties( ).append( prop_video_bit_rate_tolerance_ = 4000 * 1000 );
			properties( ).append( prop_gop_size_ = 12 );
			properties( ).append( prop_b_frames_ = 0 );
			properties( ).append( prop_mb_decision_ = FF_MB_DECISION_SIMPLE );
			properties( ).append( prop_qscale_ = 0.0 );
			properties( ).append( prop_me_method_ = int( ME_EPZS ) );
			properties( ).append( prop_mb_cmp_ = FF_CMP_SAD );
			properties( ).append( prop_ildct_cmp_ = FF_CMP_VSAD );
			properties( ).append( prop_sub_cmp_ = FF_CMP_SAD );
			properties( ).append( prop_cmp_ = FF_CMP_SAD );
			properties( ).append( prop_pre_cmp_ = FF_CMP_SAD );
			properties( ).append( prop_pre_me_ = 0 );
			properties( ).append( prop_lumi_mask_ = 0.0 );
			properties( ).append( prop_dark_mask_ = 0.0 );
			properties( ).append( prop_scplx_mask_ = 0.0 );
			properties( ).append( prop_tcplx_mask_ = 0.0 );
			properties( ).append( prop_p_mask_ = 0.0 );
			properties( ).append( prop_qns_ = 0 );
			properties( ).append( prop_video_qmin_ = 2 );
			properties( ).append( prop_video_qmax_ = 31 );
			properties( ).append( prop_video_lmin_ = 2 * FF_QP2LAMBDA );
			properties( ).append( prop_video_lmax_ = 31 * FF_QP2LAMBDA );
			properties( ).append( prop_video_mb_qmin_ = 2 );
			properties( ).append( prop_video_mb_qmax_ = 31 );
			properties( ).append( prop_video_qdiff_ = 3 );
			properties( ).append( prop_video_qblur_ = 0.5 );
			properties( ).append( prop_video_qcomp_ = 0.5 );
			properties( ).append( prop_mux_rate_ = 0 );
			properties( ).append( prop_mux_preload_ = 0.5 );
			properties( ).append( prop_mux_delay_ = 0.7 );
			properties( ).append( prop_video_packet_size_ = 0 );
			properties( ).append( prop_video_rc_max_rate_ = 0 );
			properties( ).append( prop_video_rc_min_rate_ = 0 );
			properties( ).append( prop_video_rc_buffer_size_ = 0 );
			properties( ).append( prop_video_rc_buffer_aggressivity_ = 1.0 );
			properties( ).append( prop_video_rc_initial_cplx_ = 0.0 );
			properties( ).append( prop_video_i_qfactor_ = -0.8 );
			properties( ).append( prop_video_b_qfactor_ = 1.25 );
			properties( ).append( prop_video_i_qoffset_ = 0.0 );
			properties( ).append( prop_video_b_qoffset_ = 1.25 );
			properties( ).append( prop_video_intra_quant_bias_ = FF_DEFAULT_QUANT_BIAS );
			properties( ).append( prop_video_inter_quant_bias_ = FF_DEFAULT_QUANT_BIAS );
			properties( ).append( prop_dct_algo_ = 0 );
			properties( ).append( prop_idct_algo_ = 0 );
			properties( ).append( prop_me_threshold_ = 0 );
			properties( ).append( prop_mb_threshold_ = 0 );
			properties( ).append( prop_intra_dc_precision_ = 8 );
			properties( ).append( prop_strict_ = 0 );
			properties( ).append( prop_error_rate_ = 0 );
			properties( ).append( prop_noise_reduction_ = 0 );
			properties( ).append( prop_sc_threshold_ = 0 );
			properties( ).append( prop_me_range_ = 1023 );
			properties( ).append( prop_coder_ = 0 );
			properties( ).append( prop_context_ = 0 );
			properties( ).append( prop_predictor_ = 0 );
			properties( ).append( prop_pass_ = 0 );
			properties( ).append( prop_pass_log_ = opl::wstring( L"oml_avformat.log" ) );
			properties( ).append( prop_ts_index_ = opl::wstring( L"" ) );
		}

		virtual ~avformat_store( )
		{
			if ( oc_ )
			{
				// Finalize encode for video
				if ( video_stream_ )
				{
					int out_size = 0;
					do {
						do_video_encode(true, &out_size);
					} while(out_size > 0);
				}

				if ( audio_stream_ )
				{
					while( process_audio( ) ) ;
					// FIXME: Unsure what to do with remainder (may extend duration?)
					//std::cerr << "still have " << audio_block_used_ << " bytes" << std::endl;
				}

				// Write the trailer, if any
				av_write_trailer( oc_ );

				close_video_codec();
				close_audio_codec();

				// Get the final size now
				boost::int64_t final = oc_->pb->pos;

				// Free the streams
				for( size_t i = 0; i < oc_->nb_streams; i++ )
					av_freep( &oc_->streams[ i ] );

				// Close the output file
				if ( !( fmt_->flags & AVFMT_NOFILE ) )
					url_fclose( oc_->pb );

				// Free the context
				av_free( oc_ );

				if ( ts_context_ )
				{
					char temp[ 132 ];
					snprintf( temp, 132, "eof:%d=%lld\n", push_count_, final );
					write_ts_index( temp );
					url_close( ts_context_ );
				}
			}

			// Clean up the allocated image
			if ( video_stream_ )
				av_free( av_image_.data[0] );

			// Clean up audio buffer
			av_free( audio_outbuf_ );

			// Clean up video buffer
			av_free( video_outbuf_ );

			// Clean up the log
			if ( log_file_ )
				fclose( log_file_ );
			av_free( log_ );
		}

		// Initialise method
		virtual bool init( )
		{
			bool ret = false;

			// Allocate the output context
			oc_ = avformat_alloc_context( );

			// Derive the output format
			fmt_ = alloc_output_format( );
			ret = fmt_ != 0;

			// Handle the 2 pass settings
			if ( ret )
			{
				if ( prop_pass_.value< int >( ) == 1 )
				{
					log_file_ = fopen( opl::to_string( prop_pass_log_.value< opl::wstring >( ) ).c_str( ), "w" );
					ret = log_file_ != 0;
					if ( !ret )
						fprintf( stderr, "Unable to open log file for write %s\n", opl::to_string( prop_pass_log_.value< opl::wstring >( ) ).c_str( ) );
				}
				else if ( prop_pass_.value< int >( ) == 2 )
				{
					FILE *f = fopen( opl::to_string( prop_pass_log_.value< opl::wstring >( ) ).c_str( ), "r" );
					ret = f != 0;
					if ( ret )
					{
						fseek( f, 0, SEEK_END );
						size_t size = ftell( f );
						log_ = ( char * )av_malloc( size + 1 );
						ret = log_ != 0;
						if ( ret )
						{
							fseek( f, 0, SEEK_SET );
							size = fread( log_, 1, size, f );
							log_[ size ] = '\0';
						}
						else
						{
							fprintf( stderr, "Unable to allocate log for %s\n", opl::to_string( prop_pass_log_.value< opl::wstring >( ) ).c_str( ) );
						}
						fclose( f );
					}
					else
					{
						fprintf( stderr, "Unable to open log file for read %s\n", opl::to_string( prop_pass_log_.value< opl::wstring >( ) ).c_str( ) );
					}
				}
			}

			// Set up the remainder of the encoder properties
			if ( ret )
			{
				// Assign the output format
				oc_->oformat = fmt_;

				// Determine requested codecs
				CodecID audio_codec = obtain_audio_codec_id( );
				CodecID video_codec = obtain_video_codec_id( );

				// Open video and audio codec streams
				if ( prop_enable_video_.value< int >( ) == 1 && video_codec != CODEC_ID_NONE )
					video_stream_ = add_video_stream( video_codec );
				else
					video_stream_ = 0;

				if ( prop_enable_audio_.value< int >( ) == 1 && audio_codec != CODEC_ID_NONE )
					audio_stream_ = add_audio_stream( audio_codec );
				else
					audio_stream_ = 0;

				strncpy(oc_->filename, opl::to_string( resource( ) ).c_str( ), sizeof(oc_->filename));

				// Set up the parameters and open all codecs
				if ( av_set_parameters( oc_, NULL ) >= 0 )
				{
					// Attempt to open video and audio streams
					if ( open_video_stream( ) && open_audio_stream( ) )
					{
						if ( fmt_->flags & AVFMT_NEEDNUMBER )
							if ( !av_filename_number_test(oc_->filename) )
								throw( std::string( "invalid format for output" ) );

						// We'll allow encoding to stdout
						if ( !( fmt_->flags & AVFMT_NOFILE ) ) 
							ret = url_fopen( &oc_->pb, opl::to_string( resource( ) ).c_str( ), URL_WRONLY ) >= 0;
						else
							ret = true;
					}

					// Write the header
					if ( ret )
					{
						av_write_header( oc_ );

						if ( prop_ts_index_.value< opl::wstring >( ) != L"" )
						{
							ret = url_open( &ts_context_, opl::to_string( prop_ts_index_.value< opl::wstring >( ) ).c_str( ), URL_WRONLY ) >= 0;
						}
					}
				}
			}

			return ret;
		}

		void write_ts_index( const char *data )
		{
			if ( ts_context_ )
			{
				url_write( ts_context_, ( unsigned char * )( data ), strlen( data ) );
			}
		}

		void close_video_codec( )
		{
			AVStream *stream = video_stream_;
			if ( stream && stream->codec ) {
				avcodec_close( stream->codec );
				av_free( stream->codec );
				stream->codec = NULL;
			}
		}

		void close_audio_codec( )
		{
			AVStream *stream = audio_stream_;
			if ( stream && stream->codec ) {
				avcodec_close( stream->codec );
				av_free( stream->codec );
				stream->codec = NULL;
			}
		}

		// Push a frame to the store
		virtual bool push( frame_type_ptr frame )
		{
			double audio_pts = 0.0;
			double video_pts = 0.0;

			queue( frame );

			while ( 1 )
			{
				if ( audio_stream_ != NULL )
					audio_pts = double( audio_stream_->pts.val ) * audio_stream_->time_base.num / audio_stream_->time_base.den;
				else
					audio_pts = 0.0;
		
				if ( video_stream_ )
					video_pts = double( video_stream_->pts.val ) * video_stream_->time_base.num / video_stream_->time_base.den;
				else
					video_pts = 0.0;

 				// Write interleaved audio and video frames
 				if ( !video_stream_ || ( video_stream_ && audio_stream_ && audio_pts < video_pts ) )
				{
					if ( !audio_queue_.size( ) )
						break;
					if ( !process_audio( ) )
						return false;
				}
 				else if ( video_stream_ )
				{
					if ( !video_queue_.size( ) )
						break;
					if ( !process_video( ) )
						return false;
				}
				else
				{
					return false;
				}
			}

			if ( prop_show_stats_.value< int >( ) && frame_pos_ ++ % prop_show_stats_.value< int >( ) == 0 )
				fprintf( stderr, "%06d: audio %10.4f video %10.4f\r", frame_pos_ - 1, audio_pts, video_pts );

			return true;
		}

		// Tell the store to flush all pending frames, while returning the
		// one that was pending. The default implementation applies when 
		// the store doesn't queue frames.
		virtual frame_type_ptr flush( )
		{ 
			if ( prop_show_stats_.value< int >( ) )
			{
				double audio_pts = audio_stream_ != 0 ? double( audio_stream_->pts.val ) * audio_stream_->time_base.num / audio_stream_->time_base.den : 0.0;
				double video_pts = video_stream_ != 0 ? double( video_stream_->pts.val ) * video_stream_->time_base.num / video_stream_->time_base.den : 0.0;
				fprintf( stderr, "%06d: audio %10.4f video %10.4f\r", frame_pos_ - 1, audio_pts, video_pts );
			}
			return frame_type_ptr( ); 
		}

		// Playout all queued frames and return when done. The default
		// implementation applies when the store doesn't queue frames.
		virtual void complete( )
		{ 
			if ( prop_show_stats_.value< int >( ) )
			{
				double audio_pts = audio_stream_ != 0 ? double( audio_stream_->pts.val ) * audio_stream_->time_base.num / audio_stream_->time_base.den : 0.0;
				double video_pts = video_stream_ != 0 ? double( video_stream_->pts.val ) * video_stream_->time_base.num / video_stream_->time_base.den : 0.0;
				fprintf( stderr, "%06d: audio %10.4f video %10.4f\r", frame_pos_ - 1, audio_pts, video_pts );
			}
		}

		// Allows the store to dictate when it is running empty (ie: any
		// realtime store such as an audio player or device feed needs more
		// frames in order to provide smooth playout)
		virtual bool empty( )
		{ 
			return true; 
		}

	private:
		opl::wstring resource( )
		{
			if ( resource_.find( L"avformat:" ) == 0 )
				return resource_.substr( 9 );
			return resource_;
		}

		// Very rough mapping of file name and format property
		AVOutputFormat *alloc_output_format( )
		{
			AVOutputFormat *fmt = NULL;

			if ( prop_format_.value< opl::wstring >( ) != L"" )
				fmt = guess_format( opl::to_string( prop_format_.value< opl::wstring >( ) ).c_str( ), NULL, NULL );

			if ( fmt == NULL )
				fmt = guess_format( NULL, opl::to_string( resource( ) ).c_str( ), NULL );

			if ( fmt == NULL )
				fmt = guess_format( "mpeg", NULL, NULL );

			return fmt;
		}

		// Obtain the requested video codec id
		CodecID obtain_video_codec_id( )
		{
			CodecID codec_id = fmt_->video_codec;
			if ( prop_vcodec_.value< opl::wstring >( ) != L"" )
			{
				AVCodec *codec = avcodec_find_encoder_by_name( opl::to_string( prop_vcodec_.value< opl::wstring >( ) ).c_str( ) );
				if ( codec != NULL )
					codec_id = codec->id;
			}
			if ( prop_debug_.value< int >( ) )
 				std::cerr << L"obtain_video_codec_id: found: " << codec_id << " for "
						  << opl::to_string( prop_vcodec_.value< opl::wstring >( ) ) << std::endl;
			return codec_id;
		}

		// Obtain the requested audio codec id
		CodecID obtain_audio_codec_id( )
		{
			CodecID codec_id = fmt_->audio_codec;
			if ( prop_acodec_.value< opl::wstring >( ) != L"" )
			{
				AVCodec *codec = avcodec_find_encoder_by_name( opl::to_string( prop_acodec_.value< opl::wstring >( ) ).c_str( ) );
				if ( codec != NULL )
					codec_id = codec->id;
				else if ( prop_debug_.value< int >( ) )
					std::cerr << L"obtain_audio_codec_id: failed to find codec for value: "
							   << opl::to_string( prop_acodec_.value< opl::wstring >( ) )
							   << std::endl;
			}

			if ( prop_debug_.value< int >( ) )
 				std::cerr << L"obtain_audio_codec_id: found: " << codec_id << " for "
 						  << opl::to_string( prop_acodec_.value< opl::wstring >( ) ) << std::endl;

			return codec_id;
		}


		// Generate a video stream
		AVStream *add_video_stream( CodecID codec_id )
		{
			// Create a new stream
			AVStream *st = av_new_stream( oc_, 0 );

			if ( st != NULL ) 
			{
				AVCodecContext *c = st->codec;
				c->codec_id = codec_id;
				c->codec_type = CODEC_TYPE_VIDEO;
#if LIBAVCODEC_VERSION_INT > ((51<<16)|(67 << 8))
				c->reordered_opaque = AV_NOPTS_VALUE;
#endif

				// Specify stream parameters
				c->bit_rate = prop_video_bit_rate_.value< int >( );
				c->bit_rate_tolerance = prop_video_bit_rate_tolerance_.value< int >( );
				c->width = prop_width_.value< int >( );
				c->height = prop_height_.value< int >( );
				c->time_base.den = prop_fps_num_.value< int >( );
				c->time_base.num = prop_fps_den_.value< int >( );
				c->gop_size = prop_gop_size_.value< int >( );
				opl::string pixfmt = opl::to_string( prop_pix_fmt_.value< opl::wstring >( ) );
				c->pix_fmt = pixfmt != "" ? avcodec_get_pix_fmt( pixfmt.c_str( ) ) : PIX_FMT_YUV420P;
		
				// Fix b frames
				if ( prop_b_frames_.value< int >( ) )
				{
	 				c->max_b_frames = prop_b_frames_.value< int >( );
					c->b_frame_strategy = 0;
					c->b_quant_factor = 2.0;
				}

				// Specifiy miscellaneous properties
	 			c->mb_decision = prop_mb_decision_.value< int >( );

#if LIBAVCODEC_VERSION_INT > ((51<<16)|(67 << 8))
				st->sample_aspect_ratio = c->sample_aspect_ratio = av_d2q( prop_aspect_ratio_.value< double >( ) * c->height / c->width, 255 );
#endif

				c->mb_cmp = prop_mb_cmp_.value< int >( );
				c->ildct_cmp = prop_ildct_cmp_.value< int >( );
				c->me_sub_cmp = prop_sub_cmp_.value< int >( );
				c->me_cmp = prop_cmp_.value< int >( );
				c->me_pre_cmp = prop_pre_cmp_.value< int >( );
				c->pre_me = prop_pre_me_.value< int >( );
				c->lumi_masking = static_cast<float>(prop_lumi_mask_.value< double >( ));
				c->dark_masking = static_cast<float>(prop_dark_mask_.value< double >( ));
				c->spatial_cplx_masking = static_cast<float>(prop_scplx_mask_.value< double >( ));
				c->temporal_cplx_masking = static_cast<float>(prop_tcplx_mask_.value< double >( ));
				c->p_masking = static_cast<float>(prop_p_mask_.value< double >( ));
				c->quantizer_noise_shaping= prop_qns_.value< int >( );
				c->qmin = prop_video_qmin_.value< int >( );
				c->qmax = prop_video_qmax_.value< int >( );
				c->lmin = prop_video_lmin_.value< int >( );
				c->lmax = prop_video_lmax_.value< int >( );
				c->mb_qmin = prop_video_mb_qmin_.value< int >( );
				c->mb_qmax = prop_video_mb_qmax_.value< int >( );
				c->max_qdiff = prop_video_qdiff_.value< int >( );
				c->qblur = static_cast<float>(prop_video_qblur_.value< double >( ));
				c->qcompress = static_cast<float>(prop_video_qcomp_.value< double >( ));

				c->rc_eq = strdup("tex^qComp");
				c->rc_override_count = 0;
				c->thread_count = 1;
		
				// Handle fixed quality override
				if ( prop_qscale_.value< double >( ) > 0.0 )
				{
					c->flags |= CODEC_FLAG_QSCALE;
					st->quality = float( FF_QP2LAMBDA * prop_qscale_.value< double >( ) );
				}
		
				// Allow the user to override the video fourcc
				opl::string vfourcc = opl::to_string( prop_vfourcc_.value< opl::wstring >( ) );
				if ( vfourcc != "" )
				{
					char *tail = NULL;
					const char *arg = vfourcc.c_str( );
					int tag = strtol( arg, &tail, 0);
					if( !tail || *tail )
						tag = arg[ 0 ] + ( arg[ 1 ] << 8 ) + ( arg[ 2 ] << 16 ) + ( arg[ 3 ] << 24 );
					c->codec_tag = tag;
				}
		
				// Some formats want stream headers to be seperate
				if ( oc_->oformat->flags & AVFMT_GLOBALHEADER ) 
					c->flags |= CODEC_FLAG_GLOBAL_HEADER;
		
				oc_->preload = int( prop_mux_preload_.value< double >( ) * AV_TIME_BASE );
				oc_->max_delay = int( prop_mux_delay_.value< double >( ) * AV_TIME_BASE );
				oc_->loop_output = AVFMT_NOOUTPUTLOOP;

				oc_->mux_rate = prop_mux_rate_.value< int >( );
				oc_->packet_size = prop_video_packet_size_.value< int >( );
				c->rc_max_rate = prop_video_rc_max_rate_.value< int >( );
				c->rc_min_rate = prop_video_rc_min_rate_.value< int >( );
				c->rc_buffer_size = prop_video_rc_buffer_size_.value< int >( );
				c->rc_buffer_aggressivity = static_cast<float>(prop_video_rc_buffer_aggressivity_.value< double >( ));
				c->rc_initial_cplx= static_cast<float>(prop_video_rc_initial_cplx_.value< double >( ));
				c->i_quant_factor = static_cast<float>(prop_video_i_qfactor_.value< double >( ));
				c->b_quant_factor = static_cast<float>(prop_video_b_qfactor_.value< double >( ));
				c->i_quant_offset = static_cast<float>(prop_video_i_qoffset_.value< double >( ));
				c->b_quant_offset = static_cast<float>(prop_video_b_qoffset_.value< double >( ));
				c->intra_quant_bias = prop_video_intra_quant_bias_.value< int >( );
				c->inter_quant_bias = prop_video_inter_quant_bias_.value< int >( );
				c->dct_algo = prop_dct_algo_.value< int >( );
				c->idct_algo = prop_idct_algo_.value< int >( );
				c->rc_initial_buffer_occupancy = c->rc_buffer_size*3/4;
				c->me_threshold = prop_me_threshold_.value< int >( );
				c->mb_threshold = prop_mb_threshold_.value< int >( );
				c->intra_dc_precision = prop_intra_dc_precision_.value< int >( ) - 8;
				c->strict_std_compliance = prop_strict_.value< int >( );
				c->error_rate = prop_error_rate_.value< int >( );
				c->noise_reduction = prop_noise_reduction_.value< int >( );
				c->scenechange_threshold = prop_sc_threshold_.value< int >( );
				c->me_range = prop_me_range_.value< int >( );
				c->coder_type = prop_coder_.value< int >( );
				c->context_model = prop_context_.value< int >( );
				c->prediction_method = prop_predictor_.value< int >( );
				c->me_method = prop_me_method_.value< int >( );
 			}
 
			return st;
		}

		AVStream *add_audio_stream( CodecID codec_id )
		{
			// Create a new stream
			AVStream *st = av_new_stream( oc_, 1 );

			if ( !st )
				std::cerr << "add_audio_stream: failed to create stream for: " << codec_id << std::endl;

			// If created, then initialise from properties
			if ( st != NULL ) 
			{
				AVCodecContext *c = st->codec;
				c->codec_id = codec_id;
				c->codec_type = CODEC_TYPE_AUDIO;
#if LIBAVCODEC_VERSION_INT > ((51<<16)|(67 << 8))
				c->reordered_opaque = AV_NOPTS_VALUE;
#endif

				// Specify sample parameters
				c->bit_rate = prop_audio_bit_rate_.value< int >( );
				c->sample_rate = prop_frequency_.value< int >( );
				c->channels = prop_channels_.value< int >( );

				if ( oc_->oformat->flags & AVFMT_GLOBALHEADER ) 
					c->flags |= CODEC_FLAG_GLOBAL_HEADER;

				// Allow the user to override the audio fourcc
				opl::string afourcc = opl::to_string( prop_afourcc_.value< opl::wstring >( ) );
				if ( afourcc != "" )
				{
					char *tail = NULL;
					const char *arg = afourcc.c_str( );
					int tag = strtol( arg, &tail, 0);
					if( !tail || *tail )
						tag = arg[ 0 ] + ( arg[ 1 ] << 8 ) + ( arg[ 2 ] << 16 ) + ( arg[ 3 ] << 24 );
					c->codec_tag = tag;
				}
			}

			return st;
		}

		// Open the video stream
		// Note: This method returns true if there is no video stream created
		bool open_video_stream( )
		{
			bool ret = true;

			if ( video_stream_ != NULL )
			{
				// Get the codec
				AVCodecContext *video_enc = video_stream_->codec;

				// find the video encoder
				AVCodec *codec = avcodec_find_encoder( video_enc->codec_id );

				if( codec && codec->pix_fmts )
				{
					const enum PixelFormat *p = codec->pix_fmts;
					for( ; *p != -1; p++ )
					{
						if( *p == video_enc->pix_fmt )
							break;
					}
					if( *p == -1 )
						video_enc->pix_fmt = codec->pix_fmts[ 0 ];
				}

				if ( prop_pass_.value< int >( ) )
				{
					if ( prop_pass_.value< int >( ) == 1 )
						video_enc->flags |= CODEC_FLAG_PASS1;
					else 
						video_enc->flags |= CODEC_FLAG_PASS2;
					video_enc->stats_in = log_;
				}

				// Allocate the image
				avcodec_get_frame_defaults( &av_image_ );
				avpicture_alloc( ( AVPicture * )&av_image_, video_enc->pix_fmt, prop_width_.value< int >( ), prop_height_.value< int >( ) );

				// Open the codec safely
				ret = codec != NULL && avcodec_open( video_enc, codec ) >= 0;
			}

			return ret;
		}

		// Open the audio stream
		// Note: This method returns true if there is no audio stream created
		bool open_audio_stream( )
		{
			bool ret = true;

			if ( audio_stream_ != NULL )
			{
				// Get the context
				AVCodecContext *c = audio_stream_->codec;

				// Find the encoder
				AVCodec *codec = avcodec_find_encoder( c->codec_id );

				// Continue if codec found and we can open it
				if ( codec != NULL && avcodec_open( c, codec ) >= 0 )
				{
					if ( c->frame_size <= 1 ) 
					{
						audio_input_frame_size_ = audio_outbuf_size_ / c->channels;
						switch( audio_stream_->codec->codec_id ) 
						{
							case CODEC_ID_PCM_S16LE:
							case CODEC_ID_PCM_S16BE:
							case CODEC_ID_PCM_U16LE:
							case CODEC_ID_PCM_U16BE:
								audio_input_frame_size_ >>= 1;
								break;
							default:
								break;
						}
					} 
					else 
					{
						audio_input_frame_size_ = c->frame_size;
					}

					// Some formats want stream headers to be seperate
					if( oc_->oformat->flags & AVFMT_GLOBALHEADER )
						c->flags |= CODEC_FLAG_GLOBAL_HEADER;

					ret = audio_input_frame_size_ != 0;
				}
			}

			return ret;
		}

		// Queue video and audio components from frame
		void queue( frame_type_ptr frame )
		{
			// Queue video - this is simply the frames image
			if ( video_stream_ && frame->get_image( ) )
				video_queue_.push_back( frame->get_image( ) );

			// Queue audio - audio is carved up here to match the number of bytes required by the audio codec
			if ( audio_stream_ && frame->get_audio( ) )
			{
				typedef audio< unsigned char, pcm16 > pcm16;

				audio_type_ptr current = frame->get_audio( );

				// Create an audio block if we haven't done so already
				if ( audio_block_ == 0 )
				{
					audio_block_used_ = 0;
					audio_block_ = audio_type_ptr( new audio_type( pcm16( current->frequency( ), current->channels( ), audio_input_frame_size_ ) ) );
					audio_block_->set_position( push_count_ );
				}

				// We want to ensure that 'gop boundaries' are flagged when the audio is introduced, not when just when a new 
				// audio frame is created (otherwise, a search based on the byte position will miss the start of the audio)
				if ( push_count_ % prop_gop_size_.value< int >( ) == 0 )
					audio_block_->set_position( push_count_ );
		
				int bytes = audio_input_frame_size_ * current->channels( ) * 2;
				int available = current->allocsize( );
				int offset = 0;

				while ( bytes > 0 && audio_block_used_ + available > bytes )
				{
					memcpy( audio_block_->data( ) + audio_block_used_, current->data( ) + offset, bytes - audio_block_used_ );
					audio_queue_.push_back( audio_block_ );
					available -= bytes - audio_block_used_;
					offset += bytes - audio_block_used_;
					audio_block_used_ = 0;
					audio_block_ = audio_type_ptr( new audio_type( pcm16( current->frequency( ), current->channels( ), audio_input_frame_size_ ) ) );
					audio_block_->set_position( push_count_ );
				}

				if ( offset < current->allocsize( ) )
				{
					memcpy( audio_block_->data( ) + audio_block_used_, current->data( ) + offset, current->allocsize( ) - offset );
					audio_block_used_ += current->allocsize( ) - offset;
				}
			}

			// Keep a running total of frames pushed
			push_count_ ++;
		}

		bool do_video_encode (bool finalize, int *out_size) 
		{
			bool ret;

			AVCodecContext *c = video_stream_->codec;
			AVFrame *image = &av_image_;
			if(finalize)
				image = NULL;

			*out_size = avcodec_encode_video( c, video_outbuf_, video_outbuf_size_, image );

			// If zero size, it means the image was buffered
			if ( *out_size > 0 )
			{
				AVPacket pkt;
				av_init_packet( &pkt );

				if ( c->coded_frame && boost::uint64_t( c->coded_frame->pts ) != AV_NOPTS_VALUE )
					pkt.pts = av_rescale_q( c->coded_frame->pts, c->time_base, video_stream_->time_base );
#if LIBAVCODEC_VERSION_INT > ((51<<16)|(67 << 8))
				if ( boost::uint64_t( c->reordered_opaque ) != AV_NOPTS_VALUE)
					pkt.dts = av_rescale_q(c->reordered_opaque, c->time_base, video_stream_->time_base );
#endif
				if( c->coded_frame && c->coded_frame->key_frame )
				{
					char temp[ 132 ];
					snprintf( temp, 132, "%lld=%lld\n", c->coded_frame->pts, oc_->pb->pos );
					write_ts_index( temp );
					pkt.flags |= PKT_FLAG_KEY;
				}

				pkt.stream_index = video_stream_->index;
				pkt.data = video_outbuf_;
				pkt.size = *out_size;

				if ( log_file_ && prop_pass_.value< int >( ) == 1 && c->stats_out )
					fprintf( log_file_, "%s", c->stats_out );

				// Write the compressed frame in the media file
				int err = av_interleaved_write_frame( oc_, &pkt );
				ret = err >= 0;

				if ( log_file_ && prop_pass_.value< int >( ) == 1  && c->stats_out )
					fprintf( log_file_, "%s", c->stats_out );

			}
			else
			{
				ret = true;
			}
			return ret;
		}

		// Process and output an image
		// Precondition: the video queue is not empty
		bool process_video( )
		{
			bool ret = true;

			// Obtain the next image
			il::image_type_ptr image = *( video_queue_.begin( ) );
			video_queue_.pop_front( );

			AVCodecContext *c = video_stream_->codec;

			// Convert the image to the colour space required
			const std::wstring pf = avformat_to_oil( c->pix_fmt );

			if ( pf != L"" )
			{
				image = il::convert( image, pf );
				// Need an ffmpeg fallback here...
				if ( image == 0 )
					return false;

				// Convert the oil image to an avformat one
				// TODO: Check if memcpy is needed
				int plane = 0;

				for ( plane = 0; plane < image->plane_count( ); plane ++ )
				{
					uint8_t *src = image->data( plane );
					uint8_t *dst = av_image_.data[ plane ];
					int height = image->height( plane );
					while( height -- )
					{
						memcpy( dst, src, av_image_.linesize[ plane ] );
						src += image->pitch( plane );
						dst += av_image_.linesize[ plane ];
					}
				}
			}
			else
			{
				AVPicture input;
				int width = image->width( );
				int height = image->height( );
				avpicture_fill( &input, image->data( ), oil_to_avformat( image->pf( ) ), width, height );
				img_convert_ = sws_getCachedContext( img_convert_, width, height, c->pix_fmt, width, height, oil_to_avformat( image->pf( ) ), SWS_BICUBIC, NULL, NULL, NULL );
				if ( img_convert_ != NULL )
					sws_scale( img_convert_, input.data, input.linesize, 0, height, av_image_.data, av_image_.linesize );
			}

			if ( oc_->oformat->flags & AVFMT_RAWPICTURE )  
			{
 				// raw video case. The API will change slightly in the near future for that
				AVPacket pkt;
				av_init_packet( &pkt );
		
				pkt.flags |= PKT_FLAG_KEY;
				pkt.stream_index = video_stream_->index;
				pkt.data = ( uint8_t * )&av_image_;
				pkt.size = sizeof( AVPicture );

				ret = av_interleaved_write_frame( oc_, &pkt ) == 0;
 			} 
			else 
			{
				// Set the quality
				av_image_.quality = int( video_stream_->quality );
				av_image_.pict_type = 0;

				int out_size;
				ret = do_video_encode(false, &out_size);
			}

			return ret;
		}

		// Process and output a block of audio samples (see comments in queue above)
		bool process_audio( )
		{
			bool ret = true;

			audio_type_ptr audio;
			short *data = 0;

			if ( audio_queue_.size( ) )
			{
				audio = *( audio_queue_.begin( ) );
				audio_queue_.pop_front( );
				data = ( short * )( audio->data( ) );

				if ( !video_stream_ && audio->position( ) != last_audio_ && audio->position( ) % prop_gop_size_.value< int >( ) == 0 )
				{
					last_audio_ = audio->position( );
					char temp[ 132 ];
					snprintf( temp, 132, "%d=%lld\n", audio->position( ), oc_->pb->pos );
					write_ts_index( temp );
				}
			}

			AVCodecContext *c = audio_stream_->codec;

			AVPacket pkt;
			av_init_packet( &pkt );

			pkt.size = avcodec_encode_audio( c, audio_outbuf_, audio_outbuf_size_, data );

			// Write the compressed frame in the media file
			if ( c->coded_frame && uint64_t( c->coded_frame->pts ) != AV_NOPTS_VALUE )
				pkt.pts = av_rescale_q( c->coded_frame->pts, c->time_base, audio_stream_->time_base );

			pkt.flags |= PKT_FLAG_KEY;
			pkt.stream_index = audio_stream_->index;
			pkt.data = audio_outbuf_;

			if ( pkt.size )
				ret = av_interleaved_write_frame( oc_, &pkt ) == 0;
			else if ( data == 0 )
				ret = false;

			return ret;
		}

		// The output file or device
		opl::wstring resource_;
		pcos::property prop_enable_audio_;
		pcos::property prop_enable_video_;

		// libavformat structures
		AVFormatContext *oc_;
		AVOutputFormat *fmt_;
		AVStream *audio_stream_;
		AVStream *video_stream_;
		struct SwsContext *img_convert_;
		int audio_input_frame_size_;
		int audio_outbuf_size_;
		uint8_t *audio_outbuf_;
		AVFrame av_image_; 
		uint8_t *video_outbuf_;
		int video_outbuf_size_;

		// oml structures
		std::deque< il::image_type_ptr > video_queue_;
		std::deque< audio_type_ptr > audio_queue_;
		audio_type_ptr audio_block_;
		int audio_block_used_;

		// Application facing properties
		pcos::property prop_show_stats_;
		pcos::property prop_debug_;
		pcos::property prop_format_;
		pcos::property prop_vcodec_;
		pcos::property prop_acodec_;
		pcos::property prop_pix_fmt_;
		pcos::property prop_vfourcc_;
		pcos::property prop_afourcc_;
		pcos::property prop_width_;
		pcos::property prop_height_;
		pcos::property prop_aspect_ratio_;
		pcos::property prop_fps_num_;
		pcos::property prop_fps_den_;
		pcos::property prop_frequency_;
		pcos::property prop_channels_;
		pcos::property prop_audio_bit_rate_;
		pcos::property prop_video_bit_rate_;
		pcos::property /* int */ prop_video_bit_rate_tolerance_;
		pcos::property /* int */ prop_gop_size_;
		pcos::property /* int */ prop_b_frames_;
		pcos::property /* int */ prop_mb_decision_;
		pcos::property /* double */ prop_qscale_;
		pcos::property /* int */ prop_me_method_;
		pcos::property /* int */ prop_mb_cmp_;
		pcos::property /* int */ prop_ildct_cmp_;
		pcos::property /* int */ prop_sub_cmp_;
		pcos::property /* int */ prop_cmp_;
		pcos::property /* int */ prop_pre_cmp_;
		pcos::property /* int */ prop_pre_me_;
		pcos::property /* double */ prop_lumi_mask_;
		pcos::property /* double */ prop_dark_mask_;
		pcos::property /* double */ prop_scplx_mask_;
		pcos::property /* double */ prop_tcplx_mask_;
		pcos::property /* double */ prop_p_mask_;
		pcos::property /* int */ prop_qns_;
		pcos::property /* int */ prop_video_qmin_;
		pcos::property /* int */ prop_video_qmax_;
		pcos::property /* int */ prop_video_lmin_;
		pcos::property /* int */ prop_video_lmax_;
		pcos::property /* int */ prop_video_mb_qmin_;
		pcos::property /* int */ prop_video_mb_qmax_;
		pcos::property /* int */ prop_video_qdiff_;
		pcos::property /* double */ prop_video_qblur_;
		pcos::property /* double */ prop_video_qcomp_;
		pcos::property /* int */ prop_mux_rate_;
		pcos::property /* double */ prop_mux_preload_;
		pcos::property /* double */ prop_mux_delay_;
		pcos::property /* int */ prop_video_packet_size_;
		pcos::property /* int */ prop_video_rc_max_rate_;
		pcos::property /* int */ prop_video_rc_min_rate_;
		pcos::property /* int */ prop_video_rc_buffer_size_;
		pcos::property /* double */ prop_video_rc_buffer_aggressivity_;
		pcos::property /* double */ prop_video_rc_initial_cplx_;
		pcos::property /* double */ prop_video_i_qfactor_;
		pcos::property /* double */ prop_video_b_qfactor_;
		pcos::property /* double */ prop_video_i_qoffset_;
		pcos::property /* double */ prop_video_b_qoffset_;
		pcos::property /* int */ prop_video_intra_quant_bias_;
		pcos::property /* int */ prop_video_inter_quant_bias_;
		pcos::property /* int */ prop_dct_algo_;
		pcos::property /* int */ prop_idct_algo_;
		pcos::property /* int */ prop_me_threshold_;
		pcos::property /* int */ prop_mb_threshold_;
		pcos::property /* int */ prop_intra_dc_precision_;
		pcos::property /* int */ prop_strict_;
		pcos::property /* int */ prop_error_rate_;
		pcos::property /* int */ prop_noise_reduction_;
		pcos::property /* int */ prop_sc_threshold_;
		pcos::property /* int */ prop_me_range_;
		pcos::property /* int */ prop_coder_;
		pcos::property /* int */ prop_context_;
		pcos::property /* int */ prop_predictor_;
		pcos::property /* int */ prop_pass_;
		pcos::property /* wstring */ prop_pass_log_;

		pcos::property prop_ts_index_;
		URLContext *ts_context_;

		FILE *log_file_;
		char *log_;

		int frame_pos_;
		int push_count_;
		int last_audio_;
};

class aml_index
{
	public:
		/// Read any pending data in the index
		virtual void read( int max = -1 ) = 0;

		/// Find the byte position for the position
		virtual int64_t find( int position ) = 0;

		/// Return the number of frames as reported by the index
		/// Note that if we have not reached the eof, there is a chance that the index won't
		/// accurately reflect the number of frames in the available media (since we can't
		/// guarantee that the two files will be in sync), so prior to the reception of the 
		/// terminating record, we need to return an approximation which is > current
		virtual int frames( int current ) = 0;

		/// Returns the size of the data file as indicated by the index
		virtual boost::int64_t bytes( ) = 0;

		/// Calculate the number of available frames for the given media data size 
		/// Note that the size given can be less than, equal to or greater than the size
		/// which the index reports - any data greater than the index should be ignored, 
		/// and data less than the index should report a frame count such that the last
		/// gop can be full decoded
		virtual int calculate( boost::int64_t size ) = 0;

		/// Indicates if the media can use the index for byte accurate seeking or not
		/// This is a work around for audio files which don't like byte positions
		virtual bool usable( ) = 0;

		/// User of the index can indicate byte seeking usability here
		virtual void set_usable( bool value ) = 0;
};

class aml_file_index : public aml_index
{
	public:
		aml_file_index( const char *file )
			: file_( file )
			, position_( 0 )
			, eof_( false )
			, frames_( 0 )
			, usable_( true )
		{
		}

		virtual ~aml_file_index( )
		{
		}

		virtual void read( int max = -1 )
		{
			if ( eof_ ) return;

			FILE *fd = fopen( file_.c_str( ), "r" );
			
			if ( fd )
			{
				char temp[ 132 ];
				fseek( fd, position_, SEEK_SET );
				while( true )
				{
					position_ = ftell( fd );
					if ( fgets( temp, 132, fd ) && temp[ strlen( temp ) - 1 ] == '\n' )
					{
						int index = 0;
						boost::int64_t position;
						if ( sscanf( temp, "%d=%lld", &index, &position ) == 2 )
						{
							key_index_[ index ] = position;
						}
						else if ( sscanf( temp, "eof:%d=%lld", &index, &position ) == 2 )
						{
							key_index_[ index ] = position;
							eof_ = true;
						}
						else
						{
							std::cerr << "Invalid index info " << temp << std::endl;
						}
						if ( index > frames_ )
							frames_ = index;
					}
					else
					{
						break;
					}
				}

				fclose( fd );
			}
		}

		int64_t find( int position )
		{
			boost::int64_t byte = -1;

			if ( key_index_.size( ) )
			{
				std::map< int, boost::int64_t >::iterator iter = key_index_.lower_bound( position );
				if ( iter != key_index_.end( ) )
					byte = ( *iter ).second;
			}

			return byte;
		}

		int frames( int current )
		{
			read( 1 );

			int result = frames_;

			// If the current value does not indicate that the index and data are in sync, then we want
			// to return a value which is greater than the current, and significantly less than the value
			// indicated by the index 
			if ( current < frames_ )
			{
				std::map< int, boost::int64_t >::iterator iter = key_index_.lower_bound( frames_ - 100 );
				result = ( *iter ).first >= current ? ( *iter ).first + 1 : current + 1;
			}

			return result;
		}

		boost::int64_t bytes( )
		{
			std::map< int, boost::int64_t >::iterator iter = -- key_index_.end( );
			return ( *iter ).second;
		}

		int calculate( boost::int64_t size )
		{
			std::map< int, boost::int64_t >::iterator iter = -- key_index_.end( );
			int result = ( *iter ).first;
			if ( !( eof_ && size == ( *iter ).second ) )
			{
				boost::int64_t next = ( *iter ).second;
				while ( iter != key_index_.begin( ) )
				{
					iter --;
					if ( ( *iter ).second < size && next < size )
						break;
					next = ( *iter ).second;
				}
				result = ( *iter ).first;
			}
			return result;
		}

		bool usable( )
		{
			return usable_;
		}

		void set_usable( bool value )
		{
			usable_ = value;
		}

	protected:
		std::string file_;
		int position_;
		std::map < int, boost::int64_t > key_index_;
		bool eof_;
		int frames_;
		bool usable_;
};

class aml_http_index : public aml_file_index
{
	public:
		aml_http_index( const char *file )
			: aml_file_index( file )
			, remainder_ ( "" )
			, next_time_( boost::get_system_time( ) )
		{
			if ( url_open( &ts_context_, file, URL_RDONLY ) < 0 )
			{
				usable_ = false;
				eof_ = true;
			}
		}

		virtual ~aml_http_index( )
		{
			url_close( ts_context_ );
		}

		void read( int max = -1 )
		{
			if ( eof_ ) return;

			if ( boost::get_system_time( ) < next_time_ ) return;

			if ( key_index_.size( ) == 0 )
				next_time_ = boost::get_system_time( ) + boost::posix_time::seconds( 2 );

			char temp[ 400 ];

			while ( true )
			{
				int actual = 0;
				int bytes = remainder_.length( );
				int requested = 200 - bytes;
				char *ptr = temp;

				memset( temp, 0, sizeof( temp ) );
				memcpy( temp, remainder_.c_str( ), bytes );

				actual = url_read( ts_context_, ( unsigned char * )temp + bytes, requested );
				if ( actual <= 0 ) break;

				while( ptr && strchr( ptr, '\n' ) )
				{
					int index = 0;
					boost::int64_t position;
					if ( sscanf( ptr, "%d=%lld", &index, &position ) == 2 )
					{
						key_index_[ index ] = position;
						if ( index > frames_ )
							frames_ = index;
					}
					else if ( !strncmp( ptr, "eof:", 4 ) && sscanf( ptr + 4, "%d=%lld", &index, &position ) == 2 )
					{
						key_index_[ index ] = position;
						eof_ = true;
						if ( index > frames_ )
							frames_ = index;
					}
					else
					{
						//std::cerr << "Invalid index info " << ptr << std::endl;
					}

					ptr = strchr( ptr, '\n' );
					if ( ptr ) ptr ++;
				}

				if ( ptr )
					remainder_ = ptr;
				else
					remainder_ = "";

				if ( -- max == 0 ) break;
				if ( eof_ || ( actual != requested && remainder_ == "" && boost::get_system_time( ) > next_time_ ) ) break;
			}

			next_time_ = boost::get_system_time( ) + boost::posix_time::seconds( 1 );
		}

	private:
		URLContext *ts_context_;
		std::string remainder_;
		boost::system_time next_time_;
};

static aml_index *aml_index_factory( const char *url )
{
	aml_index *result = 0;
	if ( !strncmp( url, "http://", 7 ) )
		result = new aml_http_index( url );
	else if ( !strcmp( url, "" ) )
		result = new aml_file_index( url );

	if ( result )
	{
		result->read( );
		if ( result->frames( 0 ) == 0 )
		{
			delete result;
			result = 0;
		}
	}

	return result;
}

class ML_PLUGIN_DECLSPEC avformat_input : public input_type
{
	public:
		typedef audio< unsigned char, pcm16 > pcm16_audio_type;

		// Constructor and destructor
		avformat_input( opl::wstring resource, const opl::wstring mime_type = L"" ) 
			: input_type( ) 
			, uri_( resource )
			, mime_type_( mime_type )
			, frames_( 0 )
			, is_seekable_( true )
			, first_frame_( true )
			, first_found_( 0 )
			, fps_num_( 25 )
			, fps_den_( 1 )
			, sar_num_( 59 )
			, sar_den_( 54 )
			, width_( 720 )
			, height_( 576 )
			, context_( 0 )
			, format_( 0 )
			, params_( 0 )
			, prop_video_index_( pcos::key::from_string( "video_index" ) )
			, prop_audio_index_( pcos::key::from_string( "audio_index" ) )
			, prop_gop_size_( pcos::key::from_string( "gop_size" ) )
			, prop_gop_cache_( pcos::key::from_string( "gop_cache" ) )
			, prop_format_( pcos::key::from_string( "format" ) )
			, prop_genpts_( pcos::key::from_string( "genpts" ) )
			, prop_frames_( pcos::key::from_string( "frames" ) )
			, prop_file_size_( pcos::key::from_string( "file_size" ) )
			, prop_estimate_( pcos::key::from_string( "estimate" ) )
			, prop_fps_num_( pcos::key::from_string( "fps_num" ) )
			, prop_fps_den_( pcos::key::from_string( "fps_den" ) )
			, prop_ts_filter_( pcos::key::from_string( "ts_filter" ) )
			, prop_ts_index_( pcos::key::from_string( "ts_index" ) )
			, expected_( 0 )
			, av_frame_( 0 )
			, video_codec_( 0 )
			, audio_codec_( 0 )
			, pkt_( )
			, images_( )
			, audio_( )
			, must_decode_( true )
			, must_reopen_( false )
			, key_search_( false )
			, audio_buf_used_( 0 )
			, audio_buf_offset_( 0 )
			, img_pos_( -1 )
			, aud_pos_( -1 )
			, img_inc_( 0 )
			, sync_diff_( 0 )
			, start_time_( 0 )
			, img_convert_( 0 )
			, h264_hack_( false )
			, samples_per_frame_( 0 )
			, samples_per_packet_( 0 )
			, samples_duration_( 0 )
			, ts_pusher_( )
			, ts_filter_( )
			, aml_index_( 0 )
		{
			audio_buf_size_ = (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2;
			audio_buf_ = ( uint8_t * )av_malloc( 2 * audio_buf_size_ );
			audio_buf_temp_ = ( uint8_t * )av_malloc( audio_buf_size_ );

			// Allow property control of video and audio index
			// NB: Should also have read only props for stream counts
			properties( ).append( prop_video_index_ = 0 );
			properties( ).append( prop_audio_index_ = 0 );
			properties( ).append( prop_gop_size_ = -1 );
			properties( ).append( prop_gop_cache_ = 25 );
			properties( ).append( prop_format_ = opl::wstring( L"" ) );
			properties( ).append( prop_genpts_ = 0 );
			properties( ).append( prop_frames_ = -1 );
			properties( ).append( prop_file_size_ = boost::int64_t( 0 ) );
			properties( ).append( prop_estimate_ = 0 );
			properties( ).append( prop_fps_num_ = -1 );
			properties( ).append( prop_fps_den_ = -1 );
			properties( ).append( prop_ts_filter_ = opl::wstring( L"" ) );
			properties( ).append( prop_ts_index_ = opl::wstring( L"" ) );
		}

		virtual ~avformat_input( ) 
		{
			if ( prop_video_index_.value< int >( ) >= 0 )
				close_video_codec( );
			if ( prop_audio_index_.value< int >( ) >= 0 )
				close_audio_codec( );
			if ( context_ )
				av_close_input_file( context_ );
			av_free( av_frame_ );
			av_free( audio_buf_ );
			av_free( audio_buf_temp_ );
			delete aml_index_;
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return true; }

		// For now, all oml protocol handlers are consider unsafe for threading
		virtual bool is_thread_safe( ) const
		{
			return uri_.find( L"oml:" ) != 0;
		}

		virtual double fps( ) const
		{
			int num, den;
			get_fps( num, den );
			return den != 0 ? double( num ) / double( den ) : 1;
		}

		// Basic information
		virtual const opl::wstring get_uri( ) const { return uri_; }
		virtual const opl::wstring get_mime_type( ) const { return mime_type_; }
		virtual bool has_video( ) const { return prop_video_index_.value< int >( ) != -1 && video_indexes_.size( ) > 0; }
		virtual bool has_audio( ) const { return prop_audio_index_.value< int >( ) != -1 && audio_indexes_.size( ) > 0; }

		// Audio/Visual
		virtual int get_frames( ) const 
		{
			if ( aml_index_  ) 
				return aml_index_->frames( frames_ );
			else 
				return frames_; 
		}

		virtual bool is_seekable( ) const { return is_seekable_; }

		// Visual
		virtual void get_fps( int &num, int &den ) const { num = fps_num_; den = fps_den_; }
		virtual void get_sar( int &num, int &den ) const { num = sar_num_; den = sar_den_; }
		virtual int get_video_streams( ) const { return video_indexes_.size( ); }
		virtual int get_width( ) const { return width_; }
		virtual int get_height( ) const { return height_; }

		// Audio
		virtual int get_audio_streams( ) const { return audio_indexes_.size( ); }
		virtual bool set_video_stream( const int stream ) { prop_video_index_ = stream; return true; }

		virtual bool set_audio_stream( const int stream ) 
		{
			if ( stream < 0 || stream >= int( audio_indexes_.size( ) ) )
			{
				prop_audio_index_ = -1;
			}
			else if ( stream < int( audio_indexes_.size( ) ) )
			{
				prop_audio_index_ = stream;
			}

			return true; 
		}

	protected:
		// Fetch method
		void do_fetch( frame_type_ptr &result )
		{
			ARENFORCE_MSG( context_, "Invalid media" );

			if ( aml_index_ )
			{
				if ( get_position( ) >= frames_ && aml_index_->frames( frames_ ) > frames_ )
				{
					boost::int64_t pos = context_->pb->pos;
					av_url_read_pause( url_fileno( context_->pb ), 0 );
					boost::int64_t bytes = url_seek( url_fileno( context_->pb ), -1, SEEK_END );
					frames_ = aml_index_->calculate( bytes );
					url_seek( url_fileno( context_->pb ), pos, SEEK_SET );
				}
			}

			int process_flags = get_process_flags( );

			if ( last_frame_ && get_position( ) == last_frame_->get_position( ) )
			{
				result = frame_type::shallow_copy( last_frame_ );
				return;
			}

			// Create the output frame
			result = frame_type_ptr( new frame_type( ) );

			// Seek to the correct position if necessary
			if ( get_position( ) != expected_ )
			{
				int current = get_position( ) + first_found_;
				int valid = 0;

				if ( images_.size( ) > 0 )
					valid = images_[ 0 ]->position( );
				else if ( audio_.size( ) > 0 )
					valid = audio_[ 0 ]->position( );

				if ( current >= valid && current < int( valid + 2 * fps( ) ) )
				{
				}
				else if ( seek_to_position( ) )
				{
					clear_stores( true );
				}
				else
				{
					seek( expected_ );
				}
				expected_ = get_position( ) + first_found_;
			}

			// Clear the packet
			av_init_packet( &pkt_ );

			// Loop until an error or we have what we need
			int error = 0;

			bool got_picture = has_video_for( get_position( ) );
			bool got_audio = has_audio_for( get_position( ) );

			bool process_null = false;

			while( error >= 0 && ( !got_picture || !got_audio ) )
			{
				error = av_read_frame( context_, &pkt_ );
				if ( error >= 0 && is_video_stream( pkt_.stream_index ) )
					error = decode_image( got_picture, &pkt_ );
				else if ( error >= 0 && is_audio_stream( pkt_.stream_index ) )
					error = decode_audio( got_audio );
				else if ( error < 0 && !is_seekable_ )
					frames_ = get_position( );
				else if ( prop_genpts_.value< int >( ) && error < 0 )
					frames_ = get_position( );
				else if ( error < 0 )
					process_null = true;
				av_free_packet( &pkt_ );
			}

			if ( frames_ != 1 && has_video( ) && process_null )
			{
				bool temp;
				decode_image( temp, &pkt_ );
			}

			// Hmmph
			if ( has_video( ) )
			{
				sar_num_ = get_video_stream( )->codec->sample_aspect_ratio.num;
				sar_den_ = get_video_stream( )->codec->sample_aspect_ratio.den;
				sar_num_ = sar_num_ != 0 ? sar_num_ : 1;
				sar_den_ = sar_den_ != 0 ? sar_den_ : 1;
			}

			result->set_sar( sar_num_, sar_den_ );
			result->set_fps( fps_num_, fps_den_ );
			result->set_position( get_position( ) );
			result->set_pts( expected_ * 1.0 / avformat_input::fps( ) );
			result->set_duration( 1.0 / avformat_input::fps( ) );

			bool exact_image = false;
			bool exact_audio = false;

			if ( ( process_flags & process_image ) && has_video( ) )
				exact_image = find_image( result );
			if ( ( process_flags & process_audio ) && has_audio( ) )
				exact_audio = find_audio( result );

			// Update the next expected position
			if ( exact_image || exact_audio )
				expected_ ++;

			if ( prop_genpts_.value< int >( ) && expected_ >= frames_ && error >= 0 && ( got_picture || got_audio ) )
				frames_ = expected_ + 1;

			last_frame_ = frame_type::shallow_copy( result );

#if 0
			// Temporarily commented out to avoid some false negatives (image repeat, small discrepancy
			// during start of reverse play etc)
			if ( has_audio( ) && has_video( ) )
				ARENFORCE_MSG( exact_audio && exact_image, "Incomplete frame (audio or image missing)" );
			else if ( has_audio( ) )
				ARENFORCE_MSG( exact_audio, "Incomplete frame (audio missing)" );
			else if ( has_audio( ) )
				ARENFORCE_MSG( exact_image, "Incomplete frame (image missing)" );
#endif
		}

		virtual bool initialize( )
		{
			opl::wstring resource = uri_;

			// Try to obtain the index specified
			aml_index_ = aml_index_factory( opl::to_string( prop_ts_index_.value< opl::wstring >( ) ).c_str( ) );

			// A mechanism to ensure that avformat can always be accessed
			if ( resource.find( L"avformat:" ) == 0 )
				resource = resource.substr( 9 );

			// Convenience expansion for *nix based people
			if ( resource.find( L"~" ) == 0 )
				resource = opl::to_wstring( getenv( "HOME" ) ) + resource.substr( 1 );

			// Ugly - looking to see if a dv1394 device has been specified
			if ( resource.find( L"/dev/" ) == 0 && resource.find( L"1394" ) != opl::wstring::npos )
			{
				prop_format_ = opl::wstring( L"dv" );
			}

			// Allow dv on stdin
			if ( resource == L"dv:-" )
			{
				prop_format_ = opl::wstring( L"dv" );
				resource = L"pipe:";
				is_seekable_ = false;
			}

			// Allow mpeg on stdin
			if ( resource == L"mpeg:-" )
			{
				prop_format_ = opl::wstring( L"mpeg" );
				resource = L"pipe:";
				is_seekable_ = false;
				key_search_ = true;
			}

			// Corrections for file formats
			if ( resource.find( L".mpg" ) == resource.length( ) - 4 )
				key_search_ = true;
			else if ( resource.find( L".dv" ) == resource.length( ) - 3 )
				prop_format_ = opl::wstring( L"dv" );
			else if ( resource.find( L".mp2" ) == resource.length( ) - 4 )
				prop_format_ = opl::wstring( L"mpeg" );

			// Obtain format
			if ( prop_format_.value< opl::wstring >( ) != L"" )
				format_ = av_find_input_format( opl::to_string( prop_format_.value< opl::wstring >( ) ).c_str( ) );

			// Attempt to open the resource
			int error = av_open_input_file( &context_, opl::to_string( resource ).c_str( ), format_, 0, params_ ) < 0;

			// Check for streaming
			if ( error == 0 && context_->pb && url_is_streamed( context_->pb ) )
			{
				is_seekable_ = false;
				key_search_ = true;
			}

			// Get the stream info
			if ( error == 0 )
				error = av_find_stream_info( context_ ) < 0;
			
			// Just in case the requested/derived file format is incorrect, on a failure, try again with no format
			if ( error && format_ )
			{
				format_ = 0;
				if ( context_ )
					av_close_input_file( context_ );
				error = av_open_input_file( &context_, opl::to_string( resource ).c_str( ), format_, 0, params_ ) < 0;
				if ( !error )
					error = av_find_stream_info( context_ ) < 0;
			}

			used_name_ = opl::to_string( resource );

			// Populate the input properties
			if ( error == 0 )
				populate( );

			// Allocate an av frame
			av_frame_ = avcodec_alloc_frame( );

			// If the stream is deemed seekable, then we don't need the first_found logic
			first_frame_ = !is_seekable_;

			// Check for and create the timestamp correction filter graph
			if ( prop_ts_filter_.value< opl::wstring >( ) != L"" )
			{
				ts_filter_ = create_filter( prop_ts_filter_.value< opl::wstring >( ) );
				if ( ts_filter_ )
				{
					ts_pusher_ = create_input( L"pusher:" );
					ts_filter_->connect( ts_pusher_ );
				}
			}

			// Check if we need to do additional size checks
			bool sizing = error == 0 && should_size_media( context_->iformat->name );

			// Report the file size via the file_size and frames properties
			if ( error == 0 )
				prop_file_size_ = boost::int64_t( context_->file_size );

			// Carry out the media sizing logic
			if ( error == 0 && !aml_index_ )
			{
				if ( h264_hack_ && sizing )
					av_seek_frame( context_, -1, context_->data_offset, AVSEEK_FLAG_BYTE );
				fetch( );

				if ( images_.size( ) )
					first_found_ = images_[ 0 ]->position( );

				if ( sizing && images_.size( ) )
				{
					switch( prop_estimate_.value< int >( ) )
					{
						case 0:
							frames_ = size_media_by_images( );
							break;
						case 1:
							frames_ = size_media_by_packets( );
							break;
						default:
							break;
					}
				}
				else if( sizing )
				{
					frames_ = size_media_by_packets();
				}

				// Courtesy - ensure we're reposition on the first frame
				seek( 0 );
			}
			else if ( error == 0 && aml_index_ )
			{
				aml_index_->set_usable( has_video( ) );
				boost::int64_t current = context_->pb->pos;
				boost::int64_t bytes = url_seek( url_fileno( context_->pb ), -1, SEEK_END );
				frames_ = aml_index_->calculate( bytes );
				url_seek( url_fileno( context_->pb ), current, SEEK_SET );
			}

			return error == 0;
		}


	private:
		bool is_video_stream( int index )
		{
			return has_video( ) && 
				   prop_video_index_.value< int >( ) > -1 && 
				   index == video_indexes_[ prop_video_index_.value< int >( ) ];
		}

		bool is_audio_stream( int index )
		{
			return has_audio( ) && 
				   prop_audio_index_.value< int >( ) > -1 && 
				   index == audio_indexes_[ prop_audio_index_.value< int >( ) ];
		}

		bool has_video_for( int position )
		{
			int process_flags = get_process_flags( );
			bool got_video = !has_video( );
			int current = position + first_found_;

			if ( ( process_flags & process_image ) != 0 && ( !got_video && images_.size( ) > 0 ) )
			{
				int first = images_[ 0 ]->position( );
				int last = images_[ images_.size( ) - 1 ]->position( );
				got_video = current >= first && current <= last;
			}

			return got_video;
		}

		bool has_audio_for( int position )
		{
			int process_flags = get_process_flags( );
			bool got_audio = !has_audio( );
			int current = position + first_found_;

			if ( ( process_flags & process_audio ) != 0 && ( !got_audio && audio_.size( ) > 0 ) )
			{
				int first = audio_[ 0 ]->position( );
				int last = audio_[ audio_.size( ) - 1 ]->position( );
				got_audio = current >= first && current <= last;
			}

			return got_audio;
		}

		bool is_valid( )
		{
			return context_ != 0;
		}

		// Analyse streams and set input values
		void populate( )
		{
			// Iterate through all the streams available
			for( size_t i = 0; i < context_->nb_streams; i++ ) 
			{
				// Get the codec context
   				AVCodecContext *codec_context = context_->streams[ i ]->codec;

				// Ignore streams that we don't have a decoder for
				if ( avcodec_find_decoder( codec_context->codec_id ) == NULL )
					continue;
		
				// Determine the type and obtain the first index of each type
   				switch( codec_context->codec_type ) 
				{
					case CODEC_TYPE_VIDEO:
						video_indexes_.push_back( i );
						break;
					case CODEC_TYPE_AUDIO:
						audio_indexes_.push_back( i );
		   				break;
					default:
		   				break;
				}
			}

			// Configure video input properties
			if ( has_video( ) )
			{
				AVStream *stream = get_video_stream( ) ? get_video_stream( ) : context_->streams[ 0 ];

				width_ = stream->codec->width;
				height_ = stream->codec->height;

				// TODO: Need to check this one
				if ( stream->r_frame_rate.num != 0 && stream->r_frame_rate.den != 0 )
				{
					fps_num_ = stream->r_frame_rate.num;
					fps_den_ = stream->r_frame_rate.den;
				}
				else
				{
					fps_num_ = stream->codec->time_base.den;
					fps_den_ = stream->codec->time_base.num;
				}
			}
			else if ( has_audio( ) )
			{
				if ( prop_fps_num_.value< int >( ) > 0 && prop_fps_den_.value< int >( ) > 0 )
				{
					fps_num_ = prop_fps_num_.value< int >( );
					fps_den_ = prop_fps_den_.value< int >( );
				}
				else
				{
					fps_num_ = 25;
					fps_den_ = 1;
				}
				sar_num_ = 1;
				sar_den_ = 1;
			}

			// Open the video and audio codecs
			if ( has_video( ) ) open_video_codec( );
			if ( has_audio( ) ) open_audio_codec( );

			if ( uint64_t( context_->start_time ) != AV_NOPTS_VALUE )
				start_time_ = context_->start_time;

			// Set the duration
			if ( uint64_t( context_->duration ) != AV_NOPTS_VALUE )
				frames_ = int( ( avformat_input::fps( ) * ( context_->duration - start_time_ ) ) / ( double )AV_TIME_BASE );
			else
				frames_ = 1 << 31;

			std::string format = context_->iformat->name;

			// Work around for inefficiencies on I frame only seeking
			// - this should be covered by the AVStream discard and need_parsing values
			//   but they're either wrong or poorly documented...
			if ( has_video( ) )
			{
				std::string codec( get_video_stream( )->codec->codec->name );
				if ( codec == "mjpeg" )
					must_decode_ = false;
				else if ( codec == "dvvideo" )
					must_decode_ = false;
				else if ( codec == "rawvideo" )
					must_decode_ = false;
				else if ( codec == "dirac" )
					must_reopen_ = true;

				if ( format == "mpeg" && prop_gop_size_.value< int >( ) == -1 )
					prop_gop_size_ = 24;
				else if ( format == "mp3" && prop_gop_size_.value< int >( ) == -1 )
					prop_gop_size_ = 12;
				else if ( format == "mp2" && prop_gop_size_.value< int >( ) == -1 )
					prop_gop_size_ = 12;
				else if ( format == "mpegts" && prop_gop_size_.value< int >( ) == -1 )
					prop_gop_size_ = 24;
				else if ( prop_gop_size_.value< int >( ) == -1 )
					prop_gop_size_ = 0;

				if ( format == "mpeg" || ( format == "mpegts" && codec == "h264" ) )
					h264_hack_ = true;
			}
			else
			{
				if ( prop_gop_size_.value< int >( ) == -1 )
					prop_gop_size_ = 50;
			}

			// Determine the number of frames in the media
			if ( prop_frames_.value< int >( ) != -1 )
				frames_ = prop_frames_.value< int >( );
		}

		bool should_size_media( std::string format )
		{
			bool result = is_seekable_ && prop_genpts_.value< int >( ) == 0;

			if ( result && has_video( ) )
			{
				std::string codec( get_video_stream( )->codec->codec->name );

				if ( format == "avi" )
					result = codec == "dvvideo";
				else if ( format == "image2" )
					result = false;
				else if ( format == "dv" )
					result = false;
				else if ( format == "mxf" )
					result = false;
				else if ( format == "mkv" )
					result = false;
				else if ( format == "flv" )
					result = false;
				else if ( format == "divx" )
					result = false;
			}
			else if ( result && has_audio( ) )
			{
				result = format != "wav";
			}

			return result;
		}

		int size_media_by_images( )
		{
			int last = 0;
			seek( frames_ - 1 );
			fetch( );

			do
			{
				if ( images_.size( ) )
				{
					last = images_[ images_.size( ) - 1 ]->position( ) - first_found_;
					frames_ = last + 3;
					seek( last + 2 );
				}
				else
				{
					frames_ -= 25;
					seek( frames_ - 1 );
				}

				fetch( );
			}
			while( frames_ > 0 && ( images_.size( ) == 0 || last != images_[ images_.size( ) - 1 ]->position( ) - first_found_ ) );

			if ( frames_ <= 0 )
				last = -1;

			return last + 1;
		}

		int size_media_by_packets( )
		{
			std::string format = context_->iformat->name;

			AVStream *stream = get_video_stream( ) ? get_video_stream( ) : get_audio_stream( );

			int max = frames_;

			seek( frames_ );
			seek_to_position( );

			av_init_packet( &pkt_ );
			while ( stream && av_read_frame( context_, &pkt_ ) >= 0 )
			{
				if ( is_audio_stream( pkt_.stream_index ) )
				{
					bool fallback = true;

					// Handle the custom frame rate if specified
					if ( prop_fps_num_.value< int >( ) == fps_num_ && prop_fps_den_.value< int >( ) == fps_den_ )
					{
						if ( samples_per_packet_ == 0 )
						{
							AVCodecContext *codec_context = get_audio_stream( )->codec;
							int audio_size = audio_buf_size_;
							int len = pkt_.size;
							uint8_t *data = pkt_.data;
		   					if ( avcodec_decode_audio2( codec_context, ( short * )( audio_buf_ ), &audio_size, data, len ) >= 0 )
							{
								samples_per_frame_ = int( double( codec_context->sample_rate * fps_den_ ) / fps_num_ );
								samples_per_packet_ = audio_size / codec_context->channels / 2;
								samples_duration_ = pkt_.duration;
							}
						}

						if ( pkt_.duration > 0 && samples_duration_ == pkt_.duration && samples_per_frame_ > 0 )
						{
							int64_t packet_idx = ( pkt_.dts - av_rescale_q( start_time_, ml_av_time_base_q, stream->time_base ) ) / pkt_.duration;
							int64_t total = ( packet_idx + 1 ) * samples_per_packet_;
							int result = int( total / samples_per_frame_ );
							if ( result > max )
								max = result;
							fallback = false;
						}
						else
						{
							// Make sure we don't try this logic again for this file
							prop_fps_num_ = -1;
							prop_fps_den_ = -1;
							samples_per_packet_ = 0;
						}
					}

					if ( fallback )
					{
						double dts = av_q2d( stream->time_base ) * ( pkt_.dts - av_rescale_q( start_time_, ml_av_time_base_q, stream->time_base ) );
						int result = int( dts * fps( ) + 0.5 ) + 1;
						if ( result > max )
							max = result;
					}
				}

				av_free_packet( &pkt_ );
			}

			frames_ = max > 0 ? max : frames_;
			seek( 0 );

			seek_to_position( );

			return frames_;
		}

		// Returns the current video stream
		inline AVStream *get_video_stream( )
		{
			if ( prop_video_index_.value< int >( ) >= 0 && video_indexes_.size( ) > 0 )
				return context_->streams[ video_indexes_[ prop_video_index_.value< int >( ) ] ];
			else
				return 0;
		}

		// Returns the current audio stream
		inline AVStream *get_audio_stream( )
		{
			if ( prop_audio_index_.value< int >( ) >= 0 && audio_indexes_.size( ) > 0 )
				return context_->streams[ audio_indexes_[ prop_audio_index_.value< int >( ) ] ];
			else
				return 0;
		}

		// Opens the video codec associated to the current stream
		void open_video_codec( )
		{
			AVStream *stream = get_video_stream( );
			AVCodecContext *codec_context = stream->codec;
			video_codec_ = avcodec_find_decoder( codec_context->codec_id );
			if ( video_codec_ == NULL || avcodec_open( codec_context, video_codec_ ) < 0 )
				prop_video_index_ = -1;
		}

		void close_video_codec( )
		{
			AVStream *stream = get_video_stream( );
			if ( stream && stream->codec )
				avcodec_close( stream->codec );
		}
		
		// Opens the audio codec associated to the current stream
		void open_audio_codec( )
		{
			AVStream *stream = get_audio_stream( );
			AVCodecContext *codec_context = stream->codec;
			audio_codec_ = avcodec_find_decoder( codec_context->codec_id );
			if ( audio_codec_ == NULL || avcodec_open( codec_context, audio_codec_ ) < 0 )
				prop_audio_index_ = -1;
		}

		void close_audio_codec( )
		{
			AVStream *stream = get_audio_stream( );
			if ( stream && stream->codec )
				avcodec_close( stream->codec );
		}

		// Seek to the requested frame
		bool seek_to_position( )
		{
			if ( is_seekable_ )
			{
				int position = get_position( );

				if ( must_decode_ )
					position -= prop_gop_size_.value< int >( );

				int64_t offset = int64_t( ( ( double )position / avformat_input::fps( ) ) * AV_TIME_BASE ) + start_time_;
				int64_t byte = -1;

				if ( must_reopen_ )
					reopen( );

				if ( offset < start_time_ ) offset = start_time_;

				// Use the index to get an absolute byte position in the data when available/usable
				if ( aml_index_ && aml_index_->usable( ) )
					byte = aml_index_->find( position );

				std::string format = context_->iformat->name;
				int result = -1;

				if ( byte != -1 )
					result = av_seek_frame( context_, -1, byte, AVSEEK_FLAG_BYTE );
				else if ( h264_hack_ && position < prop_gop_size_.value< int >( ) )
					result = av_seek_frame( context_, -1, context_->data_offset, AVSEEK_FLAG_BYTE );
				else
					result = av_seek_frame( context_, -1, offset, AVSEEK_FLAG_BACKWARD );

				key_search_ = true;

				aud_pos_ = img_pos_ = 0;
				img_inc_ = 0;
				audio_buf_used_ = 0;
				audio_buf_offset_ = 0;

				if ( get_audio_stream( ) )
				{
					avcodec_flush_buffers( get_audio_stream( )->codec );
					close_audio_codec( );
					open_audio_codec( );
				}

				if ( get_video_stream( ) )
					avcodec_flush_buffers( get_video_stream( )->codec );

				return result >= 0;
			}
			else
			{
				return false;
			}
		}

		// Decode the image
		int decode_image( bool &got_picture, AVPacket *packet )
		{
			AVCodecContext *codec_context = get_video_stream( )->codec;

			int ret = 0;
			int got_dummy = 0;
			int got_pict = 0;
			
			// Derive the dts
			double dts = 0;
			if ( packet && uint64_t( packet->dts ) != AV_NOPTS_VALUE )
				dts = av_q2d( get_video_stream( )->time_base ) * ( packet->dts - av_rescale_q( start_time_, ml_av_time_base_q, get_video_stream( )->time_base ) );

			// Approximate frame position
			int position = int( dts * fps( ) + 0.5 );

			if ( position < 0 )
				return 0;

			position += img_inc_;

			if ( ( packet == 0 || position == 0 || prop_genpts_.value< int >( ) || !is_seekable( ) ) && images_.size( ) )
				position = images_[ images_.size( ) - 1 ]->position( ) + 1;

			// Just in case we get a failure in the ts correction
			while( has_video_for( position ) )
				position ++;

			// Ignore packets before 0
			// Small optimisation - abandon packet now if we can (ie: we don't have to decode
			// and no image is requested for this frame)
			if ( !must_decode_ && ( get_process_flags( ) & ml::process_image ) == 0 )
			{
				got_picture = true;
				return ret;
			}

			// We have to continue processing until we get a dts >= to the requested position
			int error = 0;
 			if ( !packet )
				error = avcodec_decode_video( codec_context, av_frame_, &got_pict, NULL, 0 );
 			else if ( position >= get_position( ) + first_found_ )
				error = avcodec_decode_video( codec_context, av_frame_, &got_pict, packet->data, packet->size );
 			else if ( must_decode_ )
				error = avcodec_decode_video( codec_context, av_frame_, &got_dummy, packet->data, packet->size );

			got_picture = got_pict != 0;

			// If we have just done a search, then we need to locate the first key frame
			// all others are discarded.
			if ( key_search_ && av_frame_->key_frame == 0 && position < get_position( ) + first_found_ )
				got_picture = false;
			else
				key_search_ = false;

			il::image_type_ptr image;

			if ( error >= 0 && ( key_search_ == false && ( got_picture || got_dummy ) ) )
				image = image_convert( );

			if ( ts_pusher_ && image )
			{
				frame_type_ptr temp = frame_type_ptr( new frame_type( ) );
				temp->set_position( position );
				temp->set_image( image );
				ts_pusher_->push( temp );
				temp = ts_filter_->fetch( );
				image = temp->get_image( );
				position = temp->get_position( );
				if ( position >= get_position( ) + first_found_ ) got_picture = true;
			}

			// Store the image in the queue
			if ( ( got_picture || got_dummy ) && position >= 0 && image )
			{
				if ( position >= get_position( ) + first_found_ - prop_gop_cache_.value< int >( ) )
					store_image( image, position );
				else
					img_pos_ = position;
				if ( aud_pos_ == -1 )
					sync_diff_ = 0;
				else if ( sync_diff_ == 0 )
					sync_diff_ = aud_pos_ - img_pos_;
				if ( !prop_gop_size_.value< int >( ) && aud_pos_ != -1 && sync_diff_ > 0 && aud_pos_ - img_pos_ > sync_diff_ )
				{
					if ( ++ position >= get_position( ) + first_found_ - prop_gop_cache_.value< int >( ) )
						store_image( image, position );
					else
						img_pos_ = position;
					img_inc_ += 1;
				}
			}
			//if ( position - first_found_ >= frames_ )
				//frames_ = position - first_found_ + 1;

			return ret;
		}

		il::image_type_ptr image_convert( )
		{
			AVStream *stream = get_video_stream( );
			AVCodecContext *codec_context = stream->codec;
			int width = get_width( );
			int height = get_height( );

			return convert_to_oil( av_frame_, codec_context->pix_fmt, width, height );
		}

		void store_image( il::image_type_ptr image, int position )
		{
			if ( first_frame_ )
			{
				first_found_ = position - get_position( );
				first_frame_ = false;
			}

			image->set_position( position );

			img_pos_ = position;

			if ( images_.size( ) > 0 )
			{
				int first = images_[ 0 ]->position( );
				int last = images_[ images_.size( ) - 1 ]->position( );
				if ( position < first && position < last )
					images_.clear( );
				else if ( first < get_position( ) + first_found_ - prop_gop_cache_.value< int >( ) )
					images_.erase( images_.begin( ) );
			}

			if ( av_frame_->interlaced_frame )
				image->set_field_order( av_frame_->top_field_first ? il::top_field_first : il::bottom_field_first );

			if ( image )
			{
				image->set_writable( false );
				images_.push_back( image );
			}
		}

		inline int samples_for_frame( int frequency, int index )
		{
			return ml::audio_samples_for_frame( index, frequency, fps_num_, fps_den_ );
		}

		int decode_audio( bool &got_audio )
		{
			int ret = 0;

			// Get the audio codec context
			AVCodecContext *codec_context = get_audio_stream( )->codec;

			// The number of bytes in the packet
			int len = pkt_.size;
			uint8_t *data = pkt_.data;

			// This is the pts of the packet
			int found = 0;
			double dts = av_q2d( get_audio_stream( )->time_base ) * ( pkt_.dts - av_rescale_q( start_time_, ml_av_time_base_q, get_audio_stream( )->time_base ) );
			int64_t packet_idx = 0;

			if ( uint64_t( pkt_.dts ) != AV_NOPTS_VALUE )
			{
		   		if ( pkt_.duration > 0 && samples_duration_ == pkt_.duration && samples_per_packet_ > 0 )
				{
					packet_idx = ( pkt_.dts - av_rescale_q( start_time_, ml_av_time_base_q, get_audio_stream( )->time_base ) ) / pkt_.duration;
					int64_t total = packet_idx * samples_per_packet_;
					found = int( total / samples_per_frame_ );
				}
				else
				{
					found = int( dts * avformat_input::fps( ) );
				}
			}

			// Ignore packets before 0
			if ( found < 0 )
				return 0;

			// Get the audio info from the codec context
			int channels = codec_context->channels;
			int frequency = codec_context->sample_rate;
			int bps = 2;
			int skip = 0;

			if ( audio_buf_used_ == 0 )
			{
				if ( audio_.size( ) != 0 )
				{
					audio_buf_offset_ = ( *( audio_.end( ) - 1 ) )->position( ) + 1;
				}
				else
				{
					int64_t samples_to_now = 0;
					int64_t samples_to_next = 0;

					if ( pkt_.duration && samples_per_frame_ > 0 )
						samples_to_now = int64_t( samples_per_packet_ * packet_idx );
					else
						samples_to_now = int64_t( dts * frequency + 0.5 );

					samples_to_next = ml::audio_samples_to_frame( found, frequency, fps_num_, fps_den_ );

					if ( samples_to_now < samples_to_next )
					{
						skip = int( samples_to_next - samples_to_now ) * channels * bps;
					}
#if 0
					else
					{
						audio_buf_used_ = size_t( samples_to_dts - samples_to_next ) * channels * bps;
						if ( audio_buf_used_ > 0 )
							memset( audio_buf_, 0, audio_buf_used_ );
					}
#endif

					audio_buf_offset_ = found;
				}
			}

			// Each packet may need multiple parses
			while( len > 0 )
			{
				int audio_size = audio_buf_size_;

				// Decode the audio into the buffer
		   		ret = avcodec_decode_audio2( codec_context, ( short * )( audio_buf_temp_ ), &audio_size, data, len );

				// If no samples are returned, then break now
				if ( ret < 0 )
				{
					ret = 0;
					got_audio = true;
					break;
				}

				// Copy the new samples to the main buffer
				memcpy( audio_buf_ + audio_buf_used_, audio_buf_temp_, audio_size );

				// Decrement the length by the number of bytes parsed
				len -= ret;
				data += ret;

				// Increment the number of bytes used in the buffer
				if ( audio_size > 0 )
					audio_buf_used_ += audio_size;

				// Current index in the buffer
				int index = 0;

				if ( audio_buf_used_ > skip )
				{
					index = skip;
					skip = 0;
				}
				else
				{
					skip -= audio_buf_used_;
					audio_buf_used_ = 0;
				}

				// Now we need to split the audio up into frame sized dollops
				while( 1 )
				{
					int samples = samples_for_frame( frequency, audio_buf_offset_ );

					int bytes_for_frame = samples * channels * bps;

					if ( bytes_for_frame > ( audio_buf_used_ - index ) )
						break;

					// Create an audio frame
					index += store_audio( audio_buf_offset_, audio_buf_ + index, samples );

					if ( audio_buf_offset_ >= get_position( ) + first_found_ )
						got_audio = 1;

					//if ( audio_buf_offset_ - first_found_ >= frames_ )
						//frames_ = audio_buf_offset_ - first_found_ + 1;

					audio_buf_offset_ += 1;
				}

				audio_buf_used_ -= index;
				if ( audio_buf_used_ && index != 0 )
					memmove( audio_buf_, audio_buf_ + index, audio_buf_used_ );
			}

			return ret;
		}

		int store_audio( int position, uint8_t *buf, int samples )
		{
			// Get the audio codec context
			AVCodecContext *codec_context = get_audio_stream( )->codec;

			// Get the audio info from the codec context
			int channels = codec_context->channels;
			int frequency = codec_context->sample_rate;

			if ( first_frame_ )
			{
				first_found_ = position - get_position( );
				first_frame_ = false;
			}

			audio_type_ptr aud = audio_type_ptr( new audio_type( pcm16_audio_type( frequency, channels, samples ) ) );
			aud->set_position( position );
			memcpy( aud->data( ), buf, aud->size( ) );

			if ( audio_.size( ) > 0 )
			{
				int first = audio_[ 0 ]->position( );
				int last = audio_[ audio_.size( ) - 1 ]->position( );
				if ( position != last + 1 )
					audio_.clear( );
				else if ( first < get_position( ) + first_found_ - prop_gop_cache_.value< int >( ) )
					audio_.erase( audio_.begin( ) );
			}

			audio_.push_back( aud );

			if ( position >= get_position( ) )
				aud_pos_ = position;

			return aud->size( );
		}

		void clear_stores( bool force = false )
		{
			bool clear = true;

			if ( !force && images_.size( ) )
			{
				int current = get_position( ) + first_found_;
				int first = images_[ 0 ]->position( );
				int last = images_[ images_.size( ) - 1 ]->position( );
				if ( current >= first && current <= last )
					clear = false;
			}

			if ( clear )
			{
				images_.clear( );
				audio_.clear( );
				audio_buf_used_ = 0;
			}
		}

		bool find_image( frame_type_ptr &frame )
		{
			bool exact = false;
			int current = get_position( ) + first_found_;
			int closest = 1 << 16;
			std::deque< il::image_type_ptr >::iterator result = images_.end( );
			std::deque< il::image_type_ptr >::iterator iter;

			for ( iter = images_.begin( ); iter != images_.end( ); iter ++ )
			{
				il::image_type_ptr img = *iter;
				int diff = current - img->position( );
				if ( std::abs( diff ) <= closest )
				{
					result = iter;
					closest = std::abs( diff );
				}
				else if ( img->position( ) > current )
				{
					break;
				}
			}

			if ( result != images_.end( ) )
			{
				frame->set_image( *result );
				exact = ( *result )->position( ) == current;
			}

			return exact;
		}

		bool find_audio( frame_type_ptr &frame )
		{
			bool exact = false;
			int current = get_position( ) + first_found_;
			int closest = 1 << 16;
			std::deque< audio_type_ptr >::iterator result = audio_.end( );
			std::deque< audio_type_ptr >::iterator iter;

			for ( iter = audio_.begin( ); iter != audio_.end( ); iter ++ )
			{
				audio_type_ptr aud = *iter;
				int diff = current - aud->position( );
				if ( std::abs( diff ) <= closest )
				{
					result = iter;
					closest = std::abs( diff );
				}
				else if ( aud->position( ) > current )
				{
					break;
				}
			}

			if ( result != audio_.end( ) && ( *result )->position( ) == current )
			{
				frame->set_audio( ml::audio_type_ptr( ( *result )->clone( ) ) );
				frame->set_duration( double( ( *result )->samples( ) ) / double( ( *result )->frequency( ) ) );
				exact = true;
			}
			else
			{
				AVCodecContext *codec_context = get_audio_stream( )->codec;
				int channels = codec_context->channels;
				int frequency = codec_context->sample_rate;
				int samples = samples_for_frame( frequency, current );
				audio_type_ptr aud = audio_type_ptr( new audio_type( pcm16_audio_type( frequency, channels, samples ) ) );
				aud->set_position( current );
				memset( aud->data( ), 0, aud->size( ) );
				frame->set_audio( aud );
				frame->set_duration( double( samples ) / double( frequency ) );
			}

			return exact;
		}

		void reopen( )
		{
			if ( prop_video_index_.value< int >( ) >= 0 )
				close_video_codec( );
			if ( prop_audio_index_.value< int >( ) >= 0 )
				close_audio_codec( );
			if ( context_ )
				av_close_input_file( context_ );

			// Attempt to open the resource
			int error = av_open_input_file( &context_, opl::to_string( uri_ ).c_str( ), format_, 0, params_ ) < 0;

			// Get the stream info
			if ( error == 0 )
				error = av_find_stream_info( context_ ) < 0;
			
			// Populate the input properties
			if ( error == 0 )
				populate( );
		}

	private:
		opl::wstring uri_;
		opl::wstring mime_type_;
		int frames_;
		bool is_seekable_;
		int first_frame_;
		int first_found_;
		int fps_num_;
		int fps_den_;
		int sar_num_;
		int sar_den_;
		int width_;
		int height_;

		AVFormatContext *context_;
		AVInputFormat *format_;
		AVFormatParameters *params_;
		pcos::property prop_video_index_;
		pcos::property prop_audio_index_;
		pcos::property prop_gop_size_;
		pcos::property prop_gop_cache_;
		pcos::property prop_format_;
		pcos::property prop_genpts_;
		pcos::property prop_frames_;
		pcos::property prop_file_size_;
		pcos::property prop_estimate_;
		pcos::property prop_fps_num_;
		pcos::property prop_fps_den_;
		pcos::property prop_ts_filter_;
		pcos::property prop_ts_index_;
		std::vector < int > audio_indexes_;
		std::vector < int > video_indexes_;
		int expected_;
		AVFrame *av_frame_;
		AVCodec *video_codec_;
		AVCodec *audio_codec_;
		AVPacket pkt_;
		std::deque < il::image_type_ptr > images_;
		std::deque < audio_type_ptr > audio_;
		bool must_decode_;
		bool must_reopen_;
		bool key_search_;

		boost::uint8_t *audio_buf_;
		boost::uint8_t *audio_buf_temp_;
		int audio_buf_size_;
		int audio_buf_used_;
		int audio_buf_offset_;

		int img_pos_;
		int aud_pos_;
		int img_inc_;
		int sync_diff_;

		int64_t start_time_;
		struct SwsContext *img_convert_;
		bool h264_hack_;
		ml::frame_type_ptr last_frame_;

		int samples_per_frame_;
		int samples_per_packet_;
		int samples_duration_;

		input_type_ptr ts_pusher_;
		filter_type_ptr ts_filter_;

		mutable aml_index *aml_index_;
		std::string used_name_;
};


class ML_PLUGIN_DECLSPEC avformat_resampler_filter : public filter_type
{
	public:
		// filter_type overloads
		explicit avformat_resampler_filter(const opl::wstring &)
			: filter_type()
			, prop_output_channels_(pcos::key::from_string("channels"))
			, prop_output_sample_freq_(pcos::key::from_string("frequency"))
			, prop_enable_( pcos::key::from_string( "enable" ) )
			, input_channels_(2)
			, input_sample_freq_(48000)
			, fps_numerator_(25)
			, fps_denominator_(1)
			, context_(NULL)
			, dirty_(true)
			, in_buffer_( )
			, out_buffer_( )
		{
			property_observer_ = boost::shared_ptr<pcos::observer>(new avformat_resampler_filter::property_observer(const_cast<avformat_resampler_filter*>(this)));
		
			properties( ).append( prop_output_channels_ = 2	);
			properties( ).append( prop_output_sample_freq_ = 48000 );
			properties( ).append( prop_enable_ = 1 );
		
			prop_output_channels_.attach( property_observer_ );
			prop_output_sample_freq_.attach( property_observer_ );
		}
	
		virtual ~avformat_resampler_filter()
		{
			if(context_)
				audio_resample_close(context_);
		
			prop_output_channels_.detach(property_observer_);
			prop_output_sample_freq_.detach(property_observer_);
		}
	
		virtual const opl::wstring get_uri( ) const { return L"resampler"; }

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		virtual void on_slot_change( ml::input_type_ptr input, int )
		{
			if ( context_ )
			{
				audio_resample_close(context_);
				context_ = NULL;
			}
		}

	protected:
		void do_fetch( frame_type_ptr &current_frame )
		{
			// Cache a local copy of current position
			int current_pos = get_position();
		
			// Ensure the cache is full
			current_frame = fetch_from_slot( 0 );

			if ( prop_enable_.value< int >( ) == 0 || !current_frame )
				return;
		
			ml::audio_type_ptr current_audio = current_frame->get_audio();
			if(!current_audio)
				return;

			// Small hack to allow resampler for channel conversion only
			if(	prop_output_sample_freq_.value<int>( ) == -1 )
			{
				current_audio = audio_channel_convert( current_audio, prop_output_channels_.value<int>( ) );
				if ( current_audio )
					current_frame->set_audio( current_audio );
				return;
			}

			// since ffmpeg resampler will only accept 2 channels or less, force any input with more than 2 channels to conform
			if(current_audio->channels() > 2)
			{
				current_audio = audio_channel_convert(current_audio, 2);
				current_frame->set_audio( current_audio );
			}

			//-----------------------------------------------------------------------------------
			// Check there is a context to work with, if not create one
			//-----------------------------------------------------------------------------------
			if( !context_ )
			{
				current_frame->get_fps(fps_numerator_, fps_denominator_);
				input_channels_		= current_audio->channels();
				input_sample_freq_	= current_audio->frequency();
				dirty_ = true;
			}
			else 
			{
				dirty_ = fps_numerator_ != current_frame->get_fps_num( ) || 
					  fps_denominator_ != current_frame->get_fps_den( ) ||
					  input_channels_ != current_audio->channels( ) ||
					  input_sample_freq_ != current_audio->frequency( );
			}
			
			// If the sample frequencies are the same save some effort
			if(		(input_sample_freq_	== prop_output_sample_freq_.value<int>())
				&&	(input_channels_	== prop_output_channels_.value<int>()) )
				return;
	
			if(dirty_)
			{
				if(context_)
				{
					audio_resample_close(context_);
					context_ = NULL;
				}
				
				context_ = av_audio_resample_init(	prop_output_channels_.value<int>(), 
													input_channels_,
													prop_output_sample_freq_.value<int>(),
													input_sample_freq_,
													SAMPLE_FMT_S16, SAMPLE_FMT_S16,
													16, 10, 0, 0.8 );
				
				if(!context_)
					return;
				
				dirty_ = false;
			}
		
			//----------------------------------------------------------------------------------
			// Prepare a buffer of input data to be passed to ffmpeg's resampler for resampling
			//----------------------------------------------------------------------------------
			int input_samples_per_chan = current_audio->samples();
			
			// Allocate buffer memory
			size_t in_buffer_size = input_channels_ * input_samples_per_chan * sizeof(short);
			if ( in_buffer_.size( ) < in_buffer_size )
				in_buffer_.resize( in_buffer_size );
			
			// Appropriately copy data from cached audio for previous, current & next frames to buffer
			int offset = 0;
			int tmp = 0;
			
			tmp = input_channels_ * current_audio->samples();
			memcpy((void*)(&in_buffer_[ offset ]), (void*)current_audio->data(), tmp * sizeof(short));
			offset += tmp;
			
			// Determine size of output buffer - can be a little bigger than necessary, but can never be too small!
			int estimated_output_samples_per_chan = 0;
			if(input_sample_freq_ < prop_output_sample_freq_.value<int>())
				estimated_output_samples_per_chan = (int)((double(fps_denominator_)/fps_numerator_) * prop_output_sample_freq_.value<int>() * ((current_pos == 0 || current_pos >= get_frames() - 1) ? 2 : 3)) + 1;
			else
				estimated_output_samples_per_chan = (int)((input_samples_per_chan * prop_output_sample_freq_.value<int>() / input_sample_freq_) * (100 + (10 * prop_output_channels_.value<int>()))/100) + 1;
			
			// Allocate the memory
			size_t out_buffer_size = prop_output_channels_.value<int>() * estimated_output_samples_per_chan * sizeof(short);
			if ( out_buffer_.size( ) < out_buffer_size * 2 )
				out_buffer_.resize( out_buffer_size * 2 );
			
			// use ffmpeg to resample data
			int output_samples = audio_resample(context_, &out_buffer_[ 0 ], &in_buffer_[ 0 ], input_samples_per_chan);
			
			// Create a new audio object to hold the determined portion of the output buffer
			audio_type_ptr output_audio = audio_type_ptr(new audio_type(audio<unsigned char, pcm16>(
																			prop_output_sample_freq_.value<int>(), 
																			prop_output_channels_.value<int>(), 
																			output_samples) ) );
			if(!output_audio)
				return;
		
			// Copy data from output buffer to audio object
			memcpy(	(void*)output_audio->data(), 
					(void*)(&out_buffer_[ 0 ]), 
					output_samples * prop_output_channels_.value<int>() * sizeof(short));
	
			// Set current frame to have this new audio object, which now holds the resampled audio data
			current_frame->set_audio(output_audio);
		}
	
	private:
		class property_observer : public pcos::observer
		{
		public:
			property_observer(avformat_resampler_filter* filter)
				: filter_(filter)
			{
			}
	
			void updated(pcos::isubject*)
			{
				filter_->dirty_ = true;
			}
	
			private:
				avformat_resampler_filter* filter_;
		};
	
		boost::shared_ptr<pcos::observer> property_observer_;
		
		pcos::property	prop_output_channels_;
		pcos::property	prop_output_sample_freq_;
		pcos::property	prop_enable_;
	
		int	input_channels_,
			input_sample_freq_,
			fps_numerator_,
			fps_denominator_;
	
		ReSampleContext* 	context_;
		bool				dirty_;
	
		std::vector< short > in_buffer_;
		std::vector< short > out_buffer_;
};

//
// Plugin object
//

class ML_PLUGIN_DECLSPEC avformat_plugin : public openmedialib_plugin
{
public:

	virtual input_type_ptr input( const opl::wstring &resource )
	{
 		typedef boost::shared_ptr< avformat_input > result_type_ptr;
		return result_type_ptr( new avformat_input( resource ) );
	}

	virtual store_type_ptr store( const opl::wstring &resource, const frame_type_ptr &frame )
	{
 		typedef boost::shared_ptr< avformat_store > result_type_ptr;
		return result_type_ptr( new avformat_store( resource, frame ) );
	}
	
	virtual filter_type_ptr filter( const opl::wstring& resource )
	{
		return filter_type_ptr( new avformat_resampler_filter( resource ) );
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
        		oml::ml::avctx_opts[ i ]= avcodec_alloc_context2( CodecType( i ) );
			oml::ml::avformat_opts = avformat_alloc_context( );
			av_log_set_level( -1 );
			oml::ml::register_dv_decoder( );
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
	
	ML_PLUGIN_DECLSPEC bool openplugin_create_plugin( const char*, opl::openplugin** plug )
	{
		*plug = new plugin::avformat_plugin;
		return true;
	}
	
	ML_PLUGIN_DECLSPEC void openplugin_destroy_plugin( opl::openplugin* plug )
	{ 
		delete static_cast< plugin::avformat_plugin * >( plug ); 
	}
}

