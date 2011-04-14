
// ml::audio - provides pitch shifting capabilities

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_PITCH_H_
#define AML_AUDIO_PITCH_H_

#include <openmedialib/ml/audio.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

template< typename T > 
boost::shared_ptr< T > pitch( const audio_type_ptr &input, int required )
{
	const boost::shared_ptr< T > audio = coerce< T >( input );

	int channels = audio->channels( );
	int frequency = audio->frequency( );

	boost::shared_ptr< T > output = boost::shared_ptr< T >( new T( frequency, channels, required ) );

	typename T::sample_type *dst = output->data( );
	typename T::sample_type *src = audio->data( );
	double ratio = double( audio->samples( ) ) / required;
	int upper = channels * ( audio->samples( ) - 1 );

	output->original_samples( audio->original_samples( ) / ratio );

	for ( int i = 0; i < required; i ++ )
	{
		for ( int c = 0; c < channels; c ++ )
		{
			int offset = channels * int( i * ratio ) + c;
			if ( offset < upper )
				*dst ++ = ( typename T::sample_type )( ( src[ offset ] + src[ offset + channels ] ) / 2 );
			else
				*dst ++ = src[ offset ];
		}
	}

	return output;
}

} } } }

#endif
