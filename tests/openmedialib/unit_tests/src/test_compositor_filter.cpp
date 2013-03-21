#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/filter.hpp>
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

BOOST_AUTO_TEST_SUITE_END()
