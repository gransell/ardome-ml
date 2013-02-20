#include <boost/test/unit_test.hpp>
#include <boost/scoped_array.hpp>
#include "io_tester.hpp"

namespace io = olib::openmedialib::ml::io;
using boost::scoped_array;

const int IO_FILE_SIZE = 512;

BOOST_AUTO_TEST_SUITE( custom_io )

BOOST_AUTO_TEST_CASE( custom_protocol_read )
{
	scoped_array< unsigned char > io_file_buffer( new unsigned char[IO_FILE_SIZE] );
	bool cleanup_was_done = false;

	{
		//Fill buffer with test pattern
		for( int i = 0; i < IO_FILE_SIZE; ++i )
		{
			io_file_buffer[i] = ( i % 255 ) + 1;
		}
		io_tester::buffer = io_file_buffer.get();
		io_tester::buffer_size = IO_FILE_SIZE;

		AVIOContext *raw_ctx = NULL;
		BOOST_REQUIRE_EQUAL( io::open_file( &raw_ctx, "io_tester:dummy_filename", AVIO_FLAG_READ ), 0 );
		boost::shared_ptr< AVIOContext > ctx( raw_ctx, &close_and_check_result );

		io_tester *instance = static_cast< io_tester* >( ctx->opaque );
		instance->cleanup_flag = &cleanup_was_done;
		BOOST_REQUIRE_EQUAL( instance->uri, "io_tester:dummy_filename" );

		//Sizing test
		BOOST_REQUIRE_EQUAL( avio_size( ctx.get() ), IO_FILE_SIZE );

		const int read_offset = 54;
		const int read_length = 132;
		BOOST_REQUIRE_EQUAL( avio_seek( ctx.get(), read_offset, SEEK_SET ), read_offset );
		unsigned char read_buf[read_length];
		memset( read_buf, 0, read_length );
		BOOST_REQUIRE_EQUAL( avio_read( ctx.get(), read_buf, read_length ), read_length );
		for( int i = 0; i < read_length; ++i )
		{
			const int actual = read_buf[i];
			const int expected = io_file_buffer[i + read_offset];
			if( actual != expected )
				BOOST_FAIL( "Wrong read result at offset " << i << " (" << actual << " != " << expected << ")" );
		}
		BOOST_REQUIRE_EQUAL( avio_tell( ctx.get() ), read_offset + read_length );
	}
	
	BOOST_CHECK( cleanup_was_done );
}

BOOST_AUTO_TEST_CASE( custom_protocol_write )
{
	scoped_array< unsigned char > io_file_buffer( new unsigned char[IO_FILE_SIZE] );
	bool cleanup_was_done = false;

	{
		memset( io_file_buffer.get(), 0, IO_FILE_SIZE );
		io_tester::buffer = io_file_buffer.get();
		io_tester::buffer_size = IO_FILE_SIZE;

		AVIOContext *raw_ctx = NULL;
		BOOST_REQUIRE_EQUAL( io::open_file( &raw_ctx, "io_tester:dummy_filename", AVIO_FLAG_WRITE ), 0 );
		boost::shared_ptr< AVIOContext > ctx( raw_ctx, &close_and_check_result );

		io_tester *instance = static_cast< io_tester* >( ctx->opaque );
		instance->cleanup_flag = &cleanup_was_done;
		BOOST_REQUIRE_EQUAL( instance->uri, "io_tester:dummy_filename" );

		const int write_offset = 54;
		const int write_length = 132;
		BOOST_REQUIRE_EQUAL( avio_seek( ctx.get(), write_offset, SEEK_SET ), write_offset );
		//Fill temp buffer with test pattern
		unsigned char pattern_buf[write_length];
		for( int i = 0; i < write_length; ++i )
		{
			pattern_buf[i] = ( i % 255 ) + 1;
		}

		avio_write( ctx.get(), pattern_buf, write_length );
		avio_flush( ctx.get() );
		BOOST_REQUIRE_EQUAL( ctx->error, 0 );

		for( int i = 0; i < write_length; ++i )
		{
			const int actual = io_file_buffer[i + write_offset];
			const int expected = pattern_buf[i];
			if( actual != expected )
				BOOST_FAIL( "Wrong write result at offset " << i << " (" << actual << " != " << expected << ")" );
		}
		BOOST_REQUIRE_EQUAL( avio_tell( ctx.get() ), write_offset + write_length );
	}

	BOOST_CHECK( cleanup_was_done );
}

//Make sure that we get to know about any write failure when closing
BOOST_AUTO_TEST_CASE( error_propagation_on_write_failure )
{
	//Two variants of the same test: the first one with flushing before
	//closing, the second without.
	scoped_array< unsigned char > io_file_buffer( new unsigned char[IO_FILE_SIZE] );

	for( int i = 0; i < 2; ++i )
	{
		const bool flush_before_close = ( i == 0 );

		memset( io_file_buffer.get(), 0, IO_FILE_SIZE );
		io_tester::buffer = io_file_buffer.get();
		io_tester::buffer_size = IO_FILE_SIZE;

		AVIOContext *ctx = NULL;
		BOOST_REQUIRE_EQUAL( io::open_file( &ctx, "io_tester:dummy_filename", AVIO_FLAG_WRITE ), 0 );
		io_tester *instance = static_cast< io_tester* >( ctx->opaque );

		const int error_code = AVERROR(EIO);

		instance->write_error_code = error_code;
		unsigned char temp_data = 0;
		avio_write( ctx, &temp_data, 1 );
		if( flush_before_close )
		{
			//If we flush manually, the error should be visible on the
			//context immediately.
			avio_flush( ctx );
			instance->write_error_code = 0;
			BOOST_CHECK_EQUAL( ctx->error, error_code );
		}

		//In either case, we should be informed about the failure when closing.
		BOOST_CHECK_EQUAL( io::close_file( ctx ), error_code );
	}
}

BOOST_AUTO_TEST_CASE( dont_allow_read_write_flag )
{
	//AVIO_FLAG_READ_WRITE doesn't work. Make sure we don't allow it.
	AVIOContext *ctx = NULL;
	BOOST_CHECK_EQUAL( io::open_file( &ctx, "dummy_filename", AVIO_FLAG_READ_WRITE ), AVERROR( EINVAL ) );
}

BOOST_AUTO_TEST_CASE( report_non_seekable_read )
{
	unsigned char temp[1];
	io_tester::buffer = temp;
	io_tester::buffer_size = 1;
	AVIOContext *ctx = NULL;
	BOOST_REQUIRE_EQUAL( io::open_file( &ctx, "io_tester_non_seekable:dummy_filename", AVIO_FLAG_READ ), 0 );
	BOOST_CHECK_EQUAL( ctx->seekable, 0 );
	io::close_file( ctx );
}

BOOST_AUTO_TEST_CASE( report_non_seekable_write )
{
	unsigned char temp[1];
	io_tester::buffer = temp;
	io_tester::buffer_size = 1;
	AVIOContext *ctx = NULL;
	BOOST_REQUIRE_EQUAL( io::open_file( &ctx, "io_tester_non_seekable:dummy_filename", AVIO_FLAG_WRITE ), 0 );
	BOOST_CHECK_EQUAL( ctx->seekable, 0 );
	io::close_file( ctx );
}

BOOST_AUTO_TEST_CASE( report_seekable_read )
{
	unsigned char temp[1];
	io_tester::buffer = temp;
	io_tester::buffer_size = 1;
	AVIOContext *ctx = NULL;
	BOOST_REQUIRE_EQUAL( io::open_file( &ctx, "io_tester:dummy_filename", AVIO_FLAG_READ ), 0 );
	BOOST_CHECK_EQUAL( ctx->seekable, 1 );
	io::close_file( ctx );
}

BOOST_AUTO_TEST_CASE( report_seekable_write )
{
	unsigned char temp[1];
	io_tester::buffer = temp;
	io_tester::buffer_size = 1;
	AVIOContext *ctx = NULL;
	BOOST_REQUIRE_EQUAL( io::open_file( &ctx, "io_tester:dummy_filename", AVIO_FLAG_WRITE ), 0 );
	BOOST_CHECK_EQUAL( ctx->seekable, 1 );
	io::close_file( ctx );
}

BOOST_AUTO_TEST_SUITE_END()

