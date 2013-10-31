#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <opencorelib/cl/utilities.hpp>

using boost::uint8_t;
namespace ml = olib::openmedialib::ml;
namespace au = olib::openmedialib::ml::audio;

namespace
{
	// populates array with data starting from 1 upto size
	void initialize_array( uint8_t *arr, uint8_t size )
	{
		for( uint8_t i = 0; i < size; i++ )
			arr[i] = i + 1;
	}
}


BOOST_AUTO_TEST_SUITE( audio_utilities )

BOOST_AUTO_TEST_CASE( pack_pcm24_from_pcm32 )
{
	const int samples = 2, channels = 2, input_sample_size_in_bytes = 4, output_sample_size_in_bytes = 3;
	uint8_t test_src[samples * channels * input_sample_size_in_bytes];
	uint8_t test_dest[samples * channels * output_sample_size_in_bytes];

	initialize_array( test_src, sizeof( test_src ) );
	au::pack_pcm24_from_pcm32( test_dest, test_src, samples, channels );
	
	uint8_t *src = test_src, *dest = test_dest;
	for( int i = 0; i < samples * channels; ++i, src += input_sample_size_in_bytes, dest += output_sample_size_in_bytes )
	{
		BOOST_CHECK_EQUAL( src[1], dest[0] );
		BOOST_CHECK_EQUAL( src[2], dest[1] );
		BOOST_CHECK_EQUAL( src[3], dest[2] );
	}
}

BOOST_AUTO_TEST_CASE( pack_aiff24_from_pcm32 )
{	
	const int samples = 2, channels = 2, input_sample_size_in_bytes = 4, output_sample_size_in_bytes = 3;
	uint8_t test_src[samples * channels * input_sample_size_in_bytes];
	uint8_t test_dest[samples * channels * output_sample_size_in_bytes];

	initialize_array( test_src, sizeof( test_src ) );
	au::pack_aiff24_from_pcm32( test_dest, test_src, samples, channels );
	
	uint8_t *src = test_src, *dest = test_dest;
	for( int i = 0; i < samples * channels; ++i, src += input_sample_size_in_bytes, dest += output_sample_size_in_bytes )
	{
		BOOST_CHECK_EQUAL( src[1], dest[2] );
		BOOST_CHECK_EQUAL( src[2], dest[1] );
		BOOST_CHECK_EQUAL( src[3], dest[0] );
	}
}

BOOST_AUTO_TEST_CASE( byteswap16_inplace )
{	
	// byteswap16_inplace uses SSE instructions that need input data to be 16-byte aligned. so use aligned_alloc to allocate test data.
	boost::shared_ptr< uint8_t > test_buf( static_cast< uint8_t * >( olib::opencorelib::utilities::aligned_alloc( 16, 32 ) ), &olib::opencorelib::utilities::aligned_free );
	initialize_array( test_buf.get(), 32 );
	uint8_t test_orig_values[32];
	memcpy( test_orig_values, test_buf.get(), 32 );

	au::byteswap16_inplace( test_buf.get(), 32 );

	for( int i = 0; i < 32; i += 2 )
	{
		BOOST_CHECK_EQUAL( test_buf.get()[i], test_orig_values[i + 1] );
		BOOST_CHECK_EQUAL( test_buf.get()[i + 1], test_orig_values[i] );
	}
}

BOOST_AUTO_TEST_SUITE_END( )

BOOST_AUTO_TEST_SUITE( image_utilities )

namespace {

typedef std::map< ml::image::MLPixelFormat, int > MLPixelFormat_int_map_type;
typedef std::map< ml::image::MLPixelFormat, std::vector< int > > MLPixelFormat_vector_int_map_type;

struct pixel_format_info
{
	int storage_bytes;
	int plane_count;
	int bitdepth[4];		// the bitdepth of each plane
	int linesize_factor[4]; // the amount to divide the width*storage_bytes by to get the linesize of each plane, negative values mean to multiply
	int width_factor[4];	// the amount to divide the width by to get the width of each plane
	int height_factor[4];	// the amount to divide the height by to get the height of each plane
};

struct pixel_format_pair
{
	ml::image::MLPixelFormat pf;
	pixel_format_info pfi;
};

// Add new pixel-formats here:
const pixel_format_pair pf_pairs[] = {
	// PF, storage_bytes, planes,				  [bitdepths],		[linesizes],	[widths],	[heights]
	{ ml::image::ML_PIX_FMT_YUV420P,      { 1, 3, {8,  8,  8,  0},  {1,  2, 2, 0}, {1, 2, 2, 0}, {1, 2, 2, 0} } },
	{ ml::image::ML_PIX_FMT_YUV420P10LE,  { 2, 3, {10, 10, 10, 0},  {1,  2, 2, 0}, {1, 2, 2, 0}, {1, 2, 2, 0} } },
	{ ml::image::ML_PIX_FMT_YUV420P16LE,  { 2, 3, {16, 16, 16, 0},  {1,  2, 2, 0}, {1, 2, 2, 0}, {1, 2, 2, 0} } },
	{ ml::image::ML_PIX_FMT_YUVA420P,     { 1, 4, {8,  8,  8,  8},  {1,  2, 2, 1}, {1, 2, 2, 1}, {1, 2, 2, 1} } },
	{ ml::image::ML_PIX_FMT_UYV422,       { 1, 1, {8,  0,  0,  0},  {-2, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0} } },
	{ ml::image::ML_PIX_FMT_YUV422P,      { 1, 3, {8,  8,  8,  0},  {1,  2, 2, 0}, {1, 2, 2, 0}, {1, 1, 1, 0} } },
	{ ml::image::ML_PIX_FMT_YUV422,       { 1, 1, {8,  0,  0,  0},  {-2, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0} } },
	{ ml::image::ML_PIX_FMT_YUV422P10LE,  { 2, 3, {10, 10, 10, 0},  {1,  2, 2, 0}, {1, 2, 2, 0}, {1, 1, 1, 0} } },
	{ ml::image::ML_PIX_FMT_YUV422P16LE,  { 2, 3, {16, 16, 16, 0},  {1,  2, 2, 0}, {1, 2, 2, 0}, {1, 1, 1, 0} } },
	{ ml::image::ML_PIX_FMT_YUV444P,      { 1, 3, {8,  8,  8,  0},  {1,  1, 1, 0}, {1, 1, 1, 0}, {1, 1, 1, 0} } },
	{ ml::image::ML_PIX_FMT_YUVA444P,     { 1, 4, {8,  8,  8,  8},  {1,  1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1} } },
	{ ml::image::ML_PIX_FMT_YUV444P16LE,  { 2, 3, {16, 16, 16, 0},  {1,  1, 1, 0}, {1, 1, 1, 0}, {1, 1, 1, 0} } },
	{ ml::image::ML_PIX_FMT_YUVA444P16LE, { 2, 4, {16, 16, 16, 16}, {1,  1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1} } },
	{ ml::image::ML_PIX_FMT_YUV411P,      { 1, 3, {8,  8,  8,  0},  {1,  4, 4, 0}, {1, 4, 4, 0}, {1, 1, 1, 0} } },
	{ ml::image::ML_PIX_FMT_L8,           { 1, 1, {8,  0,  0,  0},  {1,  0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0} } },
	{ ml::image::ML_PIX_FMT_L16LE,        { 2, 1, {16, 0,  0,  0},  {1,  0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0} } },
	{ ml::image::ML_PIX_FMT_R8G8B8,       { 1, 1, {8,  0,  0,  0},  {-3, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0} } },
	{ ml::image::ML_PIX_FMT_B8G8R8,       { 1, 1, {8,  0,  0,  0},  {-3, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0} } },
	{ ml::image::ML_PIX_FMT_R8G8B8A8,     { 1, 1, {8,  0,  0,  0},  {-4, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0} } },
	{ ml::image::ML_PIX_FMT_B8G8R8A8,     { 1, 1, {8,  0,  0,  0},  {-4, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0} } },
	{ ml::image::ML_PIX_FMT_A8R8G8B8,     { 1, 1, {8,  0,  0,  0},  {-4, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0} } },
	{ ml::image::ML_PIX_FMT_A8B8G8R8,     { 1, 1, {8,  0,  0,  0},  {-4, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0} } },
	{ ml::image::ML_PIX_FMT_R16G16B16LE,  { 2, 1, {16, 0,  0,  0},  {-3, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0} } }
};

typedef std::map< ml::image::MLPixelFormat, pixel_format_info > MLPixelFormat_info_map_type;

MLPixelFormat_info_map_type pf_infos;

// convert the array to a map
// It's done like this just to provide a prettier, more readable way of defining the pixel format infos above
void init_pf_infos()
{
	int number_entries = sizeof(pf_pairs) / sizeof(pixel_format_pair);
	for (int i = 0; i < number_entries; ++i )
	{
		pf_infos[ pf_pairs[i].pf ] = pf_pairs[i].pfi;
	}
}

class pf_info_initializer
{
	public:
	pf_info_initializer()
	{
		init_pf_infos();
	}
};

pf_info_initializer initializer_object;


}


void test_image( ml::image_type_ptr image, ml::image::MLPixelFormat pf, int width, int height )
{
	BOOST_CHECK_EQUAL( image->ml_pixel_format( ), pf );
	BOOST_CHECK_EQUAL( image->height( ), height);
	BOOST_CHECK_EQUAL( image->width( ), width);
	BOOST_CHECK_EQUAL( image->storage_bytes( ), pf_infos[ pf ].storage_bytes );
	BOOST_CHECK_EQUAL( image->plane_count( ), pf_infos[ pf ].plane_count );


	for (int i=0; i < image->plane_count( ); ++i )
	{
		int linesize = pf_infos[ pf ].linesize_factor[ i ] < 0 ? image->width() * -1 * pf_infos[ pf ].linesize_factor[ i ] 
																: image->width() / pf_infos[ pf ].linesize_factor[ i ];
		linesize *= pf_infos[ pf ].storage_bytes;

		BOOST_CHECK_EQUAL( image->linesize( i ), linesize );
		BOOST_CHECK_EQUAL( image->width( i ), image->width( ) / pf_infos[ pf ].width_factor[ i ] );
		BOOST_CHECK_EQUAL( image->height( i ), image->height( ) / pf_infos[ pf ].height_factor[ i ] );
		BOOST_CHECK_EQUAL( image->bitdepth( i ), pf_infos[ pf ].bitdepth[ i ] );
	}

}

ml::image_type_ptr allocate_image( ml::image::MLPixelFormat pf, int width, int height )
{	
	ml::image_type_ptr return_image = ml::image::allocate( pf, width, height );
	BOOST_REQUIRE( return_image );
    
	std::memset( static_cast<boost::uint8_t*>(return_image->ptr()), 0x0, return_image->size( ) );
	
	test_image( return_image, pf, width, height );
	
	return return_image;
}

BOOST_AUTO_TEST_CASE( image_pixel_formats_mappings_exist )
{	
	// check that there exists mappings to t_string for all MLPixelFormat:s 
	BOOST_CHECK_EQUAL( ml::image::MLPixelFormatMap.size(), ml::image::ML_PIX_FMT_NB );

	// check that the test tests all pixel formats (i.e. this should fail if someone adds a pixel format 
	// and doesn't add it to the maps above)
	BOOST_CHECK_EQUAL( pf_infos.size(), ml::image::ML_PIX_FMT_NB );
}

BOOST_AUTO_TEST_CASE( ML_pix_fmt_to_AV_pix_fmt )
{
	for(ml::image::MLPixelFormatMap_type::const_iterator i = ml::image::MLPixelFormatMap.begin(), 
		e = ml::image::MLPixelFormatMap.end(); i != e; ++i)
	{
		ml::image::MLPixelFormat mlpf = i->second;
		BOOST_REQUIRE( -1 != ml::image::ML_to_AV( i->second ) );
	}
}

BOOST_AUTO_TEST_CASE( image_convert )
{	
	for ( int width = 4; width < 4000; width += 2000 )
	{
		for ( int height = 2; height < 2000; height += 1000 )
		{
			for(MLPixelFormat_info_map_type::const_iterator i = pf_infos.begin(),
				e = pf_infos.end(); i != e; ++i)
			{
				ml::image_type_ptr image_to_convert = allocate_image( i->first, width, height );

				for(MLPixelFormat_info_map_type::const_iterator j = pf_infos.begin(),
					f = pf_infos.end(); j != f; ++j)
				{
					ml::image_type_ptr converted_image  = ml::image::convert( image_to_convert, j->first);
					test_image( converted_image, j->first, width, height );
				}
			}
		}
	}



}

BOOST_AUTO_TEST_SUITE_END( )

