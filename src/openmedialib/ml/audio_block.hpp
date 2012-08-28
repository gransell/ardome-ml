// ml::audio - Audio block handler

// Copyright (C) 2012 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_CALCULATOR_H_
#define AML_AUDIO_CALCULATOR_H_

#include <openmedialib/ml/audio_types.hpp>
#include <openmedialib/ml/audio_utilities.hpp>
#include <openmedialib/ml/stream.hpp>
#include <boost/shared_ptr.hpp>
#include <map>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

/// The track_type object always contains the audio stream objects which are 
/// necessary to decode the audio for any frame in isolation. 
///
/// As such each track provides a consecutive sequence of packets of the form:
///
/// lead_in  ->   first -> last
///               |        |
/// [p0...][p1][p2....][p3....]
///
/// where the samples between first and last are relevant to the frame and are
/// provided by the block which the track belongs to (since all tracks are 
/// normalised to the same sample offsets). 
///
/// NB: This approach assumes that all tracks in a block have the same sample
/// frequency, and the creator of the block must ensure that this is the case.
///
/// The track associated to the next frame look like:
///
/// lead_in  ->     first -> last
///                 |        |
/// [p1][p2....][p3....][p4...]
///
/// To decode the track after a seek, we need to decode all packets and discard
/// the number of samples specified. To decode in sequence, we need to hold any 
/// samples from the last decoded packet and decode any new packets which are 
/// provided in the subsequent track.

typedef ML_DECLSPEC struct track
{
	typedef std::map< boost::int64_t, stream_type_ptr > map;
	typedef map::iterator iterator;
	typedef map::const_iterator const_iterator;

	track( )
	: index( 0 )
	, discard( 0 )
	{ }

	// Index of the stream
	size_t index;

	// Number of samples to discard after a seek from the first packet in the map
	int discard;

	// Provides the audio packets associated to the block
	map packets;
}
track_type;

/// The block_type object carries a collection of tracks and normalises all time 
/// information to sample offsets

typedef ML_DECLSPEC struct block
{
	typedef std::map< size_t, track_type > map;
	typedef map::iterator iterator;
	typedef map::const_iterator const_iterator;

	block( )
	: position( 0 )
	, samples( 0 )
	, first( 0 )
	, last( 0 )
	{ }
	
	// Position of requested frame
	int position;

	// Number of samples in frame
	int samples;

	// First sample offset to provide
	boost::int64_t first;

	// Last sample offset to provide
	boost::int64_t last;

	// Holds the track objects for this block
	map tracks;
}
block_type;

/// Holds the native timebase information for each registered stream in the 
/// calculator.

typedef ML_DECLSPEC struct info
{
	info( )
	: id( stream_unknown )
	, num( 0 )
	, den( 0 )
	, offset( 0 )
	{ }

	// Type of stream - either video, audio or unknown
	stream_id id;

	// Timebase of the stream
	int num, den;

	// Offset of the start of the stream in the native timebase
	boost::int64_t offset;
}
info_type;

/// The calculator itself holds information about each relevant stream and 
/// and provides the means to convert the native dts to the stream specific
/// position.
///
/// For video, this is a 0 based position offset and for audio it's a zero
/// based sample offset.

class ML_DECLSPEC calculator
{
	public:
		// Constructor for the calculator
		calculator( int fps_num = 0, int fps_den = 0, int frequency = 48000 );

		// Modifier to set the source (for logging purposes)
		void set_source( const std::string &source ) { source_ = source; }

		// Modifier to set the video frame rate
		void set_fps( int fps_num, int fps_den );

		// Optional modifier to set the frequency of the audio and number of samples per packet 
		void set_frequency( int frequency );

		// Set the id and timebase for the specified stream - note that this should only be invoked on video or audio streams
		void set_stream_type( size_t index, stream_id id, int num, int den );

		// Set the time base for the video or audio stream - the type and timebase must be registered before
		void set_stream_offset( size_t index, boost::int64_t offset );

		// Retrieve the stream type for the requested stream (unknown will be returned for ignored streams)
		stream_id stream_type( size_t index ) const;

		// Retrieve the time base for the stream
		void stream_time_base( size_t index, int &num, int &den ) const;

		// Retrieve the dts offset of the first packet in the stream
		boost::int64_t stream_offset( size_t index ) const;

		// Calculate the stream position from a frame position (this will be an approximation for audio)
		boost::int64_t frame_to_position( size_t index, int frame ) const;

		// Calculate the duration of a packet for a particular stream
		int packet_duration( size_t index, int duration ) const;

		// Calculate the position for each stream
		boost::int64_t dts_to_position( size_t index, boost::int64_t dts ) const;

		// Calculate the dts for each stream
		boost::int64_t position_to_dts( size_t index, boost::int64_t position ) const;

		// Allocate an empty audio block for the position - the demuxer is responsible for populating it
		block_type_ptr calculate( const int frame ) const;

		// Convenience function to return the fps as a double
		inline double fps( ) const { return double( fps_num_ ) / fps_den_; }

		// Indicates if a video stream has been registered
		inline bool has_video( ) const { return has_video_; }

		// Indicates if 1 or more audio streams have been registered
		inline bool has_audio( ) const { return has_audio_; }

		// Convenience function - calculate number of audio samples leading up to the frame position
		inline boost::int64_t samples_to_frame( int frame ) const { return audio::samples_to_frame( frame, frequency_, fps_num_, fps_den_ ); }

		// Convenience function - calculate number of audio samples for the frame
		inline boost::int64_t samples_for_frame( int frame ) const { return audio::samples_for_frame( frame, frequency_, fps_num_, fps_den_ ); }

	private:
		// Convenience types
		typedef std::map< size_t, info_type > map;
		typedef map::iterator iterator;
		typedef map::const_iterator const_iterator;

		std::string source_;
		map info_;
		int fps_num_;
		int fps_den_;
		int frequency_;
		bool has_video_;
		bool has_audio_;
};

inline double map( int position, int fps_in_num, int fps_in_den, int fps_out_num, int fps_out_den )
{
	double t1 = boost::int64_t( fps_out_num ) * fps_in_den;
	double t2 = boost::int64_t( fps_out_den ) * fps_in_num;
	return position * ( t2 / t1 );
}

inline int map_first( int position, int fps_in_num, int fps_in_den, int fps_out_num, int fps_out_den )
{
	return int( map( position, fps_in_num, fps_in_den, fps_out_num, fps_out_den ) );
}

inline int map_last( int position, int fps_in_num, int fps_in_den, int fps_out_num, int fps_out_den )
{
	return ceil( map( position + 1, fps_in_num, fps_in_den, fps_out_num, fps_out_den ) ) - 1;
}

} } } }

#endif
