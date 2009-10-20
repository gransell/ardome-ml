
// ml::audio - utility functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#include <openmedialib/ml/audio.hpp>
#include <openmedialib/ml/audio_channel_convert.hpp>
#include <openmedialib/ml/audio_channel_extract.hpp>
#include <openmedialib/ml/audio_pitch.hpp>
#include <openmedialib/ml/audio_reverse.hpp>
#include <openmedialib/ml/audio_mixer.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

// Look up table for ids
static const std::wstring id_to_af_table[ ] = { FORMAT_PCM8, FORMAT_PCM16, FORMAT_PCM24, FORMAT_PCM32, FORMAT_FLOAT };

// Convenience function to allocate an audio type by the string identifier
ML_DECLSPEC audio_type_ptr allocate( const std::wstring &af, int frequency, int channels, int samples )
{
	audio_type_ptr result;

	if ( af == FORMAT_PCM8 )
		result = pcm8_ptr( new pcm8( frequency, channels, samples ) );
	else if ( af == FORMAT_PCM16 )
		result = pcm16_ptr( new pcm16( frequency, channels, samples ) );
	else if ( af == FORMAT_PCM24 )
		result = pcm24_ptr( new pcm24( frequency, channels, samples ) );
	else if ( af == FORMAT_PCM32 )
		result = pcm32_ptr( new pcm32( frequency, channels, samples ) );
	else if ( af == FORMAT_FLOAT )
		result = floats_ptr( new floats( frequency, channels, samples ) );

	return result;
}

// Convenience funtion to allocat audio type based on existing audio object (arguments of -1 stipulate that we receive defaults from source)
ML_DECLSPEC audio_type_ptr allocate( const audio_type_ptr &source, int frequency, int channels, int samples )
{
	audio_type_ptr result;

	if ( source )
	{
		if ( frequency == -1 )
			frequency = source->frequency( );
		if ( channels == -1 )
			channels = source->channels( );
		if ( samples == -1 )
			samples = source->samples( );

		result = allocate( source->af( ), frequency, channels, samples );
	}

	return result;
}

// Convenience function to coerce an audio_type_ptr to a specific type
ML_DECLSPEC audio_type_ptr coerce( const std::wstring &af, const audio_type_ptr &source )
{
	audio_type_ptr result;

	if ( af == FORMAT_PCM8 )
		result = coerce< pcm8 >( source );
	else if ( af == FORMAT_PCM16 )
		result = coerce< pcm16 >( source );
	else if ( af == FORMAT_PCM24 )
		result = coerce< pcm24 >( source );
	else if ( af == FORMAT_PCM32 )
		result = coerce< pcm32 >( source );
	else if ( af == FORMAT_FLOAT )
		result = coerce< floats >( source );

	return result;
}

// Convenience function to force an audio_type_ptr to a specific type
ML_DECLSPEC audio_type_ptr force( const std::wstring &af, const audio_type_ptr &source )
{
	audio_type_ptr result;

	if ( af == FORMAT_PCM8 )
		result = force< pcm8 >( source );
	else if ( af == FORMAT_PCM16 )
		result = force< pcm16 >( source );
	else if ( af == FORMAT_PCM24 )
		result = force< pcm24 >( source );
	else if ( af == FORMAT_PCM32 )
		result = force< pcm32 >( source );
	else if ( af == FORMAT_FLOAT )
		result = force< floats >( source );

	return result;
}

// Convenience function to cast an audio_type_ptr to a specific type
ML_DECLSPEC audio_type_ptr cast( const std::wstring &af, const audio_type_ptr &source )
{
	audio_type_ptr result;

	if ( af == FORMAT_PCM8 )
		result = cast< pcm8 >( source );
	else if ( af == FORMAT_PCM16 )
		result = cast< pcm16 >( source );
	else if ( af == FORMAT_PCM24 )
		result = cast< pcm24 >( source );
	else if ( af == FORMAT_PCM32 )
		result = cast< pcm32 >( source );
	else if ( af == FORMAT_FLOAT )
		result = cast< floats >( source );

	return result;
}

// Convenience function to convert to another channel arrangement without changing type
ML_DECLSPEC audio_type_ptr channel_convert( const audio_type_ptr &audio, int channels )
{
	audio_type_ptr result;

	if ( audio )
	{
		if ( audio->id( ) == pcm8_id )
			result = channel_convert< pcm8 >( audio, channels );
		else if ( audio->id( ) == pcm16_id )
			result = channel_convert< pcm16 >( audio, channels );
		else if ( audio->id( ) == pcm24_id )
			result = channel_convert< pcm24 >( audio, channels );
		else if ( audio->id( ) == pcm32_id )
			result = channel_convert< pcm32 >( audio, channels );
		else if ( audio->id( ) == float_id )
			result = channel_convert< floats >( audio, channels );
	}

	return result;
}

// Convenience function to extract a channel without changing type
ML_DECLSPEC audio_type_ptr channel_extract( const audio_type_ptr &audio, int channel )
{
	audio_type_ptr result;

	if ( audio )
	{
		if ( audio->id( ) == pcm8_id )
			result = channel_extract< pcm8 >( audio, channel );
		else if ( audio->id( ) == pcm16_id )
			result = channel_extract< pcm16 >( audio, channel );
		else if ( audio->id( ) == pcm24_id )
			result = channel_extract< pcm24 >( audio, channel );
		else if ( audio->id( ) == pcm32_id )
			result = channel_extract< pcm32 >( audio, channel );
		else if ( audio->id( ) == float_id )
			result = channel_extract< floats >( audio, channel );
	}

	return result;
}

// Convenience function to change the pitch of audio object which changing sample type
ML_DECLSPEC audio_type_ptr pitch( const audio_type_ptr &audio, int required )
{
	audio_type_ptr result;

	if ( audio )
	{
		if ( audio->id( ) == pcm8_id )
			result = pitch< pcm8 >( audio, required );
		else if ( audio->id( ) == pcm16_id )
			result = pitch< pcm16 >( audio, required );
		else if ( audio->id( ) == pcm24_id )
			result = pitch< pcm24 >( audio, required );
		else if ( audio->id( ) == pcm32_id )
			result = pitch< pcm32 >( audio, required );
		else if ( audio->id( ) == float_id )
			result = pitch< floats >( audio, required );
	}

	return result;
}

// Convenience function to reverse the order of the samples in an audio object without changing type
ML_DECLSPEC audio_type_ptr reverse( const audio_type_ptr &audio )
{
	audio_type_ptr result;

	if ( audio )
	{
		if ( audio->id( ) == pcm8_id )
			result = reverse< pcm8 >( audio );
		else if ( audio->id( ) == pcm16_id )
			result = reverse< pcm16 >( audio );
		else if ( audio->id( ) == pcm24_id )
			result = reverse< pcm24 >( audio );
		else if ( audio->id( ) == pcm32_id )
			result = reverse< pcm32 >( audio );
		else if ( audio->id( ) == float_id )
			result = reverse< floats >( audio );
	}

	return result;
}

// Convenience function to mix two audio objects together
ML_DECLSPEC audio_type_ptr mixer( const audio_type_ptr &a, const audio_type_ptr &b )
{
	audio_type_ptr result;

	if ( a )
	{
		if ( a->id( ) == pcm8_id )
			result = mixer< pcm8 >( a, b );
		else if ( a->id( ) == pcm16_id )
			result = mixer< pcm16 >( a, b );
		else if ( a->id( ) == pcm24_id )
			result = mixer< pcm24 >( a, b );
		else if ( a->id( ) == pcm32_id )
			result = mixer< pcm32 >( a, b );
		else if ( a->id( ) == float_id )
			result = mixer< floats >( a, b );
	}

	return result;
}

// Convert an id to a printable string
const std::wstring &id_to_af( const identity &id )
{
	return id_to_af_table[ id ];
}

} } } }

