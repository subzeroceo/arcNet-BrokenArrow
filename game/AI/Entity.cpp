#include "../idlib/Lib.h"
#pragma hdrstop

#include "Game_local.h"


// bdube: client effects
#include "client/ClientEffect.h"
//mcg: need to know team for AddDamageEffects
#include "ai/AI_Manager.h"


/*
===============================================================================

	anEntity

===============================================================================
*/

// overridable events
const anEventDef EV_PostSpawn( "<postspawn>", nullptr );
const anEventDef EV_FindTargets( "<findTargets>", nullptr );
const anEventDef EV_Touch( "<touch>", "et" );
const anEventDef EV_GetName( "getName", nullptr, 's' );
const anEventDef EV_SetName( "setName", "s" );
const anEventDef EV_Activate( "activate", "e" );
const anEventDef EV_ActivateTargets( "activateTargets", "e" );
const anEventDef EV_NumTargets( "numTargets", nullptr, 'f' );
const anEventDef EV_GetTarget( "getTarget", "f", 'e' );
const anEventDef EV_RandomTarget( "randomTarget", "s", 'e' );
const anEventDef EV_Bind( "bind", "e" );
const anEventDef EV_BindPosition( "bindPosition", "e" );
const anEventDef EV_BindToJoint( "bindToJoint", "esf" );
const anEventDef EV_Unbind( "unbind", nullptr );
const anEventDef EV_RemoveBinds( "removeBinds" );
const anEventDef EV_SpawnBind( "<spawnbind>", nullptr );
const anEventDef EV_SetOwner( "setOwner", "e" );
const anEventDef EV_SetModel( "setModel", "s" );
const anEventDef EV_SetSkin( "setSkin", "s" );
const anEventDef EV_GetWorldOrigin( "getWorldOrigin", nullptr, 'v' );
const anEventDef EV_SetWorldOrigin( "setWorldOrigin", "v" );
const anEventDef EV_GetOrigin( "getOrigin", nullptr, 'v' );
const anEventDef EV_SetOrigin( "setOrigin", "v" );
const anEventDef EV_GetAngles( "getAngles", nullptr, 'v' );
const anEventDef EV_SetAngles( "setAngles", "v" );
const anEventDef EV_GetLinearVelocity( "getLinearVelocity", nullptr, 'v' );
const anEventDef EV_SetLinearVelocity( "setLinearVelocity", "v" );
const anEventDef EV_GetAngularVelocity( "getAngularVelocity", nullptr, 'v' );
const anEventDef EV_SetAngularVelocity( "setAngularVelocity", "v" );
const anEventDef EV_GetSize( "getSize", nullptr, 'v' );
const anEventDef EV_SetSize( "setSize", "vv" );
const anEventDef EV_GetMins( "getMins", nullptr, 'v' );
const anEventDef EV_GetMaxs( "getMaxs", nullptr, 'v' );
const anEventDef EV_IsHidden( "isHidden", nullptr, 'd' );
const anEventDef EV_Hide( "hide", nullptr );
const anEventDef EV_Show( "show", nullptr );
const anEventDef EV_Touches( "touches", "E", 'd' );
const anEventDef EV_ClearSignal( "clearSignal", "d" );
const anEventDef EV_GetShaderParm( "getShaderParm", "d", 'f' );
const anEventDef EV_SetShaderParm( "setShaderParm", "df" );
const anEventDef EV_SetShaderParms( "setShaderParms", "ffff" );
const anEventDef EV_SetColor( "setColor", "fff" );
const anEventDef EV_GetColor( "getColor", nullptr, 'v' );
const anEventDef EV_CacheSoundShader( "cacheSoundShader", "s" );
const anEventDef EV_StartSoundShader( "startSoundShader", "sd", 'f' );
const anEventDef EV_StartSound( "startSound", "sdd", 'f' );
const anEventDef EV_StopSound( "stopSound", "dd" );
const anEventDef EV_FadeSound( "fadeSound", "dff" );
const anEventDef EV_SetGuiParm( "setGuiParm", "ss" );
const anEventDef EV_SetGuiFloat( "setGuiFloat", "sf" );
const anEventDef EV_GetNextKey( "getNextKey", "ss", 's' );
const anEventDef EV_SetKey( "setKey", "ss" );
const anEventDef EV_GetKey( "getKey", "s", 's' );
const anEventDef EV_GetIntKey( "getIntKey", "s", 'f' );
const anEventDef EV_GetFloatKey( "getFloatKey", "s", 'f' );
const anEventDef EV_GetVectorKey( "getVectorKey", "s", 'v' );
const anEventDef EV_GetEntityKey( "getEntityKey", "s", 'e' );
const anEventDef EV_RestorePosition( "restorePosition" );
const anEventDef EV_UpdateCameraTarget( "<updateCameraTarget>", nullptr );
const anEventDef EV_DistanceTo( "distanceTo", "E", 'f' );
const anEventDef EV_DistanceToPoint( "distanceToPoint", "v", 'f' );
const anEventDef EV_StartFx( "startFx", "s" );
const anEventDef EV_HasFunction( "hasFunction", "s", 'd' );
const anEventDef EV_CallFunction( "callFunction", "s" );
const anEventDef EV_SetNeverDormant( "setNeverDormant", "d" );


// bgeisler: go back to default skin
const anEventDef EV_ClearSkin( "clearSkin" );
// kfuller: added events
const anEventDef EV_SetContents( "setContents", "d" );
const anEventDef EV_GetLastBlocker( "getLastBlocker", nullptr, 'e' );
const anEventDef EV_Earthquake( "earthquake", "f" );
// we should probably try to integrate this with AI_PlayAnim
const anEventDef EV_PlayAnim( "playAnimNoChannel", "s" );
const anEventDef EV_PlayAnimXTimes( "playAnimXTimes", "sf" );
// bdube: effect events
const anEventDef EV_PlayEffect( "playEffect", "ssd" );
const anEventDef EV_StopEffect( "stopEffect", "s" );
const anEventDef EV_StopAllEffects( "stopAllEffects" );
const anEventDef EV_GetHealth ( "getHealth", nullptr, 'f' );
// bdube: surface related events
const anEventDef EV_HideSurface( "hideSurface", "s" );
const anEventDef EV_ShowSurface( "showSurface", "s" );
// bdube: added gui events
const anEventDef EV_GuiEvent ( "guiEvent", "s" );
// jscott: for playback button handling
const anEventDef EV_PlaybackCallback( "playbackCallback", "ddd" );
// nmckenzie:
const anEventDef EV_GetBindMaster( "getBindMaster", nullptr, 'e' );
const anEventDef EV_ApplyImpulse( "applyImpulse", "evv" );
// abahr:
const anEventDef EV_RemoveNullTargets( "removeNullTargets" );
const anEventDef EV_IsA( "isA", "s", 'f' );
const anEventDef EV_IsSameTypeAs( "isSameTypeAs", "e", 'f' );
const anEventDef EV_MatchPrefix( "matchPrefix", "ss", 's' );
const anEventDef EV_ClearTargetList( "clearTargetList", "f" );
// twhitaker:
const anEventDef EV_AppendTarget( "appendTarget", "E", 'f' );
const anEventDef EV_RemoveTarget( "removeTarget", "e" );
// mekberg:
const anEventDef EV_SetHealth( "setHealth", "f" );


ABSTRACT_DECLARATION( anClass, anEntity )
	EVENT( EV_GetName,				anEntity::Event_GetName )
	EVENT( EV_SetName,				anEntity::Event_SetName )
	EVENT( EV_FindTargets,			anEntity::Event_FindTargets )
	EVENT( EV_ActivateTargets,		anEntity::Event_ActivateTargets )
	EVENT( EV_NumTargets,			anEntity::Event_NumTargets )
	EVENT( EV_GetTarget,			anEntity::Event_GetTarget )
	EVENT( EV_RandomTarget,			anEntity::Event_RandomTarget )
	EVENT( EV_BindToJoint,			anEntity::Event_BindToJoint )
	EVENT( EV_RemoveBinds,			anEntity::Event_RemoveBinds )
	EVENT( EV_Bind,					anEntity::Event_Bind )
	EVENT( EV_BindPosition,			anEntity::Event_BindPosition )
	EVENT( EV_Unbind,				anEntity::Event_Unbind )
	EVENT( EV_SpawnBind,			anEntity::Event_SpawnBind )
	EVENT( EV_SetOwner,				anEntity::Event_SetOwner )
	EVENT( EV_SetModel,				anEntity::Event_SetModel )
	EVENT( EV_SetSkin,				anEntity::Event_SetSkin )
	EVENT( EV_GetShaderParm,		anEntity::Event_GetShaderParm )
	EVENT( EV_SetShaderParm,		anEntity::Event_SetShaderParm )
	EVENT( EV_SetShaderParms,		anEntity::Event_SetShaderParms )
	EVENT( EV_SetColor,				anEntity::Event_SetColor )
	EVENT( EV_GetColor,				anEntity::Event_GetColor )
	EVENT( EV_IsHidden,				anEntity::Event_IsHidden )
	EVENT( EV_Hide,					anEntity::Event_Hide )
	EVENT( EV_Show,					anEntity::Event_Show )
	EVENT( EV_CacheSoundShader,		anEntity::Event_CacheSoundShader )
	EVENT( EV_StartSoundShader,		anEntity::Event_StartSoundShader )
	EVENT( EV_StartSound,			anEntity::Event_StartSound )
	EVENT( EV_StopSound,			anEntity::Event_StopSound )
	EVENT( EV_FadeSound,			anEntity::Event_FadeSound )
	EVENT( EV_GetWorldOrigin,		anEntity::Event_GetWorldOrigin )
	EVENT( EV_SetWorldOrigin,		anEntity::Event_SetWorldOrigin )
	EVENT( EV_GetOrigin,			anEntity::Event_GetOrigin )
	EVENT( EV_SetOrigin,			anEntity::Event_SetOrigin )
	EVENT( EV_GetAngles,			anEntity::Event_GetAngles )
	EVENT( EV_SetAngles,			anEntity::Event_SetAngles )
	EVENT( EV_GetLinearVelocity,	anEntity::Event_GetLinearVelocity )
	EVENT( EV_SetLinearVelocity,	anEntity::Event_SetLinearVelocity )
	EVENT( EV_GetAngularVelocity,	anEntity::Event_GetAngularVelocity )
	EVENT( EV_SetAngularVelocity,	anEntity::Event_SetAngularVelocity )
	EVENT( EV_GetSize,				anEntity::Event_GetSize )
	EVENT( EV_SetSize,				anEntity::Event_SetSize )
	EVENT( EV_GetMins,				anEntity::Event_GetMins)
	EVENT( EV_GetMaxs,				anEntity::Event_GetMaxs )
	EVENT( EV_Touches,				anEntity::Event_Touches )
	EVENT( EV_SetGuiParm, 			anEntity::Event_SetGuiParm )
	EVENT( EV_SetGuiFloat, 			anEntity::Event_SetGuiFloat )
	EVENT( EV_GetNextKey,			anEntity::Event_GetNextKey )
	EVENT( EV_SetKey,				anEntity::Event_SetKey )
	EVENT( EV_GetKey,				anEntity::Event_GetKey )
	EVENT( EV_GetIntKey,			anEntity::Event_GetIntKey )
	EVENT( EV_GetFloatKey,			anEntity::Event_GetFloatKey )
	EVENT( EV_GetVectorKey,			anEntity::Event_GetVectorKey )
	EVENT( EV_GetEntityKey,			anEntity::Event_GetEntityKey )
	EVENT( EV_RestorePosition,		anEntity::Event_RestorePosition )
	EVENT( EV_UpdateCameraTarget,	anEntity::Event_UpdateCameraTarget )
	EVENT( EV_DistanceTo,			anEntity::Event_DistanceTo )
	EVENT( EV_DistanceToPoint,		anEntity::Event_DistanceToPoint )
	EVENT( EV_StartFx,				anEntity::Event_StartFx )
	EVENT( EV_Thread_WaitFrame,		anEntity::Event_WaitFrame )
	EVENT( EV_Thread_Wait,			anEntity::Event_Wait )
	EVENT( EV_HasFunction,			anEntity::Event_HasFunction )
	EVENT( EV_CallFunction,			anEntity::Event_CallFunction )
	EVENT( EV_SetNeverDormant,		anEntity::Event_SetNeverDormant )


// bgeisler: go back to default skin
	EVENT( EV_ClearSkin,			anEntity::Event_ClearSkin )
// kfuller: added events
	EVENT( EV_SetContents,			anEntity::Event_SetContents )
	EVENT( EV_GetLastBlocker,		anEntity::Event_GetLastBlocker)
// bdube: effect events
	EVENT( EV_PlayEffect,			anEntity::Event_PlayEffect )
	EVENT( EV_StopEffect,			anEntity::Event_StopEffect )
	EVENT( EV_StopAllEffects,		anEntity::Event_StopAllEffects )
	EVENT( EV_GetHealth,			anEntity::Event_GetHealth )
// bdube: mesh events
	EVENT( EV_HideSurface,			anEntity::Event_HideSurface )
	EVENT( EV_ShowSurface,			anEntity::Event_ShowSurface )
// bdube: gui events
	EVENT( EV_GuiEvent,				anEntity::Event_GuiEvent )
// jscott: playback callback
	EVENT( EV_PlaybackCallback,		anEntity::Event_PlaybackCallback )
// nmckenzie: Check who we're bound to.
	EVENT( EV_GetBindMaster,		anEntity::Event_GetBindMaster )
	EVENT( EV_ApplyImpulse,			anEntity::Event_ApplyImpulse )
// abahr: so we can call this from script
	EVENT( EV_RemoveNullTargets,	anEntity::Event_RemoveNullTargets )
	EVENT( EV_IsA,					anEntity::Event_IsA )
	EVENT( EV_IsSameTypeAs,			anEntity::Event_IsSameTypeAs )
	EVENT( EV_MatchPrefix,			anEntity::Event_MatchPrefix )
	EVENT( EV_ClearTargetList,		anEntity::Event_ClearTargetList )
// twhitaker: to dynamically add/remove targets in script
	EVENT( EV_AppendTarget,			anEntity::Event_AppendTarget )
	EVENT( EV_RemoveTarget,			anEntity::Event_RemoveTarget )
// mekberg: added
	EVENT( EV_SetHealth,			anEntity::Event_SetHealth )

END_CLASS

/*
================
UpdateGuiParms
================
*/
void UpdateGuiParms( anUserInterface *gui, const anDict *args ) {
	if ( gui == nullptr || args == nullptr ) {
		return;
	}
	const anKeyValue *kv = args->MatchPrefix( "gui_parm", nullptr );
	while( kv ) {
		gui->SetStateString( kv->GetKey(), common->GetLocalizedString( kv->GetValue() ) );
		kv = args->MatchPrefix( "gui_parm", kv );
	}
	gui->SetStateBool( "noninteractive",  args->GetBool( "gui_noninteractive" ) ) ;
	gui->StateChanged( gameLocal.time );
}

/*
================
AddRenderGui
================
*/
void AddRenderGui( const char *name, anUserInterface **gui, const anDict *args ) {

	const anKeyValue *kv = args->MatchPrefix( "gui_parm", nullptr );
	*gui = uiManager->FindGui( name, true, ( kv != nullptr ) || args->GetBool( "gui_noninteractive" ) );
	UpdateGuiParms( *gui, args );
}

/*
================
anGameEdit::ParseSpawnArgsToRenderEntity

parse the static model parameters
this is the canonical renderEntity parm parsing,
which should be used by dmap and the editor
================
*/
void anGameEdit::ParseSpawnArgsToRenderEntity( const anDict *args, renderEntity_t *renderEntity ) {
	int			i;
	const char	*temp;
	anVec3		color;
	float		angle;
	const idDeclModelDef *modelDef;

	memset( renderEntity, 0, sizeof( *renderEntity ) );

	temp = args->GetString( "model" );

	modelDef = nullptr;
	if ( temp[0] != '\0' ) {
		if ( !strstr( temp, "." ) ) {
			modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, temp, false ) );
			if ( modelDef ) {
				renderEntity->hModel = modelDef->ModelHandle();
				if ( renderEntity->hModel && !renderEntity->hModel->IsLoaded() ) {
					renderEntity->hModel->LoadModel();
				}
			}
		}

		if ( !renderEntity->hModel ) {
			renderEntity->hModel = renderModelManager->FindModel( temp );
		}
	}
	if ( renderEntity->hModel ) {
		renderEntity->bounds = renderEntity->hModel->Bounds( renderEntity );
	} else {
		renderEntity->bounds.Zero();
	}

	temp = args->GetString( "skin" );
	if ( temp[0] != '\0' ) {
		renderEntity->customSkin = declManager->FindSkin( temp );
	} else if ( modelDef ) {
		renderEntity->customSkin = modelDef->GetDefaultSkin();
	}

	temp = args->GetString( "shader" );
	if ( temp[0] != '\0' ) {
		renderEntity->customShader = declManager->FindMaterial( temp );
	}

	args->GetVector( "origin", "0 0 0", renderEntity->origin );

	// get the rotation matrix in either full form, or single angle form
	if ( !args->GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1", renderEntity->axis ) ) {
		angle = args->GetFloat( "angle" );

// abahr: allowing up and down buttons to affect orientation
		if ( angle == -1.0f ) {
			renderEntity->axis = anAngles( -90.0f, 0.0f, 0.0f ).ToMat3();
		} else if ( angle == -2.0f ) {
			renderEntity->axis = anAngles( 90.0f, 0.0f, 0.0f ).ToMat3();
		} else

		if ( angle != 0.0f ) {
			renderEntity->axis = anAngles( 0.0f, angle, 0.0f ).ToMat3();
		} else {
			renderEntity->axis.Identity();
		}
	}


	renderEntity->referenceSoundHandle = -1;


	// get shader parms
	args->GetVector( "_color", "1 1 1", color );
	renderEntity->shaderParms[ SHADERPARM_RED ]		= color[0];
	renderEntity->shaderParms[ SHADERPARM_GREEN ]	= color[1];
	renderEntity->shaderParms[ SHADERPARM_BLUE ]	= color[2];
	renderEntity->shaderParms[3]					= args->GetFloat( "shaderParm3", "1" );
	renderEntity->shaderParms[ 4 ]					= args->GetFloat( "shaderParm4", "0" );
	renderEntity->shaderParms[ 5 ]					= args->GetFloat( "shaderParm5", "0" );
	renderEntity->shaderParms[ 6 ]					= args->GetFloat( "shaderParm6", "0" );
	renderEntity->shaderParms[ 7 ]					= args->GetFloat( "shaderParm7", "0" );
	renderEntity->shaderParms[ 8 ]					= args->GetFloat( "shaderParm8", "0" );
	renderEntity->shaderParms[ 9 ]					= args->GetFloat( "shaderParm9", "0" );
	renderEntity->shaderParms[ 10 ]					= args->GetFloat( "shaderParm10", "0" );
	renderEntity->shaderParms[ 11 ]					= args->GetFloat( "shaderParm11", "0" );

	// check noDynamicInteractions flag
	renderEntity->noDynamicInteractions = args->GetBool( "noDynamicInteractions" );

	// check noshadows flag
	renderEntity->noShadow = args->GetBool( "noshadows" );

	// check noselfshadows flag
	renderEntity->noSelfShadow = args->GetBool( "noselfshadows" );

	// init any guis, including entity-specific states
	for ( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		temp = args->GetString( i == 0 ? "gui" : va( "gui%d", i + 1 ) );
		if ( temp[0] != '\0' ) {
			AddRenderGui( temp, &renderEntity->gui[i], args );
		}
	}
}

/*
================
anGameEdit::ParseSpawnArgsToRefSound

parse the sound parameters
this is the canonical refSound parm parsing,
which should be used by dmap and the editor
================
*/
void anGameEdit::ParseSpawnArgsToRefSound( const anDict *args, refSound_t *refSound ) {
	const char	*temp;

	memset( refSound, 0, sizeof( *refSound ) );
	refSound->referenceSoundHandle = -1;


	refSound->parms.minDistance = args->GetFloat( "s_mindistance" );
	refSound->parms.maxDistance = args->GetFloat( "s_maxdistance" );
	// WARNING: This overrides the volume; it does not modify it
	if ( args->GetFloat( "s_volume" ) != 0.0f ) {
		refSound->parms.volume = anMath::dBToScale( args->GetFloat( "s_volume" ) );
	}

	if ( refSound->parms.volume < 0.0f || refSound->parms.volume > 5.0f ) {
		common->Warning( "Unreasonable volume (%g) on entity \'%s\'", refSound->parms.volume, args->GetString( "name" ) );
		refSound->parms.volume = 5.0f;
	}

	refSound->parms.shakes = args->GetFloat( "s_shakes" );

	args->GetVector( "origin", "0 0 0", refSound->origin );

	// if a diversity is not specified, every sound start will make
	// a random one.  Specifying diversity is usefull to make multiple
	// lights all share the same buzz sound offset, for instance.
	refSound->diversity = args->GetFloat( "s_diversity", "-1" );
	refSound->waitfortrigger = args->GetBool( "s_waitfortrigger" );

	if ( args->GetBool( "s_omni" ) ) {
		refSound->parms.soundShaderFlags |= SSF_OMNIDIRECTIONAL;
	}
	if ( args->GetBool( "s_looping" ) ) {
		refSound->parms.soundShaderFlags |= SSF_LOOPING;
	}
	if ( args->GetBool( "s_occlusion" ) ) {
		refSound->parms.soundShaderFlags |= SSF_NO_OCCLUSION;
	}
	if ( args->GetBool( "s_global" ) ) {
		refSound->parms.soundShaderFlags |= SSF_GLOBAL;
	}
	if ( args->GetBool( "s_unclamped" ) ) {
		refSound->parms.soundShaderFlags |= SSF_UNCLAMPED;
	}
	if ( args->GetBool( "s_center" ) ) {
		refSound->parms.soundShaderFlags |= SSF_CENTER;
	}

	refSound->parms.soundClass = args->GetInt( "s_soundClass" );

	temp = args->GetString( "s_shader" );
	if ( temp[0] != '\0' ) {
		refSound->shader = declManager->FindSound( temp );
	}


	if ( refSound->parms.maxDistance < refSound->parms.minDistance ) {
		common->Warning( "ParseSpawnArgsToRefSound: Max distance less than min distance for entity \'%s\'", args->GetString( "name", "*unknown*" ) );
	}

}

/*
===============
anEntity::UpdateChangeableSpawnArgs

Any key val pair that might change during the course of the game ( via a gui or whatever )
should be initialize here so a gui or other trigger can change something and have it updated
properly. An optional source may be provided if the values reside in an outside dictionary and
first need copied over to spawnArgs
===============
*/
void anEntity::UpdateChangeableSpawnArgs( const anDict *source ) {
	int i;
	const char *target;

	if ( !source ) {
		source = &spawnArgs;
	}
	cameraTarget = nullptr;
	target = source->GetString( "cameraTarget" );
	if ( target && target[0] ) {

// bdube: EV_UpdateCameraTarget pulls from spawnargs so we need to move the target over
		spawnArgs.Set ( "cameraTarget", target );

		// update the camera taget
		PostEventMS( &EV_UpdateCameraTarget, 0 );
	}

	for ( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		UpdateGuiParms( renderEntity.gui[i], source );
	}
}

/*
================
anEntity::anEntity
================
*/
anEntity::anEntity() {

	entityNumber	= ENTITYNUM_NONE;
	entityDefNumber = -1;

	spawnNode.SetOwner( this );
	activeNode.SetOwner( this );

	snapshotNode.SetOwner( this );
	snapshotSequence = -1;
	snapshotBits = 0;

	thinkFlags		= 0;
	dormantStart	= 0;
	cinematic		= false;
	renderView		= nullptr;
	cameraTarget	= nullptr;
	health			= 0;

	physics			= nullptr;
	bindMaster		= nullptr;
	bindJoint		= INVALID_JOINT;
	bindBody		= -1;
	teamMaster		= nullptr;
	teamChain		= nullptr;
	signals			= nullptr;

	memset( PVSAreas, 0, sizeof( PVSAreas ) );
	numPVSAreas		= -1;

	memset( &fl, 0, sizeof( fl ) );
	fl.neverDormant	= true;			// most entities never go dormant

	memset( &renderEntity, 0, sizeof( renderEntity ) );
	modelDefHandle	= -1;
	memset( &refSound, 0, sizeof( refSound ) );
	refSound.referenceSoundHandle = -1;

	mpGUIState = -1;


// rjohnson: added this to persist long thinking entities
	mLastLongThinkTime = 0;
	mLastLongThinkColor.Zero();
// ddynerman: instance, clipworld
	SetInstance( 0 );
	SetClipWorld( 0 );
	fl.persistAcrossInstances = false;
// twhitaker
	forwardDamageEnt = nullptr;
// ddynerman: optional preprediction
	predictTime = 0;

}

/*
================
anEntity::Spawn
================
*/
void anEntity::Spawn( void ) {
	int					i;
	const char			*temp;
	anVec3				origin;
	anMat3				axis;
	const anKeyValue	*networkSync;
	const char			*classname;
	const char			*scriptObjectName;

	gameLocal.RegisterEntity( this );

// bdube: make sure there is a classname before trying to use it
	if ( spawnArgs.GetString( "classname", nullptr, &classname ) ) {
		const anDeclEntityDef *def = gameLocal.FindEntityDef( classname, false );
		if ( def ) {
			entityDefNumber = def->Index();
		}
	}

	// Persona is a set of keys that augment an entity giving it its own custom persona
	const anDict* dict;
	dict = gameLocal.FindEntityDefDict ( spawnArgs.GetString ( "def_persona", "" ), false );
	if ( dict ) {
		spawnArgs.Copy ( *dict );
	}


	// parse static models the same way the editor display does
	gameEdit->ParseSpawnArgsToRenderEntity( &spawnArgs, &renderEntity );


// bdube: added hidesurface
	const anKeyValue* kv;
	for ( kv = spawnArgs.MatchPrefix ( "hidesurface", nullptr );
		  kv;
		  kv = spawnArgs.MatchPrefix ( "hidesurface", kv ) ) {
		HideSurface ( kv->GetValue() );
	}


	renderEntity.entityNum = entityNumber;


// ddynerman: LOD code
	renderEntity.shadowLODDistance = spawnArgs.GetFloat( "shadow_lod_distance", "768.0" );
	renderEntity.shadowLODDistance *= renderEntity.shadowLODDistance;

	int spawnInstance = spawnArgs.GetInt( "instance" );
	SetInstance( spawnInstance );


	// go dormant within 5 frames so that when the map starts most monsters are dormant
	dormantStart = gameLocal.time - DELAY_DORMANT_TIME + gameLocal.msec * 5;

	origin = renderEntity.origin;
	axis = renderEntity.axis;

	// do the audio parsing the same way dmap and the editor do
	gameEdit->ParseSpawnArgsToRefSound( &spawnArgs, &refSound );

	// only play SCHANNEL_PRIVATE when sndworld->PlaceListener() is called with this listenerId
	// don't spatialize sounds from the same entity
	refSound.listenerId = entityNumber + 1;

	cameraTarget = nullptr;
	temp = spawnArgs.GetString( "cameraTarget" );
	if ( temp && temp[0] ) {
		// update the camera taget
		PostEventMS( &EV_UpdateCameraTarget, 0 );
	}

	for ( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		UpdateGuiParms( renderEntity.gui[i], &spawnArgs );
	}

	fl.solidForTeam = spawnArgs.GetBool( "solidForTeam", "0" );

// bdube: usable
	fl.usable = spawnArgs.GetBool ( "usable", "0" );


	fl.neverDormant = spawnArgs.GetBool( "neverDormant", "0" );
	fl.hidden = spawnArgs.GetBool( "hide", "0" );
	if ( fl.hidden ) {
		// make sure we're hidden, since a spawn function might not set it up right
		PostEventMS( &EV_Hide, 0 );
	}
	cinematic = spawnArgs.GetBool( "cinematic", "0" );

	networkSync = spawnArgs.FindKey( "networkSync" );
	if ( networkSync ) {
		fl.networkSync = ( atoi( networkSync->GetValue() ) != 0 );
	}

	// every object will have a unique name
	temp = spawnArgs.GetString( "name", va( "%s_%s_%d", GetClassname(), spawnArgs.GetString( "classname" ), entityNumber ) );
	SetName( temp );

	// if we have targets, wait until all entities are spawned to get them
	if ( spawnArgs.MatchPrefix( "target" ) || spawnArgs.MatchPrefix( "guiTarget" ) ) {
		if ( gameLocal.GameState() == GAMESTATE_STARTUP ) {
			PostEventMS( &EV_FindTargets, 0 );
		} else {
			// not during spawn, so it's ok to get the targets
			FindTargets();
		}
	}

	health = spawnArgs.GetInt( "health" );

	InitDefaultPhysics( origin, axis );

	SetOrigin( origin );
	SetAxis( axis );

	temp = spawnArgs.GetString( "model" );
	if ( temp && *temp ) {
		SetModel( temp );
	}

	if ( spawnArgs.GetString( "bind", "", &temp ) ) {
		PostEventMS( &EV_SpawnBind, 0 );
	}

	// auto-start a sound on the entity
	if ( refSound.shader && !refSound.waitfortrigger ) {
		StartSoundShader( refSound.shader, SND_CHANNEL_ANY, 0, false, nullptr );
	}

	// setup script object
	if ( ShouldConstructScriptObjectAtSpawn() && spawnArgs.GetString( "scriptobject", nullptr, &scriptObjectName ) ) {
		if ( !scriptObject.SetType( scriptObjectName ) ) {
			gameLocal.Error( "Script object '%s' not found on entity '%s'.", scriptObjectName, name.c_str() );
		}

		ConstructScriptObject();
	}


	fl.persistAcrossInstances = false;
// bgeisler: added
	fl.triggerAnim = spawnArgs.GetBool( "trigger_anim" );

	// precache decls
	declManager->FindType( DECL_ENTITYDEF, "damage_crush", false, false );

}

/*
================
anEntity::~anEntity
================
*/
anEntity::~anEntity( void ) {
	DeconstructScriptObject();
	scriptObject.Free();

	if ( thinkFlags ) {
		BecomeInactive( thinkFlags );
	}
	activeNode.Remove();

	Signal( SIG_REMOVED );

	// we have to set back the default physics object before unbinding because the entity
	// specific physics object might be an entity variable and as such could already be destroyed.
	SetPhysics( nullptr );

	// remove any entities that are bound to me
	RemoveBinds();

	// unbind from master
	Unbind();
	QuitTeam();

	gameLocal.RemoveEntityFromHash( name.c_str(), this );

	delete renderView;
	renderView = nullptr;

	delete signals;
	signals = nullptr;


// bdube: make sure all sounds and attached effects are stopped
	StopSound( SCHANNEL_ANY, false );

	RemoveClientEntities();


	FreeModelDef();
	FreeSoundEmitter( false );

	gameLocal.UnregisterEntity( this );
}

/*
================
anEntity::Save
================
*/
void anEntity::Save( anSaveGame *savefile ) const {
	int				i, j;
	rvClientEntity* cent;

	savefile->WriteInt( entityNumber );
	savefile->WriteInt( entityDefNumber );

	// spawnNode and activeNode are restored by gameLocal

	// anLinkList<anEntity>	snapshotNode;

	savefile->WriteInt( snapshotSequence );
	savefile->WriteInt( snapshotBits );

	savefile->WriteString( name );
	savefile->WriteDict( &spawnArgs );
	scriptObject.Save( savefile );

	savefile->WriteInt( thinkFlags );
	savefile->WriteInt( dormantStart );
	savefile->WriteBool( cinematic );

	// renderView_t * renderView;

	savefile->WriteObject( cameraTarget );

	savefile->WriteInt( targets.Num() );
	for ( i = 0; i < targets.Num(); i++ ) {
		targets[i].Save( savefile );
	}

	savefile->WriteInt( health );

	savefile->WriteInt( clientEntities.Num() );
	for ( cent = clientEntities.Next(); cent; cent = cent->bindNode.Next() ) {
		savefile->WriteObject( cent );
	}

//	savefile->WriteInt( mLastLongThinkTime );			// Debug vars - don't save
//	savefile->WriteVec4( mLastLongThinkColor );			// Debug vars - don't save

	savefile->Write( &fl, sizeof( fl ) );

	savefile->WriteRenderEntity( renderEntity );
	savefile->WriteInt( modelDefHandle );
	savefile->WriteRefSound( refSound );


// mekberg: proper save
	forwardDamageEnt.Save ( savefile );


	savefile->WriteStaticObject( defaultPhysicsObj );

	savefile->WriteObject( bindMaster.GetEntity() );
	savefile->WriteJoint( bindJoint );
	savefile->WriteInt( bindBody );
	savefile->WriteObject( teamMaster );
	savefile->WriteObject( teamChain );

	savefile->WriteInt( numPVSAreas );
	for ( i = 0; i < MAX_PVS_AREAS; i++ ) {
		savefile->WriteInt( PVSAreas[i] );
	}

	if ( !signals ) {
		savefile->WriteBool( false );
	} else {
		savefile->WriteBool( true );
		for ( i = 0; i < NUM_SIGNALS; i++ ) {
			savefile->WriteInt( signals->signal[i].Num() );
			for ( j = 0; j < signals->signal[i].Num(); j++ ) {
				savefile->WriteInt( signals->signal[i][ j ].threadnum );
				savefile->WriteString( signals->signal[i][ j ].function->Name() );
			}
		}
	}

	savefile->WriteInt( mpGUIState );

	savefile->WriteInt( instance );
	savefile->WriteInt( clipWorld );
}

/*
================
anEntity::Restore
================
*/
void anEntity::Restore( anRestoreGame *savefile ) {
	int				i, j;
	int				num;
	rvClientEntity	*temp;
	anStr			funcname;

	savefile->ReadInt( entityNumber );
	savefile->ReadInt( entityDefNumber );

	// spawnNode and activeNode are restored by gameLocal

	// anLinkList<anEntity>	snapshotNode;

	savefile->ReadInt( snapshotSequence );
	savefile->ReadInt( snapshotBits );

	savefile->ReadString( name );
	SetName( name );
	savefile->ReadDict( &spawnArgs );

	scriptObject.Restore( savefile );

	savefile->ReadInt( thinkFlags );
	savefile->ReadInt( dormantStart );
	savefile->ReadBool( cinematic );

	// renderView_t *			renderView;

	savefile->ReadObject( reinterpret_cast<anClass *&>( cameraTarget ) );

	targets.Clear();
	savefile->ReadInt( num );
	targets.SetNum( num );
	for ( i = 0; i < num; i++ ) {
		targets[i].Restore( savefile );
	}

	savefile->ReadInt( health );

	savefile->ReadInt( num );
	for ( i = 0; i < num; i++ ) {
		savefile->ReadObject( reinterpret_cast<anClass *&>( temp ) );
		if ( temp ) {
			temp->bindNode.AddToEnd( clientEntities );
		}
	}

//	savefile->ReadInt( mLastLongThinkTime );			// Debug vars - don't save
//	savefile->ReadVec4( mLastLongThinkColor );			// Debug vars - don't save

	savefile->Read( &fl, sizeof( fl ) );


	savefile->ReadRenderEntity( renderEntity, &spawnArgs );

	savefile->ReadInt( modelDefHandle );
	savefile->ReadRefSound( refSound );


// mekberg: proper restore
	forwardDamageEnt.Restore ( savefile );


	savefile->ReadStaticObject( defaultPhysicsObj );
	RestorePhysics( &defaultPhysicsObj );

	anEntity *templol = 0;
	savefile->ReadObject( reinterpret_cast<anClass *&>( templol ) );
	bindMaster = templol;

	savefile->ReadJoint( bindJoint );
	savefile->ReadInt( bindBody );
	savefile->ReadObject( reinterpret_cast<anClass *&>( teamMaster ) );
	savefile->ReadObject( reinterpret_cast<anClass *&>( teamChain ) );

	savefile->ReadInt( numPVSAreas );
	for ( i = 0; i < MAX_PVS_AREAS; i++ ) {
		savefile->ReadInt( PVSAreas[i] );
	}

	bool readsignals;
	savefile->ReadBool( readsignals );
	if ( readsignals ) {
		signals = new signalList_t;
		for ( i = 0; i < NUM_SIGNALS; i++ ) {
			savefile->ReadInt( num );
			signals->signal[i].SetNum( num );
			for ( j = 0; j < num; j++ ) {
				savefile->ReadInt( signals->signal[i][ j ].threadnum );
				savefile->ReadString( funcname );
				signals->signal[i][ j ].function = gameLocal.program.FindFunction( funcname );
				if ( !signals->signal[i][ j ].function ) {
					savefile->Error( "Function '%s' not found", funcname.c_str() );
				}
			}
		}
	}

	savefile->ReadInt( mpGUIState );

	// restore must retrieve modelDefHandle from the renderer
	if ( modelDefHandle != -1 ) {
		modelDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	}

	savefile->ReadInt( instance );
	savefile->ReadInt( clipWorld );

	// precache decls
	declManager->FindType( DECL_ENTITYDEF, "damage_crush", false, false );
}

/*
================
anEntity::GetEntityDefName
================
*/
const char *anEntity::GetEntityDefName( void ) const {
	if ( entityDefNumber < 0 ) {
		return "*unknown*";
	}
	return declManager->DeclByIndex( DECL_ENTITYDEF, entityDefNumber, false )->GetName();
}

/*
================
anEntity::SetName
================
*/
void anEntity::SetName( const char *newname ) {
	if ( name.Length() ) {
		gameLocal.RemoveEntityFromHash( name.c_str(), this );
		gameLocal.program.SetEntity( name, nullptr );
	}

	name = newname;
	if ( name.Length() ) {
		if ( ( name == "nullptr" ) || ( name == "null_entity" ) ) {
			gameLocal.Error( "Cannot name entity '%s'.  '%s' is reserved for script.", name.c_str(), name.c_str() );
		}
		gameLocal.AddEntityToHash( name.c_str(), this );
		gameLocal.program.SetEntity( name, this );
	}
}

/*
================
anEntity::GetName
================
*/
const char *anEntity::GetName( void ) const {
	return name.c_str();
}


/***********************************************************************

	Thinking

***********************************************************************/

/*
================
anEntity::Think
================
*/
void anEntity::Think( void ) {
	RunPhysics();
	Present();
}

/*
================
anEntity::DoDormantTests

Monsters and other expensive entities that are completely closed
off from the player can skip all of their work
================
*/
bool anEntity::DoDormantTests( void ) {
	// Never go dormant?
	if ( fl.neverDormant || (gameLocal.inCinematic && cinematic) ) {
		return false;
	}

	// if the monster area is not topologically connected to a player
	if ( !gameLocal.InPlayerConnectedArea( this ) ) {
		return true;
	} else {
		// the monster area is topologically connected to a player, but if
		// the monster hasn't been woken up before, do the more precise PVS check
		if ( !fl.hasAwakened ) {
			if ( !gameLocal.InPlayerPVS( this ) ) {
				return true;
			}
		}
	}

	return false;
}

/*
================
anEntity::CheckDormant

Monsters and other expensive entities that are completely closed
off from the player can skip all of their work
================
*/
bool anEntity::CheckDormant( void ) {
	bool dormant;

	dormant = DoDormantTests();
	if ( dormant ) {
		if ( dormantStart == 0 ) {
			dormantStart = gameLocal.time;
		}
		if ( gameLocal.time - dormantStart < DELAY_DORMANT_TIME ) {
			dormant = false;
		}
	} else {
		dormantStart = 0;
		fl.hasAwakened = true;
	}

	if ( dormant && !fl.isDormant ) {
		fl.isDormant = true;
		DormantBegin();
	} else if ( !dormant && fl.isDormant ) {
		fl.isDormant = false;
		DormantEnd();
	}

	return dormant;
}

/*
================
anEntity::DormantBegin

called when entity becomes dormant
================
*/
void anEntity::DormantBegin( void ) {
}

/*
================
anEntity::DormantEnd

called when entity wakes from being dormant
================
*/
void anEntity::DormantEnd( void ) {
}

/*
================
anEntity::IsActive
================
*/
bool anEntity::IsActive( void ) const {
	return activeNode.InList();
}

/*
================
anEntity::BecomeActive
================
*/
void anEntity::BecomeActive( int flags ) {
	if ( ( flags & TH_PHYSICS ) ) {
		// enable the team master if this entity is part of a physics team
		if ( teamMaster && teamMaster != this ) {
			teamMaster->BecomeActive( TH_PHYSICS );
		} else if ( !( thinkFlags & TH_PHYSICS ) ) {
			// if this is a pusher


			if ( physics->IsType( anPhysics_Parametric::GetClassType() ) || physics->IsType( anPhysics_Actor::GetClassType() ) ) {

				gameLocal.sortPushers = true;
			}

// abahr:

			if ( physics->IsType( anPhysics_Spline::GetClassType() ) ) {
				gameLocal.sortPushers = true;
			}

		}
	}

	int oldFlags = thinkFlags;
	thinkFlags |= flags;
	if ( thinkFlags ) {
		if ( !IsActive() ) {
			activeNode.AddToEnd( gameLocal.activeEntities );
		} else if ( !oldFlags ) {
			// we became inactive this frame, so we have to decrease the count of entities to deactivate
			gameLocal.numEntitiesToDeactivate--;
		}
	}
}

/*
================
anEntity::BecomeInactive
================
*/
void anEntity::BecomeInactive( int flags ) {
	if ( ( flags & TH_PHYSICS ) ) {
		// may only disable physics on a team master if no team members are running physics or bound to a joints
		if ( teamMaster == this ) {
			for ( anEntity *ent = teamMaster->teamChain; ent; ent = ent->teamChain ) {
				if ( ( ent->thinkFlags & TH_PHYSICS ) || ( ( ent->bindMaster == this ) && ( ent->bindJoint != INVALID_JOINT ) ) ) {
					flags &= ~TH_PHYSICS;
					break;
				}
			}
		}
	}

	if ( thinkFlags ) {
		thinkFlags &= ~flags;
		if ( !thinkFlags && IsActive() ) {
			gameLocal.numEntitiesToDeactivate++;
		}
	}

	if ( ( flags & TH_PHYSICS ) ) {
		// if this entity has a team master
		if ( teamMaster && teamMaster != this ) {
			// if the team master is at rest
			if ( teamMaster->IsAtRest() ) {
				teamMaster->BecomeInactive( TH_PHYSICS );
			}
		}
	}
}

/***********************************************************************

	Visuals

***********************************************************************/

/*
================
anEntity::SetShaderParm
================
*/
void anEntity::SetShaderParm( int parmnum, float value ) {
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) ) {
		gameLocal.Warning( "shader parm index (%d) out of range", parmnum );
		return;
	}

	renderEntity.shaderParms[ parmnum ] = value;
	UpdateVisuals();
}

/*
================
anEntity::SetColor
================
*/
void anEntity::SetColor( float red, float green, float blue ) {
	renderEntity.shaderParms[ SHADERPARM_RED ]		= red;
	renderEntity.shaderParms[ SHADERPARM_GREEN ]	= green;
	renderEntity.shaderParms[ SHADERPARM_BLUE ]		= blue;
	UpdateVisuals();
}

/*
================
anEntity::SetColor
================
*/
void anEntity::SetColor( const anVec3 &color ) {
	SetColor( color[0], color[1], color[2] );
	UpdateVisuals();
}

/*
================
anEntity::GetColor
================
*/
void anEntity::GetColor( anVec3 &out ) const {
	out[0] = renderEntity.shaderParms[ SHADERPARM_RED ];
	out[1] = renderEntity.shaderParms[ SHADERPARM_GREEN ];
	out[2] = renderEntity.shaderParms[ SHADERPARM_BLUE ];
}

/*
================
anEntity::SetColor
================
*/
void anEntity::SetColor( const anVec4 &color ) {
	renderEntity.shaderParms[ SHADERPARM_RED ]		= color[0];
	renderEntity.shaderParms[ SHADERPARM_GREEN ]	= color[1];
	renderEntity.shaderParms[ SHADERPARM_BLUE ]		= color[2];
	renderEntity.shaderParms[ SHADERPARM_ALPHA ]	= color[3];
	UpdateVisuals();
}

/*
================
anEntity::GetColor
================
*/
void anEntity::GetColor( anVec4 &out ) const {
	out[0] = renderEntity.shaderParms[ SHADERPARM_RED ];
	out[1] = renderEntity.shaderParms[ SHADERPARM_GREEN ];
	out[2] = renderEntity.shaderParms[ SHADERPARM_BLUE ];
	out[3] = renderEntity.shaderParms[ SHADERPARM_ALPHA ];
}

/*
================
anEntity::UpdateAnimationControllers
================
*/
bool anEntity::UpdateAnimationControllers( void ) {
	// any ragdoll and IK animation controllers should be updated here
	return false;
}

/*
================
anEntity::SetModel
================
*/
void anEntity::SetModel( const char *modelname ) {
	assert( modelname );

	FreeModelDef();

	renderEntity.hModel = renderModelManager->FindModel( modelname );

	if ( renderEntity.hModel ) {
		renderEntity.hModel->Reset();
	}

	renderEntity.callback = nullptr;
	renderEntity.numJoints = 0;
	renderEntity.joints = nullptr;
	if ( renderEntity.hModel ) {
		renderEntity.bounds = renderEntity.hModel->Bounds( &renderEntity );
	} else {
		renderEntity.bounds.Zero();
	}

	UpdateVisuals();
}

/*
================
anEntity::SetSkin
================
*/
void anEntity::SetSkin( const idDeclSkin *skin ) {
	renderEntity.customSkin = skin;
	UpdateVisuals();
}

// bgeisler: go back to default skin
/*
================
anEntity::ClearSkin
================
*/
void anEntity::ClearSkin( void )
{
	if ( GetAnimator() && GetAnimator()->ModelDef() ) {
		renderEntity.customSkin = GetAnimator()->ModelDef()->GetDefaultSkin();
	} else  {
		renderEntity.customSkin = nullptr;
	}

	UpdateVisuals();
}


/*
================
anEntity::GetSkin
================
*/
const idDeclSkin *anEntity::GetSkin( void ) const {
	return renderEntity.customSkin;
}

/*
================
anEntity::FreeModelDef
================
*/
void anEntity::FreeModelDef( void ) {
	if ( modelDefHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( modelDefHandle );
		modelDefHandle = -1;

		rvClientEntity* cent;

		for ( cent = clientEntities.Next(); cent != nullptr; cent = cent->bindNode.Next() ) {
			cent->FreeEntityDef();
		}
	}
}

/*
================
anEntity::FreeLightDef
================
*/
void anEntity::FreeLightDef( void ) {
}

/*
================
anEntity::IsHidden
================
*/
bool anEntity::IsHidden( void ) const {
	return fl.hidden;
}

/*
================
anEntity::Hide
================
*/
void anEntity::Hide( void ) {
	if ( !IsHidden() ) {
		fl.hidden = true;
		FreeModelDef();
		UpdateVisuals();
	}
}

/*
================
anEntity::Show
================
*/
void anEntity::Show( void ) {
	if ( IsHidden() ) {
		fl.hidden = false;
		UpdateVisuals();
	}
}

/*
================
anEntity::UpdateModelTransform
================
*/
void anEntity::UpdateModelTransform( void ) {
	anVec3 origin;
	anMat3 axis;

	if ( GetPhysicsToVisualTransform( origin, axis ) ) {
		renderEntity.axis = axis * GetPhysics()->GetAxis();
		renderEntity.origin = GetPhysics()->GetOrigin() + origin * renderEntity.axis;
	} else {
		renderEntity.axis = GetPhysics()->GetAxis();
		renderEntity.origin = GetPhysics()->GetOrigin();
	}
}

/*
================
anEntity::UpdateModel
================
*/
void anEntity::UpdateModel( void ) {
	UpdateModelTransform();


// abahr: moved GetAnimator call because its invalid when called from a destructor
	UpdateRenderEntityCallback();


	// set to invalid number to force an update the next time the PVS areas are retrieved
	ClearPVSAreas();

	// ensure that we call Present this frame
	BecomeActive( TH_UPDATEVISUALS );
}


// abahr:
/*
================
anEntity::UpdateRenderEntityCallback
================
*/
void anEntity::UpdateRenderEntityCallback() {
}


/*
================
anEntity::UpdateVisuals
================
*/
void anEntity::UpdateVisuals( void ) {
	UpdateModel();
	UpdateSound();
}

/*
================
anEntity::UpdatePVSAreas
================
*/
void anEntity::UpdatePVSAreas( void ) {
	int localNumPVSAreas, localPVSAreas[32];
	anBounds modelAbsBounds;
	int i;

	modelAbsBounds.FromTransformedBounds( renderEntity.bounds, renderEntity.origin, renderEntity.axis );
	localNumPVSAreas = gameLocal.pvs.GetPVSAreas( modelAbsBounds, localPVSAreas, sizeof( localPVSAreas ) / sizeof( localPVSAreas[0] ) );

	// FIXME: some particle systems may have huge bounds and end up in many PVS areas
	// the first MAX_PVS_AREAS may not be visible to a network client and as a result the particle system may not show up when it should
	if ( localNumPVSAreas > MAX_PVS_AREAS ) {
		localNumPVSAreas = gameLocal.pvs.GetPVSAreas( anBounds( renderEntity.origin ).Expand( 64.0f ), localPVSAreas, sizeof( localPVSAreas ) / sizeof( localPVSAreas[0] ) );
	}

	for ( numPVSAreas = 0; numPVSAreas < MAX_PVS_AREAS && numPVSAreas < localNumPVSAreas; numPVSAreas++ ) {
		PVSAreas[numPVSAreas] = localPVSAreas[numPVSAreas];
	}

	for ( i = numPVSAreas; i < MAX_PVS_AREAS; i++ ) {
		PVSAreas[i] = 0;
	}
}

/*
================
anEntity::UpdatePVSAreas
================
*/
void anEntity::UpdatePVSAreas( const anVec3 &pos ) {
	int i;

	numPVSAreas = gameLocal.pvs.GetPVSAreas( anBounds( pos ), PVSAreas, MAX_PVS_AREAS );
	i = numPVSAreas;
	while ( i < MAX_PVS_AREAS ) {
		PVSAreas[ i++ ] = 0;
	}
}

/*
================
anEntity::GetNumPVSAreas
================
*/
int anEntity::GetNumPVSAreas( void ) {
	if ( numPVSAreas < 0 ) {
		UpdatePVSAreas();
	}
	return numPVSAreas;
}

/*
================
anEntity::GetPVSAreas
================
*/
const int *anEntity::GetPVSAreas( void ) {
	if ( numPVSAreas < 0 ) {
		UpdatePVSAreas();
	}
	return PVSAreas;
}

/*
================
anEntity::ClearPVSAreas
================
*/
void anEntity::ClearPVSAreas( void ) {
	numPVSAreas = -1;
}

/*
================
anEntity::PhysicsTeamInPVS

  FIXME: for networking also return true if any of the entity shadows is in the PVS
================
*/
bool anEntity::PhysicsTeamInPVS( pvsHandle_t pvsHandle ) {
	anEntity *part;

	if ( teamMaster ) {
		for ( part = teamMaster; part; part = part->teamChain ) {
			if ( gameLocal.pvs.InCurrentPVS( pvsHandle, part->GetPVSAreas(), part->GetNumPVSAreas() ) ) {
				return true;
			}
		}
	} else {
		return gameLocal.pvs.InCurrentPVS( pvsHandle, GetPVSAreas(), GetNumPVSAreas() );
	}
	return false;
}

/*
==============
anEntity::ProjectOverlay
==============
*/
void anEntity::ProjectOverlay( const anVec3 &origin, const anVec3 &dir, float size, const char *material ) {
	float s, c;
	anMat3 axis, axistemp;
	anVec3 localOrigin, localAxis[2];
	anPlane localPlane[2];

	// make sure the entity has a valid model handle
	if ( modelDefHandle < 0 ) {
		return;
	}

	// only do this on dynamic md5 models
	if ( renderEntity.hModel->IsDynamicModel() != DM_CACHED ) {
		return;
	}

	anMath::SinCos16( gameLocal.random.RandomFloat() * anMath::TWO_PI, s, c );

	axis[2] = -dir;
	axis[2].NormalVectors( axistemp[0], axistemp[1] );
	axis[0] = axistemp[0] * c + axistemp[1] * -s;
	axis[1] = axistemp[0] * -s + axistemp[1] * -c;

	renderEntity.axis.ProjectVector( origin - renderEntity.origin, localOrigin );
	renderEntity.axis.ProjectVector( axis[0], localAxis[0] );
	renderEntity.axis.ProjectVector( axis[1], localAxis[1] );

	size = 1.0f / size;
	localAxis[0] *= size;
	localAxis[1] *= size;

	localPlane[0] = localAxis[0];
	localPlane[0][3] = -( localOrigin * localAxis[0] ) + 0.5f;

	localPlane[1] = localAxis[1];
	localPlane[1][3] = -( localOrigin * localAxis[1] ) + 0.5f;

	const anMaterial *mtr = declManager->FindMaterial( material );

	// project an overlay onto the model
	gameRenderWorld->ProjectOverlay( modelDefHandle, localPlane, mtr );

	// make sure non-animating models update their overlay
	UpdateVisuals();
}

/*
================
anEntity::Present

Present is called to allow entities to generate refEntities, lights, etc for the renderer.
================
*/
void anEntity::Present( void ) {

	if ( !gameLocal.isNewFrame ) {
		return;
	}

	// if there is no handle yet, go ahead and add it, ignoring the last predict frame early out
	// if not, that causes next render frame to have a bunch of spurious primitive draws ( r_showPrimitives )
	// ( we suspect this is because TH_UPDATEVISUALS doesn't get cleared? )
	if ( !gameLocal.isLastPredictFrame && modelDefHandle != -1 ) {
		return;
	}


// ddynerman: don't render objects not in our instance (only on server)
	if ( gameLocal.isServer && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->GetInstance() != GetInstance() ) {
		FreeModelDef();
		return;
	}


	// don't render server demo stuff that's not in our instance
	if ( gameLocal.GetDemoState() == DEMO_PLAYING && gameLocal.IsServerDemo() ) {
		if ( instance != 0 ) {
			FreeModelDef();
			return;
		}
	}

	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}
	BecomeInactive( TH_UPDATEVISUALS );

	// camera target for remote render views

// rjohnson: removed PVS check for when func_static's are not starting in your PVS
	if ( cameraTarget ) { // && gameLocal.InPlayerPVS( this ) ) {

		renderEntity.remoteRenderView = cameraTarget->GetRenderView();
	}

	// if set to invisible, skip
	if ( !renderEntity.hModel || IsHidden() ) {
		return;
	}

	// add to refresh list
	if ( modelDefHandle == -1 ) {
		modelDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	} else {
		gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
	}
}

/*
================
anEntity::UpdateRenderEntity
================
*/
bool anEntity::UpdateRenderEntity( renderEntity_s *renderEntity, const renderView_t *renderView ) {
	if ( gameLocal.inCinematic && gameLocal.skipCinematic ) {
		return false;
	}

	anAnimator *animator = GetAnimator();
	if ( animator ) {
		return animator->CreateFrame( gameLocal.time, false );
	}

	return false;
}

/*
================
anEntity::ModelCallback

	NOTE: may not change the game state whatsoever!
================
*/
bool anEntity::ModelCallback( renderEntity_s *renderEntity, const renderView_t *renderView ) {
	anEntity *ent;

	ent = gameLocal.entities[ renderEntity->entityNum ];
	if ( !ent ) {
		gameLocal.Error( "anEntity::ModelCallback: callback with nullptr game entity '%d'", renderEntity->entityNum );
	}

	return ent->UpdateRenderEntity( renderEntity, renderView );
}

/*
================
anEntity::GetAnimator

Subclasses will be responsible for allocating animator.
================
*/
anAnimator *anEntity::GetAnimator( void ) {
	return nullptr;
}

/*
=============
anEntity::GetRenderView

This is used by remote camera views to look from an entity
=============
*/
renderView_t *anEntity::GetRenderView( void ) {
	if ( !renderView ) {
		renderView = new renderView_t;
	}
	memset( renderView, 0, sizeof( *renderView ) );

	renderView->vieworg = GetPhysics()->GetOrigin();
	renderView->fov_x = 120;
	renderView->fov_y = 120;
	renderView->viewaxis = GetPhysics()->GetAxis();

	// copy global shader parms
	for ( int i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) {
		renderView->shaderParms[i] = gameLocal.globalShaderParms[i];
	}

	renderView->globalMaterial = gameLocal.GetGlobalMaterial();

	renderView->time = gameLocal.time;

	return renderView;
}


// bdube: added convienince functions for effects

/***********************************************************************

  effects

***********************************************************************/

/*
================
anEntity::PlayEffect
================
*/
rvClientEffect* anEntity::PlayEffect( const idDecl *effect, jointHandle_t joint, const anVec3 &originOffset, const anMat3 &axisOffset, bool loop, const anVec3 &endOrigin, bool broadcast, effectCategory_t category, const anVec4& effectTint ) {
	if ( joint == INVALID_JOINT ) {
		return nullptr;
	}

	if ( !effect || !gameLocal.isNewFrame ) {
		return nullptr;
	}

	if ( !gameLocal.isClient && broadcast ) {
		anBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		idGameLocal::WriteDecl( msg, effect );
		msg.WriteLong( joint );
		msg.WriteBits( loop, 1 );
		msg.WriteFloat( endOrigin.x );
		msg.WriteFloat( endOrigin.y );
		msg.WriteFloat( endOrigin.z );
		msg.WriteByte( category );
		ServerSendInstanceEvent( EVENT_PLAYEFFECT_JOINT, &msg, false, -1 );
	}


// rjohnson: no effects on dedicated server
	if ( gameLocal.isMultiplayer && !gameLocal.isClient && !gameLocal.isListenServer ) {
		// no effects on dedicated server
		return nullptr;
	}

	if ( bse->Filtered( effect->GetName(), category ) ) {
		// Effect filtered out
		return nullptr;
	}

	if ( gameLocal.isListenServer && gameLocal.GetLocalPlayer() ) {
		if ( GetInstance() != gameLocal.GetLocalPlayer()->GetInstance() ) {
			return nullptr;
		}
	}


	PushSystemHeapID(RV_HEAP_ID_MULTIPLE_FRAME);
	rvClientEffect* clientEffect = new rvClientEffect( effect );
	PopSystemHeap();

	if ( !clientEffect ) {
		common->Warning( "Failed to create effect \'%s\'\n", effect->GetName() );
		return nullptr;
	}

	if ( clientEffect->entityNumber == -1 ) {
		common->Warning( "Failed to spawn effect \'%s\'\n", effect->GetName() );
		delete clientEffect;
		return nullptr;
	}

	clientEffect->SetOrigin( originOffset );
	clientEffect->SetAxis( axisOffset );
	clientEffect->Bind( this, joint );
	clientEffect->SetGravity( gameLocal.GetCurrentGravity( this ) );

	if ( !clientEffect->Play( gameLocal.time, loop, endOrigin ) ) {
		delete clientEffect;
		return nullptr;
	}

	clientEffect->GetRenderEffect()->shaderParms[ SHADERPARM_RED ]		= effectTint[0];
	clientEffect->GetRenderEffect()->shaderParms[ SHADERPARM_GREEN ]	= effectTint[1];
	clientEffect->GetRenderEffect()->shaderParms[ SHADERPARM_BLUE ]		= effectTint[2];
	clientEffect->GetRenderEffect()->shaderParms[ SHADERPARM_ALPHA ]	= effectTint[3];

	return clientEffect;
}

rvClientEffect* anEntity::PlayEffect( const idDecl *effect, const anVec3 &origin, const anMat3 &axis, bool loop, const anVec3 &endOrigin, bool broadcast, effectCategory_t category, const anVec4& effectTint ) {
	anVec3 localOrigin;
	anMat3 localAxis;

	if ( !effect || !gameLocal.isNewFrame ) {
		return nullptr;
	}

	if ( entityNumber == ENTITYNUM_WORLD ) {
		return gameLocal.PlayEffect( effect, origin, axis, loop, endOrigin, broadcast, false, category, effectTint );
	}

	// Calculate the local origin and axis from the given globals
	localOrigin = ( origin - renderEntity.origin ) * renderEntity.axis.Transpose();
	localAxis   = axis * renderEntity.axis.Transpose();

	if ( !gameLocal.isClient && broadcast ) {
		anBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];
		anCQuat		quat;

		quat = localAxis.ToCQuat();

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		idGameLocal::WriteDecl( msg, effect );
		msg.WriteFloat( localOrigin.x );
		msg.WriteFloat( localOrigin.y );
		msg.WriteFloat( localOrigin.z );
		msg.WriteFloat( quat.x );
		msg.WriteFloat( quat.y );
		msg.WriteFloat( quat.z );
		msg.WriteBits( loop, 1 );
		msg.WriteFloat( endOrigin.x );
		msg.WriteFloat( endOrigin.y );
		msg.WriteFloat( endOrigin.z );
		msg.WriteByte( category );
		ServerSendInstanceEvent( EVENT_PLAYEFFECT, &msg, false, -1 );
	}


// rjohnson: no effects on dedicated server
	if ( gameLocal.isMultiplayer && !gameLocal.isClient && !gameLocal.isListenServer ) {
		// no effects on dedicated server
		return nullptr;
	}

	if ( bse->Filtered( effect->GetName(), category ) ) {
		// Effect filtered out
		return( nullptr );
	}
// ddynerman: a listen server might get this far re: playing effects, don't actually play out of instance effects
	if ( gameLocal.isListenServer && gameLocal.GetLocalPlayer() ) {
		if ( GetInstance() != gameLocal.GetLocalPlayer()->GetInstance() ) {
			return nullptr;
		}
	}



// mwhitlock: Dynamic memory consolidation
	PushSystemHeapID(RV_HEAP_ID_MULTIPLE_FRAME);
	rvClientEffect* clientEffect = new rvClientEffect( effect );
	PopSystemHeap();


	if ( !clientEffect ) {
		common->Warning( "Failed to create effect \'%s\'\n", effect->GetName() );
		return nullptr;
	}

	if ( clientEffect->entityNumber == -1 ) {
		common->Warning( "Failed to spawn effect \'%s\'\n", effect->GetName() );
		delete clientEffect;
		return nullptr;
	}

	clientEffect->SetOrigin( localOrigin );
	clientEffect->SetAxis( localAxis );
	clientEffect->Bind( this );
	clientEffect->SetGravity( gameLocal.GetCurrentGravity( this ) );

	if ( !clientEffect->Play( gameLocal.time, loop, endOrigin ) ) {
		delete clientEffect;
		return nullptr;
	}

	clientEffect->GetRenderEffect()->shaderParms[ SHADERPARM_RED ]		= effectTint[0];
	clientEffect->GetRenderEffect()->shaderParms[ SHADERPARM_GREEN ]	= effectTint[1];
	clientEffect->GetRenderEffect()->shaderParms[ SHADERPARM_BLUE ]		= effectTint[2];
	clientEffect->GetRenderEffect()->shaderParms[ SHADERPARM_ALPHA ]	= effectTint[3];

	return clientEffect;
}

/*
================
anEntity::StopAllEffects
================
*/
void anEntity::StopAllEffects( bool destroyParticles ) {
	rvClientEntity* cent;
	rvClientEntity* next;

	for ( cent = clientEntities.Next(); cent != nullptr; cent = next ) {
		next = cent->bindNode.Next();
		if ( cent->IsType ( rvClientEffect::GetClassType() ) ) {
			static_cast<rvClientEffect *>( cent )->Stop( destroyParticles );
		}
	}
}

/*
================
anEntity::StopEffect
================
*/
void anEntity::StopEffect( const idDecl *effect, bool destroyParticles ) {
	rvClientEntity*	cent;
	rvClientEntity*	next;

	if ( !effect ) {
		return;
	}

	// Build a list of all the effects to stop
	for ( cent = clientEntities.Next(); cent != nullptr; cent = next ) {
		next = cent->bindNode.Next();

		// Is this client entity an effect?
		if ( !cent->IsType( rvClientEffect::GetClassType() ) ) {
			continue;
		}

		// Now check to make sure its the specific effect we want to stop
		rvClientEffect* clientEffect;
		clientEffect = static_cast<rvClientEffect *>( cent );
		if ( clientEffect->GetEffectIndex() == effect->Index() ) {
			clientEffect->Stop( destroyParticles );
		}
	}
}

void anEntity::StopEffect( const char *effectName, bool destroyParticles ) {
	StopEffect( gameLocal.GetEffect( spawnArgs, effectName ), destroyParticles );
}



/***********************************************************************

  Sound

***********************************************************************/

/*
================
anEntity::CanPlayChatterSounds

Used for playing chatter sounds on monsters.
================
*/
bool anEntity::CanPlayChatterSounds( void ) const {
	return true;
}

/*
================
anEntity::StartSound
================
*/
bool anEntity::StartSound( const char *soundName, const s_channelType channel, int soundShaderFlags, bool broadcast, int *length ) {
	const idSoundShader *shader;
	const char *sound;

	if ( length ) {
		*length = 0;
	}

	// we should ALWAYS be playing sounds from the def.
	// hardcoded sounds MUST be avoided at all times because they won't get precached.
	anStr soundNameStr = soundName;
	if ( soundNameStr.CmpPrefix( "snd_" ) && soundNameStr.CmpPrefix( "lipsync_" ) ) {
		common->Warning( "Non precached sound \'%s\'", soundName );
	}

	if ( !spawnArgs.GetString( soundName, "", &sound ) ) {
		return false;
	}

	if ( *sound == '\0' ) {
		return false;
	}

	if ( !gameLocal.isNewFrame ) {
		// don't play the sound, but don't report an error
		return true;
	}

	shader = declManager->FindSound( sound );
	return StartSoundShader( shader, channel, soundShaderFlags, broadcast, length );
}

/*
================
anEntity::StartSoundShader
================
*/
bool anEntity::StartSoundShader( const idSoundShader *shader, const s_channelType channel, int soundShaderFlags, bool broadcast, int *length ) {
	float diversity;
	int len;

	if ( length ) {
		*length = 0;
	}

	if ( !shader ) {
		return false;
	}

	if ( !gameLocal.isNewFrame ) {
		return true;
	}

	if ( gameLocal.isServer && broadcast ) {
		anBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		idGameLocal::WriteDecl( msg, shader );
		msg.WriteByte( channel );
		ServerSendInstanceEvent( EVENT_STARTSOUNDSHADER, &msg, false, -1 );
	}

	// in MP, don't play sounds from other instances
	if ( gameLocal.isMultiplayer && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->GetInstance() != instance ) {
		return false;
	}

	// rjohnson: don't play sounds on a dedicated server!
	if ( gameLocal.isMultiplayer && !gameLocal.isClient && !gameLocal.isListenServer ) {
		return false;
	}

	// set a random value for diversity unless one was parsed from the entity
	if ( refSound.diversity < 0.0f ) {
		diversity = gameLocal.random.RandomFloat();
	} else {
		diversity = refSound.diversity;
	}


	// if we don't have a soundEmitter allocated yet, get one now
	if ( !soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle ) ) {
		refSound.referenceSoundHandle = soundSystem->AllocSoundEmitter( SOUNDWORLD_GAME );
	}

	UpdateSound();

	idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
	if ( emitter ) {
		emitter->UpdateEmitter( refSound.origin, refSound.velocity, refSound.listenerId, &refSound.parms );
        len = emitter->StartSound( shader, channel, diversity, soundShaderFlags );
		if ( length ) {
			*length = len;
		}
	}


	// set reference to the sound for shader synced effects
	renderEntity.referenceSoundHandle = refSound.referenceSoundHandle;

	return true;
}

/*
================
anEntity::StopSound
================
*/
void anEntity::StopSound( const s_channelType channel, bool broadcast ) {
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	if ( gameLocal.isServer && broadcast ) {
		anBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteByte( channel );
		ServerSendInstanceEvent( EVENT_STOPSOUNDSHADER, &msg, false, -1 );
	}

	// in MP, don't play sounds from other instances
	if ( gameLocal.isMultiplayer && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->GetInstance() != instance ) {
		return;
	}


	idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
	if ( emitter ) {
		emitter->StopSound( channel );
	}

}

/*
================
anEntity::SetSoundVolume

  Must be called before starting a new sound.
================
*/
void anEntity::SetSoundVolume( float volume ) {
	refSound.parms.volume = volume;
}

/*
================
anEntity::UpdateSound
================
*/
void anEntity::UpdateSound( void ) {

	idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
	if ( emitter ) {

		anVec3 origin;
		anMat3 axis;

		if ( GetPhysicsToSoundTransform( origin, axis ) ) {
			refSound.origin = GetPhysics()->GetOrigin() + origin * axis;
		} else {
			refSound.origin = GetPhysics()->GetOrigin();
		}


		refSound.velocity = GetPhysics()->GetLinearVelocity();
		emitter->UpdateEmitter( refSound.origin, refSound.velocity, refSound.listenerId, &refSound.parms );

	}
}

/*
================
anEntity::GetListenerId
================
*/
int anEntity::GetListenerId( void ) const {
	return refSound.listenerId;
}

/*
================
anEntity::GetSoundEmitter
================
*/

int anEntity::GetSoundEmitter( void ) const {
	return( refSound.referenceSoundHandle );

}

/*
================
anEntity::FreeSoundEmitter
================
*/
void anEntity::FreeSoundEmitter( bool immediate ) {

	soundSystem->FreeSoundEmitter( SOUNDWORLD_GAME, refSound.referenceSoundHandle, immediate );
	refSound.referenceSoundHandle = -1;

}


// bdube: client entities

/***********************************************************************

  client entities

***********************************************************************/

/*
================
anEntity::RemoveClientEntities
================
*/
void anEntity::RemoveClientEntities( void ) {
	rvClientEntity* cent;
	// Unbinding should remove the node from the list so keep using the head until
	// there are no more entities
	for ( cent = clientEntities.Next(); cent != nullptr; cent = clientEntities.Next() ) {
		cent->Unbind( );
		delete cent;
	}
	clientEntities.Clear( );
}


/***********************************************************************

  entity binding

***********************************************************************/

/*
================
anEntity::PreBind
================
*/
void anEntity::PreBind( void ) {
}

/*
================
anEntity::PostBind
================
*/
void anEntity::PostBind( void ) {
}

/*
================
anEntity::PreUnbind
================
*/
void anEntity::PreUnbind( void ) {
}

/*
================
anEntity::PostUnbind
================
*/
void anEntity::PostUnbind( void ) {
}

/*
================
anEntity::InitBind
================
*/
bool anEntity::InitBind( anEntity *master ) {

	if ( master == this ) {
		gameLocal.Error( "Tried to bind an object to itself." );
		return false;
	}

	if ( this == gameLocal.world ) {
		gameLocal.Error( "Tried to bind world to another entity" );
		return false;
	}

	// unbind myself from my master
	Unbind();

	// add any bind constraints to an articulated figure


	if ( master && IsType( idAFEntity_Base::GetClassType() ) ) {

		static_cast<idAFEntity_Base *>(this)->AddBindConstraints();
	}

	if ( !master || master == gameLocal.world ) {
		// this can happen in scripts, so safely exit out.
		return false;
	}

	return true;
}

/*
================
anEntity::FinishBind
================
*/
void anEntity::FinishBind( void ) {

	// set the master on the physics object
	physics->SetMaster( bindMaster, fl.bindOrientated );

	// We are now separated from our previous team and are either
	// an individual, or have a team of our own.  Now we can join
	// the new bindMaster's team.  Bindmaster must be set before
	// joining the team, or we will be placed in the wrong position
	// on the team.
	JoinTeam( bindMaster );

	// if our bindMaster is enabled during a cinematic, we must be, too

// rjohnson: players should always have cinematic turned on, no matter what
	if ( !IsType ( anBasePlayer::GetClassType() ) ) {
		cinematic = bindMaster->cinematic;
	}


	// make sure the team master is active so that physics get run
	teamMaster->BecomeActive( TH_PHYSICS );
}

/*
================
anEntity::Bind

  bind relative to the visual position of the master
================
*/
void anEntity::Bind( anEntity *master, bool orientated ) {

	if ( !InitBind( master ) ) {
		return;
	}

	PreBind();

	bindJoint = INVALID_JOINT;
	bindBody = -1;
	bindMaster = master;
	fl.bindOrientated = orientated;

	FinishBind();

	PostBind( );
}

/*
================
anEntity::BindToJoint

  bind relative to a joint of the md5 model used by the master
================
*/
void anEntity::BindToJoint( anEntity *master, const char *jointname, bool orientated ) {
	jointHandle_t	jointnum;
	anAnimator		*masterAnimator;

	if ( !InitBind( master ) ) {
		return;
	}

	masterAnimator = master->GetAnimator();
	if ( !masterAnimator ) {
		gameLocal.Warning( "anEntity::BindToJoint: entity '%s' cannot support skeletal models.", master->GetName() );
		return;
	}

	jointnum = masterAnimator->GetJointHandle( jointname );
	if ( jointnum == INVALID_JOINT ) {
		gameLocal.Warning( "anEntity::BindToJoint: joint '%s' not found on entity '%s'.", jointname, master->GetName() );
	}

	PreBind();

	bindJoint = jointnum;
	bindBody = -1;
	bindMaster = master;
	fl.bindOrientated = orientated;

	FinishBind();

	PostBind();
}

/*
================
anEntity::BindToJoint

  bind relative to a joint of the md5 model used by the master
================
*/
void anEntity::BindToJoint( anEntity *master, jointHandle_t jointnum, bool orientated ) {

	if ( !InitBind( master ) ) {
		return;
	}

	PreBind();

	bindJoint = jointnum;
	bindBody = -1;
	bindMaster = master;
	fl.bindOrientated = orientated;

	FinishBind();

	PostBind();
}

/*
================
anEntity::BindToBody

  bind relative to a collision model used by the physics of the master
================
*/
void anEntity::BindToBody( anEntity *master, int bodyId, bool orientated ) {

	if ( !InitBind( master ) ) {
		return;
	}

	if ( bodyId < 0 ) {
		gameLocal.Warning( "anEntity::BindToBody: body '%d' not found.", bodyId );
	}

	PreBind();

	bindJoint = INVALID_JOINT;
	bindBody = bodyId;
	bindMaster = master;
	fl.bindOrientated = orientated;

	FinishBind();

	PostBind();
}

/*
================
anEntity::Unbind
================
*/
void anEntity::Unbind( void ) {
	anEntity *	prev;
	anEntity *	next;
	anEntity *	last;
	anEntity *	ent;

	// remove any bind constraints from an articulated figure


	if ( IsType( idAFEntity_Base::GetClassType() ) ) {

		static_cast<idAFEntity_Base *>(this)->RemoveBindConstraints();
	}

	if ( !bindMaster ) {
		return;
	}

	if ( !teamMaster ) {
		// Teammaster already has been freed
		bindMaster = nullptr;
		return;
	}

	PreUnbind();

	if ( physics ) {
		physics->SetMaster( nullptr, fl.bindOrientated );
	}

	// We're still part of a team, so that means I have to extricate myself
	// and any entities that are bound to me from the old team.
	// Find the node previous to me in the team
	prev = teamMaster;
	for ( ent = teamMaster->teamChain; ent && ( ent != this ); ent = ent->teamChain ) {
		prev = ent;
	}

	assert( ent == this ); // If ent is not pointing to this, then something is very wrong.

	// Find the last node in my team that is bound to me.
	// Also find the first node not bound to me, if one exists.
	last = this;
	for ( next = teamChain; next != nullptr; next = next->teamChain ) {
		if ( !next->IsBoundTo( this ) ) {
			break;
		}

		// Tell them I'm now the teamMaster
		next->teamMaster = this;
		last = next;
	}

	// disconnect the last member of our team from the old team
	last->teamChain = nullptr;

	// connect up the previous member of the old team to the node that
	// follow the last node bound to me (if one exists).
	if ( teamMaster != this ) {
		prev->teamChain = next;
		if ( !next && ( teamMaster == prev ) ) {
			prev->teamMaster = nullptr;
		}
	} else if ( next ) {
		// If we were the teamMaster, then the nodes that were not bound to me are now
		// a disconnected chain.  Make them into their own team.
		for ( ent = next; ent->teamChain != nullptr; ent = ent->teamChain ) {
			ent->teamMaster = next;
		}
		next->teamMaster = next;
	}

	// If we don't have anyone on our team, then clear the team variables.
	if ( teamChain ) {
		// make myself my own team
		teamMaster = this;
	} else {
		// no longer a team
		teamMaster = nullptr;
	}

	bindJoint = INVALID_JOINT;
	bindBody = -1;
	bindMaster = nullptr;

	PostUnbind();
}

/*
================
anEntity::RemoveBinds
================
*/
void anEntity::RemoveBinds( void ) {
	anEntity *ent;
	anEntity *next;

	for ( ent = teamChain; ent != nullptr; ent = next ) {
		next = ent->teamChain;
		if ( ent->bindMaster == this ) {
			ent->Unbind();
			ent->PostEventMS( &EV_Remove, 0 );
			next = teamChain;
		}
	}
}

/*
================
anEntity::IsBound
================
*/
bool anEntity::IsBound( void ) const {
	if ( bindMaster ) {
		return true;
	}
	return false;
}

/*
================
anEntity::IsBoundTo
================
*/

// abahr: added const so it can be called from const functions
bool anEntity::IsBoundTo( const anEntity *master ) const {

	anEntity *ent;

	if ( !bindMaster ) {
		return false;
	}

	for ( ent = bindMaster; ent != nullptr; ent = ent->bindMaster ) {
		if ( ent == master ) {
			return true;
		}
	}

	return false;
}

/*
================
anEntity::GetBindMaster
================
*/
anEntity *anEntity::GetBindMaster( void ) const {
	return bindMaster;
}

/*
================
anEntity::GetBindJoint
================
*/
jointHandle_t anEntity::GetBindJoint( void ) const {
	return bindJoint;
}

/*
================
anEntity::GetBindBody
================
*/
int anEntity::GetBindBody( void ) const {
	return bindBody;
}

/*
================
anEntity::GetTeamMaster
================
*/
anEntity *anEntity::GetTeamMaster( void ) const {
	return teamMaster;
}

/*
================
anEntity::GetNextTeamEntity
================
*/
anEntity *anEntity::GetNextTeamEntity( void ) const {
	return teamChain;
}

/*
=====================
anEntity::ConvertLocalToWorldTransform
=====================
*/
void anEntity::ConvertLocalToWorldTransform( anVec3 &offset, anMat3 &axis ) {
	UpdateModelTransform();

	offset = renderEntity.origin + offset * renderEntity.axis;
	axis *= renderEntity.axis;
}

/*
================
anEntity::GetLocalVector

Takes a vector in worldspace and transforms it into the parent
object's localspace.

Note: Does not take origin into acount.  Use getLocalCoordinate to
convert coordinates.
================
*/
anVec3 anEntity::GetLocalVector( const anVec3 &vec ) const {
	anVec3	pos;

	if ( !bindMaster ) {
		return vec;
	}

	anVec3	masterOrigin;
	anMat3	masterAxis;

	GetMasterPosition( masterOrigin, masterAxis );
	masterAxis.ProjectVector( vec, pos );

	return pos;
}

/*
================
anEntity::GetLocalCoordinates

Takes a vector in world coordinates and transforms it into the parent
object's local coordinates.
================
*/
anVec3 anEntity::GetLocalCoordinates( const anVec3 &vec ) const {
	anVec3	pos;

	if ( !bindMaster ) {
		return vec;
	}

	anVec3	masterOrigin;
	anMat3	masterAxis;

	GetMasterPosition( masterOrigin, masterAxis );
	masterAxis.ProjectVector( vec - masterOrigin, pos );

	return pos;
}


// kfuller: added method

/*
================
anEntity::DistanceTo2d
================
*/
float anEntity::DistanceTo2d ( const anVec3 &pos ) const {
	anVec3 pos1;
	anVec3 pos2;
	pos1 = pos - (pos * GetPhysics()->GetGravityNormal()) * GetPhysics()->GetGravityNormal();
	pos2 = GetPhysics()->GetOrigin();
	pos2 = pos2 - (pos2 * GetPhysics()->GetGravityNormal()) * GetPhysics()->GetGravityNormal();
	return (pos2 - pos1).LengthFast();
}

/*
================
anEntity::GetLocalAngles
================
*/
void anEntity::GetLocalAngles(anAngles &localAng)
{
	anVec3 localVec = GetPhysics()->GetAxis()[0];

	GetLocalVector(localVec);
	localAng = localVec.ToAngles();
}


/*
================
anEntity::GetWorldVector

Takes a vector in the parent object's local coordinates and transforms
it into world coordinates.

Note: Does not take origin into acount.  Use getWorldCoordinate to
convert coordinates.
================
*/
anVec3 anEntity::GetWorldVector( const anVec3 &vec ) const {
	anVec3	pos;

	if ( !bindMaster ) {
		return vec;
	}

	anVec3	masterOrigin;
	anMat3	masterAxis;

	GetMasterPosition( masterOrigin, masterAxis );
	masterAxis.UnprojectVector( vec, pos );

	return pos;
}

/*
================
anEntity::GetWorldCoordinates

Takes a vector in the parent object's local coordinates and transforms
it into world coordinates.
================
*/
anVec3 anEntity::GetWorldCoordinates( const anVec3 &vec ) const {
	anVec3	pos;

	if ( !bindMaster ) {
		return vec;
	}

	anVec3	masterOrigin;
	anMat3	masterAxis;

	GetMasterPosition( masterOrigin, masterAxis );
	masterAxis.UnprojectVector( vec, pos );
	pos += masterOrigin;

	return pos;
}

/*
================
anEntity::GetMasterPosition
================
*/
bool anEntity::GetMasterPosition( anVec3 &masterOrigin, anMat3 &masterAxis ) const {
	anVec3		localOrigin;
	anMat3		localAxis;
	anAnimator	*masterAnimator;

	if ( bindMaster ) {
		// if bound to a joint of an animated model
		if ( bindJoint != INVALID_JOINT ) {
			masterAnimator = bindMaster->GetAnimator();
			if ( !masterAnimator ) {
				masterOrigin = vec3_origin;
				masterAxis = mat3_identity;
				return false;
			} else {
				masterAnimator->GetJointTransform( bindJoint, gameLocal.time, masterOrigin, masterAxis );
				masterAxis *= bindMaster->renderEntity.axis;
				masterOrigin = bindMaster->renderEntity.origin + masterOrigin * bindMaster->renderEntity.axis;
			}
		} else if ( bindBody >= 0 && bindMaster->GetPhysics() ) {
			masterOrigin = bindMaster->GetPhysics()->GetOrigin( bindBody );
			masterAxis = bindMaster->GetPhysics()->GetAxis( bindBody );
		} else {
			masterOrigin = bindMaster->renderEntity.origin;
			masterAxis = bindMaster->renderEntity.axis;
		}
		return true;
	} else {
		masterOrigin = vec3_origin;
		masterAxis = mat3_identity;
		return false;
	}
}


// abahr: needed so client get the correct position
/*
================
anEntity::GetPosition
================
*/
void anEntity::GetPosition( anVec3 &origin, anMat3 &axis ) const {
	origin = renderEntity.origin;
	axis = renderEntity.axis;
}


/*
================
anEntity::GetWorldVelocities
================
*/
void anEntity::GetWorldVelocities( anVec3 &linearVelocity, anVec3 &angularVelocity ) const {

	linearVelocity = physics->GetLinearVelocity();
	angularVelocity = physics->GetAngularVelocity();

	if ( bindMaster ) {
		anVec3 masterOrigin, masterLinearVelocity, masterAngularVelocity;
		anMat3 masterAxis;

		// get position of master
		GetMasterPosition( masterOrigin, masterAxis );

		// get master velocities
		bindMaster->GetWorldVelocities( masterLinearVelocity, masterAngularVelocity );

		// linear velocity relative to master plus master linear and angular velocity
		linearVelocity = linearVelocity * masterAxis + masterLinearVelocity +
								masterAngularVelocity.Cross( GetPhysics()->GetOrigin() - masterOrigin );
	}
}

/*
================
anEntity::JoinTeam
================
*/
void anEntity::JoinTeam( anEntity *teammember ) {
	anEntity *ent;
	anEntity *master;
	anEntity *prev;
	anEntity *next;

	// if we're already on a team, quit it so we can join this one
	if ( teamMaster && ( teamMaster != this ) ) {
		QuitTeam();
	}

	assert( teammember );

	if ( teammember == this ) {
		teamMaster = this;
		return;
	}

	// check if our new team mate is already on a team
	master = teammember->teamMaster;
	if ( !master ) {
		// he's not on a team, so he's the new teamMaster
		master = teammember;
		teammember->teamMaster = teammember;
		teammember->teamChain = this;

		// make anyone who's bound to me part of the new team
		for ( ent = teamChain; ent != nullptr; ent = ent->teamChain ) {
			ent->teamMaster = master;
		}
	} else {
		// skip past the chain members bound to the entity we're teaming up with
		prev = teammember;
		next = teammember->teamChain;
		if ( bindMaster ) {
			// if we have a bindMaster, join after any entities bound to the entity
			// we're joining
			while( next && next->IsBoundTo( teammember ) ) {
				prev = next;
				next = next->teamChain;
			}
		} else {
			// if we're not bound to someone, then put us at the end of the team
			while( next ) {
				prev = next;
				next = next->teamChain;
			}
		}

		// make anyone who's bound to me part of the new team and
		// also find the last member of my team
		for ( ent = this; ent->teamChain != nullptr; ent = ent->teamChain ) {
			ent->teamChain->teamMaster = master;
		}

    	prev->teamChain = this;
		ent->teamChain = next;
	}

	teamMaster = master;

	// reorder the active entity list
	gameLocal.sortTeamMasters = true;
}

/*
================
anEntity::QuitTeam
================
*/
void anEntity::QuitTeam( void ) {
	anEntity *ent;

	if ( !teamMaster ) {
		return;
	}

	// check if I'm the teamMaster
	if ( teamMaster == this ) {
		// do we have more than one teammate?
		if ( !teamChain->teamChain ) {
			// no, break up the team
			teamChain->teamMaster = nullptr;
		} else {
			// yes, so make the first teammate the teamMaster
			for ( ent = teamChain; ent; ent = ent->teamChain ) {
				ent->teamMaster = teamChain;
			}
		}
	} else {
		assert( teamMaster );
		assert( teamMaster->teamChain );

		// find the previous member of the teamChain
		ent = teamMaster;
		while( ent->teamChain != this ) {
			assert( ent->teamChain ); // this should never happen
			ent = ent->teamChain;
		}

		// remove this from the teamChain
		ent->teamChain = teamChain;

		// if no one is left on the team, break it up
		if ( !teamMaster->teamChain ) {
			teamMaster->teamMaster = nullptr;
		}
	}

	teamMaster = nullptr;
	teamChain = nullptr;
}

/***********************************************************************

  Physics.

***********************************************************************/

/*
================
anEntity::InitDefaultPhysics
================
*/
void anEntity::InitDefaultPhysics( const anVec3 &origin, const anMat3 &axis ) {
	const char *temp;
	anClipModel *clipModel = nullptr;

	// check if a clipmodel key/value pair is set
	if ( spawnArgs.GetString( "clipmodel", "", &temp ) ) {

// mwhitlock: Dynamic memory consolidation
		PushHeapMemory(this);

		clipModel = new anClipModel( temp );

// mwhitlock: Dynamic memory consolidation
		PopSystemHeap();

	}

	if ( !spawnArgs.GetBool( "noclipmodel", "0" ) ) {

		// check if mins/maxs or size key/value pairs are set
		if ( !clipModel ) {
			anVec3 size;
			anBounds bounds;
			bool setClipModel = false;

			if ( spawnArgs.GetVector( "mins", nullptr, bounds[0] ) &&
				spawnArgs.GetVector( "maxs", nullptr, bounds[1] ) ) {
				setClipModel = true;
				if ( bounds[0][0] > bounds[1][0] || bounds[0][1] > bounds[1][1] || bounds[0][2] > bounds[1][2] ) {
					gameLocal.Error( "Invalid bounds '%s'-'%s' on entity '%s'", bounds[0].ToString(), bounds[1].ToString(), name.c_str() );
				}
			} else if ( spawnArgs.GetVector( "size", nullptr, size ) ) {
				if ( ( size.x < 0.0f ) || ( size.y < 0.0f ) || ( size.z < 0.0f ) ) {
					gameLocal.Error( "Invalid size '%s' on entity '%s'", size.ToString(), name.c_str() );
				}
				bounds[0].Set( size.x * -0.5f, size.y * -0.5f, 0.0f );
				bounds[1].Set( size.x * 0.5f, size.y * 0.5f, size.z );
				setClipModel = true;
			}

			if ( setClipModel ) {
				int numSides;
				anTraceModel trm;

				if ( spawnArgs.GetInt( "cylinder", "0", numSides ) && numSides > 0 ) {
					trm.SetupCylinder( bounds, numSides < 3 ? 3 : numSides );
				} else if ( spawnArgs.GetInt( "cone", "0", numSides ) && numSides > 0 ) {
					trm.SetupCone( bounds, numSides < 3 ? 3 : numSides );

// bdube: added dodecahedron
				} else if ( spawnArgs.GetInt( "dodecahedron", "0", numSides ) && numSides > 0 ) {
					trm.SetupDodecahedron ( bounds );

				} else {
					trm.SetupBox( bounds );
				}

// mwhitlock: Dynamic memory consolidation
				PushHeapMemory(this);

				clipModel = new anClipModel( trm );

// mwhitlock: Dynamic memory consolidation
				PopSystemHeap();

			}
		}

		// check if the visual model can be used as collision model
		if ( !clipModel ) {
			temp = spawnArgs.GetString( "model" );
			if ( ( temp != nullptr ) && ( *temp != 0 ) ) {

// jscott:slash problems
				anStr canonical = temp;
				canonical.BackSlashesToSlashes();

// mwhitlock: Dynamic memory consolidation
				PushHeapMemory(this);

				clipModel = new anClipModel();
				if ( !clipModel->LoadModel( canonical ) ) {
					delete clipModel;
					clipModel = nullptr;
				}

// mwhitlock: Dynamic memory consolidation
				PopSystemHeap();

			}
		}
	}

	defaultPhysicsObj.SetSelf( this );
	defaultPhysicsObj.SetClipModel( clipModel, 1.0f );
	defaultPhysicsObj.SetOrigin( origin );
	defaultPhysicsObj.SetAxis( axis );

	physics = &defaultPhysicsObj;
}

/*
================
anEntity::SetPhysics
================
*/
void anEntity::SetPhysics( anPhysics *phys ) {
	// clear any contacts the current physics object has
	if ( physics ) {
		physics->ClearContacts();
	}
	// set new physics object or set the default physics if nullptr
	if ( phys != nullptr ) {
		defaultPhysicsObj.SetClipModel( nullptr, 1.0f );
		physics = phys;
		physics->Activate();
	} else {
		physics = &defaultPhysicsObj;
	}
	physics->UpdateTime( gameLocal.time );
	physics->SetMaster( bindMaster, fl.bindOrientated );
}

/*
================
anEntity::RestorePhysics
================
*/
void anEntity::RestorePhysics( anPhysics *phys ) {
	assert( phys != nullptr );
	// restore physics pointer
	physics = phys;
}

/*
================
anEntity::RunPhysics
================
*/
bool anEntity::RunPhysics( void ) {
	int			i, reachedTime, startTime, endTime;
	anEntity *	part, *blockedPart, *blockingEntity = nullptr;
	trace_t		results;
	bool		moved;

	moved = false;


// jnewquist: Tag scope and callees to track allocations using "new".
	MEM_SCOPED_TAG(tag,MA_PHYSICS);


	// don't run physics if not enabled
	if ( !( thinkFlags & TH_PHYSICS ) ) {
		// however do update any animation controllers
		if ( UpdateAnimationControllers() ) {
			BecomeActive( TH_ANIMATE );
		}

// kfuller: we want to be able to debug draw the bbox regardless
		physics->DebugDraw();

		return false;
	}

	// if this entity is a team slave don't do anything because the team master will handle everything
	if ( teamMaster && teamMaster != this ) {
		return false;
	}

	startTime = gameLocal.previousTime;
	endTime = gameLocal.time;

	gameLocal.push.InitSavingPushedEntityPositions();
	blockedPart = nullptr;

	// save the physics state of the whole team and disable the team for collision detection
	for ( part = this; part != nullptr; part = part->teamChain ) {
		if ( part->physics ) {
			if ( !part->fl.solidForTeam ) {
				part->physics->DisableClip();
			}
			part->physics->SaveState();
		}
	}

	// move the whole team
	for ( part = this; part != nullptr; part = part->teamChain ) {

		if ( part->physics ) {

			// run physics

// ddynerman: optional pre-prediction
			moved = part->physics->Evaluate( endTime - startTime + part->predictTime, endTime );
			part->predictTime = 0;


			// check if the object is blocked
			blockingEntity = part->physics->GetBlockingEntity();
			if ( blockingEntity ) {
				blockedPart = part;
				break;
			}

			// if moved or forced to update the visual position and orientation from the physics
			if ( moved || part->fl.forcePhysicsUpdate ) {
				part->UpdateFromPhysics( false );
			}

			// update any animation controllers here so an entity bound
			// to a joint of this entity gets the correct position
			if ( part->UpdateAnimationControllers() ) {
				part->BecomeActive( TH_ANIMATE );
			}
		}
	}

	// enable the whole team for collision detection
	for ( part = this; part != nullptr; part = part->teamChain ) {
		if ( part->physics ) {
			if ( !part->fl.solidForTeam ) {
				part->physics->EnableClip();
			}
		}
	}

	// cdr: Obstacle Avoidance
	if (ai_useRVMasterMove.GetBool() && moved && fl.isAIObstacle) {
		AI_EntityMoved(this);
	}

	// if one of the team entities is a pusher and blocked
	if ( blockedPart ) {
		// move the parts back to the previous position
		for ( part = this; part != blockedPart; part = part->teamChain ) {

			if ( part->physics ) {

				// restore the physics state
				part->physics->RestoreState();

				// move back the visual position and orientation
				part->UpdateFromPhysics( true );
			}
		}
		for ( part = this; part != nullptr; part = part->teamChain ) {
			if ( part->physics ) {
				// update the physics time without moving
				part->physics->UpdateTime( endTime );
			}
		}

		// restore the positions of any pushed entities
		gameLocal.push.RestorePushedEntityPositions();

		if ( gameLocal.isClient ) {
			return false;
		}

		// if the master pusher has a "blocked" function, call it
		Signal( SIG_BLOCKED );
		ProcessEvent( &EV_TeamBlocked, blockedPart, blockingEntity );
		// call the blocked function on the blocked part
		blockedPart->ProcessEvent( &EV_PartBlocked, blockingEntity );
		return false;
	}

	// set pushed
	for ( i = 0; i < gameLocal.push.GetNumPushedEntities(); i++ ) {
		anEntity *ent = gameLocal.push.GetPushedEntity( i );
		ent->physics->SetPushed( endTime - startTime );
	}

	if ( gameLocal.isClient ) {
		return true;
	}

	// post reached event if the current time is at or past the end point of the motion
	for ( part = this; part != nullptr; part = part->teamChain ) {

		if ( part->physics ) {

			reachedTime = part->physics->GetLinearEndTime();
			if ( startTime < reachedTime && endTime >= reachedTime ) {
				part->ProcessEvent( &EV_ReachedPos );
			}
			reachedTime = part->physics->GetAngularEndTime();
			if ( startTime < reachedTime && endTime >= reachedTime ) {
				part->ProcessEvent( &EV_ReachedAng );
			}
		}
	}

	return true;
}

/*
================
anEntity::UpdateFromPhysics
================
*/
void anEntity::UpdateFromPhysics( bool moveBack ) {



	if ( IsType( anActor::GetClassType() ) ) {

		anActor *actor = static_cast<anActor *>( this );

		// set master delta angles for actors
		if ( GetBindMaster() ) {
			anAngles delta = actor->GetDeltaViewAngles();
			if ( moveBack ) {
				delta.yaw -= static_cast<anPhysics_Actor *>(physics)->GetMasterDeltaYaw();
			} else {
				delta.yaw += static_cast<anPhysics_Actor *>(physics)->GetMasterDeltaYaw();
			}
			actor->SetDeltaViewAngles( delta );
		}
	}

	UpdateVisuals();
}

/*
================
anEntity::SetOrigin
================
*/
void anEntity::SetOrigin( const anVec3 &org ) {

	GetPhysics()->SetOrigin( org );

	UpdateVisuals();
}

/*
================
anEntity::SetAxis
================
*/
void anEntity::SetAxis( const anMat3 &axis ) {



	if ( GetPhysics()->IsType( anPhysics_Actor::GetClassType() ) ) {

		static_cast<anActor *>(this)->viewAxis = axis;
	} else {
		GetPhysics()->SetAxis( axis );
	}

	UpdateVisuals();
}

/*
================
anEntity::SetAngles
================
*/
void anEntity::SetAngles( const anAngles &ang ) {
	SetAxis( ang.ToMat3() );
}

/*
================
anEntity::GetFloorPos
================
*/
bool anEntity::GetFloorPos( float max_dist, anVec3 &floorpos ) const {
	trace_t result;

	if ( !GetPhysics()->HasGroundContacts() ) {
		GetPhysics()->ClipTranslation( result, GetPhysics()->GetGravityNormal() * max_dist, nullptr );
		if ( result.fraction < 1.0f ) {
			floorpos = result.endpos;
			return true;
		} else {
			floorpos = GetPhysics()->GetOrigin();
			return false;
		}
	} else {
		floorpos = GetPhysics()->GetOrigin();
		return true;
	}
}

/*
================
anEntity::GetPhysicsToVisualTransform
================
*/
bool anEntity::GetPhysicsToVisualTransform( anVec3 &origin, anMat3 &axis ) {
	return false;
}

/*
================
anEntity::GetPhysicsToSoundTransform
================
*/
bool anEntity::GetPhysicsToSoundTransform( anVec3 &origin, anMat3 &axis ) {
	// by default play the sound at the center of the bounding box of the first clip model
	if ( GetPhysics()->GetNumClipModels() > 0 ) {
		origin = GetPhysics()->GetBounds().GetCenter();
		axis.Identity();
		return true;
	}
	return false;
}

/*
================
anEntity::Collide
================
*/
bool anEntity::Collide( const trace_t &collision, const anVec3 &velocity ) {
	// this entity collides with collision.c.entityNum
	return false;
}

/*
================
anEntity::GetImpactInfo
================
*/
void anEntity::GetImpactInfo( anEntity *ent, int id, const anVec3 &point, impactInfo_t *info ) {
	GetPhysics()->GetImpactInfo( id, point, info );
}

/*
================
anEntity::ApplyImpulse
================
*/
void anEntity::ApplyImpulse( anEntity *ent, int id, const anVec3 &point, const anVec3 &impulse, bool splash ) {
	if ( SkipImpulse(ent, id) ) {
		return;
	}

	GetPhysics()->ApplyImpulse( id, point, impulse );
}

/*
================
anEntity::AddForce
================
*/
void anEntity::AddForce( anEntity *ent, int id, const anVec3 &point, const anVec3 &force ) {
	GetPhysics()->AddForce( id, point, force );
}

/*
================
anEntity::ActivatePhysics
================
*/
void anEntity::ActivatePhysics( anEntity *ent ) {
	GetPhysics()->Activate();
}

/*
================
anEntity::IsAtRest
================
*/
bool anEntity::IsAtRest( void ) const {
	return GetPhysics()->IsAtRest();
}

/*
================
anEntity::GetRestStartTime
================
*/
int anEntity::GetRestStartTime( void ) const {
	return GetPhysics()->GetRestStartTime();
}

/*
================
anEntity::AddContactEntity
================
*/
void anEntity::AddContactEntity( anEntity *ent ) {
	GetPhysics()->AddContactEntity( ent );
}

/*
================
anEntity::RemoveContactEntity
================
*/
void anEntity::RemoveContactEntity( anEntity *ent ) {

	if ( GetPhysics() ) {

		GetPhysics()->RemoveContactEntity( ent );
	}

}



/***********************************************************************

	Damage

***********************************************************************/

/*
============
anEntity::CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/

// bdube: added ignore entity
bool anEntity::CanDamage( const anVec3 &origin, anVec3 &damagePoint, anEntity *ignoreEnt ) const {

	anVec3 	dest;
	trace_t	tr;
	anVec3 	midpoint;

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin at 0,0,0
	midpoint = ( GetPhysics()->GetAbsBounds()[0] + GetPhysics()->GetAbsBounds()[1] ) * 0.5;

	dest = midpoint;

// bdube: added ignore entity
	gameLocal.TracePoint( this, tr, origin, dest, MASK_SOLID, ignoreEnt );

	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	// this should probably check in the plane of projection, rather than in world coordinate
	dest = midpoint;
	dest[0] += 15.0;
	dest[1] += 15.0;

// bdube: added ignore entity
	gameLocal.TracePoint( this, tr, origin, dest, MASK_SOLID, ignoreEnt );
// RAVEN ENE
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[0] += 15.0;
	dest[1] -= 15.0;

// bdube: added ignore entity
	gameLocal.TracePoint( this, tr, origin, dest, MASK_SOLID, ignoreEnt );

	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[0] -= 15.0;
	dest[1] += 15.0;

// bdube: added ignore entity
	gameLocal.TracePoint( this, tr, origin, dest, MASK_SOLID, ignoreEnt );

	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[0] -= 15.0;
	dest[1] -= 15.0;

// bdube: added ignore entity
	gameLocal.TracePoint( this, tr, origin, dest, MASK_SOLID, ignoreEnt );
// RAVEN EN
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[2] += 15.0;

// ddynerman: multiple collision worlds
	gameLocal.TracePoint( this, tr, origin, dest, MASK_SOLID, nullptr );

	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[2] -= 15.0;

// ddynerman: multiple collision worlds
	gameLocal.TracePoint( this, tr, origin, dest, MASK_SOLID, nullptr );

	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	return false;
}

/*
================
anEntity::DamageFeedback

callback function for when another entity recieved damage from this entity.  damage can be adjusted and returned to the caller.
================
*/
void anEntity::DamageFeedback( anEntity *victim, anEntity *inflictor, int &damage ) {
	// implemented in subclasses
}

/*
============
Damage

this		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: this=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback in global space
point		point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted

inflictor, attacker, dir, and point can be nullptr for environmental effects

============
*/
void anEntity::Damage( anEntity *inflictor, anEntity *attacker, const anVec3 &dir,
					  const char *damageDefName, const float damageScale, const int location ) {
	if ( forwardDamageEnt.IsValid() ) {
		forwardDamageEnt->Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
		return;
	}

	if ( !fl.takedamage ) {
		return;
	}

	if ( !inflictor ) {
		inflictor = gameLocal.world;
	}

	if ( !attacker ) {
		attacker = gameLocal.world;
	}

	const anDict *damageDef = gameLocal.FindEntityDefDict( damageDefName, false );
	if ( !damageDef ) {
		gameLocal.Error( "Unknown damageDef '%s'\n", damageDefName );
	}

	int	damage = damageDef->GetInt( "damage" );

	// inform the attacker that they hit someone
	attacker->DamageFeedback( this, inflictor, damage );
	if ( damage ) {
		// do the damage
		//jshepard: this is kinda important, no?
		health -= damage;

		if ( health <= 0 ) {
			if ( health < -999 ) {
				health = -999;
			}

			Killed( inflictor, attacker, damage, dir, location );
		} else {
			Pain( inflictor, attacker, damage, dir, location );
		}
	}
}

/*
============
anEntity::SkipImpulse
============
*/

// abahr: push stuff
bool anEntity::SkipImpulse( anEntity *ent, int id ) {
	return false;//ent == this;
}

/*
============
anEntity::ApplyImpulse
============
*/
void anEntity::ApplyImpulse( anEntity *ent, int id, const anVec3 &point, const anVec3 &dir, const anDict* damageDef ) {
	ApplyImpulse( ent, id, point, dir * damageDef->GetFloat( "push", "5000" ) );
}


/*
================
anEntity::AddDamageEffect
================
*/
void anEntity::AddDamageEffect( const trace_t &collision, const anVec3 &velocity, const char *damageDefName, anEntity *inflictor ) {
	const char *sound, *decal, *key;

	const anDeclEntityDef *def = gameLocal.FindEntityDef( damageDefName, false );

// bdube: impact_blood is now in the damage def
	if ( def == nullptr || !def->dict.GetBool ( "bleed" ) ) {

		return;
	}

	const char *materialType = gameLocal.sufaceTypeNames[ collision.c.material->GetSurfaceType() ];

	// start impact sound based on material type
	key = va( "snd_%s", materialType );
	sound = spawnArgs.GetString( key );
	if ( *sound == '\0' ) {
		sound = def->dict.GetString( key );
	}
	if ( *sound != '\0' ) {
		StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_BODY, 0, false, nullptr );
	}

	if ( g_decals.GetBool() ) {
		// place a wound overlay on the model
		key = va( "mtr_wound_%s", materialType );
		decal = spawnArgs.RandomPrefix( key, gameLocal.random );
		if ( *decal == '\0' ) {
			decal = def->dict.RandomPrefix( key, gameLocal.random );
		}
		if ( *decal != '\0' ) {
			anVec3 dir = velocity;
			dir.Normalize();
			ProjectOverlay( collision.c.point, dir, 20.0f, decal );
		}
	}
}

/*
================
anEntity::CanPlayImpactEffect
================
*/
bool anEntity::CanPlayImpactEffect ( anEntity *owner, anEntity *ent ) {
	if ( gameLocal.isMultiplayer ) {
		if ( gameLocal.IsTeamGame() && !cvarSystem->GetCVarBool( "si_teamDamage" ) && owner->IsType( anBasePlayer::GetClassType() ) && ent->IsType( anBasePlayer::GetClassType() ) && ((anBasePlayer*)owner)->team == ((anBasePlayer*)ent)->team ) {
			return false;
		}

		// default to blood
		return true;
	} else {
		anActor* actorOwner;
		anSAAI* aiEnt;
		actorOwner = dynamic_cast<anActor*>( owner );

		if ( ent->IsType ( idAFAttachment::GetClassType() ) ) {
			aiEnt = dynamic_cast<anSAAI*>( static_cast<idAFAttachment*>( ent )->GetBody()  );
		} else {
			aiEnt = dynamic_cast<anSAAI*>( ent );
		}

		if ( !actorOwner || !aiEnt ) {
			return true;
		}

		return (actorOwner->team != aiEnt->team);
	}
}

/*
============
anEntity::Pain

Called whenever an entity recieves damage.  Returns whether the entity responds to the pain.
This is a virtual function that subclasses are expected to implement.
============
*/
bool anEntity::Pain( anEntity *inflictor, anEntity *attacker, int damage, const anVec3 &dir, int location ) {
	return false;
}

/*
============
anEntity::Killed

Called whenever an entity's health is reduced to 0 or less.
This is a virtual function that subclasses are expected to implement.
============
*/
void anEntity::Killed( anEntity *inflictor, anEntity *attacker, int damage, const anVec3 &dir, int location ) {
}

/***********************************************************************

  Script functions

***********************************************************************/

/*
================
anEntity::ShouldConstructScriptObjectAtSpawn

Called during anEntity::Spawn to see if it should construct the script object or not.
Overridden by subclasses that need to spawn the script object themselves.
================
*/
bool anEntity::ShouldConstructScriptObjectAtSpawn( void ) const {
	return true;
}

/*
================
anEntity::ConstructScriptObject

Called during anEntity::Spawn.  Calls the constructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
================
*/
anThread *anEntity::ConstructScriptObject( void ) {
	anThread *thread;
	const function_t *constructor;

	// init the script object's data
	scriptObject.ClearObject();

	// call script object's constructor
	constructor = scriptObject.GetConstructor();
	if ( constructor ) {
		// start a thread that will initialize after Spawn is done being called

// mwhitlock: Dynamic memory consolidation
		PushHeapMemory(this);

		thread = new anThread();

// mwhitlock: Dynamic memory consolidation
		PopSystemHeap();

		thread->SetThreadName( name.c_str() );
		thread->CallFunction( this, constructor, true );
		thread->DelayedStart( 0 );
	} else {
		thread = nullptr;
	}

	// clear out the object's memory
	scriptObject.ClearObject();

	return thread;
}

/*
================
anEntity::DeconstructScriptObject

Called during anEntity::~anEntity.  Calls the destructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
Not called during idGameLocal::MapShutdown.
================
*/
void anEntity::DeconstructScriptObject( void ) {
	anThread		*thread;
	const function_t *destructor;

	// don't bother calling the script object's destructor on map shutdown
	if ( gameLocal.GameState() == GAMESTATE_SHUTDOWN ) {
		return;
	}

	// call script object's destructor
	destructor = scriptObject.GetDestructor();
	if ( destructor ) {
		// start a thread that will run immediately and be destroyed

// mwhitlock: Dynamic memory consolidation
		PushSystemHeapID(RV_HEAP_ID_TEMPORARY);

		thread = new anThread();

// mwhitlock: Dynamic memory consolidation
		PopSystemHeap();

		thread->SetThreadName( name.c_str() );
		thread->CallFunction( this, destructor, true );
		thread->Execute();
		delete thread;
	}
}

/*
================
anEntity::HasSignal
================
*/
bool anEntity::HasSignal( signalNum_t signalnum ) const {
	if ( !signals ) {
		return false;
	}
	assert( ( signalnum >= 0 ) && ( signalnum < NUM_SIGNALS ) );
	return ( signals->signal[ signalnum ].Num() > 0 );
}

/*
================
anEntity::SetSignal
================
*/
void anEntity::SetSignal( signalNum_t signalnum, anThread *thread, const function_t *function ) {
	int			i;
	int			num;
	signal_t	sig;
	int			threadnum;

	assert( ( signalnum >= 0 ) && ( signalnum < NUM_SIGNALS ) );

	if ( !signals ) {
		signals = new signalList_t;
	}

	assert( thread );
	threadnum = thread->GetThreadNum();

	num = signals->signal[ signalnum ].Num();
	for ( i = 0; i < num; i++ ) {
		if ( signals->signal[ signalnum ][i].threadnum == threadnum ) {
			signals->signal[ signalnum ][i].function = function;
			return;
		}
	}

	if ( num >= MAX_SIGNAL_THREADS ) {
		thread->Error( "Exceeded maximum number of signals per object" );
	}

	sig.threadnum = threadnum;
	sig.function = function;
	signals->signal[ signalnum ].Append( sig );
}

/*
================
anEntity::ClearSignal
================
*/
void anEntity::ClearSignal( anThread *thread, signalNum_t signalnum ) {
	assert( thread );
	if ( ( signalnum < 0 ) || ( signalnum >= NUM_SIGNALS ) ) {
		gameLocal.Error( "Signal out of range" );
	}

	if ( !signals ) {
		return;
	}

	signals->signal[ signalnum ].Clear();
}

/*
================
anEntity::ClearSignalThread
================
*/
void anEntity::ClearSignalThread( signalNum_t signalnum, anThread *thread ) {
	int	i;
	int	num;
	int	threadnum;

	assert( thread );

	if ( ( signalnum < 0 ) || ( signalnum >= NUM_SIGNALS ) ) {
		gameLocal.Error( "Signal out of range" );
	}

	if ( !signals ) {
		return;
	}

	threadnum = thread->GetThreadNum();

	num = signals->signal[ signalnum ].Num();
	for ( i = 0; i < num; i++ ) {
		if ( signals->signal[ signalnum ][i].threadnum == threadnum ) {
			signals->signal[ signalnum ].RemoveIndex( i );
			return;
		}
	}
}

/*
================
anEntity::Signal
================
*/
void anEntity::Signal( signalNum_t signalnum ) {
	int			i;
	int			num;
	signal_t	sigs[ MAX_SIGNAL_THREADS ];
	anThread	*thread;

	assert( ( signalnum >= 0 ) && ( signalnum < NUM_SIGNALS ) );

	if ( !signals ) {
		return;
	}

	// we copy the signal list since each thread has the potential
	// to end any of the threads in the list.  By copying the list
	// we don't have to worry about the list changing as we're
	// processing it.
	num = signals->signal[ signalnum ].Num();
	for ( i = 0; i < num; i++ ) {
		sigs[i] = signals->signal[ signalnum ][i];
	}

	// clear out the signal list so that we don't get into an infinite loop
	signals->signal[ signalnum ].Clear();

	for ( i = 0; i < num; i++ ) {
		thread = anThread::GetThread( sigs[i].threadnum );
		if ( thread ) {
			thread->CallFunction( this, sigs[i].function, true );
			thread->Execute();
		}
	}
}

/*
================
anEntity::SignalEvent
================
*/
void anEntity::SignalEvent( anThread *thread, signalNum_t signalnum ) {
	if ( ( signalnum < 0 ) || ( signalnum >= NUM_SIGNALS ) ) {
		gameLocal.Error( "Signal out of range" );
	}

	if ( !signals ) {
		return;
	}

	Signal( signalnum );
}

/***********************************************************************

  Guis.

***********************************************************************/


/*
================
anEntity::TriggerGuis
================
*/
void anEntity::TriggerGuis( void ) {
	int i;
	for ( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		if ( renderEntity.gui[i] ) {
			renderEntity.gui[i]->Trigger( gameLocal.time );
		}
	}
}

/*
================
anEntity::HandleGuiCommands
================
*/
bool anEntity::HandleGuiCommands( anEntity *entityGui, const char *cmds ) {
	anEntity *targetEnt;
	bool ret = false;
	if ( entityGui && cmds && *cmds ) {
		anLexer src;
		anToken token, token2, token3, token4;
		src.LoadMemory( cmds, strlen( cmds ), "guiCommands" );
		while( 1 ) {

			if ( !src.ReadToken( &token ) ) {
				return ret;
			}

			if ( token == ";" ) {
				continue;
			}

			if ( token.Icmp( "activate" ) == 0 ) {
				bool targets = true;
				if ( src.ReadToken( &token2 ) ) {
					if ( token2 == ";" ) {
						src.UnreadToken( &token2 );
					} else {
						targets = false;
					}
				}

				if ( targets ) {
					entityGui->ActivateTargets( this );
				} else {
					anEntity *ent = gameLocal.FindEntity( token2 );
					if ( ent ) {
						ent->Signal( SIG_TRIGGER );
						ent->PostEventMS( &EV_Activate, 0, this );
					}
				}

				entityGui->renderEntity.shaderParms[ SHADERPARM_MODE ] = 1.0f;
				continue;
			}


			if ( token.Icmp( "runScript" ) == 0 ) {
				if ( src.ReadToken( &token2 ) ) {
  					while( src.CheckTokenString( "::" ) ) {
  						anToken token3;
  						if ( !src.ReadToken( &token3 ) ) {
  							gameLocal.Error( "Expecting function name following '::' in gui for entity '%s'", entityGui->name.c_str() );
  						}
  						token2 += "::" + token3;
  					}
				}

// abahr: allow parms to be passed in
// For some reason the semi colon is used as a delimeter so we need the above code
				rvScriptFuncUtility utility;
				if ( utility.Init(token2) > SFU_ERROR ) {
					utility.InsertEntity( entityGui, 0 );
					utility.CallFunc( &entityGui->spawnArgs );
				}

				continue;
			}

			if ( token.Icmp( "play" ) == 0 ) {
				if ( src.ReadToken( &token2 ) ) {
					const idSoundShader *shader = declManager->FindSound(token2);
					entityGui->StartSoundShader( shader, SND_CHANNEL_ANY, 0, false, nullptr );
				}
				continue;
			}

			if ( token.Icmp( "setkeyval" ) == 0 ) {
				if ( src.ReadToken( &token2 ) && src.ReadToken(&token3) && src.ReadToken( &token4 ) ) {
					anEntity *ent = gameLocal.FindEntity( token2 );
					if ( ent ) {
						ent->spawnArgs.Set( token3, token4 );
						ent->UpdateChangeableSpawnArgs( nullptr );
						ent->UpdateVisuals();
					}
				}
				continue;
			}

			if ( token.Icmp( "setshaderparm" ) == 0 ) {
				if ( src.ReadToken( &token2 ) && src.ReadToken(&token3) ) {
					entityGui->SetShaderParm( atoi( token2 ), atof( token3 ) );
					entityGui->UpdateVisuals();
				}
				continue;
			}

			if ( token.Icmp( "close" ) == 0 ) {
				ret = true;
				continue;
			}

			// handy for debugging GUI stuff
			if ( !token.Icmp( "print" ) ) {
				anStr msg;
				while ( src.ReadToken( &token2 ) ) {
					if ( token2 == ";" ) {
						src.UnreadToken( &token2 );
						break;
					}
					msg += token2.c_str();
				}
				common->Printf( "ent gui 0x%x '%s': %s\n", entityNumber, name.c_str(), msg.c_str() );
				continue;
			}

			// if we get to this point we don't know how to handle it
			src.UnreadToken( &token );
			if ( !HandleSingleGuiCommand( entityGui, &src ) ) {
				// not handled there see if entity or any of its targets can handle it
				// this will only work for one target atm
				if ( entityGui->HandleSingleGuiCommand( entityGui, &src ) ) {
					continue;
				}

				int c = entityGui->targets.Num();
				int i;
				for ( i = 0; i < c; i++ ) {
					targetEnt = entityGui->targets[i].GetEntity();
					if ( targetEnt && targetEnt->HandleSingleGuiCommand( entityGui, &src ) ) {
						break;
					}
				}

				if ( i == c ) {
					// not handled
					common->DPrintf( "anEntity::HandleGuiCommands: '%s' not handled\n", token.c_str() );
					src.ReadToken( &token );
				}
			}

		}
	}
	return ret;
}

/*
================
anEntity::HandleSingleGuiCommand
================
*/
bool anEntity::HandleSingleGuiCommand( anEntity *entityGui, anLexer *src ) {
	return false;
}

/***********************************************************************

  Targets

***********************************************************************/

/*
===============
anEntity::FindTargets

We have to wait until all entities are spawned
Used to build lists of targets after the entity is spawned.  Since not all entities
have been spawned when the entity is created at map load time, we have to wait
===============
*/
void anEntity::FindTargets( void ) {
	int			i;

	// targets can be a list of multiple names
	gameLocal.GetTargets( spawnArgs, targets, "target" );

	// ensure that we don't target ourselves since that could cause an infinite loop when activating entities
	for ( i = 0; i < targets.Num(); i++ ) {
		if ( targets[i].GetEntity() == this ) {
			gameLocal.Error( "Entity '%s' is targeting itself", name.c_str() );
		}
	}
}

/*
================
anEntity::RemoveNullTargets
================
*/
void anEntity::RemoveNullTargets( void ) {
	int i;

	for ( i = targets.Num() - 1; i >= 0; i-- ) {
		if ( !targets[i].GetEntity() ) {
			targets.RemoveIndex( i );
		}
	}
}

/*
==============================
anEntity::ActivateTargets

"activator" should be set to the entity that initiated the firing.
==============================
*/
void anEntity::ActivateTargets( anEntity *activator ) const {
	anEntity	*ent;
	int			i, j;

	for ( i = 0; i < targets.Num(); i++ ) {
		ent = targets[i].GetEntity();
		if ( !ent ) {
			continue;
		}
		if ( ent->RespondsTo( EV_Activate ) || ent->HasSignal( SIG_TRIGGER ) ) {
			ent->Signal( SIG_TRIGGER );
			ent->ProcessEvent( &EV_Activate, activator );
		}
		for ( j = 0; j < MAX_RENDERENTITY_GUI; j++ ) {
			if ( ent->renderEntity.gui[ j ] ) {
				ent->renderEntity.gui[ j ]->Trigger( gameLocal.time );
			}
		}
	}
}


// twhitaker: added (meant to be used from script)
/*
================
anEntity::AppendTarget
================
*/
int anEntity::AppendTarget( anEntity *appendMe ) {

	int index = -1;
	// silently fail if they pass in null
	if ( appendMe )	{
		index = targets.Append( appendMe );
		RemoveNullTargets();
	}
	return index;
}
/*
================
anEntity::RemoveTarget
================
*/
void anEntity::RemoveTarget( anEntity *removeMe ) {

	targets.Remove( removeMe );
	RemoveNullTargets();
}
/*
================
anEntity::RemoveTargets
================
*/
void anEntity::RemoveTargets( bool destroyContents ) {
	if ( destroyContents ) {
		targets.RemoveContents( true );
	} else {
		targets.Clear();
	}
}

// jshepard: added
/*
================
anEntity::UnbindTargets
================
*/
void anEntity::UnbindTargets( anEntity *activator ) const {
	anEntity	*ent;
	int			i;

	for ( i = 0; i < targets.Num(); i++ ) {
		ent = targets[i].GetEntity();
		if ( !ent ) {
			continue;
		}
		ent->Unbind();

	}
}

// bdube: added
/*
================
anEntity::Event_SetContents
================
*/
void anEntity::Event_SetContents( int contents ) {
	GetPhysics()->SetContents( contents );
}

/*
================
anEntity::Event_GetLastBlocker
================
*/
void anEntity::Event_GetLastBlocker(anThread *thread) {
	int	whichEntNum = GetLastBlocker();

	if (whichEntNum < 0 || whichEntNum == ENTITYNUM_WORLD) {
		thread->ReturnEntity(this);
		return;
	}
	thread->ReturnEntity(gameLocal.entities[whichEntNum]);
}

/*
================
anEntity::ShowSurface
================
*/
void anEntity::ShowSurface ( const char *surface ) {
	if ( !renderEntity.hModel || !surface || !*surface ) {
		return;
	}

	renderEntity.suppressSurfaceMask &= (~renderEntity.hModel->GetSurfaceMask ( surface ) );
}

/*
================
anEntity::Event_ShowSurface
================
*/
void anEntity::Event_ShowSurface ( const char *surface ) {
	ShowSurface ( surface );
}

/*
================
anEntity::HideSurface
================
*/
void anEntity::HideSurface ( const char *surface ) {
	if ( !renderEntity.hModel || !surface || !*surface ) {
		return;
	}

	renderEntity.suppressSurfaceMask |= renderEntity.hModel->GetSurfaceMask ( surface ) ;
}

/*
================
anEntity::Event_HideSurface
================
*/
void anEntity::Event_HideSurface ( const char *surface ) {
	HideSurface ( surface );
}

/*
================
anEntity::Event_GuiEvent
================
*/
void anEntity::Event_GuiEvent ( const char *eventName ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->HandleNamedEvent ( eventName );
	}
}

/*
================
anEntity::Event_clearSkin
================
*/
void anEntity::Event_ClearSkin( void ) {
	ClearSkin();
}

/*
================
anEntity::Event_StopAllEffects
================
*/
void anEntity::Event_StopAllEffects ( void ) {
	StopAllEffects();
}

/*
================
anEntity::Event_GetHealth
================
*/
void anEntity::Event_GetHealth ( void ) {
	anThread::ReturnFloat( health );
}

// jscott:
/*
================
anEntity::Event_PlaybackCallback
================
*/
void anEntity::Event_PlaybackCallback ( int type, int changed, int impulse ) {
	common->Printf( "Playback callback type %d - %d/%d\n", type, changed, impulse );
}

// nmckenzie: Check who we're bound to.
/*
================
anEntity::Event_GetBindMaster
================
*/

void anEntity::Event_GetBindMaster ( void ) {
	anThread::ReturnEntity( GetBindMaster() );
}

/*
================
anEntity::Event_ApplyImpulse
================
*/

void anEntity::Event_ApplyImpulse( anEntity *source, const anVec3 &point, const anVec3 &impulse ){
	ApplyImpulse( source, 0, point, impulse );
}

/*
================
anEntity::Event_PlayEffect
================
*/
void anEntity::Event_PlayEffect( const char *effectName, const char *jointName, bool loop ) {
	jointHandle_t joint;
	joint = GetAnimator() ? GetAnimator()->GetJointHandle ( jointName ) : INVALID_JOINT;
	if ( joint != INVALID_JOINT ) {
		PlayEffect ( effectName, joint, loop );
	} else {
		PlayEffect ( effectName, renderEntity.origin, renderEntity.axis, loop );
	}
}

/*
================
anEntity::Event_StopEffect
================
*/
void anEntity::Event_StopEffect( const char *effectName ) {
	StopEffect ( effectName );
}

// END RAVEN

/***********************************************************************

  Misc.

***********************************************************************/

/*
================
anEntity::Teleport
================
*/
void anEntity::Teleport( const anVec3 &origin, const anAngles &angles, anEntity *destination ) {
	GetPhysics()->SetOrigin( origin );
	GetPhysics()->SetAxis( angles.ToMat3() );

	UpdateVisuals();
}

/*
============
anEntity::TouchTriggers

  Activate all trigger entities touched at the current position.

  Optionally only activate triggers of ownerType
============
*/
bool anEntity::TouchTriggers( const idTypeInfo* ownerType ) const {
	int				i, numClipModels, numEntities;
	anClipModel *	cm;
	anClipModel *	clipModels[ MAX_GENTITIES ];
	anEntity *		ent;
	trace_t			trace;

	memset( &trace, 0, sizeof( trace ) );
	trace.endpos = GetPhysics()->GetOrigin();
	trace.endAxis = GetPhysics()->GetAxis();



	numClipModels = gameLocal.ClipModelsTouchingBounds( this, GetPhysics()->GetAbsBounds(), CONTENTS_TRIGGER_SEAS, clipModels, MAX_GENTITIES );

	numEntities = 0;

	for ( i = 0; i < numClipModels; i++ ) {
		cm = clipModels[i];

		// don't touch it if we're the owner
		if ( cm->GetOwner() == this ) {
			continue;
		}

		ent = cm->GetEntity();

		if ( !ent->RespondsTo( EV_Touch ) && !ent->HasSignal( SIG_TOUCH ) ) {
			continue;
		}

		if ( ownerType && !ent->IsType( *ownerType ) ) {
			continue;
		}


// abahr: needed so tram car can has collision model and touch triggers
		bool useSimpleClip = spawnArgs.GetBool( "useSimpleTriggerClip" );
		if ( !useSimpleClip && !GetPhysics()->ClipContents( cm ) ) {

			continue;
		}

		numEntities++;

		trace.c.contents = cm->GetContents();
		trace.c.entityNum = cm->GetEntity()->entityNumber;
		trace.c.id = cm->GetId();

		ent->Signal( SIG_TOUCH );
		ent->ProcessEvent( &EV_Touch, this, &trace );

		if ( !gameLocal.entities[ entityNumber ] ) {
			gameLocal.Printf( "entity was removed while touching triggers\n" );
			return true;
		}
	}

	return ( numEntities != 0 );
}

/*
================
anEntity::GetSpline
================
*/
anCurve_Spline<anVec3> *anEntity::GetSpline( void ) const {
	int i, numPoints, t;
	const anKeyValue *kv;
	anLexer lex;
	anVec3 v;
	anCurve_Spline<anVec3> *spline;
	const char *curveTag = "curve_";

	kv = spawnArgs.MatchPrefix( curveTag );
	if ( !kv ) {
		return nullptr;
	}


// mwhitlock: Dynamic memory consolidation
	PushHeapMemory(this);

	anStr str = kv->GetKey().Right( kv->GetKey().Length() - strlen( curveTag ) );
	if ( str.Icmp( "CatmullRomSpline" ) == 0 ) {
		spline = new anCurve_CatmullRomSpline<anVec3>();
	} else if ( str.Icmp( "nubs" ) == 0 ) {
		spline = new anCurve_NonUniformBSpline<anVec3>();
	} else if ( str.Icmp( "nurbs" ) == 0 ) {
		spline = new anCurve_NURBS<anVec3>();
	} else {
		spline = new anCurve_BSpline<anVec3>();
	}

// mwhitlock: Dynamic memory consolidation
	PopSystemHeap();


	spline->SetBoundaryType( anCurve_Spline<anVec3>::BT_CLAMPED );

	lex.LoadMemory( kv->GetValue(), kv->GetValue().Length(), curveTag );
	numPoints = lex.ParseInt();
	lex.ExpectTokenString( "( " );
	for ( t = i = 0; i < numPoints; i++, t += 100 ) {
		v.x = lex.ParseFloat();
		v.y = lex.ParseFloat();
		v.z = lex.ParseFloat();
		spline->AddValue( t, v );
	}
	lex.ExpectTokenString( " )" );

	return spline;
}

/*
===============
anEntity::ShowEditingDialog
===============
*/
void anEntity::ShowEditingDialog( void ) {
}

/***********************************************************************

   Events

***********************************************************************/

/*
================
anEntity::Event_GetName
================
*/
void anEntity::Event_GetName( void ) {
	anThread::ReturnString( name.c_str() );
}

/*
================
anEntity::Event_SetName
================
*/
void anEntity::Event_SetName( const char *newname ) {
	SetName( newname );
}

/*
===============
anEntity::Event_FindTargets
===============
*/
void anEntity::Event_FindTargets( void ) {
	FindTargets();
}

/*
============
anEntity::Event_ActivateTargets

Activates any entities targeted by this entity.  Mainly used as an
event to delay activating targets.
============
*/
void anEntity::Event_ActivateTargets( anEntity *activator ) {
	ActivateTargets( activator );
}


// jshepard: added
/*
============
anEntity::Event_UnbindTargets

Unbinds all targets of this entity. Useful to make held or clamped items
drop when shot, and for breakable walls.
============
*/
void anEntity::Event_UnbindTargets( anEntity *activator ) {
	UnbindTargets( activator );
}




/*
================
anEntity::Event_NumTargets
================
*/
void anEntity::Event_NumTargets( void ) {
	anThread::ReturnFloat( targets.Num() );
}

/*
================
anEntity::Event_GetTarget
================
*/
void anEntity::Event_GetTarget( float index ) {
	int i;

	i = ( int )index;
	if ( ( i < 0 ) || i >= targets.Num() ) {
		anThread::ReturnEntity( nullptr );
	} else {
		anThread::ReturnEntity( targets[i].GetEntity() );
	}
}

/*
================
anEntity::Event_RandomTarget
================
*/
void anEntity::Event_RandomTarget( const char *ignore ) {
	int			num;
	anEntity	*ent;
	int			i;
	int			ignoreNum;

	RemoveNullTargets();
	if ( !targets.Num() ) {
		anThread::ReturnEntity( nullptr );
		return;
	}

	ignoreNum = -1;
	if ( ignore && ( ignore[0] != 0 ) && ( targets.Num() > 1 ) ) {
		for ( i = 0; i < targets.Num(); i++ ) {
			ent = targets[i].GetEntity();
			if ( ent && ( ent->name == ignore ) ) {
				ignoreNum = i;
				break;
			}
		}
	}

	if ( ignoreNum >= 0 ) {
		num = gameLocal.random.RandomInt( targets.Num() - 1 );
		if ( num >= ignoreNum ) {
			num++;
		}
	} else {
		num = gameLocal.random.RandomInt( targets.Num() );
	}

	ent = targets[ num ].GetEntity();
	anThread::ReturnEntity( ent );
}


// abahr: so we can call this from script
/*
================
anEntity::Event_RemoveNullTargets
================
*/
void anEntity::Event_RemoveNullTargets() {
	RemoveNullTargets();
}

// twhitaker: So targets can be added from script
/*
================
anEntity::Event_AppendTarget
================
*/
void anEntity::Event_AppendTarget( anEntity *appendMe ) {
	anThread::ReturnFloat( AppendTarget( appendMe ) );
}

/*
================
anEntity::Event_RemoveTarget
================
*/
void anEntity::Event_RemoveTarget( anEntity *removeMe ) {
	RemoveTarget( removeMe );
}

/*
================
anEntity::Event_ClearTargetList
================
*/
void anEntity::Event_ClearTargetList( float destroyContents ) {
	RemoveTargets( destroyContents != 0.0f );
}

/*
================
anEntity::Event_MatchPrefix
================
*/
void anEntity::Event_MatchPrefix( const char *prefix, const char *previousKey ) {
	const anKeyValue* kv = (previousKey[0]) ? spawnArgs.FindKey(previousKey) : nullptr;

	kv = spawnArgs.MatchPrefix( prefix, kv );
	if ( !kv || !kv->GetValue() ) {
		anThread::ReturnString( "" );
		return;
	}

	anThread::ReturnString( kv->GetKey() );
}

/*
================
anEntity::Event_IsA
================
*/
void anEntity::Event_IsA( const char *entityDefName ) {
	const anDict* dict = gameLocal.FindEntityDefDict( entityDefName );
	if ( !dict ) {
		anThread::ReturnFloat( false );
		return;
	}

	idTypeInfo* info = anClass::GetClass( dict->GetString( "spawnclass" ) );
	if ( !info ) {
		anThread::ReturnFloat( false );
		return;
	}

	anThread::ReturnFloat( IsType(*info) );
}

/*
================
anEntity::Event_IsSameTypeAs
================
*/
void anEntity::Event_IsSameTypeAs( const anEntity *ent ) {
	assert( ent );

	anThread::ReturnFloat( IsType(ent->Type) );
}

// mekberg: allow sethealth on all entities.
// jshepard: removed clamping
/*
================
anEntity::Event_SetHealth
================
*/
void anEntity::Event_SetHealth( float newHealth ) {
	health =  newHealth;
}


/*
================
anEntity::Event_BindToJoint
================
*/
void anEntity::Event_BindToJoint( anEntity *master, const char *jointname, float orientated ) {
	BindToJoint( master, jointname, ( orientated != 0.0f ) );
}

/*
================
anEntity::Event_RemoveBinds
================
*/
void anEntity::Event_RemoveBinds( void ) {
	RemoveBinds();
}

/*
================
anEntity::Event_Bind
================
*/
void anEntity::Event_Bind( anEntity *master ) {
	Bind( master, true );
}

/*
================
anEntity::Event_BindPosition
================
*/
void anEntity::Event_BindPosition( anEntity *master ) {
	Bind( master, false );
}

/*
================
anEntity::Event_Unbind
================
*/
void anEntity::Event_Unbind( void ) {
	Unbind();
}

/*
================
anEntity::Event_SpawnBind
================
*/
void anEntity::Event_SpawnBind( void ) {
	anEntity		*parent;
	const char		*bind, *joint, *bindanim;
	jointHandle_t	bindJoint;
	bool			bindOrientated;
	int				id;
	const idAnim	*anim;
	int				animNum;
	anAnimator		*parentAnimator;

	if ( spawnArgs.GetString( "bind", "", &bind ) ) {
		if ( anStr::Icmp( bind, "worldspawn" ) == 0 ) {
			//FIXME: Completely unneccessary since the worldspawn is called "world"
			parent = gameLocal.world;
		} else {
			parent = gameLocal.FindEntity( bind );
		}
		bindOrientated = spawnArgs.GetBool( "bindOrientated", "1" );
		if ( parent ) {
			// bind to a joint of the skeletal model of the parent
			if ( spawnArgs.GetString( "bindToJoint", "", &joint ) && *joint ) {
				parentAnimator = parent->GetAnimator();
				if ( !parentAnimator ) {
					gameLocal.Error( "Cannot bind to joint '%s' on '%s'.  Entity does not support skeletal models.", joint, name.c_str() );
				}
				bindJoint = parentAnimator->GetJointHandle( joint );
				if ( bindJoint == INVALID_JOINT ) {
					gameLocal.Error( "Joint '%s' not found for bind on '%s'", joint, name.c_str() );
				}

				// bind it relative to a specific anim
				if ( ( parent->spawnArgs.GetString( "bindanim", "", &bindanim ) || parent->spawnArgs.GetString( "anim", "", &bindanim ) ) && *bindanim ) {
					animNum = parentAnimator->GetAnim( bindanim );
					if ( !animNum ) {
						gameLocal.Error( "Anim '%s' not found for bind on '%s'", bindanim, name.c_str() );
					}
					anim = parentAnimator->GetAnim( animNum );
					if ( !anim ) {
						gameLocal.Error( "Anim '%s' not found for bind on '%s'", bindanim, name.c_str() );
					}

					// make sure parent's render origin has been set
					parent->UpdateModelTransform();

					//FIXME: need a BindToJoint that accepts a joint position
					parentAnimator->CreateFrame( gameLocal.time, true );
					anJointMat *frame = parent->renderEntity.joints;
					gameEdit->ANIM_CreateAnimFrame( parentAnimator->ModelHandle(), anim->MD5Anim( 0 ), parent->renderEntity.numJoints, frame, 0, parentAnimator->ModelDef()->GetVisualOffset(), parentAnimator->RemoveOrigin() );
					BindToJoint( parent, joint, bindOrientated );
					parentAnimator->ForceUpdate();
				} else {
					BindToJoint( parent, joint, bindOrientated );
				}
			}
			// bind to a body of the physics object of the parent
			else if ( spawnArgs.GetInt( "bindToBody", "0", id ) ) {
				BindToBody( parent, id, bindOrientated );
			}
			// bind to the parent
			else {
				Bind( parent, bindOrientated );
			}
		}
	}
}

/*
================
anEntity::Event_SetOwner
================
*/
void anEntity::Event_SetOwner( anEntity *owner ) {
	int i;

	for ( i = 0; i < GetPhysics()->GetNumClipModels(); i++ ) {
		GetPhysics()->GetClipModel( i )->SetOwner( owner );
	}
}

/*
================
anEntity::Event_SetModel
================
*/
void anEntity::Event_SetModel( const char *modelname ) {
	SetModel( modelname );
}

/*
================
anEntity::Event_SetSkin
================
*/
void anEntity::Event_SetSkin( const char *skinname ) {
	renderEntity.customSkin = declManager->FindSkin( skinname );
	UpdateVisuals();
}

/*
================
anEntity::Event_GetShaderParm
================
*/
void anEntity::Event_GetShaderParm( int parmnum ) {
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) ) {
		gameLocal.Error( "shader parm index (%d) out of range", parmnum );
	}

	anThread::ReturnFloat( renderEntity.shaderParms[ parmnum ] );
}

/*
================
anEntity::Event_SetShaderParm
================
*/
void anEntity::Event_SetShaderParm( int parmnum, float value ) {
	SetShaderParm( parmnum, value );
}

/*
================
anEntity::Event_SetShaderParms
================
*/
void anEntity::Event_SetShaderParms( float parm0, float parm1, float parm2, float parm3 ) {
	renderEntity.shaderParms[ SHADERPARM_RED ]		= parm0;
	renderEntity.shaderParms[ SHADERPARM_GREEN ]	= parm1;
	renderEntity.shaderParms[ SHADERPARM_BLUE ]		= parm2;
	renderEntity.shaderParms[ SHADERPARM_ALPHA ]	= parm3;
	UpdateVisuals();
}


/*
================
anEntity::Event_SetColor
================
*/
void anEntity::Event_SetColor( float red, float green, float blue ) {
	SetColor( red, green, blue );
}

/*
================
anEntity::Event_GetColor
================
*/
void anEntity::Event_GetColor( void ) {
	anVec3 out;

	GetColor( out );
	anThread::ReturnVector( out );
}

/*
================
anEntity::Event_IsHidden
================
*/
void anEntity::Event_IsHidden( void ) {
	anThread::ReturnInt( fl.hidden );
}

/*
================
anEntity::Event_Hide
================
*/
void anEntity::Event_Hide( void ) {
	Hide();
}

/*
================
anEntity::Event_Show
================
*/
void anEntity::Event_Show( void ) {
	Show();
}

/*
================
anEntity::Event_CacheSoundShader
================
*/
void anEntity::Event_CacheSoundShader( const char *soundName ) {
	declManager->FindSound( soundName );
}

/*
================
anEntity::Event_StartSoundShader
================
*/
void anEntity::Event_StartSoundShader( const char *soundName, int channel ) {
	int length;

	StartSoundShader( declManager->FindSound( soundName ), ( s_channelType)channel, 0, false, &length );
	anThread::ReturnFloat( MS2SEC( length ) );
}

/*
================
anEntity::Event_StopSound
================
*/
void anEntity::Event_StopSound( int channel, int netSync ) {
	StopSound( channel, ( netSync != 0 ) );
}

/*
================
anEntity::Event_StartSound
================
*/
void anEntity::Event_StartSound( const char *soundName, int channel, int netSync ) {
	int time;

	StartSound( soundName, ( s_channelType )channel, 0, ( netSync != 0 ), &time );
	anThread::ReturnFloat( MS2SEC( time ) );
}

/*
================
anEntity::Event_FadeSound
================
*/
void anEntity::Event_FadeSound( int channel, float to, float over ) {

	idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
	if ( emitter ) {
		emitter->FadeSound( channel, to, over );
	}

}

/*
================
anEntity::Event_GetWorldOrigin
================
*/
void anEntity::Event_GetWorldOrigin( void ) {
	anThread::ReturnVector( GetPhysics()->GetOrigin() );
}

/*
================
anEntity::Event_SetWorldOrigin
================
*/
void anEntity::Event_SetWorldOrigin( anVec3 const &org ) {
	anVec3 neworg = GetLocalCoordinates( org );
	SetOrigin( neworg );
}

/*
================
anEntity::Event_SetOrigin
================
*/
void anEntity::Event_SetOrigin( anVec3 const &org ) {
	SetOrigin( org );
}

/*
================
anEntity::Event_GetOrigin
================
*/
void anEntity::Event_GetOrigin( void ) {
	anThread::ReturnVector( GetLocalCoordinates( GetPhysics()->GetOrigin() ) );
}

/*
================
anEntity::Event_SetAngles
================
*/
void anEntity::Event_SetAngles( anAngles const &ang ) {
	SetAngles( ang );
}

/*
================
anEntity::Event_GetAngles
================
*/
void anEntity::Event_GetAngles( void ) {
	anAngles ang = GetPhysics()->GetAxis().ToAngles();
	anThread::ReturnVector( anVec3( ang[0], ang[1], ang[2] ) );
}

/*
================
anEntity::Event_SetLinearVelocity
================
*/
void anEntity::Event_SetLinearVelocity( const anVec3 &velocity ) {
	GetPhysics()->SetLinearVelocity( velocity );
}

/*
================
anEntity::Event_GetLinearVelocity
================
*/
void anEntity::Event_GetLinearVelocity( void ) {
	anThread::ReturnVector( GetPhysics()->GetLinearVelocity() );
}

/*
================
anEntity::Event_SetAngularVelocity
================
*/
void anEntity::Event_SetAngularVelocity( const anVec3 &velocity ) {
	GetPhysics()->SetAngularVelocity( velocity );
}

/*
================
anEntity::Event_GetAngularVelocity
================
*/
void anEntity::Event_GetAngularVelocity( void ) {
	anThread::ReturnVector( GetPhysics()->GetAngularVelocity() );
}

/*
================
anEntity::Event_SetSize
================
*/
void anEntity::Event_SetSize( anVec3 const &mins, anVec3 const &maxs ) {
	GetPhysics()->SetClipBox( anBounds( mins, maxs ), 1.0f );
}

/*
================
anEntity::Event_GetSize
================
*/
void anEntity::Event_GetSize( void ) {
	anBounds bounds;

	bounds = GetPhysics()->GetBounds();
	anThread::ReturnVector( bounds[1] - bounds[0] );
}

/*
================
anEntity::Event_GetMins
================
*/
void anEntity::Event_GetMins( void ) {
	anThread::ReturnVector( GetPhysics()->GetBounds()[0] );
}

/*
================
anEntity::Event_GetMaxs
================
*/
void anEntity::Event_GetMaxs( void ) {
	anThread::ReturnVector( GetPhysics()->GetBounds()[1] );
}

/*
================
anEntity::Event_Touches
================
*/
void anEntity::Event_Touches( anEntity *ent ) {
	if ( !ent ) {
		anThread::ReturnInt( false );
		return;
	}

	const anBounds &myBounds = GetPhysics()->GetAbsBounds();
	const anBounds &entBounds = ent->GetPhysics()->GetAbsBounds();

	anThread::ReturnInt( myBounds.IntersectsBounds( entBounds ) );
}

/*
================
anEntity::Event_SetGuiParm
================
*/
void anEntity::Event_SetGuiParm( const char *key, const char *val ) {

// mekberg: added
	anStr temp = key;
	for ( int i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		if ( renderEntity.gui[i] ) {
			if ( anStr::Icmpn( key, "gui_", 4 ) ) {
				temp.Insert( "gui_", 0 );
			}
			spawnArgs.Set( temp.c_str(), val );


			renderEntity.gui[i]->SetStateString( key, val );
			renderEntity.gui[i]->StateChanged( gameLocal.time );
		}
	}
}

/*
================
anEntity::Event_SetGuiParm
================
*/
void anEntity::Event_SetGuiFloat( const char *key, float f ) {
	for ( int i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		if ( renderEntity.gui[i] ) {
			renderEntity.gui[i]->SetStateString( key, va( "%f", f ) );
			renderEntity.gui[i]->StateChanged( gameLocal.time );
		}
	}
}

/*
================
anEntity::Event_GetNextKey
================
*/
void anEntity::Event_GetNextKey( const char *prefix, const char *lastMatch ) {
	const anKeyValue *kv;
	const anKeyValue *previous;

	if ( *lastMatch ) {
		previous = spawnArgs.FindKey( lastMatch );
	} else {
		previous = nullptr;
	}

	kv = spawnArgs.MatchPrefix( prefix, previous );
	if ( !kv ) {
		anThread::ReturnString( "" );
	} else {
		anThread::ReturnString( kv->GetKey() );
	}
}

/*
================
anEntity::Event_SetKey
================
*/
void anEntity::Event_SetKey( const char *key, const char *value ) {
	spawnArgs.Set( key, value );
}

/*
================
anEntity::Event_GetKey
================
*/
void anEntity::Event_GetKey( const char *key ) {
	const char *value;

	spawnArgs.GetString( key, "", &value );
	anThread::ReturnString( value );
}

/*
================
anEntity::Event_GetIntKey
================
*/
void anEntity::Event_GetIntKey( const char *key ) {
	int value;

	spawnArgs.GetInt( key, "0", value );

	// scripts only support floats
	anThread::ReturnFloat( value );
}

/*
================
anEntity::Event_GetFloatKey
================
*/
void anEntity::Event_GetFloatKey( const char *key ) {
	float value;

	spawnArgs.GetFloat( key, "0", value );
	anThread::ReturnFloat( value );
}

/*
================
anEntity::Event_GetVectorKey
================
*/
void anEntity::Event_GetVectorKey( const char *key ) {
	anVec3 value;

	spawnArgs.GetVector( key, "0 0 0", value );
	anThread::ReturnVector( value );
}

/*
================
anEntity::Event_GetEntityKey
================
*/
void anEntity::Event_GetEntityKey( const char *key ) {
	anEntity *ent;
	const char *entname;

	if ( !spawnArgs.GetString( key, nullptr, &entname ) ) {
		anThread::ReturnEntity( nullptr );
		return;
	}

	ent = gameLocal.FindEntity( entname );
	if ( !ent ) {
		gameLocal.Warning( "Couldn't find entity '%s' specified in '%s' key in entity '%s'", entname, key, name.c_str() );
	}

	anThread::ReturnEntity( ent );
}

/*
================
anEntity::Event_RestorePosition
================
*/
void anEntity::Event_RestorePosition( void ) {
	anVec3		org;
	anAngles	angles;
	anMat3		axis;
	anEntity *	part;

	spawnArgs.GetVector( "origin", "0 0 0", org );

	// get the rotation matrix in either full form, or single angle form
	if ( spawnArgs.GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1", axis ) ) {
		angles = axis.ToAngles();
	} else {
   		angles[0] = 0;
   		angles[1] = spawnArgs.GetFloat( "angle" );
   		angles[2] = 0;
	}

	Teleport( org, angles, nullptr );

	for ( part = teamChain; part != nullptr; part = part->teamChain ) {
		if ( part->bindMaster != this ) {
			continue;
		}


		if ( part->GetPhysics()->IsType( anPhysics_Parametric::GetClassType() ) ) {
			if ( static_cast<anPhysics_Parametric *>(part->GetPhysics())->IsPusher() ) {
				gameLocal.Warning( "teleported '%s' which has the pushing mover '%s' bound to it\n", GetName(), part->GetName() );
			}
		} else if ( part->GetPhysics()->IsType( anPhysics_AF::GetClassType() ) ) {

			gameLocal.Warning( "teleported '%s' which has the articulated figure '%s' bound to it\n", GetName(), part->GetName() );
		}
	}
}

/*
================
anEntity::Event_UpdateCameraTarget
================
*/
void anEntity::Event_UpdateCameraTarget( void ) {
	const char *target;
	const anKeyValue *kv;
	anVec3 dir;

	target = spawnArgs.GetString( "cameraTarget" );

	cameraTarget = gameLocal.FindEntity( target );

	if ( cameraTarget ) {
		kv = cameraTarget->spawnArgs.MatchPrefix( "target", nullptr );
		while( kv ) {
			anEntity *ent = gameLocal.FindEntity( kv->GetValue() );
			if ( ent && anStr::Icmp( ent->GetEntityDefName(), "target_null" ) == 0) {
				dir = ent->GetPhysics()->GetOrigin() - cameraTarget->GetPhysics()->GetOrigin();
				dir.Normalize();
				cameraTarget->SetAxis( dir.ToMat3() );

// rjohnson: if you have a func_cameraview pointing to an info_null via "cameratarget" and
//			 you have a func_static pointing to the func_cameraview via a "cameratarget" then
//			 the func_static evaluates 'target' to the func_cameraview and its target it the info null
//			 the SexAxis() is then applied to the the func_static rather than the func_cameraview
//				SetAxis(dir.ToMat3());

				break;
			}
			kv = cameraTarget->spawnArgs.MatchPrefix( "target", kv );
		}
	}
	UpdateVisuals();
}

/*
================
anEntity::Event_DistanceTo
================
*/
void anEntity::Event_DistanceTo( anEntity *ent ) {
	if ( !ent ) {
		// just say it's really far away
		anThread::ReturnFloat( MAX_WORLD_SIZE );
	} else {
		float dist = ( GetPhysics()->GetOrigin() - ent->GetPhysics()->GetOrigin() ).LengthFast();
		anThread::ReturnFloat( dist );
	}
}

/*
================
anEntity::Event_DistanceToPoint
================
*/
void anEntity::Event_DistanceToPoint( const anVec3 &point ) {
	float dist = ( GetPhysics()->GetOrigin() - point ).LengthFast();
	anThread::ReturnFloat( dist );
}

/*
================
anEntity::Event_StartFx
================
*/
void anEntity::Event_StartFx( const char *fx ) {

// bdube: not used
//	anEntityFx::StartFx( fx, nullptr, nullptr, this, true );

}

/*
================
anEntity::Event_WaitFrame
================
*/
void anEntity::Event_WaitFrame( void ) {
	anThread *thread;

	thread = anThread::CurrentThread();
	if ( thread ) {
		thread->WaitFrame();
	}
}

/*
=====================
anEntity::Event_Wait
=====================
*/
void anEntity::Event_Wait( float time ) {
	anThread *thread = anThread::CurrentThread();

	if ( !thread ) {
		gameLocal.Error( "Event 'wait' called from outside thread" );
	}

	thread->WaitSec( time );
}

/*
=====================
anEntity::Event_HasFunction
=====================
*/
void anEntity::Event_HasFunction( const char *name ) {
	const function_t *func;

	func = scriptObject.GetFunction( name );
	if ( func ) {
		anThread::ReturnInt( true );
	} else {
		anThread::ReturnInt( false );
	}
}

/*
=====================
anEntity::Event_CallFunction
=====================
*/
void anEntity::Event_CallFunction( const char *funcname ) {

// bdube: states
	stateParms_t parms = {0};
	if ( ProcessState ( funcname, parms ) != SRESULT_ERROR ) {
		return;
	}
	gameLocal.CallObjectFrameCommand ( this, funcname );

}

/*
================
anEntity::Event_SetNeverDormant
================
*/
void anEntity::Event_SetNeverDormant( int enable ) {
	fl.neverDormant	= ( enable != 0 );
	dormantStart = 0;
}

/***********************************************************************

   Network

***********************************************************************/

/*
================
anEntity::ClientPredictionThink
================
*/
void anEntity::ClientPredictionThink( void ) {
	RunPhysics();
	Present();
}

/*
================
anEntity::WriteBindToSnapshot
================
*/
void anEntity::WriteBindToSnapshot( anBitMsgDelta &msg ) const {
	int bindInfo;

	if ( bindMaster ) {
		bindInfo = bindMaster->entityNumber;
		bindInfo |= ( fl.bindOrientated & 1 ) << GENTITYNUM_BITS;
		if ( bindJoint != INVALID_JOINT ) {
			bindInfo |= 1 << ( GENTITYNUM_BITS + 1 );
			bindInfo |= bindJoint << ( 3 + GENTITYNUM_BITS );
		} else if ( bindBody != -1 ) {
			bindInfo |= 2 << ( GENTITYNUM_BITS + 1 );
			bindInfo |= bindBody << ( 3 + GENTITYNUM_BITS );
		}
	} else {
		bindInfo = ENTITYNUM_NONE;
	}
	msg.WriteBits( bindInfo, GENTITYNUM_BITS + 3 + 9 );
}

/*
================
anEntity::ReadBindFromSnapshot
================
*/
void anEntity::ReadBindFromSnapshot( const anBitMsgDelta &msg ) {
	int bindInfo, bindEntityNum, bindPos;
	bool bindOrientated;
	anEntity *master;

	bindInfo = msg.ReadBits( GENTITYNUM_BITS + 3 + 9 );
	bindEntityNum = bindInfo & ( ( 1 << GENTITYNUM_BITS ) - 1 );

	if ( bindEntityNum != ENTITYNUM_NONE ) {
		master = gameLocal.entities[ bindEntityNum ];

		bindOrientated = ( bindInfo >> GENTITYNUM_BITS ) & 1;
		bindPos = ( bindInfo >> ( GENTITYNUM_BITS + 3 ) );
		switch ( ( bindInfo >> ( GENTITYNUM_BITS + 1 ) ) & 3 ) {
			case 1: {
				BindToJoint( master, (jointHandle_t) bindPos, bindOrientated );
				break;
			}
			case 2: {
				BindToBody( master, bindPos, bindOrientated );
				break;
			}
			default: {
				Bind( master, bindOrientated );
				break;
			}
		}
	} else if ( bindMaster ) {
		Unbind();
	}
}

/*
================
anEntity::WriteColorToSnapshot
================
*/
void anEntity::WriteColorToSnapshot( anBitMsgDelta &msg ) const {
	anVec4 color;

	color[0] = renderEntity.shaderParms[ SHADERPARM_RED ];
	color[1] = renderEntity.shaderParms[ SHADERPARM_GREEN ];
	color[2] = renderEntity.shaderParms[ SHADERPARM_BLUE ];
	color[3] = renderEntity.shaderParms[ SHADERPARM_ALPHA ];
	msg.WriteLong( PackColor( color ) );
}

/*
================
anEntity::ReadColorFromSnapshot
================
*/
void anEntity::ReadColorFromSnapshot( const anBitMsgDelta &msg ) {
	anVec4 color;

	UnpackColor( msg.ReadLong(), color );
	renderEntity.shaderParms[ SHADERPARM_RED ] = color[0];
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = color[1];
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = color[2];
	renderEntity.shaderParms[ SHADERPARM_ALPHA ] = color[3];
}

/*
================
anEntity::WriteGUIToSnapshot
================
*/
void anEntity::WriteGUIToSnapshot( anBitMsgDelta &msg ) const {
	// no need to loop over MAX_RENDERENTITY_GUI at this time
	if ( renderEntity.gui[0] ) {
		msg.WriteByte( renderEntity.gui[0]->State().GetInt( "networkState" ) );
	} else {
		msg.WriteByte( 0 );
	}
}

/*
================
anEntity::ReadGUIFromSnapshot
================
*/
void anEntity::ReadGUIFromSnapshot( const anBitMsgDelta &msg ) {
	int state;
	anUserInterface *gui;
	state = msg.ReadByte( );
	gui = renderEntity.gui[0];
	if ( gui && state != mpGUIState ) {
		mpGUIState = state;
		gui->SetStateInt( "networkState", state );
		gui->HandleNamedEvent( "networkState" );
	}
}

/*
================
anEntity::WriteToSnapshot
================
*/
void anEntity::WriteToSnapshot( anBitMsgDelta &msg ) const {
}

/*
================
anEntity::ReadFromSnapshot
================
*/
void anEntity::ReadFromSnapshot( const anBitMsgDelta &msg ) {
}

/*
================
anEntity::ServerSendEvent

   Saved events are also sent to any client that connects late so all clients
   always receive the events nomatter what time they join the game.
   ================
   */
void anEntity::ServerSendEvent( int eventId, const anBitMsg *msg, bool saveEvent, int excludeClient ) const {
	anBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	if ( !gameLocal.isServer ) {
		return;
	}

	// prevent dupe events caused by frame re-runs
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_EVENT );
	outMsg.WriteBits( gameLocal.GetSpawnId( this ), 32 );
	outMsg.WriteByte( eventId );
	outMsg.WriteLong( gameLocal.time );
	if ( msg ) {
		outMsg.WriteBits( msg->GetSize(), anMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
		outMsg.WriteData( msg->GetData(), msg->GetSize() );
	} else {
		outMsg.WriteBits( 0, anMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
	}

	networkSystem->ServerSendReliableMessageExcluding( excludeClient, outMsg );

	if ( saveEvent ) {
		gameLocal.Error( "Unsupported saveEvent == true in anEntity::ServerSendEvent" );
	}
}

/*
================
anEntity::ServerSendInstanceEvent
================
*/
void anEntity::ServerSendInstanceEvent( int eventId, const anBitMsg *msg, bool saveEvent, int excludeClient ) const {
	anBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	if ( !gameLocal.isServer ) {
		return;
	}

	// prevent dupe events caused by frame re-runs
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_EVENT );
	outMsg.WriteBits( gameLocal.GetSpawnId( this ), 32 );
	outMsg.WriteByte( eventId );
	outMsg.WriteLong( gameLocal.time );
	if ( msg ) {
		outMsg.WriteBits( msg->GetSize(), anMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
		outMsg.WriteData( msg->GetData(), msg->GetSize() );
	} else {
		outMsg.WriteBits( 0, anMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
	}

	gameLocal.ServerSendInstanceReliableMessageExcluding( this, excludeClient, outMsg );

	if ( saveEvent ) {
		gameLocal.Error( "Unsupported saveEvent == true in anEntity::ServerSendEvent" );
	}
}

/*
================
anEntity::ClientSendEvent
================
*/
void anEntity::ClientSendEvent( int eventId, const anBitMsg *msg ) const {
	anBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	if ( !gameLocal.isClient ) {
		return;
	}

	// prevent dupe events caused by frame re-runs
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.BeginWriting();
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_EVENT );
	outMsg.WriteBits( gameLocal.GetSpawnId( this ), 32 );
	outMsg.WriteByte( eventId );
	outMsg.WriteLong( gameLocal.time );
	if ( msg ) {
		outMsg.WriteBits( msg->GetSize(), anMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
		outMsg.WriteData( msg->GetData(), msg->GetSize() );
	} else {
		outMsg.WriteBits( 0, anMath::BitsForInteger( MAX_EVENT_PARAM_SIZE ) );
	}

	networkSystem->ClientSendReliableMessage( outMsg );
}

/*
================
anEntity::ServerReceiveEvent
================
*/
bool anEntity::ServerReceiveEvent( int event, int time, const anBitMsg &msg ) {
	switch ( event ) {
		case 0: {
		}
		default: {
			return false;
		}
	}
}

/*
================
anEntity::ClientReceiveEvent
================
*/
bool anEntity::ClientReceiveEvent( int event, int time, const anBitMsg &msg ) {
	const idSoundShader	*shader;
	s_channelType		channel;

	switch ( event ) {
		case EVENT_STARTSOUNDSHADER: {
			// the sound stuff would early out
			assert( gameLocal.isNewFrame );
			if ( time < gameLocal.realClientTime - 300 ) {
				// too old, skip it
				common->DPrintf( "ent 0x%x: start sound shader too old (%d ms)\n", entityNumber, gameLocal.realClientTime - time );
				return true;
			}
			shader = static_cast< const idSoundShader* >( idGameLocal::ReadDecl( msg, DECL_SOUND ) );
			channel = ( s_channelType)msg.ReadByte();
			StartSoundShader( shader, channel, 0, false, nullptr );
			return true;
		}
		case EVENT_STOPSOUNDSHADER: {
			// the sound stuff would early out
			assert( gameLocal.isNewFrame );
			channel = ( s_channelType)msg.ReadByte();
			StopSound( channel, false );
			return true;
		}

// bdube: new events
		case EVENT_PLAYEFFECT_JOINT: {
			const idDecl*		effect;
			anCQuat				quat;
			anVec3				origin;
			rvClientEffect*		clientEffect;
			effectCategory_t	category;
			jointHandle_t		jointHandle;
			bool				loop;

			// TMP - not quite sure this is still used for anything
			common->Warning( "FIXME: anEntity::PlayEffect happens" );
			assert( false );

			effect = idGameLocal::ReadDecl( msg, DECL_EFFECT );
			jointHandle = ( jointHandle_t )msg.ReadLong();
			loop = ( msg.ReadBits ( 1 ) != 0 );
			origin.x = msg.ReadFloat( );
			origin.y = msg.ReadFloat( );
			origin.z = msg.ReadFloat( );
			category = ( effectCategory_t )msg.ReadByte();

			if ( bse->CanPlayRateLimited( category ) ) {
			// mwhitlock: Dynamic memory consolidation
				PushSystemHeapID(RV_HEAP_ID_MULTIPLE_FRAME);
				clientEffect = new rvClientEffect( effect );
				PopSystemHeap();

				clientEffect->SetOrigin ( vec3_origin );
				clientEffect->SetAxis ( mat3_identity );
				clientEffect->Bind( this, jointHandle );

				clientEffect->Play( time, loop, origin );
			}
			return true;
		}

		case EVENT_PLAYEFFECT: {
			const idDecl*		effect;
			anCQuat				quat;
			anVec3				origin, origin2;
			rvClientEffect*		clientEffect;
			effectCategory_t	category;
			bool				loop;

			effect = idGameLocal::ReadDecl( msg, DECL_EFFECT );

			origin.x = msg.ReadFloat( );
			origin.y = msg.ReadFloat( );
			origin.z = msg.ReadFloat( );

			quat.x = msg.ReadFloat( );
			quat.y = msg.ReadFloat( );
			quat.z = msg.ReadFloat( );

			loop = ( msg.ReadBits( 1 ) != 0 );

			origin2.x = msg.ReadFloat( );
			origin2.y = msg.ReadFloat( );
			origin2.z = msg.ReadFloat( );
			category = ( effectCategory_t )msg.ReadByte();

			if ( bse->CanPlayRateLimited( category ) ) {
				// mwhitlock: Dynamic memory consolidation
				PushSystemHeapID(RV_HEAP_ID_MULTIPLE_FRAME);
				clientEffect = new rvClientEffect( effect );
				PopSystemHeap();

				clientEffect->SetOrigin ( origin );
				clientEffect->SetAxis ( quat.ToMat3() );
				clientEffect->Bind ( this );

				clientEffect->Play ( time, loop, origin2 );
			}
			return true;
		}

		default: {
			return false;
		}
	}
//unreachable
//	return false;
}


// bdube: added
/*
================
anEntity::ClientStale
================
*/
bool anEntity::ClientStale( void ) {
	FreeModelDef();
	UpdateVisuals();
	GetPhysics()->UnlinkClip();
	return false;
}

/*
================
anEntity::ClientUnstale
================
*/
void anEntity::ClientUnstale( void ) {
}

/*
================
anEntity::GetDamageEntity

Returns the entity that should take damage in place of this entity.  The default is the
entity itself.
================
*/
anEntity *anEntity::GetDamageEntity( void ) {
	return forwardDamageEnt.IsValid() ? forwardDamageEnt.GetEntity() : this;
}

// rjohnson: moved entity info out of idGameLocal into its own function
/*
================
anEntity::DrawDebugEntityInfo
================
*/
void anEntity::DrawDebugEntityInfo( anBounds *viewBounds, anBounds *viewTextBounds, anVec4 *overrideColor ) {
	anBasePlayer *player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		return;
	}

	anMat3 axis = player->viewAngles.ToMat3();
	anVec3 up = axis[2] * 5.0f;

	// skip if the entity is very far away
	if ( viewBounds && !viewBounds->IntersectsBounds( GetPhysics()->GetAbsBounds() ) ) {
		return;
	}

	const anBounds &entBounds = GetPhysics()->GetAbsBounds();

	if (overrideColor) {
		if ( !entBounds.GetVolume() ) {
			gameRenderWorld->DebugBounds( *overrideColor, entBounds.Expand( 8.0f ), vec3_origin );
		} else {
			gameRenderWorld->DebugBounds( *overrideColor, entBounds, vec3_origin );
		}
	} else {
		int contents = GetPhysics()->GetContents();
		if ( contents & CONTENTS_ACTORBODY ) {
			gameRenderWorld->DebugBounds ( colorCyan, entBounds, vec3_origin );
		} else if ( contents & CONTENTS_TRIGGER_SEAS ) {
			gameRenderWorld->DebugBounds( colorOrange, entBounds, vec3_origin );
		} else if ( contents & CONTENTS_SOLID ) {
			gameRenderWorld->DebugBounds( colorGreen, entBounds, vec3_origin );
		} else {
			if ( !entBounds.GetVolume() ) {
				gameRenderWorld->DebugBounds( colorMdGrey, entBounds.Expand( 8.0f ), vec3_origin );
			} else {
				gameRenderWorld->DebugBounds( colorMdGrey, entBounds, vec3_origin );
			}
		}
	}

	if ( !viewTextBounds || viewTextBounds->IntersectsBounds( entBounds ) ) {
		gameRenderWorld->DrawText( name.c_str(), entBounds.GetCenter(), 0.1f, colorWhite, axis, 1 );
		gameRenderWorld->DrawText( va( "#%d", entityNumber ), entBounds.GetCenter() + up, 0.1f, colorWhite, axis, 1 );

		if ( gameLocal.GetLocalPlayer() && this != gameLocal.GetLocalPlayer() && teamMaster != gameLocal.GetLocalPlayer() ) {
			gameRenderWorld->DebugLine ( colorRed, GetPhysics()->GetCenterMass(), GetPhysics()->GetCenterMass() + 50.0f * GetPhysics()->GetAxis()[0] );
			gameRenderWorld->DebugLine ( colorGreen, GetPhysics()->GetCenterMass(), GetPhysics()->GetCenterMass() + 50.0f * GetPhysics()->GetAxis()[1] );
			gameRenderWorld->DebugLine ( colorBlue, GetPhysics()->GetCenterMass(), GetPhysics()->GetCenterMass() + 50.0f * GetPhysics()->GetAxis()[2] );
		}
	}
}

/*
=====================
anEntity::SetInstance
=====================
*/
void anEntity::SetInstance( int newInstance ) {
	instance = newInstance;

	if ( gameLocal.isServer ) {
		SetClipWorld( newInstance );
	}
}

/*
=====================
anEntity::InstanceJoin
Gets called when the local player joins the same instance as this entity
=====================
*/
void anEntity::InstanceJoin( void ) {
	assert( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->GetInstance() == instance );

	BecomeActive( TH_UPDATEVISUALS );
	Present();
}

/*
=====================
anEntity::InstanceLeave
Gets called when the local player leaves the same instance as this entity
=====================
*/
void anEntity::InstanceLeave( void ) {
	assert( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->GetInstance() != instance );

	FreeLightDef();
	FreeModelDef();
	//RemoveClientEntities();
	BecomeInactive( TH_UPDATEVISUALS );
}

/*
=====================
anEntity::GetDebugInfo
=====================
*/
void anEntity::GetDebugInfo ( debugInfoProc_t proc, void* userData ) {
	// Base class first
	anClass::GetDebugInfo ( proc, userData );

	proc ( "anEntity", "health",		va( "%d",health), userData );
	proc ( "anEntity", "name",			name, userData );
	proc ( "anEntity", "entityNumber",  va( "%d",entityNumber), userData );
	proc ( "anEntity", "origin",		renderEntity.origin.ToString(), userData );

	proc ( "anEntity", "notarget",		fl.notarget?"true":"false", userData );
	proc ( "anEntity", "takedamage",	fl.takedamage?"true":"false", userData );
	proc ( "anEntity", "hidden",		fl.hidden?"true":"false", userData );
	proc ( "anEntity", "bindOrientated",fl.bindOrientated?"true":"false", userData );
	proc ( "anEntity", "isDormant",		fl.isDormant?"true":"false", userData );
	proc ( "anEntity", "neverDormant",	fl.neverDormant?"true":"false", userData );
	proc ( "anEntity", "isAIObstacle",	fl.isAIObstacle?"true":"false", userData );

	proc ( "anEntity", "forwardDamageEnt",forwardDamageEnt.GetEntity() ? forwardDamageEnt.GetEntity()->GetName() : "<none>", userData );

	proc ( "anEntity", "bindMaster",	bindMaster ? bindMaster->GetName() : "<none>", userData );
	proc ( "anEntity", "bindJoint",		va( "%d",((int)bindJoint)), userData );
	proc ( "anEntity", "bindBody",		va( "%d",bindBody), userData );

	proc ( "anEntity", "teamMaster",	teamMaster ? teamMaster->GetName() : "<none>", userData );
	proc ( "anEntity", "teamChain",		teamChain ? teamChain->GetName() : "<none>", userData );
}

// mwhitlock: memory profiling
/*
=====================
anEntity::Size()

Returns memory size of an anEntity instance
=====================
*/

size_t anEntity::Size( void ) const
{
	// TODO: more crap needs to go here!
	return sizeof (anEntity);
}




/*
===============================================================================

  anAnimatedEntity

===============================================================================
*/

const anEventDef EV_GetJointHandle( "getJointHandle", "s", 'd' );
const anEventDef EV_ClearAllJoints( "clearAllJoints" );
const anEventDef EV_ClearJoint( "clearJoint", "d" );
const anEventDef EV_SetJointPos( "setJointPos", "ddv" );
const anEventDef EV_SetJointAngle( "setJointAngle", "ddv" );
const anEventDef EV_GetJointPos( "getJointPos", "d", 'v' );
const anEventDef EV_GetJointAngle( "getJointAngle", "d", 'v' );



// bdube: programmer controlled joint events
const anEventDef EV_SetJointAngularVelocity ( "setJointAngularVelocity", "sfffd" );
const anEventDef EV_CollapseJoints ( "collapseJoints", "ss" );
// jshepard: clear out all animations still running on the model
const anEventDef EV_ClearAnims( "clearAnims" );



CLASS_DECLARATION( anEntity, anAnimatedEntity )
	EVENT( EV_GetJointHandle,		anAnimatedEntity::Event_GetJointHandle )
	EVENT( EV_ClearAllJoints,		anAnimatedEntity::Event_ClearAllJoints )
	EVENT( EV_ClearJoint,			anAnimatedEntity::Event_ClearJoint )
	EVENT( EV_SetJointPos,			anAnimatedEntity::Event_SetJointPos )
	EVENT( EV_SetJointAngle,		anAnimatedEntity::Event_SetJointAngle )
	EVENT( EV_GetJointPos,			anAnimatedEntity::Event_GetJointPos )
	EVENT( EV_GetJointAngle,		anAnimatedEntity::Event_GetJointAngle )

// RAVEEN BEGIN
// bdube: programmer controlled joint events
	EVENT( EV_SetJointAngularVelocity,	anAnimatedEntity::Event_SetJointAngularVelocity )
	EVENT( EV_CollapseJoints,			anAnimatedEntity::Event_CollapseJoints )

END_CLASS

/*
================
anAnimatedEntity::anAnimatedEntity
================
*/
anAnimatedEntity::anAnimatedEntity() {
	animator.SetEntity( this );
	damageEffects = nullptr;
}

/*
================
anAnimatedEntity::~anAnimatedEntity
================
*/
anAnimatedEntity::~anAnimatedEntity() {
	damageEffect_t	*de;

	for ( de = damageEffects; de; de = damageEffects ) {
		damageEffects = de->next;
		delete de;
	}
}

/*
================
anAnimatedEntity::Save

archives object for save game file
================
*/
void anAnimatedEntity::Save( anSaveGame *savefile ) const {
	animator.Save( savefile );

	// Wounds are very temporary, ignored at this time
	//damageEffect_t			*damageEffects;
}

/*
================
anAnimatedEntity::Restore

unarchives object from save game file
================
*/
void anAnimatedEntity::Restore( anRestoreGame *savefile ) {
	animator.Restore( savefile );

	// check if the entity has an MD5 model
	if ( animator.ModelHandle() ) {
		// set the callback to update the joints
		renderEntity.callback = anEntity::ModelCallback;
		animator.GetJoints( &renderEntity.numJoints, &renderEntity.joints );
		animator.GetBounds( gameLocal.time, renderEntity.bounds );
		if ( modelDefHandle != -1 ) {
			gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
		}
	}
}

/*
================
anAnimatedEntity::ClientPredictionThink
================
*/
void anAnimatedEntity::ClientPredictionThink( void ) {
	RunPhysics();
	UpdateAnimation();
	Present();
}

/*
================
anAnimatedEntity::Think
================
*/
void anAnimatedEntity::Think( void ) {
	RunPhysics();
	UpdateAnimation();
	Present();
}

/*
================
anAnimatedEntity::UpdateAnimation
================
*/
void anAnimatedEntity::UpdateAnimation( void ) {
	// don't do animations if they're not enabled
	if ( !( thinkFlags & TH_ANIMATE ) ) {
		return;
	}

	// is the model an MD5?
	if ( !animator.ModelHandle() ) {
		// no, so nothing to do
		return;
	}


// bgeisler: for triggered anims
	// call any frame commands that have happened in the past frame
	if ( !fl.hidden || fl.triggerAnim )  {
		animator.ServiceAnims( gameLocal.previousTime, gameLocal.time );
	}


	// if the model is animating then we have to update it
	if ( !animator.FrameHasChanged( gameLocal.time ) ) {
		// still fine the way it was
		return;
	}

	// get the latest frame bounds
	animator.GetBounds( gameLocal.time, renderEntity.bounds );
	if ( renderEntity.bounds.IsCleared() && !fl.hidden ) {
		gameLocal.DPrintf( "anAnimatedEntity %s %d: inside out bounds - %d\n", GetName(), entityNumber, gameLocal.time );
	}

	// update the renderEntity
	UpdateVisuals();

	// the animation is updated
	animator.ClearForceUpdate();
}

/*
================
anAnimatedEntity::GetAnimator
================
*/
anAnimator *anAnimatedEntity::GetAnimator( void ) {
	return &animator;
}

/*
================
anAnimatedEntity::SetModel
================
*/
void anAnimatedEntity::SetModel( const char *modelname ) {
	FreeModelDef();

	renderEntity.hModel = animator.SetModel( modelname );
	if ( !renderEntity.hModel ) {
		anEntity::SetModel( modelname );
		return;
	}

	if ( !renderEntity.customSkin ) {
		renderEntity.customSkin = animator.ModelDef()->GetDefaultSkin();
	}

	// set the callback to update the joints
	renderEntity.callback = anEntity::ModelCallback;
	animator.GetJoints( &renderEntity.numJoints, &renderEntity.joints );
	animator.GetBounds( gameLocal.time, renderEntity.bounds );

	UpdateVisuals();
}

/*
=====================
anAnimatedEntity::GetJointWorldTransform
=====================
*/
bool anAnimatedEntity::GetJointWorldTransform( jointHandle_t jointHandle, int currentTime, anVec3 &offset, anMat3 &axis ) {
	if ( g_perfTest_noJointTransform.GetBool() ) {
		offset = GetPhysics()->GetCenterMass();
		axis = renderEntity.axis;
		return true;
	}

	if ( !animator.GetJointTransform( jointHandle, currentTime, offset, axis ) ) {
		return false;
	}

	ConvertLocalToWorldTransform( offset, axis );
	return true;
}

/*
==============
anAnimatedEntity::GetJointTransformForAnim
==============
*/
bool anAnimatedEntity::GetJointTransformForAnim( jointHandle_t jointHandle, int animNum, int frameTime, anVec3 &offset, anMat3 &axis ) const {
	const idAnim	*anim;
	int				numJoints;
	anJointMat		*frame;

	if ( g_perfTest_noJointTransform.GetBool() ) {
		offset = GetPhysics()->GetCenterMass() - GetPhysics()->GetOrigin();
		axis = renderEntity.axis;
		return true;
	}

	anim = animator.GetAnim( animNum );
	if ( !anim ) {
		assert( 0 );
		return false;
	}

	numJoints = animator.NumJoints();
	if ( ( jointHandle < 0 ) || ( jointHandle >= numJoints ) ) {
		assert( 0 );
		return false;
	}

	frame = ( anJointMat * )_alloca16( numJoints * sizeof( anJointMat ) );
	gameEdit->ANIM_CreateAnimFrame( animator.ModelHandle(), anim->MD5Anim( 0 ), renderEntity.numJoints, frame, frameTime, animator.ModelDef()->GetVisualOffset(), animator.RemoveOrigin() );

	offset = frame[ jointHandle ].ToVec3();
	axis = frame[ jointHandle ].ToMat3();

	return true;
}


// ddynerman: removed/merged AddLocalDamageEffect() (redundant math)
/*
==============
anAnimatedEntity::AddDamageEffect

  Dammage effects track the animating impact position, spitting out particles.
==============
*/
void anAnimatedEntity::AddDamageEffect( const trace_t &collision, const anVec3 &velocity, const char *damageDefName, anEntity *inflictor ) {
	// ddynerman: note, on client the collision struct is incomplete.  Only contains impact point and material
	const char *splat, *decal, *key;
	anVec3 dir;

	const anDeclEntityDef *def = gameLocal.FindEntityDef( damageDefName, false );
	if ( def == nullptr || !def->dict.GetBool ( "bleed" ) ) {
		return;
	}

	if ( !spawnArgs.GetBool( "bleed" ) ) {
		return;
	}

	dir = velocity;
	dir.Normalize();

	if ( gameLocal.isServer ) {
		anBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteFloat( collision.c.point[0] );
		msg.WriteFloat( collision.c.point[1] );
		msg.WriteFloat( collision.c.point[2] );
		msg.WriteDir( dir, 24 );
		idGameLocal::WriteDecl( msg, def );
		idGameLocal::WriteDecl( msg, collision.c.material );
		ServerSendInstanceEvent( EVENT_ADD_DAMAGE_EFFECT, &msg, false, -1 );
	}

	if ( !g_decals.GetBool() ) {
		return;
	}

	if ( gameLocal.GetLocalPlayer() && GetInstance() != gameLocal.GetLocalPlayer()->GetInstance() ) {
		return; // no blood from other instances
	}

	// blood splats are thrown onto nearby surfaces
	splat = nullptr;
	if ( collision.c.material->GetMaterialType() ) {
		key = va( "mtr_splat_%s", collision.c.material->GetMaterialType()->GetName() );
		splat = spawnArgs.RandomPrefix( key, gameLocal.random );
	}
	if ( !splat || !*splat ) {
		splat = spawnArgs.RandomPrefix( "mtr_splat", gameLocal.random );
	}
	if ( splat && *splat ) {
		//jshepard original 64.0f
		// dluetscher: changed from 64. to 48. for performance reasons
		gameLocal.BloodSplat( this, collision.c.point, dir, 48.0f, splat );
	}

	// can't see wounds on the player model in single player mode
	if ( !( IsType( anBasePlayer::GetClassType() ) && !gameLocal.isMultiplayer ) ) {
		//If this is a buddy marine, no wound decals until they're actually dead unless it's mp.
		if ( gameLocal.isMultiplayer
			|| !IsType( anSAAI::GetClassType() )
			|| this->health <= 0
			|| ((anSAAI*)this)->team != AITEAM_MARINE ) {
			// place a wound overlay on the model
			decal = nullptr;
			if ( collision.c.material->GetMaterialType() ) {
				key = va( "mtr_wound_%s", collision.c.material->GetMaterialType()->GetName() );
				decal = spawnArgs.RandomPrefix( key, gameLocal.random );
			}
			if ( !decal || !*decal ) {
				decal = spawnArgs.RandomPrefix( "mtr_wound", gameLocal.random );
			}
			if ( decal && *decal ) {
				ProjectOverlay( collision.c.point, dir, 20.0f, decal );
				if ( IsType( anBasePlayer::GetClassType() ) ) {
					ProjectHeadOverlay( collision.c.point, dir, 20.0f, decal );
				}
			}
		}
	}
}


/*
==============
anAnimatedEntity::GetDefaultSurfaceType
==============
*/
int	anAnimatedEntity::GetDefaultSurfaceType( void ) const {
	return SURFTYPE_METAL;
}

/*
================
anAnimatedEntity::ClientReceiveEvent
================
*/
bool anAnimatedEntity::ClientReceiveEvent( int event, int time, const anBitMsg &msg ) {
	anVec3 origin, dir;

	switch ( event ) {
		case EVENT_ADD_DAMAGE_EFFECT: {
			origin[0] = msg.ReadFloat();
			origin[1] = msg.ReadFloat();
			origin[2] = msg.ReadFloat();
			dir = msg.ReadDir( 24 );
			const anDeclEntityDef *damageDef = static_cast< const anDeclEntityDef* >( idGameLocal::ReadDecl( msg, DECL_ENTITYDEF ) );
			const anMaterial *collisionMaterial = static_cast< const anMaterial* >( idGameLocal::ReadDecl( msg, DECL_MATERIAL ) );

// ddynerman: removed redundant AddLocalDamageEffect()
			trace_t collision;
			collision.c.point = origin;
			collision.c.material = collisionMaterial;
			AddDamageEffect( collision, dir, damageDef->GetName(), nullptr );

			return true;
		}
		default: {
			return anEntity::ClientReceiveEvent( event, time, msg );
		}
	}
//unreachable
//	return false;
}


// abahr: so we don't crash if UpdateModel is called from a destructor
/*
================
anAnimatedEntity::UpdateRenderEntityCallback
================
*/
void anAnimatedEntity::UpdateRenderEntityCallback() {
	// check if the entity has an MD5 model
	anAnimator *animator = GetAnimator();
	if ( animator && animator->ModelHandle() ) {
		// set the callback to update the joints
		renderEntity.callback = anEntity::ModelCallback;
	}
}


/*
================
anAnimatedEntity::Event_GetJointHandle

looks up the number of the specified joint.  returns INVALID_JOINT if the joint is not found.
================
*/
void anAnimatedEntity::Event_GetJointHandle( const char *jointname ) {
	jointHandle_t joint;

	joint = animator.GetJointHandle( jointname );
	anThread::ReturnInt( joint );
}

/*
================
anAnimatedEntity::Event_ClearAllJoints

removes any custom transforms on all joints
================
*/
void anAnimatedEntity::Event_ClearAllJoints( void ) {
	animator.ClearAllJoints();
}

/*
================
anAnimatedEntity::Event_ClearJoint

removes any custom transforms on the specified joint
================
*/
void anAnimatedEntity::Event_ClearJoint( jointHandle_t jointnum ) {
	animator.ClearJoint( jointnum );
}

/*
================
anAnimatedEntity::Event_ClearAnims

Clears any animation running on the animated entity
================
*/
void anAnimatedEntity::Event_ClearAnims( void ) {
	animator.Clear( ANIMCHANNEL_ALL, gameLocal.GetTime(), gameLocal.GetTime() );
}

/*
================
anAnimatedEntity::Event_SetJointPos

modifies the position of the joint based on the transform type
================
*/
void anAnimatedEntity::Event_SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const anVec3 &pos ) {
	animator.SetJointPos( jointnum, transform_type, pos );
}

/*
================
anAnimatedEntity::Event_SetJointAngle

modifies the orientation of the joint based on the transform type
================
*/
void anAnimatedEntity::Event_SetJointAngle( jointHandle_t jointnum, jointModTransform_t transform_type, const anAngles &angles ) {
	anMat3 mat;

	mat = angles.ToMat3();
	animator.SetJointAxis( jointnum, transform_type, mat );
}

/*
================
anAnimatedEntity::Event_GetJointPos

returns the position of the joint in worldspace
================
*/
void anAnimatedEntity::Event_GetJointPos( jointHandle_t jointnum ) {
	anVec3 offset;
	anMat3 axis;

	if ( !GetJointWorldTransform( jointnum, gameLocal.time, offset, axis ) ) {
		gameLocal.Warning( "Joint # %d out of range on entity '%s'",  jointnum, name.c_str() );
	}

	anThread::ReturnVector( offset );
}

/*
================
anAnimatedEntity::Event_GetJointAngle

returns the orientation of the joint in worldspace
================
*/
void anAnimatedEntity::Event_GetJointAngle( jointHandle_t jointnum ) {
	anVec3 offset;
	anMat3 axis;

	if ( !GetJointWorldTransform( jointnum, gameLocal.time, offset, axis ) ) {
		gameLocal.Warning( "Joint # %d out of range on entity '%s'",  jointnum, name.c_str() );
	}

	anAngles ang = axis.ToAngles();
	anVec3 vec( ang[0], ang[1], ang[2] );
	anThread::ReturnVector( vec );
}


// bdube: moved to anAnimatedEntity
/*
================
anAnimatedEntity::Event_SetJointAngularVelocity
================
*/
void anAnimatedEntity::Event_SetJointAngularVelocity ( const char *jointName, float pitch, float yaw, float roll, int blendTime ) {
	jointHandle_t joint = animator.GetJointHandle ( jointName );
	if ( joint == INVALID_JOINT ) {
		return;
	}

	animator.SetJointAngularVelocity ( joint, anAngles(pitch,yaw,roll), gameLocal.time, blendTime );
}

/*
================
anAnimatedEntity::Event_CollapseJoints
================
*/
void anAnimatedEntity::Event_CollapseJoints ( const char *jointnames, const char *collapseTo ) {
	jointHandle_t collapseToJoint = animator.GetJointHandle ( collapseTo );
	if ( collapseToJoint == INVALID_JOINT ) {
		return;
	}

	animator.CollapseJoints ( jointnames, collapseToJoint );
}


