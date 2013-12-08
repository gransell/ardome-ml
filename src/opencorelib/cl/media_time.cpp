#include "precompiled_headers.hpp"
#include "media_time.hpp"

#include "assert_defines.hpp"
#include "assert.hpp"
#include "log_defines.hpp"


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

		namespace
		{
			//Max size of the frame part for the specific FPS
			int get_framecount_for_framerate( olib::opencorelib::frame_rate::type ft )
			{
				switch( ft )
				{
					case frame_rate::ntsc:
						return 30;
					case frame_rate::movie:
						return 24;
					case frame_rate::ntsc_60:
						return 60;
					default:
						return boost::rational_cast< int >( frame_rate::get_fps( ft ) );
				}
			}
		}

		time_code media_time::to_time_code(olib::opencorelib::frame_rate::type ft, bool drop_frame, bool wrap_around_at_24h ) const
		{
			ARENFORCE_MSG( m_time >= 0, "Can not convert negative values to time codes.")(*this);
			const rational_time full_day( 24 * 60 * 60, 1 );
			rational_time time( m_time );

			if( wrap_around_at_24h )
			{
				while( time >= full_day )
				{
					time -= full_day;
				}
			}
			else
			{
				ARENFORCE_MSG( time < full_day, 
					"The wrap_around_at_24h parameter is false, and the time \"%1%\" is too large to be converted to timecode.")
					( time );
			}

			time_code tc( drop_frame );
			rational_time fps = frame_rate::get_fps(ft);  
			if( !drop_frame )
			{
				rational_time t = time;
				if( ft == frame_rate::ntsc || ft == frame_rate::movie || ft == frame_rate::ntsc_60 )
				{
					//If we are not using drop frame, then the timecode
					//will start to show lower time values than what the
					//video shows. This lag will happen by a factor 
					//(30000/1001) / 30 for NTSC, (24000/1001) / 24 for
					//24p etc., so we simulate this by shortening the time
					//by this factor.
					t *= (fps / get_framecount_for_framerate( ft ) );
					fps = rational_time(get_framecount_for_framerate( ft ), 1);
				}

				// Nr of whole frame seconds:
				boost::int64_t whole_sec = t.numerator() / t.denominator(); 
				tc.set_seconds(static_cast<boost::int32_t>(whole_sec % 60));
				boost::int64_t whole_min = whole_sec / 60;
				tc.set_minutes(static_cast<boost::uint32_t>(whole_min % 60));
				tc.set_hours( static_cast<boost::uint32_t>(whole_min / 60 ));

				rational_time frames(  t.numerator() % t.denominator(), 
					t.denominator()); 
				frames *= fps;
				tc.set_frames( static_cast<boost::int32_t>(frames.numerator() / frames.denominator()) );
			}
			else
			{
				ARENFORCE_MSG( ft == frame_rate::ntsc || ft == frame_rate::ntsc_60, "Drop frame can only be used with NTSC or NTSC 60" )( frame_rate::to_string( ft ) );
				rational_time totF = time * fps;

				boost::int64_t tc_base_fps = get_framecount_for_framerate( ft );
				const int frames_to_add_per_minute = ( ft == frame_rate::ntsc ? 2 : 4 );

				// Note: description below is for NTSC 30. NTSC 60 is similar, but with 4 frames
				// dropped each time instead of 2.

				// How many frames have we dropped during the time we have played?
				// We discard 2 frames every whole minute (except 0,10,20,30 ...)
				// How many whole minutes do we have: mns = time / 60;
				//									  m = mns.numerator() / mns.denominator();
				// This is the number of dropped frames :  dropped = 2 * (m - m / 10);
				// So if we have 10 minutes total, we have dropped 18 frames.
				// This means that we have skipped forward on the counter, thus
				// "adding" 18 more frames to our sequence.

				//First, calculate the number of 10-minute blocks. For each of these blocks, 2 frames
				//have been removed for 9 out of the 10 minutes.
				int frames_per_ten_minutes = tc_base_fps * 60 * 10 - frames_to_add_per_minute*9;
				boost::int64_t m1 = boost::rational_cast<boost::int64_t>( totF / frames_per_ten_minutes );

				//Next, calculate the number of minutes in what's left
				//The first minute happens at 30*60 = 1800 frames, the next at 2*30*60-2 = 3598 frames, etc. with
				//1798 frames in between.
				boost::int64_t m2 = boost::rational_cast<boost::int64_t>( ( totF - m1 * frames_per_ten_minutes - frames_to_add_per_minute ) / ( tc_base_fps * 60 - frames_to_add_per_minute ) );

				//Now we have the total number of minutes passed, so we can calculate the exact number of drop frames to add
				boost::int64_t m = m1 * 10 + m2;
				boost::int64_t dropped = boost::int64_t(frames_to_add_per_minute) * (m - (m / boost::int64_t(10)));
				totF += dropped;

				//Now that we have compensated for the dropped frames, we are
				//in the 30 fps domain again.
				fps = rational_time(tc_base_fps, 1);

				const rational_time fph(3600 * fps), fpm(60 * fps);

				rational_time hrs = totF / fph;
				tc.set_hours( static_cast<boost::uint32_t>(hrs.numerator() / hrs.denominator()) );	  
				totF -= tc.get_hours() * fph;

				rational_time mns = totF / fpm;
				tc.set_minutes( static_cast<boost::uint32_t>( mns.numerator() / mns.denominator() ));
				totF -= tc.get_minutes() * fpm;

				rational_time secs = totF / fps ;
				tc.set_seconds( static_cast<boost::uint32_t>( secs.numerator() / secs.denominator() ));    
				totF -= tc.get_seconds() * fps;

				tc.set_frames( static_cast<boost::uint32_t>(totF.numerator() / totF.denominator()));
			}

			ARASSERT_MSG(	(double)tc.get_frames() < ((double)fps.numerator()/fps.denominator()), 
				_CT("Frame count is too high"))(tc.get_frames());
			return tc;
		}


		CORE_API media_time from_time_code(olib::opencorelib::frame_rate::type ft, const time_code& tc )
		{
			boost::int64_t tc_base_fps = get_framecount_for_framerate( ft );
			boost::int64_t total_frames;

			if( tc.get_uses_drop_frame() )
			{
				ARENFORCE_MSG( ft == frame_rate::ntsc || ft == frame_rate::ntsc_60, "Drop frame can only be used with NTSC or NTSC 60" );

				if( tc.get_frames() < 2 && tc.get_seconds() == 0 && tc.get_minutes() % 10 != 0 )
				{
					//This should possibly be an ARENFORCE instead
					ARLOG_WARN("from_time_code(): Invalid NTSC timecode: \"%1%\"")( tc.to_string() );
				}

				const int frames_to_add_per_minute = ( ft == frame_rate::ntsc ? 2 : 4 );

				//Calculate the total minute count for drop frame compensation
				boost::uint32_t m = 60 * tc.get_hours() + tc.get_minutes();

				total_frames = 
					tc.get_frames() + 
					tc_base_fps * (tc.get_seconds() + 60 * tc.get_minutes() + 3600 * tc.get_hours()) -
					frames_to_add_per_minute * (m - m / 10);
			}
			else
			{
				boost::int64_t seconds = tc.get_hours() * 3600 + tc.get_minutes() * 60 + tc.get_seconds();

				total_frames = seconds * tc_base_fps + tc.get_frames();
			}

			return from_frame_number( ft, total_frames );
		}

		CORE_API t_ostream& operator<<( t_ostream& os, const media_time& mt )
		{
			os << mt.m_time.numerator() << _CT("/") << mt.m_time.denominator();
			return os;
		}

		CORE_API media_time from_frame_number(olib::opencorelib::frame_rate::type ft, boost::int64_t fr_nr )
		{
			media_time mt;
			return mt.from_frame_number(ft, fr_nr);
		}

		media_time& media_time::from_frame_number(olib::opencorelib::frame_rate::type ft,  const rational_time& fr_nr )
		{
			rational_time fps(frame_rate::get_fps(ft));
			m_time = fr_nr * (1 / fps);
			return *this;
		}

		boost::int32_t media_time::to_frame_nr(olib::opencorelib::frame_rate::type ft ) const
		{
			rational_time fps(frame_rate::get_fps(ft));
			rational_time frm_count = m_time * fps;
			return static_cast<boost::int32_t>( frm_count.numerator() / frm_count.denominator() );
		}

		void media_time::add_frames(olib::opencorelib::frame_rate::type ft , boost::int64_t nr_of_frames )
		{
			rational_time fps(frame_rate::get_fps(ft));
			m_time += nr_of_frames * (1/fps);
		}
	}
}
