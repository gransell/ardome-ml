#include "precompiled_headers.hpp"
#include <boost/test/auto_unit_test.hpp>

#include "opencorelib/cl/utilities.hpp"
#include "opencorelib/cl/uuid_16b.hpp"

using namespace boost;
using namespace olib;
using namespace olib::opencorelib;
using namespace olib::opencorelib::utilities;
using namespace olib::opencorelib::str_util;

BOOST_AUTO_TEST_CASE( test_uuid )
{
        uuid_16b id;

        t_stringstream ss;
        ss << id;
        uuid_16b id2;
        ss >> id2;

        BOOST_CHECK_EQUAL( to_string( id.to_hex_string()) , to_string( id2.to_hex_string() ) );

        uuid_16b id3;
        if( id < id3 ) 
        {
            BOOST_REQUIRE_MESSAGE( id3 > id, "Less than and greater than not consistent.");
        }
        else
        {
            BOOST_REQUIRE_MESSAGE( id3 < id, "Less than and greater than not consistent.");
        }
}

