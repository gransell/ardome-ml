#ifndef _CORE_WIN_ENUM_PROCESS_MODULES_H_
#define _CORE_WIN_ENUM_PROCESS_MODULES_H_

#include <vector>
#include "opencorelib/cl/typedefs.hpp"

namespace olib
{
	namespace opencorelib
	{
		namespace win
		{
			std::vector< library_info_ptr > enum_process_modules();
		}
	}
}


#endif // _CORE_WIN_ENUM_PROCESS_MODULES_H_
