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

#include <openpluginlib/pl/pcos/isubject.hpp>
#include <openpluginlib/pl/pcos/observer.hpp>
#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/worker.hpp>
#include <opencorelib/cl/log_defines.hpp>

#include <vector>
#include <map>
#include <set>

#include "avformat_stream.hpp"
#include "avformat_wrappers.hpp"

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
#include <libavformat/url.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

namespace cl = olib::opencorelib;
namespace ml = olib::openmedialib::ml;
namespace pl = olib::openpluginlib;
namespace il = olib::openimagelib::il;
namespace pcos = olib::openpluginlib::pcos;

namespace olib { namespace openmedialib { namespace ml {

extern const std::wstring avformat_to_oil( int );
extern const PixelFormat oil_to_avformat( const std::wstring & );
extern il::image_type_ptr convert_to_oil( AVFrame *, PixelFormat, int, int );

// Alternative to Julian's patch?
static const AVRational ml_av_time_base_q = { 1, AV_TIME_BASE };

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
			, start_time_( 0 )
		{
		}

		virtual bool seek_to_position( ) = 0;
		virtual double fps( ) const = 0;
		virtual AVStream *get_video_stream( ) = 0;
		virtual AVStream *get_audio_stream( ) = 0;
		virtual bool is_video_stream( int index ) = 0;
		virtual bool is_audio_stream( int index ) = 0;
};

class stream_cache
{
	private:
		cl::lru_key key_;
		boost::int64_t expected_;
		boost::int64_t packet_;
		int lead_in_;

	public:
		stream_cache( )
		: key_( lru_stream_cache::allocate( ) )
		, expected_( 0 )
		, packet_( 0 )
		, lead_in_( 0 )
		{ }

		void set_packet( boost::int64_t packet )
		{
			packet_ = packet;
		}

		const boost::int64_t packet( ) const
		{
			return packet_;
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

		void put( ml::stream_type_ptr stream, int duration )
		{
			lru_stream_ptr lru = lru_stream_cache::fetch( key_ );
			lru->append( stream->position( ), stream );
			expected_ = stream->position( ) + duration;
		}

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
					to = stream->position( ) + stream->samples( );
				}
			}
			return stream;
		}

		ml::stream_type_ptr fetch( boost::int64_t position )
		{
			lru_stream_ptr lru = lru_stream_cache::fetch( key_ );
			return lru->fetch( position );
		}
};

class avformat_demux
{
	private:
		bool initialised_;
		avformat_source *source;
		std::map< size_t, stream_cache > caches;
		ml::audio::calculator calculator;
		AVPacket pkt_;
		std::set< size_t > unsampled_;

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
				seek( );

				// Keep going until we have the full frame
				while( !populate( result ) )
				{
					if ( !obtain_next_packet( ) )
						break;
				}

				// Increment the next expected frame
				source->expected_ = position + 1;
			}

			// Set the source timecode
			pl::pcos::assign< int >( result->properties( ), ml::keys::source_timecode, position );
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
				calculator.set_source( pl::to_string( source->get_uri( ) ) );
				calculator.set_fps( source->fps_num_, source->fps_den_ );
				calculator.set_frequency( frequency );

				// Add the video and all relevant audio streams here
				for( size_t i = 0; i < source->context_->nb_streams; i++ )
				{
					AVStream *stream = source->context_->streams[ i ];
   					switch( stream->codec->codec_type ) 
					{
						case AVMEDIA_TYPE_VIDEO:
							if ( stream == video )
							{
								// Inform the calculator of the video related information
								calculator.set_stream_type( i, stream_video, stream->time_base.num, stream->time_base.den );

								// Create and initialise the cache
								stream_cache &handler = caches[ i ];

								// Indicate that we have not found a packet for this stream yet
								unsampled_.insert( i );
							}
							break;

						case AVMEDIA_TYPE_AUDIO:
							if ( audio && stream->codec->sample_rate == frequency )
							{
								// Inform the calculator of the audio related information for this stream
								calculator.set_stream_type( i, stream_audio, stream->time_base.num, stream->time_base.den );

								// Create and initialise the cache
								stream_cache &handler = caches[ i ];
								handler.set_lead_in( lead_in( stream ) );

								// Indicate that we have not found a packet for this stream yet
								unsampled_.insert( i );
							}
		   					break;

						default:
		   					break;
					}
				}

				if ( getenv( "AML_DTS_LOG" ) ) std::cerr << "Started analysing first packets" << std::endl;

				while ( unsampled_.size( ) )
				{
					if ( !obtain_next_packet( ) )
					{
						if ( getenv( "AML_DTS_LOG" ) ) std::cerr << "End of stream before all first packets found" << std::endl;
						break;
					}
				}

				if ( getenv( "AML_DTS_LOG" ) ) std::cerr << "Finished analysing first packets" << std::endl;

				initialised_ = true;
			}
		}

		ml::stream_type_ptr obtain_next_packet( )
		{
			size_t index = 0;
			ml::stream_type_ptr packet;

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

				if ( unsampled_.find( index ) != unsampled_.end( ) )
				{
					if ( getenv( "AML_DTS_LOG" ) ) std::cerr << "encountered stream " << index << " for the first time starting at " << pkt_.dts << std::endl;
					calculator.set_stream_offset( index, pkt_.dts );
					unsampled_.erase( index );
				}

				if ( pkt_.pos != -1 )
					source->last_packet_pos_ = pkt_.pos;

				if ( error >= 0 && source->is_video_stream( pkt_.stream_index ) )
					got_packet = ml::stream_video;
				else if ( error >= 0 && has_cache_for( index ) )
					got_packet = ml::stream_audio;
				else if ( error >= 0 )
					av_free_packet( &pkt_ );
				if ( error >= 0 && source->key_last_ == -1 && got_packet == ml::stream_video && !pkt_.flags )
					got_packet = ml::stream_unknown;
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

				// Temporary diagnostics
				if ( getenv( "AML_DTS_LOG" ) ) std::cerr << "pkt: " << index << " " << pkt_.dts << " + " << pkt_.duration << " = " << position << " + " << duration << std::endl;

				// Temporary test to ensure that we can map back from the position to the dts
				boost::int64_t test = calculator.position_to_dts( index, position );
				if ( test != pkt_.dts ) if ( getenv( "AML_DTS_LOG" ) ) std::cerr << "Mismatch " << index << ": " << position << " " << pkt_.dts << " != " << test << std::endl;

				// Sync the expected position with the found one for this stream
				found( pkt_, position );

				// Sync the last key found with the calculated position
				if ( got_packet == ml::stream_video && pkt_.flags == AV_PKT_FLAG_KEY )
					source->key_last_ = position;

				// Create the stream_type_ptr for the packet
				switch( got_packet )
				{
					case ml::stream_video:
						packet = ml::stream_type_ptr( new stream_avformat( stream->codec->codec_id, pkt_.size, position, source->key_last_, codec->bit_rate, 
																		   ml::dimensions( source->width_, source->height_ ), ml::fraction( source->sar_num_, source->sar_den_ ), 
																		   avformat_to_oil( codec->pix_fmt ), il::top_field_first, codec->gop_size ) );
						break;

					case ml::stream_audio:
						packet = ml::stream_type_ptr( new stream_avformat( stream->codec->codec_id, pkt_.size, position, position,
																		   0, codec->sample_rate, codec->channels, duration, av_get_bytes_per_sample(codec->sample_fmt), 
																		   L"", il::top_field_first, 0 ) );
						break;

					default:
						// Not supported
						break;
				}

				// Populate the stream_type_ptr
				if ( packet )
				{
					// Extract the properties object now
					pl::pcos::property_container properties = packet->properties( );

					// Copy the data from the avpacket to the stream_type_ptr
					memcpy( packet->bytes( ), pkt_.data, pkt_.size );

					// Input related properties
					pl::pcos::assign< pl::wstring >( properties,  ml::keys::uri, source->get_uri( ) );

					// Packet related properties
					pl::pcos::assign< boost::int64_t >( properties, ml::keys::pts, pkt_.pts );
					pl::pcos::assign< boost::int64_t >( properties, ml::keys::dts, pkt_.dts );
					pl::pcos::assign< int >( properties, ml::keys::duration, pkt_.duration );

					// Absolute byte offset derived from packet and location in stream before a read
					pl::pcos::assign< boost::int64_t >( properties, ml::keys::source_byte_offset, source->last_packet_pos_ );

					// Codec related properties
					pl::pcos::assign< int >( properties, ml::keys::has_b_frames, stream->codec->has_b_frames );

					// Stream related properties
					pl::pcos::assign< int >( properties, ml::keys::timebase_num, stream->time_base.num );
					pl::pcos::assign< int >( properties, ml::keys::timebase_den, stream->time_base.den );

					pl::pcos::property codec_id( ml::keys::codec_id );
					packet->properties( ).append( codec_id = int( codec->codec_id ) );		
					pl::pcos::property codec_type( ml::keys::codec_type );
					packet->properties( ).append( codec_type = int( codec->codec_type ) );
					pl::pcos::property codec_tag( ml::keys::codec_tag );
					packet->properties( ).append( codec_tag = boost::int64_t( codec->codec_tag ) );
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

					cache( index ).put( packet, duration );
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
					lead_in = 1 * 1024;
					break;

				case CODEC_ID_AC3:
					// If we're reading AC3 we have 1536 samples per packet = 31.25 fps at 48 KHz.
					lead_in = 3 * 1536;
					break;

				case CODEC_ID_MP2:
				case CODEC_ID_MP3:
					// If we're reading MP2 or MP3 we have 1152 samples per packet is 125/3 fps at 48 KHz.
					lead_in = 3 * 1152;
					break;

				default:
					break;
			}

			return lead_in;
		}

		void set_expected( int position )
		{
			source->expected_ = position;

			for( size_t i = 0; i < source->context_->nb_streams; i++ )
			{
				if ( has_cache_for( i ) )
				{
					stream_cache &handler = caches[ i ];
					boost::int64_t stream_position = calculator.frame_to_position( i, position );
					handler.set_expected( stream_position );
					handler.set_packet( stream_position );
				}
			}
		}

		void seek( )
		{
			int position = source->get_position( );
			if ( source->expected_ != position )
			{
				source->seek_to_position( );
				if ( source->aml_index_ && source->aml_index_->usable( ) )
					set_expected( source->expected_packet_ );
				else
					source->expected_ = -1;
			}
		}

		void found( AVPacket &pkt, boost::int64_t position )
		{
			// TODO: Handle unindexed positioning...
			if ( has_cache_for( pkt.stream_index ) )
			{
				stream_cache &handler = caches[ pkt.stream_index ];
				if ( position != handler.expected( ) )
				{
					if ( getenv( "AML_DTS_LOG" ) ) std::cerr << "this packet is " << pkt_.stream_index << " " << position << " but we think it's " << handler.expected( ) << std::endl;
					handler.set_expected( position );
				}
			}
		}

		bool populate( ml::frame_type_ptr result )
		{
			// TODO: CORRECTLY DETERMINE AVAILABILITY OF ALL AUDIO PACKETS
			// TODO: Handle non-0 based video

			bool success = true;
			int position = result->get_position( );

			// Try to obtain the video stream component from the cache
			if ( calculator.has_video( ) )
			{
				// Update the output frame
				ml::stream_type_ptr packet = caches[ 0 ].fetch( position );
				result->set_stream( packet );
				success = packet != ml::stream_type_ptr( );
			}

			// Try to obtain the audio stream components from the cache
			if ( success && calculator.has_audio( ) )
			{
				// Generate the unpopulated audio blocks
				ml::audio::block_type_ptr block = calculator.calculate( position );

				// Iterate through the tracks in the audio block
				for ( ml::audio::block_type::iterator iter = block->tracks.begin( ); iter != block->tracks.end( ); iter ++ )
				{
					// Obtain the stream index and block track object
					size_t index = iter->first;
					ml::audio::track_type &track = iter->second;

					// Obtain the cache object for this stream
					stream_cache &cache = caches[ index ];

					// Find the first audio stream component for this block
					ml::stream_type_ptr audio = cache.find_audio( block->first );
					success = audio != ml::stream_type_ptr( );

					// If we have the first, continue to see if we can get the rest
					if ( success )
					{
						block->tracks[ index ].discard = cache.lead_in( );

						for( boost::int64_t offset = audio->position( ); offset < block->last + cache.lead_in( ); )
						{
							audio = cache.fetch( offset );
							success = audio != ml::stream_type_ptr( );
							if ( !success ) break;
							block->tracks[ index ].packets[ offset ] = audio;
							offset += audio->samples( );

							// Calculate the discard if the first sample we want for this frame falls in this stream component
							if ( block->first >= audio->position( ) && block->first < offset )
								block->tracks[ index ].discard += block->first - audio->position( );
						}
					}
				}

				// Set the derived block on the result frame
				result->set_audio_block( block );
			}

			return success;
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
		avformat_input( pl::wstring resource, const pl::wstring mime_type = L"" ) 
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
			, av_frame_( 0 )
			, video_codec_( 0 )
			, audio_codec_( 0 )
			, images_( )
			, audio_( )
			, must_decode_( true )
			, must_reopen_( false )
			, key_search_( false )
			, audio_buf_used_( 0 )
			, audio_buf_offset_( 0 )
			, img_convert_( 0 )
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
		{
			audio_buf_size_ = (AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2;
			audio_buf_ = ( uint8_t * )av_malloc( 2 * audio_buf_size_ );

			// Allow property control of video and audio index
			// NB: Should also have read only props for stream counts
			properties( ).append( prop_video_index_ = 0 );
			properties( ).append( prop_audio_index_ = 0 );
			properties( ).append( prop_gop_size_ = -1 );
			properties( ).append( prop_gop_cache_ = 25 );
			properties( ).append( prop_format_ = pl::wstring( L"" ) );
			properties( ).append( prop_genpts_ = 0 );
			properties( ).append( prop_frames_ = -1 );
			properties( ).append( prop_file_size_ = boost::int64_t( 0 ) );
			properties( ).append( prop_estimate_ = 0 );
			properties( ).append( prop_fps_num_ = -1 );
			properties( ).append( prop_fps_den_ = -1 );
			properties( ).append( prop_ts_filter_ = pl::wstring( L"" ) );
			properties( ).append( prop_ts_index_ = pl::wstring( L"" ) );
			properties( ).append( prop_ts_auto_ = 0 );
			properties( ).append( prop_gop_open_ = 0 );
			properties( ).append( prop_gen_index_ = 0 );
			properties( ).append( prop_packet_stream_ = 0 );
			properties( ).append( prop_fake_fps_ = 0 );

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
				avformat_close_input( &context_ );
			av_free( av_frame_ );
			av_free( audio_buf_ );

			if( indexer_item_ )
			{
				ml::indexer_cancel_request( indexer_item_ );
			}
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
		virtual const pl::wstring get_uri( ) const { return uri_; }
		virtual const pl::wstring get_mime_type( ) const { return mime_type_; }
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
		virtual int get_audio_channels_in_stream( int stream_index ) const
		{
			ARENFORCE_MSG( stream_index >= 0 && stream_index < get_audio_streams(), "Invalid audio stream index: %1%" )
				( stream_index )( get_audio_streams() );

			ARENFORCE_MSG( context_, "Codec context not initialized (did you forget to initialize the input?)" );

			int effective_index = audio_indexes_[stream_index];

			ARENFORCE( context_->streams );
			ARENFORCE( context_->streams[ effective_index ] );
			ARENFORCE( context_->streams[ effective_index ]->codec );
			
			return context_->streams[ effective_index ]->codec->channels;
		}
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
			pl::wstring resource = uri_;

			// A mechanism to ensure that avformat can always be accessed
			if ( resource.find( L"avformat:" ) == 0 )
				resource = resource.substr( 9 );

			// Convenience expansion for *nix based people
			if ( resource.find( L"~" ) == 0 )
				resource = pl::to_wstring( getenv( "HOME" ) ) + resource.substr( 1 );
			if ( prop_ts_index_.value< pl::wstring >( ).find( L"~" ) == 0 )
				resource = pl::to_wstring( getenv( "HOME" ) ) + prop_ts_index_.value< pl::wstring >( ).substr( 1 );

			// Handle the auto generation of the ts_index url when requested
			if ( prop_ts_auto_.value< int >( ) && prop_ts_index_.value< pl::wstring >( ) == L"" )
				prop_ts_index_ = pl::wstring( resource + L".awi" );

			// Ugly - looking to see if a dv1394 device has been specified
			if ( resource.find( L"/dev/" ) == 0 && resource.find( L"1394" ) != pl::wstring::npos )
			{
				prop_format_ = pl::wstring( L"dv" );
			}

			// Allow dv on stdin
			if ( resource == L"dv:-" )
			{
				prop_format_ = pl::wstring( L"dv" );
				resource = L"pipe:";
				is_seekable_ = false;
			}

			// Allow mpeg on stdin
			if ( resource == L"mpeg:-" )
			{
				prop_format_ = pl::wstring( L"mpeg" );
				resource = L"pipe:";
				is_seekable_ = false;
				key_search_ = true;
			}

			// Corrections for file formats
			if ( resource.find( L".mpg" ) == resource.length( ) - 4 ||
				 resource.find( L".mpeg" ) == resource.length( ) - 5 )
			{
				prop_format_ = pl::wstring( L"mpeg" );
				key_search_ = true;
			}
			else if ( resource.find( L".dv" ) == resource.length( ) - 3 )
				prop_format_ = pl::wstring( L"dv" );
			else if ( resource.find( L".mp2" ) == resource.length( ) - 4 )
				prop_format_ = pl::wstring( L"mpeg" );

			// Obtain format
			if ( prop_format_.value< pl::wstring >( ) != L"" )
				format_ = av_find_input_format( pl::to_string( prop_format_.value< pl::wstring >( ) ).c_str( ) );

			// Since we may need to reopen the file, we'll store the modified version now
			resource_ = resource;

			// Attempt to open the resource
			int error = avformat_open_input( &context_, pl::to_string( resource ).c_str( ), format_, 0 ) < 0;

            if (error == 0 && context_->pb && context_->pb->seekable) 
			{
				boost::uint16_t index_entry_type = prop_video_index_.value< int >( ) == -1 ? 2 : 1;
				if ( prop_ts_index_.value< pl::wstring >( ) != L"" )
					indexer_item_ = ml::indexer_request( prop_ts_index_.value< pl::wstring >( ), index_entry_type );
				else
					indexer_item_ = ml::indexer_request( resource, index_entry_type );

				if ( indexer_item_ && indexer_item_->index( ) )
					aml_index_ = indexer_item_->index( );
				else if ( prop_ts_index_.value< pl::wstring >( ) != L"" && prop_ts_auto_.value< int >( ) != -1 )
					return false;
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
				error = avformat_find_stream_info( context_, 0 ) < 0;
				if ( !error && prop_format_.value< pl::wstring >( ) != L"" && uint64_t( context_->duration ) == AV_NOPTS_VALUE )
					error = true;
			}
			
			// Just in case the requested/derived file format is incorrect, on a failure, try again with no format
			if ( error && format_ )
			{
				format_ = 0;
				if ( context_ )
					avformat_close_input( &context_ );
				error = avformat_open_input( &context_, pl::to_string( resource ).c_str( ), format_, 0  ) < 0;
				if ( !error )
					error = avformat_find_stream_info( context_, 0 ) < 0;
			}

			// Populate the input properties
			if ( error == 0 )
				populate( );

			if ( error == 0 )
				image_type_ = std::string( context_->iformat->name ) == "image2" ||
							  std::string( context_->iformat->name ) == "yuv4mpegpipe";

			if ( prop_packet_stream_.value< int >( ) == 0 )
			{
				// Check for and create the timestamp correction filter graph
				if ( prop_ts_filter_.value< pl::wstring >( ) != L"" )
				{
					ts_filter_ = create_filter( prop_ts_filter_.value< pl::wstring >( ) );
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

        void do_fetch( frame_type_ptr &result )
        {
            ARENFORCE_MSG( context_, "Invalid media" );

			// Just in case
			if ( get_position( ) >= get_frames( ) )
				sync( );

            // Route to either decoded or packet extraction mechanisms
            if ( prop_packet_stream_.value< int >( ) )
                demuxer_.do_packet_fetch( result );
            else
                do_decode_fetch( result );
            
            // Add the source_timecode prop
            pcos::property tc_prop( ml::keys::source_timecode );
            tc_prop = result->get_position( );
            result->properties().append( tc_prop );
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

        		av_init_packet( &pkt_ );
        		pkt_.data = NULL;
        		pkt_.size = 0;

				int ret = decode_image( temp, &pkt_ );
				if ( !temp )
                {
                    error =-1;
                    ARLOG_WARN( "Failed to decode null packet at eof. Error = %1%" )( ret );
                } 

				if ( frames_ == 1 << 30 && images_.size( ) )
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
		void sync_with_index( )
		{
			if ( aml_index_ )
			{
				if ( aml_index_->total_frames( ) > frames_ )
				{
					av_read_pause( context_ );
					prop_file_size_ = boost::int64_t( avio_size( context_->pb ) );
					int new_frames = aml_index_->calculate( prop_file_size_.value< boost::int64_t >( ) );
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
			if ( uint64_t( context_->duration ) != AV_NOPTS_VALUE )
				frames_ = int( ( avformat_input::fps( ) * ( context_->duration - start_time_ ) ) / ( double )AV_TIME_BASE );
			else if ( std::string( context_->iformat->name ) == "yuv4mpegpipe" )
				frames_ = 1;
			else
				frames_ = 1 << 30;

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
				else
					result = format != "wav";
			}

			return result;
		}

		int size_media_by_images( )
		{
			int last = 0;
			seek( frames_ - 1 );
			fetch( );

			do
			{
				if ( images_.size( ) )
				{
					last = images_[ images_.size( ) - 1 ]->position( );
					frames_ = last + 3;
					seek( last + 2 );
				}
				else
				{
					frames_ -= 25;
					seek( frames_ - 1 );
				}

				fetch( );
			}
			while( frames_ > 0 && ( images_.size( ) == 0 || last != images_[ images_.size( ) - 1 ]->position( ) ) );

			if ( frames_ <= 0 )
				last = -1;

			return last + 1;
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
							ARENFORCE_MSG( length >= 0, "Failed to decode audio while sizing" );

		   					if ( got_frame )
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
							if ( result > max )
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
						if ( result > max )
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
			codec_context->thread_count = std::max< int >( 1, boost::thread::hardware_concurrency( ) / 2 );
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
					discard_audio_packet_count_ = 3;

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

				boost::int64_t offset = int64_t( ( ( double )position / avformat_input::fps( ) ) * AV_TIME_BASE );
				int stream = -1;
				boost::int64_t byte = -1;

				// If we have stream in the container which we can't parse, we should defer to seeking on a specific stream
				// (see populate for identification of unreliable_container_)
				if ( unreliable_container_ )
				{
					if ( has_video( ) )
					{
						offset = boost::int64_t( ( ( double )position / avformat_input::fps( ) ) / av_q2d( get_video_stream( )->time_base ) );
						stream = video_indexes_[ prop_video_index_.value< int >( ) ];
					}
					else if ( has_audio( ) )
					{
						offset = boost::int64_t( ( ( double )position / avformat_input::fps( ) ) / av_q2d( get_audio_stream( )->time_base ) );
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

				if ( get_audio_stream( ) )
					avcodec_flush_buffers( get_audio_stream( )->codec );

				if ( get_video_stream( ) )
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
			if ( position < 0 )
				return 0;

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
					else if ( images_.size( ) )
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

			return convert_to_oil( av_frame_, codec_context->pix_fmt, width, height );
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
				else if ( first < get_position( ) - prop_gop_cache_.value< int >( ) )
					images_.erase( images_.begin( ) );
			}

			if ( av_frame_->interlaced_frame )
				image->set_field_order( av_frame_->top_field_first ? il::top_field_first : il::bottom_field_first );

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
			if ( found < 0 )
				return 0;

			// Get the audio info from the codec context
			int channels = codec_context->channels;
			int frequency = codec_context->sample_rate;
			int bps = 2;
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

				ARENFORCE_MSG( ret >= 0, "Failed to decode audio" );
				
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

				int audio_size = av_samples_get_buffer_size( NULL, codec_context->channels,
														     av_frame_->nb_samples, codec_context->sample_fmt, 1);
				// Copy the new samples to the main buffer
				memcpy( audio_buf_ + audio_buf_used_, av_frame_->data[ 0 ], audio_size );

				if ( getenv( "AML_DTS_LOG" ) ) std::cerr << "bytes = " << pkt_.pts << " " << audio_size << " " << pkt_.duration << " " << int( checksum( pkt_ ) ) << " " << get_audio_stream( )->time_base.num << " " << get_audio_stream( )->time_base.den << std::endl;

				// Decrement the length by the number of bytes parsed
				len -= ret;

				// Increment the number of bytes used in the buffer
				if ( audio_size > 0 )
					audio_buf_used_ += audio_size;

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

			audio::pcm16_ptr aud = audio::pcm16_ptr( new audio::pcm16( frequency, channels, samples ) );
			aud->set_position( position );
			memcpy( aud->pointer( ), buf, aud->size( ) );

			if ( audio_.size( ) > 0 )
			{
				int first = audio_[ 0 ]->position( );
				if ( first < get_position( ) - prop_gop_cache_.value< int >( ) )
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
					audio::pcm16_ptr aud = audio::pcm16_ptr( new audio::pcm16( frequency, channels, samples ) );
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
			URLContext *keepalive = 0;
			if ( resource_.find( L"aml:cache:" ) == 0 )
			{
				ffurl_open( &keepalive, pl::to_string( resource_ ).c_str( ), AVIO_FLAG_READ, 0, 0 );

				// AML specific reopen hack - enforces a reopen from a non-cached source
				av_read_pause( context_ );
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
				ffurl_close( keepalive );

			// Restore the position request
			seek( position );
		}

	private:
		pl::wstring uri_;
		pl::wstring resource_;
		pl::wstring mime_type_;
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

		struct SwsContext *img_convert_;
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
};

input_type_ptr ML_PLUGIN_DECLSPEC create_input_avformat( const pl::wstring &resource )
{
	return input_type_ptr( new avformat_input( resource ) );
}

} } }
