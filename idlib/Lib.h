#ifndef __LIB_H__
#define __LIB_H__

/*
===============================================================================

	anLibrary contains stateless support classes and concrete types. Some classes
	do have static variables, but such variables are initialized once and
	read-only after initialization (they do not maintain a modifiable state).

	The interface pointers arcSystem, arcCommon, anCVarSystem and anFile
	should be set before using anLibrary. The pointers stored here should not
	be used by any part of the engine except for anLibrary.

	The frameNumber should be continuously set to the number of the current
	frame if frame base memory logging is required.

===============================================================================
*/

#include <cstring>
class anLibrary {
public:
	static class arcSystem		*sys;
	static class arcCommon		*common;
	static class anCVarSystem	*cvarSystem;
	static class anFile		*fileSystem;
	static int					frameNumber;

	static void					Init( void );
	static void					ShutDown( void );

	// wrapper to arcCommon functions
	static void       			Printf( VERIFY_FORMAT_STRING const char* fmt, ... ) ARC_STATIC_ATTRIBUTE_PRINTF( 1, 2 );
	static void       			PrintfIf( const bool test, VERIFY_FORMAT_STRING const char* fmt, ... ) ARC_STATIC_ATTRIBUTE_PRINTF( 2, 3 );
	static void					Error( const char *fmt, ... );
	static void   			    FatalError( VERIFY_FORMAT_STRING const char* fmt, ... ) ARC_STATIC_ATTRIBUTE_PRINTF( 1, 2 );
	static void					Warning( const char *fmt, ... );
	static void       			Warning( VERIFY_FORMAT_STRING const char* fmt, ... ) ARC_STATIC_ATTRIBUTE_PRINTF( 1, 2 );
	static void       			WarningIf( const bool test, VERIFY_FORMAT_STRING const char* fmt, ... ) ARC_STATIC_ATTRIBUTE_PRINTF( 2, 3 );

	static bool					IsMainThread() {
		return ( 0 == mainThreadInitialized ) || ( 1 == isMainThread );
	}
	static bool					IsFileInCache( int number ) {
		return fileSystem->IsFileInCache( number );
	}
	static void					SetMainThread( bool mainThread ) {
		isMainThread = mainThread;
	}
	static void					SetFrameNumber( int number ) {
		frameNumber = number;
	}
	static int					GetFrameNumber() {
		return frameNumber;
	}
	static void					SetCPUThread( class arcCommon *common ) {
		cvarSystem->SetCPUThread( common );

	}
	static class arcSystem *	GetSystem() {
		return sys;
	}
private:
	static bool					mainThreadInitialized;
	static ID_TLS				isMainThread;
};

/*
===============================================================================

	Types and defines used throughout the engine.

===============================================================================
*/

typedef unsigned char			byte;		// 8 bits
typedef unsigned short			word;		// 16 bits
typedef unsigned int			dword;		// 32 bits
typedef unsigned int			uint;
typedef unsigned long			ulong;

typedef int						arcNetHandle_t;
typedef int						qhandle_t;
#define qhandle_t 				arcNetHandle_t
class anFile;
class anVec3;
class anVec4;

#ifndef NULL
#define NULL					( (void *)0 )
#endif

#ifndef BIT
#define BIT( num )				( 1 << ( num ) )
#endif

#define	MAX_STRING_CHARS		1024		// max length of a string

// maximum world size
#define MAX_WORLD_COORD			( 128 * 1024 )
#define MIN_WORLD_COORD			( -128 * 1024 )
#define MAX_WORLD_SIZE			( MAX_WORLD_COORD - MIN_WORLD_COORD )

// basic colors
extern	anVec4 colorBlack;
extern	anVec4 colorWhite;
extern	anVec4 colorRed;
extern	anVec4 colorGreen;
extern	anVec4 colorBlue;
extern	anVec4 colorYellow;
extern	anVec4 colorMagenta;
extern	anVec4 colorCyan;
extern	anVec4 colorOrange;
extern	anVec4 colorPurple;
extern	anVec4 colorPink;
extern	anVec4 colorBrown;
extern	anVec4 colorLtGrey;
extern	anVec4 colorMdGrey;
extern	anVec4 colorDkGrey;
extern	anVec4 colorGold;
extern	anVec4 colorBrown2;
extern	anVec4 colorTrans;

// packs color floats in the range [0,1] into an integer
dword	PackColor( const anVec3 &color );
void	UnpackColor( const dword color, anVec3 &unpackedColor );
dword	PackColor( const anVec4 &color );
void	UnpackColor( const dword color, anVec4 &unpackedColor );

// little/big endian conversion
short	BigShort( short l );
short	LittleShort( short l );
int		BigLong( int l );
int		LittleLong( int l );
float	BigFloat( float l );
float	LittleFloat( float l );
void	BigRevBytes( void *bp, int elsize, int elcount );
void	LittleRevBytes( void *bp, int elsize, int elcount );
void	LittleBitField( void *bp, int elsize );
void	Swap_Init( void );

bool	Swap_IsBigEndian( void );

// for base64
void	SixtetsForInt( byte *out, int src);
int		IntForSixtets( byte *in );


#ifdef _DEBUG
void AssertFailed( const char *file, int line, const char *expression );
#undef assert
#define assert( X ) if ( X ) { } else AssertFailed( __FILE__, __LINE__, #X )
#endif

					/*==================================================**
					//					[arcExceptions]					//
					**==================================================*/

class arcExceptions {
public:
	static const int MAX_ERROR_LEN = 2048;
	//arcExceptions( const char* text = "" ) : arcExceptions( text ) { }
	//arcExceptions( const char *text = "" ) { strcpy( error, text ); }

	arcExceptions( const char *text = "" ) { strncpy( error, text, MAX_ERROR_LEN ); }

	// if GetError() were correctly const this would be named GetError(), too
	char *FatalErrorBuffer() {return arcExceptions::error;}

	int FatalErrorBufferSize() { return MAX_ERROR_LEN;}

	// this really, really should be a const function, but it's referenced too many places to change right now
	const char *GetError() { return error; }

protected:
	// if GetError() were correctly const this would be named GetError(), too
	char *GetErrorBuffer() { return error; }

	int GetErrorBufferSize() { return MAX_ERROR_LEN; }

private:
	//friend class ARCFatalExceptions;
	static char error[MAX_ERROR_LEN];
};

/*
===============================================================================

	Library headers for all ARC classes and types. YaYYY

===============================================================================
*/

// memory management and arrays
#include "Heap.h"
#include "containers/Sort.h"
#include "containers/List.h"

// math
#include "math/Simd.h"
#include "math/Simd_AVX.h"
#include "math/Simd_Macros.h"
#include "math/Math.h"
#include "math/Radians.h"
#include "math/Random.h"
#include "math/Complex.h"
#include "math/Vector.h"
#include "math/Matrix.h"
#include "math/Angles.h"
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

// bounding volumes
#include "bv/Sphere.h"
#include "bv/Bounds.h"
#include "idlib"
#include "bv/Box.h"
#include "bv/Frustum.h"

// geometry
//#include "geometry/RenderMatrix.h"
#include "geometry/DrawVert.h"
#include "geometry/JointTransform.h"
#include "geometry/Winding.h"
#include "geometry/Winding2D.h"
#include "geometry/Surface.h"
#include "geometry/Surface_Patch.h"
#include "geometry/Surface_Polytope.h"
#include "geometry/Surface_SweptSpline.h"
#include "geometry/TraceModel.h"

// text manipulation
#include "Str.h"
#include "Token.h"
#include "Lexer.h"
#include "LexerBinary.h"
#include "Parser.h"
#include "Base64.h"
#include "CmdArgs.h"

// containers
#include "containers/Array.h"
#include "containers/BTree.h"
#include "containers/BinSearch.h"
#include "containers/HashIndex.h"
#include "containers/HashTable.h"
#include "containers/StaticList.h"
#include "containers/LinkList.h"
#include "containers/Hierarchy.h"
#include "containers/Queue.h"
#include "containers/Stack.h"
#include "containers/StrList.h"
#include "containers/StrPool.h"
#include "containers/VectorSet.h"
#include "containers/PlaneSet.h"

// hashing
#include "hashing/CRC32.h"
#include "hashing/MD4.h"
#include "hashing/MD5.h"

// misc
#include "Dict.h"
#include "LangDict.h"
//#include "DataQueue.h"
#include "BitMsg.h"
#include "MapFile.h"
#include "Timer.h"
//#include "Thread.h"
#include "Lib.h"
#include "Swap.h"
//#include "Callback.h"
//#include "ParallelJobList.h"
//#include "SoftwareCache.h"
#include "FloatErrors.h"

#endif

#ifdef __cplusplus
#define ARC_TIME_T time_t
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#undef NULL
#define NULL 0
#endif

#if !defined( WIN32) || (WIN64 )
//#include "../tools/comafx/StdAfx.h"
//#include <winsock2.h>
//#include <mmsystem.h>
//#include <mmreg.h>

#pragma warning( disable : 4100 )			// unreferenced formal parameter
#pragma warning( disable : 4127 )			// conditional expression is constant
#pragma warning( disable : 4244 )			// conversion to smaller type, possible loss of data
#pragma warning( disable : 4714 )			// function marked as __forceinline not inlined
#pragma warning( disable : 4996 )			// unsafe string operations
#pragma warning( disable : 4533 )
#include <malloc.h>							// no malloc.h on mac or unix
//#include <windows.h>						// for qgl.h
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
#endif // WIN32 || WIN64
//#define STRTABLE_ID				"#str_"
//#define LSTRTABLE_ID			L"#str_"
//#define STRTABLE_ID_LENGTH		5

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
//#include <basetsd.h>				// for UINT_PTR
//#include <intrin.h>
#include <wchar.h>
//#include "/math/Math.h"

// non-portable system services
#include "/home/subzeroceo/ArC-NetSoftware-Projects/brokenarrow/sys/sys_local.h"
#include "/home/subzeroceo/ArC-NetSoftware-Projects/brokenarrow/sys/sys_public.h"

// id lib
#include "Lib.h"
#include "idlib/math/Simd_Generic.h"
#include "idlib/math/Simd_MMX.h"
#include "idlib/math/Simd_3DNow.h"
#include "idlib/math/Simd_SSE.h"
#include "idlib/math/Simd_SSE2.h"
#include "idlib/math/Simd_SSE3.h"
#include "idlib/math/Simd_AltiVec.h"

// framework
#include "/framework/BuildVersion.h"
#include "/framework/BuildDefines.h"
#include "/framework/Licensee.h"
#include "/framework/CmdSystem.h"
#include "/framework/CVarSystem.h"
#include "/framework/Common.h"
#include "/framework/File.h"
#include "/framework/FileSystem.h"
#include "/framework/UsercmdGen.h"

// We have expression parsing and evaluation code in multiple places:
// materials, sound shaders, and guis. We should unify them.
const int MAX_EXP_OPS = 4096;
const int MAX_EXP_REGS = 4096;

// renderer
#include "../renderer/Material.h"
#include "../renderer/Model.h"
#include "../renderer/ModelManager.h"
#include "../renderer/RenderSystem.h"
#include "../renderer/RenderWorld.h"
#include "../renderer/RenderWorld_local.h"
#include "../renderer/tr_local.h"
#include "../renderer/VertexCache.h"
#include "../renderer/simplex.h"
#include "../renderer/tr_local.h"
#include "../renderer/cg_explicit.h"
#include "../renderer/Image.h"
#include "../renderer/Interaction.h"
#include "../renderer/Model_ase.h"
#include "../renderer/Model_lwo.h"
#include "../renderer/Model_ma.h"
#include "../renderer/Model_beam.h"
#include "../renderer/Model_liquid.h"
#include "../renderer/Model_md5.h"
#include "../renderer/Model_md3.h"
#include "../renderer/Model_local.h"
#include "../renderer/Model.h"
#include "../renderer/Model_Decal.h"
#include "../renderer/GuiModel.h"
#include "../renderer/ModelManager.h"
// renderer Open GL Specfic main so that dececlaration can be available much
// more easily in the renderer with all include in the master header. Readily available
// for implements and developers only remove when complete. Copy that?
#include <../renderer/GLIncludes/gl.h>
#include <../renderer/GLIncludes/glext.h>
#include <../renderer/GLIncludes/glcore.h>
#include <../renderer/GLIncludes/glcorearb.h>

#include <../renderer/GLIncludes/qgl.h>
#include <../renderer/GLIncludes/qglext.h>
#include <../renderer/GLIncludes/glew.h>
#include <../renderer/GLIncludes/glut.h>
#include <../renderer/GLIncludes/glu.h>
#include <../renderer/GLIncludes/glx.h>
#include <../renderer/GLIncludes/glxext.h>
#include <../renderer/GLIncludes/glxint.h>
#include <../renderer/GLIncludes/glxtokens.h>
#include <../renderer/GLIncludes/glxmd.h>
#include <../renderer/GLIncludes/glxproto.h>
#include <../renderer/GLIncludes/qgl_linked.h>
#include <../renderer/GLIncludes/glimp_glenum.h>
#include <../renderer/GLIncludes/wglext.h>

// user interfaces
#include "../ui/ListGUI.h"
#include "../ui/UserInterface.h"

// framework
#include "../framework/Compressor.h"
#include "../framework/EventLoop.h"
#include "../framework/KeyInput.h"
#include "../framework/EditField.h"
#include "../framework/Console.h"

// The editor entry points are always declared, but may just be
// stubbed out on non-windows platforms.
#include "../tools/edit_public.h"

// Compilers for map, model, video etc. processing.
#include "../tools/compilers/compiler_public.h"