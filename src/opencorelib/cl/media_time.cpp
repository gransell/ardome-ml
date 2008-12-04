#include "precompiled_headers.hpp"
#include "media_time.hpp"

#include "assert_defines.hpp"
#include "assert.hpp"


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
            return  (lhs.m_time == rhs.m_time );
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

        time_code media_time::to_time_code(olib::opencorelib::frame_rate::type ft ) const
        {
            ARENFORCE_MSG( m_time >= 0, "Can not convert negative values to time codes.")(*this);
            time_code tc;  
            const rational_time fps = frame_rate::get_fps(ft);  
            if( ft == frame_rate::pal || ft == frame_rate::ntsc || ft == frame_rate::movie )
            {
                // Nr of whole frame seconds:
                boost::int64_t whole_sec = m_time.numerator() / m_time.denominator(); 
                tc.set_seconds(static_cast<boost::int32_t>(whole_sec % 60));
                boost::int64_t whole_min = whole_sec / 60;
                tc.set_minutes(static_cast<boost::uint32_t>(whole_min % 60));
                tc.set_hours( static_cast<boost::uint32_t>(whole_min / 60 ));

                rational_time frames(  m_time.numerator() % m_time.denominator(), 
                    m_time.denominator()); 
                frames *= fps;
                tc.set_frames( static_cast<boost::int32_t>(frames.numerator() / frames.denominator()) );
            }
            else if( ft == frame_rate::ntsc_drop_frame )
            {
                rational_time totF = m_time * fps;   
                const rational_time fph(3600 * fps), fpm(60 * fps);

                // How many frames have we dropped during the time we have played?
                // We discard 2 frames every whole minute (except 0,10,20,30 ...)
                // How many whole minutes do we have: mns = m_time / 60;
                //                                    m = mns.numerator() / mns.denominator();
                // This is the number of dropped frames :  dropped = 2 * (m - m / 10);
                // So if we have 10 minutes total, we have dropped 18 frames.
                // This means that we have skipped forward on the counter, thus
                // "adding" 18 more frames to our sequence.

                rational_time totmns = m_time / 60;
                boost::int64_t m = totmns.numerator() / totmns.denominator();
                boost::int64_t dropped = ARMAKE_I64(2) * (m - (m / ARMAKE_I64(10)));
                totF += dropped;

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

            ARASSERT_MSG(   (double)tc.get_frames() < ((double)fps.numerator()/fps.denominator()), 
                _T("Frame count is too high"))(tc.get_frames());
            return tc;
        }


        CORE_API media_time from_time_code(olib::opencorelib::frame_rate::type ft, const time_code& tc )
        {
            media_time mt;
            rational_time fps(frame_rate::get_fps(ft));

            if( ft == frame_rate::ntsc_drop_frame )
            {
                boost::uint32_t m = 60 * tc.get_hours() + tc.get_minutes();
                rational_time 
                    totfrms = tc.get_frames() + fps * (tc.get_seconds() + 60 * m) - 2 * (m - m / 10);
                return mt.from_frame_number( ft, totfrms);
            }
            else
            {
                mt.m_time = tc.get_hours() * 3600 + tc.get_minutes() * 60 + tc.get_seconds();
                mt.m_time += tc.get_frames() * (1 / fps);
                return mt;
            }
        }

        CORE_API t_ostream& operator<<( t_ostream& os, const media_time& mt )
        {
            os << mt.m_time.numerator() << _T("/") << mt.m_time.denominator();
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
