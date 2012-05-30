// avformat - A avformat plugin to ml.
//
// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.
//
// #store:avformat:
//
// Encodes many video, audio and image formats via the libavformat API.

#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/packet.hpp>
#include <openmedialib/ml/awi.hpp>
#include <openpluginlib/pl/pcos/isubject.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>

#include <vector>
#include <algorithm>

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

extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/url.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
}

#define MAX_AUDIO_PACKET_SIZE (128 * 1024)

namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;
namespace pcos = olib::openpluginlib::pcos;

namespace olib { namespace openmedialib { namespace ml {

extern std::map< CodecID, std::string > codec_name_lookup_;
extern std::map< std::string, CodecID > name_codec_lookup_;

extern const std::wstring avformat_to_oil( int );
extern const PixelFormat oil_to_avformat( const std::wstring & );
extern il::image_type_ptr convert_to_oil( AVFrame *, PixelFormat, int, int );

static const pl::pcos::key key_pts_ = pl::pcos::key::from_string( "pts" );
static const pl::pcos::key key_dts_ = pl::pcos::key::from_string( "dts" );
static const pl::pcos::key key_has_b_frames_ = pl::pcos::key::from_string( "has_b_frames" );
static const pl::pcos::key key_timebase_num_ = pl::pcos::key::from_string( "timebase_num" );
static const pl::pcos::key key_timebase_den_ = pl::pcos::key::from_string( "timebase_den" );
static const pl::pcos::key key_duration_ = pl::pcos::key::from_string( "duration" );

static const pl::pcos::key key_codec_id_ = pcos::key::from_string( "codec_id" );
static const pl::pcos::key key_codec_type_ = pcos::key::from_string( "codec_type" );
static const pl::pcos::key key_codec_tag_ = pcos::key::from_string( "codec_tag" );
static const pl::pcos::key key_max_rate_ = pl::pcos::key::from_string( "max_rate" );
static const pl::pcos::key key_buffer_size_ = pl::pcos::key::from_string( "buffer_size" );
static const pl::pcos::key key_bit_rate_ = pcos::key::from_string( "bit_rate" );
static const pl::pcos::key key_field_order_ = pcos::key::from_string( "field_order" );
static const pl::pcos::key key_bits_per_coded_sample_ = pcos::key::from_string( "bits_per_coded_sample" );
static const pl::pcos::key key_ticks_per_frame_ = pcos::key::from_string( "ticks_per_frame" );
static const pl::pcos::key key_avg_fps_num_ = pcos::key::from_string( "avg_fps_num" );
static const pl::pcos::key key_avg_fps_den_ = pcos::key::from_string( "avg_fps_den" );

static const AVRational ml_av_time_base_q = { 1, AV_TIME_BASE };

class ML_PLUGIN_DECLSPEC avformat_store : public store_type
{
	public:
		avformat_store( const pl::wstring &resource, const frame_type_ptr &frame )
			: store_type( )
			, resource_( resource )
			, prop_enable_audio_( pcos::key::from_string( "enable_audio" ) )
			, prop_enable_video_( pcos::key::from_string( "enable_video" ) )
			, oc_( 0 )
			, fmt_( 0 )
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
			//, prop_qns_( pcos::key::from_string( "qns" ) )
			, prop_video_qmin_( pcos::key::from_string( "video_qmin" ) )
			, prop_video_qmax_( pcos::key::from_string( "video_qmax" ) )
			, prop_video_lmin_( pcos::key::from_string( "video_lmin" ) )
			, prop_video_lmax_( pcos::key::from_string( "video_lmax" ) )
			//, prop_video_mb_qmin_( pcos::key::from_string( "video_mb_qmin" ) )
			//, prop_video_mb_qmax_( pcos::key::from_string( "video_mb_qmax" ) )
			, prop_video_qdiff_( pcos::key::from_string( "video_qdiff" ) )
			, prop_video_qblur_( pcos::key::from_string( "video_qblur" ) )
			, prop_video_qcomp_( pcos::key::from_string( "video_qcomp" ) )
			//, prop_mux_rate_( pcos::key::from_string( "mux_rate" ) )
			//, prop_mux_preload_( pcos::key::from_string( "mux_preload" ) )
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
			, prop_trellis_( pcos::key::from_string( "trellis" ) )
			, prop_4mv_( pcos::key::from_string( "4mv" ) )
			, prop_thread_count_( pcos::key::from_string( "thread_count" ) )
			, prop_context_( pcos::key::from_string( "context" ) )
			, prop_predictor_( pcos::key::from_string( "predictor" ) )
			, prop_pass_( pcos::key::from_string( "pass" ) )
			, prop_pass_log_( pcos::key::from_string( "pass_log" ) )
			, prop_ts_index_( pcos::key::from_string( "ts_index" ) )
			, prop_ts_auto_( pcos::key::from_string( "ts_auto" ) )
			, prop_audio_split_( pcos::key::from_string( "audio_split" ) )
			, prop_frag_key_frame_( pcos::key::from_string( "frag_key_frame" ) )
			, ts_generator_video_( 1 )
			, ts_generator_audio_( 2, false ) //Will not write index header/footer
			, ts_context_( 0 )
			, ts_last_position_( -1 )
			, ts_last_offset_( 0 )
			, log_file_( 0 )
			, log_( 0 )
			, frame_pos_( 0 )
			, push_count_( 0 )
			, last_audio_( -1 )
			, audio_packet_num_( 0 )
			, audio_variable_( false )
			, first_frame_( frame )
			, video_copy_( false )
		{
			// Show stats
			properties( ).append( prop_show_stats_ = 0 );
			properties( ).append( prop_debug_ = 0 );

			// Define the audio output buffer
			audio_outbuf_size_ = MAX_AUDIO_PACKET_SIZE;
			audio_outbuf_ptr_ = ( uint8_t * )av_malloc( audio_outbuf_size_ + 64 );
			audio_outbuf_ = audio_outbuf_ptr_ + ( 64 - ( boost::int64_t( audio_outbuf_ptr_ ) % 64 ) );
			audio_tmpbuf_ptr_ = ( uint8_t * )av_malloc( audio_outbuf_size_ + 64 );
			audio_tmpbuf_ = audio_tmpbuf_ptr_ + ( 64 - ( boost::int64_t( audio_tmpbuf_ptr_ ) % 64 ) );

			// Extract rules from the frame
			// TODO: Ensure this doesn't cause a foul up...
			properties( ).append( prop_enable_audio_ = frame->get_audio( ) != 0 ? 1 : 0 );
			properties( ).append( prop_enable_video_ = frame->has_image( ) != 0 ? 1 : 0 );

			// Extract video related info from frame
			properties( ).append( prop_width_ = 0 );
			properties( ).append( prop_height_ = 0 );
			properties( ).append( prop_aspect_ratio_ = 1.0 );
			properties( ).append( prop_fps_num_ = 25 );
			properties( ).append( prop_fps_den_ = 1 );

			if ( prop_enable_video_.value< int >( ) == 1 )
			{
				prop_width_ = frame->width( );
				prop_height_ = frame->height( );
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
			properties( ).append( prop_format_ = pl::wstring( L"" ) );
			properties( ).append( prop_acodec_ = pl::wstring( L"" ) );
			properties( ).append( prop_vcodec_ = pl::wstring( L"" ) );
			properties( ).append( prop_vfourcc_ = pl::wstring( L"" ) );
			properties( ).append( prop_afourcc_ = pl::wstring( L"" ) );
			properties( ).append( prop_pix_fmt_ = frame->has_image( ) ? frame->pf( ) : pl::wstring( L"" ) );

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
			//properties( ).append( prop_qns_ = 0 );
			properties( ).append( prop_video_qmin_ = 2 );
			properties( ).append( prop_video_qmax_ = 31 );
			properties( ).append( prop_video_lmin_ = 2 * FF_QP2LAMBDA );
			properties( ).append( prop_video_lmax_ = 31 * FF_QP2LAMBDA );
			//properties( ).append( prop_video_mb_qmin_ = 2 );
			//properties( ).append( prop_video_mb_qmax_ = 31 );
			properties( ).append( prop_video_qdiff_ = 3 );
			properties( ).append( prop_video_qblur_ = 0.5 );
			properties( ).append( prop_video_qcomp_ = 0.5 );
			//properties( ).append( prop_mux_rate_ = 0 );
			//properties( ).append( prop_mux_preload_ = 0.5 );
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
			properties( ).append( prop_trellis_ = 0 );
			properties( ).append( prop_4mv_ = 0 );
			properties( ).append( prop_thread_count_ = 1 );

			properties( ).append( prop_context_ = 0 );
			properties( ).append( prop_predictor_ = 0 );
			properties( ).append( prop_pass_ = 0 );
			properties( ).append( prop_pass_log_ = pl::wstring( L"oml_avformat.log" ) );
			properties( ).append( prop_ts_index_ = pl::wstring( L"" ) );
			properties( ).append( prop_ts_auto_ = 0 );
			properties( ).append( prop_audio_split_ = 0 );
			properties( ).append( prop_frag_key_frame_ = 0 );
		}

		virtual ~avformat_store( )
		{
			if ( oc_ )
			{
				if ( video_stream_ && !video_copy_ )
				{
					int out_size = 0;
					do 
					{
						do_video_encode( true, &out_size );
					} 
					while(out_size > 0);
				}
				else if ( video_stream_ && video_copy_ )
				{
					while( video_queue_.size( ) )
					{
						frame_type_ptr frame = *( video_queue_.begin( ) );
						video_queue_.pop_front( );
						do_stream_write( frame );
					}
				}

				// Close the container
				close_container( );

				// Clean up the codecs
				if ( !video_copy_ )
					close_video_codec( );
				close_audio_codec( );

				// Free the streams
				for( size_t i = 0; i < oc_->nb_streams; i++ )
					av_freep( &oc_->streams[ i ] );

				// Free the context
				av_free( oc_ );
			}

			// Clean up the allocated image
			if ( video_stream_ && !video_copy_ )
				av_free( av_image_.data[0] );

			// Clean up audio buffer
			av_free( audio_outbuf_ptr_ );
			av_free( audio_tmpbuf_ptr_ );

			// Clean up video buffer
			av_free( video_outbuf_ );
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
					log_file_ = fopen( pl::to_string( prop_pass_log_.value< pl::wstring >( ) ).c_str( ), "w" );
					ret = log_file_ != 0;
					if ( !ret )
						fprintf( stderr, "Unable to open log file for write %s\n", pl::to_string( prop_pass_log_.value< pl::wstring >( ) ).c_str( ) );
				}
				else if ( prop_pass_.value< int >( ) == 2 )
				{
					FILE *f = fopen( pl::to_string( prop_pass_log_.value< pl::wstring >( ) ).c_str( ), "r" );
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
							fprintf( stderr, "Unable to allocate log for %s\n", pl::to_string( prop_pass_log_.value< pl::wstring >( ) ).c_str( ) );
						}
						fclose( f );
					}
					else
					{
						fprintf( stderr, "Unable to open log file for read %s\n", pl::to_string( prop_pass_log_.value< pl::wstring >( ) ).c_str( ) );
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
					add_audio_streams( audio_codec );

				bitstream_filters_.resize( oc_->nb_streams );

				strncpy(oc_->filename, pl::to_string( resource( ) ).c_str( ), sizeof(oc_->filename));

				// Set up the parameters and open all codecs
				//if ( av_set_parameters( oc_, NULL ) >= 0 )
				{
					// Attempt to open video and audio streams
					if ( open_video_stream( ) && open_audio_stream( ) )
					{
						if ( fmt_->flags & AVFMT_NEEDNUMBER )
							if ( !av_filename_number_test(oc_->filename) )
								throw( std::string( "invalid format for output" ) );

						ret = open_container( );
					}
				}

				if ( prop_acodec_.value< pl::wstring >( ) == L"aac" && std::string( fmt_->name ) == "mp4" )
				{
					for ( size_t i = 0; i < bitstream_filters_.size( ); i ++ )
					{
						if ( oc_->streams[ i ]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
						{
							bitstream_filters_[ i ] = av_bitstream_filter_init( "aac_adtstoasc" );
						}
					}
				}
			}

			first_frame_ = frame_type_ptr( );

			return ret;
		}

		bool open_container( )
		{
			bool ret = true;

			strncpy(oc_->filename, pl::to_string( resource( ) ).c_str( ), sizeof(oc_->filename));

			// We'll allow encoding to stdout
			if ( !( fmt_->flags & AVFMT_NOFILE ) ) 
				ret = avio_open( &oc_->pb, pl::to_string( resource( ) ).c_str( ), AVIO_FLAG_WRITE ) >= 0;
			else
				ret = true;

			// Write the header
			if ( ret )
			{
				avformat_write_header( oc_, 0 );

				if ( prop_ts_auto_.value< int >( ) && prop_ts_index_.value< pl::wstring >( ) == L"" )
					ret = ffurl_open( &ts_context_, pl::to_string( pl::wstring( resource( ) + L".awi" ) ).c_str( ), AVIO_FLAG_WRITE, 0, 0 ) >= 0;
				else if ( prop_ts_index_.value< pl::wstring >( ) != L"" )
					ret = ffurl_open( &ts_context_, pl::to_string( prop_ts_index_.value< pl::wstring >( ) ).c_str( ), AVIO_FLAG_WRITE, 0 ,0 ) >= 0;
			}

			return ret;
		}

		void close_container( )
		{
			// Finalize encode for video
			if ( audio_stream_.size( ) )
			{
				if( audio_block_used_ != 0 )
				{
					audio_queue_.push_back( audio_block_ );
				}

				while( audio_queue_.size( ) && process_audio( ) ) ;
				process_audio( true );
			}

			// Write the trailer, if any
			av_write_trailer( oc_ );

			// Get the final size now
			if ( ts_context_ )
			{
				boost::int64_t final = oc_->pb->pos;
				ts_generator_audio_.close(audio_packet_num_, final);
				ts_generator_video_.close(push_count_, final);
				write_ts_index( );
				ffurl_close( ts_context_ );
			}

			// Close the output file
			if ( !( fmt_->flags & AVFMT_NOFILE ) )
				avio_close( oc_->pb );

			// Clean up the log
			if ( log_file_ )
				fclose( log_file_ );
			av_free( log_ );
		}

		void write_ts_index( )
		{
			if ( ts_context_ )
			{
				std::vector< boost::uint8_t > buffer;
				ts_generator_audio_.flush( buffer );
				ffurl_write( ts_context_, ( unsigned char * )( &buffer[ 0 ] ), buffer.size( ) );


				ts_generator_video_.flush( buffer );
				ffurl_write( ts_context_, ( unsigned char * )( &buffer[ 0 ] ), buffer.size( ) );
			}
		}

		void close_video_codec( )
		{
			if ( video_stream_ && video_stream_->codec ) 
			{
				avcodec_close( video_stream_->codec );
				av_free( video_stream_->codec );
				video_stream_->codec = NULL;
			}
		}

		void close_audio_codec( )
		{
			for ( std::vector< AVStream * >::iterator iter = audio_stream_.begin( ); iter != audio_stream_.end( ); iter ++ )
			{
				AVStream *stream = *iter;
				if ( stream && stream->codec ) 
				{
					avcodec_close( stream->codec );
					av_free( stream->codec );
					stream->codec = NULL;
				}
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
				if ( audio_stream_.size( ) )
					audio_pts = double( audio_stream_[ 0 ]->pts.val ) * audio_stream_[ 0 ]->time_base.num / audio_stream_[ 0 ]->time_base.den;
				else
					audio_pts = 0.0;
		
				if ( video_stream_ )
					video_pts = double( video_stream_->pts.val ) * video_stream_->time_base.num / video_stream_->time_base.den;
				else
					video_pts = 0.0;

 				// Write interleaved audio and video frames
 				if ( !video_stream_ || ( video_stream_ && audio_stream_.size( ) && audio_pts < video_pts ) )
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
				double audio_pts = audio_stream_.size( ) != 0 ? double( audio_stream_[ 0 ]->pts.val ) * audio_stream_[ 0 ]->time_base.num / audio_stream_[ 0 ]->time_base.den : 0.0;
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
				double audio_pts = audio_stream_.size( ) != 0 ? double( audio_stream_[ 0 ]->pts.val ) * audio_stream_[ 0 ]->time_base.num / audio_stream_[ 0 ]->time_base.den : 0.0;
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
		pl::wstring resource( )
		{
			if ( resource_.find( L"avformat:" ) == 0 )
				return resource_.substr( 9 );
			return resource_;
		}

		// Very rough mapping of file name and format property
		AVOutputFormat *alloc_output_format( )
		{
			AVOutputFormat *fmt = NULL;

			if ( prop_format_.value< pl::wstring >( ) != L"" )
				fmt = av_guess_format( pl::to_string( prop_format_.value< pl::wstring >( ) ).c_str( ), NULL, NULL );

			if ( fmt == NULL )
				fmt = av_guess_format( NULL, pl::to_string( resource( ) ).c_str( ), NULL );

			if ( fmt == NULL )
				fmt = av_guess_format( "mpeg", NULL, NULL );

			return fmt;
		}

		// Obtain the requested video codec id
		CodecID obtain_video_codec_id( )
		{
			CodecID codec_id = fmt_->video_codec;
			if ( prop_vcodec_.value< pl::wstring >( ) == L"copy" )
			{
				ARENFORCE( first_frame_ && first_frame_->get_stream( ) );
				stream_type_ptr stream = first_frame_->get_stream( );
				ARENFORCE( name_codec_lookup_.find( stream->codec( ) ) != name_codec_lookup_.end( ) );
				codec_id = name_codec_lookup_[ first_frame_->get_stream( )->codec( ) ];
				return codec_id;
			}
			else if ( prop_vcodec_.value< pl::wstring >( ) != L"" )
			{
				AVCodec *codec = avcodec_find_encoder_by_name( pl::to_string( prop_vcodec_.value< pl::wstring >( ) ).c_str( ) );
				if ( codec != NULL )
					codec_id = codec->id;
			}
			else
			{
				codec_id = av_guess_codec( oc_->oformat, NULL, pl::to_string( resource( ) ).c_str( ), NULL, AVMEDIA_TYPE_VIDEO );
			}
			if ( prop_debug_.value< int >( ) )
 				std::cerr << "obtain_video_codec_id: found: " << codec_id << " for "
						  << pl::to_string( prop_vcodec_.value< pl::wstring >( ) ) << std::endl;
			return codec_id;
		}

		// Obtain the requested audio codec id
		CodecID obtain_audio_codec_id( )
		{
			CodecID codec_id = fmt_->audio_codec;
			if ( prop_acodec_.value< pl::wstring >( ) != L"" )
			{
				AVCodec *codec = avcodec_find_encoder_by_name( pl::to_string( prop_acodec_.value< pl::wstring >( ) ).c_str( ) );
				if ( codec != NULL )
					codec_id = codec->id;
				else if ( prop_debug_.value< int >( ) )
					std::cerr << "obtain_audio_codec_id: failed to find codec for value: "
							   << pl::to_string( prop_acodec_.value< pl::wstring >( ) )
							   << std::endl;
			}
			else
			{
				codec_id = av_guess_codec( oc_->oformat, NULL, pl::to_string( resource( ) ).c_str( ), NULL, AVMEDIA_TYPE_AUDIO );
			}

			if ( prop_debug_.value< int >( ) )
 				std::cerr << "obtain_audio_codec_id: found: " << codec_id << " for "
 						  << pl::to_string( prop_acodec_.value< pl::wstring >( ) ) << std::endl;

			return codec_id;
		}


		// Generate a video stream
		AVStream *add_video_stream( CodecID codec_id )
		{
			AVCodec *codec = avcodec_find_encoder( codec_id );

			// Create a new stream
			AVStream *st = avformat_new_stream( oc_, 0 );

			if ( st != NULL ) 
			{
				AVCodecContext *c = st->codec;

				c->codec_type = avcodec_get_type( codec_id );
				c->codec_id = codec_id;

				// ? unsure
				c->reordered_opaque = AV_NOPTS_VALUE;

				// Specify stream parameters
				c->bit_rate = prop_video_bit_rate_.value< int >( );
				c->bit_rate_tolerance = prop_video_bit_rate_tolerance_.value< int >( );
				c->width = prop_width_.value< int >( );
				c->height = prop_height_.value< int >( );
				c->time_base.den = prop_fps_num_.value< int >( );
				c->time_base.num = prop_fps_den_.value< int >( );
				c->gop_size = prop_gop_size_.value< int >( );
				pl::string pixfmt = pl::to_string( prop_pix_fmt_.value< pl::wstring >( ) );
				c->pix_fmt = oil_to_avformat( prop_pix_fmt_.value< pl::wstring >( ) );

				// Fix b frames
				if ( prop_b_frames_.value< int >( ) )
				{
	 				c->max_b_frames = prop_b_frames_.value< int >( );
					c->b_frame_strategy = 0;
					c->b_quant_factor = 2.0;
				}

				// Specifiy miscellaneous properties
	 			c->mb_decision = prop_mb_decision_.value< int >( );
				st->sample_aspect_ratio = c->sample_aspect_ratio = av_d2q( prop_aspect_ratio_.value< double >( ) * c->height / c->width, 255 );
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
				//c->quantizer_noise_shaping= prop_qns_.value< int >( );
				c->qmin = prop_video_qmin_.value< int >( );
				c->qmax = prop_video_qmax_.value< int >( );
				c->lmin = prop_video_lmin_.value< int >( );
				c->lmax = prop_video_lmax_.value< int >( );
//				c->mb_qmin = prop_video_mb_qmin_.value< int >( );
//				c->mb_qmax = prop_video_mb_qmax_.value< int >( );
				c->max_qdiff = prop_video_qdiff_.value< int >( );
				c->qblur = static_cast<float>(prop_video_qblur_.value< double >( ));
				c->qcompress = static_cast<float>(prop_video_qcomp_.value< double >( ));

				c->rc_eq = strdup("tex^qComp");
				c->rc_override_count = 0;
				c->thread_count = prop_thread_count_.value< int >( );
		
				// Handle fixed quality override
				if ( prop_qscale_.value< double >( ) > 0.0 )
				{
					c->flags |= CODEC_FLAG_QSCALE;
					//st->quality = float( FF_QP2LAMBDA * prop_qscale_.value< double >( ) );
				}
		
				// Allow the user to override the video fourcc
				pl::string vfourcc = pl::to_string( prop_vfourcc_.value< pl::wstring >( ) );
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
		
				//oc_->preload = int( prop_mux_preload_.value< double >( ) * AV_TIME_BASE );
				oc_->max_delay = int( prop_mux_delay_.value< double >( ) * AV_TIME_BASE );
				//oc_->loop_output = AVFMT_NOOUTPUTLOOP;
				oc_->flags |= AVFMT_FLAG_NONBLOCK;

				//oc_->mux_rate = prop_mux_rate_.value< int >( );
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
				c->trellis = prop_trellis_.value< int >( );

				if ( prop_4mv_.value< int >( ) )
					c->flags |= CODEC_FLAG_4MV;

				c->flags2 |= CODEC_FLAG2_STRICT_GOP;
 			}
 
			return st;
		}

		void add_audio_streams( CodecID codec_id )
		{
			int streams = 1;
			int channels = prop_channels_.value< int >( );
			int channels_per_stream = channels;
			int audio_split = prop_audio_split_.value< int >( );
			if( audio_split > 0 )
			{
				channels_per_stream = audio_split;
				streams = channels / channels_per_stream;
				if( channels % channels_per_stream != 0 )
					streams++;
			}

			/*
			if ( prop_audio_split_.value< int >( ) ) // || channels > 2 )
			{
				prop_audio_split_ = 1;
				streams = channels;
				channels = 1;
			}
			*/

			for( int i = 0; i < streams; i ++ )
			{
				// Create a new stream
				AVStream *st = avformat_new_stream( oc_, 0 );

				if ( !st )
					std::cerr << "add_audio_stream: failed to create stream for: " << codec_id << std::endl;

				// If created, then initialise from properties
				if ( st != NULL ) 
				{
					AVCodecContext *c = st->codec;

					c->strict_std_compliance = prop_strict_.value< int >( );
					c->codec_type = avcodec_get_type( codec_id );
					c->codec_id = codec_id;

					//c->codec_id = codec_id;
					//c->codec_type = AVMEDIA_TYPE_AUDIO;
					c->reordered_opaque = AV_NOPTS_VALUE;

					// Specify sample parameters
					c->sample_rate = prop_frequency_.value< int >( );
					//The codec uses some strange default values that will 
					//not work for atleast the vorbis case. 
					// 1/sample_rate is a better guess 
					if (c->time_base.num == 0 && c->time_base.den == 1) {
						c->time_base.num = 1;
						c->time_base.den = c->sample_rate;
					}
					c->channels = (std::min)( channels_per_stream, channels - i * channels_per_stream );

					// The bitrate property sets the bitrate for one stream. If we have fewer channels than
					// channels_per_stream in the stream, we'll have to lower the bitrate accordingly.
					int stream_bitrate = prop_audio_bit_rate_.value< int >( ) * c->channels / channels_per_stream;
					c->bit_rate = stream_bitrate;
	
					if ( oc_->oformat->flags & AVFMT_GLOBALHEADER ) 
						c->flags |= CODEC_FLAG_GLOBAL_HEADER;
	
					// Allow the user to override the audio fourcc
					pl::string afourcc = pl::to_string( prop_afourcc_.value< pl::wstring >( ) );
					if ( afourcc != "" )
					{
						char *tail = NULL;
						const char *arg = afourcc.c_str( );
						int tag = strtol( arg, &tail, 0);
						if( !tail || *tail )
							tag = arg[ 0 ] + ( arg[ 1 ] << 8 ) + ( arg[ 2 ] << 16 ) + ( arg[ 3 ] << 24 );
						c->codec_tag = tag;
					}

					audio_stream_.push_back( st );
				}
			}
		}

		int h264_split( const uint8_t *buf, int buf_size )
		{
			int i;
			uint32_t state = -1;
			int has_sps= 0;
		
			for(i=0; i<=buf_size; i++)
			{
				if( ( state & 0xFFFFFF1F ) == 0x107 )
					has_sps = 1;
				if( ( state & 0xFFFFFF00 ) == 0x100 && ( state & 0xFFFFFF1F ) != 0x107 && ( state & 0xFFFFFF1F ) != 0x108 && ( state & 0xFFFFFF1F ) != 0x109 )
				{
					if(has_sps)
					{
						while ( i > 4 && buf[ i - 5 ] == 0 ) i--;
						return i - 4;
					}
				}
				if ( i < buf_size )
					state= ( state << 8 ) | buf[ i ];
			}
			return 0;
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

				if ( prop_vcodec_.value< pl::wstring >( ) == L"copy" )
				{
					ARENFORCE( first_frame_ && first_frame_->get_stream( ) );
					stream_type_ptr stream = first_frame_->get_stream( );
					ARENFORCE( name_codec_lookup_.find( stream->codec( ) ) != name_codec_lookup_.end( ) );
					video_enc->codec_id = name_codec_lookup_[ first_frame_->get_stream( )->codec( ) ];
					video_enc->codec_type = AVMEDIA_TYPE_VIDEO;

					//video_stream_->stream_copy = 1;

					//if( !video_enc->codec_tag )
						//if( !os->oformat->codec_tag || av_codec_get_id (os->oformat->codec_tag, ivideo_enc->codec_tag) == video_enc->codec_id || av_codec_get_tag(os->oformat->codec_tag, ivideo_enc->codec_id) <= 0)
							//video_enc->codec_tag = icodec->codec_tag;

					int size = h264_split( stream->bytes( ), stream->length( ) );
					if ( size )
					{
						uint64_t extra_size = ( uint64_t )size + FF_INPUT_BUFFER_PADDING_SIZE;
						video_enc->extradata = ( uint8_t * )av_mallocz(extra_size);
						memcpy( video_enc->extradata, stream->bytes( ), size );
						video_enc->extradata_size = size;
					}

					video_enc->bit_rate = stream->bitrate( );
					//if ( stream->properties( ).get_property_with_key( key_rc_max_rate_ ).valid( ) )
						//video_enc->rc_max_rate = stream->properties( ).get_property_with_key( key_rc_max_rate_ ).value< int >( );
					//if ( stream->properties( ).get_property_with_key( key_rc_buffer_size_ ).valid( ) )
						//video_enc->rc_buffer_size = stream->properties( ).get_property_with_key( key_rc_buffer_size_ ).value< int >( );

					//video_enc->time_base.num = first_frame_->get_fps_den( );
					//video_enc->time_base.den = first_frame_->get_fps_num( );
					video_enc->time_base.num = stream->properties( ).get_property_with_key( key_timebase_num_ ).value< int >( );
					video_enc->time_base.den = stream->properties( ).get_property_with_key( key_timebase_den_ ).value< int >( );
					av_reduce(&video_enc->time_base.num, &video_enc->time_base.den, stream->properties( ).get_property_with_key( key_timebase_num_ ).value< int >( ), stream->properties( ).get_property_with_key( key_timebase_den_ ).value< int >( ), INT_MAX);

					video_enc->pix_fmt = oil_to_avformat( stream->pf( ) );
					video_enc->width = stream->size( ).width;
					video_enc->height = stream->size( ).height;

					if ( stream->properties( ).get_property_with_key( key_has_b_frames_ ).valid( ) )
	 					video_enc->has_b_frames = stream->properties( ).get_property_with_key( key_has_b_frames_ ).value< int >( );

					video_enc->codec_id = CodecID( stream->properties( ).get_property_with_key( key_codec_id_ ).value< int >( ) );
					video_enc->codec_type = AVMediaType( stream->properties( ).get_property_with_key( key_codec_type_ ).value< int >( ) );
					//if ( stream->properties( ).get_property_with_key( key_codec_tag_ ).value< boost::int64_t >( ) != 0 )
						//if ( video_enc->codec_tag == 0 )
							//video_enc->codec_tag = stream->properties( ).get_property_with_key( key_codec_tag_ ).value< boost::int64_t >( );
					video_enc->rc_max_rate = stream->properties( ).get_property_with_key( key_max_rate_ ).value< int >( );
					video_enc->rc_buffer_size = stream->properties( ).get_property_with_key( key_buffer_size_ ).value< int >( );
					video_enc->bit_rate = stream->properties( ).get_property_with_key( key_bit_rate_ ).value< int >( );
					video_enc->field_order = AVFieldOrder( stream->properties( ).get_property_with_key( key_field_order_ ).value< int >( ) );
					video_enc->bits_per_coded_sample = stream->properties( ).get_property_with_key( key_bits_per_coded_sample_ ).value< int >( );
					video_enc->ticks_per_frame = stream->properties( ).get_property_with_key( key_ticks_per_frame_ ).value< int >( );

					video_stream_->avg_frame_rate.num = stream->properties( ).get_property_with_key( key_timebase_num_ ).value< int >( );
					video_stream_->avg_frame_rate.den = stream->properties( ).get_property_with_key( key_timebase_den_ ).value< int >( );

					if ( !video_enc->sample_aspect_ratio.num ) 
					{
						AVRational rate;
						rate.num = stream->sar( ).num;
						rate.den = stream->sar( ).den;
						video_enc->sample_aspect_ratio = rate;
						video_stream_->sample_aspect_ratio = rate;
					}

					video_copy_ = true;
				}
				else
				{

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
	
					// Provide access to properties for custom codecs
					video_enc->opaque = ( void * )this;
	
					// Open the codec safely
					ret = codec != NULL && avcodec_open2( video_enc, codec, 0 ) >= 0;
	
					#ifndef WIN32
					if ( ret && prop_thread_count_.value< int >( ) > 1 )
						video_enc->thread_count = prop_thread_count_.value< int >( );
					#endif
				}
			}

			return ret;
		}

		// Open the audio stream
		// Note: This method returns true if there is no audio stream created
		bool open_audio_stream( )
		{
			bool ret = true;

			for ( std::vector< AVStream * >::iterator iter = audio_stream_.begin( ); iter != audio_stream_.end( ); iter ++ )
			{
				// Get the context
				AVCodecContext *c = ( *iter )->codec;

				// Find the encoder
				AVCodec *codec = avcodec_find_encoder( c->codec_id );

				// Continue if codec found
				ARENFORCE( codec != NULL );

				c->sample_fmt = codec->sample_fmts[0];

				// Continue if we can open it
				ARENFORCE( avcodec_open2( c, codec, 0 ) >= 0 );

				if ( c->frame_size <= 1 ) 
				{
				   audio_input_frame_size_ = audio_outbuf_size_ / c->channels;
				   switch( ( *iter )->codec->codec_id ) 
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
				   audio_variable_ = true;
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

			return ret;
		}

		// Queue video and audio components from frame
		void queue( frame_type_ptr frame )
		{
			// Queue video - this is simply the frames image
			if ( video_stream_ && frame->has_image( ) )
			{
				AVCodecContext *c = video_stream_->codec;
				const std::wstring pf = avformat_to_oil( c->pix_fmt );
				if ( pf != L"" && !video_copy_ )
				{
					frame = ml::frame_convert( frame, pf );
					video_queue_.push_back( frame );
				}
				else
				{
					video_queue_.push_back( frame );
				}
			}

			// Queue audio - audio is carved up here to match the number of bytes required by the audio codec
			if ( audio_stream_.size( ) && frame->get_audio( ) )
			{
				audio_type_ptr current = audio::coerce( audio::FORMAT_PCM16, frame->get_audio( ) );

				// Create an audio block if we haven't done so already
				if ( audio_block_ == 0 )
				{
					audio_block_used_ = 0;
					audio_block_ = audio::pcm16_ptr( new audio::pcm16( current->frequency( ), current->channels( ), audio_input_frame_size_ ) );
					audio_block_->set_position( push_count_ );
				}

				// We want to ensure that 'gop boundaries' are flagged when the audio is introduced, not when just when a new 
				// audio frame is created (otherwise, a search based on the byte position will miss the start of the audio)
				if ( push_count_ % prop_gop_size_.value< int >( ) == 0 )
					audio_block_->set_position( push_count_ );
		
				int bytes = audio_input_frame_size_ * current->channels( ) * 2;
				int available = current->size( );
				int offset = 0;

				while ( bytes > 0 && audio_block_used_ + available >= bytes )
				{
					memcpy( ( boost::uint8_t * )audio_block_->pointer( ) + audio_block_used_, ( boost::uint8_t * )current->pointer( ) + offset, bytes - audio_block_used_ );
					audio_queue_.push_back( audio_block_ );
					available -= bytes - audio_block_used_;
					offset += bytes - audio_block_used_;
					audio_block_used_ = 0;
					audio_block_ = audio::pcm16_ptr( new audio::pcm16( current->frequency( ), current->channels( ), audio_input_frame_size_ ) );
					audio_block_->set_position( push_count_ );
				}

				if ( offset < current->size( ) )
				{
					memcpy( ( boost::uint8_t * )audio_block_->pointer( ) + audio_block_used_, ( boost::uint8_t * )current->pointer( ) + offset, current->size( ) - offset );
					audio_block_used_ += current->size( ) - offset;
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

				if( oc_->pb && c->coded_frame && c->coded_frame->key_frame && ( ( push_count_ - 1 ) != ts_last_position_ && ( oc_->pb->pos != ts_last_offset_ || ts_last_offset_ == 0 ) ) )
				{
					ts_generator_video_.enroll( c->coded_frame->pts, oc_->pb->pos );
					write_ts_index( );
					pkt.flags |= AV_PKT_FLAG_KEY;
					ts_last_position_ = push_count_ - 1;
					ts_last_offset_ = oc_->pb->pos;
				}

				pkt.stream_index = video_stream_->index;
				pkt.data = video_outbuf_;
				pkt.size = *out_size;

				if ( log_file_ && prop_pass_.value< int >( ) == 1 && c->stats_out )
					fprintf( log_file_, "%s", c->stats_out );

				// Write the compressed frame in the media file
				int err = write_packet( oc_, &pkt, c, bitstream_filters_[ pkt.stream_index ] );
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

		bool do_stream_write( frame_type_ptr frame )
		{
			bool ret = true;
			AVCodecContext *c = video_stream_->codec;

			stream_type_ptr stream = frame->get_stream( );

			if ( stream )
			{
				AVPacket pkt;
				av_init_packet( &pkt );

				AVRational time_base;

				time_base.num = stream->properties( ).get_property_with_key( key_timebase_num_ ).value< int >( );
				time_base.den = stream->properties( ).get_property_with_key( key_timebase_den_ ).value< int >( );

				pkt.stream_index = video_stream_->index;
				pkt.data = stream->bytes( );
				pkt.size = stream->length( );
				pkt.pts = av_rescale_q( stream->properties( ).get_property_with_key( key_pts_ ).value< boost::int64_t >( ), c->time_base, video_stream_->time_base );
				pkt.dts = av_rescale_q( stream->properties( ).get_property_with_key( key_dts_ ).value< boost::int64_t >( ), c->time_base, video_stream_->time_base );
				pkt.duration = stream->properties( ).get_property_with_key( key_duration_ ).value< int >( );

				boost::int64_t ost_tb_start_time = av_rescale_q( 0, ml_av_time_base_q, video_stream_->time_base );
				pkt.pts = av_rescale_q(pkt.pts, time_base, video_stream_->time_base) - ost_tb_start_time;
				pkt.dts = av_rescale_q(pkt.dts, time_base, video_stream_->time_base);

				//opkt.duration = av_rescale_q(pkt->duration, ist->st->time_base, ost->st->time_base);
				//std::cerr << "ts: " << pkt.pts << " " << pkt.dts << std::endl;

				if( oc_->pb && stream->position( ) == stream->key( ) && ( ( push_count_ - 1 ) != ts_last_position_ && ( oc_->pb->pos != ts_last_offset_ || ts_last_offset_ == 0 ) ) )
				{
					ts_generator_video_.enroll( stream->position( ), oc_->pb->pos );
					write_ts_index( );
					pkt.flags |= AV_PKT_FLAG_KEY;
					ts_last_position_ = push_count_ - 1;
					ts_last_offset_ = oc_->pb->pos;
				}


				if ( log_file_ && prop_pass_.value< int >( ) == 1 && c->stats_out )
					fprintf( log_file_, "%s", c->stats_out );

				// Write the compressed frame in the media file
				int err = write_packet( oc_, &pkt, c, bitstream_filters_[ pkt.stream_index ] );
				ret = err >= 0;

				if ( log_file_ && prop_pass_.value< int >( ) == 1  && c->stats_out )
					fprintf( log_file_, "%s", c->stats_out );
			}

			return ret;
		}

		int write_packet( AVFormatContext *s, AVPacket *pkt, AVCodecContext *avctx, AVBitStreamFilterContext *bsfc )
		{
			int ret = 0;

			while( bsfc )
			{
				AVPacket new_pkt = *pkt;
				int a = av_bitstream_filter_filter(bsfc, avctx, NULL, &new_pkt.data, &new_pkt.size, pkt->data, pkt->size, pkt->flags & AV_PKT_FLAG_KEY);
				if ( a > 0 )
				{
					av_free_packet(pkt);
					new_pkt.destruct= av_destruct_packet;
				} 
				else if ( a < 0 )
				{
					return -1;
				}
				*pkt= new_pkt;

				bsfc= bsfc->next;
			}

			return av_interleaved_write_frame(s, pkt);
		}

		// Process and output an image
		// Precondition: the video queue is not empty
		bool process_video( )
		{
			bool ret = true;

			// Obtain the next image
			frame_type_ptr frame = *( video_queue_.begin( ) );
			video_queue_.pop_front( );

			if ( video_copy_ )
				return do_stream_write( frame );

			il::image_type_ptr image = frame->get_image( );
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
		
				pkt.flags |= AV_PKT_FLAG_KEY;
				pkt.stream_index = video_stream_->index;
				pkt.data = ( uint8_t * )&av_image_;
				pkt.size = sizeof( AVPicture );

				ret = write_packet( oc_, &pkt, c, bitstream_filters_[ pkt.stream_index ] ) == 0;
 			} 
			else 
			{
				// Set the quality
				//av_image_.quality = int( video_stream_->quality );
				//av_image_.pict_type = 0;

				int out_size;
				ret = do_video_encode(false, &out_size);
			}

			return ret;
		}

		// Process and output a block of audio samples (see comments in queue above)
		// if do_flush is set, we will encode a NULL packet in order to empty the encoder buffer
		bool process_audio( bool do_flush = false )
		{
			bool ret = true;

			audio_type_ptr audio;
			const short *data = 0;

			if ( audio_queue_.size( ) )
			{
				audio = *( audio_queue_.begin( ) );
				audio_queue_.pop_front( );
				data = ( short * )( audio->pointer( ) );
			}
			else
			{
				ret = false;
			}

			int stream = 0;

			static int first = true;
			int encode_tries = 1;

			for ( std::vector< AVStream * >::iterator iter = audio_stream_.begin( ); ( do_flush || ( ret && data ) ) && iter != audio_stream_.end( ); iter ++, stream ++ )
			{
				for( int ec=0; ec<encode_tries; ec++ )
				{
					AVCodecContext *c = ( *iter )->codec;

					AVPacket pkt;
					av_init_packet( &pkt );

					if ( prop_audio_split_.value< int >( ) && !do_flush )
					{
						int samples = audio->samples( );
						int channels = audio->channels( );
						int channels_to_write = c->channels;
						int start_channel = stream * prop_audio_split_.value< int >( );

						short *dst = ( short * )audio_tmpbuf_;
						const short *src = data + start_channel;
						while( samples -- )
						{
							memcpy( dst, src, sizeof(short) * channels_to_write );
							dst += channels_to_write;
							src += channels;
						}
						pkt.size = avcodec_encode_audio( c, audio_outbuf_, audio_outbuf_size_, ( short * )audio_tmpbuf_ );
					}
					else
					{
						pkt.size = avcodec_encode_audio( c, audio_outbuf_, audio_outbuf_size_, data );
					}

					// Write the compressed frame in the media file
					if ( c->coded_frame && uint64_t( c->coded_frame->pts ) != AV_NOPTS_VALUE )
						pkt.pts = av_rescale_q( c->coded_frame->pts, c->time_base, ( *iter )->time_base );

					pkt.flags |= AV_PKT_FLAG_KEY;
					pkt.stream_index = ( *iter )->index;
					pkt.data = audio_outbuf_;

					if ( pkt.size > 0 )
					{
						if( stream == 0)
						{
							if( !first || ec == encode_tries -1 )
							{
								if( audio_packet_num_ % 8 == 0 )
								{
									ts_generator_audio_.enroll( audio_packet_num_, oc_->pb->pos );
									write_ts_index( );
								}
								audio_packet_num_++;
							}
						}

						ret = write_packet( oc_, &pkt, c, bitstream_filters_[ pkt.stream_index ] ) == 0;
						if( !first )
							break;
					}
					else if ( data == 0 )
					{
						ret = false;
					}
				}	
			}
			first = false;

			return ret;
		}

		// The output file or device
		pl::wstring resource_;
		pcos::property prop_enable_audio_;
		pcos::property prop_enable_video_;

		// libavformat structures
		AVFormatContext *oc_;
		AVOutputFormat *fmt_;
		std::vector< AVStream * > audio_stream_;
		std::vector< AVBitStreamFilterContext * > bitstream_filters_;
		AVStream *video_stream_;
		struct SwsContext *img_convert_;
		int audio_input_frame_size_;
		int audio_outbuf_size_;
		uint8_t *audio_outbuf_ptr_;
		uint8_t *audio_outbuf_;
		uint8_t *audio_tmpbuf_ptr_;
		uint8_t *audio_tmpbuf_;
		AVFrame av_image_; 
		uint8_t *video_outbuf_;
		int video_outbuf_size_;

		// oml structures
		std::deque< frame_type_ptr > video_queue_;
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
		//pcos::property /* int */ prop_qns_;
		pcos::property /* int */ prop_video_qmin_;
		pcos::property /* int */ prop_video_qmax_;
		pcos::property /* int */ prop_video_lmin_;
		pcos::property /* int */ prop_video_lmax_;
		//pcos::property /* int */ prop_video_mb_qmin_;
		//pcos::property /* int */ prop_video_mb_qmax_;
		pcos::property /* int */ prop_video_qdiff_;
		pcos::property /* double */ prop_video_qblur_;
		pcos::property /* double */ prop_video_qcomp_;
		//pcos::property /* int */ prop_mux_rate_;
		//pcos::property /* double */ prop_mux_preload_;
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
		pcos::property /* int */ prop_trellis_;
		pcos::property /* int */ prop_4mv_;
		pcos::property /* int */ prop_thread_count_;

		pcos::property /* int */ prop_context_;
		pcos::property /* int */ prop_predictor_;
		pcos::property /* int */ prop_pass_;
		pcos::property /* wstring */ prop_pass_log_;

		pcos::property prop_ts_index_;
		pcos::property prop_ts_auto_;
		pcos::property prop_audio_split_;

		pcos::property prop_frag_key_frame_;

		awi_generator_v4 ts_generator_video_;
		awi_generator_v4 ts_generator_audio_;
		URLContext *ts_context_;
		int ts_last_position_;
		boost::int64_t ts_last_offset_;

		FILE *log_file_;
		char *log_;

		int frame_pos_;
		int push_count_;
		int last_audio_;
		boost::uint32_t audio_packet_num_;
		bool audio_variable_;

		frame_type_ptr first_frame_;
		bool video_copy_;
};

store_type_ptr ML_PLUGIN_DECLSPEC create_store_avformat( const pl::wstring &resource, const frame_type_ptr &frame )
{
	return store_type_ptr( new avformat_store( resource, frame ) );
}

} } }

