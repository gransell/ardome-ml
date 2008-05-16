#ifndef _CORE_STACKWALKER_H_
#define _CORE_STACKWALKER_H_

///@file core/stack_walker.h The following code is stolen directly from wxWidgets.
/** Since we want to avoid the gigantic overhead from wxWidgets, we
	extract this diamond and use it in the core module of amf. 
	@author Mats Lindel&ouml;f

	Here is the original notice from the wxWidget file:

	///////////////////////////////////////////////////////////////////////////////
	// Name:        wx/wx/stackwalk.h
	// Purpose:     CStackWalker and related classes, common part
	// Author:      Vadim Zeitlin
	// Modified by:
	// Created:     2005-01-07
	// RCS-ID:      $Id: stackwalk.h,v 1.2 2005/01/19 01:15:03 VZ Exp $
	// Copyright:   (c) 2004 Vadim Zeitlin <vadim@wxwindows.org>
	// Licence:     wxWindows licence
	///////////////////////////////////////////////////////////////////////////////
*/
	
namespace olib
{
	namespace opencorelib
	{
		// ----------------------------------------------------------------------------
		// CStackFrame: a single stack level
		// ----------------------------------------------------------------------------

		class stack_frame_base
		{
		private:
			// put this inline function here so that it is defined before use
			stack_frame_base *ConstCast() const
			{ return const_cast<stack_frame_base*>(this); }

		public:
			stack_frame_base(size_t level, void* address = NULL)
			{
				m_level = level;

				m_line =
					m_offset = 0;

				m_address = address;
			}

			// get the level of this frame (deepest/innermost one is 0)
			size_t GetLevel() const { return m_level; }

			// return the address of this frame
			void *GetAddress() const { return m_address; }


			// return the unmangled (if possible) name of the function containing this
			// frame
			t_string GetName() const { ConstCast()->OnGetName(); return m_name; }

			// return the instruction pointer offset from the start of the function
			size_t GetOffset() const { ConstCast()->OnGetName(); return m_offset; }

			// get the module this function belongs to (not always available)
			t_string GetModule() const { ConstCast()->OnGetName(); return m_module; }


			// return true if we have the filename and line number for this frame
			bool HasSourceLocation() const { return !GetFileName().empty(); }

			// return the name of the file containing this frame, empty if
			// unavailable (typically because debug info is missing)
			t_string GetFileName() const
			{ ConstCast()->OnGetLocation(); return m_filename; }

			// return the line number of this frame, 0 if unavailable
			size_t GetLine() const { ConstCast()->OnGetLocation(); return m_line; }


			// return the number of parameters of this function (may return 0 if we
			// can't retrieve the parameters info even although the function does have
			// parameters)
			virtual size_t GetParamCount() const { return 0; }

			// get the name, type and value (in text form) of the given parameter
			//
			// any pointer may be NULL
			//
			// return true if at least some values could be retrieved
			virtual bool GetParam(size_t n,
				olib::t_string * type,
				olib::t_string * name,
				olib::t_string * value) const
			{
                ARUNUSED_ALWAYS(n);
				ARUNUSED_ALWAYS(type);
				ARUNUSED_ALWAYS(name);
				ARUNUSED_ALWAYS(value);
				return false;
			}


			// although this class is not supposed to be used polymorphically, give it
			// a virtual dtor to silence compiler warnings
			virtual ~stack_frame_base() { }

		protected:
			// hooks for derived classes to initialize some fields on demand
			virtual void OnGetName() { }
			virtual void OnGetLocation() { }


			// fields are protected, not private, so that OnGetXXX() could modify them
			// directly
			size_t m_level;

			t_string m_name,
				m_module,
				m_filename;

			size_t m_line;

			void *m_address;
			size_t m_offset;
		};

		// ----------------------------------------------------------------------------
		// CStackWalker: class for enumerating stack frames
		// ----------------------------------------------------------------------------

        // Forward declaration
        class stack_frame;

		class stack_walker_base
		{
		public:
			// ctor does nothing, use Walk() to walk the stack
			stack_walker_base() { }

			// dtor does nothing neither but should be virtual
			virtual ~stack_walker_base() { }

			// enumerate stack frames from the current location, skipping the initial
			// number of them (this can be useful when Walk() is called from some known
			// location and you don't want to see the first few frames anyhow; also
			// notice that Walk() frame itself is not included if skip >= 1)
			virtual void Walk(size_t skip = 1) = 0;

			// enumerate stack frames from the location of uncaught exception
			//
			// this version can only be called from wxApp::OnFatalException()
			virtual void WalkFromException() = 0;

		protected:
			// this function must be overrided to process the given frame
            virtual void OnStackFrame(const stack_frame& frm) = 0;
		};

	}
}

#ifdef OLIB_ON_WINDOWS
    #include "msw/stack_walker.hpp"
#elif defined(OLIB_ON_MAC)
#include "mac/stack_walker.hpp"
#else
	#include "unix/stack_walker.hpp"
#endif


#endif //_CORE_STACKWALKER_H_

