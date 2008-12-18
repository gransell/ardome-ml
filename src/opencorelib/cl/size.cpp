#include "precompiled_headers.hpp"
#include "./size.hpp"

namespace olib
{
   namespace opencorelib
    {
        size::size( double w, double h )
        {
            m_dims[0] = w;
            m_dims[1] = h;
        }

        size::size( const double* dims )
        {
            std::copy( dims, dims + 2, m_dims );
        }

        size::size( const size& other )
        {
            std::copy( other.m_dims, other.m_dims + 2, m_dims );
        }

        size& size::operator=( const size& other)
        {
            std::copy( other.m_dims, other.m_dims + 2, m_dims );
            return *this;
        }

        CORE_API bool operator==( const size& lhs, const size& rhs )
        {
            return lhs.m_dims[0] == rhs.m_dims[0] &&
                        lhs.m_dims[1] == rhs.m_dims[1];
        }

    }
}
