#ifndef _CORE_MSW_SELECTIONDIALOG_H_
#define _CORE_MSW_SELECTIONDIALOG_H_

// Present assertions under Windows
#ifdef OLIB_ON_WINDOWS

#include <vector>
#include <algorithm>
#include <windows.h>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include "../string_defines.hpp"
#include "../utilities.hpp"
#include "../guard_define.hpp"
#include "../selection_dialog.hpp"
#include "resource.hpp"
#include "../macro_definitions.hpp"

HMODULE get_dll_module_handle();

namespace olib
{
	namespace opencorelib
	{
		/// Implementation of a dialog that can display an edit box and some buttons with choices
		/** 
			@sa exception_presenter
			@sa assert_presenter
			@author Mats Lindel&ouml;f*/
		template <typename T>
        class windows_selection_dialog : public selection_dialog
		{	
		public:
			typedef std::vector< std::pair< t_string, T> > values_cont;
		
			windows_selection_dialog();

            void add_value( const t_string& val_name, int val ) 
            { 
                m_possible_values.push_back(std::make_pair(val_name, val)); 
            }
            
            void set_assembly_run_as_service( bool v )
            {
                m_assembly_run_asService = v;
            }

            void show_dialog();
			void show_dialog( HWND hw );

			void create_btns(HWND hw) const;
			void create_editbox(HWND hw) const;

			void resize( int width, int height, HWND hw);

			void handle_command( DWORD cmd, HWND hw );

			t_string caption() const;
			void caption( const t_string& t);

			t_string message() const;
			void message( const t_string& m);

			dialog_result::type result() const;
			void result( const dialog_result::type& r);

			T choice() const;
			void choice( const T& c);
        
        private:
            HWND m_hwnd;
            bool m_assembly_run_asService;
            static std::list< windows_selection_dialog<T>* > m_dialogs;
            static BOOL CALLBACK select_dlg_proc(HWND hwnd, UINT Message, WPARAM w_param, LPARAM l_param);
            BOOL dlg_proc(UINT Message, WPARAM w_param, LPARAM l_param);
            static bool compare( windows_selection_dialog* sd1, HWND hw);
            static void register_window(windows_selection_dialog<T>& sd);
            static void unregister_window(windows_selection_dialog<T>& sd);

            t_string m_text;
            t_string m_message;
            dialog_result::type m_result;
            T m_choice;
            values_cont m_possible_values;
            static void add_carriage_return( const TCHAR& c, t_string& res);	

            void show_dialog_internal(HWND hw);

		};

		static const unsigned __int64 base_id(2000);
		static const unsigned __int64 edit_id(1000);

		template <typename T>
		std::list< windows_selection_dialog<T>* > windows_selection_dialog<T>::m_dialogs;

		template <typename T>
		void windows_selection_dialog<T>::register_window(windows_selection_dialog<T>& sd)
		{
			m_dialogs.push_back(&sd);
		}

		template <typename T>
		bool windows_selection_dialog<T>::compare( windows_selection_dialog<T>* sd1, HWND hw )
		{
			assert( sd1 );
			return sd1->m_hwnd == hw;
		}

		template <typename T>
		void windows_selection_dialog<T>::unregister_window(windows_selection_dialog<T>& sd)
		{
			using boost::ref; using boost::bind;
			std::list< windows_selection_dialog<T>* >::iterator fit;
			fit = std::find_if(m_dialogs.begin(), m_dialogs.end(), bind(compare, _1, sd.m_hwnd ));
			if( fit != m_dialogs.end()) 
			{
				m_dialogs.erase(fit);
				return;
			}
			
			assert(false);
		}

		template< typename T>
			windows_selection_dialog<T>::windows_selection_dialog() : m_result( dialog_result::cancel )
		{
		}

		template< typename T>
		BOOL CALLBACK windows_selection_dialog<T>::select_dlg_proc(HWND hwnd, UINT Message, WPARAM w_param, LPARAM l_param)
		{
			std::list< windows_selection_dialog<T>* >::iterator fit;
			fit = std::find_if(m_dialogs.begin(), m_dialogs.end(), bind(compare, _1, hwnd));
			if( fit != m_dialogs.end()) 
			{
				return (*fit)->dlg_proc(Message, w_param, l_param);
			}

			return FALSE;
		}
		

		template< typename T>
		BOOL windows_selection_dialog<T>::dlg_proc(UINT Message, WPARAM w_param, LPARAM l_param)
		{
			ARUNUSED_ALWAYS(l_param);
			switch(Message)
			{
				case WM_INITDIALOG:
					{
						::SendMessage( m_hwnd, WM_SETTEXT, 0, (LPARAM)(m_text.c_str())); 
						HICON h_icon = ::LoadIcon( get_dll_module_handle() , MAKEINTRESOURCE(IDI_ICON1));
						create_editbox(m_hwnd);
						create_btns(m_hwnd);
						::SendMessage(m_hwnd, WM_SIZE, 0, 0);
						::SendMessage(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)h_icon);
						::SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)h_icon);
						return TRUE;
					}
				case WM_SIZE:
					{
						RECT rc_client;
						::GetClientRect(m_hwnd, &rc_client);
						resize(rc_client.right, rc_client.bottom, m_hwnd);
						break;
					}
				case WM_COMMAND:
						handle_command(LOWORD(w_param), m_hwnd);
						break;
				
				default: return FALSE;
			}
			return FALSE;
		}

		template <typename T>
		void windows_selection_dialog<T>::create_btns(HWND hw) const
		{
			HGDIOBJ hf_default = ::GetStockObject(DEFAULT_GUI_FONT);
			for( size_t i = 0 ; i < m_possible_values.size(); ++i)
			{
				__int64 btn_id = base_id + static_cast<int>(i);
				HWND h_btn = ::CreateWindowEx(WS_EX_WINDOWEDGE, _T("BUTTON"), m_possible_values[i].first.c_str(), 
											WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 200 + static_cast<int>(i) * 20, 200, 20, hw , 
											(HMENU)btn_id, get_dll_module_handle(), NULL);
				if(!h_btn) return;
				::SendMessage(h_btn, WM_SETFONT, (WPARAM)hf_default, MAKELPARAM(FALSE, 0));
			}
		}

		template <typename T>
		void windows_selection_dialog<T>::create_editbox(HWND hw) const
		{
			HWND h_edit = ::CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""), 
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL 
			| ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY, 
            0, 0, 100, 100, hw , (HMENU)edit_id, get_dll_module_handle(), NULL);
			
			if(!h_edit) return;

			HGDIOBJ hf_default = ::GetStockObject(DEFAULT_GUI_FONT);
			::SendMessage(h_edit, WM_SETFONT, (WPARAM)hf_default, MAKELPARAM(FALSE, 0));

			::SetDlgItemText(hw, edit_id , m_message.c_str());
		}

		template <typename T>
		void windows_selection_dialog<T>::resize( int width, int height, HWND hw)
		{
			int btn_width = width / static_cast<int>(m_possible_values.size());
			int btn_height = 30;
			int extra_margin = 5;
			HWND h_edit = ::GetDlgItem(hw, edit_id);
			::SetWindowPos(h_edit, NULL, 0, 0, width, height - btn_height - extra_margin, SWP_NOZORDER);
			for( size_t i = 0 ; i < m_possible_values.size(); ++i)
			{
				__int64 btn_id = base_id + static_cast<int>(i);
				HWND h_btn = ::GetDlgItem(hw, static_cast<int>(btn_id));
				int ii = static_cast<int>(i);
				::SetWindowPos(h_btn, NULL, 0 + ii * btn_width , 
					height - btn_height , btn_width, btn_height, SWP_NOZORDER);
			}
		}

		template <typename T>
		void windows_selection_dialog<T>::handle_command( DWORD cmd, HWND hw )
		{
			ARUNUSED_ALWAYS(hw);
			__int64 btn_id = static_cast<__int64>(cmd) - base_id ;
			if(cmd == IDCANCEL )
			{
				m_result = dialog_result::cancel ;
				::PostQuitMessage(0);
			}
			else if( btn_id >= 0 && btn_id < m_possible_values.size() )
			{
				m_result = dialog_result::selection_made;
				m_choice = m_possible_values[static_cast<int>(btn_id)].second;
				::PostQuitMessage(0);
			}
		}

        template <typename T>
        void windows_selection_dialog<T>::show_dialog()
        {
            show_dialog(0);
        }

		template <typename T>
		void windows_selection_dialog<T>::show_dialog( HWND hw)
		{
			using namespace boost;

			if(m_assembly_run_asService)
			{
                HWINSTA curr_station = ::GetProcessWindowStation(); 
                DWORD dw_thread_id = ::GetCurrentThreadId(); 
                HDESK curr_desk = ::GetThreadDesktop(dw_thread_id); 

                HWINSTA win_sta0 = ::OpenWindowStation(_T("Win_sta0"), FALSE, MAXIMUM_ALLOWED); 
				if(!win_sta0) return;

                ARGUARD( bind<BOOL>(::SetProcessWindowStation, curr_station));
				SetProcessWindowStation(win_sta0);

                HDESK user_desk = ::OpenDesktop(_T("Default"), 0, FALSE, MAXIMUM_ALLOWED); 
				if(!user_desk) return;

                ARGUARD( bind<BOOL>(::SetThreadDesktop, curr_desk));
                ::SetThreadDesktop(user_desk);
				
				show_dialog_internal(hw);
			}
			else
			{
				show_dialog_internal(hw);
			}
		}

		template <typename T>
		void windows_selection_dialog<T>::show_dialog_internal(HWND hw)
		{
			HMODULE hmod = get_dll_module_handle();
			if( hmod == 0 )
			{
				utilities::handle_system_error(true);
				return;
			}

            if(hw == NULL) hw = ::GetDesktopWindow(); 

			// Make sure we do not show the dialog if no parent window is found!!
			if(hw == NULL) return;

			register_window(*this);

			m_hwnd = ::CreateDialog( hmod, MAKEINTRESOURCE(IDD_ASSERT_DIALOG_2), hw, select_dlg_proc);

			if(m_hwnd == 0)
			{
				utilities::handle_system_error(false);
				return;
			}

			// Create_dialog will not have initialized the dialog 
			// since the dialog wasn't registered yet.
			::SendMessage( m_hwnd, WM_INITDIALOG, 0, 0); 

            RECT owner_rect = {0,0,0,0};
            int width(800), t_height(500);

            ::GetWindowRect(hw, &owner_rect);

            int owner_width = owner_rect.right - owner_rect.left;
            int owner_height = owner_rect.bottom - owner_rect.top;

            ::SetWindowPos(m_hwnd,HWND_TOP, 
                            owner_rect.left + owner_width/2 - width/2, 
                            owner_rect.top + owner_height/2 - t_height/2,
                            width, t_height, SWP_SHOWWINDOW );

            ::UpdateWindow(m_hwnd);

			MSG msg;
			while(::GetMessage(&msg, NULL, 0, 0) > 0)
			{
				::IsDialogMessage(m_hwnd, &msg);
			}

			BOOL destroy_window_ok = ::DestroyWindow(m_hwnd);
			assert( destroy_window_ok );
			unregister_window(*this);
		}

		template <typename T>
		t_string windows_selection_dialog<T>::caption() const
		{
			return m_text;
		}

		template <typename T>
		void windows_selection_dialog<T>::caption( const t_string& t)
		{
			m_text = t;
		}

		template <typename T>
		t_string windows_selection_dialog<T>::message() const
		{
			return m_message;
		}

		template <typename T>
		void windows_selection_dialog<T>::add_carriage_return( const TCHAR& c, t_string& res)
		{
			if(c == '\n') 
			{
				res.push_back('\r'); res.push_back('\n');
			}
			else
				res.push_back(c);
		}

		template <typename T>
		void windows_selection_dialog<T>::message( const t_string& m)
		{
			using boost::ref; using boost::bind;
			m_message.clear();
			std::for_each(m.begin(), m.end(), bind( add_carriage_return, _1, ref(m_message)));
		}

		template <typename T>
		dialog_result::type windows_selection_dialog<T>::result() const
		{
			return m_result;
		}

		template <typename T>
		void windows_selection_dialog<T>::result( const dialog_result::type& r)
		{
			m_result = r;
		}

		template <typename T>
		T windows_selection_dialog<T>::choice() const
		{
			return m_choice;
		}

		template <typename T>
		void windows_selection_dialog<T>::choice( const T& c)
		{
			m_choice = c;
		}
	}
}

#endif // OLIB_ON_WINDOWS

#endif // _CORE_MSW_SELECTIONDIALOG_H_