#include "..//idlib/Lib.h"
#pragma hdrstop

#include "qe3.h"
#include "WaitDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CWaitDlg::CWaitDlg(CWnd* pParent, const char *msg)
	: CDialog(CWaitDlg::IDD, pParent) {
	//{{AFX_DATA_INIT(CWaitDlg)
	waitStr = msg;
	//}}AFX_DATA_INIT
	cancelPressed = false;
	Create(CWaitDlg::IDD);
	//g_pParentWnd->SetBusy(true);
}

CWaitDlg::~CWaitDlg() {
	g_pParentWnd->SetBusy(false);
}

void CWaitDlg::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWaitDlg)
	DDX_Text(pDX, IDC_WAITSTR, waitStr);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWaitDlg, CDialog)
	//{{AFX_MSG_MAP(CWaitDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg message handlers

BOOL CWaitDlg::OnInitDialog() {
	CDialog::OnInitDialog();
	//GetDlgItem(IDC_WAITSTR)->SetWindowText(waitStr);
	GetDlgItem(IDC_WAITSTR)->SetFocus();
	UpdateData(FALSE);
	ShowWindow(SW_SHOW);

	// cancel disabled by default
	AllowCancel( false );

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CWaitDlg::SetText(const char *msg, bool append) {
	if (append) {
		waitStr = text;
		waitStr += "\r\n";
		waitStr += msg;
	} else {
		waitStr = msg;
		text = msg;
	}
	UpdateData(FALSE);
	Invalidate();
	UpdateWindow();
	ShowWindow (SW_SHOWNORMAL);
}

void CWaitDlg::AllowCancel( bool enable ) {
	// this shows or hides the Cancel button
	CWnd* pCancelButton = GetDlgItem (IDCANCEL);
	ASSERT (pCancelButton);
	if ( enable ) {
		pCancelButton->ShowWindow (SW_NORMAL);
	} else {
		pCancelButton->ShowWindow (SW_HIDE);
	}
}

bool CWaitDlg::CancelPressed( void ) {
#if _MSC_VER >= 1300
	MSG *msg = AfxGetCurrentMessage();			// TODO Robert fix me!!
#else
	MSG *msg = &m_msgCur;
#endif

	while( ::PeekMessage(msg, nullptr, nullptr, nullptr, PM_NOREMOVE) ) {
		// pump message
		if ( !AfxGetApp()->PumpMessage() ) {
		}
	}

	return cancelPressed;
}

void CWaitDlg::OnCancel() {
	cancelPressed = true;
}
