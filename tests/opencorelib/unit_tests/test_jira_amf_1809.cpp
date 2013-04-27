#include "precompiled_headers.hpp"
#include <boost/test/auto_unit_test.hpp>

#include <opencorelib/cl/logtarget.hpp>
#include <opencorelib/cl/loghandler.hpp>

using namespace olib::opencorelib;

class testing_logtarget : public logtarget
{
public:
	testing_logtarget()
		: num_messages_logged_( 0 )
		, num_exceptions_logged_( 0 )
		, wrong_message_( false )
	{}

	virtual void log( invoke_assert &a, const TCHAR *log_source)
	{
		BOOST_ERROR( "assert log call was unexpected" );
	}

	void check( exception_context &e )
	{
		wrong_message_ = ( e.message() != _CT("Value of this pointer is: ( NULL )") );
	}

	virtual void log( base_exception &e, const TCHAR *log_source)
	{
		check( e.context() );
		++num_exceptions_logged_;
	}
	virtual void log( logger &log_obj, const TCHAR *log_source)
	{
		check( *log_obj.get_context() );
		++num_messages_logged_;
	}

	int num_messages_logged_;
	int num_exceptions_logged_;
	bool wrong_message_;
};

template< typename CharType >
void test_enforce()
{
	const CharType *null_str = NULL;
	ARENFORCE_MSG( null_str != NULL, "Value of this pointer is: %1%" )( null_str );
}

template< typename CharType >
void test_log()
{
	const CharType *null_str = NULL;
	ARLOG_INFO( "Value of this pointer is: %1%" )( null_str );
}

//J#AMF-1809: Segfault when logging NULL char pointer
BOOST_AUTO_TEST_CASE( test_jira_amf_1809 )
{
	const log_level::severity current_log_level = the_log_handler::instance().get_global_log_level();
	if( current_log_level < log_level::info )
		the_log_handler::instance().set_global_log_level( log_level::info );
	
	boost::shared_ptr< testing_logtarget > my_target( new testing_logtarget() );
	the_log_handler::instance().add_target( my_target );

	test_log< char >();
	BOOST_CHECK_EQUAL( my_target->num_messages_logged_, 1 );
	BOOST_CHECK( !my_target->wrong_message_ );
	test_log< wchar_t >();
	BOOST_CHECK_EQUAL( my_target->num_messages_logged_, 2 );
	BOOST_CHECK( !my_target->wrong_message_ );

	BOOST_CHECK_THROW( test_enforce< char >(), base_exception );
	BOOST_CHECK_EQUAL( my_target->num_exceptions_logged_, 1 );
	BOOST_CHECK( !my_target->wrong_message_ );
	BOOST_CHECK_THROW( test_enforce< wchar_t >(), base_exception );
	BOOST_CHECK_EQUAL( my_target->num_exceptions_logged_, 2 );
	BOOST_CHECK( !my_target->wrong_message_ );

	the_log_handler::instance().remove_target( my_target );
	the_log_handler::instance().set_global_log_level( current_log_level );
}

