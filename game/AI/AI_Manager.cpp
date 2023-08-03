
#include "../../idlib/Lib.h"
#pragma hdrstop

#include "../Game_local.h"
#include "AI.h"
#include "AI_Manager.h"

anVec4 aiTeamColor[AITEAM_NUM] = { anVec4 ( 0.0f, 1.0f, 0.0f, 1.0f ),
								   anVec4 ( 1.0f, 0.0f, 0.0f, 1.0f ) };

rvAIManager aiManager;

/*
================
rvAIManager::rvAIManager
================
*/
rvAIManager::rvAIManager ( void ) {
	Clear();
}

/*
================
rvAIManager::IsActive
================
*/
bool rvAIManager::IsActive( void ){
	if ( gameLocal.isMultiplayer ){
		return false;
	}

	return true;
}

/*
================
rvAIManager::RunFrame
================
*/
void rvAIManager::RunFrame ( void ){
	if ( !IsActive() ) {
		return;
	}

	// Pop the top simple think off the list
	if ( !simpleThink.IsListEmpty() ) {
		simpleThink.Next()->simpleThinkNode.Remove();
	}

	// Display current ai speeds
	if ( ai_speeds.GetBool() && thinkCount > 0 ) {
		gameLocal.Printf ( "ai:%6i  n:%2i s:%2i all:%5.2f t:%5.2f e:%5.2f m:%5.f\n",
			gameLocal.framenum, thinkCount, simpleThinkCount,
			timerThink.Milliseconds(),
			timerTactical.Milliseconds(),
			timerFindEnemy.Milliseconds(),
			timerMove.Milliseconds() );
	}


	// cdr: Alternate Routes Bug
	int i;
	int j;
	// look for reaches that are no longer blocked
	for ( i=0; i<blockedReaches.Num(); i++ ) {
		aiBlocked_t& b = blockedReaches[i];

		// check all blockers for changes in position
		for ( j=0; j<b.blockers.Num(); j++ ) {
			if (gameLocal.GetTime()>b.time) {
				break;
			}

			anEntityPtr<anEntity> blockent = b.blockers[j];
			if ( !blockent.IsValid() || blockent->DistanceTo(b.positions[j])>10.0f) {
				break;
			}
			if (blockent->IsType(anSAAI::GetClassType())) {
				anSAAI* blockentAI = static_cast<anSAAI*>(blockent.GetEntity());
				if (blockentAI->move.moveDest.Dist2XY(b.positions[j])>100.0f) {
					break;
				}
			}
		}

		// if any one of the blockers moved or no longer exists, re-enable this reach
		if ( !b.blockers.Num() || j<b.blockers.Num() ) {
			b.aas->SetReachabilityState(b.reach, true);
			blockedReaches.RemoveIndex( i );
			break;
		}

		// DEBUG GRAPHICS
 		if ( ai_debugMove.GetBool() ) {
 			gameRenderWorld->DebugLine( colorRed, b.reach->start, b.reach->start + anVec3(0,0,40.0f), gameLocal.msec );
			for ( j=0; j<b.blockers.Num(); j++ ) {
				if (b.blockers[j].IsValid()) {
 					gameRenderWorld->DebugArrow( colorRed, b.blockers[j]->GetPhysics()->GetOrigin(), b.reach->start, 8, gameLocal.msec );
				}
			}
 		}
	}


	timerThink.Clear();
	timerTactical.Clear();
	timerFindEnemy.Clear();
	timerMove.Clear();

	gameDebug.SetStatInt( "ai_thinkCount", thinkCount );

	thinkCount	= 0;
	simpleThinkCount = 0;

	// Draw any debugging information
	DebugDraw();
}

/*
================
rvAIManager::Clear
================
*/
void rvAIManager::Clear( void ) {
	thinkCount = 0;
	simpleThinkCount = 0;
	timerThink.Clear();
	timerFindEnemy.Clear();
	timerTactical.Clear();
	timerMove.Clear();

	blockedReaches.Clear();
	helpers.Clear();
	simpleThink.Clear();
	avoids.Clear();

	memset ( &teamTimers, 0, sizeof(teamTimers) );
}

/*
================
rvAIManager::UnMarkAllReachBlocked
================
*/
void rvAIManager::UnMarkAllReachBlocked( void )
{
	for ( int i=0; i<blockedReaches.Num(); i++ ) {
		blockedReaches[i].aas->SetReachabilityState(blockedReaches[i].reach, true);
	}
}

/*
================
rvAIManager::ReMarkAllReachBlocked
================
*/
void rvAIManager::ReMarkAllReachBlocked( void )
{
	for ( int i=0; i<blockedReaches.Num(); i++ ) {
		blockedReaches[i].aas->SetReachabilityState(blockedReaches[i].reach, false);
	}
}

/*
================
rvAIManager::MarkReachBlocked
================
*/
void rvAIManager::MarkReachBlocked(anSEAS* aas, anReachability* reach, const anList<anEntity*>& blockers) {

	// only if not already blocked
	if ( !(reach->travelType&TFL_INVALID)) {
		aiBlocked_t blocked;
		blocked.aas = aas;
		blocked.reach = reach;
		blocked.time = gameLocal.GetTime() + 5000.0f;

		for ( int i=0; i<blockers.Num(); i++ ) {
			if (blockers[i] && blockers[i]->GetPhysics() && (blockers[i]->GetPhysics()->IsAtRest() || blockers[i]->GetPhysics()->GetLinearVelocity().LengthSqr()<2500.0f)) {
				blocked.blockers.Append(blockers[i]);
				blocked.positions.Append(blockers[i]->GetPhysics()->GetOrigin());
			}
		}
		if (blocked.blockers.Num()) {
			aas->SetReachabilityState(reach, false);
			blockedReaches.Append(blocked);
		}
	}

}

/*
================
rvAIManager::ReactToPlayerAttack
================
*/
void rvAIManager::ReactToPlayerAttack ( anBasePlayer* player, const anVec3 &origin, const anVec3 &dir ){
	anActor* actor;
	float expandSize;

	// Check all enemies and see if they need to react
	for ( actor = GetEnemyTeam ( (aiTeam_t)player->team ); actor; actor = actor->teamNode.Next() ) {
		// Skip non ai entities
		if ( !actor->IsType ( anSAAI::Type ) ) {
			continue;
		}

		anSAAI *curAI = static_cast<anSAAI*>(actor);

		// See if it will pass through an expanded bounding box
		expandSize = curAI->spawnArgs.GetFloat( "shotAtReactionRange", "16" );
		if ( !curAI->GetPhysics()->GetAbsBounds().Expand(expandSize).LineIntersection ( origin, origin + dir * curAI->combat.visRange ) ) {
			 continue;
		}

		curAI->ReactToShotAt ( player, origin, dir );
	}
}

/*
================
rvAIManager::Save
================
*/
void rvAIManager::Save( anSaveGame *savefile ) const {
	int i;
	int j;

	// Write out team list
	for ( i = 0; i < AITEAM_NUM; i ++ ) {
		anActor* actor;
		savefile->WriteInt( teams[i].Num() );
		for ( actor = teams[i].Next(); actor != nullptr; actor = actor->teamNode.Next() ) {
			savefile->WriteObject( actor );
		}
	}

	// Write out team timers
	for ( i = 0; i < AITEAM_NUM; i ++ ) {
		for ( j = 0; j < AITEAMTIMER_MAX; j ++ ) {
			savefile->WriteInt ( teamTimers[i][j] );
		}
	}

	// Write out team timers
	savefile->WriteInt ( avoids.Num() );
	for ( i = 0; i < avoids.Num(); i ++ ) {
		savefile->WriteVec3 ( avoids[i].origin );
		savefile->WriteFloat ( avoids[i].radius );
		savefile->WriteInt ( avoids[i].team );
	}
}

/*
================
rvAIManager::Restore
================
*/
void rvAIManager::Restore( anRestoreGame *savefile ){
	int i;
	int j;

	Clear();

	// Write out team list
 	for ( i = 0; i < AITEAM_NUM; i ++ ) {
		anActor* actor;
		savefile->ReadInt( j );
		for ( ; j > 0; j -- ) {
			savefile->ReadObject ( reinterpret_cast<anClass *&>( actor ) );
			if ( actor ) {
				actor->teamNode.AddToEnd ( teams[i] );
			}
		}
	}

	// Read team timers
	memset ( teamTimers, 0, sizeof(teamTimers) );
	for ( i = 0; i < AITEAM_NUM; i ++ ) {
		for ( j = 0; j < AITEAMTIMER_MAX; j ++ ) {
			savefile->ReadInt ( teamTimers[i][j] );
		}
	}

	// Read in team timers
	savefile->ReadInt ( j );
	avoids.SetNum ( j );
	for ( i = 0; i < j; i ++ ) {
		savefile->ReadVec3 ( avoids[i].origin );
		savefile->ReadFloat ( avoids[i].radius );
		savefile->ReadInt ( avoids[i].team );
	}
}

/*
=====================
rvAIManager::AddTeammate
=====================
*/
void rvAIManager::AddTeammate ( anActor* actor ) {
	// If its already in a team least then ignore the call.
	// NOTE: You have to call removeteammate before addteammate to switch the
	//       actor from one team to another
	if ( actor->teamNode.InList() ) {
		return;
	}
	actor->teamNode.AddToEnd ( teams[actor->team] );
}

/*
=====================
rvAIManager::RemoveTeammate
=====================
*/
void rvAIManager::RemoveTeammate ( anActor* actor ) {
	actor->teamNode.Remove();
}

/*
=====================
rvAIManager::IsSimpleThink

Determines whether or not the given AI entity should be simple thinking this frame
=====================
*/
bool rvAIManager::IsSimpleThink ( anSAAI* ai ) {

	// no simple think if simple thinking is disabled or we are the head node
	if ( ai_disableSimpleThink.GetBool() ) {
		return false;
	}

	// Add to simple think list
	if ( !ai->simpleThinkNode.InList() ) {
		ai->simpleThinkNode.AddToEnd ( simpleThink );
	}

	// If head of list then its a complex think
	if ( ai->simpleThinkNode.Prev() == nullptr ) {
		return false;
	}

	return true;
}

/*
================
rvAIManager::GetEnemyTeam
================
*/
anActor* rvAIManager::GetEnemyTeam ( aiTeam_t team ) {
	switch ( team ) {
		case AITEAM_MARINE:
			return teams[AITEAM_STROGG].Next();
		case AITEAM_STROGG:
			return teams[AITEAM_MARINE].Next();
	}
	return nullptr;
}

/*
================
rvAIManager::GetAllyTeam
================
*/
anActor* rvAIManager::GetAllyTeam ( aiTeam_t team ) {
	switch ( team ) {
		case AITEAM_MARINE:
			return teams[AITEAM_MARINE].Next();
		case AITEAM_STROGG:
			return teams[AITEAM_STROGG].Next();
	}
	return nullptr;
}

/*
================
rvAIManager::ValidateDestination

Validate whether or not the destinations is a destination
================
*/
bool rvAIManager::ValidateDestination ( anSAAI* ai, const anVec3& dest, bool skipCurrent, anActor* skipActor ) const {
	int			i;
	anBounds	bounds;
	anSAAI*		ignore;

	ignore = ( !skipCurrent && ai && !ai->SkipCurrentDestination()) ? ai : nullptr;

 	bounds = ai->GetPhysics()->GetBounds();
	bounds.TranslateSelf ( dest );
	bounds.ExpandSelf ( 16.0f );

	// All teams and all actors on those teams
	for ( i = 0; i < AITEAM_NUM; i ++ ) {
		anActor* actor;
		for ( actor = teams[i].Next(); actor; actor = actor->teamNode.Next() ) {
			// Ignored?
			if ( actor == ignore || actor == skipActor || actor->IsHidden() ) {
				continue;
			}

			// If the actor is AI that is moving we should check their destination rather than where they are now
			if ( actor->IsType ( anSAAI::Type ) ) {
				anSAAI* aiactor = static_cast<anSAAI*>(actor);
				if ( aiactor->move.moveCommand >= NUM_NONMOVING_COMMANDS ) {
					if ( bounds.IntersectsBounds ( aiactor->GetPhysics()->GetBounds().Translate ( aiactor->move.moveDest ) ) ) {
						return false;
					}
					continue;
				}
			}

			// Does the destination overlap this actor?
			if ( bounds.IntersectsBounds ( actor->GetPhysics()->GetAbsBounds() ) ) {
				return false;
			}
		}
	}

	// Destinations inside an avoid area are invalid
	for ( i = avoids.Num() - 1; i >= 0; i -- ) {
		const aiAvoid_t& avoid = avoids[i];
		// Skip all avoids that arent meant for this team
		if ( avoid.team != -1 && ai->team != avoid.team ) {
			continue;
		}
		// Skip if within range of the avoid
		if ( (avoid.origin - dest).LengthSqr() < Square ( avoid.radius ) ) {
			if ( ai_debugMove.GetBool() || ai_debugTactical.GetBool() ) {
				gameRenderWorld->DebugCircle( colorRed, avoid.origin, anVec3(0,0,1), avoid.radius, 25 );
			}
			return false;
		}
	}

	return true;
}

/*
================
rvAIManager::NearestTeammateToPoint

Returns the teammate closest to the given point with the given parameters
================
*/
anActor* rvAIManager::NearestTeammateToPoint ( anActor* from, anVec3 point, bool nonPlayer, float maxRange, bool checkFOV, bool checkLOS ) {
	anActor* actor = nullptr;
	anActor* closestActor = nullptr;
	float	distSqr;
	float	bestDistSqr;

	closestActor = 0x0;
	bestDistSqr  = Square ( maxRange );

	// Iterate through all teammates
	for ( actor = aiManager.GetAllyTeam ( (aiTeam_t)from->team ); actor; actor = actor->teamNode.Next() ) {
		//Hidden?
		if ( actor->fl.hidden )	{
			continue;
		}
		//Self?
		if ( actor == from ) {
			continue;
		}
		//Player?
		if ( nonPlayer && actor->IsType( anBasePlayer::GetClassType() ) ) {
			continue;
		}
		//Dead?
		if ( actor->health <= 0 ) {
			continue;
		}

		//Calc Range and check to see if closer before doing any complicated checks
		distSqr = (point - actor->GetPhysics()->GetOrigin()).LengthSqr();
		if ( distSqr >= bestDistSqr ) {
			continue;
		}

		//point in actor's in FOV?
		if ( checkFOV && !actor->CheckFOV( point ) ) {
			continue;
		}
		//actor has clear LOS to point?
		if ( checkLOS && !actor->CanSeeFrom ( from->GetEyePosition(), point, false ) ) {
			continue;
		}
		// New best actor
		bestDistSqr = distSqr;
		closestActor = actor;
	}
	return closestActor;
}

/*
================
rvAIManager::NearestTeammateEnemy

Returns the closest enemy of an ally.
================
*/
anEntity* rvAIManager::NearestTeammateEnemy( anActor* from, float maxRange, bool checkFOV, bool checkLOS, anActor** closestAllyWithEnemy ) {
	anActor*	actor;
	anSAAI*		allyAI;
	anEntity*	allyEnemy;
	anEntity*	closestAllyEnemy;
	float		distSqr;
	float		bestDistSqr;

	bestDistSqr			= Square ( maxRange );
	closestAllyEnemy	= nullptr;

	// Optionally return the ally whos enemy it is
	if ( closestAllyWithEnemy ) {
		*closestAllyWithEnemy = nullptr;
	}

	for ( actor = aiManager.GetAllyTeam ( (aiTeam_t)from->team ); actor; actor = actor->teamNode.Next() ) 	{
		//Hidden?
		if ( actor->fl.hidden )	{
			continue;
		}
		//Self?
		if ( actor == from ) {
			continue;
		}
		//Dead?
		if ( actor->health <= 0 ) {
			continue;
		}

		allyAI = dynamic_cast<anSAAI*>(actor);
		if ( !allyAI ) {
			//player?
			allyEnemy = actor->EnemyWithMostHealth();
			if ( !allyEnemy ) {
				continue;
			}
			if ( allyEnemy->health <= 0 ) {
				continue;
			}
		} else {
			allyEnemy = allyAI->GetEnemy();
			//has enemy?
			if ( !allyEnemy ) {
				continue;
			}
			//still alive?
			if ( allyAI->enemy.fl.dead ) {
				continue;
			}
		}

		//Calc Range and check to see if closer before doing any complicated checks
		distSqr = (actor->GetPhysics()->GetOrigin() - from->GetPhysics()->GetOrigin()).LengthSqr();
		if ( distSqr >= bestDistSqr ) {
			continue;
		}

		//point in actor's in FOV?
		if ( checkFOV ) {
			if ( !from->CheckFOV( actor->GetPhysics()->GetOrigin() ) ) {
				continue;
			}
		}
		//actor has clear LOS to point?
		if ( checkLOS ) {
			if ( !from->CanSee( actor, false ) ) {
				continue;
			}
		}

		bestDistSqr = distSqr;
		closestAllyEnemy = allyEnemy;

		if ( closestAllyWithEnemy ) {
			*closestAllyWithEnemy = actor;
		}
	}
	return closestAllyEnemy;
}

/*
================
rvAIManager::LocalTeamHasEnemies

Returns true if nearby members of my team have any enemies or if any nearby enemies have enemies that are nearby members of my team
================
*/
bool rvAIManager::LocalTeamHasEnemies ( anSAAI* self, float maxBuddyRange, float maxEnemyRange, bool checkPVS ) {
	anActor* actor = nullptr;
	pvsHandle_t pvs;

	if ( !self ) {
		return false;
	}

	// Iterate through all teammates
	for ( actor = aiManager.GetAllyTeam ( (aiTeam_t)self->team ); actor; actor = actor->teamNode.Next() ) {
		//Hidden?
		if ( actor->fl.hidden )	{
			continue;
		}
		//Dead?
		if ( actor->health <= 0 ) {
			continue;
		}
		if ( actor->IsType( anSAAI::GetClassType() ) ) {
			//Has an enemy?
			if ( !((anSAAI*)actor)->GetEnemy() ) {
				continue;
			}
			//Has an enemy
			if ( checkPVS ) {
				// Setup our local variables used in the search
				pvs	 = gameLocal.pvs.SetupCurrentPVS( actor->GetPVSAreas(), actor->GetNumPVSAreas() );
				// If this enemy isnt in the same pvps then use them as a backup
				if ( pvs.i > 0
					&& pvs.i < MAX_CURRENT_PVS
					&& !gameLocal.pvs.InCurrentPVS( pvs, ((anSAAI*)actor)->GetEnemy()->GetPVSAreas(), ((anSAAI*)actor)->GetEnemy()->GetNumPVSAreas() ) ) {
					gameLocal.pvs.FreeCurrentPVS( pvs );
					continue;
				}
				gameLocal.pvs.FreeCurrentPVS( pvs );
			}

		}
		//close enough?
		if ( actor != self && self->DistanceTo( actor->GetPhysics()->GetOrigin() ) > maxBuddyRange ) {
			continue;
		}
		//Anyone mad at him?
		if ( !actor->ClosestEnemyToPoint( actor->GetPhysics()->GetOrigin(), maxEnemyRange, true, checkPVS ) ) {
			continue;
		}
		return true;
	}
	return false;
}

/*
================
rvAIManager::ActorIsBehindActor

Returns true if the given 'ambuser' is behind the given 'victim'
================
*/
bool rvAIManager::ActorIsBehindActor( anActor* ambusher, anActor* victim ) {
	anVec3 dir2Ambusher = ambusher->GetPhysics()->GetOrigin() - victim->GetPhysics()->GetOrigin();
	float dist = dir2Ambusher.Normalize();
	if ( DotProduct(dir2Ambusher, victim->viewAxis[0] ) < 0  && dist < 200.0f ) {
		return true;
	}
	return false;
}

/*
===============================================================================

	rvAIManager - Helpers

===============================================================================
*/

/*
=====================
rvAIManager::RegisterHelper
=====================
*/
void rvAIManager::RegisterHelper ( anSAAIHelper* helper ) {
	helper->helperNode.AddToEnd ( helpers );
	UpdateHelpers();
}

/*
=====================
rvAIManager::UnregisterHelper
=====================
*/
void rvAIManager::UnregisterHelper ( anSAAIHelper* helper ) {
	helper->helperNode.Remove();
	UpdateHelpers();
}

/*
=====================
rvAIManager::FindClosestHelper
=====================
*/
anSAAIHelper* rvAIManager::FindClosestHelper ( const anVec3& origin ) {
	anSAAIHelper* helper;
	anSAAIHelper*	bestHelper;
	float		bestDist;
	float		dist;

	bestDist   = anMath::INFINITY;
	bestHelper = nullptr;
	for ( helper = helpers.Next(); helper; helper = helper->helperNode.Next( ) ) {
		dist = (origin - helper->GetPhysics()->GetOrigin()).LengthFast();
		if ( dist < bestDist ) {
			bestDist = dist;
			bestHelper = helper;
		}
	}
	return bestHelper;
}

/*
================
rvAIManager::UpdateHelpers
================
*/
void rvAIManager::UpdateHelpers ( void ) {
	int	i;

	for ( i = 0; i < AITEAM_NUM; i ++ ) {
		anActor* actor;
		for ( actor = teams[i].Next(); actor; actor = actor->teamNode.Next() ) {
			if ( actor->IsHidden() || !actor->IsType ( anSAAI::Type ) ) {
				continue;
			}
			static_cast<anSAAI*>(actor)->UpdateHelper();
		}
	}
}

/*
===============================================================================

	rvAIManager - Debugging

===============================================================================
*/

/*
================
rvAIManager::DebugDraw
================
*/
void rvAIManager::DebugDraw ( void ) {
	// Draw helpers?
	if ( ai_debugHelpers.GetBool() ) {
		DebugDrawHelpers();

		int i;
		for ( i = 0; i < avoids.Num(); i ++ ) {
			const aiAvoid_t& avoid = avoids[i];
			gameRenderWorld->DebugCircle ( colorOrange, avoid.origin, anVec3(0,0,1), avoid.radius, 10, 0 );
		}
	}


}

/*
================
rvAIManager::DebugDrawHelpers
================
*/
void rvAIManager::DebugDrawHelpers ( void ) {
	anSAAIHelper* helper;
	for ( helper = helpers.Next(); helper; helper = helper->helperNode.Next( ) ) {
		helper->DrawDebugEntityInfo();
	}
}
