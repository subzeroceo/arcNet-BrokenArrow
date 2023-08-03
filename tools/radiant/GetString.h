#if !defined(__GETSTRING_H__)
#define __GETSTRING_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// CGetString dialog

// NOTE: already included in qe3.h but won't compile without including it again !?
#include "../../sys/win32/rc/Radiant_resource.h"

class CGetString : public CDialog {
public:
	CGetString(LPCSTR pPrompt, CString *pFeedback, CWnd* pParent = nullptr );   // standard constructor
	virtual ~CGetString();
// Overrides

// Dialog Data

	enum { IDD = IDD_DIALOG_GETSTRING };
	
	CString	m_strEditBox;
	CString *m_pFeedback;
	LPCSTR	m_pPrompt;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
};

LPCSTR GetString(LPCSTR psPrompt);
bool GetYesNo(const char *psQuery);
void ErrorBox(const char *sString);
void InfoBox(const char *sString);
void WarningBox(const char *sString);

#endif /* !__GETSTRING_H__ */
