#ifndef _CORE_UTILITIES_H_
#define _CORE_UTILITIES_H_

// Doxygen file documentation
/*! @file core/utilities.h
	@brief This file contains ...*/

#include "./minimal_string_defines.hpp"
#include "export_defines.hpp"
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/thread/xtime.hpp>
#include "opencorelib/cl/typedefs.hpp"

namespace olib
{
	namespace opencorelib
	{
		/// Describes a l.so or .dll that is loaded into a process.
		class CORE_API library_info
		{
		public:
			/// Constructor that takes the full path and file name, plus the version of the library
			library_info( const t_path& fn );

			/// Get the full path to the library.
			const t_path& get_filename() const;

			/// Set the full path to the library.
			void set_filename( const t_path& fn );

			/// Get the version of the library.
			const t_string& get_version() const;

			/// Set the version of the library.
			void set_version( const t_string& ver );

			/// Get the name of the company that created the library.
			const t_string& get_company() const;

			/// Set the name of the company that created the library.
			void set_company( const t_string& comp );

			/// Get the name of the product
			const t_string& get_product() const;

			/// Set the name of the product
			void set_product( const t_string& prod );

			/// Get the build number of the product
			const t_string& get_build_number() const;

			/// Set the build number of the product
			void set_build_number( const t_string& build_nr );

			CORE_API friend t_ostream& operator<<( t_ostream& os, const library_info& info );

		private:
			t_path m_filename;
			t_string m_company;
			t_string m_product;
			t_string m_version;
			t_string m_build_nr;
		};

		CORE_API t_ostream& operator<<( t_ostream& os, const library_info& info );

		/// Provider of common utilities used throughout the useful_boost project.
		namespace utilities
		{
			#ifdef OLIB_ON_WINDOWS
			/// Handles Win32 errors by calling ::GetLastError 
			/** The function tries to convert the error id to a readable 
				string, which is returned and showed in a message box
				if the passed parameter is set to true. */
			CORE_API t_string handle_system_error(bool show_message_box);

			/// Converts a windows HRESULT to a string using ::format_message.
			CORE_API t_string hresult_to_string( long hr );

			CORE_API boost::int32_t get_last_error_from_os();

			#endif
            
			template< typename T >
			T clamp( const T& to_clamp, const T& min, const T& max ) { return std::min<T>( std::max<T>( to_clamp, min ), max ); }

			/// Fetch the current thread's id.
			/** @return The thread-id of the thread that calls this function. */
			CORE_API boost::uint64_t get_current_thread_id();

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
			CORE_API t_string get_stack_trace( int skip_levels, bool verbose = true );

            
            /// Small helper class that measures time in ms from creation to measuring call.
            class CORE_API timer
            {
            public:

                /// Create the timer instance and start measuring time.
                timer();

                /// Get the time in milliseconds since this time instance was created.
                boost::posix_time::time_duration elapsed();

            private:
                boost::system_time m_start;
            };

			/// Get information about all currently loaded .so or .dll files.
			/** @return A list of information about each file. */
			CORE_API std::vector< library_info_ptr > get_loaded_libraries();

            CORE_API multivalue_property_map parse_multivalue_property( const olib::t_string &prop_str);
			
			/** 
			 * Allocate size amount of bytes aligned to alignment. Make sure to use aligned_free to free the memory.
			 * @param alignment Bytes to align to
			 * @param size Amount to allocate
			 * @return A valid pointer to a block of memory or NULL if allocation failed.
			 * @sa aligmned_free
			 */
			CORE_API void *aligned_alloc( size_t alignment, size_t size );
			
			/**
			 * Free memory allocated with aligned_alloc.
			 */
			CORE_API void aligned_free( void *ptr );
		}
	}
}

#endif // _CORE_UTILITIES_H_

