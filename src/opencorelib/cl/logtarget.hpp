#ifndef _CORE_LOGTARGET_H_
#define _CORE_LOGTARGET_H_

#include "assert.hpp"
#include "enforce.hpp"
#include "logger.hpp"
#include "base_exception.hpp"

namespace olib
{
	namespace opencorelib
	{
		/// Interface used to log different events
		/** This interface is used by the_error_logger to log to a uniform
			location throughout an application. The idea is that the main 
			executable implements this interface and registers with 
			the_error_logger to keep all debug logging in one place. 
			@sa error_logger
			@author Mats Lindel&ouml;f */
		class CORE_API logtarget
		{
		public:
			/// Ensure correct destruction.
			virtual ~logtarget();
			/// An assertion occurred. log if required.
			/** @param a The assertion to log
				@param log_source The name of the class (including namespaces) that
						requests the log. */
			virtual void log( invoke_assert& a, const TCHAR* log_source) = 0;
		
			/// An exception occurred. log if required.
			/** @param e The exception to log
				@param log_source The name of the class (including namespaces) that
						requests the log. */
			virtual void log( base_exception& e, const TCHAR* log_source)  = 0;

			/// An log request occurred. log if required.
			/** @param log_msg The log message to log
				@param log_source The name of the class (including namespaces) that
					requests the log. */
			virtual void log( logger& log_msg, const TCHAR* log_source)  = 0;
		};

        /// Implement this interface to support external configuration of your logtarget.
        class CORE_API configurable_logtarget
        {
        public:
            virtual ~configurable_logtarget();

            /// Use this string to create a filename to log to.
            virtual void set_base_file_name( const t_string& ) = 0;

            /// Create the log-file at this location.
            virtual void set_log_path( const olib::t_path& wp ) = 0;

            /// Output logs according to the passed flags.
            virtual void set_output_options( print::option po ) = 0;

            /// The logtarget will filter all log-requests using this regex.
            /** The string that will be tested against the passed regex is
                the source name, for example opencorelib::str_util::to_string, that is 
                the fully qualified function name. If the regex matches this
                name, the log request will be processed. If this string is 
                empty, no filtering should occur. 
                @param regex_filter The regular expression used to filter log requests.*/
            virtual void set_log_source_regex_filter( const t_string& regex_filter ) = 0;

            /// Output to standard out if this is set to true.
            virtual void set_use_std_out( bool v ) = 0;

            /// Output to standard error if this is set to true.
            virtual void set_use_std_error( bool v ) = 0;

        };
	}
}

#endif

