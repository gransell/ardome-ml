/*
 *  stack_dump.mm
 *  au_base
 *
 *  Created by Mikael Gransell on 3/20/07.
 *  Copyright 2007 Ardendo AB. All rights reserved.
 *
 */

#include "stack_dump.hpp"
#include <boost/cstdint.hpp>
#include <opencorelib/cl/minimal_string_defines.hpp>

#import <Foundation/NSString.h>
#import <Foundation/NSAutoreleasePool.h>
#import <Foundation/NSProcessInfo.h>
#import <Foundation/NSException.h>
#import <Foundation/NSDictionary.h>
#import <ExceptionHandling/NSExceptionHandler.h>

// Add this to your application code if you want this to have any effect
//[[NSExceptionHandler defaultExceptionHandler] setExceptionHandlingMask:NSLogAndHandleEveryExceptionMask];

namespace olib {
	namespace opencorelib {
		namespace mac {
			namespace  {
				NSString *translate_trace( NSException *e )
				{
					NSString *ret = [NSString string];
					NSString *stackTrace = [[e userInfo] objectForKey:NSStackTraceKey];
                    
                    NSString *str = [NSString stringWithFormat:@"/usr/bin/atos -p %d %@ | tail -n +6 | head -n +%d | c++filt --no-verbose --no-params | cat -n",
                        [[NSProcessInfo processInfo] processIdentifier], 
                        stackTrace, 
                        ([[stackTrace componentsSeparatedByString:@"  "] count] - 4)];

					if( FILE *file = popen( [str UTF8String], "r" ) ) {
						char buffer[512];
						size_t length;
					
						while( (length = fread( buffer, 1, sizeof( buffer ), file )) ) {
							ret = [ret stringByAppendingFormat:@"%s", buffer];
						}
					
						pclose( file );
					}
					return ret;
				}
			
			}
		

			utf8_string get_stack_trace()
			{
				NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
				// Make sure that stack trace is included in exception
				[[NSExceptionHandler defaultExceptionHandler] setExceptionHandlingMask:NSLogAndHandleEveryExceptionMask];
				NSString *temp = nil;	
				@try {
					[NSException raise:@"StackTraceException" format:@"Dummy"];
				}
				@catch( NSException *e ) {
					temp = translate_trace( e );
				}
				
				utf8_string trace = "";
				if( temp ) {
					trace = [temp cStringUsingEncoding:NSUTF8StringEncoding];
				}
				[pool release];

				return trace;
			}
		}
	}
}
