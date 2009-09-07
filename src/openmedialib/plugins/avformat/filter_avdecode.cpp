
#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/packet.hpp>
#include <openpluginlib/pl/pcos/isubject.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>

#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}


namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;

namespace olib { namespace openmedialib { namespace ml {

extern std::map< CodecID, std::string > codec_name_lookup_;
extern std::map< std::string, CodecID > name_codec_lookup_;

extern const std::wstring avformat_to_oil( int );
extern const int oil_to_avformat( const std::wstring & );
extern il::image_type_ptr convert_to_oil( AVFrame *, PixelFormat, int, int );


class stream_queue
{
	public:
		typedef audio< unsigned char, pcm16 > pcm16_audio_type;

		stream_queue( ml::input_type_ptr input )
			: input_( input )
			, keys_( 0 )
			, context_( 0 )
			, codec_( 0 )
			, expected_( 0 )
			, frame_( avcodec_alloc_frame( ) )
			, offset_( 0 )
		{
			audio_buf_size_ = (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2;
			audio_buf_ = ( uint8_t * )av_malloc( audio_buf_size_ );
		}

		virtual ~stream_queue( )
		{
			if ( context_ )
				avcodec_close( context_ );
			av_free( frame_ );
		}

		void used( int position )
		{
			if ( lru_map_.find( position ) != lru_map_.end( ) )
			{
				int offset = lru_map_[ position ];
				lru_.erase( lru_.begin( ) + offset );
				lru_map_.erase( lru_map_.find( position ) );
				for ( std::deque< int >::iterator iter = lru_.begin( ) + offset; iter != lru_.end( ); iter ++ )
					lru_map_[ *iter ] --;
			}

			lru_map_[ position ] = int( lru_.size( ) );
			lru_.push_back( position );

			if ( lru_.size( ) > 100 )
			{
				int oldest = lru_.front( );
				lru_.pop_front( );
				lru_map_.erase( lru_map_.find( oldest ) );
				frames_.erase( frames_.find( oldest ) );
				if ( images_.find( oldest ) != images_.end( ) )
					images_.erase( images_.find( oldest ) );
				if ( audios_.find( oldest ) != audios_.end( ) )
					audios_.erase( audios_.find( oldest ) );
				for ( std::deque< int >::iterator iter = lru_.begin( ); iter != lru_.end( ); iter ++ )
					lru_map_[ *iter ] --;
			}
		}

		ml::frame_type_ptr fetch( int position )
		{
			ml::frame_type_ptr result;

			if ( frames_.find( position ) == frames_.end( ) )
			{
				//std::cerr << "fetching from input " << position << std::endl;
				input_->seek( position );
				result = input_->fetch( );
				while ( result && result->get_stream( ) )
				{
					//std::cerr << "obtained from input " << result->get_position( ) << " for " << position << std::endl;
					frames_[ result->get_position( ) ] = result;
					used( result->get_position( ) );
					if ( result->get_position( ) == position )
						break;
					input_->seek( result->get_position( ) + 1 );
					result = input_->fetch( );
				}
			}
			else
			{
				used( position );
				result = frames_[ position ];
			}

			return result;
		}

		il::image_type_ptr decode_image( int position )
		{
			if ( images_.find( position ) != images_.end( ) )
			{
				used( position );
				return images_[ position ];
			}

			ml::frame_type_ptr frame = fetch( position );

			if ( frame && frame->get_stream( ) )
			{
				int start = expected_;

				if ( position + offset_ != expected_ )
					start = frame->get_stream( )->key( );

				if ( start == position && position != 0 )
					start = fetch( start - 1 )->get_stream( )->key( );

				ml::frame_type_ptr temp = ml::frame_type_ptr( new frame_type( fetch( start ++ ) ) );

				while( temp && temp->get_stream( ) && decode( temp, position ) )
					temp = ml::frame_type_ptr( new frame_type( fetch( start ++ ) ) );

				return temp->get_image( );
			}

			return il::image_type_ptr( );
		}

		ml::audio_type_ptr decode_audio( int position )
		{
			if ( audios_.find( position ) != audios_.end( ) )
			{
				used( position );
				return audios_[ position ];
			}

			ml::frame_type_ptr frame = fetch( position );

			if ( frame && frame->get_stream( ) )
			{
				int start = expected_;

				if ( position + offset_ != expected_ )
					start = frame->get_stream( )->key( );

				if ( start == position && position != 0 )
					start = fetch( start - 1 )->get_stream( )->key( );

				ml::frame_type_ptr temp = ml::frame_type_ptr( new frame_type( fetch( start ++ ) ) );

				while( temp && temp->get_stream( ) && decode( temp, position ) )
					temp = ml::frame_type_ptr( new frame_type( fetch( start ++ ) ) );

				return temp->get_audio( );
			}

			return ml::audio_type_ptr( );
		}

		bool decode( ml::frame_type_ptr &result, int position )
		{
			bool found = false;

			if ( context_ == 0 )
			{
				context_ = avcodec_alloc_context( );
				codec_ = avcodec_find_decoder( name_codec_lookup_[ result->get_stream( )->codec( ) ] );
				avcodec_open( context_, codec_ );
				avcodec_thread_init( context_, 4 );
				avcodec_flush_buffers( context_ );
			}

			ml::stream_type_ptr pkt = result->get_stream( );

			if ( result->get_position( ) != expected_ )
			{
				if ( pkt->id( ) == ml::stream_video )
					avcodec_flush_buffers( context_ );
				expected_ = result->get_position( );
			}

			int got = 0;
			int audio_size = audio_buf_size_;

			switch( pkt->id( ) )
			{
				case ml::stream_video:
					if ( avcodec_decode_video( context_, frame_, &got, pkt->bytes( ), pkt->length( ) ) >= 0 )
					{
						if ( got )
						{
							il::image_type_ptr image;
							const PixelFormat fmt = context_->pix_fmt;
							const int width = context_->width;
							const int height = context_->height;
							image = convert_to_oil( frame_, fmt, width, height );
							image->set_position( pkt->position( ) );
							result->set_image( image );

							if ( position == 0 )
							{
								//std::cerr << "found first " << result->get_position( ) << " " << expected_ << std::endl;
								offset_ = result->get_position( );
							}

							images_[ result->get_position( ) - offset_ ] = image;

							if ( result->get_position( ) >= position + offset_ )
								found = true;
						}
						else
						{
							//std::cerr << "no image for " << result->get_position( ) << std::endl;
							offset_ = 1;
						}
	
						expected_ ++;
					}

					break;

				case ml::stream_audio:
		   			if ( avcodec_decode_audio2( context_, ( short * )( audio_buf_ ), &audio_size, pkt->bytes( ), pkt->length( ) ) >= 0 )
					{
						int channels = context_->channels;
						int frequency = context_->sample_rate;

						audio_type_ptr audio = audio_type_ptr( new audio_type( pcm16_audio_type( frequency, channels, audio_size / channels / 2 ) ) );
						memcpy( audio->data( ), audio_buf_, audio->size( ) );
						audio->set_position( pkt->position( ) );
						result->set_audio( audio );

						if ( result->get_position( ) >= position + offset_ )
							found = true;

						expected_ ++;
					}
					break;

				default:
					break;
			}

			return !found;
		}

	private:
		ml::input_type_ptr input_;
		std::map< int, ml::frame_type_ptr > frames_;
		std::map< int, il::image_type_ptr > images_;
		std::map< int, ml::audio_type_ptr > audios_;

		std::deque< int > lru_;
		std::map< int, int > lru_map_;

		int keys_;
		AVCodecContext *context_;
		AVCodec *codec_;
		int expected_;
		AVFrame *frame_;
		int audio_buf_size_;
		boost::uint8_t *audio_buf_;
		int offset_;
};

typedef boost::shared_ptr< stream_queue > stream_queue_ptr;

class ML_PLUGIN_DECLSPEC frame_avformat : public ml::frame_type 
{
	public:
		/// Constructor
		frame_avformat( const frame_type_ptr &other, const stream_queue_ptr &queue )
			: ml::frame_type( *other )
			, queue_( queue )
		{
		}

		/// Destructor
		virtual ~frame_avformat( )
		{
		}

		/// Indicates if the frame has an image
		virtual bool has_image( )
		{
			return image_ || stream_->id( ) == ml::stream_video;
		}

		/// Indicates if the frame has audio
		virtual bool has_audio( )
		{
			return audio_ || stream_->id( ) == ml::stream_audio;
		}

		/// Set the image associated to the frame.
		virtual void set_image( olib::openimagelib::il::image_type_ptr image )
		{
			image_ = image;
		}

		/// Get the image associated to the frame.
		virtual olib::openimagelib::il::image_type_ptr get_image( )
		{
			if ( !image_ && ( stream_ && stream_->id( ) == ml::stream_video ) )
				image_ = queue_->decode_image( stream_->position( ) );
			return image_;
		}

		/// Set the audio associated to the frame.
		virtual void set_audio( audio_type_ptr audio )
		{
			audio_ = audio;
		}

		/// Get the audio associated to the frame.
		virtual audio_type_ptr get_audio( )
		{
			if ( !audio_ && ( stream_ && stream_->id( ) == ml::stream_audio ) )
				audio_ = queue_->decode_audio( stream_->position( ) );
			return audio_;
		}

		// Calculate the duration of the frame
		virtual double get_duration( ) const
		{
			int num, den;
			get_fps( num, den );
			return double( den ) / double( num );
		}

	private:
		stream_queue_ptr queue_;
};

class avformat_decode_filter : public filter_type
{
	public:
		// Filter_type overloads
		avformat_decode_filter( )
			: ml::filter_type( )
			, initialised_( false )
			, queue_( )
		{
		}

		virtual ~avformat_decode_filter( )
		{
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// This provides the name of the plugin (used in serialisation)
		virtual const pl::wstring get_uri( ) const { return L"avdecode"; }

		virtual int get_frames( ) const
		{
			if ( fetch_slot( 0 ) ) 
				return fetch_slot( 0 )->get_frames( );
			return 0;
		}

	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			if ( !initialised_ )
			{
				result = fetch_from_slot( 0 );

				if ( result->get_stream( ) )
				{
					queue_ = stream_queue_ptr( new stream_queue( fetch_slot( 0 ) ) );
					ml::frame_type_ptr( new frame_avformat( queue_->fetch( 0 ), queue_ ) )->get_image( );
					ml::frame_type_ptr( new frame_avformat( queue_->fetch( 1 ), queue_ ) )->get_image( );
				}

				initialised_ = true;
			}

			if ( queue_ )
			{
				result = queue_->fetch( get_position( ) );
				result = ml::frame_type_ptr( new frame_avformat( result, queue_ ) );
			}
			else
			{
				result = fetch_from_slot( 0 );
			}
		}

	private:
		bool initialised_;
		stream_queue_ptr queue_;
};

filter_type_ptr ML_PLUGIN_DECLSPEC create_avdecode( const pl::wstring & )
{
	return filter_type_ptr( new avformat_decode_filter( ) );
}

} } }

