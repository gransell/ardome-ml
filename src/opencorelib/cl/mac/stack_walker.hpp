#ifndef _AUBASE_MSW_STACK_WALKER_H_
#define _AUBASE_MSW_STACK_WALKER_H_

///	@file core/mac/stack_walker.h
/** The following code is stolen directly from wxWidgets.
	Windows-based Stackwalker implementation
	Stolen from wxWidgets by Mats */

///////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/stackwalk.h
// Purpose:     CStackWalker for MSW
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2005-01-08
// RCS-ID:      $Id: stackwalk.h,v 1.2 2005/01/19 01:15:07 VZ Exp $
// Copyright:   (c) 2005 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#include <opencorelib/cl/stack_walker.hpp>


namespace olib
{
	namespace opencorelib
	{
		// ----------------------------------------------------------------------------
		// CStackFrame
		// ----------------------------------------------------------------------------

		class  stack_frame : public stack_frame_base
		{
		private:
			stack_frame* ConstCast() const
			{ return const_cast<stack_frame *>(this); }

			size_t DoGetParamCount() const { return m_paramTypes.size(); }

		public:
			stack_frame(size_t level, void *address, size_t addrFrame)
				: stack_frame_base(level, address)
			{
				m_hasName =
					m_hasLocation = false;

				m_addrFrame = addrFrame;
			}

			virtual size_t GetParamCount() const
			{
				ConstCast()->OnGetParam();
				return DoGetParamCount();
			}

			virtual bool
				GetParam(size_t n, t_string* type, t_string* name, t_string* value) const;
 
			// callback used by OnGetParam(), don't call directly
//			void OnParam(_SYMBOL_INFO* pSymInfo);

		protected:
			virtual void OnGetName();
			virtual void OnGetLocation();

			void OnGetParam();


			// helper for debug API: it wants to have addresses as DWORDs
			size_t GetSymAddr() const
			{
				return reinterpret_cast<size_t>(m_address);
			}

		private:
			bool m_hasName,
				m_hasLocation;

			size_t m_addrFrame;

			std::vector<t_string>	m_paramTypes,
									m_paramNames,
									m_paramValues;
		};

		// ----------------------------------------------------------------------------
		// CStackWalker
		// ----------------------------------------------------------------------------

		class stack_walker : public stack_walker_base
		{
		public:
			// we don't use ctor argument, it is for compatibility with Unix version
			// only
			stack_walker(const char* argv0 = NULL) { ARUNUSED_ALWAYS(argv0); }

			virtual void Walk(size_t skip = 1);
			virtual void WalkFromException();
			
			void OnGetName();
			void OnGetLocation();
			bool GetParam(size_t n,
						  t_string *type,
						  t_string *name,
						  t_string *value) const;


//			// enumerate stack frames from the given context
//			void WalkFrom(const _CONTEXT *ctx, size_t skip = 1);
//			void WalkFrom(const _EXCEPTION_POINTERS *ep, size_t skip = 1);
		};
	}
}

#endif // _AUBASE_MSW_STACK_WALKER_H_
