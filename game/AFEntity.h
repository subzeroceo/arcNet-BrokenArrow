#ifndef __GAME_AFENTITY_H__
#define __GAME_AFENTITY_H__

/*
===============================================================================

anMultiModelAF

Entity using multiple separate visual models animated with a single
articulated figure. Only used for debugging!

===============================================================================
*/
const int GIB_DELAY = 200;  // only gib this often to keep performace hits when blowing up several mobs

class anMultiModelAF : public anEntity {
public:
	CLASS_PROTOTYPE( anMultiModelAF );

	void					Spawn( void );
							~anMultiModelAF( void );

	virtual void			Think( void );
	virtual void			Present( void );

protected:
	anPhysics_AF			physicsObj;

	void					SetModelForId( int id, const anStr &modelName );

private:
	anList<anRenderModel *>	modelHandles;
	anList<int>				modelDefHandles;
};

/*
===============================================================================

anAFChain

Chain hanging down from the ceiling. Only used for debugging!

===============================================================================
*/

class anAFChain : public anMultiModelAF {
public:
	CLASS_PROTOTYPE( anAFChain );

	void					Spawn( void );

protected:
	void					BuildChain( const anStr &name, const anVec3 &origin, float linkLength, float linkWidth, float density, int numLinks, bool bindToWorld = true );
};

/*
===============================================================================

anAFAttachment

===============================================================================
*/

class anAFAttachment : public anAnimatedEntity {
public:
	CLASS_PROTOTYPE( anAFAttachment );

							anAFAttachment( void );
	virtual					~anAFAttachment( void );

	void					Spawn( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	void					SetBody( anEntity *bodyEnt, const char *headModel, jointHandle_t attachJoint );
	void					ClearBody( void );
	anEntity *				GetBody( void ) const;

	virtual void			Think( void );

	virtual void			Hide( void );
	virtual void			Show( void );

	void					PlayIdleAnim( int blendTime );

	virtual void			GetImpactInfo( anEntity *ent, int id, const anVec3 &point, impactInfo_t *info );
	virtual void			ApplyImpulse( anEntity *ent, int id, const anVec3 &point, const anVec3 &impulse );
	virtual void			AddForce( anEntity *ent, int id, const anVec3 &point, const anVec3 &force );

	virtual	void			Damage( anEntity *inflictor, anEntity *attacker, const anVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void			AddDamageEffect( const trace_t &collision, const anVec3 &velocity, const char *damageDefName );

	void					SetCombatModel( void );
	anClipModel *			GetCombatModel( void ) const;
	virtual void			LinkCombat( void );
	virtual void			UnlinkCombat( void );

protected:
	anEntity *				body;
	anClipModel *			combatModel;	// render model for hit detection of head
	int						idleAnim;
	jointHandle_t			attachJoint;
};

/*
===============================================================================

anAFEntity_Base

===============================================================================
*/

class anAFEntity_Base : public anAnimatedEntity {
public:
	CLASS_PROTOTYPE( anAFEntity_Base );

							anAFEntity_Base( void );
	virtual					~anAFEntity_Base( void );

	void					Spawn( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	virtual void			Think( void );
	virtual void			AddDamageEffect( const trace_t &collision, const anVec3 &velocity, const char *damageDefName );
	virtual void			GetImpactInfo( anEntity *ent, int id, const anVec3 &point, impactInfo_t *info );
	virtual void			ApplyImpulse( anEntity *ent, int id, const anVec3 &point, const anVec3 &impulse );
	virtual void			AddForce( anEntity *ent, int id, const anVec3 &point, const anVec3 &force );
	virtual bool			Collide( const trace_t &collision, const anVec3 &velocity );
	virtual bool			GetPhysicsToVisualTransform( anVec3 &origin, anMat3 &axis );
	virtual bool			UpdateAnimationControllers( void );
	virtual void			FreeModelDef( void );

	virtual bool			LoadAF( void );
	bool					IsActiveAF( void ) const { return af.IsActive(); }
	const char *			GetAFName( void ) const { return af.GetName(); }
	anPhysics_AF *			GetAFPhysics( void ) { return af.GetPhysics(); }
	const anAF &			GetAF( void ) { return af; }

	void					SetCombatModel( void );
	anClipModel *			GetCombatModel( void ) const;
							// contents of combatModel can be set to 0 or re-enabled (mp)
	void					SetCombatContents( bool enable );
	virtual void			LinkCombat( void );
	virtual void			UnlinkCombat( void );

	int						BodyForClipModelId( int id ) const;

	void					SaveState( anDict &args ) const;
	void					LoadState( const anDict &args );

	virtual void			Unbind( void );
	virtual bool			InitBind( anEntity *master );
	void					AddBindConstraints( void );
	void					RemoveBindConstraints( void );

	virtual void			ShowEditingDialog( void );

	static void				DropAFs( anEntity *ent, const char *type, anList<anEntity *> *list );

protected:
	arcAF					af;				// articulated figure
	anClipModel *			combatModel;	// render model for hit detection
	int						combatModelContents;
	anVec3					spawnOrigin;	// spawn origin
	anMat3					spawnAxis;		// rotation axis used when spawned
	int						nextSoundTime;	// next time this can make a sound

	void					Event_SetConstraintPosition( const char *name, const anVec3 &pos );
};

/*
===============================================================================

anAFEntity_Fragged

===============================================================================
*/

extern const anEventDef		EV_Gib;
extern const anEventDef		EV_Gibbed;

class anAFEntity_Fragged : public anAFEntity_Base {
public:
	CLASS_PROTOTYPE( anAFEntity_Fragged );

							anAFEntity_Fragged( void );
							~anAFEntity_Fragged( void );

	void					Spawn( void );
	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );
	virtual void			Present( void );
	virtual	void			Damage( anEntity *inflictor, anEntity *attacker, const anVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	void					SetThrown( bool isThrown );
	virtual void			SpawnGibs( const anVec3 &dir, const char *damageDefName );

	bool					IsGibbed() { return gibbed; };

protected:
	anRenderModel *			skeletonModel;
	int						skeletonModelDefHandle;
	bool					gibbed;
	bool 					wasThrown;

	virtual void			Gib( const anVec3 &dir, const char *damageDefName );
	void					InitSkeletonModel( void );

	void					Event_Exploded( const char *damageDefName );
};

/*
===============================================================================

	anAFEntity_Generic

===============================================================================
*/

class anAFEntity_Generic : public anAFEntity_Fragged {
public:
	CLASS_PROTOTYPE( anAFEntity_Generic );

							anAFEntity_Generic( void );
							~anAFEntity_Generic( void );

	void					Spawn( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	virtual void			Think( void );
	void					KeepRunningPhysics( void ) { keepRunningPhysics = true; }

private:
	void					Event_Activate( anEntity *activator );

	bool					keepRunningPhysics;
};

/*
===============================================================================

anAFEntity_AttachedHead

===============================================================================
*/

class anAFEntity_AttachedHead : public anAFEntity_Fragged {
public:
	CLASS_PROTOTYPE( anAFEntity_AttachedHead );

							anAFEntity_AttachedHead();
							~anAFEntity_AttachedHead();

	void					Spawn( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	void					SetupHead( void );

	virtual void			Think( void );

	virtual void			Hide( void );
	virtual void			Show( void );
	virtual void			ProjectOverlay( const anVec3 &origin, const anVec3 &dir, float size, const char *material );

	virtual void			LinkCombat( void );
	virtual void			UnlinkCombat( void );

protected:
	virtual void			Gib( const anVec3 &dir, const char *damageDefName );

private:
	anEntityPtr<anAFAttachment>	head;

	void					Event_Exploded( const char *damageDefName );
	void					Event_Activate( anEntity *activator );
};

/*
===============================================================================

anAFEntity_Vehicle

===============================================================================
*/

class anAFEntity_Vehicle : public anAFEntity_Base {
public:
	CLASS_PROTOTYPE( anAFEntity_Vehicle );

							anAFEntity_Vehicle( void );

	void					Spawn( void );
	void					Use( anBasePlayer *player );

protected:
	anBasePlayer *			player;
	jointHandle_t			eyesJoint;
	jointHandle_t			steeringWheelJoint;
	float					wheelRadius;
	float					steerAngle;
	float					steerSpeed;
	const anDeclParticle *	dustSmoke;

	float					GetSteerAngle( void );
};

/*
===============================================================================

anAFEntity_VehicleSimple

===============================================================================
*/

class anAFEntity_VehicleSimple : public anAFEntity_Vehicle {
public:
	CLASS_PROTOTYPE( anAFEntity_VehicleSimple );

							anAFEntity_VehicleSimple( void );
							~anAFEntity_VehicleSimple( void );

	void					Spawn( void );
	virtual void			Think( void );

protected:
	anClipModel *			wheelModel;
	arcAFConstraint_Suspension *	suspension[4];
	jointHandle_t			wheelJoints[4];
	float					wheelAngles[4];
};

/*
===============================================================================

anAFEntity_VehicleFourWheels

===============================================================================
*/

class anAFEntity_VehicleFourWheels : public anAFEntity_Vehicle {
public:
	CLASS_PROTOTYPE( anAFEntity_VehicleFourWheels );

							anAFEntity_VehicleFourWheels( void );

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

anAFEntity_VehicleSixWheels

===============================================================================
*/

class anAFEntity_VehicleSixWheels : public anAFEntity_Vehicle {
public:
	CLASS_PROTOTYPE( anAFEntity_VehicleSixWheels );

							anAFEntity_VehicleSixWheels( void );

	void					Spawn( void );
	virtual void			Think( void );
	float					force;
	float					velocity;
	float					steerAngle;
private:
	arcAFBody *				wheels[6];
	arcAFConstraint_Hinge *	steering[4];
	jointHandle_t			wheelJoints[6];
	float					wheelAngles[6];
};

/*
===============================================================================

anAFEntity_VehicleAutomated

===============================================================================
*/

class anAFEntity_VehicleAutomated : public anAFEntity_VehicleSixWheels {
public:
	CLASS_PROTOTYPE( anAFEntity_VehicleAutomated );

	void					Spawn( void );
	void					PostSpawn( void );
	virtual void			Think( void );

private:

	anEntity *	waypoint;
	float		steeringSpeed;
	float		currentSteering;
	float		idealSteering;
	float		originHeight;

	void		Event_SetVelocity( float _velocity );
	void		Event_SetTorque( float _torque );
	void		Event_SetSteeringSpeed( float _steeringSpeed );
	void		Event_SetWayPoint( anEntity *_waypoint );
};

/*
===============================================================================

anAFEntity_SteamPipe

===============================================================================
*/

class anAFEntity_SteamPipe : public anAFEntity_Base {
public:
	CLASS_PROTOTYPE( anAFEntity_SteamPipe );

							anAFEntity_SteamPipe( void );
							~anAFEntity_SteamPipe( void );

	void					Spawn( void );
	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

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

anAFEntity_ClawFourFingers

===============================================================================
*/

class anAFEntity_ClawFourFingers : public anAFEntity_Base {
public:
	CLASS_PROTOTYPE( anAFEntity_ClawFourFingers );

							anAFEntity_ClawFourFingers( void );

	void					Spawn( void );
	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

private:
	arcAFConstraint_Hinge *	fingers[4];

	void					Event_SetFingerAngle( float angle );
	void					Event_StopFingers( void );
};

/*
===============================================================================

Harvestable contains all of the code required to turn an entity into a harvestable
entity. The entity must create an instance of this class and call the appropriate
interface methods at the correct time.

===============================================================================
*/
class anHarvest : public anEntity {
public:
	CLASS_PROTOTYPE( anHarvest );

							anHarvest( void );
							~anHarvest( void );

	void					Spawn( void );
	void					Init( anEntity *parent );
	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	void					SetParent( anEntity *parent );

	void					Think( void );
	void					Gib( void );

protected:
	anEntityPtr<anEntity>	parentEnt;
	float					triggersize;
	anClipModel *			trigger;
	float					giveDelay;
	float					removeDelay;
	bool					given;

	anEntityPtr<anBasePlayer> player;
	int						startTime;

	bool					fxFollowPlayer;
	anEntityPtr<anEntityFx>	fx;
	anStr					fxOrient;

protected:
	void					BeginBurn( void );
	void					BeginFX( void );
	void					CalcTriggerBounds( float size, anBounds &bounds );

	bool					GetFxOrientationAxis( anMat3 &mat );

	void					Event_SpawnHarvestTrigger( void );
	void					Event_Touch( anEntity *other, trace_t *trace );
} ;


/*
===============================================================================

anAFEntity_Harvest

===============================================================================
*/

class anAFEntity_Harvest : public anAFEntity_AttachedHead {
public:
	CLASS_PROTOTYPE( anAFEntity_Harvest );

	anAFEntity_Harvest();
	~anAFEntity_Harvest();

	void					Spawn( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	virtual void			Think( void );

	virtual void			Gib( const anVec3 &dir, const char *damageDefName );

protected:
	anEntityPtr<anHarvest> harvestEnt;
protected:
	void					Event_SpawnHarvestEntity( void );

};

#endif // !__GAME_AFENTITY_H__