/*
===============================================================================

	Preprocessor settings for compiling different versions.

===============================================================================
*/

// memory debugging
//#define ARC_REDIRECT_NEWDELETE
//#define ID_DEBUG_MEMORY
//#define ID_DEBUG_UNINITIALIZED_MEMORY

// if enabled, the console won't toggle upon ~, unless you start the binary with +set com_allowConsole 1
// Ctrl+Alt+~ will always toggle the console no matter what
#ifndef ID_CONSOLE_LOCK
	#if defined(_WIN32) || defined(MACOS_X)
		#ifdef _DEBUG
			#define ID_CONSOLE_LOCK 0
		#else
			#define ID_CONSOLE_LOCK 1
		#endif
	#else
		#define ID_CONSOLE_LOCK 0
	#endif
#endif

// useful for network debugging, turns off 'LAN' checks, all IPs are classified 'internet'
#ifndef ID_NOLANADDRESS
	#define ID_NOLANADDRESS 0
#endif

// let .dds be loaded from FS without altering pure state. only for developement.
#ifndef ID_PURE_ALLOWDDS
	#define ID_PURE_ALLOWDDS 0
#endif

#ifndef ID_ENABLE_CURL
	#define ID_ENABLE_CURL 1
#endif

// fake a pure client. useful to connect an all-debug client to a server
#ifndef ID_FAKE_PURE
	#define ID_FAKE_PURE 0
#endif

// verify checksums in clientinfo traffic
// NOTE: this makes the network protocol incompatible
#ifndef ID_CLIENTINFO_TAGS
	#define ID_CLIENTINFO_TAGS 0
#endif

// don't define ID_ALLOW_TOOLS when we don't want tool code in the executable.
#if defined( _WIN32 ) && !defined( ID_DEDICATED ) && !defined( ID_DEMO_BUILD )
	#define	ID_ALLOW_TOOLS
#endif

// don't do backtraces in release builds.
// atm, we have no useful way to reconstruct the trace, so let's leave it off
#define ID_BT_STUB
#ifndef ID_BT_STUB
	#if defined( __linux__ )
		#if defined( _DEBUG )
			#define ID_BT_STUB
		#endif
	#else
		#define ID_BT_STUB
	#endif
#endif

#ifndef ID_OPENAL
#	if ( defined(_WIN32) || defined(MACOS_X) ) && !defined( ID_DEDICATED )
#		define ID_OPENAL 1
#	else
#		define ID_OPENAL 0
#	endif
#endif

