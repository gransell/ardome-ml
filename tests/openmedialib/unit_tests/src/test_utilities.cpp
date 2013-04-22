#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <opencorelib/cl/utilities.hpp>

using boost::uint8_t;
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


BOOST_AUTO_TEST_SUITE( test_audio_utilities )

BOOST_AUTO_TEST_CASE( test_pack_pcm24_from_pcm32 )
{
	const int samples = 2, channels = 2, input_sample_size_in_bytes = 4, output_sample_size_in_bytes = 3;
	uint8_t test_src[samples * channels * input_sample_size_in_bytes];
	uint8_t test_dest[samples * channels * output_sample_size_in_bytes];

	initialize_array( test_src, sizeof( test_src ) );
	au::pack_pcm24_from_pcm32( test_dest, test_src, samples, channels );
	
	uint8_t *src = test_src, *dest = test_dest;
	for( int i = 0; i < samples * channels; ++i, src += input_sample_size_in_bytes, dest += output_sample_size_in_bytes )
	{
		BOOST_CHECK_EQUAL( src[0], dest[0] );
		BOOST_CHECK_EQUAL( src[1], dest[1] );
		BOOST_CHECK_EQUAL( src[2], dest[2] );
	}
}

BOOST_AUTO_TEST_CASE( test_pack_pcm24_from_aiff32 )
{	
	const int samples = 2, channels = 2, input_sample_size_in_bytes = 4, output_sample_size_in_bytes = 3;
	uint8_t test_src[samples * channels * input_sample_size_in_bytes];
	uint8_t test_dest[samples * channels * output_sample_size_in_bytes];

	initialize_array( test_src, sizeof( test_src ) );
	au::pack_pcm24_from_aiff32( test_dest, test_src, samples, channels );
	
	uint8_t *src = test_src, *dest = test_dest;
	for( int i = 0; i < samples * channels; ++i, src += input_sample_size_in_bytes, dest += output_sample_size_in_bytes )
	{
		BOOST_CHECK_EQUAL( src[0], dest[2] );
		BOOST_CHECK_EQUAL( src[1], dest[1] );
		BOOST_CHECK_EQUAL( src[2], dest[0] );
	}
}

BOOST_AUTO_TEST_CASE( test_byteswap16_inplace )
{	
	// byteswap16_inplace uses SSE instructions that need input data to be 16-byte aligned. so use aligned_alloc to allocate test data.
	boost::shared_ptr< uint8_t[] > test_buf( static_cast< uint8_t * >( olib::opencorelib::utilities::aligned_alloc( 16, 32 ) ), &olib::opencorelib::utilities::aligned_free );
	initialize_array( test_buf.get(), 32 );
	uint8_t test_orig_values[32];
	memcpy( test_orig_values, test_buf.get(), 32 );

	au::byteswap16_inplace( test_buf.get(), 32 );

	for( int i = 0; i < 32; i += 2 )
	{
		BOOST_CHECK_EQUAL( test_buf[i], test_orig_values[i + 1] );
		BOOST_CHECK_EQUAL( test_buf[i + 1], test_orig_values[i] );
	}
}

BOOST_AUTO_TEST_SUITE_END( )

