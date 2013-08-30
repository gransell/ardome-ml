#ifndef _CORE_LOGPRINTER_H_
#define _CORE_LOGPRINTER_H_

#include <boost/utility.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem/path.hpp>
#include "logger.hpp"
#include "logtarget.hpp"

namespace olib
{
   namespace opencorelib
    {
        /// Encapsulates the options enumeration
        namespace logoutput
        {
            enum options
            {
                current_thread_id = 1,    //!< Output the current thread id.
                current_log_level = 2,   //!< Output the current log level
                output_default = current_thread_id | current_log_level
            };
        }

        /// Encapsulates useful functions related to logging.
        namespace log_utilities 
        {
            /// Deletes old log files on disk
            /** @param dir The directory where the old log files are located.
                @param pfx The prefix of the log files to examine. i.e. my_log
                    as in my_log-2006-12-18-Mon.log. Only files that start with
                    my_log will be examined.
                @param older_than_days Only files older than this value in days will
                    be deleted. */
			CORE_API void delete_old_log_files(const olib::t_path& full_path, const t_string& prefix, int older_than_days);

            /// Get a full path for app_name based on the current time.
            /** @param app_name Name of the application that wants to log.
                @return A suitable file name. i.e. "my_log-2007-12-18-Mon.log" if app_name is "my_log" */
            CORE_API t_string get_suitable_logfile_name(const t_string& app_name );

            /// Formats a stringstream according to the given date and time formats.
            /** @param date_fmt A date format string according to boost::date_time.
                @param time_fmt A time format string according to boost::date_time.
            */
            CORE_API void get_formatted_stream( t_stringstream &ss, 
                                       const t_string& date_fmt,
                                       const t_string& time_fmt );

            /// Creates a short string with useful log information.
            /** @param lvl The severity of the log request. Output if log_options
                    contains the flag: Currentlog_level.
                @param log_options Determines which information should be present
                        in the information string.
                @return A log info string. If log_options is logoutput::short the string 
                        could look like this: "11:11:20.218 (00000fe0) [info]". 
                        The first value is the time, followed by the time in milli 
                        seconds, followed by the current thread's id, followed by the
                        log_level. */
            CORE_API t_string get_log_prefix_string( log_level::severity lvl, 
                                                     t_stringstream &ss,
                                                     logoutput::options log_options );

            /// Creates a stream that is useful for log output.
            /** The function returns a stream object to log_file_path. 
                If this directory doesn't exist it creates it.
                Recommended usage is to call get_sutiable_logfile_name and pass 
                the result of that function*/
			CORE_API void get_default_log_stream(const olib::t_path& log_file_path, std::ofstream &result);
        }
    }
}


#endif // _CORE_LOGPRINTER_H_

