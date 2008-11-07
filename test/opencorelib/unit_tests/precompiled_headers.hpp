// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#include "opencorelib/cl/platform.hpp"
#include "opencorelib/cl/macro_definitions.hpp"

#if defined(AMF_COMPILED_WITH_VISUAL_STUDIO)
#include <tchar.h>
#endif

#include <iostream>

#include <boost/bind.hpp>
#include <boost/ref.hpp>

#include "opencorelib/cl/boost_link_defines.hpp"
#include "opencorelib/cl/core.hpp"
#include "opencorelib/cl/disabled_warnings.hpp"
#include "opencorelib/cl/assert_defines.hpp"
#include "opencorelib/cl/log_defines.hpp"
#include "opencorelib/cl/enforce_defines.hpp"
#include "opencorelib/cl/base_exception.hpp"
#include "opencorelib/cl/logger.hpp"

#ifdef _DEBUG
	#ifdef OLIB_ON_WINDOWS
		#define USEFULUNITTEST_EXE_NAME _T("opencorelib_unit_tests.exe")
	#elif defined __UNIX_LIKE__
		#define USEFULUNITTEST_EXE_NAME _T("opencorelib_unit_tests")
	#endif
#else
	#ifdef OLIB_ON_WINDOWS
		#define USEFULUNITTEST_EXE_NAME _T("opencorelib_unit_tests.exe")
	#elif defined __UNIX_LIKE__
		#define USEFULUNITTEST_EXE_NAME _T("core_unit_tests")
	#endif
#endif


// TODO: reference additional headers your program requires here
