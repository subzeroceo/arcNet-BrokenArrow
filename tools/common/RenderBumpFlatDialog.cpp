#include "..//idlib/Lib.h"
#pragma hdrstop
#include "../../sys/win32/common_resource.h"

anCVarSystem rbfg_DefaultWidth( "rbfg_DefaultWidth", "0", 0, "" );
anCVarSystem rbfg_DefaultHeight( "rbfg_DefaultHeight", "0", 0, "" );

static anString RBFName;

static bool CheckPow2( int Num) {
	while( num ) {
		if ((Num & 1 ) && (Num != 1 ) ) {
			return false;
		}

		Num >>= 1;
	}
	return true;
}

extern void Com_WriteConfigToFile( const char *filename );

static BOOL CALLBACK RBFProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG:
			SetDlgItemInt(hwndDlg, IDC_RBF_WIDTH, rbfg_DefaultWidth.GetInteger(), FALSE);
			SetDlgItemInt(hwndDlg, IDC_RBF_HEIGHT, rbfg_DefaultHeight.GetInteger(), FALSE);
			SetDlgItemText(hwndDlg, IDC_RBF_FILENAME, RBFName);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam) ) {
                case IDOK: {
						int		width, height;

						width = GetDlgItemInt(hwndDlg, IDC_RBF_WIDTH, 0, FALSE);
						height = GetDlgItemInt(hwndDlg, IDC_RBF_HEIGHT, 0, FALSE);

						rbfg_DefaultWidth.SetInteger( width );
						rbfg_DefaultHeight.SetInteger( height );

						Com_WriteConfigToFile( CONFIG_FILE );

						if ( !CheckPow2( width ) || !CheckPow2( height ) ) {
							return TRUE;
						}

						DestroyWindow(hwndDlg);

						cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "renderbumpflat -size %d %d %s\n", width, height, RBFName.c_str() ) );
	                    return TRUE;
					}

                case IDCANCEL:
                    DestroyWindow(hwndDlg);
                    return TRUE;
            }
    }
    return FALSE;
}

void DoRBFDialog(const char *FileName) {
	RBFName = FileName;
	Sys_GrabMouseCursor( false );
	DialogBox(0, MAKEINTRESOURCE(IDD_RENDERBUMPFLAT), 0, (DLGPROC)RBFProc);
	Sys_GrabMouseCursor( true );
}
