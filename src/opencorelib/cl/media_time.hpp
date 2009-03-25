#ifndef _CORE_MEDIA_TIME_H_
#define _CORE_MEDIA_TIME_H_


#include <boost/operators.hpp>

#include "media_definitions.hpp"
#include "time_code.hpp"
#include "span.hpp"
#include "typedefs.hpp"

namespace olib
{
   namespace opencorelib
    {
        /// Encapsulates a rational_time and enables conversions to and from time_code and frame counts.
        /** Use this class to represent a point on a media timeline. 
        The internal representation stores the number of seconds of media
        that has been passed. It uses a rational number which makes it possible
        to exactly represent positions even in NTSC (where a frame is 1001/30000 sec long).
        Depending onolib::opencorelib::frame_rate::type the time should be displayed 
        differently as time_codes. The to_time_code member takes care of this conversion
        properly.

        Instances of this class supports <, <=, >, >=, ==, +, -.

        @author Mats Lindel&ouml;f */
        class  CORE_API media_time : 
            public boost::less_than_comparable< media_time >,
            public boost::equality_comparable< media_time >,
            public boost::addable< media_time >,
            public boost::subtractable< media_time >,
            public boost::dividable< media_time >,
            public boost::multipliable< media_time >
        {
        public:
            /// Defines zero time
            static media_time zero() { return media_time(0); }

            /// Creates a 0/0 media time. (The start of the media)
            media_time();

            /// Creates a media_time representing a given number of seconds.
            /** @param curr_time The number of seconds played of the media*/
            explicit media_time( const rational_time& curr_time );

            /// Add a number of seconds to the current time.
            media_time operator+=(const media_time &tc);

            /// Remove a number of seconds from the current time.
            media_time operator-=(const media_time &tc);

            /// Division
            media_time operator/=( const media_time& tc );

            /// Multiplication
            media_time operator*=( const media_time& tc );

            /// Compares for equality
            CORE_API friend bool operator==(const media_time& lhs, const media_time& rhs);

            /// Is lhs less than rhs?
            CORE_API friend bool operator<(const media_time& lhs, const media_time& rhs);

            /// Convert the current time to a time_code.
            /** @param ft The currently usedolib::opencorelib::frame_rate::type. */
            time_code to_time_code(olib::opencorelib::frame_rate::type ft ) const;

            /// Converts the current time to a discreet frame number.
            boost::int32_t to_frame_nr(olib::opencorelib::frame_rate::type ft ) const;

            CORE_API friend media_time from_time_code(olib::opencorelib::frame_rate::type ft, const time_code& time_code );

            CORE_API friend media_time from_frame_number(olib::opencorelib::frame_rate::type ft, boost::int64_t fr_nr );

            /// Adds (or subtracts if nr_of_frames is negative) the given number of frames to the time.
            /** Equivalent to:
            <pre>
            media_time mt; 
            mt + from_frame_number(ft, nr_of_frames); // or
            mt - from_frame_number(ft, nr_of_frames); 

            // is equivalent to
            mt.add_frames(ft, nr_of_frames);

            </pre> */
            void add_frames(olib::opencorelib::frame_rate::type ft , boost::int64_t nr_of_frames );

            /// Get the stored time of this object.
            rational_time get_raw_time() const { return m_time; }

            /// Stream the time code on the format: N/D
            CORE_API friend t_ostream& operator<<( t_ostream& os, const media_time& mt );

        private:
            // Current time in seconds
            media_time& from_frame_number(olib::opencorelib::frame_rate::type ft,  const rational_time& fr_nr );
            rational_time m_time;
        };
		
        /// Create a media_time from aolib::opencorelib::frame_rate::type and a frame number.
        CORE_API media_time from_time_code(olib::opencorelib::frame_rate::type ft, const time_code& time_code );
        
        /// Create a media_time from aolib::opencorelib::frame_rate::type and a frame number.
        CORE_API media_time from_frame_number(olib::opencorelib::frame_rate::type ft, boost::int64_t fr_nr );
        
        typedef span< media_time > time_span;
    }
}

#endif // _CORE_MEDIA_TIME_H_
