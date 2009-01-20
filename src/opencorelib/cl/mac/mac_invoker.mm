/*
 *  mav_invoker.cpp
 *  core
 *
 *  Created by Mikael Gransell on 2007-09-24.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#include <boost/shared_ptr.hpp>

#include "opencorelib/cl/export_defines.hpp"
#include "opencorelib/cl/typedefs.hpp"
#include "opencorelib/cl/invoker.hpp"
#include "mac_invoker.hpp"

#import <Foundation/NSObject.h>
#import <Foundation/NSAutoreleasePool.h>
#import <Foundation/NSThread.h>

 typedef boost::shared_ptr<olib::opencorelib::invokable_function> mac_invokable_function_ptr;

typedef struct _invoker_function_holder {
    /// Holds the function that we should call when invkoed.
    /// This is a function taking no arguments and returning and invoker_result.
     mac_invokable_function_ptr _func;
     olib::opencorelib::invoke_callback_function_ptr _cb;
} invoker_function_holder;


@interface ACInvokerObject : NSObject {
    /// Holds the function that we should invoke.
    invoker_function_holder *_holder;
    /// Stores the result of the invoked function
    BOOL _result;
}

- (id)initWithFunction:(const olib::opencorelib::invokable_function *)ifh andCallback:(olib::opencorelib::invoke_callback_function_ptr)cb;

- (void)invoke;

- (int)result;

- (void)dealloc;

@end

@implementation ACInvokerObject

- (id)initWithFunction:(const olib::opencorelib::invokable_function *)fun andCallback:(olib::opencorelib::invoke_callback_function_ptr)cb
{
    if( (self = [super init]) ) {
        _result = 0;
        
        _holder = new invoker_function_holder;
        _holder->_func = mac_invokable_function_ptr( new olib::opencorelib::invokable_function(*fun) );
        _holder->_cb = cb;
    }
    return self;
}


- (void)invoke
{
    olib::opencorelib::std_exception_ptr e_ret;
    try {
        (*(_holder->_func))();
        _result = YES;
    }
    catch( const std::exception& e ) {
        _result = NO;
        e_ret = olib::opencorelib::std_exception_ptr( new std::exception(e) );
    }
    catch( ... ) {
        _result = NO;
    }
    if( _holder->_cb ) {
        (*(_holder->_cb))( _result ? olib::opencorelib::invoke_result::success : olib::opencorelib::invoke_result::failure, e_ret );
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
            
            int invoke_function_on_main_thread( const invokable_function& f, 
                                                const olib::opencorelib::invoke_callback_function_ptr& cb,
                                                bool block )
            {
                NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
                
                ACInvokerObject *inv = [[[ACInvokerObject alloc] initWithFunction:&f andCallback:cb] autorelease];
                [inv performSelectorOnMainThread:@selector(invoke) withObject:nil waitUntilDone:block ? YES : NO];
                int res = [inv result];
                
                [pool release];
                
                return res;
            }
                
        }
    }
}
