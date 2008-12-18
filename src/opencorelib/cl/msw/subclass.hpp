#ifndef _CORE_MSW_SUBCLASS_WND_H_
#define _CORE_MSW_SUBCLASS_WND_H_

#include <map>

/// Helper class to facilitate sub-classing.
/**	This class replaces the default window procedure installed 
	at window creation with the HookWndProc. This will basically 
	find the correct instance of subclass_wnd and call its WindowProc
	function. It will in turn just call the original function in the 
	base class implementation.
	
	Original implementation from Paul Dilascia, MSDN Magazine, 
	April 2006, C++ At work, p. 129

	@author Mats Lindel&ouml;f*/
namespace olib
{
	namespace opencorelib
	{
		class CORE_API subclass_wnd
		{
		public:
			/// Default constructor.
			subclass_wnd();

			///Unhooks the window if its not already done.
			~subclass_wnd();

			/// Hook the given window. 
			/** Hook means to replace the window procedure given at 
				window creation time (see RegisterClass and CreateWindow 
				in the Windows API documentation.) with another window 
				procedure. The replacement procedure just calls the 
				original one but this behaviour could easily be altered 
				by inheriting from this class. 
				@param hwnd The window handle whose procedure will be hooked.*/
			bool hook_window(HWND  hwnd);

			/// Remove any previous hooks.
			void unhook(){ hook_window((HWND)NULL); }

			/// Is this instance hooked yet?
			bool is_hooked(){ return m_hwnd != NULL; }

            HWND get_hooked_wnd() const { return m_hwnd; }

			/// The hook procedure. Not used by external consumers of this class.
			friend LRESULT CALLBACK hook_wnd_proc(HWND, UINT, WPARAM, LPARAM);	
		protected:

			/// Override this function to alter the behavior of the hooked window.
			/** Make sure you always call the this implementation at the end of
				your own. This is to make sure all default behavior continues to
				work as before the hooking took place. 
				@param msg The window message.
				@param wp The wparam. Meaning is dependent on msg.
				@param wp The lparam. Meaning is dependent on msg. */
			virtual LRESULT window_proc(UINT msg, WPARAM wp, LPARAM lp);	

			/// The hooked window handle.
			HWND m_hwnd;			
			/// The old window procedure for m_hWnd.
			WNDPROC	m_old_wnd_proc;	

			/// The next window.
			/** The first time a HWND is sub-classed this parameter is null. If the
				window already is subclassed, the subclass_wnd also becomes a linked
				list, where m_pNext points to the next node in the list.*/
			subclass_wnd* m_next;	

			/// Maps a HWND to a given CSubclass instance.
			/** This map is used by the HookWndProc to find the correct instance 
				to call subclass_wnd::WindowProc on. */
			typedef std::map< HWND, subclass_wnd*> hook_map;
			static hook_map the_hook_map;

			static void add(HWND hwnd, subclass_wnd* pSubclassWnd);
			static void remove(subclass_wnd* pUnHook);
			static void remove_all(HWND hwnd);
		};
	}
}

#endif // _CORE_MSW_SUBCLASS_WND_H_

