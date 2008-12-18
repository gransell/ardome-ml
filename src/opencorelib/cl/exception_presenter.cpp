#include "precompiled_headers.hpp"

#include "exception_presenter.hpp"
#include "base_exception.hpp"
#include "utilities.hpp"

#define LOKI_CLASS_LEVEL_THREADING
#include <loki/Singleton.h>

#include <boost/bind.hpp>

#include "create_selection_dialog.hpp"

namespace olib
{
	namespace opencorelib
	{
		enum exception_choice { Ignore, Rethrow };
		typedef Loki::SingletonHolder<	exception_presenter > the_internal_exception_presenter;

		exception_presenter& the_exception_presenter::instance()
		{
			return the_internal_exception_presenter::Instance();
		}

		exception_presenter::exception_presenter() : m_assembly_run_as_service(false)
		{
		}

		void exception_presenter::set_assembly_run_as_service( bool val )
		{
			m_assembly_run_as_service = val;
		}

		void show_dialog( void* lp_parameter)
		{
			selection_dialog* dialog = (selection_dialog*)(lp_parameter);
			/// This function will not terminate before the dialog is closed.
			dialog->show_dialog();
		}

		bool exception_presenter::show(const t_string& caption, const base_exception& e) 
		{
			boost::mutex::scoped_lock lck( m_Mtx );

            boost::shared_ptr< selection_dialog > dialog( create_selection_dialog() );

            t_stringstream outss;
            e.pretty_print(outss, static_cast< print::option>(  print::output_default | 
                                                                print::output_callstack | 
                                                                print::output_multiline  ) );

            if(!dialog) {
                T_CERR	 << "-- " << caption << " --\n"
                          << outss.str()
                          << "--\n";
                return true;
            }
            dialog->add_value(_T("Ignore"), static_cast<int>(Ignore));
            dialog->add_value(_T("Rethrow"), static_cast<int>(Rethrow));
            
            dialog->set_assembly_run_as_service(m_assembly_run_as_service);


            dialog->caption(caption);
            dialog->message(outss.str());

			#ifdef OLIB_ON_WINDOWS
                ::MessageBeep(MB_ICONEXCLAMATION);
			#else
                // #pragma message( "No beep on you platform when an exception is presented." )
            #endif

			try
			{
                /// Create a new thread to show the actual dialog. This is to 
                /// prevent the current thread from processing its message cue 
                /// while the dialog is shown.
                boost::thread dialog_thread( boost::bind( show_dialog, dialog.get()));

                /// Block the calling thread so no more asserts can occur
                /// Wait for the dialog thread to terminate.
                dialog_thread.join();
			}
			catch( boost::thread_resource_error& )
			{
				// Failed to create a new thread
				// We might be shutting down or something
				// else is preventing us from creating a new thread.
				return false;
			}

            if( dialog->result() == dialog_result::selection_made )
            {
                if(dialog->choice() == Ignore ) return false;
                else if(dialog->choice() == Rethrow ) return true;
            }

			// Closing the dialog without making a choice means ignore.
			return false;
		}
	}
}
