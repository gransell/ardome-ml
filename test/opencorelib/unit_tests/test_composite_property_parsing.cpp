#include "precompiled_headers.hpp"
#include <boost/test/test_tools.hpp>

#include "opencorelib/cl/utilities.hpp"
#include "opencorelib/cl/str_util.hpp"

using olib::t_string;
using namespace olib::opencorelib;
using namespace olib::opencorelib::str_util;

//Required in order to use wstrings with BOOST_CHECK_EQUAL
namespace std {
	inline std::ostream &operator<<(std::ostream& output_stream, const std::wstring &the_string)
	{
		return output_stream << to_string(the_string);
	}
}

bool did_throw( const t_string &test_str )
{
    bool threw_exception = false;
    
    try
    {
        utilities::parse_multivalue_property( test_str );
    }
    catch( base_exception & )
    {
        threw_exception = true;
    }

    return threw_exception;
}

void basic_test()
{
    //Test parsing a property string with correct syntax
    t_string basic_test = _CT("prop1=5|prop2=yuv420p|codec_def=http://www.ardendo.com/apf/codec/pcm");
    multivalue_property_map result = utilities::parse_multivalue_property( basic_test );

    BOOST_CHECK_EQUAL( result.size(), 3 );

    BOOST_CHECK( result.find(_CT("prop1")) != result.end() );
    BOOST_CHECK( result.find(_CT("prop2")) != result.end() );
    BOOST_CHECK( result.find(_CT("codec_def")) != result.end() );

    BOOST_CHECK_EQUAL( result[_CT("prop1")], _CT("5") );
    BOOST_CHECK_EQUAL( result[_CT("prop2")], _CT("yuv420p") );
    BOOST_CHECK_EQUAL( result[_CT("codec_def")], _CT("http://www.ardendo.com/apf/codec/pcm") );

    //Check that the empty string doesn't throw an exception
    BOOST_CHECK( ! did_throw(_CT("")) );
}

void equals_sign_test()
{
    //Test that incorrect placement or number of equals signs throws exceptions
    BOOST_CHECK( did_throw(_CT("prop")) );
    BOOST_CHECK( did_throw(_CT("prop1=rgba|prop2")) );
    BOOST_CHECK( did_throw(_CT("prop1=rgba|=yuv420p")) );
    BOOST_CHECK( did_throw(_CT("prop1=|prop2=yuv420p")) );
    BOOST_CHECK( did_throw(_CT("prop1=rgba=|prop2=yuv420p")) );
}

void missing_key_test()
{
    BOOST_CHECK( did_throw(_CT("=5")) );
    BOOST_CHECK( did_throw(_CT("foo=rgba|=bar")) );
}

void missing_value_test()
{
    BOOST_CHECK( did_throw(_CT("foo=")) );
    BOOST_CHECK( did_throw(_CT("foo=rgba|bar=")) );
}
void test_multivalue_property_parsing()
{
    basic_test();
    equals_sign_test();
    missing_key_test();
    missing_value_test();
}
