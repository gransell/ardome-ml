// ml::audio - conversion functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#include <openmedialib/ml/audio_convert.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

template <>
void convert(pcm16 &dst, const pcm16 &src)
{
	convert_memcopy(dst, src);
}

template <>
void convert(pcm24 &dst, const pcm24 &src)
{
	convert_memcopy(dst, src);
}

template <>
void convert(pcm32 &dst, const pcm32 &src)
{
	convert_memcopy(dst, src);
}

template <>
void convert(floats &dst, const floats &src)
{
	convert_memcopy(dst, src);
}

template <>
void convert(pcm32 &dst, const pcm24 &src)
{
	convert_memcopy(dst, src);
}

template <>
void convert(pcm24 &dst, const pcm32 &src)
{
	convert_memcopy(dst, src);
}

template <>
void convert(pcm24 &dst, const pcm16 &src)
{
	convert_shift(dst, src);
}

template <>
void convert(pcm32 &dst, const pcm16 &src)
{
	convert_shift(dst, src);
}

template <>
void convert(pcm16 &dst, const pcm24 &src)
{
	convert_shift(dst, src);
}

template <>
void convert(pcm16 &dst, const pcm32 &src)
{
	convert_shift(dst, src);
}

template<>
void convert(pcm16 &dst, const floats &src)
{
	convert_src_is_float(dst, src);
}

template<>
void convert(pcm24 &dst, const floats &src)
{
	convert_src_is_float(dst, src);
}

template<>
void convert(pcm32 &dst, const floats &src)
{
	convert_src_is_float(dst, src);
}

template<>
void convert(floats &dst, const pcm16 &src)
{
	convert_dst_is_float(dst, src);
}

template<>
void convert(floats &dst, const pcm24 &src)
{
	convert_dst_is_float(dst, src);
}

template<>
void convert(floats &dst, const pcm32 &src)
{
	convert_dst_is_float(dst, src);
}

template <typename D>
void convert_src_is_float(D &dst, const floats &src)
{
	int                     count   = dst.samples() * dst.channels();
	typename D::sample_type *dst_p  = dst.data();
	float                   *src_p  = src.data();
	float                   dst_min = dst.min_sample();

	while(count --) {
		boost::int64_t dst_val = *src_p * (-dst_min);
		
		if (dst_val < boost::integer_traits<typename D::sample_type>::const_min) {
			dst_val = boost::integer_traits<typename D::sample_type>::const_min;
		}
		
		if (dst_val > boost::integer_traits<typename D::sample_type>::const_max) {
			dst_val = boost::integer_traits<typename D::sample_type>::const_max;
		}

		*dst_p = dst_val;

		src_p ++;
		dst_p ++;
	}
}

template <typename S>
void convert_dst_is_float(floats &dst, const S &src)
{
	int                     count   = dst.samples( ) * dst.channels( );
	float                   *dst_p  = dst.data( );
	typename S::sample_type *src_p  = src.data( );
	float                   src_min = src.min_sample();

	while(count --) {
		*dst_p = ((float) (*src_p))/(-src_min);

		src_p ++;
		dst_p ++;
	}
}

template <typename D, typename S>
void convert_memcopy(D &dst, const S &src)
{
	int                     count  = dst.samples() * dst.channels();
	typename S::sample_type *src_p = src.data();
	typename D::sample_type *dst_p = dst.data();

	memcpy(dst_p, src_p, count * dst.sample_storage_size());
}

template <typename D, typename S>
void convert_shift(D &dst, const S &src)
{
	int                     count  = dst.samples() * dst.channels();
	typename S::sample_type *src_p = src.data();
	typename D::sample_type *dst_p = dst.data();

	if (src.id() == pcm16_id && (dst.id() == pcm24_id || dst.id() == pcm32_id))
	{
		while(count --) {
			*dst_p++ = (boost::int32_t) ((*src_p++) << 16);
		}
	} else if (dst.id() == pcm16_id && (src.id() == pcm24_id || src.id() == pcm32_id))
	{
		while(count --) {
			*dst_p++ = (boost::int32_t) ((*src_p++) >> 16);
		}
	}
}

} } } }
