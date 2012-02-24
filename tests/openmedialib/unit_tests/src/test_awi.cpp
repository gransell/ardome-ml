#include <boost/test/unit_test.hpp>
#include <openmedialib/ml/awi.hpp>

using namespace olib::openmedialib::ml;

template< typename AWI_GEN_TYPE >
void test_calculate_method_simple( AWI_GEN_TYPE &generator )
{
	//Nothing enrolled, should always give 0 frames back
	BOOST_CHECK_EQUAL( generator.calculate( 0 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 1 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 100 ), 0 );

	BOOST_REQUIRE( generator.enroll( 0, 0, 100 ) );
	BOOST_CHECK_EQUAL( generator.calculate( 0 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 99 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 100 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 101 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 500 ), 1 );

	BOOST_REQUIRE( generator.enroll( 1, 100, 50 ) );
	BOOST_CHECK_EQUAL( generator.calculate( 0 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 99 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 100 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 101 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 149 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 150 ), 2 );
	BOOST_CHECK_EQUAL( generator.calculate( 151 ), 2 );
	BOOST_CHECK_EQUAL( generator.calculate( 500 ), 2 );

	BOOST_REQUIRE( generator.enroll( 2, 150, 200 ) );
	BOOST_CHECK_EQUAL( generator.calculate( 0 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 99 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 100 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 101 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 149 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 150 ), 2 );
	BOOST_CHECK_EQUAL( generator.calculate( 151 ), 2 );
	BOOST_CHECK_EQUAL( generator.calculate( 349 ), 2 );
	BOOST_CHECK_EQUAL( generator.calculate( 350 ), 3 );
	BOOST_CHECK_EQUAL( generator.calculate( 351 ), 3 );
	BOOST_CHECK_EQUAL( generator.calculate( 500 ), 3 );
}

template< typename AWI_GEN_TYPE >
void test_calculate_method_no_length( AWI_GEN_TYPE &generator )
{
	//Nothing enrolled, should always give 0 frames back
	BOOST_CHECK_EQUAL( generator.calculate( 0 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 1 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 100 ), 0 );

	BOOST_REQUIRE( generator.enroll( 0, 0 ) );
	//Index should not account for the frame yet, since it has no length
	BOOST_CHECK_EQUAL( generator.calculate( 0 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 99 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 100 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 101 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 500 ), 0 );

	BOOST_REQUIRE( generator.enroll( 1, 100 ) );
	BOOST_CHECK_EQUAL( generator.calculate( 0 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 99 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 100 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 101 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 500 ), 1 );

	BOOST_REQUIRE( generator.enroll( 2, 150 ) );
	BOOST_CHECK_EQUAL( generator.calculate( 0 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 99 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 100 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 101 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 149 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 150 ), 2 );
	BOOST_CHECK_EQUAL( generator.calculate( 151 ), 2 );
	BOOST_CHECK_EQUAL( generator.calculate( 500 ), 2 );

	BOOST_REQUIRE( generator.close( 3, 350 ) );
	//Since the index is now closed, we should get all frames
	BOOST_CHECK_EQUAL( generator.calculate( 0 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 99 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 100 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 101 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 149 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 150 ), 2 );
	BOOST_CHECK_EQUAL( generator.calculate( 151 ), 2 );
	BOOST_CHECK_EQUAL( generator.calculate( 349 ), 2 );
	BOOST_CHECK_EQUAL( generator.calculate( 350 ), 3 );
	BOOST_CHECK_EQUAL( generator.calculate( 351 ), 3 );
	BOOST_CHECK_EQUAL( generator.calculate( 500 ), 3 );
}

template< typename AWI_GEN_TYPE >
void test_calculate_method_with_gaps( AWI_GEN_TYPE &generator )
{
	BOOST_REQUIRE( generator.enroll( 0, 50, 100 ) );
	BOOST_CHECK_EQUAL( generator.calculate( 0 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 50 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 149 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 150 ), 1 );

	BOOST_REQUIRE( generator.enroll( 1, 200, 50 ) );
	BOOST_CHECK_EQUAL( generator.calculate( 0 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 50 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 149 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 150 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 200 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 249 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 250 ), 2 );

	BOOST_REQUIRE( generator.enroll( 2, 500, 100 ) );
	BOOST_CHECK_EQUAL( generator.calculate( 0 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 50 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 149 ), 0 );
	BOOST_CHECK_EQUAL( generator.calculate( 150 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 200 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 249 ), 1 );
	BOOST_CHECK_EQUAL( generator.calculate( 250 ), 2 );
	BOOST_CHECK_EQUAL( generator.calculate( 499 ), 2 );
	BOOST_CHECK_EQUAL( generator.calculate( 500 ), 2 );
	BOOST_CHECK_EQUAL( generator.calculate( 599 ), 2 );
	BOOST_CHECK_EQUAL( generator.calculate( 600 ), 3 );
}

template< typename AWI_GEN_TYPE >
void test_calculate_method( )
{
	AWI_GEN_TYPE g1;
	test_calculate_method_simple( g1 );
	AWI_GEN_TYPE g2;
	test_calculate_method_no_length( g2 );
	AWI_GEN_TYPE g3;
	test_calculate_method_with_gaps( g3 );
}

template< >
void test_calculate_method< awi_generator_v4 >( )
{
	awi_generator_v4 g1( 0 );
	test_calculate_method_simple( g1 );
	awi_generator_v4 g2( 0 );
	test_calculate_method_no_length( g2 );
	awi_generator_v4 g3( 0 );
	test_calculate_method_with_gaps( g3 );
}

BOOST_AUTO_TEST_SUITE( awi )

BOOST_AUTO_TEST_CASE( version2 )
{
	test_calculate_method< awi_generator_v2 >( );
}

BOOST_AUTO_TEST_CASE( version3 )
{
	test_calculate_method< awi_generator_v3 >( );
}

BOOST_AUTO_TEST_CASE( version4 )
{
	test_calculate_method< awi_generator_v4 >( );
}

BOOST_AUTO_TEST_SUITE_END()
