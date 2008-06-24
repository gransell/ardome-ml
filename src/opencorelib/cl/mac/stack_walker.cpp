
/////////////////////////////////////////////////////////////////////////////
// Name:        msw/stackwalk.cpp
// Purpose:     CStackWalker implementation for Win32
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2005-01-08
// RCS-ID:      $Id: stackwalk.cpp,v 1.8 2005/08/03 23:57:00 VZ Exp $
// Copyright:   (c) 2003-2005 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

//#include "precompiled_headers.h"


#include "../precompiled_headers.hpp"
#include <opencorelib/cl/stack_walker.hpp>
#include <opencorelib/cl/str_util.hpp>

//extern EXCEPTION_POINTERS *wxGlobalSEInformation = NULL;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// CStackFrame
// ----------------------------------------------------------------------------
namespace olib { namespace opencorelib {

void stack_frame::OnGetName()
{
}

void stack_frame::OnGetLocation()
{
}

bool
stack_frame::GetParam(size_t n,
					   t_string *type,
					   t_string *name,
					   t_string *value) const
{
	return true;
}


void stack_frame::OnGetParam()
{
}


// ----------------------------------------------------------------------------
// CStackWalker
// ----------------------------------------------------------------------------


void stack_walker::Walk(size_t skip)
{
}

void stack_walker::WalkFromException()
{
}
	
}} // End of amf::Common


