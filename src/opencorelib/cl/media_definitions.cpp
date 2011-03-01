#include "precompiled_headers.hpp"
#include "./media_definitions.hpp"
#include "./enforce_defines.hpp"
#include "./enforce.hpp"
#include "str_util.hpp"

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
                else if ( ft == ntsc ) return rational_time(30000,1001);
                else if ( ft == pal_50 ) return rational_time(50,1);
                else if ( ft == ntsc_60 ) return rational_time(60000,1001);
                ARENFORCE_MSG( false, "Unknown olib::opencorelib::frame_rate::type %i")(ft);
                return rational_time(0,0);
            }

            CORE_API type get_type( const opencorelib::rational_time& rt )
            {
                if( rt == rational_time(25,1)) return pal;
                else if( rt == rational_time(30000,1001) ) return ntsc;
                else if( rt == rational_time(24000,1001)) return movie;
                else if( rt == rational_time(50,1)) return pal_50;
                else if( rt == rational_time(60000,1001)) return ntsc_60;
                return undef;
            }

            CORE_API const TCHAR* to_string( type ft )
            {
                if( ft == pal ) return _CT("pal");
                else if( ft == movie ) return _CT("movie");
                else if ( ft == ntsc ) return _CT("ntsc");
                else if ( ft == pal_50 ) return _CT("pal_50");
                else if ( ft == ntsc_60 ) return _CT("ntsc_60");
                return _CT("olib::opencorelib::frame_rate::to_string: Unknown enum value");
            }

			CORE_API type to_type( const t_string& str )
			{
				if(  str_util::compare_nocase( t_string(_CT("pal")), str) == 0 ) return pal;
				if(  str_util::compare_nocase( t_string(_CT("movie")), str) == 0 ) return movie;
				if(  str_util::compare_nocase( t_string(_CT("ntsc")), str) == 0 ) return ntsc;
				if(  str_util::compare_nocase( t_string(_CT("pal_50")), str) == 0 ) return pal_50;
				if(  str_util::compare_nocase( t_string(_CT("ntsc_60")), str) == 0 ) return ntsc_60;
				
				ARENFORCE_MSG( false, "Unknown value %s, can not convert to enumeration.")(str);
				return undef;
			}
        }
    }
}

