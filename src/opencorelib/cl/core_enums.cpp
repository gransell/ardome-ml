#include "precompiled_headers.hpp"
#include "./core_enums.hpp"
#include "./enforce_defines.hpp"
#include "./enforce.hpp"

namespace olib
{
	namespace opencorelib
	{
		namespace type_hint
		{
			CORE_API const TCHAR* to_string( type it )
			{
				if( it == th_int ) return _CT("int");
				if( it == th_float) return _CT("float");
				if( it == th_double) return _CT("double");
				if( it == th_string) return _CT("string");
				if( it == th_bool) return _CT("bool");
				if( it == th_any) return _CT("any");
				return _CT("error! Unknown enum value");
			}

			CORE_API type to_type( const t_string& str )
			{
				if( str.compare(_CT("int")) == 0 ) return th_int;
				if( str.compare(_CT("float")) == 0 ) return th_float;
				if( str.compare(_CT("double")) == 0 ) return th_double;
				if( str.compare(_CT("string")) == 0 ) return th_string;
				if( str.compare(_CT("bool")) == 0 ) return th_bool;
				if( str.compare(_CT("any")) == 0 ) return th_any;
				ARENFORCE_MSG( false, "Unknown value, can not convert to enumeration.")(str);
				return th_unknown;
			}
		}


		namespace numeric_type_hint
		{
			CORE_API const TCHAR* to_string( type it )
			{
				if( it == nth_int ) return _CT("int");
				if( it == nth_float) return _CT("float");
				if( it == nth_double) return _CT("double");
				return _CT("error! Unknown enum value");
			}

			CORE_API type to_type( const t_string& str )
			{
				if( str.compare(_CT("int")) == 0 ) return nth_int;
				if( str.compare(_CT("float")) == 0 ) return nth_float;
				if( str.compare(_CT("double")) == 0 ) return nth_double;

				ARENFORCE_MSG( false, "Unknown value, can not convert to enumeration.")(str);
				return nth_unknown;
			}
		}
	}
}
