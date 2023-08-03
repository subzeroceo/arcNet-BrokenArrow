#include "..//idlib/Lib.h"
#pragma hdrstop

#include "../../sys/win32/win_local.h"

#include "MaterialEditor.h"
#include "MEMainFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

MEMainFrame* meMainFrame = nullptr;

CFont* materialEditorFont = nullptr;

/**
* Initializes the material editor tool.
*/
void MaterialEditorInit( void ) {
	InitPropTree(win32.hInstance);

	com_editors = EDITOR_MATERIAL;

	Sys_GrabMouseCursor( false );

	InitAfx();
	InitCommonControls();

	// Initialize OLE libraries
	if ( !AfxOleInit() )
	{
		return;
	}
	AfxEnableControlContainer();

	NONCLIENTMETRICS info;
	info.cbSize = sizeof(info);

	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(info), &info, 0 );

	LOGFONT lf;
	memset(&lf, 0, sizeof (LOGFONT) );

	CWindowDC dc(nullptr );
	lf.lfCharSet = (BYTE)GetTextCharsetInfo(dc.GetSafeHdc(), nullptr, 0 );

	lf.lfHeight = info.lfMenuFont.lfHeight;
	lf.lfWeight = info.lfMenuFont.lfWeight;
	lf.lfItalic = info.lfMenuFont.lfItalic;

	// check if we should use system font
	_tcscpy(lf.lfFaceName, info.lfMenuFont.lfFaceName);

	materialEditorFont = new CFont;
	materialEditorFont->CreateFontIndirect(&lf);


	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object
	meMainFrame = new MEMainFrame;

	// create and load the frame with its resources
	meMainFrame->LoadFrame(IDR_ME_MAINFRAME, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, nullptr, nullptr );


	// hide the doom window by default
	::ShowWindow ( win32.hWnd, SW_HIDE );

	// The one and only window has been initialized, so show and update it
	meMainFrame->ShowWindow(SW_SHOW);
	meMainFrame->UpdateWindow();
}

/**
* Called every frame by the doom engine to allow the material editor to process messages.
*/
void MaterialEditorRun( void ) {
	MSG *msg = AfxGetCurrentMessage();

	while( ::PeekMessage(msg, nullptr, nullptr, nullptr, PM_NOREMOVE) ) {
		// pump message
		if ( !AfxGetApp()->PumpMessage() ) {
		}
	}
}

/**
* Called by the doom engine when the material editor needs to be destroyed.
*/
void MaterialEditorShutdown( void ) {
	delete meMainFrame;
	delete materialEditorFont;
	meMainFrame = nullptr;
}

/**
* Allows the doom engine to reflect console output to the material editors console.
*/
void MaterialEditorPrintConsole( const char *msg ) {
	if ( com_editors & EDITOR_MATERIAL ) {
		meMainFrame->PrintConsoleMessage( msg );
	}
}

/**
* Returns the handle to the main Material Editor Window
*/
HWND GetMaterialEditorWindow() {
	return meMainFrame->GetSafeHwnd();
}


// Simple about box for the material editor.
class CAboutDlg : public CDialog {
public:
	CAboutDlg();

	enum { IDD = IDD_ME_ABOUTBOX };

protected:
	virtual void DoDataExchange( CDataExchange *pDX );    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};

/**
* Constructor for the about box.
*/
CAboutDlg::CAboutDlg() : CDialog( CAboutDlg::IDD ) {
}

/**
* Called by the MFC framework to exchange data with the window controls.
*/
void CAboutDlg::DoDataExchange( CDataExchange *pDX ) {
	CDialog::DoDataExchange( pDX );
}

BEGIN_MESSAGE_MAP( CAboutDlg, CDialog )
END_MESSAGE_MAP()
