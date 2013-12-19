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
			TCHAR skip;
			ss >> m_i_hours >> skip >> m_i_min >> skip >> m_i_sec >> skip >> m_i_frames;

			check_valid();
		}

		CORE_API bool operator==(const time_code& lhs, const time_code& rhs)
		{
			return	lhs.m_i_hours == rhs.m_i_hours && 
					lhs.m_i_min == rhs.m_i_min && 
					lhs.m_i_sec == rhs.m_i_sec && 
					lhs.m_i_frames == rhs.m_i_frames &&
					lhs.m_drop_frame == rhs.m_drop_frame; 
		}

		boost::uint32_t time_code::get_tc_count_in_24_hours( frame_rate::type ft, bool drop_frame )
		{
			const int seconds_in_24h = 24 * 60 * 60;
			const int minutes_in_24h = 24 * 60;
			switch( ft )
			{
				case frame_rate::ntsc:
				{
					//2 frames dropped every minute, except every 10th
					return drop_frame
						? seconds_in_24h * 30 - 2 * ( minutes_in_24h / 10 * 9 )
						: seconds_in_24h * 30;
				}
				case frame_rate::ntsc_60:
				{
					 //4 frames dropped every minute, except every 10th
					return drop_frame
						? seconds_in_24h * 60 - 4 * ( minutes_in_24h / 10 * 9 )
						: seconds_in_24h * 60;
				}
				default:
					return seconds_in_24h * get_tc_rate_for_framerate( ft );
			}
		}

		boost::uint32_t time_code::get_tc_rate_for_framerate( frame_rate::type ft )
		{
			switch( ft )
			{
				case frame_rate::movie:
					return 24;
				case frame_rate::pal:
					return 25;
				case frame_rate::ntsc:
					return 30;
				case frame_rate::pal_50:
					return 50;
				case frame_rate::ntsc_60:
					return 60;
			}

			ARENFORCE_MSG( false, "Unknown frame rate %1%" )( ft );
			return 0;
		}
	}
}
