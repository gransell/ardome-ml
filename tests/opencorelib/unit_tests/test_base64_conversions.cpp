#include "precompiled_headers.hpp"

#include "./utils.hpp"

#include "opencorelib/cl/detail/base64_conversions.hpp"

#include <boost/test/test_tools.hpp>
#include <sstream>
#include <istream>

using std::string;
using std::stringstream;
using std::istream_iterator;
using std::ostream_iterator;
using std::back_inserter;
using std::vector;
using olib::opencorelib::detail::base64_encode;
using olib::opencorelib::detail::base64_decode;

void test( const boost::uint8_t *data, size_t length )
{
	stringstream enc_stream;
	base64_encode( data, data + length, ostream_iterator<char>(enc_stream) );

	//Check that the encoded string is of the expected length
	BOOST_CHECK_EQUAL( enc_stream.str().size(), ( length + 2 ) / 3 * 4 );

	vector<boost::uint8_t> result;
	base64_decode( istream_iterator<char>(enc_stream), istream_iterator<char>(), back_inserter(result) );

	//Check that we got the same data back out
	BOOST_CHECK_EQUAL( length, result.size() );
	for( size_t i=0; i<length; i++ )
	{
		BOOST_CHECK_EQUAL( data[i], result[i] );
	}
}

void test_base64_conversions()
{
	boost::uint8_t test_data[] = { 0x03, 0x10, 0xB8, 0xFF, 0xFF, 0x04, 0x34, 0x00 };
	
	for( size_t i=0; i<=sizeof(test_data); i++ )
	{
		test( test_data, i );
	}

	std::cout << "Running large base64 encode/decode test..." << std::endl;
	//Encode and decode a 500 KiB random data buffer
	vector<boost::uint8_t> large_test( 500 * 1024 );
	srand( (unsigned int)(time(0)) );
	for( size_t i=0; i<large_test.size(); i++ )
	{
		large_test[i] = rand()%256;
	}

	test( &large_test[0], large_test.size() );
	test( &large_test[0], large_test.size() - 1 );
	test( &large_test[0], large_test.size() - 2 );

	std::cout << "Done." << std::endl;


	//Check that whitespace in the encoded data is ignored
	vector<boost::uint8_t> whitespace_expected( test_data, test_data + sizeof(test_data) );
	vector<boost::uint8_t> whitespace_result;
	vector<string> whitespace_tests;
	whitespace_tests.push_back( " AxC4//8ENAA=" );
	whitespace_tests.push_back( " A xC4//8ENAA=" );
	whitespace_tests.push_back( "AxC4//8E  NAA=" );
	whitespace_tests.push_back( "AxC4//8ENAA = " );
	whitespace_tests.push_back( "\nAxC4//8ENAA= " );
	whitespace_tests.push_back( "\nAxC4//8ENAA=\n" );
	whitespace_tests.push_back( "\tAxC4//8ENAA=\n" );
	for( size_t i=0; i<whitespace_tests.size(); ++i )
	{
		base64_decode( whitespace_tests[i].begin(), whitespace_tests[i].end(), back_inserter( whitespace_result ) );

		BOOST_CHECK( whitespace_result == whitespace_expected );

		whitespace_result.clear();
	}


	vector<boost::uint8_t> result_vec;
	vector<string> invalid_input_tests;
	invalid_input_tests.push_back( "A" );
	invalid_input_tests.push_back( "A==" );
	invalid_input_tests.push_back( "AB" );
	invalid_input_tests.push_back( "ABC" );
	invalid_input_tests.push_back( "ABCDE" );
	invalid_input_tests.push_back( "AB=A" );
	invalid_input_tests.push_back( "=" );
	invalid_input_tests.push_back( "==" );
	invalid_input_tests.push_back( "#ABC" );
	invalid_input_tests.push_back( "ABCyz%59" );

	//Make sure that the decoder throws when given invalid input
	for( size_t i = 0; i < invalid_input_tests.size(); i++ )
	{
		const string &str = invalid_input_tests[i];
		bool did_throw = false;
		try
		{
			base64_decode( str.begin(), str.end(), back_inserter( result_vec ) );
		}
		catch( olib::opencorelib::base_exception& )
		{
			did_throw = true;
		}

		BOOST_CHECK( did_throw );

		result_vec.clear();
	}
}

