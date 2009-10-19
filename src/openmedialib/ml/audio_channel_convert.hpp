
// ml::audio - channel conversion functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_CHANNEL_CONVERT_H_
#define AML_AUDIO_CHANNEL_CONVERT_H_

#include <openmedialib/ml/audio.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

extern ML_DECLSPEC audio::pcm8_ptr channel_converter( const pcm8_ptr &, int );
extern ML_DECLSPEC audio::pcm16_ptr channel_converter( const pcm16_ptr &, int );
extern ML_DECLSPEC audio::pcm24_ptr channel_converter( const pcm24_ptr &, int );
extern ML_DECLSPEC audio::pcm32_ptr channel_converter( const pcm32_ptr &, int );
extern ML_DECLSPEC audio::floats_ptr channel_converter( const floats_ptr &, int );

} } } }

#endif

