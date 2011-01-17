#ifndef _CORE_ENFORCE_DEFINES_H_
#define _CORE_ENFORCE_DEFINES_H_

#include "./utilities.hpp"
#include "./exception_context.hpp"

/// @file Enforce_defines.h
/**	This file can not be included before Enforce.h
	If the enforcement fails, a CBase_exception is thrown. */
	#define CREATE_ARENFORCE( expr, the_msg ) \
		if( (expr) ); \
        else ++olib::opencorelib::make_enforcer().add_context(__FILE__, __LINE__, OLIB_CURRENT_FUNC_NAME, #expr ) \
                 .msg((the_msg)).ARENFORCE_A

    #define CREATE_ARENFORCE_ERR_REASON( expr, the_msg, error_reason ) \
        if( (expr) ); \
        else ++olib::opencorelib::make_enforcer().add_context(__FILE__, __LINE__, OLIB_CURRENT_FUNC_NAME, #expr ) \
                 .msg((the_msg)).reason((error_reason)).ARENFORCE_A


	/** @def ARENFORCE_MSG
		Tests a given expression if it is non-zero. If it is zero (false), 
		a base_exception is thrown. 

		The syntax is a bit odd but very convenient. After the ending parentheses 
		of the macro you can add arbitrary numbers of new parentheses. Inside each
		of these you can place a new expression (a varaible is an expression) that 
		will be stored as name-value pairs in the thrown exception. The passed values
		can give invaluable information on why the error occurred. 
		Usage:
		<pre>
			ARENFORCE_MSG( p > 0, "Unexpected value %8X")(p);
		</pre> 
		@author Mats Lindel&ouml;f*/
	#define ARENFORCE_MSG(expr, msg) CREATE_ARENFORCE((expr),(msg))

    /** @def ARENFORCE_MSG_ERR 
     Tests a given expression if it is non-zero. If it is zero (false), 
     a base_exception is thrown. 
     
     The syntax is a bit odd but very convenient. After the ending parentheses 
     of the macro you can add arbitrary numbers of new parentheses. Inside each
     of these you can place a new expression (a varaible is an expression) that 
     will be stored as name-value pairs in the thrown exception. The passed values
     can give invaluable information on why the error occurred. 
     Usage:
     <pre>
     ARENFORCE_MSG_ERR( p > 0, "Unexpected value %8X", olib::error::null_pointer())(p);
     </pre> 
     @author Mikael Gransell*/
    #define ARENFORCE_MSG_ERR(expr, msg, error_reason)          \
        CREATE_ARENFORCE_ERR_REASON((expr),(msg), (error_reason))

    /** @def ARENFORCE_ERR Same as ARENFORCE_MSG_ERR, but no message needed. */
    #define ARENFORCE_ERR(expr, error_reason) CREATE_ARENFORCE_ERR_REASON((expr),"", (error_reason))

    /** @def ARENFORCE_ Same as ARENFORCE, but no message needed. */
    #define ARENFORCE(expr) CREATE_ARENFORCE((expr),(""))

	/** @def ARENFORCE_HR_MSG 
		Works very similar to ARENFORCE except that the expression is run 
		through the SUCCEEDED macro. 
		@author Mats Lindel&ouml;f*/
	#define ARENFORCE_HR_MSG(expr, the_msg) CREATE_ARENFORCE_HR((expr),(the_msg))

    /** @def ARENFORCE_HR Same as ARENFORCE_HR_MSG, but no message needed. */
    #define ARENFORCE_HR(expr) CREATE_ARENFORCE_HR((expr),(""))

	/** @def ARENFORCE_WIN_MSG
		Works very similar to ARENFORCE except that Set_last_error is called
		with TRUE and if the tested expression is false, the HResult member
		of the exception_context (stored within the CBase_exception) is set 
		to Get_last_error. This is very useful since the value will be translated
		into a string by the print-functions of CBase_exception. 
		@author Mats Lindel&ouml;f*/
	#define ARENFORCE_WIN_MSG( expr, the_msg ) CREATE_ARENFORCE_WINAPI((expr),(the_msg))

    /** @def ARENFORCE_WIN Same as ARENFORCE_WIN_MSG, but no message needed. */
    #define ARENFORCE_WIN(expr) CREATE_ARENFORCE_WINAPI((expr),(""))

	// Makes the preprocessor recursion work (so we can add arbitrary numbers of 
	// parenthesis after the ARENFORCE macros.
	#define ARENFORCE_A(x) ARENFORCE_OP(x, B)
	#define ARENFORCE_B(x) ARENFORCE_OP(x, A)

	#define ARENFORCE_OP(x, next) \
		ARENFORCE_A.add_value(#x, (x)).ARENFORCE_ ## next \
		/**/
		
		
#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO

	#define CREATE_ARENFORCE_HR( expr, the_msg ) \
		if( SUCCEEDED(expr) ); \
		else olib::opencorelib::make_enforcer().add_context(__FILE__, __LINE__, __FUNCDNAME__, #expr ) \
                 .hr(expr).msg((the_msg)).ARENFORCE_A

	#define CREATE_ARENFORCE_WINAPI( expr, the_msg ) \
		if( (expr) ); \
		else olib::opencorelib::make_enforcer().add_context(__FILE__, __LINE__, __FUNCDNAME__, #expr ) \
			.hr( olib::opencorelib::utilities::get_last_error_from_os() ).msg((the_msg)).ARENFORCE_A

#endif //OLIB_COMPILED_WITH_VISUAL_STUDIO

#include "./enforce.hpp"

#endif //_CORE_ENFORCE_DEFINES_H_
