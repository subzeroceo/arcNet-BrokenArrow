#include "/idlib/precompiled.h"
#pragma hdrstop

#include "Common_dialog.h"

arcCVarSystem popupDialog_debug( "popupDialog_debug", "0", CVAR_BOOL | CVAR_ARCHIVE, "display debug spam" );

extern arcCVarSystem g_demoMode;

static const char * dialogStateToString[ GDM_MAX + 1 ] = {
	ASSERT_ENUM_STRING( GDM_INVALID, 0 ),
	ASSERT_ENUM_STRING( GDM_MAX, 139 )
};

arcCVarSystem dialog_saveClearLevel( "dialog_saveClearLevel", "1000", CVAR_INTEGER, "Time required to show long message" );

/*
========================
bool DialogMsgShouldWait

There are a few dialog types that should pause so the user has the ability to read what's going on
========================
*/
bool DialogMsgShouldWait( gameDialogMessages_t msg ) {
	switch ( msg ) {
		case GDM_SAVING:
		case GDM_QUICK_SAVE:
			return true;
		default:
			return false;
	}
}

/*
================================================
arcCommonDlg::ClearDialogs
================================================
*/
void arcCommonDlg::ClearDialogs( bool forceClear ) {
	bool topMessageCleared = false;
	for ( int index = 0; index < messageList.Num(); ++index ) {
		if ( !messageList[index].leaveOnClear || forceClear ) {
			ReleaseCallBacks( index );
			messageList.RemoveIndex( index );
			if ( index == 0 ) {
				topMessageCleared = true;
			}
			index--;
		}
	}

	if ( topMessageCleared ) {
		ActivateDialog( false );
	}
}

/*
================================================
arcCommonDlg::AddDialogIntVal
================================================
*/
void arcCommonDlg::AddDialogIntVal( const char * name, int val ) {
	if ( dialog != NULL ) {
		dialog->SetGlobal( name, val );
	}
}

/*
================================================
arcCommonDlg::AddDialog
================================================
*/
void arcCommonDlg::AddDialog( gameDialogMessages_t msg, dialogType_t type, idSWFScriptFunction * acceptCallback, idSWFScriptFunction * cancelCallback, bool pause, const char * location, int lineNumber, bool leaveOnMapHeapReset, bool waitOnAtlas, bool renderDuringLoad ) {
	idKeyInput::ClearStates();
	//sys->ClearEvents();
	arcLibrary::PrintfIf( popupDialog_debug.GetBool(), "[%s] msg: %s, pause: %d from: %s:%d\n", __FUNCTION__, dialogStateToString[msg], pause, location == NULL ? "NULL" : location, lineNumber );

	if ( dialog == NULL ) {
		return;
	}

	idDialogInfo info;
	info.msg = msg;
	info.type = type;
	info.acceptCB = acceptCallback;
	info.cancelCB = cancelCallback;
	info.clear = false;
	info.pause = pause;
	info.startTime = Sys_Milliseconds();
	info.killTime = 0;
	info.leaveOnClear = leaveOnMapHeapReset;
	info.renderDuringLoad = renderDuringLoad;

	AddDialogInternal( info );
}

/*
========================
arcCommonDlg::AddDynamicDialog
========================
*/
void arcCommonDlg::AddDynamicDialog( gameDialogMessages_t msg, const arcStaticList< idSWFScriptFunction *, 4 > & callbacks, const arcStaticList< idStrId, 4 > & optionText, bool pause, aRcStaticString< 256 > overrideMsg, bool leaveOnMapHeapReset, bool waitOnAtlas, bool renderDuringLoad ) {
	if ( dialog == NULL ) {
		return;
	}

	idDialogInfo info;
	info.msg = msg;
	info.overrideMsg = overrideMsg;
	info.type = DIALOG_DYNAMIC;
	info.pause = pause;
	info.leaveOnClear = leaveOnMapHeapReset;
	info.acceptCB = 0 < callbacks.Num() ? callbacks[0] : NULL;
	info.cancelCB = 1 < callbacks.Num() ? callbacks[1] : NULL;
	info.altCBOne = 2 < callbacks.Num() ? callbacks[2] : NULL;
	info.altCBTwo = 3 < callbacks.Num() ? callbacks[3] : NULL;
	info.txt1 = 0 < optionText.Num() ? optionText[0] : idStrId();
	info.txt2 = 1 < optionText.Num() ? optionText[1] : idStrId();
	info.txt3 = 2 < optionText.Num() ? optionText[2] : idStrId();
	info.txt4 = 3 < optionText.Num() ? optionText[3] : idStrId();
	info.renderDuringLoad = renderDuringLoad;

	info.clear = false;
	info.startTime = Sys_Milliseconds();
	info.killTime = 0;

	AddDialogInternal( info );
}

/*
========================
arcCommonDlg::AddDialogInternal
========================
*/
void arcCommonDlg::AddDialogInternal( idDialogInfo & info ) {
	// don't add the dialog if it's already in the list, we never want to show a duplicate dialog
	if ( HasDialogMsg( info.msg, NULL ) ) {
		return;
	}

	// Remove the delete confirmation if we remove the device and ask for a storage confirmation
	if ( info.msg == GDM_STORAGE_REQUIRED ) {
		if ( HasDialogMsg( GDM_DELETE_SAVE, NULL ) ) {
			ClearDialog( GDM_DELETE_SAVE, NULL, 0 );
		}
		if ( HasDialogMsg( GDM_DELETE_AUTOSAVE, NULL ) ) {
			ClearDialog( GDM_DELETE_AUTOSAVE, NULL, 0 );
		}
		if ( HasDialogMsg( GDM_LOAD_DAMAGED_FILE, NULL ) ) {
			ClearDialog( GDM_LOAD_DAMAGED_FILE, NULL, 0 );
		}
	}

	if ( info.acceptCB != NULL ) {
		info.acceptCB->AddRef();
	}

	if ( info.cancelCB != NULL ) {
		info.cancelCB->AddRef();
	}

	if ( info.altCBOne != NULL ) {
		info.altCBOne->AddRef();
	}

	if ( info.altCBTwo != NULL ) {
		info.altCBTwo->AddRef();
	}

	if ( messageList.Num() == 0 ) {
		messageList.Append( info );
	} else {
		// attempting to add another one beyond our set max. take off the oldest
		// one from the list in order to not crash, but this really isn't a good
		// thing to be happening...
		if ( !verify( messageList.Num() < MAX_DIALOGS ) ) {
			messageList.RemoveIndex( MAX_DIALOGS - 1 );
		}

		if ( messageList.Num() > 0 ) {
			arcLibrary::PrintfIf( popupDialog_debug.GetBool(), "[%s] msg: %s new dialog added over old\n", __FUNCTION__, dialogStateToString[info.msg] );

			dialog->Activate( false );
			messageList.Insert( info, 0 );
		}
	}

	if ( info.type == DIALOG_QUICK_SAVE || info.type == DIALOG_CRAWL_SAVE || messageList[0].msg == GDM_CALCULATING_BENCHMARK ) {
		ShowNextDialog();
	}
}

/*
========================
arcCommonDlg::ActivateDialog
========================
*/
void arcCommonDlg::ActivateDialog( bool activate ) {
	dialogInUse = activate;
	if ( dialog != NULL ) {
		dialog->Activate( activate );
	}
}

/*
================================================
arcCommonDlg::ShowDialog
================================================
*/
void arcCommonDlg::ShowDialog( const idDialogInfo & info ) {
	arcLibrary::PrintfIf( popupDialog_debug.GetBool(), "[%s] msg: %s, m.clear = %d, m.waitClear = %d, m.killTime = %d\n",
		__FUNCTION__, dialogStateToString[info.msg], info.clear, info.waitClear, info.killTime );

	// here instead of add dialog to make sure we meet the TCR, otherwise it has a chance to be visible for less than 1 second
	if ( DialogMsgShouldWait( info.msg ) && !dialogInUse ) {
		startSaveTime = Sys_Milliseconds();
		stopSaveTime = 0;
	}

	if ( IsDialogActive() ) {
		dialog->Activate( false );
	}

	arcNetString message, title;
	GetDialogMsg( info.msg, message, title );

	dialog->SetGlobal( "titleVal", title );
	if ( info.overrideMsg.IsEmpty() ) {
		dialog->SetGlobal( "messageInfo", message );
	} else {
		dialog->SetGlobal( "messageInfo", info.overrideMsg );
	}
	dialog->SetGlobal( "infoType", info.type );

	if ( info.acceptCB == NULL && ( info.type != DIALOG_WAIT && info.type != DIALOG_WAIT_BLACKOUT ) ) {
		class idSWFScriptFunction_Accept : public idSWFScriptFunction_RefCounted {
		public:
			idSWFScriptFunction_Accept( gameDialogMessages_t _msg ) {
				msg = _msg;
			}
			idSWFScriptVar Call( idSWFScriptObject * thisObject, const idSWFParmList & parms ) {
				common->Dialog().ClearDialog( msg );
				return idSWFScriptVar();
			}
		private:
			gameDialogMessages_t msg;
		};

		dialog->SetGlobal( "acceptCallBack", new (TAG_SWF) idSWFScriptFunction_Accept( info.msg ) );

	} else {
		dialog->SetGlobal( "acceptCallBack", info.acceptCB );
	}

	dialog->SetGlobal( "cancelCallBack", info.cancelCB );
	dialog->SetGlobal( "altCBOne", info.altCBOne );
	dialog->SetGlobal( "altCBTwo", info.altCBTwo );
	dialog->SetGlobal( "opt1Txt", info.txt1.GetLocalizedString() );
	dialog->SetGlobal( "opt2Txt", info.txt2.GetLocalizedString() );
	dialog->SetGlobal( "opt3Txt", info.txt3.GetLocalizedString() );
	dialog->SetGlobal( "opt4Txt", info.txt4.GetLocalizedString() );

	ActivateDialog( true );
}

/*
================================================
arcCommonDlg::ShowNextDialog
================================================
*/
void arcCommonDlg::ShowNextDialog() {
	for ( int index = 0; index < messageList.Num(); ++index ) {
		if ( !messageList[index].clear ) {
			idDialogInfo info = messageList[index];
			ShowDialog( info );
			break;
		}
	}
}

/*
================================================
arcCommonDlg::ShowSaveIndicator
================================================
*/
void arcCommonDlg::ShowSaveIndicator( bool show ) {
	arcLibrary::PrintfIf( popupDialog_debug.GetBool(), "[%s]\n", __FUNCTION__ );

	if ( show ) {
		arcNetString msg = idStrId( "#str_saving" ).GetLocalizedString();

		common->Dialog().AddDialog( GDM_SAVING, DIALOG_WAIT, NULL, NULL, true, "", 0, false, true, true );
	} else {
		common->Dialog().ClearDialog( GDM_SAVING );
	}
}

/*
========================
arcCommonDlg::RemoveSaveDialog

From TCR# 047
Games must display a message during storage writes for the following conditions and the respective amount of time:
- Writes longer than one second require the standard message be displayed for three seconds.
- Writes longer than three seconds require the standard message be displayed for the length of the write.
- Writes that last one second or less require the shorter message be displayed for one second or the standard message for three seconds.

========================
*/
void arcCommonDlg::RemoveWaitDialogs() {
	bool topMessageCleared = false;
	for ( int index = 0; index < messageList.Num(); ++index ) {
		if ( DialogMsgShouldWait( messageList[index].msg ) ) {
			if ( Sys_Milliseconds() >= messageList[index].killTime && messageList[index].waitClear ) {
				messageList[index].clear = true;
				messageList[index].waitClear = false;
				if ( index == 0 ) {
					topMessageCleared = true;
				}
			}
		}
	}

	if ( topMessageCleared && messageList.Num() > 0 ) {
		ActivateDialog( false );
	}
}

/*
================================================
arcCommonDlg::ClearAllDialogHack
================================================
*/
void arcCommonDlg::ClearAllDialogHack() {
	for ( int index = 0; index < messageList.Num(); ++index ) {
		messageList[index].clear = true;
		messageList[index].waitClear = false;
	}
}

/*
================================================
arcCommonDlg::HasDialogMsg
================================================
*/
bool arcCommonDlg::HasDialogMsg( gameDialogMessages_t msg, bool * isNowActive ) {
	for ( int index = 0; index < messageList.Num(); ++index ) {
		idDialogInfo & info = messageList[index];

		if ( info.msg == msg && !info.clear ) {
			if ( isNowActive != NULL ) {
				*isNowActive = ( index == 0 );
			}
			return true;
		}
	}

	if ( isNowActive != NULL ) {
		*isNowActive = false;
	}

	return false;
}

/*
================================================
arcCommonDlg::ClearDialog
================================================
*/
void arcCommonDlg::ClearDialog( gameDialogMessages_t msg, const char * location, int lineNumber ) {
	bool topMessageCleared = false;

	for ( int index = 0; index < messageList.Num(); ++index ) {
		idDialogInfo & info = messageList[index];
		if ( info.msg == msg && !info.clear ) {
			if ( DialogMsgShouldWait( info.msg ) ) {
				// you can have 2 saving dialogs simultaneously, if you clear back-to-back, we need to let the 2nd dialog
				// get the clear message
				if ( messageList[index].waitClear ) {
					continue;
				}

				int timeShown = Sys_Milliseconds() - messageList[index].startTime;

				// for the time being always use the long saves
				if ( timeShown < dialog_saveClearLevel.GetInteger() ) {
					messageList[index].killTime = Sys_Milliseconds() + ( dialog_saveClearLevel.GetInteger() - timeShown );
					messageList[index].waitClear = true;
				} else {
					messageList[index].clear = true;
					if ( index == 0 ) {
						topMessageCleared = true;
					}
				}
			} else {
				messageList[index].clear = true;
				if ( index == 0 ) {
					topMessageCleared = true;
				}
			}
			assert( info.msg >= GDM_INVALID && info.msg < GDM_MAX );	// not sure why /analyze complains about this
			arcLibrary::PrintfIf( popupDialog_debug.GetBool(), "[%s] msg: %s, from: %s:%d, topMessageCleared = %d, m.clear = %d, m.waitClear = %d, m.killTime = %d\n",
				__FUNCTION__, dialogStateToString[info.msg], location == NULL ? "NULL" : location, lineNumber,
				topMessageCleared, messageList[index].clear,
				messageList[index].waitClear, messageList[index].killTime );
			break;
		}
	}

	if ( topMessageCleared && messageList.Num() > 0 ) {
		ActivateDialog( false );
	}
}

/*
================================================
arcCommonDlg::ReleaseCallBacks
================================================
*/
void arcCommonDlg::ReleaseCallBacks( int index ) {
	if ( index < messageList.Num() ) {
		if ( messageList[index].acceptCB != NULL ) {
			messageList[index].acceptCB->Release();
			messageList[index].acceptCB = NULL;
		}

		if ( messageList[index].cancelCB != NULL ) {
			messageList[index].cancelCB->Release();
			messageList[index].cancelCB = NULL;
		}

		if ( messageList[index].altCBOne != NULL ) {
			messageList[index].altCBOne->Release();
			messageList[index].altCBOne = NULL;
		}

		if ( messageList[index].altCBTwo != NULL ) {
			messageList[index].altCBTwo->Release();
			messageList[index].altCBTwo = NULL;
		}
	}
}

/*
================================================
arcCommonDlg::Render
================================================
*/
void arcCommonDlg::Render( bool loading ) {
	dialogPause = false;

	if ( dialog == NULL ) {
		return;
	}

	RemoveWaitDialogs();

	bool pauseCheck = false;
	for ( int index = 0; index < messageList.Num(); ++index ) {
		if ( messageList[index].clear ) {
			arcLibrary::PrintfIf( popupDialog_debug.GetBool(), "[%s] removing %s\n", __FUNCTION__, dialogStateToString[messageList[index].msg] );
			ReleaseCallBacks( index );
			messageList.RemoveIndex( index );
			index--;
		} else {
			if ( messageList[index].pause && !pauseCheck ) {
				pauseCheck = true;
			}
		}
	}

	dialogPause = pauseCheck;

	if ( messageList.Num() > 0 && !dialog->IsActive() ) {
		ShowNextDialog();
	}

	if ( messageList.Num() == 0 && dialog->IsActive() ) {
		dialog->Activate( false );
	}

	// Decrement the time remaining on the save indicator or turn it off
	if ( !dialogShowingSaveIndicatorRequested && saveIndicator->IsActive() ) {
		ShowSaveIndicator( false );
	}

	if ( messageList.Num() > 0 && messageList[0].type == DIALOG_TIMER_ACCEPT_REVERT ) {
		int startTime = messageList[0].startTime;
		int endTime = startTime + PC_KEYBOARD_WAIT;
		int timeRemaining = ( endTime - Sys_Milliseconds() ) / 1000;

		if ( timeRemaining <= 0 ) {
			if ( messageList[0].cancelCB != NULL ) {
				idSWFParmList parms;
				messageList[0].cancelCB->Call( NULL, parms );
			}
			messageList[0].clear = true;
		} else {
			idStrId txtTime = idStrId( "#str_time_remaining" );
			dialog->SetGlobal( "countdownInfo", va( txtTime.GetLocalizedString(), timeRemaining ) );
		}
	}

	if ( messageList.Num() > 0 && loading && ( messageList[0].renderDuringLoad == false ) ) {
		return;
	}

	if ( dialog->IsActive() ) {
		dialog->Render( renderSystem, Sys_Microseconds() );
	}

	if ( saveIndicator != NULL && saveIndicator->IsActive() ) {
		saveIndicator->Render( renderSystem, Sys_Microseconds() );
	}
}

/*
================================================
arcCommonDlg::Init
================================================
*/
void arcCommonDlg::Init() {

	arcLibrary::PrintfIf( popupDialog_debug.GetBool(), "[%s]\n", __FUNCTION__ );

	Shutdown();

	dialog = new (TAG_SWF) idSWF( "dialog" );
	saveIndicator = new (TAG_SWF) idSWF( "save_indicator" );

#define BIND_DIALOG_CONSTANT( x ) dialog->SetGlobal( #x, x )
	if ( dialog != NULL ) {
		BIND_DIALOG_CONSTANT( DIALOG_ACCEPT );
		BIND_DIALOG_CONSTANT( DIALOG_CONTINUE );
		BIND_DIALOG_CONSTANT( DIALOG_ACCEPT_CANCEL );
		BIND_DIALOG_CONSTANT( DIALOG_YES_NO );
		BIND_DIALOG_CONSTANT( DIALOG_CANCEL );
		BIND_DIALOG_CONSTANT( DIALOG_WAIT );
		BIND_DIALOG_CONSTANT( DIALOG_WAIT_BLACKOUT );
		BIND_DIALOG_CONSTANT( DIALOG_WAIT_CANCEL );
		BIND_DIALOG_CONSTANT( DIALOG_DYNAMIC );
		BIND_DIALOG_CONSTANT( DIALOG_QUICK_SAVE );
		BIND_DIALOG_CONSTANT( DIALOG_TIMER_ACCEPT_REVERT );
		BIND_DIALOG_CONSTANT( DIALOG_CRAWL_SAVE );
		BIND_DIALOG_CONSTANT( DIALOG_CONTINUE_LARGE );
		BIND_DIALOG_CONSTANT( DIALOG_BENCHMARK );
	}
}

/*
================================================
arcCommonDlg::Shutdown
================================================
*/
void arcCommonDlg::Shutdown() {
	arcLibrary::PrintfIf( popupDialog_debug.GetBool(), "[%s]\n", __FUNCTION__ );

	ClearDialogs();

	delete dialog;
	dialog = NULL;

	delete saveIndicator;
	saveIndicator = NULL;
}

/*
========================
arcCommonDlg::Restart
========================
*/
void arcCommonDlg::Restart() {
	Shutdown();
	Init();
}

/*
================================================
arcCommonDlg::GetDialogMsg
================================================
*/
arcNetString arcCommonDlg::GetDialogMsg( gameDialogMessages_t msg, arcNetString & message, arcNetString & title ) {
	return message;
}

/*
================================================
arcCommonDlg::HandleDialogEvent
================================================
*/
bool arcCommonDlg::HandleDialogEvent( const sysEvent_t * sev ) {
	if ( dialog != NULL && dialog->IsLoaded() && dialog->IsActive() ) {
		if ( saveIndicator->IsActive() ) {
			return false;
		} else {
			if ( dialog->HandleEvent( sev ) ) {
				idKeyInput::ClearStates();
				//sys->ClearEvents();
			}
		}

		return true;
	}

	return false;
}

/*
================================================
arcCommonDlg::IsDialogActive
================================================
*/
bool arcCommonDlg::IsDialogActive() {
	if ( dialog != NULL ) {
		return dialog->IsActive();
	}

	return false;
}

CONSOLE_COMMAND( commonDialogClear, "clears all dialogs that may be hung", 0 ) {
	common->Dialog().ClearAllDialogHack();
}

CONSOLE_COMMAND( testShowDialog, "show a dialog", 0 ) {
	int dialogId = atoi( args.Argv( 1 ) );
	common->Dialog().AddDialog( (gameDialogMessages_t)dialogId, DIALOG_ACCEPT, NULL, NULL, false );
}

CONSOLE_COMMAND( testShowDynamicDialog, "show a dynamic dialog", 0 ) {
	class idSWFScriptFunction_Continue : public idSWFScriptFunction_RefCounted {
	public:
		idSWFScriptVar Call( idSWFScriptObject * thisObject, const idSWFParmList & parms ) {
			common->Dialog().ClearDialog( GDM_INSUFFICENT_STORAGE_SPACE );
			return idSWFScriptVar();
		}
	};

	arcStaticList< idSWFScriptFunction *, 4 > callbacks;
	arcStaticList< idStrId, 4 > optionText;
	callbacks.Append( new (TAG_SWF) idSWFScriptFunction_Continue() );
	optionText.Append( idStrId( "#str_swf_continue" ) );

	// build custom space required string
	// #str_dlg_space_required ~= "There is insufficient storage available.  Please free %s and try again."
	arcNetString format = idStrId( "#str_dlg_space_required" ).GetLocalizedString();
	arcNetString size;
	int requiredSpaceInBytes = 150000;
	if ( requiredSpaceInBytes > ( 1024 * 1024 ) ) {
		size = va( "%.1f MB", ( float ) requiredSpaceInBytes / ( 1024.0f * 1024.0f ) );
	} else {
		size = va( "%.0f KB", ( float ) requiredSpaceInBytes / 1024.0f );
	}
	arcNetString msg = va( format.c_str(), size.c_str() );

	common->Dialog().AddDynamicDialog( GDM_INSUFFICENT_STORAGE_SPACE, callbacks, optionText, true, msg );
}

CONSOLE_COMMAND( testShowDialogBug, "show a dynamic dialog", 0 ) {
	common->Dialog().ShowSaveIndicator( true );
	common->Dialog().ShowSaveIndicator( false );

	// This locks the game because it thinks it's paused because we're passing in pause = true but the
	// dialog isn't ever added because of the abuse of dialog->isActive when the save indicator is shown.
	int dialogId = atoi( args.Argv( 1 ) );
	common->Dialog().AddDialog( (gameDialogMessages_t)dialogId, DIALOG_ACCEPT, NULL, NULL, true );
}
