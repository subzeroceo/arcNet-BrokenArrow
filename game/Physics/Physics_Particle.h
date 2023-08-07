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
	anVec3					localOrigin;				// origin relative to master
	anMat3					localAxis;					// axis relative to master
	anVec3					pushVelocity;				// push velocity
	anVec3					origin;
	anVec3					velocity;
	bool					onGround;
	bool					inWater;
} particlePState_t;

class anPhysics_Particle : public anPhysics_Base {

public:

	CLASS_PROTOTYPE( anPhysics_Particle );

							anPhysics_Particle( void );
							~anPhysics_Particle( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

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
	void					SetClipModel( anClipModel *model, float density, int id = 0, bool freeOld = true );
	anClipModel *			GetClipModel( int id = 0 ) const;
	int						GetNumClipModels( void ) const;

	void					SetContents( int contents, int id = -1 );
	int						GetContents( int id = -1 ) const;

	const anBounds &		GetBounds( int id = -1 ) const;
	const anBounds &		GetAbsBounds( int id = -1 ) const;

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

	void					SetOrigin( const anVec3 &newOrigin, int id = -1 );
	void					SetAxis( const anMat3 &newAxis, int id = -1 );

	void					Translate( const anVec3 &translation, int id = -1 );
	void					Rotate( const anRotation &rotation, int id = -1 );

	const anVec3 &			GetOrigin( int id = 0 ) const;
	const anMat3 &			GetAxis( int id = 0 ) const;

	void					SetLinearVelocity( const anVec3 &newLinearVelocity, int id = 0 );

	const anVec3 &			GetLinearVelocity( int id = 0 ) const;

	void					ClipTranslation( trace_t &results, const anVec3 &translation, const anClipModel *model ) const;
	void					ClipRotation( trace_t &results, const anRotation &rotation, const anClipModel *model ) const;
	int						ClipContents( const anClipModel *model ) const;

	void					DisableClip( void );
	void					EnableClip( void );

	void					UnlinkClip( void );
	void					LinkClip( void );

	void					SetPushed( int deltaTime );
	const anVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;

	void					SetMaster( anEntity *master, const bool orientated );

	void					WriteToSnapshot( anBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const anBitMsgDelta &msg );

	anEntityPtr<anEntity>	extraPassEntity;

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
	anClipModel *			clipModel;					// clip model used for collision detection

	bool					dropToFloor;				// true if dropping to the floor and putting to rest
	bool					testSolid;					// true if testing for inside solid during a drop

	// master
	bool					hasMaster;
	bool					isOrientated;

private:

	void					DropToFloorAndRest	( void );
	bool					SlideMove			( anVec3 &start, anVec3 &velocity, const anVec3 &delta );
	void					CheckGround			( void );
	void					ApplyFriction		( float timeStep );
	void					DebugDraw			( void );
};

inline bool anPhysics_Particle::IsOnGround ( void ) const {
	return current.onGround;
}

inline bool anPhysics_Particle::IsInWater ( void ) const {
	return current.inWater;
}

inline bool anPhysics_Particle::CanBounce ( void ) const {
	return allowBounce;
}

#endif /* !__PHYSICS_PARTICLE_H__ */
