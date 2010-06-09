#ifndef _CORE_INVOKER_H_
#define _CORE_INVOKER_H_

/** @file invoker.h
    Classes and functions related to the process
    of making a gui-thread call a function 
    on a worker thread's behalf */

#include "./typedefs.hpp"
#include "./base_exception.hpp"
#include "./throw_try_catch_defines.hpp"
#include "./assert_defines.hpp"
#include "./assert.hpp"
#include <queue>

#include <boost/thread.hpp>

namespace olib
{
   namespace opencorelib
    {
       
        /// Encapsulates the type enumeration, describing the result of an invoke request.
        /** @sa invoker */
        namespace invoke_result
        {
            enum type   {  
                            success,    /**< The invoke request succeeded. */
                            failure     /**< The invoke request failed. */ 
                        };
        }

        typedef boost::shared_ptr< std::exception > std_exception_ptr;
        typedef boost::function< void ( olib::opencorelib::invoke_result::type, std_exception_ptr ) > invoke_callback_function;
        typedef boost::shared_ptr< invoke_callback_function > invoke_callback_function_ptr;

        /// Object that can run a function on a separate thread
        /** Depending on the calling thread, the invoker may or
            may not choose to run it directly or use some platform
            specific technique to make the gui-thread or main-thread
            run a function on the calling thread's behalf. 
            @author Mats Lindel&ouml;f
            <bindgen>
        		<attribute name="ref-counted" value="yes"></attribute>
        	</bindgen>
        	*/
        class CORE_API invoker
        {
        public:
            virtual ~invoker() {}

            /// Will execute the passed function on the invoker-thread.
            /** This call blocks until the thread that is attached to 
                this invoker have processed the invokalbe_function.
                If that is unacceptable, use non_blocking_invoke instead. */
            virtual olib::opencorelib::invoke_result::type invoke( const invokable_function& f ) const = 0;


            /// Will execute the passed function on the invoker-thread.
            /** The call will return immediately, not waiting for
                the invoker thread to process the call. The result will be
                received in the callback function, if it is provided. 
                If it is null, the result is just ignored. 

                Make sure that the function object is not referring to any
                stack-based variables, since theses will most certainly have 
                gone out-of-scope when f is actually run.

                @param f The function to run on the invoker-thread.
                @param result_cb The call-back function to that the invoker will call 
                        when f has been processed. */
            virtual void non_blocking_invoke(   const invokable_function& f, 
                                                const invoke_callback_function_ptr& result_cb ) const = 0;

            /// Checks whether a call to invoke would force a thread switch.
            /** @return true if a thread switch would occur during a call to invoke,
                            false if the correct thread is doing the call. */
            virtual bool need_invoke() const = 0;

        protected:

            /// Use these member to implement non_blocking_invoke.
            mutable std::queue< std::pair< invokable_function, invoke_callback_function_ptr > > m_queue;
            mutable boost::recursive_mutex m_mtx;

            /// Call these in the implementation of non_blocking_invoke.
            void report_success( const invoke_callback_function_ptr& result_cb ) const;
            void report_failure( const invoke_callback_function_ptr& result_cb, const std::exception& e) const;
            void report_failure( const invoke_callback_function_ptr& result_cb, const base_exception& e) const;
            void report_failure( const invoke_callback_function_ptr& result_cb ) const;
        };


        /// An instance of this class will always invoke the function on the calling thread.
        class CORE_API non_invoker : public invoker
        {
        public:

            non_invoker();

            /// Just runs the function on the calling thread, adding exception guards.
            /** @param f The function to run.
                @return invoke_result::success on success, otherwise invoke_result::failure */
            virtual olib::opencorelib::invoke_result::type invoke( const invokable_function& f ) const 
            {
                TRY_BASEEXCEPTION
                {
                    f(); 
                    return invoke_result::success; 
                }
                CATCH_BASEEXCEPTION_RETURN( invoke_result::failure );
            }

            virtual void non_blocking_invoke(   const invokable_function& f, 
                                                const invoke_callback_function_ptr& result_cb ) const;

            /// Invoke is never needed, could just as well run the function directly
            /** @return true if a thread switch is needed to run the function,
                    false otherwise. This implementation always returns false since
                    a thread switch is never imposed by the non_invoker. */
            virtual bool need_invoke() const 
            {
                return false; 
            }
        };
        
        /// Creates an invoker platform independently.
        /** @param thread_context Should be a pointer to a valid HWND on windows, on
                                  mac this can just be set to 0 for the main thread or a thread id for a selected thread.
            @return An invoker that can be used to forward calls to the main
                        gui-thread.*/
        CORE_API invoker_ptr create_invoker( boost::int64_t thread_context );   
        
        /// A manually stepped invoker.
        /** A special-case invoker that runs the functions waiting to be invoked only on
            an explicit call to a step() method.  This is mainly useful for testing
            programs, but may be useful in other cases when the main loop is not managed
            by a GUI toolkit. */
        class CORE_API explicit_step_invoker : public invoker
        {
        public:
            olib::opencorelib::invoke_result::type invoke( const invokable_function& f ) const;

            void non_blocking_invoke(   const invokable_function& f, 
                                        const invoke_callback_function_ptr& result_cb ) const;
            
            bool need_invoke() const;
            
            /// Call all waiting functions.
            void step();            
        };

        class CORE_API main_invoker
        {
        public:
            invoker_ptr get_global_invoker() const { return m_invoker; }
            void set_global_invoker( const invoker_ptr& inv ) { m_invoker = inv; }
        private:
            invoker_ptr m_invoker;
        };

        class CORE_API the_main_invoker
        {
        public:
            static main_invoker& instance();
        };
        
    }
}

#endif // _CORE_INVOKER_H_

