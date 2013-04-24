
// ml::audio - conversion functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_CONVERT_H_
#define AML_AUDIO_CONVERT_H_

#include <openmedialib/ml/types.hpp>
#include <string.h>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

template < typename T, typename U >
void convert( T &dst, const U &src )
{
	int count = dst.samples( ) * dst.channels( );
	typename T::sample_type *dst_p = dst.data( );
	typename U::sample_type *src_p = src.data( );

	if ( dst.id( ) == src.id( ) ||
		 ( pcm24_id == dst.id( ) && pcm32_id == src.id( ) ) ||
		 ( pcm32_id == dst.id( ) && pcm24_id == src.id( ) ) ) // special case for pcm24 <--> pcm32
	{
		memcpy( dst_p, src_p, count * dst.sample_storage_size( ) );
	}
	else if ( src.id( ) == float_id )
	{
		typename T::sample_type dst_max = dst.max_sample( );
		while( count -- )
			*dst_p ++ = ( typename T::sample_type )( *src_p ++ * dst_max );
	}
	else if ( dst.id( ) == float_id )
	{
		typename U::sample_type src_max = src.max_sample( );
		while( count -- )
			*dst_p ++ = ( typename T::sample_type )( float( *src_p ++ ) / src_max );
	}
	else
	{
		typename T::sample_type dst_max = dst.max_sample( );
		typename U::sample_type src_max = src.max_sample( );
		while( count -- )
			*dst_p ++ = ( typename T::sample_type )( ( float( *src_p ++ ) / src_max ) * dst_max );
	}
}

} } } }

#endif
