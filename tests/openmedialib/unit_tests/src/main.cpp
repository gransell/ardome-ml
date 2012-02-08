#define BOOST_TEST_MODULE openmedialib_tests
#include <boost/test/unit_test.hpp>
#include <fstream>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/utilities.hpp>

std::ofstream log_file_;

class init_fixture {
public:
	init_fixture( )
	{
		using namespace boost::unit_test;
		using namespace olib;
		using namespace olib::opencorelib;

		unit_test_log.set_format( CLF );
		unit_test_log.set_threshold_level( log_test_units );

		//Create log file
        t_path log_dir( _CT("tests/test_output/openmedialib") );
        utilities::make_sure_path_exists( log_dir );
        log_file_.open( ( log_dir / t_path( _CT("unit_test_results.clf") ) ).string().c_str() );
        ARENFORCE( log_file_.good() );
		unit_test_log.set_stream( log_file_ );
	}
};

BOOST_GLOBAL_FIXTURE( init_fixture );
