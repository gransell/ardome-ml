
// ml::audio - conversion functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#include <openmedialib/ml/audio.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

// Convert to pcm8
void convert( pcm8 &dst, const pcm8 &src )
{
	int count = dst.samples( ) * dst.channels( );
	boost::int8_t *dst_p = dst.data( );
	boost::int8_t *src_p = src.data( );
	while( count -- )
		*dst_p ++ = *src_p ++;
}

void convert( pcm8 &dst, const pcm16 &src )
{
	int count = dst.samples( ) * dst.channels( );
	boost::int8_t *dst_p = dst.data( );
	boost::int16_t *src_p = src.data( );
	while( count -- )
		*dst_p ++ = boost::int8_t( ( float( *src_p ++ ) / 32767 ) * 127 );
}

void convert( pcm8 &dst, const pcm24 &src )
{
	int count = dst.samples( ) * dst.channels( );
	boost::int8_t *dst_p = dst.data( );
	boost::int32_t *src_p = src.data( );
	while( count -- )
		*dst_p ++ = boost::int8_t( ( float( *src_p ++ ) / 8388607 ) * 127 );
}

void convert( pcm8 &dst, const pcm32 &src )
{
	int count = dst.samples( ) * dst.channels( );
	boost::int8_t *dst_p = dst.data( );
	boost::int32_t *src_p = src.data( );
	while( count -- )
		*dst_p ++ = boost::int8_t( ( float( *src_p ++ ) / 2147483647 ) * 127 );
}

void convert( pcm8 &dst, const floats &src )
{
	int count = dst.samples( ) * dst.channels( );
	boost::int8_t *dst_p = dst.data( );
	float *src_p = src.data( );
	while( count -- )
		*dst_p ++ = boost::int8_t( *src_p ++ * 127 );
}

// Convert to pcm16
void convert( pcm16 &dst, const pcm8 &src )
{
	int count = dst.samples( ) * dst.channels( );
	boost::int16_t *dst_p = dst.data( );
	boost::int8_t *src_p = src.data( );
	while( count -- )
		*dst_p ++ = boost::int16_t( ( float( *src_p ++ ) / 127 ) * 32767 );
}

void convert( pcm16 &dst, const pcm16 &src )
{
	int count = dst.samples( ) * dst.channels( );
	boost::int16_t *src_p = src.data( );
	boost::int16_t *dst_p = dst.data( );
	while( count -- )
		*dst_p ++ = *src_p ++;
}

void convert( pcm16 &dst, const pcm24 &src )
{
	int count = dst.samples( ) * dst.channels( );
	boost::int16_t *dst_p = dst.data( );
	boost::int32_t *src_p = src.data( );
	while( count -- )
		*dst_p ++ = boost::int16_t( ( float( *src_p ++ ) / 8388607 ) * 32767 );
}

void convert( pcm16 &dst, const pcm32 &src )
{
	int count = dst.samples( ) * dst.channels( );
	boost::int16_t *dst_p = dst.data( );
	boost::int32_t *src_p = src.data( );
	while( count -- )
		*dst_p ++ = boost::int16_t( ( float( *src_p ++ ) / 2147483647 ) * 32767 );
}

void convert( pcm16 &dst, const floats &src )
{
	int count = dst.samples( ) * dst.channels( );
	boost::int16_t *dst_p = dst.data( );
	float *src_p = src.data( );
	while( count -- )
		*dst_p ++ = boost::int16_t( *src_p ++ * 32767 );
}

// Convert to pcm24
void convert( pcm24 &dst, const pcm8 &src )
{
	int count = dst.samples( ) * dst.channels( );
	boost::int32_t *dst_p = dst.data( );
	boost::int8_t *src_p = src.data( );
	while( count -- )
		*dst_p ++ = boost::int16_t( ( float( *src_p ++ ) / 127 ) * 8388607 );
}

void convert( pcm24 &dst, const pcm16 &src )
{
	int count = dst.samples( ) * dst.channels( );
	boost::int32_t *dst_p = dst.data( );
	boost::int16_t *src_p = src.data( );
	while( count -- )
		*dst_p ++ = boost::int16_t( ( float( *src_p ++ ) / 32767 ) * 8388607 );
}

void convert( pcm24 &dst, const pcm24 &src )
{
	int count = dst.samples( ) * dst.channels( );
	boost::int32_t *src_p = src.data( );
	boost::int32_t *dst_p = dst.data( );
	while( count -- )
		*dst_p ++ = *src_p ++;
}

void convert( pcm24 &dst, const pcm32 &src )
{
	int count = dst.samples( ) * dst.channels( );
	boost::int32_t *dst_p = dst.data( );
	boost::int32_t *src_p = src.data( );
	while( count -- )
		*dst_p ++ = boost::int16_t( ( float( *src_p ++ ) / 32767 ) * 2147483647 );
}

void convert( pcm24 &dst, const floats &src )
{
	int count = dst.samples( ) * dst.channels( );
	boost::int32_t *dst_p = dst.data( );
	float *src_p = src.data( );
	while( count -- )
		*dst_p ++ = boost::int32_t( *src_p ++ * 8388607 );
}

// Convert to pcm32
void convert( pcm32 &dst, const pcm8 &src )
{
	int count = dst.samples( ) * dst.channels( );
	boost::int32_t *dst_p = dst.data( );
	boost::int8_t *src_p = src.data( );
	while( count -- )
		*dst_p ++ = boost::int16_t( ( float( *src_p ++ ) / 127 ) * 2147483647 );
}

void convert( pcm32 &dst, const pcm16 &src )
{
	int count = dst.samples( ) * dst.channels( );
	boost::int32_t *dst_p = dst.data( );
	boost::int16_t *src_p = src.data( );
	while( count -- )
		*dst_p ++ = boost::int16_t( ( float( *src_p ++ ) / 32767 ) * 2147483647 );
}

void convert( pcm32 &dst, const pcm24 &src )
{
	int count = dst.samples( ) * dst.channels( );
	boost::int32_t *dst_p = dst.data( );
	boost::int32_t *src_p = src.data( );
	while( count -- )
		*dst_p ++ = boost::int16_t( ( float( *src_p ++ ) / 8388607 ) * 2147483647 );
}

void convert( pcm32 &dst, const pcm32 &src )
{
	int count = dst.samples( ) * dst.channels( );
	boost::int32_t *src_p = src.data( );
	boost::int32_t *dst_p = dst.data( );
	while( count -- )
		*dst_p ++ = *src_p ++;
}

void convert( pcm32 &dst, const floats &src )
{
	int count = dst.samples( ) * dst.channels( );
	boost::int32_t *dst_p = dst.data( );
	float *src_p = src.data( );
	while( count -- )
		*dst_p ++ = boost::int32_t( *src_p ++ * 2147483647 );
}

// Convert to float
void convert( floats &dst, const pcm8 &src )
{
	int count = dst.samples( ) * dst.channels( );
	float *dst_p = dst.data( );
	boost::int8_t *src_p = src.data( );
	while( count -- )
		*dst_p ++ = float( *src_p ++ ) / 127;
}

void convert( floats &dst, const pcm16 &src )
{
	int count = dst.samples( ) * dst.channels( );
	float *dst_p = dst.data( );
	boost::int16_t *src_p = src.data( );
	while( count -- )
		*dst_p ++ = float( *src_p ++ ) / 32767;
}

void convert( floats &dst, const pcm24 &src )
{
	int count = dst.samples( ) * dst.channels( );
	float *dst_p = dst.data( );
	boost::int32_t *src_p = src.data( );
	while( count -- )
		*dst_p ++ = float( *src_p ++ ) / 8388607;
}

void convert( floats &dst, const pcm32 &src )
{
	int count = dst.samples( ) * dst.channels( );
	float *dst_p = dst.data( );
	boost::int32_t *src_p = src.data( );
	while( count -- )
		*dst_p ++ = float( *src_p ++ ) / 2147483647;
}

void convert( floats &dst, const floats &src )
{
	int count = dst.samples( ) * dst.channels( );
	float *src_p = src.data( );
	float *dst_p = dst.data( );
	while( count -- )
		*dst_p ++ = *src_p ++;
}

} } } }
