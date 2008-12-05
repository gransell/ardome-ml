#ifndef _CORE_POINT_H_
#define _CORE_POINT_H_

#include <boost/operators.hpp>
#include "./minimal_string_defines.hpp"


namespace olib
{
   namespace opencorelib
    {
        class CORE_API point :  public boost::less_than_comparable< point >,
                                public boost::equality_comparable< point >,
                                public boost::addable< point >,
                                public boost::subtractable< point >
        {
        public:
            point( double x, double y) 
                : m_x(x), m_y(y) {}

                double get_x() const { return m_x; }
                void set_x( double x ) { m_x = x; }

                double get_y() const { return m_y; }
                void set_y( double y ) { m_y = y; }

                point& operator+=(const point& other)
                {
                    m_y += other.m_y;
                    m_x += other.m_x;
                    return *this;
                }

                point& operator-=(const point& other)
                {
                    m_y -= other.m_y;
                    m_x -= other.m_x;
                    return *this;
                }

                /// Compares for equality
                CORE_API friend bool operator==(const point& lhs, const point& rhs);

                /// Is lhs less than rhs?
                CORE_API friend bool operator<(const point& lhs, const point& rhs);

                CORE_API friend t_ostream& operator<<( t_ostream& os, const point& pt );

        private:
            double m_x, m_y;

        };

        CORE_API bool operator==(const point& lhs, const point& rhs);
        CORE_API bool operator<(const point& lhs, const point& rhs);
        CORE_API t_ostream& operator<<( t_ostream& os, const point& pt );
        CORE_API bool fuzzy_equal( const double& lhs, const double& rhs, double epsilon = 10e-6 );
    }
}

#endif // _CORE_POINT_H_
