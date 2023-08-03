#include "..//idlib/Lib.h"
#pragma hdrstop

#define MASKEDIT_MAXINVALID	1024
typedef struct {
	WNDPROC	mProc;
	char	mInvalid[MASKEDIT_MAXINVALID];
} rvGEMaskEdit;

/*
================
MaskEdit_WndProc

Prevents the invalid characters from being entered
================
*/
LRESULT CALLBACK MaskEdit_WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
	rvGEMaskEdit* edit = (rvGEMaskEdit*)GetWindowLong ( hWnd, GWL_USERDATA );
	WNDPROC		  wndproc = edit->mProc;

	switch ( msg )
	{
		case WM_CHAR:
			if ( strchr ( edit->mInvalid, wParam ) )
			{
				return 0;
			}

			break;

		case WM_DESTROY:
			delete edit;
			SetWindowLong ( hWnd, GWL_WNDPROC, (LONG)wndproc );
			break;
	}

	return CallWindowProc ( wndproc, hWnd, msg, wParam, lParam );
}

/*
================
MaskEdit_Attach

Attaches the mask edit control to a normal edit control
================
*/
void MaskEdit_Attach( HWND hWnd, const char* invalid ) {
	rvGEMaskEdit* edit = new rvGEMaskEdit;
	edit->mProc = (WNDPROC)GetWindowLong ( hWnd, GWL_WNDPROC );
	strcpy ( edit->mInvalid, invalid );
	SetWindowLong ( hWnd, GWL_USERDATA, (LONG)edit );
	SetWindowLong ( hWnd, GWL_WNDPROC, (LONG)MaskEdit_WndProc );
}

/*
================
NumberEdit_Attach

Allows editing of floating point numbers
================
*/
void NumberEdit_Attach( HWND hWnd ) {
	static const char invalid[] = "`~!@#$%^&*()_+|=\\qwertyuiop[]asdfghjkl;'zxcvbnm,/QWERTYUIOP{}ASDFGHJKL:ZXCVBNM<>";
	MaskEdit_Attach ( hWnd, invalid );
}
