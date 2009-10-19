
// ml::audio - template for all audio types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_AUDIO_TEMPLATE_H_
#define AML_AUDIO_TEMPLATE_H_

#include <openmedialib/ml/types.hpp>
#include <string.h>
#include <string>

namespace olib { namespace openmedialib { namespace ml { namespace audio {

// The template which provides all the types
template< typename T, identity B >
class ML_DECLSPEC template_ : public interface
{
	public:
		template_( int frequency, int channels, int samples )
			: id_( B )
			, frequency_( frequency )
			, channels_( channels )
			, samples_( samples )
			, position_( 0 )
		{
			data_ = boost::shared_ptr< std::vector< T > >( new std::vector< T >( channels_ * samples_ ) );
			memset( data( ), 0, size( ) );
		}

		template_( const interface &other )
			: id_( B )
			, frequency_( other.frequency( ) )
			, channels_( other.channels( ) )
			, samples_( other.samples( ) )
			, position_( other.position( ) )
		{
			data_ = boost::shared_ptr< std::vector< T > >( new std::vector< T >( channels_ * samples_ ) );
			other.convert( *this );
		}

		virtual ~template_( )
		{ }

		audio_type_ptr clone( ) const
		{ return audio_type_ptr( new template_< T, B >( *this ) ); }

		identity id( ) const 
		{ return id_; }

		int sample_size( ) const
		{ return sizeof( T ); }

		int size( ) const
		{ return sizeof( T ) * samples_ * channels_; }

		int frequency( ) const 
		{ return frequency_; }

		int channels( ) const 
		{ return channels_; }

		int samples( ) const 
		{ return samples_; }

		std::wstring af( )
		{ return id_to_af( id_ ); }

		void *pointer( ) const
		{ return ( void * )&( *data_ )[ 0 ]; }

		T *data( ) const 
		{ return &( *data_ )[ 0 ]; }

		int position( ) const 
		{ return position_; }

		void set_position( int position )  
		{ position_ = position; }

		void convert( pcm8 &dst ) const
		{ audio::convert( dst, *this ); }

		void convert( pcm16 &dst ) const
		{ audio::convert( dst, *this ); }

		void convert( pcm24 &dst ) const
		{ audio::convert( dst, *this ); }

		void convert( pcm32 &dst ) const
		{ audio::convert( dst, *this ); }

		void convert( floats &dst ) const
		{ audio::convert( dst, *this ); }

	private:
		identity id_;
		int frequency_;
		int channels_;
		int samples_;
		int position_;
		boost::shared_ptr < std::vector< T > > data_;
};

} } } }

#endif
