#ifndef _CORE_UTILITIES_H_
#define _CORE_UTILITIES_H_

// Doxygen file documentation
/*! @file core/utilities.h
	@brief This file contains ...*/

#include "./minimal_string_defines.hpp"
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/thread/xtime.hpp>

namespace olib
{
	namespace opencorelib
	{
		/// Provider of common utilities used throughout the useful_boost project.
		namespace utilities
		{
			#ifdef OLIB_ON_WINDOWS
			/// Handles Win32 errors by calling ::get_last_error 
			/** The function tries to convert the error id to a readable 
				string, which is returned and showed in a message box
				if the passed parameter is set to true. */
			CORE_API t_string handle_system_error(bool show_message_box);

			/// Converts a windows HRESULT to a string using ::format_message.
			CORE_API t_string hresult_to_string( long hr );

			#endif
            
            /// Fetch the current thread's id.
            /** @return The thread-id of the thread that calls this function. */
            CORE_API boost::uint32_t get_current_thread_id();

			/// Removes the path from a string containing both path and file
			/** Should work ok on both unix and windows paths since
				it replaces \ with / before it starts. */
			CORE_API t_string strip_path_from_file( const t_string& path_and_file );


            /// Makes sure that a full path exisits
            /** Convenience function that will create all 
                intermediate directories, not only the leaf.
                @param ph The path that should be created, if it doesn't 
                            already exists. 
                @throws boost::filesystem::basic_filesystem_error< fs::wpath >*/
            CORE_API void make_sure_path_exists( const olib::t_path& ph );

            /// Creates a valid samba path from a base url and a filename.
            /** On windows, a samba path looks like this: \\host\folder\folder\file.ext
                and on mac/linux like this smb://host/foler/folder/file.ext.
                The function makes sure that either backward of forward slashes are
                used thought out the string. It then adds smb:// or \\ to the start of
                it (removing any existing smb:// and \\). Finally it adds the file_name
                to the end of the result, making sure no duplicate // or \\ are created. 
                @param host The name of the host computer.
                @param base_url The path on the host
                @param file_name The name of the file on the host.
                @return A valid samba path for the current platform.*/
            CORE_API t_string make_smb_path(  const t_string& host,
                                                const t_string& base_url, 
                                                const t_string& file_name );

            /// Creates a valid http path to a file from a base url.
            /** Will make sure that the path starts with http:// and only contains
                single forward slashes. The result is created like this:
                http://user:pwd@host:port/s1/s2/file_name
                @param user The user name, omitted if user and password are empty.
                @param password The password, omitted if user and password are empty.
                @param host The host name.
                @param port The port on the host.
                @param base_url The base url, may or may not contain the protocol prefix.
                        For instance both www.company.com and http://www.company.com will 
                        both return a string starting with http://www.company.com. 
                @param file_name The name of the file, will be appended to the base_url.
                @param protocol_prefix Typically "http:" or "ftp:". For cross platform handling
                        of samba paths, use make_smb_path instead. */
            CORE_API t_string make_protocol_path( const t_string& user,
                                                    const t_string& password,
                                                    const t_string& host,
                                                    const t_string& port,
                                                    const t_string& base_url, 
                                                    const t_string& file_name,
                                                    const t_string& protocol_prefix );

            /// Adds <?xml version="1.0" encoding="UTF-8"?> to the stream.
            CORE_API void add_xml_header_utf8( std::ostream& os );

			/// Get the current call stack as a string.
			/** @param skip_levels The number of levels from the top of the stack that
								should be discarded in the result. Must be 1 or higher.*/
			CORE_API t_string get_stack_trace( int skip_levels );

            /// Takes the current time and adds milli_sec to it, and returns the result as an xtime.
            /** This function is useful when dealing with the boost::thread
                library. Example:
                <pre>
                    // Sleep for 5 seconds.
                    boost::thread::sleep(add_millsecs_from_now(5000));
                </pre> */
            CORE_API boost::xtime add_millsecs_from_now( long milli_sec );

            /// Small helper class that measures time in ms from creation to measuring call.
            class CORE_API timer
            {
            public:

                /// Create the timer instance and start measuring time.
                timer();

                /// Get the time in milliseconds since this time instance was created.
                long elapsed_millisecs();
            private:
                boost::posix_time::ptime m_Start;
            };
		}
	}
}

#endif // _CORE_UTILITIES_H_

