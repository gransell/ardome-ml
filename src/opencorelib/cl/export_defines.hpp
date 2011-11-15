#ifndef _CORE_EXPORT_DEFINES_H_
#define _CORE_EXPORT_DEFINES_H_

/** @file core/export_defines.h This file defines the
    CORE_EXPORTS macro, needed to export classes from a
    dll on windows. Other platforms defines this macro to 
    be empty. */

#include "./platform.hpp"
#include "./platform_defines.hpp"
#include "./disabled_warnings.hpp"

#ifdef OLIB_ON_WINDOWS
    // The OLIB_DLL_EXPORT macro should only be used in plugins, since it always exports symbols.
    #define OLIB_DLL_EXPORT __declspec( dllexport )
    #if defined CORE_EXPORTS && defined(DYNAMIC_BUILD)
        #define CORE_API __declspec(dllexport)

        #pragma message ("Building open_core_library as a dynamic library")
    #elif defined CORE_EXPORTS && defined(STATIC_BUILD)
        #define CORE_API
        #pragma message ("Building open_core_library as a static library")
    #elif defined(DYNAMIC_BUILD)
        #define CORE_API __declspec(dllimport)
    #else
        #define CORE_API
    #endif
#else 
    #define OLIB_DLL_EXPORT
    #define CORE_API
#endif

#if defined (OLIB_COMPILED_WITH_VISUAL_STUDIO) && defined(CORE_EXPORTS)

#pragma warning (disable:4251)  // 4251 class 'xxx' needs to have dll-interface to be 
                                //used by clients of class 'yyyy'
#pragma warning (disable:4127)  // conditional expression is constant
#include <windows.h>
#endif


#endif //_CORE_EXPORT_DEFINES_H_

