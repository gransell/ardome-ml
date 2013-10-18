// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//


#define _WIN32_WINNT 0x0500 // Windows 2000 or later
#ifdef _WIN32_IE
    #undef _WIN32_IE
    #define _WIN32_IE 0x0601 // 6.01 or greater of internet explorer
#endif
#include <windows.h>

/* Disables some unnecessary warnings. */
#include "platform.hpp"
#include "disabled_warnings.hpp"
#include "macro_definitions.hpp"



 
