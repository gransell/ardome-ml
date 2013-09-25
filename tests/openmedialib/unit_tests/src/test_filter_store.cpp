#include <boost/test/unit_test.hpp>

#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/store.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/filter.hpp>

#include <mocks/mock_store.hpp>
#include <mocks/mock_input.hpp>

namespace ml = olib::openmedialib::ml;
namespace pcos = olib::openpluginlib::pcos;

BOOST_AUTO_TEST_SUITE( filter_store )

class TestEnv
{
public:
	TestEnv ()
	{
		m_filter_store = ml::create_filter( L"store" );
		m_filter_store->properties( ).get_property_with_string( "store" ).set_from_string( L"mock:" );
		m_input = boost::shared_ptr< mock_input >(new mock_input);
		m_frame = ml::frame_type_ptr (new ml::frame_type);
		m_store = boost::static_pointer_cast <mock_store> (ml::create_store( L"mock:", m_frame));
		m_store->reset ();
	}

	ml::filter_type_ptr m_filter_store;
	boost::shared_ptr< mock_input > m_input;
	boost::shared_ptr< mock_store > m_store;
	ml::frame_type_ptr m_frame;
};

BOOST_FIXTURE_TEST_CASE( Test_fetch_null_throws_exception, TestEnv )
{
	ml::frame_type_ptr f =  m_filter_store->fetch( );

	BOOST_CHECK_EQUAL (f->in_error( ), true);	
}

BOOST_FIXTURE_TEST_CASE( Test_inner_store_created, TestEnv )
{
	int create_count = m_store->m_create_count;
	m_filter_store->connect (m_input);
	m_filter_store->fetch( );
	
	BOOST_CHECK_EQUAL (m_store->m_create_count, create_count + 1);
}

BOOST_FIXTURE_TEST_CASE( Test_parameters_forwarded_to_inner_store, TestEnv )
{
	pcos::property mock_property1( pcos::key::from_string( "mock_property" ) );
	pcos::property mock_property2( pcos::key::from_string( "@store.mock_property" ) );
	
	mock_property1 = std::wstring(L"1");
	mock_property2 = std::wstring(L"2");

	m_store->properties( ).append( mock_property1 );
	m_filter_store->properties( ).append( mock_property2 );
	
	m_filter_store->connect (m_input);

	m_filter_store->fetch( );
	
	BOOST_CHECK (mock_property1.value <std::wstring>() == std::wstring(L"2"));
}

BOOST_FIXTURE_TEST_CASE( Test_complete_invoked_on_inner_store, TestEnv )
{
	{
		TestEnv env;
		env.m_filter_store->connect (m_input);
		env.m_filter_store->fetch( );
		BOOST_CHECK_EQUAL (m_store->m_complete, false);
	}
	
	BOOST_CHECK_EQUAL (m_store->m_complete, true);
}

BOOST_AUTO_TEST_SUITE_END()
