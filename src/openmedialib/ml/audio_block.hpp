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
		calculator( int fps_num = 0, int fps_den = 0, int pps_num = 0, int pps_den = 0, int frequency = 48000, int lead_in = 3 );

		void set_fps( int fps_num, int fps_den );

		void set_pps( int pps_num, int pps_den );

		void set_frequency( int frequency, int samples = 0 );

		void set_lead_in( int lead_in );

		int first_packet_for_frame( int position ) const;

		int last_packet_for_frame( int position ) const;

		int first_frame_for_packet( int position ) const;

		int last_frame_for_packet( int position ) const;

		block_type_ptr calculate( const int position ) const;

	private:
		int fps_num_;
		int fps_den_;
		int pps_num_;
		int pps_den_;
		int frequency_;
		int lead_in_;
};

} } } }

#endif
