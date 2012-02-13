
// ml::audio - utility functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_CHANNEL_EXTRACT_COUNT_H_
#define AML_AUDIO_CHANNEL_EXTRACT_COUNT_H_

#include <openmedialib/ml/audio.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

template< typename T > 
boost::shared_ptr< T > channel_extract_count( const audio_type_ptr &input, int count )
{
	const boost::shared_ptr< T > audio = coerce< T >( input );

	int samples = audio->samples( );
	int channels = audio->channels( );

	boost::shared_ptr< T > result = boost::shared_ptr< T >( new T( audio->frequency( ), count, samples ) );

	typename T::sample_type *src = audio->data( );
	typename T::sample_type *dst = result->data( );

	int n, skip = count >= channels ? 0 : channels - count;

	while (samples --)
	{
		for (n=0; n<count; n++)
		{
			if (n>=channels)
			{
				*dst ++ = typename T::sample_type(0);
			}
			else
			{
				*dst ++ = *src ++;
			}
		}

		src += skip;
	}

	return result;
}

} } } }

#endif
