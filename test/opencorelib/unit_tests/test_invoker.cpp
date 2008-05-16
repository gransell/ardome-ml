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
    explicit_step_invoker invoker;
    int n_calls = 0;
    invoker.invoke(bind(inc, &n_calls));
    invoker.invoke(bind(inc, &n_calls));
    BOOST_CHECK_EQUAL(0, n_calls);
    invoker.step();
    BOOST_CHECK_EQUAL(2, n_calls);
}

void bad_function() {
    throw std::runtime_error("This invoked function throws");
}

void TestExplicitStepInvokerSafe()
{
    explicit_step_invoker invoker;
    int n_calls = 0;
    invoker.invoke(bind(&inc, &n_calls));
    invoker.invoke(&bad_function);
    invoker.invoke(bind(&inc, &n_calls));
    BOOST_CHECK_EQUAL(0, n_calls);
    invoker.step();
    BOOST_CHECK_EQUAL(2, n_calls);
}

void test_invoker()
{
    TestExplicitStepInvoker();
    TestExplicitStepInvokerSafe();
}
