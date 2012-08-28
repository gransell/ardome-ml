// Stream packet decoder and encoder filters
//
// Copyright (C) 2009 Ardendo
// Released under the terms of the LGPL.
//
// #filter:avdecode
//
// Provides stream/packet based decoding. This is typically used for non-avformat:
// input implementations to provide a means to decode the packets retrieved.
// 
// #filter:avencode
// 
// Provides stream packet encoding.

#include <sstream>
#include <algorithm>

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/filter_simple.hpp>
#include <openmedialib/ml/stream.hpp>
#include <openmedialib/ml/audio_block.hpp>
#include <opencorelib/cl/profile.hpp>
#include <openpluginlib/pl/pcos/isubject.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>
#include <openmedialib/ml/scope_handler.hpp>

#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/log_defines.hpp>

#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include "utils.hpp"

#include "avformat_wrappers.hpp"
#include "avformat_stream.hpp"

namespace cl = olib::opencorelib;
namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;

namespace olib { namespace openmedialib { namespace ml {

extern const std::wstring avformat_to_oil( int );
extern const PixelFormat oil_to_avformat( const std::wstring & );
extern il::image_type_ptr convert_to_oil( AVFrame *, PixelFormat, int, int );

static const pl::pcos::key key_gop_closed_ = pl::pcos::key::from_string( "gop_closed" );
static const pl::pcos::key key_fixed_sar_ = pl::pcos::key::from_string( "fixed_sar" );

static bool is_dv( const std::string &codec )
{
	return boost::algorithm::ends_with( codec,  "dv" ) || codec == "dv25" || codec == "dv50" || codec == "dvcprohd_1080i";
}

static bool is_mpeg2( const std::string &codec )
{
	return boost::algorithm::ends_with( codec, "mpeg2" ) || codec == "mpeg2/mpeg2hd_1080i";
}

static bool is_imx( const std::string &codec )
{
	return boost::algorithm::ends_with( codec, "imx" );
}
	
void create_video_codec( const stream_type_ptr &stream, AVCodecContext **context, AVCodec **codec, bool i_frame_only, int threads )
{
	ARLOG_DEBUG5( "Creating decoder context" );
	*codec = avcodec_find_decoder( stream_to_avformat_codec_id( stream ) );
	ARENFORCE_MSG( codec, "Could not find decoder for format %1% (used %2% as a key for lookup")( stream_to_avformat_codec_id( stream ) )( stream->codec( ) );
	
	*context = avcodec_alloc_context3( *codec );
	ARENFORCE_MSG( *context, "Failed to allocate codec context" );
	
	// Work around for broken Omneon IMX streams which incorrectly assign the low delay flag in their encoder
	if ( i_frame_only )
		(*context)->flags |= CODEC_FLAG_LOW_DELAY;
	if ( threads )
		(*context)->thread_count = threads;
	
	avcodec_open2( *context, *codec, 0 );
	ARLOG_DEBUG5( "Creating new avcodec decoder context" );
}

void create_audio_codec( const stream_type_ptr &stream, AVCodecContext **context, AVCodec **codec, int sample_rate, int channels )
{
	ARLOG_DEBUG5( "Creating decoder context" );
	*codec = avcodec_find_decoder( stream_to_avformat_codec_id( stream ) );
	ARENFORCE_MSG( *codec, "Could not find decoder for format %1% (used %2% as a key for lookup")( stream_to_avformat_codec_id( stream ) )( stream->codec( ) );
	
	*context = avcodec_alloc_context3( *codec );
	ARENFORCE_MSG( *context, "Failed to allocate codec context" );
	
	// We need to set these for avformat to know how to attach the codec
	(*context)->sample_rate = sample_rate;
	(*context)->channels = channels;
	avcodec_open2( *context, *codec, 0 );
	
	ARENFORCE_MSG( (*context)->codec, "Could not open code for format %1% mapped to avcodec id %2%" )( stream->codec( ) )( (*codec)->id );
}
	
ml::audio_type_ptr av_sample_fmt_to_audio( AVSampleFormat sample_fmt, const int freq, const int channels, const int samples )
{
	switch ( sample_fmt ) {
		case AV_SAMPLE_FMT_S32:
			return audio::pcm32_ptr( new audio::pcm32(freq, channels, samples ) );
		case AV_SAMPLE_FMT_S16:
			return audio::pcm16_ptr( new audio::pcm16( freq, channels, samples ) );
		default:
			ARENFORCE_MSG( false, "Unsupported sample format" )( av_get_sample_fmt_name( sample_fmt ) );
	}
	
	return ml::audio_type_ptr();
}

	
class stream_queue
{
	public:
		stream_queue( ml::input_type_ptr input, int gop_open, const std::wstring& scope, const std::wstring& source_uri, int threads )
			: input_( input )
			, gop_open_( gop_open )
			, scope_( scope )
			, scope_uri_key_( source_uri )
			, threads_( threads )
			, keys_( 0 )
			, context_( 0 )
			, codec_( 0 )
			, expected_( 0 )
			, frame_( avcodec_alloc_frame( ) )
			, offset_( 0 )
			, lru_cache_( )
		{
			audio_buf_size_ = (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2;
			audio_buf_ = ( uint8_t * )av_malloc( audio_buf_size_ );

			lru_cache_ = ml::the_scope_handler::Instance().lru_cache( scope_ );
		}

		virtual ~stream_queue( )
		{
			if ( context_ )
			{
				ARLOG_DEBUG5( "Destroying decoder context" );
				avcodec_close( context_ );
				av_free( context_ );
			}
			av_free( frame_ );
            av_free( audio_buf_ );
		}

		ml::frame_type_ptr fetch( int position )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			ml::frame_type_ptr result;
			
			lru_cache_type::key_type my_key = lru_key_for_position( position );

			if ( position < 0 ) position = 0;

			if( !result && position < input_->get_frames( ) )
			{
				input_->seek( position );
				result = input_->fetch( );
				while ( result && result->get_stream( ) )
				{
					if ( result->get_stream( )->position( ) == result->get_stream( )->key( ) )
						look_for_closed( result );
						
					if ( result->get_position( ) == position )
						break;
					input_->seek( result->get_position( ) + 1 );
					result = input_->fetch( );
				}
			}

			return result;
		}

		il::image_type_ptr decode_image( int position )
		{
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			lru_cache_type::key_type my_key = lru_key_for_position( position );
						
			il::image_type_ptr img = lru_cache_->image_for_position( my_key );
			
			if( img )
			{
				return img;
			}
		
			ml::frame_type_ptr frame = fetch( position );

			if ( frame && frame->get_stream( ) )
			{
				int start = expected_;

				if ( frame->get_stream( )->estimated_gop_size( ) != 1 )
				{
					if ( position + offset_ != expected_ )
					{
						if ( position + offset_ < expected_ || position + offset_ > expected_ + 12 )
						{
							start = frame->get_stream( )->key( );

							if ( position < start + 3 && position != 0 )
								start = fetch( start - 1 )->get_stream( )->key( );
						}
					}
				}
				else
				{
					start = position;
				}

				ml::frame_type_ptr temp;

				if ( start < input_->get_frames( ) )
				{
					temp = fetch( start ++ )->shallow( );

					while( temp && temp->get_stream( ) && decode( temp, position ) )
					{
						if ( start < input_->get_frames( ) ) {
							temp = fetch( start ++ )->shallow( );
						} else {
							temp = ml::frame_type_ptr( );
						}
					}
				}

				/* The temp->get_stream( ) is needed for transcode, without that it will get one frame 
				 * short on browse. See Jira AMF-840 */
				if ( temp && temp->get_stream( ) )
				{
					return temp->get_image( );
				}
				else if ( frame->get_stream( )->codec( ).find( "dv" ) != 0 )
				{
					int got = 0;
					AVPacket pkt;
					pkt.data = 0;
					pkt.size = 0;
					if ( avcodec_decode_video2( context_, frame_, &got, &pkt ) >= 0 )
					{
						il::image_type_ptr image;
						if ( got )
						{
							const PixelFormat fmt = context_->pix_fmt;
							const int width = context_->width;
							const int height = context_->height;
							image = convert_to_oil( frame_, fmt, width, height );
							image->set_position( input_->get_frames( ) - 1 );

							lru_cache_->insert_image_for_position( lru_key_for_position( image->position( ) ), image );
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
			boost::recursive_mutex::scoped_lock lock( mutex_ );
			lru_cache_type::key_type my_key = lru_key_for_position( position );
			
			audio_type_ptr aud = lru_cache_->audio_for_position( my_key );
			
			if( aud )
				return aud;
			
			ml::frame_type_ptr frame = fetch( position );

			if ( frame && frame->get_stream( ) )
			{
				int start = expected_;

				if ( position + offset_ != expected_ )
					start = frame->get_stream( )->key( );

				if ( gop_open_ && start == position && position != 0 )
					start = fetch( start - 1 )->get_stream( )->key( );

				ml::frame_type_ptr temp = fetch( start ++ )->shallow( );

				while( temp && temp->get_stream( ) && decode( temp, position ) )
					temp = fetch( start ++ )->shallow( );

				return temp->get_audio( );
			}

			return ml::audio_type_ptr( );
		}

		ml::fraction sar( )
		{
			ml::fraction result( 0, 1 );
			if ( context_ )
			{
				result.num = context_->sample_aspect_ratio.num;
				result.den = context_->sample_aspect_ratio.den;

				result = extended_res_sar_workaround( result, context_->width, context_->height );
			}
			return result;
		}

	private:

		//Workaround for media with extended resolution lines
		ml::fraction extended_res_sar_workaround( const ml::fraction& org_sar, boost::int32_t width, boost::int32_t height )
		{
			boost::rational<boost::int32_t> result( org_sar.num, org_sar.den );
			
			if( result.numerator() && result.denominator() && width == 720 && ( height == 608 || height == 512 ) )
			{
				const boost::rational<boost::int32_t> aspect4_3( 4, 3 );
				const boost::rational<boost::int32_t> aspect16_9( 16, 9 );

				const boost::rational<boost::int32_t> dim( width, height );

				if( result * dim == aspect4_3 )
				{
					if( height == 608 ) //PAL
					{
						result.assign( 59, 54 );
					}
					else if( height == 512 ) //NTSC
					{
						result.assign( 10, 11 );
					}
				}
				else if( result * dim == aspect16_9 )
				{
					if( height == 608 ) //PAL
					{
						result.assign( 118, 81 );
					}
					else if( height == 512 ) //NTSC
					{
						result.assign( 40, 33 );
					}
				}
			}

			return ml::fraction( result.numerator(), result.denominator() );
		}

		boost::uint8_t *find_mpeg2_gop( const ml::stream_type_ptr &stream )
		{
			boost::uint8_t *result = 0;
			boost::uint8_t *ptr = stream->bytes( );
			unsigned int index = 0;
			unsigned int length = stream->length( );
			while ( result == 0 && index + 10 < length )
			{
				if ( ptr[ index ] == 0 && ptr[ index + 1 ] == 0 && ptr[ index + 2 ] == 1 )
				{
					if ( ptr[ index + 3 ] == 0xb8 )
					{
						if ( ptr[ index + 8 ] == 0 && ptr[ index + 9 ] == 0 && ptr[ index + 10 ] == 1 )
							result = ptr + index;
						else
							index += 4;
					}
					else if ( ptr[ index + 3 ] == 0x00 )
						index += 3;
					else
						index += 4;
				}
				else
				{
					index ++;
				}
				if ( !result && ptr[ index ] != 0x00 )
				{
					boost::uint8_t *t = ( boost::uint8_t * )memchr( ptr + index, 0x00, length - index );
					if ( t == 0 ) break;
					index = t - ptr;
				}
			}
	
			return result;
		}

		void look_for_closed( const ml::frame_type_ptr &frame )
		{
			pl::pcos::property gop_closed( key_gop_closed_ );
			if ( is_dv( frame->get_stream( )->codec( ) ) || is_imx( frame->get_stream( )->codec( ) ) )
			{
				frame->get_stream( )->properties( ).append( gop_closed = 1 );
			}
			else if ( is_mpeg2( frame->get_stream( )->codec( ) ) )
			{
				boost::uint8_t *ptr = find_mpeg2_gop( frame->get_stream( ) );
				if ( ptr ) frame->get_stream( )->properties( ).append( gop_closed = int( ( ptr[ 7 ] & 64 ) >> 6 ) );
			}
			else
			{
				frame->get_stream( )->properties( ).append( gop_closed = -1 );
			}
		}

		bool decode( ml::frame_type_ptr &result, int position )
		{
			bool found = false;

			if ( context_ == 0 )
			{
				create_video_codec( result->get_stream(), &context_, &codec_, result->get_stream()->estimated_gop_size() == 1, threads_ );
				avcodec_flush_buffers( context_);
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
			AVPacket avpkt;

			switch( pkt->id( ) )
			{
				case ml::stream_video:
					ARLOG_DEBUG5( "Decoding image %d" )( position );
					avpkt.data = pkt->bytes( );
					avpkt.size = pkt->length( );
					if ( avcodec_decode_video2( context_, frame_, &got, &avpkt ) >= 0 )
					{
						if ( got )
						{
							il::image_type_ptr image;
							const PixelFormat fmt = context_->pix_fmt;
							const int width = context_->width;
							const int height = context_->height;
							image = convert_to_oil( frame_, fmt, width, height );
							image->set_position( pkt->position( ) );

							pl::pcos::property fixed_sar_prop = pkt->properties().get_property_with_key( key_fixed_sar_ );
							if( fixed_sar_prop.valid() && fixed_sar_prop.value<int>() )
							{
								//Use the sample aspect ratio from the frame, 
								//not the one from the video essence.
								ml::fraction stream_sar = pkt->sar();
								image->set_sar_num( stream_sar.num );
								image->set_sar_den( stream_sar.den );
							}
							else
							{
								//Check the codec context for the sample aspect
								//ratio of the video essence.
								ml::fraction img_sar = sar();
								if( img_sar.num != 0)
								{
									image->set_sar_num( img_sar.num );
									image->set_sar_den( img_sar.den );
								}
							}

							//We set decoded to true, since the image is a decoded version
							//of the stream in the frame, and we don't want to remove the stream.
							result->set_image( image, true );

							if ( position == 0 )
							{
								//std::cerr << "found first " << result->get_position( ) << " " << expected_ << std::endl;
								offset_ = result->get_position( );
							}

							image->set_position( result->get_position( ) - offset_ );
							lru_cache_->insert_image_for_position( lru_key_for_position( image->position( ) ), image );

							if ( result->get_position( ) == position + offset_ )
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
					{

						avpkt.data = pkt->bytes( );
						avpkt.size = pkt->length( );
						
						avcodec_get_frame_defaults( frame_ );
						
						int got_frame = 0;
						int length = avcodec_decode_audio4( context_, frame_, &got_frame, &avpkt );
						
						ARENFORCE_MSG( length >= 0 , "Error decoding audio. Error = %2%" )( length );

						if ( got_frame )
						{
							int channels = context_->channels;
							int frequency = context_->sample_rate;
							int samples = frame_->nb_samples;

							audio_type_ptr audio = av_sample_fmt_to_audio( context_->sample_fmt, frequency, channels, samples );
							memcpy( audio->pointer( ), frame_->data[ 0 ], audio->size( ) );
							audio->set_position( pkt->position( ) );
							result->set_audio( audio );

							if ( result->get_position( ) >= position + offset_ )
								found = true;

							expected_ ++;
						}
					}
					break;

				default:
					break;
			}

			return !found;
		}
	
	lru_cache_type::key_type lru_key_for_position( boost::int32_t pos )
	{
		lru_cache_type::key_type my_key( pos, input_->get_uri() );
		if( !scope_uri_key_.empty( ) )
			my_key.second = scope_uri_key_;
		
		return my_key;
	}

	private:
		boost::recursive_mutex mutex_;
		ml::input_type_ptr input_;
		int gop_open_;
		std::wstring scope_;
		std::wstring scope_uri_key_;
		int threads_;

		int keys_;
		AVCodecContext *context_;
		AVCodec *codec_;
		int expected_;
		AVFrame *frame_;
		int audio_buf_size_;
		boost::uint8_t *audio_buf_;
		int offset_;
		lru_cache_type_ptr lru_cache_;
};

typedef boost::shared_ptr< stream_queue > stream_queue_ptr;

class ML_PLUGIN_DECLSPEC frame_avformat : public ml::frame_type 
{
	public:
		/// Constructor
		frame_avformat( const frame_type_ptr &other, const stream_queue_ptr &queue )
			: ml::frame_type( other.get( ) )
			, queue_( queue )
			, original_position_( other->get_position( ) )
		{
		}

		frame_avformat( const frame_avformat *other )
			: ml::frame_type( other )
			, queue_( other->queue_ )
			, original_position_( other->original_position_ )
		{
		}

		/// Destructor
		virtual ~frame_avformat( )
		{
		}

		/// Provide a shallow copy of the frame (and all attached frames)
		virtual frame_type_ptr shallow( ) const
		{
			return frame_type_ptr( new frame_avformat( this ) );
		}

		/// Provide a deepy copy of the frame (and a shallow copy of all attached frames)
		virtual frame_type_ptr deep( )
		{
			return frame_type_ptr( new frame_avformat( frame_type::deep( ), queue_ ) );
		}

		/// Indicates if the frame has an image
		virtual bool has_image( )
		{
			return image_ || ( stream_ && stream_->id( ) == ml::stream_video );
		}

		/// Indicates if the frame has audio
		virtual bool has_audio( )
		{
			return audio_ || ( stream_ && stream_->id( ) == ml::stream_audio );
		}

		/// Get the image associated to the frame.
		virtual olib::openimagelib::il::image_type_ptr get_image( )
		{
			if ( !image_ && ( stream_ && stream_->id( ) == ml::stream_video ) )
			{
				il::image_type_ptr img = queue_->decode_image( original_position_ );
				if ( img )
				{
					img->set_sar_num( sar_num_ );
					img->set_sar_den( sar_den_ );
					set_image( img, true );
				}
			}
			return image_;
		}

		/// Set the audio associated to the frame.
		virtual void set_audio( audio_type_ptr audio )
		{
		}

		/// Get the audio associated to the frame.
		virtual audio_type_ptr get_audio( )
		{
			if ( !audio_ && ( stream_ && stream_->id( ) == ml::stream_audio ) )
				audio_ = queue_->decode_audio( original_position_ );
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
		int original_position_;
};
	
	
class avformat_audio_decoder
{
public:
	avformat_audio_decoder( const std::vector< size_t >& tracks_to_decode )
		: tracks_to_decode_( tracks_to_decode )
		, next_packets_to_decoders_( )
		, decoded_frame_( 0 )
		, reseats_( )
		, expected_( -1 )
	{
		ARENFORCE_MSG( tracks_to_decode_.size( ), "List of channels to decode can not be empty" );
	}
	
	virtual ~avformat_audio_decoder()
	{
		tear_down_codecs( );
	}
	
	void decode( const frame_type_ptr& result )
	{
		if( !result->audio_block( ) ) return;
		
		if( audio_contexts_.empty( ) )
			initialize( result );
		
		
		audio::block_type_ptr audio_block = result->audio_block( );
		
		std::vector< audio_type_ptr > result_audios( tracks_to_decode_.size( ) );
		
		// Iterate through the tracks that we have been told to decode and get the audio out
		for( std::vector< size_t >::const_iterator tr_it = tracks_to_decode_.begin(); tr_it != tracks_to_decode_.end(); ++tr_it )
		{
			int discard = 0;
			if ( result->get_position( ) != expected_ )
			{
				// Reset the next expected packet pos to the first packet in the first track
				next_packets_to_decoders_[ *tr_it ] = audio_block->tracks[ *tr_it ].packets.begin()->first;
				reseats_[ *tr_it ]->clear();
				discard = audio_block->tracks[ *tr_it ].discard;
			}

			result_audios[ tr_it - tracks_to_decode_.begin() ] = decode_track( audio_block->tracks[ *tr_it ].packets, *tr_it,
																			   audio_block->samples, result->get_position( ),
																			   discard );
		}
		
		audio_type_ptr combined = audio::combine( result_audios );
		
		result->set_audio( combined );

		expected_ = result->get_position( ) + 1;
	}
	
private:
	
	void tear_down_codecs( )
	{
		std::map< size_t, AVCodecContext * >::const_iterator it;
		for( it = audio_contexts_.begin(); it != audio_contexts_.end(); ++it )
		{
			AVCodecContext *ctx = it->second;
			avcodec_close( ctx );
			av_free( ctx );
		}
		
		if( decoded_frame_ != 0 )
			av_free( decoded_frame_ );

		audio_contexts_.clear();
		audio_codecs_.clear();
		next_packets_to_decoders_.clear();
		reseats_.clear();
	}
	
	void initialize( const frame_type_ptr& frame )
	{
		// Make sure that we have enough tracks in our audio block to match the amount in the tracks we are to decode
		ARENFORCE_MSG( frame->audio_block()->tracks.size( ) >= tracks_to_decode_.back( ),
					  "Not enough audio tracks in audio block" )( frame->audio_block()->tracks.size( ) )( tracks_to_decode_.back( ) );
		// Get the codec id of the first track that we are to decode
		ARENFORCE_MSG( !frame->audio_block()->tracks[ tracks_to_decode_[ 0 ] ].packets.empty(),
					  "Audio track %1% not available in audio block. Can not initialize." )( tracks_to_decode_[ 0 ] );
		
		for( int i = 0; i < tracks_to_decode_.size( ); ++i )
		{
			ml::stream_type_ptr strm = frame->audio_block()->tracks[ tracks_to_decode_[ i ] ].packets.begin( )->second;
			ARENFORCE_MSG( strm, "No stream available for first requested track" )( tracks_to_decode_[ i ] );

			create_audio_codec( strm, &audio_contexts_[ tracks_to_decode_[ i ] ], 
				&audio_codecs_[ tracks_to_decode_[ i ] ], strm->frequency(), strm->channels() );
			
			reseats_[ tracks_to_decode_[ i ] ] = audio::create_reseat( );
		}
		
	}

	audio_type_ptr decode_track( const audio::track_type::map& track_packets, const int track,
								 const int wanted_samples, const int position, const int discard )
	{
		AVCodecContext *track_context = audio_contexts_[ track ];
		
		AVPacket avpkt;
		av_init_packet( &avpkt );
		
		// Discard will be set to 0 after a seek
		int left_to_discard = discard;

		audio::reseat_ptr track_reseater = reseats_[ track ];
		audio::track_type::const_iterator packets_it =
			track_packets.find( next_packets_to_decoders_[ track ] );
		
		ARENFORCE_MSG( track_reseater->has( wanted_samples ) || packets_it != track_packets.end(),
					   "Next wanted packet %1% not found in audio_block" )( next_packets_to_decoders_[ track ] );
		
		for ( ;!track_reseater->has( wanted_samples ) && packets_it != track_packets.end( ); ++packets_it )
		{
			stream_type_ptr strm = packets_it->second;
			ARENFORCE_MSG( strm, "No stream available on packet %1%" )( packets_it->first );
	
			if( decoded_frame_ == 0 ) {
				ARENFORCE_MSG( decoded_frame_ = avcodec_alloc_frame( ) , "Failed to allocate AVFrame for decoding. Out of memory?" );
			}
			else
				avcodec_get_frame_defaults( decoded_frame_ );
			
			int got_frame = 0;
			int length = avcodec_decode_audio4( track_context, decoded_frame_, &got_frame, &avpkt );
			
			ARENFORCE_MSG( length >= 0, "Error while decoding audio for track %1%. Error = %2%" )( track )( length );
			
			if( got_frame )
			{				
				int channels = track_context->channels;
				int frequency = track_context->sample_rate;
				int samples = decoded_frame_->nb_samples;
				int buffer_offset = 0;
				
				ARLOG_DEBUG7( "Managed to decode %1% bytes of data from packet %2% on track %3%. Channels = %4%, frequency = %5%, samples = %6%" )
				(length)( strm->position() )( track )( channels )( frequency )( samples );
				
				if( left_to_discard )
				{
					if( samples <= left_to_discard )
					{
						left_to_discard -= samples;
						continue;
					}
					
					samples -= left_to_discard;
					buffer_offset = left_to_discard * channels * av_get_bytes_per_sample( track_context->sample_fmt );
					left_to_discard = 0;
				}
				
				ml::audio_type_ptr audio = av_sample_fmt_to_audio( track_context->sample_fmt, track_context->sample_rate, track_context->channels, samples );
				memcpy( audio->pointer( ), decoded_frame_->data[ 0 ], audio->size( ) );
				audio->set_position( position );
				
				track_reseater->append( audio );
			}
			next_packets_to_decoders_[ track ] += packets_it->second->samples();
		}
		
		ARENFORCE_MSG( track_reseater->has( wanted_samples ), "Did not manage to get samples out of audio block." )
			( wanted_samples )( discard )( position );
		
		return track_reseater->retrieve( wanted_samples );
	}

	//The stream indexes of the tracks that we're interested in decoding
	const std::vector< size_t > tracks_to_decode_;
	
	//Maps from stream index to context/codec for that stream
	std::map< size_t, AVCodecContext * > audio_contexts_;
	std::map< size_t, AVCodec * > audio_codecs_;
	AVFrame *decoded_frame_;
	
	// Kep track of what packet to feed the decoder next. We need one for eack track
	std::map< size_t, int > next_packets_to_decoders_;
	std::map< size_t, audio::reseat_ptr > reseats_;
	
	int expected_;
};
	
typedef boost::shared_ptr< avformat_audio_decoder > avformat_audio_decoder_ptr;

class avformat_decode_filter : public filter_simple
{
	public:
		// Filter_type overloads
		avformat_decode_filter( )
			: ml::filter_simple( )
			, prop_gop_open_( pl::pcos::key::from_string( "gop_open" ) )
			, prop_scope_( pl::pcos::key::from_string( "scope" ) )
			, prop_source_uri_( pl::pcos::key::from_string( "source_uri" ) )
			, prop_threads_( pl::pcos::key::from_string( "threads" ) )
			, prop_decode_video_( pl::pcos::key::from_string( "decode_video" ) )
			, initialised_( false )
			, queue_( )
			, audio_queue_( )
		{
			properties( ).append( prop_gop_open_ = 1 );
			properties( ).append( prop_scope_ = pl::wstring(L"default_scope") );
			properties( ).append( prop_source_uri_ = pl::wstring(L"") );
			properties( ).append( prop_threads_ = 1 );
			properties( ).append( prop_decode_video_ = 1 );
		}

		virtual ~avformat_decode_filter( )
		{
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// This provides the name of the plugin (used in serialisation)
		virtual const pl::wstring get_uri( ) const { return L"avdecode"; }

	protected:
		
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			if ( !initialised_ )
			{
				result = fetch_from_slot( 0 );

				if( prop_decode_video_.value< int >() )
				{
					if ( result && result->get_stream( ) && prop_decode_video_.value< int >( ) )
					{
						// If the source_uri has not been set for this decode filter then try to use the uri for the first input
						pl::wstring uri = fetch_slot( 0 )->get_uri( );
						if( !prop_source_uri_.value< pl::wstring >( ).empty( ) )
						{
							uri = prop_source_uri_.value< pl::wstring >( );
						}
						
						queue_ = stream_queue_ptr( new stream_queue( fetch_slot( 0 ), prop_gop_open_.value< int >( ), prop_scope_.value< pl::wstring >( ), uri, prop_threads_.value< int >( ) ) );
					}
				}
				else
				{
					const audio::block_type_ptr &block = result->audio_block();
					ARENFORCE( block );
					audio::block_type::const_iterator it;
					std::vector< size_t > tracks;
					for( it = block->tracks.begin(); it != block->tracks.end(); ++it )
						tracks.push_back( it->first );
					
					audio_queue_ = avformat_audio_decoder_ptr( new avformat_audio_decoder( tracks ) );
				}

				initialised_ = true;
			}

			if ( queue_ )
			{
				// If we were initializing then we have already fetched something for this position so dont do it again.
				if( !result || result->get_stream( )->position( ) != get_position( ) )
					result = queue_->fetch( get_position( ) );
				if ( result )
					result = ml::frame_type_ptr( new frame_avformat( result, queue_ ) );
			}
			else if( audio_queue_ )
			{
				result = fetch_from_slot( 0 );
				audio_queue_->decode( result );
			}
			else
			{
				result = fetch_from_slot( 0 );
			}

			// Make sure all frames are shallow copied here
			if ( result )
				result = result->shallow( );
		}

	private:
		pl::pcos::property prop_gop_open_;
		pl::pcos::property prop_scope_;
		pl::pcos::property prop_source_uri_;
		pl::pcos::property prop_threads_;
		pl::pcos::property prop_decode_video_;
		bool initialised_;
		stream_queue_ptr queue_;
		avformat_audio_decoder_ptr audio_queue_;
};

class avformat_video_streamer : public ml::stream_type
{
	public:
		avformat_video_streamer( avformat_video *wrapper )
			: ml::stream_type( )
			, wrapper_( wrapper )
			, pending_( 0 )
		{
		}

		virtual ~avformat_video_streamer( )
		{
			delete wrapper_;
		}

		void push( ml::stream_type_ptr stream )
		{
			if ( pending_ )
			{
				while( pending_ )
				{
					push( ml::frame_type_ptr( ) );
					pending_ --;
				}
				avcodec_flush_buffers( wrapper_->context( ) );
			}

			if ( stream )
				queue_.push_back( stream );
		}

		void push( ml::frame_type_ptr frame )
		{
			ml::stream_type_ptr stream = wrapper_->encode( frame );
			ARENFORCE_MSG( stream, "Encoding failed" );

			//Protect against logical errors
			ARENFORCE( stream.get() != this );

			queue_.push_back( stream );
			if ( frame && frame->has_image( ) && stream->length( ) == 0 )
				pending_ ++;
		}

		bool more( )
		{
			return !queue_.empty( ) || pending_ > 0;
		}

		bool next( )
		{
			if ( !queue_.empty( ) )
			{
				stream_ = queue_[ 0 ];
				queue_.pop_front( );
			}
			else if ( pending_ )
			{
				push( ml::stream_type_ptr( ) );
				stream_ = queue_[ 0 ];
				queue_.pop_front( );
			}
			else
			{
				stream_ = ml::stream_type_ptr( );
			}
			return stream_ != ml::stream_type_ptr( );
		}

		/// Return the properties associated to this frame.
		virtual olib::openpluginlib::pcos::property_container &properties( )
		{
			return stream_ ? stream_->properties( ) : dummy_props_;
		}

		/// Indicates the container format of the input
		virtual const std::string &container( ) const
		{
			return stream_ ? stream_->container( ) : dummy_;
		}

		/// Indicates the codec used to decode the stream
		virtual const std::string &codec( ) const
		{
			return stream_ ? stream_->codec( ) : dummy_;
		}

		/// Returns the id of the stream (so that we can select a codec to decode it)
		virtual const enum ml::stream_id id( ) const
		{
			return stream_ ? stream_->id( ) : ml::stream_unknown;
		}

		/// Returns the length of the data in the stream
		virtual size_t length( )
		{
			return stream_ ? stream_->length( ) : 0;
		}

		/// Returns a pointer to the first byte of the stream
		virtual boost::uint8_t *bytes( )
		{
			return stream_ ? stream_->bytes( ) : 0;
		}

		/// Returns the position of the key frame associated to this packet
		virtual const boost::int64_t key( ) const
		{
			return stream_ ? stream_->key( ) : 0;
		}

		/// Returns the position of this packet
		virtual const boost::int64_t position( ) const
		{
			return stream_ ? stream_->position( ) : 0;
		}

		/// Returns the bitrate of the stream this packet belongs to
		virtual const int bitrate( ) const
		{
			return stream_ ? stream_->bitrate( ) : 0;
		}
		
		/// We dont know the gop size atm
		virtual const int estimated_gop_size( ) const
		{
			return stream_ ? stream_->estimated_gop_size( ) : 0;
		}

		/// Returns the dimensions of the image associated to this packet (0,0 if n/a)
		virtual const dimensions size( ) const 
		{
			return stream_ ? stream_->size( ) : dimensions( 0, 0 );
		}

		/// Returns the sar of the image associated to this packet (1,1 if n/a)
		virtual const fraction sar( ) const 
		{ 
			return stream_ ? stream_->sar( ) : fraction( 1, 1 );
		}

		/// Returns the frequency associated to the audio in the packet (0 if n/a)
		virtual const int frequency( ) const 
		{ 
			return stream_ ? stream_->frequency( ) : 0;
		}

		/// Returns the channels associated to the audio in the packet (0 if n/a)
		virtual const int channels( ) const 
		{ 
			return stream_ ? stream_->channels( ) : 0;
		}

		/// Returns the samples associated to the audio in the packet (0 if n/a)
		virtual const int samples( ) const 
		{ 
			return stream_ ? stream_->samples( ) : 0;
		}
	
		virtual const olib::openpluginlib::wstring pf( ) const 
		{ 
			return stream_ ? stream_->pf( ) : pl::wstring( L"" );
		}

		virtual olib::openimagelib::il::field_order_flags field_order( ) const 
		{ 
			return stream_ ? stream_->field_order( ) : olib::openimagelib::il::top_field_first;
		}

	private:
		avformat_video *wrapper_;
		int pending_;
		std::deque< ml::stream_type_ptr > queue_;
		ml::stream_type_ptr stream_;
		std::string dummy_;
		pl::pcos::property_container dummy_props_;
};

typedef boost::shared_ptr< avformat_video_streamer > avformat_video_streamer_ptr;

class avformat_encode_filter : public filter_simple
{
	public:
		// Filter_type overloads
		avformat_encode_filter( )
			: ml::filter_simple( )
			, prop_enable_( pl::pcos::key::from_string( "enable" ) )
			, prop_force_( pl::pcos::key::from_string( "force" ) )
			, prop_profile_( pl::pcos::key::from_string( "profile" ) )
			, prop_threads_( pl::pcos::key::from_string( "threads" ) )
			, initialised_( false )
			, encoding_( false )
			, video_wrapper_( 0 )
			, pf_( L"" )
		{
			properties( ).append( prop_enable_ = 1 );
			properties( ).append( prop_force_ = 0 );
			properties( ).append( prop_profile_ = pl::wstring( L"profiles/vcodec/dv25" ) );
			properties( ).append( prop_threads_ = 4 );
		}

		virtual ~avformat_encode_filter( )
		{
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return false; }

		// This provides the name of the plugin (used in serialisation)
		virtual const pl::wstring get_uri( ) const { return L"avencode"; }

	protected:
		// The main access point to the filter
		void do_fetch( ml::frame_type_ptr &result )
		{
			if ( prop_enable_.value< int >( ) )
			{
				ml::frame_type_ptr source = fetch_from_slot( 0 );
				ARENFORCE( fetch_slot(0)->get_position() == get_position() );

				if ( !initialised_ )
				{
					result = source;
	
					// Try to set up an internal graph to handle the cpu compositing of deferred frames
					pusher_ = ml::create_input( L"pusher:" );
					render_ = ml::create_filter( L"render" );
	
					if ( render_ )
						render_->connect( pusher_ );
	
					// Load the profile
					video_wrapper_ = new avformat_video( result );
					video_streamer_ = avformat_video_streamer_ptr( new avformat_video_streamer( video_wrapper_ ) );
					manager_.enroll( video_wrapper_ );
					manager_.load( pl::to_string( prop_profile_.value< pl::wstring >( ) ) );

					// State is initialised now
					initialised_ = true;
					pf_ = result->pf( );
				}
	
				if ( !last_frame_ || get_position( ) != last_frame_->get_position( ) )
				{
					int gop_closed = 0;
					bool stream_types_match = true;

					if ( source->get_stream( ) && source->get_stream( )->properties( ).get_property_with_key( key_gop_closed_ ).valid( ) )
					{
						if ( source->get_stream( )->properties( ).get_property_with_key( key_gop_closed_ ).value< int >( ) == 1 )
							gop_closed = 1;
					}
					
					if( source->get_stream( ) && ( source->get_stream( )->codec( ) != video_wrapper_->stream_codec_id( ) ) )
					   stream_types_match = false;

					//Check that bit rates match
					if( source->get_stream( ) && source->get_stream( )->bitrate( ) != video_wrapper_->context( )->bit_rate )
					{
						ARLOG_DEBUG5("Stream bitrates differ. Source: %1%, target: %2%")( source->get_stream( )->bitrate( ) )( video_wrapper_->context( )->bit_rate );
						stream_types_match = false;
					}
	
					if ( prop_force_.value< int >( ) )
					{
						encoding_ = true;
					}
					else if ( !source->get_stream( ) )
					{
						ARLOG_DEBUG3( "case 0: pos=%1%: no stream component")( get_position( ) );
						encoding_ = true;
					}
					else if ( source->is_deferred( ) )
					{
						ARLOG_DEBUG3( "case 1: pos=%1%: deferred frame must be rendered")( get_position( ) );
						encoding_ = true;
					}
					else if ( !last_frame_ && source->get_stream( )->key( ) != source->get_stream( )->position( ) )
					{
						ARLOG_DEBUG3( "case 2: pos=%1%: first frame not an I-frame")( get_position( ) );
						encoding_ = true;
					}
					else if ( !encoding_ && !stream_types_match )
					{
						ARLOG_DEBUG3( "case 3: pos=%1%: stream of different type: %2% vs %3%")( get_position( ) )
							( source->get_stream( )->codec( ) )( prop_profile_.value< pl::wstring >( ) );
						encoding_ = true;
					}
					else if ( !encoding_ && !continuous( last_frame_, source ) )
					{
						ARLOG_DEBUG3( "case 4: pos=%1%: started encoding due to non-contiguous packets")( get_position( ) );
						encoding_ = true;
					}
					else if ( encoding_ && ( !stream_types_match || source->get_stream( )->key( ) != source->get_stream( )->position( ) ) )
					{
						ARLOG_DEBUG3( "case 5: pos=%1%: already encoding and stream component does not indicate that we have reached the next key frame yet %2% %3% %4% %5% %6%")
							( get_position( ) )( gop_closed )( source->get_stream( )->codec( ) )( video_wrapper_->serialise( "video_codec" ) )
							( source->get_stream( )->key( ) )( source->get_stream( )->position( ) );
					}
					else if ( encoding_ && source->get_stream( )->key( ) == source->get_stream( )->position( ) && gop_closed )
					{
						ARLOG_DEBUG3( "case 6: pos=%1%: encoding turned off as next key is reached" )( get_position( ) );
						encoding_ = false;
					}
	
					if ( encoding_ )
					{
						result = render( source );
						result->set_position( source->get_position( ) );
						if ( result->get_image( ) )
							video_streamer_->push( result );
						else
							std::cerr << "Stream: no image after render" << std::endl;
					}
					else
					{
						video_streamer_->push( source->get_stream( ) );
						result = source;
					}

					result->set_stream( video_streamer_ );
					last_frame_ = result->shallow( );
					result = result->shallow( );	//TODO perhaps not needed
					video_streamer_->next( );
				}
				else
				{
					result = last_frame_->shallow( );
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
				//Ensure that the position of the rendered frame matches the source frame
				render_->seek( frame->get_position() );
				result = render_->fetch( );
			}
			else
			{
				result->get_image( );
			}

			result = ml::frame_convert( result, pf_ );

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

	private:
		pl::pcos::property prop_enable_;
		pl::pcos::property prop_force_;
		pl::pcos::property prop_profile_;
		pl::pcos::property prop_threads_;
		bool initialised_;
		ml::frame_type_ptr last_frame_;
		ml::input_type_ptr pusher_;
		ml::filter_type_ptr render_;
		bool encoding_;
		cl::profile_manager manager_;
		avformat_video *video_wrapper_;
		pl::wstring pf_;
		avformat_video_streamer_ptr video_streamer_;
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

