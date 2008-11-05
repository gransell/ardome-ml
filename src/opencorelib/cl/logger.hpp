#ifndef _CORE_LOGGER_H_
#define _CORE_LOGGER_H_

#include "basic_enums.hpp"

namespace olib
{
	namespace opencorelib
	{
        // Forward declaration
        class exception_context;
		
        /// Carries log information from the point of the log request to each log_target.
        /** An instance of this class is created every time
            an ARLOGXX macro is invoked (the ARLOG_IF_LEVEL will only create
            an instance if the global log level, handled by the the_log_handler,
            is high enough ). The class carries information about the location
            of the log request, the level of the request and hints about where
            a listening log_target should output the request. 
            @author Mats Lindel&ouml;f */
		class CORE_API logger
		{
		public:
            /// Default constructor
			logger(void);

            /// Makes the actual logging take place.
            /** Calls the private handle_log function
                that forwards log-requests to log_targets. */
			~logger(void);

			logger( const logger& o );
			logger& operator=(const logger& o);

            /// Used by the ARLOGXX macros.
			logger& ARLOG_A;
            
            /// Used by the ARLOGXX macros.
			logger& ARLOG_B;

            /// Create a new instance of this class. Used by the ARLOGXX macros.
			static logger make_logger();

            /// Add the context where the log request took place.
            /** Used by the ARLOGXX macros. */
			logger& add_context(	const char* filename, long linenr, 
									const char* funcdname, const char* exp = 0);

            /// Add a value that implements operator<<.
            /** @param variable_name The name of the value to add.
                @param val The value of the variable. 
                @return A reference to *this. */
			template< class T>
			logger& add_value(const char * variable_name, const T & val)
			{
                ARUNUSED_ALWAYS( variable_name );
                if(!m_context) return *this;
                ec_add_value_to_message( *m_context, val);
				return *this;
			}

            /// Set the log message. 
            /** Normally this is done via one of the ARLOGXX macros.
                <pre>
                //  Writing this:
                ARLOG("A message. The value is %i")(the_val);
                // will create a CLogger and call msg() with the 
                // string passed as an argument to the macro. 
                </pre> */
			logger& msg( const t_string& str_msg);

            /// Convenience function. Same as the t_string& version.
            logger& msg( const wchar_t* str_msg);

            /// Convenience function. Same as the t_string& version.
            logger& msg( const char* str_msg);

            /// Get the level of the log request.
			log_level::severity level() const;

            /// Set the level of the log request. 
            /** Normally you would use one of the ARLOGXX macros to 
                this.
                <pre>
                    ARLOG("Serious trouble ahead!").level(log_level::critical);
                    ARLOG_IF( a <= b, "a must be greater than b").level(log_level::error);

                    // shorter versions that do the same thing:
                    ARLOG("Serious trouble ahead!").critical();
                    ARLOG_IF( a <= b, "a must be greater than b").error();

                    // If you use the ARLOG_IF_LEVEL macro the Level is set
                    // by the macro. No need to call Level at the end.
                    ARLOG_IF_LEVEL( log_level::debug1, "debug ...");
                </pre> */
			logger& level( const log_level::severity lvl );

            /// Get the context of the log request. Used by log_targets.
			boost::shared_ptr<exception_context> context();

            /// Make a nice output representation of this instance.
			void pretty_print( t_ostream& ostrm, print::option print_option );
            /// Make a nice oneline output representation of this instance.
			void pretty_print_one_line( t_ostream& ostrm, print::option print_option );

            // Easily set the log level.
            logger& emergency() { mlog_level = log_level::emergency; return *this; }
            logger& alert() { mlog_level = log_level::alert; return *this; }
            logger& critical() { mlog_level = log_level::critical; return *this; } 
            logger& error() { mlog_level = log_level::error; return *this; } 
            logger& warning() { mlog_level = log_level::warning; return *this; } 
            logger& notice() { mlog_level = log_level::notice; return *this; } 
            logger& info() { mlog_level = log_level::info; return *this; } 
            logger& debug1() { mlog_level = log_level::debug1; return *this; } 
            logger& debug2() { mlog_level = log_level::debug2; return *this; } 
            logger& debug3() { mlog_level = log_level::debug3; return *this; } 
            logger& debug4() { mlog_level = log_level::debug4; return *this; } 
            logger& debug5() { mlog_level = log_level::debug5; return *this; } 
            logger& debug6() { mlog_level = log_level::debug6; return *this; } 
            logger& debug7() { mlog_level = log_level::debug7; return *this; } 
            logger& debug8() { mlog_level = log_level::debug8; return *this; } 
            logger& debug9() { mlog_level = log_level::debug9; return *this; } 

            // Easily set the hint to the log_targets
            logger& trace() {
                m_log_hint = static_cast<log_target::hint>(m_log_hint | log_target::trace);
                return *this; 
            }
            logger& file() {
                m_log_hint = static_cast<log_target::hint>(m_log_hint | log_target::file);
                return *this;
            }
            logger& std_out() {
                m_log_hint = static_cast<log_target::hint>(m_log_hint | log_target::standard_out);
                return *this;
            }
            logger& std_err() {
                m_log_hint = static_cast<log_target::hint>(m_log_hint | log_target::standard_error);
                return *this;
            }
            logger& status() {
                m_log_hint = static_cast<log_target::hint>(m_log_hint | log_target::gui_status);
                return *this;
            }
            logger& show() {
                m_log_hint = static_cast<log_target::hint>(m_log_hint | log_target::show_user);
                return *this;
            }

            /// log_targets could look at this to determine where a log request should go.
            log_target::hint log_hint() const { return m_log_hint; }

            /// Get the internal log context.
            boost::shared_ptr<exception_context> get_context() const { return m_context; }
                
		private:
			mutable bool m_ignore_this_log;
			boost::shared_ptr<exception_context> m_context;
			void handle_log();
			log_level::severity mlog_level;
            log_target::hint m_log_hint;
			friend class trace_logger;
		};

		/// Class used to trace function calls.
		class CORE_API trace_logger
		{
		public:
			inline trace_logger( ) {}
			~trace_logger()
			{
				if( m_logger ) m_logger->msg( _T("trace exit:") + m_msg );
			}

			trace_logger& set_logger( const logger& log );

		private:
			olib::t_string m_msg;
			boost::shared_ptr< logger > m_logger;
		};
	}
}

#endif // _CORE_LOGGER_H_

