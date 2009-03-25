#include "precompiled_headers.hpp"
#include <boost/test/test_tools.hpp>

#include "opencorelib/cl/invoker.hpp"
#include "opencorelib/cl/color.hpp"
#include "opencorelib/cl/minimal_string_defines.hpp"
#include <boost/algorithm/string.hpp>

using namespace olib::opencorelib;

void test_color()
{
    //invoker_ptr inv = create_invoker(0);

    olib::t_path a_path(_T("\\\\ardomedev\\lr0\\date\\folder\\file.mpeg"));

    olib::t_string rootn = a_path.root_name();
    olib::t_string rootp = a_path.root_path().string();

    boost::algorithm::trim_left_if( rootn, boost::algorithm::is_any_of(_T("\\/")));

    boost::uint8_t vals[8];
    memset(vals, 0, 8);
    vals[0] = 255;
    vals[4] = 128;
    vals[5] = 10;
    vals[6] = 10;
    vals[7] = 10;

    rgba_color c1(vals);
    rgba_color c2(vals + 4);

    rgba_color c3( rgba_color::aliceblue());

    BOOST_REQUIRE( c1.get_red() == 255 );
    BOOST_REQUIRE( c1.get_green() == 0 );
    BOOST_REQUIRE( c1.get_blue() == 0 );
    BOOST_REQUIRE( c1.get_alpha() == 0 );

    BOOST_REQUIRE( c2.get_red() == 128 );
    BOOST_REQUIRE( c2.get_green() == 10 );
    BOOST_REQUIRE( c2.get_blue() == 10 );
    BOOST_REQUIRE( c2.get_alpha() == 10 );

    BOOST_REQUIRE( c2.to_string() == _T("(128,10,10,10)"));
    
}

