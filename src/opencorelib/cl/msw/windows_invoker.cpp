#include "precompiled_headers.hpp"
#include "./subclass.hpp"
#include "../invoker.hpp"

namespace olib
{
   namespace opencorelib
    {

        class windows_invoker : public invoker, 
                                public subclass_wnd
        {
        public:

            windows_invoker( HWND to_subclass )
            {
                initialze( to_subclass );
                m_gui_thread_id = ::GetCurrentThreadId();
            }

            ~windows_invoker()
            {
                terminate();
            }
            
            UINT request_msg() const
            {
                static UINT unique_msg = 0;
                if( unique_msg == 0)
                    unique_msg = RegisterWindowMessage(_T("Ardendo::Common::Invoke::RequestMessage"));
                return unique_msg;
            }

            UINT success_msg() const
            {
                return 100;
            }

            UINT failure_msg() const
            {
                return 200;
            }

            virtual invoke_result::type invoke( const invokable_function& f ) const 
            {
                LRESULT res = ::SendMessage( get_hooked_wnd() , request_msg(), (WPARAM)&f, (LPARAM)0 );
                if(res == success_msg() ) return invoke_result::success;
                return invoke_result::failure;
            }

            virtual bool need_invoke() const 
            {
                return ::GetCurrentThreadId() != m_gui_thread_id;
            }

            void initialze( HWND hwnd )
            {
                subclass_wnd::hook_window(hwnd);
            }

            void terminate()
            {
                subclass_wnd::unhook();
            }

            virtual LRESULT window_proc(UINT msg, WPARAM wParam, LPARAM lParam)
            {
                if( msg == request_msg() )
                {
                    invokable_function to_invoke( *(invokable_function*)(wParam));
                    TRY_BASEEXCEPTION
                    {
                        to_invoke();
                        return (LRESULT)success_msg();
                    }
                    CATCH_BASEEXCEPTION_RETURN( failure_msg() );
                }

                return subclass_wnd::window_proc(msg, wParam, lParam );
            }

        private:
            DWORD m_gui_thread_id;
        };

        CORE_API invoker_ptr create_invoker_windows( HWND gui_thread_wnd )
        {
            invoker_ptr inv( new windows_invoker(gui_thread_wnd) );
            return inv;
        }
    }
}
