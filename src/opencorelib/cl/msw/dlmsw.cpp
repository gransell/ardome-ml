/////////////////////////////////////////////////////////////////////////////
// Name:        msw/dlmsw.cpp
// Purpose:     Win32-specific part of wxDynamicLibrary and related classes
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2005-01-10 (partly extracted from common/dynlib.cpp)
// RCS-ID:      $Id: dlmsw.cpp,v 1.10 2005/05/22 17:55:30 VZ Exp $
// Copyright:   (c) 1998-2005 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

//#include  "wx/wxprec.h"
#include "precompiled_headers.hpp"

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

//#include "wx/msw/private.h"
#include "../dynlib.hpp"
#include "./debughlp.hpp"
#include "../str_util.hpp"
#include "../utilities.hpp"

const olib::t_string wxDynamicLibrary::m_libext(_CT(".dll"));

HMODULE wxGetModuleHandle(const char *name, void *addr);

olib::t_string wxGetFullModuleName(HMODULE hmod)
{
    olib::t_string fullname;
    TCHAR buf[MAX_PATH];
    if( ! ::GetModuleFileName(hmod, buf,MAX_PATH) )
    {
        olib::opencorelib::utilities::handle_system_error(false);
    }
    else
    {
        fullname = buf;
    }

    return fullname;
}

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// wrap some functions from version.dll: load them dynamically and provide a
// clean interface
class wxVersionDLL
{
public:
    // load version.dll and bind to its functions
    wxVersionDLL();

    // return the file version as string, e.g. "x.y.z.w"
    olib::t_string GetFileVersion(const olib::t_string& filename) const;

private:
    typedef DWORD (APIENTRY *GetFileVersionInfoSize_t)(PTSTR, PDWORD);
    typedef BOOL (APIENTRY *GetFileVersionInfo_t)(PTSTR, DWORD, DWORD, PVOID);
    typedef BOOL (APIENTRY *VerQueryValue_t)(const PVOID, PTSTR, PVOID *, PUINT);

    #define DO_FOR_ALL_VER_FUNCS(what)                                        \
        what(GetFileVersionInfoSize);                                         \
        what(GetFileVersionInfo);                                             \
        what(VerQueryValue)

    #define DECLARE_VER_FUNCTION(func) func ## _t m_pfn ## func

    DO_FOR_ALL_VER_FUNCS(DECLARE_VER_FUNCTION);

    #undef DECLARE_VER_FUNCTION


    wxDynamicLibrary m_dll;

    wxVersionDLL( const wxVersionDLL& );
    wxVersionDLL& operator=(wxVersionDLL& );
};

// class used to create wxDynamicLibraryDetails objects
class wxDynamicLibraryDetailsCreator
{
public:
    // type of parameters being passed to EnumModulesProc
    struct EnumModulesProcParams
    {
        std::vector< wxDynamicLibraryDetails >* dlls;
        wxVersionDLL* verDLL;
    };

    // TODO: fix EnumerateLoadedModules() to use EnumerateLoadedModules64()
    #ifdef __WIN64__
        typedef DWORD64 DWORD_32_64;
    #else
        typedef DWORD DWORD_32_64;
    #endif

    static BOOL CALLBACK
    EnumModulesProc(PSTR name, DWORD_32_64 base, ULONG size, void *data);
};

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

// return the module handle for the given base name

HMODULE wxGetModuleHandle(const char *name, void *addr)
{
    // we want to use GetModuleHandleEx() instead of usual GetModuleHandle()
    // because the former works correctly for comctl32.dll while the latter
    // returns NULL when comctl32.dll version 6 is used under XP (note that
    // GetModuleHandleEx() is only available under XP and later, coincidence?)

    // check if we can use GetModuleHandleEx
    typedef BOOL (WINAPI *GetModuleHandleEx_t)(DWORD, LPCSTR, HMODULE *);

    static const GetModuleHandleEx_t INVALID_FUNC_PTR = (GetModuleHandleEx_t)-1;

    static GetModuleHandleEx_t s_pfnGetModuleHandleEx = INVALID_FUNC_PTR;
    if ( s_pfnGetModuleHandleEx == INVALID_FUNC_PTR )
    {
        wxDynamicLibrary dll(_CT("kernel32.dll"), wxDL_VERBATIM);
        s_pfnGetModuleHandleEx =
            (GetModuleHandleEx_t)dll.RawGetSymbol(_CT("GetModuleHandleExA"));

        // dll object can be destroyed, kernel32.dll won't be unloaded anyhow
    }

    // get module handle from its address
    if ( s_pfnGetModuleHandleEx )
    {
        // flags are GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
        //           GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
        HMODULE hmod;
        if ( s_pfnGetModuleHandleEx(6, (char *)addr, &hmod) && hmod )
            return hmod;
    }

    // Windows CE only has Unicode API, so even we have an ANSI string here, we
    // still need to use GetModuleHandleW() there and so do it everywhere to
    // avoid #ifdefs -- this code is not performance-critical anyhow...
    return ::GetModuleHandle(olib::opencorelib::str_util::to_t_string(name).c_str());
}

// ============================================================================
// wxVersionDLL implementation
// ============================================================================

// ----------------------------------------------------------------------------
// loading
// ----------------------------------------------------------------------------

wxVersionDLL::wxVersionDLL()
{
    if ( m_dll.Load(_CT("version.dll"), wxDL_VERBATIM) )
    {
        // the functions we load have either 'A' or 'W' suffix depending on
        // whether we're in ANSI or Unicode build
        #ifdef UNICODE
            #define SUFFIX L"W"
        #else // ANSI
            #define SUFFIX "A"
        #endif // UNICODE/ANSI

        #define LOAD_VER_FUNCTION(name)                                       \
            m_pfn ## name = (name ## _t)m_dll.GetSymbol(_CT(#name SUFFIX));    \
        if ( !m_pfn ## name )                                                 \
        {                                                                     \
            m_dll.Unload();                                                   \
            return;                                                           \
        }

        DO_FOR_ALL_VER_FUNCS(LOAD_VER_FUNCTION);

        #undef LOAD_VER_FUNCTION
    }
}

// ----------------------------------------------------------------------------
// wxVersionDLL operations
// ----------------------------------------------------------------------------

olib::t_string wxVersionDLL::GetFileVersion(const olib::t_string& filename) const
{
    olib::t_string ver;
    if ( m_dll.IsLoaded() )
    {
        TCHAR *pc = const_cast<TCHAR* >(filename.c_str());

        DWORD dummy;
        DWORD sizeVerInfo = m_pfnGetFileVersionInfoSize(pc, &dummy);
        if ( sizeVerInfo )
        {
            boost::shared_array< TCHAR > buf( new TCHAR[sizeVerInfo] );
            if ( m_pfnGetFileVersionInfo(pc, 0, sizeVerInfo, buf.get() ) )
            {
                void *pVer;
                UINT sizeInfo;
                if ( m_pfnVerQueryValue(buf.get(), _CT("\\"), &pVer, &sizeInfo) )
                {
                    VS_FIXEDFILEINFO *info = (VS_FIXEDFILEINFO *)pVer;
                    olib::t_format fmt(_CT("%d.%d.%d.%d"));
                    ver = (fmt % HIWORD(info->dwFileVersionMS) %
                        LOWORD(info->dwFileVersionMS) %
                        HIWORD(info->dwFileVersionLS) %
                        LOWORD(info->dwFileVersionLS)).str();
                }
            }
        }
    }
    //else: we failed to load DLL, can't retrieve version info

    return ver;
}

// ============================================================================
// wxDynamicLibraryDetailsCreator implementation
// ============================================================================

/* static */
BOOL CALLBACK
wxDynamicLibraryDetailsCreator::EnumModulesProc(PSTR name,
                                                DWORD_32_64 base,
                                                ULONG size,
                                                void *data)
{
    EnumModulesProcParams *params = (EnumModulesProcParams *)data;

    wxDynamicLibraryDetails *details = new wxDynamicLibraryDetails;

    #ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
        // conversion from 'wxDynamicLibraryDetailsCreator::DWORD_32_64' to 
        // 'void *' of greater size
        #pragma warning(push)
        #pragma warning(disable:4312)
    #endif
    // fill in simple properties
    details->m_name = olib::opencorelib::str_util::to_t_string(name);
    details->m_address = reinterpret_cast<void *>(base);
    details->m_length = size;

    // to get the version, we first need the full path
    HMODULE hmod = wxGetModuleHandle(name, (void *)base);
    
    #ifdef  OLIB_COMPILED_WITH_VISUAL_STUDIO
        #pragma warning(pop)
    #endif

    if ( hmod )
    {
        olib::t_string fullname = wxGetFullModuleName(hmod);
        if ( !fullname.empty() )
        {
            details->m_path = fullname;
            details->m_version = params->verDLL->GetFileVersion(fullname);
        }
    }

    params->dlls->push_back(*details);

    // continue enumeration (returning FALSE would have stopped it)
    return TRUE;
}

// ============================================================================
// wxDynamicLibrary implementation
// ============================================================================

// ----------------------------------------------------------------------------
// misc functions
// ----------------------------------------------------------------------------

wxDllType wxDynamicLibrary::GetProgramHandle()
{
    return (wxDllType)::GetModuleHandle(NULL);
}

// ----------------------------------------------------------------------------
// loading/unloading DLLs
// ----------------------------------------------------------------------------

/* static */
wxDllType
wxDynamicLibrary::RawLoad(const olib::t_string& libname, int flags)
{
    ARUNUSED_ALWAYS(flags);
    return ::LoadLibrary(libname.c_str());
}

/* static */
void wxDynamicLibrary::Unload(wxDllType handle)
{
    ::FreeLibrary(handle);
}

/* static */
void *wxDynamicLibrary::RawGetSymbol(wxDllType handle, const olib::t_string& name)
{
    return (void *)::GetProcAddress(handle, olib::opencorelib::str_util::to_string(name).c_str());
}

// ----------------------------------------------------------------------------
// enumerating loaded DLLs
// ----------------------------------------------------------------------------

/* static */
std::vector< wxDynamicLibraryDetails >  wxDynamicLibrary::ListLoaded()
{
    std::vector< wxDynamicLibraryDetails > dlls;

#if wxUSE_DBGHELP
    if ( wxDbgHelpDLL::Init() )
    {
        // prepare to use functions for version info extraction
        wxVersionDLL verDLL;

        wxDynamicLibraryDetailsCreator::EnumModulesProcParams params;
        params.dlls = &dlls;
        params.verDLL = &verDLL;

        if ( !wxDbgHelpDLL::EnumerateLoadedModules
                            (
                                ::GetCurrentProcess(),
                                wxDynamicLibraryDetailsCreator::EnumModulesProc,
                                &params
                            ) )
        {
            olib::opencorelib::utilities::handle_system_error(true);
        }
    }
#endif // wxUSE_DBGHELP

    return dlls;
}


