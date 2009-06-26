#ifndef _CORE_PLATFORM_H_
#define _CORE_PLATFORM_H_

/**	@file platform.h 
    File containing macros related to the 
    current platform and compiler. */

/** @def OLIB_ON_MAC
    This symbol is defined when amf is compiling on
    a mac machine. */

/** @def OLIB_ON_LINUX
    This symbol is defined when amf is compiling on
    a linux machine. */

/** @def OLIB_COMPILED_WITH_GCC 
    This symbol is defined if gcc is the
    current compiler of amf. */

/** @def OLIB_ON_WINDOWS
    If this symbol is defined, amf is currently compiling
    on a windows machine. */

/** @def OLIB_COMPILED_WITH_VISUAL_STUDIO
    This symbol is defined to the value of _MSC_VER,
    which is the version number of the visual studio
    compiler. Example value: 1300 */

// Mac, OSX
#ifdef DOXYGEN_PREPROCESSOR_RUNNING
    // Just let doxygen see all defines.
    #define OLIB_ON_MAC
    #define OLIB_ON_LINUX
    #define OLIB_COMPILED_WITH_GCC
    #define OLIB_ON_WINDOWS
    #define OLIB_COMPILED_WITH_VISUAL_STUDIO _MSC_VER
#endif


#ifdef __APPLE__
    #ifndef OLIB_ON_MAC
        #define OLIB_ON_MAC
    #endif
#endif

#ifdef __GNUC__
    #ifndef OLIB_COMPILED_WITH_GCC
        #define OLIB_COMPILED_WITH_GCC
    #endif
#endif

// Windows and Visual Studio
#ifdef WIN32
    #ifndef OLIB_ON_WINDOWS
        #define OLIB_ON_WINDOWS
    #endif
#endif


#if defined(_MSC_VER) 
    #define OLIB_COMPILED_WITH_VISUAL_STUDIO _MSC_VER
#endif 

#endif // _CORE_PLATFORM_H_ 

