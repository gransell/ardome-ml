
// ml::audio - template for all audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_TEMPLATE_H_
#define AML_AUDIO_TEMPLATE_H_

#include <openmedialib/ml/types.hpp>
#include <string.h>
#include <string>

// Needed for 16 byte memalign functions on all platforms except os x
#if defined( OLIB_ON_WINDOWS )
#include <malloc.h>
#elif defined( OLIB_ON_LINUX )
#include <stdlib.h>
#endif

#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/enforce_defines.hpp>


namespace olib { namespace openmedialib { namespace ml { namespace audio {

// The template which provides all the types
template< typename T, identity B, int min_val, int max_val >
class ML_DECLSPEC template_ : public base
{
	public:
		typedef T sample_type;

		template_( int frequency, int channels, int samples )
			: id_( B )
			, frequency_( frequency )
			, channels_( channels )
			, samples_( samples )
			, orig_samples_( samples )
			, position_( 0 )
		{
			init_data( );
		}

		template_( const base &other )
			: id_( B )
			, frequency_( other.frequency( ) )
			, channels_( other.channels( ) )
			, samples_( other.samples( ) )
			, orig_samples_( other.original_samples( ) )
			, position_( other.position( ) )
		{
			init_data( );
			other.convert( *this );
		}

		// No definition to make sure that we don't call this by mistake when we
		// intended the above copy constructor.
		template_( const template_< T, B, min_val, max_val > &other );

		virtual ~template_( )
		{
			free( data_ );
		}

		const sample_type min_sample( ) const
		{ return sample_type( min_val ); }

		const sample_type max_sample( ) const
		{ return sample_type( max_val ); }

		audio_type_ptr clone( ) const
		{ return audio_type_ptr( new template_< sample_type, B, min_val, max_val >( static_cast<const base&>(*this) ) ); }

		identity id( ) const 
		{ return id_; }

		int sample_size( ) const
		{ return sizeof( sample_type ); }

		int size( ) const
		{ return sizeof( sample_type ) * samples_ * channels_; }

		int original_size( ) const
		{ return sizeof( sample_type ) * orig_samples_ * channels_; }

		int frequency( ) const 
		{ return frequency_; }

		int channels( ) const 
		{ return channels_; }

		int samples( ) const 
		{ return samples_; }

		int original_samples( ) const
		{ return orig_samples_; }

		void original_samples( int samples )
		{ orig_samples_ = samples; }

		const std::wstring &af( ) const
		{ return id_to_af( id_ ); }

		void *pointer( ) const
		{
			return static_cast< void * >( data( ) );
		}

		sample_type *data( ) const 
		{
			if( data_size_ == 0 )
				return NULL;
			else
				return data_;
		}

		int position( ) const 
		{ return position_; }

		void set_position( int position )  
		{ position_ = position; }

		void convert( pcm8 &dst ) const
		{ audio::convert< pcm8, template_< T, B, min_val, max_val > >( dst, *this ); }

		void convert( pcm16 &dst ) const
		{ audio::convert< pcm16, template_< T, B, min_val, max_val > >( dst, *this ); }

		void convert( pcm24 &dst ) const
		{ audio::convert< pcm24, template_< T, B, min_val, max_val > >( dst, *this ); }

		void convert( pcm32 &dst ) const
		{ audio::convert< pcm32, template_< T, B, min_val, max_val > >( dst, *this ); }

		void convert( floats &dst ) const
		{ audio::convert< floats, template_< T, B, min_val, max_val > >( dst, *this ); }

	private:
		void init_data( )
		{
			// 16 bytes padding
			data_size_ = channels_ * samples_ * sizeof( sample_type ) + 16;
			
			if( data_size_ == 0 )
			{
				data_ = NULL;
			}
			else
			{
				if( data_size_ % 16 )
					data_size_ += 16 - ( data_size_ % 16 );

#if defined( OLIB_ON_WINDOWS )
				data_ = reinterpret_cast< sample_type * >( _aligned_malloc( data_size_, 16 ) );
#elif defined( OLIB_ON_MAC )
				data_ = reinterpret_cast< sample_type * >( malloc( data_size_ ) );
#else
				int ret = posix_memalign( reinterpret_cast< void** >( &data_ ), 16, data_size_ );
			   	ARENFORCE_MSG( ret == 0, "Failed to allocate aligned memory. Error = %1%" )( strerror( ret ) )( data_size_ );	
#endif
			}
		}
	
		identity id_;
		int frequency_;
		int channels_;
		int samples_;
		int orig_samples_;
		int position_;
		size_t data_size_;
		sample_type *data_;
};

} } } }

#endif
