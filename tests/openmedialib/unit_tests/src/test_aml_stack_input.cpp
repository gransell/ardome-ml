#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/input.hpp>

using namespace olib::openmedialib::ml;

BOOST_AUTO_TEST_CASE( test_empty_right_hand_side_in_property )
{
	input_type_ptr inp = create_input( L"aml_stack:" );
	BOOST_REQUIRE( inp );

	std::list< std::wstring > tokens;
	tokens.push_back( L"colour:" );
	tokens.push_back( L"colourspace=" ); //Empty right hand side

	//We basically just test that we don't crash here
	inp->property( "commands" ) = tokens;
	const std::wstring result = inp->property( "result" ).value< std::wstring >();
	BOOST_REQUIRE( result == L"OK" );
}

