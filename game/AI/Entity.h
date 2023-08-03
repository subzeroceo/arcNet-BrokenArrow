
#ifndef __GAME_ENTITY_H__
#define __GAME_ENTITY_H__

/*
===============================================================================

	Game entity base class.

===============================================================================
*/

static const int DELAY_DORMANT_TIME = 3000;

extern const anEventDef EV_PostSpawn;
extern const anEventDef EV_FindTargets;
extern const anEventDef EV_Touch;
extern const anEventDef EV_Use;
extern const anEventDef EV_Activate;
extern const anEventDef EV_ActivateTargets;
extern const anEventDef EV_Hide;
extern const anEventDef EV_Show;
extern const anEventDef EV_GetShaderParm;
extern const anEventDef EV_SetShaderParm;
extern const anEventDef EV_SetOwner;
extern const anEventDef EV_GetAngles;
extern const anEventDef EV_SetAngles;
extern const anEventDef EV_SetLinearVelocity;
extern const anEventDef EV_SetAngularVelocity;
extern const anEventDef EV_SetSkin;
extern const anEventDef EV_StartSoundShader;
extern const anEventDef EV_StopSound;
extern const anEventDef EV_CacheSoundShader;


extern const anEventDef EV_CallFunction;
extern const anEventDef EV_SetGuiParm;
extern const anEventDef EV_SetGuiFloat;
extern const anEventDef EV_ClearSkin;
// bdube: more global events for anEntity
extern const anEventDef EV_GetFloatKey;
extern const anEventDef EV_HideSurface;
extern const anEventDef EV_ShowSurface;
extern const anEventDef EV_GuiEvent;
extern const anEventDef EV_StopAllEffects;
extern const anEventDef EV_PlayEffect;
extern const anEventDef EV_Earthquake;
extern const anEventDef EV_GuiEvent;
extern const anEventDef EV_SetKey;
// jscott:
extern const anEventDef EV_PlaybackCallback;
// jshepard:
extern const anEventDef EV_UnbindTargets;
// twhitaker:
extern const anEventDef EV_ApplyImpulse;


class idGuidedProjectile;

// Think flags
enum {
	TH_ALL					= -1,
	TH_THINK				= 1,		// run think function each frame
	TH_PHYSICS				= 2,		// run physics each frame
	TH_ANIMATE				= 4,		// update animation each frame
	TH_UPDATEVISUALS		= 8,		// update renderEntity
};

//
// Signals
// make sure to change script/doom_defs.script if you add any, or change their order
//
typedef enum {
	SIG_TOUCH,				// object was touched
	SIG_USE,				// object was used
	SIG_TRIGGER,			// object was activated
	SIG_REMOVED,			// object was removed from the game
	SIG_DAMAGE,				// object was damaged
	SIG_BLOCKED,			// object was blocked

	SIG_MOVER_POS1,			// mover at position 1 (door closed)
	SIG_MOVER_POS2,			// mover at position 2 (door open)
	SIG_MOVER_1TO2,			// mover changing from position 1 to 2
	SIG_MOVER_2TO1,			// mover changing from position 2 to 1


// kfuller: added signals
	// WARNING: these entries are mirrored in scripts/defs.script so make sure if they move
	//          here that they move there as well
	SIG_REACHED,			// object reached it's rotation/motion destination


	NUM_SIGNALS
} signalNum_t;

// FIXME: At some point we may want to just limit it to one thread per signal, but
// for now, I'm allowing multiple threads.  We should reevaluate this later in the project
#define MAX_SIGNAL_THREADS 16		// probably overkill, but anList uses a granularity of 16

struct signal_t {
	int					threadnum;
	const function_t	*function;
};

class signalList_t {
public:
	anList<signal_t> signal[ NUM_SIGNALS ];
};

class anEntity : public anClass {
public:
	static const int		MAX_PVS_AREAS = 4;

	int						entityNumber;			// index into the entity list
	int						entityDefNumber;		// index into the entity def list

	anLinkList<anEntity>	spawnNode;				// for being linked into spawnedEntities list
	anLinkList<anEntity>	activeNode;				// for being linked into activeEntities list

	anLinkList<anEntity>	snapshotNode;			// for being linked into snapshotEntities list
	int						snapshotSequence;		// last snapshot this entity was in
	int						snapshotBits;			// number of bits this entity occupied in the last snapshot

	anString					name;					// name of entity
	anDict					spawnArgs;				// key/value pairs used to spawn and initialize entity
	idScriptObject			scriptObject;			// contains all script defined data for this entity

	int						thinkFlags;				// TH_? flags
	int						dormantStart;			// time that the entity was first closed off from player
	bool					cinematic;				// during cinematics, entity will only think if cinematic is set

	renderView_t *			renderView;				// for camera views from this entity
	anEntity *				cameraTarget;			// any remoteRenderMap shaders will use this

	anList< anEntityPtr<anEntity> >	targets;		// when this entity is activated these entities entity are activated

	int						health;					// FIXME: do all objects really need health?


// ddynerman: optional pre-prediction
	int						predictTime;
// bdube: client entities
	anLinkList<rvClientEntity>	clientEntities;

// rjohnson: will now draw entity info for long thinkers
	int						mLastLongThinkTime;
	anVec4					mLastLongThinkColor;


	struct entityFlags_s {
		bool				notarget			:1;	// if true never attack or target this entity
		bool				noknockback			:1;	// if true no knockback from hits
		bool				takedamage			:1;	// if true this entity can be damaged
		bool				hidden				:1;	// if true this entity is not visible
		bool				bindOrientated		:1;	// if true both the master orientation is used for binding
		bool				solidForTeam		:1;	// if true this entity is considered solid when a physics team mate pushes entities
		bool				forcePhysicsUpdate	:1;	// if true always update from the physics whether the object moved or not
		bool				selected			:1;	// if true the entity is selected for editing
		bool				neverDormant		:1;	// if true the entity never goes dormant
		bool				isDormant			:1;	// if true the entity is dormant
		bool				hasAwakened			:1;	// before a monster has been awakened the first time, use full PVS for dormant instead of area-connected
		bool				networkSync			:1; // if true the entity is synchronized over the network


// bdube: added
		bool				networkStale		:1; // was in the snapshot but isnt anymore
// bgeisler: added block
		bool				triggerAnim			:1;
		bool				usable				:1;	// if true the entity is usable by the player
// cdr: Obstacle Avoidance
		bool				isAIObstacle		:1; // if true, this entity will add itself to the obstacles list in each aas area it touches



// ddynerman: exclude this entity from instance-purging
	// twhitaker: moved variable to be within the bit flags
		bool				persistAcrossInstances	:1;
// twhitaker: exited vehicle already?
		bool				exitedVehicle			:1;
// twhitaker: blinking
		bool				allowAutoBlink			:1;
// jshepard: instant burnout when destroyed
		bool				quickBurn				:1;


	} fl;

public:
	ABSTRACT_PROTOTYPE( anEntity );

							anEntity();
							~anEntity();

	void					Spawn( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	const char *			GetEntityDefName( void ) const;
	void					SetName( const char *name );
	const char *			GetName( void ) const;
	virtual void			UpdateChangeableSpawnArgs( const anDict *source );

							// clients generate views based on all the player specific options,
							// cameras have custom code, and everything else just uses the axis orientation
	virtual renderView_t *	GetRenderView();

	// thinking
	virtual void			Think( void );
	bool					CheckDormant( void );	// dormant == on the active list, but out of PVS
	virtual	void			DormantBegin( void );	// called when entity becomes dormant
	virtual	void			DormantEnd( void );		// called when entity wakes from being dormant
	bool					IsActive( void ) const;
	void					BecomeActive( int flags );
	void					BecomeInactive( int flags );
	void					UpdatePVSAreas( const anVec3 &pos );


// abahr:
	bool					IsActive( int flags ) const { return (flags & thinkFlags ) > 0; }
	const char*				GetEntityDefClassName() const { return spawnArgs.GetString( "classname" ); }
	bool					IsEntityDefClass( const char* className ) const { return !anString::Icmp(className, GetEntityDefClassName()); }
	virtual void			GetPosition( anVec3& origin, anMat3& axis ) const;
// kfuller: added methods
	virtual void			GetLocalAngles(anAngles &localAng);


	// visuals
	virtual void			Present( void );
	// instance visuals
	virtual void			InstanceJoin( void );
	virtual void			InstanceLeave( void );

// bdube: removed virtual so it could be inlined
	renderEntity_t *		GetRenderEntity( void );
	int						GetModelDefHandle( void );

	virtual void			SetModel( const char *modelname );
	void					SetSkin( const idDeclSkin *skin );
	const idDeclSkin *		GetSkin( void ) const;


// bdube: surfaces
	void					HideSurface ( const char* surface );
	void					ShowSurface ( const char* surface );
	void					ClearSkin( void );


	void					SetShaderParm( int parmnum, float value );
	virtual void			SetColor( float red, float green, float blue );
	virtual void			SetColor( const anVec3 &color );
	virtual void			GetColor( anVec3 &out ) const;
	virtual void			SetColor( const anVec4 &color );
	virtual void			GetColor( anVec4 &out ) const;
	virtual void			FreeModelDef( void );
	virtual void			FreeLightDef( void );
	virtual void			Hide( void );
	virtual void			Show( void );
	bool					IsHidden( void ) const;
	void					UpdateVisuals( void );
	void					UpdateModel( void );

// abahr: added virtual to UpdateModelTransform
	virtual
	void					UpdateModelTransform( void );
	virtual void			UpdateRenderEntityCallback();
	virtual const idAnimator *	GetAnimator( void ) const { return nullptr; }	// returns animator object used by this entity

	virtual void			ProjectOverlay( const anVec3 &origin, const anVec3 &dir, float size, const char *material );
	int						GetNumPVSAreas( void );
	const int *				GetPVSAreas( void );
	void					ClearPVSAreas( void );
	bool					PhysicsTeamInPVS( pvsHandle_t pvsHandle );

	// animation
	virtual bool			UpdateAnimationControllers( void );
	bool					UpdateRenderEntity( renderEntity_s *renderEntity, const renderView_t *renderView );
	static bool				ModelCallback( renderEntity_s *renderEntity, const renderView_t *renderView );
	virtual idAnimator *	GetAnimator( void );	// returns animator object used by this entity

	// sound
	virtual bool			CanPlayChatterSounds( void ) const;
	bool					StartSound( const char *soundName, const s_channelType channel, int soundShaderFlags, bool broadcast, int *length );
	bool					StartSoundShader( const idSoundShader *shader, const s_channelType channel, int soundShaderFlags, bool broadcast, int *length );
	void					StopSound( const s_channelType channel, bool broadcast );	// pass SND_CHANNEL_ANY to stop all sounds
	void					SetSoundVolume( float volume );
	void					UpdateSound( void );
	int						GetListenerId( void ) const;

	int						GetSoundEmitter( void ) const;

	void					FreeSoundEmitter( bool immediate );


// bdube: added effect functions
	// effects
	rvClientEffect*			PlayEffect		( const char* effectName, jointHandle_t joint, const anVec3& originOffset, const anMat3& axisOffset, bool loop = false, const anVec3& endOrigin = vec3_origin, bool broadcast = false, effectCategory_t category = EC_IGNORE, const anVec4& effectTint = vec4_one );
	rvClientEffect*			PlayEffect		( const idDecl *effect, jointHandle_t joint, const anVec3& originOffset, const anMat3& axisOffset, bool loop = false, const anVec3& endOrigin = vec3_origin, bool broadcast = false, effectCategory_t category = EC_IGNORE, const anVec4& effectTint = vec4_one );
	rvClientEffect*			PlayEffect		( const char* effectName, jointHandle_t joint, bool loop = false, const anVec3& endOrigin = vec3_origin, bool broadcast = false, effectCategory_t category = EC_IGNORE, const anVec4& effectTint = vec4_one );

	rvClientEffect*			PlayEffect		( const char* effectName, const anVec3& origin, const anMat3& axis, bool loop = false, const anVec3& endOrigin = vec3_origin, bool broadcast = false, effectCategory_t category = EC_IGNORE, const anVec4& effectTint = vec4_one );
	rvClientEffect*			PlayEffect		( const idDecl *effect, const anVec3& origin, const anMat3& axis, bool loop = false, const anVec3& endOrigin = vec3_origin, bool broadcast = false, effectCategory_t category = EC_IGNORE, const anVec4& effectTint = vec4_one );
	void					StopEffect		( const char* effectName, bool destroyParticles = false );
	void					StopEffect		( const idDecl *effect, bool destroyParticles = false );
	void					StopAllEffects	( bool destroyParticles = false );
	void					UpdateEffects	( void );

	float					DistanceTo		( anEntity* ent );
	float					DistanceTo		( const anVec3& pos ) const;
	float					DistanceTo2d	( anEntity* ent );
	float					DistanceTo2d	( const anVec3& pos ) const;

	virtual bool			CanTakeDamage	( void ) const;


	// entity binding
	virtual void			PreBind( void );
	virtual void			PostBind( void );
	virtual void			PreUnbind( void );
	virtual void			PostUnbind( void );
	void					JoinTeam( anEntity *teammember );
	void					Bind( anEntity *master, bool orientated );
	void					BindToJoint( anEntity *master, const char *jointname, bool orientated );
	void					BindToJoint( anEntity *master, jointHandle_t jointnum, bool orientated );
	void					BindToBody( anEntity *master, int bodyId, bool orientated );
	void					Unbind( void );
	bool					IsBound( void ) const;

// abahr: added const so it can be called from const functions
	bool					IsBoundTo( const anEntity *master ) const;

	anEntity *				GetBindMaster( void ) const;
	jointHandle_t			GetBindJoint( void ) const;
	int						GetBindBody( void ) const;
	anEntity *				GetTeamMaster( void ) const;
	anEntity *				GetNextTeamEntity( void ) const;

// abahr: added virtual
	virtual
	void					ConvertLocalToWorldTransform( anVec3 &offset, anMat3 &axis );

	anVec3					GetLocalVector( const anVec3 &vec ) const;
	anVec3					GetLocalCoordinates( const anVec3 &vec ) const;
	anVec3					GetWorldVector( const anVec3 &vec ) const;
	anVec3					GetWorldCoordinates( const anVec3 &vec ) const;

	virtual	bool			GetMasterPosition( anVec3 &masterOrigin, anMat3 &masterAxis ) const;
	void					GetWorldVelocities( anVec3 &linearVelocity, anVec3 &angularVelocity ) const;

	// physics
							// set a new physics object to be used by this entity
	void					SetPhysics( anPhysics *phys );
							// get the physics object used by this entity
	anPhysics *				GetPhysics( void ) const;
							// restore physics pointer for save games
	void					RestorePhysics( anPhysics *phys );
							// run the physics for this entity
	bool					RunPhysics( void );
							// set the origin of the physics object (relative to bindMaster if not nullptr )
	void					SetOrigin( const anVec3 &org );
							// set the axis of the physics object (relative to bindMaster if not nullptr )
	void					SetAxis( const anMat3 &axis );
							// use angles to set the axis of the physics object (relative to bindMaster if not nullptr )
	void					SetAngles( const anAngles &ang );
							// get the floor position underneath the physics object
	bool					GetFloorPos( float max_dist, anVec3 &floorpos ) const;
							// retrieves the transformation going from the physics origin/axis to the visual origin/axis
	virtual bool			GetPhysicsToVisualTransform( anVec3 &origin, anMat3 &axis );
							// retrieves the transformation going from the physics origin/axis to the sound origin/axis
	virtual bool			GetPhysicsToSoundTransform( anVec3 &origin, anMat3 &axis );
							// called from the physics object when colliding, should return true if the physics simulation should stop
	virtual bool			Collide( const trace_t &collision, const anVec3 &velocity );
	virtual bool			Collide( const trace_t &collision, const anVec3 &velocity, bool &hitTeleporter ) { hitTeleporter = false; return Collide(collision, velocity); }
							// retrieves impact information, 'ent' is the entity retrieving the info
	virtual void			GetImpactInfo( anEntity *ent, int id, const anVec3 &point, impactInfo_t *info );
							// apply an impulse to the physics object, 'ent' is the entity applying the impulse
	virtual void			ApplyImpulse( anEntity *ent, int id, const anVec3 &point, const anVec3 &impulse, bool splash = false );
							// add a force to the physics object, 'ent' is the entity adding the force
	virtual void			AddForce( anEntity *ent, int id, const anVec3 &point, const anVec3 &force );
							// activate the physics object, 'ent' is the entity activating this entity
	virtual void			ActivatePhysics( anEntity *ent );
							// returns true if the physics object is at rest
	virtual bool			IsAtRest( void ) const;
							// returns the time the physics object came to rest
	virtual int				GetRestStartTime( void ) const;
							// add a contact entity
	virtual void			AddContactEntity( anEntity *ent );
							// remove a touching entity
	virtual void			RemoveContactEntity( anEntity *ent );


// kfuller: added blocked methods
	virtual void			LastBlockedBy( intblockedEntNum) {}
	virtual int				GetLastBlocker(void) { return -1; }
// rjohnson: moved entity info out of idGameLocal into its own function
	virtual void			DrawDebugEntityInfo( anBounds *viewBounds = 0, anBounds *viewTextBounds = 0, anVec4 *overrideColor = 0 );
// nmckenzie: Adding ability for non-Actors to be enemies for AI characters.  Rename this function at some point, most likely.
	virtual anVec3			GetEyePosition( void ) const { return GetPhysics()->GetOrigin(); }
// abahr:
	virtual bool			SkipImpulse( anEntity *ent, int id );
	virtual void			ApplyImpulse( anEntity* ent, int id, const anVec3& point, const anVec3& dir, const anDict* damageDef );


	// damage

	// twhitaker:			// Sets the damage enitty
	void					SetDamageEntity ( anEntity * forward ) { forwardDamageEnt = forward; }

	// bdube: added ignore entity
							// Returns the entity that should take damage for this entity
	virtual anEntity*		GetDamageEntity ( void );

							// returns true if this entity can be damaged from the given origin
	virtual bool			CanDamage( const anVec3 &origin, anVec3 &damagePoint, anEntity* ignoreEnt = nullptr ) const;

							// applies damage to this entity
	virtual	void			Damage( anEntity *inflictor, anEntity *attacker, const anVec3 &dir, const char *damageDefName, const float damageScale, const int location );
							// adds a damage effect like overlays, blood, sparks, debris etc.
	virtual void			AddDamageEffect( const trace_t &collision, const anVec3 &velocity, const char *damageDefName, anEntity* inflictor );
	virtual bool			CanPlayImpactEffect ( anEntity* attacker, anEntity* target );
							// callback function for when another entity recieved damage from this entity.  damage can be adjusted and returned to the caller.
	virtual void			DamageFeedback( anEntity *victim, anEntity *inflictor, int &damage );
							// notifies this entity that it is in pain
	virtual bool			Pain( anEntity *inflictor, anEntity *attacker, int damage, const anVec3 &dir, int location );
							// notifies this entity that is has been killed
	virtual void			Killed( anEntity *inflictor, anEntity *attacker, int damage, const anVec3 &dir, int location );

	// scripting
	virtual bool			ShouldConstructScriptObjectAtSpawn( void ) const;
	virtual anThread *		ConstructScriptObject( void );
	virtual void			DeconstructScriptObject( void );
	void					SetSignal( signalNum_t signalnum, anThread *thread, const function_t *function );
	void					ClearSignal( anThread *thread, signalNum_t signalnum );
	void					ClearSignalThread( signalNum_t signalnum, anThread *thread );
	bool					HasSignal( signalNum_t signalnum ) const;
	void					Signal( signalNum_t signalnum );
	void					SignalEvent( anThread *thread, signalNum_t signalnum );

	// gui
	void					TriggerGuis( void );
	bool					HandleGuiCommands( anEntity *entityGui, const char *cmds );
	virtual bool			HandleSingleGuiCommand( anEntity *entityGui, anLexer *src );

	// targets

// abahr: made virtual
	virtual
	void					FindTargets( void );
	virtual
	void					RemoveNullTargets( void );
	virtual
	void					ActivateTargets( anEntity *activator ) const;
// jshepard: unbind targets
	void					UnbindTargets( anEntity *activator ) const;



// twhitaker: Add to the list of targets (usually from script)
	int						AppendTarget( anEntity *appendMe );
	void					RemoveTarget( anEntity *removeMe );
	void					RemoveTargets( bool destroyContents );


	// misc
	virtual void			Teleport( const anVec3 &origin, const anAngles &angles, anEntity *destination );
	bool					TouchTriggers( const idTypeInfo* ownerType = nullptr ) const;
	anCurve_Spline<anVec3> *GetSpline( void ) const;
	virtual void			ShowEditingDialog( void );

	enum {
		EVENT_STARTSOUNDSHADER,
		EVENT_STOPSOUNDSHADER,

// bdube: new events
		EVENT_PLAYEFFECT,
		EVENT_PLAYEFFECT_JOINT,
		EVENT_STOPEFFECT,

		EVENT_MAXEVENTS
	};

	virtual void			ClientPredictionThink( void );
	virtual void			WriteToSnapshot( anBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const anBitMsgDelta &msg );
	virtual bool			ServerReceiveEvent( int event, int time, const anBitMsg &msg );
	virtual bool			ClientReceiveEvent( int event, int time, const anBitMsg &msg );

	// the entity was not in the snapshot sent by server. means it either went out of PVS, or was deleted server side
	// return true if the entity wishes to be deleted locally
	// depending on the entity, understanding stale as just another state, or a removal is best
	// ( by default, anEntity keeps the entity around after a little cleanup )
	virtual bool			ClientStale( void );
	virtual void			ClientUnstale( void );


	void					WriteBindToSnapshot( anBitMsgDelta &msg ) const;
	void					ReadBindFromSnapshot( const anBitMsgDelta &msg );
	void					WriteColorToSnapshot( anBitMsgDelta &msg ) const;
	void					ReadColorFromSnapshot( const anBitMsgDelta &msg );
	void					WriteGUIToSnapshot( anBitMsgDelta &msg ) const;
	void					ReadGUIFromSnapshot( const anBitMsgDelta &msg );

	void					ServerSendEvent( int eventId, const anBitMsg *msg, bool saveEvent, int excludeClient ) const;
	void					ServerSendInstanceEvent( int eventId, const anBitMsg *msg, bool saveEvent, int excludeClient ) const;
	void					ClientSendEvent( int eventId, const anBitMsg *msg ) const;


// bdube: debugging
	virtual void			GetDebugInfo( debugInfoProc_t proc, void* userData );
// mwhitlock: memory profiling
	virtual size_t			Size( void ) const;
// ddynerman: multiple arenas (for MP)
	virtual void			SetInstance( int newInstance );
	virtual int				GetInstance( void ) const;
// ddynerman: multiple clip world support
	virtual int				GetClipWorld( void ) const;
	virtual void			SetClipWorld( int newCW );
// scork: accessors so sound editor can indicate current-highlit ent
	virtual int				GetRefSoundShaderFlags( void ) const;
	virtual void			SetRefSoundShaderFlags( int iFlags );
// twhitaker: guided projectiles
	virtual void			GuidedProjectileIncoming( idGuidedProjectile * projectile ) { }


protected:
	renderEntity_t			renderEntity;						// used to present a model to the renderer
	int						modelDefHandle;						// handle to static renderer model
	refSound_t				refSound;							// used to present sound to the audio engine
	anEntityPtr< anEntity >	forwardDamageEnt;					// damage applied to the invoking object will be forwarded to this entity
	anEntityPtr< anEntity > bindMaster;							// entity bound to if unequal nullptr
	jointHandle_t			bindJoint;							// joint bound to if unequal INVALID_JOINT
private:
	anPhysics_Static		defaultPhysicsObj;					// default physics object
	anPhysics *				physics;							// physics used for this entity
	int						bindBody;							// body bound to if unequal -1
	anEntity *				teamMaster;							// master of the physics team
	anEntity *				teamChain;							// next entity in physics team

	int						numPVSAreas;						// number of renderer areas the entity covers
	int						PVSAreas[MAX_PVS_AREAS];			// numbers of the renderer areas the entity covers

	signalList_t *			signals;

	int						mpGUIState;							// local cache to avoid systematic SetStateInt

// abahr: changed to protected for access in children classes
protected:
// ddynerman: multiple game instances
	int						instance;
// ddynerman: multiple collision worlds
	int						clipWorld;



// bdube: made virtual
	virtual bool			DoDormantTests( void );				// dormant == on the active list, but out of PVS


	// physics
							// initialize the default physics
	void					InitDefaultPhysics( const anVec3 &origin, const anMat3 &axis );
							// update visual position from the physics
	void					UpdateFromPhysics( bool moveBack );

	// entity binding
	bool					InitBind( anEntity *master );		// initialize an entity binding
	void					FinishBind( void );					// finish an entity binding
	void					RemoveBinds( void );				// deletes any entities bound to this object
	void					QuitTeam( void );					// leave the current team

	void					UpdatePVSAreas( void );


// bdube: client entities
	void					RemoveClientEntities ( void );		// deletes any client entities bound to this object


	// events
	void					Event_GetName( void );
	void					Event_SetName( const char *name );
	void					Event_FindTargets( void );
	void					Event_ActivateTargets( anEntity *activator );
	void					Event_NumTargets( void );
	void					Event_GetTarget( float index );
	void					Event_RandomTarget( const char *ignore );
	void					Event_Bind( anEntity *master );
	void					Event_BindPosition( anEntity *master );
	void					Event_BindToJoint( anEntity *master, const char *jointname, float orientated );
	void					Event_Unbind( void );
	void					Event_RemoveBinds( void );
	void					Event_SpawnBind( void );
	void					Event_SetOwner( anEntity *owner );
	void					Event_SetModel( const char *modelname );
	void					Event_SetSkin( const char *skinname );
	void					Event_GetShaderParm( int parmnum );
	void					Event_SetShaderParm( int parmnum, float value );
	void					Event_SetShaderParms( float parm0, float parm1, float parm2, float parm3 );
	void					Event_SetColor( float red, float green, float blue );
	void					Event_GetColor( void );
	void					Event_IsHidden( void );
	void					Event_Hide( void );
	void					Event_Show( void );
	void					Event_CacheSoundShader( const char *soundName );
	void					Event_StartSoundShader( const char *soundName, int channel );
	void					Event_StopSound( int channel, int netSync );
	void					Event_StartSound( const char *soundName, int channel, int netSync );
	void					Event_FadeSound( int channel, float to, float over );
	void					Event_GetWorldOrigin( void );
	void					Event_SetWorldOrigin( anVec3 const &org );
	void					Event_GetOrigin( void );
	void					Event_SetOrigin( const anVec3 &org );
	void					Event_GetAngles( void );
	void					Event_SetAngles( const anAngles &ang );
	void					Event_SetLinearVelocity( const anVec3 &velocity );
	void					Event_GetLinearVelocity( void );
	void					Event_SetAngularVelocity( const anVec3 &velocity );
	void					Event_GetAngularVelocity( void );
	void					Event_SetSize( const anVec3 &mins, const anVec3 &maxs );
	void					Event_GetSize( void );
	void					Event_GetMins( void );
	void					Event_GetMaxs( void );
	void					Event_Touches( anEntity *ent );
	void					Event_SetGuiParm( const char *key, const char *val );
	void					Event_SetGuiFloat( const char *key, float f );
	void					Event_GetNextKey( const char *prefix, const char *lastMatch );
	void					Event_SetKey( const char *key, const char *value );
	void					Event_GetKey( const char *key );
	void					Event_GetIntKey( const char *key );
	void					Event_GetFloatKey( const char *key );
	void					Event_GetVectorKey( const char *key );
	void					Event_GetEntityKey( const char *key );
	void					Event_RestorePosition( void );
	void					Event_UpdateCameraTarget( void );
	void					Event_DistanceTo( anEntity *ent );
	void					Event_DistanceToPoint( const anVec3 &point );
	void					Event_StartFx( const char *fx );
	void					Event_WaitFrame( void );
	void					Event_Wait( float time );
	void					Event_HasFunction( const char *name );
	void					Event_CallFunction( const char *name );
	void					Event_SetNeverDormant( int enable );


// kfuller: added events
	void					Event_SetContents				( int contents );
	void					Event_GetLastBlocker			( anThread *thread );
// begisler: added
	void					Event_ClearSkin					( void );
// bdube: effect events
	void					Event_PlayEffect				( const char* effectName, const char* boneName, bool loop );
	void					Event_StopEffect				( const char* effectName );
	void					Event_StopAllEffects			( void );
	void					Event_GetHealth					( void );
// bdube: mesh events
	void					Event_ShowSurface				( const char* surface );
	void					Event_HideSurface				( const char* surface );
// bdube: gui events
	void					Event_GuiEvent					( const char* eventName );
// jscott:
	void					Event_PlaybackCallback			( int type, int changed, int impulse );
// nmckenzie: get bind master
	void					Event_GetBindMaster				( void );
	void					Event_ApplyImpulse				( anEntity *source, const anVec3 &point, const anVec3 &impulse );
// jshepard: unbind all targets
	void					Event_UnbindTargets				( anEntity *activator);
// abahr:
	void					Event_RemoveNullTargets			();
	void					Event_IsA						( const char* entityDefName );
	void					Event_IsSameTypeAs				( const anEntity* ent );
	void					Event_MatchPrefix				( const char *prefix, const char* previousKey );
	void					Event_ClearTargetList			( float destroyContents );
// twhitaker: added - to add targets from script
	void					Event_AppendTarget				( anEntity *appendMe );
	void					Event_RemoveTarget				( anEntity *removeMe );
// mekberg: added
	void					Event_SetHealth					( float newHealth );

};


// bdube: added inlines
ARC_INLINE rvClientEffect* anEntity::PlayEffect( const char* effectName, const anVec3& origin, const anMat3& axis, bool loop, const anVec3& endOrigin,
												 bool broadcast, effectCategory_t category, const anVec4& effectTint ) {
	return PlayEffect( gameLocal.GetEffect( spawnArgs, effectName ), origin, axis, loop, endOrigin, broadcast, category, effectTint );
}

ARC_INLINE rvClientEffect* anEntity::PlayEffect( const char* effectName, jointHandle_t jointHandle, bool loop, const anVec3& endOrigin,
												bool broadcast, effectCategory_t category, const anVec4& effectTint ) {
	return PlayEffect( gameLocal.GetEffect( spawnArgs, effectName ), jointHandle, vec3_origin, mat3_identity, loop, endOrigin, broadcast, category, effectTint );
}

ARC_INLINE rvClientEffect* anEntity::PlayEffect( const char* effectName, jointHandle_t jointHandle, const anVec3& originOffset, const anMat3& axisOffset, bool loop, const anVec3& endOrigin,
												bool broadcast, effectCategory_t category, const anVec4& effectTint ) {
	return PlayEffect( gameLocal.GetEffect( spawnArgs, effectName ), jointHandle, originOffset, axisOffset, loop, endOrigin, broadcast, category, effectTint );
}


ARC_INLINE anPhysics *anEntity::GetPhysics( void ) const {
	return physics;
}

ARC_INLINE renderEntity_t *anEntity::GetRenderEntity( void ) {
	return &renderEntity;
}

ARC_INLINE int anEntity::GetModelDefHandle( void ) {
	return modelDefHandle;
}

// scork: accessors so sound editor can indicate current-highlit ent
ARC_INLINE int anEntity::GetRefSoundShaderFlags( void ) const
{
	return refSound.parms.soundShaderFlags;
}

ARC_INLINE void anEntity::SetRefSoundShaderFlags( int iFlags )
{
	refSound.parms.soundShaderFlags = iFlags;
}




/*
===============================================================================

	Animated entity base class.

===============================================================================
*/

typedef struct damageEffect_s {
	jointHandle_t			jointNum;
	anVec3					localOrigin;
	anVec3					localNormal;
	int						time;

// jscott: not using
//	const idDeclParticle*	type;

	struct damageEffect_s *	next;
} damageEffect_t;

class anAnimatedEntity : public anEntity {
public:
	CLASS_PROTOTYPE( anAnimatedEntity );

							anAnimatedEntity();
							~anAnimatedEntity();

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	virtual void			ClientPredictionThink( void );
	virtual void			Think( void );

	void					UpdateAnimation( void );

	virtual idAnimator *	GetAnimator( void );
	virtual void			SetModel( const char *modelname );

	bool					GetJointWorldTransform( jointHandle_t jointHandle, int currentTime, anVec3 &offset, anMat3 &axis );
	bool					GetJointTransformForAnim( jointHandle_t jointHandle, int animNum, int currentTime, anVec3 &offset, anMat3 &axis ) const;

	virtual int				GetDefaultSurfaceType( void ) const;
	virtual void			AddDamageEffect( const trace_t &collision, const anVec3 &velocity, const char *damageDefName, anEntity* inflictor );
	virtual void			ProjectHeadOverlay( const anVec3 &point, const anVec3 &dir, float size, const char *decal ) {}

	virtual bool			ClientReceiveEvent( int event, int time, const anBitMsg &msg );


// abahr:
	virtual const idAnimator *	GetAnimator( void ) const { return &animator; }
	virtual void			UpdateRenderEntityCallback();


	enum {
		EVENT_ADD_DAMAGE_EFFECT = anEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

protected:
	idAnimator				animator;
	damageEffect_t *		damageEffects;


// jshepard:
	void					Event_ClearAnims				( void );


private:
	void					Event_GetJointHandle( const char *jointname );
	void 					Event_ClearAllJoints( void );
	void 					Event_ClearJoint( jointHandle_t jointnum );
	void 					Event_SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const anVec3 &pos );
	void 					Event_SetJointAngle( jointHandle_t jointnum, jointModTransform_t transform_type, const anAngles &angles );
	void 					Event_GetJointPos( jointHandle_t jointnum );
	void 					Event_GetJointAngle( jointHandle_t jointnum );


// bdube: programmer controlled joint events
	void					Event_SetJointAngularVelocity	( const char* jointName, float pitch, float yaw, float roll, int blendTime );
	void					Event_CollapseJoints			( const char* jointnames, const char* collapseTo );




};


void UpdateGuiParms( anUserInterface *gui, const anDict *args );


ARC_INLINE float anEntity::DistanceTo ( anEntity* ent ) {
	return DistanceTo ( ent->GetPhysics()->GetOrigin() );
}

ARC_INLINE float anEntity::DistanceTo ( const anVec3& pos ) const {
	return (pos - GetPhysics()->GetOrigin()).LengthFast();
}

ARC_INLINE float anEntity::DistanceTo2d ( anEntity* ent ) {
	return DistanceTo2d ( ent->GetPhysics()->GetOrigin() );
}

ARC_INLINE bool anEntity::CanTakeDamage ( void ) const {
	return fl.takedamage;
}


// ddynerman: MP arena stuff
ARC_INLINE int anEntity::GetInstance( void ) const {
	return instance;
}

// ddynerman: multiple collision worlds
ARC_INLINE int anEntity::GetClipWorld( void ) const {
	return clipWorld;
}

ARC_INLINE void anEntity::SetClipWorld( int newCW ) {
	clipWorld = newCW;
	if ( GetPhysics() ) {
		GetPhysics()->UnlinkClip();
		GetPhysics()->LinkClip();
	}
}

#endif /* !__GAME_ENTITY_H__ */
