// ml::audio - Audio block handler

// Copyright (C) 2012 Ardendo
// Released under the LGPL.

#include <openmedialib/ml/audio_block.hpp>
#include <openmedialib/ml/audio_utilities.hpp>
#include <opencorelib/cl/media_time.hpp>
#include <boost/cstdint.hpp>

namespace cl = olib::opencorelib;

namespace olib { namespace openmedialib { namespace ml { namespace audio {

calculator::calculator( int fps_num, int fps_den, int pps_num, int pps_den, int frequency, int lead_in )
	: fps_num_( fps_num )
	, fps_den_( fps_den )
	, pps_num_( pps_num )
	, pps_den_( pps_den )
	, frequency_( frequency )
	, lead_in_( lead_in )
{
}

void calculator::set_fps( int fps_num , int fps_den )
{
	fps_num_ = fps_num;
	fps_den_ = fps_den;
}

void calculator::set_pps( int pps_num , int pps_den )
{
	pps_num_ = pps_num;
	pps_den_ = pps_den;
}

void calculator::set_frequency( int frequency, int samples )
{
	frequency_ = frequency;
	if ( samples != 0 )
	{
		cl::rational_time pair( frequency_, samples );
		pps_num_ = pair.numerator( );
		pps_den_ = pair.denominator( );
	}
}

void calculator::set_lead_in( int lead_in )
{
	lead_in_ = lead_in;
}

int calculator::first_packet_for_frame( int position ) const
{
	return map_first( position, pps_num_, pps_den_, fps_num_, fps_den_ );
}

int calculator::last_packet_for_frame( int position ) const
{
	return map_last( position, pps_num_, pps_den_, fps_num_, fps_den_ );
}

int calculator::first_frame_for_packet( int position ) const
{
	return map_first( position, fps_num_, fps_den_, pps_num_, pps_den_ );
}

int calculator::last_frame_for_packet( int position ) const
{
	return map_last( position, fps_num_, fps_den_, pps_num_, pps_den_ );
}

block_type_ptr calculator::calculate( const int position ) const
{
	block_type_ptr block = block_type_ptr( new block_type( ) );

	int packet_position = first_packet_for_frame( position );
	int samples_in_frame = samples_for_frame( position, frequency_, fps_num_, fps_den_ );
	int samples_in_packet = samples_for_frame( packet_position, frequency_, pps_num_, pps_den_ );

	block->position = position;
	block->samples = samples_in_frame;
	block->first = packet_position;

	if ( lead_in_ )
	{
		boost::int64_t samples_in = audio::samples_to_frame( packet_position, frequency_, pps_num_, pps_den_ );
		boost::int64_t samples_lead = audio::samples_to_frame( packet_position + lead_in_, frequency_, pps_num_, pps_den_ );
		boost::int64_t samples_out = audio::samples_to_frame( position, frequency_, fps_num_, fps_den_ );

		block->discard = int( samples_lead - samples_in );
		block->discard += int( samples_out - samples_lead );

		int samples_left = ( lead_in_ + 1 ) * samples_in_packet - block->discard;

		block->count = 1 + lead_in_ + ( samples_in_frame - samples_left ) / samples_in_packet;
	}
	else
	{
		block->count = 1;
		block->discard = 0;
	}

	return block;
}

} } } }
