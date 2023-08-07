#ifndef __PRECOMPILED_H__
#define __PRECOMPILED_H__
// Copyright (C) 2007 Id Software, Inc.
//

#ifndef _COMMON_COMMON_H_
#define _COMMON_COMMON_H_

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
#ifdef MACOS_X
	CPUID_PPC							= 0x40000	// PowerPC G4/G5
#endif
} cpuid_t;

#define STRTABLE_ID				"#str_"
#define LSTRTABLE_ID			L"#str_"
#define STRTABLE_ID_LENGTH		5

#if defined(_WIN32)

#ifdef __INTEL_COMPILER
	#define ARC_STATICTEMPLATE
#else
	#define ARC_STATICTEMPLATE			static
#endif

#define inline						__forceinline
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
#define BIT( num )				BITT< num >::VALUE
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

#define ARC_STATICTEMPLATE
#define inline						inline
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
#define inline						inline
#define ARC_TLS							__thread
#define ARC_DEPRECATED

#define assertmem( x, y )

#define ARC_AL_DYNAMIC

#endif

template< typename T > inline void Swap( T& l, T& r ) {
	T temp = l;
	l = r;
	r = temp;
}

#endif // _COMMON_COMMON_H_

#ifdef __cplusplus

//-----------------------------------------------------

#define ARC_TIME_T time_t

#ifdef _WIN32

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// prevent auto literal to string conversion
#ifndef GAME_DLL

#define WINVER				0x501

#if 0
// Dedicated server hits unresolved when trying to link this way now. Likely because of the 2010/Win7 transition? - TTimo

#ifdef	ID_DEDICATED
// dedicated sets windows version here
#define	_WIN32_WINNT WINVER
#define	WIN32_LEAN_AND_MEAN
#else
// non-dedicated includes MFC and sets windows version here
#include "../tools/comafx/StdAfx.h"			// this will go away when MFC goes away
#endif
#else
#include "../tools/comafx/StdAfx.h"
#endif

#include <winsock2.h>
#include <mmsystem.h>
#include <mmreg.h>

#define DIRECTINPUT_VERSION  0x0800			// was 0x0700 with the old mssdk
#define DIRECTSOUND_VERSION  0x0800

#include <dsound.h>
#include <dinput.h>
#endif /* !GAME_DLL */

#pragma warning(disable : 4100)				// unreferenced formal parameter
#pragma warning(disable : 4244)				// conversion to smaller type, possible loss of data
#pragma warning(disable : 4714)				// function marked as __forceinline not inlined
#pragma warning(disable : 4996)				// unsafe string operations

#include <malloc.h>							// no malloc.h on mac or unix
#include <windows.h>						// for qgl.h
#undef FindText								// stupid namespace poluting Microsoft monkeys

#endif /* _WIN32 */

//-----------------------------------------------------

#if !defined( _DEBUG ) && !defined( NDEBUG )
	// don't generate asserts
	#define NDEBUG
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <typeinfo>
#include <errno.h>
#include <math.h>

//-----------------------------------------------------

// non-portable system services
#include "../sys/sys_public.h"

// id lib
#include "../idlib/Lib.h"

// framework
#include "../framework/BuildVersion.h"
#include "../framework/BuildDefines.h"
#include "../framework/Licensee.h"
#include "../framework/CmdSystem.h"
#include "../framework/CVarSystem.h"
#include "../framework/Common.h"
#include "../framework/File.h"
#include "../framework/FileSystem.h"
#include "../framework/UsercmdGen.h"

// decls
#include "../framework/DeclManager.h"
#include "../framework/DeclTable.h"
#include "../framework/DeclSkin.h"
#include "../framework/DeclEntityDef.h"
#include "../framework/DeclFX.h"
#include "../framework/DeclParticle.h"
#include "../framework/DeclAF.h"
#include "../framework/DeclMaterialClass.h"

// We have expression parsing and evaluation code in multiple places:
// materials, sound shaders, and guis. We should unify them.
const int MAX_EXPRESSION_OPS = 4096;
const int MAX_EXPRESSION_REGISTERS = 4096;

// renderer
#include "../renderer/tr_local.h"
#include "../renderer/GLIncludes/qgl.h"
//#include "../renderer/Cinematic.h"
#include "../renderer/Material.h"
#include "../renderer/Model.h"
#include "../renderer/ModelManager.h"
#include "../renderer/RenderSystem.h"
#include "../renderer/RenderWorld.h"

// sound engine
#include "../sound/sound.h"

// user interfaces
#include "../ui/ListGUI.h"
#include "../ui/UserInterface.h"

// collision detection system
#include "../cm/CollisionModel.h"

// AAS files and manager
#include "../tools/compilers/aas/AASFile.h"
#include "../tools/compilers/aas/AASFileManager.h"

// game
#include "../game/Game.h"

//-----------------------------------------------------

//#ifdef GAME_DLL
#include "../game/Game_local.h"

#include "../framework/DemoChecksum.h"

// framework
#include "../framework/Compressor.h"
#include "../framework/EventLoop.h"
#include "../framework/KeyInput.h"
#include "../framework/EditField.h"
#include "../framework/Console.h"
#include "../framework/DemoFile.h"

// The editor entry points are always declared, but may just be
// stubbed out on non-windows platforms.
#include "../tools/edit_public.h"

// Compilers for map, model, video etc. processing.
#include "../tools/compilers/compiler_public.h"

#endif /* !GAME_DLL */

//-----------------------------------------------------

#endif // __cplusplus

#endif // !__PRECOMPILED_H__
