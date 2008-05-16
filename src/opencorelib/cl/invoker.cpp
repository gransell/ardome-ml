#include "precompiled_headers.hpp"
#include "./invoker.hpp"
#include "./enforce_defines.hpp"
#include "./enforce.hpp"

#include <boost/thread.hpp>

namespace olib
{
   namespace opencorelib
    {
        #ifdef OLIB_ON_WINDOWS

        /// Creates an invoker using the wnd_proc attached to the passed wnd.
        /** The passed HWND is sub classed, which means that the current
            window procedure for that window is overridden. If a special 
            message is passed (unique to amf), the procedure executes 
            amf code on the gui thread. When destroyed the invoker object
            will restore the window procedure.
            @author Mats Lindel&ouml;f 
            @param gui_thread_wnd A hwnd created by the main gui-thread. */
            invoker_ptr create_invoker_windows( HWND gui_thread_wnd );
        #elif defined( OLIB_ON_MAC )
            invoker_ptr create_invoker_mac();
        #else
			invoker_ptr create_invoker( boost::int64_t wnd_handle );
        #endif

        CORE_API invoker_ptr create_invoker( boost::int64_t wnd_handle )
        { 
            #ifdef OLIB_ON_WINDOWS
                ARENFORCE_MSG( ::IsWindow((HWND(wnd_handle))), "The passed handle must be a true HWND" );
                return create_invoker_windows( (HWND)(wnd_handle) );
            #elif defined( OLIB_ON_MAC )
                return create_invoker_mac();
            #else
				return invoker_ptr( );
            #endif
        }
        
        invoke_result::type explicit_step_invoker::invoke( const invokable_function& f ) const {
            boost::recursive_mutex::scoped_lock lck(m_mtx);
            m_queue.push(f);
			return invoke_result::success;
        }
        
        bool explicit_step_invoker::need_invoke() const {
            return true;
        }
        
        void explicit_step_invoker::step() {
            boost::recursive_mutex::scoped_lock lck(m_mtx);
            while (!m_queue.empty()) {
                TRY_BASEEXCEPTION {
                    
                    m_queue.front()();
                    
                } CATCH_BASEEXCEPTION();
                
                m_queue.pop();
            }
        }
        
    }
}
