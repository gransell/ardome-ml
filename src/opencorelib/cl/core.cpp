// core.cpp : Defines the entry point for the DLL application.
//

#include "precompiled_headers.hpp"

#if defined(OLIB_ON_WINDOWS) 

#include "./core.hpp"
#include "./config.hpp"

HMODULE g_dll_handle = 0;

HMODULE get_dll_module_handle()
{
    if( g_dll_handle == 0 ) return ::GetModuleHandle(NULL); 
    return g_dll_handle;
}

	#if defined(DYNAMIC_BUILD)
	BOOL WINAPI DllMain( HANDLE h_module, 
	                       DWORD  ul_reason_for_call, 
	                       LPVOID lp_reserved
						 )
	{
		ARUNUSED_ALWAYS((ul_reason_for_call, lp_reserved));
	    g_dll_handle = (HMODULE)h_module;
		switch (ul_reason_for_call)
		{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
		}
	    return TRUE;
	}
	#endif // DYNAMIC_BUILD

#endif
