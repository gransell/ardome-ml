#ifndef _CORE_LOGHANDLER_H_
#define _CORE_LOGHANDLER_H_

#include "./typedefs.hpp"
#include <boost/thread.hpp>
#include <vector>

namespace olib
{
	namespace opencorelib
	{
		class invoke_assert;
        class logger;
        class base_exception;
        class logtarget;

		/// Handles logging of assertions, throwing of CBase_exceptions and log-requests using the CLogger class. 
		/** This should be a singleton in the system. Access the 
			singleton by using the_log_handler::instance function.
			@sa ARENFORCE
			@sa ARENFORCE_WIN
			@sa ARENFORCE_HR
			@sa ARASSERT
			@sa ARLOG
			@sa ARLOGALWAYS
			@author Mats Lindel&ouml;f*/
		class CORE_API log_handler : public boost::noncopyable
		{
		public:
			log_handler();
			~log_handler();

			/// log the assertion using registered loggers.
            /** @param a The invoke_assert instance created when an assertion fails.
                @param log_source A string containing the function name
                        where the assertion failed (including namespace and class 
                            specifiers.) This string is forwarded to the logtargets
                                so they can perform filtering. */
			void log( invoke_assert& a, const TCHAR* log_source);

			/// log the enforce using registered loggers.
            /** @param e The base_exception that is being thrown.
                @param log_source A string containing the function name
                where the exception was thrown (including namespace and class 
                specifiers.) This string is forwarded to the logtargets
                so they can perform filtering. */
			void log( base_exception& e, const TCHAR* log_source);

			/// log the log request using registered loggers.
            /** @param lg The logger that was created at the log request.
                @param log_source A string containing the function name
                where the exception was thrown (including namespace and class 
                specifiers.) This string is forwarded to the logtargets
                so they can perform filtering. */
			void log( logger& lg, const TCHAR* log_source ) ;

			/// Is logging enabled?
            /** @return true if logging can happen at all, false if all
                    logging is disabled. */
			bool get_is_logging();

			/// Disable or enable logging on a global basis.
            /** @param v If set to false, no log-requests will be processed. */
			void set_is_logging( bool v) ;

			/// Add a logger.
			/** Make sure you remove the logger before the module 
			where it was created goes out of scope. */
			void add_target( logtarget_ptr p);

			/// Removes a previously installed logger.
			/** @return false if no such logtarget could be found. */
			bool remove_target( logtarget_ptr p);


            /// Set the global log level of amf.
            /** logtargets should check this value before outputting 
                a log-request. 
                @param v The log level used as a cut-off level
                    for log-requests. If for instance this value is set to 
                            log_level::warning, all log-requests with a log-level
                            set to log_level::notice or higher will be ignored. 
                            So the log macro ARLOG_NOTICE("Some message") will not 
                            be output. */
            void set_global_log_level( log_level::severity v )
            {
                m_global_log_level = v;
            }

            /// Get the global log-level. 
            /** Default global log-level is log_level::warning. */
            inline log_level::severity get_global_log_level() const
            {
                return m_global_log_level;
            }

		private:
			boost::recursive_mutex m_Mtx;
			bool m_log;
			t_string m_output_filename;
			typedef std::vector< logtarget_ptr > target_vec;
			target_vec m_log_targets;
            log_level::severity m_global_log_level;
		};

		/// The one and only log_hander in the system.
		class CORE_API the_log_handler
		{
		public:
            /// Get the instance of the singleton.
            /** @return An instance to one and only log_handler object in amf. */
			static log_handler& instance();
		};
	}
}

#endif // _CORE_LOGHANDLER_H_

