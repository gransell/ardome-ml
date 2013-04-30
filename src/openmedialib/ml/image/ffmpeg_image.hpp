#ifndef FFMPEG_IMAGE_H_
#define FFMPEG_IMAGE_H_

#include <boost/shared_ptr.hpp>
#include <openmedialib/ml/openmedialib_plugin.hpp>
//#include <openmedialib/ml/types.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/str_util.hpp>

#include <openmedialib/ml/image/image_interface.hpp>
#include <openmedialib/ml/image/ffmpeg_utility.hpp>

extern "C" {
#include <libavformat/avio.h>
#include <libavutil/mem.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
}

namespace cl = olib::opencorelib;
namespace ml = olib::openmedialib::ml;
namespace image = olib::openmedialib::ml::image;

namespace olib { namespace openmedialib { namespace ml { namespace image {

template<typename T>
class ML_DECLSPEC ffmpeg_image : public image
{
public:
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
    , writable_( true )
    , pts_( 0 )
    , position_( 0 )
    , sar_num_( -1 )
    , sar_den_( 1 )
    , field_order_( progressive )
    {
        pixfmt_ = ML_to_AV(MLpixfmt_);
        ARENFORCE(pixfmt_ != AV_PIX_FMT_NONE);
        allocate( );
        desc_ = av_pix_fmt_desc_get(pixfmt_);
        av_pix_fmt_get_chroma_sub_sample( pixfmt_, &chroma_w_, &chroma_h_ );

        if ( desc_->flags & PIX_FMT_RGB ) {
            alloc_rgb_planes( );
        } else {
            alloc_yuv_planes( );
        }
    }
    ~ffmpeg_image( ) { deallocate ( ); }

	ffmpeg_image *clone( int flags = cropped )
	{
		//FIXME
		return new ffmpeg_image( MLpixfmt_, width_, height_ );
	}
	
	MLPixelFormat ml_pixel_format( ) { return MLpixfmt_; }

    int width( size_t index = 0 )
    {
        const plane *p = get_plane( index );
        return p ? p->width : 0;
    }

    int height( size_t index = 0 )
    {
        const plane *p = get_plane( index );
        return p ? p->height : 0;
    }

    int plane_count ( )
    {
        //return av_pix_fmt_count_planes( pixfmt_ );
        int i, planes[4] = { 0 }, ret = 0;

        for (i = 0; i < desc_->nb_components; i++) {
            planes[desc_->comp[i].plane] = 1;
        }
        for (i = 0; i < FF_ARRAY_ELEMS(planes); i++) {
            ret += planes[i];
        }
        return ret;
    }

    int bitdepth( size_t index = 0 )
    {
        return desc_->comp[index].depth_minus1 + 1;
    }

    int linesize( size_t index = 0 )
    {
        const plane *p = get_plane( index );
        return p ? p->linesize : 0;
    }

    int pitch( size_t index = 0 )
    {
        const plane *p = get_plane( index );
        return p ? p->pitch : 0;
    }

    int offset( size_t index = 0 )
    {
        const plane *p = get_plane( index );
        return p ? p->offset : 0;
    }

    int block_size( )
    {
        return desc_->nb_components;
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
	
    uint8_t *data ( size_t index = 0 )
    {
        return data_ + offset( index );
    }

    int depth( ) { return 1; }

    t_string pf( ) const { return cl::str_util::to_t_string( desc_->name ); }

/*
        bool is_flipped( )          const { return flipped_; }
    void set_flipped( bool flipped )  { flipped_ = flipped; crop_sync( ); }

    bool is_flopped( )          const { return flopped_; }
    void set_flopped( bool flopped )  { flopped_ = flopped; crop_sync( ); }
*/
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
		return desc_->flags & PIX_FMT_PLANAR ? true : false;
	}

	int size( ) const { return avpicture_get_size(pixfmt_, width_,height_); }

    // Crop an image
    bool crop( int x, int y, int w, int h, bool crop = true )
    {

        return true;
    }

	// Clear the cropped image by returning everything back to the storage planes
    void crop_clear( )
    {
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
    uint8_t *data_;
    bool flipped_;
    bool flopped_;
    bool writable_;
    double pts_;
    int position_;

    //Sample aspect ratio
    int sar_num_;
    int sar_den_;

    field_order_flags field_order_;
    const AVPixFmtDescriptor *desc_;
    AVPixelFormat pixfmt_;
    planes planes_;

     // Get the requested plane
    inline const plane *get_plane( size_t index ) const
    {
        return index < planes_.size( ) ? &planes_[ index ] : 0;
    }

protected:

    void allocate( )
    {
        int numBytes;
        numBytes=avpicture_get_size(pixfmt_, width_,height_);
        data_=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
    }

    void alloc_rgb_planes( )
    {
        if ( plane_count( ) == 1 ) {
            plane plane = { 0, ( ( width_ * block_size( ) + 3 ) & -4 ) * sizeof( T ), width_, height_, width_ * block_size( ) * sizeof( T ) };
            planes_.push_back( plane );

            return;
        }

        plane plane;
        plane.offset = 0;
        plane.pitch = ( ( width_ + 3 ) & -4 ) * sizeof( T );
        plane.width = width_;
        plane.height = height_;
        plane.linesize = width_ * sizeof( T );
        planes_.push_back( plane );
        if ( plane_count( ) <= 1 ) { return; }

        plane.offset += plane.pitch * sizeof( T ) * height_;
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

        if ( chroma_w_ == 1 && chroma_h_ == 1 ) {
            plane.pitch = plane.linesize = av_image_get_linesize( pixfmt_, width_, 0 );
        } else {
            plane.pitch = width_;
            plane.linesize = width_;
        }

        planes_.push_back( plane );

        if ( plane_count( ) <= 1 ) { return; }

        // PLANE 2
        plane.offset += width_ * height_;
        plane.linesize = plane.pitch = plane.width = av_image_get_linesize( pixfmt_, width_, 1 );
        if ( chroma_w_ == 1 && chroma_h_ == 1 ) {
            plane.height = height_ / 2;
        }

        planes_.push_back( plane );
        if ( plane_count( ) <= 2 ) { return; }

        // PLANE 3

        if ( chroma_w_ == 1 && chroma_h_ == 1 ) {
            plane.offset += ( width_ * height_ ) / 4;
        } else if ( chroma_w_ == 1 && chroma_h_ == 0 ) {
            plane.offset += ( width_ * height_ ) / 2;
        }

        planes_.push_back( plane );
       
				 
		if ( plane_count( ) <= 3 ) { return; }

		//TODO PLANE 4

    }

    void deallocate( )
    {
        if ( data_ ) {
            av_free( data_ );
        }
    }

};

} } } }

#endif

