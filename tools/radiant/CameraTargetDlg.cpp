#include "..//idlib/Lib.h"
#pragma hdrstop

#include "qe3.h"
#include "CameraTargetDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCameraTargetDlg::CCameraTargetDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CCameraTargetDlg::IDD, pParent) {
	m_nType = 0;
	m_strName = _T( "" );
}

void CCameraTargetDlg::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO_FIXED, m_nType);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
}

BEGIN_MESSAGE_MAP(CCameraTargetDlg, CDialog)
	ON_COMMAND(ID_POPUP_NEWCAMERA_FIXED, OnPopupNewcameraFixed)
END_MESSAGE_MAP()

void CCameraTargetDlg::OnPopupNewcameraFixed() {
	// TODO: Add your command handler code here
}
