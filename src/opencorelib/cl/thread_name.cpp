
#include "precompiled_headers.hpp"
#include "thread_name.hpp"
#include "str_util.hpp"

#if defined( OLIB_ON_MAC )		
#include <pthread.h>
#include <AvailabilityMacros.h>

#elif defined( OLIB_ON_LINUX )
#include <sys/prctl.h>
#endif

namespace olib { namespace opencorelib {
	
	namespace {
		#if defined( OLIB_ON_MAC )
		void set_thread_name_internal( const char *name )
		{
            // This function is only available on 10.6 and above
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
			pthread_setname_np( name );
#endif
		}
		
		#elif defined( OLIB_ON_WINDOWS )
		typedef struct tagTHREADNAME_INFO
		{
		   DWORD dwType; // must be 0x1000
		   LPCSTR szName; // pointer to name (in user addr space)
		   DWORD dwThreadID; // thread ID (-1=caller thread)
		   DWORD dwFlags; // reserved for future use, must be zero
		} THREADNAME_INFO;

		void set_thread_name_internal( const char *name )
		{
			THREADNAME_INFO info;
			info.dwType = 0x1000;
			info.szName = name;
			info.dwThreadID = -1;
			info.dwFlags = 0;

			__try
			{
				RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (DWORD*)&info );
			}
			__except(EXCEPTION_CONTINUE_EXECUTION)
			{
			}
		}

		#elif defined( OLIB_ON_LINUX )
		void set_thread_name_internal( const char *name )
		{
			prctl( PR_SET_NAME, name, 0, 0, 0 );
		}
		#endif
	}
	
	CORE_API void set_thread_name( const olib::t_string& name )
	{
		std::string utf8_name = str_util::to_string( name );
		set_thread_name_internal( utf8_name.c_str( ) );
	}
	
} /* opencorelib */
} /* olib */
