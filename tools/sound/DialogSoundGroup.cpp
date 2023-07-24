#include "..//idlib/precompiled.h"
#pragma hdrstop

#include "../../sys/win32/rc/SoundEditor_resource.h"

#include "DialogSoundGroup.h"

/////////////////////////////////////////////////////////////////////////////
// CDialogSoundGroup dialog
CDialogSoundGroup::CDialogSoundGroup(CWnd* pParent /*=NULL*/)
	: CDialog(CDialogSoundGroup::IDD, pParent) {
	//{{AFX_DATA_INIT(CDialogSoundGroup)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

void CDialogSoundGroup::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDialogSoundGroup)
	DDX_Control(pDX, IDC_LIST_GROUPS, lstGroups);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDialogSoundGroup, CDialog)
	//{{AFX_MSG_MAP(CDialogSoundGroup)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialogSoundGroup message handlers

void CDialogSoundGroup::OnOK() {
	CString str;
	int count = lstGroups.GetSelCount();
	for ( int i = 0; i < count; i++ ) {
		lstGroups.GetText( i, str);
		list.Append(str.GetBuffer(0 ) );
	}

	CDialog::OnOK();
}

BOOL CDialogSoundGroup::OnInitDialog() {
	CDialog::OnInitDialog();

	int count = list.Num();
	for ( int i = 0; i < count; i++ ) {
		lstGroups.AddString(list[i] );
	}

	return TRUE; // return TRUE unless you set the focus to a control
								// EXCEPTION: OCX Property Pages should return FALSE
}
