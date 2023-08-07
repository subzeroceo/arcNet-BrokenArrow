
#ifndef __GAME_MISC_H__
#define __GAME_MISC_H__

/*
===============================================================================

idSpawnableEntity

A simple, spawnable entity with a model and no functionable ability of it's own.
For example, it can be used as a placeholder during development, for marking
locations on maps for script, or for simple placed models without any behavior
that can be bound to other entities.  Should not be subclassed.
===============================================================================
*/

class idSpawnableEntity : public anEntity {
public:
	CLASS_PROTOTYPE( idSpawnableEntity );

	void				Spawn( void );

private:
};

/*
===============================================================================

  Potential spawning position for players.
  The first time a player enters the game, they will be at an 'initial' spot.
  Targets will be fired when someone spawns in on them.

  When triggered, will cause player to be teleported to spawn spot.

===============================================================================
*/

class anBasePlayerStart : public anEntity {
public:
	CLASS_PROTOTYPE( anBasePlayerStart );

	enum {
		EVENT_TELEPORTPLAYER = anEntity::EVENT_MAXEVENTS,
		EVENT_TELEPORTITEM,
		EVENT_MAXEVENTS
	};

						anBasePlayerStart( void );

	void				Spawn( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

	virtual bool		ClientReceiveEvent( int event, int time, const anBitMsg &msg );

private:
	int					teleportStage;

	void				Event_TeleportEntity( anEntity *activator, bool predicted, anVec3 &prevOrigin = vec3_origin );
	void				Event_Teleport( anEntity *activator );
	void				Teleport( anEntity *other );
	void				Event_TeleportStage( anBasePlayer *player );
	void				Event_ResetCamera( anBasePlayer *player );
	void				TeleportPlayer( anBasePlayer *player );
};


/*
===============================================================================

  Non-displayed entity used to activate triggers when it touches them.
  Bind to a mover to have the mover activate a trigger as it moves.
  When target by triggers, activating the trigger will toggle the
  activator on and off. Check "start_off" to have it spawn disabled.

===============================================================================
*/

class idActivator : public anEntity {
public:
	CLASS_PROTOTYPE( idActivator );

	void				Spawn( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

	virtual void		Think( void );

private:
	bool				stay_on;

	void				Event_Activate( anEntity *activator );
};

/*
===============================================================================

  Path entities for monsters to follow.

===============================================================================
*/
class idPathCorner : public anEntity {
public:
	CLASS_PROTOTYPE( idPathCorner );

	void				Spawn( void );

	static void			DrawDebugInfo( void );

	static idPathCorner *RandomPath( const anEntity *source, const anEntity *ignore );

private:
	void				Event_RandomPath( void );
};


// bdube: jump points
/*
===============================================================================

  Debug Jump Point

===============================================================================
*/

class rvDebugJumpPoint : public anEntity {
public:

	CLASS_PROTOTYPE( rvDebugJumpPoint );

	void				Spawn();
};

/*
===============================================================================

  Object that fires targets and changes shader parms when damaged.

===============================================================================
*/

class idDamagable : public anEntity {
public:
	CLASS_PROTOTYPE( idDamagable );

						idDamagable( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

	void				Spawn( void );
	void				Killed( anEntity *inflictor, anEntity *attacker, int damage, const anVec3 &dir, int location );


// abahr:
	virtual void		Damage( anEntity *inflictor, anEntity *attacker, const anVec3 &dir, const char *damageDefName, const float damageScale, const int location );

	int					invincibleTime;

// abahr: changed to protected
protected:
	int					stage;
	int					stageNext;
	const anDict*		stageDict;
	int					stageEndTime;
	int					stageEndHealth;
	int					stageEndSpeed;
//jshepard: used to end a stage if a moveable is on the ground (for falling objects)
	bool				stageEndOnGround;
//jshepard: we want to activate certain objects when triggered-- falling blocks yes, barrels no.
	bool				activateStageOnTrigger;

	virtual void		ExecuteStage	( void );
	void				UpdateStage		( void );
	anVec3				GetStageVector	( const char *key, const char *defaultString = "" ) const;
	float				GetStageFloat	( const char *key, const char *defaultString = "" ) const;
	int					GetStageInt		( const char *key, const char *defaultString = "" ) const;


	int					count;
	int					nextTriggerTime;

	void				BecomeBroken( anEntity *activator );
	void				Event_BecomeBroken( anEntity *activator );
	void				Event_RestoreDamagable( void );
};

/*
===============================================================================

  Hidden object that explodes when activated

===============================================================================
*/

class idExplodable : public anEntity {
public:
	CLASS_PROTOTYPE( idExplodable );

	void				Spawn( void );

private:
	void				Event_Explode( anEntity *activator );
};


/*
===============================================================================

  idSpring

===============================================================================
*/

class idSpring : public anEntity {
public:
	CLASS_PROTOTYPE( idSpring );

	void				Spawn( void );
	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

	virtual void		Think( void );

private:
	anEntityPtr<anEntity>	ent1;
	anEntityPtr<anEntity>	ent2;
	int						id1;
	int						id2;
	anVec3					p1;
	anVec3					p2;
	anForce_Spring			spring;

	void				Event_LinkSpring( void );
};


/*
===============================================================================

  anForceField

===============================================================================
*/

class anForceField : public anEntity {
public:
	CLASS_PROTOTYPE( anForceField );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

	void				Spawn( void );

	virtual void		Think( void );


// kfuller: idDamagable may want to change some things on the fly
	void				SetExplosion(float force) { forceField.Explosion(force); }




// bdube: made force field protected
protected:

	anForce_Field		forceField;

private:

	void				Toggle( void );

	void				Event_Activate( anEntity *activator );
	void				Event_Toggle( void );
	void				Event_FindTargets( void );
};


// bdube: jump pads
/*
===============================================================================

  rvJumpPad

===============================================================================
*/

class rvJumpPad : public anForceField {
public:
	CLASS_PROTOTYPE( rvJumpPad );

	rvJumpPad ( void );

	void				Spawn( void );
	void				Think( void );

private:

	int					lastEffectTime;

	void				Event_FindTargets( void );

	enum {
		EVENT_JUMPFX = anEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};
	bool				ClientReceiveEvent( int event, int time, const anBitMsg &msg );

	anMat3				effectAxis;
};


/*
===============================================================================

  idAnimated

===============================================================================
*/

class idAnimated : public idAFEntity_Gibbable {
public:
	CLASS_PROTOTYPE( idAnimated );

							idAnimated();
							~idAnimated();

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	void					Spawn( void );
	virtual bool			LoadAF( const char *keyname );
	bool					StartRagdoll( void );
	virtual bool			GetPhysicsToSoundTransform( anVec3 &origin, anMat3 &axis );


// bdube: script
	void					Think ( void );

	virtual	void			Damage( anEntity *inflictor, anEntity *attacker, const anVec3 &dir, const char *damageDefName, const float damageScale, const int location );
 	virtual bool			ShouldConstructScriptObjectAtSpawn( void ) const;


private:
	int						numAnims;
	int						currentAnimIndex;
	int						anim;
	int						blendFrames;
	jointHandle_t			soundJoint;
	anEntityPtr<anEntity>	activator;
	bool					activated;


// bdube: script variables
	// script control
	anThread *				scriptThread;
	anStr					state;
	anStr					idealState;
	int						animDoneTime[ANIM_NumAnimChannels];

	// Script state management
	void					UpdateScript( void );
	void					SetState( const char *statename, int blend );
	void					CallHandler ( const char *handler );


	void					PlayNextAnim( void );

	void					Event_Activate( anEntity *activator );
	void					Event_Start( void );
	void					Event_StartRagdoll( void );
	void					Event_AnimDone( int animIndex );
	void					Event_Footstep( void );
	void					Event_LaunchMissiles( const char *projectilename, const char *sound, const char *launchjoint, const char *targetjoint, int numshots, int framedelay );
	void					Event_LaunchMissilesUpdate( int launchjoint, int targetjoint, int numshots, int framedelay );


// kfuller: added
	void					Event_SetAnimState( const char *state, int blendframes );
	void					Event_PlayAnim( int channel, const char *animname );
	void					Event_PlayCycle( int channel, const char *animname );
	void					Event_AnimDone2( int channel, int blendFrames );

};

/*
===============================================================================

  idStaticEntity

===============================================================================
*/

class idStaticEntity : public anEntity {
public:
	CLASS_PROTOTYPE( idStaticEntity );

						idStaticEntity( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

	void				Spawn( void );
	void				ShowEditingDialog( void );
	virtual void		Hide( void );
	virtual void		Show( void );
	void				Fade( const anVec4 &to, float fadeTime );
	virtual void		Think( void );

	virtual void		WriteToSnapshot( anBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const anBitMsgDelta &msg );

private:
	void				Event_Activate( anEntity *activator );

	int					spawnTime;
	bool				active;
	anVec4				fadeFrom;
	anVec4				fadeTo;
	int					fadeStart;
	int					fadeEnd;
	bool				runGui;
};


/*
===============================================================================

idFuncEmitter

===============================================================================
*/

class idFuncEmitter : public idStaticEntity {
public:
	CLASS_PROTOTYPE( idFuncEmitter );

						idFuncEmitter( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

	void				Spawn( void );
	void				Event_Activate( anEntity *activator );

	virtual void		WriteToSnapshot( anBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const anBitMsgDelta &msg );

private:
	bool				hidden;

};



// bdube: not using
#if 0


/*
===============================================================================

idFuncSmoke

===============================================================================
*/

class idFuncSmoke : public anEntity {
public:
	CLASS_PROTOTYPE( idFuncSmoke );

							idFuncSmoke();

	void					Spawn( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	virtual void			Think( void );
	void					Event_Activate( anEntity *activator );

private:
	int						smokeTime;
	const idDeclParticle *	smoke;
	bool					restart;
};


// bdube: not using
#endif


/*
===============================================================================

idFuncSplat

===============================================================================
*/

class idFuncSplat : public idFuncEmitter {
public:
	CLASS_PROTOTYPE( idFuncSplat );

	idFuncSplat( void );

	void				Spawn( void );

private:
	void				Event_Activate( anEntity *activator );
	void				Event_Splat();
};


/*
===============================================================================

idTextEntity

===============================================================================
*/

class idTextEntity : public anEntity {
public:
	CLASS_PROTOTYPE( idTextEntity );

	void				Spawn( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

	virtual void		Think( void );
	virtual void		ClientPredictionThink( void );

private:
	anStr				text;
	bool				playerOriented;
};


/*
===============================================================================

idLocationEntity

===============================================================================
*/

class idLocationEntity : public anEntity {
public:
	CLASS_PROTOTYPE( idLocationEntity );

	void				Spawn( void );

	const char *		GetLocation( void ) const;

private:
};

class idLocationSeparatorEntity : public anEntity {
public:
	CLASS_PROTOTYPE( idLocationSeparatorEntity );

	void				Spawn( void );

private:
};

class idVacuumSeparatorEntity : public anEntity {
public:
	CLASS_PROTOTYPE( idVacuumSeparatorEntity );

						idVacuumSeparatorEntity( void );

	void				Spawn( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

	void				Event_Activate( anEntity *activator );

private:
	qhandle_t			portal;
};

class idVacuumEntity : public anEntity {
public:
	CLASS_PROTOTYPE( idVacuumEntity );

	void				Spawn( void );

private:
};


// abahr
class rvGravitySeparatorEntity : public anEntity {
public:
	CLASS_PROTOTYPE( rvGravitySeparatorEntity );

						rvGravitySeparatorEntity( void );

	void				Spawn( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

	void				Event_Activate( anEntity *activator );

private:
	qhandle_t			portal;
};

class rvGravityArea : public anEntity {
public:
	ABSTRACT_PROTOTYPE( rvGravityArea );

	void					Spawn( void );

	virtual int				GetArea() const { return area; }
	virtual const anVec3	GetGravity( const anVec3 &origin, const anMat3 &axis, int clipMask, anEntity *passEntity ) const = 0;
	virtual const anVec3	GetGravity( const anEntity *ent ) const = 0;
	virtual const anVec3	GetGravity( const rvClientEntity* ent ) const = 0;

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	bool					IsEqualTo( const rvGravityArea* area ) const;
	bool					operator==( const rvGravityArea* area ) const;
	bool					operator==( const rvGravityArea& area ) const;
	bool					operator!=( const rvGravityArea* area ) const;
	bool					operator!=( const rvGravityArea& area ) const;

protected:
	int						area;
};

class rvGravityArea_Static : public rvGravityArea {
public:
	CLASS_PROTOTYPE( rvGravityArea_Static );

	void					Spawn( void );

	virtual const anVec3	GetGravity( const anVec3 &origin, const anMat3 &axis, int clipMask, anEntity *passEntity ) const { return gravity; }
	virtual const anVec3	GetGravity( const anEntity *ent ) const { return gravity; }
	virtual const anVec3	GetGravity( const rvClientEntity* ent ) const { return gravity; }

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

protected:
	anVec3					gravity;
};

class rvGravityArea_SurfaceNormal : public rvGravityArea {
public:
	CLASS_PROTOTYPE( rvGravityArea_SurfaceNormal );

	virtual const anVec3	GetGravity( const anVec3 &origin, const anMat3 &axis, int clipMask, anEntity *passEntity ) const;
	virtual const anVec3	GetGravity( const anEntity *ent ) const;
	virtual const anVec3	GetGravity( const rvClientEntity* ent ) const;

protected:
	virtual const anVec3	GetGravity( const anPhysics* physics ) const;
};


/*
===============================================================================

  idBeam

===============================================================================
*/

class idBeam : public anEntity {
public:
	CLASS_PROTOTYPE( idBeam );

						idBeam();

	void				Spawn( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

	virtual void		Think( void );

	void				SetMaster( idBeam *masterbeam );
	void				SetBeamTarget( const anVec3 &origin );

	virtual void		Show( void );

	virtual void		WriteToSnapshot( anBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const anBitMsgDelta &msg );

private:
	void				Event_MatchTarget( void );
	void				Event_Activate( anEntity *activator );

	anEntityPtr<idBeam>	target;
	anEntityPtr<idBeam>	master;
};


/*
===============================================================================

  idLiquid

===============================================================================
*/

class idRenderModelLiquid;

class idLiquid : public anEntity {
public:
	CLASS_PROTOTYPE( idLiquid );

	void				Spawn( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

private:
	void				Event_Touch( anEntity *other, trace_t *trace );


	idRenderModelLiquid *model;
};


/*
===============================================================================

  idShaking

===============================================================================
*/

class idShaking : public anEntity {
public:
	CLASS_PROTOTYPE( idShaking );

							idShaking();
							~idShaking();

	void					Spawn( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

private:
	anPhysics_Parametric	physicsObj;
	bool					active;

	void					BeginShaking( void );
	void					Event_Activate( anEntity *activator );
};


/*
===============================================================================

  idEarthQuake

===============================================================================
*/

class idEarthQuake : public anEntity {
public:
	CLASS_PROTOTYPE( idEarthQuake );

						idEarthQuake();

	void				Spawn( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

	virtual void		Think( void );


// kfuller: look for fx entities and the like that may want to be triggered when a mortar round (aka earthquake) goes off
protected:
	void				AffectNearbyEntities(float affectRadius);


private:
	int					nextTriggerTime;
	int					shakeStopTime;
	float				wait;
	float				random;
	bool				triggered;
	bool				playerOriented;
	bool				disabled;
	float				shakeTime;

	void				Event_Activate( anEntity *activator );
};


/*
===============================================================================

  idFuncPortal

===============================================================================
*/

class idFuncPortal : public anEntity {
public:
	CLASS_PROTOTYPE( idFuncPortal );

						idFuncPortal();

	void				Spawn( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

private:
	qhandle_t			portal;
	bool				state;

	void				Event_Activate( anEntity *activator );
};

/*
===============================================================================

  idFuncAASPortal

===============================================================================
*/

class idFuncAASPortal : public anEntity {
public:
	CLASS_PROTOTYPE( idFuncAASPortal );

						idFuncAASPortal();

	void				Spawn( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

private:
	bool				state;

	void				Event_Activate( anEntity *activator );
};

/*
===============================================================================

  idFuncAASObstacle

===============================================================================
*/

class idFuncAASObstacle : public anEntity {
public:
	CLASS_PROTOTYPE( idFuncAASObstacle );

						idFuncAASObstacle();

	void				Spawn( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

	void				SetState ( bool _state );

private:
	bool				state;

	void				Event_Activate( anEntity *activator );
};


/*
===============================================================================

idFuncRadioChatter

===============================================================================
*/

class idFuncRadioChatter : public anEntity {
public:
	CLASS_PROTOTYPE( idFuncRadioChatter );

						idFuncRadioChatter();

	void				Spawn( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

	static void			RepeatLast ( void );

private:
	static anEntityPtr<idFuncRadioChatter> lastRadioChatter;
	float				time;
	bool				isActive;
	void				Event_Activate( anEntity *activator );
	void				Event_ResetRadioHud( anEntity *activator );
	void				Event_IsActive( void );
};


/*
===============================================================================

  idPhantomObjects

===============================================================================
*/

class idPhantomObjects : public anEntity {
public:
	CLASS_PROTOTYPE( idPhantomObjects );

						idPhantomObjects();

	void				Spawn( void );

	void				Save( anSaveGame *savefile ) const;
	void				Restore( anRestoreGame *savefile );

	virtual void		Think( void );

private:
	void				Event_Activate( anEntity *activator );
	void				Event_Throw( void );
	void				Event_ShakeObject( anEntity *object, int starttime );

	int					end_time;
	float				throw_time;
	float				shake_time;
	anVec3				shake_ang;
	float				speed;
	int					min_wait;
	int					max_wait;
	anEntityPtr<anActor>target;
	anList<int>			targetTime;
	anList<anVec3>		lastTargetPos;
};


/*
===============================================================================

rvFuncSaveGame

===============================================================================
*/

class rvFuncSaveGame : public anEntity {
public:
	CLASS_PROTOTYPE( rvFuncSaveGame );

	void				Spawn( void );

	void				Event_Activate		( anEntity *activator );

private:
};


#endif /* !__GAME_MISC_H__ */
