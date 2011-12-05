
// ml::audio - utility functionality for audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_UTILITIES_H_
#define AML_AUDIO_UTILITIES_H_

#include <openmedialib/ml/types.hpp>
#include <string>
#include <vector>

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
extern ML_DECLSPEC audio_type_ptr channel_convert( const audio_type_ptr &a, int channels, const audio_type_ptr &c = audio_type_ptr( ) );

// Convenience function to extract a specific channel wihtout changing type
extern ML_DECLSPEC audio_type_ptr channel_extract( const audio_type_ptr &a, int channel );

// Convenience functions to mix a channel without changing the audio sample type in the first object
extern ML_DECLSPEC audio_type_ptr channel_mixer( audio_type_ptr &a, const audio_type_ptr &b, int channel, const audio_type_ptr &c = audio_type_ptr( ) );
extern ML_DECLSPEC audio_type_ptr channel_mixer( audio_type_ptr &a, const audio_type_ptr &b, const std::vector< double > &, const audio_type_ptr &c = audio_type_ptr( ) );
extern ML_DECLSPEC audio_type_ptr channel_mixer( audio_type_ptr &a, const audio_type_ptr &b, const std::vector< double > &, double &, const audio_type_ptr &c = audio_type_ptr( ) );
extern ML_DECLSPEC audio_type_ptr channel_mixer( audio_type_ptr &a, const audio_type_ptr &b, const std::vector< double > &, double &, int, const audio_type_ptr &c = audio_type_ptr( ) );

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
extern ML_DECLSPEC int samples_for_frame( int frame, int frequency, int fps_num, int fps_den, const std::wstring& locked_profile = L"" );

// Method to determine the sample offset for a given frame number at a specified frequency and frame rate
extern ML_DECLSPEC boost::int64_t samples_to_frame( int frame, int frequency, int fps_num, int fps_den );

// Map an id to the audio format
extern ML_DECLSPEC const std::wstring &id_to_af( const identity &id );

} } } }

#endif
