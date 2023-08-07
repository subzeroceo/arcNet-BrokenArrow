#include "../../idlib/Lib.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../spawner.h"
#include "AI_Manager.h"
#include "AI_Util.h"
#include "SEAS_Find.h"

/*
===============================================================================

anSAAIHelper

===============================================================================
*/

CLASS_DECLARATION( anEntity, anSAAIHelper  )
	EVENT( EV_Activate,	anSAAIHelper::Event_Activate )
END_CLASS

/*
================
anSAAIHelper::anSAAIHelper
================
*/
anSAAIHelper::anSAAIHelper ( void ) {
	helperNode.SetOwner( this );
}

/*
================
anSAAIHelper::Spawn
================
*/
void anSAAIHelper::Spawn ( void ) {
	// Auto activate?
	if ( spawnArgs.GetBool ( "start_on" ) ) {
		PostEventMS ( &EV_Activate, 0, this );
	}
}

/*
================
anSAAIHelper::IsCombat
================
*/
bool anSAAIHelper::IsCombat ( void ) const {
	return false;
}


/*
================
anSAAIHelper::OnActivate
================
*/
void anSAAIHelper::OnActivate ( bool active ) {
	if ( active ) {
		ActivateTargets ( this );
	}
}

/*
================
anSAAIHelper::Event_Activate
================
*/
void anSAAIHelper::Event_Activate( anEntity *activator ) {
	if ( !helperNode.InList() ) {
		aiManager.RegisterHelper ( this );
		OnActivate ( true );
	} else {
		aiManager.UnregisterHelper ( this );
		OnActivate ( false );
	}
}

/*
================
anSAAIHelper::GetDirection
================
*/
anVec3 anSAAIHelper::GetDirection	( const anSAAI* ai ) const {
	if ( ai->team == 0 ) {
		return GetPhysics()->GetAxis()[0];
	}
	return -GetPhysics()->GetAxis()[0];
}

/*
================
anSAAIHelper::ValidateDestination
================
*/
bool anSAAIHelper::ValidateDestination ( const anSAAI* ent, const anVec3 &dest ) const {
	return true;
}

/*
===============================================================================

rvAICombatHelper

===============================================================================
*/

class rvAICombatHelper : public anSAAIHelper {
public:
	CLASS_PROTOTYPE( rvAICombatHelper );

	rvAICombatHelper ( void );

	void			Spawn				( void );

	virtual bool	IsCombat			( void ) const;
	virtual bool	ValidateDestination ( const anSAAI* ent, const anVec3 &dest ) const;

protected:

	virtual void	OnActivate			( bool active );

	anEntityPtr<idLocationEntity>	location;
};

CLASS_DECLARATION( anSAAIHelper, rvAICombatHelper )
END_CLASS

/*
================
rvAICombatHelper::rvAICombatHelper
================
*/
rvAICombatHelper::rvAICombatHelper ( void ) {
}

/*
================
rvAICombatHelper::Spawn
================
*/
void rvAICombatHelper::Spawn ( void ) {
}

/*
================
rvAICombatHelper::OnActivate
================
*/
void rvAICombatHelper::OnActivate ( bool active ) {
	anSAAIHelper::OnActivate ( active );

	if ( active ) {
		if ( spawnArgs.GetBool ( "tetherLocation", "1" ) ) {
			location = gameLocal.LocationForPoint ( GetPhysics()->GetOrigin() );
		} else {
			location = nullptr;
		}
	}
}

/*
================
rvAICombatHelper::IsCombat
================
*/
bool rvAICombatHelper::IsCombat ( void ) const {
	return true;
}

/*
================
rvAICombatHelper::ValidateDestination
================
*/
bool rvAICombatHelper::ValidateDestination ( const anSAAI* ai, const anVec3 &dest ) const {
	// If tethering to a location then see if the location of the given points matches our tethered location
	if ( location ) {
		if ( gameLocal.LocationForPoint ( dest - ai->GetPhysics()->GetGravityNormal() * 32.0f ) != location ) {
			return false;
		}
	}

	// Is the destination on the wrong side of the helper?
	anVec3 origin;
	anVec3 dir;

	dir = GetDirection(ai);
	if ( ai->enemy.ent ) {
		origin  = ai->enemy.lastKnownPosition;
		origin -= (dir * ai->combat.attackRange[0]);
	} else {
		origin  = GetPhysics()->GetOrigin();
		origin -= (dir * 32.0f);
	}

	if ( dir * (origin-dest) < 0.0f ) {
		return false;
	}

	// Would this destination link us to the wrong helper?
	if ( static_cast< const anSAAIHelper * >( aiManager.FindClosestHelper( dest ) ) != this ) {
		return false;
	}

	return true;
}

/*
===============================================================================

									rvAIAvoid

===============================================================================
*/

class rvAIAvoid : public anEntity {
public:
	CLASS_PROTOTYPE( rvAIAvoid );

	rvAIAvoid ( void );

	void	Spawn	( void );
};

CLASS_DECLARATION( anEntity, rvAIAvoid )
END_CLASS

/*
================
rvAIAvoid::rvAIAvoid
================
*/
rvAIAvoid::rvAIAvoid ( void ) {
}

/*
================
rvAIAvoid::Spawn
================
*/
void rvAIAvoid::Spawn ( void ) {
	int team = -1;
	if ( !spawnArgs.GetInt ( "teamFilter", "-1", team ) ) {
		//hmm, no "teamFilter" set, check "team" since many were set up like this
		team = spawnArgs.GetInt ( "team", "-1" );
	}
	aiManager.AddAvoid ( GetPhysics()->GetOrigin(), spawnArgs.GetFloat ( "radius", "64" ), team );
	PostEventMS ( &EV_Remove, 0 );
}

/*
===============================================================================

								rvAITrigger

===============================================================================
*/

const anEventDef AI_AppendFromSpawner ( "<appendFromSpawner>", "ee" );

CLASS_DECLARATION( anEntity, rvAITrigger )
	EVENT( EV_Activate,				rvAITrigger::Event_Activate )
	EVENT( AI_AppendFromSpawner,	rvAITrigger::Event_AppendFromSpawner )
END_CLASS

/*
================
rvAITrigger::rvAITrigger
================
*/
rvAITrigger::rvAITrigger( void ) {
}

/*
================
rvAITrigger::Spawn
================
*/
void rvAITrigger::Spawn ( void ) {
	nextTriggerTime = 0;
	wait = SEC2MS ( spawnArgs.GetFloat ( "wait", "-1" ) );

	conditionDead	= spawnArgs.GetBool ( "condition_dead", "0" );
	conditionTether = spawnArgs.GetBool ( "condition_tether", "0" );
	conditionStop	= spawnArgs.GetBool ( "condition_stop", "0" );

	percent			= spawnArgs.GetFloat ( "percent", "1" );

	// Start on by default?
	nextTriggerTime = spawnArgs.GetBool ( "start_on", "0" ) ? 0 : -1;
	if ( nextTriggerTime == 0 ) {
		BecomeActive ( TH_THINK );
	} else {
		BecomeInactive ( TH_THINK );
	}

	// If there are no conditions we are done
	if ( !conditionDead && !conditionTether && !conditionStop ) {
		gameLocal.Warning ( "No conditions specified on ai trigger entity '%s'", GetName() );
		PostEventMS ( &EV_Remove, 0 );
	}
}

/*
================
rvAITrigger::Save
================
*/
void rvAITrigger::Save ( anSaveGame *savefile ) const {
	int i;
	savefile->WriteInt ( testAI.Num() );
	for ( i = 0; i < testAI.Num(); i ++ ) {
		testAI[i].Save ( savefile );
	}

	savefile->WriteInt ( testSpawner.Num() );
	for ( i = 0; i < testSpawner.Num(); i ++ ) {
		testSpawner[i].Save ( savefile );
	}

	savefile->WriteBool ( conditionDead );
	savefile->WriteBool ( conditionTether );
	savefile->WriteBool ( conditionStop );

	savefile->WriteInt ( wait );
	savefile->WriteInt ( nextTriggerTime );

	savefile->WriteFloat ( percent );
}

/*
================
rvAITrigger::Restore
================
*/
void rvAITrigger::Restore ( anRestoreGame *savefile ) {
	int i;
	int num;

	savefile->ReadInt ( num );
	testAI.Clear();
	testAI.SetNum ( num );
	for ( i = 0; i < num; i ++ ) {
		testAI[i].Restore ( savefile );
	}

	savefile->ReadInt ( num );
	testSpawner.Clear();
	testSpawner.SetNum ( num );
	for ( i = 0; i < num; i ++ ) {
		testSpawner[i].Restore ( savefile );
	}

	savefile->ReadBool ( conditionDead );
	savefile->ReadBool ( conditionTether );
	savefile->ReadBool ( conditionStop );

	savefile->ReadInt ( wait );
	savefile->ReadInt ( nextTriggerTime );

	savefile->ReadFloat ( percent );
}

/*
================
rvAITrigger::Think
================
*/
void rvAITrigger::Think ( void ) {
	int		v;

	// Only trigger so often
	if ( nextTriggerTime == -1 || gameLocal.time < nextTriggerTime ) {
		return;
	}

	// If we have any attached spawners then our condition cannot be met
	if ( testSpawner.Num() ) {
		for ( v = 0; v < testSpawner.Num(); v ++ ) {
			rvSpawner* spawner = testSpawner[v];
			if ( !spawner ) {
				testSpawner.RemoveIndex ( v );
				v--;
				continue;
			}
			return;
		}
	}

	// If we have no AI we are tracking then wait until we do
	if ( !testAI.Num() ) {
		return;
	}

	int count = 0;
	for ( v = 0; v < testAI.Num(); v ++ ) {
		anSAAI* ai = testAI[v];
		if ( !ai ) {
			testAI.RemoveIndex ( v );
			v--;
			continue;
		}
		if ( !ai->aifl.dead ) {
			if ( conditionDead ) {
				continue;
			}
			if ( conditionTether && !ai->IsWithinTether() ) {
				continue;
			}
			if ( conditionStop && ai->move.fl.moving ) {
				continue;
			}
		}
		count++;
	}

	// If result is true then we should fire our trigger now
	if ( count >= (int) ( ( float )testAI.Num()*percent) ) {
		ActivateTargets ( this );

		// If only triggering once just remove ourselves now
		if ( wait < 0 ) {
			nextTriggerTime = -1;
		} else {
			nextTriggerTime = gameLocal.time + wait;
		}
	}
}

/*
================
rvAITrigger::FindTargets
================
*/
void rvAITrigger::FindTargets ( void ) {
	int t;

	anEntity::FindTargets();

	for ( t = 0; t < targets.Num(); t ++ ) {
		anEntity *ent = targets[t];
		if ( !ent ) {
			continue;
		}
		if ( ent->IsType ( anSAAI::GetClassType() ) ) {
			testAI.Append ( anEntityPtr<anSAAI>( static_cast<anSAAI*>(ent)) );
			targets.RemoveIndex ( t );
			t--;
			continue;
		}
		if ( ent->IsType ( rvSpawner::GetClassType() ) ) {
			static_cast<rvSpawner*>(ent)->AddCallback ( this, &AI_AppendFromSpawner );
			testSpawner.Append ( anEntityPtr<rvSpawner>( static_cast<rvSpawner*>(ent)) );
			targets.RemoveIndex ( t );
			t--;
			continue;
		}
	}
}

/*
================
rvAITrigger::Event_Activate
================
*/
void rvAITrigger::Event_Activate( anEntity *activator ) {

	// Add spawners and ai to the list when they come in
	if ( activator && activator->IsType ( anSAAI::GetClassType() ) ) {
		testAI.Append ( anEntityPtr<anSAAI>( static_cast<anSAAI*>(activator)) );
		return;
	}

	if ( nextTriggerTime == -1 ) {
		nextTriggerTime = 0;
		BecomeActive ( TH_THINK );
	} else {
		nextTriggerTime = -1;
		BecomeInactive ( TH_THINK );
	}
}

/*
================
rvAITrigger::Event_AppendFromSpawner
================
*/
void rvAITrigger::Event_AppendFromSpawner ( rvSpawner* spawner, anEntity *spawned ) {
	// If its an ai entity being spawned then add it to our test list
	if ( spawned && spawned->IsType ( anSAAI::GetClassType() ) ) {
		testAI.Append ( anEntityPtr<anSAAI>( static_cast<anSAAI*>( spawned)) );
	}
}

/*
===============================================================================

								anSAAITether

===============================================================================
*/
const anEventDef EV_TetherSetupLocation ( "tetherSetupLocation" );
const anEventDef EV_TetherGetLocation ( "tetherGetLocation" );
CLASS_DECLARATION( anEntity, anSAAITether )
	EVENT( EV_Activate,					anSAAITether::Event_Activate )
	EVENT( EV_TetherGetLocation,		anSAAITether::Event_TetherGetLocation )
	EVENT( EV_TetherSetupLocation,		anSAAITether::Event_TetherSetupLocation )
END_CLASS

/*
================
anSAAITether::anSAAITether
================
*/
anSAAITether::anSAAITether ( void ) {
}

/*
================
anSAAITether::InitNonPersistentSpawnArgs
================
*/
void anSAAITether::InitNonPersistentSpawnArgs ( void ) {
	tfl.canBreak   		 = spawnArgs.GetBool ( "allowBreak", "1" );
	tfl.autoBreak		 = spawnArgs.GetBool ( "autoBreak", "0" );
	tfl.forceRun   		 = spawnArgs.GetBool ( "forceRun", "0" );
	tfl.forceWalk  		 = spawnArgs.GetBool ( "forceWalk", "0" );
	tfl.becomeAggressive = spawnArgs.GetBool ( "becomeAggressive", "0" );
	tfl.becomePassive    = spawnArgs.GetBool ( "becomePassive",    "0" );

	// Check for both being set
	if ( tfl.forceRun && tfl.forceWalk ) {
		gameLocal.Warning ( "both forceRun and forceWalk were specified for tether '%s', forceRun will take precedence" );
		tfl.forceWalk = false;
	}
	if ( tfl.becomeAggressive && tfl.becomePassive ) {
		gameLocal.Warning ( "both becomePassive and becomeAggressive were specified for tether '%s', becomeAggressive will take precedence" );
		tfl.becomePassive = false;
	}
}

/*
================
anSAAITether::Spawn
================
*/
void anSAAITether::Spawn ( void ) {
	InitNonPersistentSpawnArgs();

	PostEventMS( &EV_TetherSetupLocation, 100 );
}

/*
================
anSAAITether::Event_TetherSetupLocation
================
*/
void anSAAITether::Event_TetherSetupLocation( void ) {
	//NOTE: we now do this right after spawn so we don't stomp other tether's locations
	//		if we activate after them and are in the same room as them.
	//		Dynamically-spawned locations are very, very bad!

	// All pre-existing locations should be placed and spread by now
	// Get the location entity we are attached to
	if ( spawnArgs.GetBool ( "location", "1" ) ) {
		location = gameLocal.LocationForPoint ( GetPhysics()->GetOrigin() - GetPhysics()->GetGravityNormal() * 32.0f );
		if ( !location ) {
			location = gameLocal.AddLocation ( GetPhysics()->GetOrigin() - GetPhysics()->GetGravityNormal() * 32.0f, "tether_location" );
		}
	} else {
		location = nullptr;
	}
	PostEventMS( &EV_TetherGetLocation, 100 );
}

/*
================
anSAAITether::Event_TetherGetLocation
================
*/
void anSAAITether::Event_TetherGetLocation( void ) {
	//NOW: all locations should be made & spread, get our location (may not be the one
	//		we added, it could be be the same as another tether if it's in the same room as us)
	if ( spawnArgs.GetBool ( "location", "1" ) ) {
		location = gameLocal.LocationForPoint ( GetPhysics()->GetOrigin() - GetPhysics()->GetGravityNormal() * 32.0f );
	} else {
		location = nullptr;
	}
}

/*
================
anSAAITether::Save
================
*/
void anSAAITether::Save ( anSaveGame *savefile ) const {
	location.Save( savefile );
}

/*
================
anSAAITether::Restore
================
*/
void anSAAITether::Restore ( anRestoreGame *savefile ) {
	location.Restore( savefile );

	InitNonPersistentSpawnArgs();
}

/*
================
anSAAITether::ValidateAAS
================
*/
bool anSAAITether::ValidateAAS ( anSAAI* ai ) {
	if ( !ai->aas ) {
		return false;
	}
	return true;
}

/*
================
anSAAITether::ValidateDestination
================
*/
bool anSAAITether::ValidateDestination ( anSAAI* ai, const anVec3 &dest ) {
	if ( location ) {
		if ( gameLocal.LocationForPoint ( dest - ai->GetPhysics()->GetGravityNormal() * 32.0f ) != location ) {
			return false;
		}
	}
	return true;
}

/*
================
anSAAITether::ValidateBounds
================
*/
bool anSAAITether::ValidateBounds	( const anBounds& bounds ) {
	return true;
}

/*
================
anSAAITether::FindGoal
================
*/
bool anSAAITether::FindGoal ( anSAAI* ai, seasGoal_t& goal ) {
	SEASTegtherObjective findGoal ( ai, this );
	if ( !ai->aas->FindNearestGoal( goal,
								ai->PointReachableAreaNum( ai->GetPhysics()->GetOrigin() ),
								ai->GetPhysics()->GetOrigin(),
								GetPhysics()->GetOrigin(),
								ai->move.travelFlags,
								0.0f, 0.0f,
								nullptr, 0, findGoal ) ) {
		return false;
	}
	return true;
}


/*
================
anSAAITether::Event_Activate
================
*/
void anSAAITether::Event_Activate( anEntity *activator ) {
	int i;

	//WELL!  Turns out designers are binding tethers to movers, so we have to get our location *again* on activation...
	if ( spawnArgs.GetBool ( "location", "1" ) ) {
		location = gameLocal.LocationForPoint ( GetPhysics()->GetOrigin() - GetPhysics()->GetGravityNormal() * 32.0f );
	}

	if ( activator && activator->IsType ( anSAAI::GetClassType() ) ) {
		activator->ProcessEvent ( &EV_Activate, this );
	}

	// All targetted AI will be activated with the tether AI entity
	for ( i = 0; i < targets.Num(); i ++ ) {
		if ( !targets[i] ) {
			continue;
		}
		if ( targets[i]->IsType ( anSAAI::GetClassType() ) ) {
			targets[i]->ProcessEvent ( &EV_Activate, this );

			// Aggressive/Passive stance change?
			if ( tfl.becomeAggressive ) {
				targets[i]->ProcessEvent ( &AI_BecomeAggressive );
		 	} else if ( tfl.becomePassive ) {
				targets[i]->ProcessEvent ( &AI_BecomePassive, false );
			}
		}
	}
}

/*
================
anSAAITether::DebugDraw
================
*/
void anSAAITether::DebugDraw ( void ) {
	const anBounds& bounds = GetPhysics()->GetAbsBounds();
	gameRenderWorld->DebugBounds ( colorYellow, bounds.Expand ( 8.0f ) );
	gameRenderWorld->DebugArrow ( colorWhite, bounds.GetCenter(), bounds.GetCenter() + GetPhysics()->GetAxis()[0] * 16.0f, 8.0f );
	gameRenderWorld->DrawText( name.c_str(), bounds.GetCenter(), 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1 );
	gameRenderWorld->DrawText( va( "#%d", entityNumber ), bounds.GetCenter() - GetPhysics()->GetGravityNormal() * 5.0f, 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1 );
}

/*
===============================================================================

								anSAAITetherBehind

===============================================================================
*/

CLASS_DECLARATION( anSAAITether, anSAAITetherBehind )
END_CLASS

/*
================
anSAAITetherBehind::InitNonPersistentSpawnArgs
================
*/
void anSAAITetherBehind::InitNonPersistentSpawnArgs ( void ) {
	range = spawnArgs.GetFloat ( "range" );
}

/*
================
anSAAITetherBehind::Spawn
================
*/
void anSAAITetherBehind::Spawn ( void ) {
	InitNonPersistentSpawnArgs();
}

/*
================
anSAAITetherBehind::Restore
================
*/
void anSAAITetherBehind::Restore ( anRestoreGame* savefile ) {
	InitNonPersistentSpawnArgs();
}

/*
================
anSAAITetherBehind::ValidateDestination
================
*/
bool anSAAITetherBehind::ValidateDestination ( anSAAI* ai, const anVec3 &dest ) {
	// Check base tether first
	if ( !anSAAITether::ValidateDestination ( ai, dest ) ) {
		return false;
	}

	// Make sure we include the move range in the tether
	anVec3 origin;
	if ( range ) {
		origin = GetPhysics()->GetOrigin() + GetPhysics()->GetAxis()[0] * GetOriginReachedRange() - GetPhysics()->GetAxis()[0] * range;
		if ( GetPhysics()->GetAxis()[0] * (origin-dest) > 0.0f ) {
			return false;
		}
	}

	origin = GetPhysics()->GetOrigin() - GetPhysics()->GetAxis()[0] * GetOriginReachedRange();

	// Are we on wrong side of tether?
	return ( GetPhysics()->GetAxis()[0] * (origin-dest) ) >= 0.0f;
}

/*
================
anSAAITetherBehind::ValidateBounds
================
*/
bool anSAAITetherBehind::ValidateBounds ( const anBounds& bounds ) {
	if ( !anSAAITether::ValidateBounds ( bounds ) ) {
		return false;
	}

	anPlane plane;
	int     side;
	plane.SetNormal ( GetPhysics()->GetAxis()[0] );
	plane.FitThroughPoint ( GetPhysics()->GetOrigin() );
	side = bounds.PlaneSide ( plane );
	return ( side == PLANESIDE_CROSS || side == PLANESIDE_BACK );
}

/*
================
anSAAITetherBehind::DebugDraw
================
*/
void anSAAITetherBehind::DebugDraw ( void ) {
	anVec3 dir;

	anSAAITether::DebugDraw();

	dir = GetPhysics()->GetGravityNormal().Cross ( GetPhysics()->GetAxis()[0] );
	gameRenderWorld->DebugLine ( colorPink,
								 GetPhysics()->GetOrigin() - dir * 1024.0f,
								 GetPhysics()->GetOrigin() + dir * 1024.0f );

	if ( range ) {
		anVec3 origin;
		origin = GetPhysics()->GetOrigin() - GetPhysics()->GetAxis()[0] * range;
		gameRenderWorld->DebugArrow ( colorPink, GetPhysics()->GetOrigin(), origin, 5.0f );
		gameRenderWorld->DebugLine ( colorPink,
									 origin - dir * 1024.0f,
									 origin + dir * 1024.0f );
	}
}

/*
===============================================================================

								anSAAITetherRadius

===============================================================================
*/

CLASS_DECLARATION( anSAAITether, anSAAITetherRadius )
END_CLASS

/*
================
anSAAITetherRadius::InitNonPersistentSpawnArgs
================
*/
void anSAAITetherRadius::InitNonPersistentSpawnArgs ( void ) {
	float radius;
	if ( !spawnArgs.GetFloat ( "tetherRadius", "0", radius ) ) {
		radius = spawnArgs.GetFloat ( "radius", "128" );
	}
	radiusSqr = Square ( radius );
}

/*
================
anSAAITetherRadius::Spawn
================
*/
void anSAAITetherRadius::Spawn ( void ) {
	InitNonPersistentSpawnArgs();
}

/*
================
anSAAITetherRadius::Restore
================
*/
void anSAAITetherRadius::Restore ( anRestoreGame* savefile ) {
	InitNonPersistentSpawnArgs();
}

/*
================
anSAAITetherRadius::ValidateDestination
================
*/
bool anSAAITetherRadius::ValidateDestination ( anSAAI* ai, const anVec3 &dest ) {
	// Check base tether first
	if ( !anSAAITether::ValidateDestination ( ai, dest ) ) {
		return false;
	}
	// Are we within tether radius?
	return ((dest - GetPhysics()->GetOrigin()).LengthSqr() < radiusSqr - Square ( ai->move.range ) );
}


/*
================
anSAAITetherRadius::ValidateBounds
================
*/
bool anSAAITetherRadius::ValidateBounds ( const anBounds& bounds ) {
	if ( !anSAAITether::ValidateBounds ( bounds ) ) {
		return false;
	}
	return ( Square ( bounds.ShortestDistance ( GetPhysics()->GetOrigin() ) ) < radiusSqr );
}

/*
================
anSAAITetherRadius::DebugDraw
================
*/
void anSAAITetherRadius::DebugDraw ( void ) {
	anSAAITether::DebugDraw();
	gameRenderWorld->DebugCircle( colorPink, GetPhysics()->GetOrigin(), GetPhysics()->GetGravityNormal(), anMath::Sqrt(radiusSqr), 25 );
}


/*
===============================================================================

								anSAAITetherClear

===============================================================================
*/

CLASS_DECLARATION( anSAAITether, anSAAITetherClear )
END_CLASS


/*
===============================================================================

								rvAIBecomePassive

===============================================================================
*/

CLASS_DECLARATION( anEntity, rvAIBecomePassive )
	EVENT( EV_Activate,	rvAIBecomePassive::Event_Activate )
END_CLASS

/*
================
rvAIBecomePassive::Event_Activate
================
*/
void rvAIBecomePassive::Event_Activate( anEntity *activator ) {
	int		i;
	bool	ignoreEnemies;

	ignoreEnemies = spawnArgs.GetBool ( "ignoreEnemies", "1" );

	// All targeted AI will become passive
	for ( i = 0; i < targets.Num(); i ++ ) {
		if ( !targets[i] ) {
			continue;
		}
		if ( targets[i]->IsType ( anSAAI::GetClassType() ) ) {
			targets[i]->ProcessEvent ( &AI_BecomePassive, ignoreEnemies );
		}
	}
}

/*
===============================================================================

								rvAIBecomeAggressive

===============================================================================
*/

CLASS_DECLARATION( anEntity, rvAIBecomeAggressive )
	EVENT( EV_Activate,	rvAIBecomeAggressive::Event_Activate )
END_CLASS

/*
================
rvAIBecomeAggressive::Event_Activate
================
*/
void rvAIBecomeAggressive::Event_Activate( anEntity *activator ) {
	int i;

	// All targetted AI will become aggressive
	for ( i = 0; i < targets.Num(); i ++ ) {
		if ( !targets[i] ) {
			continue;
		}
		if ( targets[i]->IsType ( anSAAI::GetClassType() ) ) {
			targets[i]->ProcessEvent ( &AI_BecomeAggressive );
		}
	}
}
