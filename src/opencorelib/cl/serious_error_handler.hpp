#ifndef _CORE_SERIOUS_ERROR_HANDLER_H_
#define _CORE_SERIOUS_ERROR_HANDLER_H_

#include <signal.h>

namespace olib
{
   namespace opencorelib
    {
        /// Should handle serious errors in amf based applications.
        /** Serious errors are things like access violations and
            abort signals. Before the os terminates the app, we would
            like to be able to dump information of what went wrong to 
            a crash-file. 
            <b>Currently not implemented at all!!! </b>*/
        class CORE_API serious_error_handler : public boost::noncopyable
        {
        public:
            serious_error_handler();

            /// Setup the error handler.
            void setup();

            /// Set the file name to dump to when something goes really wrong.
            void set_dump_file_name( const t_string& dump_file_name);

            /// Get the filename set to dump to.
            t_string get_dump_file_name() const;
        
        private:
            t_string m_dump_file_name;

        };

        /// Singleton carrying a serious_error_handler.
        class CORE_API the_serious_error_handler
        {
        public:
            /// Get the instance of the singleton.
            static serious_error_handler& instance();
        };
    }
}

#endif // _CORE_SERIOUS_ERROR_HANDLER_H_

