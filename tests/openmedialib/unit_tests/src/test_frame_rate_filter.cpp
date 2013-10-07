#include <boost/test/unit_test.hpp>
#include <boost/assign.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/frame.hpp>
#include <opencorelib/cl/str_util.hpp>
#include <openmedialib/ml/filter.hpp>
#include <openmedialib/ml/store.hpp>

using namespace olib::openmedialib::ml;
using namespace olib::openimagelib::il;
using namespace olib::opencorelib::str_util;


BOOST_AUTO_TEST_SUITE( test_frame_rate_filter )

// using the exact logic as in the filter for these conversions from srource to destination and vice versa, otherwise there are mismatch for ceil/floor etc.
double map_source_to_dest( int position, int fps_num_in, int fps_den_in, int fps_num_out, int fps_den_out ) 
{
	double t1 = boost::int64_t( fps_num_in ) * fps_den_out;
	double t2 = boost::int64_t( fps_den_in  ) * fps_num_out;
	return position * ( t2 / t1 );
}

double map_dest_to_source( int position, int fps_num_in, int fps_den_in, int fps_num_out, int fps_den_out ) 
{
	double t1 = boost::int64_t( fps_num_in ) * fps_den_out;
	double t2 = boost::int64_t( fps_den_in  ) * fps_num_out;
	return position * ( t1 / t2 );
}

input_type_ptr create_input_with_params( int fps_num, int fps_den, int frame_count = 100 )
{
	input_type_ptr colour = create_input( L"colour:" );
	BOOST_REQUIRE( colour );
	colour->property( "r" ) = 180;
	colour->property( "fps_num" ) = fps_num;
	colour->property( "fps_den" ) = fps_den;
	colour->property( "out" ) = frame_count;

	input_type_ptr tone = create_input( L"tone:" );
	BOOST_REQUIRE( tone );
	tone->property( "oscillate" ) = 16;
	tone->property( "fps_num" ) = fps_num;
	tone->property( "fps_den" ) = fps_den;
	tone->property( "out" ) = frame_count;

	filter_type_ptr muxer = create_filter( L"muxer" );
	BOOST_REQUIRE( muxer );
	muxer->connect( colour, 0 );
	muxer->connect( tone, 1 );
	muxer->sync();

	return muxer;
}

void test_frame(int frame_pos_out, input_type_ptr in, input_type_ptr out)
{
	// get the input fps
	frame_type_ptr first_frame_in = in->fetch( 0 );
	int fps_num_in = 0, fps_den_in = 0;
	first_frame_in->get_fps( fps_num_in, fps_den_in );

	// get the output fps
	frame_type_ptr first_frame_out = out->fetch( 0 );
	int fps_num_out = 0, fps_den_out = 0;
	first_frame_out->get_fps( fps_num_out, fps_den_out );

	// derive the corresponding frame position in the input
	int frame_pos_in = int( map_dest_to_source( frame_pos_out, fps_num_in, fps_den_in, fps_num_out, fps_den_out ) );

	// make sure the correct frame is returned with the fetch() method of the frame_rate filter by comparing the image in the frames.
	frame_type_ptr frame_out = out->fetch( frame_pos_out );
	frame_type_ptr frame_in = in->fetch( frame_pos_in );
	BOOST_CHECK_EQUAL( frame_out->get_image(), frame_in->get_image() );	
}

void test_frame_rate_change( int fps_num_in, int fps_den_in, int fps_num_out, int fps_den_out )
{
	int gcd_num_den = gcd( fps_num_out, fps_den_out );
	fps_num_out /= gcd_num_den;
	fps_den_out /= gcd_num_den;
	gcd_num_den = gcd( fps_num_in, fps_den_in );
	fps_num_in /= gcd_num_den;
	fps_den_in /= gcd_num_den;

	input_type_ptr in = create_input_with_params( fps_num_in, fps_den_in );
	BOOST_REQUIRE( in );

	// get some parameters before connecting to the sample_rate filter
	int frame_count_in = in->get_frames( );
	frame_type_ptr first_frame_in = in->fetch( 0 );
	int fps_num_in_actual = 0, fps_den_in_actual = 0;
	first_frame_in->get_fps( fps_num_in_actual, fps_den_in_actual );
	BOOST_CHECK_EQUAL( fps_num_in_actual, fps_num_in );
	BOOST_CHECK_EQUAL( fps_den_in_actual, fps_den_in );

	// connect to the filter
	filter_type_ptr frame_rate_filter = create_filter( L"frame_rate" );
	BOOST_REQUIRE( frame_rate_filter );
	frame_rate_filter->property( "fps_num" ) = fps_num_out;
	frame_rate_filter->property( "fps_den" ) = fps_den_out;
	BOOST_REQUIRE( frame_rate_filter->connect( in ) );
	input_type_ptr out = frame_rate_filter;

	// get some parameters after connecting to the sample_rate filter
	int frame_count_out = out->get_frames( );
	frame_type_ptr first_frame_out = out->fetch( 0 );
	int fps_num_out_actual = 0, fps_den_out_actual = 0;
	first_frame_out->get_fps( fps_num_out_actual, fps_den_out_actual );
	BOOST_CHECK_EQUAL( fps_num_out_actual, fps_num_out );
	BOOST_CHECK_EQUAL( fps_den_out_actual, fps_den_out );

	// make sure the total frame count is as it should be	
	int frame_count_out_should_be = int( ceil( map_source_to_dest( frame_count_in, fps_num_in, fps_den_in, fps_num_out, fps_den_out ) ) );
	BOOST_CHECK_EQUAL( frame_count_out, frame_count_out_should_be );

	// check the first, last and middle frames
	test_frame( 0, in, out );
	test_frame( frame_count_out - 1, in, out );
	test_frame( frame_count_out / 2, in, out );

	// check that audio samples after conversion are as they should be
	int channels = first_frame_in->get_audio()->channels();
	int single_channel_sample_size = first_frame_in->get_audio()->sample_storage_size();
	int bytes_per_sample = channels * single_channel_sample_size;

	// for each input frame, copy the same number of samples from one or more output frames into a temporary buffer, and then compare.
	int next_out_frame = 0, next_out_sample_offset = 0;	
	int temp_buffer_size = ( in->fetch( 0 )->get_audio()->samples() + 5 ) * bytes_per_sample;	// 5: number of samples in the input can vary (1601, 1602 ...)
	boost::scoped_array< char > temp_array( new char[temp_buffer_size] );
	char *temp = temp_array.get();
	for ( int i = 0; i < frame_count_in; i++ )
	{
		audio_type_ptr audio_in = in->fetch( i )->get_audio();
		int samples_in = audio_in->samples();
		void *data_in = audio_in->pointer();

		// copy equal number of samples from output corresponding to this input frame to a temporary buffer
		int temp_offset = 0;
		int remaining = samples_in;
		while ( remaining )
		{
			audio_type_ptr audio_out = out->fetch( next_out_frame )->get_audio();
			int samples_out = audio_out->samples() - next_out_sample_offset;
			void *data_out = ( ( char * ) audio_out->pointer() ) + next_out_sample_offset * bytes_per_sample;
			int to_copy = 0;

			if ( remaining > samples_out )
			{
				// we need to copy everything in this frame, and proceed to the next
				to_copy = samples_out;
				next_out_frame++;
				next_out_sample_offset = 0;
			}
			else 
			{
				// we need to copy the amount needed to fill the buffer, then come back to this output frame again for the next input frame
				to_copy = remaining;
				next_out_sample_offset += to_copy;			
			}

			if ( ( temp_offset + to_copy * bytes_per_sample ) > temp_buffer_size )
				BOOST_FAIL( "Buffer overrun for input frame " << i );

			memcpy( temp + temp_offset, data_out, to_copy * bytes_per_sample );
			remaining -= to_copy;
			temp_offset += to_copy * bytes_per_sample;
		}

		// compare the buffer with the input sample data
		int comparison_result = memcmp( temp, data_in, samples_in * bytes_per_sample );
		if ( comparison_result != 0 )
			BOOST_ERROR( "Audio samples mismatch for input frame " << i );
	}
}

BOOST_AUTO_TEST_CASE( frame_rate_filter )
{
	std::vector< std::pair< int, int > > frame_rates;
	frame_rates.push_back( std::pair< int, int >( 24, 1 ) );
	frame_rates.push_back( std::pair< int, int >( 25, 1 ) );
	frame_rates.push_back( std::pair< int, int >( 50, 1 ) );
	frame_rates.push_back( std::pair< int, int >( 30000, 1001 ) );
	frame_rates.push_back( std::pair< int, int >( 60000, 1001 ) );

	for ( int i = 0; i < frame_rates.size(); i++ )
	{
		for ( int j = 0; j < frame_rates.size(); j++ )
		{
			test_frame_rate_change( frame_rates[i].first, frame_rates[i].second, frame_rates[j].first, frame_rates[j].second );
		}		
	}
}

BOOST_AUTO_TEST_SUITE_END( )
