
// ml::audio - base interface for all audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_INTERFACE_H_
#define AML_AUDIO_INTERFACE_H_

#include <openmedialib/ml/types.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

// Definitions of the the pure virtual interface which all audio types provide
class ML_DECLSPEC base
{
	public:
		virtual ~base( ) { }

		/// The identity of the audio type
		virtual identity id( ) const = 0;

		/// The number of bytes of storage required for sample
		virtual int sample_storage_size( ) const = 0;

		/// The frequency of the audio object
		virtual int frequency( ) const  = 0;

		/// The number of channels in the audio object
		virtual int channels( ) const  = 0;

		/// The number of samples in the audio object
		virtual int samples( ) const  = 0;

		/// The number of samples in the source audio (i.e. excluding
		/// any silent padded samples)
		virtual int original_samples( ) const = 0;

		/// Set the number of samples actually coming from the source
		virtual void original_samples( int samples ) = 0;

		/// A textual description of the audio format
		virtual const std::wstring &af( ) const = 0;

		/// The position of the audio object relative to the graph it belongs to
		virtual int position( ) const  = 0;

		/// Set the position of the audio object
		virtual void set_position( int position )   = 0;

		/// The number of bytes in the audio object
		virtual int size( ) const = 0;

		/// The number of bytes in the audio object refering to real
		/// samples in the source media
		virtual int original_size( ) const = 0;

		/// A void * pointer to access the data of the audio object
		virtual void *pointer( ) const = 0;

		/// Clone this audio object
		virtual audio_type_ptr clone( ) const = 0;

		/// Convert to a pcm16 object
		virtual void convert( pcm16 &dst ) const = 0;

		/// Convert to a pcm24 object
		virtual void convert( pcm24 &dst ) const = 0;

		/// Convert to a pcm32 object
		virtual void convert( pcm32 &dst ) const = 0;

		/// Convert to a floats object
		virtual void convert( floats &dst ) const = 0;
};

} } } }

#endif
