#include "/idlib/precompiled.h"
#pragma hdrstop
#include "sys_local.h"

const char * sysLanguageNames[] = {
	"english", "spanish", "italian", "german", "french", "russian",
	"polish", "korean", "japanese", "chinese", NULL
};

arcCVarSystem sys_lang( "sys_lang", "english", CVAR_SYSTEM | CVAR_ARCHIVE,  "", sysLanguageNames, arcCmdSystem::ArgCompletion_String<sysLanguageNames> );

arcSystemLocal			sysLocal;
arcSystem *				sys = &sysLocal;

void arcSystemLocal::DebugPrintf( const char *fmt, ... ) {
	va_list argptr;

	va_start( argptr, fmt );
	Sys_DebugVPrintf( fmt, argptr );
	va_end( argptr );
}

void arcSystemLocal::DebugVPrintf( const char *fmt, va_list arg ) { Sys_DebugVPrintf( fmt, arg ); }
double arcSystemLocal::GetClockTicks( void ) { return Sys_GetClockTicks(); }
double arcSystemLocal::ClockTicksPerSecond( void ) { return Sys_ClockTicksPerSecond(); }
cpuid_t arcSystemLocal::GetProcessorId( void ) { return Sys_GetProcessorId(); }
const char *arcSystemLocal::GetProcessorString( void ) { return Sys_GetProcessorString(); }
const char *arcSystemLocal::FPU_GetState( void ) { return Sys_FPU_GetState(); }
bool arcSystemLocal::FPU_StackIsEmpty( void ) { return Sys_FPU_StackIsEmpty(); }
void arcSystemLocal::FPU_SetFTZ( bool enable ) { Sys_FPU_SetFTZ( enable ); }
void arcSystemLocal::FPU_SetDAZ( bool enable ) { Sys_FPU_SetDAZ( enable ); }
bool arcSystemLocal::LockMemory( void *ptr, int bytes ) { return Sys_LockMemory( ptr, bytes ); }
bool arcSystemLocal::UnlockMemory( void *ptr, int bytes ) { return Sys_UnlockMemory( ptr, bytes ); }

void arcSystemLocal::GetCallStack( address_t *callStack, const int callStackSize ) { Sys_GetCallStack( callStack, callStackSize ); }
const char * arcSystemLocal::GetCallStackStr( const address_t *callStack, const int callStackSize ) { return Sys_GetCallStackStr( callStack, callStackSize ); }
const char * arcSystemLocal::GetCallStackCurStr( int depth ) { return Sys_GetCallStackCurStr( depth ); }
void arcSystemLocal::ShutdownSymbols( void ) { Sys_ShutdownSymbols(); }
int arcSystemLocal::DLL_Load( const char *dllName ) { return Sys_DLL_Load( dllName ); }
void *arcSystemLocal::DLL_GetProcAddress( int dllHandle, const char *procName ) { return Sys_DLL_GetProcAddress( dllHandle, procName ); }
void arcSystemLocal::DLL_Unload( int dllHandle ) { Sys_DLL_Unload( dllHandle ); }

void arcSystemLocal::DLL_GetFileName( const char *baseName, char *dllName, int maxLength ) {
#ifdef _WIN32
	arcNetString::snPrintf( dllName, maxLength, "%s" CPUSTRING ".dll", baseName );
#elif defined( __linux__ )
	arcNetString::snPrintf( dllName, maxLength, "%s" CPUSTRING ".so", baseName );
#elif defined( MACOS_X )
	arcNetString::snPrintf( dllName, maxLength, "%s" ".dylib", baseName );
#else
#error OS define is required
#endif
}

sysEvent_t arcSystemLocal::GenerateMouseButtonEvent( int button, bool down ) {
	sysEvent_t ev;
	ev.evType = SE_KEY;
	// who did this dumbshit? SE_MOUSE not K?
	ev.evValue = SE_MOUSE1 + button - 1;
	ev.evValue2 = down;
	ev.evPtrLength = 0;
	ev.evPtr = NULL;
	return ev;
}

sysEvent_t arcSystemLocal::GenerateMouseMoveEvent( int deltax, int deltay ) {
	sysEvent_t ev;
	ev.evType = SE_MOUSE;
	ev.evValue = deltax;
	ev.evValue2 = deltay;
	ev.evPtrLength = 0;
	ev.evPtr = NULL;
	return ev;
}

void arcSystemLocal::FPU_EnableExceptions( int exceptions ) { Sys_FPU_EnableExceptions( exceptions ); }

const char *Sys_TimeStampToStr( ARC_TIME_T timeStamp ) {
	static char timeString[MAX_STRING_CHARS];
	timeString[0] = '\0';

	tm*	time = localtime( &timeStamp );
	arcNetString out;

	arcNetString lang = cvarSystem->GetCVarString( "sys_lang" );
	if ( lang.Icmp( "english" ) == 0 ) {
		// english gets "month/day/year  hour:min" + "am" or "pm"
		out = va( "%02d", time->tm_mon + 1 );
		out += "/";
		out += va( "%02d", time->tm_mday );
		out += "/";
		out += va( "%d", time->tm_year + 1900 );
		out += "\t";
		if ( time->tm_hour > 12 ) {
			out += va( "%02d", time->tm_hour - 12 );
		} else if ( time->tm_hour == 0 ) {
				out += "12";
		} else {
			out += va( "%02d", time->tm_hour );
		}
		out += ":";
		out +=va( "%02d", time->tm_min );
		if ( time->tm_hour >= 12 ) {
			out += "pm";
		} else {
			out += "am";
		}
	} else {
		// europeans get "day/month/year  24hour:min"
		out = va( "%02d", time->tm_mday );
		out += "/";
		out += va( "%02d", time->tm_mon + 1 );
		out += "/";
		out += va( "%d", time->tm_year + 1900 );
		out += "\t";
		out += va( "%02d", time->tm_hour );
		out += ":";
		out += va( "%02d", time->tm_min );
	}
	arcNetString::Copynz( timeString, out, sizeof( timeString ) );
	return timeString;
}