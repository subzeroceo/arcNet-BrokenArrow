#include "/idlib/Lib.h"
#pragma hdrstop

#include "Common_local.h"

/*
==============
anCommonLocal::InitializeMapsModes
==============
*/
void anCommonLocal::InitializeMapsModes() {}

/*
==============
anCommonLocal::StartMainMenu
==============
*/
void anCommonLocal::StartMenu( bool playIntro ) {
	if ( game && game->Shell_IsActive() ) {
		return;
	}

	//if ( readDemo ) {
	// if we're playing a demo, esc kills it
		//UnloadMap();
	//}

	if ( game ) {
		game->Shell_Show( true );
		game->Shell_SyncWithSession();
	}

	console->Close();

}

/*
===============
anCommonLocal::ExitMenu
===============
*/
void anCommonLocal::ExitMenu() {
	if ( game ) {
		game->Shell_Show( false );
	}
}

/*
==============
anCommonLocal::MenuEvent

Executes any commands returned by the gui
==============
*/
bool anCommonLocal::MenuEvent( const sysEvent_t * event ) {

	if ( session->GetSignInManager().ProcessInputEvent( event ) ) {
		return true;
	}

	if ( game && game->Shell_IsActive() ) {
		return game->Shell_HandleGuiEvent( event );
	}

	if ( game ) {
		return game->HandlePlayerGuiEvent( event );
	}

	return false;
}

/*
=================
anCommonLocal::GuiFrameEvents
=================
*/
void anCommonLocal::GuiFrameEvents() {
	if ( game ) {
		game->Shell_SyncWithSession();
	}
}
