#ifndef _CORE_FRAME_SPAN_H_
#define _CORE_FRAME_SPAN_H_

#include <boost/operators.hpp>

#include "./media_time.hpp"
#include "./media_definitions.hpp"

namespace olib
{
   namespace opencorelib
    {
        /// Represents a number of frames of some kind of media.
        class CORE_API frames :
            public boost::less_than_comparable< frames >,
            public boost::equality_comparable< frames>,
            public boost::addable< frames >,
            public boost::subtractable< frames >,
            public boost::dividable< frames >,
            public boost::multipliable< frames >
        {
        public:

            /// Sets the internal frame counter to 0
			frames();

            /// explicit constructor to avoid using pure ints where frames are expected.
            explicit frames( boost::int32_t f );

            /// Get the internal int representation.
            boost::int32_t get_frames() const { return m_frames; }
            void set_frames( const boost::int32_t f ) { m_frames = f; }

            /// Add a number of seconds to the current time.
            frames operator+=(const frames &tc);

            /// Remove a number of seconds from the current time.
            frames operator-=(const frames &tc);

            /// Division
            frames operator/=( const frames& tc );

            /// Multiplication
            frames operator*=( const frames& tc );

            frames operator-() const;

            /// Compares for equality
            CORE_API friend bool operator==(const frames& lhs, const frames& rhs);
            
            /// Is lhs less than rhs?
            CORE_API friend bool operator<(const frames& lhs, const frames& rhs);

            CORE_API friend t_ostream& operator<<( t_ostream& os, const frames& ts);

            operator boost::int32_t () const { return m_frames ;}
        private:
            boost::int32_t m_frames;
            
        };
	
		/// A span of frames, used throughout amf.
        typedef span< frames > frame_span;

        /// Convert from one frame_rate::type to another
        /** Let's say you have 25 frames in frame_rate::pal, that is 1 second of media.
            You want to convert that to the corresponding number of frames in frame_rate::ntsc,
            then you call this function and get 30 frames back (~29.97 rounded up), 
            which is the approximate number of frames per second for ntsc. 
            @param f The number of fames in from_fr
            @param from_fr The coordinate system f lives in.
            @param to_fr The coordinate system to convert to
            @return The number of frames f corresponds to in to_fr. */
        CORE_API frames convert( const frames& f, frame_rate::type from_fr, frame_rate::type to_fr );

        /// Convert a number of frames in one coordinate system to an absolute media_time.
        /** @param f The number of frames to convert.
            @param fr The frame_rate that the frames are given in.
        	@return  The media_time corresponding to the frames in the given coordinate system.*/
        CORE_API media_time to_time( const frames& f, frame_rate::type fr );

        /// Convert a media_time (the global reference system) to a number fo fames in a frame_rate::type.
        /** @param mt The media_time to convert.
            @param fr The coordinate system the returned frames live in.
            @return The corresponding number of frames in the target coordinate system. */
        CORE_API frames from_time( const media_time& mt, frame_rate::type fr );
        
    }
}


#endif // _CORE_FRAME_SPAN_H_

