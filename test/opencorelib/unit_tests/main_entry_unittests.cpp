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
void test_logprint();
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
 
namespace olib { 
    namespace opencorelib {
        std::ostream& operator << (std::ostream& out, const logger& l) {
			t_stringstream ss;
            const_cast<logger&>(l).pretty_print_one_line(ss, print::output_default);
			out << str_util::to_string(ss.str());
            return out;
        }
        std::ostream& operator << (std::ostream& out, const base_exception& e) {
			t_stringstream ss;
            e.pretty_print_one_line(ss, print::output_default);
			out << str_util::to_string(ss.str());
            return out;
        }

		std::ostream& operator << (std::ostream& out, const olib::opencorelib::invoke_assert& a) {
			t_stringstream ss;
			const_cast<invoke_assert&>(a).pretty_print(ss, print::output_default );
			out << str_util::to_string(ss.str());
			return out;
		}
    }
}

class TestingLogTarget : public olib::opencorelib::logtarget {
public:
    /// An assertion occurred. log if required.
    /** @param a The assertion to log
        @param log_source The name of the class (including namespaces) that
        requests the log. */
    virtual void log(olib::opencorelib::invoke_assert& a, const TCHAR* log_source) {
        BOOST_TEST_MESSAGE(a);
    }
    
    /// An exception occurred. log if required.
    /** @param e The exception to log
        @param log_source The name of the class (including namespaces) that
        requests the log. */
    virtual void log(olib::opencorelib::base_exception& e, const TCHAR* log_source) {
        BOOST_TEST_MESSAGE(e);
    }
    
    /// An log request occurred. log if required.
    /** @param e The log message to log
        @param log_source The name of the class (including namespaces) that
        requests the log. */
    virtual void log(olib::opencorelib::logger& log_msg, const TCHAR* log_source) {
        BOOST_TEST_MESSAGE(log_msg);
    }
};

boost::unit_test_framework::test_suite* init_unit_test_suite ( int argc, char* argv[] ){
    using namespace olib::opencorelib;
    static boost::shared_ptr<logtarget> target(new TestingLogTarget());
    the_log_handler::instance().add_target(target);
    the_log_handler::instance().set_global_log_level(log_level::debug9);
    
	typedef std::map< std::string, boost::function< void () > > TestMap;
	TestMap myTests;
	myTests["test_assert"] = &test_assert;
    myTests["test_multivalue_property_parsing"] = &test_multivalue_property_parsing;
    myTests["test_invoker"] = &test_invoker;
	myTests["test_logprint"] = &test_logprint;
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
	myTests["test_base64_conversions"] = &test_base64_conversions;

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
