#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/image/image.hpp>
#include <openmedialib/ml/openmedialib_plugin.hpp>
#include <set>

namespace ml = olib::openmedialib::ml;
namespace image = ml::image;

BOOST_AUTO_TEST_SUITE( image_class )

struct image_properties
{
	int width;
	int height;
	int bitdepth;
	int plane_count;
	int num_components;
	int linesize[4];
	int widths[4];
	int heights[4];
};

void test_image_properties( ml::image_type_ptr img, image_properties *p )
{
    BOOST_REQUIRE( img );
    BOOST_CHECK( img->width( ) == p->width );
    BOOST_CHECK( img->height( ) == p->height );
    BOOST_CHECK( img->bitdepth( ) == p->bitdepth );
    BOOST_CHECK( img->plane_count( ) == p->plane_count );
    BOOST_CHECK( img->num_components( ) == p->num_components );    
    std::set< int > offsets;
    for ( int i = 0; i < p->plane_count; i++ ) {
        BOOST_CHECK( img->linesize( i ) == p->linesize[i] );
        BOOST_CHECK( img->width( i ) == p->widths[i] );
        BOOST_CHECK( img->height( i ) == p->heights[i] );
        BOOST_CHECK( img->pitch( i ) >= img->linesize( i ) );
        BOOST_CHECK( img->pitch( i ) % ml::image::alignment( ) == 0 );
        offsets.insert( img->offset( i ) );
        int offset_min = img->offset( i );
        int offset_max = offset_min + img->height( i ) * img->pitch( i );
        BOOST_CHECK( offset_min >= 0 );
        BOOST_CHECK( offset_max <= img->size( ) );
    }
    BOOST_CHECK( int( offsets.size( ) ) == p->plane_count );
}

BOOST_AUTO_TEST_CASE( allocate )
{
	/*
	image properties: WIDTH, HEIGHT, BITDEPTH, PLANE_COUNT, BLOCK_SIZE, [ LINESIZE ] , [ WIDTHS ], [ HEIGHTS ]
	*/
	image_properties r8g8b8_137_791 = { 137, 791, 8, 1, 3, { 411 }, { 137 }, { 791 } };
	test_image_properties( ml::image::allocate( ml::image::ML_PIX_FMT_R8G8B8, 137, 791 ), &r8g8b8_137_791 );

	// RGB
	image_properties r8g8b8_1920_1080 = { 1920, 1080, 8, 1, 3, { 5760 }, { 1920 }, { 1080 } };
	test_image_properties( ml::image::allocate( ml::image::ML_PIX_FMT_R8G8B8, 1920, 1080 ), &r8g8b8_1920_1080 );
	
	// YUV420P
	image_properties yuv420p_1920_1080 = { 1920, 1080, 8, 3, 3, { 1920, 960, 960 }, { 1920, 960, 960 }, { 1080, 540, 540 } };
	test_image_properties( ml::image::allocate( ml::image::ML_PIX_FMT_YUV420P, 1920, 1080 ), &yuv420p_1920_1080 );

	// YUVA420P
	image_properties yuva420p_1920_1080 = { 1920, 1080, 8, 4, 4, { 1920, 960, 960, 1920 }, { 1920, 960, 960, 1920 }, { 1080, 540, 540, 1080 } };
	test_image_properties( ml::image::allocate( ml::image::ML_PIX_FMT_YUVA420P, 1920, 1080 ), &yuva420p_1920_1080 );
	
	// YUV422P
	image_properties yuv422p_1920_1080 = { 1920, 1080, 8, 3, 3, { 1920, 960, 960 }, { 1920, 960, 960 }, { 1080, 1080, 1080 } };
	test_image_properties( ml::image::allocate( ml::image::ML_PIX_FMT_YUV422P, 1920, 1080 ), &yuv422p_1920_1080 );

	// YUV422P10LE
	image_properties yuv422p10le_1920_1080 = { 1920, 1080, 10, 3, 3, { 3840, 1920, 1920 }, { 1920, 960, 960 }, { 1080, 1080, 1080 } };
	test_image_properties( ml::image::allocate( ml::image::ML_PIX_FMT_YUV422P10LE, 1920, 1080 ), &yuv422p10le_1920_1080 );
}

void convert_yuv420p_to_yuv422p( )
{
	ml::image_type_ptr src = ml::image::allocate( ml::image::ML_PIX_FMT_YUV420P, 1920, 1080 );
	
	ml::image_type_ptr dst = ml::image::convert( src, ml::image::ML_PIX_FMT_YUV422P );

	BOOST_CHECK( dst->ml_pixel_format( ) == ml::image::ML_PIX_FMT_YUV422P );
}

void convert_yuv422p_to_yuv420p( )
{
	ml::image_type_ptr src = ml::image::allocate( ml::image::ML_PIX_FMT_YUV422P, 1920, 1080 );
	
	ml::image_type_ptr dst = ml::image::convert( src, ml::image::ML_PIX_FMT_YUV420P );

	BOOST_CHECK( dst->ml_pixel_format( ) == ml::image::ML_PIX_FMT_YUV420P );
}

BOOST_AUTO_TEST_CASE( convert )
{
	convert_yuv420p_to_yuv422p();	
	convert_yuv422p_to_yuv420p();	

}


void rescale_yuv420p()
{
    ml::image_type_ptr src = ml::image::allocate( ml::image::ML_PIX_FMT_YUV420P, 1280, 720 );
    ml::image_type_ptr dst = ml::image::rescale( src, 1920, 1080, ml::image::BICUBIC_SAMPLING );
}

BOOST_AUTO_TEST_CASE( rescale )
{
    rescale_yuv420p( );
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( image_utilities )

namespace {

typedef std::map< ml::image::MLPixelFormat, int > MLPixelFormat_int_map_type;
typedef std::map< ml::image::MLPixelFormat, std::vector< int > > MLPixelFormat_vector_int_map_type;
typedef boost::rational< int > ratio;

struct pixel_format_info
{
	int storage_bytes;
	int plane_count;
	int bitdepth[4];			// the bitdepth of each plane
	ratio linesize_factor[4];	// the ratio to multiply the width*storage_bytes by to get the linesize of each plane
	int width_factor[4];		// the amount to divide the width by to get the width of each plane
	int height_factor[4];		// the amount to divide the height by to get the height of each plane
	int multiple[2];			// the number of pixels which represent the factor of the acceptable widths and heights
	int alpha_offset;			// The offset from plane 0 of the alpha sample (-1 indicates no alpha channel), units are in samples
	int alpha_plane;			// The index of the alpha plane (-1 indicates no alpha plane)
};

struct pixel_format_pair
{
	ml::image::MLPixelFormat pf;
	pixel_format_info pfi;
};

const ratio M0( 0 );
const ratio M1( 1 );
const ratio M2( 2 );
const ratio M3( 3 );
const ratio M4( 4 );
const ratio D2( 1, 2 );
const ratio D4( 1, 4 );

// Add new pixel-formats here:
const pixel_format_pair pf_pairs[] = {
	// PF, storage_bytes, planes,				  [bitdepths],		[linesizes],		[widths],	[heights], [multiple], ao, ap
	{ ml::image::ML_PIX_FMT_YUV420P,      { 1, 3, {8,  8,  8,  0},  {M1, D2, D2, M0}, {1, 2, 2, 0}, {1, 2, 2, 0}, {2, 2}, -1, -1 } },
	{ ml::image::ML_PIX_FMT_YUV420P10LE,  { 2, 3, {10, 10, 10, 0},  {M1, D2, D2, M0}, {1, 2, 2, 0}, {1, 2, 2, 0}, {2, 2}, -1, -1 } },
	{ ml::image::ML_PIX_FMT_YUV420P16LE,  { 2, 3, {16, 16, 16, 0},  {M1, D2, D2, M0}, {1, 2, 2, 0}, {1, 2, 2, 0}, {2, 2}, -1, -1 } },
	{ ml::image::ML_PIX_FMT_YUVA420P,     { 1, 4, {8,  8,  8,  8},  {M1, D2, D2, M1}, {1, 2, 2, 1}, {1, 2, 2, 1}, {2, 2},  0,  3 } },
	{ ml::image::ML_PIX_FMT_UYV422,       { 1, 1, {8,  0,  0,  0},  {M2, M0, M0, M0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {2, 1}, -1, -1 } },
	{ ml::image::ML_PIX_FMT_YUV422P,      { 1, 3, {8,  8,  8,  0},  {M1, D2, D2, M0}, {1, 2, 2, 0}, {1, 1, 1, 0}, {2, 1}, -1, -1 } },
	{ ml::image::ML_PIX_FMT_YUV422,       { 1, 1, {8,  0,  0,  0},  {M2, M0, M0, M0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {2, 1}, -1, -1 } },
	{ ml::image::ML_PIX_FMT_YUV422P10LE,  { 2, 3, {10, 10, 10, 0},  {M1, D2, D2, M0}, {1, 2, 2, 0}, {1, 1, 1, 0}, {2, 1}, -1, -1 } },
	{ ml::image::ML_PIX_FMT_YUV422P16LE,  { 2, 3, {16, 16, 16, 0},  {M1, D2, D2, M0}, {1, 2, 2, 0}, {1, 1, 1, 0}, {2, 1}, -1, -1 } },
	{ ml::image::ML_PIX_FMT_YUV444P,      { 1, 3, {8,  8,  8,  0},  {M1, M1, M1, M0}, {1, 1, 1, 0}, {1, 1, 1, 0}, {1, 1}, -1, -1 } },
	{ ml::image::ML_PIX_FMT_YUVA444P,     { 1, 4, {8,  8,  8,  8},  {M1, M1, M1, M1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1},  0,  3 } },
	{ ml::image::ML_PIX_FMT_YUV444P16LE,  { 2, 3, {16, 16, 16, 0},  {M1, M1, M1, M0}, {1, 1, 1, 0}, {1, 1, 1, 0}, {1, 1}, -1, -1 } },
	{ ml::image::ML_PIX_FMT_YUVA444P16LE, { 2, 4, {16, 16, 16, 16}, {M1, M1, M1, M1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1},  0,  3 } },
	{ ml::image::ML_PIX_FMT_YUV411P,      { 1, 3, {8,  8,  8,  0},  {M1, D4, D4, M0}, {1, 4, 4, 0}, {1, 1, 1, 0}, {4, 1}, -1, -1 } },
	{ ml::image::ML_PIX_FMT_L8,           { 1, 1, {8,  0,  0,  0},  {M1, M0, M0, M0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1}, -1, -1 } },
	{ ml::image::ML_PIX_FMT_L16LE,        { 2, 1, {16, 0,  0,  0},  {M1, M0, M0, M0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1}, -1, -1 } },
	{ ml::image::ML_PIX_FMT_R8G8B8,       { 1, 1, {8,  0,  0,  0},  {M3, M0, M0, M0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1}, -1, -1 } },
	{ ml::image::ML_PIX_FMT_B8G8R8,       { 1, 1, {8,  0,  0,  0},  {M3, M0, M0, M0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1}, -1, -1 } },
	{ ml::image::ML_PIX_FMT_R8G8B8A8,     { 1, 1, {8,  0,  0,  0},  {M4, M0, M0, M0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1},  3,  0 } },
	{ ml::image::ML_PIX_FMT_B8G8R8A8,     { 1, 1, {8,  0,  0,  0},  {M4, M0, M0, M0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1},  3,  0 } },
	{ ml::image::ML_PIX_FMT_A8R8G8B8,     { 1, 1, {8,  0,  0,  0},  {M4, M0, M0, M0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1},  0,  0 } },
	{ ml::image::ML_PIX_FMT_A8B8G8R8,     { 1, 1, {8,  0,  0,  0},  {M4, M0, M0, M0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1},  0,  0 } },
	{ ml::image::ML_PIX_FMT_R16G16B16LE,  { 2, 1, {16, 0,  0,  0},  {M3, M0, M0, M0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1}, -1, -1 } }
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
	BOOST_CHECK_EQUAL( image->alpha_offset( ), pf_infos[ pf ].alpha_offset );
	BOOST_CHECK_EQUAL( image->alpha_plane( ), pf_infos[ pf ].alpha_plane );

	for (int i=0; i < image->plane_count( ); ++i )
	{
		int linesize = boost::rational_cast< int >( image->width() * pf_infos[ pf ].linesize_factor[ i ] ) * pf_infos[ pf ].storage_bytes;
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
	// check that the test tests all pixel formats (i.e. this should fail if someone adds a pixel format 
	// and doesn't add it to the maps above)
	BOOST_CHECK_EQUAL( pf_infos.size(), ml::image::ML_PIX_FMT_NB );
}

BOOST_AUTO_TEST_CASE( ML_pix_fmt_to_AV_pix_fmt )
{
	BOOST_REQUIRE_EQUAL( static_cast<int>( ml::image::ML_PIX_FMT_NONE ), -1 );

	for( int pf = 0; pf < ml::image::ML_PIX_FMT_NB; ++pf )
	{
		BOOST_CHECK( -1 != ml::image::ML_to_AV( static_cast<ml::image::MLPixelFormat>( pf ) ) );
	}
}

BOOST_AUTO_TEST_CASE( image_multiples )
{
	size_t tests = sizeof( pf_pairs ) / sizeof( pixel_format_pair );
	for( size_t i = 0; i < tests; i ++ )
	{
		const struct pixel_format_pair &t = pf_pairs[ i ];
		int tw = 1, th = 1;
		ml::image::correct( t.pf, tw, th, ml::image::ceil );
		BOOST_CHECK( tw == t.pfi.multiple[ 0 ] );
		BOOST_CHECK( th == t.pfi.multiple[ 1 ] );
	}
};

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

BOOST_AUTO_TEST_CASE( compositing_pixfmts )
{
	size_t tests = sizeof( pf_pairs ) / sizeof( pixel_format_pair );
	for( size_t i = 0; i < tests; i ++ )
	{
		const struct pixel_format_pair &t = pf_pairs[ i ];
		BOOST_CHECK_NO_THROW( ml::image::composite_pf( t.pf ) );
	}
}

struct dim
{
	int w;
	int h;
};

struct correction_info
{
	ml::image::MLPixelFormat pf;
	struct dim input;
	bool valid;
	struct dim nearest;
	struct dim floor;
	struct dim ceil;
};

// We only need to test one of each of the pixel multiple combinations since the results of
// each with the same values are identical (see image_multiples above). 
//
// Hence:
//
// * yuv444p is selected to test those with 1,1
// * yuv422p is selected to test those with 2,1
// * yuv420p is selected to test those with 2,2
// * yuv411p is selected to test those with 4,1
//
// Note that the ml::image::correct and ml::image::verify functions correct and validate 
// coordinates - this means they include a subset of values which are not valid image 
// dimensions - namely, values of w * h == 0 are seen as valid.

const correction_info correction_tests[] = {
	// PF                                 input		valid 	nearest		floor	  ceil
	{ ml::image::ML_PIX_FMT_YUV444P,      { 0, 0 }, true, 	{ 0, 0 }, { 0, 0 }, { 0, 0 } },
	{ ml::image::ML_PIX_FMT_YUV444P,      { 1, 0 }, true, 	{ 1, 0 }, { 1, 0 }, { 1, 0 } },
	{ ml::image::ML_PIX_FMT_YUV444P,      { 1, 1 }, true, 	{ 1, 1 }, { 1, 1 }, { 1, 1 } },
	{ ml::image::ML_PIX_FMT_YUV444P,      { 2, 1 }, true, 	{ 2, 1 }, { 2, 1 }, { 2, 1 } },

	{ ml::image::ML_PIX_FMT_YUV422P,      { 0, 0 }, true, 	{ 0, 0 }, { 0, 0 }, { 0, 0 } },
	{ ml::image::ML_PIX_FMT_YUV422P,      { 1, 0 }, false, 	{ 2, 0 }, { 0, 0 }, { 2, 0 } },
	{ ml::image::ML_PIX_FMT_YUV422P,      { 1, 1 }, false, 	{ 2, 1 }, { 0, 1 }, { 2, 1 } },
	{ ml::image::ML_PIX_FMT_YUV422P,      { 2, 1 }, true, 	{ 2, 1 }, { 2, 1 }, { 2, 1 } },
	{ ml::image::ML_PIX_FMT_YUV422P,      { 2, 2 }, true, 	{ 2, 2 }, { 2, 2 }, { 2, 2 } },
	{ ml::image::ML_PIX_FMT_YUV422P,      { 3, 2 }, false, 	{ 4, 2 }, { 2, 2 }, { 4, 2 } },

	{ ml::image::ML_PIX_FMT_YUV420P,      { 0, 0 }, true, 	{ 0, 0 }, { 0, 0 }, { 0, 0 } },
	{ ml::image::ML_PIX_FMT_YUV420P,      { 1, 0 }, false, 	{ 2, 0 }, { 0, 0 }, { 2, 0 } },
	{ ml::image::ML_PIX_FMT_YUV420P,      { 1, 1 }, false, 	{ 2, 2 }, { 0, 0 }, { 2, 2 } },
	{ ml::image::ML_PIX_FMT_YUV420P,      { 2, 1 }, false, 	{ 2, 2 }, { 2, 0 }, { 2, 2 } },
	{ ml::image::ML_PIX_FMT_YUV420P,      { 2, 2 }, true, 	{ 2, 2 }, { 2, 2 }, { 2, 2 } },
	{ ml::image::ML_PIX_FMT_YUV420P,      { 3, 2 }, false, 	{ 4, 2 }, { 2, 2 }, { 4, 2 } },

	{ ml::image::ML_PIX_FMT_YUV411P,      { 0, 0 }, true, 	{ 0, 0 }, { 0, 0 }, { 0, 0 } },
	{ ml::image::ML_PIX_FMT_YUV411P,      { 1, 1 }, false, 	{ 0, 1 }, { 0, 1 }, { 4, 1 } },
	{ ml::image::ML_PIX_FMT_YUV411P,      { 2, 1 }, false, 	{ 4, 1 }, { 0, 1 }, { 4, 1 } },
	{ ml::image::ML_PIX_FMT_YUV411P,      { 3, 1 }, false, 	{ 4, 1 }, { 0, 1 }, { 4, 1 } },
	{ ml::image::ML_PIX_FMT_YUV411P,      { 4, 1 }, true, 	{ 4, 1 }, { 4, 1 }, { 4, 1 } },
	{ ml::image::ML_PIX_FMT_YUV411P,      { 4, 2 }, true, 	{ 4, 2 }, { 4, 2 }, { 4, 2 } },
};

void check_correct( ml::image::MLPixelFormat pf, const struct dim &input, const struct dim &output, enum ml::image::correction type )
{
	int tw = input.w, th = input.h;
	ml::image::correct( pf, tw, th, type );
	if ( tw != output.w || th != output.h ) std::cerr << pf << " " << type << " " << input.w << "x" << input.h << " wants " << output.w << "x" << output.h << " got " << tw << "x" << th << std::endl;
	BOOST_CHECK( tw == output.w );
	BOOST_CHECK( th == output.h );
}

BOOST_AUTO_TEST_CASE( image_correct )
{
	for( size_t i = 0; i < sizeof( correction_tests ) / sizeof( correction_info ); i ++ )
	{
		//std::cerr << "row " << i << std::endl;
		const struct correction_info &t = correction_tests[ i ];
		BOOST_CHECK( ml::image::verify( t.pf, t.input.w, t.input.h ) == t.valid );
		check_correct( t.pf, t.input, t.nearest, ml::image::nearest );
		check_correct( t.pf, t.input, t.floor, ml::image::floor );
		check_correct( t.pf, t.input, t.ceil, ml::image::ceil );
	}
}


BOOST_AUTO_TEST_CASE( image_rescale_object )
{
	ml::rescale_object_ptr ro( new ml::image::rescale_object );
	BOOST_CHECK( ro->get_context( ml::image::ML_PIX_FMT_YUV420P ) == 0 );
	BOOST_CHECK( ro->get_context( ml::image::ML_PIX_FMT_L8 ) == 0 );
	ml::image_type_ptr image = ml::image::allocate( ml::image::ML_PIX_FMT_YUV420P, 720, 576 );
	BOOST_REQUIRE( image );
	ml::image_type_ptr alpha = ml::image::allocate( ml::image::ML_PIX_FMT_L8, 720, 576 );
	BOOST_REQUIRE( alpha );
	ml::image_type_ptr image2 = ml::image::rescale( ro, image, 1920, 1080 );
	BOOST_REQUIRE( image2 );
	ml::image_type_ptr alpha2 = ml::image::rescale( ro, alpha, 1920, 1080 );
	BOOST_REQUIRE( alpha2 );
	BOOST_CHECK( ro->get_context( ml::image::ML_PIX_FMT_YUV420P ) != 0 );
	BOOST_CHECK( ro->get_context( ml::image::ML_PIX_FMT_L8 ) != 0 );
	BOOST_CHECK( ro->get_context( ml::image::ML_PIX_FMT_YUV420P ) != ro->get_context( ml::image::ML_PIX_FMT_L8 ) );
	BOOST_CHECK( image2->width( ) == 1920 );
	BOOST_CHECK( image2->height( ) == 1080 );
	BOOST_CHECK( alpha2->width( ) == 1920 );
	BOOST_CHECK( alpha2->height( ) == 1080 );
}

BOOST_AUTO_TEST_CASE( frame_rescale_object )
{
	ml::rescale_object_ptr ro( new ml::image::rescale_object );
	BOOST_CHECK( ro->get_context( ml::image::ML_PIX_FMT_YUV420P ) == 0 );
	BOOST_CHECK( ro->get_context( ml::image::ML_PIX_FMT_L8 ) == 0 );
	ml::image_type_ptr image = ml::image::allocate( ml::image::ML_PIX_FMT_YUV420P, 720, 576 );
	BOOST_REQUIRE( image );
	ml::image_type_ptr alpha = ml::image::allocate( ml::image::ML_PIX_FMT_L8, 720, 576 );
	BOOST_REQUIRE( alpha );
	ml::frame_type_ptr frame( new ml::frame_type );
	frame->set_position( 0 );
	frame->set_image( image );
	frame->set_alpha( alpha );
	frame->set_sar( 1, 1);
	ml::frame_type_ptr frame2 = ml::frame_rescale( ro, frame, 1920, 1080 );
	BOOST_REQUIRE( frame2 );
	BOOST_CHECK( frame2->get_image( ) );
	BOOST_CHECK( frame2->get_alpha( ) );
	BOOST_CHECK( ro->get_context( ml::image::ML_PIX_FMT_YUV420P ) != 0 );
	BOOST_CHECK( ro->get_context( ml::image::ML_PIX_FMT_L8 ) != 0 );
	BOOST_CHECK( ro->get_context( ml::image::ML_PIX_FMT_YUV420P ) != ro->get_context( ml::image::ML_PIX_FMT_L8 ) );
	BOOST_CHECK( frame2->width( ) == 1920 );
	BOOST_CHECK( frame2->height( ) == 1080 );
	BOOST_CHECK( frame2->get_sar_num( ) == 1 );
	BOOST_CHECK( frame2->get_sar_den( ) == 1 );
	BOOST_CHECK( frame2->get_image( )->width( ) == 1920 );
	BOOST_CHECK( frame2->get_image( )->height( ) == 1080 );
	BOOST_CHECK( frame2->get_image( )->get_sar_num( ) == 1 );
	BOOST_CHECK( frame2->get_image( )->get_sar_den( ) == 1 );
	BOOST_CHECK( frame2->get_alpha( )->width( ) == 1920 );
	BOOST_CHECK( frame2->get_alpha( )->height( ) == 1080 );
	BOOST_CHECK( frame2->get_alpha( )->get_sar_num( ) == 1 );
	BOOST_CHECK( frame2->get_alpha( )->get_sar_den( ) == 1 );
}

BOOST_AUTO_TEST_CASE( frame_convert_object )
{
	ml::rescale_object_ptr ro( new ml::image::rescale_object );
	ml::image_type_ptr image = ml::image::allocate( ml::image::ML_PIX_FMT_YUV420P, 720, 576 );
	BOOST_REQUIRE( image );
	ml::image_type_ptr alpha = ml::image::allocate( ml::image::ML_PIX_FMT_L8, 720, 576 );
	BOOST_REQUIRE( alpha );
	ml::frame_type_ptr frame( new ml::frame_type );
	frame->set_position( 0 );
	frame->set_image( image );
	frame->set_alpha( alpha );
	frame->set_sar( 16, 15 );
	ml::frame_type_ptr frame2 = ml::frame_convert( ro, frame, _CT( "yuva444p" ) );
	BOOST_REQUIRE( frame2 );
	BOOST_CHECK( frame2->get_image( ) );
	BOOST_CHECK( !frame2->get_alpha( ) );
	BOOST_CHECK( frame2->pf( ) == _CT( "yuva444p" ) );
	BOOST_CHECK( frame2->width( ) == 720 );
	BOOST_CHECK( frame2->height( ) == 576 );
	BOOST_CHECK( frame2->get_sar_num( ) == 16 );
	BOOST_CHECK( frame2->get_sar_den( ) == 15 );
}

typedef struct
{
	image::MLPixelFormat pf;
	int in[ 4 ];
	int components;
	int out[ 4 ];
}
rgb_arrangement;

rgb_arrangement rgb[ ] = 
{
	// pf                                   in     components    out
	{ image::ML_PIX_FMT_R8G8B8, 		{ 1, 2, 3, 4 }, 3, { 1, 2, 3, -1 } },
	{ image::ML_PIX_FMT_B8G8R8, 		{ 1, 2, 3, 4 }, 3, { 3, 2, 1, -1 } },
	{ image::ML_PIX_FMT_R8G8B8A8, 		{ 1, 2, 3, 4 }, 4, { 1, 2, 3, 4 } },
	{ image::ML_PIX_FMT_B8G8R8A8, 		{ 1, 2, 3, 4 }, 4, { 3, 2, 1, 4 } },
	{ image::ML_PIX_FMT_A8R8G8B8, 		{ 1, 2, 3, 4 }, 4, { 4, 1, 2, 3 } },
	{ image::ML_PIX_FMT_A8B8G8R8, 		{ 1, 2, 3, 4 }, 4, { 4, 3, 2, 1 } },
	{ image::ML_PIX_FMT_R16G16B16LE, 	{ 1, 2, 3, 4 }, 3, { 257, 514, 771, -1 } },
};

BOOST_AUTO_TEST_CASE( rgb_arrangement_test )
{
	int tests = sizeof( rgb ) / sizeof( rgb_arrangement );
	for ( int i = 0; i < tests; i ++ )
	{
		const rgb_arrangement &test = rgb[ i ];
		int samples[ 4 ];
		const int components = image::arrange_rgb( test.pf, samples, test.in[ 0 ], test.in[ 1 ], test.in[ 2 ], test.in[ 3 ] );
		BOOST_CHECK( components == test.components );
		bool ok = true;
		for ( int j = 0; ok && j < components; j ++ )
			ok = samples[ j ] == test.out[ j ];
		BOOST_CHECK( ok );
	}
}

BOOST_AUTO_TEST_SUITE_END( )

