/*
* ml - A media library representation.
* Copyright (C) 2013 Vizrt
* Released under the LGPL.
*/
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/image/image_types.hpp>
#include <openmedialib/ml/image/image_interface.hpp>

#include <openmedialib/ml/image/ffmpeg_image.hpp>
#include <openmedialib/ml/image/ffmpeg_utility.hpp>

#include <boost/assign/list_of.hpp>
#include <map>

namespace cl = olib::opencorelib;
namespace ml = olib::openmedialib::ml;
namespace image = olib::openmedialib::ml::image;

namespace {
	const olib::t_string empty_string;
	typedef std::map<olib::t_string, image::MLPixelFormat> MLPixelFormatMap_type;
	const MLPixelFormatMap_type MLPixelFormatMap_ = boost::assign::map_list_of
		(_CT("yuv420p"),      image::ML_PIX_FMT_YUV420P)
		(_CT("yuv420p10le"),  image::ML_PIX_FMT_YUV420P10LE)
		(_CT("yuv420p16le"),  image::ML_PIX_FMT_YUV420P16LE)
		(_CT("yuva420p"),     image::ML_PIX_FMT_YUVA420P)
		(_CT("yuv422p"),      image::ML_PIX_FMT_YUV422P)
		(_CT("yuv422"),       image::ML_PIX_FMT_YUV422)
		(_CT("uyv422"),       image::ML_PIX_FMT_UYV422)
		(_CT("yuv444p"),      image::ML_PIX_FMT_YUV444P)
		(_CT("yuva444p"),     image::ML_PIX_FMT_YUVA444P)
		(_CT("yuv444p16le"),  image::ML_PIX_FMT_YUV444P16LE)
		(_CT("yuva444p16le"), image::ML_PIX_FMT_YUVA444P16LE)
		(_CT("yuv411p"),      image::ML_PIX_FMT_YUV411P)
		(_CT("yuv422p10le"),  image::ML_PIX_FMT_YUV422P10LE)
		(_CT("yuv422p16le"),  image::ML_PIX_FMT_YUV422P16LE)
		(_CT("l8"),           image::ML_PIX_FMT_L8)
		(_CT("l16le"),        image::ML_PIX_FMT_L16LE)
		(_CT("r8g8b8"),       image::ML_PIX_FMT_R8G8B8)
		(_CT("b8g8r8"),       image::ML_PIX_FMT_B8G8R8)
		(_CT("r8g8b8a8"),     image::ML_PIX_FMT_R8G8B8A8)
		(_CT("b8g8r8a8"),     image::ML_PIX_FMT_B8G8R8A8)
		(_CT("a8r8g8b8"),     image::ML_PIX_FMT_A8R8G8B8)
		(_CT("a8b8g8r8"),     image::ML_PIX_FMT_A8B8G8R8)
		(_CT("r16g16b16le"),  image::ML_PIX_FMT_R16G16B16LE);
}

namespace olib { namespace openmedialib { namespace ml { namespace image {

ML_DECLSPEC MLPixelFormat string_to_MLPF( const olib::t_string& pf )
{
	MLPixelFormatMap_type::const_iterator i = MLPixelFormatMap_.find( pf );
	if ( MLPixelFormatMap_.end() == i )
	{
		return ML_PIX_FMT_NONE;
	}

	return i->second;
}

ML_DECLSPEC const olib::t_string& MLPF_to_string( MLPixelFormat pf ) 
{
	for(MLPixelFormatMap_type::const_iterator i = MLPixelFormatMap_.begin(),
		e = MLPixelFormatMap_.end(); i != e; ++i)
	{
		if ( i->second == pf )
		{
			return i->first;
		}
	}

	return empty_string;

}

typedef std::pair< int, int > pair;

static std::map< MLPixelFormat, pair > dimensions = boost::assign::map_list_of
	( ML_PIX_FMT_YUV420P, pair( 2, 2 ) )
	( ML_PIX_FMT_YUV420P10LE, pair( 2, 2 ) )
	( ML_PIX_FMT_YUV420P16LE, pair( 2, 2 ) )
	( ML_PIX_FMT_YUVA420P, pair( 2, 2 ) )
	( ML_PIX_FMT_UYV422, pair( 2, 1 ) )
	( ML_PIX_FMT_YUV422P, pair( 2, 1 ) )
	( ML_PIX_FMT_YUV422, pair( 2, 1 ) )
	( ML_PIX_FMT_YUV422P10LE, pair( 2, 1 ) )
	( ML_PIX_FMT_YUV422P16LE, pair( 2, 1 ) )
	( ML_PIX_FMT_YUV444P, pair( 1, 1 ) )
	( ML_PIX_FMT_YUVA444P, pair( 1, 1 ) )
	( ML_PIX_FMT_YUV444P16LE, pair( 1, 1 ) )
	( ML_PIX_FMT_YUVA444P16LE, pair( 1, 1 ) )
	( ML_PIX_FMT_YUV411P, pair( 4, 1 ) )
	( ML_PIX_FMT_L8, pair( 1, 1 ) )
	( ML_PIX_FMT_L16LE, pair( 1, 1 ) )
	( ML_PIX_FMT_R8G8B8, pair( 1, 1 ) )
	( ML_PIX_FMT_B8G8R8, pair( 1, 1 ) )
	( ML_PIX_FMT_R8G8B8A8, pair( 1, 1 ) )
	( ML_PIX_FMT_B8G8R8A8, pair( 1, 1 ) )
	( ML_PIX_FMT_A8R8G8B8, pair( 1, 1 ) )
	( ML_PIX_FMT_A8B8G8R8, pair( 1, 1 ) )
	( ML_PIX_FMT_R16G16B16LE, pair( 1, 1 ) );

inline void correct( int &value, const int multiple, enum correction type )
{
	int discrepancy = value % multiple;

	if ( discrepancy )
	{
		switch( type )
		{
			case nearest:
				if ( discrepancy >= ( multiple >> 1 ) )
					value += multiple - discrepancy;
				else
					value -= discrepancy;
				break;
			case floor:
				value -= discrepancy;
				break;
			case ceil:
				value += multiple - discrepancy;
				break;
		}
	}
}

ML_DECLSPEC void correct( MLPixelFormat pf, int &width, int &height, enum correction type )
{
	std::map< MLPixelFormat, pair >::const_iterator iter = dimensions.find( pf );
	ARENFORCE_MSG( iter != dimensions.end( ), "Unable to find the requested pf %1%" )( pf );
	pair p = iter->second;
	correct( width, p.first, type );
	correct( height, p.second, type );
}

ML_DECLSPEC void correct( const olib::t_string pf, int &width, int &height, enum correction type )
{
    MLPixelFormatMap_type::const_iterator i = MLPixelFormatMap_.find( pf );
    ARENFORCE_MSG( i != MLPixelFormatMap_.end(), "Invalid picture format");
    return correct( i->second, width, height, type );
}

ML_DECLSPEC bool verify( MLPixelFormat pf, int width, int height )
{
	int tw = width, th = height;
	correct( pf, tw, th );
	return tw == width && th == height;
}

ML_DECLSPEC bool verify( const olib::t_string pf, int width, int height )
{
	int tw = width, th = height;
	correct( pf, tw, th );
	return tw == width && th == height;
}
 
ML_DECLSPEC int image_depth ( MLPixelFormat pf ) 
{
	//XXX Pixfmts with different bitdepth for different components do exist
	return utility_bitdepth( ML_to_AV( pf ), 0 ); 
}

ML_DECLSPEC image_type_ptr allocate ( MLPixelFormat pf, int width, int height )
{
    ARENFORCE_MSG( pf > ML_PIX_FMT_NONE && pf < ML_PIX_FMT_NB, "Invalid picture format")( pf );
	ARENFORCE_MSG( verify( pf, width, height ), "Unable to allocate an image of type %1% at %2%x%3%" )( pf )( width )( height );
	if ( image_depth( pf ) == 8 )
		return ml::image::image_type_8_ptr( new ml::image::image_type_8( pf, width, height ) );
	else if ( image_depth( pf ) > 8 )
		return ml::image::image_type_16_ptr( new ml::image::image_type_16( pf, width, height ) );
	return image_type_ptr( );	
}

ML_DECLSPEC image_type_ptr allocate ( const olib::t_string& pf, int width, int height )
{
	MLPixelFormatMap_type::const_iterator i = MLPixelFormatMap_.find( pf );
	ARENFORCE_MSG( i != MLPixelFormatMap_.end(), "Invalid picture format: \"%1%\"" )( pf );
	return ml::image::allocate( i->second, width, height );
}

ML_DECLSPEC image_type_ptr allocate ( const image_type_ptr &img )
{
	return ml::image::allocate( img->ml_pixel_format( ), img->width( ), img->height( ) );
}

ML_DECLSPEC image_type_ptr convert( const image_type_ptr &src, MLPixelFormat pf )
{
	ARENFORCE_MSG( verify( pf, src->width( ), src->height( ) ), "Unable to allocate an image of type %1% at %2%x%3%" )( pf )( src->width( ) )( src->height( ) );
	geometry shape( src );
	shape.pf = pf;
	return rescale_and_convert( ml::rescale_object_ptr( ), src, shape );
}

ML_DECLSPEC image_type_ptr convert( const image_type_ptr &src, const olib::t_string& pf )
{
	MLPixelFormatMap_type::const_iterator i = MLPixelFormatMap_.find( pf );
	if ( i == MLPixelFormatMap_.end() )
		return image_type_ptr( );
	return convert( src, i->second );
}

ML_DECLSPEC image_type_ptr convert( ml::rescale_object_ptr ro, const image_type_ptr &src, const olib::t_string pf )
{
	MLPixelFormatMap_type::const_iterator i = MLPixelFormatMap_.find( pf );
	if ( i == MLPixelFormatMap_.end() )
		return image_type_ptr( );
	ARENFORCE_MSG( verify( pf, src->width( ), src->height( ) ), "Unable to allocate an image of type %1% at %2%x%3%" )( pf )( src->width( ) )( src->height( ) );
	geometry shape( src );
	shape.pf = i->second;
	return rescale_and_convert( ro, src, shape );
}

image_type_ptr rescale( const image_type_ptr &im, int new_w, int new_h, rescale_filter filter )
{
    if( im->width( ) == new_w && im->height( ) == new_h )
        return im;

	ARENFORCE_MSG( verify( im->pf( ), new_w, new_h ), "Unable to allocate an image of type %1% at %2%x%3%" )( im->pf( ) )( new_w )( new_h );
	geometry shape( im );
	shape.width = new_w;
	shape.height = new_h;
	shape.interp = filter;
	return rescale_and_convert( ml::rescale_object_ptr( ), im, shape );
}

template< typename T >
void colour_rectangle( T *ptr, boost::uint8_t value, int width, int pitch, int height, int bitdepth )
{
	typename T val_shifted = static_cast< typename T >( value << ( bitdepth - 8 ) );

	while( height -- > 0 )
	{
		//memset( ptr, value, width );
		std::fill_n( ptr, width, val_shifted );
		ptr += pitch;
	}
}

// Draw the necessary border around the cropped image
// TODO: Correctly support border colour in all colour spaces
template< typename T >
void border( ml::image_type_ptr image, geometry &shape )
{
	int yuv[ 3 ];
	ml::image::rgb24_to_yuv444( yuv[ 0 ], yuv[ 1 ], yuv[ 2 ], shape.r, shape.g, shape.b );

	const int iwidth = image->width( );
	const int iheight = image->height( );

	for ( int i = 0; i < image->plane_count( ); i ++ )
	{
		const float wps = float( image->linesize( i ) ) / iwidth;
		const float hps = float( image->height( i ) ) / iheight;
        
        typename T::data_type *ptr = ml::image::coerce< T >( image )->data( i );
		const typename T::data_type value = image->is_yuv_planar( ) ?  yuv[ i ] : 0;

		const int width = int( iwidth * wps );
		const int pitch = image->pitch( i );
		const int top_lines = int( shape.y * hps );
		const int bottom_lines = int( ( iheight - shape.y - shape.h ) * hps );
		const int bottom_offset = int( pitch * ( shape.y + shape.h ) * hps );
		const int side_lines = int( shape.h * hps );
		const int left_offset = int( pitch * shape.y * hps );
		const int right_offset = left_offset + int( ( shape.x + shape.w ) * wps );
		const int left_width = int( shape.x * wps );
		const int right_width = int( ( iwidth - shape.x - shape.w ) * wps );

		colour_rectangle< typename T::data_type >( ptr, value, width, pitch, top_lines, image->bitdepth() );
		colour_rectangle< typename T::data_type >( ptr + left_offset, value, left_width, pitch, side_lines, image->bitdepth() );
		colour_rectangle< typename T::data_type >( ptr + right_offset, value, right_width, pitch, side_lines, image->bitdepth() );
		colour_rectangle< typename T::data_type >( ptr + bottom_offset, value, width, pitch, bottom_lines, image->bitdepth() );
	}
}

void border( ml::image_type_ptr im, geometry &shape )
{
    image_type_ptr result;
    if ( ml::image::coerce< ml::image::image_type_8 >( im ) )
		border< ml::image::image_type_8 >( im, shape );
    else if ( ml::image::coerce< ml::image::image_type_16 >( im ) )
        border< ml::image::image_type_16 >( im, shape );
}

image_type_ptr rescale_and_convert( ml::rescale_object_ptr ro, const ml::image_type_ptr &im, geometry &shape )
{
	int sar_num = im->get_sar_num( );
	int sar_den = im->get_sar_den( );
	
	double aspect_ratio = im->aspect_ratio( );
	
	ml::image_type_ptr image;

	// Convert to progressive if required
	if ( shape.field_order == ml::image::progressive && im->field_order( ) != ml::image::progressive ) {
		image = ml::image::deinterlace( im );
	} else {
		image = im;
	}

	// If no conversion is specified, retain that of the input
	if ( shape.pf == ML_PIX_FMT_NONE )
		shape.pf = image->ml_pixel_format( );

	// Deal with the properties specified
	if ( shape.width == -1 && shape.height == -1 && std::abs( shape.sar_num ) == 1 && std::abs( shape.sar_den ) == 1 )
	{
		// If width and height are -1, then retain the dimensions of the source
		shape.width = image->width( );
		shape.height = image->height( );
		shape.sar_num = 1;
		shape.sar_den = 1;
	}
	else if ( shape.width == -1 && std::abs( shape.sar_num ) == 1 && std::abs( shape.sar_den ) == 1 )
	{
		// Maintain the aspect ratio of the input, and calculate width from height
		shape.width = int( shape.height * aspect_ratio );
		shape.sar_num = 1;
		shape.sar_den = 1;
	}
	else if ( shape.height == -1 && std::abs( shape.sar_num ) == 1 && std::abs( shape.sar_den ) == 1 )
	{
		// Maintain the aspect ratio of the input, and calculate height from width
		shape.height = int( shape.width / aspect_ratio );
		shape.sar_num = 1;
		shape.sar_den = 1;
	}
	else if ( shape.width != -1 && shape.height != -1 && shape.sar_num == -1 && shape.sar_den == -1 )
	{
		shape.sar_num = 1;
		shape.sar_den = 1;
	}
	else if ( shape.width != -1 && shape.height != -1 && shape.sar_num != -1 && shape.sar_den != -1 )
	{
		ml::image::calculate( shape, image );
	}
	else if ( shape.height != -1 && shape.sar_num != -1 && shape.sar_den != -1 )
	{
		shape.width = int( shape.height * aspect_ratio * shape.sar_den / shape.sar_num );
	}
	else if ( shape.width != -1 && shape.sar_num != -1 && shape.sar_den != -1 )
	{
		shape.height = int( shape.width / aspect_ratio * shape.sar_num / shape.sar_den );
	}
	else if ( shape.width == -1 && shape.height == -1 && shape.sar_num != -1 && shape.sar_den != -1 )
	{
		shape.width = image->width( );
		shape.height = image->height( );
	}
	else
	{
		ARENFORCE_MSG( false, "Unsupported options" );
	}

	// Ensure that the requested/computed dimensions are valid for the pf requested
	ml::image::correct( shape.pf, shape.width, shape.height, ceil );
	
	image_type_ptr new_im = allocate( shape.pf, shape.width, shape.height );
	
	// Crop the input and output if the shape is cropped
	if ( shape.cropped )
	{
		image->crop( shape.cx, shape.cy, shape.cw, shape.ch );
		new_im->crop( shape.x, shape.y, shape.w, shape.h );
	}
	
	rescale_and_convert_ffmpeg_image( ro, image, new_im, shape.interp );

	// Clear crop information on both images if shape is cropped
	if ( shape.cropped )
	{
		image->crop_clear( );
		new_im->crop_clear( );
		border( new_im, shape );
	}

	new_im->set_field_order( shape.field_order );
	
    return new_im;
}

// Calculate the geometry for the aspect ratio mode selected
void calculate( geometry &shape, image_type_ptr src )
{
	shape.cropped = true;

	double src_sar_num = double( src->get_sar_num( ) ); 
	double src_sar_den = double( src->get_sar_den( ) );
	double dst_sar_num = double( shape.sar_num );
	double dst_sar_den = double( shape.sar_den );

	int src_w = src->width( );
	int src_h = src->height( );
	int dst_w = shape.width;
	int dst_h = shape.height;

	if ( src_sar_num <= 0 ) src_sar_num = 1;
	if ( src_sar_den <= 0 ) src_sar_den = 1;
	if ( dst_sar_num <= 0 ) dst_sar_num = 1;
	if ( dst_sar_den <= 0 ) dst_sar_den = 1;

	// Distort image to fill
	shape.x = 0;
	shape.y = 0;
	shape.w = dst_w;
	shape.h = dst_h;

	// Specify the initial crop area
	shape.cx = 0;
	shape.cy = 0;
	shape.cw = src_w;
	shape.ch = src_h;

	// Target destination area
	dst_w = shape.w;
	dst_h = shape.h;

	// Letter and pillar box calculations
	int letter_h = int( ( shape.w * src_h * src_sar_den * dst_sar_num ) / ( src_w * src_sar_num * dst_sar_den ) );
	int pillar_w = int( ( shape.h * src_w * src_sar_num * dst_sar_den ) / ( src_h * src_sar_den * dst_sar_num ) );

	correct( shape.pf, pillar_w, letter_h, ceil );

	// Handle the requested mode
	if ( shape.mode == MODE_FILL )
	{
		shape.h = dst_h;
		shape.w = pillar_w;

		if ( shape.w > dst_w )
		{
			shape.w = dst_w;
			shape.h = letter_h;
		}
	}
	else if ( shape.mode == MODE_SMART )
	{
		shape.h = dst_h;
		shape.w = pillar_w;

		if ( shape.w < dst_w )
		{
			shape.w = dst_w;
			shape.h = letter_h;
		}
	}
	else if ( shape.mode == MODE_LETTER )
	{
		shape.w = dst_w;
		shape.h = letter_h;
	}
	else if ( shape.mode == MODE_PILLAR )
	{
		shape.h = dst_h;
		shape.w = pillar_w;
	}
	else if ( shape.mode == MODE_NATIVE )
	{
		shape.w = src_w;
		shape.h = src_h;
	}

	// Correct the cropping as required
	if ( dst_w >= shape.w )
	{
		shape.x += ( int( dst_w ) - shape.w ) / 2;
	}
	else
	{
		double diff = double( shape.w - dst_w ) / shape.w;
		shape.cx = int( shape.cw * ( diff / 2.0 ) );
		shape.cw = int( shape.cw * ( 1.0 - diff ) );
		shape.w = dst_w;
	}

	if ( dst_h >= shape.h )
	{
		shape.y += ( int( dst_h ) - shape.h ) / 2;
	}
	else
	{
		double diff = double( shape.h - dst_h ) / shape.h;
		shape.cy = int( shape.ch * ( diff / 2.0 ) );
		shape.ch = int( shape.ch * ( 1.0 - diff ) );
		shape.h = dst_h;
	}

	correct( src->ml_pixel_format( ), shape.cx, shape.cy, ceil );
	correct( src->ml_pixel_format( ), shape.cw, shape.ch, ceil );
	correct( shape.pf, shape.x, shape.y, ceil );
	correct( shape.pf, shape.w, shape.h, ceil );
}

static int locate_alpha_offset( const MLPixelFormat pf )
{
    if ( !pixfmt_has_alpha( pf ) || is_pixfmt_planar( pf ) ) return -1; // Only RGB with alpha supported

    return utility_offset( ML_to_AV( pf ), utility_nb_components( ML_to_AV( pf ) ) - 1 ); //Alpha is always last component
}

template< typename T, enum MLPixelFormat alpha >
ML_DECLSPEC image_type_ptr extract_alpha( const image_type_ptr &im )
{
    image_type_ptr result;

    if ( im )
    {
        boost::shared_ptr< T > im_type = ml::image::coerce< T >( im );
        int offset = locate_alpha_offset( im->ml_pixel_format( ) );
        if ( offset >= 0 )
        {
            result = allocate( alpha, im->width( ), im->height( ) );
            boost::shared_ptr< T > result_type = ml::image::coerce< T >( result );

            const typename T::data_type *src = im_type->data( );
            typename T::data_type *dst = result_type->data( );

            int h = im->height( );

            int src_rem = im->pitch( ) - im->linesize( );
            int dst_rem = result->pitch( ) - result->linesize( );

			const int max_input = ( 1 << im->bitdepth( ) ) - 1;
			const int max_output = ( 1 << result->bitdepth( ) ) - 1;

            while( h -- )
            {
                int w = im->width( );

                while ( w -- )
                {
                    *dst ++ = ( typename T::data_type )( float( src[ offset ] / max_input ) * max_output );
                    src += 4;
                }

                src += src_rem;
                dst += dst_rem;
            }
        }
    }

    return result;
}

ML_DECLSPEC image_type_ptr extract_alpha( const image_type_ptr &im )
{
    image_type_ptr result;
    if ( ml::image::coerce< ml::image::image_type_8 >( im ) )
        result = extract_alpha< ml::image::image_type_8, ML_PIX_FMT_L8 >( im );
    else if ( ml::image::coerce< ml::image::image_type_16 >( im ) )
        result = extract_alpha< ml::image::image_type_16, ML_PIX_FMT_L16LE >( im );
	return result;
}

template< typename T >
ML_DECLSPEC image_type_ptr deinterlace( const image_type_ptr &im )
{
	ARENFORCE_MSG( !pixfmt_has_alpha( ML_to_AV( im->ml_pixel_format( ) ) ), "Image with alpha not supported to deinterlace" )( im->pf( ) );

	if ( im->field_order( ) != progressive )
	{
		im->set_field_order( progressive );
		boost::shared_ptr< T > im_type = ml::image::coerce< T >( im );
		ARENFORCE_MSG( im_type, "Unable to coerce to image type associated to %d" )( im->bitdepth( ) );
		for ( int i = 0; i < im->plane_count( ); i ++ )
		{
			typename T::data_type *dst = im_type->data( i );
			typename T::data_type *src = im_type->data( i ) + im->pitch( i ) / sizeof( typename T::data_type );
			int linesize = im->linesize( i ) / sizeof( typename T::data_type );
			int src_pitch = ( im->pitch( i ) / sizeof( typename T::data_type ) ) - linesize;
			int height = im->height( i ) - 1;
			
			for (int h = height - 1; h >= 0; h -- )
			{
				for ( int w = 0; w < linesize; w ++ )
				{
					*dst = ( *src ++ + *dst ) >> 1;
					dst ++;
				}
				dst += src_pitch;
				src += src_pitch;
			}
		}
	}

	return im;
}

ML_DECLSPEC image_type_ptr deinterlace( const image_type_ptr &im )
{
	image_type_ptr result;
	if ( ml::image::coerce< ml::image::image_type_8 >( im ) )
		result = deinterlace< ml::image::image_type_8 >( im );
	else if ( ml::image::coerce< ml::image::image_type_16 >( im ) )
		result = deinterlace< ml::image::image_type_16 >( im );
	return result;
}

inline unsigned char clamp_sample( const int v ) { return ( unsigned char )( v < 0 ? 0 : v > 255 ? 255 : v ); }

inline void yuv444_to_rgb( unsigned char *&dst, const int y, const int rc, const int gc, const int bc )
{
    *dst ++ = clamp_sample( ( y + rc ) >> 10 );
    *dst ++ = clamp_sample( ( y - gc ) >> 10 );
    *dst ++ = clamp_sample( ( y + bc ) >> 10 );
}

inline void yuv444_to_bgr( unsigned char *&dst, const int y, const int rc, const int gc, const int bc )
{
    *dst ++ = clamp_sample( ( y + bc ) >> 10 );
    *dst ++ = clamp_sample( ( y - gc ) >> 10 );
    *dst ++ = clamp_sample( ( y + rc ) >> 10 );
}

// Obtain the fields in presentation order
template< typename T >
ML_DECLSPEC image_type_ptr field( const image_type_ptr &im, int field )
{
	// Default our return to the source
	image_type_ptr new_im = im;

	// Only do something if we have an image and it isn't progressive
	if ( im && im->field_order( ) != progressive )
	{
		// Allocate an image which is half the size of the source
		new_im = allocate( im->pf( ), im->width( ), im->height( ) / 2 );

		// The apps field 0 depends on the field order of the image
		if ( im->field_order( ) == top_field_first )
			field = field == 0 ? 0 : 1;
		else
			field = field == 1 ? 0 : 1;

		// Copy every second scan line from each plane to extract the field
		for ( int i = 0; i < im->plane_count( ); i ++ )
		{
			boost::shared_ptr< T > im_type = ml::image::coerce< T >( im );
			boost::shared_ptr< T > new_im_type = ml::image::coerce< T >( new_im );
			typename T::data_type *src = im_type->data( i ) + field * im->pitch( i ) / sizeof( typename T::data_type );
			typename T::data_type *dst = new_im_type->data( i );
			int src_pitch = 2 * im->pitch( i ) / sizeof( typename T::data_type );
			int dst_pitch = new_im->pitch( i ) / sizeof( typename T::data_type );
			int linesize = new_im->linesize( i );
			int height = new_im->height( i );
			while( height -- )
			{
				memcpy( dst, src, linesize );
				dst += dst_pitch;
				src += src_pitch;
			}
		}
	}

	return new_im;
}

ML_DECLSPEC image_type_ptr field( const image_type_ptr &im, int f )
{
	image_type_ptr result;
	if ( ml::image::coerce< ml::image::image_type_8 >( im ) )
		result = field< ml::image::image_type_8 >( im, f );
	else if ( ml::image::coerce< ml::image::image_type_16 >( im ) )
		result = field< ml::image::image_type_16 >( im, f );
	return result;
}

ML_DECLSPEC bool is_pixfmt_planar(  MLPixelFormat pf ) 
{
	return is_pixfmt_planar( ML_to_AV( pf ) );
}

ML_DECLSPEC bool is_pixfmt_rgb(  MLPixelFormat pf ) 
{
	return is_pixfmt_rgb( ML_to_AV( pf ) );
}

ML_DECLSPEC bool pixfmt_has_alpha(  MLPixelFormat pf ) 
{
	return pixfmt_has_alpha( ML_to_AV( pf ) );
}

ML_DECLSPEC int order_of_component( MLPixelFormat pf, int index ) 
{
	//utility_offset returns bytes, but we want the order as an index
	return utility_offset( ML_to_AV( pf ), index ) / ( ( image_depth( pf ) + 7 ) / 8 );
}

} } } }
