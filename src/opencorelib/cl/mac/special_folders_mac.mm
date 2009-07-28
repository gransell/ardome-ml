
#include <precompiled_headers.hpp>
#include <boost/cstdint.hpp>
#include <boost/filesystem.hpp>

#include <dlfcn.h>

#include <opencorelib/cl/minimal_string_defines.hpp>

#import <Foundation/NSString.h>
#import <Foundation/NSAutoreleasePool.h>
#import <Foundation/NSPathUtilities.h>
#import <Foundation/NSBundle.h>


namespace olib {
	namespace opencorelib {
		namespace mac {

			std::string get_home_dir()
			{
				NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
				const char *home = [NSHomeDirectory() cStringUsingEncoding:NSUTF8StringEncoding];
				std::string ret(home);
				[pool release];
				return ret;
			}
			
			
			std::string get_temporary_dir()
			{
//				NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
//				const char *tmp = [NSTemporaryDirectory() cStringUsingEncoding:NSUTF8StringEncoding];
//                std::string ret(tmp);
//				[pool release];
				return std::string(_CT("/tmp/")); 
			}
            
            std::string get_module_directory()
            {
                std::string ret;
                Dl_info info;
                int res = dladdr( __builtin_return_address(0), &info );
                if( res ) {
                    // Remove last part of the path since that is the dylib file name
                    boost::filesystem::path temp_path(info.dli_fname);
                    temp_path.remove_filename();
                    ret = temp_path.string();
                }
				return ret;
            }
		}
	}
}
