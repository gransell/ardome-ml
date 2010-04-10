#ifndef _CORE_EXCEPTION_PRESENTER_H_
#define _CORE_EXCEPTION_PRESENTER_H_

#include "./minimal_string_defines.hpp"

#include <boost/thread/mutex.hpp>

namespace olib
{
	namespace opencorelib
	{
		class base_exception;

		/// Presents an exception to the user in form of a dialog.
		class CORE_API exception_presenter : public boost::noncopyable
		{
		public:
			exception_presenter();

			/// Show the given exception to the user. 
			/** The user can select to ignore the exception or to 
				throw it to the next catch statement. Remember that exceptions
                should only be shown in debug builds, in release builds we don't 
                want user interaction built into the core of amf. 
				@param caption The caption of the window where the exception is displayed.
				@param a the exception to display
				@return true if the exception should be thrown again,
				false otherwise. */
			bool show(const t_string& caption, const base_exception& a);

			/// Use this function if your program is a Windows service
            /** If set to true on windows, the presenter will hook up
                to the client desktop and show the dialog there. If 
                false, and running as a service, the currently logged on
                user will not be able to see the dialog. */
			void set_assembly_run_as_service( bool val);
		private:
			boost::mutex m_Mtx;
			bool m_assembly_run_as_service;
		};


		/// Singleton provider of the CException_presenter.
		class CORE_API the_exception_presenter
		{
		public:
			static exception_presenter& instance();
		};

	}
}

#endif //_CORE_EXCEPTION_PRESENTER_H_

