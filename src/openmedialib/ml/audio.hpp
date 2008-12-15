
// ml - A media library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef OPENMEDIALIB_AUDIO_INC_
#define OPENMEDIALIB_AUDIO_INC_

#include <openmedialib/ml/config.hpp>
#include <openimagelib/il/basic_image.hpp>
#include <boost/shared_ptr.hpp>

namespace olib { namespace openmedialib { namespace ml {
	
// supported audio formats
const std::wstring AUDIO_FORMAT_PCM16 = L"pcm16";

template<typename T, class storage = olib::openimagelib::il::default_storage<T> >
class audio_format : public storage
{
public:
	typedef typename storage::const_pointer const_pointer;
	typedef typename storage::pointer		pointer;
	typedef typename storage::size_type		size_type;

protected:
	explicit audio_format( size_type frequency,
						   size_type channels,
						   size_type samples,
						   std::wstring af )
		: frequency_( frequency )
		, channels_( channels )
		, samples_( samples )
		, af_( af )
	{ }

public:			
	virtual ~audio_format( )
	{ }

	virtual audio_format *clone( ) = 0;

protected:
	void allocate( )
	{
		size_type siz = allocsize( );
		storage::create( siz );
	}

public:
	size_type 		frequency( )		const { return frequency_; }
	size_type 		channels( )			const { return channels_; }
	size_type 		samples( )			const { return samples_; }
	std::wstring	af( )				const { return af_; } 

public:
	virtual int sample_size( ) const = 0;
	virtual size_type allocsize( ) const = 0;

private:
	size_type		frequency_;
	size_type		channels_;
	size_type		samples_;
	std::wstring 	af_;
};

template<typename T, template<class, class> class structure, template<class> class storage = olib::openimagelib::il::default_storage>
class audio
{
public:
	typedef typename storage<T>::const_pointer				const_pointer;
	typedef typename storage<T>::pointer					pointer;
	typedef typename structure<T, storage<T> >::size_type	size_type;

	explicit audio( size_type frequency,
					size_type channels,
					size_type samples )
		: structure_( new structure<T, storage<T> >( frequency, channels, samples ) )
		, pts_( 0 )
		, position_( 0 )
		, som_(0)
		, eom_(structure_->samples())
	{ }

	// Allocate an audio based on the audio size but don't populate
	template<typename U, template<class, class> class V, template<class> class W>
	audio( const audio<U, V, W>& other )
		: structure_( new V<U, W<U> >( other.frequency( ), other.channels( ), other.samples( ) ) )
		, pts_( other.pts( ) )
		, position_( other.position( ) )
		, som_(0)
		, eom_(structure_->size( ) / (structure_->sample_size( ) * structure_->channels( )))
	{ memcpy( data( ), other.data( ), structure_->size( ) ); }

public:
	audio *clone( )
	{
		return new audio( *this );
	}
	
	bool is_cropped()	const	{ return (som_ != 0) || (eom_ != (size_type)(structure_->size( ) / (structure_->sample_size( ) * structure_->channels( )))); }

	size_type 		frequency( ) const { return structure_->frequency( ); }
	size_type 		channels( )	 const { return structure_->channels( ); }
	size_type 		samples( )	 const { return is_cropped() ? (eom_ - som_) : structure_->samples( ); }
	std::wstring	af( )		 const { return structure_->af( ); }

	const_pointer 	data( ) 	 const { return is_cropped() ? structure_->data( ) + ( som_ * structure_->sample_size( ) * structure_->channels( ) ) : structure_->data( ); }
	pointer 		data( ) 	 	   { return is_cropped() ? structure_->data( ) + ( som_ * structure_->sample_size( ) * structure_->channels( ) ) : structure_->data( ); }

	double pts( )				 const { return pts_; }
	void set_pts( double pts )		   { pts_ = pts; }

	int position( )				 const { return position_; }
	void set_position( int position )  { position_ = position; }

	size_type size( ) const		 { return structure_->size( ); }
	
	void crop_clear()
	{
		som_ = 0;
		eom_ = structure_->size( ) / (structure_->sample_size( ) * structure_->channels( ));
	}
	
	bool crop(size_type som, size_type eom)
	{
		if(is_cropped())
		{
			som += som_;
			eom += som_;
		}
		
		// Check to make sure in & out offsets don't go out of bounds
		if(		(som > structure_->size( ) / (size_type)(structure_->sample_size( ) * structure_->channels( )))
			||	(eom > structure_->size( ) / (size_type)(structure_->sample_size( ) * structure_->channels( ))) )
			return false;
		
		// Check to make sure in point is before out point
		if(som >= eom)
			return false;
		
		som_ = som;
		eom_ = eom;
		
		if(position_ < som_)
			position_ = som_;
		else if(position_ >= eom_)
			position_ = eom_ - 1;
		
		return true;
	}
	
	size_type get_som() const	{ return som_; }
	size_type get_eom() const	{ return eom_; }

public:
	size_type allocsize( ) const { return structure_->allocsize( ); }

private:
	boost::shared_ptr<structure<T, storage<T> > > structure_;
	double pts_;
	int position_;
	size_type som_;
	size_type eom_;
};

typedef ML_DECLSPEC audio<unsigned char, audio_format>	  audio_type;
typedef ML_DECLSPEC boost::shared_ptr<audio_type> audio_type_ptr;

template<typename T, class storage = olib::openimagelib::il::default_storage<T> >
class pcm16 : public audio_format<T, storage>
{
public:
	typedef typename storage::const_pointer					const_pointer;
	typedef typename storage::pointer						pointer;
	typedef typename audio_format<T, storage>::size_type	size_type;

private:
	static const int sample_size_ = 2;

public:
	explicit pcm16(	size_type frequency,
					size_type channels,
					size_type samples )
		: audio_format<T, storage>( frequency, channels, samples, AUDIO_FORMAT_PCM16 )
	{ audio_format<T, storage>::allocate( ); }

	pcm16( const pcm16& other )
		: audio_format<T, storage>( other.frequency( ), other.channels( ), other.samples( ), AUDIO_FORMAT_PCM16 )
	{ audio_format<T, storage>::allocate( ); }

	virtual ~pcm16( )
	{ }

public:
	virtual int sample_size( ) const
	{ return sample_size_; }

	virtual size_type allocsize( ) const
	{ return sample_size_ * audio_format<T, storage>::samples( ) * audio_format<T, storage>::channels( ); }
	
	virtual pcm16* clone( )
	{ return new pcm16( *this ); }
};

} } }

#endif
