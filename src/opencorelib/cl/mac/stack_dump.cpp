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
 
#include <execinfo.h>
#include <stdio.h>

namespace olib {
	namespace opencorelib {
		namespace mac {

			utf8_string get_stack_trace()
			{   
                void* callstack[128];
                int i, frames = backtrace(callstack, 128);
                char** strs = backtrace_symbols(callstack, frames);
                
                utf8_string trace = "";
                
                for (i = 0; i < frames; ++i) {
                    trace += strs[i];
                    trace += "\n";
                }
                free(strs);

				return trace;
			}
		}
	}
}
