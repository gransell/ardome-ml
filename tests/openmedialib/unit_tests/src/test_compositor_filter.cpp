#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/filter.hpp>
#include <openmedialib/ml/stack.hpp>
#include <openmedialib/ml/audio_mixer.hpp>

using namespace olib::openmedialib::ml;
using namespace olib::openpluginlib;

#define MEDIA_REPO_PREFIX L"http://releases.ardendo.se/media-repository/"
#define TEST_FILE L"MOV/DVCPro25/DVCPro25_PAL_4ch_16bit.mov"

input_type_ptr create_color_background()
{
	input_type_ptr result = create_input( L"colour:" );
	BOOST_REQUIRE( result );
	result->property( "width" ) = 720;
	result->property( "height" ) = 576;
	result->property( "sar_num" ) = 16;
	result->property( "sar_den" ) = 15;
	result->property( "interlace" ) = 2;
	result->property( "colourspace" ) = std::wstring( L"yuv411p" );

	return result;
}

input_type_ptr create_color_silence_background( )
{
	input_type_ptr color_bg = create_color_background();
	input_type_ptr silence = create_input( L"silence:" );
	BOOST_REQUIRE( silence );
	silence->property( "af" ) = std::wstring( L"pcm16" );
	silence->property( "channels" ) = 2;

	filter_type_ptr muxer = create_filter( L"muxer" );
	BOOST_REQUIRE( muxer );
	muxer->connect( color_bg, 0 );
	muxer->connect( silence, 1 );

	muxer->sync();

	return muxer;
}

input_type_ptr create_input_leg( int source_inpoint )
{
	input_type_ptr input = create_delayed_input( L"avformat:" MEDIA_REPO_PREFIX TEST_FILE );
	BOOST_REQUIRE( input );
	input->property( "packet_stream" ) = 1;
	BOOST_REQUIRE( input->init() );
	BOOST_REQUIRE_EQUAL( input->get_frames(), 287 );

	filter_type_ptr decode_filter = create_filter( L"decode" );
	BOOST_REQUIRE( decode_filter );
	decode_filter->connect( input );

	filter_type_ptr clip_filter = create_filter( L"clip" );
	BOOST_REQUIRE( clip_filter );
	clip_filter->property( "in" ) = source_inpoint;
	clip_filter->connect( decode_filter );

	filter_type_ptr offset_filter = create_filter( L"offset" );
	BOOST_REQUIRE( offset_filter );
	offset_filter->connect( clip_filter );

	offset_filter->sync();

	return offset_filter;
}

BOOST_AUTO_TEST_SUITE( compositor_filter )

void test_multiple_video_slots( bool deferred )
{
	input_type_ptr bg = create_color_silence_background();
	input_type_ptr input1 = create_input_leg( 20 );
	input_type_ptr input2 = create_input_leg( 50 );

	audio_type_ptr bg_audio = bg->fetch()->get_audio();
	audio_type_ptr audio1 = input1->fetch()->get_audio();
	audio_type_ptr audio2 = input2->fetch()->get_audio();
	BOOST_REQUIRE_EQUAL( bg_audio->samples(), audio1->samples() );
	BOOST_REQUIRE_EQUAL( bg_audio->samples(), audio2->samples() );
	audio_type_ptr reference_audio = audio::mixer( audio::mixer( bg_audio, audio1 ), audio2 );
	BOOST_REQUIRE( reference_audio );

	filter_type_ptr compositor_filter = create_filter( L"compositor" );
	compositor_filter->property( "deferred" ) = ( deferred ? 1 : 0 );
	compositor_filter->property( "slots" ) = 3;

	compositor_filter->connect( bg, 0 );
	compositor_filter->connect( input1, 1 );
	compositor_filter->connect( input2, 2 );
	compositor_filter->sync();

	stream_type_ptr stream1 = input1->fetch()->get_stream();
	BOOST_REQUIRE( stream1 );
	stream_type_ptr stream2 = input2->fetch()->get_stream();
	BOOST_REQUIRE( stream2 );

	frame_type_ptr result_frame = compositor_filter->fetch();
	BOOST_REQUIRE( result_frame );

	//Since the frame from input2 completely covers the background, it
	//obscures the frame from input1, so no compositing needs to be done.
	//This means that the original stream from input2 should have been
	//preserved.
	stream_type_ptr result_stream = result_frame->get_stream();
	BOOST_REQUIRE( result_stream );
	BOOST_REQUIRE_EQUAL( result_stream->length(), stream2->length() );
	BOOST_CHECK_EQUAL( memcmp( result_stream->bytes(), stream2->bytes(), result_stream->length() ), 0 );

	//Check that audio mixing between all the frame still took place
	audio_type_ptr result_audio = result_frame->get_audio();
	BOOST_REQUIRE( result_audio );
	BOOST_REQUIRE_EQUAL( result_audio->size(), reference_audio->size() );
	BOOST_CHECK_EQUAL( memcmp( result_audio->pointer(), reference_audio->pointer(), result_audio->size() ), 0 );
}

BOOST_AUTO_TEST_CASE( multiple_video_slots_deferred )
{
	test_multiple_video_slots( true );
}

BOOST_AUTO_TEST_CASE( multiple_video_slots_nondeferred )
{
	test_multiple_video_slots( false );
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

bool check_plane( image_type_ptr im, int plane, int value, int variance = 5 )
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
	const int var = ML_SCALE_SAMPLE( variance, ML_MAX_BIT_VALUE( im->bitdepth( ) ), 255 );

	int h = im->height( );
	bool ok = true;

	while( ok && h -- )
	{
		int w = im->width( );
		while ( ok && w -- )
		{
			int *c = samples;
			switch( components )
			{
				case 4:
					ok = in_range( int( *src ++ ), *c ++, var );
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

bool check_components( image_type_ptr im, int r, int g, int b, int a, int variance = 5 )
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

	if ( result )
	{
		if ( image::is_pixfmt_planar( image->ml_pixel_format( ) ) )
		{
			int y, u, v;
			image::rgb24_to_yuv444( y, u, v, r, g, b );
			result = check_plane( image, 0, y, variance );
			result = result && check_plane( image, 1, u, variance );
			result = result && check_plane( image, 2, v, variance );
			if ( a >= 0 )
			{
				if ( !alpha ) result = result && check_plane( image, 3, a, variance );
				else result = result && check_plane( alpha, 0, a, variance );
			}
		}
		else if ( image::is_pixfmt_rgb( image->ml_pixel_format( ) ) )
		{
			result = check_components( image, r, g, b, a );
		}
		else
		{
			result = false;
		}
	}

	return result;
}

bool check_frame( frame_type_ptr frame, int r, int g, int b, int a = -1, int variance = 5 )
{
	return frame && check_image( frame->get_image( ), frame->get_alpha( ), r, g, b, a, variance );
}

BOOST_AUTO_TEST_CASE( invalid_colour_test )
{
	// Attempts to create an invalid colour: input - expected behaviour is that 
	// each frame requested will have no image or alpha and they will be in error.

	input_type_ptr input = stack( ).push( L"colour: width=0 height=0" ).pop( );
	BOOST_REQUIRE( input );

	frame_type_ptr frame = input->fetch( );
	BOOST_REQUIRE( frame );
	BOOST_REQUIRE( frame->in_error( ) );
	BOOST_REQUIRE( !frame->get_image( ) );
	BOOST_REQUIRE( !frame->get_alpha( ) );
}

BOOST_AUTO_TEST_CASE( alpha_extract_merge )
{
	// Construct an input which provides black yuv420p + alpha mask
	input_type_ptr input = stack( ).push( L"colour: a=0" ).pop( );
	BOOST_REQUIRE( input );
	frame_type_ptr frame = input->fetch( );
	BOOST_REQUIRE( frame->get_image( ) );
	BOOST_REQUIRE( frame->get_alpha( ) );

	// Confirm that image is black and requested alpha mask is 0
	// Second test is to make sure that alpha is really checked in the test
	BOOST_CHECK( check_frame( frame, 0, 0, 0, 0 ) );
	BOOST_CHECK( !check_frame( frame, 0, 0, 0, 255 ) );

	// Store 0 alpha mask and remove from frame
	image_type_ptr alpha = frame->get_alpha( );
	frame->set_alpha( image_type_ptr( ) );

	// Convert to yuva420p - alpha plane should be populated with 255
	// Second test is to make sure that alpha is really checked in the test
	frame = frame_convert( rescale_object_ptr( ), frame, image::ML_PIX_FMT_YUVA420P );
	BOOST_CHECK( check_frame( frame, 0, 0, 0, 255 ) );
	BOOST_CHECK( !check_frame( frame, 0, 0, 0, 0 ) );

	// Merge 0 alpha and check that resultant image is all 0
	// Second test is to make sure that alpha is really checked in the test
	image_type_ptr image = merge_alpha( frame->get_image( ), alpha );
	frame->set_image( image );
	BOOST_CHECK( check_frame( frame, 0, 0, 0, 0 ) );
	BOOST_CHECK( !check_frame( frame, 0, 0, 0, 255 ) );
}

BOOST_AUTO_TEST_CASE( alpha_bg_plus_fg )
{
	// colour: r=255 colour: b=255 filter:compositor
	//
	// The output should be yuv420p. Image should be blue;

	input_type_ptr bg = create_delayed_input( L"colour:" );
	bg->property( "r" ) = 255;
	BOOST_REQUIRE( bg->init( ) );

	input_type_ptr fg = create_delayed_input( L"colour:" );
	fg->property( "b" ) = 255;
	BOOST_REQUIRE( fg->init( ) );

	filter_type_ptr compositor = create_filter( L"compositor" );
	compositor->connect( bg, 0 );
	compositor->connect( fg, 1 );

	frame_type_ptr frame = compositor->fetch( );
	BOOST_REQUIRE( frame );
	BOOST_REQUIRE( frame->get_image( ) );
	BOOST_REQUIRE( !frame->get_alpha( ) );

	BOOST_CHECK( check_frame( frame, 0, 0, 255 ) );
}

BOOST_AUTO_TEST_CASE( alpha_bg_plus_fga )
{
	// colour: background=0 r=255 colour: b=255 a=128 filter:compositor
	//
	// The output should be yuv420p. Image should be a dull purple.
	// There will be no alpha mask on the output.
	//
	// Note that background=0 is used to explictly inform the compositor that
	// the output should not be ignored. 

	input_type_ptr bg = create_delayed_input( L"colour:" );
	bg->property( "background" ) = 0;
	bg->property( "r" ) = 255;
	BOOST_REQUIRE( bg->init( ) );

	input_type_ptr fg = create_delayed_input( L"colour:" );
	fg->property( "b" ) = 255;
	fg->property( "a" ) = 128;
	BOOST_REQUIRE( fg->init( ) );

	filter_type_ptr compositor = create_filter( L"compositor" );
	compositor->connect( bg, 0 );
	compositor->connect( fg, 1 );

	frame_type_ptr frame = compositor->fetch( );
	BOOST_REQUIRE( frame );
	BOOST_REQUIRE( frame->get_image( ) );
	BOOST_REQUIRE( !frame->get_alpha( ) );

	BOOST_CHECK( check_frame( frame, 128, 0, 128 ) );
}

BOOST_AUTO_TEST_CASE( alpha_bga_plus_fg )
{
	// colour: background=0 a=0 colour: background=0 b=255 filter:compositor
	//
	// The output should be yuv420p + l8 alpha. Image should be a bright
	// blue and the alpha should be 255 for every sample.

	input_type_ptr bg = create_delayed_input( L"colour:" );
	bg->property( "background" ) = 0;
	bg->property( "a" ) = 0;
	BOOST_REQUIRE( bg->init( ) );

	input_type_ptr fg = create_delayed_input( L"colour:" );
	fg->property( "background" ) = 0;
	fg->property( "b" ) = 255;
	BOOST_REQUIRE( fg->init( ) );

	filter_type_ptr compositor = create_filter( L"compositor" );
	compositor->connect( bg, 0 );
	compositor->connect( fg, 1 );

	frame_type_ptr frame = compositor->fetch( );
	BOOST_REQUIRE( frame );
	BOOST_REQUIRE( frame->get_image( ) );
	BOOST_REQUIRE( frame->get_alpha( ) );

	BOOST_CHECK( frame->get_image( )->pf( ) == olib::t_string( _CT( "yuv420p" ) ) );
	BOOST_CHECK( frame->get_alpha( )->pf( ) == olib::t_string( _CT( "l8" ) ) );

	BOOST_CHECK( check_frame( frame, 0, 0, 255, 255 ) );
}

BOOST_AUTO_TEST_CASE( alpha_bga_plus_fga )
{
	// colour: colourspace=b8g8r8a8 r=255 a=255 colour: b=255 a=128 filter:compositor
	//
	// The output of the compositor should be b8g8r8a8, the image should be
	// a dull purple and the alpha on the output should be 255 for every sample.
	//
	// NB: The sample level tests aren't trivial to check on the image as they're very
	// much out of our control - for now, only alpha is explicitly checked.

	input_type_ptr bg = create_delayed_input( L"colour:" );
	bg->property( "background" ) = 0;
	bg->property( "colourspace" ) = std::wstring( L"b8g8r8a8" );
	bg->property( "r" ) = 255;
	bg->property( "a" ) = 255;
	BOOST_REQUIRE( bg->init( ) );

	input_type_ptr fg = create_delayed_input( L"colour:" );
	fg->property( "b" ) = 255;
	fg->property( "a" ) = 128;
	BOOST_REQUIRE( fg->init( ) );

	filter_type_ptr compositor = create_filter( L"compositor" );
	compositor->connect( bg, 0 );
	compositor->connect( fg, 1 );

	frame_type_ptr frame = compositor->fetch( );
	BOOST_REQUIRE( frame );
	BOOST_REQUIRE( frame->get_image( ) );
	BOOST_REQUIRE( !frame->get_alpha( ) );
	BOOST_CHECK( frame->pf( ) == olib::t_string( _CT( "b8g8r8a8" ) ) );

	image_type_ptr alpha = image::extract_alpha( frame->get_image( ) );
	BOOST_REQUIRE( alpha );
	BOOST_CHECK( alpha->pf( ) == olib::t_string( _CT( "l8" ) ) );

	BOOST_CHECK( check_plane( alpha, 0, 255 ) );
	BOOST_CHECK( check_frame( frame, 128, 0, 128, 255 ) );
}

BOOST_AUTO_TEST_CASE( alpha_sd_resolution )
{
	// 8 bit alpha like test for deferred compositing. The output of the compositor should be
	// a red image which has not been modified by the compostor. In the deferred case, this means
	// that the frame will have a single attached frame.
	//
	// Note that this test and others in here use the ml::stack to construct the graphs to be 
	// tested - this is purely done for the sake of my sanity. If you feel the urge to change them
	// all I can offer is good luck with that... I for one don't feel like writing and maintaining
	// many lines where a couple will do. 

	input_type_ptr input = stack( )
	.push( L"colour: colourspace=b8g8r8a8 a=0" )
	.push( L"colour: colourspace=b8g8r8a8 a=255 r=255" )
	.push( L"filter:compositor deferred=1" )
	.pop( );

	BOOST_REQUIRE( input );

	frame_type_ptr frame = input->fetch( );
	BOOST_REQUIRE( frame );
	BOOST_REQUIRE( frame->get_image( ) );
	BOOST_REQUIRE( !frame->get_alpha( ) );
	BOOST_REQUIRE( !frame->is_deferred( ) );

	BOOST_CHECK( frame->pf( ) == olib::t_string( _CT( "b8g8r8a8" ) ) );
	BOOST_CHECK( check_frame( frame, 255, 0, 0, 255 ) );

	image_type_ptr alpha = image::extract_alpha( frame->get_image( ) );
	BOOST_REQUIRE( alpha );
	BOOST_CHECK( alpha->pf( ) == olib::t_string( _CT( "l8" ) ) );
	BOOST_CHECK( check_plane( alpha, 0, 255 ) );
}

BOOST_AUTO_TEST_CASE( alpha_prores_resolution )
{
	// 16 bit alpha like test for deferred compositing. The output of the compositor should be
	// a red image which has not been modified by the compostor. In the deferred case, this means
	// that the frame will have a single attached frame.

	input_type_ptr input = stack( )
	.push( L"colour: colourspace=yuva444p16le width=1920 height=1080 interlace=1 sar_num=1 sar_den=1 a=0" )
	.push( L"colour: colourspace=yuva444p16le width=1920 height=1080 interlace=1 sar_num=1 sar_den=1 a=255 r=255" )
	.push( L"filter:compositor deferred=1" )
	.pop( );

	BOOST_REQUIRE( input );

	frame_type_ptr frame = input->fetch( );
	BOOST_REQUIRE( frame );
	BOOST_REQUIRE( frame->get_image( ) );
	BOOST_REQUIRE( !frame->get_alpha( ) );
	BOOST_REQUIRE( !frame->is_deferred( ) );

	BOOST_CHECK( frame->pf( ) == olib::t_string( _CT( "yuva444p16le" ) ) );
	BOOST_CHECK( check_plane( frame->get_image( ), 3, 255 ) );
	BOOST_CHECK( check_frame( frame, 255, 0, 0, 255 ) );

	image_type_ptr alpha = image::extract_alpha( frame->get_image( ) );
	BOOST_REQUIRE( alpha );
	BOOST_CHECK( alpha->pf( ) == olib::t_string( _CT( "l16le" ) ) );
	BOOST_CHECK( check_plane( alpha, 0, 255 ) );
}

BOOST_AUTO_TEST_CASE( offset_test )
{
	// Sets up a timeline with overlapping colour inputs and checks that the
	// expected results are obtained for each frame fetched.

	input_type_ptr input = stack( )
	.push( L"colour:" )
	.push( L"colour: background=0 r=255 filter:clip out=4 filter:offset in=1" )
	.push( L"colour: background=0 b=255 filter:clip out=1 filter:offset in=3" )
	.push( L"colour: background=0 g=255 filter:clip out=1 filter:offset in=2" )
	.push( L"filter:compositor slots=4" )
	.pop( );

	BOOST_REQUIRE( input );
	BOOST_CHECK( input->get_frames( ) == 5 );

	BOOST_CHECK( check_frame( input->fetch( 0 ), 0, 0, 0 ) );
	BOOST_CHECK( check_frame( input->fetch( 1 ), 255, 0, 0 ) );
	BOOST_CHECK( check_frame( input->fetch( 2 ), 0, 255, 0 ) );
	BOOST_CHECK( check_frame( input->fetch( 3 ), 0, 0, 255 ) );
	BOOST_CHECK( check_frame( input->fetch( 4 ), 255, 0, 0 ) );
}

BOOST_AUTO_TEST_CASE( offset_x )
{
	// Sets up a small graph which outputs an image which is black on the left
	// and red on the right.
	//
	// This test also introduces frame_crop and frame_crop_clear tests. Note that
	// because there is no shallow copy functionality associated to the image class
	// the frame_crop introduces a side effect such that all frames which refer to an
	// image are affected. This is undesirable and a fix would allow the following 
	// type of logic to be employed:
	//
	// frame_type_ptr top = frame_crop( frame, 0, 0, 360, 576 );
	// BOOST_CHECK( check_frame( top, 0, 0, 0 ) );
	// 
	// frame_type_ptr bottom = frame_crop( frame, 360, 0, 360, 576 );
	// BOOST_CHECK( check_frame( bottom, 255, 0, 0 ) );
	//
	// For now, the only way this could be achieved is via a deep_copy of the frame.
	// For the sake of efficiency, frame_crop does not enforce this.

	input_type_ptr input = stack( )
	.push( L"colour: colour: r=255 filter:lerp @@x=0.5 filter:compositor" )
	.pop( );

	frame_type_ptr frame = input->fetch( );

	frame = frame_crop( frame, 0, 0, 360, 576 );
	BOOST_CHECK( check_frame( frame, 0, 0, 0 ) );
	frame = frame_crop_clear( frame );

	frame = frame_crop( frame, 360, 0, 360, 576 );
	BOOST_CHECK( check_frame( frame, 255, 0, 0 ) );
}

BOOST_AUTO_TEST_CASE( offset_y )
{
	// Sets up a small graph which outputs an image which is black on the top 
	// and red on the bottom.

	input_type_ptr input = stack( )
	.push( L"colour: colour: r=255 filter:lerp @@y=0.5 filter:compositor" )
	.pop( );

	frame_type_ptr frame = input->fetch( );

	frame = frame_crop( frame, 0, 0, 720, 288 );
	BOOST_CHECK( check_frame( frame, 0, 0, 0 ) );
	frame = frame_crop_clear( frame );

	frame = frame_crop( frame, 0, 288, 720, 288 );
	BOOST_CHECK( check_frame( frame, 255, 0, 0 ) );
}

BOOST_AUTO_TEST_CASE( offset_xy )
{
	// Sets up a small graph which outputs an image which is black and red in the bottom right.

	input_type_ptr input = stack( )
	.push( L"colour: colour: r=255 filter:lerp @@y=0.5 @@x=0.5 filter:compositor" )
	.pop( );

	frame_type_ptr frame = input->fetch( );

	frame_type_ptr left = frame_crop( frame, 0, 0, 720, 288 );
	BOOST_CHECK( check_frame( left, 0, 0, 0 ) );
	frame = frame_crop_clear( frame );

	frame = frame_crop( frame, 0, 288, 360, 288 );
	BOOST_CHECK( check_frame( frame, 0, 0, 0 ) );
	frame = frame_crop_clear( frame );

	frame = frame_crop( frame, 360, 288, 360, 288 );
	BOOST_CHECK( check_frame( frame, 255, 0, 0 ) );
}

BOOST_AUTO_TEST_CASE( offset_x_alpha )
{
	// Sets up a small graph which outputs an image which is black and transparent on the left and 
	// red and non-transparent on the right. 
	//
	// NB: When displaying the non-blended image, the result will look incorrect. Since the alpha
	// specified on the red frame is 128, the displayed image should be a dull red, but instead will
	// be shown as a sharp red. 

	input_type_ptr input = stack( )
	.push( L"colour: a=0 colour: background=0 r=255 a=128 filter:lerp @@x=0.5 filter:compositor" )
	.pop( );

	frame_type_ptr frame = input->fetch( );

	frame = frame_crop( frame, 0, 0, 360, 576 );
	BOOST_CHECK( check_frame( frame, 0, 0, 0, 0 ) );
	frame = frame_crop_clear( frame );

	frame = frame_crop( frame, 360, 0, 360, 576 );
	BOOST_CHECK( check_frame( frame, 255, 0, 0, 128 ) );
	frame = frame_crop_clear( frame );

	// This is one mechanism to blend the image correctly. The resultant samples in the red part are 
	// difficult to predict in terms of rgb (possibly less so in terms of yuv), but the test here is 
	// deliberately fuzzy.

	input_type_ptr blender = stack( ).push( L"colour: background=0 nudger: filter:compositor" ).pop( );
	blender->fetch_slot( 1 )->push( frame );
	frame = blender->fetch( );

	BOOST_REQUIRE( frame->get_image( ) );
	BOOST_REQUIRE( !frame->get_alpha( ) );

	frame = frame_crop( frame, 0, 0, 360, 576 );
	BOOST_CHECK( check_frame( frame, 0, 0, 0 ) );
	frame = frame_crop_clear( frame );

	frame = frame_crop( frame, 360, 0, 360, 576 );
	BOOST_CHECK( check_frame( frame, 120, 0, 0 ) );
}

BOOST_AUTO_TEST_CASE( offset_x_alpha_mix )
{
	// As with the previous test, this sets up a small graph which outputs an image which is black and 
	// transparent on the left and red and non-transparent on the right. The difference is that a mixing
	// level has been introduced which causes the alpha values to be (roughly) halved.
	//
	// Note that the output is composited and not blended - thus the rgb of the foreground is more or less
	// preserved while the alpha channel is mixed down to roughly half. The values which are checked on
	// are not expected to be exactly mirrored in the resultant image and alpha on the frame, but the 
	// variance involved is selected such that the rgb values which can be derived trivially pass.

	input_type_ptr input = stack( )
	.push( L"colour: a=0 colour: background=0 r=255 a=128 filter:lerp @@x=0.5 @@mix=0.5 filter:compositor" )
	.pop( );

	frame_type_ptr frame = input->fetch( );

	frame = frame_crop( frame, 0, 0, 360, 576 );
	BOOST_CHECK( check_frame( frame, 0, 0, 0, 0 ) );
	frame = frame_crop_clear( frame );

	frame = frame_crop( frame, 360, 0, 360, 576 );
	BOOST_CHECK( check_frame( frame, 255, 0, 0, 64 ) );
	frame = frame_crop_clear( frame );
}

BOOST_AUTO_TEST_SUITE_END( )
