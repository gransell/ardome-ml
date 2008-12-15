#ifndef _CORE_SIZE_H_
#define _CORE_SIZE_H_

#include <boost/operators.hpp>

namespace olib
{
   namespace opencorelib
    {
        /// Describes the size of something (width/height) 
        class CORE_API size : 
            public boost::equality_comparable< size >,
            public boost::addable< size >,
            public boost::subtractable< size >
        {
        public:
            /// Create a size object with width and height set to w and h.
            size( double w, double h );

            /// Create a size object with widht and height set to dims[0] and dims[1]
            explicit size( const double* dims );

            /// Copy a size object into a new size object
            size( const size& other );

            /// Assign a size object to this size object.
            size& operator=( const size& other);

            /// Get the width of this size.
            double get_width() const { return m_dims[0]; }

            /// Set the width of this size
            void set_width( double w ) { m_dims[0] = w ; }

            /// Get the height of this size.
            double get_height() const { return m_dims[1]; }

            /// Set the height of this size
            void set_height( double h ) { m_dims[1] = h ; }

            /// Get the width and height as a two element array object.
            const double* get_dimensions() const { return m_dims; }

            CORE_API friend bool operator==( const size& lhs, const size& rhs );

            size& operator+=(const size& other)
            {
                m_dims[0] += other.m_dims[0];
                m_dims[1] += other.m_dims[1];
                return *this;
            }

            size& operator-=(const size& other)
            {
                m_dims[0] -= other.m_dims[0];
                m_dims[1] -= other.m_dims[1];
                return *this;
            }

        private:
            double m_dims[2];

        };

        CORE_API bool operator==( const size& lhs, const size& rhs );
    }
}

#endif // _CORE_SIZE_H_

