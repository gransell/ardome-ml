#include "precompiled_headers.hpp"

#include <boost/test/execution_monitor.hpp>
#include <boost/test/unit_test.hpp>

#include "opencorelib/cl/core.hpp"
#include "opencorelib/cl/logtarget.hpp"
#include "opencorelib/cl/str_util.hpp"

// Forward declarations:
void test_assert();
void test_multivalue_property_parsing();
void test_invoker();
void test_messagequeue();
void test_serious_error_handler();
void test_span();
void test_timecode();
void test_plugin();
void test_utilities();
void test_uuid();
void test_color();
void test_int64_conversions();
void test_string_conversions();
void test_time_helpers();
void test_event_handler();
void test_worker();
void test_base64_conversions();
void test_jira_amf_1809();
 

std::ofstream log_file;

boost::unit_test_framework::test_suite* init_unit_test_suite ( int argc, char* argv[] ){
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
    unit_test_log.set_threshold_level( log_successful_tests );
    unit_test_log.set_stream( log_file );
    
	typedef std::map< std::string, boost::function< void () > > TestMap;
	TestMap myTests;
	myTests["test_assert"] = &test_assert;
    myTests["test_multivalue_property_parsing"] = &test_multivalue_property_parsing;
    myTests["test_invoker"] = &test_invoker;
	myTests["test_messagequeue"] = &test_messagequeue;
	//myTests["test_serious_error_handler"] = &test_serious_error_handler;
	myTests["test_span"] = &test_span;
	myTests["test_timecode"] = &test_timecode;
	myTests["test_plugin"] = &test_plugin;
    myTests["test_utilities"] = &test_utilities;
    myTests["test_uuid"] = &test_uuid;
	myTests["test_int64_conversions"] = &test_int64_conversions; 
	myTests["test_string_conversions"] = &test_string_conversions; 
    myTests["test_color"] = &test_color; 
    // myTests["test_time_helpers"] = &test_time_helpers;
    myTests["test_event_handler"] = &test_event_handler;
    myTests["test_worker"] = &test_worker;
    myTests["test_jira_amf_1809"] = &test_jira_amf_1809;
	//myTests["test_base64_conversions"] = &test_base64_conversions;

	boost::unit_test_framework::test_suite* ts1 = BOOST_TEST_SUITE("All tests");
    
    // If first argument is all_tests then add everything
    if( argc > 1 && std::string(argv[1]) == "all_tests" ) {
        for( TestMap::iterator it = myTests.begin();
             it != myTests.end();
             ++it )
        {
            // The BOOST_TEST_CASE macro is usefull only with an actual function name.
            ts1->add(boost::unit_test::make_test_case(it->second, it->first));
        }
    }
    else {
        for(int i = 1; i < argc; ++i) {
            std::string test(argv[i]);
            TestMap::iterator fit = myTests.find(test);
            if(fit != myTests.end()){
                ts1->add(boost::unit_test::make_test_case(fit->second, fit->first));
            } else {
                ARLOG_INFO("Unknown test: %s")(test);
                exit(1);        // Not in a test, so can't fail
            }
        }
    }
	return ts1;
}
