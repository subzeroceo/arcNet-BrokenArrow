// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __PHYSICS_RIGIDBODYMULTIPLE_H__
#define __PHYSICS_RIGIDBODYMULTIPLE_H__

#include "Physics_RigidBody.h"

const int RBM_MAX_CONTACTS = 16;

class arcEntity;

typedef struct pointMass_s {
	anVec3	origin;
	float	mass;
} pointMass_t;

const int MAX_CONSTRAINTS = 128;
typedef arcStaticList< constraintInfo_t, MAX_CONSTRAINTS > constraintList_t;

class arcRigidBodyMulti_Body {
public:
							arcRigidBodyMulti_Body( void );
							~arcRigidBodyMulti_Body( void );

	void					SetClipModel( anClipModel* _clipModel, float density, int id, bool freeOld );
	anClipModel*			GetClipModel( void ) const { return clipModel; }
	void					Init( void );
	void					Link( arcEntity* self, const rigidBodyPState_t& current );
	void					UnLink( void );
	void					SetFrictionAxis( const anMat3& axis ) { frictionAxis = axis; }

	float					GetMass( void ) const { return mass; }
	float					GetInverseMass( void ) const { return inverseMass; }
	const anVec3&			GetCenterOfMass( void ) const { return centerOfMass; }
	const anMat3&			GetInertiaTensor( void ) const { return inertiaTensor; }
	const anMat3&			GetInverseInertiaTensor( void ) const { return inverseInertiaTensor; }
	const anVec3&			GetOffset( void ) const { return localOrigin; }
	int						GetClipMask( void ) const { return clipMask; }
	const anMat3&			GetFrictionAxis( void ) { return frictionAxis; }
	const anVec3&			GetContactFriction( void ) { return contactFriction; }
	float					GetBuoyancy( void ) { return buoyancy; }
	float					GetWaterDrag( void ) { return waterDrag; }

	void					SetOffset( const anVec3& offset ) { localOrigin = offset; }
	void					SetMass( float _mass );
	void					SetClipMask( int _clipMask ) { clipMask = _clipMask; }
	void					DebugDrawMass( void );
	void					SetContactFriction( const anVec3& value ) { contactFriction = value; }
	void					SetBuoyancy( float b ) { buoyancy = b; }
	void					SetWaterDrag( float d ) { waterDrag = d; }

	void					SetMainCenterOfMass( const anVec3& com );
	const anClipModel*		GetCenteredClipModel( void ) const { return clipModel ? centeredClipModel : nullptr; }
	anClipModel*			GetCenteredClipModel( void ) { return clipModel ? centeredClipModel : nullptr; }

private:
	anVec3					localOrigin;
	anClipModel *			clipModel;				// clip model used for collision detection
	anClipModel *			centeredClipModel;		// clip model at the center of mass of the entire object
	float					mass;					// mass of body
	float					inverseMass;			// 1 / mass
	anVec3					centerOfMass;			// center of mass of trace model
	anMat3					inertiaTensor;			// mass distribution
	anMat3					inverseInertiaTensor;	// inverse inertia tensor
	int						clipMask;
	anMat3					frictionAxis;
	anVec3					contactFriction;
	float					contactSideFriction;
	float					buoyancy;
	float					waterDrag;
};

typedef struct {
	trace_t				trace;
	int					time;
	anVec3				velocity;
} rbMultipleCollision_t;


class anPhysics_RigidBodyMultiple : public anPhysics_Base {
public:
	CLASS_PROTOTYPE( anPhysics_RigidBodyMultiple );

							anPhysics_RigidBodyMultiple( void );
							~anPhysics_RigidBodyMultiple( void );

public:
	virtual void			SetClipModel( anClipModel *model, float density, int id, bool freeOld = true );
	virtual anClipModel*	GetClipModel( int id ) const;
	virtual int				GetNumClipModels( void ) const;
	virtual void			SetMass( float mass, int id );
	virtual float			GetMass( int id ) const;
	virtual const anMat3&	GetInertiaTensor( int id = -1 ) const;
	virtual const anVec3&	GetCenterOfMass( void ) const { return mainCenterOfMass; }
	virtual void			SetInertiaTensor( const anMat3& itt );		// NOTE: use with EXTREME CAUTION
	virtual void			SetContents( int contents, int id );
	virtual int				GetContents( int id ) const;
	virtual const anBounds&	GetBounds( int id ) const;
	virtual const anBounds&	GetAbsBounds( int id ) const;
	virtual bool			Evaluate( int timeStepMSec, int endTimeMSec );
	virtual void			UpdateTime( int endTimeMSec );
	virtual void			GetImpactInfo( const int id, const anVec3 &point, impactInfo_t *info ) const;
	virtual void			ApplyImpulse( const int id, const anVec3 &point, const anVec3 &impulse );
	virtual void			AddForce( const int id, const anVec3 &point, const anVec3 &force );
	virtual void			AddLocalForce( const int id, const anVec3 &point, const anVec3 &force );	// applies in local space
	virtual void			AddForce( const anVec3& force );
	virtual void			AddTorque( const anVec3& torque );
	virtual bool			IsAtRest( void ) const;
	virtual int				GetRestStartTime( void ) const;
	virtual bool			IsPushable( void ) const;
	virtual bool			EvaluateContacts( bool addEntityContacts );
	virtual void			SaveState( void );
	virtual void			RestoreState( void );
	virtual void			SetOrigin( const anVec3 &newOrigin, int id = 0 );
	virtual void			SetAxis( const anMat3 &newAxis, int id = 0 );
	virtual void			Translate( const anVec3 &translation, int id = 0 );
	virtual void			Rotate( const anRotation &rotation, int id = 0 );
	virtual const anVec3&	GetOrigin( int id = 0 ) const;
	virtual const anMat3&	GetAxis( int id = 0 ) const;
	virtual void			SetLinearVelocity( const anVec3 &newLinearVelocity, int id = -1 );
	virtual void			SetAngularVelocity( const anVec3 &newAngularVelocity, int id = -1 );
	virtual const anVec3&	GetLinearVelocity( int id = 0 ) const;
	virtual const anVec3&	GetAngularVelocity( int id = 0 ) const;
	virtual void			ClipTranslation( trace_t &results, const anVec3 &translation, const anClipModel *model ) const;
	virtual void			ClipRotation( trace_t &results, const anRotation &rotation, const anClipModel *model ) const;
	virtual int				ClipContents( const anClipModel *model ) const;
	virtual void			SetMaster( arcEntity *master, const bool orientated );
	virtual void			SetPushed( int deltaTime );
	virtual const anVec3&	GetPushedLinearVelocity( const int id ) const;
	virtual const anVec3&	GetPushedAngularVelocity( const int id ) const;
	virtual void			DisableGravity( bool disable ) { flags.noGravity = disable; }

	virtual void			UnlinkClip( void );
	virtual void			LinkClip( void );
	virtual void			EnableClip( void );
	virtual void			DisableClip( bool activateContacting = true );

	virtual int				GetClipMask( int id ) const;
	virtual void			SetClipMask( int mask, int id );
	virtual void			DrawDebugInfo( void ) { DebugDraw(); }

	void					ApplyImpulse( const anVec3& linearImpulse, const anVec3& angularImpulse );

	void					CalculateMassProperties( void );
	void					SetBodyOffset( int id, const anVec3& offset );
	void					SetBodyBuoyancy( int id, float buoyancy );
	void					SetBodyWaterDrag( int id, float drag );
	void					SetBouncyness( const float b );
	void					SetWaterRestThreshold( float threshold );
	void					SetFriction( const float linear, const float angular );
	void					SetWaterFriction( const float linear, const float angular );
	void					SetContactFriction( const int id, const anVec3& contact );

	void					SetFrozen( bool _frozen );

	const anVec3&			GetPointVelocity( const anVec3& point, anVec3& velocity ) const;

	const anVec3&			GetLinearMomentum( void ) const { return current->i.linearMomentum; }
	const anVec3&			GetAngularMomentum( void ) const { return current->i.angularMomentum; }
	const anVec3&			GetExternalLinearForce( void ) const { return current->externalForce; }
	const anVec3&			GetExternalAngularForce( void ) const { return current->externalTorque; }

	bool					SolveLCPConstraints( constraintInfo_t *constraints, int numConstraints, float deltaTime ) const;

	void					ContactFriction( float deltaTime, bool addEntityConstraints );
	bool					CheckForCollisions( trace_t &collision );
	bool					CheckForCollisions_Simple( trace_t &collision );
	bool					CheckForPlayerCollisions( float timeDelta, trace_t &collision, bool& noCollisionDamage );
	bool					CheckForPlayerCollisions_Simple( float timeDelta, trace_t &collision, bool& noCollisionDamage );
	bool					CollisionImpulse( const trace_t& collision, anVec3& impulse, anVec3& relativeVelocity, bool noCollisionDamage );
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
	const anMat3&				GetMainInertiaTensor( void ) const { return mainInertiaTensor; }
	const anMat3&				GetMainInverseInertiaTensor( void ) const { return mainInverseInertiaTensor; }
	const anVec3&				GetMainCenterOfMass( void ) const { return mainCenterOfMass; }
	const rigidBodyPState_t&	GetCurrentState( void ) const { return *current; }
	float						GetLinearFriction( void ) const { return linearFriction; }
	float						GetAngularFriction( void ) const { return angularFriction; }
	float						GetLinearWaterFriction( void ) const { return linearFrictionWater; }
	float						GetAngularWaterFriction( void ) const { return angularFrictionWater; }
	void						GetBodyOrigin( anVec3& org, int id ) const;
	const anVec3&				GetBodyOffset( int id ) const;

	virtual float				InWater( void ) const { return waterLevel; }

	const rbMultipleCollision_t& GetLastCollision( void ) const { return lastCollision; }

	int							GetVPushClipMask() const;

private:
	const anClipModel* 		CheckWater( void );

private:
	rigidBodyPState_t		state[2];
	rigidBodyPState_t *		current;
	rigidBodyPState_t *		next;
	rigidBodyPState_t		saved;
	anVec3					localOrigin;				// origin relative to master
	anMat3					localAxis;					// axis relative to master

	// rigid body properties
	float					linearFriction;				// translational friction
	float					angularFriction;			// rotational friction
	float					linearFrictionWater;		//
	float					angularFrictionWater;		//
	float					bouncyness;					// bouncyness
	float					waterRestThreshold;
	anList< arcRigidBodyMulti_Body >	bodies;				// clip models used for collision detection
	contactInfoExt_t		contactInfoExt[ RBM_MAX_CONTACTS ];

	// derived properties
	float					mainMass;					// mass of all bodies
	float					mainInverseMass;			// 1 / mass
	anVec3					mainCenterOfMass;			// center of mass of entire object
	anBounds				mainBounds;
	anBounds				totalBounds;
	int						mainClipMask;				// OR of clipmask for all bodies
	anClipModel *			mainClipModel;
	anClipModel *			centeredMainClipModel;

	int						activateEndTime;			//

	anMat3					mainInertiaTensor;			// mass distribution
	anMat3					mainInverseInertiaTensor;	// inverse inertia tensor
	bool					customInertiaTensor;

	anODE *					integrator;					// integrator
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
