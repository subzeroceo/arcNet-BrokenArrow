#include "/idlib/precompiled.h"
#pragma hdrstop

#include "Common_local.h"
#include "../sys/sys_lobby_backend.h"


#define LAUNCH_TITLE_DOOM_EXECUTABLE		"doom1.exe"
#define LAUNCH_TITLE_DOOM2_EXECUTABLE		"doom2.exe"

arcCVarSystem com_wipeSeconds( "com_wipeSeconds", "1", CVAR_SYSTEM, "" );
arcCVarSystem com_disableAutoSaves( "com_disableAutoSaves", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
arcCVarSystem com_disableAllSaves( "com_disableAllSaves", "0", CVAR_SYSTEM | CVAR_BOOL, "" );


extern arcCVarSystem sys_lang;

extern arcCVarSystem g_demoMode;

// This is for the dirty hack to get a dialog to show up before we capture the screen for autorender.
const int NumScreenUpdatesToShowDialog = 25;

/*
================
arcCommonLocal::LaunchExternalTitle

Launches an external title  ( Doom 1, or 2 ) based on title index.
for PS3, a device number is sent in, for the game to register as a local
user by default, when title initializes.
================
*/
void arcCommonLocal::LaunchExternalTitle( int titleIndex, int device, const lobbyConnectInfo_t * const connectInfo ) {
	arcNetString deviceString( device );

	// We want to pass in the current executable, so that the launching title knows which title to return to.
	// as of right now, this feature is TBD.
	const char * currentExecutablePath = "ImNotSureYet";
	arcNetString launchingExecutablePath;

	arcCommandArgs cmdArgs;
	cmdArgs.AppendArg( currentExecutablePath );

	if ( titleIndex == LAUNCH_TITLE_DOOM ) {
			launchingExecutablePath.Format( "%s%s", Sys_DefaultBasePath(), LAUNCH_TITLE_DOOM_EXECUTABLE );
		cmdArgs.AppendArg( "d1bfg" );
	} else if ( titleIndex == LAUNCH_TITLE_DOOM2 ) {
		launchingExecutablePath.Format( "%s%s", Sys_DefaultBasePath(), LAUNCH_TITLE_DOOM2_EXECUTABLE );
		cmdArgs.AppendArg( "d2bfg" );

	} else {

		arcLibrary::Warning( "Unhandled Launch Title %d \n", titleIndex );
	}

	cmdArgs.AppendArg( deviceString.c_str() );

	// Add an argument so that the new process knows whether or not to read exitspawn data.
	if ( connectInfo != NULL ) {
		cmdArgs.AppendArg( "exitspawnInvite" );
	}

	// Add arguments so that the new process will know which command line to invoke to relaunch this process
	// if necessary.

	const int launchDataSize = ( connectInfo == NULL ) ? 0 : sizeof( *connectInfo );

	Sys_Launch(  launchingExecutablePath.c_str() , cmdArgs, const_cast< lobbyConnectInfo_t * const >( connectInfo ), launchDataSize );
}

/*
================
arcCommonLocal::StartWipe

Draws and captures the current state, then starts a wipe with that image
================
*/
void arcCommonLocal::StartWipe( const char *_wipeMaterial, bool hold ) {
	console->Close();

	Draw();

	renderSystem->CaptureRenderToImage( "_currentRender" );

	wipeMaterial = declManager->FindMaterial( _wipeMaterial, false );

	wipeStartTime = Sys_Milliseconds();
	wipeStopTime = wipeStartTime + SEC2MS( com_wipeSeconds.GetFloat() );
	wipeHold = hold;
}

/*
================
arcCommonLocal::CompleteWipe
================
*/
void arcCommonLocal::CompleteWipe() {
	while ( Sys_Milliseconds() < wipeStopTime ) {
		BusyWait();
		Sys_Sleep( 10 );
	}

	// ensure it is completely faded out
	wipeStopTime = Sys_Milliseconds();
	BusyWait();
}

/*
================
arcCommonLocal::ClearWipe
================
*/
void arcCommonLocal::ClearWipe() {
	wipeHold = false;
	wipeStopTime = 0;
	wipeStartTime = 0;
	wipeForced = false;
}

/*
===============
arcCommonLocal::StartNewGame
===============
*/
void arcCommonLocal::StartNewGame( const char * mapName, bool devmap, int gameMode ) {
	if ( session->GetSignInManager().GetMasterLocalUser() == NULL ) {
		// For development make sure a controller is registered
		// Can't just register the local user because it will be removed because of it's persistent state
		session->GetSignInManager().SetDesiredLocalUsers( 1, 1 );
		session->GetSignInManager().Pump();
	}

	arcNetString mapNameClean = mapName;
	mapNameClean.StripFileExtension();
	mapNameClean.BackSlashesToSlashes();

	arcMatchParameters matchParameters;
	matchParameters.mapName = mapNameClean;
	if ( gameMode == GAME_MODE_SINGLEPLAYER ) {
		matchParameters.numSlots = 1;
		matchParameters.gameMode = GAME_MODE_SINGLEPLAYER;
		matchParameters.gameMap = GAME_MAP_SINGLEPLAYER;
	} else {
		matchParameters.gameMap = mpGameMaps.Num();	// If this map isn't found in mpGameMaps, then set it to some undefined value (this happens when, for example, we load a box map with netmap)
		matchParameters.gameMode = gameMode;
		matchParameters.matchFlags = DefaultPartyFlags;
		for ( int i = 0; i < mpGameMaps.Num(); i++ ) {
			if ( arcNetString::Icmp( mpGameMaps[i].mapFile, mapNameClean ) == 0 ) {
				matchParameters.gameMap = i;
				break;
			}
		}
		matchParameters.numSlots = session->GetTitleStorageInt( "MAX_PLAYERS_ALLOWED", 4 );
	}

	cvarSystem->MoveCVarsToDict( CVAR_SERVERINFO, matchParameters.serverInfo );
	if ( devmap ) {
		matchParameters.serverInfo.Set( "devmap", "1" );
	} else {
		matchParameters.serverInfo.Delete( "devmap" );
	}

	session->QuitMatchToTitle();
	if ( WaitForSessionState( ARCSession::IDLE ) ) {
		session->CreatePartyLobby( matchParameters );
		if ( WaitForSessionState( ARCSession::PARTY_LOBBY ) ) {
			session->CreateMatch( matchParameters );
			if ( WaitForSessionState( ARCSession::GAME_LOBBY ) ) {
				cvarSystem->SetCVarBool( "developer", devmap );
				session->StartMatch();
			}
		}
	}
}

/*
===============
arcCommonLocal::MoveToNewMap
Single player transition from one map to another
===============
*/
void arcCommonLocal::MoveToNewMap( const char *mapName, bool devmap ) {
	arcMatchParameters matchParameters;
	matchParameters.numSlots = 1;
	matchParameters.gameMode = GAME_MODE_SINGLEPLAYER;
	matchParameters.gameMap = GAME_MAP_SINGLEPLAYER;
	matchParameters.mapName = mapName;
	cvarSystem->MoveCVarsToDict( CVAR_SERVERINFO, matchParameters.serverInfo );
	if ( devmap ) {
		matchParameters.serverInfo.Set( "devmap", "1" );
		mapSpawnData.persistentPlayerInfo.Clear();
	} else {
		matchParameters.serverInfo.Delete( "devmap" );
		mapSpawnData.persistentPlayerInfo = game->GetPersistentPlayerInfo( 0 );
	}
	session->QuitMatchToTitle();
	if ( WaitForSessionState( ARCSession::IDLE ) ) {
		session->CreatePartyLobby( matchParameters );
		if ( WaitForSessionState( ARCSession::PARTY_LOBBY ) ) {
			session->CreateMatch( matchParameters );
			if ( WaitForSessionState( ARCSession::GAME_LOBBY ) ) {
				session->StartMatch();
			}
		}
	}
}

/*
===============
arcCommonLocal::UnloadMap

Performs cleanup that needs to happen between maps, or when a
game is exited.
Exits with mapSpawned = false
===============
*/
void arcCommonLocal::UnloadMap() {
	StopPlayingRenderDemo();

	// end the current map in the game
	if ( game ) {
		game->MapShutdown();
	}

	if ( writeDemo ) {
		StopRecordingRenderDemo();
	}

	mapSpawned = false;
}

/*
===============
arcCommonLocal::LoadLoadingGui
===============
*/
void arcCommonLocal::LoadLoadingGui( const char *mapName, bool & hellMap ) {
	defaultLoadscreen = false;
	loadGUI = new idSWF( "loading/default", NULL );

	if ( g_demoMode.GetBool() ) {
		hellMap = false;
		if ( loadGUI != NULL ) {
			const arcMaterial * defaultMat = declManager->FindMaterial( "guis/assets/loadscreens/default" );
			renderSystem->LoadLevelImages();

			loadGUI->Activate( true );
			idSWFSpriteInstance * bgImg = loadGUI->GetRootObject().GetSprite( "bgImage" );
			if ( bgImg != NULL ) {
				bgImg->SetMaterial( defaultMat );
			}
		}
		defaultLoadscreen = true;
		return;
	}

	// load / program a gui to stay up on the screen while loading
	aRcStaticString< MAX_OSPATH > stripped = mapName;
	stripped.StripFileExtension();
	stripped.StripPath();

	// use default load screen for demo
	aRcStaticString< MAX_OSPATH > matName = "guis/assets/loadscreens/";
	matName.Append( stripped );
	const arcMaterial * mat = declManager->FindMaterial( matName );

	renderSystem->LoadLevelImages();

	if ( mat->GetImageWidth() < 32 ) {
		mat = declManager->FindMaterial( "guis/assets/loadscreens/default" );
		renderSystem->LoadLevelImages();
	}

	loadTipList.SetNum( loadTipList.Max() );
	for ( int i = 0; i < loadTipList.Max(); ++i ) {
		loadTipList[i] = i;
	}

	if ( loadGUI != NULL ) {
		loadGUI->Activate( true );
		nextLoadTip = Sys_Milliseconds() + LOAD_TIP_CHANGE_INTERVAL;

		idSWFSpriteInstance * bgImg = loadGUI->GetRootObject().GetSprite( "bgImage" );
		if ( bgImg != NULL ) {
			bgImg->SetMaterial( mat );
		}

		idSWFSpriteInstance * overlay = loadGUI->GetRootObject().GetSprite( "overlay" );

		const arcDeclEntityDef * mapDef = static_cast<const arcDeclEntityDef *>(declManager->FindType( DECL_MAPDEF, mapName, false ) );
		if ( mapDef != NULL ) {
			isHellMap = mapDef->dict.GetBool( "hellMap", false );

			if ( isHellMap && overlay != NULL ) {
				overlay->SetVisible( false );
			}

			arcNetString desc;
			arcNetString subTitle;
			arcNetString displayName;
			idSWFTextInstance * txtVal = NULL;

			txtVal = loadGUI->GetRootObject().GetNestedText( "txtRegLoad" );
			displayName = ARCLocalization::GetString( mapDef->dict.GetString( "name", mapName ) );

			if ( txtVal != NULL ) {
				txtVal->SetText( "#str_00408" );
				txtVal->SetStrokeInfo( true, 2.0f, 1.0f );
			}

			const arcMatchParameters & matchParameters = session->GetActingGameStateLobbyBase().GetMatchParms();
			if ( matchParameters.gameMode == GAME_MODE_SINGLEPLAYER ) {
				desc = ARCLocalization::GetString( mapDef->dict.GetString( "desc", "" ) );
				subTitle = ARCLocalization::GetString( mapDef->dict.GetString( "subTitle", "" ) );
			} else {
				const arcStringList & modes = common->GetModeDisplayList();
				subTitle = modes[ arcMath::ClampInt( 0, modes.Num() - 1, matchParameters.gameMode ) ];

				const char * modeDescs[] = { "#str_swf_deathmatch_desc", "#str_swf_tourney_desc", "#str_swf_team_deathmatch_desc", "#str_swf_lastman_desc", "#str_swf_ctf_desc" };
				desc = ARCLocalization::GetString( modeDescs[matchParameters.gameMode] );
			}

			if ( !isHellMap ) {
				txtVal = loadGUI->GetRootObject().GetNestedText( "txtName" );
			} else {
				txtVal = loadGUI->GetRootObject().GetNestedText( "txtHellName" );
			}
			if ( txtVal != NULL ) {
				txtVal->SetText( displayName );
				txtVal->SetStrokeInfo( true, 2.0f, 1.0f );
			}

			txtVal = loadGUI->GetRootObject().GetNestedText( "txtSub" );
			if ( txtVal != NULL && !isHellMap ) {
				txtVal->SetText( subTitle );
				txtVal->SetStrokeInfo( true, 1.75f, 0.75f );
			}

			txtVal = loadGUI->GetRootObject().GetNestedText( "txtDesc" );
			if ( txtVal != NULL ) {
				if ( isHellMap ) {
					txtVal->SetText( va( "\n%s", desc.c_str() ) );
				} else {
					txtVal->SetText( desc );
				}
				txtVal->SetStrokeInfo( true, 1.75f, 0.75f );
			}
		}
	}
}

/*
===============
arcCommonLocal::ExecuteMapChange

Performs the initialization of a game based on session match parameters, used for both single
player and multiplayer, but not for renderDemos, which don't create a game at all.
Exits with mapSpawned = true
===============
*/
void arcCommonLocal::ExecuteMapChange() {
	if ( session->GetState() != ARCSession::LOADING ) {
		arcLibrary::Warning( "Session state is not LOADING in ExecuteMapChange" );
		return;
	}

	// Clear all dialogs before beginning the load
	common->Dialog().ClearDialogs( true );

	// Remember the current load ID.
	// This is so we can tell if we had a new loadmap request from within an existing loadmap call
	const int cachedLoadingID = session->GetLoadingID();

	const arcMatchParameters & matchParameters = session->GetActingGameStateLobbyBase().GetMatchParms();

	if ( matchParameters.numSlots <= 0 ) {
		arcLibrary::Warning( "numSlots <= 0 in ExecuteMapChange" );
		return;
	}

	insideExecuteMapChange = true;

	common->Printf( "--------- Execute Map Change ---------\n" );
	common->Printf( "Map: %s\n", matchParameters.mapName.c_str() );

	// ensure that r_znear is reset to the default value
	// this fixes issues with the projection matrix getting messed up when switching maps or loading a saved game
	// while an in-game cinematic is playing.
	cvarSystem->SetCVarFloat( "r_znear", 3.0f );

	// reset all cheat cvars for a multiplayer game
	if ( IsMultiplayer() ) {
		cvarSystem->ResetFlaggedVariables( CVAR_CHEAT );
	}

	int start = Sys_Milliseconds();

	for ( int i = 0; i < MAX_INPUT_DEVICES; i++ ) {
		Sys_SetRumble( i, 0, 0 );
	}

	// close console and remove any prints from the notify lines
	console->Close();

	// clear all menu sounds
	soundWorld->Pause();
	menuSoundWorld->ClearAllSoundEmitters();
	soundSystem->SetPlayingSoundWorld( menuSoundWorld );
	soundSystem->Render();

	// extract the map name from serverinfo
	currentMapName = matchParameters.mapName;
	currentMapName.StripFileExtension();

	aRcStaticString< MAX_OSPATH > fullMapName = "maps/";
	fullMapName += currentMapName;
	fullMapName.SetFileExtension( "map" );

	if ( mapSpawnData.savegameFile ) {
		fileSystem->BeginLevelLoad( currentMapName, NULL, 0 );
	} else {
		fileSystem->BeginLevelLoad( currentMapName, saveFile.GetDataPtr(), saveFile.GetAllocated() );
	}

	// capture the current screen and start a wipe
	// immediately complete the wipe to fade out the level transition
	// run the wipe to completion
	StartWipe( "wipeMaterial", true );
	CompleteWipe();

	int sm = Sys_Milliseconds();
	// shut down the existing game if it is running
	UnloadMap();
	int ms = Sys_Milliseconds() - sm;
	common->Printf( "%6d msec to unload map\n", ms );

	// Free media from previous level and
	// note which media we are going to need to load
	sm = Sys_Milliseconds();
	renderSystem->BeginLevelLoad();
	soundSystem->BeginLevelLoad();
	declManager->BeginLevelLoad();
	uiManager->BeginLevelLoad();
	ms = Sys_Milliseconds() - sm;
	common->Printf( "%6d msec to free assets\n", ms );

	//Sys_DumpMemory( true );

	// load / program a gui to stay up on the screen while loading
	// set the loading gui that we will wipe to
	bool hellMap = false;
	LoadLoadingGui( currentMapName, hellMap );

	// Stop rendering the wipe
	ClearWipe();


	if ( fileSystem->UsingResourceFiles() ) {
		aRcStaticString< MAX_OSPATH > manifestName = currentMapName;
		manifestName.Replace( "game/", "maps/" );
		manifestName.Replace( "/mp/", "/" );
		manifestName += ".preload";
		aRcPreloadManifest manifest;
		manifest.LoadManifest( manifestName );
		renderSystem->Preload( manifest, currentMapName );
		soundSystem->Preload( manifest );
		game->Preload( manifest );
	}

	if ( common->IsMultiplayer() ) {
		// In multiplayer, make sure the player is either 60Hz or 120Hz
		// to avoid potential issues.
		const float mpEngineHz = ( com_engineHz.GetFloat() < 90.0f ) ? 60.0f : 120.0f;
		com_engineHz_denominator = 100LL * mpEngineHz;
		com_engineHz_latched = mpEngineHz;
	} else {
		// allow com_engineHz to be changed between map loads
		com_engineHz_denominator = 100LL * com_engineHz.GetFloat();
		com_engineHz_latched = com_engineHz.GetFloat();
	}

	// note any warning prints that happen during the load process
	common->ClearWarnings( currentMapName );

	// release the mouse cursor
	// before we do this potentially long operation
	Sys_GrabMouseCursor( false );

	// let the renderSystem load all the geometry
	if ( !renderWorld->InitFromMap( fullMapName ) ) {
		common->Error( "couldn't load %s", fullMapName.c_str() );
	}

	// for the synchronous networking we needed to roll the angles over from
	// level to level, but now we can just clear everything
	usercmdGen->InitForNewMap();

	// load and spawn all other entities ( from a savegame possibly )
	if ( mapSpawnData.savegameFile ) {
		if ( !game->InitFromSaveGame( fullMapName, renderWorld, soundWorld, mapSpawnData.savegameFile, mapSpawnData.stringTableFile, mapSpawnData.savegameVersion ) ) {
			// If the loadgame failed, end the session, which will force us to go back to the main menu
			session->QuitMatchToTitle();
		}
	} else {
		if ( !IsMultiplayer() ) {
			assert( game->GetLocalClientNum() == 0 );
			assert( matchParameters.gameMode == GAME_MODE_SINGLEPLAYER );
			assert( matchParameters.gameMap == GAME_MAP_SINGLEPLAYER );
			game->SetPersistentPlayerInfo( 0, mapSpawnData.persistentPlayerInfo );
		}
		game->SetServerInfo( matchParameters.serverInfo );
		game->InitFromNewMap( fullMapName, renderWorld, soundWorld, matchParameters.gameMode, Sys_Milliseconds() );
	}

	game->Shell_CreateMenu( true );

	// Reset some values important to multiplayer
	ResetNetworkingState();

	// If the session state is not loading here, something went wrong.
	if ( session->GetState() == ARCSession::LOADING && session->GetLoadingID() == cachedLoadingID ) {
		// Notify session we are done loading
		session->LoadingFinished();

		while ( session->GetState() == ARCSession::LOADING ) {
			Sys_GenerateEvents();
			session->UpdateSignInManager();
			session->Pump();
			Sys_Sleep( 10 );
		}
	}

	if ( !mapSpawnData.savegameFile ) {
		// run a single frame to catch any resources that are referenced by events posted in spawn
		arcUserCmdMgr emptyCommandManager;
		gameReturn_t emptyGameReturn;
		for ( int playerIndex = 0; playerIndex < MAX_PLAYERS; ++playerIndex ) {
			emptyCommandManager.PutUserCmdForPlayer( playerIndex, usercmd_t() );
		}
		if ( IsClient() ) {
			game->ClientRunFrame( emptyCommandManager, false, emptyGameReturn );
		} else {
			game->RunFrame( emptyCommandManager, emptyGameReturn );
		}
	}

	renderSystem->EndLevelLoad();
	soundSystem->EndLevelLoad();
	declManager->EndLevelLoad();
	uiManager->EndLevelLoad( currentMapName );
	fileSystem->EndLevelLoad();

	if ( !mapSpawnData.savegameFile && !IsMultiplayer() ) {
		common->Printf( "----- Running initial game frames -----\n" );

		// In single player, run a bunch of frames to make sure ragdolls are settled
		arcUserCmdMgr emptyCommandManager;
		gameReturn_t emptyGameReturn;
		for ( int i = 0; i < 100; i++ ) {
			for ( int playerIndex = 0; playerIndex < MAX_PLAYERS; ++playerIndex ) {
				emptyCommandManager.PutUserCmdForPlayer( playerIndex, usercmd_t() );
			}
			game->RunFrame( emptyCommandManager, emptyGameReturn );
		}

		// kick off an auto-save of the game (so we can always continue in this map if we die before hitting an autosave)
		common->Printf( "----- Saving Game -----\n" );
		SaveGame( "autosave" );
	}

	common->Printf( "----- Generating Interactions -----\n" );

	// let the renderSystem generate interactions now that everything is spawned
	renderWorld->GenerateAllInteractions();

	{
		int vertexMemUsedKB = vertexCache.staticData.vertexMemUsed.GetValue() / 1024;
		int indexMemUsedKB = vertexCache.staticData.indexMemUsed.GetValue() / 1024;
		arcLibrary::Printf( "Used %dkb of static vertex memory (%d%%)\n", vertexMemUsedKB, vertexMemUsedKB * 100 / ( STATIC_VERTEX_MEMORY / 1024 ) );
		arcLibrary::Printf( "Used %dkb of static index memory (%d%%)\n", indexMemUsedKB, indexMemUsedKB * 100 / ( STATIC_INDEX_MEMORY / 1024 ) );
	}

	if ( common->JapaneseCensorship() ) {
		if ( currentMapName.Icmp( "game/mp/d3xpdm3" ) == 0 ) {
			const arcMaterial * gizpool2 = declManager->FindMaterial( "textures/hell/gizpool2" );
			arcMaterial * chiglass1bluex = const_cast<arcMaterial *>( declManager->FindMaterial( "textures/sfx/chiglass1bluex" ) );
			arcTempArray<char> text( gizpool2->GetTextLength() );
			gizpool2->GetText( text.Ptr() );
			chiglass1bluex->Parse( text.Ptr(), text.Num(), false );
		}
	}

	common->PrintWarnings();

	session->Pump();

	if ( session->GetState() != ARCSession::INGAME ) {
		// Something went wrong, don't process stale reliables that have been queued up.
		reliableQueue.Clear();
	}

	usercmdGen->Clear();

	// remove any prints from the notify lines
	console->ClearNotifyLines();

	Sys_SetPhysicalWorkMemory( -1, -1 );

	// at this point we should be done with the loading gui so we kill it
	delete loadGUI;
	loadGUI = NULL;


	// capture the current screen and start a wipe
	StartWipe( "wipe2Material" );

	// we are valid for game draws now
	insideExecuteMapChange = false;
	mapSpawned = true;
	Sys_ClearEvents();


	int	msec = Sys_Milliseconds() - start;
	common->Printf( "%6d msec to load %s\n", msec, currentMapName.c_str() );
	//Sys_DumpMemory( false );

	// Issue a render at the very end of the load process to update soundTime before the first frame
	soundSystem->Render();
}

/*
===============
arcCommonLocal::UpdateLevelLoadPacifier

Pumps the session and if multiplayer, displays dialogs during the loading process.
===============
*/
void arcCommonLocal::UpdateLevelLoadPacifier() {
	autoRenderIconType_t icon = AUTORENDER_DEFAULTICON;
	bool autoswapsRunning = renderSystem->AreAutomaticBackgroundSwapsRunning( &icon );
	if ( !insideExecuteMapChange && !autoswapsRunning ) {
		return;
	}

	const int sessionUpdateTime = common->IsMultiplayer() ? 16 : 100;

	const int time = Sys_Milliseconds();

	// Throttle session pumps.
	if ( time - lastPacifierSessionTime >= sessionUpdateTime ) {
		lastPacifierSessionTime = time;

		Sys_GenerateEvents();

		session->UpdateSignInManager();
		session->Pump();
		session->ProcessSnapAckQueue();
	}

	if ( autoswapsRunning ) {
		// If autoswaps are running, only update if a Dialog is shown/dismissed
		bool dialogState = Dialog().HasAnyActiveDialog();
		if ( lastPacifierDialogState != dialogState ) {
			lastPacifierDialogState = dialogState;
			renderSystem->EndAutomaticBackgroundSwaps();
			if ( dialogState ) {
				icon = AUTORENDER_DIALOGICON; // Done this way to handle the rare case of a tip changing at the same time a dialog comes up
				for ( int i = 0; i < NumScreenUpdatesToShowDialog; ++i ) {
					UpdateScreen( false );
				}
			}
			renderSystem->BeginAutomaticBackgroundSwaps( icon );
		}
	} else {
		// On the PC just update at a constant rate for the Steam overlay
		if ( time - lastPacifierGuiTime >= 50 ) {
			lastPacifierGuiTime = time;
			UpdateScreen( false );
		}
	}

	if ( time >= nextLoadTip && loadGUI != NULL && loadTipList.Num() > 0 && !defaultLoadscreen ) {
		if ( autoswapsRunning ) {
			renderSystem->EndAutomaticBackgroundSwaps();
		}
		nextLoadTip = time + LOAD_TIP_CHANGE_INTERVAL;
		const int rnd = time % loadTipList.Num();
		aRcStaticString<20> tipId;
		tipId.Format( "#str_loadtip_%d", loadTipList[ rnd ] );
		loadTipList.RemoveIndex( rnd );

		idSWFTextInstance * txtVal = loadGUI->GetRootObject().GetNestedText( "txtDesc" );
		if ( txtVal != NULL ) {
			if ( isHellMap ) {
				txtVal->SetText( va( "\n%s", ARCLocalization::GetString( tipId ) ) );
			} else {
				txtVal->SetText( ARCLocalization::GetString( tipId ) );
			}
			txtVal->SetStrokeInfo( true, 1.75f, 0.75f );
		}
		UpdateScreen( false );
		if ( autoswapsRunning ) {
			renderSystem->BeginAutomaticBackgroundSwaps( icon );
		}
	}
}

/*
===============
arcCommonLocal::ScrubSaveGameFileName

Turns a bad file name into a good one or your money back
===============
*/
void arcCommonLocal::ScrubSaveGameFileName( arcNetString &saveFileName ) const {
	int i;
	arcNetString inFileName;

	inFileName = saveFileName;
	inFileName.RemoveColors();
	inFileName.StripFileExtension();

	saveFileName.Clear();

	int len = inFileName.Length();
	for ( i = 0; i < len; i++ ) {
		if ( strchr( "',.~!@#$%^&*()[]{}<>\\|/=?+;:-\'\"", inFileName[i] ) ) {
			// random junk
			saveFileName += '_';
		} else if ( (const unsigned char)inFileName[i] >= 128 ) {
			// high ascii chars
			saveFileName += '_';
		} else if ( inFileName[i] == ' ' ) {
			saveFileName += '_';
		} else {
			saveFileName += inFileName[i];
		}
	}
}

/*
===============
arcCommonLocal::SaveGame
===============
*/
bool arcCommonLocal::SaveGame( const char * saveName ) {
	if ( pipelineFile != NULL ) {
		// We're already in the middle of a save. Leave us alone.
		return false;
	}

	if ( com_disableAllSaves.GetBool() || ( com_disableAutoSaves.GetBool() && ( arcNetString::Icmp( saveName, "autosave" ) == 0 ) ) ) {
		return false;
	}

	if ( IsMultiplayer() ) {
		common->Printf( "Can't save during net play.\n" );
		return false;
	}

	if (mapSpawnData.savegameFile != NULL ) {
		return false;
	}

	const arcDictionary & persistentPlayerInfo = game->GetPersistentPlayerInfo( 0 );
	if ( persistentPlayerInfo.GetInt( "health" ) <= 0 ) {
		common->Printf( "You must be alive to save the game\n" );
		return false;
	}

	soundWorld->Pause();
	soundSystem->SetPlayingSoundWorld( menuSoundWorld );
	soundSystem->Render();

	Dialog().ShowSaveIndicator( true );
	if ( insideExecuteMapChange ) {
		UpdateLevelLoadPacifier();
	} else {
		// Heremake sure we pump the gui enough times to show the 'saving' dialog
		const bool captureToImage = false;
		for ( int i = 0; i < NumScreenUpdatesToShowDialog; ++i ) {
			UpdateScreen( captureToImage );
		}
		renderSystem->BeginAutomaticBackgroundSwaps( AUTORENDER_DIALOGICON );
	}

	// Make sure the file is writable and the contents are cleared out (Set to write from the start of file)
	saveFile.MakeWritable();
	saveFile.Clear( false );
	stringsFile.MakeWritable();
	stringsFile.Clear( false );

	// Setup the save pipeline
	pipelineFile = new (TAG_SAVEGAMES) arcFile_SaveGamePipelined();
	pipelineFile->OpenForWriting( &saveFile );

	// Write SaveGame Header:
	// Game Name / Version / Map Name / Persistant Player Info

	// game
	const char *gamename = GAME_NAME;
	saveFile.WriteString( gamename );

	// map
	saveFile.WriteString( currentMapName );

	saveFile.WriteBool( consoleUsed );

	game->GetServerInfo().WriteToFileHandle( &saveFile );

	// let the game save its state
	game->SaveGame( pipelineFile, &stringsFile );

	pipelineFile->Finish();

	arcSaveGameDetails gameDetails;
	game->GetSaveGameDetails( gameDetails );

	gameDetails.descriptors.Set( SAVEGAME_DETAIL_FIELD_LANGUAGE, sys_lang.GetString() );
	gameDetails.descriptors.SetInt( SAVEGAME_DETAIL_FIELD_CHECKSUM, ( int )gameDetails.descriptors.Checksum() );

	gameDetails.slotName = saveName;
	ScrubSaveGameFileName( gameDetails.slotName );

	saveFileEntryList_t files;
	files.Append( &stringsFile );
	files.Append( &saveFile );

	session->SaveGameSync( gameDetails.slotName, files, gameDetails );

	if ( !insideExecuteMapChange ) {
		renderSystem->EndAutomaticBackgroundSwaps();
	}

	syncNextEngineFrame = true;

	return true;
}

/*
===============
arcCommonLocal::LoadGame
===============
*/
bool arcCommonLocal::LoadGame( const char * saveName ) {
	if ( session->GetSignInManager().GetMasterLocalUser() == NULL ) {
		return false;
	}
	if (mapSpawnData.savegameFile != NULL ) {
		return false;
	}

	bool found = false;
	const saveGameDetailsList_t & sgdl = session->GetSaveGameManager().GetEnumeratedSavegames();
	for ( int i = 0; i < sgdl.Num(); i++ ) {
		if ( sgdl[i].slotName == saveName ) {
			if ( sgdl[i].GetLanguage() != sys_lang.GetString() ) {
				arcStaticList< idSWFScriptFunction *, 4 > callbacks;
				arcStaticList< idStrId, 4 > optionText;
				optionText.Append( idStrId( "#str_swf_continue" ) );
				aRcStaticString<256> langName = "#str_lang_" + sgdl[i].GetLanguage();
				aRcStaticString<256> msg;
				msg.Format( ARCLocalization::GetString( "#str_dlg_wrong_language" ), ARCLocalization::GetString( langName ) );
				Dialog().AddDynamicDialog( GDM_SAVEGAME_WRONG_LANGUAGE, callbacks, optionText, true, msg, false, true );
				if ( wipeForced ) {
					ClearWipe();
				}
				return false;
			}
			found = true;
			break;
		}
	}
	if ( !found ) {
		common->Printf( "Could not find save '%s'\n", saveName );
		if ( wipeForced ) {
			ClearWipe();
		}
		return false;
	}

	mapSpawnData.savegameFile = &saveFile;
	mapSpawnData.stringTableFile = &stringsFile;

	saveFileEntryList_t files;
	files.Append( mapSpawnData.stringTableFile );
	files.Append( mapSpawnData.savegameFile );

	arcNetString slotName = saveName;
	ScrubSaveGameFileName( slotName );
	saveFile.Clear( false );
	stringsFile.Clear( false );

	saveGameHandle_t loadGameHandle = session->LoadGameSync( slotName, files );
	if ( loadGameHandle != 0 ) {
		return true;
	}
	mapSpawnData.savegameFile = NULL;
	if ( wipeForced ) {
		ClearWipe();
	}
	return false;
}

/*
========================
HandleInsufficientStorage
========================
*/
void HandleInsufficientStorage( const ARCSaveLoadParms & parms ) {
	session->GetSaveGameManager().ShowRetySaveDialog( parms.directory, parms.requiredSpaceInBytes );
}

/*
========================
HandleCommonErrors
========================
*/
bool HandleCommonErrors( const ARCSaveLoadParms & parms ) {
	if ( parms.GetError() == SAVEGAME_E_NONE ) {
		return true;
	}

	common->Dialog().ShowSaveIndicator( false );

	if ( parms.GetError() & SAVEGAME_E_CORRUPTED ) {
		// This one might need to be handled by the game
		common->Dialog().AddDialog( GDM_CORRUPT_CONTINUE, DIALOG_CONTINUE, NULL, NULL, false );

		// Find the game in the enumerated details, mark as corrupt so the menus can show as corrupt
		saveGameDetailsList_t & list = session->GetSaveGameManager().GetEnumeratedSavegamesNonConst();
		for ( int i = 0; i < list.Num(); i++ ) {
			if ( arcNetString::Icmp( list[i].slotName, parms.description.slotName ) == 0 ) {
				list[i].damaged = true;
			}
		}
		return true;
	} else if ( parms.GetError() & SAVEGAME_E_INSUFFICIENT_ROOM ) {
		HandleInsufficientStorage( parms );
		return true;
	} else if ( parms.GetError() & SAVEGAME_E_UNABLE_TO_SELECT_STORAGE_DEVICE && saveGame_enable.GetBool() ) {
		common->Dialog().AddDialog( GDM_UNABLE_TO_USE_SELECTED_STORAGE_DEVICE, DIALOG_CONTINUE, NULL, NULL, false );
		return true;
	} else if ( parms.GetError() & SAVEGAME_E_INVALID_FILENAME ) {
		arcLibrary::Warning( va( "Invalid savegame filename [%s]!", parms.directory.c_str() ) );
		return true;
	} else if ( parms.GetError() & SAVEGAME_E_DLC_NOT_FOUND ) {
		common->Dialog().AddDialog( GDM_DLC_ERROR_MISSING_GENERIC, DIALOG_CONTINUE, NULL, NULL, false );
		return true;
	} else if ( parms.GetError() & SAVEGAME_E_DISC_SWAP ) {
		common->Dialog().AddDialog( GDM_DISC_SWAP, DIALOG_CONTINUE, NULL, NULL, false );
		return true;
	} else if ( parms.GetError() & SAVEGAME_E_INCOMPATIBLE_NEWER_VERSION ) {
		common->Dialog().AddDialog( GDM_INCOMPATIBLE_NEWER_SAVE, DIALOG_CONTINUE, NULL, NULL, false );
		return true;
	}

	return false;
}

/*
========================
arcCommonLocal::OnSaveCompleted
========================
*/
void arcCommonLocal::OnSaveCompleted( ARCSaveLoadParms & parms ) {
	assert( pipelineFile != NULL );
	delete pipelineFile;
	pipelineFile = NULL;

	if ( parms.GetError() == SAVEGAME_E_NONE ) {
		game->Shell_UpdateSavedGames();
	}

	if ( !HandleCommonErrors( parms ) ) {
		common->Dialog().AddDialog( GDM_ERROR_SAVING_SAVEGAME, DIALOG_CONTINUE, NULL, NULL, false );
	}
}

/*
========================
arcCommonLocal::OnLoadCompleted
========================
*/
void arcCommonLocal::OnLoadCompleted( ARCSaveLoadParms & parms ) {
	if ( !HandleCommonErrors( parms ) ) {
		common->Dialog().AddDialog( GDM_ERROR_LOADING_SAVEGAME, DIALOG_CONTINUE, NULL, NULL, false );
	}
}

/*
========================
arcCommonLocal::OnLoadFilesCompleted
========================
*/
void arcCommonLocal::OnLoadFilesCompleted( ARCSaveLoadParms & parms ) {
	if ( ( mapSpawnData.savegameFile != NULL ) && ( parms.GetError() == SAVEGAME_E_NONE ) ) {
		// just need to make the file readable
		((aRcFileMemory *)mapSpawnData.savegameFile)->MakeReadOnly();
		((aRcFileMemory *)mapSpawnData.stringTableFile)->MakeReadOnly();

		arcNetString gamename;
		arcNetString mapname;

		mapSpawnData.savegameVersion = parms.description.GetSaveVersion();
		mapSpawnData.savegameFile->ReadString( gamename );
		mapSpawnData.savegameFile->ReadString( mapname );

		if ( ( gamename != GAME_NAME ) || ( mapname.IsEmpty() ) || ( parms.description.GetSaveVersion() > BUILD_NUMBER ) ) {
			// if this isn't a savegame for the correct game, abort loadgame
			common->Warning( "Attempted to load an invalid savegame" );
		} else {
			common->DPrintf( "loading savegame\n" );

			mapSpawnData.savegameFile->ReadBool( consoleUsed );
			consoleUsed = consoleUsed || com_allowConsole.GetBool();

			arcMatchParameters matchParameters;
			matchParameters.numSlots = 1;
			matchParameters.gameMode = GAME_MODE_SINGLEPLAYER;
			matchParameters.gameMap = GAME_MAP_SINGLEPLAYER;
			matchParameters.mapName = mapname;
			matchParameters.serverInfo.ReadFromFileHandle( mapSpawnData.savegameFile );

			session->QuitMatchToTitle();
			if ( WaitForSessionState( ARCSession::IDLE ) ) {
				session->CreatePartyLobby( matchParameters );
				if ( WaitForSessionState( ARCSession::PARTY_LOBBY ) ) {
					session->CreateMatch( matchParameters );
					if ( WaitForSessionState( ARCSession::GAME_LOBBY ) ) {
						session->StartMatch();
						return;
					}
				}
			}
		}
	}
	// If we got here then we didn't actually load the save game for some reason
	mapSpawnData.savegameFile = NULL;
}

/*
========================
arcCommonLocal::TriggerScreenWipe
========================
*/
void arcCommonLocal::TriggerScreenWipe( const char * _wipeMaterial, bool hold ) {
	StartWipe( _wipeMaterial, hold );
	CompleteWipe();
	wipeForced = true;
	renderSystem->BeginAutomaticBackgroundSwaps( AUTORENDER_DEFAULTICON );
}

/*
========================
arcCommonLocal::OnEnumerationCompleted
========================
*/
void arcCommonLocal::OnEnumerationCompleted( ARCSaveLoadParms & parms ) {
	if ( parms.GetError() == SAVEGAME_E_NONE ) {
		game->Shell_UpdateSavedGames();
	}
}

/*
========================
arcCommonLocal::OnDeleteCompleted
========================
*/
void arcCommonLocal::OnDeleteCompleted( ARCSaveLoadParms & parms ) {
	if ( parms.GetError() == SAVEGAME_E_NONE ) {
		game->Shell_UpdateSavedGames();
	}
}

/*
===============
LoadGame_f
===============
*/
CONSOLE_COMMAND_SHIP( loadGame, "loads a game", arcCmdSystem::ArgCompletion_SaveGame ) {
	console->Close();
	commonLocal.LoadGame( ( args.Argc() > 1 ) ? args.Argv(1 ) : "quick" );
}

/*
===============
SaveGame_f
===============
*/
CONSOLE_COMMAND_SHIP( saveGame, "saves a game", NULL ) {
	const char * savename = ( args.Argc() > 1 ) ? args.Argv(1 ) : "quick";
	if ( commonLocal.SaveGame( savename ) ) {
		common->Printf( "Saved: %s\n", savename );
	}
}

/*
==================
Common_Map_f

Restart the server on a different map
==================
*/
CONSOLE_COMMAND_SHIP( map, "loads a map", arcCmdSystem::ArgCompletion_MapName ) {
	commonLocal.StartNewGame( args.Argv(1 ), false, GAME_MODE_SINGLEPLAYER );
}

/*
==================
Common_RestartMap_f
==================
*/
CONSOLE_COMMAND_SHIP( restartMap, "restarts the current map", NULL ) {
	if ( g_demoMode.GetBool() ) {
		cmdSystem->AppendCommandText( va( "devmap %s %d\n", commonLocal.GetCurrentMapName(), 0 ) );
	}
}

/*
==================
Common_DevMap_f

Restart the server on a different map in developer mode
==================
*/
CONSOLE_COMMAND_SHIP( devmap, "loads a map in developer mode", arcCmdSystem::ArgCompletion_MapName ) {
	commonLocal.StartNewGame( args.Argv(1 ), true, GAME_MODE_SINGLEPLAYER );
}

/*
==================
Common_NetMap_f

Restart the server on a different map in multiplayer mode
==================
*/
CONSOLE_COMMAND_SHIP( netmap, "loads a map in multiplayer mode", arcCmdSystem::ArgCompletion_MapName ) {
	int gameMode = 0; // Default to deathmatch
	if ( args.Argc() > 2 ) {
		gameMode = atoi( args.Argv(2) );
	}
	commonLocal.StartNewGame( args.Argv(1 ), true, gameMode );
}

/*
==================
Common_TestMap_f
==================
*/
CONSOLE_COMMAND( testmap, "tests a map", arcCmdSystem::ArgCompletion_MapName ) {
	arcNetString map, string;

	map = args.Argv(1 );
	if ( !map.Length() ) {
		return;
	}
	map.StripFileExtension();

	cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );

	sprintf( string, "dmap maps/%s.map", map.c_str() );
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, string );

	sprintf( string, "devmap %s", map.c_str() );
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, string );
}
