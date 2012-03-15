
// ml::audio - sample calculator functionality for audio types

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.

#include <opencorelib/cl/typedefs.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <cmath>

#include <boost/assign.hpp>
#include <boost/algorithm/string/predicate.hpp>

using olib::opencorelib::rational_time;
using std::vector;

namespace olib { namespace openmedialib { namespace ml { namespace audio {

namespace {

	typedef std::map< std::pair< rational_time, std::wstring >, std::vector< int > > sample_cycle_map;

	sample_cycle_map create_sample_cycles()
	{
		//Nicer assignments to std::vector
		using namespace boost::assign;

		sample_cycle_map result;
		rational_time ntsc( 30000, 1001 );
		rational_time p60( 60000, 1001 );
		rational_time p24( 24000, 1001 );
		
		result[ std::make_pair( ntsc, L"imx" ) ] += 1602, 1601, 1602, 1601, 1602;
		result[ std::make_pair( ntsc, L"dv" ) ] += 1600, 1602, 1602, 1602, 1602;
		result[ std::make_pair( p60, L"imx" ) ] += 800, 801, 801, 801, 801;

		//DV p60 is special in that it combines two frames into one block, so the
		//audio for each block is the same as the NTSC DV cycle. This means that
		//DV p60 will have 10 frames per cycle.
		result[ std::make_pair( p60, L"dv" ) ] += 800, 800, 801, 801, 801, 801, 801, 801, 801, 801;

		// 24P material seems to want 2002 samples per frame even though its 23.98 fps
		result[ std::make_pair( p24, L"imx" ) ] += 2002;
		result[ std::make_pair( p24, L"dv" ) ] += 2002;
		
		return result;
	}
	
	static sample_cycle_map sample_cycles = create_sample_cycles();

	int calculate_cycle_size(double frames_per_second, int samplefreq, double samples_per_frame, int &deficit)
	{
		int 	cycle_size	 	= int(floor(frames_per_second + 0.5));
		double 	extra_samples	= (int(floor(samples_per_frame + 0.5)) * int(floor(frames_per_second + 0.5))) - (samplefreq * int(floor(frames_per_second + 0.5)) / frames_per_second);
		
		while(int(extra_samples*1000) % 1000 != 0 && cycle_size <= 3000)
		{
			cycle_size *= 10;
			extra_samples *= 10;
		}
		
		if(int(floor(extra_samples + 0.5)) == 0)
			cycle_size = 1;
		else
			cycle_size /= int( gcd(cycle_size, int(floor(extra_samples + 0.5))) );
	
		deficit = int(extra_samples) < 0 ? -1 : 1;

		return cycle_size;
	}

	// If there is a special audio cycle associated with the given sample frequency and frame rate, the 
	// result parameter is filled with the cycle count values and true is returned.
	// If not, false is returned an the result parameter is left unchanged.
	bool get_audio_cycle(int samplefreq, int framerate_numerator, int framerate_denominator, 
									 const std::wstring& locked_profile, std::vector< int > &result )
	{
		rational_time fps( framerate_numerator, framerate_denominator );

		if( samplefreq == 48000 )
		{
			sample_cycle_map::const_iterator cycles = sample_cycles.find( std::make_pair( fps, locked_profile  ) );
			if( cycles != sample_cycles.end() )
			{
				result = cycles->second;
				return true;
			}
		}

		return false;
	}
}

ML_DECLSPEC int samples_for_frame(int frameoffset, int samplefreq, int framerate_numerator, int framerate_denominator, const std::wstring& locked_profile )
{
	// Work around: some video files report frame rates with a common denominator (ie: 25000:1000)
	// Without factoring that out upfront (in this implementation), the results are incorrect
	rational_time fps( framerate_numerator, framerate_denominator );

	std::vector< int > cycle;
	if( get_audio_cycle( samplefreq, fps.numerator(), fps.denominator(), locked_profile, cycle ) )
	{
		assert( cycle.size() > 0 );
		return cycle[ frameoffset % cycle.size() ];
	}

	double	frames_per_second	= double(fps.numerator()) / fps.denominator();
	double	samples_per_frame	= double(samplefreq) / frames_per_second;
	
	int	deficit;
	int frames_per_cycle = calculate_cycle_size(frames_per_second, samplefreq, samples_per_frame, deficit);

	int		samples_per_frame_base	= int((( long long )samplefreq * fps.denominator()) / fps.numerator());
	double	extra_sample_spread		= samples_per_frame - double(samples_per_frame_base);
	int		cycle_idx				= frameoffset % frames_per_cycle;

	if(cycle_idx == 0)
		return samples_per_frame_base + (extra_sample_spread > 0.5 ? deficit : 0);
	else
		return samples_per_frame_base + (int(floor((extra_sample_spread * cycle_idx) + 0.5)) > int(floor(extra_sample_spread * (cycle_idx - 1) + 0.5)) ? deficit : 0);
}

ML_DECLSPEC boost::int64_t samples_to_frame(int frameoffset, int samplefreq, int framerate_numerator, int framerate_denominator, const std::wstring& locked_profile )
{
	boost::int64_t common = gcd( framerate_numerator, framerate_denominator );
	framerate_numerator /= int( common );
	framerate_denominator /= int( common ); 

	std::vector< int > cycle;
	if( get_audio_cycle( samplefreq, framerate_numerator, framerate_denominator, locked_profile, cycle ) )
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
			total += i < remainder ? cycle[ i ] : 0;
		}

		return count * cycles + total;
	}

	double	frames_per_second	= double(framerate_numerator) / framerate_denominator;
	double	samples_per_frame	= double(samplefreq) / frames_per_second;
	
	int deficit;
	int frames_per_cycle = calculate_cycle_size(frames_per_second, samplefreq, samples_per_frame, deficit);
	
	int		samples_per_frame_base	= (samplefreq * framerate_denominator) / framerate_numerator;
	int		samples_per_cycle		= int(float(samples_per_frame * double(frames_per_cycle)));
	double	extra_sample_spread		= samples_per_frame - double(samples_per_frame_base);
	int		cycle_idx				= frameoffset % frames_per_cycle;
	
	boost::int64_t offset = ((boost::int64_t)(frameoffset / frames_per_cycle) * samples_per_cycle);
	
	if(cycle_idx > 0)
	{
		offset += cycle_idx * samples_per_frame_base;
		
		if(extra_sample_spread > 0.5)
			offset += deficit;
		
		offset += deficit * int(floor((extra_sample_spread * cycle_idx) - extra_sample_spread + 0.5));
	}

	return offset;
}

} } } }

