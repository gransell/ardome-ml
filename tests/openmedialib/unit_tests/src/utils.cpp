#include "utils.hpp"
#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/frame.hpp>

using namespace olib::openmedialib::ml;

stream_mock::stream_mock( std::string codec, size_t length, boost::int64_t position, boost::int64_t key, int bitrate,
	int frequency, int channels, int samples, int sample_size )
	: stream_type( )
	, codec_( codec )
	, id_( stream_audio )
	, length_( length )
	, data_( length + 32 )
	, position_( position )
	, key_( key )
	, bitrate_( bitrate )
	, size_( 0, 0 )
	, sar_( 1, 1 )
	, frequency_( frequency )
	, channels_( channels )
	, samples_( samples )
	, sample_size_( sample_size )
	, field_order_( olib::openmedialib::ml::image::top_field_first )
	, estimated_gop_size_( 0 )
	, index_( 0 )
{
}

stream_mock::~stream_mock( ) 
{ }

void stream_mock::set_position( boost::int64_t position )
{
	position_ = position;
}

void stream_mock::set_key( boost::int64_t key )
{
	key_ = key;
}

void stream_mock::set_index( size_t index )
{
	index_ = index;
}

size_t stream_mock::index( )
{
	return index_;
}

olib::openpluginlib::pcos::property_container &stream_mock::properties( )
{
	return properties_;
}

const std::string &stream_mock::container( ) const
{
	return container_;
}

const std::string &stream_mock::codec( ) const
{
	return codec_;
}

const enum stream_id stream_mock::id( ) const
{
	return id_;
}

size_t stream_mock::length( )
{
	return length_;
}

boost::uint8_t *stream_mock::bytes( )
{
	return &data_[ 16 - boost::int64_t( &data_[ 0 ] ) % 16 ];
}

const boost::int64_t stream_mock::key( ) const
{
	return key_;
}

const boost::int64_t stream_mock::position( ) const
{
	return position_;
}

const int stream_mock::bitrate( ) const
{
	return bitrate_;
}

const int stream_mock::estimated_gop_size( ) const
{ 
	return estimated_gop_size_;
}

const dimensions stream_mock::size( ) const 
{
	return size_;
}

const fraction stream_mock::sar( ) const 
{ 
	return sar_;
}

const int stream_mock::frequency( ) const 
{ 
	return frequency_; 
}

const int stream_mock::channels( ) const 
{ 
	return channels_; 
}

const int stream_mock::samples( ) const 
{ 
	return samples_; 
}

const int stream_mock::sample_size( ) const
{
	return sample_size_;
}

const olib::t_string stream_mock::pf( ) const 
{ 
	return pf_; 
}

olib::openmedialib::ml::image::field_order_flags stream_mock::field_order( ) const
{
	return field_order_;
}


inline bool in_range( int a, int b, int v )
{
	return ( a >= ( b - v ) && a <= ( b + v ) );
}

template< typename T >
bool check_plane( image_type_ptr im, int plane, int value, int variance )
{
	boost::shared_ptr< T > src_type = image::coerce< T >( im );
	const typename T::data_type *src = src_type->data( plane );
	const int src_rem = ( src_type->pitch( plane ) - src_type->linesize( plane ) ) / sizeof( typename T::data_type );
	const int v = ML_SCALE_SAMPLE( value, ML_MAX_BIT_VALUE( im->bitdepth( ) ), 255 );
	const int var = ML_SCALE_SAMPLE( variance, ML_MAX_BIT_VALUE( im->bitdepth( ) ), 255 );
	int h = im->height( plane );
	bool ok = true;
	while( ok && h -- )
	{
		int w = im->width( plane );
		while ( ok && w -- ) ok = ok && in_range( int( *src ++ ), v, var );
		src += src_rem;
	}
	return ok;
}

bool check_plane( image_type_ptr im, int plane, int value, int variance )
{
	bool result = false;
    if ( image::coerce< image::image_type_8 >( im ) )
        result = check_plane< image::image_type_8 >( im, plane, value, variance );
    else if ( image::coerce< image::image_type_16 >( im ) )
        result = check_plane< image::image_type_16 >( im, plane, value, variance );
	return result;
}

template < typename T >
bool check_components( image_type_ptr im, int r, int g, int b, int a, int variance )
{
	boost::shared_ptr< T > src_type = image::coerce< T >( im );
	const typename T::data_type *src = src_type->data( );
	const int src_rem = ( im->pitch( ) - im->linesize( ) ) / sizeof( typename T::data_type );
	int samples[ 4 ];
	const int components = image::arrange_rgb( im->ml_pixel_format( ), samples, r, g, b, a );
	BOOST_REQUIRE( components == 4 || a == -1 );
	const int var = ML_SCALE_SAMPLE( variance, ML_MAX_BIT_VALUE( im->bitdepth( ) ), 255 );

	int h = im->height( );
	bool ok = true;
	const bool check_alpha = ( a >= 0 );

	while( ok && h -- )
	{
		int w = im->width( );
		while ( ok && w -- )
		{
			int *c = samples;
			switch( components )
			{
				case 4:
					ok = !check_alpha || in_range( int( *src ++ ), *c ++, var );
				case 3:
					ok = ok && in_range( int( *src ++ ), *c ++, var );
					ok = ok && in_range( int( *src ++ ), *c ++, var );
					ok = ok && in_range( int( *src ++ ), *c ++, var );
					break;
				default:
					ok = false;
					break;
			}
		}
		src += src_rem;
	}
	return ok;
}

bool check_components( image_type_ptr im, int r, int g, int b, int a, int variance )
{
	bool result = false;
    if ( image::coerce< image::image_type_8 >( im ) )
        result = check_components< image::image_type_8 >( im, r, g, b, a, variance );
    else if ( image::coerce< image::image_type_16 >( im ) )
        result = check_components< image::image_type_16 >( im, r, g, b, a, variance );
	return result;
}

#if 0
/// Left in place to help diagnose issues with 8 bit rgb/yuv calcs
void print_rgb( image_type_ptr image )
{
	const int y = *( ( boost::uint8_t * )image->ptr( 0 ) );
	const int u = *( ( boost::uint8_t * )image->ptr( 1 ) );
	const int v = *( ( boost::uint8_t * )image->ptr( 2 ) );
	int r, g, b;
	image::yuv444_to_rgb24( r, g, b, y, u, v );
	std::cerr << y << ", " << u << ", " << v << " == " << r << ", " << g << ", " << b << std::endl;
}

void print_alpha( image_type_ptr image )
{
	const int a = *( ( boost::uint8_t * )image->ptr( 0 ) );
	std::cerr << a << std::endl;
}
#endif

bool check_image( image_type_ptr image, image_type_ptr alpha, int r, int g, int b, int a, int variance )
{
	bool result = image;
	const bool check_alpha = ( a >= 0 );

	if ( result )
	{
		if ( image::is_pixfmt_planar( image->ml_pixel_format( ) ) )
		{
			int y, u, v;
			image::rgb24_to_yuv444( y, u, v, r, g, b );
			result = check_plane( image, 0, y, variance );
			result = result && check_plane( image, 1, u, variance );
			result = result && check_plane( image, 2, v, variance );
			if ( check_alpha && pixfmt_has_alpha( image->ml_pixel_format( ) ) )
			{
				BOOST_REQUIRE( !alpha );
				result = result && check_plane( image, 3, a, variance );
			}
		}
		else if ( image::is_pixfmt_rgb( image->ml_pixel_format( ) ) )
		{
			if( pixfmt_has_alpha( image->ml_pixel_format( ) ) )
				result = check_components( image, r, g, b, a, variance );
			else
				result = check_components( image, r, g, b, -1, variance );
		}
		else
		{
			result = false;
		}
	}

	if ( check_alpha && !pixfmt_has_alpha( image->ml_pixel_format( ) ) )
	{
		BOOST_REQUIRE( alpha );
		result = result && check_plane( alpha, 0, a, variance );
	}

	return result;
}

bool check_frame( frame_type_ptr frame, int r, int g, int b, int a, int variance )
{
	return frame && check_image( frame->get_image( ), frame->get_alpha( ), r, g, b, a, variance );
}

