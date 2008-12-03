
#ifndef _CORE_CONVERTS_H_
#define _CORE_CONVERTS_H_


#include <opencorelib/cl/typedefs.hpp>
#include <opencorelib/cl/point.hpp>

#import <Foundation/Foundation.h>

@class OlibOpencorelibRationalTime;

OlibOpencorelibRationalTime *Convert( const olib::opencorelib::rational_time& rt );
olib::opencorelib::rational_time Convert( OlibOpencorelibRationalTime *rt );

NSPoint Convert( const olib::opencorelib::point& rt );
olib::opencorelib::point Convert( NSPoint rt );

#endif // _CORE_CONVERTS_H_
