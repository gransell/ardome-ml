
#ifndef _OLIB_ERROR_H_
#define _OLIB_ERROR_H_

#import <Foundation/NSError.h>
#import <Foundation/NSString.h>

struct ExceptionHolder;

@interface OlibError : NSObject
{
    struct ExceptionHolder *_holder;
}

/// Amf error code for the error
@property (readonly) NSString *errorCode;
/// A backtrace to where the error occured
@property (readonly) NSString *backTrace;
/// Name of the file in which the error occured
@property (readonly) NSString *fileName;
/// Name of the function in which the error occured
@property (readonly) NSString *function;
/// Line number in the file at which the error occured
@property (readonly) long lineNumber;


@end

#endif // _OLIB_ERROR_H_


