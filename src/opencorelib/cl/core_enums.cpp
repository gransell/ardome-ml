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
				if( it == th_int ) return _T("int");
				if( it == th_float) return _T("float");
				if( it == th_double) return _T("double");
				if( it == th_string) return _T("string");
				if( it == th_bool) return _T("bool");
				if( it == th_any) return _T("any");
				return _T("error! Unknown enum value");
			}

			CORE_API type to_type( const t_string& str )
			{
				if( str.compare(_T("int")) == 0 ) return th_int;
				if( str.compare(_T("float")) == 0 ) return th_float;
				if( str.compare(_T("double")) == 0 ) return th_double;
				if( str.compare(_T("string")) == 0 ) return th_string;
				if( str.compare(_T("bool")) == 0 ) return th_bool;
				if( str.compare(_T("any")) == 0 ) return th_any;
				ARENFORCE_MSG( false, "Unknown value, can not convert to enumeration.")(str);
				return th_unknown;
			}
		}


		namespace numeric_type_hint
		{
			CORE_API const TCHAR* to_string( type it )
			{
				if( it == nth_int ) return _T("int");
				if( it == nth_float) return _T("float");
				if( it == nth_double) return _T("double");
				return _T("error! Unknown enum value");
			}

			CORE_API type to_type( const t_string& str )
			{
				if( str.compare(_T("int")) == 0 ) return nth_int;
				if( str.compare(_T("float")) == 0 ) return nth_float;
				if( str.compare(_T("double")) == 0 ) return nth_double;

				ARENFORCE_MSG( false, "Unknown value, can not convert to enumeration.")(str);
				return nth_unknown;
			}
		}
	}
}
