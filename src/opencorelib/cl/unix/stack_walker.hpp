///////////////////////////////////////////////////////////////////////////////
// Name:        wx/unix/stackwalk.h
// Purpose:     declaration of wxStackWalker for Unix
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2005-01-19
// RCS-ID:      $Id: stackwalk.h,v 1.2 2005/01/19 20:13:49 VZ Exp $
// Copyright:   (c) 2005 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _AUBASE_UNIX_STACKWALK_H_
#define _AUBASE_UNIX_STACKWALK_H_

namespace Ardendo
{
    namespace Common
    {
        // ----------------------------------------------------------------------------
        // wxStackFrame
        // ----------------------------------------------------------------------------

        class CStackFrame : public CStackFrameBase
        {
        public:
            // arguments are the stack depth of this frame, its address and the return
            // value of backtrace_symbols() for it
            //
            // NB: we don't copy syminfo pointer so it should have lifetime at least as
            //     long as ours
            CStackFrame(size_t level, void *address, const char *syminfo)
                : CStackFrameBase(level, address)
            {
                m_hasName =
                    m_hasLocation = false;

                m_syminfo = syminfo;
            }

        protected:
            virtual void OnGetName();
            virtual void OnGetLocation();

        private:
            const char *m_syminfo;

            bool m_hasName,
                m_hasLocation;
        };

        // ----------------------------------------------------------------------------
        // wxStackWalker
        // ----------------------------------------------------------------------------

        class CStackWalker : public CStackWalkerBase
        {
        public:
            // we need the full path to the program executable to be able to use
            // addr2line, normally we can retrieve it from wxTheApp but if wxTheApp
            // doesn't exist or doesn't have the correct value, the path may be given
            // explicitly
            CStackWalker(const char *argv0 = NULL)
            {
                ms_exepath = Ardendo::Common::StrUtil::to_t_string(argv0);
            }

            virtual void Walk(size_t skip = 1);
            virtual void WalkFromException() { Walk(2); }

            static const Ardendo::t_string& GetExePath() { return ms_exepath; }

        private:
            static Ardendo::t_string ms_exepath;
        };

    }
}

#endif // _AUBASE_UNIX_STACKWALK_H_
