
// ml::audio - basic types for audio support

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_TYPES_H_
#define AML_AUDIO_TYPES_H_

#include <openmedialib/ml/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/integer_traits.hpp>
#include <boost/cstdint.hpp>
#include <deque>
#include <string>

#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/minimal_string_defines.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

// Supported audio formats
const olib::t_string FORMAT_PCM16 = _CT("pcm16");
const olib::t_string FORMAT_PCM24 = _CT("pcm24");
const olib::t_string FORMAT_PCM32 = _CT("pcm32");
const olib::t_string FORMAT_FLOAT = _CT("float");

// Enumerated type for audio id's
typedef enum
{
	pcm16_id,
	pcm24_id,
	pcm32_id,
	float_id
}
identity;


namespace locked_profile
{
	enum type
	{
		dv,
		mpeg,
		unknown
	};
	
	ML_DECLSPEC type from_string( const olib::t_string& p );
}


// Forward declaration to the base audio interface which all formats share
class ML_DECLSPEC base;

// Forward declaration to the template which implements the various types
template< typename T, identity B, int min_val, int max_val > class ML_DECLSPEC template_;

// Forward declarations of the specific types
typedef ML_DECLSPEC template_ < boost::int16_t, pcm16_id, boost::integer_traits<short int>::const_min, boost::integer_traits<short int>::const_max > pcm16;
typedef ML_DECLSPEC template_ < boost::int32_t, pcm24_id, boost::integer_traits<int>::const_min, boost::integer_traits<int>::const_max > pcm24;
typedef ML_DECLSPEC template_ < boost::int32_t, pcm32_id, boost::integer_traits<int>::const_min, boost::integer_traits<int>::const_max > pcm32;
typedef ML_DECLSPEC template_ < float, float_id, -1, 1 > floats;

// Declarations of the shared_ptr variants of the specific types
typedef ML_DECLSPEC boost::shared_ptr < pcm16 > pcm16_ptr;
typedef ML_DECLSPEC boost::shared_ptr < pcm24 > pcm24_ptr;
typedef ML_DECLSPEC boost::shared_ptr < pcm32 > pcm32_ptr;
typedef ML_DECLSPEC boost::shared_ptr < floats > floats_ptr;

/// Audio block definitions
struct block;
typedef ML_DECLSPEC struct block block_type;
typedef ML_DECLSPEC boost::shared_ptr< block_type > block_type_ptr;

// Forward declaration to audio reseat interface
class ML_DECLSPEC reseat;

// Audio reseat shared pointer
typedef ML_DECLSPEC boost::shared_ptr< reseat > reseat_ptr;

// Audio reseat factory method
extern ML_DECLSPEC reseat_ptr create_reseat( );

} } } }

#endif

