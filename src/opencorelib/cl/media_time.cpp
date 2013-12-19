#include "precompiled_headers.hpp"
#include "media_time.hpp"

#include "assert_defines.hpp"
#include "assert.hpp"
#include "log_defines.hpp"

using boost::int64_t;
using boost::uint32_t;
using boost::int32_t;

namespace olib
{
   namespace opencorelib
	{

		media_time::media_time() 
		{
		}


		media_time::media_time( const rational_time& curr_time ) : m_time( curr_time )
		{
		}

		bool operator==(const media_time& lhs, const media_time& rhs)
		{
			return	(lhs.m_time == rhs.m_time );
		}

		bool operator<(const media_time& lhs, const media_time& rhs)
		{
			return lhs.m_time < rhs.m_time ;
		}

		media_time media_time::operator+=(const media_time &tc) 
		{
			m_time += tc.m_time;
			return *this;
		}

		media_time media_time::operator-=(const media_time &tc) 
		{
			m_time -= tc.m_time;
			return *this;
		}

		media_time media_time::operator/=( const media_time& tc )
		{
			m_time /= tc.m_time;
			return *this;
		}

		media_time media_time::operator*=( const media_time& tc )
		{
			m_time *= tc.m_time;
			return *this;
		}

		time_code media_time::to_time_code( frame_rate::type ft, bool drop_frame, bool wrap_around_at_24h ) const
		{
			if( drop_frame )
			{
				//Note: movie framerate is 24000/1001, but due to
				//convention the timecodes never use drop frame
				ARENFORCE_MSG( ft == frame_rate::ntsc || ft == frame_rate::ntsc_60,
					"Drop frame can only be used with NTSC or NTSC 60" )
					( frame_rate::to_string( ft ) );
			}

			//Convert to frame count in target fps
			int64_t num_frames_i64 = boost::rational_cast< int64_t >( m_time * frame_rate::get_fps( ft ) );
			ARENFORCE_MSG( num_frames_i64 >= 0, "Can not convert negative values to time codes.")
				( *this )( num_frames_i64 )( frame_rate::to_string( ft ) );
			
			//Perform 24-hour wrap around, if necessary
			const uint32_t num_frames_in_24h = time_code::get_tc_count_in_24_hours( ft, drop_frame );
			if( wrap_around_at_24h )
			{
				num_frames_i64 = num_frames_i64 % num_frames_in_24h;
			}
			else
			{
				ARENFORCE_MSG( num_frames_i64 < num_frames_in_24h, 
					"The wrap_around_at_24h parameter is false, and the frame count %1% is too large to be converted to a timecode.")
					( num_frames_i64 )( frame_rate::to_string( ft ) )( drop_frame );
			}

			//We now know that num_frames_i64 is non-negative and not larger than uint32
			//max, so we can safely cast it.
			uint32_t num_frames = static_cast< uint32_t >( num_frames_i64 );
			const int tc_rate = time_code::get_tc_rate_for_framerate( ft );

			if( drop_frame )
			{
				//Drop frame TC works as follows: When rolling over to a new whole minute, 
				//the timecode is increased by 2 frames (4 frames in the case of NTSC 60),
				//_except_ if the minute count is evenly divisible by 10.
				//For example: 00:00:59:29 -> 00:01:00:02
				//			   00:01:59:29 -> 00:02:00:02
				//			   and:
				//			   00:09:59:29 -> 00:10:00:00
				//
				//We simulate this by adding the extra frames to num_frames.
				//The adjusted value of num_frames can then be used to create the
				//result timecode just as with non-drop TCs.
				const uint32_t frames_per_drop = ( ft == frame_rate::ntsc ? 2 : 4 );
				const uint32_t frames_per_ten_mins = 10 * 60 * tc_rate - 9 * frames_per_drop;
				const uint32_t ten_min_periods = num_frames / frames_per_ten_mins;
				const uint32_t rest = num_frames - ten_min_periods * frames_per_ten_mins;
				//First minute starts at xx:x0:00:00, so it has (60 * fps) TC frames
				//Other minutes start at xx:xx:00:02/04, so they are (60 * fps - drop) TC frames
				const uint32_t remaining_minutes = (rest < frames_per_drop)
					? 0
					: (rest - frames_per_drop) / (60 * tc_rate - frames_per_drop);

				num_frames += (ten_min_periods * 9 + remaining_minutes) * frames_per_drop;
			}

			//Finally, build up the resulting time code object
			const uint32_t hours = num_frames / (3600 * tc_rate);
			num_frames -= hours * (3600 * tc_rate);
			const uint32_t minutes = num_frames / (60 * tc_rate);
			num_frames -= minutes * (60 * tc_rate);
			const uint32_t seconds = num_frames / tc_rate;
			num_frames -= seconds * tc_rate;
			ARASSERT( hours < 24 );
			ARASSERT( minutes < 60 );
			ARASSERT( seconds < 60 );
			ARASSERT( num_frames < tc_rate );
			return time_code( hours, minutes, seconds, num_frames, drop_frame );
		}


		CORE_API media_time from_time_code(olib::opencorelib::frame_rate::type ft, const time_code& tc )
		{
			const uint32_t tc_rate = time_code::get_tc_rate_for_framerate( ft );
			uint32_t total_frames = tc.get_frames() + tc_rate * (tc.get_seconds() + 60 * tc.get_minutes() + 3600 * tc.get_hours());
			if( tc.get_uses_drop_frame() )
			{
				ARENFORCE_MSG( ft == frame_rate::ntsc || ft == frame_rate::ntsc_60, "Drop frame can only be used with NTSC or NTSC 60" );
				const uint32_t frames_per_drop = ( ft == frame_rate::ntsc ? 2 : 4 );

				if( tc.get_frames() < frames_per_drop && tc.get_seconds() == 0 && tc.get_minutes() % 10 != 0 )
				{
					//Nonexistent drop frame TC. This should possibly be an ARENFORCE instead.
					ARLOG_WARN("from_time_code(): Invalid %1% timecode: \"%2%\"")
						( frame_rate::to_string( ft ) )( tc.to_string() );
				}

				//Calculate the total minute count for drop frame compensation
				const uint32_t total_minutes = 60 * tc.get_hours() + tc.get_minutes();
				total_frames -= frames_per_drop * ( total_minutes - total_minutes / 10 );
			}

			return from_frame_number( ft, total_frames );
		}

		CORE_API t_ostream& operator<<( t_ostream& os, const media_time& mt )
		{
			os << mt.m_time.numerator() << _CT("/") << mt.m_time.denominator();
			return os;
		}

		CORE_API media_time from_frame_number(olib::opencorelib::frame_rate::type ft, int64_t fr_nr )
		{
			media_time mt;
			return mt.from_frame_number(ft, fr_nr);
		}

		media_time& media_time::from_frame_number(olib::opencorelib::frame_rate::type ft,  const rational_time& fr_nr )
		{
			rational_time fps(frame_rate::get_fps(ft));
			m_time = fr_nr / fps;
			return *this;
		}

		int32_t media_time::to_frame_nr(olib::opencorelib::frame_rate::type ft ) const
		{
			rational_time fps(frame_rate::get_fps(ft));
			rational_time frm_count = m_time * fps;
			return static_cast<int32_t>( frm_count.numerator() / frm_count.denominator() );
		}

		void media_time::add_frames(olib::opencorelib::frame_rate::type ft , int64_t nr_of_frames )
		{
			rational_time fps(frame_rate::get_fps(ft));
			m_time += nr_of_frames * (1/fps);
		}
	}
}
