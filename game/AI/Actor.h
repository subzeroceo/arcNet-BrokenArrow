
// bdube: note that this file is no longer merged with Doom3 updates
//
// MERGE_DATE 09/30/2004

#ifndef __GAME_ACTOR_H__
#define __GAME_ACTOR_H__

/*
===============================================================================

	anActor

===============================================================================
*/

extern const anEventDef AI_EnableEyeFocus;
extern const anEventDef AI_DisableEyeFocus;
extern const anEventDef EV_Footstep;
extern const anEventDef EV_FootstepLeft;
extern const anEventDef EV_FootstepRight;
extern const anEventDef EV_EnableWalkIK;
extern const anEventDef EV_DisableWalkIK;
extern const anEventDef EV_EnableLegIK;
extern const anEventDef EV_DisableLegIK;
extern const anEventDef AI_SetAnimPrefix;
extern const anEventDef AI_PlayAnim;
extern const anEventDef AI_PlayCycle;
extern const anEventDef AI_AnimDone;
extern const anEventDef AI_SetBlendFrames;
extern const anEventDef AI_GetBlendFrames;
extern const anEventDef AI_ScriptedMove;
extern const anEventDef AI_ScriptedDone;
extern const anEventDef AI_ScriptedStop;


// bdube: added flashlight
extern const anEventDef AI_Flashlight;
extern const anEventDef AI_EnterVehicle;
extern const anEventDef AI_ExitVehicle;
// nmckenzie:
extern const anEventDef AI_OverrideAnim;
extern const anEventDef AI_IdleAnim;
extern const anEventDef AI_SetState;
// jshepard: adjust animation speed
extern const anEventDef AI_SetAnimRate;
//MCG: damage over time
extern const anEventDef EV_DamageOverTime;
extern const anEventDef EV_DamageOverTimeEffect;
//MCG: script-callable joint crawl effect
extern const anEventDef EV_JointCrawlEffect;

// abahr:
extern const anEventDef AI_LookAt;
extern const anEventDef AI_FaceEnemy;
extern const anEventDef AI_FaceEntity;
extern const anEventDef	AI_JumpDown;
extern const anEventDef AI_SetLeader;


class idAnimState {
public:

	bool					idleAnim;
	int						animBlendFrames;
	int						lastAnimBlendFrames;		// allows override anims to blend based on the last transition time

public:
							idAnimState();
							~idAnimState();

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	void					Init( anEntity *owner, anAnimator *_animator, int animchannel );
	void					Shutdown( void );
	void					SetState		( const char *name, int blendFrames, int flags = 0 );
	void					PostState		( const char *name, int blendFrames = 0, int delay = 0, int flags = 0 );
	void					StopAnim( int frames );
	void					PlayAnim( int anim );
	void					CycleAnim( int anim );
	void					BecomeIdle( void );
	bool					UpdateState( void );
	bool					Disabled( void ) const;
	void					Enable( int blendFrames );
	void					Disable( void );
	bool					AnimDone( int blendFrames ) const;
	bool					IsIdle( void ) const;
	animFlags_t				GetAnimFlags( void ) const;

	anStateThread&			GetStateThread	( void );

	anAnimator *			GetAnimator( void ) const {return animator;};
private:

// bdube: converted self to entity ptr so any entity can use it
	anEntity *				self;

	anAnimator *			animator;
	int						channel;
	bool					disabled;

	anStateThread			stateThread;
};

inline anStateThread& idAnimState::GetStateThread ( void ) {
	return stateThread;
}

class idAttachInfo {
public:
	anEntityPtr<anEntity>	ent;
	int						channel;
};

class anActor : public idAFEntity_Gibbable {
public:
	CLASS_PROTOTYPE( anActor );

	int						team;
	anLinkList<anActor>		teamNode;
	int						rank;				// monsters don't fight back if the attacker's rank is higher
	anMat3					viewAxis;			// view axis of the actor

	anLinkList<anActor>		enemyNode;			// node linked into an entity's enemy list for quick lookups of who is attacking him
	anLinkList<anActor>		enemyList;			// list of characters that have targeted the player as their enemy

public:
							anActor( void );
	virtual					~anActor( void );

	void					Spawn( void );
	virtual void			Restart( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	virtual void			Hide( void );
	virtual void			Show( void );
	virtual int				GetDefaultSurfaceType( void ) const;
	virtual void			ProjectOverlay( const anVec3 &origin, const anVec3 &dir, float size, const char *material );

	virtual bool			LoadAF( const char *keyname = nullptr, bool purgeAF = false );
	void					SetupBody( void );

	virtual void			CheckBlink( void );

	virtual bool			GetPhysicsToVisualTransform( anVec3 &origin, anMat3 &axis );
	virtual bool			GetPhysicsToSoundTransform( anVec3 &origin, anMat3 &axis );

							// script state management
	void					ShutdownThreads		( void );
	void					UpdateState			( void );

	virtual void			OnStateThreadClear ( const char *statename, int flags = 0 );
	void					SetState		( const char *statename, int flags = 0 );
	void					PostState		( const char *statename, int delay = 0, int flags = 0 );
	void					InterruptState	( const char *statename, int delay = 0, int flags = 0 );

							// vision testing
	void					SetEyeHeight( float height );
	void					SetChestHeight ( float height );
	float					EyeHeight( void ) const;

	virtual anVec3			GetEyePosition( void ) const;
	virtual anVec3			GetChestPosition ( void ) const;
	anEntity*				GetGroundEntity ( void ) const;
	virtual anEntity*		GetGroundElevator( anEntity *testElevator=nullptr ) const;

	void					Present( void );

	virtual void			GetViewPos	( anVec3 &origin, anMat3 &axis ) const;
	void					SetFOV		( float fov, float fovClose );
	bool					CheckFOV	( const anVec3 &pos, float ang = -1.0f ) const;
	virtual bool			HasFOV		( anEntity *ent );
	virtual	bool			CanSee		( const anEntity *ent, bool useFOV ) const;
	virtual	bool			CanSeeFrom	( const anVec3 &from, const anEntity *ent, bool useFOV ) const;
	virtual	bool			CanSeeFrom	( const anVec3 &from, const anVec3 &toPos, bool useFOV ) const;

							// damage
	void					SetupDamageGroups( void );

	virtual	void			Damage( anEntity *inflictor, anEntity *attacker, const anVec3 &dir, const char *damageDefName, const float damageScale, const int location );

// nmckenzie: a final hook in the middle of the damage function
	virtual void			AdjustHealthByDamage ( int inDamage ){health -= inDamage;}


	virtual int				GetDamageForLocation( int damage, int location );
	const char *			GetDamageGroup( int location );
	void					ClearPain( void );
	virtual bool			Pain( anEntity *inflictor, anEntity *attacker, int damage, const anVec3 &dir, int location );
	virtual void			AddDamageEffect( const trace_t &collision, const anVec3 &velocity, const char *damageDefName, anEntity *inflictor );

							// model/combat model/ragdoll
	void					SetCombatModel( void );
	anClipModel *			GetCombatModel( void ) const;
	virtual void			LinkCombat( void );
	virtual void			UnlinkCombat( void );
	bool					StartRagdoll( void );
	void					StopRagdoll( void );
	virtual bool			UpdateAnimationControllers( void );

							// delta view angles to allow movers to rotate the view of the actor
	const anAngles &		GetDeltaViewAngles( void ) const;
	void					SetDeltaViewAngles( const anAngles &delta );

	bool					HasEnemies( void ) const;
	anActor *				ClosestEnemyToPoint( const anVec3 &pos, float maxRange=0.0f, bool returnFirst=false, bool checkPVS=false );
	anActor *				EnemyWithMostHealth();

	virtual bool			OnLadder			( void ) const;
	virtual void			OnStateChange		( int channel );
	virtual void			OnFriendlyFire		( anActor* attacker );

	virtual void			GetAASLocation( anSEAS *aas, anVec3 &pos, int &areaNum ) const;

	void					Attach( anEntity *ent );
	anEntity*				FindAttachment( const char *attachmentName );
	void					HideAttachment( const char *attachmentName );
	void					ShowAttachment( const char *attachmentName );
	anEntity*				GetHead() { return head; }

	virtual void			Teleport( const anVec3 &origin, const anAngles &angles, anEntity *destination );

	virtual	renderView_t *	GetRenderView();

	// Animation
	int						PlayAnim				( int channel, const char *name, int blendFrames );
	bool					PlayCycle				( int channel, const char *name, int blendFrames );
	void					IdleAnim				( int channel, const char *name, int blendFrames );
	void					OverrideAnim			( int channel );
	bool					HasAnim					( int channel, const char *name, bool forcePrefix = false );
	int						GetAnim					( int channel, const char *name, bool forcePrefix = false );
	bool					AnimDone				( int channel, int blendFrames );

	// animation state control
	void					UpdateAnimState			( void );
	void					SetAnimState			( int channel, const char *name, int blendFrames = 0, int flags = 0 );
	void					PostAnimState			( int channel, const char *name, int blendFrames = 0, int delay = 0, int flags = 0 );
	void					StopAnimState			( int channel );
	bool					InAnimState				( int channel, const char *name );

	virtual void			SpawnGibs( const anVec3 &dir, const char *damageDefName );


// bdube: added for vehicle
	bool					IsInVehicle ( void ) const;
	rvVehicleController&	GetVehicleController ( void );
	virtual void			GuidedProjectileIncoming( idGuidedProjectile * projectile );

	bool					DebugFilter	(const anCVar& test) const;

	virtual bool			IsCrouching				( void ) const {return false;};

	virtual bool			SkipImpulse( anEntity *ent, int id );

	int						lightningNextTime;
	int						lightningEffects;

protected:
	friend class			idAnimState;

	float					fovDot;				// cos( fovDegrees )
	float					fovCloseDot;		// cos( fovDegreesClose )
	float					fovCloseRange;		// range within to use fovCloseDot
	anVec3					eyeOffset;			// offset of eye relative to physics origin
	anVec3					chestOffset;		// offset of chest relative to physics origin
	anVec3					modelOffset;		// offset of visual model relative to the physics origin

	anAngles				deltaViewAngles;	// delta angles relative to view input angles

	int						pain_debounce_time;	// next time the actor can show pain
	int						pain_delay;			// time between playing pain sound

	anStringList				damageGroups;		// body damage groups
	anList<float>			damageScale;		// damage scale per damage gruop
	bool					inDamageEvent;		// hacky-ass bool to prevent us from starting a new EV_DamageOverTime in our ::Damage

	bool					use_combat_bbox;	// whether to use the bounding box for combat collision

	// joint handles
	jointHandle_t			leftEyeJoint;
	jointHandle_t			rightEyeJoint;
	jointHandle_t			soundJoint;
	jointHandle_t			eyeOffsetJoint;
	jointHandle_t			chestOffsetJoint;
	jointHandle_t			neckJoint;
	jointHandle_t			headJoint;

	anIK_Walk				walkIK;

	anStr					animPrefix;
	anStr					painType;
	anStr					painAnim;

	// blinking
	int						blink_anim;
	int						blink_time;
	int						blink_min;
	int						blink_max;

	idAnimState				headAnim;
	idAnimState				torsoAnim;
	idAnimState				legsAnim;

	anStateThread			stateThread;

	anEntityPtr<idAFAttachment>	head;				// safe pointer to attached head

	bool					disablePain;
	bool					allowEyeFocus;
	bool					finalBoss;

	int						painTime;

	anList<idAttachInfo>	attachments;

	virtual void			Gib( const anVec3 &dir, const char *damageDefName );
	void					CheckDeathObjectives( void );


// bdube: vehicles
	virtual bool			EnterVehicle ( anEntity *vehicle );
	virtual bool			ExitVehicle	 ( bool force = false );


							// removes attachments with "remove" set for when character dies
	void					RemoveAttachments( void );


// bdube: vehicles
	rvVehicleController		vehicleController;
// bdube: flashlights
	renderLight_t			flashlight;
	int						flashlightHandle;
	jointHandle_t			flashlightJoint;
	anVec3					flashlightOffset;

// bdube: death force
	int						deathPushTime;
	anVec3					deathPushForce;
	jointHandle_t			deathPushJoint;

	void					FlashlightUpdate	( bool forceOn = false );
	void					InitDeathPush		( const anVec3 &dir, int location, const anDict* damageDict, float pushScale = 1.0f );
	void					DeathPush			( void );

	// Add some dynamic externals for debugging
	virtual void			GetDebugInfo		( debugInfoProc_t proc, void* userData );


protected:

	virtual void			FootStep			( void );
	virtual void			SetupHead( const char *headDefName = "", anVec3 headOffset = anVec3(0, 0, 0) );

private:
	void					SyncAnimChannels( int channel, int syncToChannel, int blendFrames );
	void					FinishSetup( void );

	void					Event_EnableEyeFocus( void );
	void					Event_DisableEyeFocus( void );
	void					Event_EnableBlink( void );
	void					Event_DisableBlink( void );
	void					Event_Footstep( void );
	void					Event_EnableWalkIK( void );
	void					Event_DisableWalkIK( void );
	void					Event_EnableLegIK( int num );
	void					Event_DisableLegIK( int num );
	void					Event_SetAnimPrefix( const char *name );
	void					Event_LookAtEntity( anEntity *ent, float duration );
	void					Event_PreventPain( float duration );
	void					Event_DisablePain( void );
	void					Event_EnablePain( void );
	void					Event_StopAnim( int channel, int frames );
	void					Event_PlayAnim( int channel, const char *name );
	void					Event_PlayCycle( int channel, const char *name );
	void					Event_IdleAnim( int channel, const char *name );
	void					Event_SetSyncedAnimWeight( int channel, int anim, float weight );
	void					Event_OverrideAnim( int channel );
	void					Event_EnableAnim( int channel, int blendFrames );
	void					Event_SetBlendFrames( int channel, int blendFrames );
	void					Event_GetBlendFrames( int channel );
	void					Event_HasEnemies( void );
	void					Event_NextEnemy( anEntity *ent );
	void					Event_ClosestEnemyToPoint( const anVec3 &pos );
	void					Event_StopSound( int channel, int netsync );
	void					Event_GetHead( void );

	void					Event_Teleport		( anVec3 &newPos, anVec3 &newAngles );
	void					Event_Flashlight	( bool enable );
	void					Event_EnterVehicle	( anEntity *vehicle );
	void					Event_ExitVehicle	( bool force );
	void					Event_PreExitVehicle( bool force );

	void					Event_SetAnimRate	( float multiplier );
	void					Event_DamageOverTime ( int endTime, int interval, anEntity *inflictor, anEntity *attacker, anVec3 &dir, const char *damageDefName, const float damageScale, int location );
	virtual void			Event_DamageOverTimeEffect	( int endTime, int interval, const char *damageDefName );
	void					Event_JointCrawlEffect ( const char *effectKeyName, float crawlSecs );

	CLASS_STATES_PROTOTYPE ( anActor );

protected:

	// Wait states
	stateResult_t			State_Wait_LegsAnim		( const stateParms_t& parms );
	stateResult_t			State_Wait_TorsoAnim	( const stateParms_t& parms );
	stateResult_t			State_Wait_Frame		( const stateParms_t& parms );

	void					DisableAnimState		( int channel );
	void					EnableAnimState			( int channel );
	idAnimState&			GetAnimState			( int channel );
};

inline bool anActor::IsInVehicle( void ) const {
	return vehicleController.IsDriving();
}

inline rvVehicleController& anActor::GetVehicleController( void ) {
	return vehicleController;
}

#endif /* !__GAME_ACTOR_H__ */


