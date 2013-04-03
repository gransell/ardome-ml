#include "precompiled_headers.hpp"

#define BOOST_TEST_MODULE opencorelib_tests
#include <boost/test/unit_test.hpp>

#include "opencorelib/cl/logtarget.hpp"

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
		
		olib::t_path log_dir( _CT("tests/test_output/opencore") );
		olib::opencorelib::utilities::make_sure_path_exists( log_dir );

		log_file.open( ( log_dir / olib::t_path( _CT("opencore_test_results.xml") ) ).string().c_str() );
		ARENFORCE( log_file.good() );

		unit_test_log.set_format( XML );
		unit_test_log.set_threshold_level( log_test_units );
		unit_test_log.set_stream( log_file );
	}
};

BOOST_GLOBAL_FIXTURE( test_fixture );

