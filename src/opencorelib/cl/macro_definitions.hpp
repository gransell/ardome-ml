#ifndef _CORE_MACRODEFINITIONS_H_
#define _CORE_MACRODEFINITIONS_H_

/** @file macro_definitions.h Common macros used in amf. */
#include "platform.hpp"

//  helper macros to concatenate two tokens together

/** @def ARCONCAT_HELPER 
    Used to concatenate a string. Don't use this directly, use ARCONCAT instead. */
#define ARCONCAT_HELPER(text, line) text ## line

/** @def ARCONCAT 
    Concatenates two strings during compilation. */
#define ARCONCAT(text, line)        ARCONCAT_HELPER(text, line)

// Unique variable names 

/** @def ARCONCAT_LINE 
        Concatenates a string with a line or unique counter number.
        Used by the ARMAKE_UNIQUE_NAME macro.*/
#if defined(OLIB_COMPILED_WITH_VISUAL_STUDIO) && (OLIB_COMPILED_WITH_VISUAL_STUDIO >= 1300)
    #define ARCONCAT_LINE(text)         ARCONCAT(text, __COUNTER__)
#else 
    #define ARCONCAT_LINE(text)         ARCONCAT(text, __LINE__)
#endif

/** @def ARMAKE_UNIQUE_NAME 
        Creates a unique name from a prefix text string.
        Example: ARMAKE_UNIQUE_NAME(dummy_) will generate a name such as dummy_12 */
#define ARMAKE_UNIQUE_NAME(text)    ARCONCAT_LINE(text)

///  macros to avoid compiler warnings 
/** @def ARUNUSED_ALWAYS 
        Makes the compiler stop complaining about unused 
        variables in functions. This is only needed if compiling with a 
        high warning level (w4 in visual studio). */
#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
    #define ARUNUSED_ALWAYS(x) x
#elif defined __GNUC__
    #define ARUNUSED_ALWAYS(x) au_unused_var(x)
#else
    #define ARUNUSED_ALWAYS(x) x
#endif

/// Used on mac/linux (gcc-compiled) to make the ARUNUSED_ALWAYS macro work.
template <class T> inline void au_unused_var(const T&) { }

#endif /*_CORE_MACRODEFINITIONS_H_*/
