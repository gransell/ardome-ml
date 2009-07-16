
/* Define _USE_MATH_DEFINES before including math.h to expose some macro
* definitions for common math constants.  These are placed under an #ifdef
* since these commonly-defined names are not part of the C/C++ standards.*/

#define _USE_MATH_DEFINES // To get M_PI, M_E ... (see math.h)

#include <limits.h>
#include <opencorelib/cl/boost_link_defines.hpp>

#if defined( _MSC_VER )
    #pragma warning( push )
    #pragma warning( disable:4267) 
    #define _WIN32_WINNT 0x0501
#endif

#include "amf_filter_plugin.hpp"
#include "opencorelib/cl/boost_link_defines.hpp"
#include "opencorelib/cl/core.hpp"
#include "opencorelib/cl/assert_defines.hpp"
#include "opencorelib/cl/log_defines.hpp"
#include "opencorelib/cl/enforce_defines.hpp"
#include "opencorelib/cl/base_exception.hpp"
#include "opencorelib/cl/minimal_string_defines.hpp"
#include "opencorelib/cl/str_util.hpp"
#include "opencorelib/cl/enforce.hpp"
#include "opencorelib/cl/utilities.hpp"
#include "opencorelib/cl/guard_define.hpp"
#include "opencorelib/cl/machine_readable_errors.hpp"
