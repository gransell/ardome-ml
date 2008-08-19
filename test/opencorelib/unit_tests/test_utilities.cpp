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
    t_string host1(_T("my_server"));
    t_string base_url(_T("/f1/f2\\f3\\\\f5\\"));
    t_string file_name(_T("/file.bin"));

    t_string p = utilities::make_smb_path( host1, base_url, file_name );
    
    T_CHECK_EQUAL(_T("smb://my_server/f1/f2/f3/f5/file.bin"), p);

    t_string user_name(_T("amf"));
    t_string pwd(_T("amf_pwd"));
    t_string host(_T("ardwindev"));
    t_string port(_T("1999"));
    t_string burl(_T("whhops/this///was not"));
    t_string fn(_T("///some_file.name.txt"));

    t_string url = utilities::make_protocol_path(user_name, pwd, host, port, burl, fn, _T("http:"));

    T_CHECK_EQUAL(_T("http://amf:amf_pwd@ardwindev:1999/whhops/this/was not/some_file.name.txt"), url);

    olib::t_path wp = special_folder::get( special_folder::amf_resources );
    BOOST_CHECK( !wp.empty() );
}
