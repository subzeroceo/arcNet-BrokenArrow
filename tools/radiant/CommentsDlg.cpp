#include "..//idlib/Lib.h"
#pragma hdrstop

#include "qe3.h"
#include "Radiant.h"
#include "CommentsDlg.h"

IMPLEMENT_DYNAMIC(CCommentsDlg, CDialog)
CCommentsDlg::CCommentsDlg(CWnd* pParent /*=nullptr*/) : CDialog(CCommentsDlg::IDD, pParent), strName(_T( "" ) ), strPath(_T( "" ) ), strComments(_T( "" ) ){
}

CCommentsDlg::~CCommentsDlg(){
}

void CCommentsDlg::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_NAME, strName);
	DDX_Text(pDX, IDC_EDIT_PATH, strPath);
	DDX_Text(pDX, IDC_EDIT_COMMENTS, strComments);
}

BEGIN_MESSAGE_MAP(CCommentsDlg, CDialog)
END_MESSAGE_MAP()