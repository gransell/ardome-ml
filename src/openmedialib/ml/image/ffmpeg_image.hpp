#ifndef FFMPEG_IMAGE_H_
#define FFMPEG_IMAGE_H_

#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/assert_defines.hpp>

#include <openmedialib/ml/image/image_interface.hpp>
#include <openmedialib/ml/image/ffmpeg_utility.hpp>

#include <opencorelib/cl/log_defines.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace image {

template<typename T>
class ML_DECLSPEC ffmpeg_image : public image
{
public:
	typedef T 						data_type;
	typedef default_plane 			plane;
	typedef std::vector< plane > 	planes;
	typedef planes::const_iterator	const_iterator;
	typedef planes::iterator		iterator;

	ffmpeg_image ( MLPixelFormat MLpixfmt, int w, int h )
	: MLpixfmt_ ( MLpixfmt )
	, AVpixfmt_( ML_to_AV( MLpixfmt ) )
	, cx_( 0 )
	, cy_( 0 )
	, cw_( w )
	, ch_( h )
	, width_ ( w )
	, height_ ( h )
	, writable_( true )
	, position_( 0 )
	, sar_num_( -1 )
	, sar_den_( 1 )
	, field_order_( progressive )
	, size_( 0 )
	{
		ARENFORCE(AVpixfmt_ != -1);
		allocate( );
		crop_clear( );
	}

	~ffmpeg_image( ) { deallocate ( ); }
private:
	ffmpeg_image( ffmpeg_image& other, int flags )
	: MLpixfmt_ ( other.ml_pixel_format( ) )
	, AVpixfmt_ ( other.av_pixel_format( ) )
	, width_ ( other.width( 0, flags & cropped ) )
	, height_ ( other.height( 0, flags & cropped ) )
	, writable_ ( true )
	, position_ ( other.position( ) )
	, sar_num_ ( -1 )
	, sar_den_ ( 1 )
	, field_order_( other.field_order( ) )
	, size_( 0 )
	{
		allocate( );

		crop_clear( );

		// Obtain the number of planes and iterate through each
		size_t count = plane_count( );

		for ( size_t i = 0; i < count; i ++ )
		{
			// We need the src and pitch of the cropped source plane
			data_type *src = other.data( i );
			int src_pitch = other.pitch( i ) / sizeof( data_type );

			// The destination plane, pitch and height
			data_type *dst = data( i );
			int dst_pitch = pitch( i ) / sizeof( data_type );
			int dst_scan = linesize( i );
			int dst_height = height( i );

			// Now copy each scan line in the plane
			while( dst_height -- )
			{
				memcpy( dst, src, dst_scan );
				dst += dst_pitch;
				src += src_pitch;
			}
		}
	}

public:
	ffmpeg_image *clone( int flags = cropped )
	{
		return new ffmpeg_image( *this, flags );
	}

	bool matching( int flags ) const
	{
		bool match_writable = ( flags & writable ) != 0 ? is_writable( ) : true;
		bool want_crop = ( flags & cropped ) != 0;
		return match_writable && ( !is_cropped( ) || !want_crop );
	}
	
	MLPixelFormat ml_pixel_format( ) { return MLpixfmt_; }
	int av_pixel_format( ) { return AVpixfmt_; }

	int width( size_t index = 0, bool crop = true ) const
	{
		return get_plane( index, crop )->width;
	}

	int height( size_t index = 0, bool crop = true ) const
	{
		return get_plane( index, crop )->height;
	}

	int plane_count ( )
	{
		return utility_plane_count( AVpixfmt_ );
	}

	int bitdepth( size_t index = 0 )
	{
		return utility_bitdepth( AVpixfmt_, index );
	}

	int storage_bytes( ) const
	{
		return sizeof( data_type );
	}

	int linesize( size_t index = 0, bool crop = true )
	{
		return get_plane( index, crop )->linesize;
	}

	int pitch( size_t index = 0, bool crop = true )
	{
		return get_plane( index, crop )->pitch;
	}

	int offset( size_t index = 0, bool crop = true )
	{
		return get_plane( index, crop )->offset;
	}

	int num_components( ) const
	{
		return utility_nb_components( AVpixfmt_ );
	}

	int get_crop_x( ) const { return cx_; }
	int get_crop_y( ) const { return cy_; }
	int get_crop_w( ) const { return cw_; }
	int get_crop_h( ) const { return ch_; }

	iterator		begin( )				  { return planes_.begin( ); }
	const_iterator  begin( )			const { return planes_.begin( ); }
	iterator		end( )					{ return planes_.end( ); }
	const_iterator  end( )			  const { return planes_.end( ); }
	bool			empty( )			const { return planes_.empty( ); }
	
	data_type *data( size_t index = 0, bool crop = true )
	{
		const int plane_offset = offset( index, crop );
		return reinterpret_cast< data_type * >( pic_data_[ 0 ] + plane_offset );
	}

	void *ptr( size_t index = 0, bool crop = true )
	{
		return static_cast< void * >( data( index, crop ) );
	}

	t_string pf( ) const 
	{
		std::map<t_string, MLPixelFormat>::const_iterator it;
		t_string key = _CT("");
		for (it = MLPixelFormatMap.begin(); it != MLPixelFormatMap.end(); ++it)
		{
			if (it->second == MLpixfmt_)
			{
				key = it->first;
				break;
			}
		}

		return key;
	}

	bool is_writable( )		 const { return writable_; }
	void set_writable( bool writable ){ writable_ = writable; }

	int position( )			 const { return position_; }
	void set_position( int position ) { position_ = position; }

	int get_sar_num( )		  const { return sar_num_; }
	int get_sar_den( )		  const { return sar_den_; }

	void set_sar_num( int sar_num )   { sar_num_ = sar_num; }
	void set_sar_den( int sar_den )   { sar_den_ = sar_den; }

	field_order_flags field_order( ) const { return field_order_; }
	void set_field_order( field_order_flags flags ) { field_order_ = flags; }

	bool is_yuv_planar( ) 
	{
		return is_pixfmt_planar( AVpixfmt_ );
	}

	int size( ) const { return size_; }

	// Crop an image
	bool crop( int x, int y, int w, int h, bool crop = true )
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
			crop_planes( crop_, x, y, w, h, flags );

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
		std::copy( planes_.begin( ), planes_.end( ), std::back_inserter( crop_ ) );
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
	MLPixelFormat MLpixfmt_;
	int AVpixfmt_;

	uint8_t *pic_data_[ 4 ];

	int cx_;
	int cy_;
	int cw_;
	int ch_;
	int width_;
	int height_;
	int chroma_w_;
	int chroma_h_;
	bool writable_;
	int position_;

	//Sample aspect ratio
	int sar_num_;
	int sar_den_;

	field_order_flags field_order_;
	planes planes_;
	planes crop_;
	int size_;

	 // Get the requested plane
	inline const plane *get_plane( size_t index, bool crop ) const
	{
		const planes &planes = get_planes( crop ); 
		ARENFORCE_MSG( index < planes.size( ), "Request for a non-existent plane" );
		return &planes[ index ];
	}


	// Synchronise the crop information (typically used after orientation changes)
	void crop_sync( )
	{
		crop( cx_, cy_, cw_, ch_, false );
	}

	// Convenience method to assist image class
	virtual const planes &get_planes( bool crop ) const
	{ 
		return crop ? crop_ : planes_ ; 
	}
	
	virtual void crop_planes( planes &crop, int &x, int &y, int &w, int &h, int flags )
	{
		if ( is_pixfmt_rgb( AVpixfmt_ ) )
			crop_planes_rgb( crop, x, y, w, h, flags );
		else
			crop_planes_yuv( crop, x, y, w, h, flags );

	}
	
	virtual void crop_planes_rgb( planes &crop, int &x, int &y, int &w, int &h, int flags )
	{
		plane &p = crop[ 0 ];
		p.width = w;
		p.height = h;
		p.linesize = w * sizeof( data_type );
		p.offset = p.pitch * y + x * sizeof( data_type );
	}
	
	virtual void crop_planes_yuv( planes &crop, int &x, int &y, int &w, int &h, int flags )
	{
		for ( int i = 0; i < plane_count( ); i++ )
		{
			int shift_x = ( i > 0 ? chroma_w_: 0 );
			int shift_y = ( i > 0 ? chroma_h_: 0 );
			plane &p = crop[ i ];
			p.width = w >> shift_x;
			p.height = h >> shift_y;
			p.linesize = p.width;
			p.offset = ( p.pitch * ( y >> shift_y ) ) ;
			p.offset += ( x >> shift_x );
			p.offset = offset( i, false ) + p.offset;
		}
	}

protected:

	void allocate( )
	{
		int pic_linesize[ 4 ];

		size_ = utility_av_image_alloc( pic_data_, pic_linesize, width_, height_, AVpixfmt_, alignment( ) );

		ARENFORCE_MSG( size_ >= 0, "Unable to alloctate %1% at %2% x %3%" )( AVpixfmt_ )( width_ )( height_ );

		utility_av_pix_fmt_get_chroma_sub_sample( AVpixfmt_, &chroma_w_, &chroma_h_ );

		for ( int i = 0; i < 4; i ++ )
		{
			if ( pic_data_[ i ] )
			{
				int chroma_w = i == 1 || i == 2 ? chroma_w_ : 0;
				int chroma_h = i == 1 || i == 2 ? chroma_h_ : 0;
				plane plane;
				plane.offset = int( pic_data_[ i ] - pic_data_[ 0 ] );
				plane.pitch = pic_linesize[ i ];
				plane.linesize = utility_av_image_get_linesize( AVpixfmt_, width_, i );
				plane.width = width_ >> chroma_w;
				plane.height = height_ >> chroma_h;
				planes_.push_back( plane );
			}
			else
			{
				break;
			}
		}
	}

	void deallocate( )
	{
		utility_av_free( pic_data_[ 0 ] );
	}

};

} } } }

#endif

