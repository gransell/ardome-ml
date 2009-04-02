#include "precompiled_headers.hpp"
#include "./point.hpp"

namespace olib
{
   namespace opencorelib
    {
        CORE_API bool fuzzy_equal( const double& lhs, const double& rhs, double epsilon)
        {
            if( (lhs > (rhs - epsilon)) && (lhs < (rhs + epsilon)) ) return true;
            return false;
        }

        CORE_API t_ostream& operator<<( t_ostream& os, const point& pt )
        {
            os << _CT("<point ");
            os << _CT("x=\"") << pt.get_x() << _CT("\" ");
            os << _CT("y=\"") << pt.get_y() << _CT("\" />");
            return os;
        }

        CORE_API bool operator==(const point& lhs, const point& rhs)
        {
            return fuzzy_equal(lhs.m_x,rhs.m_x) && fuzzy_equal(lhs.m_y, rhs.m_y);
        }

        CORE_API bool operator<(const point& lhs, const point& rhs)
        {
            // First order by x
            if( lhs.m_x < rhs.m_x ) return true;
            if( fuzzy_equal( lhs.m_x, rhs.m_x)  ) 
            {
                // x:es are equal, order on y
                return lhs.m_y < rhs.m_y;
            }
            else
            {
                // x:es are not equal, lhs.m_x must be bigger rhs.m_x
                return false;
            }
        }
    }
}

