#include "../idlib/Lib.h"
#pragma hdrstop

#include "Game_local.h"

/*
===============================================================================

  idCamera

  Base class for cameras

===============================================================================
*/

ABSTRACT_DECLARATION( anEntity, idCamera )
END_CLASS

/*
=====================
idCamera::Spawn
=====================
*/
void idCamera::Spawn( void ) {
}

/*
=====================
idCamera::GetRenderView
=====================
*/
renderView_t *idCamera::GetRenderView() {
	renderView_t *rv = anEntity::GetRenderView();
	GetViewParms( rv );
	return rv;
}

/***********************************************************************

  idCameraView

***********************************************************************/
const anEventDef EV_Camera_SetAttachments( "<getattachments>", nullptr );


// bdube: added events
const anEventDef EV_SetFOV	(	 "setFOV", "f" );
const anEventDef EV_GetFOV		( "getFOV", nullptr, 'f' );
const anEventDef EV_BlendFOV	( "blendFOV", "fff" );


CLASS_DECLARATION( idCamera, idCameraView )
	EVENT( EV_Activate,				idCameraView::Event_Activate )
	EVENT( EV_Camera_SetAttachments, idCameraView::Event_SetAttachments )


// bdube: added events
	EVENT( EV_SetFOV,				idCameraView::Event_SetFOV )
	EVENT( EV_BlendFOV,				idCameraView::Event_BlendFOV )
	EVENT( EV_GetFOV,				idCameraView::Event_GetFOV )

END_CLASS

/*
===============
idCameraView::idCameraView
================
*/
idCameraView::idCameraView() {

// bdube: interpolate fov
// scork: get it from the cvar, don't assume 90
	fov.Init ( gameLocal.time, 0, g_fov.GetFloat(), g_fov.GetFloat() );

	attachedTo = nullptr;
	attachedView = nullptr;
}

/*
===============
idCameraView::Save
================
*/
void idCameraView::Save( anSaveGame *savefile ) const {

// bdube: fov interpolated now
	savefile->WriteInt( fov.GetDuration() );
	savefile->WriteInt( fov.GetStartTime() );
	savefile->WriteFloat( fov.GetStartValue() );
	savefile->WriteFloat( fov.GetEndValue() );

	savefile->WriteObject( attachedTo );
	savefile->WriteObject( attachedView );
}

/*
===============
idCameraView::Restore
================
*/
void idCameraView::Restore( anRestoreGame *savefile ) {

// bdube: fov interpolated now
	int set;
	float setf;
	savefile->ReadInt( set );
	fov.SetDuration( set );
	savefile->ReadInt( set );
	fov.SetStartTime( set );
	savefile->ReadFloat( setf );
	fov.SetStartValue( setf );
	savefile->ReadFloat( setf );
	fov.SetEndValue( setf );

	savefile->ReadObject( reinterpret_cast<anClass *&>( attachedTo ) );
	savefile->ReadObject( reinterpret_cast<anClass *&>( attachedView ) );
}

/*
===============
idCameraView::Event_SetAttachments
================
*/
void idCameraView::Event_SetAttachments(  ) {
	SetAttachment( &attachedTo, "attachedTo" );
	SetAttachment( &attachedView, "attachedView" );
}

/*
===============
idCameraView::Event_Activate
================
*/
void idCameraView::Event_Activate( anEntity *activator ) {
	if ( spawnArgs.GetBool( "trigger" ) ) {
		if (gameLocal.GetCamera() != this) {
			if ( g_debugCinematic.GetBool() ) {
				gameLocal.Printf( "%d: '%s' start\n", gameLocal.frameNum, GetName() );
			}

			gameLocal.SetCamera(this);
		} else {
			if ( g_debugCinematic.GetBool() ) {
				gameLocal.Printf( "%d: '%s' stop\n", gameLocal.frameNum, GetName() );
			}
			gameLocal.SetCamera(nullptr );
		}
	}
}

/*
=====================
idCameraView::Stop
=====================
*/
void idCameraView::Stop( void ) {
	if ( g_debugCinematic.GetBool() ) {
		gameLocal.Printf( "%d: '%s' stop\n", gameLocal.frameNum, GetName() );
	}
	gameLocal.SetCamera(nullptr );
	ActivateTargets( gameLocal.GetLocalPlayer() );
}


// bdube: added events
/*
=====================
idCameraView::Event_SetFOV
=====================
*/
void idCameraView::Event_SetFOV ( float newfov )
{
	fov.Init ( gameLocal.time, 0, newfov, newfov );
}

/*
=====================
idCameraView::Event_BlendFOV
=====================
*/
void idCameraView::Event_BlendFOV ( float beginFOV, float endFOV, float blendTime )
{
	fov.Init ( gameLocal.time, SEC2MS(blendTime), beginFOV, endFOV );
}

/*
=====================
idCameraView::Event_GetFOV
=====================
*/
void idCameraView::Event_GetFOV()
{
	anThread::ReturnFloat(fov.GetCurrentValue(gameLocal.time));
}


/*
=====================
idCameraView::Spawn
=====================
*/
void idCameraView::SetAttachment( anEntity **e, const char *p  ) {
	const char *cam = spawnArgs.GetString( p );
	if ( strlen ( cam ) ) {
		*e = gameLocal.FindEntity( cam );
	}
}


/*
=====================
idCameraView::Spawn
=====================
*/
void idCameraView::Spawn( void ) {
	// if no target specified use ourself
	const char *cam = spawnArgs.GetString( "cameraTarget" );
	if ( strlen ( cam ) == 0) {
		spawnArgs.Set( "cameraTarget", spawnArgs.GetString( "name" ) );
	}

// bdube: interpolate fov
// scork: ... but default from the cvar, not hardwired 90
	fov.Init ( gameLocal.time, 0, spawnArgs.GetFloat( "fov", va( "%f",g_fov.GetFloat())), spawnArgs.GetFloat( "fov", va( "%f",g_fov.GetFloat())) );


	PostEventMS( &EV_Camera_SetAttachments, 0 );

	UpdateChangeableSpawnArgs(nullptr );
}

/*
=====================
idCamera::RenderView
=====================
*/
void idCameraView::GetViewParms( renderView_t *view ) {
	assert( view );

	if (view == nullptr ) {
		return;
	}

	anVec3 dir;
	anEntity *ent;

	if ( attachedTo ) {
		ent = attachedTo;
	} else {
		ent = this;
	}

	view->vieworg = ent->GetPhysics()->GetOrigin();
	if ( attachedView ) {
		dir = attachedView->GetPhysics()->GetOrigin() - view->vieworg;
		dir.Normalize();
		view->viewaxis = dir.ToMat3();
	} else {
		view->viewaxis = ent->GetPhysics()->GetAxis();
	}


// bdube: interpolate fov
	gameLocal.CalcFov( fov.GetCurrentValue( gameLocal.time ), view->fov_x, view->fov_y );

}










// rjohnson: camera is now contained in a def for frame commands

/***********************************************************************

	rvCameraAnimation

***********************************************************************/
/*
=====================
rvCameraAnimation::rvCameraAnimation
=====================
*/
rvCameraAnimation::rvCameraAnimation( void ) {
	frameRate = 0;
}

/*
=====================
rvCameraAnimation::rvCameraAnimation
=====================
*/
rvCameraAnimation::rvCameraAnimation( const idDeclCameraDef *cameraDef, const rvCameraAnimation *anim ) {
	cameraCuts = anim->cameraCuts;
	camera = anim->camera;
	frameLookup = anim->frameLookup;
	frameCommands = anim->frameCommands;
	frameRate = anim->frameRate;
	name = anim->name;
	realname = anim->realname;
}

/*
=====================
rvCameraAnimation::~rvCameraAnimation
=====================
*/
rvCameraAnimation::~rvCameraAnimation( void ) {
}

/*
=====================
rvCameraAnimation::Name
=====================
*/
const char *rvCameraAnimation::Name( void ) const {
	return name;
}

/*
=====================
rvCameraAnimation::FullName
=====================
*/
const char *rvCameraAnimation::FullName( void ) const {
	return realname;
}

/*
=====================
rvCameraAnimation::NumFrames
=====================
*/
int	rvCameraAnimation::NumFrames( void ) const {
	return camera.Num();
}

/*
=====================
rvCameraAnimation::NumCuts
=====================
*/
int	rvCameraAnimation::NumCuts( void ) const {
	return cameraCuts.Num();
}

void rvCameraAnimation::SetAnim( const idDeclCameraDef *cameraDef, const char *sourcename, const char *animname, anStr filename ) {
	int			version;
	anLexer		parser( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT );
	anToken		token;
	int			numFrames;
	int			numCuts;
	int			i;

	filename.SetFileExtension( MD5_CAMERA_EXT );
	if ( !parser.LoadFile( filename ) ) {
		gameLocal.Error( "Unable to load '%s' on '%s'", filename.c_str(), name.c_str() );
	}

	cameraCuts.Clear();
	cameraCuts.SetGranularity( 1 );
	camera.Clear();
	camera.SetGranularity( 1 );

	parser.ExpectTokenString( MD5_VERSION_STRING );
	version = parser.ParseInt();
	if ( version != MD5_VERSION ) {
		parser.Error( "Invalid version %d.  Should be version %d\n", version, MD5_VERSION );
	}

	// skip the commandline
	parser.ExpectTokenString( "commandline" );
	parser.ReadToken( &token );

	// parse num frames
	parser.ExpectTokenString( "numFrames" );
	numFrames = parser.ParseInt();
	if ( numFrames <= 0 ) {
		parser.Error( "Invalid number of frames: %d", numFrames );
	}

	// parse framerate
	parser.ExpectTokenString( "frameRate" );
	frameRate = parser.ParseInt();
	if ( frameRate <= 0 ) {
		parser.Error( "Invalid framerate: %d", frameRate );
	}

	// parse num cuts
	parser.ExpectTokenString( "numCuts" );
	numCuts = parser.ParseInt();
	if ( ( numCuts < 0 ) || ( numCuts > numFrames ) ) {
		parser.Error( "Invalid number of camera cuts: %d", numCuts );
	}

	// parse the camera cuts
	parser.ExpectTokenString( "cuts" );
	parser.ExpectTokenString( "{" );
	cameraCuts.SetNum( numCuts );
	for ( i = 0; i < numCuts; i++ ) {
		cameraCuts[i] = parser.ParseInt();
		if ( ( cameraCuts[i] < 1 ) || ( cameraCuts[i] >= numFrames ) ) {
			parser.Error( "Invalid camera cut" );
		}
	}
	parser.ExpectTokenString( "}" );

	// parse the camera frames
	parser.ExpectTokenString( "camera" );
	parser.ExpectTokenString( "{" );
	camera.SetNum( numFrames );
	for ( i = 0; i < numFrames; i++ ) {
		parser.Parse1DMatrix( 3, camera[i].t.ToFloatPtr() );
		parser.Parse1DMatrix( 3, camera[i].q.ToFloatPtr() );
		camera[i].fov = parser.ParseFloat();
	}
	parser.ExpectTokenString( "}" );

#if 0
	if ( !gameLocal.GetLocalPlayer() ) {
		return;
	}

	idDebugGraph gGraph;
	idDebugGraph tGraph;
	idDebugGraph qGraph;
	idDebugGraph dtGraph;
	idDebugGraph dqGraph;
	gGraph.SetNumSamples( numFrames );
	tGraph.SetNumSamples( numFrames );
	qGraph.SetNumSamples( numFrames );
	dtGraph.SetNumSamples( numFrames );
	dqGraph.SetNumSamples( numFrames );

	gameLocal.Printf( "\n\ndelta vec:\n" );
	float diff_t, last_t, t;
	float diff_q, last_q, q;
	diff_t = last_t = 0.0f;
	diff_q = last_q = 0.0f;
	for ( i = 1; i < numFrames; i++ ) {
		t = ( camera[i].t - camera[ i - 1 ].t ).Length();
		q = ( camera[i].q.ToQuat() - camera[ i - 1 ].q.ToQuat() ).Length();
		diff_t = t - last_t;
		diff_q = q - last_q;
		gGraph.AddValue( ( i % 10 ) == 0 );
		tGraph.AddValue( t );
		qGraph.AddValue( q );
		dtGraph.AddValue( diff_t );
		dqGraph.AddValue( diff_q );

		gameLocal.Printf( "%d: %.8f  :  %.8f,     %.8f  :  %.8f\n", i, t, diff_t, q, diff_q  );
		last_t = t;
		last_q = q;
	}

	gGraph.Draw( colorBlue, 300.0f );
	tGraph.Draw( colorOrange, 60.0f );
	dtGraph.Draw( colorYellow, 6000.0f );
	qGraph.Draw( colorGreen, 60.0f );
	dqGraph.Draw( colorCyan, 6000.0f );
#endif
}

/*
=====================
rvCameraAnimation::AddFrameCommand

Returns nullptr if no error.
=====================
*/
const char *rvCameraAnimation::AddFrameCommand( const idDeclCameraDef *cameraDef, const anList<int>& frames, anLexer &src, const anDict *def ) {
	int					i;
	int					index;
	anStr				text;
	anStr				funcname;
	frameCommand_t		fc;
	anToken				token;

	memset( &fc, 0, sizeof( fc ) );

	if ( !src.ReadTokenOnLine( &token ) ) {
		return "Unexpected end of line";
	}

	if ( token == "call" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SCRIPTFUNCTION;
		fc.function = gameLocal.program.FindFunction( token );
		if ( !fc.function ) {
			return va( "Function '%s' not found", token.c_str() );
		}
	} else if ( token == "object_call" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SCRIPTFUNCTIONOBJECT;
		fc.string = new anStr( token );
	} else if ( token == "event" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_EVENTFUNCTION;
		const anEventDef *ev = anEventDef::FindEvent( token );
		if ( !ev ) {
			return va( "Event '%s' not found", token.c_str() );
		}
		if ( ev->GetNumArgs() != 0 ) {
			return va( "Event '%s' has arguments", token.c_str() );
		}
		fc.string = new anStr( token );
	}

// abahr:
	else if ( token == "eventArgs" ) {
		src.ParseRestOfLine( token );
		if ( token.Length() <= 0 ) {
			return "Unexpected end of line";
		}

		fc.type = FC_EVENTFUNCTION_ARGS;
		fc.parmList = new anList<anStr>();
		token.Split( *fc.parmList, ' ' );
		fc.event = anEventDef::FindEvent( (*fc.parmList)[0] );
		if ( !fc.event ) {
			SAFE_DELETE_PTR( fc.parmList );
			return va( "Event '%s' not found", (*fc.parmList)[0].c_str() );
		}

		fc.parmList->RemoveIndex( 0 );
	}

	else if ( token == "sound" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new anStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_voice" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_VOICE;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new anStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_voice2" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_VOICE2;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new anStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_body" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_BODY;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new anStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_body2" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_BODY2;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new anStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_body3" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_BODY3;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new anStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_weapon" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_WEAPON;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new anStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_global" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_GLOBAL;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new anStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_item" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_ITEM;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new anStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_chatter" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_CHATTER;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new anStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "skin" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SKIN;
		if ( token == "none" ) {
			fc.skin = nullptr;
		} else {
			fc.skin = declManager->FindSkin( token );
			if ( !fc.skin ) {
				return va( "Skin '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "fx" ) {

// bdube: use Raven effect system
		fc.type = FC_FX;

		// Get the effect name
		if ( !src.ReadTokenOnLine( &token ) ) {
			return va( "missing effect name" );
		}

		// Effect is indirect if it starts with fx_
		if ( !anStr::Icmpn ( token, "fx_", 3 ) ) {
			fc.string = new anStr ( token );
		} else {
			fc.effect = ( const idDecl * )declManager->FindEffect( token );
		}

		// Joint specified?
		if ( src.ReadTokenOnLine ( &token ) ) {
			fc.joint = new anStr ( token );
		}

		// End joint specified?
		if ( src.ReadTokenOnLine ( &token ) ) {
			fc.joint2 = new anStr ( token );
		}

	} else if ( token == "trigger" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_TRIGGER;
		fc.string = new anStr( token );

// bdube: not using
/*
	} else if ( token == "triggerSmokeParticle" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_TRIGGER_SMOKE_PARTICLE;
		fc.string = new anStr( token );
*/

	} else if ( token == "direct_damage" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_DIRECTDAMAGE;
		if ( !gameLocal.FindEntityDef( token.c_str(), false ) ) {
			return va( "Unknown entityDef '%s'", token.c_str() );
		}
		fc.string = new anStr( token );
	} else if ( token == "muzzle_flash" ) {
/*		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		if ( ( token != "" ) && !modelDef->FindJoint( token ) ) {
			return va( "Joint '%s' not found", token.c_str() );
		}
		fc.type = FC_MUZZLEFLASH;
		fc.string = new anStr( token );*/
	} else if ( token == "muzzle_flash" ) {
		fc.type = FC_MUZZLEFLASH;
		fc.string = new anStr( "" );
	} else if ( token == "footstep" ) {
		fc.type = FC_FOOTSTEP;
	} else if ( token == "leftfoot" ) {
		fc.type = FC_LEFTFOOT;
	} else if ( token == "rightfoot" ) {
		fc.type = FC_RIGHTFOOT;
	} else if ( token == "enableEyeFocus" ) {
		fc.type = FC_ENABLE_EYE_FOCUS;
	} else if ( token == "disableEyeFocus" ) {
		fc.type = FC_DISABLE_EYE_FOCUS;
	} else if ( token == "disableGravity" ) {
		fc.type = FC_DISABLE_GRAVITY;
	} else if ( token == "enableGravity" ) {
		fc.type = FC_ENABLE_GRAVITY;
	} else if ( token == "jump" ) {
		fc.type = FC_JUMP;
	} else if ( token == "enableClip" ) {
		fc.type = FC_ENABLE_CLIP;
	} else if ( token == "disableClip" ) {
		fc.type = FC_DISABLE_CLIP;
	} else if ( token == "enableWalkIK" ) {
		fc.type = FC_ENABLE_WALK_IK;
	} else if ( token == "disableWalkIK" ) {
		fc.type = FC_DISABLE_WALK_IK;
	} else if ( token == "enableLegIK" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_ENABLE_LEG_IK;
		fc.index = atoi( token );
	} else if ( token == "disableLegIK" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_DISABLE_LEG_IK;
		fc.index = atoi( token );
	} else if ( token == "recordDemo" ) {
		fc.type = FC_RECORDDEMO;
		if ( src.ReadTokenOnLine( &token ) ) {
			fc.string = new anStr( token );
		}
	} else if ( token == "aviGame" ) {
		fc.type = FC_AVIGAME;
		if ( src.ReadTokenOnLine( &token ) ) {
			fc.string = new anStr( token );
		}

// bdube: added script commands
	} else if ( token == "ai_enablePain" ) {
		fc.type = FC_AI_ENABLE_PAIN;
	} else if ( token == "ai_disablePain" ) {
		fc.type = FC_AI_DISABLE_PAIN;
	} else if ( token == "ai_enableDamage" ) {
		fc.type = FC_AI_ENABLE_DAMAGE;
	} else if ( token == "ai_disableDamage" ) {
		fc.type = FC_AI_DISABLE_DAMAGE;
	} else if ( token == "ai_lockEnemyOrigin" ) {
		fc.type = FC_AI_LOCKENEMYORIGIN;
	} else if ( token == "ai_attack" ) {
		fc.type = FC_AI_ATTACK;

		// Name of attack
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line while looking for attack Name";
		}
		fc.string = new anStr( token );

		// Joint to attack from
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line while looking for attack joint";
		}
		fc.joint = new anStr( token );
	} else if ( token == "ai_attack_melee" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line while looking for melee attack name";
		}
		fc.type = FC_AI_ATTACK_MELEE;
		fc.string = new anStr( token );
	} else if ( token == "guievent" )  {
		fc.type = FC_GUIEVENT;
		if ( src.ReadTokenOnLine( &token ) )
		{
			fc.string = new anStr( token );
		}
	} else if ( token == "speak" )  {
		fc.type = FC_AI_SPEAK;
		if ( src.ReadTokenOnLine( &token ) ) {
			fc.string = new anStr( token );
		}

	} else {
		return va( "Unknown command '%s'", token.c_str() );
	}

	// check if we've initialized the frame loopup table
	if ( !frameLookup.Num() ) {
		// we haven't, so allocate the table and initialize it
		frameLookup.SetGranularity( 1 );
		frameLookup.SetNum( camera.Num() );
		for ( i = 0; i < frameLookup.Num(); i++ ) {
			frameLookup[i].num = 0;
			frameLookup[i].firstCommand = 0;
		}
	}


// bdube: support multiple frames
	for ( int ii = 0; ii < frames.Num(); ii ++ ) {
		int frameNum = frames[ii];

// mekberg: error out of frame command is out of range.
//			-1 because we don't want commands on the loop frame.
//			If the anim doesn't loop they won't get handled.
		if ( ( frameNum < 1 ) || ( frameNum > camera.Num() -1 ) ) {
			gameLocal.Error( "Frame command out of range: %d on  anim '%s'. Max %d.", frameNum, name.c_str(), camera.Num() -1 );
		}

		// Duplicate the frame info
		if ( ii != 0 ) {
			if ( fc.string ) {
				fc.string = new anStr ( fc.string->c_str() );
			}
			if ( fc.joint ) {
				fc.joint = new anStr ( fc.joint->c_str() );
			}
			if ( fc.joint2 ) {
				fc.joint2 = new anStr ( fc.joint2->c_str() );
			}
			if ( fc.parmList ) {
				fc.parmList = new anList<anStr>( *fc.parmList );
			}
		}

		// frame numbers are 1 based in .def files, but 0 based internally
		frameNum--;


	// allocate space for a new command
	frameCommands.Alloc();

	// calculate the index of the new command
	index = frameLookup[ frameNum ].firstCommand + frameLookup[ frameNum ].num;

	// move all commands from our index onward up one to give us space for our new command
	for ( i = frameCommands.Num() - 1; i > index; i-- ) {
		frameCommands[i] = frameCommands[ i - 1 ];
	}

	// fix the indices of any later frames to account for the inserted command
	for ( i = frameNum + 1; i < frameLookup.Num(); i++ ) {
		frameLookup[i].firstCommand++;
	}

	// store the new command
	frameCommands[ index ] = fc;

	// increase the number of commands on this frame
	frameLookup[ frameNum ].num++;


// bdube: loop frame commands
	}


	// return with no error
	return nullptr;
}

/*
=====================
rvCameraAnimation::CallFrameCommandSound
=====================
*/
void rvCameraAnimation::CallFrameCommandSound ( const frameCommand_t& command, anEntity *ent, const s_channelType channel ) const {

	int flags = 0;
	if ( channel == ( FC_SOUND_GLOBAL - FC_SOUND ) ) {
		flags = SSF_PRIVATE_SOUND;
	}

	if ( command.string ) {
		ent->StartSound ( command.string->c_str(), channel, flags, false, nullptr );
	} else {
		ent->StartSoundShader( command.soundShader, channel, flags, false, nullptr );
	}
}

/*
=====================
rvCameraAnimation::CallFrameCommands
=====================
*/
void rvCameraAnimation::CallFrameCommands( anEntity *ent, int from, int to ) const {
	int index;
	int end;
	int frame;
	int numframes;

	if ( !frameLookup.Num() ) {
		return;
	}

	numframes = NumFrames();

	frame = from;
	while( frame != to ) {
		frame++;
		if ( frame >= numframes ) {
			frame = 0;
		}

		index = frameLookup[ frame ].firstCommand;
		end = index + frameLookup[ frame ].num;
		while( index < end ) {
			const frameCommand_t &command = frameCommands[ index++ ];


// bdube: frame command debugging
			if ( g_showFrameCmds.GetBool() ) {
				anStr shortName;
				shortName = name;
				shortName.StripPath();
				shortName.StripFileExtension();
				gameLocal.Printf ( "Cameraframecmd: anim=%s frame=%d cmd=%s parm=%s\n",
							 shortName.c_str(),
							 frame + 1,
							 frameCommandInfo[command.type].name,
							 command.string?command.string->c_str():"???" );
			}


			switch ( command.type ) {
				case FC_SCRIPTFUNCTION: {
					gameLocal.CallFrameCommand( ent, command.function );
					break;
				}

// bdube: rewrote
				case FC_SCRIPTFUNCTIONOBJECT: {
					ent->ProcessEvent ( &EV_CallFunction, command.string->c_str() );
					break;
				}

				case FC_EVENTFUNCTION: {
					const anEventDef *ev = anEventDef::FindEvent( command.string->c_str() );
					ent->ProcessEvent( ev );
					break;
				}

// abahr:
				case FC_EVENTFUNCTION_ARGS: {
					assert( command.event );
					ent->ProcessEvent( command.event, (int)command.parmList );
					break;
				}
// bdube: support indirection and simplify
				case FC_SOUND:
				case FC_SOUND_VOICE:
				case FC_SOUND_VOICE2:
				case FC_SOUND_BODY:
				case FC_SOUND_BODY2:
				case FC_SOUND_BODY3:
				case FC_SOUND_WEAPON:
				case FC_SOUND_ITEM:
				case FC_SOUND_GLOBAL:
				case FC_SOUND_CHATTER:
					CallFrameCommandSound ( command, ent, (const s_channelType)(command.type - FC_SOUND) );
					break;


				case FC_FX: {

// bdube: use raven effect system
					rvClientEffect* cent;
					if ( command.string ) {
						if ( command.joint ) {
							cent = ent->PlayEffect ( command.string->c_str(), ent->GetAnimator()->GetJointHandle ( *command.joint ) );
						} else {
							cent = gameLocal.PlayEffect ( ent->spawnArgs, command.string->c_str(), ent->GetRenderEntity()->origin, ent->GetRenderEntity()->axis );
						}
					} else {
						if ( command.joint ) {
							cent = ent->PlayEffect ( command.effect, ent->GetAnimator()->GetJointHandle ( *command.joint ), vec3_origin, mat3_identity );
						} else {
							cent = gameLocal.PlayEffect (  command.effect, ent->GetRenderEntity()->origin, ent->GetRenderEntity()->axis );
						}
					}
					// End origin bone specified?


					if ( cent && command.joint2 && ent->IsType ( anAnimatedEntity::GetClassType() ) ) {

						cent->SetEndOrigin ( ent->GetAnimator()->GetJointHandle ( *command.joint2 ) );
					}

					break;
				}
				case FC_SKIN:
//					ent->SetSkin( command.skin );
					break;

				case FC_TRIGGER: {
					anEntity *target;

					target = gameLocal.FindEntity( command.string->c_str() );
					if ( target ) {
						target->Signal( SIG_TRIGGER );
						target->ProcessEvent( &EV_Activate, ent );
						target->TriggerGuis();
					} else {
						gameLocal.Warning( "Framecommand 'trigger' on entity '%s', anim '%s', frame %d: Could not find entity '%s'",
							ent->name.c_str(), FullName(), frame + 1, command.string->c_str() );
					}
					break;
				}

				case FC_DIRECTDAMAGE: {
					ent->ProcessEvent( &AI_DirectDamage, command.string->c_str() );
					break;
				}
				case FC_MUZZLEFLASH: {
					break;
				}
				case FC_FOOTSTEP : {
//					ent->ProcessEvent( &EV_Footstep );
					break;
				}
				case FC_LEFTFOOT: {
//					ent->ProcessEvent( &EV_FootstepLeft );
					break;
				}
				case FC_RIGHTFOOT: {
//					ent->ProcessEvent( &EV_FootstepRight );
					break;
				}
				case FC_ENABLE_EYE_FOCUS: {
					ent->ProcessEvent( &AI_EnableEyeFocus );
					break;
				}
				case FC_DISABLE_EYE_FOCUS: {
					ent->ProcessEvent( &AI_DisableEyeFocus );
					break;
				}
				case FC_DISABLE_GRAVITY: {
					ent->ProcessEvent( &AI_DisableGravity );
					break;
				}
				case FC_ENABLE_GRAVITY: {
					ent->ProcessEvent( &AI_EnableGravity );
					break;
				}
				case FC_JUMP: {
					ent->ProcessEvent( &AI_JumpFrame );
					break;
				}
				case FC_ENABLE_CLIP: {
					ent->ProcessEvent( &AI_EnableClip );
					break;
				}
				case FC_DISABLE_CLIP: {
					ent->ProcessEvent( &AI_DisableClip );
					break;
				}
				case FC_ENABLE_WALK_IK: {
//					ent->ProcessEvent( &EV_EnableWalkIK );
					break;
				}
				case FC_DISABLE_WALK_IK: {
//					ent->ProcessEvent( &EV_DisableWalkIK );
					break;
				}
				case FC_ENABLE_LEG_IK: {
//					ent->ProcessEvent( &EV_EnableLegIK, command.index );
					break;
				}
				case FC_DISABLE_LEG_IK: {
//					ent->ProcessEvent( &EV_DisableLegIK, command.index );
					break;
				}
				case FC_RECORDDEMO: {
					if ( command.string ) {
						cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "recordDemo %s", command.string->c_str() ) );
					} else {
						cmdSystem->BufferCommandText( CMD_EXEC_NOW, "stoprecording" );
					}
					break;
				}
				case FC_AVIGAME: {
					if ( command.string ) {
						cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "aviGame %s", command.string->c_str() ) );
					} else {
						cmdSystem->BufferCommandText( CMD_EXEC_NOW, "aviGame" );
					}
					break;
				}

				case FC_AI_ENABLE_PAIN:
//					ent->ProcessEvent ( &AI_EnablePain );
					break;

				case FC_AI_DISABLE_PAIN:
//					ent->ProcessEvent ( &AI_DisablePain );
					break;

				case FC_AI_ENABLE_DAMAGE:
//					ent->ProcessEvent ( &AI_EnableDamage );
					break;

				case FC_AI_LOCKENEMYORIGIN:
//					ent->ProcessEvent ( &AI_LockEnemyOrigin );
					break;

				case FC_AI_ATTACK:
//					ent->ProcessEvent ( &AI_Attack, command.string->c_str(), command.joint->c_str() );
					break;

				case FC_AI_DISABLE_DAMAGE:
//					ent->ProcessEvent ( &AI_DisableDamage );
					break;

				case FC_AI_SPEAK:
//					ent->ProcessEvent( &AI_Speak, command.string->c_str() );
					break;

				case FC_ACT_ATTACH_HIDE:
/*
					if ( ent->IsType(anActor::GetClassType()) )
					{
						static_cast<anActor*>(ent)->HideAttachment( command.string->c_str() );
					}
*/
					break;

				case FC_ACT_ATTACH_SHOW:
/*
					if ( ent->IsType(anActor::GetClassType()) )
					{
						static_cast<anActor*>(ent)->ShowAttachment( command.string->c_str() );
					}
*/
					break;

				case FC_AI_ATTACK_MELEE:
//					ent->ProcessEvent( &AI_AttackMelee, command.string->c_str() );
					break;
			}
		}
	}
}









/***********************************************************************

	idDeclCameraDef

***********************************************************************/

/*
=====================
idDeclCameraDef::idDeclCameraDef
=====================
*/
idDeclCameraDef::idDeclCameraDef() {
}

/*
=====================
idDeclCameraDef::~idDeclCameraDef
=====================
*/
idDeclCameraDef::~idDeclCameraDef() {
	FreeData();
}


// jsinger: Added to support serialization/deserialization of binary decls
#ifdef RV_BINARYDECLS
/*
=====================
idDeclCameraDef::idDeclCameraDef( SerialInputStream &stream )
=====================
*/
idDeclCameraDef::idDeclCameraDef( SerialInputStream &stream )
{
	// not supported yet
	assert(false);
}

void idDeclCameraDef::Write( SerialOutputStream &stream ) const
{
	// not supported yet
	assert(false);
}

void idDeclCameraDef::AddReferences() const
{
	// not supported yet
	assert(false);
}
#endif

/*
=================
idDeclCameraDef::Size
=================
*/

// jscott: made more accurate
size_t idDeclCameraDef::Size( void ) const {

	size_t	size;

	size = sizeof( idDeclCameraDef );
	size += anims.Allocated();

	return( size );
}


/*
=====================
idDeclCameraDef::CopyDecl
=====================
*/
void idDeclCameraDef::CopyDecl( const idDeclCameraDef *decl ) {
	int i;

	FreeData();

	anims.SetNum( decl->anims.Num() );
	for ( i = 0; i < anims.Num(); i++ ) {
		anims[i] = new rvCameraAnimation( this, decl->anims[i] );
	}
}

/*
=====================
idDeclCameraDef::FreeData
=====================
*/
void idDeclCameraDef::FreeData( void ) {
	anims.DeleteContents( true );
}

/*
================
idDeclCameraDef::DefaultDefinition
================
*/
const char *idDeclCameraDef::DefaultDefinition( void ) const {
	return "{ }";
}



/*
=====================
idDeclCameraDef::Touch
=====================
*/
void idDeclCameraDef::Touch( void ) const {
}

/*
=====================
idDeclCameraDef::ParseAnim
=====================
*/
bool idDeclCameraDef::ParseAnim( anLexer &src, int numDefaultAnims ) {
	int					i;
	int					len;
	rvCameraAnimation	*anim;
	anStr				alias;
	anToken				realname;
	anToken				token;
	int					numAnims;

	numAnims = 0;

	if ( !src.ReadToken( &realname ) ) {
		src.Warning( "Unexpected end of file" );
		MakeDefault();
		return false;
	}
	alias = realname;

	for ( i = 0; i < anims.Num(); i++ ) {
		if ( !anStr::Cmp( anims[i]->FullName(), realname ) ) {
			break;
		}
	}

	if ( ( i < anims.Num() ) && ( i >= numDefaultAnims ) ) {
		src.Warning( "Duplicate anim '%s'", realname.c_str() );
		MakeDefault();
		return false;
	}

	if ( i < numDefaultAnims ) {
		anim = anims[i];
	} else {
		// create the alias associated with this animation
		anim = new rvCameraAnimation();
		anims.Append( anim );
	}

	// random anims end with a number.  find the numeric suffix of the animation.
	len = alias.Length();
	for ( i = len - 1; i > 0; i-- ) {
		if ( !isdigit( alias[i] ) ) {
			break;
		}
	}

	// check for zero length name, or a purely numeric name
	if ( i <= 0 ) {
		src.Warning( "Invalid animation name '%s'", alias.c_str() );
		MakeDefault();
		return false;
	}

	// remove the numeric suffix
	alias.CapLength( i + 1 );

	// parse the anims from the string
	if ( !src.ReadToken( &token ) ) {
		src.Warning( "Unexpected end of file" );
		MakeDefault();
		return false;
	}

	anim->SetAnim( this, realname, alias, token );

	// parse any frame commands or animflags
	if ( src.CheckTokenString( "{" ) ) {
		while( 1 ) {
			if ( !src.ReadToken( &token ) ) {
				src.Warning( "Unexpected end of file" );
				MakeDefault();
				return false;
			}
			if ( token == "}" ) {
				break;
			} else if ( token == "frame" ) {
				// create a frame command

// bdube: Support a list of frame numbers
//				int			frameNum;
				const char	*err;
				anList<int> frameList;

				do
				{

				// make sure we don't have any line breaks while reading the frame command so the error line # will be correct
				if ( !src.ReadTokenOnLine( &token ) ) {
					src.Warning( "Missing frame # after 'frame'" );
					MakeDefault();
					return false;
				}
				if ( token.type == TT_PUNCTUATION && token == "-" ) {
					src.Warning( "Invalid frame # after 'frame'" );
					MakeDefault();
					return false;
				} else if ( token.type != TT_NUMBER || token.subtype == TT_FLOAT ) {
					src.Error( "expected integer value, found '%s'", token.c_str() );
				}

// bdube: multiple frames
					frameList.Append ( token.GetIntValue() );

				} while ( src.CheckTokenString ( "," ) );


				// put the command on the specified frame of the animation

// bdube: Support a list of frame numbers
				err = anim->AddFrameCommand( this, frameList, src, nullptr );

				if ( err ) {
					src.Warning( "%s", err );
					MakeDefault();
					return false;
				}
			} else {
				src.Warning( "Unknown command '%s'", token.c_str() );
				MakeDefault();
				return false;
			}
		}
	}

	return true;
}

/*
================
idDeclCameraDef::Parse
================
*/
bool idDeclCameraDef::Parse( const char *text, const int textLength, bool noCaching ) {
	anStr				filename;
	anStr				extension;
	anLexer				src;
	anToken				token;
	anToken				token2;
	int					numDefaultAnims;

	TIME_THIS_SCOPE( __FUNCLINE__);

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	numDefaultAnims = 0;
	while( 1 ) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( !token.Icmp( "}" ) ) {
			break;
		}
		else if ( token == "anim" ) {
			if ( !ParseAnim( src, numDefaultAnims ) ) {
				MakeDefault();
				return false;
			}
		} else {
			src.Warning( "unknown token '%s'", token.c_str() );
			MakeDefault();
			return false;
		}
	}

	// shrink the anim list down to save space
	anims.SetGranularity( 1 );
	anims.SetNum( anims.Num() );

	return true;
}

/*
=====================
idDeclCameraDef::Validate
=====================
*/
bool idDeclCameraDef::Validate( const char *psText, int iTextLength, anStr &strReportTo ) const {
	idDeclCameraDef *pSelf = (idDeclCameraDef*) declManager->AllocateDecl( DECL_MODELDEF );
	bool bOk = pSelf->Parse( psText, iTextLength, false );
	pSelf->FreeData();
	delete pSelf->base;
	delete pSelf;

	return bOk;
}

/*
=====================
idDeclCameraDef::HasAnim
=====================
*/
bool idDeclCameraDef::HasAnim( const char *name ) const {
	int	i;

	// find any animations with same name
	for ( i = 0; i < anims.Num(); i++ ) {
		if ( !anStr::Cmp( anims[i]->Name(), name ) ) {
			return true;
		}
	}

	return false;
}

/*
=====================
idDeclCameraDef::NumAnims
=====================
*/
int idDeclCameraDef::NumAnims( void ) const {
	return anims.Num() + 1;
}

/*
=====================
idDeclCameraDef::GetSpecificAnim

Gets the exact anim for the name, without randomization.
=====================
*/
int idDeclCameraDef::GetSpecificAnim( const char *name ) const {
	int	i;

	// find a specific animation
	for ( i = 0; i < anims.Num(); i++ ) {
		if ( !anStr::Cmp( anims[i]->FullName(), name ) ) {
			return i + 1;
		}
	}

	// didn't find it
	return 0;
}

/*
=====================
idDeclCameraDef::GetAnim
=====================
*/
int idDeclCameraDef::GetAnim( const char *name ) const {
	int				i;
	int				which;
	const int		MAX_ANIMS = 64;
	int				animList[ MAX_ANIMS ];
	int				numAnims;
	int				len;

	len = strlen( name );
	if ( len && anStr::CharIsNumeric( name[ len - 1 ] ) ) {
		// find a specific animation
		return GetSpecificAnim( name );
	}

	// find all animations with same name
	numAnims = 0;
	for ( i = 0; i < anims.Num(); i++ ) {
		if ( !anStr::Cmp( anims[i]->Name(), name ) ) {
			animList[ numAnims++ ] = i;
			if ( numAnims >= MAX_ANIMS ) {
				break;
			}
		}
	}

	if ( !numAnims ) {
		return 0;
	}

	// get a random anim
	//FIXME: don't access gameLocal here?
	which = gameLocal.random.RandomInt( numAnims );
	return animList[ which ] + 1;
}










/*
===============================================================================

  idCameraAnim

===============================================================================
*/
const anEventDef EV_Camera_Start( "start", nullptr );
const anEventDef EV_Camera_Stop( "stop", nullptr );


// mekberg: wait support
const anEventDef EV_Camera_IsActive( "isActive", "", 'd' );


CLASS_DECLARATION( idCamera, idCameraAnim )
	EVENT( EV_Thread_SetCallback,	idCameraAnim::Event_SetCallback )
	EVENT( EV_Camera_Stop,			idCameraAnim::Event_Stop )
	EVENT( EV_Camera_Start,			idCameraAnim::Event_Start )
	EVENT( EV_Activate,				idCameraAnim::Event_Activate )


	// mekberg: wait support
	EVENT( EV_IsActive,				idCameraAnim::Event_IsActive )

END_CLASS


/*
=====================
idCameraAnim::idCameraAnim
=====================
*/
idCameraAnim::idCameraAnim() {
	threadNum = 0;
	offset.Zero();
	cycle = 1;
	starttime = 0;
	activator = nullptr;
	lastFrame = -1;
}

/*
=====================
idCameraAnim::~idCameraAnim
=====================
*/
idCameraAnim::~idCameraAnim() {
	if ( gameLocal.GetCamera() == this ) {
		gameLocal.SetCamera( nullptr );
	}
}

/*
===============
idCameraAnim::Save
================
*/
void idCameraAnim::Save( anSaveGame *savefile ) const {
	savefile->WriteInt( threadNum );
	savefile->WriteVec3( offset );
	savefile->WriteInt( starttime );
	savefile->WriteInt( cycle );
	savefile->WriteInt( lastFrame );		// cnicholson: Added unsaved var

	activator.Save( savefile );
}

/*
===============
idCameraAnim::Restore
================
*/
void idCameraAnim::Restore( anRestoreGame *savefile ) {
	savefile->ReadInt( threadNum );
	savefile->ReadVec3( offset );
	savefile->ReadInt( starttime );
	savefile->ReadInt( cycle );
	savefile->ReadInt( lastFrame );		// cnicholson: Added unread var

	activator.Restore( savefile );

	LoadAnim();
}

/*
=====================
idCameraAnim::Spawn
=====================
*/
void idCameraAnim::Spawn( void ) {
	if ( spawnArgs.GetVector( "old_origin", "0 0 0", offset ) ) {
		offset = GetPhysics()->GetOrigin() - offset;
	} else {
		offset.Zero();
	}

	// always think during cinematics
	cinematic = true;

	LoadAnim();


#ifndef _CONSOLE
	// touch the cinematic streaming command file during build
	if ( cvarSystem->GetCVarBool( "com_makingBuild" ) && cvarSystem->GetCVarBool( "com_Bundler" ) )
	{
		anFile *file;
		anStr filename = "cinematics/";
		filename += gameLocal.mapFileNameStripped;
		filename += "_";
		filename += name;
		filename += ".cincmd";
		file = fileSystem->OpenFileRead( filename );
		fileSystem->CloseFile( file );
	}
#endif

}

/*
================
idCameraAnim::Load
================
*/
void idCameraAnim::LoadAnim( void ) {
	anLexer		parser( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT );
	anToken		token;
	anStr		filename;
	const char	*key;

	key = spawnArgs.GetString( "camera" );
	const idDecl *mapDecl = declManager->FindType( DECL_CAMERADEF, key, false );
	cameraDef = static_cast<const idDeclCameraDef *>( mapDecl );

	key = spawnArgs.GetString( "anim" );
	if ( !key ) {
		gameLocal.Error( "Missing 'anim' key on '%s'", name.c_str() );
	}

}

/*
===============
idCameraAnim::Start
================
*/
void idCameraAnim::Start( void ) {
	cycle = spawnArgs.GetInt( "cycle" );
	if ( !cycle ) {
		cycle = 1;
	}

	if ( g_debugCinematic.GetBool() ) {
		gameLocal.Printf( "%d: '%s' start\n", gameLocal.frameNum, GetName() );
	}

	lastFrame = -1;
	starttime = gameLocal.time;
	gameLocal.SetCamera( this );
	BecomeActive( TH_THINK );


// jnewquist: Track texture usage during cinematics for streaming purposes
#ifndef _CONSOLE
	renderSystem->TrackTextureUsage( idRenderSystem::TEXTURE_TRACK_BEGIN, cameraDef->GetAnim( 1 )->GetFrameRate(), GetName() );
#endif


	// if the player has already created the renderview for this frame, have him update it again so that the camera starts this frame

// mekberg: make sure render view is valid.
	if ( gameLocal.GetLocalPlayer( )->GetRenderView( ) && gameLocal.GetLocalPlayer()->GetRenderView()->time == gameLocal.time ) {

		gameLocal.GetLocalPlayer()->CalculateRenderView();
	}
}

/*
=====================
idCameraAnim::Stop
=====================
*/
void idCameraAnim::Stop( void ) {
	if ( gameLocal.GetCamera() == this ) {
		if ( g_debugCinematic.GetBool() ) {
			gameLocal.Printf( "%d: '%s' stop\n", gameLocal.frameNum, GetName() );
		}

		BecomeInactive( TH_THINK );
		gameLocal.SetCamera( nullptr );
		if ( threadNum ) {
			anThread::ObjectMoveDone( threadNum, this );
			threadNum = 0;
		}
		ActivateTargets( activator.GetEntity() );


// jnewquist: Track texture usage during cinematics for streaming purposes
#ifndef _CONSOLE
		renderSystem->TrackTextureUsage( idRenderSystem::TEXTURE_TRACK_END, cameraDef->GetAnim( 1 )->GetFrameRate() );
#endif

	}
}

/*
=====================
idCameraAnim::Think
=====================
*/
void idCameraAnim::Think( void ) {
	int frame;
	int frameTime;

	if ( thinkFlags & TH_THINK ) {
		// check if we're done in the Think function when the cinematic is being skipped (idCameraAnim::GetViewParms isn't called when skipping cinematics).

// abahr: removed '!'
		if ( gameLocal.skipCinematic ) {

			return;
		}

		if ( cameraDef->GetAnim( 1 )->NumFrames() < 2 ) {
			// 1 frame anims never end
			return;
		}

		if ( cameraDef->GetAnim( 1 )->GetFrameRate() == gameLocal.GetMHz() ) {
			frameTime	= gameLocal.time - starttime;
			frame		= frameTime / gameLocal.msec;
		} else {
			frameTime	= ( gameLocal.time - starttime ) * cameraDef->GetAnim( 1 )->GetFrameRate();
			frame		= frameTime / 1000;
		}

		if ( frame > cameraDef->GetAnim( 1 )->NumFrames() + cameraDef->GetAnim( 1 )->NumCuts() - 2 ) {
			if ( cycle > 0 ) {
				cycle--;
			}
			lastFrame = -1;

			if ( cycle != 0 ) {
				// advance start time so that we loop
				starttime += ( ( cameraDef->GetAnim( 1 )->NumFrames() - cameraDef->GetAnim( 1 )->NumCuts() ) * 1000 ) / cameraDef->GetAnim( 1 )->GetFrameRate();
			} else {
				Stop();
			}
		}
	}
}

/*
=====================
idCameraAnim::GetViewParms
=====================
*/
void idCameraAnim::GetViewParms( renderView_t *view ) {
	int				realFrame;
	int				frame;
	int				frameTime;
	float			lerp;
	float			invlerp;
	const cameraFrame_t	*camFrame;
	int				i;
	int				cut;
	anQuat			q1, q2, q3;

	assert( view );
	if ( !view ) {
		return;
	}

//jshepard: safety first
	if ( !cameraDef )	{
		gameLocal.Warning( "Invalid cameraDef in GetViewParms" );
		return;
	}

	if ( !cameraDef->GetAnim( 1 ) ) {
		return;
	}

	if ( cameraDef->GetAnim( 1 )->NumFrames() == 0 ) {
		// we most likely are in the middle of a restore
		// FIXME: it would be better to fix it so this doesn't get called during a restore
		return;
	}

	if ( cameraDef->GetAnim( 1 )->GetFrameRate() == gameLocal.GetMHz() ) {
		frameTime	= gameLocal.time - starttime;
		frame		= frameTime / gameLocal.msec;
		lerp		= 0.0f;
	} else {
		frameTime	= ( gameLocal.time - starttime ) * cameraDef->GetAnim( 1 )->GetFrameRate();
		frame		= frameTime / 1000;
		lerp		= ( frameTime % 1000 ) * 0.001f;
	}

	// skip any frames where camera cuts occur
	realFrame = frame;
	cut = 0;
	for ( i = 0; i < cameraDef->GetAnim( 1 )->NumCuts(); i++ ) {
		if ( frame < cameraDef->GetAnim( 1 )->GetCut( i ) ) {
			break;
		}
		frame++;
		cut++;
	}
	if ( lastFrame != frame ) {
		cameraDef->GetAnim( 1 )->CallFrameCommands( this, lastFrame, frame );
		lastFrame = frame;
	}

	if ( g_debugCinematic.GetBool() ) {
		int prevFrameTime	= ( gameLocal.time - starttime - gameLocal.msec ) * cameraDef->GetAnim( 1 )->GetFrameRate();
		int prevFrame		= prevFrameTime / 1000;
		int prevCut;

		prevCut = 0;
		for ( i = 0; i < cameraDef->GetAnim( 1 )->NumCuts(); i++ ) {
			if ( prevFrame < cameraDef->GetAnim( 1 )->GetCut( i ) ) {
				break;
			}
			prevFrame++;
			prevCut++;
		}

		if ( prevCut != cut ) {
			gameLocal.Printf( "%d: '%s' cut %d\n", gameLocal.frameNum, GetName(), cut );
		}
	}

	// clamp to the first frame.  also check if this is a one frame anim.  one frame anims would end immediately,
	// but since they're mainly used for static cams anyway, just stay on it infinitely.
	if ( ( frame < 0 ) || ( cameraDef->GetAnim( 1 )->NumFrames() < 2 ) ) {
		view->viewaxis = cameraDef->GetAnim( 1 )->GetAnim( 0 )->q.ToQuat().ToMat3();
		view->vieworg = cameraDef->GetAnim( 1 )->GetAnim( 0 )->t + offset;
		view->fov_x = cameraDef->GetAnim( 1 )->GetAnim( 0 )->fov;
	} else if ( frame > cameraDef->GetAnim( 1 )->NumFrames() - 2 ) {
		if ( cycle > 0 ) {
			cycle--;
		}

		if ( cycle != 0 ) {
			// advance start time so that we loop
			starttime += ( ( cameraDef->GetAnim( 1 )->NumFrames() - cameraDef->GetAnim( 1 )->NumCuts() ) * 1000 ) / cameraDef->GetAnim( 1 )->GetFrameRate();
			GetViewParms( view );
			return;
		}

		Stop();
		if ( gameLocal.GetCamera() != nullptr ) {
			// we activated another camera when we stopped, so get it's viewparms instead
			gameLocal.GetCamera()->GetViewParms( view );
			return;
		} else {
			// just use our last frame
			camFrame = cameraDef->GetAnim( 1 )->GetAnim( cameraDef->GetAnim( 1 )->NumFrames() - 1 );
			view->viewaxis = camFrame->q.ToQuat().ToMat3();
			view->vieworg = camFrame->t + offset;
			view->fov_x = camFrame->fov;
		}
	} else if ( lerp == 0.0f ) {
		camFrame = cameraDef->GetAnim( 1 )->GetAnim( frame );
		view->viewaxis = camFrame[0].q.ToMat3();
		view->vieworg = camFrame[0].t + offset;
		view->fov_x = camFrame[0].fov;
	} else {
		camFrame = cameraDef->GetAnim( 1 )->GetAnim( frame );
		invlerp = 1.0f - lerp;
		q1 = camFrame[0].q.ToQuat();
		q2 = camFrame[1].q.ToQuat();
		q3.Slerp( q1, q2, lerp );
		view->viewaxis = q3.ToMat3();
		view->vieworg = camFrame[0].t * invlerp + camFrame[1].t * lerp + offset;
		view->fov_x = camFrame[0].fov * invlerp + camFrame[1].fov * lerp;
	}

	gameLocal.CalcFov( view->fov_x, view->fov_x, view->fov_y );

	// setup the pvs for this frame
	UpdatePVSAreas( view->vieworg );

#if 0
	static int lastFrame = 0;
	static anVec3 lastFrameVec( 0.0f, 0.0f, 0.0f );
	if ( gameLocal.time != lastFrame ) {
		gameRenderWorld->DebugBounds( colorCyan, anBounds( view->vieworg ).Expand( 16.0f ), vec3_origin, gameLocal.msec );
		gameRenderWorld->DebugLine( colorRed, view->vieworg, view->vieworg + anVec3( 0.0f, 0.0f, 2.0f ), 10000, false );
		gameRenderWorld->DebugLine( colorCyan, lastFrameVec, view->vieworg, 10000, false );
		gameRenderWorld->DebugLine( colorYellow, view->vieworg + view->viewaxis[0] * 64.0f, view->vieworg + view->viewaxis[0] * 66.0f, 10000, false );
		gameRenderWorld->DebugLine( colorOrange, view->vieworg + view->viewaxis[0] * 64.0f, view->vieworg + view->viewaxis[0] * 64.0f + anVec3( 0.0f, 0.0f, 2.0f ), 10000, false );
		lastFrameVec = view->vieworg;
		lastFrame = gameLocal.time;
	}
#endif

	if ( g_showcamerainfo.GetBool() ) {
		gameLocal.Printf( "^5Frame: ^7%d/%d\n\n\n", realFrame + 1, cameraDef->GetAnim( 1 )->NumFrames() - cameraDef->GetAnim( 1 )->NumCuts() );
	}
// jnewquist: Track texture usage during cinematics for streaming purposes
#ifndef _CONSOLE
	renderSystem->TrackTextureUsage( idRenderSystem::TEXTURE_TRACK_UPDATE, realFrame );
#endif
}


/*
===============
idCameraAnim::Event_Activate
================
*/
void idCameraAnim::Event_Activate( anEntity *_activator ) {
	activator = _activator;
	if ( thinkFlags & TH_THINK ) {
		Stop();
	} else {
		Start();
	}
}

/*
===============
idCameraAnim::Event_Start
================
*/
void idCameraAnim::Event_Start( void ) {
	Start();
}

/*
===============
idCameraAnim::Event_Stop
================
*/
void idCameraAnim::Event_Stop( void ) {
	Stop();
}

/*
================
idCameraAnim::Event_SetCallback
================
*/
void idCameraAnim::Event_SetCallback( void ) {
	if ( ( gameLocal.GetCamera() == this ) && !threadNum ) {
		threadNum = anThread::CurrentThreadNum();
		anThread::ReturnInt( true );
	} else {
		anThread::ReturnInt( false );
	}
}


// mekberg: wait support
/*
================
idCameraAnim::Event_IsActive
================
*/
void idCameraAnim::Event_IsActive( void ) {
	anThread::ReturnFloat( gameLocal.GetCamera( ) ? 1.0f : 0.0f );
}




// jscott: portal sky support

/***********************************************************************

  rvCameraPortalSky

***********************************************************************/

CLASS_DECLARATION( idCamera, rvCameraPortalSky )
END_CLASS

/*
===============
rvCameraPortalSky::Save
================
*/
void rvCameraPortalSky::Save( anSaveGame *savefile ) const
{
}

/*
===============
rvCameraPortalSky::Restore
================
*/
void rvCameraPortalSky::Restore( anRestoreGame *savefile )
{
	// Run spawn to set default values
	Spawn();
}

/*
=====================
rvCameraPortalSky::Spawn
=====================
*/
void rvCameraPortalSky::Spawn( void )
{
	if ( gameLocal.GetPortalSky() )
	{
		gameLocal.Error( "Only one portal sky camera allowed" );
	}
	gameLocal.SetPortalSky( this );
}

/*
=====================
rvCameraPortalSky::GetViewParms
=====================
*/
void rvCameraPortalSky::GetViewParms( renderView_t *view )
{
	assert( view );
	if ( view )
	{
		view->vieworg = GetPhysics()->GetOrigin();
		view->viewID = -1;
	}
}


/***********************************************************************

  rvCameraPlayback

***********************************************************************/

CLASS_DECLARATION( idCamera, rvCameraPlayback )
END_CLASS

/*
===============
rvCameraPlayback::Save
================
*/
void rvCameraPlayback::Save( anSaveGame *savefile ) const
{
}

/*
===============
rvCameraPlayback::Restore
================
*/
void rvCameraPlayback::Restore( anRestoreGame *savefile )
{
	// Run spawn to set default values
	Spawn();
}

/*
=====================
rvCameraPlayback::Spawn
=====================
*/
void rvCameraPlayback::Spawn( void )
{
	startTime = gameLocal.time;
	playback = declManager->PlaybackByIndex( g_currentPlayback.GetInteger() );
}

/*
=====================
rvCameraPlayback::GetViewParms
=====================
*/
void rvCameraPlayback::GetViewParms( renderView_t *view )
{
	rvDeclPlaybackData	pbd;

	assert( view );
	if ( view )
	{
		pbd.Init();
		if ( declManager->GetPlaybackData( playback, PBFL_GET_POSITION | PBFL_GET_ANGLES_FROM_VEL, gameLocal.time - startTime, gameLocal.time - startTime, &pbd ) )
		{
			startTime = gameLocal.time;
		}

		view->vieworg = pbd.GetPosition();
		view->viewaxis = pbd.GetAngles().ToMat3();

		// field of view

// jshepard: fov as a float for smoove transitions
		gameLocal.CalcFov ( g_fov.GetFloat(), view->fov_x, view->fov_y );


	}
}


