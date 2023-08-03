#include "..//idlib/Lib.h"
#pragma hdrstop

#include "qe3.h"
#include "Radiant.h"
#include "CommandsDlg.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCommandsDlg dialog
CCommandsDlg::CCommandsDlg(CWnd* pParent /*=nullptr*/) : CDialog(CCommandsDlg::IDD, pParent) {
	//{{AFX_DATA_INIT(CCommandsDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

void CCommandsDlg::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCommandsDlg)
	DDX_Control(pDX, IDC_LIST_COMMANDS, m_lstCommands);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CCommandsDlg, CDialog)
	//{{AFX_MSG_MAP(CCommandsDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCommandsDlg message handlers
BOOL CCommandsDlg::OnInitDialog() {
	CDialog::OnInitDialog();
	m_lstCommands.SetTabStops(120);
	int nCount = g_nCommandCount;

	CFile fileout;
	fileout.Open( "c:/commandlist.txt", CFile::modeCreate | CFile::modeWrite);
	for ( int n = 0; n < nCount; n++ ) {
		CString strLine;
		char c = g_Commands[n].m_nKey;
		CString strKeys = CString( c );
		for ( int k = 0; k < g_nKeyCount; k++ ) {
			if (g_Keys[k].m_nVKKey == g_Commands[n].m_nKey) {
				strKeys = g_Keys[k].m_strName;
				break;
			}
		}
		CString strMod( "" );
		if (g_Commands[n].m_nModifiers & RAD_SHIFT)
			strMod = "Shift";
		if (g_Commands[n].m_nModifiers & RAD_ALT)
			strMod += (strMod.GetLength() > 0 ) ? " + Alt" : "Alt";
		if (g_Commands[n].m_nModifiers & RAD_CONTROL)
			strMod += (strMod.GetLength() > 0 ) ? " + Control" : "Control";
		if (strMod.GetLength() > 0 ) {
			strMod += " + ";
		}
		strLine.Format( "%s \t%s%s", g_Commands[n].m_strCommand, strMod, strKeys);
		m_lstCommands.AddString(strLine);

		strLine.Format( "%s \t\t\t%s%s", g_Commands[n].m_strCommand, strMod, strKeys);

		fileout.Write(strLine, strLine.GetLength() );
		fileout.Write( "\r\n", 2);
	}
	fileout.Close();
	return TRUE;
}

