
// ml::audio - utility functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_UTILITIES_H_
#define AML_AUDIO_UTILITIES_H_

#include <openmedialib/ml/types.hpp>
#include <string>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

// Convenience function to allocate an audio type by the string identifier
extern ML_DECLSPEC audio_type_ptr allocate( const std::wstring &, int frequency, int channels, int samples );

// Convenience funtion to allocat audio type based on existing audio object (arguments of -1 stipulate that we receive defaults from source)
extern ML_DECLSPEC audio_type_ptr allocate( const audio_type_ptr &, int frequency = -1, int channels = -1, int samples = -1 );

// Convenience function to coerce an audio_type_ptr to a specific type
extern ML_DECLSPEC audio_type_ptr coerce( const std::wstring &af, const audio_type_ptr &source );

// Convenience function to force an audio_type_ptr to a specific type
extern ML_DECLSPEC audio_type_ptr force( const std::wstring &af, const audio_type_ptr &source );

// Convenience function to cast an audio_type_ptr to a specific type
extern ML_DECLSPEC audio_type_ptr cast( const std::wstring &af, const audio_type_ptr &source );

// Convenience function to convert to another channel arrangement without changing type
extern ML_DECLSPEC audio_type_ptr channel_convert( const audio_type_ptr &audio, int channels );

// Convenience function to extract a specific channel wihtout changing type
extern ML_DECLSPEC audio_type_ptr channel_extract( const audio_type_ptr &audio, int channel );

// Convenience function to change the pitch of an audio object without changing sample type
extern ML_DECLSPEC audio_type_ptr pitch( const audio_type_ptr &audio, int samples );

// Convenience function to mix two objects without changing the audio sample type in the first object
extern ML_DECLSPEC audio_type_ptr mixer( const audio_type_ptr& input_a, const audio_type_ptr& input_b );

// Convenience function to reverse the order of the samples in an audio object without changing type
ML_DECLSPEC audio_type_ptr reverse( const audio_type_ptr & );

// Method to determine number of samples per channel required for a given frame offset at a specified frequency and frame rate
extern ML_DECLSPEC int samples_for_frame( int frame, int frequency, int fps_num, int fps_den );

// Method to determine number of samples provided per channel required up to a given frame offset at a specified frequency and frame rate
extern ML_DECLSPEC boost::int64_t samples_to_frame( int frame, int frequency, int fps_num, int fps_den );

// Map an id to the audio format
extern ML_DECLSPEC const std::wstring &id_to_af( const identity &id );

} } } }

#endif
