#include "precompiled_headers.hpp"
#include <boost/test/test_tools.hpp> 

#include "opencorelib/cl/invoker.hpp"

using namespace boost;
using namespace olib;
using namespace olib::opencorelib;

// TODO Add tests for non_invoker.

void inc(int* n) {
    ++*n;
}

void TestExplicitStepInvoker()
{
    invoker_ptr n_inv( new non_invoker() );
    explicit_step_invoker invoker( n_inv );
    int n_calls = 0;
    invoker.invoke(boost::bind(inc, &n_calls));
    invoker.invoke(boost::bind(inc, &n_calls));
    BOOST_CHECK_EQUAL(0, n_calls);
    invoker.step();
    BOOST_CHECK_EQUAL(2, n_calls);
}

void bad_function() {
    throw std::runtime_error("This invoked function throws");
}

void TestExplicitStepInvokerSafe()
{
    invoker_ptr n_inv( new non_invoker() );
    explicit_step_invoker invoker( n_inv );
    int n_calls = 0;
    invoker.invoke(boost::bind(&inc, &n_calls));
    invoker.invoke(&bad_function);
    invoker.invoke(boost::bind(&inc, &n_calls));
    BOOST_CHECK_EQUAL(0, n_calls);
    invoker.step();
    BOOST_CHECK_EQUAL(2, n_calls);
}

void result_callback( int& to_inc, invoke_result::type res, std_exception_ptr ep )
{
    to_inc++;
}

void test_non_blocking_invoke()
{
     invoker_ptr n_inv( new non_invoker() );
     explicit_step_invoker invoker( n_inv );
     int cb_called = 0;
     int n_calls = 0;
     
     invoke_callback_function_ptr cb( 
         new invoke_callback_function(boost::bind( &result_callback, boost::ref(cb_called), _1, _2 )) );

     invoker.non_blocking_invoke(boost::bind(&inc, &n_calls), cb);
     
     BOOST_CHECK_EQUAL( 0, n_calls);
     invoker.step();
     BOOST_CHECK_EQUAL( 1, n_calls);
     BOOST_CHECK_EQUAL( 1, cb_called);
}

void test_invoker()
{
    TestExplicitStepInvoker();
    TestExplicitStepInvokerSafe();
    test_non_blocking_invoke();
}
