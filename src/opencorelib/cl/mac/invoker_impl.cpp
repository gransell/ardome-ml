
#include "precompiled_headers.hpp"
#include "opencorelib/cl/invoker.hpp"
#include "opencorelib/cl/utilities.hpp"
#include "./mac_invoker.hpp"


namespace olib
{
    namespace opencorelib
    {
        class mac_main_thread_invoker : public invoker
        {
        public:

            mac_main_thread_invoker()
            {
                m_gui_thread_id = utilities::get_current_thread_id();
            }

            ~mac_main_thread_invoker()
            {
            }
            
            virtual invoke_result::type invoke( const invokable_function& f ) const 
            {
                return mac::invoke_function_on_main_thread( f ) ? invoke_result::success : invoke_result::failure;
            }

            virtual bool need_invoke() const 
            {
                return utilities::get_current_thread_id() != m_gui_thread_id;
            }

        private:
            boost::uint32_t m_gui_thread_id;
        };
        
        invoker_ptr create_invoker_mac()
        {
            return invoker_ptr( new mac_main_thread_invoker );
        }
    }
}
