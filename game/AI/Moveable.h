
// bdube: note that this file is no longer merged with Doom3 updates
//
// MERGE_DATE 06/02/2004

#ifndef __GAME_MOVEABLE_H__
#define __GAME_MOVEABLE_H__

/*
===============================================================================

  Entity using rigid body physics.

===============================================================================
*/

extern const anEventDef EV_BecomeNonSolid;
extern const anEventDef EV_IsAtRest;

class idMoveable : public idDamagable {
public:
	CLASS_PROTOTYPE( idMoveable );

							idMoveable( void );
							~idMoveable( void );

	void					Spawn( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	virtual void			Think( void );

	virtual void			Hide( void );
	virtual void			Show( void );

	bool					AllowStep( void ) const;
	void					EnableDamage( bool enable, float duration );
	virtual bool			Collide( const trace_t &collision, const anVec3 &velocity );
	virtual void			Damage( anEntity *inflictor, anEntity *attacker, const anVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void			Killed( anEntity *inflictor, anEntity *attacker, int damage, const anVec3 &dir, int location );
	virtual void			WriteToSnapshot( anBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const anBitMsgDelta &msg );

	virtual void			AddDamageEffect( const trace_t &collision, const anVec3 &velocity, const char *damageDefName, anEntity *inflictor );

protected:

	anPhysics_RigidBody		physicsObj;				// physics object
	anStr					brokenModel;			// model set when health drops down to or below zero
	anStr					damage;					// if > 0 apply damage to hit entities
	int						nextCollideFxTime;		// next time it is ok to spawn collision fx
	float					minDamageVelocity;		// minimum velocity before moveable applies damage
	float					maxDamageVelocity;		// velocity at which the maximum damage is applied
	anCurve_Spline<anVec3> *initialSpline;			// initial spline path the moveable follows
	anVec3					initialSplineDir;		// initial relative direction along the spline path
	bool					unbindOnDeath;			// unbind from master when health drops down to or below zero
	bool					allowStep;				// allow monsters to step on the object
	bool					canDamage;				// only apply damage when this is set

	anEntityPtr<anEntity>	lastAttacker;

	virtual void			ExecuteStage	( void );

	const anMaterial *		GetRenderModelMaterial( void ) const;
	void					BecomeNonSolid( void );
	void					InitInitialSpline( int startTime );
	bool					FollowInitialSplinePath( void );

	void					Event_Activate( anEntity *activator );
	void					Event_BecomeNonSolid( void );
	void					Event_SetOwnerFromSpawnArgs( void );
	void					Event_IsAtRest( void );
	void					Event_CanDamage ( float enable );
	void					Event_SetHealth ( float newHealth );
	void					Event_RadiusDamage( anEntity *attacker, const char *splash );
};


/*
===============================================================================

  A barrel using rigid body physics. The barrel has special handling of
  the view model orientation to make it look like it rolls instead of slides.

===============================================================================
*/

class idBarrel : public idMoveable {

public:
	CLASS_PROTOTYPE( idBarrel );
							idBarrel();

	void					Spawn( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	void					BarrelThink( void );
	virtual void			Think( void );
	virtual bool			GetPhysicsToVisualTransform( anVec3 &origin, anMat3 &axis );
	virtual void			ClientPredictionThink( void );

private:
	float					radius;					// radius of barrel
	int						barrelAxis;				// one of the coordinate axes the barrel cylinder is parallel to
	anVec3					lastOrigin;				// origin of the barrel the last think frame
	anMat3					lastAxis;				// axis of the barrel the last think frame
	float					additionalRotation;		// additional rotation of the barrel about it's axis
	anMat3					additionalAxis;			// additional rotation axis
};


/*
===============================================================================

  A barrel using rigid body physics and special handling of the view model
  orientation to make it look like it rolls instead of slides. The barrel
  can burn and explode when damaged.

===============================================================================
*/

class idExplodingBarrel : public idBarrel {
public:
	CLASS_PROTOTYPE( idExplodingBarrel );

							idExplodingBarrel();
							~idExplodingBarrel();

	void					Spawn( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	virtual void			Think( void );
	virtual void			Damage( anEntity *inflictor, anEntity *attacker, const anVec3 &dir,
								const char *damageDefName, const float damageScale, const int location );
	virtual void			Killed( anEntity *inflictor, anEntity *attacker, int damage, const anVec3 &dir, int location );

	virtual void			WriteToSnapshot( anBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const anBitMsgDelta &msg );

private:
	typedef enum {
		NORMAL = 0,
		BURNING,
		BURNEXPIRED,
		EXPLODING,
		EXPLODED
	} explode_state_t;
	explode_state_t			state;

	anVec3					spawnOrigin;
	anMat3					spawnAxis;
	qhandle_t				ipsHandle;
	qhandle_t				lightHandle;
	renderEntity_t			ipsEntity;
	renderLight_t			light;
	int						ipsTime;
	int						lightTime;
	float					time;

	int						explodeFinishTime;

	void					AddIPS( const char *name, bool burn );
	void					AddLight( const char *name , bool burn );
	void					ExplodingEffects( void );

	void					Event_Activate( anEntity *activator );
	void					Event_Respawn();
	void					Event_Explode();
	void					Event_TriggerTargets();
};

#endif /* !__GAME_MOVEABLE_H__ */


