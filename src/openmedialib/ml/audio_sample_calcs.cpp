
// ml::audio - sample calculator functionality for audio types

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.

#include <openmedialib/ml/utilities.hpp>
#include <cmath>

// Windows work around
#ifndef M_PI
#	define M_PI 3.14159265358979323846
#endif

namespace olib { namespace openmedialib { namespace ml { namespace audio {

namespace {

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
}

ML_DECLSPEC int samples_for_frame(int frameoffset, int samplefreq, int framerate_numerator, int framerate_denominator)
{
	// Work around: some video files report frame rates with a common denominator (ie: 25000:1000)
	// Without factoring that out upfront (in this implementation), the results are incorrect
	boost::int64_t common = gcd( framerate_numerator, framerate_denominator );
	framerate_numerator /= int( common );
	framerate_denominator /= int( common ); 

	double	frames_per_second	= double(framerate_numerator) / framerate_denominator;
	double	samples_per_frame	= double(samplefreq) / frames_per_second;
	
	int	deficit;
	int frames_per_cycle = calculate_cycle_size(frames_per_second, samplefreq, samples_per_frame, deficit);

	int		samples_per_frame_base	= int((( long long )samplefreq * framerate_denominator) / framerate_numerator);
	double	extra_sample_spread		= samples_per_frame - double(samples_per_frame_base);
	int		cycle_idx				= frameoffset % frames_per_cycle;

	if(cycle_idx == 0)
		return samples_per_frame_base + (extra_sample_spread > 0.5 ? deficit : 0);
	else
		return samples_per_frame_base + (int(floor((extra_sample_spread * cycle_idx) + 0.5)) > int(floor(extra_sample_spread * (cycle_idx - 1) + 0.5)) ? deficit : 0);
}

ML_DECLSPEC boost::int64_t samples_to_frame(int frameoffset, int samplefreq, int framerate_numerator, int framerate_denominator)
{
	boost::int64_t common = gcd( framerate_numerator, framerate_denominator );
	framerate_numerator /= int( common );
	framerate_denominator /= int( common ); 

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

