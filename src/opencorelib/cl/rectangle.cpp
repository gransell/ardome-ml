#include "precompiled_headers.hpp"
#include "./rectangle.hpp"

#include "./enforce_defines.hpp"
#include "./enforce.hpp"

#undef max
#undef min

#include <algorithm>

namespace olib
{
   namespace opencorelib
    {

        CORE_API bool operator==(const rectangle& lhs, const rectangle& rhs)
        {
            return lhs.m_location == rhs.m_location && lhs.m_size == rhs.m_size;
        }

        CORE_API t_ostream& operator<<( t_ostream& os, const rectangle& rect )
        {
            os << _T("<rectangle ");
            os << _T("x=\"") << rect.get_x() << _T("\" ");
            os << _T("y=\"") << rect.get_y() << _T("\" ");
            os << _T("width=\"") << rect.get_width() << _T("\" ");
            os << _T("heigth=\"") << rect.get_height() << _T("\" />");
            return os;
        }

        bool rectangle::get_is_empty() const 
        { 
            return  fuzzy_equal(m_size.get_width(),0) && fuzzy_equal(m_size.get_height(),0) &&
                    fuzzy_equal(m_location.get_x(),0) && fuzzy_equal(m_location.get_y(), 0); 
        }

        rectangle rectangle::from_ltrb( double left, double top, double right, double bottom )
        {
            rectangle res;
            res.m_location.set_x(left);
            res.m_location.set_y(top);
            res.m_size.set_width( right - left );
            res.m_size.set_height( bottom - top );
            ARENFORCE_MSG( res.m_size.get_height() >= 0 && res.m_size.get_width() >= 0,
                        "Can not create a recatangel with negative size");
            return res;
        }

        void rectangle::inflate( const size& sz )
        {
            m_location += point(sz.get_width(), sz.get_height());
            m_size += sz;
        }

        bool rectangle::intersects_with( const rectangle& other ) const
        {
            if( other.get_right() <= get_left() ) return false;
            if( other.get_left() >= get_right() ) return false;
            if( other.get_top() >= get_bottom() ) return false;
            if( other.get_bottom() <= get_top() ) return false;
            return true;
        }

        bool rectangle::contains( const rectangle& other ) const
        {
            if( other.get_left() >= get_left() && other.get_top() >= get_top() &&
                other.get_right() <= get_right() && other.get_bottom() >= get_bottom() ) return true;
            return false;
        }

        CORE_API rectangle rect_intersection( const rectangle& lhs, const rectangle& rhs)
        {
            if( lhs.get_is_empty() || rhs.get_is_empty() ) return rectangle();

            if( !lhs.intersects_with(rhs) ) return rectangle();

            double x = std::max( lhs.get_x(), rhs.get_x());
            double y = std::max( lhs.get_y(), rhs.get_y());
            double r = std::min( lhs.get_right(), rhs.get_right());
            double b = std::min( lhs.get_bottom(), rhs.get_bottom());

            return rectangle::from_ltrb( x, y, r, b);
        }

        CORE_API rectangle rect_union( const rectangle& lhs, const rectangle& rhs)
        {
            if( lhs.get_is_empty() ) return rhs;
            if( rhs.get_is_empty() ) return lhs;

            double x = std::min( lhs.get_x(), rhs.get_x());
            double y = std::min( lhs.get_y(), rhs.get_y());
            double r = std::max( lhs.get_right(), rhs.get_right());
            double b = std::max( lhs.get_bottom(), rhs.get_bottom());

            return rectangle::from_ltrb( x, y, r, b);
        }



    }
}
