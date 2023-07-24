// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_PROJECTILE_H__
#define __GAME_PROJECTILE_H__

#include "physics/Physics_RigidBody.h"
#include "physics/Physics_SimpleRigidBody.h"
#include "physics/Physics_Parabola.h"
#include "Entity.h"
#include "proficiency/ProficiencyManager.h"
#include "effects/WaterEffects.h"

/*
===============================================================================

  idProjectile

===============================================================================
*/

extern const arcEventDef EV_DelayedLaunch;

class sdProjectileNetworkData : public sdEntityStateNetworkData {
public:
								sdProjectileNetworkData( void ) : physicsData( NULL ) { ; }
	virtual						~sdProjectileNetworkData( void );

	virtual void				MakeDefault( void );

	virtual void				Write( arcNetFile* file ) const;
	virtual void				Read( arcNetFile* file );

	sdEntityStateNetworkData*	physicsData;
	sdScriptObjectNetworkData	scriptData;
};

class sdProjectileBroadcastData : public sdEntityStateNetworkData {
public:
								sdProjectileBroadcastData( void ) : physicsData( NULL ), team( NULL ) { ; }
	virtual						~sdProjectileBroadcastData( void );

	virtual void				MakeDefault( void );

	virtual void				Write( arcNetFile* file ) const;
	virtual void				Read( arcNetFile* file );

	sdEntityStateNetworkData*	physicsData;
	sdScriptObjectNetworkData	scriptData;

	sdTeamInfo*					team;
	int							ownerId;
	int							enemyId;
	int							launchTime;
	float						launchSpeed;
	arcNetList< int >				owners;
	bool						hidden;
	sdBindNetworkData			bindData;
};

class idProjectile : public arcEntity {
public :
	CLASS_PROTOTYPE( idProjectile );

							idProjectile( void );
	virtual					~idProjectile( void );

	void					Spawn( void );

	virtual void			Create( arcEntity *owner, const arcVec3 &start, const arcVec3 &dir );
	virtual void			Launch( const arcVec3 &start, const arcVec3 &dir, const arcVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );

	virtual void			InitPhysics( void );
	virtual void			InitLaunchPhysics( float launchPower, const arcVec3& origin, const arcMat3& axes, const arcVec3& pushVelocity );
	void					LaunchDelayed( int delay, arcEntity* owner, const arcVec3& org, const arcVec3& dir, const arcVec3& push );

	arcEntity *				GetOwner( void ) const;
	virtual bool			IsOwner( arcEntity* other ) const;

	virtual void			Think( void );
	virtual void					PostThink( void );
	virtual idLinkList<arcEntity>*	GetPostThinkNode( void );

	virtual void			Killed( arcEntity *inflictor, arcEntity *attacker, int damage, const arcVec3 &dir, int location, const sdDeclDamage* damageDecl );
	virtual bool			Collide( const trace_t &collision, const arcVec3 &velocity, int bodyId );

	void					Thrust( void );
	void					UpdateTargeting( void );

	void					UpdateVisibility( void );

	virtual bool			DoRadiusPush( void ) const { return false; }

	virtual bool			CanCollide( const arcEntity* other, int traceId ) const;

	virtual void			SetEnemy( arcEntity* _enemy );
	virtual void			SetTarget( const arcVec3& target ) { }

	bool					SetState( const sdProgram::sdFunction* newState );
	void					UpdateScript( void );

	virtual void			SetHealth( int count ) { health = count; }
	virtual void			SetMaxHealth( int count ) { maxHealth = count; }
	virtual int				GetHealth( void ) const { return health; }
	virtual int				GetMaxHealth( void ) const { return maxHealth; }

	virtual void			SetGameTeam( sdTeamInfo* _team ) { team = _team; }
	virtual sdTeamInfo*		GetGameTeam( void ) const { return team; }

	virtual bool			ShouldConstructScriptObjectAtSpawn( void ) const { return false; }

	virtual void						ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void						ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void						WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

	virtual void			UpdateModelTransform( void );

	void					AddOwner( arcEntity* ent ) { assert( ent ); owners.AddUnique( ent ); }

	void					Event_Hide( void );
	void					Event_Show( void );
	void					Event_Freeze( bool frozen );

	void					SetThrust( bool value );
	void					SetLaunchTime( int time );

	virtual void			OnTouch( arcEntity *other, const trace_t& trace );

protected:
	arcEntityPtr< arcEntity >				owner;
	arcNetList< arcEntityPtr< arcEntity > >	owners;
	arcEntityPtr< arcEntity >				enemy;

	typedef struct projectileFlags_s {
		bool				scriptHide					: 1;
		bool				frozen						: 1;
		bool				allowOwnerCollisions		: 1;
		bool				hasThrust					: 1;
		bool				thustOn						: 1;
		bool				faceVelocity				: 1;
		bool				faceVelocitySet				: 1;
	} projectileFlags_t;

	projectileFlags_t		projectileFlags;

	float					damagePower;

	int						launchTime;
	int						thrustStartTime;
	int						targetForgetTime;
	float					launchSpeed;

	float					thrustPower;

	int						health;
	int						maxHealth;

	arcMat3					visualAxes;

	sdTeamInfo*				team;

	// state variables
	sdProgramThread*					baseScriptThread;
	const sdProgram::sdFunction*		scriptState;
	const sdProgram::sdFunction*		scriptIdealState;

	const sdProgram::sdFunction*		targetingFunction;

	const sdProgram::sdFunction*		onPostThink;
	idLinkList< arcEntity >				postThinkEntNode;

protected:

	void					Event_SetState( const char* stateName );
	void					Event_Fizzle( void );
	void					Event_DelayedLaunch( arcEntity* owner, const arcVec3 &org, arcVec3 const &dir, arcVec3 const &push );
	void					Event_GetDamagePower( void );
	void					Event_GetOwner( void );
	void					Event_SetOwner( arcEntity* _owner );
	void					Event_GetLaunchTime( void );
	void					Event_Launch( const arcVec3& velocity );
	void					Event_AddOwner( arcEntity* other );
	void					Event_SetEnemy( arcEntity* other );
	void					Event_IsOwner( arcEntity* other );
	void					Event_GetEnemy( void );
};

/*
===============================================================================

  idProjectile_RigidBody

===============================================================================
*/

class idProjectile_RigidBody : public idProjectile {
public :
	CLASS_PROTOTYPE( idProjectile_RigidBody );

							idProjectile_RigidBody( void );
	virtual ~idProjectile_RigidBody( void );

	virtual void			Spawn( void );
	virtual void			InitPhysics( void );
	virtual void			InitLaunchPhysics( float launchPower, const arcVec3& origin, const arcMat3& axes, const arcVec3& pushVelocity );

	virtual bool			StartSynced( void ) const { return true; }

	virtual void			CheckWater( const arcVec3& waterBodyOrg, const arcMat3& waterBodyAxis, arcCollisionModel* waterBodyModel );

protected:
	sdPhysics_SimpleRigidBody		physicsObj;
	sdWaterEffects			*waterEffects;
};

/*
===============================================================================

	sdProjectile_Parabolic

===============================================================================
*/

class sdProjectile_Parabolic : public idProjectile {
public :
	CLASS_PROTOTYPE( sdProjectile_Parabolic );

							sdProjectile_Parabolic( void );
	virtual					~sdProjectile_Parabolic( void );

	virtual void			Spawn( void );
	virtual void			InitPhysics( void );
	virtual void			InitLaunchPhysics( float launchPower, const arcVec3& origin, const arcMat3& axes, const arcVec3& pushVelocity );

	virtual bool			StartSynced( void ) const { return true; }

	virtual void			CheckWater( const arcVec3& waterBodyOrg, const arcMat3& waterBodyAxis, arcCollisionModel* waterBodyModel );

protected:
	sdPhysics_Parabola		physicsObj;
	sdWaterEffects			*waterEffects;
};

#endif /* !__GAME_PROJECTILE_H__ */
