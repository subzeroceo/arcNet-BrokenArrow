#include "..//idlib/precompiled.h"
#pragma hdrstop

#include "../../sys/win32/rc/common_resource.h"
#include "DialogName.h"

/////////////////////////////////////////////////////////////////////////////
// DialogName dialog
DialogName::DialogName(const char *pName, CWnd* pParent /*=NULL*/)
	: CDialog(DialogName::IDD, pParent) {
	//{{AFX_DATA_INIT(DialogName)
	m_strName = _T( "" );
	//}}AFX_DATA_INIT
	m_strCaption = pName;
}


void DialogName::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(DialogName)
	DDX_Text(pDX, IDC_TOOLS_EDITNAME, m_strName);
	//}}AFX_DATA_MAP
}

BOOL DialogName::OnInitDialog() {
	CDialog::OnInitDialog();

	SetWindowText(m_strCaption);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(DialogName, CDialog)
	//{{AFX_MSG_MAP(DialogName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// DialogName message handlers

void DialogName::OnOK() {
	CDialog::OnOK();
}
