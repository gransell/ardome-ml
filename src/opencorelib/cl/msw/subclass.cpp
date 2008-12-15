#include "precompiled_headers.hpp"
#include "./subclass.hpp"
#include "../assert_defines.hpp"
#include "../assert.hpp"

namespace olib
{
	namespace opencorelib
	{
		subclass_wnd::hook_map subclass_wnd::the_hook_map;

		subclass_wnd::subclass_wnd()
		{
			m_next = NULL;
			m_old_wnd_proc = NULL;	
			m_hwnd  = NULL;
		}

		subclass_wnd::~subclass_wnd()
		{
			if (m_hwnd) unhook(); 
		}

		bool subclass_wnd::hook_window(HWND hwnd)
		{
			if (hwnd) 
			{
				ARASSERT( m_hwnd == NULL );
				ARASSERT(::IsWindow(hwnd));
				add(hwnd, this);
			} 
			else if (m_hwnd) 
			{
				// Unhook the window
				remove(this);
				m_old_wnd_proc = NULL;
			}
			m_hwnd = hwnd;
			return TRUE;
		}

		LRESULT subclass_wnd::window_proc(UINT msg, WPARAM wp, LPARAM lp)
		{
			ARASSERT(m_old_wnd_proc);
			return m_next ? m_next->window_proc(msg, wp, lp) :	
						::CallWindowProc(m_old_wnd_proc, m_hwnd, msg, wp, lp);
		}

		LRESULT CALLBACK hook_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
		{
			subclass_wnd::hook_map::iterator it = subclass_wnd::the_hook_map.find(hwnd);
			ARASSERT( it != subclass_wnd::the_hook_map.end());
			if( it == subclass_wnd::the_hook_map.end()) return 0;

			subclass_wnd* a_subclass_wnd = it->second;
			ARASSERT(a_subclass_wnd);

			if ( msg == WM_NCDESTROY ) 
			{
				WNDPROC wndproc = a_subclass_wnd->m_old_wnd_proc;
				subclass_wnd::remove_all(hwnd);
				return ::CallWindowProc(wndproc, hwnd, msg, wp, lp);
			} 
			else 
			{
				return a_subclass_wnd->window_proc(msg, wp, lp);
			}
		}


		void subclass_wnd::add(HWND hwnd, subclass_wnd* pSubclassWnd)
		{
			ARASSERT( hwnd && ::IsWindow(hwnd) );

			hook_map::iterator it = the_hook_map.find(hwnd);
			pSubclassWnd->m_next = it == the_hook_map.end() ? 0 : it->second;
			the_hook_map[hwnd] = pSubclassWnd;

			if (pSubclassWnd->m_next == NULL) 
			{
				// If this is the first hook added, subclass the window
				pSubclassWnd->m_old_wnd_proc = 
					(WNDPROC)(INT_PTR)SetWindowLong(hwnd, GWL_WNDPROC, (LONG)(INT_PTR)hook_wnd_proc);

			} 
			else 
			{
				pSubclassWnd->m_old_wnd_proc = pSubclassWnd->m_next->m_old_wnd_proc;
			}

			ARASSERT(pSubclassWnd->m_old_wnd_proc);
		}

		void subclass_wnd::remove(subclass_wnd* un_hook_wnd)
		{
			HWND hwnd = un_hook_wnd->m_hwnd;
			ARASSERT(hwnd && ::IsWindow(hwnd));

			hook_map::iterator it = the_hook_map.find(hwnd);
			ARASSERT(it != the_hook_map.end() );
			if( it == the_hook_map.end()) return;

			subclass_wnd* a_hook = it->second;

			if ( a_hook == un_hook_wnd) 
			{
				if (a_hook->m_next)
				{
					the_hook_map[hwnd] = a_hook->m_next;
				}
				else 
				{
					// This is the last hook for this window: restore wnd proc
					the_hook_map.erase(it);
					SetWindowLong(hwnd, GWL_WNDPROC, (LONG)(INT_PTR)a_hook->m_old_wnd_proc);
				}
			} 
			else 
			{
				// Hook to remove is in the middle: just remove from linked list
				while (a_hook->m_next != un_hook_wnd)
					a_hook = a_hook->m_next;
				ARASSERT(a_hook && a_hook->m_next == un_hook_wnd);
				a_hook->m_next = un_hook_wnd->m_next;
			}
		}

		void subclass_wnd::remove_all(HWND hwnd)
		{
			hook_map::iterator it = the_hook_map.find(hwnd);
			for( ; it != the_hook_map.end(); it = the_hook_map.find(hwnd))
			{
				it->second->unhook();	
			}	
		}
	}
}

