

#include <emmintrin.h>

#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/log_defines.hpp>

extern "C" {

#include <libavutil/intreadwrite.h>
#include <libavcodec/avcodec.h>
	
#define AES3_HEADER_LEN 4
	
typedef struct aml_AES3DecodeContext {
	AVFrame frame;
} aml_AES3DecodeContext;
	

#define CONV_SLOW_16() dst[b0] = buf[1]>>4 | buf[2]<<4; \
						dst[b1] = buf[2]>>4 | buf[3]<<4; dst += 2; buf += 4
#define CONV_SLOW_24() dst[b0] = buf[0]>>4 | buf[1]<<4; \
						dst[b1] = buf[1]>>4 | buf[2]<<4; dst[b2] = buf[2]>>4 | buf[3]<<4; \
						dst += 3; buf += 4
#define CONV_SLOW_32() dst[b0] = (buf[0]>>4&0xff) | (buf[1]<<4&0xff); \
						dst[b1] = (buf[1]>>4&0xff) | (buf[2]<<4&0xff); \
						dst[b2] = (buf[2]>>4&0xff) | (buf[3]<<4&0xff); \
						dst[b3] = 0; dst += 4; buf += 4

	
static int aml_AES3_decode_frame(AVCodecContext *avctx, void *data,
								  int *got_frame_ptr, AVPacket *avpkt)
{
	int ret = 0;
	aml_AES3DecodeContext *s = reinterpret_cast< aml_AES3DecodeContext * >( avctx->priv_data );
	const uint8_t *buf = avpkt->data;
	int buf_size       = avpkt->size;
	
	__m128i (*sse_load_funtion)(__m128i const *p) = _mm_load_si128;
	
	if (memcmp(buf, "\x80\x80\x07", 3) == 0 || memcmp(buf, "\x00\x80\x07", 3) == 0) {
		// PAL
		buf += 4;
		buf_size -= 4;
		
	} else if ((buf[0] & 0xff) >= 0x80 && (buf[0] & 0xff) <= 0x85 &&
			   buf[1] >= 0x40 && buf[1] <= 0x42 && buf[2] == 0x06) {
		// NTSC
		buf += 4;
		buf_size -= 4;
	}
	
	// If we dont have correct byte allignment then use unalligned SSE load
	if( ( reinterpret_cast< boost::int64_t >( buf ) % 16 ) != 0 )
		sse_load_funtion = _mm_loadu_si128;
	
	int tpf = 0, samps_per_frame[] = { 1920, 1601, 1602 };
	for( int i = 0; i < (int)(sizeof(samps_per_frame) / sizeof(int)); i++) {
		if( samps_per_frame[ i ] * 32 == buf_size )
		{
			tpf = 8;
			s->frame.nb_samples = samps_per_frame[ i ];
			avctx->sample_fmt = AV_SAMPLE_FMT_S32;
			break;
		}
		else if( samps_per_frame[ i ] * 16 == buf_size )
		{
			tpf = 4;
			s->frame.nb_samples = samps_per_frame[ i ];
			avctx->sample_fmt = AV_SAMPLE_FMT_S16;
			break;
		}
	}
	
	if( ( ret = avctx->get_buffer( avctx, &s->frame ) ) < 0 ) {
		//ARLOG_ERR( "Failed to allocate buffer" );
		return ret;
    }
	
	unsigned char * dst = s->frame.data[ 0 ];
	
	if( avctx->bits_per_raw_sample == 16 )
	{
		if( tpf == 4 || avctx->channels <= 4 )
		{
			int source_step_size = 4 * tpf;
			int dst_step_size = 2 * avctx->channels;
			
			int num_steps_in_src = buf_size / source_step_size;
			int num_steps_in_dest = s->frame.nb_samples;
			
			int total_iterations = std::min( num_steps_in_src, num_steps_in_dest );
			
			if( avctx->channels == 4 )
			{
				// We can do double the amount in one iterations since each _mm_packs_epi32
				// now can contain valid data in both arguments
				total_iterations /= 2;
				
				while( total_iterations-- > 0 )
				{
					__m128i src = sse_load_funtion( reinterpret_cast< const __m128i * >( buf ) );
					__m128i l_shift = _mm_slli_epi32( src, 4 );
					__m128i final = _mm_srai_epi32( l_shift, 16 );
					
					buf += source_step_size;

					src = sse_load_funtion( reinterpret_cast< const __m128i * >( buf ) );
					l_shift = _mm_slli_epi32( src, 4 );
					__m128i final_2 = _mm_srai_epi32( l_shift, 16 );
					
					__m128i res = _mm_packs_epi32( final, final_2 );
					
					_mm_storeu_si128( reinterpret_cast< __m128i * >( dst ), res );
					
					buf += source_step_size;
					dst += dst_step_size * 2;
				}
			}
			else
			{
				while( total_iterations-- > 0 )
				{
					__m128i src = sse_load_funtion( reinterpret_cast< const __m128i * >( buf ) );
					__m128i l_shift = _mm_slli_epi32( src, 4 );
					__m128i final = _mm_srai_epi32( l_shift, 16 );
					
					__m128i res = _mm_packs_epi32( final, final );
					
					_mm_storeu_si128( reinterpret_cast< __m128i * >( dst ), res );
					
					buf += source_step_size;
					dst += dst_step_size;
				}
			}
		}
		else
		{
			assert( tpf == 8 );
			
			int source_step_size = 16;
			int dst_step_size = 8 + ( ( avctx->channels - 4 ) * 2 );
			
			int num_steps_in_src = buf_size / ( source_step_size * 2 );
			int num_steps_in_dest = s->frame.nb_samples;
			
			int total_iterations = std::min( num_steps_in_src, num_steps_in_dest );
						
			while( total_iterations-- > 0 )
			{
				__m128i src = sse_load_funtion( reinterpret_cast< const __m128i * >( buf ) );
				__m128i l_shift = _mm_slli_epi32( src, 4 );
				__m128i final = _mm_srai_epi32( l_shift, 16 );
				
				buf += source_step_size;
				
				src = sse_load_funtion( reinterpret_cast< const __m128i * >( buf ) );
				l_shift = _mm_slli_epi32( src, 4 );
				__m128i final_2 = _mm_srai_epi32( l_shift, 16 );
				
				__m128i res = _mm_packs_epi32( final, final_2 );
				
				_mm_storeu_si128( reinterpret_cast< __m128i * >( dst ), res );
				
				buf += source_step_size;
				dst += dst_step_size;
			}
		}
	}
	else if( avctx->bits_per_raw_sample == 24 )
	{
		if( tpf == 4 || avctx->channels <= 4 )
		{
			int source_step_size = 4 * tpf;
			int dst_step_size = 4 * avctx->channels;
			
			int num_steps_in_src = buf_size / source_step_size;
			int num_steps_in_dest = s->frame.nb_samples;
			
			int total_iterations = std::min( num_steps_in_src, num_steps_in_dest );
			
			while( total_iterations-- > 0 )
			{
				__m128i src = sse_load_funtion( reinterpret_cast< const __m128i * >( buf ) );
				__m128i l_shift = _mm_slli_epi32( src, 4 );
				__m128i final = _mm_srai_epi32( l_shift, 8 );
				_mm_storeu_si128( reinterpret_cast< __m128i * >( dst ), final );
				
				buf += source_step_size;
				dst += dst_step_size;
			}
		}
		else
		{
			assert( tpf == 8 );
			
			int source_step_size = 16;
			int dst_step_size_1 = 16;
			int dst_step_size_2 = ( avctx->channels - 4 ) * 4;
			
			int num_steps_in_src = buf_size / ( source_step_size * 2 );
			int num_steps_in_dest = s->frame.nb_samples;
			
			int total_iterations = std::min( num_steps_in_src, num_steps_in_dest );
			
			while( total_iterations-- > 0 )
			{
				__m128i src = sse_load_funtion( reinterpret_cast< const __m128i * >( buf ) );
				__m128i l_shift = _mm_slli_epi32( src, 4 );
				__m128i final = _mm_srai_epi32( l_shift, 8 );
				_mm_storeu_si128( reinterpret_cast< __m128i * >( dst ), final );
				
				buf += source_step_size;
				dst += dst_step_size_1;
				
				
				src = sse_load_funtion( reinterpret_cast< const __m128i * >( buf ) );
				l_shift = _mm_slli_epi32( src, 4 );
				final = _mm_srai_epi32( l_shift, 8 );
				_mm_storeu_si128( reinterpret_cast< __m128i * >( dst ), final );
				
				buf += source_step_size;
				dst += dst_step_size_2;
			}
		}
	}
	else
	{
		return 0;
	}
	
	*got_frame_ptr   = 1;
	*(AVFrame *)data = s->frame;
	
	return avpkt->size;
}

static int aml_AES3_decode_init(AVCodecContext *avctx)
{
	aml_AES3DecodeContext *s = reinterpret_cast< aml_AES3DecodeContext * >( avctx->priv_data );
	
	avcodec_get_frame_defaults(&s->frame);
	avctx->coded_frame = &s->frame;
	
	return 0;
}

static AVCodec aml_AES3_decoder;
	
}

namespace olib { namespace openmedialib { namespace ml {
	
extern const int AML_AES3_CODEC_ID = 0x17FFF;

void register_aml_aes3( )
{
	memset( &aml_AES3_decoder, 0, sizeof( AVCodec ) );
	
	aml_AES3_decoder.name           = "aml-aes3",
	aml_AES3_decoder.type           = AVMEDIA_TYPE_AUDIO,
	aml_AES3_decoder.id             = static_cast< enum CodecID >( AML_AES3_CODEC_ID ),
	aml_AES3_decoder.priv_data_size = sizeof(aml_AES3DecodeContext),
	aml_AES3_decoder.init           = aml_AES3_decode_init,
	aml_AES3_decoder.decode         = aml_AES3_decode_frame,
	aml_AES3_decoder.capabilities   = CODEC_CAP_DR1,
	
	avcodec_register( &aml_AES3_decoder );
}

} } }
