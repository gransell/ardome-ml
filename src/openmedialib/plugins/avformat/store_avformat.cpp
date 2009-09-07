
#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/packet.hpp>
#include <openmedialib/ml/awi.hpp>
#include <openpluginlib/pl/pcos/isubject.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>

#include <vector>

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
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#define MAX_AUDIO_PACKET_SIZE (128 * 1024)

namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;
namespace pcos = olib::openpluginlib::pcos;

namespace olib { namespace openmedialib { namespace ml {

extern const std::wstring avformat_to_oil( int );
extern const PixelFormat oil_to_avformat( const std::wstring & );
extern il::image_type_ptr convert_to_oil( AVFrame *, PixelFormat, int, int );

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
			, ts_context_( 0 )
			, ts_last_position_( -1 )
			, ts_last_offset_( 0 )
			, log_file_( 0 )
			, log_( 0 )
			, frame_pos_( 0 )
			, push_count_( 0 )
			, last_audio_( -1 )
			, audio_variable_( false )
		{
			// Show stats
			properties( ).append( prop_show_stats_ = 100 );
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
			properties( ).append( prop_format_ = pl::wstring( L"" ) );
			properties( ).append( prop_acodec_ = pl::wstring( L"" ) );
			properties( ).append( prop_vcodec_ = pl::wstring( L"" ) );
			properties( ).append( prop_vfourcc_ = pl::wstring( L"" ) );
			properties( ).append( prop_afourcc_ = pl::wstring( L"" ) );
			properties( ).append( prop_pix_fmt_ = pl::wstring( L"" ) );

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
			properties( ).append( prop_trellis_ = 2 );
			properties( ).append( prop_4mv_ = 0 );
			properties( ).append( prop_thread_count_ = 1 );

			properties( ).append( prop_context_ = 0 );
			properties( ).append( prop_predictor_ = 0 );
			properties( ).append( prop_pass_ = 0 );
			properties( ).append( prop_pass_log_ = pl::wstring( L"oml_avformat.log" ) );
			properties( ).append( prop_ts_index_ = pl::wstring( L"" ) );
			properties( ).append( prop_ts_auto_ = 0 );
			properties( ).append( prop_audio_split_ = 0 );
		}

		virtual ~avformat_store( )
		{
			if ( oc_ )
			{
				if ( video_stream_ )
				{
					int out_size = 0;
					do 
					{
						do_video_encode( true, &out_size );
					} 
					while(out_size > 0);
				}

				// Close the container
				close_container( );

				// Clean up the codecs
				close_video_codec( );
				close_audio_codec( );

				// Free the streams
				for( size_t i = 0; i < oc_->nb_streams; i++ )
					av_freep( &oc_->streams[ i ] );

				// Free the context
				av_free( oc_ );
			}

			// Clean up the allocated image
			if ( video_stream_ )
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

				strncpy(oc_->filename, pl::to_string( resource( ) ).c_str( ), sizeof(oc_->filename));

				// Set up the parameters and open all codecs
				if ( av_set_parameters( oc_, NULL ) >= 0 )
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
			}

			return ret;
		}

		bool open_container( )
		{
			bool ret = true;

			strncpy(oc_->filename, pl::to_string( resource( ) ).c_str( ), sizeof(oc_->filename));

			// We'll allow encoding to stdout
			if ( !( fmt_->flags & AVFMT_NOFILE ) ) 
				ret = url_fopen( &oc_->pb, pl::to_string( resource( ) ).c_str( ), URL_WRONLY ) >= 0;
			else
				ret = true;

			// Write the header
			if ( ret )
			{
				av_write_header( oc_ );

				if ( prop_ts_auto_.value< int >( ) && prop_ts_index_.value< pl::wstring >( ) == L"" )
					ret = url_open( &ts_context_, pl::to_string( pl::wstring( resource( ) + L".awi" ) ).c_str( ), URL_WRONLY ) >= 0;
				else if ( prop_ts_index_.value< pl::wstring >( ) != L"" )
					ret = url_open( &ts_context_, pl::to_string( prop_ts_index_.value< pl::wstring >( ) ).c_str( ), URL_WRONLY ) >= 0;
			}

			return ret;
		}

		void close_container( )
		{
			// Finalize encode for video
			if ( audio_stream_.size( ) )
			{
				if ( audio_variable_ || audio_input_frame_size_ == audio_block_used_ / ( prop_channels_.value< int >( ) * 2 ) )
					audio_queue_.push_back( audio_block_ );
				while( audio_queue_.size( ) && process_audio( ) ) ;
			}

			// Write the trailer, if any
			av_write_trailer( oc_ );

			// Get the final size now
			boost::int64_t final = oc_->pb->pos;

			if ( ts_context_ )
			{
				ts_generator_.close( push_count_, final );
				write_ts_index( );
				url_close( ts_context_ );
			}

			// Close the output file
			if ( !( fmt_->flags & AVFMT_NOFILE ) )
				url_fclose( oc_->pb );

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
				ts_generator_.flush( buffer );
				url_write( ts_context_, ( unsigned char * )( &buffer[ 0 ] ), buffer.size( ) );
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
				fmt = guess_format( pl::to_string( prop_format_.value< pl::wstring >( ) ).c_str( ), NULL, NULL );

			if ( fmt == NULL )
				fmt = guess_format( NULL, pl::to_string( resource( ) ).c_str( ), NULL );

			if ( fmt == NULL )
				fmt = guess_format( "mpeg", NULL, NULL );

			return fmt;
		}

		// Obtain the requested video codec id
		CodecID obtain_video_codec_id( )
		{
			CodecID codec_id = fmt_->video_codec;
			if ( prop_vcodec_.value< pl::wstring >( ) != L"" )
			{
				AVCodec *codec = avcodec_find_encoder_by_name( pl::to_string( prop_vcodec_.value< pl::wstring >( ) ).c_str( ) );
				if ( codec != NULL )
					codec_id = codec->id;
			}
			if ( prop_debug_.value< int >( ) )
 				std::cerr << L"obtain_video_codec_id: found: " << codec_id << " for "
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
					std::cerr << L"obtain_audio_codec_id: failed to find codec for value: "
							   << pl::to_string( prop_acodec_.value< pl::wstring >( ) )
							   << std::endl;
			}

			if ( prop_debug_.value< int >( ) )
 				std::cerr << L"obtain_audio_codec_id: found: " << codec_id << " for "
 						  << pl::to_string( prop_acodec_.value< pl::wstring >( ) ) << std::endl;

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
				c->thread_count = prop_thread_count_.value< int >( );
		
				// Handle fixed quality override
				if ( prop_qscale_.value< double >( ) > 0.0 )
				{
					c->flags |= CODEC_FLAG_QSCALE;
					st->quality = float( FF_QP2LAMBDA * prop_qscale_.value< double >( ) );
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

			if ( prop_audio_split_.value< int >( ) ) // || channels > 2 )
			{
				prop_audio_split_ = 1;
				streams = channels;
				channels = 1;
			}

			for( int i = 0; i < streams; i ++ )
			{
				// Create a new stream
				AVStream *st = av_new_stream( oc_, i + 1 );

				if ( !st )
					std::cerr << "add_audio_stream: failed to create stream for: " << codec_id << std::endl;

				// If created, then initialise from properties
				if ( st != NULL ) 
				{
					AVCodecContext *c = st->codec;
					c->codec_id = codec_id;
					c->codec_type = CODEC_TYPE_AUDIO;
					c->reordered_opaque = AV_NOPTS_VALUE;

					// Specify sample parameters
					c->bit_rate = prop_audio_bit_rate_.value< int >( );
					c->sample_rate = prop_frequency_.value< int >( );
					c->channels = channels;
	
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

				// Provide access to properties for custom codecs
				video_enc->opaque = ( void * )this;

				// Open the codec safely
				ret = codec != NULL && avcodec_open( video_enc, codec ) >= 0;

				if ( ret && prop_thread_count_.value< int >( ) > 1 )
					avcodec_thread_init( video_enc, prop_thread_count_.value< int >( ) );
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

				// Continue if codec found and we can open it
				if ( codec != NULL && avcodec_open( c, codec ) >= 0 )
				{
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
			if ( audio_stream_.size( ) && frame->get_audio( ) )
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

				while ( bytes > 0 && audio_block_used_ + available >= bytes )
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


				if ( boost::uint64_t( c->reordered_opaque ) != AV_NOPTS_VALUE)
					pkt.dts = av_rescale_q(c->reordered_opaque, c->time_base, video_stream_->time_base );
				if( c->coded_frame && c->coded_frame->key_frame && ( ( push_count_ - 1 ) != ts_last_position_ && ( oc_->pb->pos != ts_last_offset_ || ts_last_offset_ == 0 ) ) )
				{
					ts_generator_.enroll( c->coded_frame->pts, oc_->pb->pos );
					write_ts_index( );
					pkt.flags |= PKT_FLAG_KEY;
					ts_last_position_ = push_count_ - 1;
					ts_last_offset_ = oc_->pb->pos;
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
					ts_generator_.enroll( audio->position( ), oc_->pb->pos );
					write_ts_index( );
				}
			}
			else
			{
				ret = false;
			}

			int stream = 0;

			for ( std::vector< AVStream * >::iterator iter = audio_stream_.begin( ); ret && data && iter != audio_stream_.end( ); iter ++, stream ++ )
			{
				AVCodecContext *c = ( *iter )->codec;

				AVPacket pkt;
				av_init_packet( &pkt );

				if ( prop_audio_split_.value< int >( ) )
				{
					short *dst = ( short * )audio_tmpbuf_;
					short *src = data + stream;
					int samples = audio->samples( );
					int channels = audio->channels( );
					while( samples -- )
					{
						*dst ++ = *src;
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

				pkt.flags |= PKT_FLAG_KEY;
				pkt.stream_index = ( *iter )->index;
				pkt.data = audio_outbuf_;

				if ( pkt.size )
				{
					ret = av_interleaved_write_frame( oc_, &pkt ) == 0;
				}
				else if ( data == 0 )
				{
					ret = false;
				}
			}

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

		awi_generator ts_generator_;
		URLContext *ts_context_;
		int ts_last_position_;
		boost::uint64_t ts_last_offset_;

		FILE *log_file_;
		char *log_;

		int frame_pos_;
		int push_count_;
		int last_audio_;
		bool audio_variable_;
};

store_type_ptr ML_PLUGIN_DECLSPEC create_store_avformat( const pl::wstring &resource, const frame_type_ptr &frame )
{
	return store_type_ptr( new avformat_store( resource, frame ) );
}

} } }
