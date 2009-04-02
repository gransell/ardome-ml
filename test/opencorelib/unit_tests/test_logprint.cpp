#include "precompiled_headers.hpp"
#include <opencorelib/cl/logprinter.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem;
using namespace boost;
using namespace olib;
using namespace olib::opencorelib;
using namespace olib::opencorelib::log_utilities;

namespace Foo
{
    namespace Bar
    {
        void LogTest()
        {
            t_string short_str(_CT("some val"));
            ARLOG("This is a cool test: The value is = %s")(short_str);
            ARLOG_IF_LEVEL( log_level::emergency ,"Another cool test %s" )(short_str);
            ARLOG_IF( short_str.size() > 8, "The file name: %s" )( short_str);
        }
    }
}
void test_logprint()
{	
	olib::t_string prefix(_CT("TestLogprint"));

    t_string dateFmt(_CT("%a-")), timeFmt(_CT("%H:%M:%S%F"));
    t_string str = get_log_prefix_string(  log_level::debug2, dateFmt, timeFmt, logoutput::output_default );
    t_string short_str = get_log_prefix_string(log_level::debug2, dateFmt, timeFmt, logoutput::output_default );

    olib::t_path fpath = log_utilities::get_sutiable_logfile_name(prefix);
	fs::remove( fpath );

    // This is tested elsewhere, in render_unit_tests.

 //   shared_ptr< default_log_target > 
 //       myTarget( new default_log_target(get_default_log_stream(fpath)));
 //   the_log_handler::instance().add_target(myTarget);

 //   for(int i = 0; i < 5; ++i)
 //   {
 //       ARLOG("i = %i")(i);
 //   }

 //   Foo::Bar::LogTest();

	//// Make sure file exists
	//std::cout << "Checking that " <<  str_util::to_string(fpath.string()) << " exists" << std::endl;
	//BOOST_CHECK( fs::exists( fpath ) );
    // Just make the test fail ... to see if we get mail :-)
    // BOOST_CHECK( false );
}
