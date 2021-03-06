
#ifndef _CORE_ENUMS_H_
#define _CORE_ENUMS_H_

#include "./minimal_string_defines.hpp"
#include "./export_defines.hpp"

namespace olib
{
	namespace opencorelib
	{
		namespace type_hint
		{
			enum type
			{
				th_int,
				th_float,
				th_double,
				th_string,
				th_bool,
				th_any,
				th_unknown
			};

			CORE_API const TCHAR* to_string( type it );
			CORE_API type to_type( const t_string& str );
		}


		namespace numeric_type_hint
		{
			enum type
			{
				nth_int,
				nth_float,
				nth_double,
				nth_unknown
			};

			CORE_API const TCHAR* to_string( type it );
			CORE_API type to_type( const t_string& str );
		}

        namespace thread_sleep
        {
            enum result
            {
                interrupted, 
                slept_full_time
            };
        }

        namespace thread_sleep_activity
        {
            enum type
            {
                block, 
                handle_incomming_messages
            };
        }        
	}
}


#endif // _CORE_ENUMS_H
