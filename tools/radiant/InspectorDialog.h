#pragma once
#include "afxcmn.h"

#include "entitydlg.h"
#include "ConsoleDlg.h"
#include "TabsDlg.h"


// CInspectorDialog dialog

class CInspectorDialog : public CTabsDlg
{
	//DECLARE_DYNAMIC(CInspectorDialog)w

public:
	CInspectorDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInspectorDialog();

// Dialog Data
	enum { IDD = IDD_DIALOG_INSPECTORS };

protected:
	bool initialized;
	unsigned int dockedTabs;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	void AssignModel ();
	CTabCtrl tabInspector;
	//idGLConsoleWidget consoleWnd;
	CConsoleDlg consoleWnd;
	CNewTexWnd texWnd;
	CDialogTextures mediaDlg;
	CEntityDlg entityDlg;
	void SetMode( int mode, bool updateTabs = true);
	void UpdateEntitySel(eclass_t *ent);
	void UpdateSelectedEntity();
	void FillClassList();
	bool GetSelectAllCriteria(arcNetString &key, arcNetString &val);

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	void SetDockedTabs ( bool docked , int ID );
};

extern CInspectorDialog *g_Inspectors;