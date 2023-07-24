#include "/idlib/precompiled.h"
#include "../sys/sys_local.h"
#pragma hdrstop

#include "TypeInfoGen.h"

arcDeclerationManager *		declManager = NULL;
arcEventLoop *		eventLoop = NULL;

int arcEventLoop::JournalLevel( void ) const { return 0; }

/*
==============================================================

	arcCommon

==============================================================
*/

#define STDIO_PRINT( pre, post )	\
	va_list argptr;					\
	va_start( argptr, fmt );		\
	printf( pre );					\
	vprintf( fmt, argptr );			\
	printf( post );					\
	va_end( argptr )


class arcCommonLocal : public arcCommon {
public:
							arcCommonLocal( void ) {}

	virtual void			Init( int argc, const char **argv, const char *cmdline ) {}
	virtual void			Shutdown( void ) {}
	virtual void			Quit( void ) {}
	virtual bool			IsInitialized( void ) const { return true; }
	virtual void			Frame( void ) {}
	virtual void			GUIFrame( bool execCmd, bool network  ) {}
	virtual void			Async( void ) {}
	virtual void			StartupVariable( const char *match, bool once ) {}
	virtual void			InitTool( const toolFlag_t tool, const arcDictionary *dict ) {}
	virtual void			ActivateTool( bool active ) {}
	virtual void			WriteConfigToFile( const char *filename ) {}
	virtual void			WriteFlaggedCVarsToFile( const char *filename, int flags, const char *setCmd ) {}
	virtual void			BeginRedirect( char *buffer, int buffersize, void (*flush)( const char * ) ) {}
	virtual void			EndRedirect( void ) {}
	virtual void			SetRefreshOnPrint( bool set ) {}
	virtual void			Printf( const char *fmt, ... ) { STDIO_PRINT( "", "" ); }
	virtual void			VPrintf( const char *fmt, va_list arg ) { vprintf( fmt, arg ); }
	virtual void			DPrintf( const char *fmt, ... ) { /*STDIO_PRINT( "", "" );*/ }
	virtual void			Warning( const char *fmt, ... ) { STDIO_PRINT( "WARNING: ", "\n" ); }
	virtual void			DWarning( const char *fmt, ...) { /*STDIO_PRINT( "WARNING: ", "\n" );*/ }
	virtual void			PrintWarnings( void ) {}
	virtual void			ClearWarnings( const char *reason ) {}
	virtual void			Error( const char *fmt, ... ) { STDIO_PRINT( "ERROR: ", "\n" ); exit(0 ); }
	virtual void			FatalError( const char *fmt, ... ) { STDIO_PRINT( "FATAL ERROR: ", "\n" ); exit(0 ); }
	virtual const arcLangDictionary *GetLanguageDict() { return NULL; }
	//virtual const char *	KeysFromBinding( const char *bind ) { return NULL; }
	//virtual const char *	BindingFromKey( const char *key ) { return NULL; }
	virtual const char *	KeysFromBinding( const char *bind, int layer ) { return NULL; }
	virtual const char *	BindingFromKey( const char *key, int layer ) { return NULL; }
	virtual int				ButtonState( int key ) { return 0; }
	virtual int				KeyState( int key ) { return 0; }

	virtual void InputInit( void ) {}
	virtual void Shutdown( void ) {}
	virtual	void LoadGraphics( void ) {}
	virtual void PrintMsg(const char * in_msg) {}
	virtual void DrawMsgs( void ) {}
	virtual void ClearMsgs( void ) {}
	virtual int GetComFrameTime( void ) { return 0; }
};

arcCVarSystem com_developer( "developer", "0", CVAR_BOOL|CVAR_SYSTEM, "developer mode" );

arcCommonLocal		commonLocal;
arcCommon *			common = &commonLocal;

/*
==============================================================

	arcSystem

==============================================================
*/

void			Sys_Mkdir( const char *path ) {}
ARC_TIME_T			Sys_FileTimeStamp( FILE *fp ) { return 0; }

#ifdef _WIN32

#include <io.h>
#include <direct.h>

const char *Sys_Cwd( void ) {
	static char cwd[1024];

	_getcwd( cwd, sizeof( cwd ) - 1 );
	cwd[sizeof( cwd ) - 1] = 0;

	int i = arcNetString::FindText( cwd, CD_BASEDIR, false );
	if ( i >= 0 ) {
		cwd[i + strlen( CD_BASEDIR )] = '\0';
	}

	return cwd;
}

const char *Sys_DefaultCDPath( void ) {
	return "";
}

const char *Sys_DefaultBasePath( void ) {
	return Sys_Cwd();
}

const char *Sys_DefaultSavePath( void ) {
	return cvarSystem->GetCVarString( "fs_basepath" );
}

const char *Sys_EXEPath( void ) {
	return "";
}

int Sys_ListFiles( const char *directory, const char *extension, arcStringList &list ) {
	arcNetString		search;
	struct _finddata_t findinfo;
	int			findhandle;
	int			flag;

	if ( !extension) {
		extension = "";
	}

	// passing a slash as extension will find directories
	if ( extension[0] == '/' && extension[1] == 0 ) {
		extension = "";
		flag = 0;
	} else {
		flag = _A_SUBDIR;
	}

	sprintf( search, "%s\\*%s", directory, extension );

	// search
	list.Clear();

	findhandle = _findfirst( search, &findinfo );
	if ( findhandle == -1 ) {
		return -1;
	}

	do {
		if ( flag ^ ( findinfo.attrib & _A_SUBDIR ) ) {
			list.Append( findinfo.name );
		}
	} while ( _findnext( findhandle, &findinfo ) != -1 );

	_findclose( findhandle );

	return list.Num();
}

#else

const char *	Sys_DefaultCDPath( void ) { return ""; }
const char *	Sys_DefaultBasePath( void ) { return ""; }
const char *	Sys_DefaultSavePath( void ) { return ""; }
int				Sys_ListFiles( const char *directory, const char *extension, arcStringList &list ) { return 0; }

#endif

xthreadInfo *	g_threads[MAX_THREADS];
int				g_thread_count;

void			Sys_CreateThread( xthread_t function, void *parms, xthreadPriority priority, xthreadInfo &info, const char *name, xthreadInfo *threads[MAX_THREADS], int *thread_count ) {}
void			Sys_DestroyThread( xthreadInfo& info ) {}

void			Sys_EnterCriticalSection( int index ) {}
void			Sys_LeaveCriticalSection( int index ) {}

void			Sys_WaitForEvent( int index ) {}
void			Sys_TriggerEvent( int index ) {}

/*
==============
arcSystemLocal stub
==============
*/
void			arcSystemLocal::DebugPrintf( const char *fmt, ... ) {}
void			arcSystemLocal::DebugVPrintf( const char *fmt, va_list arg ) {}

double			arcSystemLocal::GetClockTicks( void ) { return 0.0; }
double			arcSystemLocal::ClockTicksPerSecond( void ) { return 1.0; }
cpuid_t			arcSystemLocal::GetProcessorId( void ) { return ( cpuid_t )0; }
const char *	arcSystemLocal::GetProcessorString( void ) { return ""; }
const char *	arcSystemLocal::FPU_GetState( void ) { return ""; }
bool			arcSystemLocal::FPU_StackIsEmpty( void ) { return true; }
void			arcSystemLocal::FPU_SetFTZ( bool enable ) {}
void			arcSystemLocal::FPU_SetDAZ( bool enable ) {}

bool			arcSystemLocal::LockMemory( void *ptr, int bytes ) { return false; }
bool			arcSystemLocal::UnlockMemory( void *ptr, int bytes ) { return false; }

void			arcSystemLocal::GetCallStack( address_t *callStack, const int callStackSize ) { memset( callStack, 0, callStackSize * sizeof( callStack[0] ) ); }
const char *	arcSystemLocal::GetCallStackStr( const address_t *callStack, const int callStackSize ) { return ""; }
const char *	arcSystemLocal::GetCallStackCurStr( int depth ) { return ""; }
void			arcSystemLocal::ShutdownSymbols( void ) {}

int				arcSystemLocal::DLL_Load( const char *dllName ) { return 0; }
void *			arcSystemLocal::DLL_GetProcAddress( int dllHandle, const char *procName ) { return NULL; }
void			arcSystemLocal::DLL_Unload( int dllHandle ) { }
void			arcSystemLocal::DLL_GetFileName( const char *baseName, char *dllName, int maxLength ) { }

sysEvent_t		arcSystemLocal::GenerateMouseButtonEvent( int button, bool down ) { sysEvent_t ev; memset( &ev, 0, sizeof( ev ) ); return ev; }
sysEvent_t		arcSystemLocal::GenerateMouseMoveEvent( int deltax, int deltay ) { sysEvent_t ev; memset( &ev, 0, sizeof( ev ) ); return ev; }

void			arcSystemLocal::OpenURL( const char *url, bool quit ) { }
void			arcSystemLocal::StartProcess( const char *exeName, bool quit ) { }

void			arcSystemLocal::FPU_EnableExceptions( int exceptions ) { }

arcSystemLocal		sysLocal;
arcSystem *			sys = &sysLocal;


/*
==============================================================

	main

==============================================================
*/

int main( int argc, char** argv ) {
	arcNetString fileName, sourcePath;
	ARCInfoGen *generator;

	arcLibrary::common = common;
	arcLibrary::cvarSystem = cvarSystem;
	arcLibrary::fileSystem = fileSystem;
	arcLibrary::sys = sys;

	arcLibrary::Init();
	cmdSystem->Init();
	cvarSystem->Init();
	arcCVarSystem::RegisterStaticVars();
	fileSystem->Init();

	generator = new ARCInfoGen;


	if ( argc > 1 ) {
		sourcePath = arcNetString( "../" SOURCE_CODE_BASE_FOLDER"/" ) + argv[1];
	//} else {
		//sourcePath = "../" SOURCE_FOLDER"/game";
	}
	for ( int i = 3; i < argc; i++ ) {
		generator->AddDefine( argv[i] );
	} else {
		generator->AddDefine( "__cplusplus" );
		generator->AddDefine( "ENGINE_DLL" );
		generator->AddDefine( "ID_TYPEINFO" );
	}

	generator->CreateTypeInfo( sourcePath );
	generator->WriteTypeInfo( fileName );

	delete generator;

	fileName.Clear();
	sourcePath.Clear();

	fileSystem->Shutdown( false );
	cvarSystem->Shutdown();
	cmdSystem->Shutdown();
	arcLibrary::ShutDown();

	return 0;
}
