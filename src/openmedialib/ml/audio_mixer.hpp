
// ml::audio - channel conversion functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_MIXER_H_
#define AML_AUDIO_MIXER_H_

#include <openmedialib/ml/audio.hpp>
#include <cmath>
#include <vector>

// Windows work around
#ifndef M_PI
#	define M_PI 3.14159265358979323846
#endif

namespace olib { namespace openmedialib { namespace ml { namespace audio {

#define CLAMP( v, l, u )	( v < l ? l : v > u ? u : v )

template < typename T >
boost::shared_ptr< T > mixer( const audio_type_ptr &a_, const audio_type_ptr &b_, const audio_type_ptr &c_ )
{
	const boost::shared_ptr< T > a = coerce< T >( a_ );
	const boost::shared_ptr< T > b = coerce< T >( b_ );
	const boost::shared_ptr< T > c = coerce< T >( c_ );

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
	const float cutoff_to_fs_ratio	= 0.5;
	const float exponent = exp(-2.0 * M_PI * cutoff_to_fs_ratio);
	const float one_minus_exponent	= 1 - exponent;
	
	float sample_summation = 0;
	typename T::sample_type clipped_sample;

	typename T::sample_type *po = mix->data( );
	typename T::sample_type *pa = a->data( );
	typename T::sample_type *pb = b->data( );
	typename T::sample_type *pc = c ? c->data( ) : 0;

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
				clipped_sample = typename T::sample_type( sample_summation );
			
			// low pass filter to counter any effects from clipping
			if(sample_idx == 0 && !c)
				*po = typename T::sample_type(one_minus_exponent * clipped_sample);
			else if ( sample_idx == 0 )
				*po = typename T::sample_type(one_minus_exponent * clipped_sample + exponent * ( *( pc + c->samples( ) * c->channels( ) - channels_out + channel_idx ) ) );
			else
				*po = typename T::sample_type(one_minus_exponent * clipped_sample + exponent * ( *( po - channels_out ) ) );

			po ++;
		}
	}
	
	return mix;
}

template < typename T >
boost::shared_ptr< T > channel_mixer( audio_type_ptr &a, const audio_type_ptr &b, const std::vector< double > &volume, double &max_level, int mute, const audio_type_ptr &c )
{
	// Coerce both inputs to the size specified
	boost::shared_ptr< T > output = coerce< T >( a );
	boost::shared_ptr< T > input = coerce< T >( channel_convert( b, 1 ) );
	boost::shared_ptr< T > last = coerce< T >( c );

	// Shouldn't happen - precondition is that channel provides audio
	if ( !input )
		return output;

	// Valid case - on first usage, there won't be an output audio, so create it to match the first channel input
	if ( output == 0 )
	{
		const int frequency = input->frequency( );
		const int samples = input->samples( );
		output = boost::shared_ptr< T >( new T( frequency, volume.size( ), samples ) );
	}

	// Should never happen, but can accept if it does
	if ( size_t( output->channels( ) ) != volume.size( ) )
	{
        //PL_LOG(pl::level::warning, "Channels doesn't match volumes in mix_channel");
		output = coerce< T >( ml::audio::channel_convert( input, static_cast<int>(volume.size( ) ) ) );
		if ( output == 0 )
			return output;
	}

	// Do I reject or do a rough resample?
	// For now, we'll do a pitch shift
	if ( input->frequency( ) != output->frequency( ) )
	{
	}

	// In theory, this shouldn't happen, but just in case, force samples to match
	if ( input->samples( ) != output->samples( ) )
	{
		input = coerce< T >( ml::audio::pitch( input, output->samples( ) ) );
	}

	// Extract samples, number of channels and data
	int samples = input->samples( );
	int channels = output->channels( );
	typename T::sample_type *dst = output->data( );
	typename T::sample_type *src = input->data( );
	typename T::sample_type *old = last ? last->data( ) : 0;
	typename T::sample_type min_sample = input->min_sample( );
	typename T::sample_type max_sample = input->max_sample( );

	// We'll report the max sample here
	typename T::sample_type max_value = 0;
	max_level = 0.0;

	const float cutoff_to_fs_ratio	= 0.5f;
	const float exponent = exp( -2.0f * M_PI * cutoff_to_fs_ratio );
	const float minus_exponent	= 1.0f - exponent;

	float sum = 0.0;
	float clipped = 0;

	int solo_channel = -1;
	bool solo = true;

	for ( size_t index = 0; index < volume.size( ); index ++ )
	{
		if ( volume[ index ] != 0.0 )
		{
			if ( solo_channel == -1 )
				solo_channel = int( index );
			else
				solo = false;
		}
	}

	if ( !solo )
	{
		// Get the max sample value now
		max_value = std::abs( *src ) > max_value ? std::abs( *src ) : max_value;
	
		// Special case for first sample
		for ( int c = 0; c < channels; c ++ )
		{
			if ( !mute && volume[ c ] != 0.0 )
			{
				// Apply the volume
				sum =  *dst + volume[ c ] * *src;
	
				// Clip channels appropriately
				clipped = CLAMP( sum, min_sample, max_sample );
	
				// Low pass filter to counter any effects from clipping
				if ( !old )
					*dst = typename T::sample_type( minus_exponent * clipped + exponent * *dst );
				else
					*dst = typename T::sample_type( minus_exponent * clipped + exponent * *( old + last->samples( ) * last->channels( ) - channels + c ) );
			}

			dst ++;
		}
	
		// Mangle the audio samples
		for( int index = 1; index < samples; index ++ )
		{
			src ++;
	
			// Get the max sample value now
			max_value = std::abs( *src ) > max_value ? std::abs( *src ) : max_value;
	
			for ( int c = 0; c < channels; c ++ )
			{
				if ( !mute && volume[ c ] != 0.0 )
				{
					// Apply the volume
					sum = float( *dst ) + volume[ c ] * float( *src );
	
					// Clip channels appropriately
					clipped = CLAMP( sum, min_sample, max_sample );
	
					// Low pass filter to counter any effects from clipping
					*dst = typename T::sample_type( minus_exponent * clipped + exponent * *( dst - channels ) );
				}

				dst ++;
			}
		}
	}
	else if ( solo_channel >= 0 && solo_channel < channels )
	{
		double vol = volume[ solo_channel ];

		// Offset to first sample
		dst += solo_channel;

		// Get the max sample value now
		max_value = std::abs( *src ) > max_value ? std::abs( *src ) : max_value;
	
		if ( !mute )
		{
			// Apply the volume
			sum = *dst + vol * *src;
	
			// Clip channels appropriately
			clipped = CLAMP( sum, min_sample, max_sample );
	
			// Low pass filter to counter any effects from clipping
			if ( !old )
				*dst = typename T::sample_type( minus_exponent * clipped + exponent * *dst );
			else
				*dst = typename T::sample_type( minus_exponent * clipped + exponent * *( old + last->samples( ) * last->channels( ) - channels + solo_channel ) );
		}
	
		// Mangle the audio samples
		for( int index = 1; index < samples; index ++ )
		{
			src ++;
			dst += channels;
	
			// Get the max sample value now
			max_value = std::abs( *src ) > max_value ? std::abs( *src ) : max_value;
	
			if ( !mute )
			{
				// Apply the volume
				sum = *dst + vol * *src;
	
				// Clip channels appropriately
				clipped = CLAMP( sum, min_sample, max_sample );
	
				// Low pass filter to counter any effects from clipping
				*dst = typename T::sample_type( minus_exponent * clipped + exponent * *( dst - channels ) );
			}
		}
	}

	// Calculate the max level
	max_level = double( max_value ) / max_sample;

	return output;
}


} } } }

#endif
