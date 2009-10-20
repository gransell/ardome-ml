
// ml::audio - basic types for audio support

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_TYPES_H_
#define AML_AUDIO_TYPES_H_

#include <openmedialib/ml/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <deque>
#include <string>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

// Supported audio formats
const std::wstring FORMAT_PCM8 = L"pcm8";
const std::wstring FORMAT_PCM16 = L"pcm16";
const std::wstring FORMAT_PCM24 = L"pcm24";
const std::wstring FORMAT_PCM32 = L"pcm32";
const std::wstring FORMAT_FLOAT = L"float";

// Enumerated type for audio id's
typedef enum
{
	pcm8_id,
	pcm16_id,
	pcm24_id,
	pcm32_id,
	float_id
}
identity;

// Forward declaration to the base audio interface which all formats share
class ML_DECLSPEC interface;

// Forward declaration to the template which implements the various types
template< typename T, identity B, int min_val, int max_val > class ML_DECLSPEC template_;

// Forward declarations of the specific types
typedef ML_DECLSPEC template_ < boost::int8_t, pcm8_id, -127, 127 > pcm8;
typedef ML_DECLSPEC template_ < boost::int16_t, pcm16_id, -32767, 32767 > pcm16;
typedef ML_DECLSPEC template_ < boost::int32_t, pcm24_id, -8388607, 8388607 > pcm24;
typedef ML_DECLSPEC template_ < boost::int32_t, pcm32_id, -2147483647, 2147483647 > pcm32;
typedef ML_DECLSPEC template_ < float, float_id, -1, 1 > floats;

// Declarations of the shared_ptr variants of the specific types
typedef ML_DECLSPEC boost::shared_ptr < pcm8 > pcm8_ptr;
typedef ML_DECLSPEC boost::shared_ptr < pcm16 > pcm16_ptr;
typedef ML_DECLSPEC boost::shared_ptr < pcm24 > pcm24_ptr;
typedef ML_DECLSPEC boost::shared_ptr < pcm32 > pcm32_ptr;
typedef ML_DECLSPEC boost::shared_ptr < floats > floats_ptr;

// Forward declaration to audio reseat interface
class ML_DECLSPEC reseat;

// Audio reseat shared pointer
typedef ML_DECLSPEC boost::shared_ptr< reseat > reseat_ptr;

// Audio reseat factory method
extern ML_DECLSPEC reseat_ptr create_reseat( );

} } } }

#endif

