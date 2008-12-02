
#ifndef _RATIONAL_TIME_H_
#define _RATIONAL_TIME_H_

#import <Foundation/NSObject.h>


@interface OlibOpencorelibRationalTime : NSObject {
@private
    NSInteger _numerator;
    NSInteger _denominator;
}

- (id)initWithNumerator:(NSInteger)num andDenominator:(NSInteger)den;

@property (readwrite) NSInteger numerator;
@property (readwrite) NSInteger denominator;

/**
 * Get the value of this rational as an integer.
 * @returns Integer value of rational
 */
- (int)intValue;

/**
 * Get the value of this rational as a double.
 * @returns Double value of rational
 */
- (double)doubleValue;

/**
 * Increase the value of this rational with the specified valu.
 * @returns Poiner to self.
 */
- (void)increaseWithValue:(double)toAdd;

- (void)decreaseWithValue:(double)toSubtract;

@end

#endif
