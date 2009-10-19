
// ml::audio - casting functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_CAST_H_
#define AML_AUDIO_CAST_H_

#include <openmedialib/ml/audio.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

// Attempt to cast the neutral audio_type_ptr to a specific audio type
template < typename T >
inline boost::shared_ptr< T > cast( const audio_type_ptr &audio )
{
	return boost::dynamic_pointer_cast< T >( audio );
}

// Force a convert and copy of a given neutral audio_type_ptr
template < typename T >
inline boost::shared_ptr< T > force( const audio_type_ptr &audio )
{
	return boost::shared_ptr< T >( new T( *( audio.get( ) ) ) );
}

// Coerce an audio type to a specific type (either by casting or forcing)
template < typename T >
inline boost::shared_ptr< T > coerce( const audio_type_ptr &audio )
{
	boost::shared_ptr< T > result = cast< T >( audio );
	if ( audio && !result )
		result = force< T >( audio );
	return result;
}

} } } }

#endif
