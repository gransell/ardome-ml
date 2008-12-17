

#define id _id_avoid_cpp_conflicts
#undef Nil

#include <boost/shared_ptr.hpp>
#include "opencorelib/cl/core.hpp"
#include "opencorelib/cl/machine_readable_errors.hpp"
#include "opencorelib/cl/assert_defines.hpp"
#include "opencorelib/cl/enforce_defines.hpp"
#include "opencorelib/cl/loghandler.hpp"
#include "opencorelib/cl/log_defines.hpp"
#include "opencorelib/cl/logtarget.hpp"
#include "opencorelib/cl/base_exception.hpp"
#include "opencorelib/cl/special_folders.hpp"
#include "opencorelib/cl/plugin_loader.hpp"
#include "opencorelib/cl/central_plugin_factory.hpp"

#undef id

#import "CoreConverts.h"
#import "OlibOpencorelibRationalTime.h"


OlibOpencorelibRationalTime *Convert( const olib::opencorelib::rational_time& rt )
{
    return [[[OlibOpencorelibRationalTime alloc] initWithNumerator:rt.numerator() andDenominator:rt.denominator()] autorelease];
}

olib::opencorelib::rational_time Convert( OlibOpencorelibRationalTime *rt )
{
    return olib::opencorelib::rational_time( [rt numerator], [rt denominator] );
}

NSPoint Convert( const olib::opencorelib::point& p )
{
    return NSMakePoint( p.get_x(), p.get_y() );
}

olib::opencorelib::point Convert( NSPoint p )
{
    return olib::opencorelib::point( p.x, p.y );
}

NSString *Convert( const std::string& str )
{
    return [[[NSString alloc] initWithUTF8String:str.c_str()] autorelease];
}

std::string Convert( NSString *str )
{
    return std::string([str UTF8String]);
}

NSUInteger Convert( const olib::opencorelib::frames& f )
{
    return f.get_frames();
}

olib::opencorelib::frames Convert( NSUInteger f )
{
    return olib::opencorelib::frames(f);
}



