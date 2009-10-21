
// ml::audio - provides volume changing capabilities

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_VOLUME_H_
#define AML_AUDIO_VOLUME_H_

#include <openmedialib/ml/audio.hpp>
#include <cmath>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

#define CLAMP( v, l, u )	( v < l ? l : v > u ? u : v )

template< typename T > 
boost::shared_ptr< T > volume( const audio_type_ptr &input, float volume, float end )
{
	// Get the audio object
	boost::shared_ptr< T > audio = force< T >( input );

	// Extract samples, number of channels and data
	int samples = audio->samples( );
	int channels = audio->channels( );

	typename T::sample_type *data = audio->data( );
	typename T::sample_type min_sample = audio->min_sample( );
	typename T::sample_type max_sample = audio->max_sample( );

	// Assign initial volume and increment value (assume fixed for now)
	float increment = ( end - volume ) / samples;

	const float cutoff_to_fs_ratio	= 0.5f;
	const float exponent = exp( -2.0f * M_PI * cutoff_to_fs_ratio );
	const float minus_exponent	= 1.0f - exponent;

	float sum = 0.0f;
	float clipped = 0;

	// Special case for first sample
	for ( int c = 0; c < channels; c ++ )
	{
		// Apply the volume
		sum = volume * *data;

		// Clip channels appropriately
		clipped = CLAMP( sum, min_sample, max_sample );

		// Low pass filter to counter any effects from clipping
		*data ++ = typename T::sample_type( minus_exponent * clipped );
	}

	// Mangle the audio samples
	for( int index = 1; index < samples; index ++ )
	{
		volume += increment;

		for ( int c = 0; c < channels; c ++ )
		{
			// Apply the volume
			sum = volume * *data;

			// Clip channels appropriately
			clipped = CLAMP( sum, min_sample, max_sample );

			// Low pass filter to counter any effects from clipping
			*data = typename T::sample_type( minus_exponent * clipped + exponent * *( data - channels ) );
			data ++;
		}
	}

	return audio;
}

} } } }

#endif
