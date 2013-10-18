
// ml::audio - conversion functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_CONVERT_H_
#define AML_AUDIO_CONVERT_H_

#include <openmedialib/ml/types.hpp>
#include <openmedialib/ml/audio.hpp>
#include <string.h>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

template <typename D, typename S>
void convert(D &dst, const S &src);

template <>
void convert(pcm16 &dst, const pcm16 &src);
    
template <>
void convert(pcm24 &dst, const pcm24 &src);
    
template <>
void convert(pcm32 &dst, const pcm32 &src);
    
template <>
void convert(floats &dst, const floats &src);


template <>
void convert(pcm32 &dst, const pcm24 &src);
    
template <>
void convert(pcm24 &dst, const pcm32 &src);

template <>
void convert(pcm24 &dst, const pcm16 &src);

template <>
void convert(pcm32 &dst, const pcm16 &src);

template <>
void convert(pcm16 &dst, const pcm24 &src);

template <>
void convert(pcm16 &dst, const pcm32 &src);


template<>
void convert(floats &dst, const pcm16 &src);
	
template<>
void convert(floats &dst, const pcm24 &src);
	
template<>
void convert(floats &dst, const pcm32 &src);


template<>
void convert(pcm16 &dst, const floats &src);

template<>
void convert(pcm24 &dst, const floats &src);

template<>
void convert(pcm24 &dst, const floats &src);


template <typename D>
void convert_src_is_float(D &dst, const floats &src);

template <typename S>
void convert_dst_is_float(floats &dst, const S &src);


template <typename D, typename S>
void convert_memcopy(D &dst, const S &src);

template <typename D, typename S>
void convert_shift(D &dst, const S &src);

} } } }

#endif

