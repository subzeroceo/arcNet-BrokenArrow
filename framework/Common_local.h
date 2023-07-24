static const int MAX_USERCMD_BACKUP = 256;
static const int NUM_USERCMD_RELAY = 10;
static const int NUM_USERCMD_SEND = 8;

static const int initialHz = 60;
static const int initialBaseTicks = 1000 / initialHz;
static const int initialBaseTicksPerSec = initialHz * initialBaseTicks;

static const int LOAD_TIP_CHANGE_INTERVAL = 12000;
static const int LOAD_TIP_COUNT = 26;

class idGameThread : public arcSysThread {
public:
	idGameThread() :
		gameTime(),
		drawTime(),
		threadTime(),
		threadGameTime(),
		threadRenderTime(),
		userCmdMgr( NULL ),
		ret(),
		numGameFrames(),
		isClient()
	{}

	// the gameReturn_t is from the previous frame, the
	// new frame will be running in parallel on exit
	gameReturn_t	RunGameAndDraw( int numGameFrames, arcUserCmdMgr & userCmdMgr_, int startGameFrame );

	// Accessors to the stored frame/thread time information
	void			SetThreadTotalTime( const int inTime ) { threadTime = inTime; }
	int				GetThreadTotalTime() const { return threadTime; }

	void			SetThreadGameTime( const int time ) { threadGameTime = time; }
	int				GetThreadGameTime() const { return threadGameTime; }

	void			SetThreadRenderTime( const int time ) { threadRenderTime = time; }
	int				GetThreadRenderTime() const { return threadRenderTime; }

private:
	virtual int	Run();

	int				gameTime;
	int				drawTime;
	int				threadTime;					// total time : game time + foreground render time
	int				threadGameTime;				// game time only
	int				threadRenderTime;			// render fg time only
	arcUserCmdMgr *	userCmdMgr;
	gameReturn_t	ret;
	int				numGameFrames;
	bool			isClient;
};

enum errorParm_t {
	ERP_NONE,
	ERP_FATAL,						// exit the entire game with a popup window
	ERP_DROP,						// print to console and disconnect from game
	ERP_DISCONNECT					// don't kill server
};

enum gameLaunch_t {
	LAUNCH_TITLE_DOOM = 0,
	LAUNCH_TITLE_DOOM2,
};

struct netTimes_t {
	int localTime;
	int serverTime;
};

struct frameTiming_t {
	uint64	startSyncTime;
	uint64	finishSyncTime;
	uint64	startGameTime;
	uint64	finishGameTime;
	uint64	finishDrawTime;
	uint64	startRenderTime;
	uint64	finishRenderTime;
};

#define	MAX_PRINT_MSG_SIZE	4096
#define MAX_WARNING_LIST	256

#define SAVEGAME_CHECKPOINT_FILENAME		"gamedata.save"
#define SAVEGAME_DESCRIPTION_FILENAME		"gamedata.txt"
#define SAVEGAME_STRINGS_FILENAME			"gamedata.strings"

class arcCommonLocal : public arcCommon {
public:
								arcCommonLocal();

	virtual void				Init( int argc, const char * const * argv, const char *cmdline );
	virtual void				Shutdown();
	virtual	void				CreateMainMenu();
	virtual void				Quit();
	virtual bool				IsInitialized() const;
	virtual void				Frame();
	virtual void				UpdateScreen( bool captureToImage );
	virtual void				UpdateLevelLoadPacifier();
	virtual void				StartupVariable( const char *match );
	virtual void				WriteConfigToFile( const char *filename );
	virtual void				BeginRedirect( char *buffer, int buffersize, void (* flush)(const char *) );
	virtual void				EndRedirect();
	virtual void				SetRefreshOnPrint( bool set );
	virtual void				Printf( VERIFY_FORMAT_STRING const char *fmt, ... );
	virtual void				VPrintf( const char *fmt, va_list arg );
	virtual void				DPrintf( VERIFY_FORMAT_STRING const char *fmt, ... );
	virtual void				Warning( VERIFY_FORMAT_STRING const char *fmt, ... );
	virtual void				DWarning( VERIFY_FORMAT_STRING const char *fmt, ...);
	virtual void				PrintWarnings();
	virtual void				ClearWarnings( const char *reason );
	virtual void				Error( VERIFY_FORMAT_STRING const char *fmt, ... );
	virtual void				FatalError( VERIFY_FORMAT_STRING const char *fmt, ... );
	virtual bool				IsShuttingDown() const { return com_shuttingDown; }

	virtual const char *		KeysFromBinding( const char *bind );
	virtual const char *		BindingFromKey( const char *key );

	virtual bool				IsClient();

	virtual bool				GetConsoleUsed() { return consoleUsed; }

	virtual int					GetSnapRate();

	virtual bool				ProcessEvent( const sysEvent_t *event );

	virtual bool				LoadGame( const char * saveName );
	virtual bool				SaveGame( const char * saveName );

	virtual int					ButtonState( int key );
	virtual int					KeyState( int key );

	virtual ARCDemoFile *		ReadDemo() { return readDemo; }
	virtual ARCDemoFile *		WriteDemo() { return writeDemo; }

	virtual ARCEngine *			Game() { return game; }
	virtual ARCRenderWorld *	RW() { return renderWorld; }
	virtual ARCSoundWorld *		SW() { return soundWorld; }
	virtual ARCSoundWorld *		MenuSW() { return menuSoundWorld; }
	virtual arcCommonDlg &		Dialog() { return commonDialog; }

	virtual void				OnSaveCompleted( ARCSaveLoadParms & parms );
	virtual void				OnLoadCompleted( ARCSaveLoadParms & parms );
	virtual void				OnLoadFilesCompleted( ARCSaveLoadParms & parms );
	virtual void				OnEnumerationCompleted( ARCSaveLoadParms & parms );
	virtual void				OnDeleteCompleted( ARCSaveLoadParms & parms );
	virtual void				TriggerScreenWipe( const char * _wipeMaterial, bool hold );

	virtual void				OnStartHosting( arcMatchParameters & parms );

	virtual int					GetGameFrame() { return gameFrame; }

	virtual void				LaunchExternalTitle( int titleIndex, int device, const lobbyConnectInfo_t * const connectInfo ); // For handling invitations. NULL if no invitation used.

	virtual void				InitializeMPMapsModes();
	virtual const arcStringList &GetModeList() const { return mpGameModes; }
	virtual const arcStringList &GetModeDisplayList() const { return mpDisplayGameModes; }
	virtual const arcNetList<mpMap_t> &GetMapList() const { return mpGameMaps; }

	virtual void				ResetPlayerInput( int playerIndex );

	virtual void				QueueShowShell() { showShellRequested = true; }

	virtual currentGame_t		GetCurrentGame() const { return currentGame; }
	virtual void				SwitchToGame( currentGame_t newGame );

public:
	void	Draw();			// called by gameThread

	int		GetGameThreadTotalTime() const { return gameThread.GetThreadTotalTime(); }
	int		GetGameThreadGameTime() const { return gameThread.GetThreadGameTime(); }
	int		GetGameThreadRenderTime() const { return gameThread.GetThreadRenderTime(); }
	int		GetRendererBackEndMicroseconds() const { return time_backend; }
	int		GetRendererShadowsMicroseconds() const { return time_shadows; }
	int		GetRendererIdleMicroseconds() const { return mainFrameTiming.startRenderTime - mainFrameTiming.finishSyncTime; }
	int		GetRendererGPUMicroseconds() const { return time_gpu; }

	frameTiming_t		frameTiming;
	frameTiming_t		mainFrameTiming;

public:	// These are public because they are called directly by static functions in this file

	const char * GetCurrentMapName() { return currentMapName.c_str(); }

	// loads a map and starts a new game on it
	void	StartNewGame( const char * mapName, bool devmap, int gameMode );
	void	LeaveGame();

	void	DemoShot( const char *name );
	void	StartRecordingRenderDemo( const char *name );
	void	StopRecordingRenderDemo();
	void	StartPlayingRenderDemo( arcNetString name );
	void	StopPlayingRenderDemo();
	void	CompressDemoFile( const char *scheme, const char *name );
	void	TimeRenderDemo( const char *name, bool twice = false, bool quit = false );
	void	AVIRenderDemo( const char *name );
	void	AVIGame( const char *name );

	// localization
	void	InitLanguageDict();
	void	LocalizeGui( const char *fileName, arcLangDictionary &langDict );
	void	LocalizeMapData( const char *fileName, arcLangDictionary &langDict );
	void	LocalizeSpecificMapData( const char *fileName, arcLangDictionary &langDict, const arcLangDictionary &replaceArgs );

	arcUserCmdMgr & GetUCmdMgr() { return userCmdMgr; }

private:
	bool						com_fullyInitialized;
	bool						com_refreshOnPrint;		// update the screen every print for dmap
	errorParm_t					com_errorEntered;
	bool						com_shuttingDown;

	arcNetFile *					logFile;

	char						errorMessage[MAX_PRINT_MSG_SIZE];

	char *						rd_buffer;
	int							rd_buffersize;
	void						(*rd_flush)( const char *buffer );

	arcNetString						warningCaption;
	arcStringList					warningList;
	arcStringList					errorList;

	int							engineDLL;

	arcCommonDlg				commonDialog;

	arcFile_SaveGame 			saveFile;
	arcFile_SaveGame 			stringsFile;
	arcFile_SaveGamePipelined 	*pipelineFile;

	// The main render world and sound world
	ARCRenderWorld *			renderWorld;
	ARCSoundWorld *				soundWorld;

	// The renderer and sound system will write changes to writeDemo.
	// Demos can be recorded and played at the same time when splicing.
	ARCDemoFile *				readDemo;
	ARCDemoFile *				writeDemo;

	bool						menuActive;
	ARCSoundWorld *				menuSoundWorld;			// so the game soundWorld can be muted

	bool						insideExecuteMapChange;	// Enable Pacifier Updates

	// This is set if the player enables the console, which disables achievements
	bool						consoleUsed;

	// This additional information is required for ExecuteMapChange for SP games ONLY
	// This data is cleared after ExecuteMapChange
	struct mapSpawnData_t {
		arcFile_SaveGame *		savegameFile;				// Used for loading a save game
		arcFile_SaveGame *		stringTableFile;			// String table read from save game loaded
		arcFile_SaveGamePipelined *pipelineFile;
		int						savegameVersion;			// Version of the save game we're loading
		arcDictionary				persistentPlayerInfo;		// Used for transitioning from map to map
	};
	mapSpawnData_t				mapSpawnData;
	arcNetString					currentMapName;			// for checking reload on same level
	bool						mapSpawned;				// cleared on Stop()

	bool						insideUpdateScreen;		// true while inside ::UpdateScreen()

	arcUserCmdMgr				userCmdMgr;

	int							nextUsercmdSendTime;	// Next time to send usercmds
	int							nextSnapshotSendTime;	// Next time to send a snapshot

	ARCSnapShot					lastSnapShot;		// last snapshot we received from the server
	struct reliableMsg_t {
		int	client;
		int type;
		int dataSize;
		byte * data;
	};
	arcNetList<reliableMsg_t> reliableQueue;

	// Snapshot interpolation
	ARCSnapShot		oldss;				// last local snapshot
										// (ie on server this is the last "master" snapshot  we created)
										// (on clients this is the last received snapshot)
										// used for comparisons with the new snapshot for com_drawSnapshot

	// This is ultimately controlled by net_maxBufferedSnapshots by running double speed, but this is the hard max before seeing visual popping
	static const int RECEIVE_SNAPSHOT_BUFFER_SIZE = 16;

	int				readSnapshotIndex;
	int				writeSnapshotIndex;
	arcArray<ARCSnapShot,RECEIVE_SNAPSHOT_BUFFER_SIZE>	receivedSnaps;

	float			optimalPCTBuffer;
	float			optimalTimeBuffered;
	float			optimalTimeBufferedWindow;

	uint64			snapRate;
	uint64			actualRate;

	uint64			snapTime;			// time we got the most recent snapshot
	uint64			snapTimeDelta;		// time interval that current ss was sent in

	uint64			snapTimeWrite;
	uint64			snapCurrentTime;	// realtime playback time
	netTimes_t		snapCurrent;		// current snapshot
	netTimes_t		snapPrevious;		// previous snapshot
	float			snapCurrentResidual;

	float			snapTimeBuffered;
	float			effectiveSnapRate;
	int				totalBufferedTime;
	int				totalRecvTime;

	int					clientPrediction;

	int					gameFrame;			// Frame number of the local game
	double				gameTimeResidual;	// left over msec from the last game frame
	bool				syncNextEngineFrame;

	bool				aviCaptureMode;		// if true, screenshots will be taken and sound captured
	arcNetString			aviDemoShortName;	//
	int					aviDemoFrameCount;

	enum timeDemo_t {
		TD_NO,
		TD_YES,
		TD_YES_THEN_QUIT
	};
	timeDemo_t			timeDemo;
	int					timeDemoStartTime;
	int					numDemoFrames;		// for timeDemo and demoShot
	int					demoTimeOffset;
	renderView_t		currentDemoRenderView;

	idSWF *				loadGUI;
	int					nextLoadTip;
	bool				isHellMap;
	bool				defaultLoadscreen;
	arcStaticList<int, LOAD_TIP_COUNT> loadTipList;

	const arcMaterial *	splashScreen;

	const arcMaterial *	whiteMaterial;

	const arcMaterial *	wipeMaterial;
	int					wipeStartTime;
	int					wipeStopTime;
	bool				wipeHold;
	bool				wipeForced;		// used for the PS3 to start an early wipe while we are accessing saved game data

	idGameThread		gameThread;				// the game and draw code can be run in parallel

	// com_speeds times
	int					count_numGameFrames;	// total number of game frames that were run
	int					time_gameFrame;			// game logic time
	int					time_maxGameFrame;		// maximum single frame game logic time
	int					time_gameDraw;			// game present time
	uint64				time_frontend;			// renderer frontend time
	uint64				time_backend;			// renderer backend time
	uint64				time_shadows;			// renderer backend waiting for shadow volumes to be created
	uint64				time_gpu;				// total gpu time, at least for PC

	// Used during loading screens
	int					lastPacifierGuiTime;
	bool				lastPacifierDialogState;

	bool				showShellRequested;
private:
	void	InitCommands();
	void	InitSIMD();
	void	AddStartupCommands();
	void	ParseCommandLine( int argc, const char * const * argv );
	bool	SafeMode();
	void	CloseLogFile();
	void	WriteConfiguration();
	void	DumpWarnings();
	void	LoadARCEngineDLL();
	void	UnloadARCEngineDLL();
	void	CleanupShell();
	void	RenderBink( const char * path );
	void	RenderSplash();
	void	FilterLangList( arcStringList* list, arcNetString lang );
	void	CheckStartupStorageRequirements();

	void	ExitMenu();
	bool	MenuEvent( const sysEvent_t * event );

	void	StartMenu( bool playIntro = false );
	void	GuiFrameEvents();

	void	BeginAVICapture( const char *name );
	void	EndAVICapture();

	void	AdvanceRenderDemo( bool singleFrameOnly );

	void	ProcessGameReturn( const gameReturn_t & ret );

	void	ExecuteReliableMessages();


	// Snapshot interpolation
	void	ProcessSnapshot( ARCSnapShot & ss );
	int		CalcSnapTimeBuffered( int & totalBufferedTime, int & totalRecvTime  );
	void	ProcessNextSnapshot();
	//void	InterpolateSnapshot( netTimes_t & prev, netTimes_t & next, float fraction, bool predict );
	//void	ResetNetworkingState();

	//int		NetworkFrame();
	void	SendUsercmds( int localClientNum );

	void	LoadLoadingGui(const char *mapName, bool & hellMap );

	// Meant to be used like:
	// while ( waiting ) { BusyWait(); }
	//void	BusyWait();

	void	ExecuteMapChange();
	void	UnloadMap();

	void	Stop( bool resetSession = true );

	// called by Draw when the scene to scene wipe is still running
	void	DrawWipeModel();
	void	StartWipe( const char *materialName, bool hold = false);
	void	CompleteWipe();
	void	ClearWipe();

	void	MoveToNewMap( const char * mapName, bool devmap );

	void	PlayIntroGui();

	void	ScrubSaveGameFileName( arcNetString &saveFileName ) const;
};

extern arcCommonLocal commonLocal;