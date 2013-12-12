
// ml::image - casting functionality for image types

// Copyright (C) 2009 Ardendo
// Released under the LGPL.

#ifndef AML_IMAGE_CAST_H_
#define AML_IMAGE_CAST_H_

#include <openmedialib/ml/image/image.hpp>

namespace olib { namespace openmedialib { namespace ml { namespace image {

template < typename T >
inline boost::shared_ptr< T > coerce( const image_type_ptr &image )
{
    return boost::dynamic_pointer_cast< T >( image );
}

} } } }

#endif
