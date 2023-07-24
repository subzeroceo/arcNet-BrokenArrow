#include "..//idlib/precompiled.h"
#pragma hdrstop

#include "qe3.h"
#include "Radiant.h"
#include "CapDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCapDialog::CCapDialog(CWnd* pParent /*=NULL*/) : CDialog(CCapDialog::IDD, pParent) {
	m_nCap = 0;
}


void CCapDialog::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO_CAP, m_nCap);
}


BEGIN_MESSAGE_MAP(CCapDialog, CDialog)
END_MESSAGE_MAP()