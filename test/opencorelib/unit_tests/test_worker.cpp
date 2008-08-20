#include "precompiled_headers.hpp"

#include "opencorelib/cl/worker.hpp"
#include "opencorelib/cl/utilities.hpp"
#include "opencorelib/cl/jobbase.hpp"
#include "opencorelib/cl/thread_pool.hpp"

#include <boost/test/test_tools.hpp> 

using namespace boost;
using namespace olib;
using namespace olib::opencorelib;
using namespace olib::opencorelib::utilities;

#include <boost/date_time/posix_time/posix_time.hpp>
using namespace boost::posix_time;

class MyJobClass : public base_job
{
public:
    MyJobClass( long nrOfMSToRun ) : m_NrOfMSToRun(nrOfMSToRun) {}
    
    int do_work()
    {
        //boost::recursive_mutex::scoped_lock lock( m_Mtx );
        //std::cout << "Running job " << m_dwNumber << std::endl;
        for(int i = 0; i < (m_NrOfMSToRun / 50) ; ++i )
        {   
			
            boost::thread::sleep(utilities::add_millsecs_from_now(50));
			
            if( get_should_terminate_job() )
            {
                boost::recursive_mutex::scoped_lock lock( m_Mtx );
                // std::cout << "Terminating job " << m_dwNumber << "...\n";
                return -100;
            }
        }

        return 1;
    }
private:
    long m_NrOfMSToRun;
    static boost::recursive_mutex m_Mtx;
};

class MyErrorJob : public base_job
{
public:
    int do_work()
    {
        ARENFORCE(false).msg("The work function crashed");
        return 1;
    }
};


boost::recursive_mutex MyJobClass::m_Mtx;

void TestWorker()
{
    worker aWorker;
    boost::shared_ptr< MyJobClass > aJob( new MyJobClass(500) );
    
    aWorker.start();
    aWorker.add_job(aJob);

    BOOST_CHECK(aJob->wait_for_job_done(1000));
    aWorker.stop(1000);
}

void TestWorker2()
{
    worker aWorker;

    aWorker.start();
    olib::opencorelib::utilities::timer aTimer;

    for(int i = 0; i < 10; ++i)
    {
        boost::shared_ptr< MyJobClass > aJob( new MyJobClass(200) );
        aWorker.add_job(aJob);
    }

    // Starting to wait for completion 
    BOOST_CHECK( aWorker.wait_for_all_jobs_completed(5000) );
    BOOST_CHECK( aTimer.elapsed_millisecs() < 5000 );
}

void TestTerminateWorker()
{
    worker aWorker;
    aWorker.start();

    boost::shared_ptr< MyJobClass > aJob( new MyJobClass(20000) );
    aWorker.add_job(aJob);
	
	// Sleep so that the job has time to start
    boost::thread::sleep(utilities::add_millsecs_from_now(50));
	
    // Cancelling the job prematurely. Accept 500 ms delay
    olib::opencorelib::utilities::timer aTimer;
    BOOST_CHECK( aWorker.cancel_current_job(500) );
    BOOST_CHECK( aTimer.elapsed_millisecs() < 500 );

    // If thread terminated, the return value is -100.
    BOOST_CHECK( aJob->get_result() == -100 );
}

void TestWorkerError()
{
    worker aWorker;
    aWorker.start();
    boost::shared_ptr< MyErrorJob > aJob( new MyErrorJob() );
    aWorker.add_job(aJob);
    BOOST_CHECK( aJob->wait_for_job_done(1000) );
    BOOST_CHECK( aJob->get_result() == -1 );
    BOOST_CHECK( aJob->get_base_exception() );

    if( aJob->get_base_exception() )
    {
        BOOST_CHECK( aJob->get_base_exception()->context()
            .message().compare(_T("The work function crashed")) == 0 );
    }
}

void TestThreadPool()
{
    thread_pool myPool(10, 5000);
    for(int i = 0; i < 50; ++i)
    {
        boost::shared_ptr< MyJobClass > aJob( new MyJobClass(200) );
        myPool.add_job(aJob);
    }

    // Starting to wait for 50 jobs (á 200 ms) to terminate (max 15 sec)
    olib::opencorelib::utilities::timer aTimer;
    BOOST_CHECK( myPool.wait_for_all_jobs_completed(15000) );
    BOOST_CHECK( aTimer.elapsed_millisecs() < 15000 );
}

class my_reoccurring_job : public base_job
{
public:
    my_reoccurring_job( const std::string& outp, int& counter ) : m_to_output(outp), m_counter(counter) {}
    int do_work()
    {
        // T_CERR << m_to_output.c_str() << " " << time_value::now().get_seconds() << std::endl;
        m_counter += 1;
        return 1;
    }
private:
    std::string m_to_output;
    int& m_counter;
};

void test_reoccurring_job()
{
    worker aWorker;
    aWorker.start();
    int reoccur_counter(0), dummy(0);
    boost::shared_ptr< my_reoccurring_job > aJob( new my_reoccurring_job("reoccurring every second", reoccur_counter) );
    boost::shared_ptr< my_reoccurring_job > another_job( new my_reoccurring_job("occurs once, after 2,5 secs", dummy) );
    aWorker.add_reoccurring_job(aJob, time_value(1,0));
    aWorker.add_job(another_job, time_value(2,500000));
    // Sleep on the main thread.
    olib::opencorelib::thread_sleeper sleeper;
    sleeper.current_thread_sleep( time_value(10,0) );
    BOOST_CHECK( reoccur_counter >= 10 );
    BOOST_CHECK_EQUAL( 1, dummy );
}

void test_worker()
{
    TestWorker();
    TestWorker2();
    TestTerminateWorker();
    TestThreadPool();
    TestWorkerError();
    test_reoccurring_job();
}

