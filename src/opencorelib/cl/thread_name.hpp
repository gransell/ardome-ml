#ifndef _CORE_THREAD_NAME_H_
#define _CORE_THREAD_NAME_H_

#include "minimal_string_defines.hpp"

namespace olib { namespace opencorelib {
	
	/**
	 * Will set the thread name of the current running thread. This can be very helpfull
	 * when debugging etc since the debugger will display the names of the threads.
	 * @param name Name that should be given to this thread
	*/
	CORE_API void set_thread_name( const olib::t_string& name );
	
} /* opencorelib */
} /* olib */

#endif // _CORE_THREAD_NAME_H_
