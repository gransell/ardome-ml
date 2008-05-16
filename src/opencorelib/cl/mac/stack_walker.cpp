
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
//	if ( m_hasName )
//		return;
//
//	m_hasName = true;
//
//	// get the name of the function for this stack frame entry
//	static const size_t MAX_NAME_LEN = 1024;
//	BYTE symbolBuffer[sizeof(SYMBOL_INFO) + MAX_NAME_LEN];
//	AUZEROMEMORY(symbolBuffer);
//
//	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)symbolBuffer;
//	pSymbol->SizeOfStruct = sizeof(symbolBuffer);
//	pSymbol->MaxNameLen = MAX_NAME_LEN;
//
//	DWORD64 symDisplacement = 0;
//	if ( !wxDbgHelpDLL::SymFromAddr
//		(
//		::GetCurrentProcess(),
//		GetSymAddr(),
//		&symDisplacement,
//		pSymbol
//		) )
//	{
//		//wxDbgHelpDLL::LogError(_T("SymFromAddr"));
//		return;
//	}
//
//    m_name = StrUtil::to_t_string(pSymbol->Name);
//	m_offset = static_cast<size_t>(symDisplacement);
}

void stack_frame::OnGetLocation()
{
//	if ( m_hasLocation )
//		return;
//
//	m_hasLocation = true;
//
//	// get the source line for this stack frame entry
//	IMAGEHLP_LINE lineInfo = { sizeof(IMAGEHLP_LINE) };
//	DWORD dwLineDisplacement;
//	if ( !wxDbgHelpDLL::SymGetLineFromAddr
//		(
//		    ::GetCurrentProcess(),
//		    static_cast<DWORD>(GetSymAddr()),
//		    &dwLineDisplacement,
//		    &lineInfo
//		) )
//	{
//		// it is normal that we don't have source info for some symbols,
//		// notably all the ones from the system DLLs...
//		//wxDbgHelpDLL::LogError(_T("SymGetLineFromAddr"));
//		return;
//	}
//
//    m_filename = StrUtil::to_t_string(lineInfo.FileName);
//	m_line = lineInfo.LineNumber;
}

bool
stack_frame::GetParam(size_t n,
					   t_string *type,
					   t_string *name,
					   t_string *value) const
{
//	if ( !DoGetParamCount() )
//		ConstCast()->OnGetParam();
//
//	if ( n >= DoGetParamCount() )
//		return false;
//
//	if ( type )
//		*type = m_paramTypes[n];
//	if ( name )
//		*name = m_paramNames[n];
//	if ( value )
//		*value = m_paramValues[n];
//
	return true;
}


void stack_frame::OnGetParam()
{
//	// use SymSetContext to get just the locals/params for this frame
//	IMAGEHLP_STACK_FRAME imagehlpStackFrame;
//	AUZEROMEMORY(imagehlpStackFrame);
//	imagehlpStackFrame.InstructionOffset = GetSymAddr();
//	if ( !wxDbgHelpDLL::SymSetContext
//		(
//		::GetCurrentProcess(),
//		&imagehlpStackFrame,
//		0           // unused
//		) )
//	{
//		// for symbols from kernel DLL we might not have access to their
//		// address, this is not a real error
//		if ( ::GetLastError() != ERROR_INVALID_ADDRESS )
//		{
//			wxDbgHelpDLL::LogError(_T("SymSetContext"));
//		}
//
//		return;
//	}
//
//	if ( !wxDbgHelpDLL::SymEnumSymbols
//		(
//		::GetCurrentProcess(),
//		NULL,               // DLL base: use current context
//		NULL,               // no mask, get all symbols
//		EnumSymbolsProc,    // callback
//		this                // data to pass to it
//		) )
//	{
//		wxDbgHelpDLL::LogError(_T("SymEnumSymbols"));
//	}
}


// ----------------------------------------------------------------------------
// CStackWalker
// ----------------------------------------------------------------------------


void stack_walker::Walk(size_t skip)
{
	// to get a CONTEXT for the current location, simply force an exception and
	// get EXCEPTION_POINTERS from it
	//
	// note:
	//  1. we additionally skip RaiseException() and WalkFromException() frames
	//  2. explicit cast to EXCEPTION_POINTERS is needed with VC7.1 even if it
	//     shouldn't have been according to the docs
//	__try
//	{
//		RaiseException(0x1976, 0, 0, NULL);
//	}
//	__except( WalkFrom((EXCEPTION_POINTERS *)GetExceptionInformation(),
//		skip + 2), EXCEPTION_CONTINUE_EXECUTION )
//	{
//		// never executed because of WalkFromException() return value
//	}
}

void stack_walker::WalkFromException()
{
}
	
}} // End of amf::Common


