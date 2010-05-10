
// ml::audio - channel conversion functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_PLACE_H_
#define AML_AUDIO_PLACE_H_

#include <openmedialib/ml/audio.hpp>
#include <cmath>
#include <vector>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

template < typename T >
boost::shared_ptr< T > place( audio_type_ptr &a_, const audio_type_ptr &b_, int out, int in )
{
	const boost::shared_ptr< T > a = coerce< T >( a_ );
	const boost::shared_ptr< T > b = coerce< T >( b_ );

	// Reject if audio input frequencies differ
	if( a->frequency( ) != b->frequency( ) )
		return boost::shared_ptr< T >( );
	
	// Reject if number of audio samples differ
	if( a->samples( ) != b->samples( ) )
		return boost::shared_ptr< T >( );
	
	const int samples_out  = a->samples();   // a & b are the same.
	const int channels_out = a->channels();
	const int channels_in = b->channels();
	
	typename T::sample_type *pa = a->data( ) + out;
	typename T::sample_type *pb = b->data( ) + in;

	for( int sample_idx = 0; sample_idx < samples_out; sample_idx++ )
	{
		*pa = *pb;
		pa += channels_out;
		pb += channels_in;
	}
	
	return a;
}

} } } }

#endif
