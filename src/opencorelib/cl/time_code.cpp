#include "precompiled_headers.hpp"
#include "time_code.hpp"


namespace olib
{
   namespace opencorelib
    {
		
        CORE_API t_ostream& operator<<( t_ostream& os, const time_code& tc )
        {
            //Use a semicolon between seconds and frames for drop frame timecodes
            t_format fmt( tc.get_uses_drop_frame() ? _CT("%02i:%02i:%02i;%02i") : _CT("%02i:%02i:%02i:%02i") );
            os << (fmt % tc.m_i_hours % tc.m_i_min % tc.m_i_sec % tc.m_i_frames);
            return os;
        }

        t_string time_code::to_string() const
        {
            t_stringstream ss;
            ss << *this;
            return ss.str();
        }

		void time_code::from_string(const t_string &timestr)
		{
			ARENFORCE_MSG(timestr.size() == 11 && (timestr[8] == _CT(';') || timestr[8] == _CT(':') ),"Invalid input format")(timestr);

			if(timestr[8] == _CT(';'))
				m_drop_frame = true;
			else
				m_drop_frame = false;

			t_stringstream ss(timestr);
			wchar_t skip;
			ss >> m_i_hours >> skip >> m_i_min >> skip >> m_i_sec >> skip >> m_i_frames;

			check_valid();
		}

        CORE_API bool operator==(const time_code& lhs, const time_code& rhs)
        {
            return  lhs.m_i_hours == rhs.m_i_hours && 
                    lhs.m_i_min == rhs.m_i_min && 
                    lhs.m_i_sec == rhs.m_i_sec && 
                    lhs.m_i_frames == rhs.m_i_frames &&
                    lhs.m_drop_frame == rhs.m_drop_frame; 
        }
    }
}
