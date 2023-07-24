
#ifndef __PHYSICS_PLAYER_H__
#define __PHYSICS_PLAYER_H__

/*
===================================================================================

	Player physics

	Simulates the motion of a player through the environment. Input from the
	player is used to allow a certain degree of control over the motion.

===================================================================================
*/

// movementType
typedef enum {
	PM_NORMAL,				// normal physics
	PM_DEAD,				// no acceleration or turning, but free falling
	PM_SPECTATOR,			// flying without gravity but with collision detection
	PM_FREEZE,				// stuck in place without control
	PM_NOCLIP				// flying without collision detection nor gravity
} pmtype_t;

typedef enum {
	WATERLEVEL_NONE,
	WATERLEVEL_FEET,
	WATERLEVEL_WAIST,
	WATERLEVEL_HEAD
} waterLevel_t;

#define	MAXTOUCH					32

typedef struct playerPState_s {
	arcVec3					origin;
	arcVec3					velocity;
	arcVec3					localOrigin;
	arcVec3					pushVelocity;
// RAVEN BEGIN
// bdube: added
	arcVec3					lastPushVelocity;
// RAVEN END
	float					stepUp;
	int						movementType;
	int						movementFlags;
	int						movementTime;
// RAVEN BEGIN
// bdube: crouch slide
	int						crouchSlideTime;
// RAVEN END
} playerPState_t;

class idPhysics_Player : public idPhysics_Actor {

public:
	CLASS_PROTOTYPE( idPhysics_Player );

							idPhysics_Player( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

							// initialisation
	void					SetSpeed( const float newWalkSpeed, const float newCrouchSpeed );
	void					SetMaxStepHeight( const float newMaxStepHeight );
	float					GetMaxStepHeight( void ) const;
	void					SetMaxJumpHeight( const float newMaxJumpHeight );
	void					SetMovementType( const pmtype_t type );
	void					SetPlayerInput( const usercmd_t &cmd, const idAngles &newViewAngles );
	void					SetKnockBack( const int knockBackTime );
	void					SetDebugLevel( bool set );
							// feed back from last physics frame
	waterLevel_t			GetWaterLevel( void ) const;
	int						GetWaterType( void ) const;
	bool					HasJumped( void ) const;
	bool					HasSteppedUp( void ) const;
	float					GetStepUp( void ) const;
	bool					IsCrouching( void ) const;
	bool					OnLadder( void ) const;
	const arcVec3 &			PlayerGetOrigin( void ) const;	// != GetOrigin

public:	// common physics interface
	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	void					GetImpactInfo( const int id, const arcVec3 &point, impactInfo_t *info ) const;
	void					ApplyImpulse( const int id, const arcVec3 &point, const arcVec3 &impulse );
	bool					IsAtRest( void ) const;
	int						GetRestStartTime( void ) const;

	void					SaveState( void );
	void					RestoreState( void );

	void					SetOrigin( const arcVec3 &newOrigin, int id = -1 );
	void					SetAxis( const arcMat3 &newAxis, int id = -1 );

	void					Translate( const arcVec3 &translation, int id = -1 );
	void					Rotate( const idRotation &rotation, int id = -1 );

	void					SetLinearVelocity( const arcVec3 &newLinearVelocity, int id = 0 );

	const arcVec3 &			GetLinearVelocity( int id = 0 ) const;

	void					SetPushed( int deltaTime );
	const arcVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;
	void					ClearPushedVelocity( void );

	void					SetMaster( idEntity *master, const bool orientated = true );

	void					WriteToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const idBitMsgDelta &msg );

// RAVEN BEGIN
// kfuller: Added
	bool					IsNoclip( void ) const;
	bool					IsDead( void ) const;
// RAVEN END

	void					SetClipModelNoLink( idClipModel *clip );

private:
	// player physics state
	playerPState_t			current;
	playerPState_t			saved;

	// properties
	float					walkSpeed;
	float					crouchSpeed;
	float					maxStepHeight;
	float					maxJumpHeight;
	int						debugLevel;				// if set, diagnostic output will be printed

	// player input
	usercmd_t				command;
	idAngles				viewAngles;

	// run-time variables
	int						framemsec;
	float					frametime;
	float					playerSpeed;
	arcVec3					viewForward;
	arcVec3					viewRight;

	// walk movement
	bool					walking;
	bool					groundPlane;
	trace_t					groundTrace;
	const arcMaterial *		groundMaterial;

	// ladder movement
	bool					ladder;
	arcVec3					ladderNormal;

	// results of last evaluate
	waterLevel_t			waterLevel;
	int						waterType;

private:
	float					CmdScale( const usercmd_t &cmd ) const;
	void					Accelerate( const arcVec3 &wishdir, const float wishspeed, const float accel );
	bool					SlideMove( bool gravity, bool stepUp, bool stepDown, bool push );
	void					Friction( void );
	void					WaterJumpMove( void );
	void					WaterMove( void );
	void					FlyMove( void );
	void					AirMove( void );
	void					WalkMove( void );
	void					DeadMove( void );
	void					NoclipMove( void );
	void					SpectatorMove( void );
	void					LadderMove( void );
	void					CorrectAllSolid( trace_t &trace, int contents );
// RAVEN BEGIN
// MrE: check stuck
	void					CheckGround( bool checkStuck );
// RAVEN END
	void					CheckDuck( void );
	void					CheckLadder( void );
	bool					CheckJump( void );
	bool					CheckWaterJump( void );
	void					SetWaterLevel( void );
	void					DropTimers( void );
	void					MovePlayer( int msec );

	float					Pm_Accelerate( void );
	float					Pm_AirAccelerate( void );
};

ARC_INLINE bool idPhysics_Player::IsNoclip( void ) const {
	return current.movementType == PM_NOCLIP;
}

ARC_INLINE bool idPhysics_Player::IsDead( void ) const {
	return current.movementType == PM_DEAD;
}

#endif /* !__PHYSICS_PLAYER_H__ */
