#ifndef _CORE_TIMECODE_H_
#define _CORE_TIMECODE_H_

#include "media_definitions.hpp"
#include <boost/operators.hpp>
#include "enforce_defines.hpp"
#include "enforce.hpp"

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
	#pragma warning (push)
	#pragma warning (disable:4512)
#endif

#include <boost/rational.hpp>

#ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
	#pragma warning (pop)
#endif

namespace olib
{
   namespace opencorelib
	{
		/// Represents a label that should be presented together with a frame of media.
		/** Use this class to present media times to the user. It represents a time code
			of the form HH:MM:SS:FF (hours:minutes:seconds:frames). 
			
			Depending on how the time_code object was created (whicholib::opencorelib::frame_rate::type that was 
			specified) the maximum frame count is different (max for PAL is 24, max
			for NTSC is 29).

			Internally you should use the media_time class to hold the actual
			time the media cursor is pointing to. To get a time_code to present,
			use media_time::to_time_code.

			@author Mats Lindel&ouml;f
			*/
		class CORE_API time_code :
			public boost::equality_comparable< time_code >
		{
		public:

			/// Creates a zero initialized time code.
			time_code(bool drop_frame)
				: m_i_hours(0)
				, m_i_min(0)
				, m_i_sec(0)
				, m_i_frames(0)
				, m_drop_frame(drop_frame)
			{
			}
			
			/// Creates a time code with the given values.
			/** @param hours The number of hours of media (< 24)
				@param minutes The number of min of media (<60)
				@param seconds The number of sec of media (<60)
				@param frames The number of frames of media (<25 for PAL, <30 for NTSC) 
				@throw CBase_exception if the values are out of range. */
			time_code( boost::uint32_t hours, 
				boost::uint32_t minutes, 
				boost::uint32_t seconds, 
				boost::uint32_t frames,
				bool drop_frame )
				: m_i_hours(hours), m_i_min(minutes), m_i_sec(seconds), m_i_frames(frames), m_drop_frame(drop_frame)
			{
				check_valid();
			}

			time_code( const t_string &inputstr ) { from_string(inputstr); }
			time_code( const TCHAR *inputcstr ) { from_string(inputcstr);	}

			boost::uint32_t operator[]( int idx )
			{
				ARENFORCE( idx >= 0 && idx < 4 );
				if(idx == 0) return m_i_hours;
				if(idx == 1) return m_i_min;
				if(idx == 2) return m_i_sec;
				return m_i_frames;
			}

			/// Get the number of hours.
			boost::uint32_t get_hours() const { return m_i_hours; }

			/// Get the number of minutes
			boost::uint32_t get_minutes() const { return m_i_min; }

			/// Get the number of seconds.
			boost::uint32_t get_seconds() const { return m_i_sec; }

			/// Get the number of frames.
			boost::uint32_t get_frames() const { return m_i_frames; }

			/// Returns whether the timecode uses drop frame for NTSC or not
			bool get_uses_drop_frame() const { return m_drop_frame; }
 
			/// Set the number of hours.
			void set_hours( boost::uint32_t h ) { ARENFORCE(h < 24); m_i_hours = h; } 

			/// Set the number of minutes.
			void set_minutes( boost::uint32_t m ) { ARENFORCE(m < 60); m_i_min = m; }

			/// Set the number of seconds.
			void set_seconds( boost::uint32_t s ) {  ARENFORCE(s < 60); m_i_sec = s; }

			/// Set the number of frames.
			void set_frames( boost::uint32_t f ) { m_i_frames = f; }

			/// Sets whether the timecode is to be interpreted using drop frame or not
			void set_uses_drop_frame( bool drop_frame ) { m_drop_frame = drop_frame; }

			/// Convert the time code to a string. If the timecode uses drop frame the
			/// format will use a semicolon instead of a colon between seconds and frames,
			/// like so: "HH:MM:SS;FF". If it does not use drop frames then the format is
			/// "HH:MM:SS:FF".
			t_string to_string() const;

			/// Set a the time code object with the contents of a string. The format expected 
			/// is described above to_string()
			void from_string(const t_string &timestr);

			/// Stream the timecode as a string to the specified stream. If the timecode
			/// uses drop frame the format will use a semicolon instead of a colon between 
			/// seconds and frames, like so: "HH:MM:SS;FF". If it does not use drop frames
			/// then the format is "HH:MM:SS:FF".
			CORE_API friend t_ostream& operator<<( t_ostream& os, const time_code& tc );

			/// compare two time codes for equality.
			/** The preferred way is to convert the time_code to a media_time and
				use media_time to do comparisons. */
			CORE_API friend bool operator==(const time_code& lhs, const time_code& rhs);

		private:
			void check_valid()
			{
				ARENFORCE( m_i_hours < 24 && m_i_min < 60 && m_i_sec < 60 )
					(m_i_hours)(m_i_min)(m_i_sec)(m_i_frames);
			}

			boost::uint32_t m_i_hours, m_i_min, m_i_sec, m_i_frames;
			bool m_drop_frame;
		};
	}
}


#endif //_CORE_TIMECODE_H_

