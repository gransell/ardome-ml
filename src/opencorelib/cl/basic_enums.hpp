#ifndef _CORE_BASICENUMS_H_
#define _CORE_BASICENUMS_H_

#include "./minimal_string_defines.hpp"

namespace olib
{
   namespace opencorelib
    {
        /// Encapsulates the assert_level enum
        namespace assert_level
        {
            /// The severity of the assert.
            enum severity
            {
                debug,      //!< Just a warning in debug. Ignoring this assert should be safe
                warning,    //!< A warning. Take a serious look at this!
                error,      //!< This is an error. The program will probably not be able to continue.
                fatal,       //!< Serious error. Expect abnormal program termination.
                undefined
            };
            
            CORE_API t_string to_string(severity lvl);
            CORE_API severity from_string(const TCHAR* str_lvl);
        }

        /// Encapsulates the action enum.
        namespace assert_response
        {
            /// What action is the choice of the end user?
            enum action 
            {	
                ignore_once,     //!< Ignore this particular assert, this time
                ignore_always,   //!< Ignore this particular assert always.
                ignore_all,      //!< Ignore all asserts in this program
                debug_program,   //!< debug the program from where this assert was fired.
                abort_program    //!< Terminate the program, no meaning to continue.
            };
        }

        /// Encapsulates the severity enumeration
        namespace log_level
        {
            /// The severity of a log request. 
            /** This can be used by log_targets to determine
                where a certain log request should go. */
            enum severity 
            { 
                emergency = 0,  //!< emergency situation. 
                alert = 1,      //!< An extremely serious error has occurred
                critical = 2,   //!< A serious error has occured
                error = 3,      //!< An error has occurred
                warning = 4,    //!< warning, something bad could happen.
                notice = 5,     //!< The user should notice this
                info = 6,       //!< Information to the user
                debug1 = 7,     //!< Used for debugging purposes.
                debug2 = 8,     //!< Used for debugging purposes.
                debug3 = 9,     //!< Used for debugging purposes.
                debug4 = 10,    //!< Used for debugging purposes.
                debug5 = 11,    //!< Used for debugging purposes.
                debug6 = 12,    //!< Used for debugging purposes.
                debug7 = 13,    //!< Used for debugging purposes.
                debug8 = 14,    //!< Used for debugging purposes.
                debug9 = 15,    //!< Used for debugging purposes.
                unknown = 16    //!< unknown severity
            };

            /// Convert a severity to a string.
            CORE_API t_string to_string(const severity lvl);

            /// Convert a string to a severity.
            CORE_API severity from_string(const TCHAR* str_lvl);

            /// Convert a assert_level::severity to a log_level::severity
            CORE_API severity to_level( assert_level::severity al );

            CORE_API severity to_level_from_int( int lvl );
        }

        /// Encapsulates the hint enumeration
        namespace log_target
        {
            /** log_targets could use these values as hints 
                when outputting log requests. 
                During logging they are not used directly but
                added through function calls. For instance, to
                make a log appear on standard out, and to make 
                the device visible to the user (if hidden):
                <pre>
                    ARLOG("Hello world!").std_out().show_user();
                </pre> 
                  
                To make it appear in the status bar in a GUI application:
                <pre>
                    ARLOG("Hello world!").gui_status();
                </pre> 

                
                */
            enum hint
            {
                not_set = 0,         //!< There is no hint to the log_target.
                trace = 1,          //!< Output to debugger, if it is connected
                file = 2,           //!< Output to standard log-file, if present.
                standard_out = 4,    //!< Output to standard out. Could be a GUI component in a GUI app.
                standard_error = 8,  //!< Output to standard error. Could be a GUI component in a GUI app.
                gui_status = 16,     //!< Output to the status-bar in a GUI app.
                show_user = 32      //!< If the output device is currently hidden, display it to the user.
            };
        }

        /// Encapsulates the option enumeration
        namespace print
        {
            /// Defines what to print when outputting a excption_context
            enum option 
            { 
                output_expression = 1,   //!< Output the tested expression
                output_message = 2,      //!< Output the message
                output_name_values = 4,   //!< Output the name-value map.
                output_hr = 8,           //!< Output the HRESULT
                output_callstack = 16,    //!< Output the callstack
                output_source = 32,      //!< Output the source ( foo::bar::my_class::do_work )
                output_file_and_line = 64, //!< Output the file name and the line number.
                output_empty_fields = 128, //!< Empty fields are not output, if this flag is not present
                output_machine_error = 256, //!< Output the machine readable error.
                output_multiline = 512, //!< Allow multiple lines in the output, otherwise one single line
                shorter = output_expression | output_message | output_source,
                output_short = shorter | output_name_values | output_hr ,
                output_default =  output_short | output_file_and_line | output_machine_error
            };
        }

    }
}


#endif // _CORE_BASICENUMS_H_

