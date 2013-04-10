#include "precompiled_headers.hpp"
#include <boost/test/auto_unit_test.hpp>

#include "opencorelib/cl/worker.hpp"
#include "opencorelib/cl/utilities.hpp"
#include "opencorelib/cl/jobbase.hpp"
#include "opencorelib/cl/thread_pool.hpp"

#include <boost/thread.hpp>

using namespace boost;
using namespace olib;
using namespace olib::opencorelib;
using namespace olib::opencorelib::utilities;

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>

using namespace boost::posix_time;

class MyJobClass : public base_job
{
public:
	MyJobClass( long nrOfMSToRun, boost::uint32_t *job_counter )
		: m_NrOfMSToRun(nrOfMSToRun)
		, job_counter_( job_counter )
	{
	}
    
    int do_work()
    {
        //boost::recursive_mutex::scoped_lock lock( m_Mtx );
        //std::cout << "Running job " << m_dwNumber << std::endl;
        for(int i = 0; i < (m_NrOfMSToRun / 50) ; ++i )
        {   
            boost::thread::sleep( boost::get_system_time() + boost::posix_time::milliseconds(50) );
			
            if( get_should_terminate_job() )
            {
                boost::recursive_mutex::scoped_lock lock( m_Mtx );
                // std::cout << "Terminating job " << m_dwNumber << "...\n";
                return -100;
            }
        }

		boost::recursive_mutex::scoped_lock lock( m_Mtx );
		if( job_counter_ )
			++(*job_counter_);

        return 1;
    }
private:
    long m_NrOfMSToRun;
	boost::uint32_t *job_counter_;
    static boost::recursive_mutex m_Mtx;
};

class MyErrorJob : public base_job
{
public:
    int do_work()
    {
        ARENFORCE(false).msg("The work function crashed");
        //throw std::exception("The work function crashed");
        return 1;
    }
};


boost::recursive_mutex MyJobClass::m_Mtx;

BOOST_AUTO_TEST_SUITE( test_worker )

BOOST_AUTO_TEST_CASE( test_single_job )
{
    worker aWorker;
    boost::shared_ptr< MyJobClass > aJob( new MyJobClass(500, NULL) );
    
    aWorker.start();
    aWorker.add_job(aJob);

    BOOST_CHECK(aJob->wait_for_job_done( boost::posix_time::seconds(1)));
    aWorker.stop( boost::posix_time::seconds(1) );
}

BOOST_AUTO_TEST_CASE( test_multiple_jobs )
{
    worker aWorker;

    aWorker.start();
    olib::opencorelib::utilities::timer aTimer;

    for(int i = 0; i < 10; ++i)
    {
        boost::shared_ptr< MyJobClass > aJob( new MyJobClass(200, NULL) );
        aWorker.add_job(aJob);
    }

    // Starting to wait for completion 
    BOOST_CHECK( aWorker.wait_for_all_jobs_completed(boost::posix_time::seconds(5)) );
    BOOST_CHECK( aTimer.elapsed() < boost::posix_time::seconds(5) );
}

BOOST_AUTO_TEST_CASE( test_terminate_worker )
{
    worker aWorker;
    aWorker.start();

    boost::shared_ptr< MyJobClass > aJob( new MyJobClass(20000, NULL) );
    aWorker.add_job(aJob);
	
	// Sleep so that the job has time to start
    boost::thread::sleep( boost::get_system_time() + boost::posix_time::milliseconds(50));
	
    // Cancelling the job prematurely. Accept 500 ms delay
    olib::opencorelib::utilities::timer aTimer;
    BOOST_CHECK( aWorker.cancel_current_job( boost::posix_time::milliseconds(500) ) );
    BOOST_CHECK( aTimer.elapsed() < boost::posix_time::milliseconds(500) );

    // If thread terminated, the return value is -100.
    BOOST_CHECK( aJob->get_result() == -100 );
}

BOOST_AUTO_TEST_CASE( test_worker_error )
{
    worker aWorker;
    aWorker.start();
    boost::shared_ptr< MyErrorJob > aJob( new MyErrorJob() );
    aWorker.add_job(aJob);
    BOOST_CHECK( aJob->wait_for_job_done( boost::posix_time::seconds(1) ) );
    BOOST_CHECK( aJob->get_result() == -1 );
    BOOST_CHECK( aJob->get_base_exception() );

    if( aJob->get_base_exception() )
    {
        BOOST_CHECK( aJob->get_base_exception()->context()
            .message().compare(_CT("The work function crashed")) == 0 );
    }
}

BOOST_AUTO_TEST_CASE( test_thread_pool )
{
	const boost::uint32_t num_workers = 10;
	const boost::uint32_t num_jobs_to_run = 50;
	const boost::uint32_t job_duration = 200;
	const boost::uint32_t time_to_wait = 2 * num_jobs_to_run / num_workers * job_duration / 1000;
	std::cout << "Will wait max " << time_to_wait << " seconds for thread pool to complete." << std::endl;

    thread_pool myPool(num_workers, boost::posix_time::seconds(5) );

	
	//Check that the correct number of jobs were actually run
	boost::uint32_t job_counter = 0;
	

    for(boost::uint32_t i = 0; i < num_jobs_to_run; ++i)
    {
        boost::shared_ptr< MyJobClass > aJob( new MyJobClass(job_duration, &job_counter) );
        myPool.add_job(aJob);
    }

    // Starting to wait for 50 jobs (á 200 ms) to terminate (max 15 sec)
    olib::opencorelib::utilities::timer aTimer;
    BOOST_CHECK( myPool.wait_for_all_jobs_completed( boost::posix_time::seconds(time_to_wait) ) );
    BOOST_CHECK( aTimer.elapsed() < boost::posix_time::seconds(time_to_wait) );
    BOOST_CHECK_EQUAL( job_counter, num_jobs_to_run );
}

class my_reoccurring_job : public base_job
{
public:
    my_reoccurring_job( const std::string& outp, int& counter ) : m_to_output(outp), m_counter(counter) {}
    int do_work()
    {
        std::cerr << m_to_output.c_str() << " " << boost::posix_time::to_simple_string( boost::get_system_time()) << std::endl;
        m_counter += 1;
        return 1;
    }
private:
    std::string m_to_output;
    int& m_counter;
};

BOOST_AUTO_TEST_CASE( test_reoccurring_job )
{
    worker aWorker;
    aWorker.start();
    int reoccur_counter(0), dummy(0);
    boost::shared_ptr< my_reoccurring_job > aJob( new my_reoccurring_job("reoccurring every second", reoccur_counter) );
    boost::shared_ptr< my_reoccurring_job > another_job( new my_reoccurring_job("occurs once, after 2,5 secs", dummy) );
    aWorker.add_reoccurring_job(aJob, boost::posix_time::seconds(1));
    aWorker.add_job(another_job, boost::posix_time::milliseconds(2500));
    // Sleep on the main thread.
    olib::opencorelib::thread_sleeper sleeper;

    /*boost::recursive_mutex m_mtx;
    boost::condition_variable_any m_wait_condition;

    boost::recursive_mutex::scoped_lock lock( m_mtx );
    bool res = m_wait_condition.timed_wait(lock, boost::get_system_time() + boost::posix_time::seconds(10));*/

    sleeper.current_thread_sleep( boost::posix_time::seconds(10) );
	// T_COUT << _CT("reoccur_counter=") << reoccur_counter << std::endl;
    BOOST_CHECK( reoccur_counter >= 9 );
    BOOST_CHECK_EQUAL( 1, dummy );
}

BOOST_AUTO_TEST_SUITE_END()

