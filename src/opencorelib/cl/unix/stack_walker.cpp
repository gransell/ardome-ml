/////////////////////////////////////////////////////////////////////////////
// Name:        msw/stackwalk.cpp
// Purpose:     CStackWalker implementation for Unix/glibc
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2005-01-18
// RCS-ID:      $Id: stackwalk.cpp,v 1.5 2005/07/14 09:14:31 VZ Exp $
// Copyright:   (c) 2005 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "../precompiled_headers.h"

// ----------------------------------------------------------------------------
// tiny helper wrapper around popen/pclose()
// ----------------------------------------------------------------------------

namespace Ardendo
{
    namespace Common
    {

        class wxStdioPipe
        {
        public:
            // ctor parameters are passed to popen()
            wxStdioPipe(const char *command, const char *type)
            {
            }

            // conversion to stdio FILE
            operator FILE *() const { return 0; }

            // dtor closes the pipe
            ~wxStdioPipe()
            {
            }

        };

        // ============================================================================
        // implementation
        // ============================================================================

        Ardendo::t_string CStackWalker::ms_exepath;

        // ----------------------------------------------------------------------------
        // CStackFrame
        // ----------------------------------------------------------------------------

        void CStackFrame::OnGetName()
        {
        }

        void CStackFrame::OnGetLocation()
        {
        }

        // ----------------------------------------------------------------------------
        // CStackWalker
        // ----------------------------------------------------------------------------

        void CStackWalker::Walk(size_t skip)
        {
        }

    }
}

