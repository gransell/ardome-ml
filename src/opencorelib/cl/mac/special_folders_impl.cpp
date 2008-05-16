#include "../precompiled_headers.hpp"
#include <opencorelib/cl/special_folders.hpp>
#include <opencorelib/cl/exception_context.hpp>
#include <opencorelib/cl/enforce_defines.hpp>
#include <opencorelib/cl/enforce.hpp>
#include <opencorelib/cl/str_util.hpp>

#include <boost/filesystem/operations.hpp>

#include "special_folders_mac.hpp"


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
            }
            
			olib::t_path get(  type folder_type, 
                                           const t_string& app_name )
            {
                if( folder_type == special_folder::data )
                    return olib::t_path( _T("/Library/Application Data/") );
                else if( folder_type == special_folder::local_data )
                    return get_module_path();
                else if( folder_type == special_folder::plugins )
                    return get( user_config, app_name ) / _T("Contents/Plugins");
                else if( folder_type == special_folder::user_config )
                    return olib::t_path( str_util::to_t_string(mac::get_home_dir()) ) / 
                        olib::t_path( _T("/Library/Application Data/") ) / 
                        str_util::to_t_string(app_name);
                else if( folder_type == special_folder::user_data )
                    return get(user_config, app_name) / str_util::to_t_string(app_name);
                else if( folder_type == special_folder::temp )
                    return olib::t_path( str_util::to_t_string(mac::get_temporary_dir()) );
                else if( folder_type == special_folder::home )
                    return olib::t_path( str_util::to_t_string(mac::get_home_dir()) );
                else if( folder_type == special_folder::amf_resources )
                    return olib::t_path( str_util::to_t_string(mac::get_module_directory()) ) /
                        _T("Resources");
                
				olib::t_path fpath( "" );
                fpath /= str_util::to_string(app_name);
                bfs::create_directory(fpath);
                return fpath;
            }
        }
    }
}


