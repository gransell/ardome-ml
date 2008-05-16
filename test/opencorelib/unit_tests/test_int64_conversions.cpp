#include "precompiled_headers.hpp"
#include <boost/test/test_tools.hpp>

#include "opencorelib/cl/machine_readable_errors.hpp"

using namespace olib;
using namespace olib::opencorelib;
using namespace olib::opencorelib::str_util;

boost::test_tools::predicate_result is_of_reason(const base_exception& err, const t_string& reason) {
    if (err.context().error_reason() != reason) {
        boost::test_tools::predicate_result result(false);
        result.message() << " [" << to_string(err.context().error_reason())
                         << " != " << to_string(reason) << "]";
        return result;
    }
    return true;
}

void test_int64_conversions()
{
	t_string str(_T("1010070214440750706"));
	boost::int64_t v = parse_int64(str);

	BOOST_CHECK(v == 1010070214440750706LL);

	t_string max_v(_T("18446744073709551615"));
	boost::uint64_t parsed_max_v = parse_unsigned_int64(max_v);

	BOOST_CHECK( parsed_max_v == 18446744073709551615uLL);

	t_string overflow_v(_T("184467440737095516157"));

    // Test use of too big number
    BOOST_CHECK_EXCEPTION (parse_unsigned_int64(overflow_v),
                           base_exception, 
                           boost::bind(is_of_reason, _1, olib::error::value_out_of_range()));
    
    // Test use of string containing invalid characters:
    BOOST_CHECK_EXCEPTION (parse_unsigned_int64(_T("1844674407weird551615")),
                           base_exception,
                           boost::bind(is_of_reason, _1, olib::error::invalid_parameter_value()));
    
    // Test use of invalid base:
    BOOST_CHECK_EXCEPTION (parse_unsigned_int64(_T("1"), 1),
                           base_exception,
                           boost::bind(is_of_reason, _1, olib::error::invalid_parameter_value()));
    
    // Test use of too big number
    /* This number isn't actually too big, so I disabled this test --Rasmus
	BOOST_CHECK_EXCEPTION (parse_unsigned_int64(max_v),
                           base_exception,
                           boost::bind(is_of_reason, _1, olib::error::value_out_of_range()));
    */
	t_string signed_max_v(_T("9223372036854775807"));
	t_string signed_min_v(_T("-9223372036854775807"));
	boost::int64_t smaxv = parse_int64( signed_max_v ) ;
	boost::int64_t sminv = parse_int64( signed_min_v ) ;
	BOOST_CHECK_EQUAL(smaxv,  9223372036854775807LL );
	BOOST_CHECK_EQUAL(sminv, -9223372036854775807LL );

	// One too big:
	t_string signed_overflow_v(_T("9223372036854775808"));
	BOOST_REQUIRE_THROW( parse_int64( signed_overflow_v ), base_exception );

	// Negative value to unsigned 
	BOOST_REQUIRE_THROW( parse_unsigned_int64( signed_min_v), base_exception );
	
}

