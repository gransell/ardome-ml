#ifndef AML_AVFORMAT_WRAPPERS_H
#define AML_AVFORMAT_WRAPPERS_H

#include <string>

#include <openmedialib/ml/packet.hpp>
#include <opencorelib/cl/profile.hpp>

#include "avformat_stream.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

namespace cl = olib::opencorelib;
namespace ml = olib::openmedialib::ml;
namespace il = olib::openimagelib::il;
namespace pl = olib::openpluginlib;

namespace olib { namespace openmedialib { namespace ml {

extern const std::wstring avformat_to_oil( int );
extern const PixelFormat oil_to_avformat( const std::wstring & );
extern il::image_type_ptr convert_to_oil( AVFrame *, PixelFormat, int, int );

class avformat_video : public cl::profile_wrapper, public cl::profile_property
{
	public:
		avformat_video( ml::frame_type_ptr frame )
			: profile_wrapper( false )
			, profile_property( false )
			, context_( 0 )
			, codec_( 0 )
			, picture_( 0 )
			, video_codec_( "" )
			, stream_codec_id_( "" )
			, outbuf_size_( 0 )
			, outbuf_( 0 )
			, position_( 0 )
			, key_( 0 )
			, dim_( 0, 0 )
			, sar_( 0, 0 )
		{ 
			init( frame ); 
		}

		virtual ~avformat_video( )
		{
			#ifndef WIN32
			if ( codec_ && context_ && context_->thread_count > 1 )
				avcodec_thread_free( context_ );
			#endif

			av_free( context_ );
			av_free( picture_ );

			free( outbuf_ );
		}

		void init( ml::frame_type_ptr frame )
		{
			init( ); 
			pf_ = frame->pf( );
			dim_ = ml::dimensions( frame->width( ), frame->height( ) );
			sar_ = ml::fraction( frame->get_sar_num( ), frame->get_sar_den( ) );
			context_->pix_fmt = oil_to_avformat( frame->pf( ) );
			context_->width = frame->width( );
			context_->height = frame->height( );
			AVRational avr = { frame->get_fps_den( ), frame->get_fps_num( ) };
			context_->time_base = avr;
			AVRational sar = { frame->get_sar_num( ), frame->get_sar_den( ) };
			context_->sample_aspect_ratio = sar;
			outbuf_size_ = std::max< int >( 1024 * 256, context_->width * context_->height * 6 + 200 );
			outbuf_ = ( boost::uint8_t * )malloc( outbuf_size_ );
		}

		void init( )
		{
			picture_ = avcodec_alloc_frame( );
			context_ = avcodec_alloc_context( );
			enroll( "video_codec", this );
			enroll( "stream_codec_id", this );
			enroll( "video_bit_rate", context_->bit_rate );
			enroll( "video_bit_rate_tolerance", context_->bit_rate_tolerance );
			enroll( "video_flags", context_->flags );
			enroll( "video_sub_id", context_->sub_id );
			enroll( "video_me_method", context_->me_method );
			enroll( "video_gop_size", context_->gop_size );
			enroll( "video_rate_emu", context_->rate_emu );
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
			enroll( "video_luma_elim_threshold", context_->luma_elim_threshold );
			enroll( "video_chroma_elim_threshold", context_->chroma_elim_threshold );
			enroll( "video_strict_std_compliance", context_->strict_std_compliance );
			enroll( "video_b_quant_offset", context_->b_quant_offset );
			enroll( "video_has_b_frames", context_->has_b_frames );
			enroll( "video_mpeg_quant", context_->mpeg_quant );
			enroll( "video_rc_qsquish", context_->rc_qsquish );
			enroll( "video_rc_qmod_amp", context_->rc_qmod_amp );
			enroll( "video_rc_qmod_freq", context_->rc_qmod_freq );
			//enroll( "video_rc_eq", context_->rc_eq );
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
			enroll( "video_dsp_mask", context_->dsp_mask );
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
			enroll( "video_inter_threshold", context_->inter_threshold );
			enroll( "video_flags2", context_->flags2 );
			enroll( "video_error_rate", context_->error_rate );
			enroll( "video_quantizer_noise_shaping", context_->quantizer_noise_shaping );
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
			enroll( "video_crf", context_->crf );
			enroll( "video_cqp", context_->cqp );
			enroll( "video_keyint_min", context_->keyint_min );
			enroll( "video_refs", context_->refs );
			enroll( "video_chromaoffset", context_->chromaoffset );
			enroll( "video_bframebias", context_->bframebias );
			enroll( "video_trellis", context_->trellis );
			enroll( "video_complexityblur", context_->complexityblur );
			enroll( "video_deblockalpha", context_->deblockalpha );
			enroll( "video_deblockbeta", context_->deblockbeta );
			enroll( "video_partitions", context_->partitions );
			enroll( "video_directpred", context_->directpred );
			enroll( "video_cutoff", context_->cutoff );
			enroll( "video_scenechange_factor", context_->scenechange_factor );
			enroll( "video_mv0_threshold", context_->mv0_threshold );
			enroll( "video_b_sensitivity", context_->b_sensitivity );
			enroll( "video_compression_level", context_->compression_level );
			enroll( "video_use_lpc", context_->use_lpc );
			enroll( "video_lpc_coeff_precision", context_->lpc_coeff_precision );
			enroll( "video_min_prediction_order", context_->min_prediction_order );
			enroll( "video_max_prediction_order", context_->max_prediction_order );
			enroll( "video_prediction_order_method", context_->prediction_order_method );
			enroll( "video_min_partition_order", context_->min_partition_order );
			enroll( "video_max_partition_order", context_->max_partition_order );
			enroll( "video_timecode_frame_start", context_->timecode_frame_start );
			enroll( "video_bits_per_raw_sample", context_->bits_per_raw_sample );
			enroll( "video_channel_layout", context_->channel_layout );
			enroll( "video_rc_max_available_vbv_use", context_->rc_max_available_vbv_use );
			enroll( "video_rc_min_vbv_overflow_use", context_->rc_min_vbv_overflow_use );
		}

		/// Implementation of the profile_property associated to this class
		void assign( const std::string &name, const cl::profile_op &op, const std::string &value )
		{
			if ( name == "video_codec" ) video_codec_ = value;
			if( name == "stream_codec_id" ) stream_codec_id_ = value;
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
			if ( !codec_ && video_codec_ != "" )
			{
				codec_ = avcodec_find_encoder_by_name( video_codec_.c_str( ) );

				if ( codec_ && avcodec_open( context_, codec_ ) < 0 ) 
				{
					std::cerr << "Unable to open codec" << std::endl;
					codec_ = 0;
				}

				#ifndef WIN32
				if ( codec_ && context_->thread_count > 1 )
				{
					avcodec_thread_init( context_, context_->thread_count );
				}
				#endif
			}

			return codec_;
		}

		/// Encode the frame provided
		ml::stream_type_ptr encode( frame_type_ptr frame )
		{
			stream_avformat *stream = 0;

			if( codec( ) )
			{
				int out_size = 0;
				bool new_image = frame && frame->get_image( );

				if ( new_image )
				{
					picture_->data[ 0 ] = frame->get_image( )->data( 0 );
					picture_->data[ 1 ] = frame->get_image( )->data( 1 );
					picture_->data[ 2 ] = frame->get_image( )->data( 2 );
					picture_->linesize[ 0 ] = frame->get_image( )->pitch( 0 );
					picture_->linesize[ 1 ] = frame->get_image( )->pitch( 1 );
					picture_->linesize[ 2 ] = frame->get_image( )->pitch( 2 );

					if ( frame->get_image( )->field_order( ) != il::progressive )
					{
						picture_->interlaced_frame  = 1;
						picture_->top_field_first = frame->get_image( )->field_order( ) == il::top_field_first;
					}

					out_size = avcodec_encode_video( context_, outbuf_, outbuf_size_, picture_ );
				}
				else
				{
					out_size = avcodec_encode_video( context_, outbuf_, outbuf_size_, 0 );
				}

				if ( context_->coded_frame && context_->coded_frame->key_frame )
					key_ = position_;

				stream = new stream_avformat( stream_codec_id_, out_size, position_, key_, context_->bit_rate, dim_, sar_, pf_ );

				if ( out_size )
					memcpy( stream->bytes( ), outbuf_, out_size );

				if ( out_size )
					position_ ++;
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
		std::string video_codec_;
		std::string stream_codec_id_;
		int outbuf_size_;
		boost::uint8_t *outbuf_;
		int position_;
		int key_;
		ml::dimensions dim_;
		ml::fraction sar_;
		pl::wstring pf_;
};

} } }

#endif
