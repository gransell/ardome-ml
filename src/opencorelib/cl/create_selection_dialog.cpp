#include "precompiled_headers.hpp"
#include "create_selection_dialog.hpp"

#ifdef OLIB_ON_WINDOWS
    #include "msw/selection_dialog.hpp"
#elif defined OLIB_ON_MAC
	#include <opencorelib/cl/mac/mac_selection_dlg.hpp>
#endif


namespace olib
{
   namespace opencorelib
    {
        boost::shared_ptr< selection_dialog > create_selection_dialog()
        {
            #ifdef OLIB_ON_WINDOWS
                boost::shared_ptr<selection_dialog> d( new windows_selection_dialog<int>());
                return d;
			#elif defined OLIB_ON_MAC
				boost::shared_ptr<selection_dialog> d( new mac_selection_dlg() );
				return d;
			#else
                boost::shared_ptr<selection_dialog> d;
                return d;
            #endif
        }
    }
}

