
// ml::audio - conversion functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_CONVERT_H_
#define AML_AUDIO_CONVERT_H_

#include <openmedialib/ml/types.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

template < typename T, typename U >
void convert( T &dst, const U &src )
{
	int count = dst.samples( ) * dst.channels( );
	typename T::sample_type *dst_p = dst.data( );
	typename U::sample_type *src_p = src.data( );
	typename T::sample_type dst_max = dst.max_sample( );
	typename U::sample_type src_max = src.max_sample( );
	while( count -- )
		*dst_p ++ = ( typename T::sample_type )( ( float( *src_p ++ ) / src_max ) * dst_max );
}

} } } }

#endif
