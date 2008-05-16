
#include "precompiled_headers.hpp"

#include "opencorelib/cl/utilities.hpp"
#include "opencorelib/cl/uuid_16b.hpp"

#include <boost/test/test_tools.hpp> 

using namespace boost;
using namespace olib;
using namespace olib::opencorelib;
using namespace olib::opencorelib::utilities;
using namespace olib::opencorelib::str_util;


void test_uuid()
{
        uuid_16b id;

        // T_COUT << id << std::endl;

        t_stringstream ss;
        ss << id;
        uuid_16b id2;
        ss >> id2;

        //T_COUT << id2 << std::endl;

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

        //for(int i = 0; i < 10; ++i )
        //    T_COUT << uuid_16b() << std::endl;
}
