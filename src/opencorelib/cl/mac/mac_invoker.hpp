/*
 *  mav_invoker.h
 *  core
 *
 *  Created by Mikael Gransell on 2007-09-24.
 *  Copyright 2007 Ardendo AB. All rights reserved.
 *
 */

namespace olib {
    namespace opencorelib {
        namespace mac {
            
            int invoke_function_on_main_thread( const invokable_function& f, const invoke_callback_function_ptr& result_cb, bool block );
                        
        }
    }
}
