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
                    unique_msg = RegisterWindowMessage(_T("olib::opencorelib::windows_invoker::request_msg"));
                return unique_msg;
            }

            UINT post_request_msg() const
            {
                static UINT unique_msg = 0;
                if( unique_msg == 0)
                    unique_msg = RegisterWindowMessage(_T("olib::opencorelib::windows_invoker::post_request_msg"));
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

            virtual void non_blocking_invoke(   const invokable_function& f, 
                                                const invoke_callback_function_ptr& result_cb ) const
            {
                 boost::recursive_mutex::scoped_lock lck(m_mtx);
                 m_queue.push(std::make_pair( f, result_cb ) );
                 ::PostMessage( get_hooked_wnd() , post_request_msg(), (WPARAM)0, (LPARAM)0 );
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
                else if( msg == post_request_msg() )
                {
                    boost::recursive_mutex::scoped_lock lck(m_mtx);
                    while (!m_queue.empty()) 
                    {
                        try
                        {  
                            m_queue.front().first();
                            report_success( m_queue.front().second );
                        } 
                        catch( const base_exception& be )
                        {
                            report_failure( m_queue.front().second, be );
                        }
                        catch( const std::exception& e )
                        {
                            report_failure( m_queue.front().second, e );
                        }
                        catch( ... )
                        {
                            report_failure( m_queue.front().second );
                        }

                        m_queue.pop();
                    }

                    return (LRESULT)success_msg();
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
