// avformat - A avformat plugin to ml.
//
// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.
//
// #input:avformat:
//
// Parses and decodes many video, audio and image formats via the libavformat
// API.

#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <openmedialib/ml/stream.hpp>
#include <openmedialib/ml/awi.hpp>
#include <openmedialib/ml/indexer.hpp>
#include <openmedialib/ml/audio_block.hpp>
#include <openmedialib/ml/keys.hpp>
#include <openmedialib/ml/io.hpp>

#include <openpluginlib/pl/pcos/isubject.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>
#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/worker.hpp>
#include <opencorelib/cl/log_defines.hpp>

#include <boost/foreach.hpp>

#include <vector>
#include <map>
#include <set>

#include "avformat_stream.hpp"
#include "avformat_wrappers.hpp"
#include "utils.hpp"
#include "avformat_streamable.hpp"
#include "avaudio_convert.hpp"

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
#include <libavformat/avio.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

namespace cl = olib::opencorelib;
namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;
namespace pcos = olib::openpluginlib::pcos;

namespace olib { namespace openmedialib { namespace ml {

// Alternative to Julian's patch?
static const AVRational ml_av_time_base_q = { 1, AV_TIME_BASE };

namespace {
	int context_to_sample_width( const AVCodecContext *ctx )
	{
		// special case for our representation of pcm24 / aiff24
		if ( CODEC_ID_PCM_S24LE == ctx->codec_id ||
			 CODEC_ID_PCM_S24BE == ctx->codec_id )
		{
			return 3;
		}

		switch( ctx->sample_fmt )
		{
			case AV_SAMPLE_FMT_U8:
			case AV_SAMPLE_FMT_U8P:
				return sizeof( boost::int8_t );

			case AV_SAMPLE_FMT_S16:
			case AV_SAMPLE_FMT_S16P:
				return sizeof( boost::int16_t );

			case AV_SAMPLE_FMT_S32:
			case AV_SAMPLE_FMT_S32P:
				return sizeof( boost::int32_t );

			case AV_SAMPLE_FMT_FLT:
			case AV_SAMPLE_FMT_FLTP:
				return sizeof( float );

			case AV_SAMPLE_FMT_DBL:
			case AV_SAMPLE_FMT_DBLP:
				return sizeof( double );

			case AV_SAMPLE_FMT_NONE:
			case AV_SAMPLE_FMT_NB:
				break;
		}

		ARENFORCE_MSG( false, "Unsupported sample format" )( av_get_sample_fmt_name( ctx->sample_fmt ) );
		return 0;
	}

}
	

// The avformat_source abstract class provides the common functionality which 
// is required for both the avformat_input class (which, by default, will demux 
// and decode everything on demand) and the avformat_demux class which is used
// by avformat_input when the packet_stream property is assigned - instead of 
// decoding this usage will provide a frame with a non-decoded stream_type_ptr 
// which holds the video packet and an audio::block_type_ptr which holds the 
// associated and adjacent audio packets.
//
// To minimise the impact on the existing avformat_input, avformat_source just
// holds a collection of member variables which were originally defined as 
// private to the avformat_input, but are now public to allow access from both
// avformat_input and avformat_demux.

class avformat_source : public input_type
{
	public:
		// This holds the next frame we expect to receive a request for
		int expected_;

		// This holds the next (video) packet we expect to receive while demuxing
		int expected_packet_;

		// This holds the position of the last I frame we demuxed
		boost::int64_t key_last_;

		// This holds the absolute byte offset in the source input of the last packet
		boost::int64_t last_packet_pos_;

		// These hold the frame rate of the container
		int fps_num_, fps_den_;

		// These hold the sample aspect ratio of the video
		int sar_num_, sar_den_;

		// These hold the dimensions of the video
		int width_, height_;

		// This holds the last packet read
		AVPacket pkt_;

		// This holds the avformat context for demuxing the input
		AVFormatContext *context_;

		// The avformat I/O context, which is also a part of context_ above
		AVIOContext *io_context_;

		// This holds the 'start time' of the first packet in the container
		int64_t start_time_;

		// This holds the awi index object and is derived from the ts_index property of the input
		mutable awi_index_ptr aml_index_;

		// These hold our internal view of the streams in a media
		std::vector < int > video_indexes_, audio_indexes_;

		avformat_source( )
			: expected_( 0 )
			, expected_packet_( 0 )
			, key_last_( -1 )
			, last_packet_pos_( 0 )
			, fps_num_( 25 )
			, fps_den_( 1 )
			, sar_num_( 59 )
			, sar_den_( 54 )
			, width_( 720 )
			, height_( 576 )
			, pkt_( )
			, context_( 0 )
			, io_context_( 0 )
			, start_time_( 0 )
		{
		}

		// Physically seek to the requested position within the media
		virtual bool seek_to_position( ) = 0;

		// Get the selected video stream
		virtual AVStream *get_video_stream( ) = 0;

		// Get the selected audio stream
		virtual AVStream *get_audio_stream( ) = 0;

		virtual bool is_video_stream( int index ) = 0;

		virtual bool is_indexing( ) = 0;
};

class stream_cache
{
	private:
		cl::lru_key key_;
		enum AVMediaType type_;
		size_t index_;
		boost::int64_t expected_;
		int lead_in_;
		boost::int64_t offset_;

	public:
		stream_cache( )
		: key_( lru_stream_cache::allocate( ) )
		, type_( AVMEDIA_TYPE_UNKNOWN )
		, index_( 0 )
		, expected_( 0 )
		, lead_in_( 0 )
		, offset_( 0 )
		{ }

		void set_type( enum AVMediaType type )
		{
			type_ = type;
		}

		void set_index( size_t index )
		{
			index_ = index;
		}

		size_t index( ) const
		{
			return index_;
		}

		void set_expected( boost::int64_t expected )
		{
			expected_ = expected;
		}

		const boost::int64_t expected( ) const
		{
			return expected_;
		}

		const int lead_in( ) const
		{
			return lead_in_;
		}

		void set_lead_in( int lead_in )
		{
			lead_in_ = lead_in;
		}

		void set_offset( boost::int64_t offset )
		{
			offset_ = offset;
		}

		boost::int64_t offset( ) const
		{
			return offset_;
		}

		void put( ml::stream_type_ptr stream, int duration )
		{
			lru_stream_ptr lru = lru_stream_cache::fetch( key_ );
			lru->resize( 300 );
			lru->append( stream->position( ), stream );
			expected_ = stream->position( ) + duration;
		}

		ml::stream_type_ptr fetch( boost::int64_t position )
		{
			ml::stream_type_ptr result;

			switch( type_ )
			{
				case AVMEDIA_TYPE_VIDEO:
					result = find_video( position );
					break;

				case AVMEDIA_TYPE_AUDIO:
					result = find_audio( position );
					break;

				default:
					ARENFORCE_MSG( false, "Unkown stream type %2%" )( type_ );
					break;
			};

			return result;
		}

	private:
		ml::stream_type_ptr find_audio( boost::int64_t position )
		{
			lru_stream_ptr lru = lru_stream_cache::fetch( key_ );
			boost::int64_t upper = std::max< boost::int64_t >( 0, position );
			boost::int64_t lower = std::max< boost::int64_t >( 0, upper - 4096 );
			ml::stream_type_ptr stream = lru->highest_in_range( upper, lower );
			if ( stream )
			{
				boost::int64_t from = stream->position( );
				boost::int64_t to = stream->position( ) + stream->samples( );
				while( position > to )
				{
					stream_type_ptr stream = lru->fetch( to );
					if ( !stream ) break;
					from = stream->position( );
					to = from + stream->samples( );
				}
				if ( position < from || position >= to )
					stream = ml::stream_type_ptr( );
			}
			return stream;
		}

		ml::stream_type_ptr find_video( boost::int64_t position )
		{
			lru_stream_ptr lru = lru_stream_cache::fetch( key_ );
			lru->resize( 300 );
			return lru->fetch( position );
		}
};

class avformat_demux
{
	private:
		typedef boost::shared_ptr< stream_avformat > stream_avformat_ptr;

		bool initialised_;
		avformat_source *source;
		std::map< size_t, stream_cache > caches;
		ml::audio::calculator calculator;
		AVPacket pkt_;
		std::map< size_t, boost::int64_t > unsampled_;

	public:
		avformat_demux( avformat_source *src )
		: initialised_( false )
		, source( src )
		{ }

		void do_packet_fetch( frame_type_ptr &result )
		{
			// Make sure we're initialised properly
			initialise( );

			// Get the requested position
			int position = source->get_position( );

			// Create the output frame
			result = frame_type_ptr( new frame_type( ) );
			result->set_position( position );
			result->set_fps( source->fps_num_, source->fps_den_ );

			if ( !populate( result ) )
			{
				// Handle unexpected position here
				if( seek( ) && position != 0 )
				{
					ARENFORCE_MSG( check_direction( position ), "Unable to get to %1%" )( position );
				}

				// Keep going until we have the full frame
				while( !populate( result ) )
				{
					if ( !obtain_next_packet( ) )
						break;
				}

				if ( result->audio_block( ) )
				{
					
				}

				// Increment the next expected frame
				source->expected_ = position + 1;
			}

			// Set the source timecode
			pl::pcos::assign< int >( result->properties( ), ml::keys::source_timecode, position );
		}

		bool check_direction( int position )
		{
			typedef std::map< size_t, stream_cache >::iterator iterator;
			bool result = true;

			do
			{
				// Set it back to true at the start of each iteration
				result = true;

				// Check that we have at least one packet per stream
				for ( iterator iter = caches.begin( ); result && iter != caches.end( ); iter ++ )
					result = iter->second.expected( ) != -1;

				// If result is false, then obtain another packet and check streams again unless eof is hit
				ARENFORCE_MSG( result || obtain_next_packet( ), "End of file reached" );
			}
			while( !result );

			// Now we need to confirm that we either have the packet in each stream or it's expected
			for ( iterator iter = caches.begin( ); result && iter != caches.end( ); iter ++ )
			{
				boost::int64_t stream_position = calculator.frame_to_position( iter->second.index( ), position ) - iter->second.lead_in( );
				if ( stream_position < 0 ) stream_position = 0;
				result = !( iter->second.fetch( stream_position ) == ml::stream_type_ptr( ) && iter->second.expected( ) > stream_position );
			}

			return result;
		}

	private:
		void initialise( )
		{
			// Initialise
			if ( !initialised_ )
			{
				// Ensure that we're always initialising from the start of the stream
				ARENFORCE_MSG( source->get_position( ) == 0, "Trying to initialise from a non zero position" );

				// Ensure we seek first
				seek( );

				// Obtain the requested video and audio stream objects
				AVStream *video = source->get_video_stream( );
				AVStream *audio = source->get_audio_stream( );

				// Ensure that we have at least one stream available
				ARENFORCE_MSG( video || audio, "No video or audio stream available" );

				// Determine the frequency of the selected stream (if none are selected, ignore audio)
				int frequency = audio ? audio->codec->sample_rate : 0;

				// Set the calculator's fps and derived frequency
				calculator.set_source( olib::opencorelib::str_util::to_string( source->get_uri( ) ) );
				calculator.set_fps( source->fps_num_, source->fps_den_ );
				calculator.set_frequency( frequency );

				// Add the video and all relevant audio streams here
				for( size_t i = 0; i < source->context_->nb_streams; i++ )
				{
					AVStream *stream = source->context_->streams[ i ];
					ml::stream_id type = ml::stream_unknown;

   					switch( stream->codec->codec_type ) 
					{
						case AVMEDIA_TYPE_VIDEO:
							if ( stream == video )
								type = ml::stream_video;
							break;

						case AVMEDIA_TYPE_AUDIO:
							if ( audio && stream->codec->sample_rate == frequency )
								type = ml::stream_audio;
		   					break;

						default:
		   					break;
					}

					if ( type != ml::stream_unknown )
					{
						// Inform the calculator of the video related information
						calculator.set_stream_type( i, type, stream->time_base.num, stream->time_base.den );

						// Create and initialise the cache
						stream_cache &handler = caches[ i ];
						handler.set_index( i );
						handler.set_type( stream->codec->codec_type );
						handler.set_lead_in( lead_in( stream ) );

						// Indicate that we have not found a packet for this stream yet
						unsampled_[ i ] = 0;
					}
				}

				if ( getenv( "AML_DTS_LOG" ) ) std::cerr << "Started analysing first packets" << std::endl;

				std::map< size_t, std::map< boost::int64_t, stream_avformat_ptr > > temporary_cache;

				while ( unsampled_.size( ) )
				{
					stream_avformat_ptr packet = obtain_next_packet( );

					if ( !packet )
					{
						if ( getenv( "AML_DTS_LOG" ) ) std::cerr << "End of stream before all first packets found" << std::endl;
						break;
					}

					size_t index = packet->index( );

					if ( unsampled_.find( index ) != unsampled_.end( ) )
					{
						if ( contiguous( index, temporary_cache[ index ], packet ) )
						{
							cache_block( index, temporary_cache[ index ] );
							unsampled_.erase( index );
						}
					}
				}

				if ( getenv( "AML_DTS_LOG" ) ) std::cerr << "Finished analysing first packets" << std::endl;

				initialised_ = true;
			}
		}

		bool contiguous( size_t index, std::map< boost::int64_t, stream_avformat_ptr > &block, stream_avformat_ptr packet )
		{
			bool result = false;

			pl::pcos::property_container properties = packet->properties( );
			boost::int64_t dts = pl::pcos::value< boost::int64_t >( properties, ml::keys::dts, 0 );
			int duration = pl::pcos::value< int >( properties, ml::keys::duration, 0 );
			int key_frame = pl::pcos::value< int >( properties, ml::keys::picture_coding_type, 0 );

			if ( block.size( ) == 0 )
			{
				if ( key_frame )
				{
					block[ dts ] = packet;
					calculator.set_stream_offset( index, dts );
				}
			}
			else
			{
				stream_avformat_ptr previous = ( -- block.end( ) )->second;
				pl::pcos::property_container prev_properties = previous->properties( );
				boost::int64_t prev_dts = pl::pcos::value< boost::int64_t >( prev_properties, ml::keys::dts, 0 );
				int prev_duration = pl::pcos::value< int >( prev_properties, ml::keys::duration, 0 );

				if ( prev_duration == 0 ) prev_duration = duration;

				if ( dts == prev_dts + prev_duration )
				{
					block[ dts ] = packet;
					result = key_frame == 1;
				}
				else
				{
					block.erase( block.begin( ), block.end( ) );
					if ( key_frame )
						block[ dts ] = packet;
				}
			}

			return result;
		}

		void cache_block( size_t index, std::map< boost::int64_t, stream_avformat_ptr > &block )
		{
			source->key_last_ = 0;
			for ( std::map< boost::int64_t, stream_avformat_ptr >::iterator iter = block.begin( ); iter != block.end( ); iter ++ )
			{
				stream_avformat_ptr packet = iter->second;
				pl::pcos::property_container properties = packet->properties( );
				boost::int64_t dts = pl::pcos::value< boost::int64_t >( properties, ml::keys::dts, 0 );
				int key_frame = pl::pcos::value< int >( properties, ml::keys::picture_coding_type, 0 );
				int pkt_duration = pl::pcos::value< int >( properties, ml::keys::duration, 0 );
				if ( iter == block.begin( ) )
					calculator.set_stream_offset( index, dts );
				boost::int64_t position = calculator.dts_to_position( index, dts );
				int duration = calculator.packet_duration( index, pkt_duration );
				size_t index = packet->index( );
				if ( index == 0 && key_frame )
					source->key_last_ = position;
				packet->set_position( position );
				packet->set_key( source->key_last_ );
				cache( index ).put( packet, duration );
			}
		}

		stream_avformat_ptr obtain_next_packet( )
		{
			size_t index = 0;
			stream_avformat_ptr packet;

			// Loop until we find the packet we want or an error occurs
			ml::stream_id got_packet = ml::stream_unknown;
			int error = 0;

			while( error >= 0 && got_packet == ml::stream_unknown )
			{
				// Clear the packet
				av_init_packet( &pkt_ );

				source->last_packet_pos_ = avio_tell( source->context_->pb );
				error = av_read_frame( source->context_, &pkt_ );
				index = pkt_.stream_index;

				if ( pkt_.pos != -1 )
					source->last_packet_pos_ = pkt_.pos;

				if ( error >= 0 && source->is_video_stream( pkt_.stream_index ) )
					got_packet = ml::stream_video;
				else if ( error >= 0 && has_cache_for( index ) )
					got_packet = ml::stream_audio;
				else if ( error >= 0 )
					av_free_packet( &pkt_ );

				if ( got_packet == ml::stream_video && source->key_last_ == -1 && pkt_.flags != AV_PKT_FLAG_KEY )
				{
					av_free_packet( &pkt_ );
					got_packet = ml::stream_unknown;
				}
			}

			if ( got_packet != ml::stream_unknown )
			{
				// Get the stream object associated to this packet
				AVStream *stream = got_packet == ml::stream_video ? source->get_video_stream( ) : source->context_->streams[ index ];

				// Obtain the codec context from the stream
				AVCodecContext *codec = stream->codec;

				// Calculate the position and duration of this packet
				boost::int64_t position = calculator.dts_to_position( index, pkt_.dts );
				int duration = calculator.packet_duration( index, pkt_.duration );

				// During the initial gop parsing, we rely entirely on dts evaluation, but subsequently we follow the expected packet
				if ( unsampled_.find( index ) == unsampled_.end( ) )
				{
					// Temporary diagnostics
					if ( getenv( "AML_DTS_LOG" ) ) std::cerr << "pkt: " << index << " " << pkt_.dts << " + " << pkt_.duration << " = " << position << " + " << duration << std::endl;

					// Temporary test to ensure that we can map back from the position to the dts
					boost::int64_t test = calculator.position_to_dts( index, position );
					if ( test != pkt_.dts ) if ( getenv( "AML_DTS_LOG" ) ) std::cerr << "Mismatch " << index << ": " << position << " " << pkt_.dts << " != " << test << std::endl;

					// Sync the expected position with the found one for this stream
					found( pkt_, position );
				}

				// Sync the last key found with the calculated position
				if ( got_packet == ml::stream_video && pkt_.flags == AV_PKT_FLAG_KEY )
					source->key_last_ = position;

				// Create the stream_type_ptr for the packet
				switch( got_packet )
				{
					case ml::stream_video:
					{
						// Avformat does not seem to have a clue of what the gop size is so we guess based on frame rate
						int estimated_gop_size = ceil( (double)source->fps_num_ / source->fps_den_ ) / 2;
						packet = stream_avformat_ptr( new stream_avformat( stream->codec->codec_id, pkt_.size, position, source->key_last_, codec->bit_rate, 
																		   ml::dimensions( source->width_, source->height_ ), ml::fraction( source->sar_num_, source->sar_den_ ), 
																		   avformat_to_oil( codec->pix_fmt ), il::top_field_first, estimated_gop_size ) );
					}
						break;

					case ml::stream_audio:
						packet = stream_avformat_ptr( new stream_avformat( stream->codec->codec_id, pkt_.size, position, position,
																		   0, codec->sample_rate, codec->channels, duration, context_to_sample_width( codec ) ) );
						break;

					default:
						// Not supported
						break;
				}

				// Populate the stream_type_ptr
				if ( packet )
				{
					// Obtain the handler because it's holding the byte offset state
					stream_cache &handler = caches[ index ];

					// If this packet has a position, use it, otherwise use the previously computed position
					if ( pkt_.pos != -1 )
						handler.set_offset( pkt_.pos );

					// Extract the properties object now
					pl::pcos::property_container properties = packet->properties( );

					// Set the stream index on the packet
					packet->set_index( index );

					// Copy the data from the avpacket to the stream_type_ptr
					memcpy( packet->bytes( ), pkt_.data, pkt_.size );

					// Input related properties
					pl::pcos::assign< std::wstring >( properties,  ml::keys::uri, source->get_uri( ) );

					if( source->context_->pb )
						pl::pcos::assign< boost::int64_t >( properties, ml::keys::source_position, source->context_->pb->pos );
					else
						pl::pcos::assign< boost::int64_t >( properties, ml::keys::source_position, 0 );

					// Packet related properties
					pl::pcos::assign< boost::int64_t >( properties, ml::keys::pts, pkt_.pts );
					pl::pcos::assign< boost::int64_t >( properties, ml::keys::dts, pkt_.dts );
					pl::pcos::assign< int >( properties, ml::keys::duration, pkt_.duration );

					// Absolute byte offset derived from packet and location in stream before a read
					pl::pcos::assign< boost::int64_t >( properties, ml::keys::source_byte_offset, handler.offset( ) );
					pl::pcos::assign< boost::int64_t >( properties, ml::keys::codec_tag, boost::int64_t( codec->codec_tag ) );
					pl::pcos::assign< int >( properties, ml::keys::codec_id, int( codec->codec_id ) );
					pl::pcos::assign< int >( properties, ml::keys::codec_type, int( codec->codec_type ) );

					// Codec related properties
					pl::pcos::assign< int >( properties, ml::keys::has_b_frames, codec->has_b_frames );

					// Stream related properties
					pl::pcos::assign< int >( properties, ml::keys::pid, stream->id );
					pl::pcos::assign< int >( properties, ml::keys::timebase_num, stream->time_base.num );
					pl::pcos::assign< int >( properties, ml::keys::timebase_den, stream->time_base.den );

					pl::pcos::property max_rate( ml::keys::max_rate );
					packet->properties( ).append( max_rate = codec->rc_max_rate );
					pl::pcos::property buffer_size( ml::keys::buffer_size );
					packet->properties( ).append( buffer_size = codec->rc_buffer_size );
					pl::pcos::property bit_rate( ml::keys::bit_rate );
					packet->properties( ).append( bit_rate = codec->bit_rate );
					pl::pcos::property field_order( ml::keys::field_order );
					packet->properties( ).append( field_order = int( codec->field_order ) );
					pl::pcos::property bits_per_coded_sample( ml::keys::bits_per_coded_sample );
					packet->properties( ).append( bits_per_coded_sample = codec->bits_per_coded_sample );
					pl::pcos::property ticks_per_frame( ml::keys::ticks_per_frame );
					packet->properties( ).append( ticks_per_frame = codec->ticks_per_frame );

					pl::pcos::property avg_fps_num( ml::keys::avg_fps_num );
					packet->properties( ).append( avg_fps_num = stream->avg_frame_rate.num );
					pl::pcos::property avg_fps_den( ml::keys::avg_fps_den );
					packet->properties( ).append( avg_fps_den = stream->avg_frame_rate.den );

					pl::pcos::property picture_coding_type( ml::keys::picture_coding_type );
					packet->properties( ).append( picture_coding_type = ( pkt_.flags == AV_PKT_FLAG_KEY ? 1 : 0 ) );

					// While we're caching the first gop, we don't have fixed positions, so don't cache yet
					// this will be done during the call to cache_block
					if ( unsampled_.find( index ) == unsampled_.end( ) && position >= 0 )
						cache( index ).put( packet, duration );

					// Update the byte offset for the next packet
					handler.set_offset( handler.offset( ) + packet->length( ) );
				}

				av_free_packet( &pkt_ );

			}

			return packet;
		}

		boost::int64_t lead_in( AVStream *stream )
		{
			boost::int64_t lead_in = 0;

			switch( stream->codec->codec_id )
			{
				case CODEC_ID_AAC:
					// If we're reading AAC we have 1024 samples per packet = 375/8 fps at 48 KHz.
					lead_in = 1024;
					break;

				case CODEC_ID_AC3:
					// If we're reading AC3 we have 1536 samples per packet = 31.25 fps at 48 KHz.
					lead_in = 1536;
					break;

				case CODEC_ID_MP2:
				case CODEC_ID_MP3:
					// If we're reading MP2 or MP3 we have 1152 samples per packet is 125/3 fps at 48 KHz.
					lead_in = 1152;
					break;

				default:
					break;
			}

			return lead_in;
		}

		void set_expected( int position, bool calc = true )
		{
			source->expected_ = position;

			for( size_t i = 0; i < source->context_->nb_streams; i++ )
			{
				if ( has_cache_for( i ) )
				{
					stream_cache &handler = caches[ i ];
					if ( calc )
					{
						boost::int64_t stream_position = calculator.frame_to_position( i, position );
						handler.set_expected( stream_position );
					}
					else
					{
						handler.set_expected( position );
					}
				}
			}
		}

		bool seek( )
		{
			bool result = false;
			int position = source->get_position( );
			if ( !source->is_indexing( ) && source->expected_ != position )
			{
				source->seek_to_position( );
				result = true;
				if ( source->aml_index_ && source->aml_index_->usable( ) )
				{
					set_expected( source->expected_packet_ );
				}
				else
				{
					source->expected_ = -1;
					set_expected( -1, false );
				}
				source->key_last_ = -1;
			}
			return result;
		}

		void found( AVPacket &pkt, boost::int64_t &position )
		{
			if ( has_cache_for( pkt.stream_index ) )
			{
				stream_cache &handler = caches[ pkt.stream_index ];
				if ( position != handler.expected( ) )
				{
					if ( getenv( "AML_DTS_LOG" ) ) std::cerr << "this packet is " << pkt_.stream_index << " " << position << " but we think it's " << handler.expected( ) << std::endl;
					if ( source->is_indexing( ) )
						position = handler.expected( );
					else
						handler.set_expected( position );
				}
			}
		}

		bool populate( ml::frame_type_ptr result )
		{
			// Count the number of tracks that fail
			int failures = 0;

			// Absolute frame position to populate
			int position = result->get_position( );

			// Try to obtain the video stream component from the cache
			if ( calculator.has_video( ) )
			{
				AVStream *video_stream = source->get_video_stream();
				ARENFORCE_MSG( video_stream, "Video stream is null even though calculator.has_video( ) returned true" )( position );
				// Update the output frame
				ml::stream_type_ptr packet = caches[ video_stream->index ].fetch( position );
				result->set_stream( packet );
				if ( packet == ml::stream_type_ptr( ) ) failures ++;
			}

			// Try to obtain the audio stream components from the cache
			if ( calculator.has_audio( ) )
			{
				// Generate the unpopulated audio blocks
				ml::audio::block_type_ptr block = calculator.calculate( position );

				// Iterate through the tracks in the audio block
				for ( ml::audio::block_type::iterator iter = block->tracks.begin( ); iter != block->tracks.end( ); iter ++ )
				{
					// Obtain the stream index
					size_t index = iter->first;

					// Obtain the cache object for this stream
					stream_cache &cache = caches[ index ];

					// Find the first audio stream component for this block
					ml::stream_type_ptr audio = cache.fetch( block->first );

					// If we have the first, continue to see if we can get the rest
					if ( audio != ml::stream_type_ptr( ) )
					{
						block->tracks[ index ].discard = cache.lead_in( );

						for( boost::int64_t offset = audio->position( ); offset < block->last + cache.lead_in( ); )
						{
							audio = cache.fetch( offset );
							if ( audio != ml::stream_type_ptr( ) )
							{
								block->tracks[ index ].packets[ offset ] = audio;
								offset += audio->samples( );

								// Calculate the discard if the first sample we want for this frame falls in this stream component
								if ( block->first >= audio->position( ) && block->first < offset )
									block->tracks[ index ].discard += block->first - audio->position( );
							}
							else
							{
								failures ++;
								break;
							}
						}
					}
					else
					{
						failures ++;
					}
				}


				// Set the derived block on the result frame
				result->set_audio_block( block );
			}

			return failures == 0;
		}

		stream_cache &cache( size_t id )
		{
			return caches[ id ];
		}

		bool has_cache_for( size_t id )
		{
			return caches.find( id ) != caches.end( );
		}
};

class ML_PLUGIN_DECLSPEC avformat_input : public avformat_source
{
	public:
		// Constructor and destructor
		avformat_input( std::wstring resource, const std::wstring mime_type = L"" ) 
			: uri_( resource )
			, mime_type_( mime_type )
			, frames_( 0 )
			, is_seekable_( true )
			, eof_ ( false )
			, first_video_frame_( true )
			, first_video_found_( 0 )
			, first_audio_frame_( true )
			, first_audio_found_( 0 )
			, format_( 0 )
			, prop_video_index_( pcos::key::from_string( "video_index" ) )
			, prop_audio_index_( pcos::key::from_string( "audio_index" ) )
			, prop_gop_size_( pcos::key::from_string( "gop_size" ) )
			, prop_gop_cache_( pcos::key::from_string( "gop_cache" ) )
			, prop_format_( pcos::key::from_string( "format" ) )
			, prop_genpts_( pcos::key::from_string( "genpts" ) )
			, prop_frames_( pcos::key::from_string( "frames" ) )
			, prop_file_size_( pcos::key::from_string( "file_size" ) )
			, prop_estimate_( pcos::key::from_string( "estimate" ) )
			, prop_fps_num_( pcos::key::from_string( "fps_num" ) )
			, prop_fps_den_( pcos::key::from_string( "fps_den" ) )
			, prop_ts_filter_( pcos::key::from_string( "ts_filter" ) )
			, prop_ts_index_( pcos::key::from_string( "ts_index" ) )
			, prop_ts_auto_( pcos::key::from_string( "ts_auto" ) )
			, prop_gop_open_( pcos::key::from_string( "gop_open" ) )
			, prop_gen_index_( pcos::key::from_string( "gen_index" ) )
			, prop_packet_stream_( pcos::key::from_string( "packet_stream" ) )
			, prop_fake_fps_( pcos::key::from_string( "fake_fps" ) )
			, prop_frequency_( pcos::key::from_string( "frequency" ) )
			, prop_inner_threads_( pcos::key::from_string( "inner_threads" ) )
			, prop_strict_( pcos::key::from_string( "strict" ) )
			, prop_whitelist_( pcos::key::from_string( "whitelist" ) )
			, av_frame_( 0 )
			, video_codec_( 0 )
			, audio_codec_( 0 )
			, images_( )
			, audio_( )
			, must_decode_( true )
			, must_reopen_( false )
			, key_search_( false )
			, key_last_( -1 )
			, audio_buf_used_( 0 )
			, audio_buf_offset_( 0 )
			, scaler_( 0 )
			, samples_per_frame_( 0.0 )
			, samples_per_packet_( 0 )
			, samples_duration_( 0 )
			, ts_pusher_( )
			, ts_filter_( )
			, next_time_( boost::get_system_time( ) + boost::posix_time::seconds( 2 ) )
			, unreliable_container_( false )
			, check_indexer_( true )
			, image_type_( false )
			, first_video_dts_( -1.0 )
			, first_audio_dts_( -1.0 )
			, discard_audio_packet_count_( 0 )
			, discard_audio_after_seek_( false )
			, demuxer_( this )
			, is_streaming_( false )
			, audio_filter_( 0 )
		{
			audio_buf_size_ = ( 192000 * 3) / 2;
			audio_buf_ = ( uint8_t * )av_malloc( 2 * audio_buf_size_ );

			// Allow property control of video and audio index
			// NB: Should also have read only props for stream counts
			properties( ).append( prop_video_index_ = 0 );
			properties( ).append( prop_audio_index_ = 0 );
			properties( ).append( prop_gop_size_ = -1 );
			properties( ).append( prop_gop_cache_ = 25 );
			properties( ).append( prop_format_ = std::wstring( L"" ) );
			properties( ).append( prop_genpts_ = 0 );
			properties( ).append( prop_frames_ = -1 );
			properties( ).append( prop_file_size_ = boost::int64_t( 0 ) );
			properties( ).append( prop_estimate_ = 0 );
			properties( ).append( prop_fps_num_ = -1 );
			properties( ).append( prop_fps_den_ = -1 );
			properties( ).append( prop_ts_filter_ = std::wstring( L"" ) );
			properties( ).append( prop_ts_index_ = std::wstring( L"" ) );
			properties( ).append( prop_ts_auto_ = 0 );
			properties( ).append( prop_gop_open_ = 0 );
			properties( ).append( prop_gen_index_ = 0 );
			properties( ).append( prop_packet_stream_ = 0 );
			properties( ).append( prop_fake_fps_ = 0 );
			properties( ).append( prop_frequency_ = -1 );
			properties( ).append( prop_inner_threads_ = 1 );
			properties( ).append( prop_strict_ = FF_COMPLIANCE_NORMAL );
			properties( ).append( prop_whitelist_ = std::wstring( L"" ) );

			// Allocate an av frame
			av_frame_ = avcodec_alloc_frame( );
		}

		virtual ~avformat_input( ) 
		{
			if ( prop_video_index_.value< int >( ) >= 0 )
				close_video_codec( );
			if ( prop_audio_index_.value< int >( ) >= 0 )
				close_audio_codec( );
			if ( context_ )
			{
				avformat_close_input( &context_ );
			}
			if ( io_context_ )
			{
				io::close_file( io_context_ );
				io_context_ = 0;
			}
			av_free( av_frame_ );
			av_free( audio_buf_ );

			if( indexer_item_ )
				ml::indexer_cancel_request( indexer_item_ );

			if ( scaler_ )
				sws_freeContext( scaler_ );

			delete audio_filter_;
		}

		// Indicates if the input will enforce a packet decode
		virtual bool requires_image( ) const { return true; }

		// For now, all oml protocol handlers are consider unsafe for threading
		virtual bool is_thread_safe( )
		{
			return uri_.find( L"oml:" ) != 0;
		}

		virtual double fps( ) const
		{
			int num, den;
			get_fps( num, den );
			return den != 0 ? double( num ) / double( den ) : 1;
		}

		// Basic information
		virtual const std::wstring get_uri( ) const { return uri_; }
		virtual const std::wstring get_mime_type( ) const { return mime_type_; }
		virtual bool has_video( ) const { return prop_video_index_.value< int >( ) >= 0 && prop_video_index_.value< int >( ) < int( video_indexes_.size( ) ); }
		virtual bool has_audio( ) const { return prop_audio_index_.value< int >( ) >= 0 && prop_audio_index_.value< int >( ) < int( audio_indexes_.size( ) ); }

		// Audio/Visual
		virtual int get_frames( ) const 
		{
			if (!is_seekable_ && !eof_) {
				return INT_MAX;
			}
			return frames_; 
		}

		virtual bool is_seekable( ) const { return is_seekable_; }

		// Visual
		virtual void get_fps( int &num, int &den ) const { num = fps_num_; den = fps_den_; }
		virtual void get_sar( int &num, int &den ) const { num = sar_num_; den = sar_den_; }
		virtual int get_video_streams( ) const { return video_indexes_.size( ); }
		virtual int get_width( ) const { return width_; }
		virtual int get_height( ) const { return height_; }

		// Audio
		virtual int get_audio_streams( ) const { return audio_indexes_.size( ); }

		virtual bool set_video_stream( const int stream ) { prop_video_index_ = stream; return true; }

		virtual bool set_audio_stream( const int stream ) 
		{
			if ( stream < 0 || stream >= int( audio_indexes_.size( ) ) )
			{
				prop_audio_index_ = -1;
			}
			else if ( stream < int( audio_indexes_.size( ) ) )
			{
				prop_audio_index_ = stream;
			}

			return true; 
		}

	protected:
		virtual bool initialize( )
		{
			// We will modify the input uri as required here
			std::wstring resource = uri_;

			// A mechanism to ensure that avformat can always be accessed
			if ( resource.find( L"avformat:" ) == 0 )
				resource = resource.substr( 9 );

			// Convenience expansion for *nix based people
			if ( resource.find( L"~" ) == 0 )
				resource = olib::opencorelib::str_util::to_wstring( getenv( "HOME" ) ) + resource.substr( 1 );
			if ( prop_ts_index_.value< std::wstring >( ).find( L"~" ) == 0 )
				resource = olib::opencorelib::str_util::to_wstring( getenv( "HOME" ) ) + prop_ts_index_.value< std::wstring >( ).substr( 1 );

			// Handle the auto generation of the ts_index url when requested
			if ( prop_ts_auto_.value< int >( ) && prop_ts_index_.value< std::wstring >( ) == L"" )
				prop_ts_index_ = std::wstring( resource + L".awi" );

			// Ugly - looking to see if a dv1394 device has been specified
			if ( resource.find( L"/dev/" ) == 0 && resource.find( L"1394" ) != std::wstring::npos )
			{
				prop_format_ = std::wstring( L"dv" );
			}

			// Allow dv on stdin
			if ( resource == L"dv:-" )
			{
				prop_format_ = std::wstring( L"dv" );
				resource = L"pipe:";
				is_seekable_ = false;
			}

			// Allow mpeg on stdin
			if ( resource == L"mpeg:-" )
			{
				prop_format_ = std::wstring( L"mpeg" );
				resource = L"pipe:";
				is_seekable_ = false;
				key_search_ = true;
			}

			// Allows url to be treated as a non-seekable, unknown length source
			if ( resource.find( L"avindex:" ) == 0 )
			{
				resource = resource.substr( 8 );
				is_seekable_ = false;
				key_search_ = true;
			}

			// Corrections for file formats
			if ( resource.find( L".mpg" ) == resource.length( ) - 4 ||
				 resource.find( L".mpeg" ) == resource.length( ) - 5 )
			{
				prop_format_ = std::wstring( L"mpeg" );
				key_search_ = true;
			}
			else if ( resource.find( L".dv" ) == resource.length( ) - 3 )
			{
				prop_format_ = std::wstring( L"dv" );
			}
			else if ( resource.find( L".mp2" ) == resource.length( ) - 4 )
			{
				prop_format_ = std::wstring( L"mpeg" );
			}
			else if ( resource.find( L".mp3" ) == resource.length( ) - 4 )
			{
				prop_video_index_ = -1;
				if ( prop_fps_num_.value< int >( ) == -1 || prop_fps_den_.value< int >( ) == -1 )
				{
					prop_fps_num_ = 25;
					prop_fps_den_ = 1;
				}
			}

			// Obtain format
			if ( prop_format_.value< std::wstring >( ) != L"" )
				format_ = av_find_input_format( olib::opencorelib::str_util::to_string( prop_format_.value< std::wstring >( ) ).c_str( ) );

			// Since we may need to reopen the file, we'll store the modified version now
			resource_ = resource;

			// Attempt to open the resource
			int error_code = io::open_file( &io_context_, cl::str_util::to_string( resource ).c_str(), AVIO_FLAG_READ );

			context_ = avformat_alloc_context();
			context_->pb = io_context_;
			error_code = avformat_open_input( &context_, cl::str_util::to_string( resource ).c_str( ), format_, 0 );
			int error = ( error_code < 0 );
			if( error )
			{
				ARLOG_ERR( "Got error %1% from avformat_open_input when opening file %2%" )
					( error_code )( resource );
			}

			if (error == 0 && context_->pb && context_->pb->seekable && resource.find( L"rtsp://" ) != 0) 
			{
				boost::uint16_t index_entry_type = prop_video_index_.value< int >( ) == -1 ? 2 : 1;
				if ( prop_ts_index_.value< std::wstring >( ) != L"" )
					indexer_item_ = ml::indexer_request( prop_ts_index_.value< std::wstring >( ), ml::index_type::awi, index_entry_type );
				else
					indexer_item_ = ml::indexer_request( resource, ml::index_type::media, index_entry_type );

				if ( indexer_item_ && indexer_item_->index( ) )
					aml_index_ = indexer_item_->index( );
				else if ( prop_ts_index_.value< std::wstring >( ) != L"" && prop_ts_auto_.value< int >( ) != -1 )
				{
					ARLOG_ERR( "Indexing of file through awi index failed.\nFile: %1%\nIndex: %2%" )
						( uri_ )( prop_ts_index_.value< std::wstring >( ) );
					return false;
				}
			}

			// Check for streaming
			if ( error == 0 && context_->pb && !context_->pb->seekable )
			{
				is_seekable_ = false;
				key_search_ = true;
			}

			// Get the stream info
			if ( error == 0 )
			{
				error_code = avformat_find_stream_info( context_, 0 );
				error = ( error_code < 0 );
				if ( error )
				{
					ARLOG_ERR( "avformat_find_stream_info returned code %1% for file %2%" )
						( error_code )( uri_ );
				}
				else if ( prop_format_.value< std::wstring >( ) != L"" && uint64_t( context_->duration ) == AV_NOPTS_VALUE )
				{
					ARLOG_WARN( "Tried opening %1% with supplied format %2%, but it failed. Will retry with format auto-detect." )
						( uri_ )( prop_format_.value< std::wstring >( ) );
					error = true;
				}
			}
			
			// Just in case the requested/derived file format is incorrect, on a failure, try again with no format
			if ( error && format_ )
			{
				format_ = 0;
				if ( context_ )
					avformat_close_input( &context_ );
				error_code = avformat_open_input( &context_, cl::str_util::to_string( resource ).c_str( ), format_, 0  );
				error = ( error_code < 0 );
				if ( error )
				{
					ARLOG_ERR( "avformat_open_input return code %1% for file %2%" )( error_code )( uri_ );
				}
				else
				{
					error_code = avformat_find_stream_info( context_, 0 );
					error = ( error_code < 0 );
					if ( error )
					{
						ARLOG_ERR( "avformat_find_stream_info returned code %1% for file %2%" )
							( error_code )( uri_ );
					}
				}
			}

			// Populate the input properties
			if ( error == 0 )
				populate( );

			if ( error == 0 )
				image_type_ = std::string( context_->iformat->name ) == "image2" ||
							  std::string( context_->iformat->name ) == "yuv4mpegpipe";

			// Determine if packet streaming has been requested
			is_streaming_ = is_streaming( );

			// Set up the input for decode or demux
			if ( !is_streaming_ )
			{
				// Check for and create the timestamp correction filter graph
				if ( prop_ts_filter_.value< std::wstring >( ) != L"" )
				{
					ts_filter_ = create_filter( prop_ts_filter_.value< std::wstring >( ) );
					if ( ts_filter_ )
					{
						ts_pusher_ = create_input( L"pusher:" );
						ts_filter_->connect( ts_pusher_ );
					}
				}

				// Align with first frame
				if ( error == 0 && !image_type_ )
				{
					seek_to_position( );
					fetch( );
				}

				// Determine number of frames in the input
				if ( error == 0 )
				{
					check_indexer_ = false;
					size_media( );
					check_indexer_ = true;
				}
			}
			else if ( error == 0 )
			{
				seek_to_position( );
				fetch( );
			}

			return error == 0;
		}

		bool is_streaming( )
		{
			bool result = false;

   			if ( context_ && prop_packet_stream_.value< int >( ) != 0 )
			{
				if ( prop_packet_stream_.value< int >( ) > 0 )
				{
					// If packet_stream > 0, then we will attempt to honour the request
					result = true;
				}
				else
				{
					// If packet_stream < 0, then we will only accept it with specific combinations of container, indexing and codecs
					std::string format = context_->iformat->name;
					std::string codec = has_video( ) ? get_video_stream( )->codec->codec->name : "";
					std::string whitelist = cl::str_util::to_string( prop_whitelist_.value< std::wstring >( ) );

					result = avformat::is_streamable( whitelist, format, codec, aml_index_ != awi_index_ptr( ) );

					ARLOG_DEBUG3( "Packet stream is %s with %s/%s and indexing %s" )( result ? "on" : "off" )( format )( codec )( aml_index_ ? "on" : "off" );
				}
			}

			return result;
		}

		void do_fetch( frame_type_ptr &result )
        {
            ARENFORCE_MSG( context_, "Invalid media" );

			// Just in case
			if ( get_position( ) >= get_frames( ) )
				sync( );

            // Route to either decoded or packet extraction mechanisms
            if ( is_streaming_ )
                demuxer_.do_packet_fetch( result );
            else
                do_decode_fetch( result );
            
            // Add the source_timecode prop
			pl::pcos::assign< int >( result->properties( ), ml::keys::source_timecode, result->get_position( ) );
			pl::pcos::assign< std::wstring >( result->properties( ), ml::keys::source_format, olib::opencorelib::str_util::to_wstring( context_->iformat->name ) );
        }

		// Fetch method
		void do_decode_fetch( frame_type_ptr &result )
		{
			ARSCOPELOG_IF_ENV( "AMF_PERFORMANCE_LOGGING" );

			// Repeat last frame if necessary
			if ( last_frame_ && get_position( ) == last_frame_->get_position( ) )
			{
				ARLOG_IF_ENV( "AMF_PERFORMANCE_LOGGING", "Reusing last frame at position %1%" )( get_position() );
				result = last_frame_->shallow( );
				return;
			}

			int process_flags = get_process_flags( );

			// Check if we have this position now
			bool got_picture = has_video_for( get_position( ) );
			bool got_audio = has_audio_for( get_position( ) );

			// Create the output frame
			result = frame_type_ptr( new frame_type( ) );

			// Seek to the correct position if necessary
			if ( get_position( ) != expected_ )
			{
				int current = get_position( );
				int first = 0;
				int last = 0;

				if ( images_.size( ) > 0 )
				{
					first = images_[ 0 ]->position( );
					last = images_[ images_.size( ) - 1 ]->position( );
				}
				else if ( audio_.size( ) > 0 )
				{
					first = audio_[ 0 ]->position( );
					last = audio_[ audio_.size( ) - 1 ]->position( );
				}

				if ( ( has_video( ) && has_audio( ) && got_picture && got_audio ) || 
					 ( has_video( ) && !has_audio( ) && got_picture ) ||
					 ( !has_video( ) && has_audio( ) && got_audio ) )
				{
				}
				else if ( key_last_ != -1 && current >= first && current < int( key_last_ + prop_gop_cache_.value< int >( ) ) )
				{
				}
				else if ( current >= first && current < int( last + prop_gop_cache_.value< int >( ) ) )
				{
				}
				else if ( aml_index_ && aml_index_->usable( ) )
				{
					if ( aml_index_->key_frame_of( current ) != aml_index_->key_frame_of( last ) )
					{
						if ( ( current < first || current > last + 2 ) && seek_to_position( ) )
							clear_stores( true );
					}
				}
				else if ( seek_to_position( ) )
				{
					clear_stores( true );
				}
				else
				{
					seek( expected_ );
				}
				expected_ = get_position( );
			}

			// Clear the packet
			av_init_packet( &pkt_ );

			// Loop until an error or we have what we need
			int error = 0;
			bool packet_failure = false;

			// Check if we still have both components after handling position shift
			got_picture = has_video_for( get_position( ) );
			got_audio = has_audio_for( get_position( ) );

			while( error >= 0 && ( !got_picture || !got_audio ) )
			{
				last_packet_pos_ = avio_tell( context_->pb );
				error = av_read_frame( context_, &pkt_ );
				if ( error < 0)
				{
					if (url_feof(context_->pb)) {
						ARLOG_ERR( "Got EOF at position %1%" )( get_position( ) );
						eof_ = true; //Mark eof to let get_frames start returning the correct number of frames
					} else {
						ARLOG_ERR( "Failed to read frame. Position = %1% real error = %2% eof = %3%" )( get_position( ) )( error )( url_feof(context_->pb) );

					}
					packet_failure = true;
					break;
				}
				if ( is_video_stream( pkt_.stream_index ) )
					error = decode_image( got_picture, &pkt_ );
				else if ( is_audio_stream( pkt_.stream_index ) )
					error = decode_audio( got_audio );
				else if ( !is_seekable_ )
					frames_ = get_position( );
				else if ( prop_genpts_.value< int >( ) == 1 )
					frames_ = get_position( );

				av_free_packet( &pkt_ );
				
                if ( error < 0 )
                {
                    ARLOG_ERR( "Failed to decode frame. Position = %1% Error = %2%" )( get_position( ) )( error );
					break;
                }
			}

			// Special case for eof - last video frame is retrieved via a null packet
            // If we have an index then it has to be completed for us to do this
			if ( frames_ != 1 && has_video( ) && packet_failure &&
                 ( aml_index_ == 0 || ( aml_index_ != 0 && aml_index_->finished( ) ) ) )
			{
				bool temp = false;
				int ret = 0;
				int count = first_video_found_;

				while ( count -- >= 0 )
				{
					av_init_packet( &pkt_ );
					pkt_.data = NULL;
					pkt_.size = 0;

					ret = decode_image( temp, &pkt_ );
					if ( !temp )
					{
						error =-1;
						ARLOG_WARN( "Failed to decode null packet at eof. Error = %1%" )( ret );
					} 
				}

				if ( frames_ == INT_MAX && images_.size( ) )
				{
					frames_ = ( *( -- images_.end( ) ) )->position( ) + 1;
				}
			}

			// Hmmph
			if ( has_video( ) )
			{
				if ( get_video_stream( )->codec->sample_aspect_ratio.num )
				{
					sar_num_ = get_video_stream( )->codec->sample_aspect_ratio.num;
					sar_den_ = get_video_stream( )->codec->sample_aspect_ratio.den;
				}
				else if ( get_video_stream( )->sample_aspect_ratio.num )
				{
					sar_num_ = get_video_stream( )->sample_aspect_ratio.num;
					sar_den_ = get_video_stream( )->sample_aspect_ratio.den;
				}

				sar_num_ = sar_num_ != 0 ? sar_num_ : 1;
				sar_den_ = sar_den_ != 0 ? sar_den_ : 1;
			}

			result->set_sar( sar_num_, sar_den_ );
			result->set_fps( fps_num_, fps_den_ );
			result->set_position( get_position( ) );
			result->set_pts( expected_ * 1.0 / avformat_input::fps( ) );
			result->set_duration( 1.0 / avformat_input::fps( ) );

			bool exact_image = false;
			bool exact_audio = false;

			if ( ( process_flags & process_image ) && has_video( ) )
				exact_image = find_image( result );
			if ( ( process_flags & process_audio ) && has_audio( ) )
				exact_audio = find_audio( result );

			// Update the next expected position
			if ( exact_image || exact_audio )
				expected_ ++;

			last_frame_ = result->shallow( );
		}

		void sync_frames( )
		{
			if ( check_indexer_ )
				sync_with_index( );
		}

	private:
		void select_audio_stream_from_freq( const int frequency )
		{
			size_t i = 0, ai = 0;
			for( ; i < context_->nb_streams; ++i ) {
				if( context_->streams[ i ]->codec->sample_rate == frequency ) {
					prop_audio_index_ = int( ai );
					break;
				}
				if( context_->streams[ i ]->codec->codec_type == AVMEDIA_TYPE_AUDIO ) ++ai;
			}
			
			if( i == context_->nb_streams ) {
				ARLOG_WARN( "No audio stream with frequency %1% found" )( frequency );
				prop_audio_index_ = -1;
			}
		}
	
		void sync_with_index( )
		{
			if ( aml_index_ )
			{
				if ( aml_index_->total_frames( ) > frames_ )
				{
					const boost::int64_t file_size = avio_size( context_->pb );
					prop_file_size_ = file_size;
					int new_frames = aml_index_->calculate( file_size );
					ARENFORCE_MSG( new_frames >= frames_, "Media shrunk on sync! Last duration was: %1%, new duration is: %2% (index frames: %3%)" )
						( frames_ )( new_frames )( aml_index_->total_frames( ) );
					frames_ = new_frames;
					ARLOG_DEBUG3( "Resynced with index. Calculated frames = %1%. Index frames = %2%" )( frames_ )( aml_index_->total_frames( ) );
					last_frame_ = ml::frame_type_ptr( );
				}
			}
			else if ( indexer_item_ )
			{
				if ( indexer_item_->size( ) > prop_file_size_.value< boost::int64_t >( ) )
				{
					reopen( );
					last_frame_ = ml::frame_type_ptr( );
				}
			}
		}

		void size_media( )
		{
			// Check if we need to do additional size checks
			bool sizing = should_size_media( context_->iformat->name );

			// Avoid offsetting audio incorrectly with certain formats
			if ( !strcmp( context_->iformat->name, "avi" ) || !strcmp( context_->iformat->name, "mkv" ) || !strcmp( context_->iformat->name, "flv" ) )
				first_audio_dts_ = 0.0;

			// Carry out the media sizing logic
			if ( !aml_index_ )
			{
				fetch( );

				if ( sizing && images_.size( ) )
				{
					switch( prop_estimate_.value< int >( ) )
					{
						case 0:
							frames_ = size_media_by_images( );
							break;
						case 1:
							frames_ = size_media_by_packets( );
							break;
						default:
							break;
					}
				}
				else if( sizing )
				{
					frames_ = size_media_by_packets();
				}

				// Courtesy - ensure we're reposition on the first frame
				seek( 0 );
			}
			else if ( aml_index_ )
			{
				boost::int64_t bytes = boost::int64_t( avio_size( context_->pb ) );
				frames_ = aml_index_->calculate( bytes );
			}
		}

		virtual bool is_indexing( ) { return !is_seekable_; }

		bool is_video_stream( int index )
		{
			return has_video( ) && 
				   prop_video_index_.value< int >( ) > -1 && 
				   prop_video_index_.value< int >( ) < int( video_indexes_.size( ) ) && 
				   index == video_indexes_[ prop_video_index_.value< int >( ) ];
		}

		bool is_audio_stream( int index )
		{
			return has_audio( ) && 
				   prop_audio_index_.value< int >( ) > -1 && 
				   prop_audio_index_.value< int >( ) < int( audio_indexes_.size( ) ) && 
				   index == audio_indexes_[ prop_audio_index_.value< int >( ) ];
		}

		bool has_video_for( int position )
		{
			int process_flags = get_process_flags( );
			bool got_video = !has_video( );
			int current = position;

			if ( ( process_flags & process_image ) != 0 && ( !got_video && images_.size( ) > 0 ) )
			{
				int first = images_[ 0 ]->position( );
				int last = images_[ images_.size( ) - 1 ]->position( );
				got_video = current >= first && current <= last;
			}

			return got_video;
		}

		bool has_audio_for( int position )
		{
			int process_flags = get_process_flags( );
			bool got_audio = !has_audio( );
			int current = position;

			if ( ( process_flags & process_audio ) != 0 && ( !got_audio && audio_.size( ) > 0 ) )
			{
				int first = audio_[ 0 ]->position( );
				int last = audio_[ audio_.size( ) - 1 ]->position( );
				got_audio = ( first == first_audio_found_ && current <= first_audio_found_ ) || ( current >= first && current <= last );
			}

			return got_audio;
		}

		bool is_valid( )
		{
			return context_ != 0;
		}

		void get_fps_from_stream( AVStream *stream )
		{
			// TODO: Need to check this one
			if ( stream->r_frame_rate.num != 0 && stream->r_frame_rate.den != 0 )
			{
				fps_num_ = stream->r_frame_rate.num;
				fps_den_ = stream->r_frame_rate.den;
			}
			else
			{
				fps_num_ = stream->codec->time_base.den;
				fps_den_ = stream->codec->time_base.num;
			}
			
			// Correct some fps values coming form avformat
			if( fps_num_ == 2997 && fps_den_ == 100 )
			{
				fps_num_ = 30000;
				fps_den_ = 1001;
			}

			if ( prop_fake_fps_.value< int >( ) )
			{
				// HACK FOR 60000:1001 - TREAT AS TELECINE
				if ( fps_num_ == 60000 && fps_den_ == 1001 )
				{
					fps_num_ = 24000;
					fps_den_ = 1001;
				}
				// HACK FOR 50:1 - TREAT AS PAL
				else if ( fps_num_ == 50 && fps_den_ == 1 )
				{
					fps_num_ = 25;
					fps_den_ = 1;
				}
			}
		}

		// Analyse streams and set input values
		void populate( )
		{
			// Iterate through all the streams available
			for( size_t i = 0; i < context_->nb_streams; i++ ) 
			{
				// Get the codec context
   				AVCodecContext *codec_context = context_->streams[ i ]->codec;

				// Ignore streams that we don't have a decoder for
				if ( avcodec_find_decoder( codec_context->codec_id ) == NULL )
				{
					unreliable_container_ = true;
					continue;
				}
		
				// Determine the type and obtain the first index of each type
   				switch( codec_context->codec_type ) 
				{
					case AVMEDIA_TYPE_VIDEO:
						video_indexes_.push_back( i );
						break;
					case AVMEDIA_TYPE_AUDIO:
						audio_indexes_.push_back( i );
		   				break;
					default:
		   				break;
				}
			}
			
			// If the client has selected a frequency then we set audio_index to the first
			// stream with that frequency
			if( prop_frequency_.value< int >() != -1 )
				select_audio_stream_from_freq( prop_frequency_.value< int >( ) );


			// Configure video input properties
			if ( has_video( ) )
			{
				AVStream *stream = get_video_stream( ) ? get_video_stream( ) : context_->streams[ 0 ];

				width_ = stream->codec->width;
				height_ = stream->codec->height;

				sar_num_ = stream->codec->sample_aspect_ratio.num;
				sar_den_ = stream->codec->sample_aspect_ratio.den;

				sar_num_ = sar_num_ != 0 ? sar_num_ : 1;
				sar_den_ = sar_den_ != 0 ? sar_den_ : 1;

				get_fps_from_stream( stream );
		                frames_ = stream->nb_frames;
			}
			else if ( has_audio( ) )
			{
				AVStream *video_stream = video_indexes_.size( ) > 0 ? context_->streams[ 0 ] : 0;
				AVStream *audio_stream = get_audio_stream();
				ARENFORCE_MSG( audio_stream != 0, "Invalid audio stream index: %1%" )( prop_audio_index_.value< int >( ) );
				
				if ( prop_fps_num_.value< int >( ) > 0 && prop_fps_den_.value< int >( ) > 0 )
				{
					fps_num_ = prop_fps_num_.value< int >( );
					fps_den_ = prop_fps_den_.value< int >( );
				}
				else if( audio_stream->codec->codec_id == CODEC_ID_AAC && aml_index_ )
				{
					//If we're reading AAC with index, the index "frames" will be
					//one AAC packet. 1024 samples per packet is 375/8 fps at 48 KHz.
					int freq = audio_stream->codec->sample_rate;
					cl::rational_time aac_fps( freq, 1024 );
					fps_num_ = aac_fps.numerator();
					fps_den_ = aac_fps.denominator();
				}
				else if( audio_stream->codec->codec_id == CODEC_ID_MP2 && aml_index_ && aml_index_->type( ) == 2 )
				{
					//If we're reading MP2 with index, the index "frames" will be
					//one MP2 packet. 1152 samples per packet is 125/3 fps at 48 KHz.
					int freq = audio_stream->codec->sample_rate;
					cl::rational_time aac_fps( freq, 1152 );
					fps_num_ = aac_fps.numerator();
					fps_den_ = aac_fps.denominator();
				}
				else if ( video_stream )
				{
					get_fps_from_stream( video_stream );
				}
				else
				{
					fps_num_ = 25;
					fps_den_ = 1;
				}

				sar_num_ = 1;
				sar_den_ = 1;
			}

			// Open the video and audio codecs
			if ( has_video( ) ) open_video_codec( );
			if ( has_audio( ) ) open_audio_codec( );

			if ( uint64_t( context_->start_time ) != AV_NOPTS_VALUE )
				start_time_ = context_->start_time;

			// Set the duration
			if ( aml_index_ )
				sync_with_index();
			else if (frames_ > 0)
		                ;
			else if ( uint64_t( context_->duration ) != AV_NOPTS_VALUE )
				frames_ = int( ceil( ( avformat_input::fps( ) * context_->duration ) / ( double )AV_TIME_BASE ) );
			else if ( std::string( context_->iformat->name ) == "yuv4mpegpipe" )
				frames_ = 1;
			else
				frames_ = INT_MAX;

			std::string format = context_->iformat->name;

			// Work around for inefficiencies on I frame only seeking
			// - this should be covered by the AVStream discard and need_parsing values
			//   but they're either wrong or poorly documented...
			if ( has_video( ) )
			{
				std::string codec( get_video_stream( )->codec->codec->name );
				if ( codec == "mjpeg" )
					must_decode_ = false;
				else if ( codec == "dvvideo" )
					must_decode_ = false;
				else if ( codec == "rawvideo" )
					must_decode_ = false;
				else if ( codec == "dirac" )
					must_reopen_ = true;

				if ( format == "mpeg" && prop_gop_open_.value< int >( ) == 0 )
					prop_gop_open_ = 2;
				else if ( format == "mpegts" && prop_gop_open_.value< int >( ) == 0 )
					prop_gop_open_ = 2;

				if ( format == "mpeg" && prop_gop_size_.value< int >( ) == -1 )
					prop_gop_size_ = 24;
				else if ( format == "mpegts" && prop_gop_size_.value< int >( ) == -1 )
					prop_gop_size_ = 24;
				else if ( format == "mov,mp4,m4a,3gp,3g2,mj2" && prop_gop_size_.value< int >( ) == -1 )
					prop_gop_size_ = 12;
				else if ( prop_gop_size_.value< int >( ) == -1 )
					prop_gop_size_ = 0;
			}
			else
			{
				if ( format == "mpeg" && prop_gop_open_.value< int >( ) == 0 )
					prop_gop_open_ = 2;
				else if ( format == "mpegts" && prop_gop_open_.value< int >( ) == 0 )
					prop_gop_open_ = 2;

				if ( format == "mp3" && prop_gop_size_.value< int >( ) == -1 )
					prop_gop_size_ = 12;
				else if ( format == "mp2" && prop_gop_size_.value< int >( ) == -1 )
					prop_gop_size_ = 12;
				else if ( format == "mpegts" && prop_gop_size_.value< int >( ) == -1 )
					prop_gop_size_ = 24;
				else if ( format == "mov,mp4,m4a,3gp,3g2,mj2" && prop_gop_size_.value< int >( ) == -1 )
					prop_gop_size_ = 12;
				else if ( prop_gop_size_.value< int >( ) == -1 )
					prop_gop_size_ = 1;
			}

			// Report the file size via the file_size and frames properties
			prop_file_size_ = boost::int64_t( avio_size( context_->pb ) );

			if ( has_audio( ) )
			{
				AVCodecContext *codec_context = get_audio_stream( )->codec;
				discard_audio_after_seek_ |= codec_context->codec_id == CODEC_ID_AAC;
				discard_audio_after_seek_ |= codec_context->codec_id == CODEC_ID_AC3;
				discard_audio_after_seek_ |= codec_context->codec_id == CODEC_ID_MP2;
				discard_audio_after_seek_ |= codec_context->codec_id == CODEC_ID_MP3;
			}
		}

		bool should_size_media( std::string format )
		{
			bool result = is_seekable_ && prop_genpts_.value< int >( ) == 0;

			if ( result && has_video( ) )
			{
				std::string codec( get_video_stream( )->codec->codec->name );

				if ( format == "avi" )
					result = codec == "dvvideo";
				else if ( format == "image2" )
					result = false;
				else if ( format == "yuv4mpegpipe" )
					result = false;
				else if ( format == "dv" )
					result = false;
				else if ( format == "mxf" )
					result = false;
				else if ( format == "matroska" )
				{
					result = false;
					prop_genpts_ = 2;
				}
				else if ( format == "flv" )
					result = false;
				else if ( format == "divx" )
					result = false;
				else if ( format == "mov,mp4,m4a,3gp,3g2,mj2" )
					result = true;
				else if ( format == "rtsp" )
				        result = false;
				else if ( format == "rtp" )
					result = false;
				else if ( format == "sdp" )
					result = false;
			}
			else if ( result && has_audio( ) )
			{
				if ( format == "avi" )
					result = false;
				else if ( format == "image2" )
					result = false;
				else if ( format == "dv" )
					result = false;
				else if ( format == "mxf" )
					result = false;
				else if ( format == "matroska" )
				{
					result = false;
					prop_genpts_ = 2;
				}
				else if ( format == "flv" )
					result = false;
				else if ( format == "divx" )
					result = false;
				else if ( format == "mov,mp4,m4a,3gp,3g2,mj2" )
					result = false;
				else if ( format == "rtsp" )
					result = false;
				else if ( format == "rtp" )
					result = false;
				else if ( format == "sdp" )
					result = false;
				else
					result = format != "wav";
			}

			return result;
		}

		int size_media_by_images( )
		{
			ARENFORCE_MSG( frames_ > 0, "Invalid initial evaluation of frame count %d - cannot be <= 0" )( frames_ );

			int attempts = 50;

			// Seek backwards in gop sized blocks until an image with the requested position is found
			do
			{
				ml::frame_type_ptr frame = fetch( frames_ - 1 );

				if ( frame && frame->get_image( ) && frame->get_image( )->position( ) == get_position( ) )
					break;
				else
					frames_ = frames_ > 12 ? frames_ - 12 : frames_ - 1;
			}
			while( frames_ > 0 && attempts -- );

			ARENFORCE_MSG( attempts > 0, "Unable to determine the duration" );

			// Step forward until we no longer get an image at the requested position
			while( frames_ > 0 && attempts -- )
			{
				frames_ += 1;
				ml::frame_type_ptr frame = fetch( frames_ - 1 );
				if ( !( frame && frame->get_image( ) && frame->get_image( )->position( ) == get_position( ) ) )
				{
					frames_ -= 1;
					break;
				}
			}

			return frames_;
		}

		int size_media_by_packets( )
		{
			AVStream *stream = get_video_stream( ) ? get_video_stream( ) : get_audio_stream( );

			int max = frames_;

			seek( frames_ );
			seek_to_position( );

			av_init_packet( &pkt_ );
			bool continue_trying = true;
			while ( stream && av_read_frame( context_, &pkt_ ) >= 0 )
			{
				if ( is_audio_stream( pkt_.stream_index ) )
				{
					bool fallback = true;

					// Handle the custom frame rate if specified
					if ( continue_trying && prop_fps_num_.value< int >( ) == fps_num_ && prop_fps_den_.value< int >( ) == fps_den_ )
					{
						if ( samples_per_packet_ == 0 )
						{
							AVCodecContext *codec_context = get_audio_stream( )->codec;

							int got_frame = 0;
							int length = avcodec_decode_audio4( codec_context, av_frame_, &got_frame, &pkt_ );
							if ( length <= 0 )
							{
								ARLOG_WARN( "Failed to decode audio while sizing. Will use fallback sizing mechanism." );
							}
							else if ( got_frame )
							{
								samples_per_frame_ = double( int64_t( codec_context->sample_rate ) * fps_den_ ) / fps_num_;
								samples_per_packet_ = av_frame_->nb_samples;
								samples_duration_ = pkt_.duration;
							}
						}

						if ( pkt_.duration > 0 && samples_duration_ == pkt_.duration && samples_per_frame_ > 0.0 )
						{
							int64_t packet_idx = ( pkt_.dts - av_rescale_q( start_time_, ml_av_time_base_q, stream->time_base ) ) / pkt_.duration;
							int64_t total = ( packet_idx + 1 ) * samples_per_packet_;
							int result = int( total / samples_per_frame_ );
							if ( result > max || max == INT_MAX )
								max = result;
							fallback = false;
						}
						else
						{
							// Make sure we don't try this logic again for this file
							//BT17386: Previously the fps property was set to -1 here to signal that 
							//we shouldn't try again, but this caused problems in input_lazy when aquiring
							//a graph, since the fps for the input and source didn't match.
							continue_trying = false;
							samples_per_packet_ = 0;
							samples_duration_ = 0;
							fallback = true;
						}
					}

					if ( fallback )
					{
						double dts = av_q2d( stream->time_base ) * ( pkt_.dts - av_rescale_q( start_time_, ml_av_time_base_q, stream->time_base ) );
						int result = int( dts * fps( ) + 0.5 ) + 1;
						if ( result > max || max == INT_MAX )
							max = result;
					}
				}

				av_free_packet( &pkt_ );
			}

			frames_ = max > 0 ? max : frames_;
			seek( 0 );

			seek_to_position( );

			return frames_;
		}

		// Returns the current video stream
		AVStream *get_video_stream( )
		{
			if ( prop_video_index_.value< int >( ) >= 0 && video_indexes_.size( ) > 0 )
				return context_->streams[ video_indexes_[ prop_video_index_.value< int >( ) ] ];
			else
				return 0;
		}

		// Returns the current audio stream
		AVStream *get_audio_stream( )
		{
			if ( prop_audio_index_.value< int >( ) >= 0 && audio_indexes_.size( ) > 0 )
				return context_->streams[ audio_indexes_[ prop_audio_index_.value< int >( ) ] ];
			else
				return 0;
		}

		// Opens the video codec associated to the current stream
		void open_video_codec( )
		{
			AVStream *stream = get_video_stream( );
			AVCodecContext *codec_context = stream->codec;
			video_codec_ = avcodec_find_decoder( codec_context->codec_id );
			if ( prop_inner_threads_.value< int >( ) <= 0 )
				codec_context->thread_count = boost::thread::hardware_concurrency( );
			else
				codec_context->thread_count = prop_inner_threads_.value< int >( );
			codec_context->strict_std_compliance = prop_strict_.value< int >( );
			if ( video_codec_ == NULL || avcodec_open2( codec_context, video_codec_, 0 ) < 0 )
				prop_video_index_ = -1;
		}

		void close_video_codec( )
		{
			AVStream *stream = get_video_stream( );
			if ( stream && stream->codec )
				avcodec_close( stream->codec );
		}
		
		// Opens the audio codec associated to the current stream
		void open_audio_codec( )
		{
			AVStream *stream = get_audio_stream( );
			AVCodecContext *codec_context = stream->codec;
			audio_codec_ = avcodec_find_decoder( codec_context->codec_id );
			codec_context->strict_std_compliance = prop_strict_.value< int >( );
			if ( audio_codec_ == NULL || avcodec_open2( codec_context, audio_codec_, 0 ) < 0 )
				prop_audio_index_ = -1;
		}

		void close_audio_codec( )
		{
			AVStream *stream = get_audio_stream( );
			if ( stream && stream->codec )
				avcodec_close( stream->codec );
		}

		// Seek to the requested frame
		bool seek_to_position( )
		{
			if ( is_seekable_ )
			{
				// For compressed audio (mp2, mp3 and aac at least) we need to decode 3 
				// packets before we start getting reliable output from the codec - as a
				// result, we discard the audio obtained from the 3 packets immediately 
				// after a seek.
				if ( discard_audio_after_seek_ )
					discard_audio_packet_count_ = 1;

				int position = get_position( );

				if ( aml_index_ && aml_index_->usable( ) )
				{
					position = aml_index_->key_frame_of( position );

					// Allow us to specify how many gops are relevant - normally this will be 0, 1 or 2
					int gops = prop_gop_open_.value< int >( );
					while( gops -- )
						position = aml_index_->key_frame_of( position - 1 );

					if ( position > 0 )
						expected_packet_ = position;
					else
						expected_packet_ = 0;
				}
				else if ( must_decode_ )
				{
					position -= prop_gop_size_.value< int >( );
				}
				
				if ( position < 0 ) position = 0;

				boost::int64_t offset = int64_t( ( ( double )position / avformat_input::fps( ) ) * AV_TIME_BASE ) + context_->start_time;
				int stream = -1;
				boost::int64_t byte = -1;

				// If we have stream in the container which we can't parse, we should defer to seeking on a specific stream
				// (see populate for identification of unreliable_container_)
				if ( unreliable_container_ )
				{
					if ( has_video( ) )
					{
						offset = get_video_stream( )->start_time + boost::int64_t( ( ( double )position / avformat_input::fps( ) ) / av_q2d( get_video_stream( )->time_base ) );
						stream = video_indexes_[ prop_video_index_.value< int >( ) ];
					}
					else if ( has_audio( ) )
					{
						offset = get_audio_stream( )->start_time + boost::int64_t( ( ( double )position / avformat_input::fps( ) ) / av_q2d( get_audio_stream( )->time_base ) );
						stream = audio_indexes_[ prop_audio_index_.value< int >( ) ];
					}
				}

				// Use the index to get an absolute byte position in the data when available/usable
				if ( aml_index_ && aml_index_->usable( ) )
				{
					byte = aml_index_->find( position );
				}

				if ( must_reopen_ )
					reopen( );

				int result = -1;

				if ( byte != -1 )
					result = av_seek_frame( context_, -1, byte, AVSEEK_FLAG_BYTE );
				else
					result = av_seek_frame( context_, stream, offset, AVSEEK_FLAG_BACKWARD );

				key_search_ = true;
				key_last_ = -1;

				audio_buf_used_ = 0;
				audio_buf_offset_ = 0;

				if ( get_audio_stream( ) && get_audio_stream( )->codec )
					avcodec_flush_buffers( get_audio_stream( )->codec );

				if ( get_video_stream( ) && get_video_stream( )->codec )
					avcodec_flush_buffers( get_video_stream( )->codec );

				return result >= 0;
			}
			else
			{
				return false;
			}
		}

		// Decode the image
		int decode_image( bool &got_picture, AVPacket *packet )
		{
			AVCodecContext *codec_context = get_video_stream( )->codec;

			int ret = 0;
			int got_pict = 0;
			
			// Derive the dts
			double dts = 0;
			if ( packet && uint64_t( packet->dts ) != AV_NOPTS_VALUE )
				dts = av_q2d( get_video_stream( )->time_base ) * ( packet->dts - av_rescale_q( start_time_, ml_av_time_base_q, get_video_stream( )->time_base ) );

			if ( first_video_frame_ )
				first_video_dts_ = dts;

			// Approximate frame position (maybe 0 if dts is present)
			int position = int( dts * fps( ) + 0.5 );

			// Ignore if less than 0
			//if ( position < 0 )
				//return 0;

			// If we have no position, then use the last position + 1
			if ( ( packet == 0 || packet->data == 0 || position == 0 || prop_genpts_.value< int >( ) || !is_seekable( ) ) && images_.size( ) )
				position = images_[ images_.size( ) - 1 ]->position( ) + first_video_found_ + 1;

			// Small optimisation - abandon packet now if we can (ie: we don't have to decode and no image is requested for this frame)
			if ( ( position < get_position( ) && !must_decode_ ) || ( get_process_flags( ) & ml::process_image ) == 0 )
			{
				got_picture = true;
				return ret;
			}

			// If we have just done a search, then we need to locate the first key frame
			// all others are discarded.
			// NOTE: if replaces prop_gen_index_ logic which is now removed (use packet_stream=1 and the awi store instead)
			if ( true )
			{
				il::image_type_ptr image;

				int error = avcodec_decode_video2( codec_context, av_frame_, &got_pict, packet );

				if ( error < 0 || ( !got_pict && packet->data == 0 ) || ( !got_pict && images_.size( ) == 0 ) )
				{
					if ( aml_index_ && aml_index_->usable( ) )
					{
						if ( packet->flags )
							expected_packet_ ++;
					}
					return 0;
				}

				if ( !got_pict )
					return 0;

				if ( key_search_ && av_frame_->key_frame == 0 && position < get_position( ) )
					got_picture = false;
				else
					key_search_ = false;

				if ( error >= 0 && ( key_search_ == false && got_pict ) )
					image = image_convert( );
				else if ( images_.size( ) )
					image = il::image_type_ptr( images_.back( )->clone( ) );

				// mvitc is deprecated - only supported now to remove the signal
				if ( ts_pusher_ && ts_filter_->properties( ).get_property_with_key( ml::keys::enable ).value< int >( ) && image )
				{
					frame_type_ptr temp = frame_type_ptr( new frame_type( ) );
					temp->set_position( position );
					temp->set_image( image );
					ts_pusher_->push( temp );
					temp = ts_filter_->fetch( );
					image = temp->get_image( );
				}

				if ( image )
				{
					// Correct position
					if ( images_.size( ) == 0 && ( aml_index_ && aml_index_->usable( ) ) )
						position = expected_packet_;
					else if ( images_.size( ) && !( aml_index_ && aml_index_->usable( ) ) )
						position = images_[ images_.size( ) - 1 ]->position( ) + first_video_found_ + 1;
				}

				// Calculate the gop size
				if ( av_frame_->key_frame && image )
				{
					if ( key_last_ >= 0 && position - key_last_ > prop_gop_cache_.value< int >( ) )
						prop_gop_cache_ = position - key_last_;
					if ( position >= 0 )
						key_last_ = position;
				}

				// Store the image in the queue
				if ( image && position >= 0 )
				{
					store_image( image, position );
					got_picture = images_.back( )->position( ) >= get_position( );
				}

				expected_packet_ ++;
			}
				
			return ret;
		}

		il::image_type_ptr image_convert( )
		{
			AVStream *stream = get_video_stream( );
			AVCodecContext *codec_context = stream->codec;
			int width = get_width( );
			int height = get_height( );

			return convert_to_oil( scaler_, av_frame_, codec_context->pix_fmt, width, height );
		}

		void store_image( il::image_type_ptr image, int position )
		{
			if ( first_video_frame_ )
			{
				first_video_found_ = position - get_position( );
				first_video_frame_ = false;
			}

			image->set_position( position - first_video_found_ );

			if ( images_.size( ) > 0 )
			{
				int first = images_[ 0 ]->position( );
				int last = images_[ images_.size( ) - 1 ]->position( );
				if ( position < first && position < last )
					images_.clear( );
				else if ( first < get_position( ) - prop_gop_cache_.value< int >( ) || int( images_.size( ) ) > prop_gop_cache_.value< int >( ) )
					if ( ( *images_.begin( ) )->position( ) < get_position( ) )
						images_.erase( images_.begin( ) );
			}

			AVStream *stream = get_video_stream( );
			AVCodecContext *codec_context = stream->codec;

			if ( av_frame_->interlaced_frame )
				image->set_field_order( av_frame_->top_field_first ? il::top_field_first : il::bottom_field_first );
			else if ( codec_context->field_order == AV_FIELD_TT || codec_context->field_order == AV_FIELD_BT )
				image->set_field_order( il::top_field_first );
			else if ( codec_context->field_order == AV_FIELD_BB || codec_context->field_order == AV_FIELD_TB )
				image->set_field_order( il::bottom_field_first );

			if ( image )
			{
				image->set_writable( false );
				images_.push_back( image );
			}
		}

		inline int samples_for_frame( int frequency, int index )
		{
			return ml::audio::samples_for_frame( index, frequency, fps_num_, fps_den_ );
		}

		boost::uint8_t checksum( AVPacket &pkt )
		{
			boost::uint8_t check = 0;
			for ( int i = 0; i < pkt.size; i ++ )
				check ^= pkt.data[ i ];
			return check;
		}

		int decode_audio( bool &got_audio )
		{
			int ret = 0;

			// Get the audio codec context
			AVCodecContext *codec_context = get_audio_stream( )->codec;

			// The number of bytes in the packet
			int len = pkt_.size;

			// This is the pts of the packet
			int found = 0;
			double dts = av_q2d( get_audio_stream( )->time_base ) * ( pkt_.dts - av_rescale_q( start_time_, ml_av_time_base_q, get_audio_stream( )->time_base ) );
			int64_t packet_idx = 0;

			// Sync with video
			if ( has_video( ) && first_audio_frame_ )
				if ( first_video_dts_ == -1.0 || dts < first_video_dts_ ) return 0;

			if( first_audio_dts_ < 0.0 )
			{
				if( dts >= 0.0 )
				{
					first_audio_dts_ = dts;
					dts = 0.0;
				}
			}
			else
			{
				dts -= first_audio_dts_;
			}

			if ( uint64_t( pkt_.dts ) != AV_NOPTS_VALUE )
			{
				if ( pkt_.duration > 0 && samples_duration_ == pkt_.duration && samples_per_frame_ > 0 )
				{
					packet_idx = ( pkt_.dts - av_rescale_q( start_time_, ml_av_time_base_q, get_audio_stream( )->time_base ) ) / pkt_.duration;
					int64_t total = packet_idx * samples_per_packet_;
					found = int( total / samples_per_frame_ );
				}
				else
				{
					found = int( dts * avformat_input::fps( ) );

					//Hack - the AAC decoder gives back the data for the previous frame
					//that it was fed, so we have to subtract one from the dts here
					if( codec_context->codec_id == CODEC_ID_AAC )
					{
						found = int( dts * avformat_input::fps( ) + 0.5 ) - 1;
					}
				}
			}

			got_audio = 0;

			// Ignore packets before 0
			//if ( found < 0 )
				//return 0;

			if ( audio_filter_ == 0 )
			{
				// should we check if it's pcm24 here, and if so, send that as last parameter instead?
				audio_filter_ = new avaudio_convert_to_aml( codec_context->sample_rate, codec_context->sample_rate, codec_context->channels, codec_context->channels, codec_context->sample_fmt, AVSampleFormat_to_aml_id(codec_context->sample_fmt) );
			}

			// Get the audio info from the codec context
			int channels = codec_context->channels;
			int frequency = codec_context->sample_rate;
			int bps = id_to_storage_bytes_per_sample( audio_filter_->get_out_format( ) );
			int skip = 0;
			bool ignore = false;

			if ( audio_buf_used_ == 0 && audio_.size( ) == 0 )
			{
				if ( audio_.size( ) == 0 && aml_index_ && aml_index_->usable( ) )
				{
					int key_position = 0;

					if ( pkt_.pos != -1 )
						key_position = aml_index_->key_frame_from( pkt_.pos );
					else
						key_position = aml_index_->key_frame_from( last_packet_pos_ );

					if ( found < key_position - 100 )
					{
						if ( samples_per_packet_ > 0 )
							packet_idx = int( key_position * ( samples_per_frame_ / samples_per_packet_ ) );
						found = key_position;
						dts = double( found ) / avformat_input::fps( );
					}
				}

				int64_t samples_to_packet = 0;
				int64_t samples_to_frame = 0;

				if ( pkt_.duration && samples_per_packet_ > 0 )
					samples_to_packet = int64_t( samples_per_packet_ * packet_idx );
				else
					samples_to_packet = int64_t( dts * frequency + 0.5 );
				
				samples_to_frame = ml::audio::samples_to_frame( found, frequency, fps_num_, fps_den_ );

				if ( samples_to_packet < samples_to_frame )
				{
					skip = int( samples_to_frame - samples_to_packet ) * channels * bps;
				}
				else if ( samples_to_packet > samples_to_frame )
				{
					audio_buf_used_ = size_t( samples_to_packet - samples_to_frame ) * channels * bps;
					if ( audio_buf_used_ < audio_buf_size_ / 2 && codec_context->codec_id != CODEC_ID_AAC )
					{
						memset( audio_buf_, 0, audio_buf_used_ );
						ignore = found < get_position( );
					}
					else
					{
						audio_buf_used_ = 0;
					}
				}

				audio_buf_offset_ = found;
			}
			else if ( audio_.size( ) != 0 )
			{
				audio_buf_offset_ = ( *( audio_.end( ) - 1 ) )->position( ) + 1;
			}

			// Each packet may need multiple parses
			while( len > 0 )
			{
				avcodec_get_frame_defaults( av_frame_ );
				
				int got_frame = 0;
				ret = avcodec_decode_audio4( codec_context, av_frame_, &got_frame, &pkt_ );

				//ARENFORCE_MSG( ret >= 0, "Failed to decode audio" );
				
				// If we need to discard packets, do that now
				if( discard_audio_packet_count_ )
				{
					discard_audio_packet_count_ --;
					ret = 0;
					got_audio = false;
					audio_buf_used_ = 0;
					if ( !has_video( ) ) expected_packet_ ++;
					break;
				}

				// If no samples are returned, then break now
				if ( !got_frame )
				{
					ret = 0;
					//got_audio = true;
					audio_buf_used_ = 0;
					break;
				}

				audio_type_ptr temp = audio_filter_->convert( ( const boost::uint8_t ** )av_frame_->data, av_frame_->nb_samples );

				// Copy the new samples to the main buffer
				memcpy( audio_buf_ + audio_buf_used_, temp->pointer( ), temp->size( ) );

				if ( getenv( "AML_DTS_LOG" ) ) std::cerr << "bytes = " << pkt_.pts << " " << temp->size( ) << " " << pkt_.duration << " " << int( checksum( pkt_ ) ) << " " << get_audio_stream( )->time_base.num << " " << get_audio_stream( )->time_base.den << std::endl;

				// Decrement the length by the number of bytes parsed
				len -= ret;

				// Increment the number of bytes used in the buffer
				if ( temp->size( ) > 0 )
					audio_buf_used_ += temp->size( );

				// Current index in the buffer
				int index = 0;

				if ( audio_buf_used_ > skip )
				{
					index = skip;
					skip = 0;
				}
				else if ( skip > 0 )
				{
					skip -= audio_buf_used_;
					audio_buf_used_ = 0;
				}

				// Now we need to split the audio up into frame sized dollops
				while( 1 )
				{
					int samples = samples_for_frame( frequency, audio_buf_offset_ );

					int bytes_for_frame = samples * channels * bps;

					if ( bytes_for_frame > ( audio_buf_used_ - index ) )
						break;

					// Create an audio frame
					if ( !ignore )
					{
						index += store_audio( audio_buf_offset_, audio_buf_ + index, samples );
						audio_buf_offset_ = audio_.back( )->position( );

						if ( audio_buf_offset_ ++  >= get_position( ) )
							got_audio = 1;
					}
					else
					{
						index += samples * channels * 2;
					}

				}

				audio_buf_used_ = std::max< int >( 0, audio_buf_used_ - index );	// Warning audio_buf_used_ could become negative here.

				if ( audio_buf_used_ && index != 0 )
					memmove( audio_buf_, audio_buf_ + index, audio_buf_used_ );
			}

			return ret;
		}

		int store_audio( int position, uint8_t *buf, int samples )
		{
			// Get the audio codec context
			AVCodecContext *codec_context = get_audio_stream( )->codec;

			// Get the audio info from the codec context
			int channels = codec_context->channels;
			int frequency = codec_context->sample_rate;

			if ( first_audio_frame_ )
			{
				first_audio_found_ = position - get_position( );
				first_audio_frame_ = false;
			}

			audio_type_ptr aud = ml::audio::allocate( audio_filter_->get_out_format( ), frequency, channels, samples, false );
			aud->set_position( position );
			memcpy( aud->pointer( ), buf, aud->size( ) );

			if ( audio_.size( ) > 0 )
			{
				int first = audio_[ 0 ]->position( );
				if ( first < get_position( ) - prop_gop_cache_.value< int >( ) || int( audio_.size( ) ) > prop_gop_cache_.value< int >( ) )
					if ( ( *audio_.begin( ) )->position( ) < get_position( ) )
						audio_.erase( audio_.begin( ) );
			}

			audio_.push_back( aud );

			return aud->size( );
		}

		void clear_stores( bool force = false )
		{
			bool clear = true;

			if ( !force && images_.size( ) )
			{
				int current = get_position( );
				int first = images_[ 0 ]->position( );
				int last = images_[ images_.size( ) - 1 ]->position( );
				if ( current >= first && current <= last )
					clear = false;
			}

			if ( clear )
			{
				images_.clear( );
				audio_.clear( );
				audio_buf_used_ = 0;
			}
		}

		bool find_image( frame_type_ptr &frame )
		{
			bool exact = false;
			int current = get_position( );
			int closest = 1 << 16;
			std::deque< il::image_type_ptr >::iterator result = images_.end( );
			std::deque< il::image_type_ptr >::iterator iter;

			for ( iter = images_.begin( ); iter != images_.end( ); ++iter )
			{
				il::image_type_ptr img = *iter;
				int diff = current - img->position( );
				if ( std::abs( diff ) <= closest )
				{
					result = iter;
					closest = std::abs( diff );
				}
				else if ( img->position( ) > current )
				{
					break;
				}
			}

			if ( result != images_.end( ) )
			{
				frame->set_image( *result );
				exact = ( *result )->position( ) == current;
			}

			return exact;
		}

		bool find_audio( frame_type_ptr &frame )
		{
			bool exact = false;
			int current = get_position( );
			int closest = 1 << 16;
			std::deque< audio_type_ptr >::iterator result = audio_.end( );
			std::deque< audio_type_ptr >::iterator iter;

			for ( iter = audio_.begin( ); iter != audio_.end( ); ++iter )
			{
				audio_type_ptr aud = *iter;
				int diff = current - aud->position( );
				if ( std::abs( diff ) <= closest )
				{
					result = iter;
					closest = std::abs( diff );
				}
				else if ( aud->position( ) > current )
				{
					break;
				}
			}

			if ( result != audio_.end( ) && ( *result )->position( ) == current )
			{
				ml::audio_type_ptr audio_clone = (*result)->clone();
				frame->set_audio( audio_clone );
				frame->set_duration( double( ( *result )->samples( ) ) / double( ( *result )->frequency( ) ) );
				exact = true;
			}
			else
			{
				AVCodecContext *codec_context = get_audio_stream( )->codec;
				int channels = codec_context->channels;
				int frequency = codec_context->sample_rate;

				if( channels != 0 && frequency != 0 )
				{
					int samples = samples_for_frame( frequency, current );
					audio_type_ptr aud = audio::allocate( audio_filter_->get_out_format( ), frequency, channels, samples, true );
					aud->set_position( current );
					frame->set_audio( aud );
					frame->set_duration( double( samples ) / double( frequency ) );
				}
			}

			return exact;
		}

		void reopen( )
		{	  
			// With a cached item, we want to avoid it going out of scope/expiring so obtain
			// a reference to the existing item now
			AVIOContext *keepalive = 0;
			if ( resource_.find( L"aml:cache:" ) == 0 )
			{
				io::open_file( &keepalive, olib::opencorelib::str_util::to_string( resource_ ).c_str( ), AVIO_FLAG_READ );

				// AML specific reopen hack - enforces a reopen from a non-cached source
				avio_size( context_->pb );
			}

			// Preserve the current position (since we need to seek to start and end here)
			int position = get_position( );

			// Close the existing codec and file objects
			if ( prop_video_index_.value< int >( ) >= 0 )
			{
				close_video_codec( );
				video_indexes_.clear( );
			}
			if ( prop_audio_index_.value< int >( ) >= 0 )
			{
				close_audio_codec( );
				audio_indexes_.clear( );
			}
			if ( context_ )
				avformat_close_input( &context_ );

			// Clear video and audio index vectors
			video_indexes_.erase( video_indexes_.begin( ), video_indexes_.end( ) );
			audio_indexes_.erase( audio_indexes_.begin( ), audio_indexes_.end( ) );

			// Reopen the media
			seek( 0 );
			clear_stores( true );
			initialize( );

			// Close the keep alive
			if ( keepalive )
				io::close_file( keepalive );

			// Restore the position request
			seek( position );
		}

	private:
		std::wstring uri_;
		std::wstring resource_;
		std::wstring mime_type_;
		int frames_;
		bool is_seekable_;
		bool eof_;
		bool first_video_frame_;
		int first_video_found_;
		bool first_audio_frame_;
		int first_audio_found_;

		AVInputFormat *format_;
		pcos::property prop_video_index_;
		pcos::property prop_audio_index_;
		pcos::property prop_gop_size_;
		pcos::property prop_gop_cache_;
		pcos::property prop_format_;
		pcos::property prop_genpts_;
		pcos::property prop_frames_;
		pcos::property prop_file_size_;
		pcos::property prop_estimate_;
		pcos::property prop_fps_num_;
		pcos::property prop_fps_den_;
		pcos::property prop_ts_filter_;
		pcos::property prop_ts_index_;
		pcos::property prop_ts_auto_;
		pcos::property prop_gop_open_;
		pcos::property prop_gen_index_;
		pcos::property prop_packet_stream_;
		pcos::property prop_fake_fps_;
		pcos::property prop_frequency_;
		pcos::property prop_inner_threads_;
		pcos::property prop_strict_;
		pcos::property prop_whitelist_;
		AVFrame *av_frame_;
		AVCodec *video_codec_;
		AVCodec *audio_codec_;

		std::deque < il::image_type_ptr > images_;
		std::deque < audio_type_ptr > audio_;
		bool must_decode_;
		bool must_reopen_;
		bool key_search_;
		int key_last_;

		boost::uint8_t *audio_buf_;
		int audio_buf_size_;
		int audio_buf_used_;
		int audio_buf_offset_;

		struct SwsContext *scaler_;
		ml::frame_type_ptr last_frame_;

		double samples_per_frame_;
		int samples_per_packet_;
		int samples_duration_;

		input_type_ptr ts_pusher_;
		filter_type_ptr ts_filter_;

		ml::indexer_item_ptr indexer_item_;

		boost::system_time next_time_;

		bool unreliable_container_;
		bool check_indexer_;
		bool image_type_;

		double first_video_dts_;
		double first_audio_dts_;
		int discard_audio_packet_count_;
		bool discard_audio_after_seek_;

		avformat_demux demuxer_;
		bool is_streaming_;

		avaudio_convert_to_aml *audio_filter_;
};

input_type_ptr ML_PLUGIN_DECLSPEC create_input_avformat( const std::wstring &resource )
{
	return input_type_ptr( new avformat_input( resource ) );
}

} } }
