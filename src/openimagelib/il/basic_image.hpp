/* -*- tab-width: 4; indent-tabs-mode: t -*- */ 
// il - A image library representation.

// Copyright (C) 2005-2006 VM Inc.
// Released under the LGPL.
// For more information, see http://www.openlibraries.org.

#ifndef BASIC_IMAGE_INC_
#define BASIC_IMAGE_INC_

#include <vector>

#include <openpluginlib/pl/pool.hpp>
#include <boost/shared_ptr.hpp>
#include <cstring>

namespace opl = olib::openpluginlib;

namespace olib { namespace openimagelib { namespace il {

namespace detail
{
	struct Allocate_size
	{
	protected:
		virtual	~Allocate_size( ) { }

	public:
		virtual int operator( )( int block_size, int width, int height, int depth ) const = 0;
	};

	template<typename T>
	struct dds_Allocate_size : public Allocate_size
	{
		virtual int operator( )( int block_size, int width, int height, int depth ) const
		{ return block_size * ( ( width + 3 ) / 4 ) * ( ( height + 3 ) / 4 ) * ( ( depth + 3 ) / 4 ) * sizeof( T ); }
	};
	
	template<typename T>
	struct rgb_Allocate_size : public Allocate_size
	{
		virtual int operator( )( int block_size, int width, int height, int depth ) const
		{ return ( ( block_size * width + 3 ) & -4 ) * height * depth * sizeof( T ); }
	};

}

// The default plane template
template<typename T>
struct default_plane 
{ 
	T offset; 
	T pitch; 
	T width; 
	T height; 
	T linesize;
};

// TODO: Switch to consts
// Clone flags
typedef enum
{
	cropped = 0x1,
	flipped = 0x2,
	flopped = 0x4,
	writable = 0x8
}
clone_flags;

// Field order flags
typedef enum
{
	progressive = 0x0,
	top_field_first = 0x1,
	bottom_field_first = 0x2
}
field_order_flags;

//
template<typename T>
class default_storage
{
public:
	typedef T*		 container;
	typedef T* 		 pointer;
	typedef const T* const_pointer;
	// See note in surface_format if this is changed
	typedef int 	 size_type;

protected:
	default_storage( ) 
		: data_( 0 ) 
		, original_data_( 0 ) 
	{ }

	virtual ~default_storage( )
	{ destroy( ); }
		
	void create( size_type siz ) { 
        original_data_ = ( T * )openpluginlib::pool::realloc( original_data_, siz * sizeof( T ) + 16 );
        size_ = original_data_ ? siz : 0;
        data_ = (T*)( &((char*)original_data_)[ 16 - ((unsigned long long)&original_data_[0]) % 16 ]);
    }
	void destroy( )				 { 
        openpluginlib::pool::free( ( unsigned char * )original_data_ ); 
        original_data_ = 0; 
        data_ = 0; 
        size_ = 0; 
    }

			
public:
	const_pointer data( ) const	 { return data_; }
	pointer data( )				 { return data_; }
	size_type size( ) const	 	 { return size_; }

private:
	container data_;
	container original_data_;
	int size_;
};

//		
template<typename T, class storage = default_storage<T> >
class surface_format : public storage
{
public:
	typedef typename storage::const_pointer const_pointer;
	typedef typename storage::pointer		pointer;
	typedef typename storage::size_type		size_type;
	// If storage::size_type changes, also change it here (required for gcc 3.4.4 apparently)
	typedef default_plane< int > 			plane;
	typedef std::vector< plane >			planes;
	typedef planes::const_iterator			const_iterator;
	typedef planes::iterator				iterator;

protected:
	explicit surface_format( size_type	  block_size,
							 size_type	  width, 
							 size_type	  height, 
							 size_type	  depth, 
							 size_type	  count, 
							 bool		  cubemap, 
							 std::wstring pf )
		: block_size_( block_size )
		, width_( width )
		, height_( height )
		, depth_( depth == 0 ? 1 : depth )
		, count_( count == 0 ? 1 : count )
		, cubemap_( cubemap )
		, volume_( depth != 0 && depth != 1 )
		, pf_( pf )
	{ }

public:			
	virtual ~surface_format( )
	{ }

	virtual surface_format* clone( size_type w, size_type h ) = 0;

protected:
	void allocate( )
	{
		size_type width  = width_;
		size_type height = height_;
		size_type depth  = depth_;
				
		size_type siz = 0;

		for( size_type i = 0; i < count_ && ( width || height ); ++i )
		{
			siz += allocsize( width, height, depth );
						
			width  >>= 1;
			height >>= 1;
			depth  >>= 1;
						
			if( width  == 0 ) width  = 1;
			if( height == 0 ) height = 1;
			if( depth  == 0 ) depth  = 1;
		}
					
		if( cubemap_ ) siz *= 6; // all faces need to be present.

		storage::create( siz );

		allocplanes( planes_ );
	}

public:
	size_type 		width( int index )		const { const plane *p = get_plane( index ); return p ? p->width : 0; }
	size_type 		height( int index )		const { const plane *p = get_plane( index ); return p ? p->height : 0; }
	size_type 		pitch( int index )		const { const plane *p = get_plane( index ); return p ? p->pitch : 0; }
	size_type 		offset( int index )		const { const plane *p = get_plane( index ); return p ? p->offset : 0; }
	size_type 		linesize( int index )	const { const plane *p = get_plane( index ); return p ? p->linesize : 0; }

	size_type		depth( )			const { return depth_; }
	size_type		count( )			const { return count_; }
	size_type		block_size( )		const { return block_size_; }

	iterator		begin( )				  { return planes_.begin( ); }
	const_iterator	begin( )			const { return planes_.begin( ); }
	iterator		end( )					  { return planes_.end( ); }
	const_iterator	end( )				const { return planes_.end( ); }
	bool			empty( )			const { return planes_.empty( ); }
	size_type		plane_count( )		const { return static_cast<int>( planes_.size( ) ); }

public:
	bool			is_cubemap( )		const { return cubemap_; }
	bool			is_volume( )		const { return volume_; }
	std::wstring	pf( )				const { return pf_; }
	
	virtual bool	is_float( )			const { return false; }
	
public:
	virtual size_type allocsize( size_type width, size_type height, size_type depth ) const = 0;
	virtual size_type bitdepth( ) const = 0;

	// Default method that works for most single plane images
	virtual void allocplanes( planes &planes ) 
	{ 
		plane plane = { 0, ( ( width_ * block_size_ + 3 ) & -4 ) * sizeof( T ), width_, height_, width_ * block_size_ * sizeof( T ) }; 
		planes.push_back( plane ); 
	}

	// This is the default that assumes a single plane and a valid block size - it should
	// be overridden in the traits if this criteria is not met. 
	virtual void crop_planes( planes &crop, size_type &x, size_type &y, size_type &w, size_type &h, int flags )
	{
		bool is_flipped = ( flags & flipped ) != 0;
		bool is_flopped = ( flags & flopped ) != 0;

		plane &p = crop[ 0 ];
		p.width = w;
		p.height = h;
		p.linesize = w * block_size_;

		if ( !is_flipped )
			p.offset = p.pitch * y;
		else
			p.offset = ( ( height_ - y - h ) * p.pitch );

		if ( !is_flopped )
			p.offset += x * block_size_;
		else
			p.offset += ( width_ - w - x ) * block_size_;
	}

	// Flop scan which assumes block_size_ is known and <= 4 - associated plane is
	// the first argument, thus allowing a different block/sample size per plane
	virtual void flop_scan_line( size_t, pointer dst, const_pointer src, size_type w ) const
	{
		src += block_size_ * ( w - 1 );
		while( w -- )
		{
			switch( block_size_ )
			{
				case 4:	*dst ++ = *src ++;
				case 3:	*dst ++ = *src ++;
				case 2:	*dst ++ = *src ++;
				case 1:	*dst ++ = *src ++;
			}
			src -= 2 * block_size_;
		}
	}

	// Convenience method to assist image class
	virtual const planes &get_planes( ) 
	{ 
		return planes_; 
	}

	// Get the requested plane
	inline const plane *get_plane( size_t index ) const
	{ 
		return index < planes_.size( ) ? &planes_[ index ] : 0;
	}
	
private:
	size_type 		block_size_;
	size_type 		width_;
	size_type 		height_;
	size_type 		depth_;
	size_type 		count_;
	bool	  		cubemap_;
	bool	  		volume_;
	std::wstring 	pf_;
	planes			planes_;
};

/////////////////////////////////////////////////
//
template<typename T, template<class, class> class structure, template<class> class storage = default_storage>
class image
{
public:
	typedef typename storage<T>::const_pointer				const_pointer;
	typedef typename storage<T>::pointer					pointer;
	typedef typename structure<T, storage<T> >::plane		plane;
	typedef typename structure<T, storage<T> >::planes		planes;
	typedef typename structure<T, storage<T> >::size_type	size_type;

	explicit image( size_type width, 
				    size_type height, 
				    size_type depth, 
				    size_type count		= 1, 
				    bool cubemap		= false )
		: cx_( 0 )
		, cy_( 0 )
		, cw_( width )
		, ch_( height )
		, structure_( new structure<T, storage<T> >( width, height, depth, count, cubemap ) )
		, flipped_( false )
		, flopped_( false )
		, writable_( true )
		, pts_( 0 )
		, position_( 0 )
		, sar_num_( -1 )
		, sar_den_( 1 )
		, field_order_( progressive )
	{ crop_clear( ); }

	// Allocate an image based on the cropped image size but don't populate the image
	template<typename U, template<class, class> class V, template<class> class W>
	image( const image<U, V, W>& other )
		: structure_( new V<U, W<U> >( other.width(), other.height(), other.depth(), other.count(), other.is_cubemap() ) )
		, flipped_( other.is_flipped( ) )
		, flopped_( other.is_flopped( ) )
		, writable_( true )
		, pts_( other.pts( ) )
		, position_( other.position( ) )
		, sar_num_( -1 )
		, sar_den_( 1 )
		, field_order_( other.field_order( ) )
	{ crop_clear( ); }

private:
	// Allocate and populate an image either by cropped or non-cropped size
	template<typename U, template<class, class> class V, template<class> class W>
	image( const image<U, V, W>& other, int flags )
		: structure_( other.structure_->clone( other.width( 0, flags & cropped ), other.height( 0, flags & cropped ) ) )
		, flipped_( ( flags & flipped ) != 0 )
		, flopped_( ( flags & flopped ) != 0 )
		, writable_( true )
		, pts_( other.pts( ) )
		, position_( other.position( ) )
		, sar_num_( -1 )
		, sar_den_( 1 )
		, field_order_( other.field_order( ) )
	{
		// We need to initialise the new instance crop info - this ignores
		// the crop info on other since our new image won't inherit that info
		crop_clear( );

		if ( other.matching( flags ) )
		{
			// Copy the data in a single shot (optimisation for full image copy)
			memcpy( data( ), other.structure_->data( ), structure_->size( ) );
		}
		else
		{
			// Extract specific requirements from the flags
			bool need_flip = is_flipped( ) != other.is_flipped( );
			bool need_flop = is_flopped( ) != other.is_flopped( );

			// Obtain the number of planes and iterate through each
			size_t count = structure_->plane_count( );

			for ( size_t i = 0; i < count; i ++ )
			{
				// We need the src and pitch of the cropped source plane
				const_pointer src = other.data( i );
				size_type src_pitch = other.pitch( i );

				// The destination plane, pitch and height
				pointer dst = data( i );
				size_type dst_width = width( i );
				size_type dst_pitch = pitch( i );
				size_type dst_scan = linesize( i );
				size_type dst_height = height( i );

				// We need to orient the dest correctly
				if ( need_flip )
				{
					dst += dst_pitch * ( dst_height - 1 );
					dst_pitch = - dst_pitch;
				}

				// Now copy each scan line in the plane
				while( dst_height -- )
				{
					if ( need_flop )
						structure_->flop_scan_line( i, dst, src, dst_width );
					else
						memcpy( dst, src, dst_scan );
					dst += dst_pitch;
					src += src_pitch;
				}
			}
		}
	}
			
public:
	image *clone( int flags = cropped )
	{
		return new image( *this, flags );
	}

	bool matching( int flags ) const
	{
		bool match_writable = ( flags & writable ) != 0 ? is_writable( ) : true;
		bool want_crop = ( flags & cropped ) != 0;
		bool want_flip = ( flags & flipped ) != 0;
		bool want_flop = ( flags & flopped ) != 0;
		return match_writable && ( want_flip == is_flipped( ) ) && ( want_flop == is_flopped( ) ) && ( !is_cropped( ) || !want_crop );
	}

	size_type plane_count( ) const
	{
		return structure_->plane_count( );
	}

	size_type width( size_t index = 0, bool crop = true ) const 
	{ 
		const plane *p = get_plane( index, crop ); 
		return p ? p->width : 0; 
	}

	size_type height( size_t index = 0, bool crop = true ) const 
	{ 
		const plane *p = get_plane( index, crop ); 
		return p ? p->height : 0; 
	}

	size_type pitch( size_t index = 0, bool crop = true ) const 
	{ 
		const plane *p = get_plane( index, crop ); 
		return p ? p->pitch : 0; 
	}

	size_type offset( size_t index = 0, bool crop = true ) const 
	{ 
		const plane *p = get_plane( index, crop ); 
		return p ? p->offset : 0; 
	}

	size_type linesize( size_t index = 0, bool crop = true ) const 
	{ 
		const plane *p = get_plane( index, crop ); 
		return p ? p->linesize : 0; 
	}

	size_type	get_crop_x( ) const { return cx_; }
	size_type	get_crop_y( ) const { return cy_; }
	size_type	get_crop_w( ) const { return cw_; }
	size_type	get_crop_h( ) const { return ch_; }

	const_pointer data( size_t index = 0, bool crop = true ) const	 
	{
		const plane *p = get_plane( index, crop );
		return structure_->data( ) + ( p ? p->offset : 0 );
	}

	pointer data( size_t index = 0, bool crop = true )
	{
		const plane *p = get_plane( index, crop );
		return structure_->data( ) + ( p ? p->offset : 0 );
	}

	std::string bits( size_t index = 0, bool crop = true ) { return std::string( ( char* ) data( index, crop ), size( ) ); }
	
	size_type	depth( )		const { return structure_->depth( ); }
	size_type	count( )		const { return structure_->count( ); }
	size_type	block_size( )	const { return structure_->block_size( ); }
	size_type	bitdepth( )		const { return structure_->bitdepth( ); }

	bool is_flipped( ) 			const { return flipped_; }
	void set_flipped( bool flipped )  { flipped_ = flipped; crop_sync( ); }

	bool is_flopped( ) 			const { return flopped_; }
	void set_flopped( bool flopped )  { flopped_ = flopped; crop_sync( ); }
	
	bool is_writable( ) 		const { return writable_; }
	void set_writable( bool writable ){ writable_ = writable; }
	
	double pts( )				const { return pts_; }
	void set_pts( double pts )		  { pts_ = pts; }

	int position( )				const { return position_; }
	void set_position( int position ) { position_ = position; }

	int get_sar_num( )			const { return sar_num_; }
	int get_sar_den( )			const { return sar_den_; }

	void set_sar_num( int sar_num )	  { sar_num_ = sar_num; }
	void set_sar_den( int sar_den )	  { sar_den_ = sar_den; }

	field_order_flags field_order( ) const { return field_order_; }
	void set_field_order( field_order_flags flags ) { field_order_ = flags; }

public:
	bool			is_cubemap( )	const { return structure_->is_cubemap( ); }
	bool			is_volume( )	const { return structure_->is_volume( ); }
	bool			is_float( )		const { return structure_->is_float( ); }
	std::wstring	pf( )			const { return structure_->pf( ); }

public:
	// This returns the size of the full image
	size_type size( ) const		 { return structure_->size( ); }

public:
	size_type allocsize( size_type width, size_type height, size_type depth ) const
	{ return structure_->allocsize( width, height, depth ); }

	// Crop an image 
	bool crop( size_type x, size_type y, size_type w, size_type h, bool crop = true )
	{
		// Get the original width and height
		int ow = width( 0, crop );
		int oh = height( 0, crop );

		// Ensure the specified geometry is valid
		bool valid = x >= 0 && y >=0 && x < ow && y < oh && x + w <= ow && y + h <= oh;

		if ( valid )
		{
			// Need these flags to ensure that the plane crop is correct
			int flags = 0;
			flags |= is_flipped( ) ? flipped : 0;
			flags |= is_flopped( ) ? flopped : 0;

			// If we're cropping a crop, increment the x,y by the previous coords
			if ( crop )
			{
				x += cx_;
				y += cy_;
			}

			// Return to the storage planes
			crop_clear( );

			// Invoke the virtual method for cropping the planes - note that the 
			// geometry may change to meet the requirements of a colour space (for 
			// example, ensuring that x falls on an even pixel)
			structure_->crop_planes( crop_, x, y, w, h, flags );

			// Store the new geometry of the crop
			cx_ = x;
			cy_ = y;
			cw_ = w;
			ch_ = h;
		}

		return valid;
	}

	// Clear the cropped image by returning everything back to the storage planes
	void crop_clear( )
	{
		crop_.clear( );
		std::copy(structure_->begin( ), structure_->end( ),std::back_inserter(crop_));
		cx_ = cy_ = 0;
		cw_ = width( );
		ch_ = height( );
	}

	// Determines if the image is cropped or not
	bool is_cropped( ) const
	{
		return cx_ != 0 || cy_ != 0 || cw_ != width( 0, false ) || ch_ != height( 0, false );
	}

private:
	// Get the planes to use
	inline const planes &get_planes( bool crop ) const
	{ 
		return crop ? crop_ : structure_->get_planes( ); 
	}

	// Get the plane requested
	inline const plane *get_plane( size_t index, bool crop ) const
	{ 
		const planes &planes = get_planes( crop ); 
		return index < planes.size( ) ? &planes[ index ] : 0;
	}

	// Synchronise the crop information (typically used after orientation changes)
	void crop_sync( )
	{
		crop( cx_, cy_, cw_, ch_, false );
	}

private:
	size_type cx_;
	size_type cy_;
	size_type cw_;
	size_type ch_;
	boost::shared_ptr<structure<T, storage<T> > > structure_;
	planes crop_;
	bool flipped_;
	bool flopped_;
	bool writable_;
	double pts_;
	int position_;

	//Sample aspect ratio
	int sar_num_;
	int sar_den_;

	field_order_flags field_order_;
};

typedef image<unsigned char, surface_format> image_type;
typedef boost::shared_ptr<image_type>		 image_type_ptr;

} } }

#endif
