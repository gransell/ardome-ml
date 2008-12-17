
#ifndef _CORE_CONVERTS_H_
#define _CORE_CONVERTS_H_

#include <string>

#include <opencorelib/cl/typedefs.hpp>
#include <opencorelib/cl/point.hpp>
#include <opencorelib/cl/frames.hpp>

#import <Foundation/Foundation.h>

@class OlibOpencorelibRationalTime;

OlibOpencorelibRationalTime *Convert( const olib::opencorelib::rational_time& rt );
olib::opencorelib::rational_time Convert( OlibOpencorelibRationalTime *rt );

NSPoint Convert( const olib::opencorelib::point& rt );
olib::opencorelib::point Convert( NSPoint rt );

NSString *Convert( const std::string& str );
std::string Convert( NSString *str );

NSUInteger Convert( const olib::opencorelib::frames& f );
olib::opencorelib::frames Convert( NSUInteger f );


#endif // _CORE_CONVERTS_H_
