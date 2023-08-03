#ifndef __AI_MANAGER_H__
#define __AI_MANAGER_H__

typedef enum {
	AITEAM_MARINE,
	AITEAM_STROGG,
	AITEAM_NUM
} aiTeam_t;

typedef enum {
	AITEAMTIMER_ANNOUNCE_TACTICAL,					// Tactical change
	AITEAMTIMER_ANNOUNCE_SUPPRESSING,
	AITEAMTIMER_ANNOUNCE_SUPPRESSED,
	AITEAMTIMER_ANNOUNCE_FRIENDLYFIRE,				// Shot by a teammate
	AITEAMTIMER_ANNOUNCE_ENEMYSTEATH,
	AITEAMTIMER_ANNOUNCE_NEWENEMY,					// New enemy was aquired
	AITEAMTIMER_ANNOUNCE_SNIPER,					// Sniper sighted
	AITEAMTIMER_ANNOUNCE_CANIHELPYOU,				// Player standing in front of a friendly too long
	AITEAMTIMER_ANNOUNCE_SIGHT,						// First time seeing an enemy
	AITEAMTIMER_ACTION_RELAX,						// Play relax animation
	AITEAMTIMER_ACTION_PEEK,						// Play peek animation
	AITEAMTIMER_ACTION_TALK,						// Able to talk to another person yet?
	AITEAMTIMER_MAX
} aiTeamTimer_t;

extern anVec4 aiTeamColor[AITEAM_NUM];

/*
=====================
blockedReach_t
=====================
*/
// cdr: Alternate Routes Bug
typedef struct aiBlocked_s {
	anSEAS*							aas;
	anReachability*					reach;
	int								time;
	anList< anEntityPtr<anEntity> >	blockers;
	anList< anVec3 >				positions;
} aiBlocked_t;

/*
=====================
aiAvoid_t
=====================
*/
typedef struct aiAvoid_s {
	anVec3							origin;
	float							radius;
	int								team;
} aiAvoid_t;

/*
===============================================================================

anSAAIHelper

===============================================================================
*/

class anSAAIHelper : public anEntity {
public:
	CLASS_PROTOTYPE( anSAAIHelper );

	anSAAIHelper ( void );

	anLinkList<anSAAIHelper>	helperNode;

	void			Spawn					( void );

	virtual bool	IsCombat				( void ) const;
	virtual bool	ValidateDestination		( const anSAAI* ent, const anVec3& dest ) const;

	anVec3			GetDirection			( const anSAAI* ent ) const;

protected:

	virtual void	OnActivate				( bool active );

private:

	void			Event_Activate			( anEntity *activator );
};

/*
===============================================================================

rvAIManager

===============================================================================
*/

class rvAIManager {
public:

	rvAIManager ( void );
	~rvAIManager ( void ) {}

	/*
	===============================================================================
									General
	===============================================================================
	*/

	void				RunFrame				( void );

	void				Save					( anSaveGame *savefile ) const;
	void				Restore					( anRestoreGame *savefile );

	void				Clear					( void );

	bool				IsActive				( void );
	bool				IsSimpleThink			( anSAAI* ai );

	/*
	===============================================================================
									Navigation
	===============================================================================
	*/

	void				UnMarkAllReachBlocked	( void );
	void				ReMarkAllReachBlocked	( void );
	void				MarkReachBlocked		( anSEAS* aas, anReachability* reach, const anList<anEntity*>& blockers);
	bool				ValidateDestination		( anSAAI* ignore, const anVec3& dest, bool skipCurrent = false, anActor* skipActor = nullptr ) const;
	void				AddAvoid				( const anVec3& origin, float range, int team );

	/*
	===============================================================================
									Helpers
	===============================================================================
	*/

	void				RegisterHelper			( anSAAIHelper* helper );
	void				UnregisterHelper		( anSAAIHelper* helper );
	anSAAIHelper*			FindClosestHelper		( const anVec3& origin );

	/*
	===============================================================================
									Team Management
	===============================================================================
	*/

	void				AddTeammate				( anActor* ent );
	void				RemoveTeammate			( anActor* ent );

	anActor*			GetAllyTeam				( aiTeam_t team );
	anActor*			GetEnemyTeam			( aiTeam_t team );

	anActor*			NearestTeammateToPoint	( anActor* from, anVec3 point, bool nonPlayer = false, float maxRange = 1000.0f, bool checkFOV = false, bool checkLOS = false );
	anEntity*			NearestTeammateEnemy	( anActor* from, float maxRange=1000.0f, bool checkFOV = false, bool checkLOS = false, anActor** ally = nullptr );
	bool				LocalTeamHasEnemies		( anSAAI* self, float maxBuddyRange=640.0f, float maxEnemyRange=1024.0f, bool checkPVS=false );
	bool				ActorIsBehindActor		( anActor* ambusher, anActor* victim );

	/*
	===============================================================================
									Team Timers
	===============================================================================
	*/

	bool				CheckTeamTimer			( int team, aiTeamTimer_t timer );
	void				ClearTeamTimer			( int team, aiTeamTimer_t timer );
	void				SetTeamTimer			( int team, aiTeamTimer_t timer, int delay );

	/*
	===============================================================================
									Announcements
	===============================================================================
	*/

	void				AnnounceKill			( anSAAI* victim, anEntity* attacker, anEntity* inflictor );
	void				AnnounceDeath			( anSAAI* victim, anEntity* attacker );

	/*
	===============================================================================
									Reactions
	===============================================================================
	*/

	void				ReactToPlayerAttack		( anBasePlayer* player, const anVec3 &origOrigin, const anVec3 &origDir );

	/*
	===============================================================================
									Debugging
	===============================================================================
	*/

	anTimer				timerFindEnemy;
	anTimer				timerTactical;
	anTimer				timerMove;
	anTimer				timerThink;

	int					thinkCount;
	int					simpleThinkCount;

protected:

	void				UpdateHelpers			( void );

	void				DebugDraw				( void );
	void				DebugDrawHelpers		( void );

	anList<aiBlocked_t>		blockedReaches;
	anLinkList<anSAAI>		simpleThink;
	anLinkList<anSAAIHelper>	helpers;

	anLinkList<anActor>		teams[AITEAM_NUM];
	int						teamTimers[AITEAM_NUM][AITEAMTIMER_MAX];

	anList<aiAvoid_t>		avoids;
};

ARC_INLINE bool rvAIManager::CheckTeamTimer ( int team, aiTeamTimer_t timer ) {
	return gameLocal.time >= teamTimers[team][timer];
}

ARC_INLINE void rvAIManager::ClearTeamTimer ( int team, aiTeamTimer_t timer ) {
	teamTimers[team][timer] = 0;
}

ARC_INLINE void rvAIManager::SetTeamTimer ( int team, aiTeamTimer_t timer, int delay ) {
	teamTimers[team][timer] = gameLocal.time + delay;
}

ARC_INLINE void rvAIManager::AddAvoid ( const anVec3& origin, float radius, int team ) {
	if ( !IsActive() ) {
		return;
	}
	aiAvoid_t& a = avoids.Alloc();
	a.origin = origin;
	a.radius = radius;
	a.team   = team;
}

extern rvAIManager aiManager;

#endif // __AI_MANAGER_H__
