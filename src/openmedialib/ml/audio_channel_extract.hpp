
// ml::audio - utility functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_CHANNEL_EXTRACT_H_
#define AML_AUDIO_CHANNEL_EXTRACT_H_

#include <openmedialib/ml/audio.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

template< typename T > 
boost::shared_ptr< T > channel_extract( const audio_type_ptr &input, int channel )
{
	const boost::shared_ptr< T > audio = coerce< T >( input );

	int samples = audio->samples( );
	int channels = audio->channels( );

	boost::shared_ptr< T > result = boost::shared_ptr< T >( new T( audio->frequency( ), 1, samples, false ) );

	if ( channel < channels )
	{
		typename T::sample_type *src = audio->data( ) + channel;
		typename T::sample_type *dst = result->data( );
		while( samples -- )
		{
			*dst ++ = *src;
			src += channels;
		}
	}

	return result;
}

} } } }

#endif
