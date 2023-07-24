// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __PHYSICS_RIGIDBODYMULTIPLE_H__
#define __PHYSICS_RIGIDBODYMULTIPLE_H__

#include "Physics_RigidBody.h"

const int RBM_MAX_CONTACTS = 16;

class arcEntity;

typedef struct pointMass_s {
	arcVec3	origin;
	float	mass;
} pointMass_t;

const int MAX_CONSTRAINTS = 128;
typedef arcStaticList< constraintInfo_t, MAX_CONSTRAINTS > constraintList_t;

class arcRigidBodyMulti_Body {
public:
							arcRigidBodyMulti_Body( void );
							~arcRigidBodyMulti_Body( void );

	void					SetClipModel( arcClipModel* _clipModel, float density, int id, bool freeOld );
	arcClipModel*			GetClipModel( void ) const { return clipModel; }
	void					Init( void );
	void					Link( arcEntity* self, const rigidBodyPState_t& current );
	void					UnLink( void );
	void					SetFrictionAxis( const arcMat3& axis ) { frictionAxis = axis; }

	float					GetMass( void ) const { return mass; }
	float					GetInverseMass( void ) const { return inverseMass; }
	const arcVec3&			GetCenterOfMass( void ) const { return centerOfMass; }
	const arcMat3&			GetInertiaTensor( void ) const { return inertiaTensor; }
	const arcMat3&			GetInverseInertiaTensor( void ) const { return inverseInertiaTensor; }
	const arcVec3&			GetOffset( void ) const { return localOrigin; }
	int						GetClipMask( void ) const { return clipMask; }
	const arcMat3&			GetFrictionAxis( void ) { return frictionAxis; }
	const arcVec3&			GetContactFriction( void ) { return contactFriction; }
	float					GetBuoyancy( void ) { return buoyancy; }
	float					GetWaterDrag( void ) { return waterDrag; }

	void					SetOffset( const arcVec3& offset ) { localOrigin = offset; }
	void					SetMass( float _mass );
	void					SetClipMask( int _clipMask ) { clipMask = _clipMask; }
	void					DebugDrawMass( void );
	void					SetContactFriction( const arcVec3& value ) { contactFriction = value; }
	void					SetBuoyancy( float b ) { buoyancy = b; }
	void					SetWaterDrag( float d ) { waterDrag = d; }

	void					SetMainCenterOfMass( const arcVec3& com );
	const arcClipModel*		GetCenteredClipModel( void ) const { return clipModel ? centeredClipModel : NULL; }
	arcClipModel*			GetCenteredClipModel( void ) { return clipModel ? centeredClipModel : NULL; }

private:
	arcVec3					localOrigin;
	arcClipModel *			clipModel;				// clip model used for collision detection
	arcClipModel *			centeredClipModel;		// clip model at the center of mass of the entire object
	float					mass;					// mass of body
	float					inverseMass;			// 1 / mass
	arcVec3					centerOfMass;			// center of mass of trace model
	arcMat3					inertiaTensor;			// mass distribution
	arcMat3					inverseInertiaTensor;	// inverse inertia tensor
	int						clipMask;
	arcMat3					frictionAxis;
	arcVec3					contactFriction;
	float					contactSideFriction;
	float					buoyancy;
	float					waterDrag;
};

typedef struct {
	trace_t				trace;
	int					time;
	arcVec3				velocity;
} rbMultipleCollision_t;


class arcPhysics_RigidBodyMultiple : public arcPhysics_Base {
public:
	CLASS_PROTOTYPE( arcPhysics_RigidBodyMultiple );

							arcPhysics_RigidBodyMultiple( void );
							~arcPhysics_RigidBodyMultiple( void );

public:
	virtual void			SetClipModel( arcClipModel *model, float density, int id, bool freeOld = true );
	virtual arcClipModel*	GetClipModel( int id ) const;
	virtual int				GetNumClipModels( void ) const;
	virtual void			SetMass( float mass, int id );
	virtual float			GetMass( int id ) const;
	virtual const arcMat3&	GetInertiaTensor( int id = -1 ) const;
	virtual const arcVec3&	GetCenterOfMass( void ) const { return mainCenterOfMass; }
	virtual void			SetInertiaTensor( const arcMat3& itt );		// NOTE: use with EXTREME CAUTION
	virtual void			SetContents( int contents, int id );
	virtual int				GetContents( int id ) const;
	virtual const arcBounds&	GetBounds( int id ) const;
	virtual const arcBounds&	GetAbsBounds( int id ) const;
	virtual bool			Evaluate( int timeStepMSec, int endTimeMSec );
	virtual void			UpdateTime( int endTimeMSec );
	virtual void			GetImpactInfo( const int id, const arcVec3 &point, impactInfo_t *info ) const;
	virtual void			ApplyImpulse( const int id, const arcVec3 &point, const arcVec3 &impulse );
	virtual void			AddForce( const int id, const arcVec3 &point, const arcVec3 &force );
	virtual void			AddLocalForce( const int id, const arcVec3 &point, const arcVec3 &force );	// applies in local space
	virtual void			AddForce( const arcVec3& force );
	virtual void			AddTorque( const arcVec3& torque );
	virtual bool			IsAtRest( void ) const;
	virtual int				GetRestStartTime( void ) const;
	virtual bool			IsPushable( void ) const;
	virtual bool			EvaluateContacts( bool addEntityContacts );
	virtual void			SaveState( void );
	virtual void			RestoreState( void );
	virtual void			SetOrigin( const arcVec3 &newOrigin, int id = 0 );
	virtual void			SetAxis( const arcMat3 &newAxis, int id = 0 );
	virtual void			Translate( const arcVec3 &translation, int id = 0 );
	virtual void			Rotate( const idRotation &rotation, int id = 0 );
	virtual const arcVec3&	GetOrigin( int id = 0 ) const;
	virtual const arcMat3&	GetAxis( int id = 0 ) const;
	virtual void			SetLinearVelocity( const arcVec3 &newLinearVelocity, int id = -1 );
	virtual void			SetAngularVelocity( const arcVec3 &newAngularVelocity, int id = -1 );
	virtual const arcVec3&	GetLinearVelocity( int id = 0 ) const;
	virtual const arcVec3&	GetAngularVelocity( int id = 0 ) const;
	virtual void			ClipTranslation( trace_t &results, const arcVec3 &translation, const arcClipModel *model ) const;
	virtual void			ClipRotation( trace_t &results, const idRotation &rotation, const arcClipModel *model ) const;
	virtual int				ClipContents( const arcClipModel *model ) const;
	virtual void			SetMaster( arcEntity *master, const bool orientated );
	virtual void			SetPushed( int deltaTime );
	virtual const arcVec3&	GetPushedLinearVelocity( const int id ) const;
	virtual const arcVec3&	GetPushedAngularVelocity( const int id ) const;
	virtual void			DisableGravity( bool disable ) { flags.noGravity = disable; }

	virtual void			UnlinkClip( void );
	virtual void			LinkClip( void );
	virtual void			EnableClip( void );
	virtual void			DisableClip( bool activateContacting = true );

	virtual int				GetClipMask( int id ) const;
	virtual void			SetClipMask( int mask, int id );
	virtual void			DrawDebugInfo( void ) { DebugDraw(); }

	void					ApplyImpulse( const arcVec3& linearImpulse, const arcVec3& angularImpulse );

	void					CalculateMassProperties( void );
	void					SetBodyOffset( int id, const arcVec3& offset );
	void					SetBodyBuoyancy( int id, float buoyancy );
	void					SetBodyWaterDrag( int id, float drag );
	void					SetBouncyness( const float b );
	void					SetWaterRestThreshold( float threshold );
	void					SetFriction( const float linear, const float angular );
	void					SetWaterFriction( const float linear, const float angular );
	void					SetContactFriction( const int id, const arcVec3& contact );

	void					SetFrozen( bool _frozen );

	const arcVec3&			GetPointVelocity( const arcVec3& point, arcVec3& velocity ) const;

	const arcVec3&			GetLinearMomentum( void ) const { return current->i.linearMomentum; }
	const arcVec3&			GetAngularMomentum( void ) const { return current->i.angularMomentum; }
	const arcVec3&			GetExternalLinearForce( void ) const { return current->externalForce; }
	const arcVec3&			GetExternalAngularForce( void ) const { return current->externalTorque; }

	bool					SolveLCPConstraints( constraintInfo_t* constraints, int numConstraints, float deltaTime ) const;

	void					ContactFriction( float deltaTime, bool addEntityConstraints );
	bool					CheckForCollisions( trace_t &collision );
	bool					CheckForCollisions_Simple( trace_t &collision );
	bool					CheckForPlayerCollisions( float timeDelta, trace_t &collision, bool& noCollisionDamage );
	bool					CheckForPlayerCollisions_Simple( float timeDelta, trace_t &collision, bool& noCollisionDamage );
	bool					CollisionImpulse( const trace_t& collision, arcVec3& impulse, arcVec3& relativeVelocity, bool noCollisionDamage );
	void					Integrate( float deltaTime );
	bool					TestIfAtRest( void ) const;
	void					DebugDraw( void );

	void					Rest( int time );
	void					PutToRest( void );
	void					NoContact( void );
	virtual void			Activate( void );
	virtual void			EnableImpact( void );
	virtual void			DisableImpact( void );
	void					ClearClipModels( void );

	void					SetFastPath( bool enabled ) { flags.useFastPath = enabled; }
	virtual void			SetComeToRest( bool enabled ) { flags.comeToRest = enabled; }
	int						GetBodyContacts( const int id, const contactInfo_t** contacts, int maxContacts ) const;
	int						GetBodyGroundContacts( const int id, const contactInfo_t** _contacts, int maxContacts ) const;
	void					SetContactFrictionEpsilonScale( int id, float scale );

	float						GetMainMass( void ) const { return mainMass; }
	float						GetMainInverseMass( void ) const { return mainInverseMass; }
	const arcMat3&				GetMainInertiaTensor( void ) const { return mainInertiaTensor; }
	const arcMat3&				GetMainInverseInertiaTensor( void ) const { return mainInverseInertiaTensor; }
	const arcVec3&				GetMainCenterOfMass( void ) const { return mainCenterOfMass; }
	const rigidBodyPState_t&	GetCurrentState( void ) const { return *current; }
	float						GetLinearFriction( void ) const { return linearFriction; }
	float						GetAngularFriction( void ) const { return angularFriction; }
	float						GetLinearWaterFriction( void ) const { return linearFrictionWater; }
	float						GetAngularWaterFriction( void ) const { return angularFrictionWater; }
	void						GetBodyOrigin( arcVec3& org, int id ) const;
	const arcVec3&				GetBodyOffset( int id ) const;

	virtual float				InWater( void ) const { return waterLevel; }

	const rbMultipleCollision_t& GetLastCollision( void ) const { return lastCollision; }

	int							GetVPushClipMask() const;

private:
	const arcClipModel* 		CheckWater( void );

private:
	rigidBodyPState_t		state[2];
	rigidBodyPState_t *		current;
	rigidBodyPState_t *		next;
	rigidBodyPState_t		saved;
	arcVec3					localOrigin;				// origin relative to master
	arcMat3					localAxis;					// axis relative to master

	// rigid body properties
	float					linearFriction;				// translational friction
	float					angularFriction;			// rotational friction
	float					linearFrictionWater;		//
	float					angularFrictionWater;		//
	float					bouncyness;					// bouncyness
	float					waterRestThreshold;
	arcNetList< arcRigidBodyMulti_Body >	bodies;				// clip models used for collision detection
	contactInfoExt_t		contactInfoExt[ RBM_MAX_CONTACTS ];

	// derived properties
	float					mainMass;					// mass of all bodies
	float					mainInverseMass;			// 1 / mass
	arcVec3					mainCenterOfMass;			// center of mass of entire object
	arcBounds				mainBounds;
	arcBounds				totalBounds;
	int						mainClipMask;				// OR of clipmask for all bodies
	arcClipModel *			mainClipModel;
	arcClipModel *			centeredMainClipModel;

	int						activateEndTime;			//

	arcMat3					mainInertiaTensor;			// mass distribution
	arcMat3					mainInverseInertiaTensor;	// inverse inertia tensor
	bool					customInertiaTensor;

	idODE *					integrator;					// integrator
	idLCP*					lcp;						// lcp for contact force resolution

	typedef struct rbMultipleFlags_s {
		bool				testSolid		: 1;		// true if testing for solid when dropping to the floor
		bool				noImpact		: 1;		// if true do not activate when another object collides
		bool				noContact		: 1;		// if true do not determine contacts and no contact friction
		bool				isOrientated	: 1;
		bool				useFastPath		: 1;
		bool				comeToRest		: 1;
		bool				noGravity		: 1;
		bool				frozen			: 1;
	} rbMultipleFlags_t;

	rbMultipleFlags_t		flags;
	float					waterLevel;

	// master
	arcEntity *				masterEntity;

	rbMultipleCollision_t	lastCollision;
	int						blockedTime;

	void					SetupVPushCollection( void );
	arcClipModelCollection	vpushCollection;
};

#endif // __PHYSICS_RIGIDBODYMULTIPLE_H__

