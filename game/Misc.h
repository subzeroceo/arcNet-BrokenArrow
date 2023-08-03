#ifndef __GAME_MISC_H__
#define __GAME_MISC_H__

#include "ScriptEntity.h"
#include "physics/Force_Spring.h"
#include "physics/Force_Field.h"
#include "physics/Physics_Parametric.h"
#include "physics/Physics_StaticMulti.h"
#include "Trigger.h"
#include "AFEntity.h"

/*
===============================================================================

idSpawnableEntity

A simple, spawnable entity with a model and no functionable ability of it's own.
For example, it can be used as a placeholder during development, for marking
locations on maps for script, or for simple placed models without any behavior
that can be bound to other entities.  Should not be subclassed.
===============================================================================
*/

class arcSpawnableEntity : public arcEntity {
public:
	CLASS_PROTOTYPE( arcSpawnableEntity );

	void				Spawn( void );

private:
};

/*
===============================================================================

  Potential spawning position for players.

===============================================================================
*/

class arcPlayerStart : public arcEntity {
public:
	CLASS_PROTOTYPE( arcPlayerStart );

						arcPlayerStart( void );

	virtual void		PostMapSpawn( void );

	bool				ShouldConstructScriptObjectAtSpawn( void ) const { return false; }

	static bool			InhibitSpawn( const anDict& args );
};

/*
===============================================================================

  arcAnimated

===============================================================================
*/

class arcAnimated : public arcAFEntity_Base {
public:
	CLASS_PROTOTYPE( arcAnimated );

							arcAnimated();
							~arcAnimated();

	void					Spawn( void );
	virtual bool			LoadAF( void );
	bool					StartRagdoll( void );
	virtual bool			GetPhysicsToSoundTransform( anVec3 &origin, anMat3 &axis );

private:
	int						num_anims;
	int						current_anim_index;
	int						anim;
	int						blendFrames;
	jointHandle_t			soundJoint;
	arcEntityPtr<arcEntity>	activator;
	bool					activated;

	void					PlayNextAnim( void );

	void					Event_Activate( arcEntity *activator );
	void					Event_Start( void );
	void					Event_StartRagdoll( void );
	void					Event_AnimDone( int animIndex );
};


/*
===============================================================================

  StaticEntity

===============================================================================
*/

class arcStaticEntity : public arcEntity {
public:
	CLASS_PROTOTYPE( arcStaticEntity );

						arcStaticEntity( void );
	virtual				~iarcStaticEntity( void );

	void				Spawn( void );
	virtual void		Hide( void );
	virtual void		Show( void );
	virtual void		Think( void );

	static bool			InhibitSpawn( const anDict& args );

	virtual void		PostMapSpawn( void );

	void				UpdateThinkStatus( void );
};

/*
===============================================================================

	Entity Enviorment Definition

===============================================================================
*/

class arcEntityEnvDefinition : public arcEntity {
public:
	CLASS_PROTOTYPE( arcEntityEnvDefinition );

						arcEntityEnvDefinition( void ) { ; }
	void				Spawn( void );
};

/*
===============================================================================

	sdLiquid

===============================================================================
*/

class arcLiquid : public arcEntity {
public:
	CLASS_PROTOTYPE( arcLiquid );

										arcLiquid( void );
	void								Spawn( void );

	virtual void						GetWaterCurrent( anVec3 &waterCurrent ) const { waterCurrent = current; }

private:
	anVec3								current;
};

/*
===============================================================================

	arcLODEntity

===============================================================================
*/

class arcLODEntity : public arcEntity {
public:
	CLASS_PROTOTYPE( arcLODEntity );

								arcLODEntity();
	virtual						~arcLODEntity();

	void						Spawn();
	void						PostMapSpawn();

	void						AddClipModel( anClipModel* model, const anVec3& origin, const anMat3& axes );
	void						AddRenderEntity( const renderEntity_t& entity, int ID );

private:
	void						FreeModelDefs();

private:
	anList<int>				modelDefHandles;
	anList<int>				modelID;
	anPhysics_StaticMulti		physicsObj;
};

/*
===============================================================================

	sdEnvBound

===============================================================================
*/

class arcEnvBoundsEntity : public arcEntity {
public:
	CLASS_PROTOTYPE( arcEnvBoundsEntity );

						sdEnvBoundsEntity( void ) { ; }
	void				Spawn( void );
};


/*
===============================================================================

	sdLadderEntity

===============================================================================
*/

class arcLadderEntity : public arcEntity {
public:
	CLASS_PROTOTYPE( arcLadderEntity );

	void				Spawn( void );
	virtual void		Think( void );

	virtual				~arcLadderEntity( void );

	bool				IsActive( void ) const { return !fl.forceDisableClip; }
	anVec3				GetLadderNormal( void ) const;
	anClipModel *		GetLadderModel( void ) const { return ladderModel; }

private:
	anVec3				ladderNormal;
	anClipModel *		ladderModel;
};

/*
===============================================================================

	anSEASObstacleEntity

	Allows turning on and off AAS Obstacles

===============================================================================
*/

class anSEASObstacleEntity : public arcEntity {
public:
	CLASS_PROTOTYPE( anSEASObstacleEntity );

						anSEASObstacleEntity( void );

	void				Spawn( void );

	bool				IsEnabled( void ) { return enabled; }
	int					GetTeam( void ) { return team; }

private:
	void				Event_Activate( arcEntity *activator );

	void				ChangeAreaState();

	bool				enabled;
	int					team;
};

#endif // !__GAME_MISC_H__
