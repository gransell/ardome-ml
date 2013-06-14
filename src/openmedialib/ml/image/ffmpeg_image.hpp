#ifndef FFMPEG_IMAGE_H_
#define FFMPEG_IMAGE_H_

#include <opencorelib/cl/enforce_defines.hpp>

#include <openmedialib/ml/image/image_interface.hpp>
#include <openmedialib/ml/image/ffmpeg_utility.hpp>

#include <opencorelib/cl/log_defines.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace image {

template<typename T>
class ML_DECLSPEC ffmpeg_image : public image
{
public:
    typedef T data_type;
    typedef default_plane 					plane;
    typedef std::vector< plane > 			planes;
    typedef planes::const_iterator          const_iterator;
    typedef planes::iterator                iterator;
    ffmpeg_image ( MLPixelFormat MLpixfmt, int w, int h )
    : MLpixfmt_ ( MLpixfmt )
    , cx_( 0 )
    , cy_( 0 )
    , cw_( w )
    , ch_( h )
    , width_ ( w )
    , height_ ( h )
	, flipped_( false )
	, flopped_( false )
    , writable_( true )
    , pts_( 0 )
    , position_( 0 )
    , sar_num_( -1 )
    , sar_den_( 1 )
    , field_order_( progressive )
    {
        AVpixfmt_ = ML_to_AV(MLpixfmt_);
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
		, flipped_( ( flags & flipped ) != 0 )
		, flopped_( ( flags & flopped ) != 0 )
        , pts_ ( other.pts( ) )
        , position_ ( other.position( ) )
        , sar_num_ ( -1 )
        , sar_den_ ( 1 )
        , field_order_( other.field_order( ) )
    {
        allocate( );
        //memcpy( data( ), other.data( ), other.size( ));

		crop_clear( );

	
		// Extract specific requirements from the flags
		bool need_flip = is_flipped( ) != other.is_flipped( );
		bool need_flop = is_flopped( ) != other.is_flopped( );

		// Obtain the number of planes and iterate through each
		size_t count = plane_count( );

		for ( size_t i = 0; i < count; i ++ )
		{
			// We need the src and pitch of the cropped source plane
            data_type *src = other.data( i );
			int src_pitch = other.pitch( i );

			// The destination plane, pitch and height
            data_type *dst = data( i );
			int dst_width = width( i );
			int dst_pitch = pitch( i );
			int dst_scan = linesize( i );
			int dst_height = height( i );

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
					flop_scan_line( i, dst, src, dst_width );
				else
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
		bool want_flip = ( flags & flipped ) != 0;
		bool want_flop = ( flags & flopped ) != 0;
		return match_writable && ( want_flip == is_flipped( ) ) && ( want_flop == is_flopped( ) ) && ( !is_cropped( ) || !want_crop );
	}
	
	MLPixelFormat ml_pixel_format( ) { return MLpixfmt_; }
	int av_pixel_format( ) { return AVpixfmt_; }

    int width( size_t index = 0, bool crop = true ) const
    {
        const plane *p = get_plane( index, crop );
        return p ? p->width : 0;
    }

    int height( size_t index = 0, bool crop = true ) const
    {
        const plane *p = get_plane( index, crop );
        return p ? p->height : 0;
    }

    int plane_count ( )
    {
        return utility_plane_count( AVpixfmt_ );
    }

    int bitdepth( size_t index = 0 )
    {
        return utility_bitdepth( AVpixfmt_, index );
    }

    int linesize( size_t index = 0, bool crop = true )
    {
        const plane *p = get_plane( index, crop );
        return p ? p->linesize : 0;
    }

    int pitch( size_t index = 0, bool crop = true )
    {
        const plane *p = get_plane( index, crop );
        return p ? p->pitch : 0;
    }

    int offset( size_t index = 0, bool crop = true )
    {
        const plane *p = get_plane( index, crop );
        return p ? p->offset : 0;
    }

    int block_size( ) const
    {
        return utility_nb_components( AVpixfmt_ );
    }

    int get_crop_x( ) const { return cx_; }
    int get_crop_y( ) const { return cy_; }
    int get_crop_w( ) const { return cw_; }
    int get_crop_h( ) const { return ch_; }

	int 			count( ) 			const { return 1; }
	iterator        begin( )                  { return planes_.begin( ); }
    const_iterator  begin( )            const { return planes_.begin( ); }
    iterator        end( )                    { return planes_.end( ); }
    const_iterator  end( )              const { return planes_.end( ); }
    bool            empty( )            const { return planes_.empty( ); }
	
    data_type *data( size_t index = 0, bool crop = true )
    {
        return data_ + offset( index, crop );
    }

	void *ptr( size_t index = 0, bool crop = true )
	{
		return static_cast< void * >( data( index, crop ) );
	}

    int depth( ) { return 1; }

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

    bool is_flipped( )          const { return flipped_; }
    void set_flipped( bool flipped )  { flipped_ = flipped; crop_sync( ); }

    bool is_flopped( )          const { return flopped_; }
    void set_flopped( bool flopped )  { flopped_ = flopped; crop_sync( ); }

    bool is_writable( )         const { return writable_; }
    void set_writable( bool writable ){ writable_ = writable; }

    double pts( )               const { return pts_; }
    void set_pts( double pts )        { pts_ = pts; }

    int position( )             const { return position_; }
    void set_position( int position ) { position_ = position; }

    int get_sar_num( )          const { return sar_num_; }
    int get_sar_den( )          const { return sar_den_; }

    void set_sar_num( int sar_num )   { sar_num_ = sar_num; }
    void set_sar_den( int sar_den )   { sar_den_ = sar_den; }

    field_order_flags field_order( ) const { return field_order_; }
    void set_field_order( field_order_flags flags ) { field_order_ = flags; }

	bool is_yuv_planar( ) 
	{
		return is_pixfmt_planar( AVpixfmt_ );
	}

	int size( ) const { return utility_avpicture_get_size(AVpixfmt_, width_,height_); }

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
    int cx_;
    int cy_;
    int cw_;
    int ch_;
    int width_;
    int height_;
    int chroma_w_;
    int chroma_h_;
    data_type *data_;
    bool flipped_;
    bool flopped_;
    bool writable_;
    double pts_;
    int position_;

    //Sample aspect ratio
    int sar_num_;
    int sar_den_;

    field_order_flags field_order_;
    UtilityAVPixFmtDescriptor *desc_;
    int AVpixfmt_;
    planes planes_;
	planes crop_;

     // Get the requested plane
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

	// Convenience method to assist image class
	virtual const planes &get_planes( bool crop ) const
	{ 
		return crop ? crop_ : planes_ ; 
	}

	
	virtual void crop_planes( planes &crop, int &x, int &y, int &w, int &h, int flags )
	{
     
        if ( is_pixfmt_rgb( AVpixfmt_ ) )
			crop_planes_rgb( crop, x, y, w, h, flags );
		else if (MLpixfmt_ == ML_PIX_FMT_UYYVYY411)
			crop_planes_411_packed( crop, x, y, w, h, flags );
		else
			crop_planes_yuv( crop, x, y, w, h, flags );
           

	}
	
	virtual void crop_planes_rgb( planes &crop, int &x, int &y, int &w, int &h, int flags )
	{
		bool is_flipped = ( flags & flipped ) != 0;
		bool is_flopped = ( flags & flopped ) != 0;

		plane &p = crop[ 0 ];
		p.width = w;
		p.height = h;
		p.linesize = w * block_size( );

		if ( !is_flipped )
			p.offset = p.pitch * y;
		else
			p.offset = ( ( height_ - y - h ) * p.pitch );

		if ( !is_flopped )
			p.offset += x * block_size( );
		else
			p.offset += ( width_ - w - x ) * block_size( );
	}
	
	virtual void crop_planes_yuv( planes &crop, int &x, int &y, int &w, int &h, int flags )
	{
		bool is_flipped = ( flags & flipped ) != 0;
		bool is_flopped = ( flags & flopped ) != 0;

		for ( int i = 0; i < plane_count( ); i++ )
		//for ( int i = 0; i < 1; i++ )
		{
			int shift_x = ( i > 0 ? chroma_w_: 0 );
			int shift_y = ( i > 0 ? chroma_h_: 0 );

			plane &p = crop[ i ];
			p.width = w >> shift_x;
			p.height = h >> shift_y;
			//p.linesize = w;
			p.linesize = p.width;
			
			//p.linesize = av_image_get_linesize( pixfmt_, p.width, i );

			if ( !is_flipped )
				p.offset = ( p.pitch * ( y >> shift_y ) ) ;
			else
				p.offset = ( ( ( height_ - y - h ) * p.pitch ) >> shift_y ) ;

			if ( !is_flopped )
				p.offset += ( x >> shift_x );
			else
				p.offset += ( (width_ - w - x) >> shift_x ) ;

			p.offset = offset( i, false ) + p.offset;
		}
	}

	virtual void crop_planes_411_packed( planes &crop, int &x, int &y, int &w, int &h, int flags )
	{
		bool is_flipped = ( flags & flipped ) != 0;
		bool is_flopped = ( flags & flopped ) != 0;

		plane &p = crop[ 0 ];
		p.width = w;
		p.height = h;
		p.linesize = w * 3 / 2;

		if ( !is_flipped )
			p.offset = p.pitch * y;
		else
			p.offset = ( ( height_ - y - h ) * p.pitch );

		if ( !is_flopped )
			p.offset += x * 3 / 2;
		else
			p.offset += ( width_ - w - x ) * 3 / 2;
	}


	// Flop scan which assumes block_size_ is known and <= 4 - associated plane is
	// the first argument, thus allowing a different block/sample size per plane
	virtual void flop_scan_line( size_t, data_type *dst, const data_type *src, int w ) const
	{


		int bytes_to_copy = 1;		// yuv, with planes
        if ( is_pixfmt_rgb( AVpixfmt_ ) ) {
			bytes_to_copy = block_size( );
        } else if (AVpixfmt_ == ML_PIX_FMT_UYYVYY411) {
			w /= 4;		// amr: why?
			bytes_to_copy = 6;
		}
		
		src += w - bytes_to_copy;
		while( w -- )
		{
			switch( bytes_to_copy )
			{
				case 6:	*dst ++ = *src ++;
				case 5:	*dst ++ = *src ++;
				case 4:	*dst ++ = *src ++;
				case 3:	*dst ++ = *src ++;
				case 2:	*dst ++ = *src ++;
				case 1:	*dst ++ = *src ++;
			}

			src -= 2 * bytes_to_copy;
		}
	}

protected:

    void allocate( )
    {
        int numBytes;
        numBytes=utility_avpicture_get_size(AVpixfmt_, width_,height_);
        data_=(data_type *)utility_av_malloc(numBytes);

        desc_ = utility_av_pix_fmt_desc_get(AVpixfmt_);
        
        utility_av_pix_fmt_get_chroma_sub_sample( AVpixfmt_, &chroma_w_, &chroma_h_ );

        if ( is_pixfmt_rgb( AVpixfmt_ ) ) {
            alloc_rgb_planes( );
        } else {
            alloc_yuv_planes( );
        }
    }

    void alloc_rgb_planes( )
    {
        if ( plane_count( ) == 1 ) {
            plane plane = { 0, ( ( width_ * block_size( ) + 3 ) & -4 ) * sizeof( data_type ), width_, height_, width_ * block_size( ) * sizeof( data_type ) };
            planes_.push_back( plane );

            return;
        }

        plane plane;
        plane.offset = 0;
        plane.pitch = ( ( width_ + 3 ) & -4 ) * sizeof( data_type );
        plane.width = width_;
        plane.height = height_;
        plane.linesize = width_ * sizeof( data_type );
        planes_.push_back( plane );
        if ( plane_count( ) <= 1 ) { return; }

        plane.offset += plane.pitch * sizeof( data_type ) * height_;
        planes_.push_back( plane );
        if ( plane_count( ) <= 2 ) { return; }

        plane.offset *= 2;
        planes_.push_back( plane );
        if ( plane_count( ) <= 3 ) { return; }

        plane.offset *= 2;
        planes_.push_back( plane );
    }

    void alloc_yuv_planes( )
    {
        plane plane;

        // PLANE 1
        plane.offset = 0;
        plane.width = width_;
        plane.height = height_;
        plane.pitch = plane.linesize = utility_av_image_get_linesize( AVpixfmt_, width_, 0 );
        
        planes_.push_back( plane );

        if ( plane_count( ) <= 1 ) { return; }

        // PLANE 2
        plane.offset += width_ * height_;
        plane.linesize = plane.pitch = plane.width = utility_av_image_get_linesize( AVpixfmt_, width_, 1 );
        if ( chroma_w_ == 1 && chroma_h_ == 1 ) {
            plane.height = height_ / 2;
        }

        planes_.push_back( plane );
        if ( plane_count( ) <= 2 ) { return; }

        // PLANE 3

        plane.linesize = plane.pitch = plane.width = utility_av_image_get_linesize( AVpixfmt_, width_, 2 );
        if ( chroma_w_ == 1 && chroma_h_ == 1 ) {
            plane.offset += ( width_ * height_ ) / 4;
        } else if ( chroma_w_ == 1 && chroma_h_ == 0 ) {
            plane.offset += ( width_ * height_ ) / 2;
        }

        planes_.push_back( plane );
       
				 
		if ( plane_count( ) <= 3 ) { return; }

		// PLANE 4
		plane.linesize = plane.pitch = plane.width = utility_av_image_get_linesize( AVpixfmt_, width_, 3 );
		plane.offset +=  width_ * height_;
		planes_.push_back( plane );

    }

    void deallocate( )
    {
        if ( data_ ) {
            utility_av_free( data_ );
        }
    }

};

} } } }

#endif

