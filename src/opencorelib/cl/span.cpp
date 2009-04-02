#include "precompiled_headers.hpp"
#include "./span.hpp"
#include "./enforce_defines.hpp"
#include "./enforce.hpp"

namespace olib
{
   namespace opencorelib
    {
        namespace side
        {
            CORE_API const TCHAR* to_string( type ta )
            {
                if( ta == left ) return _CT("left");
                else if( ta == right) return _CT("right");
                ARENFORCE_MSG(false, "Unknown enum value %i")(ta);
                return _CT("error: unknown enum type!");
            }

            CORE_API type to_type( const t_string& str )
            {
                if( str.compare(_CT("left")) == 0 ) return left;
                else if( str.compare(_CT("right")) == 0 ) return right;
                ARENFORCE_MSG(false, "Can not convert %s to side::type.")(str);
                return unknown;
            }
        }
    }
}
