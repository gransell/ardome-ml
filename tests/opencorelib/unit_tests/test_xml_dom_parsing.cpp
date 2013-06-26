#include "precompiled_headers.hpp"

#include <boost/test/auto_unit_test.hpp>

#include <opencorelib/cl/xerces_dom_document.hpp>
#include <xercesc/sax/SAXParseException.hpp>

using namespace olib::opencorelib::xml;

static const std::string valid_xml(
	"<outer_tag>\n"
	"  <inner_tag attrib1=\"value1\" attrib2=\"value2\">String value here</inner_tag>\n"
	"  <inner_tag/>\n"
	"  \n"
	"  <nested_tag attrib=\"value\">\n"
	"    <nested_tag attrib=\"another value\">5</nested_tag>\n"
	"  </nested_tag>\n"
	"</outer_tag>\n" );

static const std::string invalid_xml( 
	"<open_tag>\n"
	"  <inner_tag></inner_tag>\n"
	"</closing_tag_does_not_match>\n" );

void check_attrib( const dom::node &node, const char *attrib_name, const char *expected_value )
{
	BOOST_REQUIRE( node.valid() );
	BOOST_REQUIRE( node.hasAttribute( attrib_name ) );
	std::string actual_value;
	BOOST_REQUIRE( node.getAttribute( attrib_name, actual_value ) );
	BOOST_REQUIRE_EQUAL( actual_value, expected_value );
}

template< typename ParserClass, typename InputType >
void test_xml_parsing( InputType &xml_input )
{
	ParserClass instance( xml_input );

	dom::node outer_node = instance.first( "outer_tag" );
	BOOST_REQUIRE( outer_node.valid() );
	BOOST_REQUIRE_EQUAL( outer_node.getSourceLineNumber(), 1 );

	dom::nodes inner_nodes = outer_node.all( "inner_tag" );
	BOOST_REQUIRE_EQUAL( inner_nodes.size(), 2 );

	BOOST_REQUIRE( inner_nodes[0].valid() );
	BOOST_REQUIRE_EQUAL( inner_nodes[0].getSourceLineNumber(), 2 );
	check_attrib( inner_nodes[0], "attrib1", "value1" );
	check_attrib( inner_nodes[0], "attrib2", "value2" );
	BOOST_REQUIRE_EQUAL( inner_nodes[0].getValue(), "String value here" );

	BOOST_REQUIRE( inner_nodes[1].valid() );
	BOOST_REQUIRE_EQUAL( inner_nodes[1].getSourceLineNumber(), 3 );
	BOOST_REQUIRE_EQUAL( inner_nodes[1].getValue(), "" );

	dom::node nested_node = outer_node.first( "nested_tag" );
	BOOST_REQUIRE( nested_node.valid() );
	BOOST_REQUIRE_EQUAL( nested_node.getSourceLineNumber(), 5 );
	check_attrib( nested_node, "attrib", "value" );
	BOOST_REQUIRE_EQUAL( nested_node.getValue(), "" );
	BOOST_REQUIRE_EQUAL( nested_node.getValueRecursively(), "\n    5\n  " );

	dom::node nested_node2 = nested_node.first( "nested_tag" );
	BOOST_REQUIRE( nested_node2.valid() );
	BOOST_REQUIRE_EQUAL( nested_node2.getSourceLineNumber(), 6 );
	check_attrib( nested_node2, "attrib", "another value" );
	BOOST_REQUIRE_EQUAL( nested_node2.getValue(), "5" );
}

template< typename ParserClass, typename InputType >
void test_invalid_xml( InputType &invalid_input )
{
	bool did_throw = false;
	try
	{
		ParserClass instance( invalid_input );
	}
	catch( const XERCES_CPP_NAMESPACE::SAXParseException &exc )
	{
		did_throw = true;
		BOOST_CHECK_EQUAL( exc.getLineNumber(), 3 );
	}

	BOOST_CHECK( did_throw );
}

BOOST_AUTO_TEST_SUITE( test_xml_dom_parsing )

BOOST_AUTO_TEST_CASE( test_file_document_parser )
{
	std::istringstream strm( valid_xml );
	test_xml_parsing< dom::file_document >( strm );
}

BOOST_AUTO_TEST_CASE( test_fragment_parser )
{
	test_xml_parsing< dom::fragment >( valid_xml );
}

BOOST_AUTO_TEST_CASE( test_invalid_xml_with_file_document_parser )
{
	std::istringstream strm( invalid_xml );
	test_invalid_xml< dom::file_document >( strm );
}

BOOST_AUTO_TEST_CASE( test_invalid_xml_with_fragment_parser )
{
	test_invalid_xml< dom::fragment >( invalid_xml );
}

BOOST_AUTO_TEST_SUITE_END()

