#include "precompiled_headers.hpp"
#include "./basic_enums.hpp"
#include "./str_util.hpp"

namespace olib
{
   namespace opencorelib
    {
        using namespace str_util;
        
        namespace assert_level
        {
            CORE_API t_string to_string(severity lvl)
            {
                if(lvl == debug ) return _T("debug");
                if(lvl == warning ) return _T("warning");
                if(lvl == error ) return _T("error");
                if(lvl == fatal ) return _T("fatal");
                return _T("undefined");
            }

            CORE_API severity from_string(const TCHAR* str_lvl)
            {
                t_string s1(str_lvl);
                if(compare_nocase(s1, t_string(_T("debug"))) == 0 ) return debug;
                if(compare_nocase(s1, t_string(_T("warning"))) == 0 ) return warning;
                if(compare_nocase(s1, t_string(_T("error"))) == 0 ) return error;
                if(compare_nocase(s1, t_string(_T("fatal"))) == 0 ) return fatal;
                return undefined;
            }
        }

        namespace log_level
        {
            CORE_API t_string to_string(const severity lvl)
            {
                if(lvl == emergency) return _T("emergency");
                if(lvl == alert) return _T("alert");
                if(lvl == critical) return _T("critical");
                if(lvl == error) return _T("error");
                if(lvl == warning) return _T("warning");
                if(lvl == notice) return _T("notice");
                if(lvl == info) return _T("info");
                if(lvl == debug1) return _T("debug1");
                if(lvl == debug2) return _T("debug2");
                if(lvl == debug3) return _T("debug3");
                if(lvl == debug4) return _T("debug4");
                if(lvl == debug5) return _T("debug5");
                if(lvl == debug6) return _T("debug6");
                if(lvl == debug7) return _T("debug7");
                if(lvl == debug8) return _T("debug8");
                if(lvl == debug9) return _T("debug9");
                return _T("unknown");
            }
 
            CORE_API severity from_string(const TCHAR* str_lvl)
            {
                t_string s1(str_lvl);
                if(compare_nocase(s1, t_string(_T("emergency"))) == 0 ) return emergency;
                if(compare_nocase(s1, t_string( _T("alert"))) == 0 ) return alert;
                if(compare_nocase(s1, t_string( _T("critical"))) == 0 ) return critical;
                if(compare_nocase(s1, t_string( _T("error"))) == 0 ) return error;
                if(compare_nocase(s1, t_string( _T("warning"))) == 0 ) return warning;
                if(compare_nocase(s1, t_string( _T("notice"))) == 0 ) return notice;
                if(compare_nocase(s1, t_string( _T("info"))) == 0 ) return info;
                if(compare_nocase(s1, t_string( _T("debug1"))) == 0 ) return debug1;
                if(compare_nocase(s1, t_string( _T("debug2"))) == 0 ) return debug2;
                if(compare_nocase(s1, t_string( _T("debug3"))) == 0 ) return debug3;
                if(compare_nocase(s1, t_string( _T("debug4"))) == 0 ) return debug4;
                if(compare_nocase(s1, t_string( _T("debug5"))) == 0 ) return debug5;
                if(compare_nocase(s1, t_string( _T("debug6"))) == 0 ) return debug6;
                if(compare_nocase(s1, t_string( _T("debug7"))) == 0 ) return debug7;
                if(compare_nocase(s1, t_string( _T("debug8"))) == 0 ) return debug8;
                if(compare_nocase(s1, t_string( _T("debug9"))) == 0 ) return debug9;
                return unknown;
            }

            CORE_API severity to_level( assert_level::severity al )
            {
                if( al == assert_level::debug ) return debug1;
                if( al == assert_level::error ) return error;
                if( al == assert_level::fatal ) return emergency;
                if( al == assert_level::warning ) return warning;
                return unknown;
            }

            CORE_API severity to_level_from_int( int lvl )
            {
                if(lvl == emergency) return emergency;
                if(lvl == alert) return alert;
                if(lvl == critical) return critical;
                if(lvl == error) return error;
                if(lvl == warning) return warning;
                if(lvl == notice) return notice;
                if(lvl == info) return info;
                if(lvl == debug1) return debug1;
                if(lvl == debug2) return debug2;
                if(lvl == debug3) return debug3;
                if(lvl == debug4) return debug4;
                if(lvl == debug5) return debug5;
                if(lvl == debug6) return debug6;
                if(lvl == debug7) return debug7;
                if(lvl == debug8) return debug8;
                if(lvl == debug9) return debug9;
                return debug9;
            }
        }
    }
}
