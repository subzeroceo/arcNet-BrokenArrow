/*
===============================================================================

SEAS_Find.cpp

This file has all aas search classes.

===============================================================================
*/

#include "../../idlib/Lib.h"
#pragma hdrstop

#include "../Game_local.h"
#include "AI.h"
#include "AI_Manager.h"
#include "AI_Util.h"
#include "SEAS_Find.h"

/*
===============================================================================

	anSEASTacticalManager

===============================================================================
*/

/*
============
anSEASTacticalManager::anSEASTacticalManager
============
*/
anSEASTacticalManager::anSEASTacticalManager( const anVec3 &hideFromPos ) {
	int			numPVSAreas;
	anBounds	bounds( hideFromPos - anVec3( 16, 16, 0 ), hideFromPos + anVec3( 16, 16, 64 ) );

	// setup PVS
	numPVSAreas = gameLocal.pvs.GetPVSAreas( bounds, PVSAreas, anEntity::MAX_PVS_AREAS );
	hidePVS		= gameLocal.pvs.SetupCurrentPVS( PVSAreas, numPVSAreas );
}

/*
============
anSEASTacticalManager::~anSEASTacticalManager
============
*/
anSEASTacticalManager::~anSEASTacticalManager() {
	gameLocal.pvs.FreeCurrentPVS( hidePVS );
}

/*
anSEASTacticalManager
anSEASFindHide::TestArea
============
*/
bool anSEASTacticalManager::TestArea( class anSEAS *aas, int areaNum, const seasArea_t& area ) {
	int	numPVSAreas;
	int	PVSAreas[ anEntity::MAX_PVS_AREAS ];

	numPVSAreas = gameLocal.pvs.GetPVSAreas( anBounds( area.center ).Expand( 16.0f ).TranslateSelf(anVec3(0,0,1)), PVSAreas, anEntity::MAX_PVS_AREAS );
	if ( !gameLocal.pvs.InCurrentPVS( hidePVS, PVSAreas, numPVSAreas ) ) {
		return true;
	}

	return false;
}

/*
===============================================================================

	anAASTargetOutOfRange

===============================================================================
*/

/*
============
anAASTargetOutOfRange::anAASTargetOutOfRange
============
*/
anAASTargetOutOfRange::anAASTargetOutOfRange( anSAAI* _owner ) {
	owner = _owner;
}

/*
============
anSEASFindAreaOutOfRange::TestArea
============
*/
bool anAASTargetOutOfRange::TestPoint ( anSEAS* aas, const anVec3& pos, const float zAllow ) {
	return aiManager.ValidateDestination ( owner, pos );
}

/*
===============================================================================

anAASHolstileCoordnation

Find a position to move to that allows the ai to at their target

===============================================================================
*/

/*
============
anAASHolstileCoordnation::anAASHolstileCoordnation
============
*/
anAASHolstileCoordnation::anAASHolstileCoordnation( anSAAI* _owner ) {
	owner		= _owner;
	cachedIndex = 0;
}

/*
============
anAASHolstileCoordnation::~anAASHolstileCoordnation
============
*/
anAASHolstileCoordnation::~anAASHolstileCoordnation( void ) {
}

/*
============
anAASHolstileCoordnation::Init
============
*/
void anAASHolstileCoordnation::Init ( void ) {
	// setup PVS
	int	numPVSAreas;
	numPVSAreas = gameLocal.pvs.GetPVSAreas( owner->enemy.ent->GetPhysics()->GetAbsBounds(), PVSAreas, anEntity::MAX_PVS_AREAS );
	targetPVS	= gameLocal.pvs.SetupCurrentPVS( PVSAreas, numPVSAreas );

	cachedGoals.SetGranularity ( 1024 );
}

/*
============
anAASHolstileCoordnation::Finish
============
*/
void anAASHolstileCoordnation::Finish ( void ) {
	gameLocal.pvs.FreeCurrentPVS( targetPVS );
}

/*
============
anAASHolstileCoordnation::TestArea
============
*/
bool anAASHolstileCoordnation::TestArea( class anSEAS *aas, int areaNum, const seasArea_t& area ) {
	int	numPVSAreas;
	int	PVSAreas[ anEntity::MAX_PVS_AREAS ];

	cachedAreaNum = areaNum;

	// If the whole area is out of range then skip it
	float range;
	range = area.bounds.ShortestDistance( owner->enemy.lastKnownPosition );
	if ( range > owner->combat.attackRange[1] ) {
		return false;
	}

	// Out of pvs?
	numPVSAreas = gameLocal.pvs.GetPVSAreas( anBounds( area.center ).Expand( 16.0f ), PVSAreas, anEntity::MAX_PVS_AREAS );
	return gameLocal.pvs.InCurrentPVS( targetPVS, PVSAreas, numPVSAreas );
}

/*
============
anAASHolstileCoordnation::TestPoint
============
*/
bool anAASHolstileCoordnation::TestPoint( class anSEAS *aas, const anVec3& point, const float zAllow ) {
	float dist;

	anVec3 localPoint = point;
	float bestZ = owner->enemy.ent->GetPhysics()->GetOrigin().z;
	if ( bestZ > localPoint.z ) {
		if ( bestZ > zAllow ) {
			localPoint.z = zAllow;
		} else {
			localPoint.z = bestZ;
		}
	}

	// Out of attack range?
	dist = (localPoint - owner->enemy.ent->GetPhysics()->GetOrigin()).LengthFast();
	if ( dist < owner->combat.attackRange[0] || dist > owner->combat.attackRange[1] ) {
		return false;
	}

	// If tethered make sure the point is within the tether range
	if ( owner->tether ) {
		anVec3 localPoint = point;
		float bestZ = owner->tether.GetEntity()->GetPhysics()->GetOrigin().z;
		if ( bestZ > localPoint.z ) {
			if ( bestZ > zAllow ) {
				localPoint.z = zAllow;
			} else {
				localPoint.z = bestZ;
			}
		}
		if ( !owner->tether->ValidateDestination ( owner, localPoint ) ) {
			return false;
		}
	}

	seasGoal_t& goal = cachedGoals.Alloc();
	goal.areaNum = cachedAreaNum;
	goal.origin  = localPoint;

	return false;
}

/*
============
anAASHolstileCoordnation::TestCachedGoal
============
*/
bool anAASHolstileCoordnation::TestCachedGoal ( int index ) {
	const seasGoal_t& goal = cachedGoals[index];

	// Out of attack range?
	float dist = (goal.origin - owner->enemy.ent->GetPhysics()->GetOrigin() ).LengthFast();
	if ( dist < owner->combat.attackRange[0] || dist > owner->combat.attackRange[1] ) {
		return false;
	}

	// Someone already there?
	if ( !aiManager.ValidateDestination( owner, goal.origin, true ) ) {
		return false;
	}

	// Can we see the enemy from this position?
	if ( !owner->CanSeeFrom( goal.origin - owner->GetPhysics()->GetGravityNormal() * owner->combat.visStandHeight, owner->GetEnemy(), false ) ) {
		return false;
	}

	return true;
}

/*
============
anAASHolstileCoordnation::TestCachedPoints
============
*/
bool anAASHolstileCoordnation::TestCachedGoals( int count, seasGoal_t& goal ) {
	int i;

	goal.areaNum = 0;

	// Test as many points as we are allowed to test
	for ( i = 0; i < count && cachedIndex < cachedGoals.Num(); cachedIndex ++, i ++ ) {
		// Retest simple checks
		if ( TestCachedGoal( cachedIndex ) ) {
			goal = cachedGoals[cachedIndex];
			return true;
		}
	}

	return !(cachedIndex >= cachedGoals.Num());
}

/*
===============================================================================

SEASTegtherObjective

Find a goal to move to that is within the given tether.

===============================================================================
*/

/*
============
SEASTegtherObjective::SEASTegtherObjective
============
*/
SEASTegtherObjective::SEASTegtherObjective( anSAAI* _owner, anSAAITether* _tether ) {
	owner  = _owner;
	tether = _tether;
}

/*
============
SEASTegtherObjective::SEASTegtherObjective
============
*/
SEASTegtherObjective::~SEASTegtherObjective( void ) {
}

/*
============
SEASTegtherObjective::TestArea
============
*/
bool SEASTegtherObjective::TestArea( class anSEAS *aas, int areaNum, const seasArea_t& area ) {
	// Test super class first
	if ( !anSEASCallback::TestArea ( aas, areaNum, area ) ) {
		return false;
	}

	// Make sure the area bounds is remotely valid for the tether
	anBounds tempBounds = area.bounds;
	tempBounds[1].z = area.ceiling;
	if ( !tether->ValidateBounds ( tempBounds ) ) {
		return false;
	}
	return true;
}

/*
============
SEASTegtherObjective::TestPoint
============
*/
bool SEASTegtherObjective::TestPoint( class anSEAS* aas, const anVec3& point, const float zAllow ) {
	if ( !tether ) {
		return false;
	}

	anVec3 localPoint = point;
	float bestZ = tether->GetPhysics()->GetOrigin().z;
	if ( bestZ > localPoint.z )	{
		if ( bestZ > zAllow ) {
			localPoint.z = zAllow;
		} else {
			localPoint.z = bestZ;
		}
	}
	if ( !tether->ValidateDestination( owner, localPoint ) ) {
		return false;
	}

	if ( !aiManager.ValidateDestination( owner, localPoint ) ) {
		return false;
	}

	return true;
}
