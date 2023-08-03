#include "..//idlib/Lib.h"
#pragma hdrstop

#include "../../sys/win32/rc/debugger_resource.h"
#include "DebuggerApp.h"
#include "DebuggerFindDlg.h"

char rvDebuggerFindDlg::mFindText[ 256 ];

/*
================
rvDebuggerFindDlg::rvDebuggerFindDlg
================
*/
rvDebuggerFindDlg::rvDebuggerFindDlg( void ) {
}

/*
================
rvDebuggerFindDlg::DoModal

Launch the dialog
================
*/
bool rvDebuggerFindDlg::DoModal( rvDebuggerWindow* parent ) {
	if ( DialogBoxParam ( parent->GetInstance(), MAKEINTRESOURCE(IDD_DBG_FIND), parent->GetWindow(), DlgProc, (LONG)this ) ) {
		return true;
	}

	return false;
}

/*
================
rvrvDebuggerFindDlg::DlgProc

Dialog Procedure for the find dialog
================
*/
INT_PTR CALLBACK rvDebuggerFindDlg::DlgProc( HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam ) {
	rvDebuggerFindDlg* dlg = (rvDebuggerFindDlg*) GetWindowLong( wnd, GWL_USERDATA );

	switch ( msg ) {
		case WM_CLOSE:
			EndDialog ( wnd, 0 );
			break;

		case WM_INITDIALOG:
			dlg = (rvDebuggerFindDlg*) lparam;
			SetWindowLong ( wnd, GWL_USERDATA, (LONG) dlg );
			dlg->mWnd = wnd;
			SetWindowText ( GetDlgItem ( dlg->mWnd, IDC_DBG_FIND ), dlg->mFindText );
			return TRUE;

		case WM_COMMAND:
			switch ( LOWORD(wparam) ) {
				case IDOK: {
					GetWindowText ( GetDlgItem ( wnd, IDC_DBG_FIND ), dlg->mFindText, sizeof( dlg->mFindText ) - 1 );
					EndDialog ( wnd, 1 );
					break;
				}

				case IDCANCEL:
					EndDialog ( wnd, 0 );
					break;
			}
			break;
	}

	return FALSE;
}
