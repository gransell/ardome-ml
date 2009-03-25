#ifndef _CORE_SELECTIONDIALOG_H_
#define _CORE_SELECTIONDIALOG_H_

#include "./minimal_string_defines.hpp"

namespace olib
{
   namespace opencorelib
    {
        /// Did the user select anything or did he/she just hit cancel?
        namespace dialog_result
        {
            enum type   { 
                            selection_made ,    /**< The user actually selected a choice in the dialog */
                            cancel              /**< The user cancelled the dialog alltogether */
                        };
        }

        /// An abstraction of a gui dialog that lets the user make simple selections.
        class selection_dialog
        {
        public:
            virtual ~selection_dialog() {}

            /// Show the dialog to the user.
            /** This function is called on a new thread, make sure you 
                have message pump before you actually display something. */
            virtual void show_dialog() = 0;

            /// Add a value to the list of values the user can choose from when viewing the dialog.
            virtual void add_value( const t_string& val_name, int val ) = 0;

            /// If the current assembly is running as a windows service, set this to true.
            /** Other platforms might just give this function an empty 
                implementation. */
            virtual void set_assembly_run_as_service( bool v ) = 0;
  
            /// Get the caption of the dialog
            virtual t_string caption() const = 0;

            /// Set the caption of the dialog.
            virtual void caption( const t_string& t) = 0;

            /// Get the message displayed in the dialog.
            virtual t_string message() const = 0;

            /// Set the message displayed in the dialog.
            virtual void message( const t_string& m) = 0;

            /// Get the result of the dialog display.
            virtual dialog_result::type result() const = 0;

            /// Set the result of the dialog display.
            virtual void result( const dialog_result::type& r) = 0;

            /// If the user made a selection, get it here.
            virtual int choice() const = 0;

            /// Set the user choice.
            virtual void choice( const int& c) = 0;
        };
    }
}


#endif

