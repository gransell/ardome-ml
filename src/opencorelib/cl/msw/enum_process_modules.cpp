#include "precompiled_headers.hpp"
#include "./enum_process_modules.hpp"
#include "opencorelib/cl/enforce.hpp"
#include "opencorelib/cl/enforce_defines.hpp"
#include "opencorelib/cl/guard_define.hpp"
#include "opencorelib/cl/base_exception.hpp"
#include <boost/bind.hpp>
#include <Psapi.h>

namespace olib
{
	namespace opencorelib
	{
		namespace win
		{

			namespace 
			{

				t_string extract_version(LPVOID pVersionBlock)
				{
					VS_FIXEDFILEINFO *pVersionInfo;
					UINT uiLength;
					if( ::VerQueryValue(pVersionBlock, _CT("\\"), (LPVOID *) &pVersionInfo, &uiLength) ) 
					{
						int iMajor = pVersionInfo->dwFileVersionMS >> 16;
						int iMinor1 = pVersionInfo->dwFileVersionMS & 0xffff;
						int iMinor2 = pVersionInfo->dwFileVersionLS >> 16;

						t_stringstream strOutStream;
						strOutStream << iMajor << _CT(".") << iMinor1;
						if (iMinor2)
						{
							strOutStream << _CT(".") << iMinor2;
						}

						return strOutStream.str();
					}

					return t_string();
				}

				t_string extract_build_number(LPVOID pVersionBlock) 
				{
					VS_FIXEDFILEINFO *pVersionInfo;
					UINT uiLength;
					if( ::VerQueryValue(pVersionBlock, _CT("\\"), (LPVOID *) &pVersionInfo, &uiLength)) 
					{
						int iBuild = pVersionInfo->dwFileVersionLS & 0xffff;

						t_stringstream strOutStream;
						strOutStream << iBuild;
						return strOutStream.str();
					}

					return t_string();
				}

				// Get a specific ver info item (CompanyName, etc)
				t_string get_item(LPCTSTR szItem, LPCTSTR szTransBlock, LPVOID pVersionBlock)
				{
					// Format sub-block
					TCHAR szSubBlock[1024];
					::wsprintf( szSubBlock, _CT("%s%s"), szTransBlock, szItem );

					// Get result
					TCHAR *szOut = NULL;
					UINT nLength = 0;
					if( ::VerQueryValue(pVersionBlock, szSubBlock, (LPVOID*)&szOut, &nLength)) 
					{
						return t_string(szOut);
					}

					return t_string();
				}

				t_string get_module_file_name( HANDLE proc, HMODULE mod )
				{
					std::vector<TCHAR> buff(MAX_PATH, 0);

					// Get the full path to the module's file.
					ARENFORCE_WIN( ::GetModuleFileNameEx( proc, mod, &buff[0], buff.size() * sizeof(TCHAR)) != 0 );
					return t_string( &buff[0] );
				}

				void update_module_info( library_info_ptr info )
				{

					ARENFORCE( info );
					DWORD unused(0);
					DWORD info_size = ::GetFileVersionInfoSize( info->get_filename().c_str(), &unused );
					if( info_size == 0 ) return;

					std::vector< char > data( info_size );
					ARENFORCE_WIN( ::GetFileVersionInfo( info->get_filename().c_str(), unused, data.size(), (void*)&data[0]) != 0 );

					// Get the language ID
					TCHAR  szLang[30];
					TCHAR *pszLang = szLang;
					UINT   nLen = 0;

					ARENFORCE_WIN(::VerQueryValue((LPVOID) &data[0], _CT("\\VarFileInfo\\Translation"), (LPVOID *)&pszLang, &nLen));

					/* The retrieved 'hex' value looks a little confusing, but
					essentially what it is, is the hexadecimal representation
					of a couple different values that represent the language
					and character set that we are wanting string values for.
					040904E4 is a very common one, because it means:
					US English, Windows MultiLingual character set
					Or to pull it all apart:
					04------        = SUBLANG_ENGLISH_USA
					--09----        = LANG_ENGLISH
					----04E4 = 1252 = Codepage for Windows:Multilingual
					*/

					// Swap the words so wsprintf() will print the lang-charset in the correct format
					*(LPDWORD)pszLang = MAKELONG(HIWORD(*(LPDWORD)pszLang), LOWORD(*(LPDWORD)pszLang));

					TCHAR szTransBlock[255];

					// Now save the whole string info including the language ID
					// and final backslash as m_szTransBlock
					::wsprintf(szTransBlock, _CT("\\StringFileInfo\\%08lx\\"), *(LPDWORD)(pszLang));

					// Assign members
					info->set_product(  get_item(_CT("ProductName"), szTransBlock, (LPVOID)&data[0]) );
					info->set_version( extract_version((LPVOID)&data[0]) );
					info->set_build_number( extract_build_number((LPVOID)&data[0]) );
					info->set_company( get_item(_CT("CompanyName"), szTransBlock, (LPVOID)&data[0]) );
				}


			}
			std::vector< library_info_ptr > enum_process_modules()
			{
				
				DWORD current_process_id = ::GetCurrentProcessId();
				HANDLE current_process_handle(0);
				ARENFORCE_WIN( current_process_handle = 
					::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, current_process_id ));

				ARGUARD( boost::bind<BOOL>( ::CloseHandle, current_process_handle ));
  
				std::vector< HMODULE > h_mods(1024);
				DWORD cb_needed;
				BOOL enum_ok = ::EnumProcessModules(	current_process_handle, &h_mods[0], 
														h_mods.size() * sizeof(HMODULE), &cb_needed);

				if( cb_needed > h_mods.size() * sizeof(HMODULE ) )
				{
					h_mods.resize( cb_needed / sizeof(HMODULE), 0 );
					enum_ok = ::EnumProcessModules(	current_process_handle, &h_mods[0], 
														h_mods.size() * sizeof(HMODULE), &cb_needed);
				}

				std::vector< library_info_ptr > res;

				for ( size_t i = 0; i < ( cb_needed / sizeof(HMODULE)) && i < h_mods.size() ; ++i )
				{
					try
					{
						t_path name( get_module_file_name( current_process_handle, h_mods[i] ) );
						library_info_ptr info( new library_info( name ));
						update_module_info( info );
						res.push_back(info);
					}
					catch (const base_exception& )
					{
						// Just ignore, this function can fail to some extent. 
						// The error will have been logged.
					}
				}

				return res;
			}
		}
	}
}

