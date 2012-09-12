
// ml::audio - channel conversion functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_MIX_MATRIX_H_
#define AML_AUDIO_MIX_MATRIX_H_

#include <openmedialib/ml/audio.hpp>
#include <cmath>
#include <vector>

#include <opencorelib/cl/profile.hpp>
#include <opencorelib/cl/enforce_defines.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

namespace {
	bool mix_matrix_is_passthrough( const std::vector< double > &matrix, int channels )
	{
		int next_active_mix_line = 0;

		//Go through the mix matrix and make sure that it only has ones
		//on the diagonal, and zeros everywhere else (i.e. it's the
		//identity matrix).
		for( int i = 0; i < matrix.size( ); ++i )
		{
			double val = matrix[ i ];
			if( i == next_active_mix_line )
			{
				if( val < 0.999 || val > 1.001 )
					return false;

				next_active_mix_line += ( channels + 1 );
			}
			else
			{
				if( val > 0.001 )
					return false;
			}
		}

		return true;
	}
}

template < typename T >
boost::shared_ptr< T > mix_matrix( audio_type_ptr &a, const std::vector< double > &matrix, int channels )
{
	ARENFORCE_MSG( a, "No audio input provided" );
	ARENFORCE_MSG( matrix.size( ) == size_t( channels * a->channels( ) ), 
		"Malformed matrix - Got %1% input channels and %2% output channels, so %3% entries was expected but got %4%" )
		( a->channels( ) )( channels )( channels * a->channels( ) )( matrix.size( ) );

	//Avoid creating an audio object with zero samples
	if( channels == 0 )
	{
		return boost::shared_ptr< T >();
	}

	// Coerce the input to the correct type
	boost::shared_ptr< T > input = coerce< T >( a );

	// Allocate an output with the same number of samples with the target channel count
	ml::audio_type_ptr result = allocate( input, -1, channels );
	boost::shared_ptr< T > output = coerce< T >( result );

	if( a->channels( ) == channels )
	{
		if( mix_matrix_is_passthrough( matrix, channels ) )
		{
			//The mix matrix is passthrough, i.e. it won't change the audio,
			//so we can just do a straight copy instead of processing each
			//sample individually.
			memcpy( output->data( ), input->data( ), input->size( ) );
			return output;
		}
	}

	// Extract samples, number of channels and data
	int samples = input->samples( );
	int input_channels = input->channels( );
	typename T::sample_type *dst = output->data( );
	typename T::sample_type *src = input->data( );
	typename T::sample_type max_sample = input->max_sample( );

	float sa, sb;

	for( int index = 0; index < samples; index ++ )
	{
		for ( int c = 0; c < channels; c ++ )
		{
			sa = 0.0f;

			for ( int d = 0, p = c; d < input_channels; d ++, p += channels )
			{
				sb = ( float( *( src + d ) ) / max_sample ) * matrix[ p ];
				if ( sa < 0.0f && sb < 0.0f )
					sa = sa + sb + sa * sb;
				else
					sa = sa + sb - sa * sb;
			}

			*dst ++ = ( typename T::sample_type )( sa * max_sample );
		}
		src += input_channels;
	}

	return output;
}


} } } }

#endif
