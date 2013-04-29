
// ml::audio - utility functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/audio.hpp>
#include <openmedialib/ml/audio_channel_convert.hpp>
#include <openmedialib/ml/audio_channel_extract.hpp>
#include <openmedialib/ml/audio_channel_extract_count.hpp>
#include <openmedialib/ml/audio_mix_matrix.hpp>
#include <openmedialib/ml/audio_pitch.hpp>
#include <openmedialib/ml/audio_place.hpp>
#include <openmedialib/ml/audio_reverse.hpp>
#include <openmedialib/ml/audio_mixer.hpp>
#include <openmedialib/ml/audio_reseat.hpp>
#include <openmedialib/ml/audio_volume.hpp>
#include <emmintrin.h>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

// Look up table for ids
static const olib::t_string id_to_af_table[ ] = { FORMAT_PCM16, FORMAT_PCM24, FORMAT_PCM32, FORMAT_FLOAT };

// Convenience function to allocate an audio type by id
ML_DECLSPEC audio_type_ptr allocate( identity id, int frequency, int channels, int samples, bool init_to_zero )
{
	audio_type_ptr result;

	switch( id )
	{
		case ml::audio::pcm16_id:
			result = pcm16_ptr( new pcm16( frequency, channels, samples, init_to_zero ) );
			break;
		case ml::audio::pcm24_id:
			result = pcm24_ptr( new pcm24( frequency, channels, samples, init_to_zero ) );
			break;
		case ml::audio::pcm32_id:
			result = pcm32_ptr( new pcm32( frequency, channels, samples, init_to_zero ) );
			break;
		case ml::audio::float_id:
			result = floats_ptr( new floats( frequency, channels, samples, init_to_zero ) );
			break;
	}

	return result;
}

// Convenience funtion to allocat audio type based on existing audio object (arguments of -1 stipulate that we receive defaults from source)
ML_DECLSPEC audio_type_ptr allocate( const audio_type_ptr &source, int frequency, int channels, int samples, bool init_to_zero )
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

		result = allocate( source->id( ), frequency, channels, samples, init_to_zero );
	}

	return result;
}

// Convenience function to coerce an audio_type_ptr to a specific type
ML_DECLSPEC audio_type_ptr coerce( identity id, const audio_type_ptr &source )
{
	audio_type_ptr result;

	switch( id )
	{
		case ml::audio::pcm16_id:
			result = coerce< pcm16 >( source );
			break;
		case ml::audio::pcm24_id:
			result = coerce< pcm24 >( source );
			break;
		case ml::audio::pcm32_id:
			result = coerce< pcm32 >( source );
			break;
		case ml::audio::float_id:
			result = coerce< floats >( source );
			break;
	}

	return result;
}


// Convenience function to force an audio_type_ptr to a specific type
ML_DECLSPEC audio_type_ptr force( identity id, const audio_type_ptr &source )
{
	audio_type_ptr result;
	switch( id )
	{
		case ml::audio::pcm16_id:
			result = force< pcm16 >( source );
			break;
		case ml::audio::pcm24_id:
			result = force< pcm24 >( source );
			break;
		case ml::audio::pcm32_id:
			result = force< pcm32 >( source );
			break;
		case ml::audio::float_id:
			result = force< floats >( source );
			break;
	}

	return result;
}

// Convenience function to cast an audio_type_ptr to a specific type
ML_DECLSPEC audio_type_ptr cast( identity id, const audio_type_ptr &source )
{
	audio_type_ptr result;

	switch( id )
	{
		case ml::audio::pcm16_id:
			result = cast< pcm16 >( source );
			break;
		case ml::audio::pcm24_id:
			result = cast< pcm24 >( source );
			break;
		case ml::audio::pcm32_id:
			result = cast< pcm32 >( source );
			break;
		case ml::audio::float_id:
			result = cast< floats >( source );
			break;
	}

	return result;
}

// Convenience function to convert to another channel arrangement without changing type
ML_DECLSPEC audio_type_ptr channel_convert( const audio_type_ptr &audio, int channels, const audio_type_ptr &last )
{
	audio_type_ptr result;

	if ( audio )
	{
		if ( audio->id( ) == pcm16_id )
			result = channel_convert< pcm16 >( audio, channels, last );
		else if ( audio->id( ) == pcm24_id )
			result = channel_convert< pcm24 >( audio, channels, last );
		else if ( audio->id( ) == pcm32_id )
			result = channel_convert< pcm32 >( audio, channels, last );
		else if ( audio->id( ) == float_id )
			result = channel_convert< floats >( audio, channels, last );
	}

	return result;
}

// Convenience function to extract a channel without changing type
ML_DECLSPEC audio_type_ptr channel_extract( const audio_type_ptr &audio, int channel )
{
	audio_type_ptr result;

	if ( audio )
	{
		if ( audio->id( ) == pcm16_id )
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

ML_DECLSPEC audio_type_ptr channel_extract_count( const audio_type_ptr &audio, int count )
{
	audio_type_ptr result;

	if ( audio )
	{
		if ( audio->id( ) == pcm16_id )
			result = channel_extract_count< pcm16 >( audio, count );
		else if ( audio->id( ) == pcm24_id )
			result = channel_extract_count< pcm24 >( audio, count );
		else if ( audio->id( ) == pcm32_id )
			result = channel_extract_count< pcm32 >( audio, count );
		else if ( audio->id( ) == float_id )
			result = channel_extract_count< floats >( audio, count );
	}

	return result;
}


// Convenience function to change the pitch of audio object which changing sample type
ML_DECLSPEC audio_type_ptr pitch( const audio_type_ptr &audio, int required )
{
	audio_type_ptr result;

	if ( audio )
	{
		if ( audio->id( ) == pcm16_id )
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
		if ( audio->id( ) == pcm16_id )
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

// Convenience function to place a channel from b directly in place into b
ML_DECLSPEC audio_type_ptr channel_place( audio_type_ptr &a, const audio_type_ptr &b, int out, int in )
{
	audio_type_ptr result;

	if ( a )
	{
		if ( a->id( ) == pcm16_id )
			result = place< pcm16 >( a, b, out, in );
		else if ( a->id( ) == pcm24_id )
			result = place< pcm24 >( a, b, out, in );
		else if ( a->id( ) == pcm32_id )
			result = place< pcm32 >( a, b, out, in );
		else if ( a->id( ) == float_id )
			result = place< floats >( a, b, out, in );
	}

	return result;
}

// Convenience function to mix two audio objects together
ML_DECLSPEC audio_type_ptr mixer( const audio_type_ptr &a, const audio_type_ptr &b, const audio_type_ptr &c )
{
	audio_type_ptr result;

	if ( a )
	{
		if ( a->id( ) == pcm16_id )
			result = mixer< pcm16 >( a, b, c );
		else if ( a->id( ) == pcm24_id )
			result = mixer< pcm24 >( a, b, c );
		else if ( a->id( ) == pcm32_id )
			result = mixer< pcm32 >( a, b, c );
		else if ( a->id( ) == float_id )
			result = mixer< floats >( a, b, c );
	}

	return result;
}

// Convenience function to change the volume of the samples in an audio object without changing type
ML_DECLSPEC audio_type_ptr volume( const audio_type_ptr &a, float start, float end )
{
	audio_type_ptr result;

	if ( a )
	{
		if ( a->id( ) == pcm16_id )
			result = volume< pcm16 >( a, start, end );
		else if ( a->id( ) == pcm24_id )
			result = volume< pcm24 >( a, start, end );
		else if ( a->id( ) == pcm32_id )
			result = volume< pcm32 >( a, start, end );
		else if ( a->id( ) == float_id )
			result = volume< floats >( a, start, end );
	}

	return result;
}

// Convenience functions to mix a channel without changing the audio sample type in the first object
ML_DECLSPEC audio_type_ptr channel_mixer( audio_type_ptr &a, const audio_type_ptr &b, const std::vector< double > &levels, double &max_level, int mute, const audio_type_ptr &c )
{
	audio_type_ptr result = a;

	if ( b )
	{
		if ( ( a && a->id( ) == pcm16_id ) || b->id( ) == pcm16_id )
			result = channel_mixer< pcm16 >( a, b, levels, max_level, mute, c );
		else if ( ( a && a->id( ) == pcm24_id ) || b->id( ) == pcm24_id )
			result = channel_mixer< pcm24 >( a, b, levels, max_level, mute, c );
		else if ( ( a && a->id( ) == pcm32_id ) || b->id( ) == pcm32_id, c )
			result = channel_mixer< pcm32 >( a, b, levels, max_level, mute, c );
		else if ( ( a && a->id( ) == float_id ) || b->id( ) == float_id )
			result = channel_mixer< floats >( a, b, levels, max_level, mute, c );
	}

	return result;
}

// Provides a quick method to mix b with a specific channel of a
ML_DECLSPEC audio_type_ptr channel_mixer( audio_type_ptr &a, const audio_type_ptr &b, int channel, const audio_type_ptr &c )
{
	audio_type_ptr result = a;

	if ( a && b )
	{
		double max_level = 0.0;
		std::vector< double > levels;
		for ( int i = 0; i < a->channels( ); i ++ )
		{
			if ( i == channel )
				levels.push_back( 1.0 );
			else
				levels.push_back( 0.0 );
		}
		result = channel_mixer( a, b, levels, max_level, 0, c );
	}

	return result;
}

ML_DECLSPEC audio_type_ptr combine( std::vector< audio_type_ptr >& audios )
{
	ARENFORCE_MSG( !audios.empty(), "Trying to combine empty audio vector" );
	if( audios.size() == 1 )
		return audios[ 0 ];
	
	identity id = (*audios.begin())->id();
	
	int channels = 0;
	for( size_t i = 0; i < audios.size(); ++i )
	{
		ARENFORCE_MSG( audios[ i ], "Null audio objects are not allowed in audio vector" );
	
		if( audios[ i ]->id() != id) audios[ i ] = coerce( id, audios[ i ] );
		
		channels += audios[ i ]->channels();
	}
	
	audio_type_ptr ret = allocate( id, (*audios.begin())->frequency(), channels, (*audios.begin())->samples() );
	
	int current_channel = 0;
	for( size_t i = 0; i < audios.size(); ++i )
	{
		audio_type_ptr a = audios[ i ];
		for( int ch = 0; ch < a->channels(); ++ch, ++current_channel )
		{
			channel_place( ret, a, current_channel, ch );
		}
	}
	
	return ret;
}

ML_DECLSPEC audio_type_ptr mix_matrix( audio_type_ptr &a, const std::vector< double > &b, int c )
{
	audio_type_ptr result;

	if ( a )
	{
		if ( a->id( ) == pcm16_id )
			result = mix_matrix< pcm16 >( a, b, c );
		else if ( a->id( ) == pcm24_id )
			result = mix_matrix< pcm24 >( a, b, c );
		else if ( a->id( ) == pcm32_id )
			result = mix_matrix< pcm32 >( a, b, c );
		else if ( a->id( ) == float_id )
			result = mix_matrix< floats >( a, b, c );
	}

	return result;
}

ML_DECLSPEC audio_type_ptr channel_mixer( audio_type_ptr &a, const audio_type_ptr &b, const std::vector< double > &levels, const audio_type_ptr &c )
{
	double max_level = 0.0;
	return channel_mixer( a, b, levels, max_level, 0, c );
}

ML_DECLSPEC audio_type_ptr channel_mixer( audio_type_ptr &a, const audio_type_ptr &b, const std::vector< double > &levels, double &max_level, const audio_type_ptr &c )
{
	return channel_mixer( a, b, levels, max_level, 0, c );
}

// Factory method for creating an audio reseat instance
ML_DECLSPEC reseat_ptr create_reseat( )
{
	return reseat_ptr( new reseat_impl( ) );
}

// Convert a string to an id
ML_DECLSPEC identity af_to_id( const olib::t_string &af )
{
	identity id = ml::audio::pcm32_id;

	if ( af == ml::audio::FORMAT_PCM16 ) id = ml::audio::pcm16_id;
	else if ( af == ml::audio::FORMAT_PCM24 ) id = ml::audio::pcm24_id;
	else if ( af == ml::audio::FORMAT_PCM32 ) id = ml::audio::pcm32_id;
	else if ( af == ml::audio::FORMAT_FLOAT ) id = ml::audio::float_id;
	else ARENFORCE_MSG( false, "Could not convert af-string to id" )( af );
	
	return id;
}

// Convert an id to a printable string
ML_DECLSPEC const olib::t_string &id_to_af( identity id )
{
	return id_to_af_table[ id ];
}

ML_DECLSPEC int id_to_significant_bytes_per_sample( identity id )
{
	switch ( id )
	{
		case audio::pcm16_id:
			return audio::pcm16::sample_storage_size_static();
		case audio::pcm24_id:
			return 3;
		case audio::pcm32_id:
			return audio::pcm32::sample_storage_size_static();
		case audio::float_id:
			return audio::floats::sample_storage_size_static();
	}

	ARENFORCE_MSG( false, "invalid audio id" );
	return 0;
}

ML_DECLSPEC int id_to_storage_bytes_per_sample( identity id )
{
	switch ( id )
	{
		case audio::pcm16_id:
			return audio::pcm16::sample_storage_size_static();
		case audio::pcm24_id:
			return audio::pcm24::sample_storage_size_static();
		case audio::pcm32_id:
			return audio::pcm32::sample_storage_size_static();
		case audio::float_id:
			return audio::floats::sample_storage_size_static();
	}

	ARENFORCE_MSG( false, "invalid audio id" );
	return 0;
}

namespace locked_profile {

ML_DECLSPEC type from_string( const olib::t_string& p )
{
	if( p == _CT("dv") ) 
		return dv;
	if( p == _CT("imx") || p == _CT("mpeg") )
		return mpeg;
	
	ARENFORCE_MSG( false, "Could not convert string to locked_profile" )( p );
	return dv;
}

}

ML_DECLSPEC uint32_t bswap_32( const uint32_t src )
{
	return ((src >> 24) & 0xff) | ((src >>  8) & 0xff00) | ((src <<  8) & 0xff0000) | ((src << 24) & 0xff000000);
}

// aiff is stored reversed. so reverse every 4-byte word, then copy first 3 bytes of the result onto the output.
ML_DECLSPEC void pack_aiff24_from_pcm32( uint8_t *dest, const uint8_t *src, const uint32_t samples, const uint32_t channels )
{
	for( uint32_t i = 0; i < samples * channels; ++i, src += 4, dest += 3 )
	{
		uint32_t unswapped = *reinterpret_cast< const uint32_t * >( src );
		uint32_t swapped = bswap_32( unswapped );
		memcpy( dest, reinterpret_cast< const uint8_t * >( &swapped ), 3 );
	}
}

// just copy the last 3 bytes of every 4 bytes onto the output buffer
ML_DECLSPEC void pack_pcm24_from_pcm32( uint8_t *dest, const uint8_t *src, const uint32_t samples, const uint32_t channels )
{
	src++;	// instead of having to do a +1 for every memcpy, just start from an offsetted location and increase by 4 each time.
	for( uint32_t i = 0; i < samples * channels; ++i, src += 4, dest += 3 )
	{
		memcpy( dest, src, 3 );
	}
}

// swaps the bytes of every 16-bit word. works on 128-bit (16-byte) chunks at a time. so the input data has to be 16-byte aligned.
// pcm to aiff16. byteswap16_inplace
ML_DECLSPEC void byteswap16_inplace( uint8_t *data, int32_t num_bytes )
{		
	for( int i = 0; i < num_bytes; i += 16 )
	{
		__m128i s = _mm_load_si128(  reinterpret_cast< const __m128i * >( &data[ i ] ) );		// load data
		s = _mm_or_si128( _mm_slli_epi16( s, 8 ), _mm_srli_epi16( s, 8 ) );						// shift left, shift right, OR
		_mm_store_si128( reinterpret_cast< __m128i * >( &data[ i ] ), s );						// store swapped data
	}
}


} } } }

