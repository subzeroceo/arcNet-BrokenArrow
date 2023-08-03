#pragma once

class CMediaPreviewDlg : public CDialog {
	DECLARE_DYNAMIC(CMediaPreviewDlg)

public:
	enum { MATERIALS, GUIS };
	CMediaPreviewDlg(CWnd* pParent = nullptr );   // standard constructor
	virtual ~CMediaPreviewDlg();

	void SetMode( int _mode) {
		mode = _mode;
	}

	void SetMedia(const char *_media);
	void Refresh();

// Dialog Data
	enum { IDD = IDD_DIALOG_EDITPREVIEW };

protected:
	idGLDrawable testDrawable;
	idGLDrawableMaterial drawMaterial;
	idGLWidget wndPreview;
	int mode;
	anString media;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
};
