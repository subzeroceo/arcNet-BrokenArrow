// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __PHYSICS_SIMPLERIGIDBODY_H__
#define __PHYSICS_SIMPLERIGIDBODY_H__

/*
===================================================================================

	"Simple" rigid body physics

	Mimics a rigid body, but isn't a rigid body at all - this is more akin
	to Q1/2/3 grenades than a rigid body, but outwardly seems very rigid-body-like

===================================================================================
*/

#include "Physics_Base.h"

typedef struct simpleRigidBodyPState_s {
	int						atRest;						// set when simulation is suspended
	float					lastTimeStep;				// length of last time step
	arcVec6					pushVelocity;				// push velocity

	// dynamic state
	arcVec3					position;					// position of trace model
	arcMat3					orientation;				// orientation of trace model
	arcVec3					linearVelocity;				// translational velocity relative to center of mass
	arcVec3					angularVelocity;			// rotational velocity relative to center of mass
} simpleRigidBodyPState_t;

class sdSimpleRigidBodyNetworkState : public sdEntityStateNetworkData {
public:
							sdSimpleRigidBodyNetworkState( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( arcNetFile* file ) const;
	virtual void			Read( arcNetFile* file );

	arcVec3					position;
	idCQuat 				orientation;
	arcVec3					linearVelocity;
	arcVec3					angularVelocity;
};

class sdSimpleRigidBodyBroadcastState : public sdEntityStateNetworkData {
public:
							sdSimpleRigidBodyBroadcastState( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( arcNetFile* file ) const;
	virtual void			Read( arcNetFile* file );

	arcVec3					localPosition;
	idCQuat					localOrientation;
	int						atRest;
	bool					orientedClip;
};

class arcPhysics_SimpleRigidBody : public arcPhysics_Base {

public:

	CLASS_PROTOTYPE( arcPhysics_SimpleRigidBody );

							arcPhysics_SimpleRigidBody( void );
							~arcPhysics_SimpleRigidBody( void );

							// initialisation
	void					SetFriction( const float linear, const float angular, const float contact ) { ; }
	void					SetWaterFriction( const float linear, const float angular );
	void					SetBouncyness( const float b );
	void					SetBouncyness( float normal, float tangential, float angular );
	void					SetStopSpeed( float stopSpeed );

	void					SetBuoyancy( float b );
							// same as above but drop to the floor first
	void					DropToFloor( void );
							// no contact determination and contact friction
	void					NoContact( void );
							// enable/disable activation by impact
	virtual void			EnableImpact( void );
	virtual void			DisableImpact( void );

public:	// common physics interface
	void					SetClipModel( arcClipModel *model, float density, int id = 0, bool freeOld = true );
	arcClipModel *			GetClipModel( int id = 0 ) const;
	int						GetNumClipModels( void ) const;

	void					SetMass( float mass, int id = -1 );
	float					GetMass( int id = -1 ) const;

	virtual const arcMat3&	GetInertiaTensor( int id = -1 ) const { return inertiaTensor; }

	void					SetContents( int contents, int id = -1 );
	int						GetContents( int id = -1 ) const;

	const arcBounds &		GetBounds( int id = -1 ) const;
	const arcBounds &		GetAbsBounds( int id = -1 ) const;

	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	void					GetImpactInfo( const int id, const arcVec3 &point, impactInfo_t *info ) const;
	void					ApplyImpulse( const int id, const arcVec3 &point, const arcVec3 &impulse );
	void					AddForce( const int id, const arcVec3 &point, const arcVec3 &force );
	void					Activate( void );
	void					PutToRest( void );
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

	virtual const arcVec3&	GetCenterOfMass() const { return centerOfMass; }

	void					SetLinearVelocity( const arcVec3 &newLinearVelocity, int id = 0 );
	void					SetAngularVelocity( const arcVec3 &newAngularVelocity, int id = 0 );

	const arcVec3 &			GetLinearVelocity( int id = 0 ) const;
	const arcVec3 &			GetAngularVelocity( int id = 0 ) const;

	void					ClipTranslation( trace_t &results, const arcVec3 &translation, const arcClipModel *model ) const;
	void					ClipRotation( trace_t &results, const idRotation &rotation, const arcClipModel *model ) const;
	int						ClipContents( const arcClipModel *model ) const;

	void					UnlinkClip( void );
	void					LinkClip( void );
	void					DisableClip( bool activateContacting = true );
	void					EnableClip( void );

	bool					EvaluateContacts( CLIP_DEBUG_PARMS_DECLARATION_ONLY );

	void					SetPushed( int deltaTime );
	const arcVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;
	const arcVec3 &			GetPushedAngularVelocity( const int id = 0 ) const;

	void					SetMaster( arcEntity *master, const bool orientated );

	virtual void			ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual bool			CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual void			WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual void			ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

	virtual void			DrawDebugInfo( void ) { DebugDraw(); }

	virtual float			InWater( void ) const { return waterLevel; }

	void					SetClipOriented( bool oriented ) { orientedClip = oriented; LinkClip(); }

private:
	void					CheckWater( void );

private:
	// state of the rigid body
	simpleRigidBodyPState_t		current;
	simpleRigidBodyPState_t		saved;
	arcVec3					localOrigin;				// origin relative to master
	arcMat3					localAxis;					// axis relative to master
	arcVec3					lastCollideNormal;

	// rigid body properties
	float					normalBouncyness;			// bouncyness normal to the surface
	float					tangentialBouncyness;		// bouncyness tangential to the surface
	float					stopSpeed;					// bounce speed at which it will stop moving
	float					angularBouncyness;			// scales the angular veocity on a bounce

	float					linearFrictionWater;		// translational friction when in water
	float					angularFrictionWater;		// rotational friction when in water

	float					buoyancy;
	arcClipModel *			clipModel;					// clip model used for collision detection
	arcClipModel	*			centeredClipModel;			// clip model at the center of mass

	// derived properties
	float					mass;						// mass of body
	float					inverseMass;				// 1 / mass
	arcVec3					centerOfMass;				// center of mass of trace model
	arcMat3					inertiaTensor;				// mass distribution
	arcMat3					inverseInertiaTensor;		// inverse inertia tensor

	bool					dropToFloor;				// true if dropping to the floor and putting to rest
	bool					testSolid;					// true if testing for solid when dropping to the floor
	bool					noImpact;					// if true do not activate when another object collides
	bool					noContact;					// if true do not determine contacts and no contact friction

	// master
	bool					hasMaster;
	bool					isOrientated;
	float					waterLevel;

	bool					orientedClip;

	const sdProgram::sdFunction* restFunc;

private:
	void					Integrate( const float deltaTime, simpleRigidBodyPState_t &next );
	bool					CheckForCollisions( const float deltaTime, simpleRigidBodyPState_t &next, trace_t &collision );
	bool					CollisionResponse( const trace_t &collision, arcVec3 &impulse );
	void					DropToFloorAndRest( void );
	bool					TestIfAtRest( void ) const;
	void					Rest( void );
	void					DebugDraw( void );
};

#endif /* !__PHYSICS_SIMPLERIGIDBODY_H__ */
