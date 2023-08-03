#include "../idlib/Lib.h"
#pragma hdrstop

#include "Game_local.h"

const int IMPULSE_DELAY = 150;
/*
==============
arcNetBasePlayerView::arcNetBasePlayerView
==============
*/
arcNetBasePlayerView::arcNetBasePlayerView() {
	memset( screenBlobs, 0, sizeof( screenBlobs ) );
	memset( &view, 0, sizeof( view ) );
	player = nullptr;
	dvMaterial = declManager->FindMaterial( "_scratch" );
	irGogglesMaterial = declManager->FindMaterial( "textures/decals/irblend" );
	bloodSprayMaterial = declManager->FindMaterial( "textures/decals/bloodspray" );
	dvFinishTime = 0;
	kickFinishTime = 0;
	kickAngles.Zero();
	lastDamageTime = 0.0f;
	fadeTime = 0;
	fadeRate = 0.0;
	fadeFromColor.Zero();
	fadeToColor.Zero();
	fadeColor.Zero();
	shakeAng.Zero();

	ClearEffects();
}

/*
==============
arcNetBasePlayerView::SetPlayerEntity
==============
*/
void arcNetBasePlayerView::SetPlayerEntity( arcNetBasePlayer *playerEnt ) {
	player = playerEnt;
}

/*
==============
arcNetBasePlayerView::ClearEffects
==============
*/
void arcNetBasePlayerView::ClearEffects() {
	lastDamageTime = MS2SEC( gameLocal.time - 99999 );

	dvFinishTime = ( gameLocal.time - 99999 );
	kickFinishTime = ( gameLocal.time - 99999 );

	for ( int i = 0 ; i < MAX_SCREEN_BLOBS ; i++ ) {
		screenBlobs[i].finishTime = gameLocal.time;
	}

	fadeTime = 0;
	bfgVision = false;
}

/*
==============
arcNetBasePlayerView::GetScreenBlob
==============
*/
screenBlob_t *arcNetBasePlayerView::GetScreenBlob() {
	screenBlob_t *oldest = &screenBlobs[0];

	for ( int i = 1 ; i < MAX_SCREEN_BLOBS ; i++ ) {
		if ( screenBlobs[i].finishTime < oldest->finishTime ) {
			oldest = &screenBlobs[i];
		}
	}
	return oldest;
}

/*
==============
arcNetBasePlayerView::DamageImpulse

LocalKickDir is the direction of force in the player's coordinate system,
which will determine the head kick direction
==============
*/
void arcNetBasePlayerView::DamageImpulse( anVec3 localKickDir, const anDict *damageDef ) {
	float dvTime = damageDef->GetFloat( "dv_time" );
	if ( dvTime ) {
		if ( dvFinishTime < gameLocal.time ) {
			dvFinishTime = gameLocal.time;
		}
		dvFinishTime += g_dvTime.GetFloat() * dvTime;
		// don't let it add up too much in god mode
		if ( dvFinishTime > gameLocal.time + 5000 ) {
			dvFinishTime = gameLocal.time + 5000;
		}
	}

	//
	// head angle kick
	//
	float	kickTime = damageDef->GetFloat( "kick_time" );
	if ( kickTime ) {
		kickFinishTime = gameLocal.time + g_kickTime.GetFloat() * kickTime;

		// forward / back kick will pitch view
		kickAngles[0] = localKickDir[0];

		// side kick will yaw view
		kickAngles[1] = localKickDir[1]*0.5f;

		// up / down kick will pitch view
		kickAngles[0] += localKickDir[2];

		// roll will come from  side
		kickAngles[2] = localKickDir[1];

		float kickAmplitude = damageDef->GetFloat( "kick_amplitude" );
		if ( kickAmplitude ) {
			kickAngles *= kickAmplitude;
		}
	}

	//
	// screen blob
	//
	float	blobTime = damageDef->GetFloat( "blob_time" );
	if ( blobTime ) {
		screenBlob_t	*blob = GetScreenBlob();
		blob->startFadeTime = gameLocal.time;
		blob->finishTime = gameLocal.time + blobTime * g_blobTime.GetFloat();

		const char *materialName = damageDef->GetString( "mtr_blob" );
		blob->material = declManager->FindMaterial( materialName );
		blob->x = damageDef->GetFloat( "blob_x" );
		blob->x += ( gameLocal.random.RandomInt()&63 ) - 32;
		blob->y = damageDef->GetFloat( "blob_y" );
		blob->y += ( gameLocal.random.RandomInt()&63 ) - 32;

		float scale = ( 256 + ( ( gameLocal.random.RandomInt()&63 ) - 32 ) ) / 256.0f;
		blob->w = damageDef->GetFloat( "blob_width" ) * g_blobSize.GetFloat() * scale;
		blob->h = damageDef->GetFloat( "blob_height" ) * g_blobSize.GetFloat() * scale;
		blob->s1 = 0;
		blob->t1 = 0;
		blob->s2 = 1;
		blob->t2 = 1;
	}

	//
	// save lastDamageTime for tunnel vision accentuation
	//
	lastDamageTime = MS2SEC( gameLocal.time );

}

/*
==================
arcNetBasePlayerView::AddBloodSpray

If we need a more generic way to add blobs then we can do that
but having it localized here lets the material be pre-looked up etc.
==================
*/
void arcNetBasePlayerView::AddBloodSpray( float duration ) {
}

/*
==================
arcNetBasePlayerView::WeaponFireFeedback

Called when a weapon fires, generates head twitches, etc
==================
*/
void arcNetBasePlayerView::WeaponFireFeedback( const anDict *weaponDef ) {
	int recoilTime = weaponDef->GetInt( "recoilTime" );
	// don't shorten a damage kick in progress
	if ( recoilTime && kickFinishTime < gameLocal.time ) {
		anAngles angles;
		weaponDef->GetAngles( "recoilAngles", "5 0 0", angles );
		kickAngles = angles;
		int	finish = gameLocal.time + g_kickTime.GetFloat() * recoilTime;
		kickFinishTime = finish;
	}
}

/*
===================
arcNetBasePlayerView::CalculateShake
===================
*/
void arcNetBasePlayerView::CalculateShake() {
	anVec3	origin, matrix;

	float shakeVolume = gameSoundWorld->CurrentShakeAmplitudeForPosition( gameLocal.time, player->firstPersonViewOrigin );
	//
	// shakeVolume should somehow be molded into an angle here
	// it should be thought of as being in the range 0.0 -> 1.0, although
	// since CurrentShakeAmplitudeForPosition() returns all the shake sounds
	// the player can hear, it can go over 1.0 too.
	//
	shakeAng[0] = gameLocal.random.CRandomFloat() * shakeVolume;
	shakeAng[1] = gameLocal.random.CRandomFloat() * shakeVolume;
	shakeAng[2] = gameLocal.random.CRandomFloat() * shakeVolume;
}

/*
===================
arcNetBasePlayerView::ShakeAxis
===================
*/
anMat3 arcNetBasePlayerView::ShakeAxis() const {
	return shakeAng.ToMat3();
}

/*
===================
arcNetBasePlayerView::AngleOffset

  kickVector, a world space direction that the attack should
===================
*/
anAngles arcNetBasePlayerView::AngleOffset() const {
	anAngles ang.Zero();

	if ( gameLocal.time < kickFinishTime ) {
		float offset = kickFinishTime - gameLocal.time;

		anAngles ang = kickAngles * offset * offset * g_kickAmplitude.GetFloat();

		for ( int i = 0 ; i < 3 ; i++ ) {
			if ( ang[i] > 70.0f ) {
				ang[i] = 70.0f;
			} else if ( ang[i] < -70.0f ) {
				ang[i] = -70.0f;
			}
		}
	}
	return ang;
}

/*
==================
arcNetBasePlayerView::SingleView
==================
*/
void arcNetBasePlayerView::SingleView( anUserInterface *hud, const renderView_t *view ) {
	// normal rendering
	if ( !view ) {
		return;
	}

	// place the sound origin for the player
	gameSoundWorld->PlaceListener( view->vieworg, view->viewaxis, player->entityNumber + 1, gameLocal.time, hud ? hud->State().GetString( "location" ) : "Undefined" );

	// if the objective system is up, don't do normal drawing
	if ( player->objectiveSystemOpen ) {
		player->objectiveSystem->Redraw( gameLocal.time );
		return;
	}

	// hack the shake in at the very last moment, so it can't cause any consistency problems
	renderView_t	hackedView = *view;
	hackedView.viewaxis = hackedView.viewaxis * ShakeAxis();

	gameRenderWorld->RenderScene( &hackedView );

	// draw screen blobs
	if ( !pm_thirdPerson.GetBool() ) {
		for ( int i = 0 ; i < MAX_SCREEN_BLOBS ; i++ ) {
			screenBlob_t	*blob = &screenBlobs[i];
			if ( blob->finishTime <= gameLocal.time ) {
				continue;
			}

			blob->y += blob->driftAmount;

			float	fade = ( float )( blob->finishTime - gameLocal.time ) / ( blob->finishTime - blob->startFadeTime );
			if ( fade > 1.0f ) {
				fade = 1.0f;
			}
			if ( fade ) {
				renderSystem->SetColor4( 1,1,1,fade );
				renderSystem->DrawStretchPic( blob->x, blob->y, blob->w, blob->h,blob->s1, blob->t1, blob->s2, blob->t2, blob->material );
			}
		}
	player->DrawHUD( hud );

	// test a single material drawn over everything
	if ( g_testPostProcess.GetString()[0] ) {
		const anMaterial *mtr = declManager->FindMaterial( g_testPostProcess.GetString(), false );
		if ( !mtr ) {
			common->Printf( "Material not found.\n" );
			g_testPostProcess.SetString( "" );
		} else {
			renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
			renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, mtr );
		}
	}
}

/*
=================
arcNetBasePlayerView::Flash

flashes the player view with the given color
=================
*/
void arcNetBasePlayerView::Flash( anVec4 color, int time ) {
	Fade( anVec4( 0, 0, 0, 0), time );
	fadeFromColor = colorWhite;
}

/*
=================
arcNetBasePlayerView::Fade

used for level transition fades
assumes: color.w is 0 or 1
=================
*/
void arcNetBasePlayerView::Fade( anVec4 color, int time ) {
	if ( !fadeTime ) {
		fadeFromColor.Set( 0.0f, 0.0f, 0.0f, 1.0f - color[ 3 ] );
	} else {
		fadeFromColor = fadeColor;
	}
	fadeToColor = color;

	if ( time <= 0 ) {
		fadeRate = 0;
		time = 0;
		fadeColor = fadeToColor;
	} else {
		fadeRate = 1.0f / ( float )time;
	}

	if ( gameLocal.realClientTime == 0 && time == 0 ) {
		fadeTime = 1;
	} else {
		fadeTime = gameLocal.realClientTime + time;
	}
}

/*
=================
arcNetBasePlayerView::ScreenFade
=================
*/
void arcNetBasePlayerView::ScreenFade() {
	int		msec;
	float	t;

	if ( !fadeTime ) {
		return;
	}

	msec = fadeTime - gameLocal.realClientTime;

	if ( msec <= 0 ) {
		fadeColor = fadeToColor;
		if ( fadeColor[ 3 ] == 0.0f ) {
			fadeTime = 0;
		}
	} else {
		t = ( float )msec * fadeRate;
		fadeColor = fadeFromColor * t + fadeToColor * ( 1.0f - t );
	}

	if ( fadeColor[ 3 ] != 0.0f ) {
		renderSystem->SetColor4( fadeColor[ 0 ], fadeColor[ 1 ], fadeColor[ 2 ], fadeColor[ 3 ] );
		renderSystem->DrawStretchPic( 0, 0, 640, 480, 0, 0, 1, 1, declManager->FindMaterial( "_white" ) );
	}
}

/*
===================
arcNetBasePlayerView::RenderPlayerView
===================
*/
void arcNetBasePlayerView::RenderPlayerView( anUserInterface *hud ) {
	const renderView_t *view = player->GetRenderView();
	SingleView( hud, view );
	ScreenFade();
}

