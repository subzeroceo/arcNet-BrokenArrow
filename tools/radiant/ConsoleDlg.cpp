#include "..//idlib/Lib.h"
#pragma hdrstop

#include "qe3.h"
#include "Radiant.h"
#include "ConsoleDlg.h"

// CConsoleDlg dialog
IMPLEMENT_DYNCREATE(CConsoleDlg, CDialog)
CConsoleDlg::CConsoleDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CConsoleDlg::IDD) {
    currentHistoryPosition = -1;
    currentCommand = "";
	saveCurrentCommand = true;
}

CConsoleDlg::~CConsoleDlg() {
}

void CConsoleDlg::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_CONSOLE, editConsole);
	DDX_Control(pDX, IDC_EDIT_INPUT, editInput);
}

void CConsoleDlg::AddText( const char *msg ) {
	anString work;
	CString work2;

	work = msg;
	work.RemoveColors();
	work = CEntityDlg::TranslateString( work.c_str() );
	editConsole.GetWindowText( work2 );
	int len = work2.GetLength();
	if ( len + work.Length() > ( int )editConsole.GetLimitText() ) {
		work2 = work2.Right( editConsole.GetLimitText() * 0.75 );
		len = work2.GetLength();
		editConsole.SetWindowText(work2);
	}
	editConsole.SetSel( len, len );
	editConsole.ReplaceSel( work );
}

BEGIN_MESSAGE_MAP(CConsoleDlg, CDialog)
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()

// CConsoleDlg message handlers
void CConsoleDlg::OnSize(UINT nType, int cx, int cy) {
	CDialog::OnSize(nType, cx, cy);
	if (editInput.GetSafeHwnd() == nullptr ) {
		return;
	}

	CRect rect, crect;
	GetWindowRect(rect);
	editInput.GetWindowRect(crect);

	editInput.SetWindowPos(nullptr, 4, rect.Height() - 4 - crect.Height(), rect.Width() - 8, crect.Height(), SWP_SHOWWINDOW);
	editConsole.SetWindowPos(nullptr, 4, 4, rect.Width() - 8, rect.Height() - crect.Height() - 8, SWP_SHOWWINDOW);
}

BOOL CConsoleDlg::PreTranslateMessage(MSG* pMsg) {
	if (pMsg->hwnd == editInput.GetSafeHwnd() ) {
		if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE ) {
			Select_Deselect();
			g_pParentWnd->SetFocus ();
			return TRUE;
		}

		if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN ) {
			ExecuteCommand();
			return TRUE;
		}

		if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE ) {
			if (pMsg->wParam == VK_ESCAPE) {
				g_pParentWnd->GetCamera()->SetFocus();
				Select_Deselect();
			}

			return TRUE;
		}

		if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_UP ) {
            //save off the current in-progress command so we can get back to it
			if ( saveCurrentCommand == true ) {
                CString str;
                editInput.GetWindowText ( str );
                currentCommand = str.GetBuffer ( 0 );
				saveCurrentCommand = false;
				}

			if ( consoleHistory.Num () > 0 ) {
				editInput.SetWindowText ( consoleHistory[currentHistoryPosition] );

				int selLocation = consoleHistory[currentHistoryPosition].Length ();
				editInput.SetSel ( selLocation , selLocation + 1 );
				}

			if ( currentHistoryPosition > 0 ) {
                --currentHistoryPosition;
            }

			return TRUE;
		}

		if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_DOWN ) {
            int selLocation = 0;
            if ( currentHistoryPosition < consoleHistory.Num () - 1 ) {
                ++currentHistoryPosition;
                editInput.SetWindowText ( consoleHistory[currentHistoryPosition] );
                selLocation = consoleHistory[currentHistoryPosition].Length ();
            } else {
                editInput.SetWindowText ( currentCommand );
                selLocation = currentCommand.Length ();
				currentCommand.Clear ();
				saveCurrentCommand = true;
            }

            editInput.SetSel ( selLocation , selLocation + 1 );

			return TRUE;
		}
		if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_TAB ) {
			common->Printf ( "Command History\n----------------\n" );
			for ( int i = 0; i < consoleHistory.Num ();i++ ) {
				common->Printf ( "[cmd %d]:  %s\n" , i , consoleHistory[i].c_str() );
			}
			common->Printf ( "----------------\n" );
		}
		if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_NEXT) {
			editConsole.LineScroll ( 10 );
		}

		if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_PRIOR ) {
			editConsole.LineScroll ( -10 );
		}

		if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_HOME ) {
			editConsole.LineScroll ( -editConsole.GetLineCount() );
		}

		if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_END ) {
			editConsole.LineScroll ( editConsole.GetLineCount() );
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CConsoleDlg::OnSetFocus(CWnd* pOldWnd) {
	CDialog::OnSetFocus(pOldWnd);
	editInput.SetFocus();
}

void CConsoleDlg::SetConsoleText ( const anString& text ) {
	editInput.Clear ();
	editInput.SetWindowText ( text.c_str() );
}

void CConsoleDlg::ExecuteCommand ( const anString& cmd ) {
	CString str;
	if ( cmd.Length() > 0 ) {
		str = cmd;
	} else {
		editInput.GetWindowText(str);
	}
	if ( str != "" ) {
		editInput.SetWindowText( "" );
		common->Printf( "%s\n", str.GetBuffer(0 ) );

		//avoid adding multiple identical commands in a row
		int index = consoleHistory.Num ();

		if ( index == 0 || str.GetBuffer(0 ) != consoleHistory[index-1] ) {
			//keep the history to 16 commands, removing the oldest command
			if ( consoleHistory.Num () > 16 ) {
				consoleHistory.RemoveIndex ( 0 );
			}
			currentHistoryPosition = consoleHistory.Append ( str.GetBuffer (0 ) );
		} else {
			currentHistoryPosition = consoleHistory.Num () - 1;
		}

		currentCommand.Clear ();

		bool propogateCommand = true;

		//process some of our own special commands
		if ( str.CompareNoCase ( "clear" ) == 0 ) {
			editConsole.SetSel ( 0 , -1 );
			editConsole.Clear ();
		} else if ( str.CompareNoCase ( "edit" ) == 0 ) {
			propogateCommand = false;
		}
		if ( propogateCommand ) {
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, str );
		}

		Sys_UpdateWindows(W_ALL);
	}
}

void CConsoleDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) {
	CDialog::OnActivate(nState, pWndOther, bMinimized);
	if ( nState == WA_ACTIVE || nState == WA_CLICKACTIVE ) {
		editInput.SetFocus();
	}
}
