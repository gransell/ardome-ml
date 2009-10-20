
// ml::audio - channel conversion functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_MIXER_H_
#define AML_AUDIO_MIXER_H_

#include <openmedialib/ml/audio.hpp>
#include <cmath>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

template < typename T >
boost::shared_ptr< T > mixer( const audio_type_ptr &a_, const audio_type_ptr &b_ )
{
	const boost::shared_ptr< T > a = coerce< T >( a_ );
	const boost::shared_ptr< T > b = coerce< T >( b_ );

	// Reject if audio input frequencies differ
	if( a->frequency( ) != b->frequency( ) )
		return boost::shared_ptr< T >( );
	
	// Reject if number of audio samples differ
	if( a->samples( ) != b->samples( ) )
		return boost::shared_ptr< T >( );
	
	// Reject if number of audio channels differ
	if( a->channels( ) != b->channels( ) )
		return boost::shared_ptr< T >( );
	
	const int samples_out  = a->samples();   // a & b are the same.
	const int channels_out = a->channels();
	
	boost::shared_ptr< T > mix = boost::shared_ptr< T >( new T( a->frequency( ), channels_out, samples_out ) );

	// upfront filter calcs
	const double	cutoff_to_fs_ratio	= 0.5;
	const double	exponent			= exp(-2.0 * M_PI * cutoff_to_fs_ratio);
	const double	one_minus_exponent	= 1 - exponent;
	
	float sample_summation = 0;
	typename T::sample_type clipped_sample;

	typename T::sample_type *po = mix->data( );
	typename T::sample_type *pa = a->data( );
	typename T::sample_type *pb = b->data( );

	typename T::sample_type min_sample = mix->min_sample( );
	typename T::sample_type max_sample = mix->max_sample( );

	for( int sample_idx = 0; sample_idx < samples_out; sample_idx++)
	{
		for( int channel_idx = 0; channel_idx < channels_out; ++channel_idx)
		{
			// Add a & b together
			sample_summation = float( *pa ++ ) + float( *pb ++ );
			
			// clip appropriately
			if(sample_summation < min_sample)
				clipped_sample = min_sample; 
			else if(sample_summation > max_sample)
				clipped_sample = max_sample;
			else
				clipped_sample = ( typename T::sample_type )sample_summation;
			
			// low pass filter to counter any effects from clipping
			if(sample_idx == 0)
				*po = ( typename T::sample_type )(one_minus_exponent * clipped_sample);
			else
				*po = ( typename T::sample_type )(one_minus_exponent * clipped_sample + exponent * ( *( po - channels_out ) ) );

			po ++;
		}
	}
	
	return mix;
}

} } } }

#endif
