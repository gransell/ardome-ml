#ifndef AML_AVFORMAT_WRAPPERS_H
#define AML_AVFORMAT_WRAPPERS_H

#include <string>

#include <openmedialib/ml/stream.hpp>
#include <opencorelib/cl/profile.hpp>

#include "avformat_stream.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}

namespace cl = olib::opencorelib;
namespace ml = olib::openmedialib::ml;

namespace pl = olib::openpluginlib;

namespace olib { namespace openmedialib { namespace ml {

extern const PixelFormat oil_to_avformat( const olib::t_string & );
extern ml::image_type_ptr convert_to_oil( AVFrame *, PixelFormat, int, int );

#define OPT_PREFIX "video_"
static const char *OPT_NON_LINEAR_QUANT = OPT_PREFIX "non_linear_quant";
static const char *OPT_INTRA_VLC = OPT_PREFIX "intra_vlc";

class avformat_video : public cl::profile_wrapper, public cl::profile_property
{
	public:
		avformat_video( ml::frame_type_ptr frame )
			: profile_wrapper( false )
			, profile_property( false )
			, context_( 0 )
			, codec_( 0 )
			, picture_( 0 )
			, pkt_( )
			, codec_opened_( false )
			, video_codec_( "" )
			, stream_codec_id_( "" )
			, outbuf_size_( 0 )
			, outbuf_( 0 )
			, position_( 0 )
			, key_( 0 )
			, dim_( 0, 0 )
			, sar_( 0, 0 )
			, fps_( 0, 0 )
		{ 
			init( frame ); 
		}

		virtual ~avformat_video( )
		{
			avcodec_close( context_ );
			av_free( picture_ );

			free( outbuf_ );
		}

		void init( ml::frame_type_ptr frame )
		{
			init( ); 
			pf_ = frame->pf( );
			frame->get_image( );
			field_order_ = frame->field_order( );
			dim_ = ml::dimensions( frame->width( ), frame->height( ) );
			sar_ = ml::fraction( frame->get_sar_num( ), frame->get_sar_den( ) );
			fps_ = ml::fraction( frame->get_fps_num( ), frame->get_fps_den( ) );
			outbuf_size_ = std::max< int >( 1024 * 256, dim_.width * dim_.height * 6 + 200 );
			outbuf_ = ( boost::uint8_t * )malloc( outbuf_size_ );
		}

		void init( )
		{
			picture_ = avcodec_alloc_frame( );
			context_ = avcodec_alloc_context3( 0 );
			enroll( "video_codec", this );
			enroll( "stream_codec_id", this );
			enroll( OPT_NON_LINEAR_QUANT, this );
			enroll( OPT_INTRA_VLC, this );
			enroll( "video_bit_rate", context_->bit_rate );
			enroll( "video_bit_rate_tolerance", context_->bit_rate_tolerance );
			enroll( "video_flags", context_->flags );
			enroll( "video_me_method", context_->me_method );
			enroll( "video_gop_size", context_->gop_size );
			enroll( "video_sample_rate", context_->sample_rate );
			enroll( "video_channels", context_->channels );
			enroll( "video_delay", context_->delay );
			enroll( "video_qcompress", context_->qcompress );
			enroll( "video_qblur", context_->qblur );
			enroll( "video_qmin", context_->qmin );
			enroll( "video_qmax", context_->qmax );
			enroll( "video_max_qdiff", context_->max_qdiff );
			enroll( "video_max_b_frames", context_->max_b_frames );
			enroll( "video_b_quant_factor", context_->b_quant_factor );
			enroll( "video_b_frame_strategy", context_->b_frame_strategy );
			enroll( "video_rtp_payload_size", context_->rtp_payload_size );
			enroll( "video_codec_tag", context_->codec_tag );
			enroll( "video_workaround_bugs", context_->workaround_bugs );
			enroll( "video_strict_std_compliance", context_->strict_std_compliance );
			enroll( "video_b_quant_offset", context_->b_quant_offset );
			enroll( "video_has_b_frames", context_->has_b_frames );
			enroll( "video_mpeg_quant", context_->mpeg_quant );
			enroll( "video_rc_qsquish", context_->rc_qsquish );
			enroll( "video_rc_qmod_amp", context_->rc_qmod_amp );
			enroll( "video_rc_qmod_freq", context_->rc_qmod_freq );
			enroll( "video_rc_max_rate", context_->rc_max_rate );
			enroll( "video_rc_min_rate", context_->rc_min_rate );
			enroll( "video_rc_buffer_size", context_->rc_buffer_size );
			enroll( "video_rc_buffer_aggressivity", context_->rc_buffer_aggressivity );
			enroll( "video_i_quant_factor", context_->i_quant_factor );
			enroll( "video_i_quant_offset", context_->i_quant_offset );
			enroll( "video_rc_initial_cplx", context_->rc_initial_cplx );
			enroll( "video_dct_algo", context_->dct_algo );
			enroll( "video_lumi_masking", context_->lumi_masking );
			enroll( "video_temporal_cplx_masking", context_->temporal_cplx_masking );
			enroll( "video_spatial_cplx_masking", context_->spatial_cplx_masking );
			enroll( "video_p_masking", context_->p_masking );
			enroll( "video_dark_masking", context_->dark_masking );
			enroll( "video_idct_algo", context_->idct_algo );
			enroll( "video_prediction_method", context_->prediction_method );
			enroll( "video_me_cmp", context_->me_cmp );
			enroll( "video_me_sub_cmp", context_->me_sub_cmp );
			enroll( "video_mb_cmp", context_->mb_cmp );
			enroll( "video_ildct_cmp", context_->ildct_cmp );
			enroll( "video_dia_size", context_->dia_size );
			enroll( "video_last_predictor_count", context_->last_predictor_count );
			enroll( "video_pre_me", context_->pre_me );
			enroll( "video_me_pre_cmp", context_->me_pre_cmp );
			enroll( "video_pre_dia_size", context_->pre_dia_size );
			enroll( "video_me_subpel_quality", context_->me_subpel_quality );
			enroll( "video_me_range", context_->me_range );
			enroll( "video_intra_quant_bias", context_->intra_quant_bias );
			enroll( "video_inter_quant_bias", context_->inter_quant_bias );
			enroll( "video_global_quality", context_->global_quality );
			enroll( "video_coder_type", context_->coder_type );
			enroll( "video_context_model", context_->context_model );
			enroll( "video_mb_decision", context_->mb_decision );
			enroll( "video_scenechange_threshold", context_->scenechange_threshold );
			enroll( "video_lmin", context_->lmin );
			enroll( "video_lmax", context_->lmax );
			enroll( "video_noise_reduction", context_->noise_reduction );
			enroll( "video_rc_initial_buffer_occupancy", context_->rc_initial_buffer_occupancy );
			enroll( "video_flags2", context_->flags2 );
			enroll( "video_error_rate", context_->error_rate );
			enroll( "video_thread_count", context_->thread_count );
			enroll( "video_me_threshold", context_->me_threshold );
			enroll( "video_mb_threshold", context_->mb_threshold );
			enroll( "video_intra_dc_precision", context_->intra_dc_precision );
			enroll( "video_nsse_weight", context_->nsse_weight );
			enroll( "video_profile", context_->profile );
			enroll( "video_level", context_->level );
			enroll( "video_frame_skip_threshold", context_->frame_skip_threshold );
			enroll( "video_frame_skip_factor", context_->frame_skip_factor );
			enroll( "video_frame_skip_exp", context_->frame_skip_exp );
			enroll( "video_frame_skip_cmp", context_->frame_skip_cmp );
			enroll( "video_border_masking", context_->border_masking );
			enroll( "video_mb_lmin", context_->mb_lmin );
			enroll( "video_mb_lmax", context_->mb_lmax );
			enroll( "video_me_penalty_compensation", context_->me_penalty_compensation );
			enroll( "video_bidir_refine", context_->bidir_refine );
			enroll( "video_brd_scale", context_->brd_scale );
			enroll( "video_keyint_min", context_->keyint_min );
			enroll( "video_refs", context_->refs );
			enroll( "video_chromaoffset", context_->chromaoffset );
			enroll( "video_trellis", context_->trellis );
			enroll( "video_cutoff", context_->cutoff );
			enroll( "video_scenechange_factor", context_->scenechange_factor );
			enroll( "video_mv0_threshold", context_->mv0_threshold );
			enroll( "video_b_sensitivity", context_->b_sensitivity );
			enroll( "video_compression_level", context_->compression_level );
			enroll( "video_min_prediction_order", context_->min_prediction_order );
			enroll( "video_max_prediction_order", context_->max_prediction_order );
			enroll( "video_timecode_frame_start", context_->timecode_frame_start );
			enroll( "video_bits_per_raw_sample", context_->bits_per_raw_sample );
			enroll( "video_rc_max_available_vbv_use", context_->rc_max_available_vbv_use );
			enroll( "video_rc_min_vbv_overflow_use", context_->rc_min_vbv_overflow_use );
		}

		/// Implementation of the profile_property associated to this class
		void assign( const std::string &name, const cl::profile_op &op, const std::string &value )
		{
			ARENFORCE_MSG( op == cl::op_equals, "Only assignment operation may be used for profile property \"%1%\"" )
				( name );

			if ( name == "video_codec" )
			{
				video_codec_ = value;
				codec_ = avcodec_find_encoder_by_name( video_codec_.c_str( ) );
				ARENFORCE_MSG( codec_ != NULL, "avcodec_find_encoder_by_name call failed for codec %1%" )
					( video_codec_ );
				const int ret = avcodec_get_context_defaults3( context_, codec_ );
				ARENFORCE_MSG( ret == 0, "avcodec_get_context_defaults3 call failed with code %1% for codec %2%" )
					( ret )( video_codec_ );

				context_->pix_fmt = oil_to_avformat( pf_ );
				context_->width = dim_.width;
				context_->height = dim_.height;
				AVRational avr = { fps_.den, fps_.num };
				context_->time_base = avr;
				AVRational sar = { sar_.num, sar_.den };
				context_->sample_aspect_ratio = sar;

				context_->codec_type = avcodec_get_type( codec_->id );
				context_->codec_id = codec_->id;

				if ( video_codec_ == "mjpeg" )
					context_->pix_fmt = PIX_FMT_YUVJ420P;
			}
			else if ( name == "stream_codec_id" )
			{
				stream_codec_id_ = value;
			}
			else if ( name == OPT_NON_LINEAR_QUANT || name == OPT_INTRA_VLC )
			{
				ARENFORCE( boost::algorithm::starts_with( name, OPT_PREFIX ) );

				int int_val;
				ARENFORCE_MSG( sscanf( value.c_str(), "%i", &int_val ) == 1, 
					"Failed to interpret the value \"%1%\" of profile property \"%2%\" as an integer." )
					( value )( name );

				const int av_opt_result = av_opt_set_int( context_->priv_data, name.c_str() + strlen( OPT_PREFIX ), int_val, 0 );
				ARENFORCE_MSG( av_opt_result == 0, "av_opt_set_int failed for profile property \"%1%\"" )
					( name )( av_opt_result );
			}
		}

		/// String serialisation courtesy function
		std::string serialise( const std::string &name ) const
		{
			std::string result;
			if ( name == "video_codec" ) result = video_codec_;
			if ( name == "stream_codec_id" ) result = stream_codec_id_;
			return result;
		}

		AVCodecContext *context( )
		{
			return context_;
		}

		/// Provide the codec itself
		AVCodec *codec( )
		{
			if( codec_ && !codec_opened_ )
			{
				const int ret = avcodec_open2( context_, codec_, 0 );
				if( ret < 0 )
				{
					std::cerr << "Unable to open codec (code: " << ret << ")" << std::endl;
					codec_ = 0;
				}
				codec_opened_ = true;
			}

			return codec_;
		}

		/// Encode the frame provided
		ml::stream_type_ptr encode( frame_type_ptr frame )
		{
			stream_avformat *stream = 0;

			if( codec( ) )
			{
				bool new_image = frame && frame->get_image( );
				int got_packet = 0, error = 0;
				
				av_init_packet( &pkt_ );
				pkt_.data = outbuf_;
				pkt_.size = outbuf_size_;

				if ( new_image )
				{
                    boost::shared_ptr< ml::image::image_type_8 > image_type =
                        ml::image::coerce< ml::image::image_type_8 >( frame->get_image( ) );
					picture_->data[ 0 ] = image_type->data( 0 );
					picture_->data[ 1 ] = image_type->data( 1 );
					picture_->data[ 2 ] = image_type->data( 2 );
					picture_->linesize[ 0 ] = frame->get_image( )->pitch( 0 );
					picture_->linesize[ 1 ] = frame->get_image( )->pitch( 1 );
					picture_->linesize[ 2 ] = frame->get_image( )->pitch( 2 );

					if ( field_order_ != ml::image::progressive )
					{
						picture_->interlaced_frame = 1;
						picture_->top_field_first = ( field_order_ == ml::image::top_field_first );
					}

					error = avcodec_encode_video2( context_, &pkt_, picture_, &got_packet );
				}
				else
				{
					error = avcodec_encode_video2( context_, &pkt_, 0, &got_packet );
				}
				
				ARENFORCE_MSG( error == 0, "Failed to encode frame. Error = %1%" )( error );

				if ( context_->coded_frame && context_->coded_frame->key_frame )
					key_ = position_;

				stream = new stream_avformat( stream_codec_id_, pkt_.size, position_, key_, context_->bit_rate, dim_, sar_, pf_, field_order_, context_->gop_size );

				if( got_packet ) {
					memcpy( stream->bytes( ), outbuf_, pkt_.size );
					position_++;
				}
			}

			return ml::stream_type_ptr( stream );
		}
	
		const std::string& video_codec( ) const
		{
			return video_codec_;
		}
		
		const std::string& stream_codec_id( ) const
		{
			return stream_codec_id_;
		}

	private:
		AVCodecContext *context_;
		AVCodec *codec_;
		AVFrame *picture_;
		AVPacket pkt_;
		bool codec_opened_;
		std::string video_codec_;
		std::string stream_codec_id_;
		int outbuf_size_;
		boost::uint8_t *outbuf_;
		int position_;
		int key_;
		ml::dimensions dim_;
		ml::fraction sar_;
		ml::fraction fps_;
		t_string pf_;
		ml::image::field_order_flags field_order_;
};

} } }

#endif
