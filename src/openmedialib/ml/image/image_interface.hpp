#ifndef ML_IMAGE_INC_
#define ML_IMAGE_INC_

namespace olib { namespace openmedialib { namespace ml { namespace image {

struct default_plane
{
	int offset;
	//The number of bytes from the start of one line
	//in the plane to the next one. This is not
	//necessarily the same as linesize, since it may
	//include extra padding bytes at the end of each
	//line.
	int pitch;
	//The number of samples per line in the plane.
	int width;
	//The number of lines in the plane.
	int height;
	//The combined byte size of all samples in a line
	//in the plane.
	int linesize;
};

typedef enum
{
	cropped = 0x1,
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

class ML_DECLSPEC image
{
public:
	virtual ~image( ) { }
	virtual image *clone( int flags = cropped ) = 0;
	virtual MLPixelFormat ml_pixel_format( ) = 0;
	virtual int width( size_t index = 0, bool crop = true ) const = 0;
	virtual int height( size_t index = 0, bool crop = true ) const = 0;
	virtual int bitdepth( size_t index = 0 ) = 0;
	virtual int storage_bytes( ) const = 0;
	virtual int plane_count( ) = 0;
	virtual int linesize( size_t index = 0, bool crop = true ) = 0;
	virtual int pitch( size_t index = 0, bool crop = true ) = 0;
	virtual int offset( size_t index = 0, bool crop = true ) = 0;
	virtual int num_components( ) const = 0;
	virtual int get_crop_x( ) const = 0;
	virtual int get_crop_y( ) const = 0;
	virtual int get_crop_w( ) const = 0;
	virtual int get_crop_h( ) const = 0;
	virtual bool is_writable( ) const = 0;
	virtual void set_writable( bool writable ) = 0;
	virtual int position( ) const = 0;
	virtual void set_position( int position ) = 0;
	virtual int get_sar_num( ) const = 0;
	virtual int get_sar_den( ) const = 0;
	virtual double aspect_ratio( ) const = 0;
	virtual void set_sar_num( int sar_num ) = 0;
	virtual void set_sar_den( int sar_den ) = 0;
	virtual field_order_flags field_order( ) const = 0;
	virtual void set_field_order( field_order_flags flags ) = 0;
	virtual bool empty( ) const = 0;
	virtual bool matching( int flags ) const = 0;
	virtual const t_string& pf( ) const = 0;
	virtual bool is_yuv_planar( ) = 0;
	// Returns the component offset of the alpha within its plane (or -1 if no alpha is present)
	virtual int alpha_offset( ) const = 0;
	// Returns the plane containing the alpha component (or -1 if no alpha is present)
	virtual int alpha_plane( ) const = 0;
	// The size of the full image
	virtual int size( ) const = 0;
	// Crop an image
	virtual bool crop( int x, int y, int w, int h, bool crop = true ) = 0;
	virtual void crop_clear( ) = 0;
	virtual void *ptr( size_t index = 0, bool cropped = true ) = 0;
};

} } } }

#endif
