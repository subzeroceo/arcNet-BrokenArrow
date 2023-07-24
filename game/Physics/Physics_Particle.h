#ifndef __PHYSICS_PARTICLE_H__
#define __PHYSICS_PARTICLE_H__

/*
===================================================================================

	Particle Physics

	Employs an impulse based dynamic simulation which is not very accurate but
	relatively fast and still reliable due to the continuous collision detection.

===================================================================================
*/

typedef struct particlePState_s {
	int						atRest;						// set when simulation is suspended
	arcVec3					localOrigin;				// origin relative to master
	arcMat3					localAxis;					// axis relative to master
	arcVec3					pushVelocity;				// push velocity
	arcVec3					origin;
	arcVec3					velocity;
	bool					onGround;
	bool					inWater;
} particlePState_t;

class rvPhysics_Particle : public idPhysics_Base {

public:

	CLASS_PROTOTYPE( rvPhysics_Particle );

							rvPhysics_Particle( void );
							~rvPhysics_Particle( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

							// initialisation
	void					SetFriction( const float linear, const float angular, const float contact );
	void					SetBouncyness( const float b, bool allowBounce );

							// put to rest untill something collides with this physics object
	void					PutToRest( void );
							// same as above but drop to the floor first
	void					DropToFloor( void );

							// returns true if touching the ground
	bool					IsOnGround ( void ) const;
	bool					IsInWater ( void ) const;

public:	// common physics interface
	void					SetClipModel( idClipModel *model, float density, int id = 0, bool freeOld = true );
	idClipModel *			GetClipModel( int id = 0 ) const;
	int						GetNumClipModels( void ) const;

	void					SetContents( int contents, int id = -1 );
	int						GetContents( int id = -1 ) const;

	const arcBounds &		GetBounds( int id = -1 ) const;
	const arcBounds &		GetAbsBounds( int id = -1 ) const;

	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	bool					CanBounce ( void ) const;

	bool					EvaluateContacts( void );

	void					Activate( void );
	bool					IsAtRest( void ) const;
	int						GetRestStartTime( void ) const;
	bool					IsPushable( void ) const;

	void					SaveState( void );
	void					RestoreState( void );

	void					SetOrigin( const arcVec3 &newOrigin, int id = -1 );
	void					SetAxis( const arcMat3 &newAxis, int id = -1 );

	void					Translate( const arcVec3 &translation, int id = -1 );
	void					Rotate( const idRotation &rotation, int id = -1 );

	const arcVec3 &			GetOrigin( int id = 0 ) const;
	const arcMat3 &			GetAxis( int id = 0 ) const;

	void					SetLinearVelocity( const arcVec3 &newLinearVelocity, int id = 0 );

	const arcVec3 &			GetLinearVelocity( int id = 0 ) const;

	void					ClipTranslation( trace_t &results, const arcVec3 &translation, const idClipModel *model ) const;
	void					ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const;
	int						ClipContents( const idClipModel *model ) const;

	void					DisableClip( void );
	void					EnableClip( void );

	void					UnlinkClip( void );
	void					LinkClip( void );

	void					SetPushed( int deltaTime );
	const arcVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;

	void					SetMaster( idEntity *master, const bool orientated );

	void					WriteToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const idBitMsgDelta &msg );

	idEntityPtr<idEntity>	extraPassEntity;

private:
	// state of the particle
	particlePState_t		current;
	particlePState_t		saved;

	// particle properties
	float					linearFriction;				// translational friction
	float					angularFriction;			// rotational friction
	float					contactFriction;			// friction with contact surfaces
	float					bouncyness;					// bouncyness
	bool					allowBounce;				// Allowed to bounce at all?
	idClipModel *			clipModel;					// clip model used for collision detection

	bool					dropToFloor;				// true if dropping to the floor and putting to rest
	bool					testSolid;					// true if testing for inside solid during a drop

	// master
	bool					hasMaster;
	bool					isOrientated;

private:

	void					DropToFloorAndRest	( void );
	bool					SlideMove			( arcVec3& start, arcVec3& velocity, const arcVec3& delta );
	void					CheckGround			( void );
	void					ApplyFriction		( float timeStep );
	void					DebugDraw			( void );
};

ARC_INLINE bool rvPhysics_Particle::IsOnGround ( void ) const {
	return current.onGround;
}

ARC_INLINE bool rvPhysics_Particle::IsInWater ( void ) const {
	return current.inWater;
}

ARC_INLINE bool rvPhysics_Particle::CanBounce ( void ) const {
	return allowBounce;
}

#endif /* !__PHYSICS_PARTICLE_H__ */
