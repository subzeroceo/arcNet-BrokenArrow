#ifndef __GAME_AFENTITY_H__
#define __GAME_AFENTITY_H__

/*
===============================================================================

arcMultiModelAF

Entity using multiple separate visual models animated with a single
articulated figure. Only used for debugging!

===============================================================================
*/
const int GIB_DELAY = 200;  // only gib this often to keep performace hits when blowing up several mobs

class arcMultiModelAF : public arcEntity {
public:
	CLASS_PROTOTYPE( arcMultiModelAF );

	void					Spawn( void );
							~arcMultiModelAF( void );

	virtual void			Think( void );
	virtual void			Present( void );

protected:
	arcPhysics_AF			physicsObj;

	void					SetModelForId( int id, const idStr &modelName );

private:
	arcList<idRenderModel *>	modelHandles;
	arcList<int>				modelDefHandles;
};

/*
===============================================================================

arcNetChain

Chain hanging down from the ceiling. Only used for debugging!

===============================================================================
*/

class arcNetChain : public arcMultiModelAF {
public:
	CLASS_PROTOTYPE( arcNetChain );

	void					Spawn( void );

protected:
	void					BuildChain( const idStr &name, const arcVec3 &origin, float linkLength, float linkWidth, float density, int numLinks, bool bindToWorld = true );
};

/*
===============================================================================

arcAFAttachment

===============================================================================
*/

class arcAFAttachment : public arcAnimatedEntity {
public:
	CLASS_PROTOTYPE( arcAFAttachment );

							arcAFAttachment( void );
	virtual					~arcAFAttachment( void );

	void					Spawn( void );

	void					Save( arcSaveGame *savefile ) const;
	void					Restore( arcRestoreGame *savefile );

	void					SetBody( arcEntity *bodyEnt, const char *headModel, jointHandle_t attachJoint );
	void					ClearBody( void );
	arcEntity *				GetBody( void ) const;

	virtual void			Think( void );

	virtual void			Hide( void );
	virtual void			Show( void );

	void					PlayIdleAnim( int blendTime );

	virtual void			GetImpactInfo( arcEntity *ent, int id, const arcVec3 &point, impactInfo_t *info );
	virtual void			ApplyImpulse( arcEntity *ent, int id, const arcVec3 &point, const arcVec3 &impulse );
	virtual void			AddForce( arcEntity *ent, int id, const arcVec3 &point, const arcVec3 &force );

	virtual	void			Damage( arcEntity *inflictor, arcEntity *attacker, const arcVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void			AddDamageEffect( const trace_t &collision, const arcVec3 &velocity, const char *damageDefName );

	void					SetCombatModel( void );
	arcClipModel *			GetCombatModel( void ) const;
	virtual void			LinkCombat( void );
	virtual void			UnlinkCombat( void );

protected:
	arcEntity *				body;
	arcClipModel *			combatModel;	// render model for hit detection of head
	int						idleAnim;
	jointHandle_t			attachJoint;
};

/*
===============================================================================

arcAFEntity_Base

===============================================================================
*/

class arcAFEntity_Base : public arcAnimatedEntity {
public:
	CLASS_PROTOTYPE( arcAFEntity_Base );

							arcAFEntity_Base( void );
	virtual					~arcAFEntity_Base( void );

	void					Spawn( void );

	void					Save( arcSaveGame *savefile ) const;
	void					Restore( arcRestoreGame *savefile );

	virtual void			Think( void );
	virtual void			GetImpactInfo( arcEntity *ent, int id, const arcVec3 &point, impactInfo_t *info );
	virtual void			ApplyImpulse( arcEntity *ent, int id, const arcVec3 &point, const arcVec3 &impulse );
	virtual void			AddForce( arcEntity *ent, int id, const arcVec3 &point, const arcVec3 &force );
	virtual bool			Collide( const trace_t &collision, const arcVec3 &velocity );
	virtual bool			GetPhysicsToVisualTransform( arcVec3 &origin, arcMat3 &axis );
	virtual bool			UpdateAnimationControllers( void );
	virtual void			FreeModelDef( void );

	virtual bool			LoadAF( void );
	bool					IsActiveAF( void ) const { return af.IsActive(); }
	const char *			GetAFName( void ) const { return af.GetName(); }
	arcPhysics_AF *			GetAFPhysics( void ) { return af.GetPhysics(); }

	void					SetCombatModel( void );
	arcClipModel *			GetCombatModel( void ) const;
							// contents of combatModel can be set to 0 or re-enabled (mp)
	void					SetCombatContents( bool enable );
	virtual void			LinkCombat( void );
	virtual void			UnlinkCombat( void );

	int						BodyForClipModelId( int id ) const;

	void					SaveState( arcDict &args ) const;
	void					LoadState( const arcDict &args );

	void					AddBindConstraints( void );
	void					RemoveBindConstraints( void );

	virtual void			ShowEditingDialog( void );

	static void				DropAFs( arcEntity *ent, const char *type, arcList<arcEntity *> *list );

protected:
	arcAF					af;				// articulated figure
	arcClipModel *			combatModel;	// render model for hit detection
	int						combatModelContents;
	arcVec3					spawnOrigin;	// spawn origin
	arcMat3					spawnAxis;		// rotation axis used when spawned
	int						nextSoundTime;	// next time this can make a sound

	void					Event_SetConstraintPosition( const char *name, const arcVec3 &pos );
};

/*
===============================================================================

arcAFEntity_Gibbable

===============================================================================
*/

extern const arcEventDef		EV_Gib;
extern const arcEventDef		EV_Gibbed;

class arcAFEntity_Gibbable : public arcAFEntity_Base {
public:
	CLASS_PROTOTYPE( arcAFEntity_Gibbable );

							arcAFEntity_Gibbable( void );
							~arcAFEntity_Gibbable( void );

	void					Spawn( void );
	void					Save( arcSaveGame *savefile ) const;
	void					Restore( arcRestoreGame *savefile );
	virtual void			Present( void );
	virtual	void			Damage( arcEntity *inflictor, arcEntity *attacker, const arcVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void			SpawnGibs( const arcVec3 &dir, const char *damageDefName );

protected:
	idRenderModel *			skeletonModel;
	int						skeletonModelDefHandle;
	bool					gibbed;

	virtual void			Gib( const arcVec3 &dir, const char *damageDefName );
	void					InitSkeletonModel( void );

	void					Event_Gib( const char *damageDefName );
};

/*
===============================================================================

	arcAFEntity_Generic

===============================================================================
*/

class arcAFEntity_Generic : public arcAFEntity_Gibbable {
public:
	CLASS_PROTOTYPE( arcAFEntity_Generic );

							arcAFEntity_Generic( void );
							~arcAFEntity_Generic( void );

	void					Spawn( void );

	void					Save( arcSaveGame *savefile ) const;
	void					Restore( arcRestoreGame *savefile );

	virtual void			Think( void );
	void					KeepRunningPhysics( void ) { keepRunningPhysics = true; }

private:
	void					Event_Activate( arcEntity *activator );

	bool					keepRunningPhysics;
};

/*
===============================================================================

arcAFEntity_WithAttachedHead

===============================================================================
*/

class arcAFEntity_WithAttachedHead : public arcAFEntity_Gibbable {
public:
	CLASS_PROTOTYPE( arcAFEntity_WithAttachedHead );

							arcAFEntity_WithAttachedHead();
							~arcAFEntity_WithAttachedHead();

	void					Spawn( void );

	void					Save( arcSaveGame *savefile ) const;
	void					Restore( arcRestoreGame *savefile );

	void					SetupHead( void );

	virtual void			Think( void );

	virtual void			Hide( void );
	virtual void			Show( void );
	virtual void			ProjectOverlay( const arcVec3 &origin, const arcVec3 &dir, float size, const char *material );

	virtual void			LinkCombat( void );
	virtual void			UnlinkCombat( void );

protected:
	virtual void			Gib( const arcVec3 &dir, const char *damageDefName );

private:
	arcEntityPtr<arcAFAttachment>	head;

	void					Event_Gib( const char *damageDefName );
	void					Event_Activate( arcEntity *activator );
};

/*
===============================================================================

arcAFEntity_Vehicle

===============================================================================
*/

class arcAFEntity_Vehicle : public arcAFEntity_Base {
public:
	CLASS_PROTOTYPE( arcAFEntity_Vehicle );

							arcAFEntity_Vehicle( void );

	void					Spawn( void );
	void					Use( arcNetBasePlayer *player );

protected:
	arcNetBasePlayer *		player;
	jointHandle_t			eyesJoint;
	jointHandle_t			steeringWheelJoint;
	float					wheelRadius;
	float					steerAngle;
	float					steerSpeed;
	const arcDeclParticle *	dustSmoke;

	float					GetSteerAngle( void );
};

/*
===============================================================================

arcAFEntity_VehicleSimple

===============================================================================
*/

class arcAFEntity_VehicleSimple : public arcAFEntity_Vehicle {
public:
	CLASS_PROTOTYPE( arcAFEntity_VehicleSimple );

							arcAFEntity_VehicleSimple( void );
							~arcAFEntity_VehicleSimple( void );

	void					Spawn( void );
	virtual void			Think( void );

protected:
	arcClipModel *			wheelModel;
	arcAFConstraint_Suspension *	suspension[4];
	jointHandle_t			wheelJoints[4];
	float					wheelAngles[4];
};

/*
===============================================================================

arcAFEntity_VehicleFourWheels

===============================================================================
*/

class arcAFEntity_VehicleFourWheels : public arcAFEntity_Vehicle {
public:
	CLASS_PROTOTYPE( arcAFEntity_VehicleFourWheels );

							arcAFEntity_VehicleFourWheels( void );

	void					Spawn( void );
	virtual void			Think( void );

protected:
	arcAFBody *				wheels[4];
	arcAFConstraint_Hinge *	steering[2];
	jointHandle_t			wheelJoints[4];
	float					wheelAngles[4];
};


/*
===============================================================================

arcAFEntity_VehicleSixWheels

===============================================================================
*/

class arcAFEntity_VehicleSixWheels : public arcAFEntity_Vehicle {
public:
	CLASS_PROTOTYPE( arcAFEntity_VehicleSixWheels );

							arcAFEntity_VehicleSixWheels( void );

	void					Spawn( void );
	virtual void			Think( void );

private:
	arcAFBody *				wheels[6];
	arcAFConstraint_Hinge *	steering[4];
	jointHandle_t			wheelJoints[6];
	float					wheelAngles[6];
};

/*
===============================================================================

arcAFEntity_SteamPipe

===============================================================================
*/

class arcAFEntity_SteamPipe : public arcAFEntity_Base {
public:
	CLASS_PROTOTYPE( arcAFEntity_SteamPipe );

							arcAFEntity_SteamPipe( void );
							~arcAFEntity_SteamPipe( void );

	void					Spawn( void );
	void					Save( arcSaveGame *savefile ) const;
	void					Restore( arcRestoreGame *savefile );

	virtual void			Think( void );

private:
	int						steamBody;
	float					steamForce;
	float					steamUpForce;
	arcForce_Constant		force;
	renderEntity_t			steamRenderEntity;
	qhandle_t				steamModelDefHandle;

	void					InitSteamRenderEntity( void );
};

/*
===============================================================================

arcAFEntity_ClawFourFingers

===============================================================================
*/

class arcAFEntity_ClawFourFingers : public arcAFEntity_Base {
public:
	CLASS_PROTOTYPE( arcAFEntity_ClawFourFingers );

							arcAFEntity_ClawFourFingers( void );

	void					Spawn( void );
	void					Save( arcSaveGame *savefile ) const;
	void					Restore( arcRestoreGame *savefile );

private:
	arcAFConstraint_Hinge *	fingers[4];

	void					Event_SetFingerAngle( float angle );
	void					Event_StopFingers( void );
};

#endif // !__GAME_AFENTITY_H__