#include "precompiled_headers.hpp"
#include "assert_presenter.hpp"
#include "assert.hpp"
#include "assert_persistance.hpp"
#include "utilities.hpp"
#include "create_selection_dialog.hpp"

#define LOKI_CLASS_LEVEL_THREADING
#include <loki/Singleton.h>

#include <boost/bind.hpp>

#ifdef OLIB_ON_MAC
#include <CoreServices/CoreServices.h>
#endif

namespace olib
{
	namespace opencorelib
	{
        typedef Loki::SingletonHolder< assert_presenter, Loki::CreateUsingNew, Loki::DefaultLifetime,
                                       Loki::ClassLevelLockable > the_internal_assert_presenter;
		
		enum assert_choice {ignore, ignore_all, always_ignore, debug, abort };

		assert_presenter& the_assert_presenter::instance() 
		{
			return the_internal_assert_presenter::Instance();
		}

		assert_presenter::assert_presenter() : m_assembly_run_as_service(false)
		{
		}

		void assert_presenter::set_assembly_run_as_service( bool val)
		{
			m_assembly_run_as_service = val;
		}

		void assert_presenter::break_into_debugger() 
		{
			#ifdef OLIB_ON_WINDOWS	
			    ::DebugBreak();
            #elif defined(OLIB_ON_MAC)
                Debugger();
            #else
            // #pragma message( "No way to break into the debugger on your platform" )
			#endif	
		}

		void assert_presenter::terminate_app() 
		{
			#ifdef WIN32	
                ::FatalAppExit(0,_CT("You have choosen to abort the application. It will now close down."));
			#else
                exit(99);
            #endif
		}

		namespace
		{
			void show_assert_dialog(void* lp_parameter)
			{
				selection_dialog* dialog = (selection_dialog*)(lp_parameter);
				/// This function will not terminate before the dialog is closed.
				dialog->show_dialog();	
			}
		}
		
		void assert_presenter::show(invoke_assert& a) 
		{
			boost::mutex::scoped_lock lck(m_Mtx);

            boost::shared_ptr< selection_dialog > dialog( create_selection_dialog() );
            if(!dialog) return;

            dialog->add_value(str_util::to_t_string("Ignore"), static_cast<int>(ignore));
            dialog->add_value(str_util::to_t_string("ignore_all"), static_cast<int>(ignore_all));
            dialog->add_value(str_util::to_t_string("Always_ignore"), static_cast<int>(always_ignore));
            dialog->add_value(str_util::to_t_string("debug"), static_cast<int>(debug));
            dialog->add_value(str_util::to_t_string("Abort"), static_cast<int>(abort));

            dialog->set_assembly_run_as_service(m_assembly_run_as_service);

            t_stringstream outss;
            a.pretty_print(outss,  static_cast< print::option>( print::output_default | 
                                                                print::output_callstack | 
                                                                print::output_multiline  ) );

			t_string dialog_title = str_util::to_t_string("Assertion Failed [")
                + a.assert_level_asstring() + str_util::to_t_string("]");
			dialog->caption(dialog_title);
			dialog->message(outss.str());

			#ifdef OLIB_ON_WINDOWS
            ::MessageBeep(MB_ICONEXCLAMATION);
			#endif
				
			try
			{
				/// Create a new thread to show the actual dialog. This is to 
				/// prevent the current thread from processing its message cue 
				/// while the dialog is shown.
				boost::thread dialog_thread( boost::bind( &show_assert_dialog, dialog.get()));
				
				/// Block the calling thread so no more asserts can occur
				/// Wait for the dialog thread to terminate.
				dialog_thread.join();
			}
			catch( boost::thread_resource_error& )
			{
				// Failed to create a new thread
				// We might be shutting down or something
				// else is preventing us from creating a new thread.
				return;
			}
			
			if( dialog->result() == dialog_result::selection_made )
			{
				switch(dialog->choice()) 
				{
					case ignore : // Ignore, Don't do anything
						break;
					case ignore_all :
						the_assert_persister::instance().ignore_all(true);
						break;
					case always_ignore :
						the_assert_persister::instance().ignore_assert(a, assert_response::ignore_always);
						break;
					case debug:
						break_into_debugger();
						break;
					case abort :
						terminate_app();
						break;
				default:
						break;
				}
			}			
		}
	}
}
