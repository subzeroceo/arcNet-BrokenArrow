/*
================

AI.h

================
*/

#ifndef __AI_H__
#define __AI_H__

// moved all motion related code to AI_Move.h
#ifndef __AI_MOVE_H__
	#include "AI_Move.h"
#endif
#ifndef __SEAS_TACTICAL_H__
	#include "AAS_tactical.h"
#endif

typedef enum {
	AITACTICAL_NONE,
	AITACTICAL_MELEE,					// Rush towards the enemy and perform melee attacks when within range
	AITACTICAL_MOVE_FOLLOW,				// Move towards leader, stop when within range
	AITACTICAL_MOVE_TETHER,				// Move within tether, stop when within tether
	AITACTICAL_MOVE_PLAYERPUSH,			// Move away when the player pushes us (or another ai entity pushes us that was pushe by player)
	AITACTICAL_COVER,					// Move to cover and perform attacks from that cover position
 	AITACTICAL_COVER_FLANK,
 	AITACTICAL_COVER_ADVANCE,
 	AITACTICAL_COVER_RETREAT,
 	AITACTICAL_COVER_AMBUSH,
	AITACTICAL_RANGED,					// Move to position in which the enemy can be attacked from range, stop when there and attack enemy
	AITACTICAL_TURRET,					// Stay in current position and attack enemy
	AITACTICAL_HIDE,					// Move to a position where we cannot be seen by our enemy
	AITACTICAL_PASSIVE,					// Stay in current position with multiple idles, twitch animations, and conversations

	AITACTICAL_MAX
} aiTactical_t;

typedef enum  {
	AICTRESULT_OK,
	AICTRESULT_SKIP,
	AICTRESULT_NOMOVE,
} aiCTResult_t;

const int AITACTICAL_NONE_BIT				= BIT(AITACTICAL_NONE);
const int AITACTICAL_MELEE_BIT				= BIT(AITACTICAL_MELEE);
const int AITACTICAL_MOVE_FOLLOW_BIT		= BIT(AITACTICAL_MOVE_FOLLOW);
const int AITACTICAL_MOVE_TETHER_BIT		= BIT(AITACTICAL_MOVE_TETHER);
const int AITACTICAL_MOVE_PLAYERPUSH_BIT	= BIT(AITACTICAL_MOVE_PLAYERPUSH);
const int AITACTICAL_COVER_BIT				= BIT(AITACTICAL_COVER);
const int AITACTICAL_COVER_FLANK_BIT		= BIT(AITACTICAL_COVER_FLANK);
const int AITACTICAL_COVER_ADVANCE_BIT		= BIT(AITACTICAL_COVER_ADVANCE);
const int AITACTICAL_COVER_RETREAT_BIT		= BIT(AITACTICAL_COVER_RETREAT);
const int AITACTICAL_COVER_AMBUSH_BIT		= BIT(AITACTICAL_COVER_AMBUSH);
const int AITACTICAL_RANGED_BIT				= BIT(AITACTICAL_RANGED);
const int AITACTICAL_TURRET_BIT				= BIT(AITACTICAL_TURRET);
const int AITACTICAL_HIDE_BIT				= BIT(AITACTICAL_HIDE);
const int AITACTICAL_PASSIVE_BIT			= BIT(AITACTICAL_PASSIVE);

const int AITACTICAL_COVER_BITS				= (AITACTICAL_COVER_BIT|AITACTICAL_COVER_FLANK_BIT|AITACTICAL_COVER_ADVANCE_BIT|AITACTICAL_COVER_RETREAT_BIT|AITACTICAL_COVER_AMBUSH_BIT);
const int AITACTICAL_RANGED_BITS			= (AITACTICAL_RANGED_BIT);
const int AITACTICAL_NONMOVING_BITS			= (AITACTICAL_NONE_BIT|AITACTICAL_TURRET_BIT|AITACTICAL_PASSIVE_BIT);

typedef enum {
	TALKMSG_NONE,
	TALKMSG_PRIMARY,
	TALKMSG_SECONDARY,
	TALKMSG_LOOP,
} talkMessage_t;

typedef enum {
	AIFLAGOVERRIDE_DAMAGE,
	AIFLAGOVERRIDE_DISABLEPAIN,
	AIFLAGOVERRIDE_NOTURN,
	AIFLAGOVERRIDE_NOGRAVITY
} aiFlagOverride_t;

typedef enum {
	TALK_NEVER,						// Never talk to the player
	TALK_DEAD,						// Cant talk due to being dead
	TALK_OK,						// Can talk
	TALK_FOLLOW,					// Talking to will cause a follow
	TALK_BUSY,						// Cant talk right now, is busy
	TALK_WAIT,						// Wait a bit - he's probably in the middle of a conversation (this is so you can still see their names but they won't talk to you when clicked on)
	NUM_TALK_STATES
} talkState_t;

//chance that AI will make an announcement when they have the option (0 - 1.0f)
#define AISPEAK_CHANCE 0.2f

typedef enum {
	AIFOCUS_NONE,
	AIFOCUS_LEADER,
	AIFOCUS_TARGET,
	AIFOCUS_TALK,
	AIFOCUS_PLAYER,
	AIFOCUS_USE_DIRECTIONAL_MOVE,
	AIFOCUS_ENEMY = AIFOCUS_USE_DIRECTIONAL_MOVE,
	AIFOCUS_COVER,
	AIFOCUS_COVERLOOK,
	AIFOCUS_HELPER,
	AIFOCUS_TETHER,
	AIFOCUS_MAX
} aiFocus_t;

typedef enum {
	AIMOVESPEED_DEFAULT,			// Choose run/walk depending on situation
	AIMOVESPEED_RUN,				// Always run
	AIMOVESPEED_WALK,				// alwasy walk
} aiMoveSpeed_t;

typedef struct rvAIFuncs_s {
	rvScriptFuncUtility		first_sight;		// script to run when an enemy is first sighted
	rvScriptFuncUtility		sight;				// script to run every time an enemy is sighted
	rvScriptFuncUtility		pain;				// script to run when the AI takes pain
	rvScriptFuncUtility		damage;				// script to run when the AI takes damage
	rvScriptFuncUtility		death;				// script to run when the AI dies
	rvScriptFuncUtility		attack;				// script to run when attacking an enemy
	rvScriptFuncUtility		init;				// script to run on initialization
	rvScriptFuncUtility		onclick;			// script to run when a friendly AI is clicked on
	rvScriptFuncUtility		launch_projectile;	// script to run when a projectile is launched
	rvScriptFuncUtility		footstep;			// script to run on a footstep
} rvAIFuncs_t;

typedef struct rvAICombat_s{
	struct combatFlags_s {
		bool		ignoreEnemies		:1;
		bool		alert				:1;
 		bool		aware				:1;
		bool		tetherNoBreak		:1;				// Set to true to prevent enemies from breaking tethers
		bool		tetherAutoBreak		:1;				// Set to true to automatically break tethers when within range
		bool		tetherOutOfRange	:1;				// Set to true when we are out of range of our curren tether
		bool		seenEnemyDirectly	:1;				// Has directly seen an enemy (as opposed to having heard or been told about him)
		bool		noChatter			:1;				// No combat announcements
		bool		crouchViewClear		:1;				// Can I crouch at the position that I stopped at?
	} fl;

	float					max_chasing_turn;
	float					shotAtTime;
	float					shotAtAngle;
	anVec2					hideRange;
	anVec2					attackRange;
	int						attackSightDelay;
	float					meleeRange;
	float					aggressiveRange;			// Range to become more aggressive
	float					aggressiveScale;			// Scale to use when altering numbers due to aggression
 	int						investigateTime;

	float					visStandHeight;				// Height to check enemy visibiliy while standing
	float					visCrouchHeight;			// Height to check enemy visiblity while crouching
 	float					visRange;					// Maximum distance to check enemy visibility
 	float					earRange;					// Maximum distance to check hearing an enemy from
 	float					awareRange;					// Distance to become automatically aware of an enemy

	int						tacticalMaskAvailable;		// Currently available tactical states
	int						tacticalMaskUpdate;			// states currently being evaluated
	aiTactical_t			tacticalCurrent;			// Current tactical state
	int						tacticalUpdateTime;			// Last time the tacitcal state was updated (for delaying updating)
 	int						tacticalPainTaken;			// Amount of damage taken in the current tactical state
 	int						tacticalPainThreshold;		// Threshold of pain before invalidating the current tactical state
 	int						tacticalFlinches;			// Number of flinches that have occured at the current tactical state

	float					threatBase;					// Base amount of threat generated by AI
	float					threatCurrent;				// Current amount of threat generatd by AI based on current state

 	int						maxLostVisTime;

	int						coverValidTime;
	int						maxInvalidCoverTime;
} rvAICombat_t;

typedef struct rvAIPain_s {
	float					threshold;
	float					takenThisFrame;
	int						lastTakenTime;
	int						loopEndTime;
	anString					loopType;
} rvAIPain_t;

typedef struct rvAIEnemy_s {
	struct flags_s {
		bool		lockOrigin		:1;					// Stop tracking enemy origin until state changes
		bool		dead			:1;					// Enemy is dead
		bool		inFov			:1;					// Enemy is currently in fov
		bool		sighted			:1;					// Enemy was sighted at least once
		bool		visible			:1;					// Enemy is visible?
	} fl;

	anEntityPtr<anEntity>	ent;
	int						lastVisibleChangeTime;			// last time the visible state of the enemy changed
	anVec3					lastVisibleFromEyePosition;		// Origin used in last successfull visibility check
	anVec3					lastVisibleEyePosition;			// Origin of last known visible eye position
	anVec3					lastVisibleChestPosition;		// Origin of last known visible chest position
	int						lastVisibleTime;				// Time we last saw and enemy

	anVec3					smoothedLinearVelocity;
	anVec3					smoothedPushedVelocity;
	float					smoothVelocityRate;

	anVec3					lastKnownPosition;				// last place the enemy was known to be (does not mean visiblity)

	float					range;							// range to enemy
	float					range2d;						// 2d range to enemy
	int						changeTime;						// Time enemy was last changed
	int						checkTime;						// Time we last checked for a new enemy
} rvAIEnemy_t;

typedef struct rvAIPassive_s {
	struct flags_s {
		bool		disabled		:1;						// No advanced passive state
		bool		multipleIdles	:1;						// Has multiple idle animations to play
		bool		fidget			:1;						// Has fidget animations
	} fl;

	anString					animIdlePrefix;
	anString					animFidgetPrefix;
	anString					animTalkPrefix;
	//anString					talkPrefix;

	anString					prefix;
	anString					idleAnim;
	int						idleAnimChangeTime;
	int						fidgetTime;
	int						talkTime;
} rvAIPassive_t;

typedef struct rvAIAttackAnimInfo_s {
	anVec3					attackOffset;
	anVec3					eyeOffset;
} rvAIAttackAnimInfo_t;

#define AIACTIONF_ATTACK	BIT(0)
#define AIACTIONF_MELEE		BIT( 1 )

class rvAIActionTimer {
public:

	rvAIActionTimer ( void );

	bool	Init			( const anDict& args, const char* name );

	void	Save			( anSaveGame *savefile ) const;
	void	Restore			( anRestoreGame *savefile );

	void	Clear			( int currentTime );
	void	Reset			( int currentTime, float diversity = 0.0f, float scale = 1.0f );
	void	Add				( int _time, float diversity = 0.0f );

	bool	IsDone			( int currentTime ) const;

	int		GetTime			( void ) const;
	int		GetRate			( void ) const;

protected:

	int			time;
	int			rate;
};

ARC_INLINE bool rvAIActionTimer::IsDone ( int currentTime ) const {
	return currentTime >= time;
}

ARC_INLINE int rvAIActionTimer::GetTime ( void ) const {
	return time;
}

ARC_INLINE int rvAIActionTimer::GetRate ( void ) const {
	return rate;
}
class rvAIAction {
public:

	rvAIAction ( void );

	bool	Init			( const anDict& args, const char* name, const char* defaultState, int flags );

	void	Save			( anSaveGame *savefile ) const;
	void	Restore			( anRestoreGame *savefile );

	enum EStatus {
		STATUS_UNUSED,
		STATUS_OK,								// action was performed
		STATUS_FAIL_DISABLED,					// action is current disabled
		STATUS_FAIL_TIMER,						// actions timer has not finished
		STATUS_FAIL_EXTERNALTIMER,				// external timer passed in to PerformAction has not finished
		STATUS_FAIL_MINRANGE,					// enemy is not within minimum range
		STATUS_FAIL_MAXRANGE,					// enemy is out of maximum range
		STATUS_FAIL_CHANCE,						// action chance check failed
		STATUS_FAIL_ANIM,						// bad animation
		STATUS_FAIL_CONDITION,					// condition given to PerformAction failed
		STATUS_FAIL_NOENEMY,					// enemy cant be attacked
		STATUS_MAX
	};

	struct flags_s {
		bool		disabled			:1;		// action disabled?
		bool		noPain				:1;		// no pain during action
		bool		noTurn				:1;		// no turning during action
		bool		isAttack			:1;		// attack?
		bool		isMelee				:1;		// melee?
		bool		overrideLegs		:1;		// override legs on this action?
		bool		noSimpleThink		:1;		// dont use simple think logic for this action
	} fl;


	anStringList			anims;
	anString				state;

	rvAIActionTimer		timer;

	int					blendFrames;
	int					failRate;

	float				minRange;
	float				maxRange;
	float				minRange2d;
	float				maxRange2d;

	float				chance;
	float				diversity;

	EStatus				status;
};

typedef bool (anSAAI::*checkAction_t)(rvAIAction*,int);

class rvPlaybackDriver
{
public:
							rvPlaybackDriver( void ) { mPlaybackDecl = nullptr; mOldPlaybackDecl = nullptr; }

	bool					Start( const char *playback, anEntity *owner, int flags, int numFrames );
	bool					UpdateFrame( anEntity *ent, rvDeclPlaybackData &out );
	void					EndFrame( void );
	bool					IsActive( void ) { return( !!mPlaybackDecl || !!mOldPlaybackDecl ); }

	const char				*GetDestination( void );

// cnicholson: Begin  Added save/restore functionality
	void					Save			( anSaveGame *savefile ) const;
	void					Restore			( anRestoreGame *savefile );
// cnicholson: End  Added save/restore functionality

private:
	int						mLastTime;
	int						mTransitionTime;

	int						mStartTime;
	int						mFlags;
	const rvDeclPlayback	*mPlaybackDecl;
	anVec3					mOffset;

	int						mOldStartTime;
	int						mOldFlags;
	const rvDeclPlayback	*mOldPlaybackDecl;
	anVec3					mOldOffset;
};

/*
===============================================================================

	anSAAI

===============================================================================
*/

const float	AI_TURN_SCALE				= 60.0f;
const float	AI_SEEK_PREDICTION			= 0.3f;
const float	AI_FLY_DAMPENING			= 0.15f;
const float	AI_HEARING_RANGE			= 2048.0f;
const float AI_COVER_MINRANGE			= 4.0f;
const float AI_PAIN_LOOP_DELAY			= 200;
const int	DEFAULT_FLY_OFFSET			= 68.0f;

#define ATTACK_IGNORE					0
#define ATTACK_ON_DAMAGE				BIT(0)
#define ATTACK_ON_ACTIVATE				BIT( 1 )
#define ATTACK_ON_SIGHT					BIT(2)

// defined in script/ai_base.script.  please keep them up to date.

#define	DI_NODIR	-1


//
// events
//
extern const anEventDef AI_DirectDamage;
extern const anEventDef AI_JumpFrame;
extern const anEventDef AI_EnableClip;
extern const anEventDef AI_DisableClip;
extern const anEventDef AI_EnableGravity;
extern const anEventDef AI_DisableGravity;
extern const anEventDef AI_EnablePain;
extern const anEventDef AI_DisablePain;
extern const anEventDef AI_EnableTarget;
extern const anEventDef AI_DisableTarget;
extern const anEventDef AI_EnableMovement;
extern const anEventDef AI_DisableMovement;
extern const anEventDef AI_Vagary_ChooseObjectToThrow;
extern const anEventDef AI_Speak;
extern const anEventDef AI_SpeakRandom;
extern const anEventDef AI_Attack;
extern const anEventDef AI_AttackMelee;
extern const anEventDef AI_WaitMove;
extern const anEventDef AI_EnableDamage;
extern const anEventDef AI_DisableDamage;
extern const anEventDef AI_LockEnemyOrigin;
extern const anEventDef AI_SetEnemy;
extern const anEventDef AI_ScriptedAnim;
extern const anEventDef AI_ScriptedDone;
extern const anEventDef AI_ScriptedStop;
extern const anEventDef AI_SetScript;
extern const anEventDef AI_BecomeSolid;
extern const anEventDef AI_BecomePassive;
extern const anEventDef AI_BecomeAggressive;
extern const anEventDef AI_SetHealth;
extern const anEventDef AI_TakeDamage;
extern const anEventDef AI_EnableBlink;
extern const anEventDef AI_DisableBlink;
extern const anEventDef AI_EnableAutoBlink;
extern const anEventDef AI_DisableAutoBlink;

class idPathCorner;
class idProjectile;
class rvSpawner;
class anSAAIHelper;
class anSAAITether;

class anSAAI : public anActor {
friend class rvAIManager;
friend class anSEASFindAttackPosition;
public:
	CLASS_PROTOTYPE( anSAAI );

							anSAAI();
							~anSAAI();

	void					Save							( anSaveGame *savefile ) const;
	void					Restore							( anRestoreGame *savefile );

	void					Spawn							( void );
	virtual void			TalkTo							( anActor *actor );

	anEntity*				GetEnemy						( void ) const;
 	anEntity*				GetGoalEntity					( void ) const;
	talkState_t				GetTalkState					( void ) const;
	const anVec2&			GetAttackRange					( void ) const;
	const anVec2&			GetFollowRange					( void ) const;
	int						GetTravelFlags					( void ) const;

	void					TouchedByFlashlight				( anActor *flashlight_owner );

 	anEntity *				FindEnemy						( bool inFov, bool forceNearest, float maxDistSqr = 0.0f );
	void					SetSpawner						( rvSpawner* _spawner );
	rvSpawner*				GetSpawner						( void );

	anActor*				GetLeader						( void ) const;

							// Outputs a list of all monsters to the console.
	static void				List_f( const anCommandArgs &args );


	// Add some dynamic externals for debugging
	virtual void			GetDebugInfo					( debugInfoProc_t proc, void* userData );


   	bool					IsEnemyVisible					( void ) const;
  	bool					InCoverMode						( void ) const;
  	bool					InCrouchCoverMode				( void ) const;
 	bool					LookAtCoverTall					( void ) const;
 	bool					InLookAtCoverMode				( void ) const;
 	bool					IsBehindCover					( void ) const;
 	bool					IsLipSyncing					( void ) const;
 	bool					IsSpeaking						( void ) const;
   	bool					IsFacingEnt						( anEntity* targetEnt );
 	bool					IsCoverValid					( void ) const;
	virtual bool			IsCrouching						( void ) const;


public:

	anLinkList<anSAAI>		simpleThinkNode;

	// navigation
	anSEAS*					aas;
	anSEASCallback*			aasFind;

	// movement
	idMoveState				move;
	idMoveState				savedMove;

	// physics
	anPhysics_Monster		physicsObj;

	// weapon/attack vars
	bool					lastHitCheckResult;
	int						lastHitCheckTime;
	int						lastAttackTime;
	float					projectile_height_to_distance_ratio;	// calculates the maximum height a projectile can be thrown
	anList<rvAIAttackAnimInfo_t>	attackAnimInfo;

	mutable anClipModel*	projectileClipModel;
	anEntityPtr<idProjectile> projectile;

	// chatter/talking
	int						chatterTime;
	int						chatterRateCombat;
	int						chatterRateIdle;
	talkState_t				talkState;
	anEntityPtr<anActor>	talkTarget;
	talkMessage_t			talkMessage;
	int						talkBusyCount;
	int						speakTime;

	// Focus
	anEntityPtr<anEntity>	lookTarget;
	aiFocus_t				focusType;
	anEntityPtr<anEntity>	focusEntity;
	float					focusRange;
	int						focusAlignTime;
	int						focusTime;
	anVec3					currentFocusPos;

	// Looking
	bool					allowJointMod;
	int						alignHeadTime;
	int						forceAlignHeadTime;
	anAngles				eyeAng;
	anAngles				lookAng;
	anAngles				destLookAng;
	anAngles				lookMin;
	anAngles				lookMax;
	anList<jointHandle_t>	lookJoints;
	anList<anAngles>		lookJointAngles;
	float					eyeVerticalOffset;
	float					eyeHorizontalOffset;
	float					headFocusRate;
	float					eyeFocusRate;

	// joint controllers
	anAngles				eyeMin;
	anAngles				eyeMax;
	jointHandle_t			orientationJoint;

	anEntityPtr<anEntity>	pusher;
	anEntityPtr<anEntity>	scriptedActionEnt;

	// script variables
	struct aiFlags_s {
		bool		awake					:1;			// set to false until state_wakeup is called.
		bool		damage					:1;
		bool		pain					:1;
		bool		dead					:1;
		bool		activated				:1;
		bool		jump					:1;
		bool		hitEnemy				:1;
		bool		pushed					:1;
		bool		disableAttacks			:1;
		bool		scriptedEndWithIdle 	:1;
		bool		scriptedNeverDormant	:1;			// Prevent going dormant while in scripted sequence
		bool		scripted				:1;
		bool		simpleThink				:1;
		bool		ignoreFlashlight		:1;
		bool		action					:1;
		bool		lookAtPlayer			:1;
		bool		disableLook				:1;
		bool		undying					:1;
		bool		tetherMover				:1;			// Currently using a dynamic tether to a mover
		bool		meleeSuperhero			:1;
		bool		killerGuard				:1;			// Do 100 points of damage with each hit
	} aifl;

	//
	// ai/ai.cpp
	//
	void					SetAAS							( void );
	virtual	void			DormantBegin					( void );		// called when entity becomes dormant
	virtual	void			DormantEnd						( void );		// called when entity wakes from being dormant
	virtual void			Think							( void );
	void					Activate						( anEntity *activator );
	virtual void			Hide							( void );
	virtual void			Show							( void );
	virtual void			AdjustHealthByDamage			( int inDamage );
	void					CalculateAttackOffsets			( void );

	void					InitNonPersistentSpawnArgs		( void );

	/*
	===============================================================================
									Speaking & Chatter
	===============================================================================
	*/
public:

	bool					Speak							( const char *speechDecl, bool random = false );
 	void					StopSpeaking					( bool stopAnims );
	virtual void			CheckBlink						( void );

protected:

	virtual bool			CanPlayChatterSounds			( void ) const;
	void					UpdateChatter					( void );


	/*
	===============================================================================
									Movement
	===============================================================================
	*/

	// static helper functions
public:
							// Finds a path around dynamic obstacles.
	static bool				FindPathAroundObstacles			( const anPhysics *physics, const anSEAS *aas, const anEntity *ignore, const anVec3 &startPos, const anVec3 &seekPos, obstaclePath_t &path );
							// Frees any nodes used for the dynamic obstacle avoidance.
	static void				FreeObstacleAvoidanceNodes		( void );
							// Predicts movement, returns true if a stop event was triggered.
	static bool				PredictPath						( const anEntity *ent, const anSEAS *aas, const anVec3 &start, const anVec3 &velocity, int totalTime, int frameTime, int stopEvent, predictedPath_t &path, const anEntity *ignore = nullptr );
							// Return true if the trajectory of the clip model is collision free.
	static bool				TestTrajectory					( const anVec3 &start, const anVec3 &end, float zVel, float gravity, float time, float max_height, const anClipModel *clip, int clipmask, const anEntity *ignore, const anEntity *targetEntity, int drawtime );
							// Finds the best collision free trajectory for a clip model.
	static bool				PredictTrajectory				( const anVec3 &firePos, const anVec3 &target, float projectileSpeed, const anVec3 &projGravity, const anClipModel *clip, int clipmask, float max_height, const anEntity *ignore, const anEntity *targetEntity, int drawtime, anVec3 &aimDir );


	// special flying code
	void					AdjustFlyingAngles				( void );
	void					AddFlyBob						( anVec3 &vel );
	void					AdjustFlyHeight					( anVec3 &vel, const anVec3 &goalPos );
	void					FlySeekGoal						( anVec3 &vel, anVec3 &goalPos );
	void					AdjustFlySpeed					( anVec3 &vel );
	void					FlyTurn							( void );

	// movement types
	void					Move							( void );
	virtual	void			DeadMove						( void );
	void					AnimMove						( void );
	void					SlideMove						( void );
	void					PlaybackMove					( void );
	void					FlyMove							( void );
	void					StaticMove						( void );
	void					RVMasterMove					( void );
	void					SetMoveType						( moveType_t moveType );
	//twhitaker: added custom move type
	virtual void			CustomMove						( void );

	// movement actions
	void					KickObstacles					( const anVec3 &dir, float force, anEntity *alwaysKick );

	// steering
	virtual void			ApplyImpulse					( anEntity *ent, int id, const anVec3 &point, const anVec3 &impulse, bool splash = false );
	void					GetAnimMoveDelta				( const anMat3 &oldaxis, const anMat3 &axis, anVec3 &delta );
	void					CheckObstacleAvoidance			( const anVec3 &goalPos, anVec3 &newPos, anReachability* goalReach=0  );
	bool					GetMovePos						( anVec3 &seekPos, anReachability** seekReach=0 );


	// navigation
	float					TravelDistance					( const anVec3 &end ) const;
	float					TravelDistance					( const anVec3 &start, const anVec3 &end ) const;
	float					TravelDistance					( anEntity* ent ) const;
	float					TravelDistance					( anEntity* start, anEntity* end ) const;
	int						PointReachableAreaNum			( const anVec3 &pos, const float boundsScale = 2.0f ) const;
	bool					PathToGoal						( seasPath_t &path, int areaNum, const anVec3 &origin, int goalAreaNum, const anVec3 &goalOrigin ) const;
	void					BlockedFailSafe					( void );

	// turning
	void					Turn							( void );
	bool					TurnToward						( float yaw );
	bool					TurnToward						( const anVec3 &pos );
	bool					TurnTowardLeader				( bool faceLeaderByDefault=false );
	bool					FacingIdeal						( void );
	bool					DirectionalTurnToward			( const anVec3 &pos );

	// movement control
	bool					FaceEnemy						( void );
	bool					FaceEntity						( anEntity *ent );
  	bool					SlideToPosition					( const anVec3 &pos, float time );
 	bool					WanderAround					( void );
	bool					StepDirection					( float dir );
	bool					NewWanderDir					( const anVec3 &dest );


	/*
	===============================================================================
									Reactions
	===============================================================================
	*/
	void					ReactToShotAt					( anEntity* attacker, const anVec3 &origOrigin, const anVec3 &origDir );
	void					ReactToPain						( anEntity* attacker, int damage );

	/*
	===============================================================================
									AI helpers
	===============================================================================
	*/

public:

	void					UpdateHelper					( void );
	anSAAIHelper*				GetActiveHelper					( void );

	/*
	===============================================================================
									Sensory Perception
	===============================================================================
	*/

public:

	const anVec3&			LastKnownPosition				( const anEntity *ent );
	const anVec3&			LastKnownFacing					( const anEntity *ent );
	anEntity *				HeardSound						( int ignore_team );
	int						ReactionTo						( const anEntity *ent );
	void					SetLastVisibleEnemyTime			( int time=-1/* DEFAULT IS CURRENT TIME*/ );
	bool					IsEnemyRecentlyVisible			( float maxLostVisTimeScale = 1.0f ) const;

	/*
	===============================================================================
									Passive
	===============================================================================
	*/

public:

	void					SetTalkState					( talkState_t state );
	void					SetPassivePrefix				( const char* prefix );

protected:

	bool					GetPassiveAnimPrefix			( const char* animName, anString& animPrefix );

	/*
	===============================================================================
									Combat
	===============================================================================
	*/

public:

	// enemy managment
	bool					SetEnemy						( anEntity *newEnemy );
	void					ClearEnemy						( bool dead = false );

	void					UpdateEnemy						( void );
	void					UpdateEnemyPosition				( bool force = true );
	void					UpdateEnemyVisibility			( void );

	// Attack direction
	bool					GetAimDir						( const anVec3& source, const anEntity* aimAtEnt, const anDict* projectileDict, anEntity *ignore, anVec3 &aimDir, float aimOffset, float predict ) const;
	void					GetPredictedAimDirOffset		( const anVec3& source, const anVec3& target, float projectileSpeed, const anVec3& targetVelocity, anVec3& offset ) const;

	// damage
	virtual bool			Pain							( anEntity *inflictor, anEntity *attacker, int damage, const anVec3 &dir, int location );
	virtual void			Killed							( anEntity *inflictor, anEntity *attacker, int damage, const anVec3 &dir, int location );
	bool					CheckDeathCausesMissionFailure	( void );

	// attacks
	virtual bool			Attack							( const char* attackName, jointHandle_t joint, anEntity* target, const anVec3& pushVelocity = vec3_origin );
	virtual idProjectile*	AttackRanged					( const char* attackName, const anDict* attackDict, jointHandle_t joint, anEntity* target, const anVec3& pushVelocity = vec3_origin );
	virtual idProjectile*	AttackProjectile				( const anDict* projectileDict, const anVec3 &org, const anAngles &ang );
	virtual bool			AttackMelee						( const char* attackName, const anDict* meleeDict );

	void					CreateProjectileClipModel		( void ) const;
	idProjectile*			CreateProjectile				( const anDict* projectileDict, const anVec3 &pos, const anVec3 &dir );
	void					RemoveProjectile				( void );
	virtual void			DamageFeedback					( anEntity *victim, anEntity *inflictor, int &damage );
	void					DirectDamage					( const char *meleeDefName, anEntity *ent );
	bool					TestMelee						( void ) const;
	void					PushWithAF						( void );
	bool					IsMeleeNeeded					( void );

	// special effects
	void					GetMuzzle						( jointHandle_t joint, anVec3 &muzzle, anMat3 &axis );
	void					LerpLookAngles					( anAngles &curAngles, anAngles newAngles, float orientationJointYaw, float focusRate );
	virtual bool			UpdateAnimationControllers		( void );

	// AI script state management
	void					UpdateStates					( void );
	void					UpdateFocus						( const anMat3& orientationAxis );
	void					SetFocus						( aiFocus_t focus, int time );

	// event?
	virtual void			FootStep						( void );

	void					CreateMissile					( jointHandle_t joint  );
	void					RadiusDamageFromJoint			( const char *jointname, const char *damageDefName );
	void					BecomeSolid						( void );
	void					BecomeNonSolid					( void );
	const char *			ChooseAnim						( int channel, const char *animname );

	//
	// ai/ai_events.cpp
	//
public:


	virtual bool			CanTakeDamage					( void ) const;
	virtual bool			CanTakePain						( void ) const;
	virtual bool			CanTurn							( void ) const;
	virtual bool			CanMove							( void ) const;
	virtual bool			CanAnnounce						( float chance ) const;

	virtual bool			SkipCurrentDestination			( void ) const;

// ----------------------------- Functions ------------------------------------

	virtual bool			DoDormantTests					( void );

	void					OverrideFlag					( aiFlagOverride_t flag, bool value );
	void					RestoreFlag						( aiFlagOverride_t flag );

	virtual bool			SkipImpulse						( anEntity *ent, int id );

	virtual const char*		GetIdleAnimName					( void );

	bool					RespondToFlashlight				( void )				{ return !aifl.ignoreFlashlight;}
	bool					ForceFaceEnemy					( void )				{ return ( move.moveCommand == MOVE_TO_ENEMY ); }

	bool					CanBecomeSolid					( void );
	bool					CanHitEnemyFromAnim				( int animNum, anVec3 offset = vec3_origin );
	bool					CanHitEnemy						( void );
	bool					CanHitEnemyFromJoint			( const char *jointname );

	float					GetTurnDelta					( void );

	int						TestTrajectory					( const anVec3 &firePos, const anVec3 &target, const char *projectileName );
	bool					TestAnimMove					( int animNum, anEntity *ignore = nullptr, anVec3 *pMoveVec = nullptr );
	void					ExecScriptFunction				( rvScriptFuncUtility& func, anEntity* parm = nullptr );
	void					SetLeader						( anEntity *newLeader );

	int						CheckMelee						( bool disableAttack );
	bool					CheckForEnemy					( bool useFov, bool force = false );
 	bool					CheckForCloserEnemy				( void );
 	bool					CheckForReplaceEnemy			( anEntity* replacement );
	bool					CheckForTeammateEnemy			( void );

 	void					DrawSuspicion					( void );
 	float					RateSuspiciousness				( anActor* shady, bool rateSound = false );
 	void					RateSuspicionLevel				( void );

	void					UpdatePlayback					( anVec3 &goalPos, anVec3 &delta, anVec3 &oldorigin, anMat3 &oldaxis );

	void					LookAtEntity					( anEntity *ent, float duration );

// ----------------------------- Variables ------------------------------------

	int						actionAnimNum;			// Index of animation to use for the upcoming action
	int						actionTime;				// Time line for actions (time is stopped when an action is running)
	int						actionSkipTime;			// Time to use if an action is skipped by another

	int						flagOverrides;

	float					announceRate;			// How often (0 - 1.0f) the AI will make certain announcements.

	rvAICombat_t			combat;					// Members related to combat state
	rvAIPassive_t			passive;				// Members related to passive state
	rvAIEnemy_t				enemy;					// Members related to tracking enemies
	rvAIPain_t				pain;
	rvAIFuncs_t				funcs;

	rvPlaybackDriver		mPlayback;
	rvPlaybackDriver		mLookPlayback;

	anSEASTacticalSensor*	aasSensor;

	anEntityPtr<anSAAITether>	tether;
	anEntityPtr<anSAAIHelper>	helperCurrent;
	anEntityPtr<anSAAIHelper>	helperIdeal;
	anEntityPtr<anActor>	leader;
	anEntityPtr<rvSpawner>	spawner;

	bool						ValidateCover					( void );

	virtual bool				UpdateRunStatus					( void );
	bool						UpdateTactical					( int delay = 0 );
	void						ForceTacticalUpdate				( void );
	bool						UpdateTactical_r				( void );
	virtual int					FilterTactical					( int availableTactical );
	void						WakeUpTargets					( void );

	virtual aiCTResult_t		CheckTactical					( aiTactical_t tactical );

	void						Begin							( void );
	void						WakeUp							( void );

	virtual void				Prethink						( void );
	virtual void				Postthink						( void );

	/*
	===============================================================================
							  Threat Management
	===============================================================================
	*/

protected:

	virtual void			UpdateThreat					( void );
	virtual float			CalculateEnemyThreat			( anEntity* enemy );

	/*
	===============================================================================
									Tethers
	===============================================================================
	*/

public:

	virtual bool			IsTethered						( void ) const;
	bool					IsWithinTether					( void ) const;
	anSAAITether*				GetTether						( void );
	virtual void			SetTether						( anSAAITether* newTether );

	/*
	===============================================================================
									Scripting
	===============================================================================
	*/

public:

	void					ScriptedMove					( anEntity* destEnt, float minDist, bool endWithIdle );
	void					ScriptedFace					( anEntity* faceEnt, bool endWithIdle );
	void					ScriptedAnim					( const char* animname, int blendFrames, bool loop, bool endWithIdle );
	void					ScriptedPlaybackMove			( const char* playback, int flags, int numFrames );
	void					ScriptedPlaybackAim				( const char* playback, int flags, int numFrames );
	void					ScriptedAction					( anEntity* actionEnt, bool endWithIdle );
	void					ScriptedStop					( void );

	void					SetScript						( const char* scriptName, const char* funcName );

private:

	bool					ScriptedBegin					( bool endWithIdle, bool allowDormant = false );
	void					ScriptedEnd						( void );

	/*
	===============================================================================
									Handlers
	===============================================================================
	*/

protected:

	virtual void			OnDeath							( void );
	virtual void			OnStateChange					( int channel );
	virtual void			OnUpdatePlayback				( const rvDeclPlaybackData& pbd );
	virtual void			OnEnemyChange					( anEntity* oldEnemy );
	virtual void			OnLeaderChange					( anEntity* oldLeader );
	virtual void			OnStartMoving					( void );
	virtual void			OnStopMoving					( aiMoveCommand_t oldMoveCommand );
	virtual void			OnTacticalChange				( aiTactical_t oldTactical );
	virtual void			OnFriendlyFire					( anActor* attacker );
	virtual void			OnWakeUp						( void );
	virtual void			OnTouch							( anEntity *other, trace_t *trace );
	virtual void			OnCoverInvalidated				( void );
	virtual void			OnCoverNotFacingEnemy			( void );
	virtual void			OnEnemyVisiblityChange			( bool oldVisible );
	virtual void			OnStartAction					( void );
	virtual void			OnStopAction					( void );
	virtual void			OnSetKey						( const char* key, const char* value );

	/*
	===============================================================================
									Movement / Turning
	===============================================================================
	*/

protected:

	bool					StartMove						( aiMoveCommand_t command, const anVec3& goalOrigin, int goalArea, anEntity* goalEntity, seasFeature_t* feature, float range );
	void					StopMove						( moveStatus_t status );

	bool					MoveTo							( const anVec3 &pos, float range = 0.0f );
	bool					MoveToAttack					( anEntity *ent, int attack_anim );
	bool					MoveToTether					( anSAAITether* tether );
	virtual bool			MoveToEnemy						( void );
	bool					MoveToEntity					( anEntity *ent, float range = 0.0f );
	bool					MoveToCover						( float minRange, float maxRange, aiTactical_t coverType );
	bool					MoveToHide						( void );

	bool					MoveOutOfRange					( anEntity *entity, float range, float minRange=0.0f );

	void					AnimTurn						( float angles, bool force );

	bool					ReachedPos						( const anVec3 &pos, const aiMoveCommand_t moveCommand, float range = 0.0f ) const;

	/*
	===============================================================================
									Debug
	===============================================================================
	*/

public:

	void					DrawRoute						( void ) const;
	void					DrawTactical					( void );

	/*
	===============================================================================
									Announcements
	===============================================================================
	*/

public:

	bool					ActorIsBehindActor				( anActor* ambusher, anActor* victim );
	void					AnnounceNewEnemy				( void );
	void					AnnounceKill					( anActor *victim );
	void					AnnounceTactical				( aiTactical_t newTactical );
	void					AnnounceSuppressed				( anActor *suppressor );
	void					AnnounceSuppressing				( void );
	void					AnnounceFlinch					( anEntity *attacker );
	void					AnnounceInjured					( void );
	void					AnnounceFriendlyFire			( anActor* attacker );
	void					AnnounceGrenade					( void );
	void					AnnounceGrenadeThrow			( void );

	/*
	===============================================================================
									Actions
	===============================================================================
	*/

protected:

	rvAIActionTimer			actionTimerRangedAttack;
	rvAIActionTimer			actionTimerEvade;
	rvAIActionTimer			actionTimerSpecialAttack;
	rvAIActionTimer			actionTimerPain;

	rvAIAction				actionEvadeLeft;
	rvAIAction				actionEvadeRight;
	rvAIAction				actionRangedAttack;
	rvAIAction				actionMeleeAttack;
	rvAIAction				actionLeapAttack;
	rvAIAction				actionJumpBack;

	bool					UpdateAction						( void );
	virtual bool			CheckActions						( void );
	virtual bool			CheckPainActions					( void );

	bool					PerformAction						( rvAIAction* action, bool (anSAAI::*)(rvAIAction*,int), rvAIActionTimer* timer = nullptr );
	void					PerformAction						( const char* stateName, int blendFrames = 0, bool noPain = false );


	// twhitaker: needed this for difficulty settings
	virtual void			Event_PostSpawn						( void );


public:

	virtual bool			CheckAction_EvadeLeft				( rvAIAction* action, int animNum );
	virtual bool			CheckAction_EvadeRight				( rvAIAction* action, int animNum );
	virtual bool			CheckAction_RangedAttack			( rvAIAction* action, int animNum );
	virtual bool			CheckAction_MeleeAttack				( rvAIAction* action, int animNum );
	bool					CheckAction_LeapAttack				( rvAIAction* action, int animNum );
	virtual bool			CheckAction_JumpBack				( rvAIAction* action, int animNum );

	/*
	===============================================================================
									Events
	===============================================================================
	*/

private:

	// Orphaned events
	void					Event_ClosestReachableEnemyOfEntity	( anEntity *team_mate );
	void					Event_GetReachableEntityPosition	( anEntity *ent );
	void					Event_EntityInAttackCone			( anEntity *ent );
	void					Event_TestAnimMoveTowardEnemy		( const char *animname );
	void					Event_TestAnimMove					( const char *animname );
	void					Event_TestMoveToPosition			( const anVec3 &position );
	void					Event_TestMeleeAttack				( void );
	void					Event_TestAnimAttack				( const char *animname );
	void					Event_SaveMove						( void );
	void					Event_RestoreMove					( void );
	void					Event_ThrowMoveable					( void );
	void					Event_ThrowAF						( void );
	void					Event_PredictEnemyPos				( float time );
	void					Event_FindActorsInBounds			( const anVec3 &mins, const anVec3 &maxs );

	void					Event_Activate						( anEntity *activator );
	void					Event_Touch							( anEntity *other, trace_t *trace );
	void					Event_LookAt						( anEntity* lookAt );

	void					Event_SetAngles						( anAngles const &ang );
	void					Event_SetEnemy						( anEntity *ent );
	void					Event_SetHealth						( float newHealth );
	void					Event_SetTalkTarget					( anEntity *target );
	void					Event_SetTalkState					( int state );
	void					Event_SetLeader						( anEntity *newLeader );
	void					Event_SetScript						( const char* scriptName, const char* funcName );
	void					Event_SetMoveSpeed					( int speed );
	void					Event_SetPassivePrefix				( const char* prefix );

	void					Event_GetAngles						( void );
	void					Event_GetEnemy						( void );
	void					Event_GetLeader						( void );

	void					Event_Attack						( const char* attackName, const char* jointName );
	void					Event_AttackMelee					( const char *meleeDefName );

	void					Event_DirectDamage					( anEntity *damageTarget, const char *damageDefName );
	void					Event_RadiusDamageFromJoint			( const char *jointname, const char *damageDefName );
	void					Event_CanBecomeSolid				( void );
	void					Event_BecomeSolid					( void );
	void					Event_BecomeNonSolid				( void );
	void					Event_BecomeRagdoll					( void );
	void					Event_StopRagdoll					( void );
	void					Event_FaceEnemy						( void );
	void					Event_FaceEntity					( anEntity *ent );
	void					Event_WaitMove						( void );

	void					Event_BecomePassive					( int ignoreEnemies );
	void					Event_BecomeAggressive				( void );

	void					Event_EnableDamage					( void );
	void					Event_EnableClip					( void );
	void					Event_EnableGravity					( void );
	void					Event_EnableAFPush					( void );
	void					Event_EnablePain					( void );
	void					Event_DisableDamage					( void );
	void					Event_DisableClip					( void );
	void					Event_DisableGravity				( void );
	void					Event_DisableAFPush					( void );
	void					Event_DisablePain					( void );
	void					Event_EnableTarget					( void );
	void					Event_DisableTarget					( void );
	void					Event_TakeDamage					( float takeDamage );
	void					Event_SetUndying					( float setUndying );
	void					Event_EnableAutoBlink				( void );
	void					Event_DisableAutoBlink				( void );

	void					Event_LockEnemyOrigin				( void );

	void					Event_StopThinking					( void );
	void					Event_JumpFrame						( void );
	void					Event_RealKill						( void );
	void					Event_Kill							( void );
	void					Event_RemoveUpdateSpawner			( void );
	void					Event_AllowHiddenMovement			( int enable );
	void 					Event_CanReachPosition				( const anVec3 &pos );
	void 					Event_CanReachEntity				( anEntity *ent );
	void					Event_CanReachEnemy					( void );

	void					Event_IsSpeaking					( void );
	void					Event_IsTethered					( void );
	void					Event_IsWithinTether				( void );
	void					Event_IsMoving						( void );

	void					Event_Speak							( const char *speechDecl );
	void					Event_SpeakRandom					( const char *speechDecl );

	void					Event_ScriptedMove					( anEntity* destEnt, float minDist, bool endWithIdle );
	void					Event_ScriptedFace					( anEntity* faceEnt, bool endWithIdle );
	void					Event_ScriptedAnim					( const char* animname, int blendFrames, bool loop, bool endWithIdle );
	void					Event_ScriptedPlaybackMove			( const char* playback, int flags, int numFrames );
	void					Event_ScriptedPlaybackAim			( const char* playback, int flags, int numFrames );
	void					Event_ScriptedAction				( anEntity* actionEnt, bool endWithIdle );
	void					Event_ScriptedDone					( void );
	void					Event_ScriptedStop					( void );
	void					Event_ScriptedJumpDown				( float yaw );

	void					Event_FindEnemy						( float distSquare );
	void					Event_SetKey						( const char *key, const char *value );


	/*
	===============================================================================
									States
	===============================================================================
	*/

protected:

	// Wait states
	stateResult_t			State_Wait_Activated				( const stateParms_t& parms );
	stateResult_t			State_Wait_ScriptedDone				( const stateParms_t& parms );
	stateResult_t			State_Wait_Action					( const stateParms_t& parms );
	stateResult_t			State_Wait_ActionNoPain				( const stateParms_t& parms );

	// Global states
	stateResult_t			State_WakeUp						( const stateParms_t& parms );
	stateResult_t			State_TriggerAnim					( const stateParms_t& parms );
	stateResult_t			State_Wander						( const stateParms_t& parms );
	stateResult_t			State_Killed						( const stateParms_t& parms );
	stateResult_t			State_Dead							( const stateParms_t& parms );
	stateResult_t			State_LightningDeath				( const stateParms_t& parms );
	stateResult_t			State_Burn							( const stateParms_t& parms );
	stateResult_t			State_Remove						( const stateParms_t& parms );

	stateResult_t			State_Passive						( const stateParms_t& parms );

	stateResult_t			State_Combat						( const stateParms_t& parms );
	stateResult_t			State_CombatCover					( const stateParms_t& parms );
	stateResult_t			State_CombatMelee					( const stateParms_t& parms );
	stateResult_t			State_CombatRanged					( const stateParms_t& parms );
	stateResult_t			State_CombatTurret					( const stateParms_t& parms );
	virtual stateResult_t	State_CombatHide					( const stateParms_t& parms );

	stateResult_t			State_MovePlayerPush				( const stateParms_t& parms );
	stateResult_t			State_MoveTether					( const stateParms_t& parms );
	stateResult_t			State_MoveFollow					( const stateParms_t& parms );

	stateResult_t			State_ScriptedMove					( const stateParms_t& parms );
	stateResult_t			State_ScriptedFace					( const stateParms_t& parms );
	stateResult_t			State_ScriptedStop					( const stateParms_t& parms );
	stateResult_t			State_ScriptedPlaybackMove			( const stateParms_t& parms );
	stateResult_t			State_ScriptedPlaybackAim			( const stateParms_t& parms );
	stateResult_t			State_ScriptedJumpDown				( const stateParms_t& parms );

	// Torso States
	stateResult_t			State_Torso_Idle					( const stateParms_t& parms );
	stateResult_t			State_Torso_Sight					( const stateParms_t& parms );
	stateResult_t			State_Torso_CustomCycle				( const stateParms_t& parms );
	stateResult_t			State_Torso_Action					( const stateParms_t& parms );
	stateResult_t			State_Torso_FinishAction			( const stateParms_t& parms );
	stateResult_t			State_Torso_Pain					( const stateParms_t& parms );
	stateResult_t			State_Torso_ScriptedAnim			( const stateParms_t& parms );
	stateResult_t			State_Torso_PassiveIdle				( const stateParms_t& parms );
	stateResult_t			State_Torso_PassiveFidget			( const stateParms_t& parms );

	// Leg States
	stateResult_t			State_Legs_Idle						( const stateParms_t& parms );
	stateResult_t			State_Legs_TurnLeft					( const stateParms_t& parms );
	stateResult_t			State_Legs_TurnRight				( const stateParms_t& parms );
	stateResult_t			State_Legs_Move						( const stateParms_t& parms );
	stateResult_t			State_Legs_MoveThink				( const stateParms_t& parms );
	stateResult_t			State_Legs_ChangeDirection			( const stateParms_t& parms );

	// Head states
	stateResult_t			State_Head_Idle						( const stateParms_t& parms );

	CLASS_STATES_PROTOTYPE ( anSAAI );
};

/*
===============================================================================

	anSAAI Inlines

===============================================================================
*/

ARC_INLINE int DelayTime( int min, int range ) {
	return min + gameLocal.random.RandomInt ( range + 1 );
}

ARC_INLINE const anVec2& anSAAI::GetAttackRange ( void ) const {
	return combat.attackRange;
}

ARC_INLINE const anVec2& anSAAI::GetFollowRange ( void ) const {
	return move.followRange;
}

ARC_INLINE int anSAAI::GetTravelFlags ( void ) const {
	return move.travelFlags;
}

ARC_INLINE bool anSAAI::IsEnemyVisible ( void ) const {
	return enemy.ent && enemy.fl.visible;
}

ARC_INLINE bool anSAAI::IsEnemyRecentlyVisible( float maxLostVisTimeScale ) const {
	return (enemy.ent
			&& combat.fl.seenEnemyDirectly
 			&& (enemy.lastVisibleTime && gameLocal.time-enemy.lastVisibleTime < (combat.maxLostVisTime * maxLostVisTimeScale)));
}

ARC_INLINE bool anSAAI::LookAtCoverTall( void ) const {
 	return ( aasSensor->Look()
 			&& (aasSensor->Look()->flags&FEATURE_LOOK_OVER)
 			&& aasSensor->Look()->height > 40.0f );
}

ARC_INLINE bool anSAAI::InLookAtCoverMode ( void ) const {
 	return ( !IsBehindCover()
 			&& !aifl.action
			&& move.fl.moving
 			&& (combat.fl.alert || combat.fl.aware)
			&& aasSensor->Look()
 			&& combat.tacticalCurrent != AITACTICAL_MELEE
 			&& combat.tacticalCurrent != AITACTICAL_HIDE
 			&& !IsEnemyRecentlyVisible(0.2f));
}

ARC_INLINE bool anSAAI::InCoverMode ( void ) const {
	return ( (1<<combat.tacticalCurrent) & AITACTICAL_COVER_BITS ) && aasSensor->Reserved();
}

ARC_INLINE bool anSAAI::InCrouchCoverMode ( void ) const {
	return ( InCoverMode() && (aasSensor->Reserved()->flags&FEATURE_LOOK_OVER) );
}

ARC_INLINE bool anSAAI::IsBehindCover ( void ) const {
	return ( InCoverMode() && move.fl.done && (aifl.action || DistanceTo2d ( aasSensor->ReservedOrigin() ) < AI_COVER_MINRANGE) );
}

ARC_INLINE bool anSAAI::IsSpeaking ( void ) const {
	return speakTime && gameLocal.time < speakTime;
}

ARC_INLINE bool anSAAI::IsFacingEnt ( anEntity* targetEnt ) {
	return( move.moveCommand == MOVE_FACE_ENTITY && move.goalEntity == targetEnt && FacingIdeal() );
}

ARC_INLINE bool anSAAI::IsCoverValid (  ) const {
 	return combat.coverValidTime && (gameLocal.time - combat.coverValidTime < combat.maxInvalidCoverTime);
}

ARC_INLINE anActor* anSAAI::GetLeader ( void ) const {
	return leader;
}

ARC_INLINE anEntity* anSAAI::GetGoalEntity ( void ) const {
	return move.goalEntity;
}

ARC_INLINE bool anSAAI::CanTakeDamage( void ) const {
	return anActor::CanTakeDamage( );
}

ARC_INLINE bool anSAAI::CanTakePain ( void ) const {
	return !disablePain;
}

ARC_INLINE bool anSAAI::CanTurn ( void ) const {
	return move.turnRate && !move.fl.noTurn && !move.fl.disabled;
}

ARC_INLINE bool anSAAI::CanMove ( void ) const {
	return !move.fl.disabled && !move.fl.blocked && gameLocal.GetTime()>move.blockTime;
}

ARC_INLINE bool anSAAI::CanAnnounce ( float chance ) const {
	return !aifl.dead && !IsSpeaking() && !af.IsActive() && !combat.fl.noChatter && ( gameLocal.random.RandomFloat() < chance );
}

ARC_INLINE void anSAAI::SetFocus ( aiFocus_t focus, int time ) {
	focusType = focus;
	focusTime = gameLocal.time + time;
}

ARC_INLINE anEntity *anSAAI::GetEnemy( void ) const {
	return enemy.ent;
}

ARC_INLINE void anSAAI::ForceTacticalUpdate ( void ) {
	combat.tacticalUpdateTime = 0;
	combat.tacticalMaskUpdate = 0;
	delete aasFind;
	aasFind = nullptr;
}

/*
===============================================================================

	externs

===============================================================================
*/

extern const char* aiActionStatusString [ rvAIAction::STATUS_MAX ];
extern const char* aiTalkMessageString	[ ];
extern const char* aiTacticalString		[ AITACTICAL_MAX ];
extern const char* aiMoveCommandString	[ NUM_MOVE_COMMANDS ];
extern const char* aiFocusString		[ AIFOCUS_MAX ];

#endif /* !__AI_H__ */

