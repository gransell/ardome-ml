// ml::audio - Audio block handler

// Copyright (C) 2012 Ardendo
// Released under the LGPL.

#include <openmedialib/ml/audio_block.hpp>
#include <opencorelib/cl/media_time.hpp>
#include <opencorelib/cl/log_defines.hpp>
#include <boost/cstdint.hpp>

namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { namespace audio {

calculator::calculator( int fps_num, int fps_den, int frequency )
: fps_num_( fps_num )
, fps_den_( fps_den )
, frequency_( frequency )
, has_video_( false )
, has_audio_( false )
{
}

// Modifier to set the video frame rate
void calculator::set_fps( int fps_num, int fps_den )
{
	ARLOG_DEBUG3( "Setting frame rate of %1% as %2%:%3%" )( source_)( fps_num )( fps_den );
	fps_num_ = fps_num;
	fps_den_ = fps_den;
}

// Optional modifier to set the frequency of the audio and number of samples per packet 
void calculator::set_frequency( int frequency )
{
	ARLOG_DEBUG3( "Setting frequency of %1% as %2%" )( source_)( frequency );
	frequency_ = frequency;
}

// Set the id for the specified stream - note that this should only be invoked on video or audio streams
void calculator::set_stream_type( size_t index, stream_id id, int num, int den )
{
	ARENFORCE_MSG( id == stream_video || id == stream_audio, "Registering stream %1% of %2% failed - unsupported type" )( index )( source_ );

	ARLOG_DEBUG3( "Registering stream %1% of %2% as %3% at %4%:%5%" )( index )( source_ )( id == stream_video ? "video" : "audio" )( num )( den );

	info_[ index ].id = id;
	info_[ index ].num = num;
	info_[ index ].den = den;

	if ( id == stream_video )
		has_video_ = true;
	else if ( id == stream_audio )
		has_audio_ = true;
}

// Set the time base for the video or audio stream
void calculator::set_stream_offset( size_t index, boost::int64_t offset )
{
	ARENFORCE_MSG( info_.find( index ) != info_.end( ), "Registering stream %1% of %2% with offset %3% failed - previously unregistered stream" )( index )( source_ )( offset );
	ARLOG_DEBUG3( "Registering stream %1% of %2% with offset %3%" )( index )( source_ )( offset );
	info_[ index ].offset = offset;
}

// Retrieve the stream type for the requested stream (unknown will be returned for ignored streams)
stream_id calculator::stream_type( size_t index ) const
{
	stream_id id = stream_unknown;
	const_iterator iter = info_.find( index );
	if ( iter != info_.end( ) )
		id = iter->second.id;
	return id;
}

// Retrieve the time base for the stream
void calculator::stream_time_base( size_t index, int &num, int &den ) const
{
	const_iterator iter = info_.find( index );
	num = den = 0;
	if ( iter != info_.end( ) )
	{
		num = iter->second.num;
		den = iter->second.den;
	}
}

// Retrieve the pts offset of the first packet in the stream
boost::int64_t calculator::stream_offset( size_t index ) const
{
	boost::int64_t offset = 0;
	const_iterator iter = info_.find( index );
	if ( iter != info_.end( ) )
		offset = iter->second.offset;
	return offset;
}

// Calculate the stream position from a frame position (this will be an approximation for audio)
boost::int64_t calculator::frame_to_position( size_t index, int frame ) const
{
	boost::int64_t position = 0;

	switch( stream_type( index ) )
	{
		case stream_video:
			position = frame;
			break;

		case stream_audio:
			position = samples_to_frame( frame );
			break;

		default:
			ARENFORCE_MSG( false, "Attempt to calculate a frame position on an unknown stream %1%" )( index );
			break;
	}

	return position;
}

// Calculate the duration of a packet for a particular stream
int calculator::packet_duration( size_t index, int duration ) const
{
	int result = 0;

	int num, den;
	stream_time_base( index, num, den );

	switch( stream_type( index ) )
	{
		case stream_video:
			// TODO: Ensure that duration does match the timebase
			result = 1;
			break;

		case stream_audio:
			result = ceil( frequency_ * double( num * duration ) / den );
			break;

		default:
			ARENFORCE_MSG( false, "Attempt to calculate a packet duration on an unknown stream %1% at %2%" )( index )( duration );
			break;
	}

	return result;
}

// Calculate the position for each stream
boost::int64_t calculator::dts_to_position( size_t index, boost::int64_t dts ) const
{
	boost::int64_t position = 0;

	int num, den;
	stream_time_base( index, num, den );

	dts -= stream_offset( index );

	switch( stream_type( index ) )
	{
		case stream_video:
			position = int( 0.5 + fps( ) * double( num * dts ) / den );
			break;

		case stream_audio:
			position = ceil( frequency_ * double( num * dts ) / den );
			break;

		default:
			ARENFORCE_MSG( false, "Attempt to calculate a position on an unknown stream %1% at %2%" )( index )( dts );
			break;
	}

	return position;
}

// Calculate the dts for each stream
boost::int64_t calculator::position_to_dts( size_t index, boost::int64_t position ) const
{
	int num, den;
	stream_time_base( index, num, den );

	boost::int64_t dts = stream_offset( index );

	switch( stream_type( index ) )
	{
		case stream_video:
			dts += ceil( double( position * den ) / num / fps( ) );
			break;

		case stream_audio:
			dts += ceil( double( position * den ) / num / frequency_ );
			break;

		default:
			ARENFORCE_MSG( false, "Attempt to calculate a dts on an unknown stream %1% at %2%" )( index )( position );
			break;
	}

	return dts;
}

// Allocate an empty audio block for the position - the demuxer is responsible for populating it
block_type_ptr calculator::calculate( const int position ) const
{
	block_type_ptr block;

	if ( has_audio( ) )
	{
		// Allocate the block structure and initialise it with the known values for this frame
		block = block_type_ptr( new block_type( ) );
		int samples_in_frame = samples_for_frame( position );
		block->position = position;
		block->samples = samples_in_frame;
		block->first = samples_to_frame( position );
		block->last = block->first + block->samples;

		// Create an empty audio track for each stream here
		for ( const_iterator iter = info_.begin( ); iter != info_.end( ); iter ++ )
		{
			const info_type &info = iter->second;
			size_t index = iter->first;
			if ( info.id == stream_audio )
				block->tracks[ index ].index = index;
		}
	}

	return block;
}

} } } }
