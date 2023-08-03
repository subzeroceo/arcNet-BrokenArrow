#include "/idlib/Lib.h"
#pragma hdrstop
#include "Common_local.h"
#include "../renderer/Image.h"
#include "../renderer/ImageOpts.h"
/*

New for tech4x:

Unlike previous SMP work, the actual GPU command drawing is done in the main thread, which avoids the
OpenGL problems with needing windows to be created by the same thread that creates the context, as well
as the issues with passing context ownership back and forth on the 360.

The game tic and the generation of the draw command list is now run in a separate thread, and overlapped
with the interpretation of the previous draw command list.

While the game tic should be nicely contained, the draw command generation winds through the user interface
code, and is potentially hazardous.  For now, the overlap will be restricted to the renderer back end,
which should also be nicely contained.

*/
#define DEFAULT_FIXED_TIC "0"
#define DEFAULT_NO_SLEEP "0"

anCVarSystem com_deltaTimeClamp( "com_deltaTimeClamp", "50", CVAR_INTEGER, "don't process more than this time in a single frame" );

anCVarSystem com_fixedTic( "com_fixedTic", DEFAULT_FIXED_TIC, CVAR_BOOL, "run a single game frame per render frame" );
anCVarSystem com_noSleep( "com_noSleep", DEFAULT_NO_SLEEP, CVAR_BOOL, "don't sleep if the game is running too fast" );
anCVarSystem com_smp( "com_smp", "1", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "run the game and draw code in a separate thread" );
anCVarSystem com_aviDemoSamples( "com_aviDemoSamples", "16", CVAR_SYSTEM, "" );
anCVarSystem com_aviDemoWidth( "com_aviDemoWidth", "256", CVAR_SYSTEM, "" );
anCVarSystem com_aviDemoHeight( "com_aviDemoHeight", "256", CVAR_SYSTEM, "" );
anCVarSystem com_skipGameDraw( "com_skipGameDraw", "0", CVAR_SYSTEM | CVAR_BOOL, "" );

anCVarSystem com_sleepGame( "com_sleepGame", "0", CVAR_SYSTEM | CVAR_INTEGER, "intentionally add a sleep in the game time" );
anCVarSystem com_sleepDraw( "com_sleepDraw", "0", CVAR_SYSTEM | CVAR_INTEGER, "intentionally add a sleep in the draw time" );
anCVarSystem com_sleepRender( "com_sleepRender", "0", CVAR_SYSTEM | CVAR_INTEGER, "intentionally add a sleep in the render time" );

anCVarSystem com_drawDebugHud( "com_drawDebugHud", "0", CVAR_SYSTEM | CVAR_INTEGER, "0 = None, 1 = Hud 1, 2 = Hud 2, 3 = Snapshots" );

anCVarSystem timescale( "timescale", "1", CVAR_SYSTEM | CVAR_FLOAT, "Number of game frames to run per render frame", 0.001f, 100.0f );

extern anCVarSystem in_useJoystick;
extern anCVarSystem in_joystickRumble;

/*
===============
idGameThread::Run

Run in a background thread for performance, but can also
be called directly in the foreground thread for comparison.
===============
*/
int idGameThread::Run() {
	commonLocal.frameTiming.startGameTime = Sys_Microseconds();

	// debugging tool to test frame dropping behavior
	if ( com_sleepGame.GetInteger() ) {
		Sys_Sleep( com_sleepGame.GetInteger() );
	}

	if ( numGameFrames == 0 ) {
		// Ensure there's no stale gameReturn data from a paused game
		ret = gameReturn_t();
	}

	if ( isClient ) {
		// run the game logic
		for ( int i = 0; i < numGameFrames; i++ ) {
			SCOPED_PROFILE_EVENT( "Client Prediction" );
			if ( userCmdMgr ) {
				game->ClientRunFrame( *userCmdMgr, ( i == numGameFrames - 1 ), ret );
			}
			if ( ret.syncNextEngineFrame || ret.sessionCommand[0] != 0 ) {
				break;
			}
		}
	} else {
		// run the game logic
		for ( int i = 0; i < numGameFrames; i++ ) {
			SCOPED_PROFILE_EVENT( "GameTic" );
			if ( userCmdMgr ) {
				game->RunFrame( *userCmdMgr, ret );
			}
			if ( ret.syncNextEngineFrame || ret.sessionCommand[0] != 0 ) {
				break;
			}
		}
	}

	// we should have consumed all of our usercmds
	if ( userCmdMgr ) {
		if ( userCmdMgr->HasUserCmdForPlayer( game->GetLocalClientNum() ) && common->GetCurrentGame() == DOOM3_BFG ) {
			anLibrary::Printf( "idGameThread::Run: didn't consume all usercmds\n" );
		}
	}

	commonLocal.frameTiming.finishGameTime = Sys_Microseconds();

	SetThreadGameTime( ( commonLocal.frameTiming.finishGameTime - commonLocal.frameTiming.startGameTime ) / 1000 );

	// build render commands and geometry
	{
		SCOPED_PROFILE_EVENT( "Draw" );
		commonLocal.Draw();
	}

	commonLocal.frameTiming.finishDrawTime = Sys_Microseconds();

	SetThreadRenderTime( ( commonLocal.frameTiming.finishDrawTime - commonLocal.frameTiming.finishGameTime ) / 1000 );

	SetThreadTotalTime( ( commonLocal.frameTiming.finishDrawTime - commonLocal.frameTiming.startGameTime ) / 1000 );

	return 0;
}

/*
===============
idGameThread::RunGameAndDraw

===============
*/
gameReturn_t idGameThread::RunGameAndDraw( int numGameFrames_, arcUserCmdMgr & userCmdMgr_, bool isClient_, int startGameFrame ) {
	// this should always immediately return
	this->WaitForThread();

	// save the usercmds for the background thread to pick up
	userCmdMgr = &userCmdMgr_;

	isClient = isClient_;

	// grab the return value created by the last thread execution
	gameReturn_t latchedRet = ret;

	numGameFrames = numGameFrames_;

	// start the thread going
	if ( com_smp.GetBool() == false ) {
		// run it in the main thread so PIX profiling catches everything
		Run();
	} else {
		this->SignalWork();
	}

	// return the latched result while the thread runs in the background
	return latchedRet;
}

/*
===============
anCommonLocal::DrawWipeModel

Draw the fade material over everything that has been drawn
===============
*/
void anCommonLocal::DrawWipeModel() {
	if ( wipeStartTime >= wipeStopTime ) {
		return;
	}

	int currentTime = Sys_Milliseconds();

	if ( !wipeHold && currentTime > wipeStopTime ) {
		return;
	}

	float fade = ( float )( currentTime - wipeStartTime ) / ( wipeStopTime - wipeStartTime );
	renderSystem->SetColor4( 1, 1, 1, fade );
	renderSystem->DrawStretchPic( 0, 0, 640, 480, 0, 0, 1, 1, wipeMaterial );
}

/*
===============
anCommonLocal::Draw
===============
*/
void anCommonLocal::Draw() {
	// debugging tool to test frame dropping behavior
	if ( com_sleepDraw.GetInteger() ) {
		Sys_Sleep( com_sleepDraw.GetInteger() );
	}

	if ( loadGUI != nullptr ) {
		loadGUI->Render( renderSystem, Sys_Milliseconds() );
	} else if ( currentGame == DOOM_CLASSIC || currentGame == DOOM2_CLASSIC ) {
		const float sysWidth = renderSystem->GetWidth() * renderSystem->GetPixelAspect();
		const float sysHeight = renderSystem->GetHeight();
		const float sysAspect = sysWidth / sysHeight;
		const float trueAspect = 4.0f / 3.0f;
		const float adjustment = sysAspect / trueAspect;
		const float barHeight = ( adjustment >= 1.0f ) ? 0.0f : ( 1.0f - adjustment ) * ( float )SCREEN_HEIGHT * 0.25f;
		const float barWidth = ( adjustment <= 1.0f ) ? 0.0f : ( adjustment - 1.0f ) * ( float )SCREEN_WIDTH * 0.25f;
		if ( barHeight > 0.0f ) {
			renderSystem->SetColor( colorBlack );
			renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, barHeight, 0, 0, 1, 1, whiteMaterial );
			renderSystem->DrawStretchPic( 0, SCREEN_HEIGHT - barHeight, SCREEN_WIDTH, barHeight, 0, 0, 1, 1, whiteMaterial );
		}
		if ( barWidth > 0.0f ) {
			renderSystem->SetColor( colorBlack );
			renderSystem->DrawStretchPic( 0, 0, barWidth, SCREEN_HEIGHT, 0, 0, 1, 1, whiteMaterial );
			renderSystem->DrawStretchPic( SCREEN_WIDTH - barWidth, 0, barWidth, SCREEN_HEIGHT, 0, 0, 1, 1, whiteMaterial );
		}
		renderSystem->SetColor4( 1, 1, 1, 1 );
		renderSystem->DrawStretchPic( barWidth, barHeight, SCREEN_WIDTH - barWidth * 2.0f, SCREEN_HEIGHT - barHeight * 2.0f, 0, 0, 1, 1, doomClassicMaterial );
	} else if ( game && game->Shell_IsActive() ) {
		bool gameDraw = game->Draw( game->GetLocalClientNum() );
		if ( !gameDraw ) {
			renderSystem->SetColor( colorBlack );
			renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 1, 1, whiteMaterial );
		}
		game->Shell_Render();
		//renderWorld->RenderScene( &currentDemoRenderView );
	} else if ( mapSpawned ) {
		bool gameDraw = false;
		// normal drawing for both single and multi player
		if ( !com_skipGameDraw.GetBool() && Game()->GetLocalClientNum() >= 0 ) {
			// draw the game view
			int	start = Sys_Milliseconds();
			if ( game ) {
				gameDraw = game->Draw( Game()->GetLocalClientNum() );
			}
			int end = Sys_Milliseconds();
			time_gameDraw += ( end - start );	// note time used for com_speeds
		}
		if ( !gameDraw ) {
			renderSystem->SetColor( colorBlack );
			renderSystem->DrawStretchPic( 0, 0, 640, 480, 0, 0, 1, 1, whiteMaterial );
		}

		renderSystem->SetColor4( 0, 0, 0, 1 );
		renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 1, 1, whiteMaterial );

	{
		SCOPED_PROFILE_EVENT( "Post-Draw" );

		// draw the wipe material on top of this if it hasn't completed yet
		DrawWipeModel();

		Dialog().Render( loadGUI != nullptr );

		// draw the half console / notify console on top of everything
		console->Draw( false );
	}
}

/*
===============
anCommonLocal::UpdateScreen

This is an out-of-sequence screen update, not the normal game rendering
===============
*/
void anCommonLocal::UpdateScreen( bool captureToImage ) {
	if ( insideUpdateScreen ) {
		return;
	}
	insideUpdateScreen = true;

	// make sure the game / draw thread has completed
	gameThread.WaitForThread();

	// release the mouse capture back to the desktop
	Sys_GrabMouseCursor( false );

	// build all the draw commands without running a new game tic
	Draw();

	if ( captureToImage ) {
		renderSystem->CaptureRenderToImage( "_currentRender", false );
	}

	// this should exit right after vsync, with the GPU idle and ready to draw
	const setBufferCommand_t * cmd = renderSystem->SwapCommandBuffers( &time_frontend, &time_backend, &time_shadows, &time_gpu );

	// get the GPU busy with new commands
	renderSystem->RenderCommandBuffers( cmd );

	insideUpdateScreen = false;
}
/*
================
anCommonLocal::ProcessGameReturn
================
*/
void anCommonLocal::ProcessGameReturn( const gameReturn_t & ret ) {
	// set joystick rumble
	if ( in_useJoystick.GetBool() && in_joystickRumble.GetBool() && !game->Shell_IsActive() && session->GetSignInManager().GetMasterInputDevice() >= 0 ) {
		Sys_SetRumble( session->GetSignInManager().GetMasterInputDevice(), ret.vibrationLow, ret.vibrationHigh );		// Only set the rumble on the active controller
	} else {
		for ( int i = 0; i < MAX_INPUT_DEVICES; i++ ) {
			Sys_SetRumble( i, 0, 0 );
		}
	}

	syncNextEngineFrame = ret.syncNextEngineFrame;

	if ( ret.sessionCommand[0] ) {
		anCommandArgs args;

		args.TokenizeString( ret.sessionCommand, false );

		if ( !anString::Icmp( args.Argv(0 ), "map" ) ) {
			MoveToNewMap( args.Argv( 1 ), false );
		} else if ( !anString::Icmp( args.Argv(0 ), "devmap" ) ) {
			MoveToNewMap( args.Argv( 1 ), true );
		} else if ( !anString::Icmp( args.Argv(0 ), "died" ) ) {
			if ( !IsMultiplayer() ) {
				game->Shell_Show( true );
			}
		} else if ( !anString::Icmp( args.Argv(0 ), "disconnect" ) ) {
			cmdSystem->BufferCommandText( CMD_EXEC_INSERT, "stoprecording; disconnect" );
		} else if ( !anString::Icmp( args.Argv(0 ), "endOfDemo" ) ) {
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "endOfDemo" );
		}
	}
}

extern anCVarSystem com_forceGenericSIMD;

/*
=================
anCommonLocal::Frame
=================
*/
void anCommonLocal::Frame() {
	try {
		SCOPED_PROFILE_EVENT( "Common::Frame" );
		// This is the only place this is incremented
		anLibrary::frameNumber++;
		// allow changing SIMD usage on the fly
		if ( com_forceGenericSIMD.IsModified() ) {
			arcSIMD::InitProcessor( "SIMD", com_forceGenericSIMD.GetBool() );
			com_forceGenericSIMD.ClearModified();
		}

		// Do the actual switch between Doom 3 and the classics here so
		// that things don't get confused in the middle of the frame.
		PerformGameswitch ();

		// pump all the events
		Sys_GenerateEvents();

		// write config file if anything changed
		WriteConfiguration();

		eventLoop->RunEventLoop();

		// Activate the shell if it's been requested
		if ( showShellRequested && game ) {
			game->Shell_Show( true );
			showShellRequested = false;
		}

		// if the console or another gui is down, we don't need to hold the mouse cursor
		bool chatting = false;
		if ( console->Active() || Dialog().IsDialogActive() || session->IsSystemUIShowing() || ( game && game->InhibitControls() && !IsPlayingDoomClassic() ) ) {
			Sys_GrabMouseCursor( false );
			usercmdGen->InhibitUsercmd( INHIBIT_SESSION, true );
			chatting = true;
		} else {
			Sys_GrabMouseCursor( true );
			usercmdGen->InhibitUsercmd( INHIBIT_SESSION, false );
		}

		//const bool pauseGame = ( !mapSpawned || ( !IsMultiplayer() && ( Dialog().IsDialogPausing() || session->IsSystemUIShowing() || ( game && game->Shell_IsActive() ) ) ) ) && !IsPlayingDoomClassic();

		// save the screenshot and audio from the last draw if needed
		//if ( aviCaptureMode ) {
		//	anString name = va( "demos/%s/%s_%05i.tga", aviDemoShortName.c_str(), aviDemoShortName.c_str(), aviDemoFrameCount++ );
		//	renderSystem->TakeScreenshot( com_aviDemoWidth.GetInteger(), com_aviDemoHeight.GetInteger(), name, com_aviDemoSamples.GetInteger(), nullptr );

			// remove any printed lines at the top before taking the screenshot
			console->ClearNotifyLines();

			// this will call Draw, possibly multiple times if com_aviDemoSamples is > 1
			//renderSystem->TakeScreenshot( com_aviDemoWidth.GetInteger(), com_aviDemoHeight.GetInteger(), name, com_aviDemoSamples.GetInteger(), nullptr );
		//}

		//--------------------------------------------
		// wait for the GPU to finish drawing
		//
		// It is imporant to minimize the time spent between this
		// section and the call to renderSystem->RenderCommandBuffers(),
		// because the GPU is completely idle.
		//--------------------------------------------
		// this should exit right after vsync, with the GPU idle and ready to draw
		// This may block if the GPU isn't finished renderng the previous frame.
		frameTiming.startSyncTime = Sys_Microseconds();
		const setBufferCommand_t * renderCommands = nullptr;
		if ( com_smp.GetBool() ) {
			renderCommands = renderSystem->SwapCommandBuffers( &time_frontend, &time_backend, &time_shadows, &time_gpu );
		} else {
			// the GPU will stay idle through command generation for minimal
			// input latency
			renderSystem->SwapCommandBuffers_FinishRendering( &time_frontend, &time_backend, &time_shadows, &time_gpu );
		}
		frameTiming.finishSyncTime = Sys_Microseconds();

		//--------------------------------------------
		// Determine how many game tics we are going to run,
		// now that the previous frame is completely finished.
		//
		// It is important that any waiting on the GPU be done
		// before this, or there will be a bad stuttering when
		// dropping frames for performance management.
		//--------------------------------------------

		// input:
		// thisFrameTime
		// com_noSleep
		// com_engineHz
		// com_fixedTic
		// com_deltaTimeClamp
		// IsMultiplayer
		//
		// in/out state:
		// gameFrame
		// gameTimeResidual
		// lastFrameTime
		// syncNextFrame
		//
		// Output:
		// numGameFrames

		// How many game frames to run
		int numGameFrames = 0;

		for (;;) {
			const int thisFrameTime = Sys_Milliseconds();
			static int lastFrameTime = thisFrameTime;	// initialized only the first time
			const int deltaMilliseconds = thisFrameTime - lastFrameTime;
			lastFrameTime = thisFrameTime;

			// if there was a large gap in time since the last frame, or the frame
			// rate is very very low, limit the number of frames we will run
			const int clampedDeltaMilliseconds = Min( deltaMilliseconds, com_deltaTimeClamp.GetInteger() );

			gameTimeResidual += clampedDeltaMilliseconds * timescale.GetFloat();

			// don't run any frames when paused
			if ( pauseGame ) {
				gameFrame++;
				gameTimeResidual = 0;
				break;
			}

			// debug cvar to force multiple game tics
			if ( com_fixedTic.GetInteger() > 0 ) {
				numGameFrames = com_fixedTic.GetInteger();
				gameFrame += numGameFrames;
				gameTimeResidual = 0;
				break;
			}

			if ( syncNextEngineFrame ) {
				// don't sleep at all
				syncNextEngineFrame = false;
				gameFrame++;
				numGameFrames++;
				gameTimeResidual = 0;
				break;
			}

			for ( ;; ) {
				// How much time to wait before running the next frame,
				// based on com_engineHz
				const int frameDelay = FRAME_TO_MSEC( gameFrame + 1 ) - FRAME_TO_MSEC( gameFrame );
				if ( gameTimeResidual < frameDelay ) {
					break;
				}
				gameTimeResidual -= frameDelay;
				gameFrame++;
				numGameFrames++;
				// if there is enough residual left, we may run additional frames
			}

			if ( numGameFrames > 0 ) {
				// ready to actually run them
				break;
			}

			// if we are vsyncing, we always want to run at least one game
			// frame and never sleep, which might happen due to scheduling issues
			// if we were just looking at real time.
			if ( com_noSleep.GetBool() ) {
				numGameFrames = 1;
				gameFrame += numGameFrames;
				gameTimeResidual = 0;
				break;
			}

			// not enough time has passed to run a frame, as might happen if
			// we don't have vsync on, or the monitor is running at 120hz while
			// com_engineHz is 60, so sleep a bit and check again
			Sys_Sleep( 0 );
		}

		//--------------------------------------------
		// It would be better to push as much of this as possible
		// either before or after the renderSystem->SwapCommandBuffers(),
		// because the GPU is completely idle.
		//--------------------------------------------

		// Update session and syncronize to the new session state after sleeping
		session->UpdateSignInManager();
		session->Pump();
		session->ProcessSnapAckQueue();

		if ( session->GetState() == ARCSession::LOADING ) {
			// If the session reports we should be loading a map, load it!
			ExecuteMapChange();
			mapSpawnData.savegameFile = nullptr;
			mapSpawnData.persistentPlayerInfo.Clear();
			return;
		} else if ( session->GetState() != ARCSession::INGAME && mapSpawned ) {
			// If the game is running, but the session reports we are not in a game, disconnect
			// This happens when a server disconnects us or we sign out
			LeaveGame();
			return;
		}

		if ( mapSpawned && !pauseGame ) {
			if ( IsClient() ) {
				RunNetworkSnapshotFrame();
			}
		}

		ExecuteReliableMessages();

		// send frame and mouse events to active guis
		GuiFrameEvents();

		//--------------------------------------------
		// Prepare usercmds and kick off the game processing
		// in a background thread
		//--------------------------------------------

		// get the previous usercmd for bypassed head tracking transform
		const usercmd_t	previousCmd = usercmdGen->GetCurrentUsercmd();

		// build a new usercmd
		int deviceNum = session->GetSignInManager().GetMasterInputDevice();
		usercmdGen->BuildCurrentUsercmd( deviceNum );
		if ( deviceNum == -1 ) {
			for ( int i = 0; i < MAX_INPUT_DEVICES; i++ ) {
				Sys_PollJoystickInputEvents( i );
				Sys_EndJoystickInputEvents();
			}
		}
		if ( pauseGame ) {
			usercmdGen->Clear();
		}

		usercmd_t newCmd = usercmdGen->GetCurrentUsercmd();

		// Store server game time - don't let time go past last SS time in case we are extrapolating
		if ( IsClient() ) {
			newCmd.serverGameMilliseconds = std::min( Game()->GetServerGameTimeMs(), Game()->GetSSEndTime() );
		} else {
			newCmd.serverGameMilliseconds = Game()->GetServerGameTimeMs();
		}

		userCmdMgr.MakeReadPtrCurrentForPlayer( Game()->GetLocalClientNum() );

		// Stuff a copy of this userCmd for each game frame we are going to run.
		// Ideally, the usercmds would be built in another thread so you could
		// still get 60hz control accuracy when the game is running slower.
		for ( int i = 0; i < numGameFrames; i++ ) {
			newCmd.clientGameMilliseconds = FRAME_TO_MSEC( gameFrame-numGameFrames+i+1 );
			userCmdMgr.PutUserCmdForPlayer( game->GetLocalClientNum(), newCmd );
		}

		// start the game / draw command generation thread going in the background
		gameReturn_t ret = gameThread.RunGameAndDraw( numGameFrames, userCmdMgr, IsClient(), gameFrame - numGameFrames );

		if ( !com_smp.GetBool() ) {
			// in non-smp mode, run the commands we just generated, instead of
			// frame-delayed ones from a background thread
			renderCommands = renderSystem->SwapCommandBuffers_FinishCommandBuffers();
		}

		//----------------------------------------
		// Run the render back end, getting the GPU busy with new commands
		// ASAP to minimize the pipeline bubble.
		//----------------------------------------
		frameTiming.startRenderTime = Sys_Microseconds();
		renderSystem->RenderCommandBuffers( renderCommands );
		if ( com_sleepRender.GetInteger() > 0 ) {
			// debug tool to test frame adaption
			Sys_Sleep( com_sleepRender.GetInteger() );
		}
		frameTiming.finishRenderTime = Sys_Microseconds();

		// make sure the game / draw thread has completed
		// This may block if the game is taking longer than the render back end
		gameThread.WaitForThread();

		// Send local usermds to the server.
		// This happens after the game frame has run so that prediction data is up to date.
		SendUsercmds( Game()->GetLocalClientNum() );

		// Now that we have an updated game frame, we can send out new snapshots to our clients
		session->Pump(); // Pump to get updated usercmds to relay
		SendSnapshots();

		// Render the sound system using the latest commands from the game thread
		if ( pauseGame ) {
			soundWorld->Pause();
			soundSystem->SetPlayingSoundWorld( menuSoundWorld );
		} else {
			soundWorld->UnPause();
			soundSystem->SetPlayingSoundWorld( soundWorld );
		}
		soundSystem->Render();

		// process the game return for map changes, etc
		ProcessGameReturn( ret );

		idLobbyBase & lobby = session->GetActivePlatformLobbyBase();
		if ( lobby.HasActivePeers() ) {
			if ( com_drawDebugHud.GetInteger() == 1 ) {
				lobby.DrawDebugNetworkHUD();
			}
			if ( com_drawDebugHud.GetInteger() == 2 ) {
				lobby.DrawDebugNetworkHUD2();
			}
			lobby.DrawDebugNetworkHUD_ServerSnapshotMetrics( com_drawDebugHud.GetInteger() == 3 );
		}

		// report timing information
		if ( com_speeds.GetBool() ) {
			static int lastTime = Sys_Milliseconds();
			int	nowTime = Sys_Milliseconds();
			int	com_frameMsec = nowTime - lastTime;
			lastTime = nowTime;
			Printf( "frame:%d all:%3d gfr:%3d rf:%3lld bk:%3lld\n", anLibrary::frameNumber, com_frameMsec, time_gameFrame, time_frontend / 1000, time_backend / 1000 );
			time_gameFrame = 0;
			time_gameDraw = 0;
		}

		// the FPU stack better be empty at this point or some bad code or compiler bug left values on the stack
		if ( !Sys_FPU_StackIsEmpty() ) {
			Printf( Sys_FPU_GetState() );
			FatalError( "anCommon::Frame: the FPU stack is not empty at the end of the frame\n" );
		}

		mainFrameTiming = frameTiming;

		//session->GetSaveGameManager().Pump();
	} catch( arcExceptions & ) {
		return;			// an ERP_DROP was thrown
	}
}