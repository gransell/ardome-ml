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
    timer aTimer;

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
    timer aTimer;
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
    timer aTimer;
    BOOST_CHECK( myPool.wait_for_all_jobs_completed(15000) );
    BOOST_CHECK( aTimer.elapsed_millisecs() < 15000 );
}

void test_worker()
{
    TestWorker();
    TestWorker2();
    TestTerminateWorker();
    TestThreadPool();
    TestWorkerError();
}

