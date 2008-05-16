#include "precompiled_headers.hpp"
#include "./frames.hpp"

namespace olib
{
   namespace opencorelib
    {
		frames::frames() : m_frames( 0 ) 
		{
		}
		
        frames::frames( boost::int32_t f ) : m_frames(f)
        {

        }

        frames frames::operator+=( const frames &tc )
        {
            m_frames += tc.m_frames; return *this;
        }

        frames frames::operator-=( const frames &tc )
        {
            m_frames -= tc.m_frames; return *this;
        }

        frames frames::operator-() const
        {
            return frames(-m_frames);
        }

        frames frames::operator/=( const frames& tc )
        {
            m_frames /= tc.m_frames; return *this;
        }

        frames frames::operator*=( const frames& tc )
        {
            m_frames *= tc.m_frames; return *this;
        }

        CORE_API bool operator==(const frames& lhs, const frames& rhs)
        {
            return lhs.m_frames == rhs.m_frames;
        }

        CORE_API bool operator<(const frames& lhs, const frames& rhs)
        {
            return lhs.m_frames < rhs.m_frames;
        }
		
		
		CORE_API t_ostream& operator<<( t_ostream& os, const frames& ts)
		{
            os << ts.m_frames ;
            return os;
		}

        CORE_API frames convert( const frames& f, frame_rate::type from_fr, frame_rate::type to_fr )
        {
            media_time mt = to_time(f, from_fr);
            return from_time(mt, to_fr);
        }

        CORE_API media_time to_time( const frames& f, frame_rate::type fr )
        {
            return from_frame_number(fr, f.get_frames() );
        }

        CORE_API frames from_time( const media_time& mt, frame_rate::type fr )
        {
            return frames( mt.to_frame_nr(fr) );
        }
    }
}
