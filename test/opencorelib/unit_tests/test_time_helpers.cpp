#include "precompiled_headers.hpp"
#include <boost/test/test_tools.hpp>

#include "opencorelib/cl/time_helpers.hpp"
#include "opencorelib/cl/worker.hpp"
#include "opencorelib/cl/function_job.hpp"

#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>

using namespace olib;
using namespace olib::opencorelib;

void worker_sleep(  thread_sleeper& ts, 
                  const boost::posix_time::time_duration& time_to_sleep,
                    bool& awake )
{
    awake = false;
    ts.current_thread_sleep( time_to_sleep );
    awake = true;
}


/// custom comparison predicate that checks that a time is in range
boost::test_tools::predicate_result
in_range(const boost::posix_time::time_duration& min, const boost::posix_time::time_duration& max, const boost::posix_time::time_duration& actual) {
    using namespace boost::posix_time;
    if (min > actual) {
        boost::test_tools::predicate_result res(false);
        res.message() << " [" << to_simple_string(actual) << " < (" << to_simple_string(min) << "; " << to_simple_string(max) << ")]";
        return res;
    }
    if (max < actual) {
        boost::test_tools::predicate_result res(false);
        res.message() << " [" << to_simple_string(actual) << " > (" << to_simple_string(min) << "; " << to_simple_string(max) << ")]";
        return res;
    }

    return true;
}

namespace {
    // This should probably be 1ms or even less, but allow some for for our overloaded build servers
    // On my machine, the normal case is about 400us, but extrems up to 3.5ms happens -- rasmus
    const boost::posix_time::time_duration epsilon = boost::posix_time::millisec(4);
}

boost::test_tools::predicate_result fast(   const boost::posix_time::time_duration& actual, 
                                            const boost::posix_time::time_duration& limit = epsilon) {
    return in_range( boost::posix_time::seconds(0), limit, actual);
}

void test_full_sleep()
{
    timer a_timer ;
    thread_sleeper worker_thread_sleeper;
    worker a_worker;
    a_worker.start();
    a_timer.start();

    // The job:  Sleep for a quarter of a second
    boost::posix_time::time_duration tv = boost::posix_time::milliseconds(250);
    bool is_awake(false);

    function_job_ptr a_job( new function_job(
        boost::bind(::worker_sleep, boost::ref(worker_thread_sleeper), tv, boost::ref(is_awake) )));

    a_worker.add_job( a_job );
    BOOST_CHECK_MESSAGE(a_worker.wait_for_all_jobs_completed( boost::posix_time::milliseconds(2500) ),
                        "Worker should complete before timeout");
    BOOST_CHECK( is_awake );
    a_timer.stop();

    // Allow some ms for the work administration
    BOOST_CHECK_MESSAGE(in_range(tv, tv + epsilon + epsilon, a_timer.elapsed()),
                        "Elapsed time in allowed range");
}

void test_interrupted_sleep()
{
    timer a_timer, b_timer;
    thread_sleeper worker_thread_sleeper;
    worker a_worker;
    a_worker.start();

    a_timer.start(); 

    // The job: Sleep for two seconds.
    boost::posix_time::time_duration tv = boost::posix_time::seconds(2);
    bool is_awake(false);

    function_job_ptr a_job( new function_job(
        boost::bind(::worker_sleep, boost::ref(worker_thread_sleeper), tv, boost::ref(is_awake) )));

    a_worker.add_job( a_job );

    BOOST_CHECK_MESSAGE(fast(a_timer.elapsed()), "Fast setup");
    
    // Wait for a quarter second, then interrupt the sleeping worker
    boost::posix_time::time_duration intrtime = boost::posix_time::millisec(250);
    b_timer.start();
    thread_sleeper main_thread_sleep;
    main_thread_sleep.current_thread_sleep(intrtime);
    BOOST_CHECK_MESSAGE(in_range(intrtime, intrtime+epsilon, b_timer.elapsed()),
                        "Correct sleep");

    b_timer.start();
    worker_thread_sleeper.wake_sleeping_thread();
    BOOST_CHECK_MESSAGE(fast(b_timer.elapsed()), "Fast wakeup");

    b_timer.start();
    BOOST_CHECK_MESSAGE(a_worker.wait_for_all_jobs_completed( boost::posix_time::millisec(50) ),
                        "Worker should be interrupted before this timeout");
    BOOST_CHECK( is_awake );
    BOOST_CHECK_MESSAGE(fast(b_timer.elapsed()), "Fast check for completeness");
    a_timer.stop();

    BOOST_CHECK_MESSAGE(in_range(intrtime, intrtime + epsilon + epsilon + epsilon, a_timer.elapsed()),
                        "Elapsed Time should be in range");
}

void test_time_helpers()
{
    test_full_sleep();
    test_interrupted_sleep();
}
