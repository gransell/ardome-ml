#ifndef _CORE_CREATE_SELECTIONDIALOG_H_
#define _CORE_CREATE_SELECTIONDIALOG_H_

#include "selection_dialog.hpp"

namespace olib
{
   namespace opencorelib
    {
        /// Creates a selection_dialog
        /** A selection_dialog lets the user see a message from
            amf (often created by the enforce and assert macros)
            and select an action to take in response to the message
            (terminate the app, ignore, ignore all assertions ... ).
            The dialog code is platform specific and this function
            hides that fact from the rest of the code base, which just
            calls this function on all platforms.
            @return A gui element used to communicate with the user */
        boost::shared_ptr< selection_dialog > create_selection_dialog();
    }
}

#endif // _CORE_CREATE_SELECTIONDIALOG_H_

