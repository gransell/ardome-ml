#include "precompiled_headers.hpp"
#include <boost/test/test_tools.hpp>

#include "opencorelib/cl/time_helpers.hpp"
#include "opencorelib/cl/worker.hpp"
#include "opencorelib/cl/function_job.hpp"

#include <boost/bind.hpp>
#include <boost/ref.hpp>

using namespace olib;
using namespace olib::opencorelib;

void test_time_value_arithmetic()
{
    time_value a1(7, 800000), b1(0, 300000), c1(8, 100000);
    BOOST_CHECK_EQUAL(c1, a1 + b1);
    BOOST_CHECK_EQUAL(a1, c1 - b1);
    BOOST_CHECK_EQUAL(b1, c1 - a1);
    BOOST_CHECK_EQUAL(time_value()-a1, b1 - c1);
    BOOST_CHECK_EQUAL(time_value()-b1, a1 - c1);

    time_value a2(7, 800000), b2(0, 199999), c2(7, 999999);
    BOOST_CHECK_EQUAL(c2, a2 + b2);
    BOOST_CHECK_EQUAL(a2, c2 - b2);
    BOOST_CHECK_EQUAL(b2, c2 - a2);
    BOOST_CHECK_EQUAL(time_value()-a2, b2 - c2);
    BOOST_CHECK_EQUAL(time_value()-b2, a2 - c2);
}

void worker_sleep(  thread_sleeper& ts, 
                    const time_value& time_to_sleep,
                    bool& awake )
{
    awake = false;
    ts.current_thread_sleep( time_to_sleep );
    awake = true;
}


/// custom comparison predicate that checks that a time is in range
boost::test_tools::predicate_result
in_range(const time_value& min, const time_value& max, const time_value& actual) {
    if (min > actual) {
        boost::test_tools::predicate_result res(false);
        res.message() << " [" << actual << " < (" << min << "; " << max << ")]";
        return res;
    }
    if (max < actual) {
        boost::test_tools::predicate_result res(false);
        res.message() << " [" << actual << " > (" << min << "; " << max << ")]";
        return res;
    }

    return true;
}

namespace {
    // This should probably be 1ms or even less, but allow some for for our overloaded build servers
    // On my machine, the normal case is about 400us, but extrems up to 3.5ms happens -- rasmus
    const time_value epsilon(0, 40000);
}

boost::test_tools::predicate_result fast(const time_value& actual, const time_value& limit = epsilon) {
    return in_range(time_value(), limit, actual);
}

void test_full_sleep()
{
    timer a_timer ;
    thread_sleeper worker_thread_sleeper;
    worker a_worker;
    a_worker.start();
    a_timer.start();

    // The job:  Sleep for a quarter of a second
    time_value tv(0,250000);
    bool is_awake(false);

    function_job_ptr a_job( new function_job(
        boost::bind(::worker_sleep, boost::ref(worker_thread_sleeper), tv, boost::ref(is_awake) )));

    a_worker.add_job( a_job );
    BOOST_CHECK_MESSAGE(a_worker.wait_for_all_jobs_completed( 2500 ),
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
    time_value tv(2,0);
    bool is_awake(false);

    function_job_ptr a_job( new function_job(
        boost::bind(::worker_sleep, boost::ref(worker_thread_sleeper), tv, boost::ref(is_awake) )));

    a_worker.add_job( a_job );

    BOOST_CHECK_MESSAGE(fast(a_timer.elapsed()), "Fast setup");
    
    // Wait for a quarter second, then interrupt the sleeping worker
    time_value intrtime(0, 250000);
    b_timer.start();
    thread_sleeper main_thread_sleep;
    main_thread_sleep.current_thread_sleep(intrtime);
    BOOST_CHECK_MESSAGE(in_range(intrtime, intrtime+epsilon, b_timer.elapsed()),
                        "Correct sleep");

    b_timer.start();
    worker_thread_sleeper.wake_sleeping_thread();
    BOOST_CHECK_MESSAGE(fast(b_timer.elapsed()), "Fast wakeup");

    b_timer.start();
    BOOST_CHECK_MESSAGE(a_worker.wait_for_all_jobs_completed( 50 ),
                        "Worker should be interrupted before this timeout");
    BOOST_CHECK( is_awake );
    BOOST_CHECK_MESSAGE(fast(b_timer.elapsed()), "Fast check for completeness");
    a_timer.stop();

    BOOST_CHECK_MESSAGE(in_range(intrtime, intrtime + epsilon + epsilon + epsilon, a_timer.elapsed()),
                        "Elapsed Time should be in range");
}

void test_time_helpers()
{
    test_time_value_arithmetic();
    test_full_sleep();
    test_interrupted_sleep();
}
