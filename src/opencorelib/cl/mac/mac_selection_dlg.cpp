
#include "../precompiled_headers.hpp"
#include "opencorelib/cl/str_util.hpp"
#include "mac_selection_dlg.hpp"
#include <CoreFoundation/CoreFoundation.h>

namespace olib
{
	namespace opencorelib
	{
        
        void mac_selection_dlg::show_dialog()
        {
			CFOptionFlags responseFlags = 0;
			CFStringRef title = CFStringCreateWithCString( NULL, str_util::to_string(mText).c_str(), kCFStringEncodingUTF8 );
			CFStringRef message = CFStringCreateWithCString( NULL, str_util::to_string(mMessage).c_str(), kCFStringEncodingUTF8 );
            CFStringRef altButton = CFStringCreateWithCString( NULL, "Break", kCFStringEncodingUTF8 );
			SInt32 ret = CFUserNotificationDisplayAlert(0, 
                                                        kCFUserNotificationStopAlertLevel, // Alert level
                                                        NULL, NULL, NULL, // Resources (Icon, Sound, localization)
                                                        title, message, // Messages
                                                        NULL, altButton, NULL, // Buttons (Default, Alternate, Other)
                                                        &responseFlags );
            
            if( responseFlags == kCFUserNotificationAlternateResponse ) {
                mResult = dialog_result::selection_made;
            }
            
			CFRelease(title);
			CFRelease(message);
			CFRelease(altButton);
        }

		
		
		mac_selection_dlg::mac_selection_dlg() : mResult( dialog_result::cancel )
		{
#ifdef _DEBUG
            mChoice = 3;
#endif
		}

		
		
		t_string mac_selection_dlg::caption() const
		{
			return mText;
		}
		
		
		void mac_selection_dlg::caption( const t_string& t)
		{
			mText = t;
		}
		
		
		t_string mac_selection_dlg::message() const
		{
			return mMessage;
		}
		
		
		void mac_selection_dlg::message( const t_string& m)
		{
			mMessage = m;
		}
		
		
		dialog_result::type mac_selection_dlg::result() const
		{
			return mResult;
		}
		
		
		void mac_selection_dlg::result( const dialog_result::type& r)
		{
			mResult = r;
		}
		
		
		int mac_selection_dlg::choice() const
		{
			return mChoice;
		}
		
		
		void mac_selection_dlg::choice( const int& c)
		{
			mChoice = c;
		}
	}
}
