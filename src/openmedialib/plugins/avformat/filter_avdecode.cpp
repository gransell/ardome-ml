
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

#include "avformat_stream.hpp"

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

		stream_queue( ml::input_type_ptr input, int gop_open )
			: input_( input )
			, gop_open_( gop_open )
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

			if ( frames_.find( position ) == frames_.end( ) && position < input_->get_frames( ) )
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
			else if ( position < input_->get_frames( ) )
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

				ml::frame_type_ptr temp;

				if ( start < input_->get_frames( ) )
				{
					temp = ml::frame_type_ptr( new frame_type( fetch( start ++ ) ) );

					while( temp && temp->get_stream( ) && decode( temp, position ) )
					{
						if ( start < input_->get_frames( ) )
							temp = ml::frame_type_ptr( new frame_type( fetch( start ++ ) ) );
						else
							temp = ml::frame_type_ptr( );
					}
				}

				if ( temp )
				{
					return temp->get_image( );
				}
				else
				{
					int got = 0;
					if ( avcodec_decode_video( context_, frame_, &got, 0, 0 ) >= 0 )
					{
						il::image_type_ptr image;
						if ( got )
						{
							const PixelFormat fmt = context_->pix_fmt;
							const int width = context_->width;
							const int height = context_->height;
							image = convert_to_oil( frame_, fmt, width, height );
							image->set_position( input_->get_frames( ) - 1 );
							if ( frame_->interlaced_frame )
								image->set_field_order( frame_->top_field_first ? il::top_field_first : il::bottom_field_first );
							images_[ input_->get_frames( ) - 1 ] = image;
						}
						expected_ ++;
						return image;
					}
				}
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

				if ( gop_open_ && start == position && position != 0 )
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

							if ( frame_->interlaced_frame )
								image->set_field_order( frame_->top_field_first ? il::top_field_first : il::bottom_field_first );

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
		int gop_open_;

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
			, prop_gop_open_( pl::pcos::key::from_string( "gop_open" ) )
			, initialised_( false )
			, queue_( )
		{
			properties( ).append( prop_gop_open_ = 1 );
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
					queue_ = stream_queue_ptr( new stream_queue( fetch_slot( 0 ), prop_gop_open_.value< int >( ) ) );
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
		pl::pcos::property prop_gop_open_;
		bool initialised_;
		stream_queue_ptr queue_;
};

class avformat_encode_filter : public filter_type
{
	public:
		// Filter_type overloads
		avformat_encode_filter( )
			: ml::filter_type( )
			, prop_enable_( pl::pcos::key::from_string( "enable" ) )
			, prop_force_( pl::pcos::key::from_string( "force" ) )
			, codec_( "mpeg2" )
			, initialised_( false )
			, encoding_( false )
			, context_( 0 )
			, instance_( 0 )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_force_ = 0 );
		}

		virtual ~avformat_encode_filter( )
		{
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// This provides the name of the plugin (used in serialisation)
		virtual const pl::wstring get_uri( ) const { return L"avencode"; }

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
			if ( prop_enable_.value< int >( ) )
			{
				if ( !initialised_ )
				{
					result = fetch_from_slot( 0 );
	
					// Try to set up an internal graph to handle the cpu compositing of deferred frames
					pusher_ = ml::create_input( L"pusher:" );
					render_ = ml::create_filter( L"render" );
	
					if ( render_ )
						render_->connect( pusher_ );
	
					initialised_ = true;
				}
	
				if ( !last_frame_ || get_position( ) != last_frame_->get_position( ) )
				{
					ml::frame_type_ptr source = fetch_from_slot( 0 );
	
					if ( prop_force_.value< int >( ) )
					{
						encoding_ = true;
					}
					else if ( !source->get_stream( ) )
					{
						std::cerr << "case 0: " << get_position( ) << " no stream component" << std::endl;
						encoding_ = true;
					}
					else if ( source->is_deferred( ) )
					{
						std::cerr << "case 1: " << get_position( ) << " deferred frame must be rendered" << std::endl;
						encoding_ = true;
					}
					else if ( !last_frame_ && source->get_stream( )->key( ) != source->get_stream( )->position( ) )
					{
						std::cerr << "case 2: " << get_position( ) << " first frame not an i frame" << std::endl;
						encoding_ = true;
					}
					else if ( !encoding_ && source->get_stream( )->codec( ) != codec_ )
					{
						std::cerr << "case 3: " << get_position( ) << " stream of different type" << std::endl;
						encoding_ = true;
					}
					else if ( !encoding_ && !continuous( last_frame_, source ) )
					{
						std::cerr << "case 4: " << get_position( ) << " started encoding due to non-contiguous packets" << std::endl;
						encoding_ = true;
					}
					else if ( encoding_ && ( source->get_stream( )->codec( ) != codec_ || source->get_stream( )->key( ) != source->get_stream( )->position( ) ) )
					{
						std::cerr << "case 5: " << get_position( ) << " already encoding and stream component does not indicate that we have reached the next key frame yet" << std::endl;
					}
					else if ( encoding_ && source->get_stream( )->key( ) == source->get_stream( )->position( ) )
					{
						std::cerr << "case 6: " << get_position( ) << " encoding turned off as next key is reached" << std::endl;
						encoding_ = false;
						avcodec_flush_buffers( context_ );
					}
	
					if ( encoding_ )
					{
						result = render( source );
						result->set_position( source->get_position( ) );
						encode( result );
					}
					else
					{
						result = source;
					}
	
					last_frame_ = result;
				}
				else
				{
					result = last_frame_;
				}
			}
			else
			{
				result = fetch_from_slot( 0 );
			}
		}
	
		ml::frame_type_ptr render( const ml::frame_type_ptr &frame )
		{
			ml::frame_type_ptr result = frame;

			if ( render_ )
			{
				pusher_->push( frame );
				result = render_->fetch( );
			}
			else
			{
				result->get_image( );
			}

			return result;
		}

		bool continuous( const ml::frame_type_ptr &last, const ml::frame_type_ptr &current )
		{
			bool result = true;

			// Check for continuity
			// TODO: ensure that the source hasn't changed
			if ( last && last->get_stream( ) )
			{
				result = last->get_stream( )->position( ) == current->get_stream( )->position( ) - 1;
				if ( !result )
					result = current->get_stream( )->position( ) == current->get_stream( )->key( );
			}
			else
			{
				result = current->get_stream( )->position( ) == current->get_stream( )->key( );
			}

			return result;
		}

		void encode( const ml::frame_type_ptr &result )
		{
			if ( context_ == 0 )
			{
				instance_ = avcodec_find_encoder( CODEC_ID_MPEG2VIDEO );
				context_ = avcodec_alloc_context( );
				picture_ = avcodec_alloc_frame( );
				context_->bit_rate = 50000000;
				context_->width = result->width( );
				context_->height = result->height( );
				AVRational avr = { result->get_fps_den( ), result->get_fps_num( ) };
				context_->time_base= avr;
				context_->gop_size = 12;
				context_->max_b_frames = 0;
				context_->pix_fmt = PIX_FMT_YUV422P;
				context_->flags |= CODEC_FLAG_CLOSED_GOP | CODEC_FLAG_INTERLACED_ME;
				context_->scenechange_threshold = 1000000000;
				context_->thread_count = 4;

				if ( avcodec_open( context_, instance_ ) < 0 ) 
				{
					std::cerr << "Unable to open codec" << std::endl;
				}

				if ( context_->thread_count )
					avcodec_thread_init( context_, context_->thread_count );

				outbuf_size_ = 5000000;
				outbuf_ = ( boost::uint8_t * )malloc( outbuf_size_ );
			}

			picture_->data[ 0 ] = result->get_image( )->data( 0 );
			picture_->data[ 1 ] = result->get_image( )->data( 1 );
			picture_->data[ 2 ] = result->get_image( )->data( 2 );
			picture_->linesize[ 0 ] = result->get_image( )->pitch( 0 );
			picture_->linesize[ 1 ] = result->get_image( )->pitch( 1 );
			picture_->linesize[ 2 ] = result->get_image( )->pitch( 2 );

			if ( result->get_image( )->field_order( ) != il::progressive )
			{
				picture_->interlaced_frame  = 1;
				picture_->top_field_first = result->get_image( )->field_order( ) != il::top_field_first;
			}

			int out_size = avcodec_encode_video( context_, outbuf_, outbuf_size_, picture_ );

			ml::stream_type_ptr packet = ml::stream_type_ptr( new stream_avformat( CODEC_ID_MPEG2VIDEO, out_size, get_position( ), 0, 0, ml::dimensions( result->width( ), result->height( ) ), ml::fraction( result->get_sar_num( ), result->get_sar_den( ) ) ) );
			memcpy( packet->bytes( ), outbuf_, out_size );
			result->set_stream( packet );
		}

	private:
		pl::pcos::property prop_enable_;
		pl::pcos::property prop_force_;
		std::string codec_;
		bool initialised_;
		ml::frame_type_ptr last_frame_;
		ml::input_type_ptr pusher_;
		ml::filter_type_ptr render_;
		bool encoding_;
		AVCodecContext *context_;
		AVCodec *instance_;
		AVFrame *picture_;
		int outbuf_size_;
		boost::uint8_t *outbuf_;
};

filter_type_ptr ML_PLUGIN_DECLSPEC create_avdecode( const pl::wstring & )
{
	return filter_type_ptr( new avformat_decode_filter( ) );
}

filter_type_ptr ML_PLUGIN_DECLSPEC create_avencode( const pl::wstring & )
{
	return filter_type_ptr( new avformat_encode_filter( ) );
}

} } }

