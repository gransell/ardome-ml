#ifndef _CORE_CORE_H_
#define _CORE_CORE_H_

    #include "export_defines.hpp"

    // Don't allow 1.33.1 style usage of boost::filesystem
    #define BOOST_FILESYSTEM_NO_DEPRECATED 1


    #ifndef CORE_EXPORTS
        #ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
            #pragma message ("Linking with open_core_library.lib")
            #pragma comment (lib, "open_core_library.lib")
        #endif

    #endif


#endif // _CORE_CORE_H_
