
// ml::audio - conversion functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_CONVERT_H_
#define AML_AUDIO_CONVERT_H_

#include <openmedialib/ml/types.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

// Convert to pcm8
extern void convert( pcm8 &dst, const pcm8 &src );
extern void convert( pcm8 &dst, const pcm16 &src );
extern void convert( pcm8 &dst, const pcm24 &src );
extern void convert( pcm8 &dst, const pcm32 &src );
extern void convert( pcm8 &dst, const floats &src );

// Convert to pcm16
extern void convert( pcm16 &dst, const pcm8 &src );
extern void convert( pcm16 &dst, const pcm16 &src );
extern void convert( pcm16 &dst, const pcm24 &src );
extern void convert( pcm16 &dst, const pcm32 &src );
extern void convert( pcm16 &dst, const floats &src );

// Convert to pcm24
extern void convert( pcm24 &dst, const pcm8 &src );
extern void convert( pcm24 &dst, const pcm16 &src );
extern void convert( pcm24 &dst, const pcm24 &src );
extern void convert( pcm24 &dst, const pcm32 &src );
extern void convert( pcm24 &dst, const floats &src );

// Convert to pcm32
extern void convert( pcm32 &dst, const pcm8 &src );
extern void convert( pcm32 &dst, const pcm16 &src );
extern void convert( pcm32 &dst, const pcm24 &src );
extern void convert( pcm32 &dst, const pcm32 &src );
extern void convert( pcm32 &dst, const floats &src );

// Convert to float
extern void convert( floats &dst, const pcm8 &src );
extern void convert( floats &dst, const pcm16 &src );
extern void convert( floats &dst, const pcm24 &src );
extern void convert( floats &dst, const pcm32 &src );
extern void convert( floats &dst, const floats &src );

} } } }

#endif
