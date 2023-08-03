#include "/idlib/Lib.h"
#pragma hdrstop

#include "Common_local.h"
#include "ConsoleHistory.h"

#include "../sound/sound.h"
#include "../sys/sys_savegame.h"

#if defined( _DEBUG )
	#define BUILD_DEBUG "-debug"
#else
	#define BUILD_DEBUG ""
#endif

struct version_s {
			version_s() { sprintf( string, "%s.%d%s %s %s %s", ENGINE_VERSION, BUILD_NUMBER, BUILD_DEBUG, BUILD_STRING, __DATE__, __TIME__ ); }
	char	string[256];
} version;

anCVarSystem com_version( "si_version", version.string, CVAR_SYSTEM|CVAR_ROM|CVAR_SERVERINFO, "engine version" );
anCVarSystem com_forceGenericSIMD( "com_forceGenericSIMD", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "force generic platform independent SIMD" );

#ifdef ID_RETAIL
anCVarSystem com_allowConsole( "com_allowConsole", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_INIT, "allow toggling console with the tilde key" );
#else
anCVarSystem com_allowConsole( "com_allowConsole", "1", CVAR_BOOL | CVAR_SYSTEM | CVAR_INIT, "allow toggling console with the tilde key" );
#endif

anCVarSystem com_developer( "developer", "0", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "developer mode" );
anCVarSystem com_speeds( "com_speeds", "0", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "show engine timings" );
anCVarSystem com_showFPS( "com_showFPS", "0", CVAR_BOOL|CVAR_SYSTEM|CVAR_ARCHIVE|CVAR_NOCHEAT, "show frames rendered per second" );
anCVarSystem com_showMemoryUsage( "com_showMemoryUsage", "0", CVAR_BOOL|CVAR_SYSTEM|CVAR_NOCHEAT, "show total and per frame memory usage" );
anCVarSystem com_updateLoadSize( "com_updateLoadSize", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "update the load size after loading a map" );

anCVarSystem com_productionMode( "com_productionMode", "0", CVAR_SYSTEM | CVAR_BOOL, "0 - no special behavior, 1 - building a production build, 2 - running a production build" );
anCVarSystem preload_CommonAssets( "preload_CommonAssets", "1", CVAR_SYSTEM | CVAR_BOOL, "preload common assets" );

extern anCVarSystem g_demoMode;

anCVarSystem com_engineHz( "com_engineHz", "60", CVAR_FLOAT | CVAR_ARCHIVE, "Frames per second the engine runs at", 10.0f, 1024.0f );
float com_engineHz_latched = 60.0f; // Latched version of cvar, updated between map loads
int64 com_engineHz_numerator = 100LL * 1000LL;
int64 com_engineHz_denominator = 100LL * 60LL;

HWND com_hwndMsg = nullptr;

#ifdef __ARCENGINE_DLL__
ARCEngine *		game = nullptr;
aRcEngineEditors *engineEdit = nullptr;
#endif

anCommonLocal	commonLocal;
anCommon *		common = &commonLocal;

anCVarSystem com_skipIntroVideos( "com_skipIntroVideos", "0", CVAR_BOOL , "skips intro videos" );

/*
==================
anCommonLocal::anCommonLocal
==================
*/
anCommonLocal::anCommonLocal() :
	readSnapshotIndex( 0 ),
	writeSnapshotIndex( 0 ),
	optimalTimeBuffered( 0.0f ),
	optimalTimeBufferedWindow( 0.0f ),
	optimalPCTBuffer( 0.5f ),
	lastPacifierGuiTime( 0 ),
	lastPacifierDialogState( false ),
	showShellRequested( false ),

	snapTimeBuffered	= 0.0f;
	effectiveSnapRate	= 0.0f;
	totalBufferedTime	= 0;
	totalRecvTime		= 0;

	com_fullyInitialized = false;
	com_refreshOnPrint = false;
	com_errorEntered = ERP_NONE;
	com_shuttingDown = false;

	logFile = nullptr;

	strcpy( errorMessage, "" );

	rd_buffer = nullptr;
	rd_buffersize = 0;
	rd_flush = nullptr;

	engineDLL = 0;

	loadGUI = nullptr;
	nextLoadTip = 0;
	wipeForced = false;
	defaultLoadscreen = false;

	menuSoundWorld = nullptr;

	insideUpdateScreen = false;
	insideExecuteMapChange = false;

	mapSpawnData.savegameFile = nullptr;

	currentMapName.Clear();

	renderWorld = nullptr;
	soundWorld = nullptr;
	menuSoundWorld = nullptr;

	engineFrame = 0;
	engineTimeResidual = 0;
	syncNextEngineFrame = true;
	mapSpawned = false;

	//nextSnapshotSendTime = 0;
	nextUsercmdSendTime = 0;

	//clientPrediction = 0;

	saveFile = nullptr;
	stringsFile = nullptr;

	ClearWipe();
}

/*
==================
anCommonLocal::Quit
==================
*/
void anCommonLocal::Quit() {
	// don't try to shutdown if we are in a recursive error
	if ( !com_errorEntered ) {
		Shutdown();
	}
	Sys_Quit();
}

/*
============================================================================

COMMAND LINE FUNCTIONS

+ characters separate the commandLine string into multiple console
command lines.

All of these are valid:

doom +set test blah +map test
doom set test blah+map test
doom set test blah + map test

============================================================================
*/

#define		MAX_CONSOLE_LINES	32
int			com_numConsoleLines;
anCommandArgs	com_consoleLines[MAX_CONSOLE_LINES];

/*
==================
anCommonLocal::ParseCommandLine
==================
*/
void anCommonLocal::ParseCommandLine( int argc, const char *const * argv ) {
	com_numConsoleLines = 0;
	int current_count = 0;

	// API says no program path
	for ( int i = 0; i < argc; i++ ) {
	if ( argv[i][ 0 ] == '+' ) {
			com_numConsoleLines++;
			com_consoleLines[ com_numConsoleLines-1 ].AppendArg( argv[i] + 1 );
		} else {
			if ( !com_numConsoleLines ) {
				com_numConsoleLines++;
			}
			com_consoleLines[ com_numConsoleLines-1 ].AppendArg( argv[i] );
		}
	}
}

/*
==================
anCommonLocal::SafeMode

Check for "safe" on the command line, which will
skip loading of config file (DoomConfig.cfg)
==================
*/
bool anCommonLocal::SafeMode() {
	for ( int i = 0; i < com_numConsoleLines; i++ ) {
		if ( !anString::Icmp( com_consoleLines[i].Argv(0 ), "safe" )
			|| !anString::Icmp( com_consoleLines[i].Argv(0 ), "cvar_restart" ) ) {
			com_consoleLines[i].Clear();
			return true;
		}
	}
	return false;
}

/*
==================
anCommonLocal::StartupVariable

Searches for command line parameters that are set commands.
If match is not nullptr, only that cvar will be looked for.
That is necessary because cddir and basedir need to be set
before the filesystem is started, but all other sets should
be after execing the config and default.
==================
*/
void anCommonLocal::StartupVariable( const char *match ) {
	int i = 0;
	while (	i < com_numConsoleLines ) {
		if ( strcmp( com_consoleLines[i].Argv( 0 ), "set" ) != 0 ) {
			i++;
			continue;
		}
		const char *s = com_consoleLines[i].Argv(1 );

		if ( !match || !anString::Icmp( s, match ) ) {
			cvarSystem->SetCVarString( s, com_consoleLines[i].Argv( 2 ) );
		}
		i++;
	}
}

/*
==================
anCommonLocal::AddStartupCommands

Adds command line parameters as script statements
Commands are separated by + signs

Returns true if any late commands were added, which
will keep the demoloop from immediately starting
==================
*/
void anCommonLocal::AddStartupCommands() {
	// quote every token, so args with semicolons can work
	for ( int i = 0; i < com_numConsoleLines; i++ ) {
		if ( !com_consoleLines[i].Argc() ) {
			continue;
		}
		// directly as tokenized so nothing gets screwed
		cmdSystem->BufferCommandArgs( CMD_EXEC_APPEND, com_consoleLines[i] );
	}
}

/*
==================
anCommonLocal::WriteConfigToFile
==================
*/
void anCommonLocal::WriteConfigToFile( const char *filename ) {
	anFile * f = fileSystem->OpenFileWrite( filename );
	if ( !f ) {
		Printf ( "Couldn't write %s.\n", filename );
		return;
	}

	idKeyInput::WriteBindings( f );
	cvarSystem->WriteFlaggedVariables( CVAR_ARCHIVE, "set", f );
	fileSystem->CloseFile( f );
}

/*
===============
anCommonLocal::WriteConfiguration

Writes key bindings and archived cvars to config file if modified
===============
*/
void anCommonLocal::WriteConfiguration() {
	// if we are quiting without fully initializing, make sure
	// we don't write out anything
	if ( !com_fullyInitialized ) {
		return;
	}

	if ( !( cvarSystem->GetModifiedFlags() & CVAR_ARCHIVE ) ) {
		return;
	}
	cvarSystem->ClearModifiedFlags( CVAR_ARCHIVE );

	// save to the profile
	idLocalUser * user = session->GetSignInManager().GetMasterLocalUser();
	if ( user != nullptr ) {
		user->SaveProfileSettings();
	}

#ifdef CONFIG_FILE
	// disable printing out the "Writing to:" message
	bool developer = com_developer.GetBool();
	com_developer.SetBool( false );

	WriteConfigToFile( CONFIG_FILE );

	// restore the developer cvar
	com_developer.SetBool( developer );
#endif
}

/*
===============
KeysFromBinding()
Returns the key bound to the command
===============
*/
const char* anCommonLocal::KeysFromBinding( const char *bind ) {
	return idKeyInput::KeysFromBinding( bind );
}

/*
===============
BindingFromKey()
Returns the binding bound to key
===============
*/
const char* anCommonLocal::BindingFromKey( const char *key ) {
	return idKeyInput::BindingFromKey( key );
}

/*
===============
ButtonState()
Returns the state of the button
===============
*/
int	anCommonLocal::ButtonState( int key ) {
	return usercmdGen->ButtonState(key);
}

/*
===============
ButtonState()
Returns the state of the key
===============
*/
int	anCommonLocal::KeyState( int key ) {
	return usercmdGen->KeyState(key);
}

/*
============
ARCCmdSysLocal::PrintMemInfo_f

This prints out memory debugging data
============
*/
CONSOLE_COMMAND( printMemInfo, "prints memory debugging data", nullptr ) {
	MemInfo_t mi;
	memset( &mi, 0, sizeof( mi ) );
	mi.filebase = commonLocal.GetCurrentMapName();

	renderSystem->PrintMemInfo( &mi ); // textures and models
	soundSystem->PrintMemInfo( &mi ); // sounds

	common->Printf( " Used image memory: %s bytes\n", anString::FormatNumber( mi.imageAssetsTotal ).c_str() );
	mi.assetTotals += mi.imageAssetsTotal;

	common->Printf( " Used model memory: %s bytes\n", anString::FormatNumber( mi.modelAssetsTotal ).c_str() );
	mi.assetTotals += mi.modelAssetsTotal;

	common->Printf( " Used sound memory: %s bytes\n", anString::FormatNumber( mi.soundAssetsTotal ).c_str() );
	mi.assetTotals += mi.soundAssetsTotal;

	common->Printf( " Used asset memory: %s bytes\n", anString::FormatNumber( mi.assetTotals ).c_str() );

	// write overview file
	anFile *f;

	f = fileSystem->OpenFileAppend( "maps/printmeminfo.txt" );
	if ( !f ) {
		return;
	}

	f->Printf( "total(%s ) image(%s ) model(%s ) sound(%s ): %s\n", anString::FormatNumber( mi.assetTotals ).c_str(), anString::FormatNumber( mi.imageAssetsTotal ).c_str(),
		anString::FormatNumber( mi.modelAssetsTotal ).c_str(), anString::FormatNumber( mi.soundAssetsTotal ).c_str(), mi.filebase.c_str() );

	fileSystem->CloseFile( f );
}

/*
==================
Com_Error_f

Just throw a fatal error to test error shutdown procedures.
==================
*/
CONSOLE_COMMAND( error, "causes an error", nullptr ) {
	if ( !com_developer.GetBool() ) {
		commonLocal.Printf( "error may only be used in developer mode\n" );
		return;
	}

	if ( args.Argc() > 1 ) {
		commonLocal.FatalError( "Testing fatal error" );
	} else {
		commonLocal.Error( "Testing drop error" );
	}
}

/*
==================
Com_Freeze_f

Just freeze in place for a given number of seconds to test error recovery.
==================
*/
CONSOLE_COMMAND( freeze, "freezes the game for a number of seconds", nullptr ) {
	float	s;
	int		start, now;

	if ( args.Argc() != 2 ) {
		commonLocal.Printf( "freeze <seconds>\n" );
		return;
	}

	if ( !com_developer.GetBool() ) {
		commonLocal.Printf( "freeze may only be used in developer mode\n" );
		return;
	}

	s = atof( args.Argv(1 ) );

	start = eventLoop->Milliseconds();

	while ( 1 ) {
		now = eventLoop->Milliseconds();
		if ( ( now - start ) * 0.001f > s ) {
			break;
		}
	}
}

/*
=================
Com_Crash_f

A way to force a bus error for development reasons
=================
*/
CONSOLE_COMMAND( crash, "causes a crash", nullptr ) {
	if ( !com_developer.GetBool() ) {
		commonLocal.Printf( "crash may only be used in developer mode\n" );
		return;
	}

	* ( int*) 0 = 0x12345678;
}

/*
=================
Com_Quit_f
=================
*/
CONSOLE_COMMAND_SHIP( quit, "quits the game", nullptr ) {
	commonLocal.Quit();
}
CONSOLE_COMMAND_SHIP( exit, "exits the game", nullptr ) {
	commonLocal.Quit();
}

/*
===============
Com_WriteConfig_f

Write the config file to a specific name
===============
*/
CONSOLE_COMMAND( writeConfig, "writes a config file", nullptr ) {
	anString	filename;

	if ( args.Argc() != 2 ) {
		commonLocal.Printf( "Usage: writeconfig <filename>\n" );
		return;
	}

	filename = args.Argv(1 );
	filename.DefaultFileExtension( ".cfg" );
	commonLocal.Printf( "Writing %s.\n", filename.c_str() );
	commonLocal.WriteConfigToFile( filename );
}

/*
========================
anCommonLocal::CheckStartupStorageRequirements
========================
*/
void anCommonLocal::CheckStartupStorageRequirements() {
	int64 availableSpace = 0;
	// ------------------------------------------------------------------------
	// Savegame and Profile required storage
	// ------------------------------------------------------------------------
	{
		// Make sure the save path exists in case it was deleted.
		// If the path cannot be created we can safely assume there is no
		// free space because in that case nothing can be saved anyway.
		const char *savepath = cvarSystem->GetCVarString( "fs_savepath" );
		anString directory = savepath;
		//anString directory = fs_savepath.GetString();
		directory += "\\";	// so it doesn't think the last part is a file and ignores in the directory creation
		fileSystem->CreateOSPath( directory );

		// Get the free space on the save path.
		availableSpace = Sys_GetDriveFreeSpaceInBytes( savepath );

		// If free space fails then get space on drive as a fall back
		// (the directory will be created later anyway)
		if ( availableSpace <= 1 ) {
			anString savePath( savepath );
			if ( savePath.Length() >= 3 ) {
				if ( savePath[ 1 ] == ':' && savePath[ 2 ] == '\\' &&
					( ( savePath[ 0 ] >= 'A' && savePath[ 0 ] <= 'Z' ) ||
					( savePath[ 0 ] >= 'a' && savePath[ 0 ] <= 'z' ) ) ) {
						savePath = savePath.Left( 3 );
						availableSpace = Sys_GetDriveFreeSpaceInBytes( savePath );
				}
			}
		}
	}

	const int MIN_SAVE_STORAGE_PROFILE		= 1024 * 1024;
	const int MIN_SAVE_STORAGE_SAVEGAME		= MIN_SAVEGAME_SIZE_BYTES;

	uint64 requiredSizeBytes = MIN_SAVE_STORAGE_SAVEGAME + MIN_SAVE_STORAGE_PROFILE;

	anLibrary::Printf( "requiredSizeBytes: %lld\n", requiredSizeBytes );

	if ( ( int64 )( requiredSizeBytes - availableSpace ) > 0 ) {
		class idSWFScriptFunction_Continue : public idSWFScriptFunction_RefCounted {
		public:
			virtual ~idSWFScriptFunction_Continue() {}
			idSWFScriptVar Call( idSWFScriptObject * thisObject, const idSWFParmList & parms ) {
				common->Dialog().ClearDialog( GDM_INSUFFICENT_STORAGE_SPACE );
				common->Quit();
				return idSWFScriptVar();
			}
		};

		arcStaticList< idSWFScriptFunction *, 4 > callbacks;
		arcStaticList< anStringId, 4 > optionText;
		callbacks.Append( new (TAG_SWF) idSWFScriptFunction_Continue() );
		optionText.Append( anStringId( "#STR_SWF_ACCEPT" ) );

		// build custom space required string
		// #str_dlg_space_required ~= "There is insufficient storage available.  Please free %s and try again."
		anString format = anStringId( "#str_dlg_startup_insufficient_storage" ).GetLocalizedString();
		anString size;
		if ( requiredSizeBytes > ( 1024 * 1024 ) ) {
			size = va( "%.1f MB", ( float )requiredSizeBytes / ( 1024.0f * 1024.0f ) + 0.1f );	// +0.1 to avoid truncation
		} else {
			size = va( "%.1f KB", ( float )requiredSizeBytes / 1024.0f + 0.1f );
		}
		anString msg = va( format.c_str(), size.c_str() );

		common->Dialog().AddDynamicDialog( GDM_INSUFFICENT_STORAGE_SPACE, callbacks, optionText, true, msg );
	}
}

/*
===============
anCommonLocal::FilterLangList
===============
*/
void anCommonLocal::FilterLangList( anStringList* list, anString lang ) {
	anString temp;
	for ( int i = 0; i < list->Num(); i++ ) {
		temp = ( *list )[i];
		temp = temp.Right( temp.Length()-strlen( "strings/" ) );
		temp = temp.Left( lang.Length() );
		if ( anString::Icmp( temp, lang ) != 0 ) {
			list->RemoveIndex( i );
			i--;
		}
	}
}

/*
===============
anCommonLocal::InitLanguageDict
===============
*/
extern anCVarSystem sys_lang;
void anCommonLocal::InitLanguageDict() {
	anString fileName;

	//D3XP: Instead of just loading a single lang file for each language
	//we are going to load all files that begin with the language name
	//similar to the way pak files work. So you can place english001.lang
	//to add new strings to the english language dictionary
	anFileList*	langFiles;
	langFiles =  fileSystem->ListFilesTree( "strings", ".lang", true );

	anStringList langList = langFiles->GetList();

	// Loop through the list and filter
	anStringList currentLangList = langList;
	FilterLangList( &currentLangList, sys_lang.GetString() );

	if ( currentLangList.Num() == 0 ) {
		// reset to english and try to load again
		sys_lang.SetString( ID_LANG_ENGLISH );
		currentLangList = langList;
		FilterLangList( &currentLangList, sys_lang.GetString() );
	}

	ARCLocalization::ClearDictionary();
	for ( int i = 0; i < currentLangList.Num(); i++ ) {
		//common->Printf( "%s\n", currentLangList[i].c_str() );
		const byte * buffer = nullptr;
		int len = fileSystem->ReadFile( currentLangList[i], (void**)&buffer );
		if ( len <= 0 ) {
			assert( false && "couldn't read the language dict file" );
			break;
		}
		ARCLocalization::LoadDictionary( buffer, len, currentLangList[i] );
		fileSystem->FreeFile( (void *)buffer );
	}

	fileSystem->FreeFileList( langFiles );
}

/*
=================
ReloadLanguage_f
=================
*/
CONSOLE_COMMAND( reloadLanguage, "reload language dict", nullptr ) {
	commonLocal.InitLanguageDict();
}

#include "../renderer/Image.h"

/*
=================
Com_StartBuild_f
=================
*/
CONSOLE_COMMAND( startBuild, "prepares to make a build", nullptr ) {
	globalImages->StartBuild();
}

/*
=================
Com_FinishBuild_f
=================
*/
CONSOLE_COMMAND( finishBuild, "finishes the build process", nullptr ) {
	if ( game ) {
		game->CacheDictionaryMedia( nullptr );
	}
	globalImages->FinishBuild( ( args.Argc() > 1 ) );
}

/*
=================
anCommonLocal::RenderSplash
=================
*/
void anCommonLocal::RenderSplash() {
	const float sysWidth = renderSystem->GetWidth() * renderSystem->GetPixelAspect();
	const float sysHeight = renderSystem->GetHeight();
	const float sysAspect = sysWidth / sysHeight;
	const float splashAspect = 16.0f / 9.0f;
	const float adjustment = sysAspect / splashAspect;
	const float barHeight = ( adjustment >= 1.0f ) ? 0.0f : ( 1.0f - adjustment ) * ( float )SCREEN_HEIGHT * 0.25f;
	const float barWidth = ( adjustment <= 1.0f ) ? 0.0f : ( adjustment - 1.0f ) * ( float )SCREEN_WIDTH * 0.25f;
	if ( barHeight > 0.0f ) {
		renderSystem->SetColor( colorBlack );
		renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, barHeight, 0, 0, 1, 1, whiteMaterial );
		renderSystem->DrawStretchPic( 0, SCREEN_HEIGHT - barHeight, SCREEN_WIDTH, barHeight, 0, 0, 1, 1, whiteMaterial );
	}
	if ( barWidth > 0.0f ) {
		renderSystem->SetColor( colorBlack );
		renderSystem->DrawStretchPic( 0, 0, barWidth, SCREEN_HEIGHT, 0, 0, 1, 1, whiteMaterial );
		renderSystem->DrawStretchPic( SCREEN_WIDTH - barWidth, 0, barWidth, SCREEN_HEIGHT, 0, 0, 1, 1, whiteMaterial );
	}
	renderSystem->SetColor4( 1, 1, 1, 1 );
	renderSystem->DrawStretchPic( barWidth, barHeight, SCREEN_WIDTH - barWidth * 2.0f, SCREEN_HEIGHT - barHeight * 2.0f, 0, 0, 1, 1, splashScreen );

	const setBufferCommand_t * cmd = renderSystem->SwapCommandBuffers( &time_frontend, &time_backend, &time_shadows, &time_gpu );
	renderSystem->RenderCommandBuffers( cmd );
}

/*
=================
anCommonLocal::InitSIMD
=================
*/
void anCommonLocal::InitSIMD() {
	arcSIMD::InitProcessor( "SIMD", com_forceGenericSIMD.GetBool() );
	com_forceGenericSIMD.ClearModified();
}

/*
=================
anCommonLocal::LoadARCEngineDLL
=================
*/
void anCommonLocal::LoadARCEngineDLL() {
#ifdef __ARCENGINE_DLL__
	char			dllPath[ MAX_OSPATH ];

	engineImport_t	engineImport;
	engineExport_t	engineExport;
	GetGameAPI_t	GetEngineAPI;

	fileSystem->FindDLL( "EnginePath", dllPath, true );

	if ( !dllPath[ 0 ] ) {
		common->FatalError( "couldn't find Engine dynamic library" );
		return;
	}
	common->DPrintf( "Loading Engine DLL: '%s'\n", dllPath );
	engineDLL = sys->DLL_Load( dllPath );
	if ( !engineDLL ) {
		common->FatalError( "couldn't load Engine dynamic library" );
		return;
	}

	const char *functionName = "GetEnginesAPI";
	GetEngineAPI = (GetGameAPI_t) Sys_DLL_GetProcAddress( engineDLL, functionName );
	if ( !GetEngineAPI ) {
		Sys_DLL_Unload( engineDLL );
		engineDLL = nullptr;
		common->FatalError( "couldn't find Engine DLL API" );
		return;
	}

	engineImport.version					= ARCENGINE_API_VERSION;
	engineImport.sys						= ::sys;
	engineImport.common					= ::common;
	engineImport.cmdSystem				= ::cmdSystem;
	engineImport.cvarSystem				= ::cvarSystem;
	engineImport.fileSystem				= ::fileSystem;
	engineImport.renderSystem				= ::renderSystem;
	engineImport.soundSystem				= ::soundSystem;
	engineImport.renderModelManager		= ::renderModelManager;
	engineImport.uiManager				= ::uiManager;
	engineImport.declManager				= ::declManager;
	engineImport.SEASFileManager			= ::SEASFileManager;
	engineImport.collisionModelManager	= ::collisionModelManager;

	engineExport							= *GetEngineAPI( &engineImport );

	if ( engineExport.version != ARCENGINE_API_VERSION ) {
		Sys_DLL_Unload( engineDLL );
		engineDLL = nullptr;
		common->FatalError( "wrong Engine DLL API version" );
		return;
	}

	game								= engineExport.game;
	engineEdit							= engineExport.engineEdit;
#endif

	// initialize the game object
	if ( game != nullptr ) {
		game->Init();
	}
}

/*
=================
anCommonLocal::UnloadARCEngineDLL
=================
*/
void anCommonLocal::CleanupShell() {
	if ( game != nullptr ) {
		game->Shell_Cleanup();
	}
}

/*
=================
anCommonLocal::UnloadARCEngineDLL
=================
*/
void anCommonLocal::UnloadARCEngineDLL() {
	// shut down the game object
	if ( game != nullptr ) {
		game->Shutdown();
	}

	if ( engineDLL ) {
		Sys_DLL_Unload( engineDLL );
		engineDLL = nullptr;
	}
	game = nullptr;
	engineEdit = nullptr;
}

/*
=================
anCommonLocal::IsInitialized
=================
*/
bool anCommonLocal::IsInitialized() const {
	return com_fullyInitialized;
}

//======================================================================================

/*
=================
anCommonLocal::Init
=================
*/
void anCommonLocal::Init( int argc, const char *const * argv, const char *cmdline ) {
	try {
		// set interface pointers used by anLibrary
		anLibrary::sys			= sys;
		anLibrary::common		= common;
		anLibrary::cvarSystem	= cvarSystem;
		anLibrary::fileSystem	= fileSystem;

		// initialize anLibrary
		anLibrary::Init();

		// clear warning buffer
		ClearWarnings( GAME_NAME " initialization" );

		anLibrary::Printf( va( "Command line: %s\n", cmdline ) );
		//::MessageBox( nullptr, cmdline, "blah", MB_OK );
		// parse command line options
		anCommandArgs args;
		if ( cmdline ) {
			// tokenize if the OS doesn't do it for us
			args.TokenizeString( cmdline, true );
			argv = args.GetArgs( &argc );
		}
		ParseCommandLine( argc, argv );

		// init console command system
		cmdSystem->Init();

		// init CVar system
		cvarSystem->Init();

		// register all static CVars
		anCVarSystem::RegisterStaticVars();

		anLibrary::Printf( "QA Timing INIT: %06dms\n", Sys_Milliseconds() );

		// print engine version
		Printf( "%s\n", version.string );

		// initialize key input/binding, done early so bind command exists
		idKeyInput::Init();

		// init the console so we can take prints
		console->Init();

		// get architecture info
		Sys_Init();

		// override cvars from command line
		StartupVariable( nullptr );

		consoleUsed = com_allowConsole.GetBool();

		if ( Sys_AlreadyRunning() ) {
			Sys_Quit();
		}

		// initialize processor specific SIMD implementation
		InitSIMD();

		// initialize the file system
		fileSystem->Init();

		const char *defaultLang = Sys_DefaultLanguage();

		// Allow the system to set a default lanugage
		Sys_SetLanguageFromSystem();

		// Pre-allocate our 20 MB save buffer here on time, instead of on-demand for each save....

		saveFile.SetNameAndType( SAVEGAME_CHECKPOINT_FILENAME, SAVEGAMEFILE_BINARY );
		saveFile.PreAllocate( MIN_SAVEGAME_SIZE_BYTES );

		stringsFile.SetNameAndType( SAVEGAME_STRINGS_FILENAME, SAVEGAMEFILE_BINARY );
		stringsFile.PreAllocate( MAX_SAVEGAME_STRING_TABLE_SIZE );

		fileSystem->BeginLevelLoad( "_startup", saveFile.GetDataPtr(), saveFile.GetAllocated() );

		// initialize the declaration manager
		declManager->Init();

		// init journalling, etc
		eventLoop->Init();

		// init the parallel job manager
		parallelJobManager->Init();

		// exec the startup scripts
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "exec default.cfg\n" );

#ifdef CONFIG_FILE
		// skip the config file if "safe" is on the command line
		if ( !SafeMode() && !g_demoMode.GetBool() ) {
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "exec " CONFIG_FILE "\n" );
		}
#endif

		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "exec autoexec.cfg\n" );

		// run cfg execution
		cmdSystem->ExecuteCommandBuffer();

		// re-override anything from the config files with command line args
		StartupVariable( nullptr );

		// if any archived cvars are modified after this, we will trigger a writing of the config file
		cvarSystem->ClearModifiedFlags( CVAR_ARCHIVE );

		// init OpenGL, which will open a window and connect sound and input hardware
		renderSystem->InitOpenGL();

		// Support up to 2 digits after the decimal point
		com_engineHz_denominator = 100LL * com_engineHz.GetFloat();
		com_engineHz_latched = com_engineHz.GetFloat();

		// start the sound system, but don't do any hardware operations yet
		soundSystem->Init();

		// initialize the renderSystem data structures
		renderSystem->Init();

		whiteMaterial = declManager->FindMaterial( "_white" );

		if ( anString::Icmp( sys_lang.GetString(), ID_LANG_GERMAN ) == 0 ) {
			splashScreen = declManager->FindMaterial( "guis/assets/splash/" );
		} else if ( anString::Icmp( defaultLang, ID_LANG_GERMAN ) == 0 ) {
			splashScreen = declManager->FindMaterial( "guis/assets/splash/legal_figs" );
		} else {
			// Otherwise show it in english
			splashScreen = declManager->FindMaterial( "guis/assets/splash/legal_english" );
		}

		const int legalMinTime = 4000;
		const bool showVideo = ( !com_skipIntroVideos.GetBool () && fileSystem->UsingResourceFiles() );
		if ( showVideo ) {
			RenderBink( "video\\loadvideo.bik" );
			RenderSplash();
			RenderSplash();
		} else {
			anLibrary::Printf( "Skipping Intro Videos!\n" );
			// display the legal splash screen
			// No clue why we have to render this twice to show up...
			RenderSplash();
			RenderSplash();
		}

		int legalStartTime = Sys_Milliseconds();
		declManager->Init2();

		// initialize string database so we can use it for loading messages
		InitLanguageDict();

		// spawn the game thread, even if we are going to run without SMP
		// one meg stack, because it can parse decls from gui surfaces (unfortunately)
		// use a lower priority so job threads can run on the same core
		gameThread.StartWorkerThread( "Engine/Draw", CORE_1B, THREAD_BELOW_NORMAL, 0x100000 );
		// boost this thread's priority, so it will prevent job threads from running while
		// the render back end still has work to do

		// init the user command input code
		usercmdGen->Init();

		Sys_SetRumble( 0, 0, 0 );

		// initialize the user interfaces
		uiManager->Init();

		// load the game dll
		LoadARCEngineDLL();

		// On the PC touch them all so they get included in the resource build
		if ( !fileSystem->UsingResourceFiles() ) {
			declManager->FindMaterial( "guis/assets/splash/legal_english" );
			declManager->FindMaterial( "guis/assets/splash/legal_figs" );
			// register the japanese font so it gets included
			renderSystem->RegisterFont( "DFPHeiseiGothicW7" );
			// Make sure all videos get touched because you can bring videos from one map to another, they need to be included in all maps
			for ( int i = 0; i < declManager->GetNumDecls( DECL_VIDEO ); i++ ) {
				declManager->DeclByIndex( DECL_VIDEO, i );
			}
		}

		fileSystem->UnloadResourceContainer( "_ordered" );

		// the same anRenderWorld will be used for all games
		// and demos, insuring that level specific models
		// will be freed
		renderWorld = renderSystem->AllocRenderWorld();
		soundWorld = soundSystem->AllocSoundWorld( renderWorld );

		menuSoundWorld = soundSystem->AllocSoundWorld( nullptr );
		menuSoundWorld->PlaceListener( vec3_origin, mat3_identity, 0 );

		// init the session
		session->Initialize();
		session->InitializeSoundRelatedSystems();

		CreateMainMenu();

		commonDialog.Init();

		// load the console history file
		consoleHistory.LoadHistoryFile();

		AddStartupCommands();

		StartMenu( true );

		while ( Sys_Milliseconds() - legalStartTime < legalMinTime ) {
			RenderSplash();
			Sys_GenerateEvents();
			Sys_Sleep( 10 );
		};

		// print all warnings queued during initialization
		PrintWarnings();

		// remove any prints from the notify lines
		console->ClearNotifyLines();

		CheckStartupStorageRequirements();

		if ( preload_CommonAssets.GetBool() && fileSystem->UsingResourceFiles() ) {
			anPreloadManifest manifest;
			manifest.LoadManifest( "_common.preload" );
			globalImages->Preload( manifest, false );
			soundSystem->Preload( manifest );
		}

		fileSystem->EndLevelLoad();

		com_fullyInitialized = true;
		// No longer need the splash screen
		if ( splashScreen != nullptr ) {
			for ( int i = 0; i < splashScreen->GetNumStages(); i++ ) {
				anImage * image = splashScreen->GetStage( i )->texture.image;
				if ( image != nullptr ) {
					image->PurgeImage();
				}
			}
		}

		Printf( "--- Common Initialization Complete ---\n" );

		anLibrary::Printf( "Teck Engine Timing IIS: %06dms\n", Sys_Milliseconds() );
	} catch( arcExceptions & ) {
		Sys_Error( "ERROR: Initialization" );
	}
}

/*
=================
anCommonLocal::Shutdown
=================
*/
void anCommonLocal::Shutdown() {
	if ( com_shuttingDown ) {
		return;
	}
	com_shuttingDown = true;

	// Kill any pending saves...
	printf( "session->GetSaveGameManager().CancelToTerminate();\n" );
	session->GetSaveGameManager().CancelToTerminate();

	// kill sound first
	printf( "soundSystem->StopAllSounds();\n" );
	soundSystem->StopAllSounds();

	printf( "Stop();\n" );
	Stop();

	printf( "CleanupShell();\n" );
	CleanupShell();

	printf( "delete loadGUI;\n" );
	delete loadGUI;
	loadGUI = nullptr;

	printf( "delete renderWorld;\n" );
	delete renderWorld;
	renderWorld = nullptr;

	printf( "delete soundWorld;\n" );
	delete soundWorld;
	soundWorld = nullptr;

	printf( "delete menuSoundWorld;\n" );
	delete menuSoundWorld;
	menuSoundWorld = nullptr;

	// shut down the user interfaces
	printf( "uiManager->Shutdown();\n" );
	uiManager->Shutdown();

	// shut down the sound system
	printf( "soundSystem->Shutdown();\n" );
	soundSystem->Shutdown();

	// shut down the user command input code
	printf( "usercmdGen->Shutdown();\n" );
	usercmdGen->Shutdown();

	// shut down the event loop
	printf( "eventLoop->Shutdown();\n" );
	eventLoop->Shutdown();

	// shutdown the decl manager
	printf( "declManager->Shutdown();\n" );
	declManager->Shutdown();

	// shut down the renderSystem
	printf( "renderSystem->Shutdown();\n" );
	renderSystem->Shutdown();

	printf( "commonDialog.Shutdown();\n" );
	commonDialog.Shutdown();

	// unload the game dll
	printf( "UnloadEngineDLL();\n" );
	UnloadEngineDLL();

	printf( "saveFile.Clear( true );\n" );
	saveFile.Clear( true );
	printf( "stringsFile.Clear( true );\n" );
	stringsFile.Clear( true );

	// only shut down the log file after all output is done
	printf( "CloseLogFile();\n" );
	CloseLogFile();

	// shut down the file system
	printf( "fileSystem->Shutdown( false );\n" );
	fileSystem->Shutdown( false );

	// shut down non-portable system services
	printf( "Sys_Shutdown();\n" );
	Sys_Shutdown();

	// shut down the console
	printf( "console->Shutdown();\n" );
	console->Shutdown();

	// shut down the key system
	printf( "idKeyInput::Shutdown();\n" );
	idKeyInput::Shutdown();

	// shut down the cvar system
	printf( "cvarSystem->Shutdown();\n" );
	cvarSystem->Shutdown();

	// shut down the console command system
	printf( "cmdSystem->Shutdown();\n" );
	cmdSystem->Shutdown();

	// free any buffered warning messages
	printf( "ClearWarnings( ENGINE_NAME \" shutdown\" );\n" );
	ClearWarnings( ENGINE_NAME " shutdown" );
	printf( "warningCaption.Clear();\n" );
	warningCaption.Clear();
	printf( "errorList.Clear();\n" );
	errorList.Clear();

	// shutdown anLibrary
	printf( "anLibrary::ShutDown();\n" );
	anLibrary::ShutDown();
}

/*
========================
anCommonLocal::CreateMainMenu
========================
*/
void anCommonLocal::CreateMainMenu() {
	if ( game != nullptr ) {
		// note which media we are going to need to load
		declManager->BeginLevelLoad();
		renderSystem->BeginLevelLoad();
		soundSystem->BeginLevelLoad();
		uiManager->BeginLevelLoad();

		// create main inside an "empty" game level load - so assets get
		// purged automagically when we transition to a "real" map
		game->Shell_CreateMenu( false );
		game->Shell_Show( true );
		game->Shell_SyncWithSession();

		// load
		renderSystem->EndLevelLoad();
		soundSystem->EndLevelLoad();
		declManager->EndLevelLoad();
		uiManager->EndLevelLoad( "" );
	}
}

/*
===============
anCommonLocal::Stop

called on errors and game exits
===============
*/
void anCommonLocal::Stop( bool resetSession ) {
	ClearWipe();

	// clear mapSpawned and demo playing flags
	UnloadMap();

	soundSystem->StopAllSounds();

	insideUpdateScreen = false;
	insideExecuteMapChange = false;

	// drop all guis
	ExitMenu();
}

/*
===============
anCommonLocal::BusyWait
===============
*/
void anCommonLocal::BusyWait() {
	Sys_GenerateEvents();

	const bool captureToImage = false;
	UpdateScreen( captureToImage );
	session->Pump();
}

/*
========================
anCommonLocal::LeaveGame
========================
*/
void anCommonLocal::LeaveGame() {
	const bool captureToImage = false;
	UpdateScreen( captureToImage );

	Stop( false );

	CreateMainMenu();

	StartMenu();
}

/*
===============
anCommonLocal::ProcessEvent
===============
*/
bool anCommonLocal::ProcessEvent( const sysEvent_t *event ) {
	// hitting escape anywhere brings up the menu
	if ( game && game->IsInGame() ) {
		if ( event->evType == SE_KEY && event->evValue2 == 1 && ( event->evValue == K_ESCAPE || event->evValue == K_JOY9 ) ) {
			if ( !game->Shell_IsActive() ) {
				// menus / etc
				if ( MenuEvent( event ) ) {
					return true;
				}

				console->Close();

				StartMenu();
				return true;
			} else {
				console->Close();

				// menus / etc
				if ( MenuEvent( event ) ) {
					return true;
				}

				game->Shell_ClosePause();
			}
		}
	}

	// let the pull-down console take it if desired
	if ( console->ProcessEvent( event, false ) ) {
		return true;
	}
	if ( session->ProcessInputEvent( event ) ) {
		return true;
	}

	if ( Dialog().IsDialogActive() ) {
		Dialog().HandleDialogEvent( event );
		return true;
	}

	// menus / etc
	if ( MenuEvent( event ) ) {
		return true;
	}

	// if we aren't in a game, force the console to take it
	if ( !mapSpawned ) {
		console->ProcessEvent( event, true );
		return true;
	}

	// in game, exec bindings for all key downs
	if ( event->evType == SE_KEY && event->evValue2 == 1 ) {
		idKeyInput::ExecKeyBinding( event->evValue );
		return true;
	}

	return false;
}

/*
========================
anCommonLocal::ResetPlayerInput
========================
*/
void anCommonLocal::ResetPlayerInput( int playerIndex ) {
	userCmdMgr.ResetPlayer( playerIndex );
}

/*
========================
anCommonLocal::SwitchToGame
========================
*/
void anCommonLocal::SwitchToGame( currentGame_t newGame ) {
	idealCurrentGame = newGame;
}

/*
========================
anCommonLocal::PerformGameSwitch
========================
*/
void anCommonLocal::PerformGameswitch () {
		// Pause sound.
	if ( menuSoundWorld != nullptr ) {
			menuSoundWorld->Pause();
	}

	// The classics use the usercmd manager too, clear it.
	userCmdMgr.SetDefaults();

	if ( menuSoundWorld != nullptr ) {
		menuSoundWorld->UnPause();
	}
	currentGame = idealCurrentGame;
}

/*
==================
Common_WritePrecache_f
==================
*/
CONSOLE_COMMAND( writePrecache, "writes precache commands", nullptr ) {
	if ( args.Argc() != 2 ) {
		common->Printf( "USAGE: writePrecache <execFile>\n" );
		return;
	}
	anString	str = args.Argv(1 );
	str.DefaultFileExtension( ".cfg" );
	anFile *f = fileSystem->OpenFileWrite( str );
	declManager->WritePrecacheCommands( f );
	renderModelManager->WritePrecacheCommands( f );
	uiManager->WritePrecacheCommands( f );

	fileSystem->CloseFile( f );
}

/*
================
Common_Disconnect_f
================
*/
CONSOLE_COMMAND_SHIP( disconnect, "disconnects", nullptr ) {
}

/*
===============
Common_Hitch_f
===============
*/
CONSOLE_COMMAND( hitch, "hitches the game", nullptr ) {
	if ( args.Argc() == 2 ) {
		Sys_Sleep( atoi(args.Argv(1 ) ) );
	} else {
		Sys_Sleep( 100 );
	}
}

CONSOLE_COMMAND( showStringMemory, "shows memory used by strings", nullptr ) {
	anString::ShowMemoryUsage_f( args );
}
CONSOLE_COMMAND( showDictMemory, "shows memory used by dictionaries", nullptr ) {
	anDict::ShowMemoryUsage_f( args );
}
CONSOLE_COMMAND( listDictKeys, "lists all keys used by dictionaries", nullptr ) {
	anDict::ListKeys_f( args );
}
CONSOLE_COMMAND( listDictValues, "lists all values used by dictionaries", nullptr ) {
	anDict::ListValues_f( args );
}
CONSOLE_COMMAND( testSIMD, "test SIMD code", nullptr ) {
	arcSIMD::Test_f( args );
}
