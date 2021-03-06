#define BOOST_TEST_MODULE openmedialib_tests
#include <boost/test/unit_test.hpp>
#include <fstream>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/utilities.hpp>
#include <opencorelib/cl/special_folders.hpp>
#include <openpluginlib/pl/openpluginlib.hpp>
#include <openmedialib/ml/indexer.hpp>

#include "../../../utils.hpp"
#include "io_tester.hpp"

namespace pl = olib::openpluginlib;
namespace cl = olib::opencorelib;
namespace io = olib::openmedialib::ml::io;

void init_pl()
{
#ifdef WIN32
    #define PATH_LEN 1024

    char mod_name[ PATH_LEN ];
    DWORD ret = ::GetModuleFileNameA( NULL, mod_name, PATH_LEN );

    ARENFORCE_MSG( ret > 0 && ret < PATH_LEN, "Failed to get the path to the current executable. Error returned was:\n%1%" )
        ( cl::utilities::handle_system_error( false ) )( ret );

    std::string a_path(mod_name);
    std::string::size_type pos = a_path.rfind("\\");
    ARENFORCE_MSG( pos != std::string::npos, "Unexpected format of path to current executable: %1%" )( a_path );

    std::string a_path2 = a_path.substr(0, pos);
    a_path2 += "\\aml-plugins";

    pl::init( a_path2 );
#elif defined( __APPLE__ )
        olib::t_path plugins_path = cl::special_folder::get( cl::special_folder::plugins );
        pl::init( plugins_path.string( ) );
#else
        setlocale(LC_CTYPE, "");
        pl::init( );
#endif
}

std::ofstream log_file_;

class init_fixture {
public:
	init_fixture( )
	{
		using namespace boost::unit_test;

		const olib::t_string log_path = olib::configure_test_runner( _CT("openmedialib") );
		log_file_.open( log_path.c_str() );
		ARENFORCE( log_file_.good() );
		unit_test_log.set_stream( log_file_ );

		init_pl( );
		register_io_tester_protocol();
	}

	~init_fixture( )
	{
		olib::openmedialib::ml::indexer_shutdown( );
		olib::openpluginlib::uninit( );
	}
};

BOOST_GLOBAL_FIXTURE( init_fixture );
