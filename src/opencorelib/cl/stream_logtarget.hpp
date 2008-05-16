#ifndef STREAMLOGTARGET_H // -*- c++ -*-
#define STREAMLOGTARGET_H
#include "logtarget.hpp"

namespace olib {namespace opencorelib {

        /// A logtarget that writes to a standard c++ stream.
        class CORE_API stream_logtarget : public olib::opencorelib::logtarget {
        public:
            stream_logtarget(t_ostream& out) : m_out(out) {}
      
            /// An assertion occurred. log if required.
            /** @param a The assertion to log
                @param log_source The name of the class (including namespaces) that
                requests the log. */
            virtual void log(olib::opencorelib::invoke_assert& a, const TCHAR* log_source);
      
            /// An exception occurred. log if required.
            /** @param e The exception to log
                @param log_source The name of the class (including namespaces) that
                requests the log. */
            virtual void log(olib::opencorelib::base_exception& e, const TCHAR* log_source);
    
            /// An log request occurred. log if required.
            /** @param e The log message to log
                @param log_source The name of the class (including namespaces) that
                requests the log. */
            virtual void log(olib::opencorelib::logger& log_msg, const TCHAR* log_source);
      
        private:
            t_ostream& m_out;
        };

}}

#endif
