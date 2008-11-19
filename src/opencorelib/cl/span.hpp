#ifndef _CORE_SPAN_H_ 
#define _CORE_SPAN_H_

#undef max
#undef min

/** @file span.h
    Defines useful functions related to the span class. */

#include <algorithm>
#include <boost/operators.hpp>
#include "./minimal_string_defines.hpp"
#include "boost_headers.hpp"

namespace olib
{
   namespace opencorelib
    {
        /// Points to a side of a span (the left or right side)
		namespace side 
        { 
            enum type 
            {  
                left,   /**< The left side of the span. */
                right,  /**< The right side of the span. */
                unknown /**< Unknown, used to initialize to an unknown value */
            };

            /// Convert a trim_anchor::type value to a string.
            CORE_API const TCHAR* to_string( type ta );

            /// Convert a string to a trim_anchor::type value.
            CORE_API type to_type( const t_string& str );
        }
        
        /// Describes the relationship of two spans.
        namespace span_relation
        {
            enum type
            {
                left_of,    /**< span A is to the left of span B */
                right_of,   /**< span A is to the right of span B */
                enclosing,  /**< span A is enclosing span B */
                enclosed    /**< span B is enclosing span A */
            };
        }
		
        /// A span describes a distance on a timeline. 
        /** It has an in-point and a duration, from which the
            out-point can be derived.
            @param boundary_type The unit of the timeline where the span resides.
            @author Mikael Gransell and Mats Lindel&oumlf */
        template < class boundary_type >
		class span : 
			public boost::equality_comparable< span< boundary_type > >,
			public boost::less_than_comparable< span< boundary_type > >
        {
        public:
            typedef boost::function< bool ( const span& lhs, const span& rhs ) > compare_function;

            /// Create a span from an in-point and a duration.
            /** @param in_t The in-point of the span on the timeline.
                @param duration_t The duration of the span, must be greater than 0.
                @throws A base_exception if the duration is smaller than zero. */
            span(   boundary_type in_t, 
                    boundary_type duration_t ) 
                : m_in(in_t), m_duration(duration_t) 
            {
				ARENFORCE_MSG( m_duration >= boundary_type(0), "Can not have a negative duration" );
            }

            /// Create an empty span, with 0 duration.
            span() : m_in(boundary_type(0)), m_duration(boundary_type(0)) { }

            /// Get the in-point of the span.
            const boundary_type& get_in() const { return m_in; }

            /// Get the out-point of the span.
			boundary_type get_out() const { return m_in + m_duration; }

            /// Get the duration of the span.
            const boundary_type& get_duration() const { return m_duration; }

            /// Set the in-point of the span
            /** @param in_t The new in-point.*/
            void set_in( const boundary_type& in_t) 
            {
                m_in = in_t; 
            }

            /// Set the duration of the span
            /** @param duration_t The new duration of the span.
                @throws A base_exception if the new duration is smaller than 0. */
            void set_duration( const boundary_type& duration_t )
            {
				ARENFORCE_MSG( m_duration >= boundary_type(0), "Can not have a negative duration" );
                m_duration = duration_t; 
            }

            /// Translate the time span by an offset, i.e. move the time span.
            void translate( const boundary_type& mt )
            {
                m_in += mt;
            }

            /// Equality comparable.
            template < class T >
            friend bool operator==( const span<T>& lhs, const span<T>& rhs);
			
            /// Less-than comparable.
			template< class T >
			friend bool operator<( const span<T>& lhs, const span<T>& rhs );
            
            /// Stream as text.
            template < class T >
            friend t_ostream& operator<<( t_ostream& os, const span<T>& ts);

        private:
            boundary_type m_in;
            boundary_type m_duration;
        };


        template <class boundary_type >
        bool operator==( const span<boundary_type>& lhs, const span<boundary_type>& rhs)
        {
            return lhs.m_in == rhs.m_in && lhs.m_duration == rhs.m_duration;
        }
		
        template <class boundary_type >
		bool operator<( const span<boundary_type>& lhs, const span<boundary_type>& rhs)
        {
			return lhs.m_in < rhs.m_in;
        }

        template <class boundary_type >
        t_ostream& operator<<( t_ostream& os, const span<boundary_type>& ts)
        {
            os << _T("<span in='") << ts.get_in() << _T("' duration='") << ts.get_duration() << _T("'/>");
            return os;
        }

        /// Subtract one span from another span. 
        /** If the spans don't intersect, the left-hand-side will 
            be returned unaffected (nothing is subtracted from it).
            If they intersect, it can be a left, right or middle
            intersection. If left or right a single subtracted span 
            is returned, otherwise (a middle intersection) two spans
            are returned in the result. 
            @param lhs The span to subtract from.
            @param rhs The span to subtract with.
            @return A vector of one or two spans. */
        template <class boundary_type >
        std::vector< span<boundary_type> >
        span_subtraction( const span<boundary_type>& lhs, const span<boundary_type>& rhs )
        {
            typedef span< boundary_type > span_type;
            std::vector< span_type > res;
            span_type intsect = span_intersection(lhs, rhs);
            if( intsect.get_duration() == boundary_type(0) ) 
            {
                res.push_back(lhs);
                return res;
            }
            else if( intsect.get_duration() == lhs.get_duration() )
            {
                // Nothing left
                return res;
            }

            // We have an intersection.
            // Single or double result?
            if( lhs.get_in() < rhs.get_in() && lhs.get_out() > rhs.get_out() )
            {
                // lhs  ***********
                // rhs     -----
                // res  ###     ###
                res.push_back( span_type(lhs.get_in(), rhs.get_in() - lhs.get_in() ) );
                res.push_back( span_type(rhs.get_out(), lhs.get_out() - rhs.get_out() ) );
                return res;
            }
            
            if( rhs.get_in() <= lhs.get_in() )
            {
                // lhs     ********
                // rhs  --------
                // res          ###
                res.push_back( span_type(rhs.get_out(), lhs.get_out() - rhs.get_out() ) );
                return res;
            }

            // lhs     ***********
            // rhs        -----------
            // res     ###    
            res.push_back( span_type(lhs.get_in(), rhs.get_in() - lhs.get_in() ) );
            return res;
        }

        /// Get the union of two spans.
        /** @param lhs One of the spans to union.
            @param rhs One of the spans to union.
            @return If the spans doesn't intersect, return both in the vector.
                If they touch or intersect, return one big span that covers their
                complete presence on the timeline. */
        template <class boundary_type >
        std::vector< span<boundary_type> > 
        span_union( const span<boundary_type>& lhs, const span<boundary_type>& rhs )
        {
            typedef span< boundary_type > span_type;
            std::vector< span_type > res;
            // If we have no intersection, a single union doesn't exist.
            // Return an undefined value.
            if( span_intersection( lhs, rhs ).get_duration() == boundary_type(0) )
            {
				if( lhs < rhs ) {
					if( lhs.get_out() == rhs.get_in() ) {
						// out and in are the same so we add them together into one span.
						res.push_back( span_type( lhs.get_in(), rhs.get_out() - lhs.get_in() ) );
					}
					else {
						res.push_back(lhs);
						res.push_back(rhs);
					}
				}
				else {
					if( rhs.get_out() == lhs.get_in() ) {
						// out and in are the same so we add them together into one span.
						res.push_back( span_type( rhs.get_in(), lhs.get_out() - rhs.get_in() ) );
					}
					else {
						res.push_back(rhs);
						res.push_back(lhs);
					}
				}
                return res;
            }

            const span<boundary_type>* smallest, *largest;
            if( lhs.get_in() <= rhs.get_in() ) 
            {
                smallest = &lhs;
                largest = &rhs;
            }
            else
            {
                smallest = &rhs;
                largest = &lhs;
            }
                
            res.push_back( span_type( smallest->get_in(), largest->get_out() - smallest->get_in() ) );
            return res;
        }

        /// Intersect two spans, that is find the area on the timeline they both occupy.
        /** @param lhs One span to intersect.
            @param rhs The other span to intersect.
            @return The area both spans occupy on the timeline. */
        template <class boundary_type >
        span<boundary_type> span_intersection( const span<boundary_type>& lhs, const span<boundary_type>& rhs )
        {
            const span<boundary_type>* smallest, *largest;
            if( lhs.get_in() < rhs.get_in() ) 
            {
                smallest = &lhs;
                largest = &rhs;
            }
            else if( lhs.get_in() > rhs.get_in() )
            {
                smallest = &rhs;
                largest = &lhs;
            }
            else
            {
                return span<boundary_type>( lhs.get_in() , std::min(lhs.get_out(), rhs.get_out()) - lhs.get_in());
            }

            if( smallest->get_out() <= largest->get_in() ) return span<boundary_type>();
            boundary_type new_in(std::max( lhs.get_in(), rhs.get_in() ));
            boundary_type new_out(std::min(lhs.get_out(), rhs.get_out()));
            return span<boundary_type>( new_in, new_out - new_in );
        }

        /// Is the left-hand-side's out-point, smaller than or equal to the righ-hand-side's in-point?
        template <class boundary_type >
		bool span_out_before_in( const span<boundary_type>& lhs, const span<boundary_type>& rhs )
        {
            return lhs.get_out() <= rhs.get_in();
        }

        /// Is the left-hand-side's in-point, smaller than or equal to the righ-hand-side's out-point?
        template <class boundary_type >
		bool span_in_before_out( const span<boundary_type>& lhs, const span<boundary_type>& rhs )
        {
            return lhs.get_in() <= rhs.get_out();
        }

        /// Is a given point on the timeline within the bounds of a given span?
        template <class boundary_type >
        bool point_in_span( const span<boundary_type>& lhs, const boundary_type& rhs )
        {
            if( rhs  >= lhs.get_in() &&
                rhs < lhs.get_out() ) return true;
            return false;
        }
		
    /// Trim a span.
        /** @param s The span to trim.
            @param fs The number of units to trim (could be negative)
            @param a The side (left or right) of the span to alter, the other side stays constant.
            @return A trimmed span.
            @throws A base_exception if the number of units to trim is larger than the 
                        duration of the span. */
		template < class boundary_type >
		span< boundary_type > trim( const span<boundary_type>& s, const boundary_type& fs, side::type which_side )
		{
			span< boundary_type > ret = s;
            if( which_side == side::left )
			{
                if( fs > boundary_type(0) )
                {
                    ARENFORCE_MSG( s.get_duration() >= fs, "Can not trim longer than duration." );
                }

				boundary_type out_t = s.get_out();
				ret.translate(fs);
				ret.set_duration( out_t - ret.get_in() );
			}
			else
			{
                if( fs < boundary_type(0) )
                {
                    ARENFORCE_MSG( s.get_duration() >= fs, "Can not trim longer than duration." );
                }
				ret.set_duration( s.get_duration() + fs );
			}
			return ret;
		}
            
        /**
         * Lets the caller know how spans are related to each other regarding position.
         * This method will let the caller know how s is positioned relative to relative_span. Mind that
         * this method does not let the caller know if overlaps are present when returning left or right.
         * It is up to the caller to check that using for example span_intersection.
         * @param s Our main span.
         * @param relative_span The span that we want to se how s is positioned 
         * @return span_relation::left_of if s is positioned left of relative_span
         *          span_relation::right_of if s is positioned left of relative_span
         *          span_relation::enclosing if s covers all of relative_span
         *          span_relation::enclosed if relative_span covers all of s
         * @sa span_relation
         * @sa span_intersection
         */
        template< class boundary_type >
        span_relation::type relative_position( const span<boundary_type>& s, const span<boundary_type>& relative_span ) 
        {
            if( s < relative_span ) {
                if( s.get_out() > relative_span.get_out() ) {
                    return span_relation::enclosing;
                }
                else {
                    return span_relation::left_of;
                }
            }
            else {
                if( s.get_out() > relative_span.get_out() ) {
                    return span_relation::right_of;
                }
                else {
                    return span_relation::enclosed;
                }
            }
        }
    }
}

#endif // _CORE_SPAN_H_

