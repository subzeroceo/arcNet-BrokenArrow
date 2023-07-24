#include "/idlib/precompiled.h"
#pragma hdrstop

#include "Common_local.h"

/*
==============
arcCommonLocal::InitializeMapsModes
==============
*/
void arcCommonLocal::InitializeMapsModes() {}

/*
==============
arcCommonLocal::StartMainMenu
==============
*/
void arcCommonLocal::StartMenu( bool playIntro ) {
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
arcCommonLocal::ExitMenu
===============
*/
void arcCommonLocal::ExitMenu() {
	if ( game ) {
		game->Shell_Show( false );
	}
}

/*
==============
arcCommonLocal::MenuEvent

Executes any commands returned by the gui
==============
*/
bool arcCommonLocal::MenuEvent( const sysEvent_t * event ) {

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
arcCommonLocal::GuiFrameEvents
=================
*/
void arcCommonLocal::GuiFrameEvents() {
	if ( game ) {
		game->Shell_SyncWithSession();
	}
}
