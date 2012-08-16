// ml::audio - Audio block handler

// Copyright (C) 2012 Ardendo
// Released under the LGPL.

#include <openmedialib/ml/audio_types.hpp>
#include <openmedialib/ml/stream.hpp>
#include <boost/shared_ptr.hpp>
#include <map>

#ifndef AML_AUDIO_CALCULATOR_H_
#define AML_AUDIO_CALCULATOR_H_

namespace olib { namespace openmedialib { namespace ml { namespace audio {

typedef ML_DECLSPEC struct block
{
	typedef std::map< int, ml::stream_type_ptr > channel_map;
	typedef std::map< int, channel_map > track_map;
	
	// Position of requested frame
	int position;

	// Number of samples in frame
	int samples;

	// First packet to provide
	int first;

	// Number of contiguous packets to provide
	int count;

	// Number of samples to discard after seeking
	int discard;

	track_map tracks;
}
block_type;

typedef ML_DECLSPEC struct blockv2
{
	typedef std::map< boost::int64_t, ml::stream_type_ptr > channel_map;
	typedef std::map< int, channel_map > track_map;
	
	// Position of requested frame
	int position;

	// Number of samples in frame
	int samples;

	// First pts to provide
	boost::int64_t first;

	// Last pts to provide
	boost::int64_t last;

	track_map tracks;
}
block_typev2;


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

class ML_DECLSPEC calculator
{
	public:
		// Constructor for the calculator
		calculator( int fps_num = 0, int fps_den = 0, int pps_num = 0, int pps_den = 0, int frequency = 48000, int lead_in = 3 );

		// Modifier to set the video frame rate
		void set_fps( int fps_num, int fps_den );

		// Optional modifier to set the packets per second for audio
		void set_pps( int pps_num, int pps_den );

		// Optional modifier to set the frequency of the audio and number of samples per packet 
		void set_frequency( int frequency, int samples = 0 );

		// Set the number of audio packets to decode and ignore after a seek
		void set_lead_in( int lead_in );

		int first_packet_for_frame( int position ) const;

		int last_packet_for_frame( int position ) const;

		int first_frame_for_packet( int position ) const;

		int last_frame_for_packet( int position ) const;

		block_type_ptr calculate( const int position ) const;

		double fps( ) const { return double( fps_num_ ) / fps_den_; }

		double pps( ) const { return double( pps_num_ ) / pps_den_; }

	private:
		int fps_num_;
		int fps_den_;
		int pps_num_;
		int pps_den_;
		int frequency_;
		int lead_in_;
};

#if 0
class ML_DECLSPEC calculatorv2
{
	public:
		// Constructor for the calculator
		calculatorv2( int fps_num = 0, int fps_den = 0, int frequency = 48000, int lead_in = 3 );

		// Modifier to set the video frame rate
		void set_fps( int fps_num, int fps_den );

		// Optional modifier to set the frequency of the audio and number of samples per packet 
		void set_frequency( int frequency );

		// Set the number of audio packets to decode and ignore after a seek
		void set_lead_in( int lead_in );

		boost::int64_t first_pts_for_frame( boost::int64_t position ) const;

		boost::int64_t last_pts_for_frame( boost::int64_t position ) const;

		boost::int64_t first_frame_for_pts( boost::int64_t position ) const;

		boost::int64_t last_frame_for_pts( boost::int64_t position ) const;

		block_type_ptr calculate( const int position ) const;

		double fps( ) const { return double( fps_num_ ) / fps_den_; }

	private:
		int fps_num_;
		int fps_den_;
		int frequency_;
		int lead_in_;
};
#endif

} } } }

#endif
