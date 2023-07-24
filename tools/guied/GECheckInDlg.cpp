#include "..//idlib/precompiled.h"
#pragma hdrstop

#include "../../sys/win32/rc/guied_resource.h"

#include "GEApp.h"

typedef struct {
	const char*		mFilename;
	arcNetString*			mComment;
} GECHECKINDLG;

/*
================
GECheckInDlg_GeneralProc

Dialog procedure for the check in dialog
================
*/
static INT_PTR CALLBACK GECheckInDlg_GeneralProc ( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
	GECHECKINDLG* dlg = (GECHECKINDLG*) GetWindowLong ( hwnd, GWL_USERDATA );

	switch ( msg ) {
		case WM_INITDIALOG:
			SetWindowLong ( hwnd, GWL_USERDATA, lParam );
			dlg = (GECHECKINDLG*) lParam;

			SetWindowText ( GetDlgItem ( hwnd, IDC_GUIED_FILENAME ), dlg->mFilename );
			break;

		case WM_COMMAND:
			switch ( LOWORD ( wParam ) ) {
				case IDOK: {
					char* temp;
					int	  tempsize;

					tempsize = GetWindowTextLength ( GetDlgItem ( hwnd, IDC_GUIED_COMMENT ) );
					temp = new char [ tempsize + 2 ];
					GetWindowText ( GetDlgItem ( hwnd, IDC_GUIED_COMMENT ), temp, tempsize + 1 );

					*dlg->mComment = temp;

					delete[] temp;

					EndDialog ( hwnd, 1 );
					break;
				}
				case IDCANCEL:
					EndDialog ( hwnd, 0 );
					break;
			}
			break;
	}
	return FALSE;
}

/*
================
GECheckInDlg_DoModal

Starts the check in dialog
================
*/
bool GECheckInDlg_DoModal ( HWND parent, const char* filename, arcNetString* comment ) {
	GECHECKINDLG	dlg;

	dlg.mComment = comment;
	dlg.mFilename = filename;

	if ( !DialogBoxParam ( gApp.GetInstance(), MAKEINTRESOURCE(IDD_GUIED_CHECKIN), parent, GECheckInDlg_GeneralProc, (LPARAM) &dlg ) ) {
		return false;
	}
	return true;
}
