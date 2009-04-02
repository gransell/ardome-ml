
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

//#include "precompiled_headers.hpp"


#ifdef __BORLANDC__
	#pragma hdrstop
#endif

#include "precompiled_headers.hpp"
#include "./stack_walker.hpp"
#include "../str_util.hpp"
#include "./debughlp.hpp"

extern EXCEPTION_POINTERS *wxGlobalSEInformation = NULL;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// CStackFrame
// ----------------------------------------------------------------------------
namespace olib {namespace opencorelib {

void stack_frame::OnGetName()
{
	if ( m_hasName )
		return;

	m_hasName = true;

	// get the name of the function for this stack frame entry
	static const size_t MAX_NAME_LEN = 1024;
    const int bufsize = sizeof(SYMBOL_INFO) + MAX_NAME_LEN;
	BYTE symbolBuffer[bufsize];
	ZeroMemory(symbolBuffer, bufsize);

	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)symbolBuffer;
	pSymbol->SizeOfStruct = sizeof(symbolBuffer);
	pSymbol->MaxNameLen = MAX_NAME_LEN;

	DWORD64 symDisplacement = 0;
	if ( !wxDbgHelpDLL::SymFromAddr
		(
		::GetCurrentProcess(),
		GetSymAddr(),
		&symDisplacement,
		pSymbol
		) )
	{
		//wxDbgHelpDLL::LogError(_CT("SymFromAddr"));
		return;
	}

    m_name = str_util::to_t_string(pSymbol->Name);
	m_offset = static_cast<size_t>(symDisplacement);
}

void stack_frame::OnGetLocation()
{
	if ( m_hasLocation )
		return;

	m_hasLocation = true;

	// get the source line for this stack frame entry
	IMAGEHLP_LINE lineInfo = { sizeof(IMAGEHLP_LINE) };
	DWORD dwLineDisplacement;
	if ( !wxDbgHelpDLL::SymGetLineFromAddr
		(
		    ::GetCurrentProcess(),
		    static_cast<DWORD>(GetSymAddr()),
		    &dwLineDisplacement,
		    &lineInfo
		) )
	{
		// it is normal that we don't have source info for some symbols,
		// notably all the ones from the system DLLs...
		//wxDbgHelpDLL::LogError(_CT("SymGetLineFromAddr"));
		return;
	}

    m_filename = str_util::to_t_string(lineInfo.FileName);
	m_line = lineInfo.LineNumber;
}

bool
stack_frame::GetParam(size_t n,
					   t_string *type,
					   t_string *name,
					   t_string *value) const
{
	if ( !DoGetParamCount() )
		ConstCast()->OnGetParam();

	if ( n >= DoGetParamCount() )
		return false;

	if ( type )
		*type = m_paramTypes[n];
	if ( name )
		*name = m_paramNames[n];
	if ( value )
		*value = m_paramValues[n];

	return true;
}

void stack_frame::OnParam(_SYMBOL_INFO* pSymInfo)
{
	m_paramTypes.push_back(t_string(_CT("")));
	m_paramNames.push_back(str_util::to_t_string(pSymInfo->Name));

	// if symbol information is corrupted and we crash, the exception is going
	// to be ignored when we're called from WalkFromException() because of the
	// exception handler there returning EXCEPTION_CONTINUE_EXECUTION, but we'd
	// be left in an inconsistent state, so deal with it explicitly here (even
	// if normally we should never crash, of course...)
#ifdef _CPPUNWIND
	try
#else
	__try
#endif
	{
		// as it is a parameter (and not a global var), it is always offset by
		// the frame address
		DWORD_PTR pValue = static_cast<DWORD_PTR>(m_addrFrame + pSymInfo->Address);
		m_paramValues.push_back(wxDbgHelpDLL::DumpSymbol(pSymInfo, (void *)pValue));
	}
#ifdef _CPPUNWIND
	catch ( ... )
#else
	__except ( EXCEPTION_EXECUTE_HANDLER )
#endif
	{
		m_paramValues.push_back(t_string(_CT("")));
	}
}

BOOL CALLBACK
EnumSymbolsProc(PSYMBOL_INFO pSymInfo, ULONG symSize, PVOID data)
{
    ARUNUSED_ALWAYS(symSize);

	stack_frame *frame = static_cast<stack_frame *>(data);

	// we're only interested in parameters
	if ( pSymInfo->Flags & IMAGEHLP_SYMBOL_INFO_PARAMETER )
	{
		frame->OnParam(pSymInfo);
	}

	// return true to continue enumeration, false would have stopped it
	return TRUE;
}

void stack_frame::OnGetParam()
{
	// use SymSetContext to get just the locals/params for this frame
	IMAGEHLP_STACK_FRAME imagehlpStackFrame;
	ZeroMemory(&imagehlpStackFrame, sizeof(IMAGEHLP_STACK_FRAME));
	imagehlpStackFrame.InstructionOffset = GetSymAddr();
	if ( !wxDbgHelpDLL::SymSetContext
		(
		::GetCurrentProcess(),
		&imagehlpStackFrame,
		0           // unused
		) )
	{
		// for symbols from kernel DLL we might not have access to their
		// address, this is not a real error
		if ( ::GetLastError() != ERROR_INVALID_ADDRESS )
		{
			wxDbgHelpDLL::LogError(_CT("SymSetContext"));
		}

		return;
	}

	if ( !wxDbgHelpDLL::SymEnumSymbols
		(
		::GetCurrentProcess(),
		NULL,               // DLL base: use current context
		NULL,               // no mask, get all symbols
		EnumSymbolsProc,    // callback
		this                // data to pass to it
		) )
	{
		wxDbgHelpDLL::LogError(_CT("SymEnumSymbols"));
	}
}


// ----------------------------------------------------------------------------
// CStackWalker
// ----------------------------------------------------------------------------

void stack_walker::WalkFrom(const CONTEXT *pCtx, size_t skip)
{
	if ( !wxDbgHelpDLL::Init() )
	{
		// don't log a user-visible error message here because the stack trace
		// is only needed for debugging/diagnostics anyhow and we shouldn't
		// confuse the user by complaining that we couldn't generate it
		//wxLogDebug(_CT("Failed to get stack backtrace: %s"),
		//	wxDbgHelpDLL::GetErrorMessage().c_str());
		return;
	}

	// according to MSDN, the first parameter should be just a unique value and
	// not process handle (although the parameter is prototyped as "HANDLE
	// hProcess") and actually it advises to use the process id and not handle
	// for Win9x, but then we need to use the same value in StackWalk() call
	// below which should be a real handle... so this is what we use
	const HANDLE hProcess = ::GetCurrentProcess();

	static bool done_once = false;

	if ( !done_once && !wxDbgHelpDLL::SymInitialize
		(
		hProcess,
		NULL,   // use default symbol search path
		TRUE    // load symbols for all loaded modules
		) )
	{
		wxDbgHelpDLL::LogError(_CT("SymInitialize"));

		return;
	}

	done_once = true;

	CONTEXT ctx = *pCtx; // will be modified by StackWalk()

	DWORD dwMachineType;

	// initialize the initial frame: currently we can do it for x86 only
	STACKFRAME sf;
	ZeroMemory(&sf, sizeof(STACKFRAME));

#ifdef _M_IX86
	sf.AddrPC.Offset       = ctx.Eip;
	sf.AddrPC.Mode         = AddrModeFlat;
	sf.AddrStack.Offset    = ctx.Esp;
	sf.AddrStack.Mode      = AddrModeFlat;
	sf.AddrFrame.Offset    = ctx.Ebp;
	sf.AddrFrame.Mode      = AddrModeFlat;

	dwMachineType = IMAGE_FILE_MACHINE_I386;
#else
	#error "Need to initialize STACKFRAME on non x86"
#endif // _M_IX86

	// iterate over all stack frames
	for ( size_t nLevel = 0; ; nLevel++ )
	{
		// get the next stack frame
		if ( !wxDbgHelpDLL::StackWalk
			(
			dwMachineType,
			hProcess,
			::GetCurrentThread(),
			&sf,
			&ctx,
			NULL,       // read memory function (default)
			wxDbgHelpDLL::SymFunctionTableAccess,
			wxDbgHelpDLL::SymGetModuleBase,
			NULL        // address translator for 16 bit
			) )
		{
			if ( ::GetLastError() )
				wxDbgHelpDLL::LogError(_CT("StackWalk"));

			break;
		}

        #ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
            // conversion from 'DWORD' to 'void *' of greater size
            #pragma warning(push)
            #pragma warning(disable: 4312) 
        #endif
        
		// don't show this frame itself in the output
		if ( nLevel >= skip )
		{
			stack_frame frame(nLevel - skip,
				(void*)(sf.AddrPC.Offset),
				sf.AddrFrame.Offset);

			OnStackFrame(frame);
		}

        #ifdef OLIB_COMPILED_WITH_VISUAL_STUDIO
            #pragma warning(pop)
        #endif
	}

	// this results in crashes inside ntdll.dll when called from
	// exception handler ...
#if 0
	if ( !wxDbgHelpDLL::SymCleanup(hProcess) )
	{
		wxDbgHelpDLL::LogError(_CT("SymCleanup"));
	}
#endif
}

void stack_walker::WalkFrom(const _EXCEPTION_POINTERS *ep, size_t skip)
{
	WalkFrom(ep->ContextRecord, skip);
}

void stack_walker::WalkFromException()
{
	extern EXCEPTION_POINTERS *wxGlobalSEInformation;

    // 	_CT("CStackWalker::WalkFromException() can only be called from wxApp::OnFatalException()") );
	if( !wxGlobalSEInformation ) return;
	
	// don't skip any frames, the first one is where we crashed
	WalkFrom(wxGlobalSEInformation, 0);
}

void stack_walker::Walk(size_t skip)
{
	// to get a CONTEXT for the current location, simply force an exception and
	// get EXCEPTION_POINTERS from it
	//
	// note:
	//  1. we additionally skip RaiseException() and WalkFromException() frames
	//  2. explicit cast to EXCEPTION_POINTERS is needed with VC7.1 even if it
	//     shouldn't have been according to the docs
	__try
	{
		RaiseException(0x1976, 0, 0, NULL);
	}
	__except( WalkFrom((EXCEPTION_POINTERS *)GetExceptionInformation(),
		skip + 2), EXCEPTION_CONTINUE_EXECUTION )
	{
		// never executed because of WalkFromException() return value
	}
}

}} // End of olib::useful

