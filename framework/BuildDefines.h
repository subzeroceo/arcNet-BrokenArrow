/*
===============================================================================

	Preprocessor settings for compiling different versions.

===============================================================================
*/

typedef enum {
	CPUID_NONE							= 0x00000,
	CPUID_UNSUPPORTED					= 0x00001,	// unsupported (386/486)
	CPUID_GENERIC						= 0x00002,	// unrecognized processor
	CPUID_INTEL							= 0x00004,	// Intel
	CPUID_AMD							= 0x00008,	// AMD
	CPUID_MMX							= 0x00010,	// Multi Media Extensions
	CPUID_3DNOW							= 0x00020,	// 3DNow!
	CPUID_SSE							= 0x00040,	// Streaming SIMD Extensions
	CPUID_SSE2							= 0x00080,	// Streaming SIMD Extensions 2
	CPUID_SSE3							= 0x00100,	// Streaming SIMD Extentions 3 aka Prescott's New Instructions
	CPUID_ALTIVEC						= 0x00200,	// AltiVec
	CPUID_XENON							= 0x00400,	// Xenon
	CPUID_HTT							= 0x01000,	// Hyper-Threading Technology
	CPUID_CMOV							= 0x02000,	// Conditional Move (CMOV) and fast floating point comparison (FCOMI) instructions
	CPUID_FTZ							= 0x04000,	// Flush-To-Zero mode (denormal results are flushed to zero)
	CPUID_DAZ							= 0x08000,	// Denormals-Are-Zero mode (denormal source operands are set to zero)
	CPUID_PPC							= 0x40000	// PowerPC G4/G5
} cpuid_t;

#if defined(_WIN32)
#ifdef __INTEL_COMPILER
	#define ARC_STATICTEMPLATE
#else
	#define ARC_STATICTEMPLATE			static
#endif

#define ARC_INLINE						__forceinline
#define ARC_TLS							__declspec( thread )
#define ARC_DEPRECATED					__declspec( deprecated )

#include <basetsd.h>					// needed for UINT_PTR
#include <stddef.h>						// needed for offsetof
#include <memory.h>						// needed for memcmp

#if !defined(_WIN64)
	#define	BUILD_STRING				"win-x86"
	#define BUILD_OS_ID					0
	#define	CPUSTRING					"x86"
	#define CPU_EASYARGS				1
#else
	#define	BUILD_STRING				"win-x64"
	#define BUILD_OS_ID					0
	#define	CPUSTRING					"x64"
	#define CPU_EASYARGS				0
#endif

#define ALIGN16( x )					__declspec(align(16)) x
#define PACKED

#include <malloc.h>
#define _alloca16( x )					((void *)((((UINT_PTR)_alloca( (x)+15 )) + 15) & ~15))

#define PATHSEPARATOR_STR				"\\"
#define PATHSEPARATOR_CHAR				'\\'

#define assertmem( x, y )				assert( _CrtIsValidPointer( x, y, true ) )

#define ARC_AL_DYNAMIC
#endif

#ifndef BIT
#define BIT( num )				BITT<num>::VALUE
#endif

template< unsigned int B >
class BITT {
public:
	typedef enum bitValue_e {
		VALUE = 1 << B,
	} bitValue_t;
};

// Mac OSX
#if defined(MACOS_X) || defined(__APPLE__)
#ifdef __ppc__
	#define BUILD_STRING				"MacOSX-ppc"
	#define BUILD_OS_ID					1
	#define	CPUSTRING					"ppc"
	#define CPU_EASYARGS				0
#elif defined(__i386__)
	#define BUILD_STRING				"MacOSX-x86"
	#define BUILD_OS_ID					1
	#define	CPUSTRING					"x86"
	#define CPU_EASYARGS				1
#endif

#if defined(__i386__)
#define ALIGN16( x )					x __attribute__ ((aligned (16)))
#else
#define ALIGN16( x )					x
#endif

#ifdef __MWERKS__
#define PACKED
#include <alloca.h>
#else
#define PACKED							__attribute__((packed))
#endif

#define UINT_PTR						unsigned long

#define _alloca							alloca
#define _alloca16( x )					((void *)((((int)_alloca( (x)+15 )) + 15) & ~15))

#define PATHSEPARATOR_STR				"/"
#define PATHSEPARATOR_CHAR				'/'

#define __cdecl
#define ASSERT							assert
#define ASSERT							Assert

#define ARC_STATICTEMPLATE
#define ARC_INLINE						inline
#define ARC_DEPRECATED
// from gcc 4.0 manual:
// The __thread specifier may be used alone, with the extern or static specifiers, but with no other storage class specifier. When used with extern or static, __thread must appear immediately after the other storage class specifier.
// The __thread specifier may be applied to any global, file-scoped static, function-scoped static, or static data member of a class. It may not be applied to block-scoped automatic or non-static data member.
#define ARC_TLS							__thread
#define assertmem( x, y )
#endif

// Linux
#ifdef __linux__
#ifdef __i386__
	#define	BUILD_STRING				"linux-x86"
	#define BUILD_OS_ID					2
	#define CPUSTRING					"x86"
	#define CPU_EASYARGS				1
#elif defined(__ppc__)
	#define	BUILD_STRING				"linux-ppc"
	#define BUILD_OS_ID					2
	#define CPUSTRING					"ppc"
	#define CPU_EASYARGS				0
#endif

#include <stddef.h>						// needed for offsetof

#define UINT_PTR						unsigned long

#include <alloca.h>
#define _alloca							alloca
#define _alloca16( x )					((void *)((((int)_alloca( (x)+15 )) + 15) & ~15))

#define ALIGN16( x )					x __attribute__ ((aligned (16)))
#define PACKED							__attribute__((packed))

#define PATHSEPARATOR_STR				"/"
#define PATHSEPARATOR_CHAR				'/'

#define __cdecl
#define ASSERT							assert

#define ARC_STATICTEMPLATE
#define ARC_INLINE						inline
#define ARC_TLS							__thread
#define ARC_DEPRECATED

#define assertmem( x, y )

#define ARC_AL_DYNAMIC
#endif

template< typename T > ARC_INLINE void Swap( T& l, T& r ) {
	T temp = l;
	l = r;
	r = temp;
}

#if !defined( ARC_CONDITIONAL_ASSERT )
	#if !defined( _WIN32 )
		#define ARC_CONDITIONAL_ASSERT
	#endif
#endif

// memory debugging
#define ARC_REDIRECT_NEWDELETE
#define ARC_DEBUG_MEMORY
#define ARC_DEBUG_UNINITIALIZED_MEMORY

// if enabled, the console won't toggle upon ~, unless you start the binary with +set com_allowConsole 1
// Ctrl+Alt+~ will always toggle the console no matter what
#ifndef ARC_CONSOLE_LOCK
	#if defined(_WIN32) || defined(MACOS_X)
		#ifdef _DEBUG
			#define ARC_CONSOLE_LOCK 0
		#else
			#define ARC_CONSOLE_LOCK 1
		#endif
	#else
		#define ARC_CONSOLE_LOCK 0
	#endif
#endif

// build an exe with no CVAR_CHEAT controls
#ifndef ID_ALLOW_CHEATS
	#define ID_ALLOW_CHEATS 0
#endif

// useful for network debugging, turns off 'LAN' checks, all IPs are classified 'internet'
#ifndef ID_NOLANADDRESS
	#define ID_NOLANADDRESS 0
#endif

// let .dds be loaded from FS without altering pure state. only for developement.
#ifndef ARC_PURE_ALLOWDDS
	#define ARC_PURE_ALLOWDDS 0
#endif

#ifndef ARC_ENABLE_CURL
	#define ARC_ENABLE_CURL 1
#endif

// fake a pure client. useful to connect an all-debug client to a server
#ifndef ARC_FAKE_PURE
	#define ARC_FAKE_PURE 0
#endif

// don't define ID_ALLOW_TOOLS when we don't want tool code in the executable.
#if defined( _WIN32 ) && !defined( ID_DEDICATED ) && !defined( ARCNET_DEMO_BUILD )
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

#if !defined( COLLISION_USE_SHORT_EDGES )
	#define COLLISION_USE_SHORT_EDGES
#endif

#if !defined( COLLISION_SHORT_INDICES )
	#define COLLISION_SHORT_INDICES
#endif

#ifdef __cplusplus
#define ARC_TIME_T time_t
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#undef NULL
#define NULL 0
#endif


#if !defined( WIN32) || (WIN64 )
// disable some /analyze warnings here
#pragma warning( disable: 6255 )	// warning C6255: _alloca indicates failure by raising a stack overflow exception. Consider using _malloca instead. (Note: _malloca requires _freea.)
#pragma warning( disable: 6262 )	// warning C6262: Function uses '36924' bytes of stack: exceeds /analyze:stacksize'32768'. Consider moving some data to heap
#pragma warning( disable: 6326 )	// warning C6326: Potential comparison of a constant with another constant

#pragma warning( disable: 6031 )	//  warning C6031: Return value ignored
// this warning fires whenever you have two calls to new in a function, but we assume new never fails, so it is not relevant for us
#pragma warning( disable: 6211 )	// warning C6211: Leaking memory 'staticModel' due to an exception. Consider using a local catch block to clean up memory

// we want to fix all these at some point...
#pragma warning( disable: 6246 )	// warning C6246: Local declaration of 'es' hides declaration of the same name in outer scope. For additional information, see previous declaration at line '969' of 'w:\tech5\rage\game\ai\fsm\fsm_combat.cpp': Lines: 969
#pragma warning( disable: 6244 )	// warning C6244: Local declaration of 'viewList' hides previous declaration at line '67' of 'w:\tech5\engine\renderer\rendertools.cpp'

// win32 needs this, but 360 doesn't
#pragma warning( disable: 6540 )	// warning C6540: The use of attribute annotations on this function will invalidate all of its existing __declspec annotations [D:\tech5\engine\engine-10.vcxproj]

// checking format strings catches a LOT of errors
#include <CodeAnalysis\SourceAnnotations.h>
#define	VERIFY_FORMAT_STRING	[SA_FormatString(Style="printf" )]
// We need to inform the compiler that Error() and FatalError() will
// never return, so any conditions that leeds to them being called are
// guaranteed to be false in the following code
#define NO_RETURN __declspec(noreturn)
#include "../tools/comafx/StdAfx.h"
#include <winsock2.h>
#include <mmsystem.h>
#include <mmreg.h>

#pragma warning( disable : 4100 )			// unreferenced formal parameter
#pragma warning( disable : 4127 )			// conditional expression is constant
#pragma warning( disable : 4244 )			// conversion to smaller type, possible loss of data
#pragma warning( disable : 4714 )			// function marked as __forceinline not inlined
#pragma warning( disable : 4996 )			// unsafe string operations
#pragma warning( disable : 4533 )
#include <malloc.h>							// no malloc.h on mac or unix
#include <windows.h>						// for qgl.h
#else
#if !defined( _DEBUG ) && !defined( NDEBUG )
#define NDEBUG
#endif // _DEBUG
#undef							min
#undef							abs
#undef							max
#define UINT_PTR				unsigned long
#define __cdecl
#define ASSERT					assert
#endif