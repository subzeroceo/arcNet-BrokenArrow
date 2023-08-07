#ifndef __GAME_H__
#define __GAME_H__

/*
===============================================================================

	Public game interface with methods to run the game.

===============================================================================
*/


// bgeisler: moved into scripts directory
// default scripts
#define SCRIPT_DEFAULTDEFS			"scripts/defs.script"
#define SCRIPT_DEFAULT				"scripts/main.script"

#define SCRIPT_DEFAULTFUNC			"doom_main"

struct gameReturn_t {
	char		sessionCommand[MAX_STRING_CHARS];	// "map", "disconnect", "victory", etc
	int			consistencyHash;					// used to check for network game divergence
	int			health;
	int			heartRate;
	int			stamina;
	int			combat;
	bool		syncNextGameFrame;					// used when cinematics are skipped to prevent session from simulating several game frames to
													// keep the game time in sync with real time
};

enum allowReply_t {
	ALLOW_YES = 0,
	ALLOW_BADPASS,	// core will prompt for password and connect again
	ALLOW_NOTYET,	// core will wait with transmitted message
	ALLOW_NO		// core will abort with transmitted message
};

enum escReply_t {
	ESC_IGNORE = 0,	// do nothing
	ESC_MAIN,		// start main menu GUI
	ESC_GUI			// set an explicit GUI
};

enum demoState_t {
	DEMO_NONE,
	DEMO_RECORDING,
	DEMO_PLAYING
};

enum demoReliableGameMessage_t {
	DEMO_RECORD_CLIENTNUM,
	DEMO_RECORD_EXCLUDE,
	DEMO_RECORD_COUNT
};

//
// these defines work for all startsounds from all entity types
// make sure to change script/doom_defs.script if you add any channels, or change their order
//
typedef enum {
	SND_CHANNEL_ANY = SCHANNEL_ANY,
	SND_CHANNEL_VOICE = SCHANNEL_ONE,
	SND_CHANNEL_VOICE2,
	SND_CHANNEL_BODY,
	SND_CHANNEL_BODY2,
	SND_CHANNEL_BODY3,
	SND_CHANNEL_WEAPON,
	SND_CHANNEL_ITEM,
	SND_CHANNEL_HEART,
	SND_CHANNEL_DEMONIC,
	SND_CHANNEL_RADIO,

	// internal use only.  not exposed to script or framecommands.
	SND_CHANNEL_AMBIENT,
	SND_CHANNEL_DAMAGE


// bdube: added custom to tell us where the end of the predefined list is
	,
	SND_CHANNEL_POWERUP,
	SND_CHANNEL_POWERUP_IDLE,
	SND_CHANNEL_MP_ANNOUNCER,
	SND_CHANNEL_CUSTOM

} gameSoundChannel_t;


// bdube: forward reference
class rvClientEffect;


struct ClientStats_t {
	bool	isLastPredictFrame;
	bool	isLagged;
	bool	isNewFrame;
};

typedef struct userOrigin_s {
	anVec3	origin;
	int		followClient;
} userOrigin_t;

class idGame {
public:
	virtual						~idGame() {}

	// Initialize the game for the first time.

// jsinger: attempt to eliminate cross-DLL allocation issues
#ifdef RV_UNIFIED_ALLOCATOR
	virtual void				Init( void *(*allocator)( size_t size ), void (*deallocator)( void *ptr ), size_t (*msize)( void *ptr ) ) = 0;
#else
	virtual void				Init( void ) = 0;
#endif

	// Shut down the entire game.
	virtual void				Shutdown( void ) = 0;

	// Set the local client number. Distinguishes listen ( == 0 ) / dedicated ( == -1 )
	virtual void				SetLocalClient( int clientNum ) = 0;

	// Sets the user info for a client.
	// The game can modify the user info in the returned dictionary pointer, server will forward back.
	virtual const anDict *		SetUserInfo( int clientNum, const anDict &userInfo, bool isClient ) = 0;

	// Retrieve the game's userInfo dict for a client.
	virtual const anDict *		GetUserInfo( int clientNum ) = 0;

	// Sets the user info for a viewer.
	// The game can modify the user info in the returned dictionary pointer.
	virtual const anDict *		RepeaterSetUserInfo( int clientNum, const anDict &userInfo ) = 0;

	// Checks to see if a client is active
	virtual bool				IsClientActive( int clientNum ) = 0;

	// The game gets a chance to alter userinfo before they are emitted to server.
	virtual void				ThrottleUserInfo( void ) = 0;

	// Sets the serverinfo at map loads and when it changes.
	virtual void				SetServerInfo( const anDict &serverInfo ) = 0;

	// The session calls this before moving the single player game to a new level.
	virtual const anDict &		GetPersistentPlayerInfo( int clientNum ) = 0;

	// The session calls this right before a new level is loaded.
	virtual void				SetPersistentPlayerInfo( int clientNum, const anDict &playerInfo ) = 0;

	// Loads a map and spawns all the entities.
	virtual void				InitFromNewMap( const char *mapName, anRenderWorld *renderWorld, bool isServer, bool isClient, int randseed ) = 0;

	// Loads a map from a savegame file.
	virtual bool				InitFromSaveGame( const char *mapName, anRenderWorld *renderWorld, anFile *saveGameFile ) = 0;

	// Saves the current game state, the session may have written some data to the file already.

// mekberg: added saveTypes
	virtual void				SaveGame( anFile *saveGameFile, saveType_t saveType = ST_REGULAR ) = 0;


	// Shut down the current map.
	virtual void				MapShutdown( void ) = 0;

	// Caches media referenced from in key/value pairs in the given dictionary.
	virtual void				CacheDictionaryMedia( const anDict *dict ) = 0;

	// Spawns the player entity to be used by the client.
	virtual void				SpawnPlayer( int clientNum ) = 0;


	// Runs a game frame, may return a session command for level changing, etc
	// lastCatchupFrame is always true except if we are running several game frames in a row and this one is not the last one
	// subsystems which can tolerate skipping frames will not run during those catchup frames
	// several game frames in a row happen when game + renderer time goes above the tick time ( 16ms )
	virtual gameReturn_t		RunFrame( const usercmd_t *clientCmds, int activeEditors, bool lastCatchupFrame, int serverGameFrame ) = 0;

	virtual void				MenuFrame( void ) = 0;


	// Runs a repeater frame
	virtual void				RepeaterFrame( const userOrigin_t *clientOrigins, bool lastCatchupFrame, int serverGameFrame ) = 0;

	// Makes rendering and sound system calls to display for a given clientNum.
	virtual bool				Draw( int clientNum ) = 0;

	// Let the game do it's own UI when ESCAPE is used
	virtual escReply_t			HandleESC( anUserInterface **gui ) = 0;

	// get the games menu if appropriate ( multiplayer )
	virtual anUserInterface *	StartMenu() = 0;

	// When the game is running it's own UI fullscreen, GUI commands are passed through here
	// return nullptr once the fullscreen UI mode should stop, or "main" to go to main menu
	virtual const char *		HandleGuiCommands( const char *menuCommand ) = 0;

	// main menu commands not caught in the engine are passed here
	virtual void				HandleMainMenuCommands( const char *menuCommand, anUserInterface *gui ) = 0;

	// Early check to deny connect.
	virtual allowReply_t		ServerAllowClient( int clientId, int numClients, const char *IP, const char *guid, const char *password, const char *privatePassword, char reason[MAX_STRING_CHARS] ) = 0;

	// Connects a client.
	virtual void				ServerClientConnect( int clientNum, const char *guid ) = 0;

	// Spawns the player entity to be used by the client.
	virtual void				ServerClientBegin( int clientNum ) = 0;

	// Disconnects a client and removes the player entity from the game.
	virtual void				ServerClientDisconnect( int clientNum ) = 0;

	// Writes initial reliable messages a client needs to recieve when first joining the game.
	virtual void				ServerWriteInitialReliableMessages( int clientNum ) = 0;

	// Early check to deny connect.
	virtual allowReply_t		RepeaterAllowClient( int clientId, int numClients, const char *IP, const char *guid, bool repeater, const char *password, const char *privatePassword, char reason[MAX_STRING_CHARS] ) = 0;

	// Connects a client.
	virtual void				RepeaterClientConnect( int clientNum ) = 0;

	// Spawns the player entity to be used by the client.
	virtual void				RepeaterClientBegin( int clientNum ) = 0;

	// Disconnects a client and removes the player entity from the game.
	virtual void				RepeaterClientDisconnect( int clientNum ) = 0;

	// Writes initial reliable messages a client needs to recieve when first joining the game.
	virtual void				RepeaterWriteInitialReliableMessages( int clientNum ) = 0;

	// Writes a snapshot of the server game state for the given client.
	virtual void				ServerWriteSnapshot( int clientNum, int sequence, anBitMsg &msg, dword *clientInPVS, int numPVSClients, int lastSnapshotFrame ) = 0;

	// Patches the network entity states at the server with a snapshot for the given client.
	virtual bool				ServerApplySnapshot( int clientNum, int sequence ) = 0;

	// Processes a reliable message from a client.
	virtual void				ServerProcessReliableMessage( int clientNum, const anBitMsg &msg ) = 0;

	// Patches the network entity states at the server with a snapshot for the given client.
	virtual bool				RepeaterApplySnapshot( int clientNum, int sequence ) = 0;

	// Processes a reliable message from a client.
	virtual void				RepeaterProcessReliableMessage( int clientNum, const anBitMsg &msg ) = 0;

	// Reads a snapshot and updates the client game state.
	virtual void				ClientReadSnapshot( int clientNum, int snapshotSequence, const int gameFrame, const int gameTime, const int dupeUsercmds, const int aheadOfServer, const anBitMsg &msg ) = 0;

	// Patches the network entity states at the client with a snapshot.
	virtual bool				ClientApplySnapshot( int clientNum, int sequence ) = 0;

	// Processes a reliable message from the server.
	virtual void				ClientProcessReliableMessage( int clientNum, const anBitMsg &msg ) = 0;

	// Runs prediction on entities at the client.
	virtual gameReturn_t		ClientPrediction( int clientNum, const usercmd_t *clientCmds, bool lastPredictFrame = true, ClientStats_t *cs = nullptr ) = 0;


// ddynerman: client game frame
	virtual void				ClientRun( void ) = 0;
	virtual void				ClientEndFrame( void ) = 0;

// jshepard: rcon password check
	virtual void				ProcessRconReturn( bool success ) = 0;



	virtual bool				ValidateServerSettings( const char *map, const char *gameType ) = 0;

	// Returns a summary of stats for a given client
	virtual void				GetClientStats( int clientNum, char *data, const int len ) = 0;

	// Switch a player to a particular team
	virtual void				SwitchTeam( int clientNum, int team ) = 0;

	virtual bool				DownloadRequest( const char *IP, const char *guid, const char *paks, char urls[ MAX_STRING_CHARS ] ) = 0;

	// return true to allow download from the built-in http server
	virtual bool				HTTPRequest( const char *IP, const char *file, bool isGamePak ) = 0;


// jscott: for the effects system
	virtual void				StartViewEffect( int type, float time, float scale ) = 0;
	virtual rvClientEffect*		PlayEffect( const idDecl *effect, const anVec3 &origin, const anMat3 &axis, bool loop = false, const anVec3 &endOrigin = vec3_origin, bool broadcast = false, bool predictBit = false, effectCategory_t category = EC_IGNORE, const anVec4& effectTint = vec4_one ) = 0;
	virtual void				GetPlayerView( anVec3 &origin, anMat3 &axis ) = 0;
	virtual const anVec3		GetCurrentGravity( const anVec3 &origin, const anMat3 &axis ) const = 0;
	virtual void				Translation( trace_t &trace, anVec3 &source, anVec3 &dest, anTraceModel *trm, int clipMask ) = 0;
	virtual void				SpawnClientMoveable ( const char *name, int lifetime, const anVec3 &origin, const anMat3 &axis, const anVec3 &velocity, const anVec3 &angular_velocity ) = 0;
// bdube: debugging stuff
	virtual void				DebugSetString ( const char *name, const char *value ) = 0;
	virtual void				DebugSetFloat ( const char *name, float value ) = 0;
	virtual void				DebugSetInt ( const char *name, int value ) = 0;
	virtual const char*			DebugGetStatString ( const char *name ) = 0;
	virtual int					DebugGetStatInt ( const char *name ) = 0;
	virtual float				DebugGetStatFloat ( const char *name ) = 0;
	virtual bool				IsDebugHudActive ( void ) const = 0;
// rjohnson: for new note taking mechanism
	virtual bool				GetPlayerInfo( anVec3 &origin, anMat3 &axis, int PlayerNum = -1, anAngles *deltaViewAngles = nullptr, int reqClientNum = -1 ) = 0;
	virtual void				SetPlayerInfo( anVec3 &origin, anMat3 &axis, int PlayerNum = -1 ) = 0;
	virtual	bool				PlayerChatDisabled( int clientNum ) = 0;
	virtual void				SetViewComments( const char *text = 0 ) = 0;
// ddynerman: utility functions
	virtual void				GetPlayerName( int clientNum, char *name ) = 0;
	virtual void				GetPlayerClan( int clientNum, char *clan ) = 0;
	virtual void				SetFriend( int clientNum, bool isFriend ) = 0;
	virtual const char*			GetLongGametypeName( const char *gametype ) = 0;
	virtual void				ReceiveRemoteConsoleOutput( const char *output ) = 0;
// rjohnson: entity usage stats
	virtual void				ListEntityStats( const anCommandArgs &args ) = 0;
// shouchard:  for ban lists
	virtual void				RegisterClientGuid( int clientNum, const char *guid ) = 0;
	virtual bool				IsMultiplayer( void ) = 0;
// mekberg: added
	virtual bool				InCinematic( void ) = 0;
// mekberg: so banlist can be populated outside of multiplayer game
	virtual void				PopulateBanList( anUserInterface* hud ) = 0;
	virtual void				RemoveGuidFromBanList( const char *guid ) = 0;
// mekberg: interface
	virtual void				AddGuidToBanList( const char *guid ) = 0;
	virtual const char*			GetGuidByClientNum( int clientNum ) = 0;
// jshepard: updating player post-menu
	virtual void				UpdatePlayerPostMainMenu( void ) = 0;
	virtual void				ResetRconGuiStatus( void ) = 0;



// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT)
	virtual void				FlushBeforelevelLoad( void ) = 0;
#endif


	// Set the demo state.
	virtual void				SetDemoState( demoState_t state, bool serverDemo, bool timeDemo ) = 0;

	// Set the repeater state; engine will call this with true for isRepeater if this is a repeater, and true for serverIsRepeater if we are connected to a repeater
	virtual void				SetRepeaterState( bool isRepeater, bool serverIsRepeater ) = 0;

	// Writes current network info to a file (used as initial state for demo recording).
	virtual void				WriteNetworkInfo( anFile *file, int clientNum ) = 0;

	// Reads current network info from a file (used as initial state for demo playback).
	virtual void				ReadNetworkInfo( int gameTime, anFile *file, int clientNum ) = 0;

	// Let gamecode decide if it wants to accept demos from older releases of the engine.
	virtual bool				ValidateDemoProtocol( int minor_ref, int minor ) = 0;

	// Write a snapshot for server demo recording.
	virtual void				ServerWriteServerDemoSnapshot( int sequence, anBitMsg &msg, int lastSnapshotFrame ) = 0;

	// Read a snapshot from a server demo stream.
	virtual void				ClientReadServerDemoSnapshot( int sequence, const int gameFrame, const int gameTime, const anBitMsg &msg ) = 0;

	// Write a snapshot for repeater clients.
	virtual void				RepeaterWriteSnapshot( int clientNum, int sequence, anBitMsg &msg, dword *clientInPVS, int numPVSClients, const userOrigin_t &pvs_origin, int lastSnapshotFrame ) = 0;

	// Done writing snapshots for repeater clients.
	virtual void				RepeaterEndSnapshots( void ) = 0;

	// Read a snapshot from a repeater stream.
	virtual void				ClientReadRepeaterSnapshot( int sequence, const int gameFrame, const int gameTime, const int aheadOfServer, const anBitMsg &msg ) = 0;

	// Get the currently followed client in demo playback
	virtual int					GetDemoFollowClient( void ) = 0;

	// Build a bot's userCmd
	virtual void				GetBotInput( int clientNum, usercmd_t &userCmd ) = 0;

	// Return the name of a gui to override the loading screen
	virtual const char *		GetLoadingGui( const char *mapDeclName ) = 0;

	// Set any additional gui variables needed by the loading screen
	virtual void				SetupLoadingGui( anUserInterface *gui ) = 0;
};

extern idGame *					game;


/*
===============================================================================

	Public game interface with methods for in-game editing.

===============================================================================
*/

struct refSound_t {

	int							referenceSoundHandle;	// this is the interface to the sound system, created
														// with idSoundWorld::AllocSoundEmitter() when needed

	anVec3						origin;

// jscott: for Miles doppler
	anVec3						velocity;

	int							listenerId;		// SSF_PRIVATE_SOUND only plays if == listenerId from PlaceListener
												// no spatialization will be performed if == listenerID
	const idSoundShader *		shader;			// this really shouldn't be here, it is a holdover from single channel behavior
	float						diversity;		// 0.0 to 1.0 value used to select which
												// samples in a multi-sample list from the shader are used
	bool						waitfortrigger;	// don't start it at spawn time
	soundShaderParms_t			parms;			// override volume, flags, etc
};

enum {
	TEST_PARTICLE_MODEL = 0,
	TEST_PARTICLE_IMPACT,
	TEST_PARTICLE_MUZZLE,
	TEST_PARTICLE_FLIGHT,
	TEST_PARTICLE_SELECTED
};

class anEntity;
class anMD6Anim;

// bdube: more forward declarations
class idProgram;
class idInterpreter;
class anThread;

typedef void (*debugInfoProc_t) ( const char *classname, const char *name, const char *value, void *userdata );


// FIXME: this interface needs to be reworked but it properly separates code for the time being
class anGameEdit {
public:
	virtual						~anGameEdit( void ) {}

	// These are the canonical anDict to parameter parsing routines used by both the game and tools.
	virtual bool				ParseSpawnArgsToRenderLight( const anDict *args, renderLight_t *renderLight );
	virtual void				ParseSpawnArgsToRenderEntity( const anDict *args, renderEntity_t *renderEntity );
	virtual void				ParseSpawnArgsToRefSound( const anDict *args, refSound_t *refSound );

	// Animation system calls for non-game based skeletal rendering.
	virtual anRenderModel *		ANIM_GetModelFromEntityDef( const char *classname );
	virtual const anVec3 		&ANIM_GetModelOffsetFromEntityDef( const char *classname );
	virtual anRenderModel *		ANIM_GetModelFromEntityDef( const anDict *args );
	virtual anRenderModel *		ANIM_GetModelFromName( const char *modelName );
	virtual const anMD6Anim *	ANIM_GetAnimFromEntityDef( const char *classname, const char *animname );

// bdube: added
// scork: added 'const' qualifiers so other stuff would compile
	virtual const anMD6Anim *	ANIM_GetAnimFromEntity( const anEntity *ent, int animNum );
	virtual float				ANIM_GetAnimPlaybackRateFromEntity ( anEntity *ent, int animNum );
	virtual const char*			ANIM_GetAnimNameFromEntity ( const anEntity *ent, int animNum );

	virtual int					ANIM_GetNumAnimsFromEntityDef( const anDict *args );
	virtual const char *		ANIM_GetAnimNameFromEntityDef( const anDict *args, int animNum );
	virtual const anMD6Anim *	ANIM_GetAnim( const char *fileName );
	virtual int					ANIM_GetLength( const anMD6Anim *anim );
	virtual int					ANIM_GetNumFrames( const anMD6Anim *anim );

// bdube: added
	virtual const char *		ANIM_GetFilename( const anMD6Anim* anim );
	virtual int					ANIM_ConvertFrameToTime ( const anMD6Anim* anim, int frame );
	virtual int					ANIM_ConvertTimeToFrame ( const anMD6Anim* anim, int time );

	virtual void				ANIM_CreateAnimFrame( const anRenderModel *model, const anMD6Anim *anim, int numJoints, anJointMat *frame, int time, const anVec3 &offset, bool remove_origin_offset );
	virtual anRenderModel *		ANIM_CreateMeshForAnim( anRenderModel *model, const char *classname, const char *animname, int frame, bool remove_origin_offset );


// mekberg: access to animationlib functions for radiant
	virtual void				FlushUnusedAnims( void );


	// Articulated Figure calls for AF editor and Radiant.
	virtual bool				AF_SpawnEntity( const char *fileName );
	virtual void				AF_UpdateEntities( const char *fileName );
	virtual void				AF_UndoChanges( void );
	virtual anRenderModel *		AF_CreateMesh( const anDict &args, anVec3 &meshOrigin, anMat3 &meshAxis, bool &poseIsSet );


	// Entity selection.
	virtual void				ClearEntitySelection( void );
	virtual int					GetSelectedEntities( anEntity *list[], int max );
	virtual void				AddSelectedEntity( anEntity *ent );

	// Selection methods
	virtual void				TriggerSelected();

	// Entity defs and spawning.
	virtual const anDict *		FindEntityDefDict( const char *name, bool makeDefault = true ) const;
	virtual void				SpawnEntityDef( const anDict &args, anEntity **ent );
	virtual anEntity *			FindEntity( const char *name ) const;
	virtual const char *		GetUniqueEntityName( const char *classname ) const;

	// Entity methods.
	virtual void				EntityGetOrigin( anEntity *ent, anVec3 &org ) const;
	virtual void				EntityGetAxis( anEntity *ent, anMat3 &axis ) const;
	virtual void				EntitySetOrigin( anEntity *ent, const anVec3 &org );
	virtual void				EntitySetAxis( anEntity *ent, const anMat3 &axis );
	virtual void				EntityTranslate( anEntity *ent, const anVec3 &org );

// scork: const-qualified 'ent' so other things would compile
	virtual const anDict *		EntityGetSpawnArgs( const anEntity *ent ) const;

	virtual void				EntityUpdateChangeableSpawnArgs( anEntity *ent, const anDict *dict );
	virtual void				EntityChangeSpawnArgs( anEntity *ent, const anDict *newArgs );
	virtual void				EntityUpdateVisuals( anEntity *ent );
	virtual void				EntitySetModel( anEntity *ent, const char *val );
	virtual void				EntityStopSound( anEntity *ent );
	virtual void				EntityDelete( anEntity *ent );
	virtual void				EntitySetColor( anEntity *ent, const anVec3 color );

// bdube: added
	virtual const char*			EntityGetName ( anEntity *ent ) const;
	virtual int					EntityToSafeId( anEntity *ent ) const;
	virtual anEntity *			EntityFromSafeId( int safeID) const;
	virtual void				EntitySetSkin ( anEntity *ent, const char *temp ) const;
	virtual void				EntityClearSkin ( anEntity *ent ) const;
	virtual void				EntityShow ( anEntity *ent ) const;
	virtual void				EntityHide ( anEntity *ent ) const;
	virtual void				EntityGetBounds ( anEntity *ent, anBounds &bounds ) const;
	virtual int					EntityPlayAnim ( anEntity *ent, int animNum, int time, int blendtime );
	virtual void				EntitySetFrame ( anEntity *ent, int animNum, int frame, int time, int blendtime );
	virtual void				EntityStopAllEffects ( anEntity *ent );
	virtual void				EntityGetDelta ( anEntity *ent, int fromTime, int toTime, anVec3 &delta );
	virtual void				EntityRemoveOriginOffset ( anEntity *ent, bool remove );
	virtual const char*			EntityGetClassname ( anEntity *ent ) const;
	virtual bool				EntityIsDerivedFrom ( anEntity *ent, const char *classname ) const;
	virtual renderEntity_t*		EntityGetRenderEntity ( anEntity *ent );
// scork: accessor functions for various utils
	virtual	anEntity *			EntityGetNextTeamEntity( anEntity *pEnt ) const;
	virtual void				GetPlayerInfo( anVec3 &v3Origin, anMat3 &mat3Axis, int PlayerNum = -1, anAngles *deltaViewAngles = nullptr ) const;
	virtual void				SetPlayerInfo( anVec3 &v3Origin, anMat3 &mat3Axis, int PlayerNum = -1 ) const;
	virtual void				EntitySetName( anEntity *pEnt, const char *psName );


	// Player methods.
	virtual bool				PlayerIsValid() const;
	virtual void				PlayerGetOrigin( anVec3 &org ) const;
	virtual void				PlayerGetAxis( anMat3 &axis ) const;
	virtual void				PlayerGetViewAngles( anAngles &angles ) const;
	virtual void				PlayerGetEyePosition( anVec3 &org ) const;

// bdube: new game edit stuff
	virtual bool				PlayerTraceFromEye ( trace_t &results, float length, int contentMask );

	// Effect methods
	virtual void				EffectRefreshTemplate ( const idDecl *effect ) const;

	// Light entity methods
	virtual void				LightSetParms ( anEntity *ent, int maxLevel, int currentLevel, float radius );

	// arCNet editing functions
	virtual int					GetGameTime ( int *previous = nullptr ) const;
	virtual void				SetGameTime	( int time ) const;
	virtual bool				TracePoint ( trace_t &results, const anVec3 &start, const anVec3 &end, int contentMask ) const;
	virtual void				CacheDictionaryMedia ( const anDict* dict ) const;
	virtual void				SetCamera ( anEntity *camera ) const;

// bdube: added
	virtual int					GetGameEntityRegisterTime ( void ) const;
	virtual anEntity*			GetFirstSpawnedEntity ( void ) const;
	virtual anEntity*			GetNextSpawnedEntity ( anEntity *from ) const;
// jscott: added
	virtual	void				DrawPlaybackDebugInfo( void );
	virtual	void				RecordPlayback( const usercmd_t &cmd, anEntity *source );
	virtual	bool				PlayPlayback( void );
	virtual	void				ShutdownPlaybacks( void );


	// Script methods
	virtual int					ScriptGetStatementLineNumber ( idProgram* program, int instructionPointer ) const;
	virtual const char*			ScriptGetStatementFileName ( idProgram* program, int instructionPointer ) const;
	virtual int					ScriptGetStatementOperator ( idProgram* program, int instructionPointer ) const;
	virtual void*				ScriptGetCurrentFunction ( idInterpreter* interpreter ) const;
	virtual const char*			ScriptGetCurrentFunctionName ( idInterpreter* interpreter ) const;
	virtual int					ScriptGetCallstackDepth ( idInterpreter* interpreter ) const;
	virtual void*				ScriptGetCallstackFunction ( idInterpreter* interpreter, int depth ) const;
	virtual const char*			ScriptGetCallstackFunctionName ( idInterpreter* interpreter, int depth ) const;
	virtual int					ScriptGetCallstackStatement ( idInterpreter* interpreter, int depth ) const;
	virtual bool				ScriptIsReturnOperator ( int op ) const;
	virtual const char*			ScriptGetRegisterValue ( idInterpreter* interpreter, const char *varname, int callstackDepth ) const;
	virtual anThread*			ScriptGetThread ( idInterpreter* interpreter ) const;

	// Thread methods
	virtual int					ThreadGetCount ( void );
	virtual anThread*			ThreadGetThread ( int index );
	virtual const char*			ThreadGetName ( anThread* thread );
	virtual int					ThreadGetNumber ( anThread* thread );
	virtual const char*			ThreadGetState ( anThread* thread );

	// Class externals for entity viewer
	virtual void				GetClassDebugInfo ( const anEntity *entity, debugInfoProc_t proc, void* userdata );

	// In game map editing support.
	virtual const anDict *		MapGetEntityDict( const char *name ) const;
	virtual void				MapSave( const char *path = nullptr ) const;

// rjohnson: added entity export
	virtual bool				MapHasExportEntities( void ) const;
// scork: simple func for the sound editor
	virtual const char*			MapLoaded( void ) const;
// cdr: AASTactical
	virtual anSEASFile*			GetSEASFile( int i );
// jscott: added entries for memory tracking
	virtual void				PrintMemInfo( MemInfo *mi );
	virtual size_t				ScriptSummary( const anCommandArgs &args ) const;
	virtual size_t				ClassSummary( const anCommandArgs &args ) const;
	virtual size_t				EntitySummary( const anCommandArgs &args ) const;

	virtual void				MapSetEntityKeyVal( const char *name, const char *key, const char *val ) const ;
	virtual void				MapCopyDictToEntity( const char *name, const anDict *dict ) const;
	virtual int					MapGetUniqueMatchingKeyVals( const char *key, const char *list[], const int max ) const;
	virtual void				MapAddEntity( const anDict *dict ) const;
	virtual int					MapGetEntitiesMatchingClassWithString( const char *classname, const char *match, const char *list[], const int max ) const;
	virtual void				MapRemoveEntity( const char *name ) const;
	virtual void				MapEntityTranslate( const char *name, const anVec3 &v ) const;
};

extern anGameEdit *				gameEdit;


// bdube: game logging
/*
===============================================================================

	Game Log.

===============================================================================
*/
class rvGameLog {
public:
	virtual				~rvGameLog( void ) {}

	virtual void		Init		( void ) = 0;
	virtual void		Shutdown	( void ) = 0;

	virtual void		BeginFrame	( int time ) = 0;
	virtual void		EndFrame	( void ) = 0;

	virtual	void		Set			( const char *keyword, int value ) = 0;
	virtual void		Set			( const char *keyword, float value ) = 0;
	virtual void		Set			( const char *keyword, const char *value ) = 0;
	virtual void		Set			( const char *keyword, bool value ) = 0;

	virtual void		Add			( const char *keyword, int value ) = 0;
	virtual void		Add			( const char *keyword, float value ) = 0;
};

extern rvGameLog *				gameLog;

#define GAMELOG_SET(x,y)		{if (g_gamelog.GetBool())gameLog->Set ( x, y );}
#define GAMELOG_ADD(x,y)		{if (g_gamelog.GetBool())gameLog->Add ( x, y );}

#define GAMELOG_SET_IF(x,y,z)	{if (g_gamelog.GetBool()&&(z))gameLog->Set ( x, y );}
#define GAMELOG_ADD_IF(x,y,z)	{if (g_gamelog.GetBool()&&(z))gameLog->Add ( x, y );}



/*
===============================================================================

	Game API.

===============================================================================
*/

// 4: network demos
// 5: fix idNetworkSystem ( memory / DLL boundary related )
// 6: more network demo APIs
// 7: cleanups
// 8: added some demo functions to the FS class
// 9: bump up for 1.1 patch
// 9: Q4 Gold
// 10: Patch 2 changes
// 14: 1.3
// 26: 1.4 beta
// 30: 1.4
// 37: 1.4.2
const int GAME_API_VERSION		= 37;

struct gameImport_t {

	int							version;				// API version
	idSys *						sys;					// non-portable system services
	arCNet *					common;					// common
	idCmdSystem *				cmdSystem;				// console command system
	anCVarSystem *				cvarSystem;				// console variable system
	anFileSystem *				fileSystem;				// file system
	idNetworkSystem *			networkSystem;			// network system
	idRenderSystem *			renderSystem;			// render system
	idSoundSystem *				soundSystem;			// sound system
	idRenderModelManager *		renderModelManager;		// render model manager
	anUserInterfaceManager *	uiManager;				// user interface manager
	idDeclManager *				declManager;			// declaration manager
	anSEASFileManager *			SEASFileManager;			// AAS file manager
	anCollisionModelManager *	collisionModelManager;	// collision model manager


// jscott:
	rvBSEManager *				bse;					// Raven effects system



// dluetscher: added the following members to exchange memory system data
#ifdef _RV_MEM_SYS_SUPPORT
	rvHeapArena *				heapArena;								// main heap arena that all other heaps use
	rvHeap *					systemHeapArray[MAX_SYSTEM_HEAPS];		// array of pointers to rvHeaps that are common to anLib, Game, and executable
#endif

};

struct gameExport_t {

	int							version;				// API version
	idGame *					game;					// interface to run the game
	anGameEdit *				gameEdit;				// interface for in-game editing

// bdube: added
	rvGameLog *					gameLog;				// interface for game logging

};

extern "C" {
typedef gameExport_t * (*GetGameAPI_t)( gameImport_t *import );
}

#endif /* !__GAME_H__ */
