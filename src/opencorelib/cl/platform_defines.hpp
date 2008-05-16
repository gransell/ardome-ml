#ifndef _CORE_PLATFORM_DEFINES_H_
#define _CORE_PLATFORM_DEFINES_H_

/** @file platform_defines.h
	This file contains some macros handling a platform independent way to 
    get the function name of the currently compiled function. */

#ifdef DOXYGEN_PREPROCESSOR_RUNNING
    /** @def OLIB_CURRENT_FUNC_NAME
        The name of the currently compiled function.
        This is a compiler specific macro, so wee need 
        some abstraction to hide that. */
    #define OLIB_CURRENT_FUNC_NAME
#endif

#include "platform.hpp"

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
    #define OLIB_CURRENT_FUNC_NAME __FUNCDNAME__
#elif defined OLIB_COMPILED_WITH_GCC
    #define OLIB_CURRENT_FUNC_NAME __FUNCTION__
#else
    #error("Unsupported compiler.")
#endif

#endif // _CORE_PLATFORM_DEFINES_H_

