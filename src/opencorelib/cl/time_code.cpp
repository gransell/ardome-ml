#include "precompiled_headers.hpp"
#include "time_code.hpp"


namespace olib
{
   namespace opencorelib
    {
		
        CORE_API t_ostream& operator<<( t_ostream& os, const time_code& tc )
        {
            t_format fmt(_CT("%02i:%02i:%02i:%02i"));
            os << (fmt % tc.m_i_hours % tc.m_i_min % tc.m_i_sec % tc.m_i_frames);
            return os;
        }

        t_string time_code::to_string() const
        {
            t_stringstream ss;
            ss << *this;
            return ss.str();
        }

        CORE_API bool operator==(const time_code& lhs, const time_code& rhs)
        {
            return  lhs.m_i_hours == rhs.m_i_hours && 
                    lhs.m_i_min == rhs.m_i_min && 
                    lhs.m_i_sec == rhs.m_i_sec && 
                    lhs.m_i_frames == rhs.m_i_frames; 
        }
    }
}
