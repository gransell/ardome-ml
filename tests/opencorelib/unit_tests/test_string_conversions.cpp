#include "precompiled_headers.hpp"
#include <boost/test/auto_unit_test.hpp>

using namespace olib::opencorelib;

BOOST_AUTO_TEST_SUITE( test_string_conversions )

// &aring; = 0xc3a5
BOOST_AUTO_TEST_CASE( correct_utf8_to_utf16 )
{
	std::string input("hello \303\245");
	std::wstring result( str_util::to_wstring(input));

	BOOST_CHECK( result.compare(L"hello \345") == 0 );
}

BOOST_AUTO_TEST_CASE( correct_utf16_to_utf8 )
{
	std::wstring input(L"hello \345");
	std::string result( str_util::to_string(input));

	BOOST_CHECK( result.compare("hello \303\245") == 0 );

};

BOOST_AUTO_TEST_CASE( try_latin1_to_utf16 )
{
	std::string input("hello \345ff");
	BOOST_REQUIRE_THROW(str_util::to_wstring(input), std::exception );

	std::string input2("hello \345");
	BOOST_REQUIRE_THROW(str_util::to_wstring(input2), std::exception );
}

BOOST_AUTO_TEST_CASE( t_string_roundtrip_8 )
{
	std::string input("hello \303\245");
	std::string result = str_util::to_string( str_util::to_t_string(input));

	BOOST_CHECK( result.compare(input) == 0 );
}

BOOST_AUTO_TEST_CASE( t_string_roundtrip_16 )
{
	std::wstring input(L"hello \345");
	std::wstring result = str_util::to_wstring( str_util::to_t_string(input));

	BOOST_CHECK( result.compare(input) == 0 );
}

BOOST_AUTO_TEST_CASE( test_string_trim )
{
	olib::t_string test1(_CT("   "));
	olib::t_string result = str_util::trim(test1);

	//Make sure that a string full of whitespace that is trimmed
	//returns an empty string.
	BOOST_CHECK( result.empty() );
}

BOOST_AUTO_TEST_SUITE_END()

