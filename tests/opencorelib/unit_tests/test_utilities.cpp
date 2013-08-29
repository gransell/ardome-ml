#include "precompiled_headers.hpp"
#include "opencorelib/cl/utilities.hpp"
#include "opencorelib/cl/special_folders.hpp"
#include "opencorelib/cl/str_util.hpp"
#include <boost/test/test_tools.hpp>

using namespace olib;
using namespace olib::opencorelib;

// NOTE: This is a local fix for a global problem, we should fix so
// BOOST_CHECK_EQUAL can work with t_strings.
void T_CHECK_EQUAL(const t_string& correct, const t_string& given) {
  std::string msg = "Expected " + str_util::to_string(correct)
    + ", got " + str_util::to_string(given);
  BOOST_CHECK_MESSAGE(given.compare(correct) == 0, msg);
}

void test_utilities()
{
    t_string host1(_CT("my_server"));
    t_string base_url(_CT("/f1/f2\\f3\\\\f5\\"));
    t_string file_name(_CT("/file.bin"));

    t_string p = utilities::make_smb_path( host1, base_url, file_name );
    
    T_CHECK_EQUAL(_CT("smb://my_server/f1/f2/f3/f5/file.bin"), p);

    t_string user_name(_CT("amf"));
    t_string pwd(_CT("amf_pwd"));
    t_string host(_CT("ardwindev"));
    t_string port(_CT("1999"));
    t_string burl(_CT("whhops/this///was not"));
    t_string fn(_CT("///some_file.name.txt"));

    t_string url = utilities::make_protocol_path(user_name, pwd, host, port, burl, fn, _CT("http:"));

    T_CHECK_EQUAL(_CT("http://amf:amf_pwd@ardwindev:1999/whhops/this/was not/some_file.name.txt"), url);

    olib::t_path wp = special_folder::get( special_folder::amf_resources );
    BOOST_CHECK( !wp.empty() );

	std::vector< olib::opencorelib::library_info_ptr > infos = olib::opencorelib::utilities::get_loaded_libraries();

	t_string regex_chars_escaped = utilities::regex_escape(_CT("{}^.$|()[]*+?/\\"));
	T_CHECK_EQUAL(_CT("\\{\\}\\^\\.\\$\\|\\(\\)\\[\\]\\*\\+\\?\\/\\\\"), regex_chars_escaped);

	/*for(size_t i = 0; i < infos.size(); ++i  )
	{
		T_COUT << *(infos[i]) << std::endl;
	}*/
}
