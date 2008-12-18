#ifndef _CORE_RECTANGLE_H_
#define _CORE_RECTANGLE_H_

#include "./point.hpp"
#include "./size.hpp"
#include <boost/operators.hpp>

namespace olib
{
   namespace opencorelib
    {
        class CORE_API rectangle : public boost::equality_comparable< rectangle >
        {
        public:
            rectangle() : m_location(0,0), m_size(0,0) {}
            rectangle( const point& origin, const size& sze  )
                : m_location(origin), m_size(sze) {}

            static rectangle from_ltrb( double left, double top, double right, double bottom );

            double get_bottom() const { return m_location.get_y() + m_size.get_height(); }
            double get_top() const { return m_location.get_y();  }
            double get_left() const { return m_location.get_x(); }
            double get_right() const { return m_location.get_x() + m_size.get_width() ; }

            double get_height() const { return m_size.get_height(); }
            void set_height( double h ) { m_size.set_height(h); } 

            double get_width() const { return m_size.get_width(); }
            void set_width( double w ) { m_size.set_width(w); }

            double get_x() const { return m_location.get_x(); }
            void set_x( double x ) { m_location.set_x(x); }

            double get_y() const { return m_location.get_y(); }
            void set_y( double y ) { m_location.set_y(y); }

            /// Returns true if location is (0,0) and size is (0,0), otherwise false.
            bool get_is_empty() const;
            
            point get_location() const { return m_location; }
            void set_location( const point& loc ) { m_location = loc; }

            size get_size() const { return m_size; }
            void set_size( const size& szc ) { m_size = szc; }

            void inflate( const size& sz );

            CORE_API friend bool operator==(const rectangle& lhs, const rectangle& rhs);

            bool intersects_with( const rectangle& other ) const;
            bool contains( const rectangle& other ) const;

            CORE_API friend rectangle rect_union( const rectangle& lhs, const rectangle& rhs);
            CORE_API friend rectangle rect_intersection( const rectangle& lhs, const rectangle& rhs);
            
            CORE_API friend t_ostream& operator<<( t_ostream& os, const rectangle& rect );
            
        private:
            point m_location;
            size m_size;
        };

        CORE_API rectangle rect_union( const rectangle& lhs, const rectangle& rhs);
        CORE_API rectangle rect_intersection( const rectangle& lhs, const rectangle& rhs);
        CORE_API bool operator==(const rectangle& lhs, const rectangle& rhs);
        CORE_API t_ostream& operator<<( t_ostream& os, const rectangle& rect );

    }
}

#endif // _CORE_RECTANGLE_H_
