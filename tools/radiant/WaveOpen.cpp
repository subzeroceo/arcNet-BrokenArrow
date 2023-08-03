#include "..//idlib/Lib.h"
#pragma hdrstop

#include "qe3.h"
#include "Radiant.h"
#include "WaveOpen.h"
#include "mmsystem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWaveOpen

IMPLEMENT_DYNAMIC(CWaveOpen, CFileDialog)

CWaveOpen::CWaveOpen(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
		DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd) :
		CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd)
{
  m_ofn.Flags |= (OFN_EXPLORER | OFN_ENABLETEMPLATE);
  m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_PLAYWAVE);
}


BEGIN_MESSAGE_MAP(CWaveOpen, CFileDialog)
	//{{AFX_MSG_MAP(CWaveOpen)
	ON_BN_CLICKED(IDC_BTN_PLAY, OnBtnPlay)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CWaveOpen::OnFileNameChange() {
  CString str = GetPathName();
  str.MakeLower();
  CWnd *pWnd = GetDlgItem(IDC_BTN_PLAY);
  if (pWnd == nullptr )
  {
    return;
  }
  if (str.Find( ".wav" ) >= 0 )
  {
    pWnd->EnableWindow(TRUE);
  }
  else
  {
    pWnd->EnableWindow(FALSE);
  }
}

void CWaveOpen::OnBtnPlay() {
    sndPlaySound(nullptr, nullptr );
    CString str = GetPathName();
    if (str.GetLength() > 0 ) {
      sndPlaySound(str, SND_FILENAME | SND_ASYNC);
    }
}

BOOL CWaveOpen::OnInitDialog() {
	CFileDialog::OnInitDialog();

  CWnd *pWnd = GetDlgItem(IDC_BTN_PLAY);
  if (pWnd != nullptr ) {
    pWnd->EnableWindow(FALSE);
  }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}