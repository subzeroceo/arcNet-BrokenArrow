#include "/idlib/Lib.h"
#pragma hdrstop
#include "sys_local.h"

const char *sysLanguageNames[] = {
	"english", "spanish", "italian", "german", "french", "russian",
	"polish", "korean", "japanese", "chinese", nullptr
};

anCVarSystem sys_lang( "sys_lang", "english", CVAR_SYSTEM | CVAR_ARCHIVE,  "", sysLanguageNames, arcCmdSystem::ArgCompletion_String<sysLanguageNames> );

anSystemLocal			sysLocal;
arcSystem *				sys = &sysLocal;

void anSystemLocal::DebugPrintf( const char *fmt, ... ) {
	va_list argptr;

	va_start( argptr, fmt );
	Sys_DebugVPrintf( fmt, argptr );
	va_end( argptr );
}

void anSystemLocal::DebugVPrintf( const char *fmt, va_list arg ) { Sys_DebugVPrintf( fmt, arg ); }
double anSystemLocal::GetClockTicks( void ) { return Sys_GetClockTicks(); }
double anSystemLocal::ClockTicksPerSecond( void ) { return Sys_ClockTicksPerSecond(); }
cpuid_t anSystemLocal::GetProcessorId( void ) { return Sys_GetProcessorId(); }
const char *anSystemLocal::GetProcessorString( void ) { return Sys_GetProcessorString(); }
const char *anSystemLocal::FPU_GetState( void ) { return Sys_FPU_GetState(); }
bool anSystemLocal::FPU_StackIsEmpty( void ) { return Sys_FPU_StackIsEmpty(); }
void anSystemLocal::FPU_SetFTZ( bool enable ) { Sys_FPU_SetFTZ( enable ); }
void anSystemLocal::FPU_SetDAZ( bool enable ) { Sys_FPU_SetDAZ( enable ); }
bool anSystemLocal::LockMemory( void *ptr, int bytes ) { return Sys_LockMemory( ptr, bytes ); }
bool anSystemLocal::UnlockMemory( void *ptr, int bytes ) { return Sys_UnlockMemory( ptr, bytes ); }

void anSystemLocal::GetCallStack( address_t *callStack, const int callStackSize ) { Sys_GetCallStack( callStack, callStackSize ); }
const char *anSystemLocal::GetCallStackStr( const address_t *callStack, const int callStackSize ) { return Sys_GetCallStackStr( callStack, callStackSize ); }
const char *anSystemLocal::GetCallStackCurStr( int depth ) { return Sys_GetCallStackCurStr( depth ); }
void anSystemLocal::ShutdownSymbols( void ) { Sys_ShutdownSymbols(); }
int anSystemLocal::DLL_Load( const char *dllName ) { return Sys_DLL_Load( dllName ); }
void *anSystemLocal::DLL_GetProcAddress( int dllHandle, const char *procName ) { return Sys_DLL_GetProcAddress( dllHandle, procName ); }
void anSystemLocal::DLL_Unload( int dllHandle ) { Sys_DLL_Unload( dllHandle ); }

void anSystemLocal::DLL_GetFileName( const char *baseName, char *dllName, int maxLength ) {
#ifdef _WIN32
	anString::snPrintf( dllName, maxLength, "%s" CPUSTRING ".dll", baseName );
#elif defined( __linux__ )
	anString::snPrintf( dllName, maxLength, "%s" CPUSTRING ".so", baseName );
#elif defined( MACOS_X )
	anString::snPrintf( dllName, maxLength, "%s" ".dylib", baseName );
#else
#error OS define is required
#endif
}

sysEvent_t anSystemLocal::GenerateMouseButtonEvent( int button, bool down ) {
	sysEvent_t ev;
	ev.evType = SE_KEY;
	// who did this dumbshit? SE_MOUSE not K?
	ev.evValue = SE_MOUSE1 + button - 1;
	ev.evValue2 = down;
	ev.evPtrLength = 0;
	ev.evPtr = nullptr;
	return ev;
}

sysEvent_t anSystemLocal::GenerateMouseMoveEvent( int deltax, int deltay ) {
	sysEvent_t ev;
	ev.evType = SE_MOUSE;
	ev.evValue = deltax;
	ev.evValue2 = deltay;
	ev.evPtrLength = 0;
	ev.evPtr = nullptr;
	return ev;
}

void anSystemLocal::FPU_EnableExceptions( int exceptions ) { Sys_FPU_EnableExceptions( exceptions ); }

const char *Sys_TimeStampToStr( ARC_TIME_T timeStamp ) {
	static char timeString[MAX_STRING_CHARS];
	timeString[0] = '\0';

	tm*	time = localtime( &timeStamp );
	anString out;

	anString lang = cvarSystem->GetCVarString( "sys_lang" );
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
	anString::Copynz( timeString, out, sizeof( timeString ) );
	return timeString;
}