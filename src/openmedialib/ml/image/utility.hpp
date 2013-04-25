/*
* ml - A media library representation.
* Copyright (C) 2013 Vizrt
* Released under the LGPL.
*/
#ifndef AML_IMAGE_UTILITIES_H_
#define AML_IMAGE_UTILITIES_H_

#include <openmedialib/ml/types.hpp>
namespace olib { namespace openmedialib { namespace ml { namespace image {

extern ML_DECLSPEC image_type_ptr allocate ( MLPixelFormat pf, int width, int height );
extern ML_DECLSPEC image_type_ptr allocate ( const olib::t_string pf, int width, int height );

extern ML_DECLSPEC image_type_ptr convert( const image_type_ptr &src, const MLPixelFormat pf );

} } } }

#endif
