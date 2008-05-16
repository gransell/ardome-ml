/*
 *  mav_invoker.cpp
 *  core
 *
 *  Created by Mikael Gransell on 2007-09-24.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#include <opencorelib/cl/typedefs.hpp>
#include "mac_invoker.hpp"

#import <Foundation/NSObject.h>
#import <Foundation/NSAutoreleasePool.h>
#import <Foundation/NSThread.h>

typedef struct _invoker_function_holder {
    /// Holds the function that we should call when invkoed.
    /// This is a function taking no arguments and returning and invoker_result.
    olib::opencorelib::invokable_function _func;
} invoker_function_holder;


@interface ACInvokerObject : NSObject {
    /// Holds the function that we should invoke.
    invoker_function_holder *_holder;
    /// Stores the result of the invoked function
    BOOL _result;
}

- (id)initWithFunction:(const olib::opencorelib::invokable_function *)ifh;

- (void)invoke;

- (int)result;

- (void)dealloc;

@end

@implementation ACInvokerObject

- (id)initWithFunction:(const olib::opencorelib::invokable_function *)fun
{
    if( (self = [super init]) ) {
        _result = 0;
        
        _holder = new invoker_function_holder;
        _holder->_func = *fun;
    }
    return self;
}


- (void)invoke
{
    try {
        _holder->_func();
        _result = YES;
    }
    catch( ... ) {
        _result = NO;
    }
}

- (int)result
{
    return _result;
}

- (void)finalize
{
    delete _holder;
    [super finalize];
}

- (void)dealloc
{
    delete _holder;
    [super dealloc];
}

@end



namespace olib {
    namespace opencorelib {
        namespace mac {
            
            int invoke_function_on_main_thread( const invokable_function& f )
            {
                NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
                
                ACInvokerObject *inv = [[[ACInvokerObject alloc] initWithFunction:&f] autorelease];
                [inv performSelectorOnMainThread:@selector(invoke) withObject:nil waitUntilDone:YES];
                int res = [inv result];
                
                [pool release];
                
                return res;
            }
                
        }
    }
}
