#ifndef __AFX_STDAFX_H__
#define __AFX_STDAFX_H__

//  include file for standard system include files,
//  or project specific include files that are used frequently, but
//  are changed infrequently

//#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include "afxwin.h"         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC OLE automation classes
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

void InitAfx( void );

// tool tips
typedef struct toolTip_s {
	int id;
	char *tip;
} toolTip_t;

int DefaultOnToolHitTest( const toolTip_t *toolTips, const CDialog *dialog, CPoint point, TOOLINFO* pTI );
BOOL DefaultOnToolTipNotify( const toolTip_t *toolTips, UINT id, NMHDR *pNMHDR, LRESULT *pResult );

// edit control
bool EditControlEnterHit( CEdit *edit );
float EditVerifyFloat( CEdit *edit, bool allowNegative = true );
float EditSpinFloat( CEdit *edit, bool up );

// combo box
int SetSafeComboBoxSelection( CComboBox *combo, const char *string, int skip );
int GetSafeComboBoxSelection( CComboBox *combo, CString &string, int skip );
int UnsetSafeComboBoxSelection( CComboBox *combo, CString &string );
#endif