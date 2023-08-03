#ifndef __LIB_H__
#define __LIB_H__
#include "..//renderer/GLIncludes/gl.h"
#include "..//renderer/GLIncludes/glext.h"
#include "..//renderer/GLIncludes/glcore.h"
#include "..//renderer/GLIncludes/glcorearb.h"

//#include "..//renderer/GLIncludes/qgl.h"
#include "..//renderer/GLIncludes/glext.h"
#include "..//renderer/GLIncludes/glew.h"
#include "..//renderer/GLIncludes/glut.h"
#include "..//renderer/GLIncludes/glu.h"
#include "..//renderer/GLIncludes/glx.h"
#include "..//renderer/GLIncludes/glxext.h"
#include "..//renderer/GLIncludes/glxint.h"
#include "..//renderer/GLIncludes/glxtokens.h"
#include "..//renderer/GLIncludes/glxmd.h"
#include "..//renderer/GLIncludes/glxproto.h"
#include "..//renderer/GLIncludes/qgl_linked.h"
#include "..//renderer/GLIncludes/glimp_glenum.h"
#include "..//renderer/GLIncludes/wglext.h"

#include <cstring>
#include <cassert>
#include <cstddef>		// for offsetof
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
#include <cmath>
#include <basetsd.h>				// for UINT_PTR
#include <intrin.h>
#include <wchar.h>

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
#define ARC_FORCE_INLINE				forceinline
#define ARC_AL_DYNAMIC
#define assertmem( x, y )

#endif

template< typename T > ARC_INLINE void Swap( T& l, T& r ) {
	T temp = l;
	l = r;
	r = temp;
}

#ifdef __cplusplus

//-----------------------------------------------------

#define ARC_TIME_T time_t

#ifdef _WIN32
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// prevent auto literal to string conversion
#ifndef ENGINE_DLL

#define WINVER				0x501

#if 0
// Dedicated server hits unresolved when trying to link this way now. Likely because of the 2010/Win7 transition? - TTimo

#ifdef	ARC_DEDICATED
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
#endif /* !ENGINE_DLL */

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

//-----------------------------------------------------

// non-portable system services
//#include "../sys/sys_public.h"

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

//#ifdef ENGINE_DLL
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
#include "containers/HashIndexImpl.h"

#endif /* !ENGINE_DLL */

//-----------------------------------------------------

#if 0
	#ifdef _DEBUG
	#define DEBUG_NEW new(__FILE__, __LINE__)

	#ifdef _WIN32
	#include <crtdbg.h>
	#endif // _WIN32

	inline void* __cdecl operator new(size_t nSize, const char* lpszFileName, int nLine) {
		return ::operator new(nSize, _NORMAL_BLOCK, lpszFileName, nLine);
	}

	inline void __cdecl operator delete(void* pData, const char* /* lpszFileName */, int /* nLine */) {
		::operator delete(pData);
	}

	inline void* __cdecl operator new[](size_t nSize, const char* lpszFileName, int nLine) {
		return ::operator new[](nSize, _NORMAL_BLOCK, lpszFileName, nLine);
	}

	inline void __cdecl operator delete[](void* pData, const char* /* lpszFileName */, int /* nLine */) {
		::operator delete(pData);
	}
	#endif
#else
	#ifdef _DEBUG
	#undef DEBUG_NEW
	#define DEBUG_NEW new
	#endif
#endif

#include "../common/common.h"
#include <string.h>
#include <limits.h>

/*
===============================================================================

	idLib contains stateless support classes and concrete types. Some classes
	do have static variables, but such variables are initialized once and
	read-only after initialization (they do not maintain a modifiable state).

	The interface pointers anSys, anCommon, anCVarSystem and anFileSystem
	should be set before using idLib. The pointers stored here should not
	be used by any part of the engine except for idLib.

	The frameNumber should be continuously set to the number of the current
	frame if frame base memory logging is required.

===============================================================================
*/

class anLib {
public:
	static class anSys *		sys;
	static class anCommon *		common;
	static class anCVarSystem *	cvarSystem;
	static class anFileSystem *	fileSystem;
	static int					frameNumber;

	static void					Init( void );
	static void					ShutDown( void );

	// wrapper to anCommon functions
	static void		   			Printf( const char *fmt, ... );
	static void					Error( const char *fmt, ... );
	static void					Warning( const char *fmt, ... );
};

/*
===============================================================================

	Asserts and Exceptions

===============================================================================
*/

/*
The verify(x) macro just returns true or false in release mode, but breaks
in debug mode.  That way the code can take a non-fatal path in release mode
if something that's not supposed to happen happens.

if ( !verify(game) ) {
	// This should never happen!
	return;
}
*/

template <bool CompileTimeCheckValue> struct sdCompileTimeAssert {};
template<> struct sdCompileTimeAssert<true> { static void assertX() {}; };
#define CompileTimeAssert(__a) {const bool __b = (__a) ? true : false; sdCompileTimeAssert<__b>::assertX();}

#ifdef _DEBUG
void AssertFailed( const char *file, int line, const char *expression );
#undef assert

	#ifdef ID_CONDITIONAL_ASSERT
		// lets you disable an assertion at runtime when needed
		// could extend this to count and produce an assert log - useful for 'release with asserts' builds
		#define assert( x ) \
		{ \
			volatile static bool assert_enabled = true; \
			if ( assert_enabled ) { \
				if ( x ) { } else AssertFailed( __FILE__, __LINE__, #x );	\
			} \
		}
		#define verify( x ) \
		( \
			( ( x ) ? true : ( \
				( { \
					volatile static bool assert_enabled = true; \
					if ( assert_enabled ) { AssertFailed( __FILE__, __LINE__, #x ); } \
				} ) \
				, false ) ) \
		)
	#else
		#define assert( x )		if ( x ) { } else AssertFailed( __FILE__, __LINE__, #x )
		#define verify( x )		( ( x ) ? true : ( AssertFailed( __FILE__, __LINE__, #x ), false ) )
	#endif

#else

#define verify( x )		( ( x ) ? true : false )
#undef assert
#define assert( x )

#endif

#define assert_8_byte_aligned( pointer )		assert( ( ((UINT_PTR)(pointer)) &  7 ) == 0 );
#define assert_16_byte_aligned( pointer )		assert( ( ((UINT_PTR)(pointer)) & 15 ) == 0 );
#define assert_32_byte_aligned( pointer )		assert( ( ((UINT_PTR)(pointer)) & 31 ) == 0 );
#define assert_64_byte_aligned( pointer )		assert( ( ((UINT_PTR)(pointer)) & 63 ) == 0 );

#ifndef __TYPE_INFO_GEN__
#define compile_time_assert( x )				{ typedef int compile_time_assert_failed[(x) ? 1 : -1]; }
#define file_scoped_compile_time_assert( x )	extern int compile_time_assert_failed[(x) ? 1 : -1]
#define assert_sizeof( type, size )				file_scoped_compile_time_assert( sizeof( type ) == size )
#define assert_offsetof( type, field, offset )	file_scoped_compile_time_assert( offsetof( type, field ) == offset )
#define assert_offsetof_16_byte_multiple( type, field )	file_scoped_compile_time_assert( ( offsetof( type, field ) & 15 ) == 0 )
#define assert_offsetof_8_byte_multiple( type, field )	file_scoped_compile_time_assert( ( offsetof( type, field ) & 7 ) == 0 )
#define assert_sizeof_8_byte_multiple( type )	file_scoped_compile_time_assert( ( sizeof( type ) &  7 ) == 0 )
#define assert_sizeof_16_byte_multiple( type )	file_scoped_compile_time_assert( ( sizeof( type ) & 15 ) == 0 )
#define assert_sizeof_32_byte_multiple( type )	file_scoped_compile_time_assert( ( sizeof( type ) & 31 ) == 0 )
#else
#define compile_time_assert( x )
#define file_scoped_compile_time_assert( x )
#define assert_sizeof( type, size )
#define assert_offsetof( type, field, offset )
#define assert_offsetof_16_byte_multiple( type, field )
#define assert_sizeof_8_byte_multiple( type )
#define assert_sizeof_16_byte_multiple( type )
#define assert_sizeof_32_byte_multiple( type )
#endif

class anException {
public:
	char error[1024];

	anException( const char *text = "" ) { strcpy( error, text ); }
};

#if defined( __GNUC__ )
	#define an_attribute(x) __attribute__(x)
#else
	#define an_attribute(x)
#endif /* __GNUC__ */

/*
===============================================================================

	Types and defines used throughout the engine.

===============================================================================
*/

// using shorts for triangle indexes can save a significant amount of traffic, but
// to support the large models that renderBump loads, they need to be 32 bits
#if defined( USE_INDEX_SIZE_16 )
	#define GL_INDEX_TYPE		GL_UNSIGNED_SHORT
	#define GL_INDEX_SHORT
	typedef unsigned short		glIndex_t;
	typedef glIndex_t			vertIndex_t;
#else
	#define GL_INDEX_TYPE		GL_UNSIGNED_INT
	#define GL_INDEX_INT
	typedef int					glIndex_t;
	typedef glIndex_t			vertIndex_t;
#endif

typedef int					arcNetHandle_t;
typedef int					qhandle_t;
#define qhandle_t 			arcNetHandle_t

typedef unsigned char		byte;		// 8 bits
typedef unsigned short		word;		// 16 bits
typedef unsigned int		dword;		// 32 bits
typedef unsigned int		uint;
typedef unsigned long		ulong;

typedef byte				GLubyte;
typedef float				GLfloat;
typedef unsigned char		GLubyte8;		// 8-bit unsigned integer
typedef unsigned short		GLuint16;		// 16-bit unsigned integer
typedef unsigned int		GLuint32;		// 32-bit unsigned integer
typedef unsigned long long	GLuint64;		// 64-bit unsigned integer

typedef signed char			GLbyte8;		// 8-bit signed integer
typedef signed short		GLshort16;		// 16-bit signed integer
typedef signed int			GLint32;		// 32-bit signed integer
typedef signed long long	GLint64;		// 64-bit signed integer

typedef float				GLfloat32;		// 32-bit floating-point number
typedef double				GLfloat64;		// 64-bit floating-point number
typedef unsigned int		GLenum;
typedef unsigned char		GLboolean, GLbool;
typedef bool				GLboolean, GLbool;
typedef unsigned int		GLbitfield;
typedef void				GLvoid;
typedef signed char			GLbyte;		// 1-byte signed
typedef short				GLshort;	// 2-byte signed
typedef int					GLint;		// 4-byte signed
typedef unsigned char		GLubyte;	// 1-byte unsigne
typedef unsigned short		GLushort;	// 2-byte unsigned
typedef unsigned int		GLuint;		// 4-byte unsigned
typedef int					GLsizei;	// 4-byte signed
typedef float				GLfloat;	// single precision float
typedef float				GLclampf;	// single precision float in [0,1]
typedef double				GLdouble;	// double precision float
typedef double				GLclampd;	// double precision float in [0,1]

/*
64 bit notes:
Microsoft compiler is LLP64: int and long are 32 bits on all platforms, long long is 64 bits on all platforms
gcc is LP64: int is 32 bits, long is 64 bits on 64 bit environment

when you explicitely need a 64 bit integer, do not use __int64 which is Microsoft compiler specific
instead, use int64_t which is defined in the C99 standard. sadly, M$ doesn't ship <stdint.h> with MSVC
*/

#ifdef _MSC_VER
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

#define INT64_MIN _I64_MIN
#define INT64_MAX _I64_MAX

#define UINT64_MIN 0
#define UINT64_MAX _UI64_MAX

#else
#include <stdint.h>
#endif

// The C/C++ standard guarantees the size of an unsigned type is the same as the signed type.
// The exact size in bytes of several types is guaranteed here.
assert_sizeof( bool,	1 );
assert_sizeof( char,	1 );
assert_sizeof( short,	2 );
assert_sizeof( int,		4 );
//assert_sizeof( long,	8 );
assert_sizeof( float,	4 );
assert_sizeof( byte,	1 );
assert_sizeof( word,	2 );
assert_sizeof( dword,	4 );
#ifdef __STDC_ISO_10646__
// starting with glibc 2.2, Linux uses a 32bit wchar_t conformant to ISO 10646
// let's assume other platforms do as well
assert_sizeof( wchar_t, 4 );
#else
// _WIN32
assert_sizeof( wchar_t, 2 );
#endif

typedef int						qhandle_t;

class idFile;
class anVec4;

struct idNullPtr {
	// one pointer member initialized to zero so you can pass NULL as a vararg
	void *value; idNullPtr() : value( 0 ) { }
	// implicit conversion to all pointer types
	template<typename T1> operator T1*() const { return 0; }
	// implicit conversion to all pointer to member types
	template<typename T1, typename T2> operator T1 T2::*() const { return 0; }
};

#undef NULL
#if defined( _WIN32 ) && !( defined( _AFXEXT ) || defined( _USRDLL ) )
#define NULL					idNullPtr()
#else
#define NULL					0
#endif
#define nullPtr 				nullptr

#define	MAX_STRING_CHARS		1024		// max length of a static string

// maximum world size
#define MAX_WORLD_COORD			( 128 * 1024 )
#define MIN_WORLD_COORD			( -128 * 1024 )
#define MAX_WORLD_SIZE			( MAX_WORLD_COORD - MIN_WORLD_COORD )

// basic colors
extern	const anVec4 colorBlack;
extern	const anVec4 colorWhite;
extern	const anVec4 colorRed;
extern	const anVec4 colorGreen;
extern	const anVec4 colorBlue;
extern	const anVec4 colorYellow;
extern	const anVec4 colorMagenta;
extern	const anVec4 colorCyan;
extern	const anVec4 colorOrange;
extern	const anVec4 colorPurple;
extern	const anVec4 colorPink;
extern	const anVec4 colorBrown;
extern	const anVec4 colorLtGrey;
extern	const anVec4 colorMdGrey;
extern	const anVec4 colorDkGrey;

extern	const anVec4 colorLtBlue;
extern	const anVec4 colorDkRed;

// little/big endian conversion
ARC_INLINE bool		Swap_IsBigEndian( void ) { short s = 256; return ( *((byte *)&s) != 0 ); }
void				Swap_Init( void );

short				BigShort( short l );
short				LittleShort( short l );
int					BigLong( int l );
int					LittleLong( int l );
float				BigFloat( float l );
float				LittleFloat( float l );
double				LittleDouble( double l );
void				BigRevBytes( void *bp, int elsize, int elcount );
void				LittleRevBytes( void *bp, int elsize, int elcount );
void				LittleBitField( void *bp, int elsize );

ARC_INLINE void		SwapLittleShort( short &c ) { c = LittleShort( c ); }
ARC_INLINE void		SwapLittleUnsignedShort( unsigned short &c ) { c = LittleShort( c ); }
ARC_INLINE void		SwapLittleInt( int &c ) { c = LittleLong( c ); }
ARC_INLINE void		SwapLittleUnsignedInt( unsigned int &c ) { c = LittleLong( c ); }
ARC_INLINE void		SwapLittleFloat( float &c ) { c = LittleFloat( c ); }
template<class type>
ARC_INLINE void		SwapLittleFloatClass( type &c ) { for ( int i = 0; i < c.GetDimension(); i++ ) { c.ToFloatPtr()[i] = LittleFloat( c.ToFloatPtr()[i] ); } }

// for base64
void				SixtetsForInt( byte *out, int src);
int					IntForSixtets( byte *in );

// using shorts for triangle indexes can save a significant amount of traffic, but
// to support the large models that renderBump loads, they need to be 32 bits

#if defined( SD_USE_INDEX_SIZE_16 )
	#define GL_INDEX_TYPE		GL_UNSIGNED_SHORT
	#define GL_INDEX_SHORT
	typedef unsigned short		glIndex_t;
	typedef glIndex_t			vertIndex_t;
#else
	#define GL_INDEX_TYPE		GL_UNSIGNED_INT
	#define GL_INDEX_INT
	typedef int					glIndex_t;
	typedef glIndex_t			vertIndex_t;
#endif

/*
===============================================================================

	idLib headers.

===============================================================================
*/
#define ARC_INLINE inline __attribute__((always_inline))
#define ARC_INLINE __inline__

// turn float to int conversions into compile errors
#include "math/FloatErrors.h"

//#include "threading/ThreadingDefs.h"
//#include "threading/Lock.h"

// memory management and arrays
#include "Heap.h"
#endif

#include "containers/Array.h"
#include "containers/Sort.h"
#include "containers/List.h"
#include "containers/ObjArray.h"
#include "containers/PtrArray.h"
// more complex memory allocators
#include "containers/PoolAllocator.h"
#include "containers/Swap.h"

// text manipulation
#include "Str.h"
//#include "StrSimple.h"
//#include "text/StrBuilder.h"

/*
// threading
#include "threading/Signal.h"
#include "threading/Thread.h"
#include "threading/ThreadProcess.h"*/


// text manipulation
#include "Base64.h"
#include "CmdArgs.h"
#include "Token.h"
#include "UTF8.h"

// math
#include "math/Simd.h"
#include "math/BasicTypes.h"
#include "math/Math.h"
#include "math/Radians.h"
#include "math/Random.h"
#include "math/Complex.h"
#include "math/Vector.h"
#include "math/Matrix.h"
#include "math/Angles.h"
#include "math/QuadNode.h"
#include "math/Quat.h"
#include "math/Rotation.h"
#include "math/Plane.h"
#include "math/Pluecker.h"
#include "math/Polynomial.h"
#include "math/Extrapolate.h"
#include "math/Interpolate.h"
#include "math/Curve.h"
#include "math/Ode.h"
#include "math/Lcp.h"
#include "math/Perlin.h"
#include "math/Radians.h"

// bounding volumes
#include "bv/Sphere.h"
#include "bv/Bounds.h"
#include "bv/Bounds2D.h"
#include "bv/BoundsShort.h"
#include "bv/Box.h"
#include "bv/Frustum.h"

// geometry
#include "geometry/DrawVert.h"
#include "geometry/JointTransform.h"
#include "geometry/Winding.h"
#include "geometry/Winding2D.h"
#include "geometry/Surface.h"
#include "geometry/Surface_Patch.h"
#include "geometry/Surface_Polytope.h"
#include "geometry/Surface_SweptSpline.h"
#include "geometry/Surface_Traceable.h"
#include "geometry/TraceModel.h"
#include "geometry/TraceSurface.h"

// containers
#include "containers/Pair.h"
#include "containers/BinSearch.h"
#include "containers/BlockPool.h"
#include "containers/BTree.h"
#include "containers/HashIndex.h"
#include "containers/HashMap.h"
#include "containers/HashMapGeneric.h"
#include "containers/StaticList.h"
#include "containers/LinkList.h"
#include "containers/Hierarchy.h"
#include "containers/LinkedList.h"
#include "containers/Queue.h"
#include "containers/Stack.h"
#include "containers/StrList.h"
#include "containers/StrPool.h"
#include "containers/VectorSet.h"
#include "containers/PlaneSet.h"
//#include "containers/VectorWeld.h"
#include "containers/Grid.h"
#include "containers/QuadTree.h"
#include "containers/BitField.h"
#include "containers/Handles.h"
#include "containers/Deque.h"

// text manipulation
#include "LexerBinary.h"
#include "Lexer.h"
#include "Parser.h"

// hashing
#include "hashing/CRC8.h"
#include "hashing/CRC16.h"
#include "hashing/CRC32.h"
#include "hashing/Honeyman.h"
#include "hashing/MD4.h"
#include "hashing/MD5.h"

// misc
#include "Dict.h"
#include "LangDict.h"
#include "BitMsg.h"
#include "MapFile.h"
#include "Timer.h"
#include "FFT.h"
#include "Singleton.h"
#include "PtrPolicies.h"
#include "containers/AutoPtr.h"
#include "Factory.h"
#include "Callable.h"
//#include "Color.h"
//#include "SystemMemory.h"

// type info
/*
#include "typeinfo/TypeInfoFile.h"
#include "typeinfo/TypeInfoTools.h"
#include "typeinfo/TypeInfoObject.h"
#include "typeinfo/TypeInfoTree.h"
*/
#endif	// !__LIB_H__