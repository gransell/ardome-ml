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
// ---------------------------------------------------------------------------

#include "../precompiled_headers.hpp"

#include <opencorelib/cl/dynlib.hpp>

const amf::t_string wxDynamicLibrary::m_libext = _T("dynlib");

/* static */
wxDllType
wxDynamicLibrary::RawLoad(const amf::t_string& libname, int flags)
{
    return NULL;
}

/* static */
void wxDynamicLibrary::Unload(wxDllType handle)
{
}

/* static */
void *wxDynamicLibrary::RawGetSymbol(wxDllType handle, const amf::t_string& name)
{
    return NULL;
}


