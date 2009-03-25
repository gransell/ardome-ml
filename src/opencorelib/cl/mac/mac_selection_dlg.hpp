#ifndef _AUBASE_MAC_SELECTIONDIALOG_H_
#define _AUBASE_MAC_SELECTIONDIALOG_H_

#include <vector>

#include <opencorelib/cl/minimal_string_defines.hpp>
#include <opencorelib/cl/selection_dialog.hpp>

namespace olib
{
	namespace opencorelib
	{
		/// Implementation of a dialog that can display an edit box and some buttons with choices
		/** 
			@sa CExceptionPresenter
			@sa CAssertPresenter
			@author Mikael Gransellf*/
        class mac_selection_dlg : public selection_dialog
		{	
		public:
			typedef std::vector< std::pair< t_string, int> > ValuesCont;
		
			mac_selection_dlg();

            void add_value( const t_string& valName, int val ) 
            { 
                mPossibleValues.push_back(std::make_pair(valName, val)); 
            }
            
            void set_assembly_run_as_service( bool v )
            {
            }

            void show_dialog();

			t_string caption() const;
			void caption( const t_string& t);

			t_string message() const;
			void message( const t_string& m);

			dialog_result::type result() const;
			void result( const dialog_result::type& r);

			int choice() const;
			void choice( const int& c);
        
        private:
            t_string mText;
            t_string mMessage;
            dialog_result::type mResult;
            int mChoice;
            ValuesCont mPossibleValues;
		};

	}
}

#endif // _AUBASE_MSW_SELECTIONDIALOG_H_
