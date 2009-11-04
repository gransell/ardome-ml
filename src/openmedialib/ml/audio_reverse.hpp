
// ml::audio - reverse the audio of the samples

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_REVERSE_H_
#define AML_AUDIO_REVERSE_H_

#include <openmedialib/ml/audio.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

template < typename T >
boost::shared_ptr< T > reverse( const audio_type_ptr &input )
{
	boost::shared_ptr< T > audio = force< T >( input );

	const int channels = audio->channels( );
	const int samples = audio->samples( );
	
	typename T::sample_type *in = audio->data( );
	typename T::sample_type *out = in + channels * samples - channels;
	typename T::sample_type t;
	int c;

	while ( in < out )
	{
		c = channels;
		while( c -- )
		{
			t = *in;
			*in ++ = *out;
			*out ++ = t;
		}
		out -= 2 * channels;
	}

	return audio;
}

} } } }

#endif
