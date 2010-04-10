#ifndef _CORE_ASSERT_PRESENTER_H_
#define _CORE_ASSERT_PRESENTER_H_

#include <boost/thread/mutex.hpp>

namespace olib
{
	namespace opencorelib
	{
		class invoke_assert;

        /// Presents an assertion in a dialog.
		class CORE_API assert_presenter : public boost::noncopyable
		{
		public:
			assert_presenter();

            /// Show the assertion to the user
            /** This will let the user break into the debugger,
                terminate the app, disable this assert for the rest
                of the current session or disable all assertions for
                the rest of this session. */
			void show(invoke_assert& a);

			/// Use this function if your program is a Windows service
            /** If so, the dialog is presented on the desktop, which
                is really useful to get direct feed-back from a running
                service. Remember, assertions are only issued in debug
                builds, so, if the service is running on a server, this
                will not be a problem. */
			void set_assembly_run_as_service( bool val);

		private:
			boost::mutex m_Mtx;
			bool m_assembly_run_as_service;
			void break_into_debugger();
			void terminate_app();
		};


		class CORE_API the_assert_presenter
		{
		public:
			static assert_presenter& instance();
		};

	}
}

#endif //_CORE_ASSERT_PRESENTER_H_

