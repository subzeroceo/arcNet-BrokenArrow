#include "..//idlib/precompiled.h"
#pragma hdrstop

#include "../common/ColorButton.h"
#include "GEApp.h"
#include "GEPropertyPage.h"

rvGEPropertyPage::rvGEPropertyPage ( )
{
	mPage = NULL;
}

/*
================
rvGEPropertyPage::WndProc

Window procedure for the property page class.
================
*/
INT_PTR CALLBACK rvGEPropertyPage::WndProc ( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	rvGEPropertyPage* page = (rvGEPropertyPage*) GetWindowLong ( hwnd, GWL_USERDATA );

	// Pages dont get the init dialog since their Init method is called instead
	if ( msg == WM_INITDIALOG )
	{
		PROPSHEETPAGE* psp = (PROPSHEETPAGE*) lParam;

		page = (rvGEPropertyPage*) psp->lParam;

		SetWindowLong ( hwnd, GWL_USERDATA, (LONG)page );
		page->mPage = hwnd;

		page->Init ( );

		return FALSE;
	}
	else if ( !page )
	{
		return FALSE;
	}

	// See if the derived class wants to handle the message
	return page->HandleMessage ( msg, wParam, lParam );
}

/*
================
rvGEPropertyPage::HandleMessage

Handles all messages that the base property page must handle.
================
*/
int rvGEPropertyPage::HandleMessage ( UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
		case WM_NOTIFY:
			switch (((NMHDR FAR *) lParam)->code)
			{
				case PSN_APPLY:
					if ( !Apply ( ) )
					{
						SetWindowLong ( mPage, DWL_MSGRESULT, PSNRET_INVALID );
						return TRUE;
					}
					break;

				case PSN_SETACTIVE:
					SetActive ( );
					break;

				case PSN_KILLACTIVE:
					KillActive ( );
					break;
			}
			break;
	}

	return FALSE;
}

