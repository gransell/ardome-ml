#include <boost/test/unit_test.hpp>
#include <boost/assign.hpp>
#include <openmedialib/ml/utilities.hpp>
#include <openmedialib/ml/input.hpp>
#include <openmedialib/ml/frame.hpp>
#include <openmedialib/ml/filter.hpp>
#include <opencorelib/cl/str_util.hpp>
#include <opencorelib/cl/jobbase.hpp>
#include <opencorelib/cl/worker.hpp>
#include "mocks/mock_input.hpp"

using namespace olib::openmedialib::ml;
using namespace olib::opencorelib;
using namespace olib::opencorelib::str_util;
using boost::shared_ptr;
typedef shared_ptr< worker > worker_ptr;

//Helper class for running a boost::function as a job in a worker
template< typename T >
class custom_job : public base_job
{
public:
	custom_job( boost::function< T () > func )
		: func_( func )
		, result_()
		, was_run_( false )
	{
	}

	//Override: base_job::do_work
	int do_work()
	{
		result_ = func_();
		was_run_ = true;
		return 0;
	}

	T &get_result()
	{
		BOOST_REQUIRE( was_run_ );
		return result_;
	}

private:
	boost::function< T () > func_;
	T result_;
	bool was_run_;
};

//Specialization of custom_job for functions returning void.
//Has no get_result() method.
template<>
class custom_job< void > : public base_job
{
public:
	custom_job( boost::function< void () > func )
		: func_( func )
	{
	}

	//Override: base_job::do_work
	int do_work()
	{
		func_();
		return 0;
	}

private:
	boost::function< void () > func_;
};

void seek_in_thread( const worker_ptr &worker, const filter_type_ptr &filter, int position )
{
	boost::function< void () > func = boost::bind( &filter_type::seek, filter.get(), position, false );
	base_job_ptr job( new custom_job< void >( func ) );
	worker->add_job( job );
	BOOST_REQUIRE( job->wait_for_job_done( boost::posix_time::seconds( 1 ) ) );
}

frame_type_ptr fetch_in_thread( const worker_ptr &worker, const filter_type_ptr &filter )
{
	boost::function< frame_type_ptr () > func = boost::bind( &filter_type::fetch, filter.get() );
	shared_ptr< custom_job< frame_type_ptr > > job( new custom_job< frame_type_ptr >( func ) );
	worker->add_job( job );
	BOOST_REQUIRE( job->wait_for_job_done( boost::posix_time::seconds( 1 ) ) );
	return job->get_result();
}

int get_filter_position_in_thread( const worker_ptr &worker, const filter_type_ptr &filter )
{
	boost::function< int () > func = boost::bind( &filter_type::get_position, filter.get() );
	shared_ptr< custom_job< int > > job( new custom_job< int >( func ) );
	worker->add_job( job );
	BOOST_REQUIRE( job->wait_for_job_done( boost::posix_time::seconds( 1 ) ) );
	return job->get_result();
}

struct F
{
	F()
	{
		input.reset( new mock_input() );
		lock_filter = create_filter( L"lock" );
		BOOST_REQUIRE( lock_filter );
		lock_filter->connect( input );
	}

	boost::shared_ptr< mock_input > input;
	filter_type_ptr lock_filter;
};

BOOST_AUTO_TEST_SUITE( lock_filter )

BOOST_FIXTURE_TEST_CASE( test_that_seeking_is_thread_specific, F )
{
	input->m_get_frames = 100;
	lock_filter->sync();

	worker_ptr w( new worker() );
	w->start();

	lock_filter->seek( 10 );
	seek_in_thread( w, lock_filter, 20 );

	//Check that the filter reports its position correctly to
	//each thread
	BOOST_REQUIRE_EQUAL( lock_filter->get_position(), 10 );
	BOOST_REQUIRE_EQUAL( get_filter_position_in_thread( w, lock_filter ), 20 );

	//Check that each thread gets the frame it seeked to
	frame_type_ptr my_frame = lock_filter->fetch();
	BOOST_REQUIRE( my_frame );
	BOOST_REQUIRE_EQUAL( my_frame->get_position(), 10 );

	frame_type_ptr other_frame = fetch_in_thread( w, lock_filter );
	BOOST_REQUIRE( other_frame );
	BOOST_REQUIRE_EQUAL( other_frame->get_position(), 20 );
}

BOOST_FIXTURE_TEST_CASE( test_sync_property, F )
{
	input->m_get_frames = 10;
	lock_filter->sync();
	BOOST_REQUIRE_EQUAL( lock_filter->get_frames(), 10 );

	//When the sync property is 0, sync() is a no-op, so
	//we should still have the old duration.
	input->m_get_frames = 20;
	BOOST_REQUIRE_EQUAL( lock_filter->get_frames(), 10 );
	lock_filter->property( "sync" ) = 0;
	BOOST_REQUIRE_EQUAL( lock_filter->get_frames(), 10 );
	lock_filter->sync();
	BOOST_REQUIRE_EQUAL( lock_filter->get_frames(), 10 );

	//With the sync property set to 1, we should get the
	//new duration.
	lock_filter->property( "sync" ) = 1;
	BOOST_REQUIRE_EQUAL( lock_filter->get_frames(), 10 );
	lock_filter->sync();
	BOOST_REQUIRE_EQUAL( lock_filter->get_frames(), 20 );

	input->m_get_frames = 30;
	lock_filter->sync();
	BOOST_REQUIRE_EQUAL( lock_filter->get_frames(), 30 );
}

BOOST_FIXTURE_TEST_CASE( test_image_property, F )
{
	input->m_get_frames = 10;
	lock_filter->sync();

	{
		lock_filter->property( "image" ) = 0;
		frame_type_ptr frame = lock_filter->fetch();
		mock_frame *mf = dynamic_cast< mock_frame * >( frame.get() );
		BOOST_REQUIRE( mf );
		BOOST_CHECK_EQUAL( mf->m_state_num_get_image_calls, 0 );
	}

	{
		//When the image property is set, the lock filter should
		//call get_image() to evaluate the image on the frame.
		lock_filter->property( "image" ) = 1;
		frame_type_ptr frame = lock_filter->fetch();
		mock_frame *mf = dynamic_cast< mock_frame * >( frame.get() );
		BOOST_REQUIRE( mf );
		BOOST_CHECK_EQUAL( mf->m_state_num_get_image_calls, 1 );
	}
}

BOOST_FIXTURE_TEST_CASE( test_queue_property, F )
{
	input->m_get_frames = 10;
	lock_filter->sync();
	BOOST_REQUIRE_EQUAL( input->m_num_fetch_calls, 0 );

	lock_filter->property( "queue" ) = 5;
	for( int i = 0; i < 5; ++i )
	{
		lock_filter->seek( i );
		lock_filter->fetch();
	}
	BOOST_REQUIRE_EQUAL( input->m_num_fetch_calls, 5 );

	//Re-fetching within the queued frames will not
	//trigger new fetches on the input.
	for( int i = 0; i < 5; ++i )
	{
		lock_filter->seek( i );
		lock_filter->fetch();
	}
	BOOST_REQUIRE_EQUAL( input->m_num_fetch_calls, 5 );

	//Fetching outside the queued frames will invalidate
	//the least recently used frame (0 in this case)
	lock_filter->seek( 5 );
	lock_filter->fetch();
	BOOST_REQUIRE_EQUAL( input->m_num_fetch_calls, 6 );

	for( int i = 1; i < 6; ++i )
	{
		lock_filter->seek( i );
		lock_filter->fetch();
	}
	BOOST_REQUIRE_EQUAL( input->m_num_fetch_calls, 6 );

	lock_filter->seek( 0 );
	lock_filter->fetch();
	BOOST_REQUIRE_EQUAL( input->m_num_fetch_calls, 7 );
}

BOOST_AUTO_TEST_SUITE_END()
