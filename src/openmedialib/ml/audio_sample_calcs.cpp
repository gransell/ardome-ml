
// ml::audio - sample calculator functionality for audio types

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.

#include <opencorelib/cl/typedefs.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <cmath>

#include <boost/assign.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/rational.hpp>

using olib::opencorelib::rational_time;
using std::vector;

namespace olib { namespace openmedialib { namespace ml { namespace audio {

namespace {

	typedef std::map< std::pair< rational_time, locked_profile::type >, std::vector< int > > sample_cycle_map;

	sample_cycle_map create_sample_cycles()
	{
		//Nicer assignments to std::vector
		using namespace boost::assign;

		sample_cycle_map result;
		rational_time ntsc( 30000, 1001 );
		rational_time p60( 60000, 1001 );
		rational_time p24( 24000, 1001 );
		
		result[ std::make_pair( ntsc, locked_profile::mpeg ) ] += 1602, 1601, 1602, 1601, 1602;
		result[ std::make_pair( ntsc, locked_profile::dv ) ] += 1600, 1602, 1602, 1602, 1602;
		result[ std::make_pair( p60, locked_profile::mpeg ) ] += 800, 801, 801, 801, 801;

		//DV p60 is special in that it combines two frames into one block, so the
		//audio for each block is the same as the NTSC DV cycle. This means that
		//DV p60 will have 10 frames per cycle.
		result[ std::make_pair( p60, locked_profile::dv ) ] += 800, 800, 801, 801, 801, 801, 801, 801, 801, 801;

		// 24P material seems to want 2002 samples per frame even though its 23.98 fps
		result[ std::make_pair( p24, locked_profile::mpeg ) ] += 2002;
		result[ std::make_pair( p24, locked_profile::dv ) ] += 2002;
		
		return result;
	}
	
	static sample_cycle_map sample_cycles = create_sample_cycles();

	// If there is a special audio cycle associated with the given sample frequency and frame rate, the 
	// result parameter is filled with the cycle count values and true is returned.
	// If not, false is returned an the result parameter is left unchanged.
	bool get_audio_cycle(int samplefreq, int framerate_numerator, int framerate_denominator, 
									 locked_profile::type profile, std::vector< int > &result )
	{
		rational_time fps( framerate_numerator, framerate_denominator );

		if( samplefreq == 48000 )
		{
			sample_cycle_map::const_iterator cycles = sample_cycles.find( std::make_pair( fps, profile  ) );
			if( cycles != sample_cycles.end() )
			{
				result = cycles->second;
				return true;
			}
		}

		return false;
	}
}

ML_DECLSPEC int samples_for_frame(int frameoffset, int samplefreq, int framerate_numerator, int framerate_denominator, locked_profile::type profile )
{
	// Work around: some video files report frame rates with a common denominator (ie: 25000:1000)
	// Without factoring that out upfront (in this implementation), the results are incorrect
	rational_time fps( framerate_numerator, framerate_denominator );

	std::vector< int > cycle;
	if( get_audio_cycle( samplefreq, fps.numerator(), fps.denominator(), profile, cycle ) )
	{
		assert( !cycle.empty() );
		return cycle[ frameoffset % cycle.size() ];
	}

	boost::rational< int > framerate = boost::rational< int >( framerate_numerator, framerate_denominator );
	boost::rational< int > frequency = boost::rational< int >( samplefreq );
	boost::rational< int > sequence = frequency / framerate;
	int offset = frameoffset % sequence.denominator( );

	return boost::rational_cast< int >( sequence * ( offset + 1 ) ) - boost::rational_cast< int >( sequence * offset );
}

ML_DECLSPEC boost::int64_t samples_to_frame(int frameoffset, int samplefreq, int framerate_numerator, int framerate_denominator, locked_profile::type profile )
{
	boost::int64_t common = gcd( framerate_numerator, framerate_denominator );
	framerate_numerator /= int( common );
	framerate_denominator /= int( common ); 

	std::vector< int > cycle;
	if( get_audio_cycle( samplefreq, framerate_numerator, framerate_denominator, profile, cycle ) )
	{
		// Determine the size of the cycle
		size_t size = cycle.size( );
		assert( size > 0 );

		// Determine the number of cycles and remainder
		int cycles = frameoffset / size;
		int remainder = frameoffset % size;

		// Calculate samples in the cycle
		boost::int64_t total = 0;
		boost::int64_t count = 0;
		for ( size_t i = 0; i < size; i ++ ) 
		{
			count += cycle[ i ];
			total += int( i ) < remainder ? cycle[ i ] : 0;
		}

		return count * cycles + total;
	}

	boost::rational< int > fps = boost::rational< int >( framerate_numerator, framerate_denominator );
	boost::rational< int > frequency = boost::rational< int >( samplefreq );
	boost::rational< int > sequence = frequency / fps;
	int offset = frameoffset % sequence.denominator( );
	int cycles = ( frameoffset - offset ) / sequence.denominator( );

	return boost::int64_t( cycles ) * sequence.numerator( ) + boost::rational_cast< int >( sequence * offset );
}

} } } }

