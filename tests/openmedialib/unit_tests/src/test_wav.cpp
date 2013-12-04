#include <boost/test/unit_test.hpp>

#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/guard_define.hpp>
#include <opencorelib/cl/special_folders.hpp>
#include <opencorelib/cl/uuid_16b.hpp>
#include <opencorelib/cl/str_util.hpp>

#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/store.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/audio.hpp>

#include <boost/filesystem.hpp>

using namespace olib::opencorelib::str_util;
namespace cl = olib::opencorelib;
namespace ml = olib::openmedialib::ml;
namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE( test_wav )

namespace
{
	ml::audio_type_ptr create_test_pattern_pcm24( )
	{
		ml::audio_type_ptr aud = ml::audio::allocate( ml::audio::pcm24_id, 48000, 1, 1920, false );
		unsigned char *audio_data = static_cast< unsigned char * >( aud->pointer() );
		int c = 0;
		for( int i = 0; i < aud->samples( ); i+=4 )
		{
			audio_data[i] = 0;
			audio_data[i+1] = ( c++ % 255 ) + 1;
			audio_data[i+2] = ( c++ % 255 ) + 1;
			audio_data[i+3] = ( c++ % 255 ) + 1;
		}
		return aud;
	}

	void cleanup( const fs::path &p )
	{
		boost::system::error_code ec;
		const bool path_existed = fs::remove( p, ec );
		if( path_existed )
		{
			//Fail the test on error, but don't throw since we're getting called from a destructor
			BOOST_CHECK_MESSAGE( !ec, "Failed to remove temp file " << to_string( p.native() ) << ": " << ec );
		}
	}
}

/*
 * Store generated samples with wav store and open the same file in wav input and compare the
 * samples with the generated samples.
 * */
BOOST_AUTO_TEST_CASE( test_pcm24_store_and_input )
{
	ml::audio_type_ptr store_audio = create_test_pattern_pcm24( );
	ml::frame_type_ptr initial_frame( new ml::frame_type( ) );
	initial_frame->set_audio( store_audio );

	olib::t_string filename = _CT("pcm24.wav");
	cl::uuid_16b unique_id;
	const fs::path unique_path =
        cl::special_folder::get( cl::special_folder::temp ) /
        ( unique_id.to_hex_string() + filename );

	ARGUARD( boost::bind( &cleanup, unique_path ) );
	ml::store_type_ptr wav_store = ml::create_store( L"wav:" + to_wstring( unique_path.native( ) ), initial_frame );

	int num_frames = 3;
	for( int i = 0; i < num_frames; ++i )
	{
		ml::frame_type_ptr fr( new ml::frame_type( ) );
		fr->set_position( i );
		fr->set_audio( store_audio );
		BOOST_REQUIRE( wav_store->push( fr ) );
	}

	wav_store->complete();

	ml::input_type_ptr wav_input = ml::create_input( L"wav:" + to_wstring( unique_path.native( ) ) );

    BOOST_REQUIRE (wav_input != NULL);

	for (int i = 0; i < num_frames; i++ ) {
		wav_input->seek( i );
		ml::frame_type_ptr frame = wav_input->fetch( );
		BOOST_REQUIRE (frame != NULL);

		BOOST_REQUIRE_EQUAL( i, frame->get_position( ) );

		BOOST_CHECK_EQUAL (frame->has_audio(), true);
		ml::audio_type_ptr input_audio = frame->get_audio( );

		BOOST_REQUIRE (input_audio != NULL);

		BOOST_REQUIRE_EQUAL( input_audio->size( ), store_audio->size( ) );
		BOOST_REQUIRE_EQUAL( input_audio->samples( ), store_audio->samples( ) );
		BOOST_REQUIRE_EQUAL( input_audio->id( ), ml::audio::pcm24_id );	

		uint8_t *store_audio_data = static_cast< uint8_t* >( store_audio->pointer() );
		uint8_t *input_audio_data = static_cast< uint8_t* >( input_audio->pointer() );

		for( int j = 0; j < input_audio->samples( ); j++ )
		{
			if(store_audio_data[j] != input_audio_data[j])  {
				BOOST_FAIL( "Store audio data (" << (int)store_audio_data[j] << 
							") mismatch with input (" << (int)input_audio_data[j] << 
							") at offset " << i );
			}
		}
	}
}

BOOST_AUTO_TEST_SUITE_END( )

