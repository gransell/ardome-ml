/////////////////////////////////////////////////////////////////////////////
// Name:        dynlib.cpp
// Purpose:     Dynamic library management
// Author:      Guilhem Lavaux
// Modified by:
// Created:     20/07/98
// RCS-ID:      $Id: dynlib.cpp,v 1.107 2005/04/27 01:16:00 DW Exp $
// Copyright:   (c) 1998 Guilhem Lavaux
//                  2000-2005 Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

//FIXME:  This class isn't really common at all, it should be moved into
//        platform dependent files (already done for Windows and Unix)

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "precompiled_headers.hpp"

#ifndef OLIB_ON_MAC


#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#   pragma implementation "dynlib.h"
#endif

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

#include "dynlib.hpp"

#if defined(__WXMAC__)
    #include "wx/mac/private.h"
#endif


// ============================================================================
// implementation
// ============================================================================

// ---------------------------------------------------------------------------
// wxDynamicLibrary
// ---------------------------------------------------------------------------

#if defined(__WXPM__) || defined(__EMX__)
    const TCHAR *wxDynamicLibrary::ms_dllext = _T(".dll");
#elif defined(__WXMAC__) && !defined(__DARWIN__)
    const TCHAR *wxDynamicLibrary::ms_dllext = _T("");
#endif

// for MSW/Unix it is defined in platform-specific file
#if !(defined(OLIB_ON_WINDOWS) || defined(__UNIX__)) || defined(__EMX__)
wxDllType wxDynamicLibrary::GetProgramHandle()
{
   // wxFAIL_MSG( wxT("GetProgramHandle() is not implemented under this platform"));
   return 0;
}

#endif // __WXMSW__ || __UNIX__


bool wxDynamicLibrary::Load(const olib::t_string& libnameOrig, int flags)
{
    // Library already loaded
    if(m_handle != 0) return false;

    // add the proper extension for the DLL ourselves unless told not to
    olib::t_string libname = libnameOrig;
    if ( !(flags & wxDL_VERBATIM) )
    {
        olib::t_string ext;
        // and also check that the libname doesn't already have it
        olib::t_string::size_type extPos = libnameOrig.find_last_of(_T("."));
        if(extPos != olib::t_string::npos ) 
            ext = olib::t_string( libnameOrig, extPos, libnameOrig.size() - extPos -1);

        if ( ext.empty() )
        {
            libname += GetDllExt();
        }
    }

    // different ways to load a shared library
    //
    // FIXME: should go to the platform-specific files!
#if defined(__WXMAC__) && !defined(__DARWIN__)
    FSSpec      myFSSpec;
    Ptr         myMainAddr;
    Str255      myErrName;

    wxMacFilename2FSSpec( libname , &myFSSpec );

    if( GetDiskFragment( &myFSSpec,
                         0,
                         kCFragGoesToEOF,
                         "\p",
                         kPrivateCFragCopy,
                         &m_handle,
                         &myMainAddr,
                         myErrName ) != noErr )
    {
        wxLogSysError( _("Failed to load shared library '%s' error '%s'"),
                       libname.c_str(),
                       wxMacMakeStringFromPascal( myErrName ).c_str() );
        m_handle = 0;
    }
#else
    m_handle = RawLoad(libname, flags);
#endif

    if ( m_handle == 0 )
    {
#ifdef wxHAVE_DYNLIB_ERROR
        error();
#else
        //wxLogSysError(_("Failed to load shared library '%s'"), libname.c_str());
#endif
    }

    return IsLoaded();
}

// for MSW and Unix this is implemented in the platform-specific file
//
// TODO: move the rest to os2/dlpm.cpp and mac/dlmac.cpp!
#if (!defined(OLIB_ON_WINDOWS) && !defined(__UNIX__)) || defined(__EMX__)

/* static */
void wxDynamicLibrary::Unload(wxDllType handle)
{
#if defined(__WXPM__) || defined(__EMX__)
    DosFreeModule( handle );
#elif defined(__WXMAC__) && !defined(__DARWIN__)
    CloseConnection( (CFragConnectionID*) &handle );
#elif defined(OLIB_ON_MAC)
#else
    #error  "runtime shared lib support not implemented"
#endif
}

#endif // !(__WXMSW__ || __UNIX__)

void* wxDynamicLibrary::DoGetSymbol(const olib::t_string &name, bool *success) const
{
    // Can't load symbol from unloaded library
    assert( IsLoaded() );
                 
    void    *symbol = 0;
    ARUNUSED_ALWAYS(symbol);

#if defined(__WXMAC__) && !defined(__DARWIN__)
    Ptr                 symAddress;
    CFragSymbolClass    symClass;
    Str255              symName;
#if TARGET_CARBON
    c2pstrcpy( (StringPtr) symName, name.fn_str() );
#else
    strcpy( (char *)symName, name.fn_str() );
    c2pstr( (char *)symName );
#endif
    if( FindSymbol( m_handle, symName, &symAddress, &symClass ) == noErr )
        symbol = (void *)symAddress;
#elif defined(__WXPM__) || defined(__EMX__)
    DosQueryProcAddr( m_handle, 1L, (PSZ)name.c_str(), (PFN*)symbol );
#else
    symbol = RawGetSymbol(m_handle, name);
#endif

    if ( success )
        *success = symbol != NULL;

    return symbol;
}

void *wxDynamicLibrary::GetSymbol(const olib::t_string& name, bool *success) const
{
    void *symbol = DoGetSymbol(name, success);
    if ( !symbol )
    {
#ifdef wxHAVE_DYNLIB_ERROR
        error();
#else
        //wxLogSysError(_("Couldn't find symbol '%s' in a dynamic library"),
        //              name.c_str());
        assert(false);
#endif
    }

    return symbol;
}

#endif // #ifndef OLIB_ON_MAC
