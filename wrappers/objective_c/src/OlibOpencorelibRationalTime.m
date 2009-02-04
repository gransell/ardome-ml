
#import "OlibOpencorelibRational.h"

#import <Foundation/Foundation.h>

@implementation OlibOpencorelibRational

@synthesize numerator = _numerator;
@synthesize denominator = _denominator;

- (id)initWithNumerator:(NSInteger)num andDenominator:(NSInteger)den;
{
    self = [super init];
    if (self != nil) {
        _numerator = num;
        _denominator = den;
    }
    return self;
}

- (int)intValue;
{
    return round((double)_numerator / (double)_denominator);
}

- (double)doubleValue
{
    return (double)_numerator / (double)_denominator;
}

- (void)increaseWithValue:(double)toAdd
{
    _numerator += toAdd * _denominator;
}

- (void)decreaseWithValue:(double)toSubtract
{
    _numerator -= toSubtract * _denominator;
}

@end
