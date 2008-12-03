

#import "CoreCovnerts.h"
#import "OlibOpencorelibRationalTime.h"


OlibOpencorelibRationalTime *Convert( const olib::opencorelib::rational_time& rt )
{
    return [[[OlibOpencorelibRationalTime alloc] initWithNumerator:rt.numerator() andDenominator:rt.denominator()] autorelease];
}

olib::opencorelib::rational_time Convert( OlibOpencorelibRationalTime rt )
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

