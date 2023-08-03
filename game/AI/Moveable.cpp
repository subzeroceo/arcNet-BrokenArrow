
// bdube: note that this file is no longer merged with Doom3 updates
//
// MERGE_DATE 09/30/2004

#include "../idlib/Lib.h"
#pragma hdrstop

#include "Game_local.h"

#if !defined(__GAME_PROJECTILE_H__)
	#include "Projectile.h"
#endif

/*
===============================================================================

  idMoveable

===============================================================================
*/

const anEventDef EV_BecomeNonSolid( "becomeNonSolid" );
const anEventDef EV_SetOwnerFromSpawnArgs( "<setOwnerFromSpawnArgs>" );
const anEventDef EV_IsAtRest( "isAtRest", nullptr, 'd' );
const anEventDef EV_CanDamage( "canDamage", "f" );
const anEventDef EV_SetHealth( "setHealth", "f" );
const anEventDef EV_RadiusDamage( "<radiusDamage>", "es" );

CLASS_DECLARATION( idDamagable, idMoveable )
	EVENT( EV_Activate,					idMoveable::Event_Activate )
	EVENT( EV_BecomeNonSolid,			idMoveable::Event_BecomeNonSolid )
	EVENT( EV_SetOwnerFromSpawnArgs,	idMoveable::Event_SetOwnerFromSpawnArgs )
	EVENT( EV_IsAtRest,					idMoveable::Event_IsAtRest )
	EVENT( EV_CanDamage,				idMoveable::Event_CanDamage )
	EVENT( EV_SetHealth,				idMoveable::Event_SetHealth )
	EVENT( EV_RadiusDamage,				idMoveable::Event_RadiusDamage )
END_CLASS

static const float BOUNCE_SOUND_MIN_VELOCITY	= 80.0f;
static const float BOUNCE_SOUND_MAX_VELOCITY	= 400.0f;
static const int   BOUNCE_SOUND_DELAY_MIN		= 500.0f;
static const int   BOUNCE_SOUND_DELAY_MAX		= 1200.0f;

/*
================
idMoveable::idMoveable
================
*/
idMoveable::idMoveable( void ) {
 	minDamageVelocity	= 100.0f;
 	maxDamageVelocity	= 200.0f;
	nextCollideFxTime	= 0;
 	initialSpline		= nullptr;
 	initialSplineDir	= vec3_zero;
	unbindOnDeath		= false;
	allowStep			= false;
 	canDamage			= false;

	lastAttacker		= nullptr;
}

/*
================
idMoveable::~idMoveable
================
*/
idMoveable::~idMoveable( void ) {
 	delete initialSpline;
 	initialSpline = nullptr;
 	SetPhysics( nullptr );
}

/*
================
idMoveable::Spawn
================
*/
void idMoveable::Spawn( void ) {
	anTraceModel trm;
	float density, friction, bouncyness;
	int clipShrink;
	anString clipModelName;
	bool setClipModel = false;
	anBounds bounds;

	// check if a clip model is set
	spawnArgs.GetString( "clipmodel", "", clipModelName );
	if ( !clipModelName[0] ) {
		anVec3 size;
		if ( spawnArgs.GetVector( "mins", nullptr, bounds[0] ) &&
			spawnArgs.GetVector( "maxs", nullptr, bounds[1] ) ) {
			setClipModel = true;
			if ( bounds[0][0] > bounds[1][0] || bounds[0][1] > bounds[1][1] || bounds[0][2] > bounds[1][2] ) {
				gameLocal.Error( "Invalid bounds '%s'-'%s' on moveable '%s'", bounds[0].ToString(), bounds[1].ToString(), name.c_str() );
			}
		} else if ( spawnArgs.GetVector( "size", nullptr, size ) ) {
			if ( ( size.x < 0.0f ) || ( size.y < 0.0f ) || ( size.z < 0.0f ) ) {
				gameLocal.Error( "Invalid size '%s' on moveable '%s'", size.ToString(), name.c_str() );
			}
			bounds[0].Set( size.x * -0.5f, size.y * -0.5f, 0.0f );
			bounds[1].Set( size.x * 0.5f, size.y * 0.5f, size.z );
			setClipModel = true;
		}
	}

	if ( setClipModel ) {
		trm.SetupBox( bounds );
	} else {
		if ( !clipModelName[0] ) {
			clipModelName = spawnArgs.GetString ( "model" );		// use the visual model
		}
		clipModelName.BackSlashesToSlashes();

		if ( !collisionModelManager->TrmFromModel( gameLocal.GetMapName(), clipModelName, trm ) ) {
			gameLocal.Error( "idMoveable '%s': cannot load collision model %s", name.c_str(), clipModelName.c_str() );
			return;
		}
	}

	// if the model should be shrinked
	clipShrink = spawnArgs.GetInt( "clipshrink" );
	if ( clipShrink != 0 ) {
		trm.Shrink( clipShrink * CM_CLIP_EPSILON );
	}

	// get rigid body properties
	spawnArgs.GetFloat( "density", "0.5", density );
	density = anMath::ClampFloat( 0.001f, 1000.0f, density );
	spawnArgs.GetFloat( "friction", "0.05", friction );
	friction = anMath::ClampFloat( 0.0f, 1.0f, friction );
	spawnArgs.GetFloat( "bouncyness", "0.6", bouncyness );
	bouncyness = anMath::ClampFloat( 0.0f, 1.0f, bouncyness );
	unbindOnDeath = spawnArgs.GetBool( "unbindondeath" );

	nextCollideFxTime = 0;

	damage = spawnArgs.GetString( "def_damage", "" );
	canDamage = spawnArgs.GetBool( "damageWhenActive" ) ? false : true;
	health = spawnArgs.GetInt( "health", "0" );
	spawnArgs.GetString( "broken", "", brokenModel );

	if ( health ) {
		if ( brokenModel != "" && !renderModelManager->CheckModel( brokenModel ) ) {
			gameLocal.Error( "idMoveable '%s' at (%s): cannot load broken model '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString(0), brokenModel.c_str() );
		}
	}

	fl.takedamage = (health > 0 );

	// setup the physics
	physicsObj.SetSelf( this );

// mwhitlock: Dynamic memory consolidation
	PUSH_HEAP_MEM( this );

	physicsObj.SetClipModel( new anClipModel( trm, GetRenderModelMaterial() ), density );

// mwhitlock: Dynamic memory consolidation
	POP_HEAP();

	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetBouncyness( bouncyness );
	physicsObj.SetFriction( 0.6f, 0.6f, friction );
	physicsObj.SetGravity( gameLocal.GetGravity() );
	physicsObj.SetContents( CONTENTS_SOLID );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_ACTORBODY | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP | CONTENTS_WATER );
	SetPhysics( &physicsObj );

	if ( spawnArgs.GetBool( "nodrop" ) ) {
		physicsObj.PutToRest();
	} else {
		physicsObj.DropToFloor();
	}

	if ( spawnArgs.GetBool( "noimpact" ) || spawnArgs.GetBool( "notPushable" ) ) {
		physicsObj.DisableImpact();
	}

	if ( spawnArgs.GetBool( "nonsolid" ) ) {
		BecomeNonSolid();
	}

	allowStep = spawnArgs.GetBool( "allowStep", "1" );


// cdr: Obstacle Avoidance
	fl.isAIObstacle = !physicsObj.IsPushable();


	PostEventMS( &EV_SetOwnerFromSpawnArgs, 0 );
}

/*
================
idMoveable::Save
================
*/
void idMoveable::Save( anSaveGame *savefile ) const {

	savefile->WriteStaticObject( physicsObj );

	savefile->WriteString( brokenModel );
	savefile->WriteString( damage );
	savefile->WriteInt( nextCollideFxTime );
 	savefile->WriteFloat( minDamageVelocity );
 	savefile->WriteFloat( maxDamageVelocity );

	savefile->WriteInt( initialSpline != nullptr ? initialSpline->GetTime( 0 ) : -1 );
	savefile->WriteVec3( initialSplineDir );

	savefile->WriteBool( unbindOnDeath );
	savefile->WriteBool( allowStep );
	savefile->WriteBool( canDamage );

	lastAttacker.Save( savefile);		// cnicholson: Added unsaved var
}

/*
================
idMoveable::Restore
================
*/
void idMoveable::Restore( anRestoreGame *savefile ) {
   	int initialSplineTime;

	savefile->ReadStaticObject( physicsObj );

	savefile->ReadString( brokenModel );
	savefile->ReadString( damage );
	savefile->ReadInt( nextCollideFxTime );
 	savefile->ReadFloat( minDamageVelocity );
 	savefile->ReadFloat( maxDamageVelocity );

	savefile->ReadInt( initialSplineTime );
	if ( initialSplineTime != -1 ) {
   		InitInitialSpline( initialSplineTime );
 	} else {
 		initialSpline = nullptr;
   	}

	savefile->ReadVec3( initialSplineDir );
	savefile->ReadBool( unbindOnDeath );
	savefile->ReadBool( allowStep );
	savefile->ReadBool( canDamage );

	lastAttacker.Restore( savefile);

   	RestorePhysics( &physicsObj );
}

/*
================
idMoveable::Hide
================
*/
void idMoveable::Hide( void ) {

// abahr: changed parent scope
	idDamagable::Hide();
	physicsObj.SetContents( 0 );
}

/*
================
idMoveable::Show
================
*/
void idMoveable::Show( void ) {

// abahr: changed parent scope
	idDamagable::Show();
	if ( !spawnArgs.GetBool( "nonsolid" ) ) {
		physicsObj.SetContents( CONTENTS_SOLID );
	}
}

/*
=================
idMoveable::Collide
=================
*/
bool idMoveable::Collide( const trace_t &collision, const anVec3 &velocity ) {
	float len, f;
	anVec3 dir;
	anEntity *ent;

	dir = velocity;
	len = dir.NormalizeFast();

	if ( len > BOUNCE_SOUND_MIN_VELOCITY ) {
		if ( gameLocal.time > nextCollideFxTime ) {
			PlayEffect ( gameLocal.GetEffect( spawnArgs,"fx_collide",collision.c.materialType), collision.c.point, collision.c.normal.ToMat3() );

// jscott: fixed negative sqrt call
			if ( len > BOUNCE_SOUND_MAX_VELOCITY ) {
				f = 1.0f;
			} else if ( len <= BOUNCE_SOUND_MIN_VELOCITY ) {
				f = 0.0f;
			} else {
				f = ( len - BOUNCE_SOUND_MIN_VELOCITY ) * ( 1.0f / ( BOUNCE_SOUND_MAX_VELOCITY - BOUNCE_SOUND_MIN_VELOCITY ) );
			}

			SetSoundVolume( f );
			StartSound( "snd_bounce", SND_CHANNEL_BODY, 0, false, nullptr );

			nextCollideFxTime = gameLocal.time + BOUNCE_SOUND_DELAY_MIN + gameLocal.random.RandomInt(BOUNCE_SOUND_DELAY_MAX - BOUNCE_SOUND_DELAY_MIN);
		}
	}

	if ( canDamage && damage.Length() ) {
		ent = gameLocal.entities[ collision.c.entityNum ];
		if ( ent && len > minDamageVelocity ) {

// jscott: fixed negative sqrt call
			if ( len > maxDamageVelocity ) {
				f = 1.0f;
			} else if ( len <= minDamageVelocity ) {
				f = 0.0f;
			} else {
				f = anMath::Sqrt( len - minDamageVelocity ) * ( 1.0f / anMath::Sqrt( maxDamageVelocity - minDamageVelocity ) );
			}

			ent->Damage( this, GetPhysics()->GetClipModel()->GetOwner(), dir, damage, f, INVALID_JOINT );
		}
	}

	return false;
}

/*
============
idMoveable::Damage
============
*/
void idMoveable::Damage( anEntity *inflictor, anEntity *attacker, const anVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	idDamagable::Damage ( inflictor, attacker, dir, damageDefName, damageScale, location );

	// Cache the attacker to ensure credit for a kill is given to the entity that caused the damage
	lastAttacker = inflictor;
// jshepard:
//	gameLocal.Warning( "idMoveable Damaged! Health is %d", health);

}

/*
============
idMoveable::Killed
============
*/
void idMoveable::Killed( anEntity *inflictor, anEntity *attacker, int damage, const anVec3 &dir, int location ) {

// jshepard: I am dead!
//	gameLocal.Warning( "idMoveable Killed! Health is %d", health);

/*
	// No more taking damage
	fl.takedamage = false;

	// Should the moveable launch around when killed?
	int launchTime;
	launchTime = SEC2MS ( spawnArgs.GetFloat ( "launch_time", "0" );
	if ( launchTime > 0 ) {
		launchOffset = spawnArgs.GetVector ( "launch_offset" );
		launchDir    = spawnArgs.GetVector ( "
	}

	spawnArgs.GetFloat ( "explode_lapse",
	PostEventSec ( &EV_Remove, explodeLapse );

	// Two part explosion?
	explode_impulse
	explode_lapse

	if ( unbindOnDeath ) {
		Unbind();
	}

	if ( brokenModel != "" ) {
		SetModel( brokenModel );
	}

	if ( explode ) {
		if ( brokenModel == "" ) {
			Hide();
			physicsObj.PutToRest();
			GetPhysics()->SetContents( 0 );
			PostEventMS( &EV_Remove, 0 );
		}

		const char *splash = spawnArgs.GetString( "def_splash_damage", "" );
		if ( splash && *splash ) {
			gameLocal.RadiusDamage( GetPhysics()->GetOrigin(), inflictor, inflictor, this, this, splash );
		}

		StartSound( "snd_explode", SND_CHANNEL_ANY );

		StopAllEffects();
		gameLocal.PlayEffect ( gameLocal.GetEffect( spawnArgs, "fx_explode" ), GetPhysics()->GetOrigin(), (-GetPhysics()->GetGravityNormal()).ToMat3(), false, vec3_origin, true );
	}

	if ( renderEntity.gui[ 0 ] ) {
		renderEntity.gui[ 0 ] = nullptr;
	}

	ActivateTargets( this );

	fl.takedamage = false;
*/
}


/*
================
idMoveable::ExecuteStage
================
*/
void idMoveable::ExecuteStage ( void ) {
	// Splash damage?
	const char *splash;
	if ( stageDict->GetString( "def_splash_damage", "", &splash ) && *splash ) {
//		gameLocal.RadiusDamage( GetPhysics()->GetOrigin(), this, lastAttacker.GetEntity()?lastAttacker.GetEntity():this, this, this, splash );
		PostEventMS( &EV_RadiusDamage, 0, lastAttacker.GetEntity()?lastAttacker.GetEntity():this, splash );
	}

	idDamagable::ExecuteStage();
}

/*
================
idMoveable::AddDamageEffect
================
*/
void idMoveable::AddDamageEffect ( const trace_t &collision, const anVec3 &velocity, const char *damageDefName, anEntity* inflictor ) {

	// Play an impact effect during this stage?
	if ( stageDict ) {
		PlayEffect ( gameLocal.GetEffect ( *stageDict, "fx_impact" ),
					 collision.c.point, collision.c.normal.ToMat3(),
					 true, vec3_origin, true );
	}
}

/*
================
idMoveable::AllowStep
================
*/
bool idMoveable::AllowStep( void ) const {
	return allowStep;
}

/*
================
idMoveable::BecomeNonSolid
================
*/
void idMoveable::BecomeNonSolid( void ) {
	// set CONTENTS_RENDERMODEL so bullets still collide with the moveable
	physicsObj.SetContents( CONTENTS_CORPSE | CONTENTS_RENDERMODEL );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_CORPSE | CONTENTS_MOVEABLECLIP );
}

/*
================
idMoveable::EnableDamage
================
*/
void idMoveable::EnableDamage( bool enable, float duration ) {
	canDamage = enable;
	if ( duration ) {
		PostEventSec( &EV_CanDamage, duration, ( !enable ) ? 0.0f : 1.0f );
	}
}

/*
================
idMoveable::InitInitialSpline
================
*/
void idMoveable::InitInitialSpline( int startTime ) {
	int initialSplineTime;

	initialSpline = GetSpline();
	initialSplineTime = spawnArgs.GetInt( "initialSplineTime", "300" );

	if ( initialSpline != nullptr ) {
		initialSpline->MakeUniform( initialSplineTime );
		initialSpline->ShiftTime( startTime - initialSpline->GetTime( 0 ) );
		initialSplineDir = initialSpline->GetCurrentFirstDerivative( startTime );
		initialSplineDir *= physicsObj.GetAxis().Transpose();
		initialSplineDir.Normalize();
		BecomeActive( TH_THINK );
	}
}

/*
================
idMoveable::FollowInitialSplinePath
================
*/
bool idMoveable::FollowInitialSplinePath( void ) {
	if ( initialSpline != nullptr ) {
		if ( gameLocal.time < initialSpline->GetTime( initialSpline->GetNumValues() - 1 ) ) {
			anVec3 splinePos = initialSpline->GetCurrentValue( gameLocal.time );
			anVec3 linearVelocity = ( splinePos - physicsObj.GetOrigin() ) * gameLocal.GetMHz();
			physicsObj.SetLinearVelocity( linearVelocity );

			anVec3 splineDir = initialSpline->GetCurrentFirstDerivative( gameLocal.time );
			anVec3 dir = initialSplineDir * physicsObj.GetAxis();
			anVec3 angularVelocity = dir.Cross( splineDir );
			angularVelocity.Normalize();
			angularVelocity *= anMath::ACos16( dir * splineDir / splineDir.Length() ) * gameLocal.GetMHz();
			physicsObj.SetAngularVelocity( angularVelocity );
			return true;
		} else {
			delete initialSpline;
			initialSpline = nullptr;
		}
	}
	return false;
}

/*
================
idMoveable::Think
================
*/
void idMoveable::Think( void ) {
	if ( thinkFlags & TH_THINK ) {
		// Move to the next stage?
		UpdateStage();

		if ( !FollowInitialSplinePath() && stage == -1 ) {
			BecomeInactive( TH_THINK );
		}
	}

// abahr: changed parent scope
	idDamagable::Think();
}

/*
================
idMoveable::GetRenderModelMaterial
================
*/
const anMaterial *idMoveable::GetRenderModelMaterial( void ) const {
	if ( renderEntity.customShader ) {
		return renderEntity.customShader;
	}
	if ( renderEntity.hModel && renderEntity.hModel->NumSurfaces() ) {
		 return renderEntity.hModel->Surface( 0 )->shader;
	}
	return nullptr;
}

/*
================
idMoveable::WriteToSnapshot
================
*/
void idMoveable::WriteToSnapshot( anBitMsgDelta &msg ) const {
	physicsObj.WriteToSnapshot( msg );
}

/*
================
idMoveable::ReadFromSnapshot
================
*/
void idMoveable::ReadFromSnapshot( const anBitMsgDelta &msg ) {
	physicsObj.ReadFromSnapshot( msg );
	if ( msg.HasChanged() ) {
		UpdateVisuals();
	}
}

/*
================
idMoveable::Event_BecomeNonSolid
================
*/
void idMoveable::Event_BecomeNonSolid( void ) {
	BecomeNonSolid();
}

/*
================
idMoveable::Event_Activate
================
*/
void idMoveable::Event_Activate( anEntity *activator ) {
	float delay;
	anVec3 init_velocity, init_avelocity;

	Show();

	if ( !spawnArgs.GetInt( "notPushable" ) ) {
        physicsObj.EnableImpact();
	}

	physicsObj.Activate();

	spawnArgs.GetVector( "init_velocity", "0 0 0", init_velocity );
	spawnArgs.GetVector( "init_avelocity", "0 0 0", init_avelocity );

	delay = spawnArgs.GetFloat( "init_velocityDelay", "0" );
	if ( delay == 0.0f ) {
		physicsObj.SetLinearVelocity( init_velocity );
	} else {
		PostEventSec( &EV_SetLinearVelocity, delay, init_velocity );
	}

	delay = spawnArgs.GetFloat( "init_avelocityDelay", "0" );
	if ( delay == 0.0f ) {
		physicsObj.SetAngularVelocity( init_avelocity );
	} else {
		PostEventSec( &EV_SetAngularVelocity, delay, init_avelocity );
	}

	InitInitialSpline( gameLocal.time );


// jshepard: we should update it's stage on activation, specifically for falling blocks.
	UpdateStage();


}

/*
================
idMoveable::Event_SetOwnerFromSpawnArgs
================
*/
void idMoveable::Event_SetOwnerFromSpawnArgs( void ) {
	anString owner;

	if ( spawnArgs.GetString( "owner", "", owner ) ) {
		ProcessEvent( &EV_SetOwner, gameLocal.FindEntity( owner ) );
	}
}

/*
================
idMoveable::Event_IsAtRest
================
*/
void idMoveable::Event_IsAtRest( void ) {
	anThread::ReturnInt( physicsObj.IsAtRest() );
}

/*
================
idMoveable::Event_CanDamage
================
*/
void idMoveable::Event_CanDamage( float enable ) {
	canDamage = ( enable != 0.0f );
}

/*
================
idMoveable::Event_SetHealth
================
*/
void idMoveable::Event_SetHealth( float newHealth ) {
	health = newHealth;
}

/*
================
idMoveable::Event_RadiusDamage
================
*/
void idMoveable::Event_RadiusDamage( anEntity *attacker, const char* splash ) {
	gameLocal.RadiusDamage( GetPhysics()->GetOrigin(), this, attacker, this, this, splash );
}

/*
===============================================================================

  idBarrel

===============================================================================
*/

CLASS_DECLARATION( idMoveable, idBarrel )
END_CLASS

/*
================
idBarrel::idBarrel
================
*/
idBarrel::idBarrel() {
	radius = 1.0f;
	barrelAxis = 0;
	lastOrigin.Zero();
	lastAxis.Identity();
	additionalRotation = 0.0f;
	additionalAxis.Identity();
	fl.networkSync = true;
}

/*
================
idBarrel::Save
================
*/
void idBarrel::Save( anSaveGame *savefile ) const {
	savefile->WriteFloat( radius );
	savefile->WriteInt( barrelAxis );
	savefile->WriteVec3( lastOrigin );
	savefile->WriteMat3( lastAxis );
	savefile->WriteFloat( additionalRotation );
	savefile->WriteMat3( additionalAxis );
}

/*
================
idBarrel::Restore
================
*/
void idBarrel::Restore( anRestoreGame *savefile ) {
	savefile->ReadFloat( radius );
	savefile->ReadInt( barrelAxis );
	savefile->ReadVec3( lastOrigin );
	savefile->ReadMat3( lastAxis );
	savefile->ReadFloat( additionalRotation );
	savefile->ReadMat3( additionalAxis );
}

/*
================
idBarrel::BarrelThink
================
*/
void idBarrel::BarrelThink( void ) {
	bool wasAtRest, onGround;
	float movedDistance, rotatedDistance, angle;
	anVec3 curOrigin, gravityNormal, dir;
	anMat3 curAxis, axis;

	wasAtRest = IsAtRest();

	// Progress to the next stage?
	UpdateStage();

	// run physics
	RunPhysics();

	// only need to give the visual model an additional rotation if the physics were run
	if ( !wasAtRest ) {

		// current physics state
		onGround = GetPhysics()->HasGroundContacts();
		curOrigin = GetPhysics()->GetOrigin();
		curAxis = GetPhysics()->GetAxis();

		// if the barrel is on the ground
		if ( onGround ) {
			gravityNormal = GetPhysics()->GetGravityNormal();

			dir = curOrigin - lastOrigin;
			dir -= gravityNormal * dir * gravityNormal;
			movedDistance = dir.LengthSqr();

			// if the barrel moved and the barrel is not aligned with the gravity direction
			if ( movedDistance > 0.0f && anMath::Fabs( gravityNormal * curAxis[barrelAxis] ) < 0.7f ) {

				// barrel movement since last think frame orthogonal to the barrel axis
				movedDistance = anMath::Sqrt( movedDistance );
				dir *= 1.0f / movedDistance;
				movedDistance = ( 1.0f - anMath::Fabs( dir * curAxis[barrelAxis] ) ) * movedDistance;

				// get rotation about barrel axis since last think frame
				angle = lastAxis[(barrelAxis+1)%3] * curAxis[(barrelAxis+1)%3];
				angle = anMath::ACos( angle );
				// distance along cylinder hull
				rotatedDistance = angle * radius;

				// if the barrel moved further than it rotated about it's axis
				if ( movedDistance > rotatedDistance ) {

					// additional rotation of the visual model to make it look
					// like the barrel rolls instead of slides
					angle = 180.0f * (movedDistance - rotatedDistance) / (radius * anMath::PI);
					if ( gravityNormal.Cross( curAxis[barrelAxis] ) * dir < 0.0f ) {
						additionalRotation += angle;
					} else {
						additionalRotation -= angle;
					}
					dir = vec3_origin;
					dir[barrelAxis] = 1.0f;
					additionalAxis = anRotation( vec3_origin, dir, additionalRotation ).ToMat3();
				}
			}
		}

		// save state for next think
		lastOrigin = curOrigin;
		lastAxis = curAxis;
	}

	Present();
}

/*
================
idBarrel::Think
================
*/
void idBarrel::Think( void ) {
	if ( thinkFlags & TH_THINK ) {
		if ( !FollowInitialSplinePath() && stage == -1 ) {
			BecomeInactive( TH_THINK );
		}
	}

	BarrelThink();
}

/*
================
idBarrel::GetPhysicsToVisualTransform
================
*/
bool idBarrel::GetPhysicsToVisualTransform( anVec3 &origin, anMat3 &axis ) {
	origin = vec3_origin;
	axis = additionalAxis;
	return true;
}

/*
================
idBarrel::Spawn
================
*/
void idBarrel::Spawn( void ) {
	const anBounds &bounds = GetPhysics()->GetBounds();

	// radius of the barrel cylinder
	radius = ( bounds[1][0] - bounds[0][0] ) * 0.5f;

	// always a vertical barrel with cylinder axis parallel to the z-axis
	barrelAxis = 2;

	lastOrigin = GetPhysics()->GetOrigin();
	lastAxis = GetPhysics()->GetAxis();

	additionalRotation = 0.0f;
	additionalAxis.Identity();
}

/*
================
idBarrel::ClientPredictionThink
================
*/
void idBarrel::ClientPredictionThink( void ) {
	Think();
}


/*
===============================================================================

idExplodingBarrel

===============================================================================
*/
const anEventDef EV_Respawn( "<respawn>" );
const anEventDef EV_TriggerTargets( "<triggertargets>" );

CLASS_DECLARATION( idBarrel, idExplodingBarrel )
	EVENT( EV_Activate,					idExplodingBarrel::Event_Activate )
	EVENT( EV_Respawn,					idExplodingBarrel::Event_Respawn )
	EVENT( EV_Explode,					idExplodingBarrel::Event_Explode )
	EVENT( EV_TriggerTargets,			idExplodingBarrel::Event_TriggerTargets )
END_CLASS

/*
================
idExplodingBarrel::idExplodingBarrel
================
*/
idExplodingBarrel::idExplodingBarrel() {
	spawnOrigin.Zero();
	spawnAxis.Zero();
	state = NORMAL;
	ipsHandle = -1;
	lightHandle = -1;
	memset( &ipsEntity, 0, sizeof( ipsEntity ) );
	memset( &light, 0, sizeof( light ) );
	ipsTime = 0;
	lightTime = 0;
	time = 0.0f;
 	explodeFinishTime = 0;
}

/*
================
idExplodingBarrel::~idExplodingBarrel
================
*/
idExplodingBarrel::~idExplodingBarrel() {
	if ( ipsHandle >= 0 ){
		gameRenderWorld->FreeEntityDef( ipsHandle );
	}
	if ( lightHandle >= 0 ) {
		gameRenderWorld->FreeLightDef( lightHandle );
	}
}

/*
================
idExplodingBarrel::Save
================
*/
void idExplodingBarrel::Save( anSaveGame *savefile ) const {

	savefile->WriteInt( state );
	savefile->WriteVec3( spawnOrigin );
	savefile->WriteMat3( spawnAxis );
	savefile->WriteInt( ipsHandle );
	savefile->WriteInt( lightHandle );
	savefile->WriteRenderEntity( ipsEntity );
	savefile->WriteRenderLight( light );
	savefile->WriteInt( ipsTime );
	savefile->WriteInt( lightTime );
	savefile->WriteFloat( time );
	savefile->WriteInt( explodeFinishTime );
}

/*
================
idExplodingBarrel::Restore
================
*/
void idExplodingBarrel::Restore( anRestoreGame *savefile ) {

	savefile->ReadInt( ( int&)state );
	savefile->ReadVec3( spawnOrigin );
	savefile->ReadMat3( spawnAxis );
	savefile->ReadInt( ( int&)ipsHandle );
	savefile->ReadInt( ( int&)lightHandle );

	savefile->ReadRenderEntity( ipsEntity, &spawnArgs );

	savefile->ReadRenderLight( light );
	if ( lightHandle != -1 ) {
		//get the handle again as it's out of date after a restore!
		lightHandle = gameRenderWorld->AddLightDef( &light );
	}
	savefile->ReadInt( ipsTime );
	savefile->ReadInt( lightTime );
	savefile->ReadFloat( time );
	savefile->ReadInt( explodeFinishTime );

	// precache decls
	const char *splash = spawnArgs.GetString( "def_splash_damage", "damage_explosion" );
	if ( splash && *splash ) {
		declManager->FindType( DECL_ENTITYDEF, splash, false, false );
	}

 	if ( ipsHandle != -1 ) {
 		ipsHandle = gameRenderWorld->AddEntityDef( &ipsEntity );
 	}
}

/*
================
idExplodingBarrel::Spawn
================
*/
void idExplodingBarrel::Spawn( void ) {
	health = spawnArgs.GetInt( "health", "5" );
	fl.takedamage = true;
	spawnOrigin = GetPhysics()->GetOrigin();
	spawnAxis = GetPhysics()->GetAxis();
	state = NORMAL;
	ipsHandle = -1;
	lightHandle = -1;
	lightTime = 0;
	ipsTime = 0;
	time = spawnArgs.GetFloat( "time" );
	memset( &ipsEntity, 0, sizeof( ipsEntity ) );
	memset( &light, 0, sizeof( light ) );
	explodeFinishTime = 0;

	// precache decls
	const char *splash = spawnArgs.GetString( "def_splash_damage", "damage_explosion" );
	if ( splash && *splash ) {
		declManager->FindType( DECL_ENTITYDEF, splash, false, false );
	}
}

/*
================
idExplodingBarrel::Think
================
*/
void idExplodingBarrel::Think( void ) {
	idBarrel::BarrelThink();

	// MP: EXPLODED means no effect on client when updating the state
	if ( !gameLocal.isClient && state == EXPLODING ) {
		if ( gameLocal.time > explodeFinishTime ) {
			state = EXPLODED;
		}
	}

	if ( lightHandle >= 0 ){
		if ( state == BURNING ) {
			// ramp the color up over 250 ms
			float pct = (gameLocal.time - lightTime) / 250.f;
			if ( pct > 1.0f ) {
				pct = 1.0f;
			}
			light.origin = physicsObj.GetAbsBounds().GetCenter();
			light.axis = mat3_identity;
			light.shaderParms[ SHADERPARM_RED ] = pct;
			light.shaderParms[ SHADERPARM_GREEN ] = pct;
			light.shaderParms[ SHADERPARM_BLUE ] = pct;
			light.shaderParms[ SHADERPARM_ALPHA ] = pct;
			gameRenderWorld->UpdateLightDef( lightHandle, &light );
		} else {
			if ( gameLocal.time - lightTime > 250 ) {
				gameRenderWorld->FreeLightDef( lightHandle );
				lightHandle = -1;
			}
			return;
		}
	}

	if ( !gameLocal.isClient && state != BURNING && state != EXPLODING ) {
		BecomeInactive( TH_THINK );
		return;
	}

	if ( ipsHandle >= 0 ){
		ipsEntity.origin = physicsObj.GetAbsBounds().GetCenter();
		ipsEntity.axis = mat3_identity;
		gameRenderWorld->UpdateEntityDef( ipsHandle, &ipsEntity );
	}
}

/*
================
idExplodingBarrel::AddIPS
================
*/
void idExplodingBarrel::AddIPS( const char *name, bool burn ) {
	if ( name && *name ) {
		if ( ipsHandle >= 0 ){
			gameRenderWorld->FreeEntityDef( ipsHandle );
		}
		memset( &ipsEntity, 0, sizeof ( ipsEntity ) );
		const idDeclModelDef *modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, name ) );
		if ( modelDef ) {
			ipsEntity.origin = physicsObj.GetAbsBounds().GetCenter();
			ipsEntity.axis = mat3_identity;
			ipsEntity.hModel = modelDef->ModelHandle();
			float rgb = ( burn ) ? 0.0f : 1.0f;
			ipsEntity.shaderParms[ SHADERPARM_RED ] = rgb;
			ipsEntity.shaderParms[ SHADERPARM_GREEN ] = rgb;
			ipsEntity.shaderParms[ SHADERPARM_BLUE ] = rgb;
			ipsEntity.shaderParms[ SHADERPARM_ALPHA ] = rgb;
			ipsEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
			ipsEntity.shaderParms[ SHADERPARM_DIVERSITY ] = ( burn ) ? 1.0f : gameLocal.random.RandomInt( 90 );
			if ( !ipsEntity.hModel ) {
				ipsEntity.hModel = renderModelManager->FindModel( name );
			}
			ipsHandle = gameRenderWorld->AddEntityDef( &ipsEntity );
			if ( burn ) {
				BecomeActive( TH_THINK );
			}
			ipsTime = gameLocal.time;
		}
	}
}

/*
================
idExplodingBarrel::AddLight
================
*/
void idExplodingBarrel::AddLight( const char *name, bool burn ) {
	if ( lightHandle >= 0 ){
		gameRenderWorld->FreeLightDef( lightHandle );
	}
	memset( &light, 0, sizeof ( light ) );
	light.axis = mat3_identity;
	light.lightRadius.x = spawnArgs.GetFloat( "light_radius" );
	light.lightRadius.y = light.lightRadius.z = light.lightRadius.x;
	light.origin = physicsObj.GetOrigin();
	light.origin.z += 128;
	light.pointLight = true;

// dluetscher: added detail levels to render lights
	light.detailLevel = DEFAULT_LIGHT_DETAIL_LEVEL;
// dluetscher: made sure that barrels are set to no shadows
	light.noShadows = true;

	light.shader = declManager->FindMaterial( name );
	light.shaderParms[ SHADERPARM_RED ] = 2.0f;
	light.shaderParms[ SHADERPARM_GREEN ] = 2.0f;
	light.shaderParms[ SHADERPARM_BLUE ] = 2.0f;
	light.shaderParms[ SHADERPARM_ALPHA ] = 2.0f;
	lightHandle = gameRenderWorld->AddLightDef( &light );
	lightTime = gameLocal.time;
	BecomeActive( TH_THINK );
}

/*
================
idExplodingBarrel::ExplodingEffects
================
*/
void idExplodingBarrel::ExplodingEffects( void ) {
	const char *temp;

	StartSound( "snd_explode", SND_CHANNEL_ANY, 0, false, nullptr );

	temp = spawnArgs.GetString( "model_damage", "" );
	if ( temp && *temp ) {
		SetModel( temp );
		Show();
	}


// bdube: replaced with playing an effect
/*
	temp = spawnArgs.GetString( "mtr_lightexplode", "" );
	if ( temp && *temp ) {
		AddLight( temp, false );
	}
*/
	StopEffect ( "fx_burn" );
	gameLocal.PlayEffect ( gameLocal.GetEffect( spawnArgs, "fx_explode" ), GetPhysics()->GetOrigin(), (-GetPhysics()->GetGravityNormal()).ToMat3(), false, vec3_origin, true );

	gameLocal.ProjectDecal( GetPhysics()->GetOrigin(), GetPhysics()->GetGravity(), 128.0f, true, 96.0f, "textures/decals/genericdamage" );
}

/*
================
idExplodingBarrel::Killed
================
*/
void idExplodingBarrel::Killed( anEntity *inflictor, anEntity *attacker, int damage, const anVec3 &dir, int location ) {

 	if ( IsHidden() || state == EXPLODED || state == EXPLODING || state == BURNING ) {
		return;
	}

	float f = spawnArgs.GetFloat( "burn" );
	if ( f > 0.0f && state == NORMAL ) {
		state = BURNING;
		PostEventSec( &EV_Explode, f );
		StartSound( "snd_burn", SND_CHANNEL_ANY, 0, false, nullptr );
		PlayEffect ( gameLocal.GetEffect( spawnArgs,"fx_burn" ),
					 vec3_origin, (-GetPhysics()->GetGravityNormal()).ToMat3(), true, vec3_origin, true );
		return;
	} else {
		state = EXPLODING;
 		spawnArgs.GetInt( "explode_lapse", "1000", explodeFinishTime );
 		explodeFinishTime += gameLocal.time;
	}

	// do this before applying radius damage so the ent can trace to any damagable ents nearby
	Hide();
	physicsObj.SetContents( 0 );

	const char *splash = spawnArgs.GetString( "def_splash_damage", "damage_explosion" );
	if ( splash && *splash ) {
//		gameLocal.RadiusDamage( GetPhysics()->GetOrigin(), this, inflictor, this, this, splash );
		PostEventMS( &EV_RadiusDamage, 0, this, splash);
	}

	ExplodingEffects( );


// bdube: replaced with playing an effect
/*
	//FIXME: need to precache all the debris stuff here and in the projectiles
	const anKeyValue *kv = spawnArgs.MatchPrefix( "def_debris" );
	// bool first = true;
	while ( kv ) {
		const anDict *debris_args = gameLocal.FindEntityDefDict( kv->GetValue(), false );
		if ( debris_args ) {
			anEntity *ent;
			anVec3 dir;
			idDebris *debris;
			//if ( first ) {
				dir = physicsObj.GetAxis()[1];
			//	first = false;
			//} else {
				dir.x += gameLocal.random.CRandomFloat() * 4.0f;
				dir.y += gameLocal.random.CRandomFloat() * 4.0f;
				//dir.z = gameLocal.random.RandomFloat() * 8.0f;
			//}
			dir.Normalize();

			gameLocal.SpawnEntityDef( *debris_args, &ent, false );
			if ( !ent || !ent->IsType( idDebris::Type ) ) {
				gameLocal.Error( "'projectile_debris' is not an idDebris" );
			}

			debris = static_cast<idDebris *>(ent);
			debris->Create( this, physicsObj.GetOrigin(), dir.ToMat3() );
			debris->Launch();
			debris->GetRenderEntity()->shaderParms[ SHADERPARM_TIME_OF_DEATH ] = ( gameLocal.time + 1500 ) * 0.001f;
			debris->UpdateVisuals();

		}
		kv = spawnArgs.MatchPrefix( "def_debris", kv );
	}
*/


	physicsObj.PutToRest();
	CancelEvents( &EV_Explode );
	CancelEvents( &EV_Activate );

	f = spawnArgs.GetFloat( "respawn" );
	if ( f > 0.0f ) {
		PostEventSec( &EV_Respawn, f );
	} else {
		PostEventMS( &EV_Remove, 5000 );
	}

	if ( spawnArgs.GetBool( "triggerTargets" ) ) {
		ActivateTargets( this );
	}
}

/*
================
idExplodingBarrel::Think
================
*/
void idExplodingBarrel::Damage( anEntity *inflictor, anEntity *attacker, const anVec3 &dir,
					  const char *damageDefName, const float damageScale, const int location ) {


	const anDict *damageDef = gameLocal.FindEntityDefDict( damageDefName, false );
	if ( !damageDef ) {
		gameLocal.Error( "Unknown damageDef '%s'\n", damageDefName );
	}
	if ( damageDef->FindKey( "radius" ) && GetPhysics()->GetContents() != 0 && GetBindMaster() == nullptr ) {
		PostEventMS( &EV_Explode, 400 );
	} else {
		anEntity::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
	}
}


/*
================
idExplodingBarrel::Event_TriggerTargets
================
*/
void idExplodingBarrel::Event_TriggerTargets() {
	ActivateTargets( this );
}

/*
================
idExplodingBarrel::Event_Explode
================
*/
void idExplodingBarrel::Event_Explode() {
	if ( state == NORMAL || state == BURNING ) {
		state = BURNEXPIRED;
		Killed( nullptr, nullptr, 0, vec3_zero, 0 );
	}
}

/*
================
idExplodingBarrel::Event_Respawn
================
*/
void idExplodingBarrel::Event_Respawn() {
 	const char *temp = spawnArgs.GetString( "model" );
	if ( temp && *temp ) {
		SetModel( temp );
	}
	health = spawnArgs.GetInt( "health", "5" );
	fl.takedamage = (health > 0);
	physicsObj.SetOrigin( spawnOrigin );
	physicsObj.SetAxis( spawnAxis );
	physicsObj.SetContents( CONTENTS_SOLID );
	physicsObj.DropToFloor();
	state = NORMAL;
	Show();
	UpdateVisuals();
}

/*
================
idMoveable::Event_Activate
================
*/
void idExplodingBarrel::Event_Activate( anEntity *activator ) {
	Killed( activator, activator, 0, vec3_origin, 0 );
}



/*
================
idMoveable::WriteToSnapshot
================
*/
void idExplodingBarrel::WriteToSnapshot( anBitMsgDelta &msg ) const {
	idMoveable::WriteToSnapshot( msg );
	msg.WriteBits( IsHidden(), 1 );
	msg.WriteBits( state, 3 );
}

/*
================
idMoveable::ReadFromSnapshot
================
*/
void idExplodingBarrel::ReadFromSnapshot( const anBitMsgDelta &msg ) {
	explode_state_t newState;

	idMoveable::ReadFromSnapshot( msg );
	if ( msg.ReadBits( 1 ) ) {
		Hide();
	} else {
		Show();
	}
	newState = (explode_state_t)msg.ReadBits( 3 );
	if ( newState != state ) {
		state = newState;
		if ( state == EXPLODING ) {
			ExplodingEffects( );
		}
	}
}


