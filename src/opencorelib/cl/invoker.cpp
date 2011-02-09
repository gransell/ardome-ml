#include "precompiled_headers.hpp"
#include "./invoker.hpp"
#include "./enforce_defines.hpp"
#include "./enforce.hpp"

#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#define LOKI_CLASS_LEVEL_THREADING
#include <loki/Singleton.h>

namespace olib
{
   namespace opencorelib
    {
        typedef Loki::SingletonHolder< main_invoker > the_internal_main_invoker;

        main_invoker& the_main_invoker::instance()
        {
            return the_internal_main_invoker::Instance();
        }

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
				return invoker_ptr( new non_invoker() );
            #endif
        }

        void invoker::report_success( const invoke_callback_function_ptr& result_cb ) const
        {
            if( result_cb ) 
            {
                std_exception_ptr no_exception;
                (*result_cb)( invoke_result::success, no_exception );
            }
        }

        void invoker::report_failure( const invoke_callback_function_ptr& result_cb, const std::exception& e) const
        {
            if( result_cb ) 
            {
                std_exception_ptr ex( new std::exception(e) );
                (*result_cb)( invoke_result::failure, ex);
            }
        }

        void invoker::report_failure( const invoke_callback_function_ptr& result_cb, const base_exception& e) const
        {
            if( result_cb ) 
            {
                std_exception_ptr ex( new base_exception(e) );
                (*result_cb)( invoke_result::failure, ex);
            }
        }

        void invoker::report_failure( const invoke_callback_function_ptr& result_cb ) const
        {
            if( result_cb ) 
            {
                std_exception_ptr ex( new std::runtime_error("non_blocking_invoke call failed. Unknown reason.") );
                (*result_cb)( invoke_result::failure, ex);
            }
        }

        non_invoker::non_invoker()
        {
        }

        void non_invoker::non_blocking_invoke(  const invokable_function& f, 
                                                const invoke_callback_function_ptr& result_cb ) const 
        {
            try
            {
                f(); 
                report_success( result_cb );   
            }
            catch( base_exception& be )
            {
                report_failure( result_cb, be );
            }
            catch( std::exception& e )
            {
                report_failure( result_cb, e);   
            }
            catch( ... )
            {
                report_failure( result_cb );
            }
        }

        explicit_step_invoker::explicit_step_invoker( const invoker_ptr &parent )
            : m_parent( parent )
            , m_was_a_blocking_invoke( new invoke_callback_function( boost::bind( &blocking_invoke_tagging_function, _1, _2 ) ) )
        {
        }
        
        invoke_result::type explicit_step_invoker::invoke( const invokable_function& f ) const 
        {
            boost::recursive_mutex::scoped_lock lck(m_mtx);
            m_queue.push( std::make_pair( f, m_was_a_blocking_invoke ) );
			return invoke_result::success;
        }
        
        bool explicit_step_invoker::need_invoke() const 
        {
            return true;
        }
        
        void explicit_step_invoker::step() 
        {
            boost::recursive_mutex::scoped_lock lck(m_mtx);
            while (!m_queue.empty()) 
            {
                if( m_queue.front().second == m_was_a_blocking_invoke )
                {
                    m_parent->invoke( m_queue.front().first );
                }
                else
                {
                    m_parent->non_blocking_invoke( m_queue.front().first, m_queue.front().second );
                }

                m_queue.pop();
            }

            boost::shared_ptr< explicit_step_invoker > exp_inv;
            if( exp_inv = boost::dynamic_pointer_cast< explicit_step_invoker >( m_parent ) )
            {
                exp_inv->step( );
            }
        }

        void explicit_step_invoker::non_blocking_invoke(   const invokable_function& f, 
                                                            const invoke_callback_function_ptr& result_cb ) const
        {
            boost::recursive_mutex::scoped_lock lck(m_mtx);
            m_queue.push(std::make_pair( f, result_cb ) );
        }

        
    }
}
