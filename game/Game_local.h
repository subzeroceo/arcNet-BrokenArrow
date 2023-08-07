
// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_LOCAL_H__
#define	__GAME_LOCAL_H__

void gameError( const char *fmt, ... );

#include "physics/Clip.h"
#include "physics/ClipModel.h"
#include "physics/TraceModelCache.h"
#include "physics/Push.h"
#include "Game.h"
#include "Pvs.h"
#include "Entity.h"
#include "PlayerProperties.h"
#include "physics/PhysicsEvent.h"
#include "PlayerView.h"

#include "anim/Anim.h"

#include "gamesys/Pvs.h"

// Gordon: For oob access prevention
template< typename T, const int MAX >
class sdSafeArray {
public:
	inline T& operator[] ( int index ) {
		assert( index >= 0 && index < MAX );
		return data[index];
	}

	inline const T& operator[] ( int index ) const {
		assert( index >= 0 && index < MAX );
		return data[index];
	}

	inline void Memset( int value ) {
		memset( data, value, sizeof( data ) );
	}

private:
	T		data[ MAX ];
};

class sdLoggedTrace : public anClass {
public:
	CLASS_PROTOTYPE( sdLoggedTrace );

							sdLoggedTrace( void );
							~sdLoggedTrace( void );

	void					Init( int index, const trace_t& _trace );
	idScriptObject*			GetScriptObject( void ) { return object; }
	const trace_t*			GetTrace( void ) const { return &trace; }
	int						GetIndex( void ) const { return index; }

	void					Event_GetTraceFraction( void );
	void					Event_GetTraceEndPos( void );
	void					Event_GetTracePoint( void );
	void					Event_GetTraceNormal( void );
	void					Event_GetTraceEntity( void );
	void					Event_GetTraceSurfaceFlags( void );
	void					Event_GetTraceSurfaceType( void );
	void					Event_GetTraceSurfaceColor( void );
	void					Event_GetTraceJoint( void );
	void					Event_GetTraceBody( void );

private:
	idScriptObject*			object;
	trace_t					trace;
	int						index;
};

class sdUIScopeParser {
public:
				sdUIScopeParser( const char *text );

	bool		IsLastEntry( void ) const { return entryIndex == entries.Num(); }
	const char *GetNextEntry( void ) { if ( entryIndex < entries.Num() ) { return entries[ entryIndex++ ]; } return nullptr; }
	void		Revert( void ) { entryIndex--; assert( entryIndex >= 0 ); }

private:
	char							entryText[ 256 ];
	anStaticList< const char*, 32 > entries;
	int								entryIndex;
};

struct userInfo_t {
	void					ToDict( anDict &info ) const;
	void					FromDict( const anDict &info );

	anStr					name;
	anStr					baseName;
	anStr					rawName;
	anStr					cleanName;
	idWStr					wideName;
	bool					showGun;
	bool					ignoreExplosiveWeapons;
	bool					autoSwitchEmptyWeapons;
	bool					postArmFindBestWeapon;
	bool					advancedFlightControls;
	bool					rememberCameraMode;
	bool					drivingCameraFreelook;
	bool					isBot;
	bool					voipReceiveGlobal;
	bool					voipReceiveTeam;
	bool					voipReceiveFireTeam;
	bool					showComplaints;
	bool					swapFlightYawAndRoll;
};

/*
===============================================================================

	Local implementation of the public game interface.

===============================================================================
*/

const int LAGO_IMG_WIDTH	= 64;
const int LAGO_IMG_HEIGHT	= 64;
const int LAGO_WIDTH		= 64;
const int LAGO_HEIGHT		= 44;
#define LAGO_MATERIAL		"textures/sfx/lagometer"
#define LAGO_IMAGE			"textures/sfx/lagometer.tga"

const int MAX_BATTLESENSE_RANKS = 5;

// if set to 1 the server sends the client PVS with snapshots and the client compares against what it sees
#ifndef ASYNC_WRITE_PVS
	#define ASYNC_WRITE_PVS 0
#endif

// the "gameversion" client command will print this plus compile date
#define	GAME_VERSION		"baseETQW-1"

const int NUM_RENDER_PORTAL_BITS	= anMath::BitsForInteger( PS_BLOCK_ALL );

const unsigned short DOF_VIEWWEAPON_VIEW_ID	= 0xFFFD;

// classes used by idGameLocal
class anBasePlayerStart;
class anEntity;
class anActor;
class anBasePlayer;
class idCamera;
class idWorldspawn;
class idTestModel;
class idTypeInfo;
class idEditEntities;
class anRenderWorld;
class idSoundWorld;
class sdGameRules;
class sdDeclMapInfo;
class sdDeclDamage;
class sdTeamInfo;
class idGameLocal;
class sdVehiclePathGrid;
class sdDeployMask;
class sdDeployRequest;
class sdDeployZone;
class idProgram;
class sdKeyCommand;
class sdBindContext;
class sdPlayerStatEntry;

extern anRenderWorld*				gameRenderWorld;
extern idSoundWorld*				gameSoundWorld;

#ifdef _XENON
class LiveManager;					//Forward Declarations to prevent rebuild all
class LiveService;					//downside is anyone who used it must include the header.
extern LiveService*					liveService;
extern LiveManager*					liveManager;
#endif

extern anCVar						com_timescale;
extern anCVar						com_timeServer;

extern anCVar						g_debugPlayerList;
extern anCVar						g_skipLocalizedPrecipitation;


//============================================================================

class sdPlayZone {
public:
	enum playZoneFlags_t {
		PZF_COMMANDMAP		= BITT< 0 >::VALUE,
		PZF_PLAYZONE		= BITT< 1 >::VALUE,
		PZF_DEPLOYMENT		= BITT< 2 >::VALUE,
		PZF_PATH			= BITT< 3 >::VALUE,
		PZF_HEIGHTMAP		= BITT< 4 >::VALUE,
		PZF_VEHICLEROUTE	= BITT< 5 >::VALUE,
		PZF_WORLD			= BITT< 6 >::VALUE,
		PZF_CHOOSABLE		= BITT< 7 >::VALUE,
	};

							sdPlayZone( const anDict &info, const anBounds& bounds );
							~sdPlayZone( void );

	const anVec2&			GetSize( void ) const { return _size; }
	const an2DBounds&		GetBounds( void ) const { return _bounds; }
	int						GetFlags( void ) const { return _flags; }
	const sdDeployMaskInstance*	GetMask( qhandle_t handle ) const;
	int						GetPriority( void ) const { return _priority; }
	const char*				GetTarget( void ) const { return _target; }
	sdVehiclePathGrid*		GetPath( const char *name ) const;
	const sdHeightMapInstance&	GetHeightMap( void ) const { return _heightMap; }

	void					SaveMasks( void );

	const anMaterial*		GetCommandMapMaterial( void ) const { return _commandmapMaterial; }

	const sdDeclLocStr*		GetTitle( void ) const { return _title; }

private:
	an2DBounds				_bounds;
	anVec2					_size;
	anList< sdDeployMaskInstance >	_masks;
	int						_flags;
	int						_priority;
	anStr					_target;
	const anMaterial*		_commandmapMaterial;
	sdHeightMapInstance		_heightMap;
	const sdDeclLocStr*		_title;

	typedef anPair< anStr, sdVehiclePathGrid* > playzonePath_t;
	anList< playzonePath_t >	_paths;
};

/*
============
serverInfo_t
============
*/
struct serverInfo_t {
	int						timeLimit;
	bool					adminStart;
	bool					votingDisabled;
	bool					noProficiency;
	int						minPlayers;
	float					readyPercent;
	bool					gameReviewReadyWait;
};


/*
============
gameDecalInfo_t
============
*/
struct gameDecalInfo_t {
	qhandle_t				renderEntityHandle;
	renderEntity_t			renderEntity;
};

enum radiusPushFlags_t {
	RP_GROUNDONLY = BITT< 0 >::VALUE,
};

/*
============
idSpawnPoint
============
*/
class idSpawnPoint {
public:
									idSpawnPoint( void ) { _lastUsedTime = 0; _relativePosition = true; _parachute = false; _inUse = false; }

	void							Clear( void );

	anEntity*						GetOwner( void ) const;
	const sdRequirementContainer&	GetRequirements( void ) const { return _requirements; }
	const anVec3&					GetOffset( void ) const { return _offset; }
	const anAngles&					GetAngles( void ) const { return _angles; }
	bool							GetRelativePositioning( void ) const { return _relativePosition; }
	int								GetLastUsedTime( void ) const { return _lastUsedTime; }
	void							SetLastUsedTime( int lastTime ) const { _lastUsedTime = lastTime; }

	sdRequirementContainer&			GetRequirements( void ) { return _requirements; }
	void							SetOwner( anEntity *owner );
	void							SetPosition( const anVec3 &offset, const anAngles& angles ) { _offset = offset; _angles = angles; }
	void							SetRelativePositioning( bool value ) { _relativePosition = value; }

	bool							GetParachute( void ) const { return _parachute; }
	void							SetParachute( bool parachute ) { _parachute = parachute; }

	void							SetInUse( bool inUse ) { _inUse = inUse; }
	bool							InUse( void ) const { return _inUse; }

private:
	anEntityPtr< anEntity >			_owner;

	anVec3							_offset;
	anAngles						_angles;

	sdRequirementContainer			_requirements;

	bool							_relativePosition;
	bool							_parachute;

	mutable int						_lastUsedTime;

	bool							_inUse;
};


/*
============
surfaceProperties_t
============
*/
struct surfaceProperties_t {
	surfaceProperties_t() :	friction( 1.0f ) {}
	float friction;
};

/*
===============================================================================

	sdEntityCollection

===============================================================================
*/

class sdEntityCollection {
public:
										sdEntityCollection( void ) { }
										~sdEntityCollection( void ) { }

	void								SetName( const char *_name ) { name = _name; }

	void								Add( anEntity *entity ) { mask.Set( entity->entityNumber ); list.Alloc() = entity; }
	void								Remove( anEntity *entity );
	int									Num( void ) const { return list.Num(); }
	anEntity*							operator[]( int index ) const { return list[index]; }
	const char*							GetName( void ) const { return name; }
	bool								Contains( anEntity *entity ) const { return Contains( entity->entityNumber ); }
	bool								Contains( int entityNumber ) const { return mask[ entityNumber ] != 0; }
	void								Clear( void ) { mask.Clear(); list.SetNum( 0, false ); }

private:
	anStr								name;
	sdBitField< MAX_GENTITIES >			mask;
	anList< anEntityPtr< anEntity > >	list;
};


/*
============
sdEntityNetEvent
============
*/
class sdEntityNetEvent {
public:
	typedef anLinkList< sdEntityNetEvent > nodeType_t;

	static const size_t		MAX_EVENT_PARAM_SIZE = 128;

							sdEntityNetEvent( void );

	void					Create( const anEntity *_entity, int _event, bool _saveEvent, const anBitMsg* _msg );
	void					Create( const sdEntityNetEvent& other );

	void					Read( const anBitMsg& msg );
	void					Write( anBitMsg& msg ) const;
	void					OutputParms( anBitMsg& msg ) const;

	void					Write( anFile *file ) const;
	void					Read( anFile *file );

	int						GetEntityNumber( void ) const { return ( spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 ) ); }
	int						GetId( void ) const { return spawnId >> GENTITYNUM_BITS; }
	int						GetSpawnId( void ) const { return spawnId; }
	int						GetEvent( void ) const { return event; }
	int						GetTime( void ) const { return time; }
	int						GetParamsSize( void ) const { return paramsSize; }
	const byte*				GetParams( void ) const { return paramsBuf; }

	bool					ShouldSaveEvent( void ) const { return saveEvent; }

	int						GetTotalSize( void ) const;

	nodeType_t&				GetNode( void ) { return node; }
	const nodeType_t&		GetNode( void ) const { return node; }

private:
	int						spawnId;
	int						event;
	int						time;
	bool					saveEvent;
	int						paramsSize;
	byte					paramsBuf[ MAX_EVENT_PARAM_SIZE ];

	nodeType_t				node;
};

/*
============
sdUnreliableEntityNetEvent
============
*/
class sdUnreliableEntityNetEvent : public sdEntityNetEvent {
public:
	typedef anLinkList< sdUnreliableEntityNetEvent > unreliableNodeType_t;

									sdUnreliableEntityNetEvent( void );

	void							ClearSent( void );
	void							SetSent( int clientTo );
	bool							GetSent( int clientTo ) const;
#ifdef SD_SUPPORT_REPEATER
	void							SetRepeaterSent( int clientTo );
	bool							GetRepeaterSent( int clientTo ) const;
#endif // SD_SUPPORT_REPEATER
	bool							HasExpired( void ) const;

	unreliableNodeType_t&			GetUnreliableNode( void ) { return unreliableNode; }
	const unreliableNodeType_t&		GetUnreliableNode( void ) const { return unreliableNode; }

private:
	sdBitField< MAX_CLIENTS+1 >		sentClients;
#ifdef SD_SUPPORT_REPEATER
	sdBitField_Dynamic				sentRepeaterClients;
	int								numRepeaterClients;
#endif // SD_SUPPORT_REPEATER

	unreliableNodeType_t			unreliableNode;
};

/*
============
sdEnvDefinition
============
*/
struct sdEnvDefinition {
	anVec3 origin;
	anStr name;
	int size;
};

class sdEntityDebugInfo {
public:
	enum operation_t {
		OP_CREATE,
		OP_DESTROY,
		OP_NONE,
	};
public:
							sdEntityDebugInfo( void );

	void					OnCreate( anEntity *self );
	void					OnDestroy( anEntity *self );

	void					PrintStatus( int index );

	int						lastOperationTime;
	operation_t				lastOperation;
	idTypeInfo*				type;
	anStr					name;
};

struct savedPlayerStat_t {
	const sdDeclRank*		rank;
	const sdTeamInfo*		team;
	anStr					name;
	float					value;
	float					data[ MAX_CLIENTS ];
};

//===============================================================



//============================================================================
class sdUserInterfaceLocal;
class sdUIWindow;

const int NUM_DEPLOY_REQUESTS = MAX_CLIENTS;
const int GUI_GLOBALS_HANDLE = -999;

struct persistentRank_t {
	int						rank;
	bool					calculated;
};

struct lifeStat_t {
	anStr					stat;
	const sdDeclLocStr*		text;			// used for lists
	const sdDeclLocStr*		textLong;		// used for in-game announcements
	bool					isTimeBased;
};

struct quickChatMuteEntry_t {
	anStr					name;
};

class idGameLocal : public idGame {
public:
	anDict					serverInfo;				// all the tunable parameters, like numclients, etc
	serverInfo_t			serverInfoData;
	int						numClients;				// pulled from serverInfo and verified

	// [
	usercmd_t				usercmds[ MAX_CLIENTS ];	// client input commands
	sdDeployRequest*		deployRequests[ NUM_DEPLOY_REQUESTS ];
	bool					clientConnected[ MAX_CLIENTS ];
	persistentRank_t		clientRanks[ MAX_CLIENTS ];
	sdNetStatKeyValList		clientStatsList[ MAX_CLIENTS ];
	anHashIndex				clientStatsHash[ MAX_CLIENTS ];
	int						clientLastBanIndexReceived[ MAX_CLIENTS ];
	bool					clientStatsRequestsPending;

#if !defined( SD_DEMO_BUILD )
	sdNetTask*				clientStatsRequestTask;
	int						clientStatsRequestIndex;
#endif /* !SD_DEMO_BUILD */
	int						clientCompaintCount[ MAX_CLIENTS ];
	anList< sdNetClientId >	clientUniqueComplaints[ MAX_CLIENTS ];
	sdBitField< MAX_CLIENTS >clientMuteMask[ MAX_CLIENTS ]; // Mask of people who have muted this player

	anList< quickChatMuteEntry_t > clientQuickChatMuteList;		// List of players the local client has muted for quickchat
	// ]

	// [
	sdSafeArray< anEntity*, MAX_GENTITIES > entities;// index to entities
	int						spawnIds[ MAX_GENTITIES ];// for use in anEntityPtr
	sdEntityDebugInfo		entityDebugInfo[ MAX_GENTITIES ]; // for debugging ( mainly network ) issues
	// ]

	int						firstFreeIndex;			// first free index in the entities array
	int						numEntities;			// current number <= MAX_GENTITIES
	anHashIndex				entityHash;				// hash table to quickly find entities by name
	idWorldspawn*			world;					// world entity
	anLinkList< anEntity >	spawnedEntities;		// all spawned entities
	anLinkList< anEntity >	networkedEntities;		// all entities that want to send/receive network traffic
	anLinkList< anEntity >	nonNetworkedEntities;	// all entities that dont send/receive network traffic
	anLinkList< anEntity >	activeEntities;			// all thinking entities (anEntity::thinkFlags != 0)
	anLinkList< anEntity >	activeNetworkEntities;	// all thinking entities (anEntity::thinkFlags != 0)
	anLinkList< anEntity >	postThinkEntities;		//
	anList< anEntityPtr< anEntity > > changedEntities;
	bool					sortPushers;			// true if active lists needs to be reordered to place pushers at the front
	bool					sortTeamMasters;		// true if active lists needs to be reordered to place physics team masters before their slaves
	bool					insideExecuteMapChange;

	anTraceModelCache		traceModelCache;

	sdLock						clientEntLock;
	class rvClientEntity*		clientEntities[ MAX_CENTITIES ];	// index to client entities
	int							clientSpawnIds[ MAX_CENTITIES ];	// for use in idClientEntityPtr
	anLinkList<rvClientEntity>	clientSpawnedEntities;			// all client side entities
	int							num_clientEntities;				// current number of client entities
	int							firstFreeClientIndex;			// first free index in the client entities array

	int							mapLoadCount;

	sdPlayerStatEntry*			totalShotsFiredStat;
	sdPlayerStatEntry*			totalShotsHitStat;

	sdPhysicsEvent::nodeType_t	physicsEvents;

	anBasePlayerView				playerView;			// handles damage kicks and effects

	anRandom				random;					// random number generator used throughout the game

	idProgram*				program;				// currently loaded script and data space
	idProgramThread*	frameCommandThread;

	arcClip					clip;					// collision detection
	idPush					push;					// geometric pushing
	idPVS					pvs;					// potential visible set

	sdAORManagerLocal		aorManager;

	sdPersistentRankInfo					rankInfo;
	sdPersistentRankInfo::sdRankInstance	rankScratchInfo;

	idTestModel *			testmodel;				// for development testing of models

	sdGameRules*			rules;
	const sdDeclMapInfo*	mapInfo;
	const anDict*			mapMetaData;
	const anDict			defaultMetaData;
	const sdDeclStringMap*	mapSkinPool;
	qhandle_t				playzoneMask;

	const sdDeclProficiencyItem* battleSenseBonus[ MAX_BATTLESENSE_RANKS ];

	idEditEntities *		editEntities;			// in game editing

	typedef struct targetTimer_s {
		anStr				name;
		int					endTimes[ MAX_CLIENTS ];
		int					serverHandle;
	} targetTimer_t;

	anList< targetTimer_t>	targetTimers;
	anList< int >			targetTimerLookup;
	int						numServerTimers;

	anList< savedPlayerStat_t > endGameStats;
	anList< lifeStat_t >		lifeStats;

	int						frameNum;
	int						startTime;
	int						previousTime;			// time in msec of last frame
	int						time;					// in msec
	int						timeOffset;				//
	int						msec;					// time since last update in milliseconds
	int						localViewChangedTime;	// time that the local player view changed
	int						playerSpawnTime;		// time at which players are allowed to spawn again

	int						nextTeamBalanceCheckTime;

	bool					isServer;				// set if the game is run for a dedicated or listen server
	bool					isClient;				// set if the game is run for a client
	bool					isRepeater;
	bool					serverIsRepeater;
	bool					snapShotClientIsRepeater;
	mutable int				repeaterClientFollowIndex;

	bool					isPaused;
	bool					pauseViewInited;
	anVec3					pauseViewOrg;
	anAngles				pauseViewAngles;
	anAngles				pauseViewAnglesBase;
	int						pauseStartGuiTime;

													// discriminates between the RunFrame path and the ClientPrediction path
													// NOTE: on a listen server, isClient is false
	int						localClientNum;			// number of the local client. MP: -1 on a dedicated
	anLinkList<anEntity>	snapshotEntities;		// entities from the last snapshot
	anLinkList<anEntity>	snapshotVisbileEntities;// entities from the last snapshot that are actually visible
	int						realClientTime;			// real client time
	bool					isNewFrame;				// true if this is a new game frame, not a rerun due to prediction
	bool					predictionUpdateRequired;

	float					flightCeilingLower;
	float					flightCeilingUpper;


	anHashIndex						entityCollectionsHash;
	anList< sdEntityCollection* >	entityCollections;

	sdGUIDFile						guanFile;

	sdPlayerProperties		localPlayerProperties;
	sdGlobalProperties		globalProperties;
	sdLimboProperties		limboProperties;
	sdUpdateManager			updateManager;

	anFile*					proficiencyLog;
	anFile*					networkLog;
	anFile*					objectiveLog;

	float					globalShaderParms[ MAX_GLOBAL_SHADER_PARMS ];

	sdBindContext*			defaultBindContext;

	// fps unlock
	anCVar					*com_unlockFPS;			// shortcut to the core cvar
	anLinkList<anEntity>	interpolateEntities;	// entities marked for interpolation

	struct idUnlock {
		bool					canUnlockFrames;		// tracks when extra view and angles updates are allowed
		bool					unlockedDraw;			// differenciate between doing draws for unlocked frames and regular draws
		anVec3					originlog[2];			// log last two origins
		anAngles				minAngles;
		anAngles				maxAngles;
		anAngles				refAngles;
		int						lastFullDrawFrame;
		bool					doWeapon;
		anVec3					viewOrigin;
		anVec3					weaponOrigin;
		anMat3					weaponAxis;
		anVec3					weaponGUIOrigin;
		anMat3					weaponGUIAxis;
	} unlock;

	// ---------------------- Public idGame Interface -------------------

							idGameLocal();

	const anList< guiScope_t >& GetUIScopes() const { return uiScopes; }

	sdEntityCollection*		GetEntityCollection( const char *name, bool allowCreate = false );
	qhandle_t				GetPlayZoneMask( void ) const { return playzoneMask; }

	void					LoadScript( void );

	void					OnPreMapStart( void );
	void					OnNewMapLoad( const char *mapName );
	void					OnMapStart( void );

	static void				CleanName( anStr& name );

	bool					IsMultiPlayer( void );
	bool					IsMetaDataValidForPlay( const metaDataContext_t& context, bool checkBrowserStatus );

	virtual void			Init( void );
	virtual void			Shutdown( void );
	virtual void			UserInfoChanged( int clientNum );
	virtual bool			ValidateUserInfo( int clientNum, anDict &_userInfo );
	virtual void			SetServerInfo( const anDict &serverInfo );
	void					ParseServerInfo( void );

	void					LoadLifeStatsData( void );

	void					PushChangedEntity( anEntity *ent );

	sdDeployZone*			TerritoryForPoint( const anVec3 &point, sdTeamInfo* team = nullptr, bool requireTeam = false, bool requireActive = false ) const;

	void					MakeRules( void );
	idTypeInfo*				GetRulesType( bool errorOnFail );

	void					ChangeLocalSpectateClient( int spectateeNum );
	int						GetRepeaterFollowClientIndex( void ) const;

#ifdef SD_SUPPORT_REPEATER
	virtual void			RepeaterClientDisconnect( int clientNum );
	virtual void			RepeaterWriteInitialReliableMessages( int clientNum );
	virtual void			RepeaterWriteSnapshot( int clientNum, int sequence, anBitMsg &msg, anBitMsg &ucmdmsg, const repeaterUserOrigin_t& userOrigin, bool clientIsRepeater );
	virtual allowReply_t	RepeaterAllowClient( int numViewers, int maxViewers, const clientNetworkAddress_t& address, const sdNetClientId& netClientId, const char *guid, const char *password, allowFailureReason_t& reason, bool isRepeater );
	virtual void			RepeaterClientBegin( int clientNum );
	virtual void			SetRepeaterState( bool isRepeater );
	virtual bool			RepeaterApplySnapshot( int clientNum, int sequence );
	virtual void			RepeaterProcessReliableMessage( int clientNum, const anBitMsg &msg );
	void					BuildRepeaterInfo( anDict &repeaterInfo );
	void					UpdateRepeaterInfo( void );
	int						GetNumRepeaterClients( void ) const;
	bool					IsRepeaterClientConnected( int clientNum ) const;
	void					ShutdownRepeatersNetworkStates( void );
	void					ShutdownRepeatersNetworkState( int clientNum );
#endif // SD_SUPPORT_REPEATER

	virtual void			InitFromNewMap( const char *mapName, anRenderWorld *renderWorld, idSoundWorld *soundWorld, bool isServer, bool isClient, int randSeed, int startTime, bool isUserChange );
	virtual void			MapShutdown( void );
	void					CacheDictionaryMedia_r( const idTypeInfo* cls, const anDict &dict );
	virtual void			CacheDictionaryMedia( const anDict &dict );
	void					DumpOggSounds();
	void					GetShakeSounds( const anDict &dict );
	virtual void			FinishBuild( void );
	virtual void			SpawnPlayer( int clientNum, bool isBot );
	virtual void			RunFrame( const usercmd_t* clientCmds, int elapsedTime );
	virtual bool			Draw();
	virtual bool			Draw2D();

	bool					IsPaused( void );
	void					SetPaused( bool value );
	void					OnPausedChanged( void );
	void					SendPauseInfo( const sdReliableMessageClientInfoBase& info );
	int						ToGuiTime( int time ) { return time + timeOffset; }
	void					GetPausedView( anVec3 &origin, anMat3 &axis );
	void					UpdatePauseNoClip( usercmd_t& cmd );

	void					SetActionCommand( const char *action );

	sdBindContext*			GetDefaultBindContext( void ) { return defaultBindContext; }

	anBasePlayer*				GetActiveViewer( void ) const;

	virtual void			OnServerShutdown( void );
	virtual allowReply_t	ServerAllowClient( int numClients, int numBots, const clientNetworkAddress_t& address, const sdNetClientId& netClientId, const char *guid, const char *password, allowFailureReason_t& reason );

	void					HandleGuiScriptMessage( anBasePlayer* player, anEntity *entity, const char *message );
	void					HandleNetworkMessage( anBasePlayer* player, anEntity *entity, const char *message );
	void					HandleNetworkEvent( anEntity *entity, const char *message );

	virtual void			PacifierUpdate();

	sdNetworkStateObject&	GetGameStateObject( extNetworkStateMode_t mode );
	void					WriteGameState( extNetworkStateMode_t mode, snapshot_t* snapshot, clientNetworkInfo_t& info, anBitMsg& msg );
	void					ReadGameState( extNetworkStateMode_t mode, snapshot_t* snapshot, const anBitMsg& msg );
	void					ResetGameState( extNetworkStateMode_t mode );

	void					ParseClamp( angleClamp_t &clamp, const char *prefix, const anDict &dict );

	void					FreeGameState( sdGameState* state );
	void					FreeNetworkState( sdEntityState* state );
	void					CreateNetworkState( int entityNum );
	void					CreateNetworkState( clientNetworkInfo_t& networkInfo, int entityNum );
	void					FreeNetworkState( clientNetworkInfo_t& networkInfo, int entityNum );
	void					FreeNetworkState( int entityNum );
	void					ClientSpawn( int entityNum, int spawnId, int typeNum, int entityDefNumber, int mapSpawnId );
	void					OnEntityCreateMessage( const anBitMsg& msg );

	snapshot_t*				AllocateSnapshot( int sequence, clientNetworkInfo_t& nwInfo );
	void					WriteSnapshotGameStates( snapshot_t* snapshot, clientNetworkInfo_t& nwInfo, anBitMsg& msg );
	void					WriteSnapshotEntityStates( snapshot_t* snapshot, clientNetworkInfo_t& nwInfo, int clientNum, anBitMsg& msg, bool useAOR );
	void					WriteSnapshotUserCmds( snapshot_t* snapshot, anBitMsg& msg, int ignoreClientNum, bool useAOR );
	void					WriteInitialReliableMessages( const sdReliableMessageClientInfoBase& target );
	void					HandleNewEntityEvent( const anBitMsg &msg );

	virtual void			ServerClientConnect( int clientNum );
	virtual void			ServerClientBegin( int clientNum, bool isBot );
	virtual void			SetClientNum( int clientNum, bool server );
	virtual void			ServerClientDisconnect( int clientNum );
	virtual void			ServerWriteInitialReliableMessages( int clientNum );
	virtual void			ServerWriteSnapshot( int clientNum, int sequence, anBitMsg &msg, anBitMsg &ucmdmsg );
	virtual bool			ServerApplySnapshot( int clientNum, int sequence );
	virtual void			ServerProcessReliableMessage( int clientNum, const anBitMsg &msg );
	void					EnsureAlloced( int entityNum, anEntity *ent, const char *oldState, const char *currentState );
	void					ClientWriteGameState( anFile *file );
	void					ClientReadGameState( anFile *file );
	bool					SetupClientAoR( void );
	virtual bool			ClientReadSnapshot( int sequence, const int gameFrame, const int gameTime, const int numDuplicatedUsercmds, const int aheadOfServer, const anBitMsg &msg, const anBitMsg &ucmdmsg );
	virtual bool			ClientApplySnapshot( int sequence );
	virtual void			ClientProcessReliableMessage( const anBitMsg &msg );
	virtual void			ClientPrediction( const usercmd_t* clientCmds, const usercmd_t* demoCmd );
	virtual void			OnSnapshotHitch( int snapshotTime );
	virtual void			OnClientDisconnected( void );
	virtual void			ClientUpdateView( const usercmd_t &cmd, int timeLeft );
	virtual void			WriteClientNetworkInfo( anFile *file );
	virtual void			ReadClientNetworkInfo( anFile *file );
	virtual void			WriteUserInfo( anBitMsg& msg, const anDict &info );
	virtual void			ReadUserInfo( const anBitMsg& msg, anDict &info );

	virtual void			CreateStatusResponseDict( const anDict &serverInfo, anDict &statusResponseDict );
	virtual int				GetProbeTime() const;
	virtual byte			GetProbeState() const;
	virtual void			WriteExtendedProbeData( anBitMsg& msg );

	void					LogComplaint( anBasePlayer* player, anBasePlayer* attacker );
	bool					DoSkyCheck( const anVec3 &location ) const;

	virtual bool			HandleGuiEvent( const sdSysEvent* event );
	virtual bool			TranslateGuiBind( const idKey& key, sdKeyCommand** cmd );

	virtual void			ShowMainMenu();
	virtual void			HideMainMenu();
	virtual bool			IsMainMenuActive();
	virtual void			DrawMainMenu();
	virtual void			DrawLoadScreen();
	virtual void			DrawPureWaitScreen();
	virtual void			DrawSystemUI();
	virtual void			GuiFrameEvents( bool outOfSequence );

	virtual void			AddChatLine( const wchar_t* text );

	virtual userMapChangeResult_e OnUserStartMap( const char *text, anStr& reason, anStr& mapName );
	virtual void			ArgCompletion_StartGame( const anCommandArgs& args, argCompletionCallback_t callback );

	virtual void			RunFrame();

	virtual bool			DownloadRequest( const char *IP, const char *guid, const char *paks, char urls[ MAX_STRING_CHARS ] );
	virtual bool			HTTPRequest( const char *IP, const char *file, bool isGamePak );

#ifdef ARC_DEBUG_MEMORY
	virtual void			MemDump( const char *fileName ) { Mem_Dump( fileName ); }
	virtual void			MemDumpCompressed( const char *fileName, memoryGroupType_t memGroup, memorySortType_t memSort, int sortCallStack, int numFrames, bool xlFriendly ) { Mem_DumpCompressed( fileName, memGroup, memSort, sortCallStack, numFrames, xlFriendly ); }
	virtual void			MemDumpPerClass( const char *fileName ) { Mem_DumpPerClass( fileName ); }
#endif

	virtual void				OnInputInit( void );
	virtual void				OnInputShutdown( void );

	virtual void				OnLanguageInit( void );
	virtual void				OnLanguageShutdown( void );

	virtual sdKeyCommand*		Translate( const idKey& key );
	virtual usercmdbuttonType_t	SetupBinding( const char *binding, int& action );
	virtual void				HandleLocalImpulse( int action, bool down );

	virtual int					GetBotFPS( void ) const;
	virtual botDebugInfo_t		GetBotDebugInfo( int clientNum );
	virtual bool				GetRandomBotName( int clientNum, anStr& botName );

	const char*					GetCookieString( const char *value );
	int							GetCookieInt( const char *value );

	void						SetCookieString( const char *key, const char *value );
	void						SetCookieInt( const char *key, int value );

	sdProficiencyTable&			GetProficiencyTable( int entityNumber ) { return proficiencyTables[ entityNumber ]; }
	const sdProficiencyTable&	GetProficiencyTable( int entityNumber ) const { return proficiencyTables[ entityNumber ]; }

	int						ClassCount( const anDeclPlayerClass* pc, anBasePlayer* skip, sdTeamInfo* team );

	void					SetTargetTimer( qhandle_t timerHandle, anBasePlayer* player, int t );
	qhandle_t				AllocTargetTimer( const char *targetName );
	qhandle_t				GetTargetTimerForServerHandle( qhandle_t timerHandle );
	const char*				GetTargetTimerName( qhandle_t timerHandle ) { return targetTimers[ timerHandle ].name; }
	int						GetTargetTimerValue( qhandle_t timerHandle, anBasePlayer* player );
	void					ClearTargetTimers();

	void					StartRecordingDemo( void );
	void					GetDemoName( anStr& output );

	sdDeployRequest*		GetDeploymentRequest( anBasePlayer* player );
	bool					RequestDeployment( anBasePlayer* player, const sdDeclDeployableObject* object, const anVec3 &position, float rotation, int delayMS );
	void					UpdateDeploymentRequests( void );
	void					ClearDeployRequests( void );
	void					ClearDeployRequest( int deployIndex );
	deployResult_t			CheckDeploymentRequestBlock( const anBounds& bounds );

	bool					IsDeveloper( void ) const { return cvarSystem->GetCVarBool( "developer" ); }

	virtual sdUserInterfaceManager* GetUIManager();

	virtual void			MessageBox( msgBoxType_t type, const wchar_t* message, const sdDeclLocStr* title );
	virtual void			CloseMessageBox();

	virtual void				SetUpdateAvailability( updateAvailability_t type );
	virtual void				SetUpdateState( updateState_t state );
	virtual void				SetUpdateFromServer( bool fromServer );
	virtual guiUpdateResponse_t	GetUpdateResponse();
	virtual void				SetUpdateProgress( float percent );
	virtual void				SetUpdateMessage( const wchar_t* text );

	virtual void			AddSystemNotification( const wchar_t* text );

	virtual void			UpdateLevelLoadScreen( const wchar_t* status );


	// bdube: added effect calls
	const rvDeclEffect *			FindEffect( const char *name, bool makeDefault = true );
	virtual rvClientEffect*			PlayEffect			( const int effectHandle, const anVec3 &color, const anVec3 &origin, const anMat3 &axis, bool loop = false, const anVec3 &endOrigin = vec3_origin, float distanceOffset = 0.0f );
	rvClientEffect*					PlayEffect			( const anDict &args, const anVec3 &color, const char *effectName, const char *materialType, const anVec3 &origin, const anMat3 &axis, bool loop = false, const anVec3 &endOrigin = vec3_origin );
	int								GetEffectHandle		( const anDict &args, const char *effectName, const char *materialType );
	int								GetDecal			( const anDict &args, const char *decalName, const char *materialType );

	// jscott: for effects system
	virtual void					StartViewEffect( int type, float time, float scale );
	virtual void					GetPlayerView( anVec3 &origin, anMat3 &axis, float& fovx );
	virtual void					TracePoint( trace_t& trace, const anVec3 &source, const anVec3 &dest, int clipMask );
	virtual void					Translation( trace_t &trace, const anVec3 &source, const anVec3 &dest, const anTraceModel &trm, int clipMask );
	virtual rvClientMoveable*		SpawnClientMoveable( const char *name, int lifetime, const anVec3 &origin, const anMat3 &axis, const anVec3 &velocity, const anVec3 &angular_velocity, int effectSet = 0 );



	void							AddCheapDecal( const anDict &decalDict, anEntity *attachTo, anVec3 &origin, anVec3 &normal, int jointIdx, int id, const char *decalName, const char *materialName );

	virtual bool					ClientsOnSameTeam( int clientNum1, int clientNum2, voiceMode_t voiceMode );
	virtual bool					AllowClientAudio( int clientNum1, voiceMode_t voiceMode );

	// Setup game rules
	void							SetRules( idTypeInfo* type );
	bool							HasMapInfo( void ) const { return mapInfo != nullptr; }
	const sdDeclMapInfo&			GetMapInfo( void ) const { assert( mapInfo ); return *mapInfo; }
	const anDict&					GetMapMetaData( void ) const { assert( mapMetaData ); return *mapMetaData; }

	int								GetNumMapEntities( void ) const { return mapFile == nullptr ? 0 : mapFile->GetNumEntities(); }

	virtual bool					KeyMove( char forward, char right, char up, usercmd_t& cmd );
	virtual void					ControllerMove( bool doGameCallback, const int numControllers, const int* controllerNumbers,
													const float** controllerAxis, anVec3 &viewAngles, usercmd_t& cmd );
	virtual void					MouseMove( const anVec3 &angleBase, anVec3 &angleDelta );

	virtual bool					GetSensitivity( float& scaleX, float& scaleY );

	virtual void					UsercommandCallback( usercmd_t& cmd );

	virtual void					ShowLevelLoadScreen( const char *mapName );
	virtual void					HideLevelLoadScreen( void );

	bool							NextMap( void );
	virtual void					BeginLevelLoad();
	virtual	void					EndLevelLoad();

	void							StartAutorecording();
	void							StopAutorecording();
	void							GetScoreboardShotName( anStr& output );
	void							OnEndGameScoreboardActive();

	virtual void					DrawLCD( sdLogitechLCDSystem* lcd );

	void							MutePlayerLocal( anBasePlayer* player, int clientIndex );
	void							UnMutePlayerLocal( anBasePlayer* player, int clientIndex );
	void							MutePlayerQuickChatLocal( int clientIndex );
	void							UnMutePlayerQuickChatLocal( int clientIndex );
	bool							IsClientQuickChatMuted( anBasePlayer* player );

	void							OnUserNameChanged( anBasePlayer* player, anStr oldName, anStr newName );

	guiHandle_t						LoadUserInterface( const char *name, bool requireUnique, bool permanent, const char *theme = "default", sdHudModule* module = nullptr );
	void							FreeUserInterface( guiHandle_t handle );
	sdUserInterfaceLocal*			GetUserInterface( guiHandle_t handle );
	sdUIWindow*						GetUserInterfaceWindow( const guiHandle_t handle, const char *windowName );
	sdProperties::sdProperty*		GetUserInterfaceProperty( const guiHandle_t handle, const char *propertyName, sdProperties::ePropertyType expectedType );
	sdProperties::sdProperty*		GetUserInterfaceProperty_r( const guiHandle_t handle, const char *propertyName, sdProperties::ePropertyType expectedType );
	sdUserInterfaceScope*			GetGlobalUserInterfaceScope( sdUserInterfaceScope& scope, const char *name );

	sdUserInterfaceScope*			GetUserInterfaceScope( sdUserInterfaceScope& scope, anLexer* src );
	sdUserInterfaceScope*			GetUserInterfaceScope( sdUserInterfaceScope& scope, sdUIScopeParser& src );

	void							SetGUIInt( int handle, const char *name, int value );
	void							SetGUIFloat( int handle, const char *name, float value );
	void							SetGUIVec2( int handle, const char *name, const anVec2& value );
	void							SetGUIVec3( int handle, const char *name, const anVec3 &value );
	void							SetGUIVec4( int handle, const char *name, const anVec4& value );
	void							SetGUIString( int handle, const char *name, const char *value );
	void							SetGUIWString( int handle, const char *name, const wchar_t* value );
	void							SetGUITheme( guiHandle_t handle, const char *theme );
	void							GUIPostNamedEvent( int handle, const char *window, const char *name );
	float							GetGUIFloat( int handle, const char *name );

	void							ResetEntityStates( void );
	void							SetupGameStateBase( clientNetworkInfo_t& networkInfo );
	void							SetupEntityStateBase( clientNetworkInfo_t& networkInfo );

	qhandle_t						GetDeploymentMask( const char *name );

	// playzone manipulation
	void							CreatePlayZone( const anDict &info, const anBounds& bounds );
	void							SavePlayZoneMasks( void );
	void							ClearPlayZones( void );
	void							DebugDeploymentMask( qhandle_t handle );
	void							SetPlayZoneAreaName( int index, const char *name );
	void							LinkPlayZoneAreas( void );

	const sdDeclTargetInfo*			GetMDFExportTargets( void ) { return declTargetInfoType[ "target_mdfExport" ]; }

	// playzone querying
	int								GetWorldPlayZoneIndex( const anVec3 &point ) const;
	const sdPlayZone*				GetPlayZone( const anVec3 &point, int flags ) const;
	const sdPlayZone*				GetChoosablePlayZone( int id ) const;
	int								GetIndexForChoosablePlayZone( const sdPlayZone* zone ) const;
	int								GetNumChoosablePlayZones() const;
	const sdPlayZone*				ClosestPlayZone( const anVec3 &point, float& dist, int flags ) const;

	idSpawnPoint&					RegisterSpawnPoint( anEntity *owner, const anVec3 &offset, const anAngles& angles );
	void							UnRegisterSpawnPoint( idSpawnPoint* point );

	void							RegisterTargetEntity( anLinkList< anEntity >& node );
	void							RegisterIconEntity( anLinkList< anEntity >& node );

	sdEntityState*					AllocEntityState( networkStateMode_t mode, anEntity *ent ) const;
	sdGameState*					AllocGameState( const sdNetworkStateObject& object ) const;

	int								AllocEndGameStat( void );
	void							SetEndGameStatValue( int index, anBasePlayer* player, float value );
	void							SetEndGameStatWinner( int statIndex, anBasePlayer* player );
	void							SendEndGameStats( const sdReliableMessageClientInfoBase& target );
	void							ClearEndGameStats( void );
	void							OnEndGameStatsReceived( void );

public:
	// ---------------------- Public idGameLocal Interface -------------------

	void							Printf( const char *fmt, ... ) const;
	void							DPrintf( const char *fmt, ... ) const;
	void							Warning( const char *fmt, ... ) const;
	void							DWarning( const char *fmt, ... ) const;
	void							Error( const char *fmt, ... ) const;

	void							LoadMap( const char *mapName, int randSeed, int startTime );

	void							OnLocalMapRestart( void );
	void							LocalMapRestart( void );
	void							MapRestart( void );
	static void						MapRestart_f( const anCommandArgs &args );
	static void						NextMap_f( const anCommandArgs &args );
	static void						StartDemos_f( const anCommandArgs &args );

	anBasePlayer*						GetClient( int i ) const { assert( i >= 0 && i < MAX_CLIENTS ); return reinterpret_cast< anBasePlayer *>( entities[i] ); }

	anMapFile *						GetLevelMap( void );
	const char *					GetMapName( void ) const;

	bool							CheatsOk( bool requirePlayer = true );
	gameState_t						GameState( void ) const;

	template< typename T >
	T* SpawnEntityTypeT( bool callPostMapSpawn, const anDict *args = nullptr ) {
		return static_cast< T* >( SpawnEntityType( T::Type, callPostMapSpawn, args ) );
	}
	anEntity *						SpawnEntityType( const idTypeInfo &classdef, bool callPostMapSpawn, const anDict *args = nullptr );
	bool							SpawnEntityDef( const anDict &args, bool callPostMapSpawn, anEntity **ent = nullptr, int entityNum = -1, int mapSpawnId = -1 );
	bool							SpawnClientEntityDef( const anDict &args, rvClientEntity **ent = nullptr, int mapSpawnId = -1 );

	int								GetSpawnId( const anEntity *ent ) const { return ent ? ( ( spawnIds[ ent->entityNumber ] << GENTITYNUM_BITS ) | ent->entityNumber ) : 0; }
	int								SpawnNumForSpawnId( int spawnId ) const { return spawnId >> GENTITYNUM_BITS; }
	int								EntityNumForSpawnId( int spawnId ) const { return spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 ); }
	anEntity*						EntityForSpawnId( int spawnId ) const { int entityNum = EntityNumForSpawnId( spawnId ); return ( ( spawnId >> GENTITYNUM_BITS ) == spawnIds[ entityNum ] ) ? entities[ entityNum ] : nullptr; }

	void							CallSpawnFuncs( anEntity *entity, const anDict *args );
	anEntity*						CreateEntityType( const idTypeInfo &classdef );

	const anDict*					FindEntityDefDict( const char *name, bool makeDefault = false ) const;


	void							RegisterClientEntity( rvClientEntity *cent );
	void							UnregisterClientEntity( rvClientEntity *cent );


	float							RangeSquare( const anEntity *ent1, const anEntity *ent2 ) const	{ return ( ent1->GetPhysics()->GetOrigin() - ent2->GetPhysics()->GetOrigin() ).LengthSqr(); }

	void							ToggleGodMode( anBasePlayer* player ) const;
	void							ToggleNoclipMode( anBasePlayer* player ) const;
	void							NetworkSpawn( const char *classname, anBasePlayer* player );

	void							RegisterEntity( anEntity *ent );
	void							UnregisterEntity( anEntity *ent );

	void							ForceNetworkUpdate( anEntity *ent );

	void							SetCamera( idCamera *cam );
	idCamera *						GetCamera( void ) const;
	void							CalcFov( float base_fov, float &fov_x, float &fov_y, const int width = SCREEN_WIDTH, const int height = SCREEN_HEIGHT, const float correctYAspect = 0.f ) const;

	void							AddEntityToHash( const char *name, anEntity *ent );
	bool							RemoveEntityFromHash( const char *name, anEntity *ent );
	int								GetTargets( const anDict &args, anList< anEntityPtr<anEntity> > &list, const char *ref ) const;

									// returns the master entity of a trace.  for example, if the trace entity is the player's head, it will return the player.
	anEntity *						GetTraceEntity( const trace_t &trace ) const;

	static void						ArgCompletion_EntityName( const anCommandArgs &args, void(*callback)( const char *s ) );
	anEntity*						FindTraceEntity( anVec3 start, anVec3 end, const idTypeInfo &c, const anEntity *skip ) const;
	anEntity*						FindEntity( const char *name ) const;
	anEntity*						FindEntityByType( anEntity *from, const anDeclEntityDef* type ) const;
	anEntity*						FindClassType( anEntity *from, const idTypeInfo& type, anEntity *ignore = nullptr ) const;
	anEntity*						FindClassTypeReverse( anEntity *from, const idTypeInfo& type, anEntity *ignore = nullptr ) const;
	anEntity*						FindClassTypeInRadius( const anVec3 &org, float radius, anEntity *from, const idTypeInfo& type, anEntity *ignore = nullptr ) const;


	template< typename T > T* FindClassTypeT( anEntity *from ) const;

	template< typename T > T* EntityFromRenderEntity( renderEntity_t* entity ) const {
			anEntity *other = EntityForSpawnId( entity->spawnID );
			if ( other == nullptr || !other->IsType( T::Type ) ) {
				return nullptr;
			}

			return static_cast< T* >( other );
	}

	int								EntitiesWithinRadius( const anVec3 org, float radius, anEntity **entityList, int maxCount ) const;
	int								EntitiesOfClass( const char *name, anList< anEntityPtr<anEntity> > &list ) const;

	void							KillBox( anEntity *ent );
	void							RadiusDamage( const anVec3 &origin, anEntity *inflictor, anEntity *attacker, anEntity *ignoreDamage, anEntity *ignorePush, const sdDeclDamage* damage, float dmgPower = 1.f, float radiusScale = 1.f );
	void							RadiusPush( const anVec3 &origin, float radius, const sdDeclDamage* damageDecl, float pushScale, const anEntity *inflictor, const anEntity *ignore, int flags, bool saveEvent );
	void							RadiusPushClipModel( const anVec3 &origin, const float push, const anClipModel *clipModel );
	void							RadiusPushEntities( const anVec3 &origin, float force, float radius );

	void							ProjectDecal( const anVec3 &origin, const anVec3 &dir, float depth, bool parallel, float size, const char *material, float angle = 0 );
	void							CreateProjectedDecal( const anVec3 &origin, const anVec3 &dir, float depth, bool parallel, float width, float height, float angle, const anVec4& color, anRenderModel* model );
	void							ProjectDecal( const anVec3 &origin, const anVec3 &dir, float depth, bool parallel, float size, const anMaterial *material, float angle = 0 );

	void							CallFrameCommand( const idProgram::sdFunction* frameCommand );
	void							CallFrameCommand( idScriptObject* object, const idProgram::sdFunction* frameCommand );
	void							CallObjectFrameCommand( idScriptObject* object, const char *frameCommand, bool allowError );

	const anVec3&					GetGravity( void ) const;
	const anVec3&					GetWindVector( const anVec3 & origin ) const;

	// added the following to assist licensees with merge issues
	int								GetFrameNum() const { return frameNum; }
	int								GetTime() const { return time; }
	int								GetMSec() const { return msec; }

	anBasePlayer*						GetClientByName( const char *name ) const;

	void							OnLocalViewPlayerChanged( void );
	void							OnLocalViewPlayerTeamChanged( void );

	void							ResetTeamAssets( void );

	void							CheckTeamBalance( void );
	sdTeamInfo*						FindUnbalancedTeam( sdTeamInfo** lowest );

	anBasePlayer*						GetLocalViewPlayer( void ) const;
	anBasePlayer*						GetLocalPlayer( void ) const {
		if ( localClientNum == ASYNC_DEMO_CLIENT_INDEX ) {
			return nullptr;
		}
#ifdef SD_SUPPORT_REPEATER
		if ( localClientNum == REPEATER_CLIENT_INDEX ) {
			return nullptr;
		}
#endif // SD_SUPPORT_REPEATER
		return GetClient( localClientNum );
	}

	anBasePlayer*						GetSnapshotPlayer( void ) const { return snapShotPlayer; }
	void							SetSnapShotPlayer( anBasePlayer* player );
	anBasePlayer*						GetSnapshotClient( void ) const { return snapShotClient; }
	void							SetSnapShotClient( anBasePlayer* client );
	void							UpdatePlayerShadows( void );
	const snapshot_t*				GetActiveSnapshot( void ) { return activeSnapshot; }
	bool							IsLocalViewPlayer( const anEntity *player ) const;
	bool							IsLocalPlayer( const anEntity *player ) const { return player->entityNumber == localClientNum; }

	bool							DoClientSideStuff() const;

	void							LogDamage( const char *message );
	void							LogProficiency( const char *message );
	void							LogNetwork( const char *message );
	void							LogObjective( const char *message );
	void							LogDebugText( const anEntity *entFrom, const char *fmt, ... );

	int								GetNumEntityDefBits( void ) const { return numEntityDefBits; }
	int								GetNumDamageDeclBits( void ) const { return numDamageDeclBits; }
	int								GetNumInvItemBits( void ) const { return numInvItemBits; }
	int								GetNumSkinDeclBits( void ) const { return numSkinDeclBits; }
	int								GetNumDeployObjectBits( void ) const { return numDeployObjectBits; }
	int								GetNumPlayerClassBits( void ) const { return numPlayerClassBits; }
	int								GetNumClientIndexBits( void ) const { return numClientIndexBits; }

	bool							SelectInitialSpawnPointForRepeaterClient( anVec3 &outputOrg, anAngles& outputAngles );
	const idSpawnPoint*				SelectInitialSpawnPoint( anBasePlayer *player, anVec3 &outputOrg, anAngles& outputAngles );
	static int						SortSpawnsByAge( const void* a, const void* b );

	void							SetPortalState( qhandle_t portal, int blockingBits );
	void							SaveEntityNetworkEvent( const sdEntityNetEvent& oldEvent );
	void							FreeEntityNetworkEvents( const anEntity *ent, int event );
	void							SendUnreliableEntityNetworkEvent( const anEntity *ent, int event, const anBitMsg *msg );
	void							SendUnreliableEntityNetworkEvent( const sdUnreliableEntityNetEvent& oldEvent );
	void							WriteUnreliableEntityNetEvents( int clientNum, bool repeaterClient, anBitMsg &msg );

	void							SetGlobalMaterial( const anMaterial *mat );
	const anMaterial *				GetGlobalMaterial();

	void							ServerSendQuickChatMessage( anBasePlayer* player, const sdDeclQuickChat* quickChat, anBasePlayer* recipient = nullptr, anEntity *target = nullptr );

#ifdef SD_SUPPORT_REPEATER
	clientNetworkInfo_t&			GetRepeaterNetworkInfo( int clientNum ) {
		assert( repeaterNetworkInfo[ clientNum ] != nullptr );
		return *repeaterNetworkInfo[ clientNum ];
	}

	const clientNetworkInfo_t&		GetRepeaterNetworkInfo( int clientNum ) const {
		assert( repeaterNetworkInfo[ clientNum ] != nullptr );
		return *repeaterNetworkInfo[ clientNum ];
	}
#endif // SD_SUPPORT_REPEATER

	clientNetworkInfo_t&			GetNetworkInfo( int clientNum ) {
		if ( clientNum == ASYNC_DEMO_CLIENT_INDEX ) {
			return demoClientNetworkInfo;
		}
#ifdef SD_SUPPORT_REPEATER
		if ( clientNum == REPEATER_CLIENT_INDEX ) {
			return repeaterClientNetworkInfo;
		}
#endif // SD_SUPPORT_REPEATER
		return clientNetworkInfo[ clientNum ];
	}
	const clientNetworkInfo_t&		GetNetworkInfo( int clientNum ) const  {
		if ( clientNum == ASYNC_DEMO_CLIENT_INDEX ) {
			return demoClientNetworkInfo;
		}
#ifdef SD_SUPPORT_REPEATER
		if ( clientNum == REPEATER_CLIENT_INDEX ) {
			return repeaterClientNetworkInfo;
		}
#endif // SD_SUPPORT_REPEATER
		return clientNetworkInfo[ clientNum ];
	}

	enum {
		EVENT_MAXEVENTS
	};

	int								GetLastMarker( int clientNum, int entityNum ) const { return GetNetworkInfo( clientNum ).lastMarker[ entityNum ]; }

	sdLoggedTrace*					RegisterLoggedTrace( const trace_t& trace );
	void							FreeLoggedTrace( sdLoggedTrace* trace );

	int								RegisterLoggedDecal( const anMaterial* material = nullptr );
	gameDecalInfo_t*				GetLoggedDecal( int handle );
	void							FreeLoggedDecal( int index );
	void							ResetLoggedDecal( int index );

	void							NetworkEventWarning( const sdEntityNetEvent& event, const char *fmt, ... );
	void							UnreliableNetworkEventWarning( const sdUnreliableEntityNetEvent& event, const char *fmt, ... );
	void							ShutdownClientNetworkState( int clientNum );

	static surfaceProperties_t&		GetSurfaceTypeForIndex( int index );

	void							AddEnvDefinition( sdEnvDefinition &envDef ) { envDefs.Append( envDef ); }
	const anList<sdEnvDefinition>&	GetEnvDefinitions( void ) { return envDefs; }

	// GUIs
	const guiHandle_t&				GetMainMenuGui() const { return uiMainMenuHandle; }

	void							ApplyRulesData( const sdEntityStateNetworkData& newState );
	void							ReadRulesData( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const anBitMsg& msg ) const;
	void							WriteRulesData( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, anBitMsg& msg ) const;
	bool							CheckRulesData( const sdEntityStateNetworkData& baseState ) const;
	sdEntityStateNetworkData*		CreateRulesData( void ) const;

#ifndef _XENON
	void							UpdateGameSession( void );

	sdNetManager&					GetSDNet() { return sdnet; }
#endif

	anLinkList< anEntity >*			GetTargetEntities( void ) { return targetEntities.NextNode(); }
	anLinkList< anEntity >*			GetIconEntities( void ) { return iconEntities.NextNode(); }

	//int								GetBytesNeededForMapLoad( void ) const { return bytesNeededForMapLoad; }

	const anCommandArgs&				GetActionArgs( void ) { return actionArgs; }

	void							PurgeAndLoadTeamAssets( const sdDeclStringMap* newPartialLoadTeamAssets );

	const anList< anEntity *>&		GetOcclusionQueryList() const { return occlusionQueryList; }
	qhandle_t						AddEntityOcclusionQuery( anEntity *ent );
	void							FreeEntityOcclusionQuery( anEntity *ent );

	const idWStrList&				GetSystemNotifications() { return systemNotifications; }

	const char*						GetTimeText( void ) {
		sysTime_t time;
		sys->RealTime( &time );
		return va( "%d-%02d-%02d %02d:%02d:%02d", 1900 + time.tm_year, 1 + time.tm_mon, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec );
	}

	sdGameRules*					GetRulesInstance( const char *type ) const;

	bool							IsDoingMapRestart( void ) const { return doingMapRestart; }

	int								GetMaxPrivateClients( void );

	void							EnablePlayerHeadModels( void );
	void							DisablePlayerHeadModels( void );

	void							StartSendingBanList( anBasePlayer* player );
	void							SendBanList( anBasePlayer* player );

private:
//	int								GetBytesNeededForMapLoad( const char *map );
//	void							SetBytesNeededForMapLoad( const char *map, int numBytes );

	void							UpdateLagometer( int aheadOfServer, int numDuplicatedUsercmds );

	void							ReserveClientSlot( const sdNetClientId& netClientId );
	void							CheckForExpiredReservedSlots( void );

	void							UpdateCampaignStats( bool allowMedia );

	static void						SurfaceTypePostParse( anDecl* decl );

	void							LoadMainMenuPartialMedia( bool blocking );
	void							PurgeMainMenuPartialMedia();

	void							ControlBotPopulation( void );
	void							TouchMedia();

private:
	int														nextBotPopulationCheck;

	static anList< surfaceProperties_t >					surfaceTypes;

	idWStrList												systemNotifications;

	const static int										INITIAL_SPAWN_COUNT = 1;

	int														commandmapOverlayIndex;

	anLinkList< anEntity >									targetEntities;
	anLinkList< anEntity >									iconEntities;

	int														numEntityDefBits;		// bits required to store an entity def number
	int														numDamageDeclBits;		//
	int														numInvItemBits;			// bits required to store an inventory item number
	int														numSkinDeclBits;		// bits required to store a skin index
	int														numDeployObjectBits;	//
	int														numPlayerClassBits;		//
	int														numClientIndexBits;		// bits required for a clientnumber

	bool													reloadingSameMap;		// only guaranteed to be correct during map load
	anStr													mapFileName;			// name of the map, empty string if no map loaded
	anMapFile *												mapFile;				// will be nullptr during the game unless in-game editing is used
	bool													doingMapRestart;
	bool													useSimpleEffect;

	anMapFile *												botMapFile;				// the bot's map file info.

	int														spawnCount;

	anStringList												deploymentMasks;
	anList< sdPlayZone* >									playZones;
	anList< sdPlayZone* >									worldPlayZones;
	anList< sdPlayZone* >									choosablePlayZones;

	anEntityPtr< idCamera >									camera;
	const anMaterial *										globalMaterial;			// for overriding everything

	anDict													spawnArgs;				// spawn args used during entity spawning  FIXME: shouldn't be necessary anymore

	anVec3													gravity;				// global gravity vector
	gameState_t												gamestate;				// keeps track of whether we're spawning, shutting down, or normal gameplay

	clientNetworkInfo_t										clientNetworkInfo[ MAX_CLIENTS ];
	clientNetworkInfo_t										demoClientNetworkInfo;

#ifdef SD_SUPPORT_REPEATER
	clientNetworkInfo_t										repeaterClientNetworkInfo;
	anList< clientNetworkInfo_t* >							repeaterNetworkInfo;
#endif // SD_SUPPORT_REPEATER

	sdProficiencyTable										proficiencyTables[ MAX_CLIENTS ];

	anBlockAlloc< snapshot_t, 64 >							snapshotAllocator;

	sdEntityNetEvent::nodeType_t							entityNetEventQueue;
	sdEntityNetEvent::nodeType_t							savedEntityNetEventQueue;
	sdUnreliableEntityNetEvent::unreliableNodeType_t		unreliableEntityNetEventQueue;
	anBlockAlloc< sdEntityNetEvent, 32 >					entityNetEventAllocator;
	anBlockAlloc< sdUnreliableEntityNetEvent, 32 >			unreliableEntityNetEventAllocator;

	anCommandArgs												actionArgs;

	sdNetworkStateObject_Generic< idGameLocal, &idGameLocal::ApplyRulesData, &idGameLocal::ReadRulesData, &idGameLocal::WriteRulesData, &idGameLocal::CheckRulesData, &idGameLocal::CreateRulesData > rulesStateObject;

	anStaticList< idSpawnPoint, 256 >						spawnSpots;
	anList< anList< sdPlayZone* >* >						playZoneAreas;
	anStringList												playZoneAreaNames;
	anBasePlayer*												snapShotPlayer;
	anBasePlayer*												snapShotClient;
	snapshot_t*												activeSnapshot;

	anFile*													damageLogFile;
	anFile*													debugLogFile;

	anDict													newInfo;

	int														bytesNeededForMapLoad;

	anStaticList< sdLoggedTrace*, MAX_LOGGED_TRACES >		loggedTraces;
	anStaticList< gameDecalInfo_t*, MAX_LOGGED_DECALS >		loggedDecals;

	// Defines name + origin of ambient cubemaps, can be rendered out with the makeEnvMaps command
	anList<sdEnvDefinition>									envDefs;

	guiHandle_t												uiMainMenuHandle;
	guiHandle_t												uiSystemUIHandle;
	guiHandle_t												uiLevelLoadHandle;
	guiHandle_t												pureWaitHandle;

	const anMaterial*										lagoMaterial;
	byte													lagometer[ LAGO_IMG_HEIGHT ][ LAGO_IMG_WIDTH ][ 4 ];

#ifndef _XENON
	sdNetManager											sdnet;
#endif

	anStringList												shakeSounds;

	const sdDeclStringMap*									currentPartialLoadTeamAssets;

	static anList< qhandle_t >								taskListHandles;

	anList< guiScope_t >									uiScopes;

	// clients allowed access to the reserved slots, setup when a client gets invited to a session
	struct reservedSlot_t {
		sdNetClientId	netClientId;
		int				time;
	};
	anStaticList< reservedSlot_t, MAX_CLIENTS >				reservedClientSlots;

	anList< anEntity *>										occlusionQueryList;

	enum fireTeamListType_t {
		FIRETEAMLIST_MYFIRETEAM = 0,
		FIRETEAMLIST_MAIN,
		FIRETEAMLIST_JOIN,
		FIRETEAMLIST_INVITE,
		FIRETEAMLIST_KICK,
		FIRETEAMLIST_PROMOTE,
		FIRETEAMLIST_MANAGE
	};

	static const int		FIRETEAMLIST_PAGE_NUM_PLAYERS = 8;

	// keep an instance of each type of game rules around
	// we use these in various places where we need to use virtual functions on
	// an instance that doesn't match the current local rules
	// (eg, the server browser needs to query information from rules running on servers)

	typedef sdHashMapGeneric< anStr, sdGameRules*, sdHashCompareStrCmp > rulesMap_t;
	rulesMap_t rulesCache;

	bool					isAutorecording;
	bool					hasTakenScoreShot;

public:
	void					Clear( void );
							// returns true if the entity shouldn't be spawned at all in this game type or difficulty level
	bool					InhibitEntitySpawn( anDict &spawnArgs );
							// spawn entities from the map file
	void					SpawnMapEntities( void );
							// commons used by init, shutdown, and restart
	void					MapClear( bool clearClients );

	void					UpdateGravity( void );
	void					SortActiveEntityList( void );
	void					ShowTargets( void );
	void					RunDebugInfo( void );

	void					InitScriptForMap( void );

	void					InitConsoleCommands( void );
	void					ShutdownConsoleCommands( void );

	void					UpdateServerRankStats( void );

#if !defined( SD_DEMO_BUILD )
	void					FreeClientStatsRequestTask( void );
	void					FinishClientStatsRequest( void );
	bool					StartClientStatsRequest( int clientIndex );
#endif /* !SD_DEMO_BUILD */

	void					CreateClientStatsHash( int clientIndex );
	void					GetGlobalStatsValueMax( int clientIndex, const char *name, sdPlayerStatEntry::statValue_t& value );
	void					SetupFixedClientRank( int clientIndex );

	const sdDeclRank*		FindRankForLevel( int rankLevel );

	void					UpdateLoggedDecals( void );

	void					InitAsyncNetwork( void );
	void					ShutdownAsyncNetwork( void );
	void					InitLocalClient( int clientNum, bool server );
	void					FreeSnapshotsOlderThanSequence( clientNetworkInfo_t& nwInfo, int sequence );
	bool					ApplySnapshot( clientNetworkInfo_t& nwInfo, int sequence );

	void					ClientProcessEntityNetworkEventQueue( void );
	void					ClientShowSnapshot( int clientNum ) const;
	void					ClientShowAOR( int clientNum ) const;

	bool					ClientReceiveEvent( const anVec3 &origin, int event, int time, const anBitMsg &msg );

							// call after any change to serverInfo. Will update various quick-access flags
	void					UpdateServerInfoFlags( void );

	void					UpdateHudStats( anBasePlayer* player );

	static void				CreateDemoList( sdUIList* list );
	static void				CreateModList( sdUIList* list );
	static void				CreateCrosshairList( sdUIList* list );
	static void				CreateKeyBindingList( sdUIList* list );
	static void				CreateActiveTaskList( sdUIList* list );
	static qhandle_t		GetTaskListHandle( int index );
	static void				CreateVehiclePlayerList( sdUIList* list );
	static void				CreateFireTeamListEntry( sdUIList* list, int index, anBasePlayer* player );
	static void				CreateFireTeamList( sdUIList* list );
	static void				CreateInventoryList( sdUIList* list );
	static void				CreateScoreboardList( sdUIList* list );
	static void				CreatePlayerAdminList( sdUIList* list );
	static void				CreateUserGroupList( sdUIList* list );
	static void				CreateServerConfigList( sdUIList* list );
	static void				CreateVideoModeList( sdUIList* list );
	static void				CreateCampaignList( sdUIList* list );
	static void				CreateMapList( sdUIList* list );
	static void				CreateWeaponSwitchList( sdUIList* list );
	static void				CreateColorList( sdUIList* list );
	static void				CreateSpawnLocationList( sdUIList* list );
	static void				CreateMSAAList( sdUIList* list );
	static void				CreateSoundPlaybackList( sdUIList* list );
	static void				CreateSoundCaptureList( sdUIList* list );

	static void				CreateFireTeamList_MyFireTeam( sdUIList* list );
	static void				CreateFireTeamList_Main( sdUIList* list );
	static void				CreateFireTeamList_Join( sdUIList* list );
	static void				CreateFireTeamList_Invite( sdUIList* list );
	static void				CreateFireTeamList_Kick( sdUIList* list );
	static void				CreateFireTeamList_Promote( sdUIList* list );
	static void				CreateFireTeamList_Manage( sdUIList* list );

	static void				CreateLifeStatsList( sdUIList* list );
	static void				CreatePredictedUpgradesList( sdUIList* list );
	static void				CreateUpgradesReviewList( sdUIList* list );

	static void				GeneratePlayerListForTask( anStr& playerList, const sdPlayerTask* task );
	static int				InsertTask( sdUIList* list, const sdPlayerTask* task, bool highlightActive );

	static void				TestGUI_f( const anCommandArgs& args );
	static void				ListClientEntities_f( const anCommandArgs& args );

	void					SetupMapMetaData( const char *mapName );

public:
	anDeclTypeTemplate< anDeclModelDef,			&declModelDefInfo >			declModelDefType;
	anDeclTypeTemplate< anDecl,					&declExportDefInfo >		declExportDefType;
	anDeclTypeTemplate< sdDeclVehicleScript,	&declVehicleScriptDefInfo >	declVehicleScriptDefType;
	anDeclTypeTemplate< sdDeclAmmoType,			&declAmmoTypeInfo >			declAmmoTypeType;
	anDeclTypeTemplate< sdDeclInvSlot,			&declInvSlotInfo >			declInvSlotType;
	anDeclTypeTemplate< anInventoryItemType,		&declInvItemTypeInfo >		declInvItemTypeType;
	anDeclTypeTemplate< anInventoryItem,			&declInvItemInfo >			declInvItemType;
	anDeclTypeTemplate< sdDeclItemPackage,		&declItemPackageInfo >		declItemPackageType;
	anDeclTypeTemplate< sdDeclStringMap,		&declStringMapInfo >		declStringMapType;
	anDeclTypeTemplate< sdDeclDamage,			&declDamageInfo >			declDamageType;
	anDeclTypeTemplate< sdDeclDamageFilter,		&declDamageFilterInfo >		declDamageFilterType;
	anDeclTypeTemplate< sdDeclCampaign,			&declCampaignInfo >			declCampaignType;
	anDeclTypeTemplate< sdDeclQuickChat,		&declQuickChatInfo >		declQuickChatType;
	anDeclTypeTemplate< sdDeclMapInfo,			&declMapInfoInfo >			declMapInfoType;
	anDeclTypeTemplate< sdDeclToolTip,			&declToolTipInfo >			declToolTipType;
	anDeclTypeTemplate< sdDeclTargetInfo,		&declTargetInfoInfo >		declTargetInfoType;
	anDeclTypeTemplate< sdDeclProficiencyType,	&declProficiencyTypeInfo >	declProficiencyTypeType;
	anDeclTypeTemplate< sdDeclProficiencyItem,	&declProficiencyItemInfo >	declProficiencyItemType;
	anDeclTypeTemplate< sdDeclRank,				&declRankInfo >				declRankType;
	anDeclTypeTemplate< sdDeclDeployableObject,	&declDeployableObjectInfo >	declDeployableObjectType;
	anDeclTypeTemplate< sdDeclDeployableZone,	&declDeployableZoneInfo >	declDeployableZoneType;
	anDeclTypeTemplate< anDeclPlayerClass,		&declPlayerClassInfo >		declPlayerClassType;
	anDeclTypeTemplate< sdDeclKeyBinding,		&declKeyBindingInfo >		declKeyBindingType;

	anDeclTypeTemplate< sdDeclGUI,				&declGUIInfo >				declGUIType;
	anDeclTypeTemplate< sdDeclGUITheme,			&declGUIThemeInfo >			declGUIThemeType;

	anDeclTypeTemplate< sdDeclStringMap,		&declTeamInfoInfo >			declTeamInfoType;
	anDeclTypeTemplate< sdDeclPlayerTask,		&declPlayerTaskInfo >		declPlayerTaskType;
	anDeclTypeTemplate< sdDeclRequirement,		&declRequirementInfo >		declRequirementType;

	anDeclTypeTemplate< sdDeclVehiclePath,		&declVehiclePathInfo >		declVehiclePathType;
	anDeclTypeTemplate< sdDeclRadialMenu,		&declRadialMenuInfo >		declRadialMenuType;
	anDeclTypeTemplate< sdDeclAOR,				&declAreaOfRelevanceInfo >	declAORType;
	anDeclTypeTemplate< sdDeclRating,			&declRatingInfo >			declRatingType;

	anDeclTypeTemplate< sdDeclHeightMap,		&declHeightMapInfo >		declHeightMapType;
	anDeclTypeTemplate< sdDeclDeployMask,		&declDeployMaskInfo >		declDeployMaskType;

	sdDeclWrapperTemplate< anDeclTable >		declTableType;
	sdDeclWrapperTemplate< anMaterial >			declMaterialType;
	sdDeclWrapperTemplate< anDeclSkin >			declSkinType;
	sdDeclWrapperTemplate< idSoundShader >		declSoundShaderType;
	sdDeclWrapperTemplate< anDeclEntityDef >	declEntityDefType;
	sdDeclWrapperTemplate< anDeclAF >			declAFType;
	sdDeclWrapperTemplate< class rvDeclEffect >	declEffectsType;
	sdDeclWrapperTemplate< sdDeclAtmosphere >	declAtmosphereType;
	sdDeclWrapperTemplate< sdDeclStuffType >	declStuffTypeType;
	sdDeclWrapperTemplate< sdDeclDecal >		declDecalType;
	sdDeclWrapperTemplate< sdDeclSurfaceType >	declSurfaceTypeType;

	anMetaDataList*						mapMetaDataList;
	anMetaDataList*						campaignMetaDataList;

	static anCVar								g_cacheDictionaryMedia;
};

/*
===============
idGameLocal::ProjectDecal
===============
*/
inline void idGameLocal::ProjectDecal( const anVec3 &origin, const anVec3 &dir, float depth, bool parallel, float size, const char *material, float angle ) {
	ProjectDecal( origin, dir, depth, parallel, size, declHolder.declMaterialType.LocalFind( material ), angle );
}


// bdube: inlines
inline rvClientEffect* idGameLocal::PlayEffect ( const anDict &args, const anVec3 &color, const char *effectName, const char *materialType, const anVec3 &origin, const anMat3 &axis, bool loop, const anVec3 &endOrigin ) {
	return PlayEffect ( GetEffectHandle ( args, effectName, materialType ), color, origin, axis, loop, endOrigin );
}

//============================================================================

extern idGameLocal			gameLocal;
extern anAnimManager		animationLib;

//============================================================================

class idGameError : public arcException {
public:
	idGameError( const char *text ) : arcException( text ) {}
};

//============================================================================

//
// these defines work for all startsounds from all entity types
// make sure to change script/defs.script and def/sound.def if you add any channels, or change their order
//
typedef enum {
	SND_ANY = SCHANNEL_ANY,
	SND_VOICE = SCHANNEL_ONE,
	SND_BODY,
	SND_DOOR,
	SND_ITEM,
	SND_MUSIC,

	SND_ENGINE = SND_ANY + 10,
	SND_ENGINE_LAST = ( SND_ENGINE + /* MAX ENGINE SOUNDS */ 8 ) - 1,

	SND_WALKER_LEGS = SND_ANY + 20,
	SND_WALKER_LEGS_LAST = ( SND_WALKER_LEGS + /* MAX WALKER LEG SOUNDS */ 8 ) - 1,

	// quickchat starts at a crazy number just so that if MAX_CLIENTS
	// is increased then it won't collide with other channels
	SND_QUICKCHAT = SND_ANY + 1000,
	SND_QUICKCHAT_LAST = ( SND_QUICKCHAT + MAX_CLIENTS ) - 1,

	//
	// Player sounds
	SND_PLAYER = SND_ANY + 50,
	SND_PLAYER_JUMP,
	SND_PLAYER_LAND,
	SND_PLAYER_HURT,
	SND_PLAYER_DEATH,
	SND_PLAYER_CHAT,
	SND_PLAYER_VO,
	SND_PLAYER_VO_MCP_UPDATE,
	SND_PLAYER_TARGETLOCK,
	SND_PLAYER_TOOLTIP,
	SND_PLAYER_FALL,
	SND_PLAYER_MOVE,
	SND_PLAYER_ALARM,

	//
	// Weapon sounds
	SND_WEAPON = SND_ANY + 100,
	SND_WEAPON_BRASS,
	SND_WEAPON_FIRE,
	SND_WEAPON_FIRE2,
	SND_WEAPON_FIRE3,
	SND_WEAPON_FIRE_FAR,
	SND_WEAPON_FIRE_LOCAL,
	SND_WEAPON_RELOAD,
	SND_WEAPON_COCK,
	SND_WEAPON_RAISE,
	SND_WEAPON_LOWER,
	SND_WEAPON_SIG,
	SND_WEAPON_MECH,
	SND_WEAPON_MOVE,
	SND_WEAPON_FOLEY,
	SND_WEAPON_BOUNCE,
	SND_WEAPON_ARM,
	SND_WEAPON_DISARM,
	SND_WEAPON_REMOVE,
	SND_WEAPON_DEPLOY,
	SND_WEAPON_IDLE,
	SND_WEAPON_MODE,
	SND_WEAPON_DRYFIRE,
	SND_WEAPON_DEPLOY_ROTATION,
	SND_WEAPON_DEPLOY_MISC,

	//
	// Vehicle sounds
	SND_VEHICLE = SND_ANY + 200,
	SND_VEHICLE_IDLE,
	SND_VEHICLE_START,
	SND_VEHICLE_STOP,
	SND_VEHICLE_BRAKE,
	SND_VEHICLE_ENTER,
	SND_VEHICLE_EXIT,
	SND_VEHICLE_POWERUP,
	SND_VEHICLE_POWERDOWN,
	SND_VEHICLE_DEATH,
	SND_VEHICLE_DISABLE,
	SND_VEHICLE_OVERDRIVE,
	SND_VEHICLE_SKID,
	SND_VEHICLE_MISC,
	SND_VEHICLE_RADIO,
	SND_VEHICLE_ALARM,
	SND_VEHICLE_OFFROAD,
	SND_VEHICLE_DRIVE,
	SND_VEHICLE_DRIVE2,
	SND_VEHICLE_DRIVE3,
	SND_VEHICLE_DRIVE4,
	SND_VEHICLE_DRIVE5,
	SND_VEHICLE_WALK,
	SND_VEHICLE_WALK2,
	SND_VEHICLE_WALK3,
	SND_VEHICLE_WALK4,
	SND_VEHICLE_WALK5,
	SND_VEHICLE_WALK6,
	SND_VEHICLE_HORN,
	SND_VEHICLE_REV,
	SND_VEHICLE_JUMP,
	SND_VEHICLE_ZOOM,

	//
	// Vehicle interior sounds
	SND_VEHICLE_INTERIOR = SND_ANY + 250,
	SND_VEHICLE_INTERIOR_IDLE,
	SND_VEHICLE_INTERIOR_START,
	SND_VEHICLE_INTERIOR_STOP,
	SND_VEHICLE_INTERIOR_POWERUP,
	SND_VEHICLE_INTERIOR_POWERDOWN,
	SND_VEHICLE_INTERIOR_OFFROAD,
	SND_VEHICLE_INTERIOR_DRIVE,
	SND_VEHICLE_INTERIOR_DRIVE2,
	SND_VEHICLE_INTERIOR_DRIVE3,
	SND_VEHICLE_INTERIOR_DRIVE4,
	SND_VEHICLE_INTERIOR_DRIVE5,
	SND_VEHICLE_INTERIOR_OVERDRIVE,
	SND_VEHICLE_INTERIOR_LOWHEALTH,

	//
	// Deployable sounds
	SND_DEPLOYABLE = SND_ANY + 300,
	SND_DEPLOYABLE_BRASS,
	SND_DEPLOYABLE_FIRE,
	SND_DEPLOYABLE_FIRE2,
	SND_DEPLOYABLE_FIRE3,
	SND_DEPLOYABLE_FIRE_FAR,
	SND_DEPLOYABLE_RELOAD,
	SND_DEPLOYABLE_SIG,
	SND_DEPLOYABLE_MECH,
	SND_DEPLOYABLE_IDLE,
	SND_DEPLOYABLE_DEATH,
	SND_DEPLOYABLE_DEPLOY,
	SND_DEPLOYABLE_DEPLOY2,
	SND_DEPLOYABLE_DEPLOY3,
	SND_DEPLOYABLE_DEPLOY4,
	SND_DEPLOYABLE_DEPLOY5,
	SND_DEPLOYABLE_YAW,
	SND_DEPLOYABLE_PITCH,

	//
	// Structure sounds
	SND_STRUCTURE = SND_ANY + 400,
	SND_STRUCTURE_IDLE,
	SND_STRUCTURE_POWERUP,
	SND_STRUCTURE_POWERDOWN,
	SND_STRUCTURE_ALARM,
	SND_STRUCTURE_DEPLOY,
	SND_STRUCTURE_DEPLOY2,
	SND_STRUCTURE_DEPLOY3,
	SND_STRUCTURE_DEPLOY4,
	SND_STRUCTURE_DEPLOY5,
	SND_STRUCTURE_SPAWNLOCATION,
	SND_STRUCTURE_CAPTUREPOINT,

	//
	// Motor sounds for vehicles and other script entities
	SND_MOTOR = SND_ANY + 450,
	SND_MOTOR_LAST = ( SND_MOTOR + 50 ) - 1,

	//
	// Mover sounds
	SND_MOVER = SND_ANY + 500,
	SND_MOVER_IDLE,
	SND_MOVER_POWERUP,
	SND_MOVER_POWERDOWN,
	SND_MOVER_ALARM,
	SND_MOVER_START,
	SND_MOVER_MOVE,
	SND_MOVER_STOP,

	SND_LAST
} gameSoundChannel_t;

//============================================================================

// TTimo: those can't be inlined in the class declaration, as gameLocal is not defined yet
inline anEntity*	idSpawnPoint::GetOwner( void ) const { return _owner.GetEntity(); }
inline void		idSpawnPoint::SetOwner( anEntity *owner ) { _owner = owner; }

#include "EntityPtr.h"
#include "client/ClientEntityInlines.h"

#include "Player.h"
#include "ScriptEntity.h"

#endif	/* !__GAME_LOCAL_H__ */
