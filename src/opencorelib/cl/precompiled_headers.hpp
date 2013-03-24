#ifndef CORESRCPRECOMPHEADERS
#define CORESRCPRECOMPHEADERS

#include "platform.hpp"

#if defined(OLIB_COMPILED_WITH_VISUAL_STUDIO)
    #include "msw/stdafx.hpp"  
#endif

// Defines the CORE_API macro used to export classes.
#include "export_defines.hpp"

#include "macro_definitions.hpp"
#include "platform_defines.hpp"

#include "boost_headers.hpp"

#include "typedefs.hpp"
#include "core_enums.hpp"
#include "assert_defines.hpp"
#include "assert.hpp"
#include "log_defines.hpp"
#include "enforce_defines.hpp"
#include "enforce.hpp"
#include "machine_readable_errors.hpp"
#include "string_defines.hpp"
#include "string_conversions.hpp"
#include "str_util.hpp"

#include <boost/operators.hpp>
#include <boost/scoped_array.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/any.hpp>

#include <vector>
#include <list>

#endif
