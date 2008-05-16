#include "precompiled_headers.hpp"
#include "./media_definitions.hpp"
#include "./enforce_defines.hpp"
#include "./enforce.hpp"

namespace olib
{
   namespace opencorelib
    {
        namespace frame_rate
        {
            CORE_API rational_time get_fps( type ft )
            {
                if( ft == pal ) return rational_time(25,1);
                else if( ft == movie ) return rational_time(24000,1001);
                else if (   ft == ntsc || 
                            ft == ntsc_drop_frame) return rational_time(30000,1001);
                ARENFORCE_MSG( false, "Unknown frame_rate::type %i")(ft);
                return rational_time(0,0);
            }

            CORE_API type get_type( const opencorelib::rational_time& rt, 
                                    bool drop_frame  )
            {
                if( rt == rational_time(25,1)) return pal;
                else if( rt == rational_time(30000,1001) && drop_frame ) return ntsc_drop_frame;
                else if( rt == rational_time(30000,1001) ) return ntsc;
                else if( rt == rational_time(24000,1001)) return movie;
                return undef;
            }

            CORE_API bool uses_drop_frame( type ft)
            {
                if( ft == ntsc_drop_frame ) return true;
                return false;
            }

            CORE_API const TCHAR* to_string( type ft )
            {
                if( ft == pal ) return _T("pal");
                else if( ft == movie ) return _T("movie");
                else if ( ft == ntsc ) return _T("ntsc");
                else if( ft == ntsc_drop_frame ) return _T("ntsc_drop_frame");
                return _T("olib::opencorelib::frame_rate::to_string: Unknown enum value");
            }

			CORE_API type to_type( const t_string& str )
			{
				if(  str_util::compare_nocase( t_string(_T("pal")), str) == 0 ) return pal;
				if(  str_util::compare_nocase( t_string(_T("movie")), str) == 0 ) return movie;
				if(  str_util::compare_nocase( t_string(_T("ntsc")), str) == 0 ) return ntsc;
				if(  str_util::compare_nocase( t_string(_T("ntsc_drop_frame")), str) == 0 ) return ntsc_drop_frame;
				
				ARENFORCE_MSG( false, "Unknown value %s, can not convert to enumeration.")(str);
				return undef;
			}
        }
    }
}

