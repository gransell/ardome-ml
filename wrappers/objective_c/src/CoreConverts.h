
#ifndef _CORE_CONVERTS_H_
#define _CORE_CONVERTS_H_

#include <opencorelib/cl/typedefs.hpp>

@class OlibOpencorelibRationalTime;

OlibOpencorelibRationalTime *Convert( const olib::opencorelib::rational_time& rt );
olib::opencorelib::rational_time Convert( OlibOpencorelibRationalTime *rt );

#endif // _CORE_CONVERTS_H_
