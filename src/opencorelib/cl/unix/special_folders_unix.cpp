#include "../precompiled_headers.hpp"
#include <opencorelib/cl/str_util.hpp>
#include <opencorelib/cl/special_folders.hpp>
#include <opencorelib/cl/exception_context.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/enforce.hpp>
#include <opencorelib/cl/log_defines.hpp>
#include <opencorelib/cl/logger.hpp>
#include <opencorelib/cl/loghandler.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <stdlib.h>

namespace bfs = boost::filesystem;

namespace olib
{
    namespace opencorelib
    { 
        namespace special_folder 
        {
            namespace 
            {
			
                olib::t_path get_module_path()
                {
                    return olib::t_path( "" );
                }

                std::string get_prefix() {
                    const char* env_prefix = getenv("AMF_PREFIX_PATH");
                    ARLOG_DEBUG4("Compiled prefix: %1%, env %2% = %3%")
                        (PREFIX)("AMF_PREFIX_PATH")(env_prefix);
                    
                    if (env_prefix && *env_prefix) {
                        ARLOG_DEBUG("Using prefix %1% from env")(env_prefix);
                        return env_prefix;
                    } else {
                        // TODO Try to deduce the prefix from /proc/$pid/exe
                        // ... before falling back to the compile-time PREFIX.
                        ARLOG_DEBUG("Using compilation prefix " PREFIX);
                        return PREFIX;
                    }
                }
                
                std::string get_home() {
                    const char* home_env = getenv( "HOME" );
                    if (home_env && *home_env)
                        return home_env;
                    else
                        return "/";
                }
            }
            
            olib::t_path get(  type folder_type, const t_string& app_name )
            {
                static std::string prefix = get_prefix();
				static std::string home = get_home();

                if( folder_type == special_folder::data ) 
                    return get_module_path();
                else if( folder_type == special_folder::local_data ) 
                    return get_module_path();
                else if( folder_type == special_folder::plugins ) 
                    return olib::t_path( prefix ) / "share" / app_name / "plugins";
                else if( folder_type == special_folder::user_config ) 
                    return olib::t_path( home ) / olib::t_path( "/.amf/" );
                else if( folder_type == special_folder::user_data ) 
                    return get(user_config, app_name) / app_name;
                else if( folder_type == special_folder::temp ) 
                    return olib::t_path( "/tmp" );
                else if( folder_type == special_folder::home ) 
                    return olib::t_path( home );
                else if( folder_type == special_folder::amf_resources ) 
                    return olib::t_path( prefix ) / "share" / app_name;
                
                olib::t_path fpath( "" );
                fpath /= app_name;
                bfs::create_directory(fpath);
                return fpath;
            }
        }
    }
}


