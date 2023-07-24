/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "..//idlib/precompiled.h"
#pragma hdrstop

#include "qe3.h"
#include "Radiant.h"
#include "DialogInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDialogInfo dialog
CDialogInfo g_dlgInfo;

void ShowInfoDialog(const char* pText)
{
  if (g_dlgInfo.GetSafeHwnd() )
  {
    g_dlgInfo.m_wndInfo.SetWindowText(pText);
    g_dlgInfo.ShowWindow(SW_SHOW);
  }
  else
  {
    g_dlgInfo.Create(IDD_DLG_INFORMATION);
    g_dlgInfo.m_wndInfo.SetWindowText(pText);
    g_dlgInfo.ShowWindow(SW_SHOW);
  }
  g_pParentWnd->SetFocus();
}

void HideInfoDialog()
{
  if (g_dlgInfo.GetSafeHwnd() )
    g_dlgInfo.ShowWindow(SW_HIDE);
}


CDialogInfo::CDialogInfo(CWnd* pParent /*=NULL*/)
	: CDialog(CDialogInfo::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDialogInfo)
	//}}AFX_DATA_INIT
}


void CDialogInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDialogInfo)
	DDX_Control(pDX, IDC_EDIT1, m_wndInfo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDialogInfo, CDialog)
	//{{AFX_MSG_MAP(CDialogInfo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialogInfo message handlers

BOOL CDialogInfo::OnInitDialog()
{
	CDialog::OnInitDialog();
	// TODO: Add extra initialization here

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
