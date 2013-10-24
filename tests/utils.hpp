#ifndef TEST_UTILS_HPP_INCLUDED_
#define TEST_UTILS_HPP_INCLUDED_

namespace olib {

//test_suite_name: The name of the module that is tested, e.g. "opencorelib".
//                 This name will be used to name the output directory where the
//                 test result file shows up. In the above example, the output
//                 directory would be tests/test_output/opencorelib.
//Returns the path where the test log file should be stored. The caller should
//use this path when setting the log output stream for the test.
t_string configure_test_runner( const olib::t_string &test_suite_name )
{
	using namespace boost::unit_test;
	using namespace olib;
	using namespace olib::opencorelib;

	bool use_xml = false;
	if( framework::master_test_suite().argc >= 2 )
	{
		const std::string arg = framework::master_test_suite().argv[1];
		bool args_ok = false;
		if( framework::master_test_suite().argc == 2 )
		{
			if( arg == "--xml" )
			{
				use_xml = true;
				args_ok = true;
			}
		}
		if( !args_ok )
		{
			std::cerr << "Usage: openmedialib_unit_tests [--xml]" << std::endl;
			throw framework::setup_error("Unknown command-line argument");
		}
	}

	unit_test_log.set_threshold_level( log_test_units );
	if( use_xml )
	{
		std::cout << "Setting log format to XML" << std::endl;
		unit_test_log.set_format( XML );
	}
	else
	{
		std::cout << "Setting log format to CLF" << std::endl;
		unit_test_log.set_format( CLF );
	}

	t_string log_dir = _CT("tests/test_output/");
	log_dir += test_suite_name;
	utilities::make_sure_path_exists( log_dir );

	t_string log_file_name = 
		test_suite_name +
		_CT("_test_results") +
		( use_xml ? _CT(".xml") : _CT(".clf") );

	const t_string log_path = log_dir + _CT("/") + log_file_name;

	T_COUT << "Test log path: " << log_path << std::endl;
	return log_path;
}

}
#endif
