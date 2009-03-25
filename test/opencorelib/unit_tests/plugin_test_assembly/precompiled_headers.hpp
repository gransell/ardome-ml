#ifndef _CORE_UNIT_TEST_PLUGIN_TEST_ASSEMBLY_PRECOMPILED_HEADERS_H_
#define _CORE_UNIT_TEST_PLUGIN_TEST_ASSEMBLY_PRECOMPILED_HEADERS_H_

#include "opencorelib/cl/boost_link_defines.hpp"
#include "opencorelib/cl/core.hpp"

#ifdef OLIB_ON_WINDOWS
    #if defined PLUGIN_TEST_ASSEMBLY_EXPORTS 
        #define PLUGIN_API __declspec(dllexport)
    #else
        #define PLUGIN_API __declspec(dllimport)
    #endif
#else 
    #define PLUGIN_API
#endif


#endif // _CORE_UNIT_TEST_PLUGIN_TEST_ASSEMBLY_PRECOMPILED_HEADERS_H_
