#include "..//idlib/precompiled.h"
#pragma hdrstop

#include "qe3.h"
#include "Radiant.h"
#include "mediapreviewdlg.h"

IMPLEMENT_DYNAMIC(CMediaPreviewDlg, CDialog)
CMediaPreviewDlg::CMediaPreviewDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMediaPreviewDlg::IDD, pParent) {
	mode = MATERIALS;
	media = "";
}

void CMediaPreviewDlg::SetMedia(const char *_media) {
	media = _media;
	Refresh();
}

void CMediaPreviewDlg::Refresh() {
	if (mode == GUIS) {
		const arcMaterial *mat = declManager->FindMaterial( "guiSurfaces/guipreview" );
		mat->SetGui( media );
		drawMaterial.setMedia( "guiSurfaces/guipreview" );
		drawMaterial.setScale( 4.4f );
	} else {
		drawMaterial.setMedia(media);
		drawMaterial.setScale( 1.0f );
	}
	wndPreview.setDrawable(&drawMaterial);
	wndPreview.Invalidate();
	wndPreview.RedrawWindow();
	RedrawWindow();
}

CMediaPreviewDlg::~CMediaPreviewDlg() {
}

void CMediaPreviewDlg::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PREVIEW, wndPreview);
}

BEGIN_MESSAGE_MAP(CMediaPreviewDlg, CDialog)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

// CMediaPreviewDlg message handlers
BOOL CMediaPreviewDlg::OnInitDialog() {
	CDialog::OnInitDialog();

	wndPreview.setDrawable(&testDrawable);
	CRect rct;
	LONG lSize = sizeof(rct);
	if (LoadRegistryInfo( "Radiant::EditPreviewWindow", &rct, &lSize) )  {
		SetWindowPos(NULL, rct.left, rct.top, rct.Width(), rct.Height(), SWP_SHOWWINDOW);
	}

	GetClientRect(rct);
	int h = (mode == GUIS) ? (rct.Width() - 8) / 1.333333f : rct.Height() - 8;
	wndPreview.SetWindowPos(NULL, 4, 4, rct.Width() - 8, h, SWP_SHOWWINDOW);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CMediaPreviewDlg::OnSize(UINT nType, int cx, int cy) {
	CDialog::OnSize(nType, cx, cy);
	if (wndPreview.GetSafeHwnd() == NULL) {
		return;
	}
	CRect rect;
	GetClientRect(rect);
	//int h = (mode == GUIS) ? (rect.Width() - 8) / 1.333333f : rect.Height() - 8;
	int h = rect.Height() - 8;
	wndPreview.SetWindowPos(NULL, 4, 4, rect.Width() - 8, h, SWP_SHOWWINDOW);
}

void CMediaPreviewDlg::OnDestroy() {
	if (GetSafeHwnd() ) {
		CRect rct;
		GetWindowRect(rct);
		SaveRegistryInfo( "Radiant::EditPreviewWindow", &rct, sizeof(rct) );
	}

	CDialog::OnDestroy();
}

void CMediaPreviewDlg::OnLButtonDown(UINT nFlags, CPoint point) {
	if (mode == GUIS) {
		arcUserInterfaces *gui = uiManager->FindGui( media );
		if (gui) {
			sysEvent_t  ev;
			memset( &ev, 0, sizeof( ev ) );
			ev.evType = SE_KEY;
			ev.evValue = K_MOUSE1;
			ev.evValue2 = 1;
			gui->HandleEvent(&ev,0 );
		}
	}
	CDialog::OnLButtonDown(nFlags, point);
}

void CMediaPreviewDlg::OnLButtonUp(UINT nFlags, CPoint point) {
	if (mode == GUIS) {
		arcUserInterfaces *gui = uiManager->FindGui( media );
		if (gui) {
			sysEvent_t  ev;
			memset( &ev, 0, sizeof( ev ) );
			ev.evType = SE_KEY;
			ev.evValue = K_MOUSE1;
			ev.evValue2 = 0;
			gui->HandleEvent(&ev,0 );
		}
	}
	CDialog::OnLButtonUp(nFlags, point);
}

void CMediaPreviewDlg::OnMouseMove(UINT nFlags, CPoint point) {
	if (mode == GUIS) {
		arcUserInterfaces *gui = uiManager->FindGui( media );
		if (gui) {
			CRect rct;
			wndPreview.GetClientRect(rct);
			sysEvent_t  ev;
			memset( &ev, 0, sizeof( ev ) );
			ev.evType = SE_MOUSE;
			ev.evValue = (point.x / rct.Width() ) * 640.0f;
			ev.evValue2 = (point.y / rct.Height() ) * 480.0f;
			gui->HandleEvent(&ev, 0 );
		}
	}
	CDialog::OnMouseMove(nFlags, point);
}
