#include "precompiled_headers.hpp"

#include <boost/test/test_tools.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

using namespace olib;
namespace fs = boost::filesystem;
namespace ac = olib::opencorelib;

namespace
{
    void SetupCrashReport()
    {
        fs::wpath wd = fs::initial_path<fs::wpath>();
        wd /= L"aubase_unittest_crasch_report.dmp";
        // t_string pth = ac::str_util::to_t_string(wd.string());

        // olib::opencorelib::the_serious_error_handler::instance().setup();
        // olib::opencorelib::the_serious_error_handler::instance().set_dump_file_name(pth);
    }
}


void DivideByZero()
{
    SetupCrashReport();
    
    try
    {
        int zeroVal(0);
        int a = 17 / zeroVal;
        ARUNUSED_ALWAYS(a);
    }
    catch( std::exception& se)
    {
        std::cout << se.what() << std::endl;
    }
}

void test_serious_error_handler()
{
    SetupCrashReport();

    try
    {
        //*((int*)0) = 1;
    }
    catch( std::exception& se)
    {
        std::cout << se.what() << std::endl;
    }
}
