#include "precompiled_headers.hpp"

#include <boost/test/auto_unit_test.hpp>
#include "opencorelib/cl/color.hpp"

using namespace olib::opencorelib;

BOOST_AUTO_TEST_CASE( test_color )
{
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

    BOOST_REQUIRE( c1.get_r() == 255 );
    BOOST_REQUIRE( c1.get_g() == 0 );
    BOOST_REQUIRE( c1.get_b() == 0 );
    BOOST_REQUIRE( c1.get_alpha() == 0 );

    BOOST_REQUIRE( c2.get_r() == 128 );
    BOOST_REQUIRE( c2.get_g() == 10 );
    BOOST_REQUIRE( c2.get_b() == 10 );
    BOOST_REQUIRE( c2.get_alpha() == 10 );

    BOOST_REQUIRE( c2.to_string() == _CT("(128,10,10,10)"));
    
}

