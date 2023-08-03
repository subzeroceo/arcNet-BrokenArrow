
#ifndef __PHYSICS_RIGIDBODY_H__
#define __PHYSICS_RIGIDBODY_H__

/*
===================================================================================

	Rigid body physics

	Employs an impulse based dynamic simulation which is not very accurate but
	relatively fast and still reliable due to the continuous collision detection.

===================================================================================
*/

typedef struct rididBodyIState_s {
	anVec3					position;					// position of trace model
	anMat3					orientation;				// orientation of trace model
	anVec3					linearMomentum;				// translational momentum relative to center of mass
	anVec3					angularMomentum;			// rotational momentum relative to center of mass
} rigidBodyIState_t;

typedef struct rigidBodyPState_s {
	int						atRest;						// set when simulation is suspended
	float					lastTimeStep;				// length of last time step
	anVec3					localOrigin;				// origin relative to master
	anMat3					localAxis;					// axis relative to master
	anVec6					pushVelocity;				// push velocity
	anVec3					externalForce;				// external force relative to center of mass
	anVec3					externalTorque;				// external torque relative to center of mass
	rigidBodyIState_t		i;							// state used for integration
} rigidBodyPState_t;

class anPhysics_RigidBody : public anPhysics_Base {

public:

	CLASS_PROTOTYPE( anPhysics_RigidBody );

							anPhysics_RigidBody( void );
							~anPhysics_RigidBody( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

							// initialisation
	void					SetFriction( const float linear, const float angular, const float contact );
	void					SetBouncyness( const float b );
							// same as above but drop to the floor first
	void					DropToFloor( void );
							// no contact determination and contact friction
	void					NoContact( void );
							// enable/disable activation by impact
	void					EnableImpact( void );
	void					DisableImpact( void );

public:	// common physics interface
	void					SetClipModel( anClipModel *model, float density, int id = 0, bool freeOld = true );
	anClipModel *			GetClipModel( int id = 0 ) const;
	int						GetNumClipModels( void ) const;

	void					SetMass( float mass, int id = -1 );
	float					GetMass( int id = -1 ) const;


// bdube: means of getting center of mass
	anVec3					GetCenterMass ( int id = -1 ) const;


	void					SetContents( int contents, int id = -1 );
	int						GetContents( int id = -1 ) const;

	const anBounds &		GetBounds( int id = -1 ) const;
	const anBounds &		GetAbsBounds( int id = -1 ) const;

	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	void					GetImpactInfo( const int id, const anVec3 &point, impactInfo_t *info ) const;
	void					ApplyImpulse( const int id, const anVec3 &point, const anVec3 &impulse );
	void					AddForce( const int id, const anVec3 &point, const anVec3 &force );
	void					Activate( void );
	void					PutToRest( void );
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
	void					SetAngularVelocity( const anVec3 &newAngularVelocity, int id = 0 );

	const anVec3 &			GetLinearVelocity( int id = 0 ) const;
	const anVec3 &			GetAngularVelocity( int id = 0 ) const;

	void					ClipTranslation( trace_t &results, const anVec3 &translation, const anClipModel *model ) const;
	void					ClipRotation( trace_t &results, const anRotation &rotation, const anClipModel *model ) const;
	int						ClipContents( const anClipModel *model ) const;

	void					DisableClip( void );
	void					EnableClip( void );

	void					UnlinkClip( void );
	void					LinkClip( void );

	bool					EvaluateContacts( void );

	void					SetPushed( int deltaTime );
	const anVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;
	const anVec3 &			GetPushedAngularVelocity( const int id = 0 ) const;

	void					SetMaster( anEntity *master, const bool orientated );

	void					WriteToSnapshot( anBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const anBitMsgDelta &msg );

protected:

	// state of the rigid body
	rigidBodyPState_t		current;
	rigidBodyPState_t		saved;

private:

	// rigid body properties
	float					linearFriction;				// translational friction
	float					angularFriction;			// rotational friction
	float					contactFriction;			// friction with contact surfaces
	float					bouncyness;					// bouncyness
	anClipModel *			clipModel;					// clip model used for collision detection

	// derived properties
	float					mass;						// mass of body
	float					inverseMass;				// 1 / mass
	anVec3					centerOfMass;				// center of mass of trace model
	anMat3					inertiaTensor;				// mass distribution
	anMat3					inverseInertiaTensor;		// inverse inertia tensor

	anODE *					integrator;					// integrator
	bool					dropToFloor;				// true if dropping to the floor and putting to rest
	bool					testSolid;					// true if testing for solid when dropping to the floor
	bool					noImpact;					// if true do not activate when another object collides
	bool					noContact;					// if true do not determine contacts and no contact friction

	// master
	bool					hasMaster;
	bool					isOrientated;

private:
	friend void				RigidBodyDerivatives( const float t, const void *clientData, const float *state, float *derivatives );
	void					Integrate( const float deltaTime, rigidBodyPState_t &next );
	bool					CheckForCollisions( const float deltaTime, rigidBodyPState_t &next, trace_t &collision );
	bool					CollisionImpulse( const trace_t &collision, anVec3 &impulse );
	void					ContactFriction( float deltaTime );
	void					DropToFloorAndRest( void );
	bool					TestIfAtRest( void ) const;
	void					Rest( void );
	void					DebugDraw( void );
};

#endif // !__PHYSICS_RIGIDBODY_H__
