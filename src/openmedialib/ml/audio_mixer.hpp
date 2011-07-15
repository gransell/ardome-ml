
// ml::audio - channel conversion functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_MIXER_H_
#define AML_AUDIO_MIXER_H_

#include <openmedialib/ml/audio.hpp>
#include <cmath>
#include <vector>

#include <opencorelib/cl/profile.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

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

	typename T::sample_type *po = mix->data( );
	typename T::sample_type *pa = a->data( );
	typename T::sample_type *pb = b->data( );

	typename T::sample_type max_sample = mix->max_sample( );
	float sa, sb;

	for( int sample_idx = 0; sample_idx < samples_out; sample_idx++)
	{
		for( int channel_idx = 0; channel_idx < channels_out; ++channel_idx)
		{
			sa = float( *pa ) / max_sample;
			sb = float( *pb ) / max_sample;
			*po ++ = ( typename T::sample_type )( ( ( sa + sb ) - ( sa * sb ) ) * max_sample );
			pa ++;
			pb ++;
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
	typename T::sample_type max_sample = input->max_sample( );

	// We'll report the max sample here
	typename T::sample_type max_value = 0;
	max_level = 0.0;

	int solo_channel = -1;
	bool solo = true;
	float sa, sb;

	for ( size_t index = 0; index < volume.size( ); index ++ )
	{
		if ( volume[ index ] != 0.0 )
		{
			ARENFORCE_MSG( volume[ index ] >= 0 && volume[ index ] <= 1.0, "Audio volume out of range" );
			if ( solo_channel == -1 )
				solo_channel = int( index );
			else
				solo = false;
		}
	}

	if ( !solo )
	{
		// Mangle the audio samples
		for( int index = 0; index < samples; index ++ )
		{
			// Get the max sample value now
			max_value = std::abs( *src ) > max_value ? std::abs( *src ) : max_value;
	
			for ( int c = 0; c < channels; c ++ )
			{
				if ( !mute && volume[ c ] != 0.0 )
				{
					sa = float( *dst ) / max_sample;
					sb = ( float( *src ) / max_sample ) * volume[ c ];
					*dst = ( typename T::sample_type )( ( ( sa + sb ) - ( sa * sb ) ) * max_sample );
				}

				dst ++;
			}

			src ++;
		}
	}
	else if ( solo_channel >= 0 && solo_channel < channels )
	{
		double vol = volume[ solo_channel ];

		// Offset to first sample
		dst += solo_channel;

		// Get the max sample value now
		max_value = std::abs( *src ) > max_value ? std::abs( *src ) : max_value;
	
		// Mangle the audio samples
		for( int index = 0; index < samples; index ++ )
		{
			// Get the max sample value now
			max_value = std::abs( *src ) > max_value ? std::abs( *src ) : max_value;
	
			if ( !mute )
			{
				sa = float( *dst ) / max_sample;
				sb = ( float( *src ) / max_sample ) * vol;
				*dst = ( typename T::sample_type )( ( ( sa + sb ) - ( sa * sb ) ) * max_sample );
			}

			src ++;
			dst += channels;
		}
	}

	// Calculate the max level
	max_level = double( max_value ) / max_sample;

	return output;
}


} } } }

#endif
