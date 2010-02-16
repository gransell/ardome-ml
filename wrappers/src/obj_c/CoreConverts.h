
#ifndef _CORE_CONVERTS_H_
#define _CORE_CONVERTS_H_

#include <string>

#include <opencorelib/cl/typedefs.hpp>
#include <opencorelib/cl/point.hpp>
#include <opencorelib/cl/frames.hpp>

#import <Foundation/Foundation.h>

@class OlibRationalTime;

OlibRationalTime *ConvertRationalTime( const olib::opencorelib::rational_time& rt );
olib::opencorelib::rational_time ConvertRationalTime( OlibRationalTime *rt );

NSPoint ConvertPoint( const olib::opencorelib::point& rt );
olib::opencorelib::point ConvertPoint( NSPoint rt );

NSString *ConvertString( const std::string& str );
std::string ConvertString( NSString *str );

NSUInteger ConvertFrames( const olib::opencorelib::frames& f );
olib::opencorelib::frames ConvertFrames( NSUInteger f );


#endif // _CORE_CONVERTS_H_
