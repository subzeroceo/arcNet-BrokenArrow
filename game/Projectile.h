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

extern const anEventDef EV_DelayedLaunch;

class sdProjectileNetworkData : public sdEntityStateNetworkData {
public:
								sdProjectileNetworkData( void ) : physicsData( nullptr ) { ; }
	virtual						~sdProjectileNetworkData( void );

	virtual void				MakeDefault( void );

	virtual void				Write( anFile *file ) const;
	virtual void				Read( anFile *file );

	sdEntityStateNetworkData*	physicsData;
	sdScriptObjectNetworkData	scriptData;
};

class sdProjectileBroadcastData : public sdEntityStateNetworkData {
public:
								sdProjectileBroadcastData( void ) : physicsData( nullptr ), team( nullptr ) { ; }
	virtual						~sdProjectileBroadcastData( void );

	virtual void				MakeDefault( void );

	virtual void				Write( anFile *file ) const;
	virtual void				Read( anFile *file );

	sdEntityStateNetworkData*	physicsData;
	sdScriptObjectNetworkData	scriptData;

	sdTeamInfo*					team;
	int							ownerId;
	int							enemyId;
	int							launchTime;
	float						launchSpeed;
	anList< int >				owners;
	bool						hidden;
	sdBindNetworkData			bindData;
};

class idProjectile : public anEntity {
public :
	CLASS_PROTOTYPE( idProjectile );

							idProjectile( void );
	virtual					~idProjectile( void );

	void					Spawn( void );

	virtual void			Create( anEntity *owner, const anVec3 &start, const anVec3 &dir );
	virtual void			Launch( const anVec3 &start, const anVec3 &dir, const anVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );

	virtual void			InitPhysics( void );
	virtual void			InitLaunchPhysics( float launchPower, const anVec3 &origin, const anMat3 &axes, const anVec3 &pushVelocity );
	void					LaunchDelayed( int delay, anEntity *owner, const anVec3 &org, const anVec3 &dir, const anVec3 &push );

	anEntity *				GetOwner( void ) const;
	virtual bool			IsOwner( anEntity *other ) const;

	virtual void			Think( void );
	virtual void					PostThink( void );
	virtual anLinkList<anEntity>*	GetPostThinkNode( void );

	virtual void			Killed( anEntity *inflictor, anEntity *attacker, int damage, const anVec3 &dir, int location, const sdDeclDamage* damageDecl );
	virtual bool			Collide( const trace_t &collision, const anVec3 &velocity, int bodyId );

	void					Thrust( void );
	void					UpdateTargeting( void );

	void					UpdateVisibility( void );

	virtual bool			DoRadiusPush( void ) const { return false; }

	virtual bool			CanCollide( const anEntity *other, int traceId ) const;

	virtual void			SetEnemy( anEntity *_enemy );
	virtual void			SetTarget( const anVec3 &target ) { }

	bool					SetState( const idProgram::sdFunction* newState );
	void					UpdateScript( void );

	virtual void			SetHealth( int count ) { health = count; }
	virtual void			SetMaxHealth( int count ) { maxHealth = count; }
	virtual int				GetHealth( void ) const { return health; }
	virtual int				GetMaxHealth( void ) const { return maxHealth; }

	virtual void			SetGameTeam( sdTeamInfo* _team ) { team = _team; }
	virtual sdTeamInfo*		GetGameTeam( void ) const { return team; }

	virtual bool			ShouldConstructScriptObjectAtSpawn( void ) const { return false; }

	virtual void						ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void						ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const anBitMsg& msg ) const;
	virtual void						WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, anBitMsg& msg ) const;
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

	virtual void			UpdateModelTransform( void );

	void					AddOwner( anEntity *ent ) { assert( ent ); owners.AddUnique( ent ); }

	void					Event_Hide( void );
	void					Event_Show( void );
	void					Event_Freeze( bool frozen );

	void					SetThrust( bool value );
	void					SetLaunchTime( int time );

	virtual void			OnTouch( anEntity *other, const trace_t& trace );

protected:
	anEntityPtr< anEntity >				owner;
	anList< anEntityPtr< anEntity > >	owners;
	anEntityPtr< anEntity >				enemy;

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

	anMat3					visualAxes;

	sdTeamInfo*				team;

	// state variables
	idProgramThread*					baseScriptThread;
	const idProgram::sdFunction*		scriptState;
	const idProgram::sdFunction*		scriptIdealState;

	const idProgram::sdFunction*		targetingFunction;

	const idProgram::sdFunction*		onPostThink;
	anLinkList< anEntity >				postThinkEntNode;

protected:

	void					Event_SetState( const char *stateName );
	void					Event_Fizzle( void );
	void					Event_DelayedLaunch( anEntity *owner, const anVec3 &org, anVec3 const &dir, anVec3 const &push );
	void					Event_GetDamagePower( void );
	void					Event_GetOwner( void );
	void					Event_SetOwner( anEntity *_owner );
	void					Event_GetLaunchTime( void );
	void					Event_Launch( const anVec3 &velocity );
	void					Event_AddOwner( anEntity *other );
	void					Event_SetEnemy( anEntity *other );
	void					Event_IsOwner( anEntity *other );
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
	virtual void			InitLaunchPhysics( float launchPower, const anVec3 &origin, const anMat3 &axes, const anVec3 &pushVelocity );

	virtual bool			StartSynced( void ) const { return true; }

	virtual void			CheckWater( const anVec3 &waterBodyOrg, const anMat3 &waterBodyAxis, anCollisionModel* waterBodyModel );

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
	virtual void			InitLaunchPhysics( float launchPower, const anVec3 &origin, const anMat3 &axes, const anVec3 &pushVelocity );

	virtual bool			StartSynced( void ) const { return true; }

	virtual void			CheckWater( const anVec3 &waterBodyOrg, const anMat3 &waterBodyAxis, anCollisionModel* waterBodyModel );

protected:
	sdPhysics_Parabola		physicsObj;
	sdWaterEffects			*waterEffects;
};

#endif /* !__GAME_PROJECTILE_H__ */
