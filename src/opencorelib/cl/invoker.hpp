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

        /// Object that can run a function on a separate thread
        /** Depending on the calling thread, the invoker may or
            may not choose to run it directly or use some platform
            specific technique to make the gui-thread or main-thread
            run a function on the calling thread's behalf. 
            @author Mats Lindel&ouml;f */
        class CORE_API invoker
        {
        public:
            virtual ~invoker() {}

            /// Will execute the passed function on the main-thread.
            virtual invoke_result::type invoke( const invokable_function& f ) const = 0;

            /// Checks whether a call to invoke would force a thread switch.
            /** @return true if a thread switch would occur during a call to invoke,
                            false if the correct thread is doing the call. */
            virtual bool need_invoke() const = 0;
        };


        /// An instance of this class will always invoke the function on the calling thread.
        class CORE_API non_invoker : public invoker
        {
        public:

            /// Just runs the function on the calling thread, adding exception guards.
            /** @param f The function to run.
                @return invoke_result::success on success, otherwise invoke_result::failure */
            virtual invoke_result::type invoke( const invokable_function& f ) const 
            {
                TRY_BASEEXCEPTION
                {
                    f(); 
                    return invoke_result::success; 
                }
                CATCH_BASEEXCEPTION_RETURN( invoke_result::failure );
            }

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
            an explicit call to a step() method.  This is mainly usefull for testing
            programs, but may be usefull in other cases when the main loop is not managed
            by a gui toolkit.
        */
        class CORE_API explicit_step_invoker : public invoker
        {
        public:
            invoke_result::type invoke( const invokable_function& f ) const;
            
            bool need_invoke() const;
            
            /// Call all waiting functions.
            void step();
            
        private:
            mutable std::queue<invokable_function> m_queue;
            mutable boost::recursive_mutex m_mtx;
        };
        
    }
}

#endif // _CORE_INVOKER_H_

