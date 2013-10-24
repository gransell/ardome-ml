#include "precompiled_headers.hpp"

#define BOOST_TEST_MODULE opencorelib_tests
#include <boost/test/unit_test.hpp>

#include "opencorelib/cl/logtarget.hpp"

#include "../../utils.hpp"

std::ofstream log_file;

class test_fixture
{
public:
	test_fixture()
	{
		using namespace olib::opencorelib;
		using namespace boost::unit_test;

		// NOTE: The unit testing is verbose by design.  It is stored to a temporary
		// and only dumped to the build log / terminal if there is an error!
		the_log_handler::instance().set_global_log_level(olib::opencorelib::log_level::debug9);

		const olib::t_string log_path = olib::configure_test_runner( _CT("opencorelib") );
		log_file.open( log_path.c_str() );
		ARENFORCE( log_file.good() );
		unit_test_log.set_stream( log_file );
	}
};

BOOST_GLOBAL_FIXTURE( test_fixture );

