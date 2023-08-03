
// bdube: note that this file is no longer merged with Doom3 updates
//
// MERGE_DATE 09/30/2004

#ifndef __GAME_PROJECTILE_H__
#define __GAME_PROJECTILE_H__

/*
===============================================================================

  idProjectile

===============================================================================
*/

extern const anEventDef EV_Explode;

class idProjectile : public anEntity {
public :
	CLASS_PROTOTYPE( idProjectile );

							idProjectile();
	virtual					~idProjectile();

	void					Spawn( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	void					Create( anEntity *owner, const anVec3 &start, const anVec3 &dir, anEntity* ignore = nullptr, anEntity* extraPassEntity = nullptr );
	virtual void			Launch( const anVec3 &start, const anVec3 &dir, const anVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float dmgPower = 1.0f );

	virtual void			FreeLightDef( void );

//RITUAL BEGIN
	void					SetOwner(anEntity* ent)	{ owner = ent;	}
// RITUAL END

	anEntity *				GetOwner( void ) const;

	virtual void			Think( void );
	virtual void			Killed( anEntity *inflictor, anEntity *attacker, int damage, const anVec3 &dir, int location );
	virtual bool			GetPhysicsToVisualTransform( anVec3 &origin, anMat3 &axis );

	virtual bool			Collide( const trace_t &collision, const anVec3 &velocity );
	virtual bool			Collide( const trace_t &collision, const anVec3 &velocity, bool &hitTeleporter );
	virtual void			Explode( const trace_t *collision, const bool showExplodeFX, anEntity *ignore = nullptr, const char *sndExplode = "snd_explode" );
	void					Fizzle( void );

	static anVec3			GetVelocity( const anDict *projectile );
	static anVec3			GetGravity( const anDict *projectile );

	void					SetSpeed		( float s, int accelTime = 0 );
	float					GetSpeed		( void ) const;

	virtual void			UpdateVisualAngles();

	// information about what kind of projectile we are, used for death messages
	int						methodOfDeath;

	virtual void			ClientPredictionThink( void );
	virtual void			WriteToSnapshot( anBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const anBitMsgDelta &msg );

	virtual bool			ClientStale( void );

protected:
	void					SpawnImpactEntities(const trace_t& collision, const anVec3 projectileDirection);


	anEntityPtr<anEntity>	owner;

	struct projectileFlags_s {
		bool				detonate_on_world			: 1;
		bool				detonate_on_actor			: 1;
		bool				detonate_on_bounce			: 1;		// Detonate if hit a bounce surface
		bool				randomShaderSpin			: 1;
		bool				isTracer					: 1;
	} projectileFlags;

	float					damagePower;

	renderLight_t			renderLight;
	qhandle_t				lightDefHandle;				// handle to renderer light def
	anVec3					lightOffset;
	int						lightStartTime;
	int						lightEndTime;
	anVec3					lightColor;

	anEntity*				impactedEntity;

	anPhysics_Particle		physicsObj;
	anAngles				visualAngles;
	anAngles				angularVelocity;
	idInterpolate<float>	speed;
	bool					updateVelocity;

	rvSphericalInterpolate	rotation;

	rvClientEffectPtr		flyEffect;
	float					flyEffectAttenuateSpeed;

	int						bounceCount;
	bool					sticky;

	anString					impactEntity;
	int						numImpactEntities;
	int						ieMinPitch;
	int						ieMaxPitch;
	float					ieSlicePercentage;


// ddynerman: hit count for stats
	int						hitCount;
// ddynerman: pre-prediction ( rocket jumping )
	int						prePredictTime;

	typedef enum {
		SPAWNED = 0,
		CREATED = 1,
		LAUNCHED = 2,
		FIZZLED = 3,
		EXPLODED = 4
	} projectileState_t;

	projectileState_t		state;

	void					PlayPainEffect		( anEntity* ent, int damage, const rvDeclMatType* materialType, const anVec3& origin, const anVec3& direction );
	virtual void			PlayDetonateEffect	( const anVec3& origin, const anMat3& axis );

private:
	void					DefaultDamageEffect	( const trace_t &collision, const anVec3 &velocity, const char *damageDefName );

	void					Event_Explode			( void );
	void					Event_Fizzle			( void );
	void					Event_RadiusDamage		( anEntity *ignore );
	void					Event_ResidualDamage	( anEntity *ignore );
	void					Event_Touch				( anEntity *other, trace_t *trace );

	bool					syncPhysics;

	// cheap linear client side projectiles
	// transmitted in snapshot
	int						launchTime;
	anVec3					launchOrig;
	anVec3					launchDir;
	// set from def file in :Launch on both client and server
	float					launchSpeed;
};

ARC_INLINE float idProjectile::GetSpeed ( void ) const {
	return speed.GetCurrentValue( gameLocal.time );
}

/*
===============================================================================

idGuidedProjectile

===============================================================================
*/

extern const anEventDef EV_UpdateGuideTarget;
extern const anEventDef EV_GuideToEntity;
extern const anEventDef EV_GuideToPos;

class idGuidedProjectile : public idProjectile {
public :
	CLASS_PROTOTYPE( idGuidedProjectile );

							idGuidedProjectile( void );
							~idGuidedProjectile( void );

	enum {
		GUIDE_NONE,
		GUIDE_ENTITY,
		GUIDE_POS,
		GUIDE_DIR,
		GUIDE_MAX
	};

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	virtual void			Think( void );
	virtual void			Launch( const anVec3 &start, const anVec3 &dir, const anVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float dmgPower = 1.0f );

	void					GuideTo			( const anVec3& post, const anVec3& dir );
	void					GuideTo			( const anVec3& pos );
	void					GuideTo			( anEntity* ent, jointHandle_t guideJoint=INVALID_JOINT, const anVec3 &offset=vec3_origin );
	void					CancelGuide		( void );

	int						GetGuideType	( void ) const;

	virtual void			Killed( anEntity *inflictor, anEntity *attacker, int damage, const anVec3 &dir, int location );

protected:

	int						guideType;
	anEntityPtr<anEntity>	guideEnt;
	anVec3					guideDir;
	anVec3					guidePos;
	jointHandle_t			guideJoint;
	float					guideMinDist;

	int						driftTime;
	int						driftRate;
	float					driftRange;
	float					driftRadius;
	float					driftDiversity;
	float					driftAngle;
	float					driftAngleStep;
	float					driftProjectRange;

	virtual bool			GetGuideDir		( anVec3 &outDir, float& outDist );

private:

	idInterpolate<float>	turn_max;
	int						launchTime;
	int						guideDelay;
	int						driftDelay;
};

ARC_INLINE int idGuidedProjectile::GetGuideType ( void ) const {
	return guideType;
}

ARC_INLINE void idGuidedProjectile::GuideTo ( const anVec3& pos, const anVec3& dir ) {
	guideType = GUIDE_DIR;
	guidePos  = pos;
	guideDir  = dir;
}

ARC_INLINE void idGuidedProjectile::GuideTo ( const anVec3& pos ) {
	guideType = GUIDE_POS;
	guidePos  = pos;
}

ARC_INLINE void idGuidedProjectile::GuideTo ( anEntity* ent, jointHandle_t joint, const anVec3 &offset ) {
	guideType = GUIDE_ENTITY;
	guideEnt  = ent;
	guideJoint = joint;
	guidePos = offset;

	if ( guideEnt.IsValid() ) {
		guideEnt->GuidedProjectileIncoming( this );
	}
}

ARC_INLINE void idGuidedProjectile::CancelGuide ( void ) {
	guideType = GUIDE_NONE;

	// twhitaker: TEMP
	if ( guideEnt.IsValid() ) {
		guideEnt->GuidedProjectileIncoming( nullptr );
	}
	// </twhitaker>
}

/*
===============================================================================

rvDriftingProjectile

===============================================================================
*/

class rvDriftingProjectile : public idProjectile {
public :
	CLASS_PROTOTYPE( rvDriftingProjectile );

							rvDriftingProjectile ( void );
							~rvDriftingProjectile ( void );

	void					Save			( anSaveGame *savefile ) const;
	void					Restore			( anRestoreGame *savefile );

	virtual void			Think			( void );
	virtual void			Launch			( const anVec3 &start, const anVec3 &dir, const anVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float dmgPower = 1.0f );

protected:

	virtual void			UpdateVisualAngles	( void );

	anVec3					startDir;
	anVec3					startOrigin;
	anMat3					startAxis;
	float					startSpeed;

	idInterpolateAccelDecelLinear<float>	driftOffset[2];
	idInterpolateAccelDecelLinear<float>	driftSpeed;
	float									driftOffsetMax;
	float									driftSpeedMax;
	float									driftTime;
};

/*
===============================================================================

rvSpawnerProjectile

===============================================================================
*/

class rvSpawner;
class rvSpawnerProjectile : public idProjectile {
public :
	CLASS_PROTOTYPE( rvSpawnerProjectile );

							rvSpawnerProjectile ( void );
							~rvSpawnerProjectile ( void );

	void					Spawn			( void );
	virtual void			Think			( void );

	void					SetSpawner		( rvSpawner* spawner );

protected:

	anEntityPtr<rvSpawner>	spawner;

	enum {
		STATE_NONE,
		STATE_ADDED,
	} spawnState;

private:

	void					Event_PostSpawn	( void );
};

/*
===============================================================================

rvMIRVProjectile

===============================================================================
*/

class rvMIRVProjectile : public idProjectile {
	CLASS_PROTOTYPE( rvMIRVProjectile );

						rvMIRVProjectile ( void );
						~rvMIRVProjectile (  void );


	void				Spawn				( void );

private:

	void				Event_LaunchWarheads ( void );
};

#endif /* !__GAME_PROJECTILE_H__ */


