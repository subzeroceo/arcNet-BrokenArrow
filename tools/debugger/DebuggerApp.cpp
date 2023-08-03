
#include "..//idlib/Lib.h"
#pragma hdrstop

#include "../../sys/win32/rc/debugger_resource.h"
#include "DebuggerApp.h"

/*
================
rvDebuggerApp::rvDebuggerApp
================
*/
rvDebuggerApp::rvDebuggerApp() :
	mOptions ( "Software\\id Software\\DOOM3\\Tools\\Debugger" ) {
	mInstance		= nullptr;
	mDebuggerWindow = nullptr;
	mAccelerators   = nullptr;
}

/*
================
rvDebuggerApp::~rvDebuggerApp
================
*/
rvDebuggerApp::~rvDebuggerApp() {
	if ( mAccelerators ) {
		DestroyAcceleratorTable( mAccelerators );
	}
}

/*
================
rvDebuggerApp::Initialize

Initializes the debugger application by creating the debugger window
================
*/
bool rvDebuggerApp::Initialize( HINSTANCE instance ) {
	INITCOMMONCONTROLSEX ex;
	ex.dwICC = ICC_USEREX_CLASSES | ICC_LISTVIEW_CLASSES | ICC_WIN95_CLASSES;
	ex.dwSize = sizeof(INITCOMMONCONTROLSEX);

	mInstance = instance;

	mOptions.Load();

	mDebuggerWindow = new rvDebuggerWindow;

	if ( !mDebuggerWindow->Create( instance ) )
	{
		delete mDebuggerWindow;
		return false;
	}

	// Initialize the network connection for the debugger
	if ( !mClient.Initialize() )
	{
		return false;
	}

	mAccelerators = LoadAccelerators( mInstance, MAKEINTRESOURCE(IDR_DBG_ACCELERATORS) );

	return true;
}

/*
================
rvDebuggerApp::ProcessWindowMessages

Process windows messages
================
*/
bool rvDebuggerApp::ProcessWindowMessages( void ) {
	MSG	msg;

	while ( PeekMessage( &msg, nullptr, 0, 0, PM_NOREMOVE ) ) {
		if ( !GetMessage(&msg, nullptr, 0, 0 ) ) {
			return false;
		}

		if ( !TranslateAccelerator ( &msg ) ) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return true;
}

/*
================
rvDebuggerApp::TranslateAccelerator

Translate any accelerators destined for this window
================
*/
bool rvDebuggerApp::TranslateAccelerator( LPMSG msg ) {
	if ( mDebuggerWindow && ::TranslateAccelerator ( mDebuggerWindow->GetWindow(), mAccelerators, msg ) ) {
		return true;
	}

	return false;
}

/*
================
rvDebuggerApp::Run

Main Loop for the debugger application
================
*/
int rvDebuggerApp::Run( void ) {
	// Main message loop:
	while ( ProcessWindowMessages() ) {
		mClient.ProcessMessages();

		Sleep( 0 );
	}

	mClient.Shutdown();
	mOptions.Save();

	delete mDebuggerWindow;

	return 1;
}

