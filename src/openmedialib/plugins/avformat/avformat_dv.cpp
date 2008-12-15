
#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/packet.hpp>
#include <boost/thread/recursive_mutex.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
extern int img_convert(AVPicture *dst, int dst_pix_fmt, const AVPicture *src, int src_pix_fmt, int src_width, int src_height);
}

#include <iostream>

namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;

namespace olib { namespace openmedialib { namespace ml {

const std::wstring avformat_to_oil( int fmt );
const int oil_to_avformat( const std::wstring &fmt );

/// The packet decoder interface.
class ML_PLUGIN_DECLSPEC avformat_dv_decoder : public ml::packet_decoder
{
	public:
		typedef boost::recursive_mutex::scoped_lock scoped_lock;

		avformat_dv_decoder( )
			: ml::packet_decoder( )
			, mutex_( )
		{
		}

		/// Destructor
		virtual ~avformat_dv_decoder( ) 
		{
			for ( std::vector< AVCodecContext * >::iterator iter = codecs_.begin( ); iter != codecs_.end( ); iter ++ )
				avcodec_close( *iter );
		}

		/// Dictates which type can be decoded by this decoder
		virtual enum ml::packet_id id( )
		{
			return ml::dv25;
		}

		/// Decode the packet on the frame
		virtual il::image_type_ptr decode( ml::packet_type_ptr pkt )
		{
			il::image_type_ptr image;
			AVCodecContext *context = acquire( );
			if ( context )
			{
				AVFrame *frame = avcodec_alloc_frame( );
				int got = 0;

				if ( avcodec_decode_video( context, frame, &got, pkt->bytes( ), pkt->length( ) ) >= 0 )
				{
					dimensions size = pkt->size( );
					int width = size.width;
					int height = size.height;

					std::wstring format = avformat_to_oil( context->pix_fmt );

 					if ( format == L"" )
 					{
 						image = il::allocate( L"yuv420p", width, height );
         				AVPicture output;
         				avpicture_fill( &output, image->data( ), PIX_FMT_YUV420P, width, height );
         				img_convert( &output, PIX_FMT_YUV420P, (AVPicture *)frame, context->pix_fmt, width, height );
 					}
 					else
 					{
 						image = il::allocate( format, width, height );
 						AVPicture output;
 						avpicture_fill( &output, image->data( ), context->pix_fmt, width, height );
 						img_convert( &output, context->pix_fmt, (AVPicture *)frame, context->pix_fmt, width, height );
 					}

					if ( frame->interlaced_frame )
						image->set_field_order( frame->top_field_first ? il::top_field_first : il::bottom_field_first );

					av_free( frame );
				}

				release( context );
			}

			return image;
		}

	private:
		AVCodecContext *acquire( )
		{
			scoped_lock lock( mutex_ );
			AVCodecContext *result = 0;

			if ( codecs_.size( ) == 0 )
			{
				AVCodecContext *context = avcodec_alloc_context( );
				AVCodec *codec = avcodec_find_decoder( CODEC_ID_DVVIDEO );
				if ( avcodec_open( context, codec ) >= 0 )
					codecs_.push_back( context );
				else
					avcodec_close( context );
			}
			
			if ( codecs_.size( ) > 0 )
			{
				result = codecs_[ 0 ];
				codecs_.erase( codecs_.begin( ) );
			}

			return result;
		}

		void release( AVCodecContext *codec )
		{
			scoped_lock lock( mutex_ );
			codecs_.push_back( codec );
		}

		std::vector< AVCodecContext * > codecs_;
		boost::recursive_mutex mutex_;
};

void ML_PLUGIN_DECLSPEC register_dv_decoder( )
{
	packet_handler( ml::packet_decoder_ptr( new avformat_dv_decoder( ) ) );
}

} } }
