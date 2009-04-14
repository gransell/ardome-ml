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
#include <boost/regex.hpp>
#include <opencorelib/cl/minimal_string_defines.hpp>
 
#include <execinfo.h>
#include <stdio.h>
#include <iostream>

namespace abi {
extern "C" char* __cxa_demangle (const char* mangled_name,
                                 char* buf,
                                 size_t* n,
                                 int* status);
    
}

namespace olib {
	namespace opencorelib {
		namespace mac {

			utf8_string get_stack_trace()
			{   
                using namespace abi;
                
                void* callstack[128];
                int i, frames = backtrace(callstack, 128);
                char** strs = backtrace_symbols(callstack, frames);
                
                utf8_string trace = "";
                int status = 0;
                
                // spaces (line number) spaces (library) spaces (adr) space (routine)
                boost::regex e("\\s*(\\d+)\\s+(\\S+)\\s+(\\S+)\\s*(\\S+)(.+)");
                boost::smatch what;
                
                for (i = 0; i < frames; ++i) {
                    trace += "\r\n";
                    
                    utf8_string line(strs[i]);
                    
                    if( boost::regex_match( line, what, e, boost::match_extra ) ) {
                    
                        // Demangle name
                        char demangled[4096];
                        size_t l = sizeof(demangled);
                        std::string routine( what[4] );
                        __cxa_demangle( routine.c_str() , demangled, &l, &status );
                        if( status < 0 ) {
                            // Name could not be demangled. Use original one
                            trace += strs[i];
                        }
                        else {
                            trace += std::string(what[1]) + "\t" + std::string(what[2]) + "\t" + demangled;
                        }
                    }
                }
                trace += "\r\n";
                
                free(strs);

				return trace;
			}
		}
	}
}
