#include "precompiled_headers.hpp"
#include <boost/test/test_tools.hpp>

using namespace olib::opencorelib;

// &aring; = 0xc3a5
void correct_utf8_to_utf16()
{
	std::string input("hello \303\245");
	std::wstring result( str_util::to_wstring(input));

	BOOST_CHECK( result.compare(L"hello \345") == 0 );
}

void correct_utf16_to_utf8()
{
	std::wstring input(L"hello \345");
	std::string result( str_util::to_string(input));

	BOOST_CHECK( result.compare("hello \303\245") == 0 );

};

void try_latin1_to_utf16()
{
	std::string input("hello \345ff");
	BOOST_REQUIRE_THROW(str_util::to_wstring(input), std::exception );

	std::string input2("hello \345");
	BOOST_REQUIRE_THROW(str_util::to_wstring(input2), std::exception );
}

void t_string_roundtrip_8()
{
	std::string input("hello \303\245");
	std::string result = str_util::to_string( str_util::to_t_string(input));

	BOOST_CHECK( result.compare(input) == 0 );
}

void t_string_roundtrip_16()
{
	std::wstring input(L"hello \345");
	std::wstring result = str_util::to_wstring( str_util::to_t_string(input));

	BOOST_CHECK( result.compare(input) == 0 );
}

void test_string_conversions()
{
	correct_utf8_to_utf16();
	correct_utf16_to_utf8();
	try_latin1_to_utf16();
	t_string_roundtrip_8();
	t_string_roundtrip_16();
}
