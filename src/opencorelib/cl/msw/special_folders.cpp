#include "precompiled_headers.hpp"
#include "../special_folders.hpp"
#include "../exception_context.hpp"
#include "../enforce_defines.hpp"
#include "../enforce.hpp"
#include "../str_util.hpp"
#include "../loghandler.hpp"
#include "../logger.hpp"
#include "../log_defines.hpp"
#include "../assert_defines.hpp"
#include "../assert.hpp"

// msw version of standard_paths.cpp
#ifdef OLIB_ON_WINDOWS 
#include <shlobj.h>
#include <boost/filesystem/operations.hpp>

using namespace olib::opencorelib::str_util;
namespace fs = boost::filesystem;

HMODULE get_dll_module_handle();

namespace olib
{
   namespace opencorelib
    { 
        namespace 
        {
            olib::t_path get_this_modules_path()
            {
                HMODULE h_mod = get_dll_module_handle();
                wchar_t buff[MAX_PATH];
                ARENFORCE_WIN( ::GetModuleFileNameW(h_mod, buff, MAX_PATH) );

                std::wstring module_path(buff);
                olib::t_path full_path( module_path );
                return full_path.remove_filename();
            }

            olib::t_path get_module_path( const t_string& module_name )
            {
                HMODULE h_mod;
                ARENFORCE_WIN( ( h_mod = ::GetModuleHandle(module_name.c_str())) != 0 )(module_name);
                wchar_t buff[MAX_PATH];
                ARENFORCE_WIN( ::GetModuleFileNameW(h_mod, buff, MAX_PATH) );

                std::wstring module_path(buff);
                ARENFORCE( module_path.size() > module_name.size() )(module_name).msg(_T("Path not found"));
                std::wstring lib_path(module_path,0, module_path.size() - module_name.size());

                return olib::t_path( lib_path );
            }

            olib::t_path get_home_dir()
            {
                // If we have a valid HOME directory, as is used on many machines that
                // have unix utilities on them, we should use that.
                t_string home_dir(_T("HOME"));
                if( str_util::env_var_exists(home_dir) )
                {
                    home_dir = str_util::get_env_var( home_dir );
                    home_dir = str_util::expand_env_vars(home_dir);
                    return olib::t_path( home_dir );
                }

                t_string home_drive(_T("HOMEDRIVE")), home_path(_T("HOMEPATH"));
                if( str_util::env_var_exists(home_drive) && str_util::env_var_exists(home_path))
                {
                    // the idea is that under NT these variables have default values
                    // of "%systemdrive%:" and "\\". As we don't want to create our
                    // config files in the root directory of the system drive, we will
                    // create it in our program's dir. However, if the user took care
                    // to set HOMEPATH to something other than "\\", we suppose that he
                    // knows what he is doing and use the supplied value.
                    home_drive = str_util::get_env_var(home_drive);
                    home_path = str_util::get_env_var(home_path);
                    if(home_path.compare(_T("\\")) != 0)
                    {
                        olib::t_path res( home_drive + home_path);
                        ARASSERT( fs::is_directory(res) )(res);
                        return res;
                    }
                }
                
                // If we have a valid USERPROFILE directory, as is the case in
                // Windows NT, 2000 and XP, we should use that as our home directory.
                t_string user_profile(_T("USERPROFILE"));
                if( str_util::env_var_exists(user_profile) )
                {
                    user_profile = str_util::get_env_var( user_profile );
                    olib::t_path res( to_wstring(user_profile) ) ;
                    ARASSERT( fs::is_directory(res) )(res);
                    return res;
                }
                
                return get_this_modules_path();
            }

            olib::t_path get_temp_dir()
            {
                t_string temp_env(_T("TEMP"));
                if( str_util::env_var_exists(temp_env))
                {
                    t_string  temp_path = str_util::get_env_var(temp_env);
                    olib::t_path wp( to_wstring( temp_path) );
                    fs::create_directory(wp);
                    return wp;
                }
                
                ARLOG_ERR(_T("Could not find TEMP environment variable"));
                olib::t_path wp = get( special_folder::user_data ) / L"temp";
                fs::create_directory(wp);
                return wp;
            }
        }

        CORE_API olib::t_path special_folder::get(  special_folder::type sf,
                                                 const t_string& app_name )
        {
            HRESULT hr(S_OK); 
            DWORD desired_folder = CSIDL_FLAG_CREATE;

            if( sf == special_folder::amf_resources ) return get_this_modules_path(); 
            else if( sf == special_folder::config ) desired_folder |= CSIDL_COMMON_APPDATA;
            else if( sf == special_folder::data ) desired_folder |= CSIDL_APPDATA;
            else if( sf == special_folder::local_data ) desired_folder |= CSIDL_LOCAL_APPDATA;
            else if( sf == special_folder::plugins ) return get_this_modules_path() / L"plugins";
            else if( sf == special_folder::user_config ) return get_home_dir();
            else if( sf == special_folder::user_data ) return get(user_config, app_name) / to_wstring(app_name) ;
            else if( sf == special_folder::temp ) return get_temp_dir();
            else /*( sf == special_folder::user_local_data )*/ desired_folder |= CSIDL_LOCAL_APPDATA;
            
            wchar_t sz_path[MAX_PATH];
            ARENFORCE_HR( (hr = ::SHGetFolderPathW(0, desired_folder, NULL, 0, sz_path)) );
            olib::t_path fpath( sz_path );
            if( !app_name.empty()) fpath /= app_name;
            fs::create_directory(fpath);
            return fpath;
        }
    }
}

#endif

