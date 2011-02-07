#ifndef _CORE_ASSERT_DEFINES_H_
#define _CORE_ASSERT_DEFINES_H_

#include "platform.hpp"

/// @file Assert_defines.h Defines the ARASSERT macro.
/** This file can not be included before Assert.h.

	@def ARASSERT

	The ARASSERT macro is used like this:
	<pre>
		ARASSERT( val > cutoff, "This a warning %i %i")(val)(cutoff);
	</pre>
	The above will test if i is greater than a. If not
	an assertion failed dialog will displayed with the 
	values of i and a and the warning message.

	Basically the ARASSERT macro creates an CAssert object
	if the assertion failed. 
	@sa CAssert
	@author Mats Lindel&ouml;f*/

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
    // To compile with warning level 4 we need to turn off
    // 4127 : conditional expression is constant
    // ARASSERT will create this warning in release builds.
    // and in debug builds when the always_fail is used.
    // ARASSERT( assert::always_fail() );
    #pragma warning(disable : 4127 )
#endif
	
// The __FUNCDNAME__ parameter is not standard. Define 
// the macro differently for each compiler.

#define CREATE_ASSERT( expr, the_msg ) \
    if( (expr) ); \
    else ++olib::opencorelib::invoke_assert::make_assert().add_context(__FILE__, __LINE__, OLIB_CURRENT_FUNC_NAME, #expr ).msg(the_msg).ARASSERT_A

#ifdef _DEBUG
	#define ARASSERT_MSG(expr, the_msg) CREATE_ASSERT( expr, the_msg )
    #define ARASSERT(expr) CREATE_ASSERT( expr, "")
#else // Release: will be optimized away
	#define ARASSERT_MSG(expr, the_msg) CREATE_ASSERT((true), the_msg)
    #define ARASSERT(expr) CREATE_ASSERT((true), "")
#endif

#define ARASSERT_A(x) ARASSERT_OP(x, B)
#define ARASSERT_B(x) ARASSERT_OP(x, A)

#define ARASSERT_OP(x, next) \
	ARASSERT_A.add_value(#x, (x)).ARASSERT_ ## next \

#include "./assert.hpp"

#endif //_CORE_ASSERT_DEFINES_H_
