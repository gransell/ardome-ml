
#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/packet.hpp>
#include <boost/thread/recursive_mutex.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <iostream>

namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;

namespace olib { namespace openmedialib { namespace ml {

extern const std::wstring avformat_to_oil( int );
extern const int oil_to_avformat( const std::wstring & );
extern il::image_type_ptr convert_to_oil( AVFrame *, PixelFormat, int, int );

/// The packet decoder interface.
class ML_PLUGIN_DECLSPEC avformat_decoder : public ml::packet_decoder
{
	public:
		typedef boost::recursive_mutex::scoped_lock scoped_lock;

		avformat_decoder( enum ml::packet_id id, CodecID codec )
			: ml::packet_decoder( )
			, mutex_( )
			, id_( id )
			, codec_( codec )
		{
		}

		/// Destructor
		virtual ~avformat_decoder( ) 
		{
			for ( std::vector< AVCodecContext * >::iterator iter = codecs_.begin( ); iter != codecs_.end( ); iter ++ )
				avcodec_close( *iter );
		}

		/// Dictates which type can be decoded by this decoder
		virtual enum ml::packet_id id( )
		{
			return id_;
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
					const PixelFormat fmt = context->pix_fmt;
					const int width = context->width;
					const int height = context->height;
					image = convert_to_oil( frame, fmt, width, height );
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
				AVCodec *codec = avcodec_find_decoder( codec_ );
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
		enum ml::packet_id id_; 
		CodecID codec_;
};

void ML_PLUGIN_DECLSPEC register_dv_decoder( )
{
	packet_handler( ml::packet_decoder_ptr( new avformat_decoder( ml::dv25, CODEC_ID_DVVIDEO ) ) );
	packet_handler( ml::packet_decoder_ptr( new avformat_decoder( ml::dv50, CODEC_ID_DVVIDEO ) ) );
	packet_handler( ml::packet_decoder_ptr( new avformat_decoder( ml::dv100, CODEC_ID_DVVIDEO ) ) );
	packet_handler( ml::packet_decoder_ptr( new avformat_decoder( ml::imx, CODEC_ID_MPEG2VIDEO ) ) );
	packet_handler( ml::packet_decoder_ptr( new avformat_decoder( ml::png, CODEC_ID_PNG ) ) );
	packet_handler( ml::packet_decoder_ptr( new avformat_decoder( ml::jpeg, CODEC_ID_JPEG2000 ) ) );
}

} } }
