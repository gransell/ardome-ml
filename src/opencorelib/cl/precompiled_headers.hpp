#ifndef CORESRCPRECOMPHEADERS
#define CORESRCPRECOMPHEADERS

#include "platform.hpp"
#include "boost_headers.hpp"

#if defined(OLIB_COMPILED_WITH_VISUAL_STUDIO)
    #include "msw/stdafx.hpp"  
#endif

// Defines the CORE_API macro used to export classes.
#include "export_defines.hpp"

#include "macro_definitions.hpp"
#include "platform_defines.hpp"


/*	Defines t_ types that abstracts if the underlying string is 
in unicode or ansi. */
#include "string_defines.hpp"


#endif
