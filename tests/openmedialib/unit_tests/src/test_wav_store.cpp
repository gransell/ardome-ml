#include <boost/test/unit_test.hpp>
#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/uuid_16b.hpp>
#include <opencorelib/cl/special_folders.hpp>
#include <opencorelib/cl/guard_define.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/store.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/audio.hpp>
#include <boost/scoped_array.hpp>
#include "io_tester.hpp"

namespace cl = olib::opencorelib;
namespace ml = olib::openmedialib::ml;

namespace
{
	const int WAV_HEADER_SIZE = 80;

	ml::audio_type_ptr create_test_pattern_audio()
	{
		ml::audio_type_ptr aud = ml::audio::allocate( ml::audio::FORMAT_PCM16, 48000, 1, 1920, false );
		unsigned char *audio_data = static_cast< unsigned char * >( aud->pointer() );
		//1,2,3...254,254,1,2,3...
		for( int i = 0; i < aud->samples() * 2; ++i )
		{
			audio_data[i] = ( i % 255 ) + 1;
		}

		return aud;
	}
	
	void perform_basic_write_test( bool seekable_output )
	{
		const int WAV_BUFFER_SIZE = 65536;

		boost::scoped_array< unsigned char > wav_buffer( new unsigned char[WAV_BUFFER_SIZE] );
		memset( wav_buffer.get(), 0, WAV_BUFFER_SIZE );

		io_tester::buffer = wav_buffer.get();
		io_tester::buffer_size = WAV_BUFFER_SIZE;

		ml::audio_type_ptr aud = create_test_pattern_audio();

		ml::frame_type_ptr initial_frame( new ml::frame_type( ) );
		initial_frame->set_audio( aud );
		ml::store_type_ptr wav_store = seekable_output ?
			ml::create_store( "wav:io_tester:dummy_filename", initial_frame ) :
			ml::create_store( "wav:io_tester_non_seekable:dummy_filename", initial_frame );

		const int num_frames = 10;
		const int WAV_HEADER_SIZE = 80;
		const int total_sample_byte_size = aud->samples() * 2 * num_frames;
		BOOST_REQUIRE( WAV_HEADER_SIZE + total_sample_byte_size <= WAV_BUFFER_SIZE );

		for( int i = 0; i < num_frames; ++i )
		{
			ml::frame_type_ptr fr( new ml::frame_type( ) );
			fr->set_position( i );
			fr->set_audio( aud );
			BOOST_REQUIRE( wav_store->push( fr ) );
		}

		wav_store->complete();

		const unsigned char *result_buf_ptr = &wav_buffer[WAV_HEADER_SIZE];
		const unsigned char *audio_data = static_cast< unsigned char * >( aud->pointer() );
		for( int f = 0; f < num_frames; ++f )
		{
			for( int i = 0; i < aud->samples() * 2; ++i, ++result_buf_ptr )
			{
				int expected = audio_data[i];
				int actual = *result_buf_ptr;
				if( actual != expected )
					BOOST_FAIL( "Unexpected WAV data found at offset " << i << " (" << actual << " != " << expected << ")" );
			}
		}

		//Make sure that the header is RIFF and that its size is correct
		BOOST_CHECK( memcmp( &wav_buffer[0], "RIFF", 4 ) == 0 );
		const uint32_t riff_block_size = *reinterpret_cast< uint32_t * >( &wav_buffer[4] );
		if( seekable_output )
		{
			BOOST_CHECK_EQUAL( riff_block_size , WAV_HEADER_SIZE + total_sample_byte_size - 8 );
		}
		else
		{
			BOOST_CHECK_EQUAL( riff_block_size , WAV_HEADER_SIZE - 8 );
		}

		//Make sure that there is no ds64 block. Its preallocated space should be a JUNK block.
		BOOST_CHECK( memcmp( &wav_buffer[0x24], "JUNK", 4 ) == 0 );

		//Make sure there is a data block and that its size is set correctly
		BOOST_CHECK( memcmp( &wav_buffer[WAV_HEADER_SIZE - 8], "data", 4 ) == 0 );
		const uint32_t wav_data_block_size = *reinterpret_cast< uint32_t * >( &wav_buffer[WAV_HEADER_SIZE - 4] );
		if( seekable_output )
		{
			//Check that the correct length was written in the header
			BOOST_CHECK_EQUAL( wav_data_block_size, total_sample_byte_size );
		}
		else
		{
			//The length will be left as 0
			BOOST_CHECK_EQUAL( wav_data_block_size, 0 );
		}
	}
}

BOOST_AUTO_TEST_SUITE( wav_store )

BOOST_AUTO_TEST_CASE( basic_write )
{
	perform_basic_write_test( true );
}

BOOST_AUTO_TEST_CASE( non_seekable_output )
{
	perform_basic_write_test( false );
}

BOOST_AUTO_TEST_CASE( size_larger_than_4gb )
{
	const int WAV_BUFFER_SIZE = WAV_HEADER_SIZE;

	boost::scoped_array< unsigned char > wav_buffer( new unsigned char[WAV_BUFFER_SIZE] );
	memset( wav_buffer.get(), 0, WAV_BUFFER_SIZE );

	io_tester::buffer = wav_buffer.get();
	io_tester::buffer_size = WAV_BUFFER_SIZE;

	//Make the I/O layer pretend that the writes succeed, since we can't allocate a 4 GB+ memory buffer
	io_tester::allow_write_beyond_buffer_size = true;

	ml::audio_type_ptr aud = create_test_pattern_audio();

	ml::frame_type_ptr initial_frame( new ml::frame_type( ) );
	initial_frame->set_audio( aud );
	ml::store_type_ptr wav_store = ml::create_store( "wav:io_tester:dummy_filename", initial_frame );

	//Enough frames to make the size larger than 4 GB
	const int64_t num_frames = 1200000;
	
	const int64_t total_sample_byte_size = aud->samples() * 2 * num_frames;

	for( int i = 0; i < num_frames; ++i )
	{
		ml::frame_type_ptr fr( new ml::frame_type( ) );
		fr->set_position( i );
		fr->set_audio( aud );
		if( !wav_store->push( fr ) )
		{
			BOOST_FAIL( "Push to wav store failed on frame " << i );
		}
	}

	wav_store->complete();

	//Make sure that the header is RF64 and that its size is set to 0xFFFFFFFF, since the real size is in the ds64 block
	BOOST_CHECK( memcmp( &wav_buffer[0], "RF64", 4 ) == 0 );
	const uint32_t rf64_block_size = *reinterpret_cast< uint32_t * >( &wav_buffer[0x04] );
	BOOST_CHECK_EQUAL( rf64_block_size , 0xFFFFFFFF );
	
	//Make sure that there is a ds64 block and that its size is correct
	BOOST_CHECK( memcmp( &wav_buffer[0x24], "ds64", 4 ) == 0 );
	const uint64_t ds64_data_size = *reinterpret_cast< uint64_t * >( &wav_buffer[0x34] );
	BOOST_CHECK_EQUAL( ds64_data_size, total_sample_byte_size );

	//Make sure that the data block size is set to 0xFFFFFFFF, since the real size is in the ds64 block
	BOOST_CHECK( memcmp( &wav_buffer[WAV_HEADER_SIZE - 8], "data", 4 ) == 0 );
	const uint32_t wav_data_block_size = *reinterpret_cast< uint32_t * >( &wav_buffer[WAV_HEADER_SIZE - 4] );
	BOOST_CHECK_EQUAL( wav_data_block_size, 0xFFFFFFFF );
}

BOOST_AUTO_TEST_SUITE_END()
