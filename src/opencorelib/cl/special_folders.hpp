#ifndef _CORE_SPECIAL_FOLDERS_H_
#define _CORE_SPECIAL_FOLDERS_H_

#include <boost/filesystem/path.hpp>
#include "./minimal_string_defines.hpp"

namespace olib
{
   namespace opencorelib
    {
        /// Encapsulates functions and enums related to special filesystem folders.
        namespace special_folder
        {
            enum type
            {	
                config,		    /**<    Examples:
                                        <ul>
                                        <li>Windows: <i>C:\\Documents and Settings\\All Users\\Application Data\\AppName </i>
                                        <li>Mac:
                                        <li>Linux:
                                        </ul> */
                data,		    /**< Examples:
                                        <ul>
                                        <li>Windows: <i>C:\\Documents and Settings\\username\\Application Data\\AppName</i>
                                        <li>Mac:
                                        <li>Linux:
                                        </ul> */
                local_data,	    /**< Examples:
                                        <ul>
                                        <li>Windows: <i>C:\\Documents and Settings\\username\\Local Settings\\Application Data\\AppName</i>
                                        <li>Mac:
                                        <li>Linux:
                                        </ul> */
                plugins,	    /**< Examples:
                                        <ul>
                                        <li>Windows: The folder where EasyCut stores its amf-plugins.
                                                    <i>C:\\Program Files\\Ardendo\\EasyCut\\bin\\plugins </i>
                                        <li>Mac:
                                        <li>Linux:
                                        </ul>*/
                user_config,	/**< Examples:
                                        <ul>
                                        <li>Windows: <i>HOME</i>
                                        <li>Mac:
                                        <li>Linux:
                                        </ul>*/
                user_data,      /**< Examples:
                                        <ul>
                                        <li>Windows: <i>HOME\\AppName</i>
                                        <li>Mac:
                                        <li>Linux:
                                        </ul>*/
                user_local_data,/**< Examples:
                                        <ul>
                                        <li>Windows: <i>C:\\Documents and Settings\\username\\Local Settings\\Application Data\\AppName</i>
                                        <li>Mac:
                                        <li>Linux:
                                        </ul>*/
				temp,           /**< Examples:
                                        <ul>
                                        <li>Windows: <i>TEMP</i>
                                        <li>Mac:
                                        <li>Linux:
                                        </ul>*/
				home,           /**< Examples:
                                        <ul>
                                        <li>Windows: <i>HOME</i>
                                        <li>Mac:
                                        <li>Linux:
                                        </ul>*/
                amf_resources /**< This folder is where the resources related to amf should be found.
                                    For instance, the xml-schemas used by amf should be
                                    found in a sub-folder of amf_resources called schemas.
                                    Examples:
                                    <ul>
                                    <li>Windows: The folder where amf.core.dll resides
                                    <li>Mac: the app folder
                                    <li>Linux: /usr/local/share/amf/1.0/ 
                                    </ul>*/
            };
    
            /// Get the location of one of the special folders.
            /** @param folder_type The kind of folder whose location is required.
                @param app_name The name of the application. This is not always used 
                        by the get function but could be so.
                @return A path to the folder. If it doesn't exist before the 
                        call to get, get ensures its existence. */
            CORE_API olib::t_path get(  type folder_type, 
                                                    const t_string& app_name = _T("amf") );
        }
    }
}


#endif // _CORE_SPECIAL_FOLDERS_H_

