#include <boost/test/unit_test.hpp>
#include <opencorelib/cl/core.hpp>
#include <opencorelib/cl/assert_defines.hpp>
#include "io_tester.hpp"

namespace io = olib::openmedialib::ml::io;

unsigned char *io_tester::buffer = NULL;
size_t io_tester::buffer_size = 0;
bool io_tester::allow_write_beyond_buffer_size = false;

io_tester::io_tester()
	: current_position( 0 )
	, write_error_code( 0 )
	, cleanup_flag( NULL )
{
	BOOST_REQUIRE( io_tester::buffer != NULL );
	BOOST_REQUIRE( io_tester::buffer_size > 0 );
}

io_tester::~io_tester()
{
	if( cleanup_flag != NULL )
		*cleanup_flag = true;

	io_tester::buffer = NULL;
	io_tester::buffer_size = 0;
	io_tester::allow_write_beyond_buffer_size = false;
}

void close_and_check_result( AVIOContext *ctx )
{
	BOOST_REQUIRE_EQUAL( io::close_file( ctx ), 0 );
}

int io_tester_open( void **out_private_data, const char *uri, int flags )
{
	BOOST_REQUIRE( out_private_data != NULL );
	BOOST_REQUIRE( uri != NULL );

	io_tester *instance = new io_tester();
	instance->uri = uri;

	*out_private_data = instance;

	return 0;
}

int io_tester_close( void *private_data )
{
	BOOST_REQUIRE( private_data != NULL );
	io_tester *instance = static_cast< io_tester* >( private_data );
	delete instance;

	return 0;
}

int io_tester_read( void *private_data, unsigned char *buf, int size )
{
	//Cannot use BOOST_REQUIRE here, since we're being called by C code.
	//Also, those functions write to the unit test log on every invocation,
	//which slows things down.
	ARASSERT( buf != NULL );
	ARASSERT( size > 0 );
	ARASSERT( private_data != NULL );
	
	io_tester *instance = static_cast< io_tester* >( private_data );

	int bytes_to_read = size;
	if( instance->current_position > io_tester::buffer_size )
	{
		bytes_to_read = 0;
	}
	else if( instance->current_position + size > io_tester::buffer_size )
	{
		bytes_to_read = static_cast< int >( io_tester::buffer_size - instance->current_position );
	}

	memcpy( buf, io_tester::buffer + instance->current_position, bytes_to_read );
	instance->current_position += bytes_to_read;

	return bytes_to_read;
}

int io_tester_write( void *private_data, const unsigned char *buf, int size )
{
	//Cannot use BOOST_REQUIRE here, since we're being called by C code.
	//Also, those functions write to the unit test log on every invocation,
	//which slows things down.
	ARASSERT( buf != NULL );
	ARASSERT( size > 0 );
	ARASSERT( private_data != NULL );

	io_tester *instance = static_cast< io_tester* >( private_data );

	int bytes_to_write = size;
	if( instance->current_position > io_tester::buffer_size )
	{
		bytes_to_write = 0;
	}
	else if( instance->current_position + size > io_tester::buffer_size )
	{
		bytes_to_write = static_cast< int >( io_tester::buffer_size - instance->current_position );
	}

	memcpy( io_tester::buffer + instance->current_position, buf, bytes_to_write );
	instance->current_position += bytes_to_write;

	ARASSERT( instance->write_error_code <= 0 );
	if( instance->write_error_code < 0 )
		return instance->write_error_code;
	else
	{
		if( io_tester::allow_write_beyond_buffer_size )
			return size;
		else
			return bytes_to_write;
	}
}

boost::int64_t io_tester_seek( void *private_data, boost::int64_t offset, int whence )
{
	io_tester *instance = static_cast< io_tester* >( private_data );

	boost::int64_t result_offset = -1;
	if( whence == SEEK_SET )
		result_offset = offset;
	else if( whence == SEEK_CUR )
		result_offset = instance->current_position + offset;
	else if( whence == SEEK_END )
		result_offset = io_tester::buffer_size + offset;
	else if( whence == AVSEEK_SIZE )
		result_offset = io_tester::buffer_size;
	else
		ARASSERT_MSG( false, "Unknown value for whence parameter in seek call." );

	if( result_offset < 0 )
		result_offset = 0;
	else if( result_offset > io_tester::buffer_size )
		result_offset = io_tester::buffer_size;

	if( whence != AVSEEK_SIZE )
		instance->current_position = result_offset;

	return result_offset;
}

void register_io_tester_protocol()
{
	io::protocol_funcs io_tester_prot = 
	{
		io_tester_open,
		io_tester_close,
		io_tester_read,
		io_tester_write,
		io_tester_seek
	};

	io::register_protocol_handler( "io_tester", io_tester_prot );

	io_tester_prot.seek_func = NULL;

	io::register_protocol_handler( "io_tester_non_seekable", io_tester_prot );
}
