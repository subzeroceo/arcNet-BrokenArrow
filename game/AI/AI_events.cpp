
#include "../../idlib/Lib.h"
#pragma hdrstop

#include "../Game_local.h"

#include "AI.h"

#ifndef __GAME_PROJECTILE_H__
#include "../Projectile.h"
#endif

#include "AI_Manager.h"
#include "../vehicle/Vehicle.h"
#include "../spawner.h"


#define anSAAI_EVENT( eventname, inparms, outparm)					\
	const anEventDef AI_##eventname( #eventname, inparms, outparm);	\
	void anSAAI::Event_##eventname


/***********************************************************************

	AI Events

***********************************************************************/

// Get / Set
const anEventDef AI_GetLeader						( "getLeader", nullptr, 'e' );
const anEventDef AI_SetLeader						( "setLeader", "E" );
const anEventDef AI_GetEnemy						( "getEnemy", nullptr, 'e' );
const anEventDef AI_SetEnemy						( "setEnemy", "E" );
const anEventDef AI_SetHealth						( "setHealth", "f" );
const anEventDef AI_SetTalkState					( "setTalkState", "d" );
const anEventDef AI_SetScript						( "setScript", "ss" );
const anEventDef AI_SetMoveSpeed					( "setMoveSpeed", "d" );
const anEventDef AI_SetPassivePrefix				( "setPassivePrefix", "s" );

// Enable / Disable
const anEventDef AI_EnableClip						( "enableClip" );
const anEventDef AI_DisableClip						( "disableClip" );
const anEventDef AI_EnableGravity					( "enableGravity" );
const anEventDef AI_DisableGravity					( "disableGravity" );
const anEventDef AI_EnableAFPush					( "enableAFPush" );
const anEventDef AI_DisableAFPush					( "disableAFPush" );
const anEventDef AI_EnableDamage					( "enableDamage" );
const anEventDef AI_DisableDamage					( "disableDamage" );
const anEventDef AI_EnableTarget					( "enableTarget" );
const anEventDef AI_DisableTarget					( "disableTarget" );
const anEventDef AI_EnableMovement					( "enableMovement" );
const anEventDef AI_DisableMovement					( "disableMovement" );
const anEventDef AI_TakeDamage						( "takeDamage", "f" );
const anEventDef AI_SetUndying						( "setUndying", "f" );
const anEventDef AI_EnableAutoBlink					( "enableAutoBlink" );
const anEventDef AI_DisableAutoBlink				( "disableAutoBlink" );

// Scripted Sequences
const anEventDef AI_ScriptedMove					( "scriptedMove", "efd" );
const anEventDef AI_ScriptedFace					( "scriptedFace", "ed" );
const anEventDef AI_ScriptedAnim					( "scriptedAnim", "sddd" );
const anEventDef AI_ScriptedPlaybackMove			( "scriptedPlaybackMove", "sdd" );
const anEventDef AI_ScriptedPlaybackAim				( "scriptedPlaybackAim", "sdd" );
const anEventDef AI_ScriptedAction					( "scriptedAction", "ed" );
const anEventDef AI_ScriptedDone					( "scriptedDone", nullptr, 'd' );
const anEventDef AI_ScriptedStop					( "scriptedStop" );
const anEventDef AI_ScriptedJumpDown				( "scriptedJumpDown", "f" );

// Misc
const anEventDef AI_LookAt							( "lookAt", "E" );
const anEventDef AI_Attack							( "attack", "ss" );
const anEventDef AI_LockEnemyOrigin					( "lockEnemyOrigin" );
const anEventDef AI_StopThinking					( "stopThinking" );
const anEventDef AI_JumpFrame						( "<jumpframe>" );
const anEventDef AI_RealKill						( "<kill>" );
const anEventDef AI_Kill							( "kill" );
const anEventDef AI_RemoveUpdateSpawner				( "removeUpdateSpawner" );
const anEventDef AI_AllowHiddenMovement				( "allowHiddenMovement", "d" );
const anEventDef AI_Speak							( "speak", "s" );
const anEventDef AI_SpeakRandom						( "speakRandom", "s" );
const anEventDef AI_IsSpeaking						( "isSpeaking", nullptr, 'f' );
const anEventDef AI_IsTethered						( "isTethered", nullptr, 'f' );
const anEventDef AI_IsWithinTether					( "isWithinTether", nullptr, 'f' );
const anEventDef AI_LaunchMissile					( "launchMissile", "vv", 'e' );
const anEventDef AI_AttackMelee						( "attackMelee", "s", 'd' );
const anEventDef AI_DirectDamage					( "directDamage", "es" );
const anEventDef AI_RadiusDamageFromJoint			( "radiusDamageFromJoint", "ss" );
const anEventDef AI_MeleeAttackToJoint				( "meleeAttackToJoint", "ss", 'd' );
const anEventDef AI_CanBecomeSolid					( "canBecomeSolid", nullptr, 'f' );
const anEventDef AI_BecomeSolid						( "becomeSolid" );
const anEventDef AI_BecomeRagdoll					( "becomeRagdoll", nullptr, 'd' );
const anEventDef AI_BecomePassive					( "becomePassive", "d" );
const anEventDef AI_BecomeAggressive				( "becomeAggressive" );
const anEventDef AI_StopRagdoll						( "stopRagdoll" );
const anEventDef AI_FaceEnemy						( "faceEnemy" );
const anEventDef AI_FaceEntity						( "faceEntity", "E" );

//jshepard
const anEventDef AI_FindEnemy						( "findEnemy", "f", 'e');

void anSAAI::Event_Activate( anEntity *activator )											{ Activate( activator );}
void anSAAI::Event_Touch( anEntity *other, trace_t *trace )									{ OnTouch( other, trace ); }
void anSAAI::Event_SetEnemy( anEntity *ent )													{ if ( !ent ) ClearEnemy(); else SetEnemy( ent );}
void anSAAI::Event_DirectDamage( anEntity *damageTarget, const char *damageDefName )			{ DirectDamage( damageDefName, damageTarget ); }
void anSAAI::Event_RadiusDamageFromJoint( const char *jointname, const char *damageDefName )	{ RadiusDamageFromJoint( jointname, damageDefName ); }
void anSAAI::Event_CanBecomeSolid( void )														{ anThread::ReturnFloat( CanBecomeSolid() ); }
void anSAAI::Event_BecomeSolid( void )														{ BecomeSolid(); }
void anSAAI::Event_BecomeNonSolid( void )														{ BecomeNonSolid(); }
void anSAAI::Event_BecomeRagdoll( void )														{ anThread::ReturnInt( StartRagdoll() ); }
void anSAAI::Event_StopRagdoll( void )														{ StopRagdoll(); SetPhysics( &physicsObj ); }
void anSAAI::Event_SetHealth( float newHealth )												{ health = newHealth; fl.takedamage = true; if ( health > 0 ) aifl.dead = false; else aifl.dead = true; }
void anSAAI::Event_FaceEnemy( void )															{ FaceEnemy(); }
void anSAAI::Event_FaceEntity( anEntity *ent )												{ FaceEntity( ent ); }
void anSAAI::Event_SetTalkState( int state )													{ SetTalkState ( (talkState_t)state );  }
void anSAAI::Event_Speak( const char *speechDecl )											{ Speak( speechDecl ); }
void anSAAI::Event_SpeakRandom( const char *speechDecl )										{ Speak( speechDecl, true ); }
void anSAAI::Event_GetLeader( void )															{ anThread::ReturnEntity( leader ); }
void anSAAI::Event_SetLeader( anEntity* ent )													{ SetLeader ( ent ); }
void anSAAI::Event_GetEnemy( void )															{ anThread::ReturnEntity( enemy.ent ); }
void anSAAI::Event_TakeDamage( float takeDamage )												{ fl.takedamage = ( takeDamage ) ? true : false; }
void anSAAI::Event_SetUndying( float setUndying )												{ aifl.undying = ( setUndying ) ? true : false; }


void anSAAI::Event_IsSpeaking ( void )														{ anThread::ReturnFloat ( IsSpeaking() ); }
void anSAAI::Event_IsTethered ( void )														{ anThread::ReturnFloat ( IsTethered() ); }
void anSAAI::Event_IsWithinTether ( void )													{ anThread::ReturnFloat ( IsWithinTether() ); }
void anSAAI::Event_IsMoving ( void )															{ anThread::ReturnFloat ( move.fl.moving  ); }

CLASS_DECLARATION( anActor, anSAAI )
	EVENT( EV_Activate,							anSAAI::Event_Activate )
	EVENT( EV_Touch,							anSAAI::Event_Touch )

	// Enable / Disable
	EVENT( AI_EnableClip,						anSAAI::Event_EnableClip )
	EVENT( AI_DisableClip,						anSAAI::Event_DisableClip )
	EVENT( AI_EnableGravity,					anSAAI::Event_EnableGravity )
	EVENT( AI_DisableGravity,					anSAAI::Event_DisableGravity )
	EVENT( AI_EnableAFPush,						anSAAI::Event_EnableAFPush )
	EVENT( AI_DisableAFPush,					anSAAI::Event_DisableAFPush )
	EVENT( AI_EnableDamage,						anSAAI::Event_EnableDamage )
	EVENT( AI_DisableDamage,					anSAAI::Event_DisableDamage )
	EVENT( AI_EnablePain,						anSAAI::Event_EnablePain )
	EVENT( AI_DisablePain,						anSAAI::Event_DisablePain )
	EVENT( AI_EnableTarget,						anSAAI::Event_EnableTarget )
	EVENT( AI_DisableTarget,					anSAAI::Event_DisableTarget )
	EVENT( AI_TakeDamage,						anSAAI::Event_TakeDamage )
	EVENT( AI_SetUndying,						anSAAI::Event_SetUndying )
	EVENT( AI_EnableAutoBlink,					anSAAI::Event_EnableAutoBlink )
	EVENT( AI_DisableAutoBlink,					anSAAI::Event_DisableAutoBlink )

	// Scripted sequences
	EVENT( AI_ScriptedMove,						anSAAI::Event_ScriptedMove )
	EVENT( AI_ScriptedFace,						anSAAI::Event_ScriptedFace )
	EVENT( AI_ScriptedAnim,						anSAAI::Event_ScriptedAnim )
	EVENT( AI_ScriptedAction,					anSAAI::Event_ScriptedAction )
	EVENT( AI_ScriptedPlaybackMove,				anSAAI::Event_ScriptedPlaybackMove )
	EVENT( AI_ScriptedPlaybackAim,				anSAAI::Event_ScriptedPlaybackAim )
	EVENT( AI_ScriptedDone,						anSAAI::Event_ScriptedDone )
	EVENT( AI_ScriptedStop,						anSAAI::Event_ScriptedStop )
	EVENT( AI_ScriptedJumpDown,					anSAAI::Event_ScriptedJumpDown )

	// Get / Set
	EVENT( AI_SetTalkState,						anSAAI::Event_SetTalkState )
	EVENT( AI_SetLeader,						anSAAI::Event_SetLeader )
	EVENT( AI_GetLeader,						anSAAI::Event_GetLeader )
	EVENT( AI_SetEnemy,							anSAAI::Event_SetEnemy )
	EVENT( AI_GetEnemy,							anSAAI::Event_GetEnemy )
	EVENT( EV_GetAngles,						anSAAI::Event_GetAngles )
	EVENT( EV_SetAngles,						anSAAI::Event_SetAngles )
	EVENT( AI_SetScript,						anSAAI::Event_SetScript )
	EVENT( AI_SetMoveSpeed,						anSAAI::Event_SetMoveSpeed )
	EVENT( AI_SetPassivePrefix,					anSAAI::Event_SetPassivePrefix )

	// Misc
	EVENT( AI_Attack,							anSAAI::Event_Attack )
	EVENT( AI_AttackMelee,						anSAAI::Event_AttackMelee )

	EVENT( AI_LookAt,							anSAAI::Event_LookAt )
	EVENT( AI_DirectDamage,						anSAAI::Event_DirectDamage )
	EVENT( AI_RadiusDamageFromJoint,			anSAAI::Event_RadiusDamageFromJoint )
	EVENT( AI_CanBecomeSolid,					anSAAI::Event_CanBecomeSolid )
	EVENT( AI_BecomeSolid,						anSAAI::Event_BecomeSolid )
	EVENT( EV_BecomeNonSolid,					anSAAI::Event_BecomeNonSolid )
	EVENT( AI_BecomeRagdoll,					anSAAI::Event_BecomeRagdoll )
	EVENT( AI_BecomePassive,					anSAAI::Event_BecomePassive )
	EVENT( AI_BecomeAggressive,					anSAAI::Event_BecomeAggressive )
	EVENT( AI_StopRagdoll,						anSAAI::Event_StopRagdoll )
	EVENT( AI_SetHealth,						anSAAI::Event_SetHealth )
	EVENT( AI_FaceEnemy,						anSAAI::Event_FaceEnemy )
	EVENT( AI_FaceEntity,						anSAAI::Event_FaceEntity )
	EVENT( AI_StopThinking,						anSAAI::Event_StopThinking )
	EVENT( AI_LockEnemyOrigin,					anSAAI::Event_LockEnemyOrigin )
	EVENT( AI_JumpFrame,						anSAAI::Event_JumpFrame )
	EVENT( AI_RealKill,							anSAAI::Event_RealKill )
	EVENT( AI_Kill,								anSAAI::Event_Kill )
	EVENT( AI_RemoveUpdateSpawner,				anSAAI::Event_RemoveUpdateSpawner )
	EVENT( AI_AllowHiddenMovement,				anSAAI::Event_AllowHiddenMovement )
	EVENT( AI_Speak,							anSAAI::Event_Speak )
	EVENT( AI_SpeakRandom,						anSAAI::Event_SpeakRandom )
	EVENT( AI_IsSpeaking,						anSAAI::Event_IsSpeaking )
	EVENT( AI_IsTethered,						anSAAI::Event_IsTethered )
	EVENT( AI_IsWithinTether,					anSAAI::Event_IsWithinTether )
	EVENT( EV_IsMoving,							anSAAI::Event_IsMoving )
	EVENT( AI_TakeDamage,						anSAAI::Event_TakeDamage )
	EVENT( AI_FindEnemy,						anSAAI::Event_FindEnemy )
	EVENT( EV_SetKey,							anSAAI::Event_SetKey )

	// twhitaker: needed this for difficulty settings
	EVENT( EV_PostSpawn,						anSAAI::Event_PostSpawn )

END_CLASS

/*
=====================
anSAAI::Event_PredictEnemyPos
=====================
*/
void anSAAI::Event_PredictEnemyPos( float time ) {
	predictedPath_t path;
	anEntity*		enemyEnt = enemy.ent;

	// if no enemy set
	if ( !enemyEnt ) {
		anThread::ReturnVector( physicsObj.GetOrigin() );
		return;
	}

	// predict the enemy movement
	anSAAI::PredictPath( enemyEnt, aas, enemy.lastKnownPosition, enemyEnt->GetPhysics()->GetLinearVelocity(), SEC2MS( time ), SEC2MS( time ), ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	anThread::ReturnVector( path.endPos );
}

/*
=====================
anSAAI::Event_TestAnimMoveTowardEnemy
=====================
*/
void anSAAI::Event_TestAnimMoveTowardEnemy( const char *animname ) {
	int				anim;
	predictedPath_t path;
	anVec3			moveVec;
	float			yaw;
	anVec3			delta;
	anEntity		*enemyEnt;

	enemyEnt = enemy.ent;
	if ( !enemyEnt ) {
		anThread::ReturnInt( false );
		return;
	}

	anim = GetAnim( ANIMCHANNEL_LEGS, animname );
	if ( !anim ) {
		gameLocal.DWarning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		anThread::ReturnInt( false );
		return;
	}

	delta = enemyEnt->GetPhysics()->GetOrigin() - physicsObj.GetOrigin();
    yaw = delta.ToYaw();

	moveVec = animator.TotalMovementDelta( anim ) * anAngles( 0.0f, yaw, 0.0f ).ToMat3() * physicsObj.GetGravityAxis();
	anSAAI::PredictPath( this, aas, physicsObj.GetOrigin(), moveVec, 1000, 1000, ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	if ( DebugFilter(ai_debugMove) ) {
		gameRenderWorld->DebugLine( colorGreen, physicsObj.GetOrigin(), physicsObj.GetOrigin() + moveVec, gameLocal.msec );
		gameRenderWorld->DebugBounds( path.endEvent == 0 ? colorYellow : colorRed, physicsObj.GetBounds(), physicsObj.GetOrigin() + moveVec, gameLocal.msec );
	}

	anThread::ReturnInt( path.endEvent == 0 );
}

/*
=====================
anSAAI::Event_TestAnimMove
=====================
*/
void anSAAI::Event_TestAnimMove( const char *animname ) {
	int				anim;
	predictedPath_t path;
	anVec3			moveVec;
	int				animLen;

	anim = GetAnim( ANIMCHANNEL_LEGS, animname );
	if ( !anim ) {
		gameLocal.DWarning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		anThread::ReturnInt( false );
		return;
	}

	moveVec = animator.TotalMovementDelta( anim ) * anAngles( 0.0f, move.ideal_yaw, 0.0f ).ToMat3() * physicsObj.GetGravityAxis();
	animLen = animator.AnimLength( anim );
	anSAAI::PredictPath( this, aas, physicsObj.GetOrigin(), moveVec, 1000, 1000, ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	if (  DebugFilter(ai_debugMove) ) {
		gameRenderWorld->DebugLine( colorGreen, physicsObj.GetOrigin(), physicsObj.GetOrigin() + moveVec, gameLocal.msec );
		gameRenderWorld->DebugBounds( path.endEvent == 0 ? colorYellow : colorRed, physicsObj.GetBounds(), physicsObj.GetOrigin() + moveVec, gameLocal.msec );
	}

	anThread::ReturnInt( path.endEvent == 0 );
}

/*
=====================
anSAAI::Event_TestMoveToPosition
=====================
*/
void anSAAI::Event_TestMoveToPosition( const anVec3 &position ) {
	predictedPath_t path;

	anSAAI::PredictPath( this, aas, physicsObj.GetOrigin(), position - physicsObj.GetOrigin(), 1000, 1000, ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	if ( DebugFilter(ai_debugMove) ) {
		gameRenderWorld->DebugLine( colorGreen, physicsObj.GetOrigin(), position, gameLocal.msec );
 		gameRenderWorld->DebugBounds( colorYellow, physicsObj.GetBounds(), position, gameLocal.msec );
		if ( path.endEvent ) {
			gameRenderWorld->DebugBounds( colorRed, physicsObj.GetBounds(), path.endPos, gameLocal.msec );
		}
	}

	anThread::ReturnInt( path.endEvent == 0 );
}

/*
=====================
anSAAI::Event_TestMeleeAttack
=====================
*/
void anSAAI::Event_TestMeleeAttack( void ) {
	bool result = TestMelee();
	anThread::ReturnInt( result );
}

/*
=====================
anSAAI::Event_TestAnimAttack
=====================
*/
void anSAAI::Event_TestAnimAttack( const char *animname ) {
	int				anim;
	predictedPath_t path;

	anim = GetAnim( ANIMCHANNEL_LEGS, animname );
	if ( !anim ) {
		gameLocal.DWarning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		anThread::ReturnInt( false );
		return;
	}

	anSAAI::PredictPath( this, aas, physicsObj.GetOrigin(), animator.TotalMovementDelta( anim ), 1000, 1000, ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

	anThread::ReturnInt( path.blockingEntity && ( path.blockingEntity == enemy.ent ) );
}

/*
=====================
anSAAI::Event_LockEnemyOrigin
=====================
*/
void anSAAI::Event_LockEnemyOrigin ( void ) {
	enemy.fl.lockOrigin = true;
}

/*
=====================
anSAAI::Event_StopThinking
=====================
*/
void anSAAI::Event_StopThinking( void ) {
	BecomeInactive( TH_THINK );
	anThread *thread = anThread::CurrentThread();
	if ( thread ) {
		thread->DoneProcessing();
	}
}

/*
=====================
anSAAI::Event_JumpFrame
=====================
*/
void anSAAI::Event_JumpFrame( void ) {
	aifl.jump = true;
}

/*
=====================
anSAAI::Event_EnableClip
=====================
*/
void anSAAI::Event_EnableClip( void ) {
	physicsObj.SetClipMask( MASK_MONSTERSOLID );
	Event_EnableGravity();
}

/*
=====================
anSAAI::Event_DisableClip
=====================
*/
void anSAAI::Event_DisableClip( void ) {
	physicsObj.SetClipMask( 0 );
	Event_DisableGravity();
}

/*
=====================
anSAAI::Event_EnableGravity
=====================
*/
void anSAAI::Event_EnableGravity( void ) {
	OverrideFlag ( AIFLAGOVERRIDE_NOGRAVITY, false );
}

/*
=====================
anSAAI::Event_DisableGravity
=====================
*/
void anSAAI::Event_DisableGravity( void ) {
	OverrideFlag ( AIFLAGOVERRIDE_NOGRAVITY, true );
}

/*
=====================
anSAAI::Event_EnableAFPush
=====================
*/
void anSAAI::Event_EnableAFPush( void ) {
	move.fl.allowPushMovables = true;
}

/*
=====================
anSAAI::Event_DisableAFPush
=====================
*/
void anSAAI::Event_DisableAFPush( void ) {
	move.fl.allowPushMovables = false;
}

/*
=====================
anSAAI::Event_EnableDamage
=====================
*/
void anSAAI::Event_EnableDamage ( void ) {
	OverrideFlag ( AIFLAGOVERRIDE_DAMAGE, true );
}

/*
=====================
anSAAI::Event_DisableDamage
=====================
*/
void anSAAI::Event_DisableDamage ( void ) {
	OverrideFlag ( AIFLAGOVERRIDE_DAMAGE, false );
}

/*
===============
anSAAI::Event_DisablePain
===============
*/
void anSAAI::Event_DisablePain( void ) {
	OverrideFlag ( AIFLAGOVERRIDE_DISABLEPAIN, true );
}

/*
===============
anSAAI::Event_EnablePain
===============
*/
void anSAAI::Event_EnablePain( void ) {
	OverrideFlag ( AIFLAGOVERRIDE_DISABLEPAIN, false );
}

/*
===============
anSAAI::Event_EnableTarget
===============
*/
void anSAAI::Event_EnableTarget ( void ) {
	fl.notarget = false;
}

/*
===============
anSAAI::Event_DisableTarget
===============
*/
void anSAAI::Event_DisableTarget ( void ) {
	fl.notarget = true;
}


/*
=====================
anSAAI::Event_EnableAutoBlink
=====================
*/
void anSAAI::Event_EnableAutoBlink( void ) {
	fl.allowAutoBlink = true;
}

/*
=====================
anSAAI::Event_DisableAutoBlink
=====================
*/
void anSAAI::Event_DisableAutoBlink( void ) {
	fl.allowAutoBlink = false;
}

/*
=====================
anSAAI::Event_BecomeAggressive
=====================
*/
void anSAAI::Event_BecomeAggressive ( void ) {
	combat.fl.ignoreEnemies = false;
	combat.fl.aware = true;
	ForceTacticalUpdate();
}

/*
=====================
anSAAI::Event_BecomePassive
=====================
*/
void anSAAI::Event_BecomePassive ( int ignoreEnemies ) {
	combat.fl.ignoreEnemies = (ignoreEnemies != 0);
	combat.fl.aware = false;
	SetEnemy ( nullptr );
	ForceTacticalUpdate();
}

/*
=====================
anSAAI::Event_LookAt
=====================
*/
void anSAAI::Event_LookAt	( anEntity* lookAt ) {
	lookTarget = lookAt;
}

/*
=====================
anSAAI::LookAtEntity
=====================
*/
void anSAAI::LookAtEntity( anEntity *ent, float duration ) {
	if ( ent == this ) {
		ent = nullptr;
	}

	if ( ( ent != focusEntity.GetEntity() ) || ( focusTime < gameLocal.time ) ) {
		focusEntity	= ent;
		alignHeadTime = gameLocal.time;
		forceAlignHeadTime = gameLocal.time + SEC2MS( 1 );
		blink_time = 0;
	}

	focusTime = gameLocal.time + SEC2MS( duration );
}

/*
================
anSAAI::Event_ThrowMoveable
================
*/
void anSAAI::Event_ThrowMoveable( void ) {
	anEntity *ent;
	anEntity *moveable = nullptr;

	for ( ent = GetNextTeamEntity(); ent != nullptr; ent = ent->GetNextTeamEntity() ) {


		if ( ent->GetBindMaster() == this && ent->IsType( idMoveable::GetClassType() ) ) {

			moveable = ent;
			break;
		}
	}
	if ( moveable ) {
		moveable->Unbind();
		moveable->PostEventMS( &EV_SetOwner, 200, nullptr );
	}
}

/*
================
anSAAI::Event_ThrowAF
================
*/
void anSAAI::Event_ThrowAF( void ) {
	anEntity *ent;
	anEntity *af = nullptr;

	for ( ent = GetNextTeamEntity(); ent != nullptr; ent = ent->GetNextTeamEntity() ) {


		if ( ent->GetBindMaster() == this && ent->IsType( idAFEntity_Base::GetClassType() ) ) {

			af = ent;
			break;
		}
	}
	if ( af ) {
		af->Unbind();
		af->PostEventMS( &EV_SetOwner, 200, nullptr );
	}
}

/*
================
anSAAI::Event_SetAngles
================
*/
void anSAAI::Event_SetAngles( anAngles const &ang ) {
	move.current_yaw = ang.yaw;
	viewAxis = anAngles( 0, move.current_yaw, 0 ).ToMat3();
}

/*
================
anSAAI::Event_GetAngles
================
*/
void anSAAI::Event_GetAngles( void ) {
	anThread::ReturnVector( anVec3( 0.0f, move.current_yaw, 0.0f ) );
}

/*
================
anSAAI::Event_RealKill
================
*/
void anSAAI::Event_RealKill( void ) {
	health = 0;

	if ( af.IsLoaded() ) {
		// clear impacts
		af.Rest();

		// physics is turned off by calling af.Rest()
		BecomeActive( TH_PHYSICS );
	}

	Killed( this, this, 0, vec3_zero, INVALID_JOINT );
}

/*
================
anSAAI::Event_Kill
================
*/
void anSAAI::Event_Kill( void ) {
	PostEventMS( &AI_RealKill, 0 );
}

/*
================
anSAAI::Event_RemoveUpdateSpawner
================
*/
void anSAAI::Event_RemoveUpdateSpawner( void ) {
	// Detach from any spawners
	if ( GetSpawner() ) {
		GetSpawner()->Detach( this );
		SetSpawner( nullptr );
	}

	PostEventMS( &EV_Remove, 0 );
}
/*
=====================
anSAAI::Event_FindActorsInBounds
=====================
*/
void anSAAI::Event_FindActorsInBounds( const anVec3 &mins, const anVec3 &maxs ) {
	anEntity *	ent;
	anEntity *	entityList[ MAX_GENTITIES ];
	int			numListedEntities;
	int			i;



	numListedEntities = gameLocal.EntitiesTouchingBounds( this, anBounds( mins, maxs ), CONTENTS_ACTORBODY, entityList, MAX_GENTITIES );

	for ( i = 0; i < numListedEntities; i++ ) {
		ent = entityList[i];


		if ( ent != this && !ent->IsHidden() && ( ent->health > 0 ) && ent->IsType( anActor::GetClassType() ) ) {

			anThread::ReturnEntity( ent );
			return;
		}
	}

	anThread::ReturnEntity( nullptr );
}

/*
=====================
anSAAI::Event_ClosestReachableEnemyOfEntity
=====================
*/
void anSAAI::Event_ClosestReachableEnemyOfEntity( anEntity *team_mate ) {
	anActor *actor;
	anActor *ent;
	anActor	*bestEnt;
	float	bestDistSquared;
	float	distSquared;
	anVec3	delta;
	int		areaNum;
	int		enemyAreaNum;
	seasPath_t path;



	if ( !team_mate->IsType( anActor::GetClassType() ) ) {

		gameLocal.Error( "Entity '%s' is not an AI character or player", team_mate->GetName() );
	}

	actor = static_cast<anActor *>( team_mate );

	const anVec3 &origin = physicsObj.GetOrigin();
	areaNum = PointReachableAreaNum( origin );

	bestDistSquared = anMath::INFINITY;
	bestEnt = nullptr;
	for ( ent = actor->enemyList.Next(); ent != nullptr; ent = ent->enemyNode.Next() ) {
		if ( ent->fl.hidden ) {
			continue;
		}
		delta = ent->GetPhysics()->GetOrigin() - origin;
		distSquared = delta.LengthSqr();
		if ( distSquared < bestDistSquared ) {
			const anVec3 &enemyPos = ent->GetPhysics()->GetOrigin();
			enemyAreaNum = PointReachableAreaNum( enemyPos );
			if ( ( areaNum != 0 ) && PathToGoal( path, areaNum, origin, enemyAreaNum, enemyPos ) ) {
				bestEnt = ent;
				bestDistSquared = distSquared;
			}
		}
	}

	anThread::ReturnEntity( bestEnt );
}

/*
=====================
anSAAI::Event_EntityInAttackCone
=====================
*/
void anSAAI::Event_EntityInAttackCone( anEntity *ent ) {
	float	attack_cone;
	anVec3	delta;
	float	yaw;
	float	relYaw;

	if ( !ent ) {
		anThread::ReturnInt( false );
		return;
	}

	delta = ent->GetPhysics()->GetOrigin() - GetEyePosition();

	// get our gravity normal
	const anVec3 &gravityDir = GetPhysics()->GetGravityNormal();

	// infinite vertical vision, so project it onto our orientation plane
	delta -= gravityDir * ( gravityDir * delta );

	delta.Normalize();
	yaw = delta.ToYaw();

	attack_cone = spawnArgs.GetFloat( "attack_cone", "70" );
	relYaw = anMath::AngleNormalize180( move.ideal_yaw - yaw );
	if ( anMath::Fabs( relYaw ) < ( attack_cone * 0.5f ) ) {
		anThread::ReturnInt( true );
	} else {
		anThread::ReturnInt( false );
	}
}

/*
================
anSAAI::Event_CanReachPosition
================
*/
void anSAAI::Event_CanReachPosition( const anVec3 &pos ) {
	seasPath_t	path;
	int			toAreaNum;
	int			areaNum;

	toAreaNum = PointReachableAreaNum( pos );
	areaNum	= PointReachableAreaNum( physicsObj.GetOrigin() );
	if ( !toAreaNum || !PathToGoal( path, areaNum, physicsObj.GetOrigin(), toAreaNum, pos ) ) {
		anThread::ReturnInt( false );
	} else {
		anThread::ReturnInt( true );
	}
}

/*
================
anSAAI::Event_CanReachEntity
================
*/
void anSAAI::Event_CanReachEntity( anEntity *ent ) {
	seasPath_t	path;
	int			toAreaNum;
	int			areaNum;
	anVec3		pos;

	if ( !ent ) {
		anThread::ReturnInt( false );
		return;
	}

	if ( move.moveType != MOVETYPE_FLY ) {
		if ( !ent->GetFloorPos( 64.0f, pos ) ) {
			anThread::ReturnInt( false );
			return;
		}


		if ( ent->IsType( anActor::GetClassType() ) && static_cast<anActor *>( ent )->OnLadder() ) {

			anThread::ReturnInt( false );
			return;
		}
	} else {
		pos = ent->GetPhysics()->GetOrigin();
	}

	toAreaNum = PointReachableAreaNum( pos );
	if ( !toAreaNum ) {
		anThread::ReturnInt( false );
		return;
	}

	const anVec3 &org = physicsObj.GetOrigin();
	areaNum	= PointReachableAreaNum( org );
	if ( !toAreaNum || !PathToGoal( path, areaNum, org, toAreaNum, pos ) ) {
		anThread::ReturnInt( false );
	} else {
		anThread::ReturnInt( true );
	}
}

/*
================
anSAAI::Event_CanReachEnemy
================
*/
void anSAAI::Event_CanReachEnemy( void ) {
	seasPath_t	path;
	int			toAreaNum = 0;
	int			areaNum;
	anVec3		pos;
	anEntity	*enemyEnt;

	enemyEnt = enemy.ent;
	if ( !enemyEnt ) {
		anThread::ReturnInt( false );
		return;
	}

	if ( move.moveType != MOVETYPE_FLY ) {
		if ( enemyEnt->IsType( anActor::GetClassType() ) ){
			anActor *enemyAct = static_cast<anActor *>( enemyEnt );
			if ( enemyAct->OnLadder() ) {
				anThread::ReturnInt( false );
				return;
			}
			enemyAct->GetAASLocation( aas, pos, toAreaNum );
		}
	}  else {
		pos = enemyEnt->GetPhysics()->GetOrigin();
		toAreaNum = PointReachableAreaNum( pos );
	}

	if ( !toAreaNum ) {
		anThread::ReturnInt( false );
		return;
	}

	const anVec3 &org = physicsObj.GetOrigin();
	areaNum	= PointReachableAreaNum( org );
	if ( !PathToGoal( path, areaNum, org, toAreaNum, pos ) ) {
		anThread::ReturnInt( false );
	} else {
		anThread::ReturnInt( true );
	}
}

/*
================
anSAAI::Event_GetReachableEntityPosition
================
*/
void anSAAI::Event_GetReachableEntityPosition( anEntity *ent ) {
	int		toAreaNum;
	anVec3	pos;

	if ( move.moveType != MOVETYPE_FLY ) {
		if ( !ent->GetFloorPos( 64.0f, pos ) ) {
			anThread::ReturnInt( false );
			return;
		}


		if ( ent->IsType( anActor::GetClassType() ) && static_cast<anActor *>( ent )->OnLadder() ) {

			anThread::ReturnInt( false );
			return;
		}
	} else {
		pos = ent->GetPhysics()->GetOrigin();
	}

	if ( aas ) {
		toAreaNum = PointReachableAreaNum( pos );
		aas->PushPointIntoAreaNum( toAreaNum, pos );
	}

	anThread::ReturnVector( pos );
}

/*
================
anSAAI::Event_ScriptedMove
================
*/
void anSAAI::Event_ScriptedMove ( anEntity* destEnt, float minDist, bool endWithIdle ) {
	ScriptedMove ( destEnt, minDist, endWithIdle );
}

/*
================
anSAAI::Event_ScriptedFace
================
*/
void anSAAI::Event_ScriptedFace ( anEntity* faceEnt, bool endWithIdle ) {
	ScriptedFace ( faceEnt, endWithIdle );
}

/*
================
anSAAI::Event_ScriptedAnim
================
*/
void anSAAI::Event_ScriptedAnim ( const char* animname, int blendFrames, bool loop, bool endWithIdle ) {
	ScriptedAnim ( animname, blendFrames, loop, endWithIdle );
}

/*
================
anSAAI::Event_ScriptedAction
================
*/
void anSAAI::Event_ScriptedAction ( anEntity* actionEnt, bool endWithIdle ) {
	ScriptedAction ( actionEnt, endWithIdle );
}

/*
================
anSAAI::Event_ScriptedPlaybackMove
================
*/
void anSAAI::Event_ScriptedPlaybackMove ( const char* playback, int flags, int numFrames ) {
	ScriptedPlaybackMove ( playback, flags, numFrames );
}

/*
================
anSAAI::Event_ScriptedPlaybackAim
================
*/
void anSAAI::Event_ScriptedPlaybackAim( const char* playback, int flags, int numFrames ) {
	ScriptedPlaybackAim ( playback, flags, numFrames );
}

/*
================
anSAAI::Event_ScriptedDone
================
*/
void anSAAI::Event_ScriptedDone ( void ) {
	anThread::ReturnFloat ( !aifl.scripted );
}

/*
================
anSAAI::Event_ScriptedStop
================
*/
void anSAAI::Event_ScriptedStop ( void ) {
	ScriptedStop();
}

/*
================
anSAAI::Event_AllowHiddenMovement
================
*/
void anSAAI::Event_AllowHiddenMovement( int enable ) {
	move.fl.allowHiddenMove = ( enable != 0 );
}

/*
================
anSAAI::Event_SetScript
================
*/
void anSAAI::Event_SetScript ( const char* scriptName, const char* funcName ) {
	SetScript ( scriptName, funcName );
}

/*
================
anSAAI::Event_SetMoveSpeed
================
*/
void anSAAI::Event_SetMoveSpeed ( int speed ) {
	switch ( speed ) {
		case AIMOVESPEED_DEFAULT:
			move.fl.noRun = false;
			move.fl.noWalk = false;
			break;

		case AIMOVESPEED_RUN:
			move.fl.noRun = false;
			move.fl.noWalk = true;
			break;

		case AIMOVESPEED_WALK:
			move.fl.noRun = true;
			move.fl.noWalk = false;
			break;
	}
}

/*
================
anSAAI::Event_SetPassivePrefix
================
*/
void anSAAI::Event_SetPassivePrefix ( const char* prefix ) {
	SetPassivePrefix ( prefix );
}

/*
================
anSAAI::Event_Attack
================
*/
void anSAAI::Event_Attack ( const char* attackName, const char* jointName ) {
	Attack ( attackName, animator.GetJointHandle ( jointName ), enemy.ent ); // , physicsObj.GetPushedLinearVelocity() );
}

/*
================
anSAAI::Event_AttackMelee
================
*/
void anSAAI::Event_AttackMelee( const char* meleeName ) {
	const anDict* meleeDict;
	meleeDict = gameLocal.FindEntityDefDict ( spawnArgs.GetString ( va( "def_attack_%s", meleeName ) ), false );
	if ( !meleeDict ) {
		gameLocal.Error ( "missing meleeDef '%s' for ai entity '%s'", meleeName, GetName() );
	}
	AttackMelee ( meleeName, meleeDict );
}

/*
================
anSAAI::Event_ScriptedJumpDown
================
*/
void anSAAI::Event_ScriptedJumpDown( float yaw ) {
	if ( animator.HasAnim( "jumpdown_start" ) )
	{
		aifl.scripted = true;
		move.ideal_yaw = yaw;
		SetState( "State_ScriptedJumpDown" );
	}
}

/*
================
anSAAI::Event_FindEnemy
================
*/
void anSAAI::Event_FindEnemy( float distSqr )	{
		anThread::ReturnEntity ( FindEnemy( false, 1, distSqr ) );
}

/*
================
anSAAI::Event_SetKey
================
*/
void anSAAI::Event_SetKey( const char *key, const char *value ) {
	spawnArgs.Set( key, value );

	OnSetKey ( key, value );
}

/*
================
anSAAI::Event_PostSpawn
================
*/
void anSAAI::Event_PostSpawn( void ) {

	// twhitaker: difficulty levels
	if ( team == TEAM_MARINE ) {
		//health /= 1.0f + gameLocal.GetDifficultyModifier( );

		//buddies are a little more healthy on hard & nightmare since the baddies deal so much more damage
		switch ( g_skill.GetInteger() ) {
		case 3:
			health *= 1.4f;
			break;
		case 2:
			health *= 1.2f;
			break;
		case 0:
			health *= 1.2f;
			break;
		case 1:
		default:
			break;
		}
	} else {
		health *= 1.0f + gameLocal.GetDifficultyModifier( );
	}

}
