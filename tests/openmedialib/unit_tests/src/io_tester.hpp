#include <openmedialib/ml/io.hpp>

struct io_tester
{
	io_tester( );
	~io_tester();

	std::string uri;
	boost::int64_t current_position;
	int write_error_code;
	bool *cleanup_flag;

	static unsigned char *buffer;
	static size_t buffer_size;
	static bool allow_write_beyond_buffer_size;
};

void close_and_check_result( AVIOContext *ctx );

void register_io_tester_protocol();
