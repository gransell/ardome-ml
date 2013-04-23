
// ml::audio - utility functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_UTILITIES_H_
#define AML_AUDIO_UTILITIES_H_

#include <openmedialib/ml/types.hpp>
#include <string>
#include <vector>

using boost::uint8_t;
using boost::uint16_t;
using boost::uint32_t;
using boost::int32_t;

namespace olib { namespace openmedialib { namespace ml { namespace audio {

// Convenience function to allocate an audio type by the identity
extern ML_DECLSPEC audio_type_ptr allocate( const identity &id, int frequency, int channels, int samples, bool init_to_zero = true );

// Convenience function to allocate an audio type by the string identifier
extern ML_DECLSPEC audio_type_ptr allocate( const std::wstring &, int frequency, int channels, int samples, bool init_to_zero = true );

// Convenience funtion to allocat audio type based on existing audio object (arguments of -1 stipulate that we receive defaults from source)
extern ML_DECLSPEC audio_type_ptr allocate( const audio_type_ptr &, int frequency = -1, int channels = -1, int samples = -1, bool init_to_zero = true );

// Convenience function to coerce an audio_type_ptr to a specific type
extern ML_DECLSPEC audio_type_ptr coerce( const identity &id, const audio_type_ptr &source );

// Convenience function to coerce an audio_type_ptr to a specific type
extern ML_DECLSPEC audio_type_ptr coerce( const std::wstring &af, const audio_type_ptr &source );

// Convenience function to force an audio_type_ptr to a specific type
extern ML_DECLSPEC audio_type_ptr force( const identity &id, const audio_type_ptr &source );

// Convenience function to force an audio_type_ptr to a specific type
extern ML_DECLSPEC audio_type_ptr force( const std::wstring &af, const audio_type_ptr &source );

// Convenience function to cast an audio_type_ptr to a specific type
extern ML_DECLSPEC audio_type_ptr cast( const identity &id, const audio_type_ptr &source );

// Convenience function to cast an audio_type_ptr to a specific type
extern ML_DECLSPEC audio_type_ptr cast( const std::wstring &af, const audio_type_ptr &source );

// Convenience function to convert to another channel arrangement without changing type
extern ML_DECLSPEC audio_type_ptr channel_convert( const audio_type_ptr &a, int channels, const audio_type_ptr &c = audio_type_ptr( ) );

// Convenience function to extract a specific channel wihtout changing type
extern ML_DECLSPEC audio_type_ptr channel_extract( const audio_type_ptr &a, int channel );

// Convenience function to extract count channels, 0-count, wihtout changing type
extern ML_DECLSPEC audio_type_ptr channel_extract_count( const audio_type_ptr &a, int count );

// Convenience functions to mix a channel without changing the audio sample type in the first object
extern ML_DECLSPEC audio_type_ptr channel_mixer( audio_type_ptr &a, const audio_type_ptr &b, int channel, const audio_type_ptr &c = audio_type_ptr( ) );
extern ML_DECLSPEC audio_type_ptr channel_mixer( audio_type_ptr &a, const audio_type_ptr &b, const std::vector< double > &, const audio_type_ptr &c = audio_type_ptr( ) );
extern ML_DECLSPEC audio_type_ptr channel_mixer( audio_type_ptr &a, const audio_type_ptr &b, const std::vector< double > &, double &, const audio_type_ptr &c = audio_type_ptr( ) );
extern ML_DECLSPEC audio_type_ptr channel_mixer( audio_type_ptr &a, const audio_type_ptr &b, const std::vector< double > &, double &, int, const audio_type_ptr &c = audio_type_ptr( ) );
	
// Function to take a number of audio objects and combine them into one.
// The channels will be layed out in the order they are given in the vector. All audio objects
// will be coerced to the same format as the first one.
extern ML_DECLSPEC audio_type_ptr combine( std::vector< audio_type_ptr >& audios );

// Apply the mix matrix to the specified audio
extern ML_DECLSPEC audio_type_ptr mix_matrix( audio_type_ptr &input, const std::vector< double > &matrix, int channels );

// Place the specified channel from b in the channel from b
extern ML_DECLSPEC audio_type_ptr channel_place( audio_type_ptr &a, const audio_type_ptr &b, int out, int in );

// Convenience function to mix two objects without changing the audio sample type in the first object
extern ML_DECLSPEC audio_type_ptr mixer( const audio_type_ptr &a, const audio_type_ptr &b, const audio_type_ptr &c = audio_type_ptr( ) );

// Convenience function to change the pitch of an audio object without changing sample type
extern ML_DECLSPEC audio_type_ptr pitch( const audio_type_ptr &a, int samples );

// Convenience function to reverse the order of the samples in an audio object without changing type
extern ML_DECLSPEC audio_type_ptr reverse( const audio_type_ptr &a );

// Convenience function to change the volume of the samples in an audio object without changing type
extern ML_DECLSPEC audio_type_ptr volume( const audio_type_ptr &ra, float start, float end );

// Factory method for creating an audio reseat instance
extern ML_DECLSPEC reseat_ptr create_reseat( );

// Method to determine number of samples per channel required for a given frame offset at a specified frequency and frame rate
// locked_profile is used to lock the audio sample pattern down to conform with some i frame only material in NTSC.
// This should be set to either "dv" or "imx" and will produce the following patterns:
// NTSC dv:  1600, 1602, 1602, 1602, 1602
// NTSC imx: 1602, 1601, 1602, 1601, 1600
// p60 dv: 800, 800, 801, 801, 801, 801, 801, 801, 801, 801
// p60 imx: 800, 801, 801, 801, 801
// 24p: 2002
extern ML_DECLSPEC int samples_for_frame( int frame, int frequency, int fps_num, int fps_den, locked_profile::type profile = locked_profile::unknown );

// Method to determine the sample offset for a given frame number at a specified frequency and frame rate
extern ML_DECLSPEC boost::int64_t samples_to_frame( int frame, int frequency, int fps_num, int fps_den, locked_profile::type profile = locked_profile::unknown );

// Convert a string to an id
extern ML_DECLSPEC identity af_to_id( const std::wstring &af );

// Map an id to the audio format
extern ML_DECLSPEC const std::wstring &id_to_af( const identity &id );

// Returns the reversed bytes in the 32-bit input src. For example: 0x12345678 becomes 0x78563412
extern ML_DECLSPEC uint32_t bswap_32( const uint32_t src );

// AIFF is stored reversed. so reverse every 4-byte word, then copy first 3 bytes of the result onto the output.
// Size of src should be at least samples * channels * 4 bytes.
// Size of dest should be at least samples * channels * 3 bytes.
// byte position	3	2	1	0
// input pcm32		C	B	A	X
// output aiff24		A	B	C
extern ML_DECLSPEC void pack_aiff24_from_pcm32( uint8_t *dest, const uint8_t *src, const uint32_t samples, const uint32_t channels );

// Just copy the last 3 bytes of every 4 bytes onto the output buffer (dest).
// Size of src should be at least samples * channels * 4 bytes.
// Size of dest should be at least samples * channels * 3 bytes.
// byte position	3	2	1	0
// input pcm32		C	B	A	X
// output pcm24			C	B	A
extern ML_DECLSPEC void pack_pcm24_from_pcm32( uint8_t *dest, const uint8_t *src, const uint32_t samples, const uint32_t channels );

// Swaps the bytes of every 16-bit word. works on 128-bit (16-byte) chunks at a time (SSE). so the input data has to be 16-byte aligned.
// num_bytes should be the total number of bytes in the buffer (data). It should be divisible by 16.
extern ML_DECLSPEC void byteswap16_inplace( uint8_t *data, int32_t num_bytes );

} } } }

#endif
