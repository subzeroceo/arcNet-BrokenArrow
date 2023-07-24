#include "../precompiled.h"
#pragma hdrstop

#include "Simd_Generic.h"
#include "Simd_MMX.h"
#include "Simd_3DNow.h"
#include "Simd_SSE.h"
#include "Simd_SSE2.h"
#include "Simd_SSE3.h"
#include "Simd_AltiVec.h"
#include "Simd_AVX.h"
#include "Simd_Macros.h"

arcSIMDProcessor	*	processor = NULL;			// pointer to SIMD processor
arcSIMDProcessor *	generic = NULL;				// pointer to generic SIMD implementation
arcSIMDProcessor *	SIMDProcessor = NULL;

arcSIMD::arcSIMD() {
	LOG( "Constructing arcSIMD object" );
}

arcSIMD::~arcSIMD() {
}

// r = s0 + s1
void arcSIMD::AddPS(float *r, float *s0, float *s1) {
    __m128 a, b, c;

    a = _mm_load_ps(s0);
    b = _mm_load_ps(s1);
    c = _mm_add_ps(a, b);
    _mm_store_ps(r, c);
}

// r = s0 + s1
void arcSIMD::AddPS(float *r, float *s0, float *s1, int n) {
    __m128 a, b, c;

    for ( int i = 0; i < n; i++ ) {
        a = _mm_load_ps(s0);
        b = _mm_load_ps(s1);
        c = _mm_add_ps(a, b);
        _mm_store_ps(r, c);
        s0 += 4;
        s1 += 4;
        r += 4;
    }
}


// r = s0 + s1
void arcSIMD::AddPS(float *r, float *s0, float *s1, int n) {
    __m128 a, b, c;

    for ( int i = 0; i < n; i++ ) {
        a = _mm_load_ps(s0);
        b = _mm_load_ps(s1);
        c = _mm_add_ps(a, b);
        _mm_store_ps(r, c);
        s0 += 4;
        s1 += 4;
        r += 4;
    }
}

// r = s0 - s1
void arcSIMD::Subtract(float *r, float *s0, float *s1, int n) {
    __m128 a, b, c;

    for ( int i = 0; i < n; i++ ) {
        a = _mm_load_ps(s0);
        b = _mm_load_ps(s1);
        c = _mm_sub_ps(a, b);
        _mm_store_ps(r, c);
        s0 += 4;
        s1 += 4;
        r += 4;
    }
}


// r = s0 * s1
void arcSIMD::Multiply(float *r, float *s0, float *s1) {
    __m128 a, b, c;

    a = _mm_load_ps(s0);
    b = _mm_load_ps(s1);
    c = _mm_mul_ps(a, b);
    _mm_store_ps(r, c);
}

// r = s0 * s1
void arcSIMD::Multiply(float *r, float *s0, float *s1, int n) {
    __m128 a, b, c;

    for ( int i = 0; i < n; i++ ) {
        a = _mm_load_ps(s0);
        b = _mm_load_ps(s1);
        c = _mm_mul_ps(a, b);
        _mm_store_ps(r, c);
        s0 += 4;
        s1 += 4;
        r += 4;
    }
}

/*
================
arcSIMD::Init
================
*/
void arcSIMD::Init( void ) {
	generic = new arcSIMD_Generic;
	generic->cpuid = CPUID_GENERIC;
	processor = NULL;
	SIMDProcessor = generic;
}

/*
============
arcSIMD::InitProcessor
============
*/
void arcSIMD::InitProcessor( const char *module, bool forceGeneric ) {
	cpuid_t cpuid;
	arcSIMDProcessor *newProcessor;

	cpuid = arcLibrary::sys->GetProcessorId();

	if ( forceGeneric ) {
		newProcessor = generic;
	} else {
		if ( !processor ) {
			if ( ( cpuid & CPUID_ALTIVEC ) ) {
				processor = new arcSIMD_AltiVec;
			} else if ( ( cpuid & CPUID_MMX ) && ( cpuid & CPUID_SSE ) && ( cpuid & CPUID_SSE2 ) && ( cpuid & CPUID_SSE3 ) ) {
				processor = new arcSIMD_SSE3;
			} else if ( ( cpuid & CPUID_MMX ) && ( cpuid & CPUID_SSE ) && ( cpuid & CPUID_SSE2 ) ) {
				processor = new idSIMD_SSE2;
			} else if ( ( cpuid & CPUID_MMX ) && ( cpuid & CPUID_SSE ) ) {
				processor = new idSIMD_SSE;
			} else if ( ( cpuid & CPUID_MMX ) && ( cpuid & CPUID_3DNOW ) ) {
				processor = new idSIMD_3DNow;
			} else if ( ( cpuid & CPUID_MMX ) ) {
				processor = new arcSIMD_MMX;
			} else {
				processor = generic;
			}
			processor->cpuid = cpuid;
		}

		newProcessor = processor;
	}

	if ( newProcessor != SIMDProcessor ) {
		SIMDProcessor = newProcessor;
		arcLibrary::common->Printf( "%s using %s for SIMD processing\n", module, SIMDProcessor->GetName() );
	}

	if ( cpuid & CPUID_FTZ ) {
		arcLibrary::sys->FPU_SetFTZ( true );
		arcLibrary::common->Printf( "enabled Flush-To-Zero mode\n" );
	}

	if ( cpuid & CPUID_DAZ ) {
		arcLibrary::sys->FPU_SetDAZ( true );
		arcLibrary::common->Printf( "enabled Denormals-Are-Zero mode\n" );
	}
}

/*
================
arcSIMD::Shutdown
================
*/
void arcSIMD::Shutdown( void ) {
	if ( processor != generic ) {
		delete processor;
	}
	delete generic;
	generic = NULL;
	processor = NULL;
	SIMDProcessor = NULL;
}

//===============================================================
//
// Test code
//
//===============================================================

#define COUNT		1024		// data count
#define BIG_COUNT	COUNT*5		// Some tests need a larger count
#define NUMTESTS	2048		// number of tests

#define RANDOM_SEED		1013904223L	//( ( int )arcLibrary::sys->GetClockTicks() )

arcSIMDProcessor *p_simd;
arcSIMDProcessor *p_generic;
long baseClocks = 0;

#define TIME_TYPE int

#pragma warning(disable : 4731)     // frame pointer register 'ebx' modified by inline assembly code

long saved_ebx = 0;

#define StartRecordTime( start )			\
	__asm mov saved_ebx, ebx				\
	__asm xor eax, eax						\
	__asm cpuid								\
	__asm rdtsc								\
	__asm mov start, eax					\
	__asm xor eax, eax						\
	__asm cpuid

#define StopRecordTime( end )				\
	__asm xor eax, eax						\
	__asm cpuid								\
	__asm rdtsc								\
	__asm mov end, eax						\
	__asm mov ebx, saved_ebx				\
	__asm xor eax, eax						\
	__asm cpuid

#elif MACOS_X

#include <stdlib.h>
#include <unistd.h>			// this is for sleep()
#include <sys/time.h>
#include <sys/resource.h>
#include <mach/mach_time.h>

double ticksPerNanosecond;

#define TIME_TYPE uint64_t

#ifdef __MWERKS__ //TimeInMilliseconds is missing
/*

    .text
	.align 2
	.globl _GetTB
_GetTB:

loop:
	        mftbu   r4	;  load from TBU
	        mftb    r5	;  load from TBL
	        mftbu   r6	;  load from TBU
	        cmpw    r6, r4	;  see if old == new
	        bne     loop	;  if not, carry occured, therefore loop

	        stw     r4, 0(r3)
	        stw     r5, 4(r3)

done:
	        blr		;  return

*/
typedef struct {
	unsigned int hi;
	unsigned int lo;
} U64;


asm void GetTB(U64 *in) {
	nofralloc			// suppress prolog
	machine 603			// allows the use of mftb & mftbu functions

loop:
	mftbu	r5			// grab the upper time base register (TBU)
	mftb	r4			// grab the lower time base register (TBL)
	mftbu	r6			// grab the upper time base register (TBU) again

	cmpw	r6,r5		// see if old TBU == new TBU
	bne-	loop		// loop if carry occurred (predict branch not taken)

	stw  	r4,4(r3)	// store TBL in the low 32 bits of the return value
	stw  	r5,0(r3)	// store TBU in the high 32 bits of the return value

	blr
}

double TBToDoubleNano( U64 startTime, U64 stopTime, double ticksPerNanosecond );

#if __MWERKS__
asm void GetTB( U64 * );
#else
void GetTB( U64 * );
#endif

double TBToDoubleNano( U64 startTime, U64 stopTime, double ticksPerNanosecond ) {
	#define K_2POWER32 4294967296.0
	#define TICKS_PER_NANOSECOND 0.025
	double nanoTime;
	U64 diffTime;

	// calc the difference in TB ticks
	diffTime.hi = stopTime.hi - startTime.hi;
	diffTime.lo = stopTime.lo - startTime.lo;

	// convert TB ticks into time
	nanoTime = ( double )(diffTime.hi)*(( double )K_2POWER32) + ( double )(diffTime.lo);
	nanoTime = nanoTime/ticksPerNanosecond;
	return (nanoTime);
}

TIME_TYPE TimeInMilliseconds( void ) {
	#define K_2POWER32 4294967296.0
	#define TICKS_PER_NANOSECOND 0.025

	U64 the_time;
	double nanoTime, milliTime;

	GetTB( &the_time );

	// convert TB ticks into time
	nanoTime = ( double )(the_time.hi)*(( double )K_2POWER32) + ( double )(the_time.lo);
	nanoTime = nanoTime/ticksPerNanosecond;

	// nanoseconds are 1 billionth of a second. I want milliseconds
	milliTime = nanoTime * 1000000.0;

	printf( "ticks per nanosec -- %lf\n", ticksPerNanosecond );
	printf( "nanoTime is %lf -- milliTime is %lf -- as int is %i\n", nanoTime, milliTime, ( int )milliTime );

	return ( int )milliTime;
}

#define StartRecordTime( start )			\
	start = TimeInMilliseconds();

#define StopRecordTime( end )				\
	end = TimeInMilliseconds();


#else
#define StartRecordTime( start ) \
	start = mach_absolute_time();

#define StopRecordTime( end ) \
	end = mach_absolute_time();
#endif
#else

#define TIME_TYPE int
#define StartRecordTime( start ) \
	start = 0;
#define StopRecordTime( end ) \
	end = 1;
#endif
#define GetBest( start, end, best ) \
	if ( !best || end - start < best ) { \
		best = end - start; \
	}

/*
============
PrintClocks
============
*/
void PrintClocks( char *string, int dataCount, int clocks, int otherClocks = 0 ) {
	arcLibrary::common->Printf( string );
	for ( int i = arcNetString::LengthWithoutColors( string ); i < 48; i++ ) {
		arcLibrary::common->Printf( " " );
	}
	clocks -= baseClocks;
	if ( otherClocks && clocks ) {
		otherClocks -= baseClocks;
		int p = ( int ) ( ( float ) ( otherClocks - clocks ) * 100.0f / ( float ) otherClocks );
		arcLibrary::common->Printf( "c = %4d, clcks = %5d, %d%%\n", dataCount, clocks, p );
	} else {
		arcLibrary::common->Printf( "c = %4d, clcks = %5d\n", dataCount, clocks );
	}
}

/*
============
GetBaseClocks
============
*/
void GetBaseClocks( void ) {
	int start, end;

	int bestClocks = 0;
	for ( int i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	baseClocks = bestClocks;
}

/*
============
TestAdd
============
*/
void TestAdd( void ) {
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT] );
	ALIGN16( float fdst1[COUNT] );
	ALIGN16( float fsrc0[COUNT] );
	ALIGN16( float fsrc1[COUNT] );
	arcRandom srnd( RANDOM_SEED );

	for ( int i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
		fsrc1[i] = srnd.CRandomFloat() * 10.0f;
	}

	arcLibrary::common->Printf( "====================================\n" );

	int bestClocksGeneric = 0;
	for ( int i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Add( fdst0, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Add( float + float[] )", COUNT, bestClocksGeneric );

	int bestClocksSIMD = 0;
	for ( int i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Add( fdst1, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( int i = 0; i < COUNT; i++ ) {
		if ( arcMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	const char *result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Add( float + float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	int bestClocksGeneric = 0;
	for ( int i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Add( fdst0, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Add( float[] + float[] )", COUNT, bestClocksGeneric );

	int bestClocksSIMD = 0;
	for ( int i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Add( fdst1, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( int i = 0; i < COUNT; i++ ) {
		if ( arcMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( int i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Add( float[] + float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestSub
============
*/
void TestSub( void ) {
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT] );
	ALIGN16( float fdst1[COUNT] );
	ALIGN16( float fsrc0[COUNT] );
	ALIGN16( float fsrc1[COUNT] );
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( int i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
		fsrc1[i] = srnd.CRandomFloat() * 10.0f;
	}

	arcLibrary::common->Printf( "====================================\n" );

	int bestClocksGeneric = 0;
	for ( int i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Sub( fdst0, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Sub( float + float[] )", COUNT, bestClocksGeneric );

	int bestClocksSIMD = 0;
	for ( int i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Sub( fdst1, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( int i = 0; i < COUNT; i++ ) {
		if ( arcMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	const char *result = ( int i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Sub( float + float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Sub( fdst0, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Sub( float[] + float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Sub( fdst1, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( arcMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Sub( float[] + float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMul
============
*/
void TestMul( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT] );
	ALIGN16( float fdst1[COUNT] );
	ALIGN16( float fsrc0[COUNT] );
	ALIGN16( float fsrc1[COUNT] );
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
		fsrc1[i] = srnd.CRandomFloat() * 10.0f;
	}

	arcLibrary::common->Printf( "====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Mul( fdst0, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Mul( float * float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Mul( fdst1, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( arcMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Mul( float * float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Mul( fdst0, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Mul( float[] * float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Mul( fdst1, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( arcMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Mul( float[] * float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestDiv
============
*/
void TestDiv( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT] );
	ALIGN16( float fdst1[COUNT] );
	ALIGN16( float fsrc0[COUNT] );
	ALIGN16( float fsrc1[COUNT] );
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
		do {
			fsrc1[i] = srnd.CRandomFloat() * 10.0f;
		} while( arcMath::Fabs( fsrc1[i] ) < 0.1f );
	}

	arcLibrary::common->Printf( "====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Div( fdst0, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Div( float * float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Div( fdst1, 4.0f, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( arcMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Div( float * float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Div( fdst0, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Div( float[] * float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Div( fdst1, fsrc0, fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( arcMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-3f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Div( float[] * float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMulAdd
============
*/
void TestMulAdd( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT] );
	ALIGN16( float fdst1[COUNT] );
	ALIGN16( float fsrc0[COUNT] );
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
	}

	arcLibrary::common->Printf( "====================================\n" );

	for ( j = 0; j < 50 && j < COUNT; j++ ) {
		bestClocksGeneric = 0;
		for ( i = 0; i < NUMTESTS; i++ ) {
			for ( int k = 0; k < COUNT; k++ ) {
				fdst0[k] = k;
			}
			StartRecordTime( start );
			p_generic->MulAdd( fdst0, 0.123f, fsrc0, j );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		PrintClocks( va( "generic->MulAdd( float * float[%2d] )", j ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( i = 0; i < NUMTESTS; i++ ) {
			for ( int k = 0; k < COUNT; k++ ) {
				fdst1[k] = k;
			}
			StartRecordTime( start );
			p_simd->MulAdd( fdst1, 0.123f, fsrc0, j );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		for ( i = 0; i < COUNT; i++ ) {
			if ( arcMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
				break;
			}
		}
		result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MulAdd( float * float[%2d] ) %s", j, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMulSub
============
*/
void TestMulSub( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT] );
	ALIGN16( float fdst1[COUNT] );
	ALIGN16( float fsrc0[COUNT] );
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
	}

	arcLibrary::common->Printf( "====================================\n" );

	for ( j = 0; j < 50 && j < COUNT; j++ ) {
		bestClocksGeneric = 0;
		for ( i = 0; i < NUMTESTS; i++ ) {
			for ( int k = 0; k < COUNT; k++ ) {
				fdst0[k] = k;
			}
			StartRecordTime( start );
			p_generic->MulSub( fdst0, 0.123f, fsrc0, j );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		PrintClocks( va( "generic->MulSub( float * float[%2d] )", j ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( i = 0; i < NUMTESTS; i++ ) {
			for ( int k = 0; k < COUNT; k++ ) {
				fdst1[k] = k;
			}
			StartRecordTime( start );
			p_simd->MulSub( fdst1, 0.123f, fsrc0, j );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		for ( i = 0; i < COUNT; i++ ) {
			if ( arcMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
				break;
			}
		}
		result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MulSub( float * float[%2d] ) %s", j, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestDot
============
*/
void TestDot( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT] );
	ALIGN16( float fdst1[COUNT] );
	ALIGN16( float fsrc0[COUNT] );
	ALIGN16( float fsrc1[COUNT] );
	ALIGN16( arcVec3 v3src0[COUNT] );
	ALIGN16( arcVec3 v3src1[COUNT] );
	ALIGN16( arcVec3 v3constant ) ( 1.0f, 2.0f, 3.0f );
	ALIGN16( arcPlane v4src0[COUNT] );
	ALIGN16( arcPlane v4constant ) (1.0f, 2.0f, 3.0f, 4.0f);
	ALIGN16( arcDrawVert drawVerts[COUNT] );
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
		fsrc1[i] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][0] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][1] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][2] = srnd.CRandomFloat() * 10.0f;
		v3src1[i][0] = srnd.CRandomFloat() * 10.0f;
		v3src1[i][1] = srnd.CRandomFloat() * 10.0f;
		v3src1[i][2] = srnd.CRandomFloat() * 10.0f;
		v4src0[i] = v3src0[i];
		v4src0[i][3] = srnd.CRandomFloat() * 10.0f;
		drawVerts[i].xyz = v3src0[i];
	}

	arcLibrary::common->Printf( "====================================\n" );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Dot( fdst0, v3constant, v3src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Dot( arcVec3 * arcVec3[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Dot( fdst1, v3constant, v3src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( arcMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Dot( arcVec3 * arcVec3[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Dot( fdst0, v3constant, v4src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Dot( arcVec3 * arcPlane[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Dot( fdst1, v3constant, v4src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( arcMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Dot( arcVec3 * arcPlane[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Dot( fdst0, v3constant, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Dot( arcVec3 * arcDrawVert[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Dot( fdst1, v3constant, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( arcMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Dot( arcVec3 * arcDrawVert[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Dot( fdst0, v4constant, v3src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Dot( arcPlane * arcVec3[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Dot( fdst1, v4constant, v3src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( arcMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Dot( arcPlane * arcVec3[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Dot( fdst0, v4constant, v4src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Dot( arcPlane * arcPlane[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Dot( fdst1, v4constant, v4src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( arcMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Dot( arcPlane * arcPlane[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Dot( fdst0, v4constant, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Dot( arcPlane * arcDrawVert[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Dot( fdst1, v4constant, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( arcMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-5f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Dot( arcPlane * arcDrawVert[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Dot( fdst0, v3src0, v3src1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Dot( arcVec3[] * arcVec3[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Dot( fdst1, v3src0, v3src1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( arcMath::Fabs( fdst0[i] - fdst1[i] ) > 1e-4f ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Dot( arcVec3[] * arcVec3[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	arcLibrary::common->Printf( "====================================\n" );

	float dot1 = 0.0f, dot2 = 0.0f;
	for ( j = 0; j < 50 && j < COUNT; j++ ) {

		bestClocksGeneric = 0;
		for ( i = 0; i < NUMTESTS; i++ ) {
			StartRecordTime( start );
			p_generic->Dot( dot1, fsrc0, fsrc1, j );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		PrintClocks( va( "generic->Dot( float[%2d] * float[%2d] )", j, j ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( i = 0; i < NUMTESTS; i++ ) {
			StartRecordTime( start );
			p_simd->Dot( dot2, fsrc0, fsrc1, j );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}
		result = arcMath::Fabs( dot1 - dot2 ) < 1e-4f ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->Dot( float[%2d] * float[%2d] ) %s", j, j, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestCompare
============
*/
void TestCompare( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fsrc0[COUNT] );
	ALIGN16( byte bytedst[COUNT] );
	ALIGN16( byte bytedst2[COUNT] );
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
	}

	arcLibrary::common->Printf( "====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->CmpGT( bytedst, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpGT( float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->CmpGT( bytedst2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CmpGT( float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst, 0, COUNT );
		StartRecordTime( start );
		p_generic->CmpGT( bytedst, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpGT( 2, float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst2, 0, COUNT );
		StartRecordTime( start );
		p_simd->CmpGT( bytedst2, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CmpGT( 2, float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	// ======================

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->CmpGE( bytedst, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpGE( float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->CmpGE( bytedst2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CmpGE( float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst, 0, COUNT );
		StartRecordTime( start );
		p_generic->CmpGE( bytedst, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpGE( 2, float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst2, 0, COUNT );
		StartRecordTime( start );
		p_simd->CmpGE( bytedst2, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CmpGE( 2, float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	// ======================

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->CmpLT( bytedst, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpLT( float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->CmpLT( bytedst2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CmpLT( float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst, 0, COUNT );
		StartRecordTime( start );
		p_generic->CmpLT( bytedst, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpLT( 2, float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst2, 0, COUNT );
		StartRecordTime( start );
		p_simd->CmpLT( bytedst2, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CmpLT( 2, float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	// ======================

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->CmpLE( bytedst, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpLE( float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->CmpLE( bytedst2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CmpLE( float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst, 0, COUNT );
		StartRecordTime( start );
		p_generic->CmpLE( bytedst, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CmpLE( 2, float[] >= float )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		memset( bytedst2, 0, COUNT );
		StartRecordTime( start );
		p_simd->CmpLE( bytedst2, 2, fsrc0, 0.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( bytedst[i] != bytedst2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CmpLE( 2, float[] >= float ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMinMax
============
*/
void TestMinMax( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fsrc0[COUNT] );
	ALIGN16( arcVec2 v2src0[COUNT] );
	ALIGN16( arcVec3 v3src0[COUNT] );
	ALIGN16( arcDrawVert drawVerts[COUNT] );
	ALIGN16( int indexes[COUNT] );
	float min = 0.0f, max = 0.0f, min2 = 0.0f, max2 = 0.0f;
	arcVec2 v2min, v2max, v2min2, v2max2;
	arcVec3 vmin, vmax, vmin2, vmax2;
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
		v2src0[i][0] = srnd.CRandomFloat() * 10.0f;
		v2src0[i][1] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][0] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][1] = srnd.CRandomFloat() * 10.0f;
		v3src0[i][2] = srnd.CRandomFloat() * 10.0f;
		drawVerts[i].xyz = v3src0[i];
		indexes[i] = i;
	}

	arcLibrary::common->Printf( "====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		min = arcMath::INFINITY;
		max = -arcMath::INFINITY;
		StartRecordTime( start );
		p_generic->MinMax( min, max, fsrc0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( min2, max2, fsrc0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( min == min2 && max == max2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->MinMax( v2min, v2max, v2src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( arcVec2[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( v2min2, v2max2, v2src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( v2min == v2min2 && v2max == v2max2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( arcVec2[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->MinMax( vmin, vmax, v3src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( arcVec3[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( vmin2, vmax2, v3src0, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( vmin == vmin2 && vmax == vmax2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( arcVec3[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->MinMax( vmin, vmax, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( arcDrawVert[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( vmin2, vmax2, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( vmin == vmin2 && vmax == vmax2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( arcDrawVert[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->MinMax( vmin, vmax, drawVerts, indexes, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MinMax( arcDrawVert[], indexes[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->MinMax( vmin2, vmax2, drawVerts, indexes, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	result = ( vmin == vmin2 && vmax == vmax2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MinMax( arcDrawVert[], indexes[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestClamp
============
*/
void TestClamp( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fdst0[COUNT] );
	ALIGN16( float fdst1[COUNT] );
	ALIGN16( float fsrc0[COUNT] );
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = srnd.CRandomFloat() * 10.0f;
	}

	arcLibrary::common->Printf( "====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->Clamp( fdst0, fsrc0, -1.0f, 1.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Clamp( float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->Clamp( fdst1, fsrc0, -1.0f, 1.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( fdst0[i] != fdst1[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Clamp( float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->ClampMin( fdst0, fsrc0, -1.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ClampMin( float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->ClampMin( fdst1, fsrc0, -1.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( fdst0[i] != fdst1[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->ClampMin( float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->ClampMax( fdst0, fsrc0, 1.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ClampMax( float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->ClampMax( fdst1, fsrc0, 1.0f, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( fdst0[i] != fdst1[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->ClampMax( float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMemcpy
============
*/
void TestMemcpy( void ) {
	byte test0[8192];
	byte test1[8192];

	arcRandom random( RANDOM_SEED );

	arcLibrary::common->Printf( "====================================\n" );

	for ( int i = 5; i < 8192; i += 31 ) {
		for ( int j = 0; j < i; j++ ) {
			test0[j] = random.RandomInt( 255 );
		}
		p_simd->Memcpy( test1, test0, 8192 );
		for ( int j = 0; j < i; j++ ) {
			if ( test1[j] != test0[j] ) {
				//#modified-fva; BEGIN
				//arcLibrary::common->Printf( "   simd->Memcpy() "S_COLOR_RED"X\n" );
				arcLibrary::common->Printf( "   simd->Memcpy() " S_COLOR_RED"X\n" );
				//#modified-fva; END
				return;
			}
		}
	}
	arcLibrary::common->Printf( "   simd->Memcpy() ok\n" );
}

/*
============
TestMemset
============
*/
void TestMemset( void ) {
	byte test[8192];

	for ( int i = 0; i < 8192; i++ ) {
		test[i] = 0;
	}

	for ( int i = 5; i < 8192; i += 31 ) {
		for ( int j = -1; j <= 1; j++ ) {
			p_simd->Memset( test, j, i );
			for ( int k = 0; k < i; k++ ) {
				if ( test[k] != ( byte )j ) {
					//#modified-fva; BEGIN
					//arcLibrary::common->Printf( "   simd->Memset() "S_COLOR_RED"X\n" );
					arcLibrary::common->Printf( "   simd->Memset() " S_COLOR_RED"X\n" );
					//#modified-fva; END
					return;
				}
			}
		}
	}
	arcLibrary::common->Printf( "   simd->Memset() ok\n" );
}

#define	MATX_SIMD_EPSILON			1e-5f

/*
============
TestMatXMultiplyVecX
============
*/
void TestMatXMultiplyVecX( void ) {
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	arcMatX mat;
	arcVecX src(6);
	arcVecX dst(6);
	arcVecX tst(6);

	src[0] = 1.0f;
	src[1] = 2.0f;
	src[2] = 3.0f;
	src[3] = 4.0f;
	src[4] = 5.0f;
	src[5] = 6.0f;

	arcLibrary::common->Printf( "================= NxN * Nx1 ===================\n" );

	for ( int i = 1; i <= 6; i++ ) {
		mat.Random( i, i, RANDOM_SEED, -10.0f, 10.0f );
		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_MultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyVecX %dx%d*%dx1", i, i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( int j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_MultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_MultiplyVecX %dx%d*%dx1 %s", i, i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	arcLibrary::common->Printf( "================= Nx6 * 6x1 ===================\n" );

	for ( int i = 1; i <= 6; i++ ) {
		mat.Random( i, 6, RANDOM_SEED, -10.0f, 10.0f );
		bestClocksGeneric = 0;
		for ( int j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_MultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyVecX %dx6*6x1", i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( int j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_MultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_MultiplyVecX %dx6*6x1 %s", i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	arcLibrary::common->Printf( "================= 6xN * Nx1 ===================\n" );

	for ( int i = 1; i <= 6; i++ ) {
		mat.Random( 6, i, RANDOM_SEED, -10.0f, 10.0f );
		bestClocksGeneric = 0;
		for ( int j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_MultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyVecX 6x%d*%dx1", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( int j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_MultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_MultiplyVecX 6x%d*%dx1 %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXMultiplyAddVecX
============
*/
void TestMatXMultiplyAddVecX( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	arcMatX mat;
	arcVecX src(6);
	arcVecX dst(6);
	arcVecX tst(6);

	src[0] = 1.0f;
	src[1] = 2.0f;
	src[2] = 3.0f;
	src[3] = 4.0f;
	src[4] = 5.0f;
	src[5] = 6.0f;

	arcLibrary::common->Printf( "================= NxN * Nx1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( i, i, RANDOM_SEED, -10.0f, 10.0f );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_MultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyAddVecX %dx%d*%dx1", i, i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_MultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_MultiplyAddVecX %dx%d*%dx1 %s", i, i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	arcLibrary::common->Printf( "================= Nx6 * 6x1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( i, 6, RANDOM_SEED, -10.0f, 10.0f );
		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_MultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyAddVecX %dx6*6x1", i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_MultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_MultiplyAddVecX %dx6*6x1 %s", i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	arcLibrary::common->Printf( "================= 6xN * Nx1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( 6, i, RANDOM_SEED, -10.0f, 10.0f );
		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_MultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyAddVecX 6x%d*%dx1", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_MultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_MultiplyAddVecX 6x%d*%dx1 %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXTransposeMultiplyVecX
============
*/
void TestMatXTransposeMultiplyVecX( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	arcMatX mat;
	arcVecX src(6);
	arcVecX dst(6);
	arcVecX tst(6);

	src[0] = 1.0f;
	src[1] = 2.0f;
	src[2] = 3.0f;
	src[3] = 4.0f;
	src[4] = 5.0f;
	src[5] = 6.0f;

	arcLibrary::common->Printf( "================= Nx6 * Nx1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( i, 6, RANDOM_SEED, -10.0f, 10.0f );
		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_TransposeMultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_TransposeMulVecX %dx6*%dx1", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_TransposeMultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_TransposeMulVecX %dx6*%dx1 %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	arcLibrary::common->Printf( "================= 6xN * 6x1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( 6, i, RANDOM_SEED, -10.0f, 10.0f );
		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_TransposeMultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_TransposeMulVecX 6x%d*6x1", i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_TransposeMultiplyVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_TransposeMulVecX 6x%d*6x1 %s", i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXTransposeMultiplyAddVecX
============
*/
void TestMatXTransposeMultiplyAddVecX( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	arcMatX mat;
	arcVecX src(6);
	arcVecX dst(6);
	arcVecX tst(6);

	src[0] = 1.0f;
	src[1] = 2.0f;
	src[2] = 3.0f;
	src[3] = 4.0f;
	src[4] = 5.0f;
	src[5] = 6.0f;

	arcLibrary::common->Printf( "================= Nx6 * Nx1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( i, 6, RANDOM_SEED, -10.0f, 10.0f );
		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_TransposeMultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_TransposeMulAddVecX %dx6*%dx1", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_TransposeMultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_TransposeMulAddVecX %dx6*%dx1 %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	arcLibrary::common->Printf( "================= 6xN * 6x1 ===================\n" );

	for ( i = 1; i <= 6; i++ ) {
		mat.Random( 6, i, RANDOM_SEED, -10.0f, 10.0f );
		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_generic->MatX_TransposeMultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_TransposeMulAddVecX 6x%d*6x1", i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			dst.Zero();
			StartRecordTime( start );
			p_simd->MatX_TransposeMultiplyAddVecX( dst, mat, src );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_TransposeMulAddVecX 6x%d*6x1 %s", i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXMultiplyMatX
============
*/
#define TEST_VALUE_RANGE			10.0f
#define	MATX_MATX_SIMD_EPSILON		1e-4f

void TestMatXMultiplyMatX( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	arcMatX m1, m2, dst, tst;

	arcLibrary::common->Printf( "================= NxN * Nx6 ===================\n" );

	// NxN * Nx6
	for ( i = 1; i <= 5; i++ ) {
		m1.Random( i, i, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		m2.Random( i, 6, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		dst.SetSize( i, 6 );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyMatX %dx%d*%dx6", i, i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_MultiplyMatX %dx%d*%dx6 %s", i, i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	arcLibrary::common->Printf( "================= 6xN * Nx6 ===================\n" );

	// 6xN * Nx6
	for ( i = 1; i <= 5; i++ ) {
		m1.Random( 6, i, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		m2.Random( i, 6, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		dst.SetSize( 6, 6 );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyMatX 6x%d*%dx6", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_MultiplyMatX 6x%d*%dx6 %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	arcLibrary::common->Printf( "================= Nx6 * 6xN ===================\n" );

	// Nx6 * 6xN
	for ( i = 1; i <= 5; i++ ) {
		m1.Random( i, 6, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		m2.Random( 6, i, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		dst.SetSize( i, i );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyMatX %dx6*6x%d", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_MultiplyMatX %dx6*6x%d %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	arcLibrary::common->Printf( "================= 6x6 * 6xN ===================\n" );

	// 6x6 * 6xN
	for ( i = 1; i <= 6; i++ ) {
		m1.Random( 6, 6, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		m2.Random( 6, i, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		dst.SetSize( 6, i );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_MultiplyMatX 6x6*6x%d", i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_MultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_MultiplyMatX 6x6*6x%d %s", i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXTransposeMultiplyMatX
============
*/
void TestMatXTransposeMultiplyMatX( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	arcMatX m1, m2, dst, tst;

	arcLibrary::common->Printf( "================= Nx6 * NxN ===================\n" );

	// Nx6 * NxN
	for ( i = 1; i <= 5; i++ ) {
		m1.Random( i, 6, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		m2.Random( i, i, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		dst.SetSize( 6, i );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_TransposeMultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_TransMultiplyMatX %dx6*%dx%d", i, i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_TransposeMultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_TransMultiplyMatX %dx6*%dx%d %s", i, i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}

	arcLibrary::common->Printf( "================= 6xN * 6x6 ===================\n" );

	// 6xN * 6x6
	for ( i = 1; i <= 6; i++ ) {
		m1.Random( 6, i, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		m2.Random( 6, 6, RANDOM_SEED, -TEST_VALUE_RANGE, TEST_VALUE_RANGE );
		dst.SetSize( i, 6 );

		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_TransposeMultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = dst;

		PrintClocks( va( "generic->MatX_TransMultiplyMatX 6x%d*6x6", i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_TransposeMultiplyMatX( dst, m1, m2 );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = dst.Compare( tst, MATX_MATX_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_TransMultiplyMatX 6x%d*6x6 %s", i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

#define MATX_LTS_SIMD_EPSILON		1.0f
#define MATX_LTS_SOLVE_SIZE			100

/*
============
TestMatXLowerTriangularSolve
============
*/
void TestMatXLowerTriangularSolve( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	arcMatX L;
	arcVecX x, b, tst;

	arcLibrary::common->Printf( "====================================\n" );

	L.Random( MATX_LTS_SOLVE_SIZE, MATX_LTS_SOLVE_SIZE, 0, -1.0f, 1.0f );
	x.SetSize( MATX_LTS_SOLVE_SIZE );
	b.Random( MATX_LTS_SOLVE_SIZE, 0, -1.0f, 1.0f );

	for ( i = 1; i < MATX_LTS_SOLVE_SIZE; i++ ) {
		x.Zero( i );
		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_LowerTriangularSolve( L, x.ToFloatPtr(), b.ToFloatPtr(), i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = x;
		x.Zero();

		PrintClocks( va( "generic->MatX_LowerTriangularSolve %dx%d", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_LowerTriangularSolve( L, x.ToFloatPtr(), b.ToFloatPtr(), i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = x.Compare( tst, MATX_LTS_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_LowerTriangularSolve %dx%d %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestMatXLowerTriangularSolveTranspose
============
*/
void TestMatXLowerTriangularSolveTranspose( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	arcMatX L;
	arcVecX x, b, tst;

	arcLibrary::common->Printf( "====================================\n" );

	L.Random( MATX_LTS_SOLVE_SIZE, MATX_LTS_SOLVE_SIZE, 0, -1.0f, 1.0f );
	x.SetSize( MATX_LTS_SOLVE_SIZE );
	b.Random( MATX_LTS_SOLVE_SIZE, 0, -1.0f, 1.0f );

	for ( i = 1; i < MATX_LTS_SOLVE_SIZE; i++ ) {
		x.Zero( i );
		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_generic->MatX_LowerTriangularSolveTranspose( L, x.ToFloatPtr(), b.ToFloatPtr(), i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}
		tst = x;
		x.Zero();

		PrintClocks( va( "generic->MatX_LowerTriangularSolveT %dx%d", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			StartRecordTime( start );
			p_simd->MatX_LowerTriangularSolveTranspose( L, x.ToFloatPtr(), b.ToFloatPtr(), i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = x.Compare( tst, MATX_LTS_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_LowerTriangularSolveT %dx%d %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

#define MATX_LDLT_SIMD_EPSILON			0.1f
#define MATX_LDLT_FACTOR_SOLVE_SIZE		64

/*
============
TestMatXLDLTFactor
============
*/
void TestMatXLDLTFactor( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	const char *result;
	arcMatX src, original, mat1, mat2;
	arcVecX invDiag1, invDiag2;

	arcLibrary::common->Printf( "====================================\n" );

	original.SetSize( MATX_LDLT_FACTOR_SOLVE_SIZE, MATX_LDLT_FACTOR_SOLVE_SIZE );
	src.Random( MATX_LDLT_FACTOR_SOLVE_SIZE, MATX_LDLT_FACTOR_SOLVE_SIZE, 0, -1.0f, 1.0f );
	src.TransposeMultiply( original, src );

	for ( i = 1; i < MATX_LDLT_FACTOR_SOLVE_SIZE; i++ ) {
		bestClocksGeneric = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			mat1 = original;
			invDiag1.Zero( MATX_LDLT_FACTOR_SOLVE_SIZE );
			StartRecordTime( start );
			p_generic->MatX_LDLTFactor( mat1, invDiag1, i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksGeneric );
		}

		PrintClocks( va( "generic->MatX_LDLTFactor %dx%d", i, i ), 1, bestClocksGeneric );

		bestClocksSIMD = 0;
		for ( j = 0; j < NUMTESTS; j++ ) {
			mat2 = original;
			invDiag2.Zero( MATX_LDLT_FACTOR_SOLVE_SIZE );
			StartRecordTime( start );
			p_simd->MatX_LDLTFactor( mat2, invDiag2, i );
			StopRecordTime( end );
			GetBest( start, end, bestClocksSIMD );
		}

		result = mat1.Compare( mat2, MATX_LDLT_SIMD_EPSILON ) && invDiag1.Compare( invDiag2, MATX_LDLT_SIMD_EPSILON ) ? "ok" : S_COLOR_RED"X";
		PrintClocks( va( "   simd->MatX_LDLTFactor %dx%d %s", i, i, result ), 1, bestClocksSIMD, bestClocksGeneric );
	}
}

/*
============
TestBlendJoints
============
*/
void TestBlendJoints( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( idJointQuat baseJoints[COUNT] );
	ALIGN16( idJointQuat joints1[COUNT] );
	ALIGN16( idJointQuat joints2[COUNT] );
	ALIGN16( idJointQuat blendJoints[COUNT] );
	ALIGN16( int index[COUNT] );
	float lerp = 0.3f;
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		arcAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		baseJoints[i].q = angles.ToQuat();
		baseJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		blendJoints[i].q = angles.ToQuat();
		blendJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		blendJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
		index[i] = i;
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < COUNT; j++ ) {
			joints1[j] = baseJoints[j];
		}
		StartRecordTime( start );
		p_generic->BlendJoints( joints1, blendJoints, lerp, index, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->BlendJoints()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < COUNT; j++ ) {
			joints2[j] = baseJoints[j];
		}
		StartRecordTime( start );
		p_simd->BlendJoints( joints2, blendJoints, lerp, index, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !joints1[i].t.Compare( joints2[i].t, 1e-3f ) ) {
			break;
		}
		if ( !joints1[i].q.Compare( joints2[i].q, 1e-2f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->BlendJoints() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestConvertJointQuatsToJointMats
============
*/
void TestConvertJointQuatsToJointMats( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( idJointQuat baseJoints[COUNT] );
	ALIGN16( arcJointMat joints1[COUNT] );
	ALIGN16( arcJointMat joints2[COUNT] );
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		arcAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		baseJoints[i].q = angles.ToQuat();
		baseJoints[i].t[0] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[1] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].t[2] = srnd.CRandomFloat() * 10.0f;
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->ConvertJointQuatsToJointMats( joints1, baseJoints, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ConvertJointQuatsToJointMats()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->ConvertJointQuatsToJointMats( joints2, baseJoints, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !joints1[i].Compare( joints2[i], 1e-4f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->ConvertJointQuatsToJointMats() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestConvertJointMatsToJointQuats
============
*/
void TestConvertJointMatsToJointQuats( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( arcJointMat baseJoints[COUNT] );
	ALIGN16( idJointQuat joints1[COUNT] );
	ALIGN16( idJointQuat joints2[COUNT] );
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		arcAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		baseJoints[i].SetRotation( angles.ToMat3() );
		arcVec3 v;
		v[0] = srnd.CRandomFloat() * 10.0f;
		v[1] = srnd.CRandomFloat() * 10.0f;
		v[2] = srnd.CRandomFloat() * 10.0f;
		baseJoints[i].SetTranslation( v );
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->ConvertJointMatsToJointQuats( joints1, baseJoints, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->ConvertJointMatsToJointQuats()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->ConvertJointMatsToJointQuats( joints2, baseJoints, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !joints1[i].q.Compare( joints2[i].q, 1e-4f ) ) {
			arcLibrary::common->Printf( "ConvertJointMatsToJointQuats: broken q %i\n", i );
			break;
		}
		if ( !joints1[i].t.Compare( joints2[i].t, 1e-4f ) ) {
			arcLibrary::common->Printf( "ConvertJointMatsToJointQuats: broken t %i\n", i );
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->ConvertJointMatsToJointQuats() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestTransformJoints
============
*/
void TestTransformJoints( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( arcJointMat joints[COUNT+1] );
	ALIGN16( arcJointMat joints1[COUNT+1] );
	ALIGN16( arcJointMat joints2[COUNT+1] );
	ALIGN16( int parents[COUNT+1] );
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i <= COUNT; i++ ) {
		arcAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		joints[i].SetRotation( angles.ToMat3() );
		arcVec3 v;
		v[0] = srnd.CRandomFloat() * 2.0f;
		v[1] = srnd.CRandomFloat() * 2.0f;
		v[2] = srnd.CRandomFloat() * 2.0f;
		joints[i].SetTranslation( v );
		parents[i] = i - 1;
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j <= COUNT; j++ ) {
			joints1[j] = joints[j];
		}
		StartRecordTime( start );
		p_generic->TransformJoints( joints1, parents, 1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->TransformJoints()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j <= COUNT; j++ ) {
			joints2[j] = joints[j];
		}
		StartRecordTime( start );
		p_simd->TransformJoints( joints2, parents, 1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !joints1[i+1].Compare( joints2[i+1], 1e-4f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->TransformJoints() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestUntransformJoints
============
*/
void TestUntransformJoints( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( arcJointMat joints[COUNT+1] );
	ALIGN16( arcJointMat joints1[COUNT+1] );
	ALIGN16( arcJointMat joints2[COUNT+1] );
	ALIGN16( int parents[COUNT+1] );
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i <= COUNT; i++ ) {
		arcAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		joints[i].SetRotation( angles.ToMat3() );
		arcVec3 v;
		v[0] = srnd.CRandomFloat() * 2.0f;
		v[1] = srnd.CRandomFloat() * 2.0f;
		v[2] = srnd.CRandomFloat() * 2.0f;
		joints[i].SetTranslation( v );
		parents[i] = i - 1;
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j <= COUNT; j++ ) {
			joints1[j] = joints[j];
		}
		StartRecordTime( start );
		p_generic->UntransformJoints( joints1, parents, 1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->UntransformJoints()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j <= COUNT; j++ ) {
			joints2[j] = joints[j];
		}
		StartRecordTime( start );
		p_simd->UntransformJoints( joints2, parents, 1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !joints1[i+1].Compare( joints2[i+1], 1e-4f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->UntransformJoints() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestTransformVerts
============
*/
#define NUMJOINTS	64
#define NUMVERTS	COUNT/2
void TestTransformVerts( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( arcDrawVert drawVerts1[NUMVERTS] );
	ALIGN16( arcDrawVert drawVerts2[NUMVERTS] );
	ALIGN16( arcJointMat joints[NUMJOINTS] );
	ALIGN16( arcVec4 weights[COUNT] );
	ALIGN16( int weightIndex[COUNT*2] );
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < NUMJOINTS; i++ ) {
		arcAngles angles;
		angles[0] = srnd.CRandomFloat() * 180.0f;
		angles[1] = srnd.CRandomFloat() * 180.0f;
		angles[2] = srnd.CRandomFloat() * 180.0f;
		joints[i].SetRotation( angles.ToMat3() );
		arcVec3 v;
		v[0] = srnd.CRandomFloat() * 2.0f;
		v[1] = srnd.CRandomFloat() * 2.0f;
		v[2] = srnd.CRandomFloat() * 2.0f;
		joints[i].SetTranslation( v );
	}

	for ( i = 0; i < COUNT; i++ ) {
		weights[i][0] = srnd.CRandomFloat() * 2.0f;
		weights[i][1] = srnd.CRandomFloat() * 2.0f;
		weights[i][2] = srnd.CRandomFloat() * 2.0f;
		weights[i][3] = srnd.CRandomFloat();
		weightIndex[i*2+0] = ( i * NUMJOINTS / COUNT ) * sizeof( arcJointMat );
		weightIndex[i*2+1] = i & 1;
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->TransformVerts( drawVerts1, NUMVERTS, joints, weights, weightIndex, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->TransformVerts()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->TransformVerts( drawVerts2, NUMVERTS, joints, weights, weightIndex, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < NUMVERTS; i++ ) {
		if ( !drawVerts1[i].xyz.Compare( drawVerts2[i].xyz, 0.5f ) ) {
			break;
		}
	}
	result = ( i >= NUMVERTS ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->TransformVerts() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestTracePointCull
============
*/
void TestTracePointCull( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( arcPlane planes[4] );
	ALIGN16( arcDrawVert drawVerts[COUNT] );
	ALIGN16( byte cullBits1[COUNT] );
	ALIGN16( byte cullBits2[COUNT] );
	byte totalOr1 = 0, totalOr2 = 0;
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	planes[0].SetNormal( arcVec3(  1,  0, 0 ) );
	planes[1].SetNormal( arcVec3( -1,  0, 0 ) );
	planes[2].SetNormal( arcVec3(  0,  1, 0 ) );
	planes[3].SetNormal( arcVec3(  0, -1, 0 ) );
	planes[0][3] = -5.3f;
	planes[1][3] = 5.3f;
	planes[2][3] = -3.4f;
	planes[3][3] = 3.4f;

	for ( i = 0; i < COUNT; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			drawVerts[i].xyz[j] = srnd.CRandomFloat() * 10.0f;
		}
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->TracePointCull( cullBits1, totalOr1, 0.0f, planes, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->TracePointCull()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->TracePointCull( cullBits2, totalOr2, 0.0f, planes, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( cullBits1[i] != cullBits2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT && totalOr1 == totalOr2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->TracePointCull() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestDecalPointCull
============
*/
void TestDecalPointCull( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( arcPlane planes[6] );
	ALIGN16( arcDrawVert drawVerts[COUNT] );
	ALIGN16( byte cullBits1[COUNT] );
	ALIGN16( byte cullBits2[COUNT] );
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	planes[0].SetNormal( arcVec3(  1,  0,  0 ) );
	planes[1].SetNormal( arcVec3( -1,  0,  0 ) );
	planes[2].SetNormal( arcVec3(  0,  1,  0 ) );
	planes[3].SetNormal( arcVec3(  0, -1,  0 ) );
	planes[4].SetNormal( arcVec3(  0,  0,  1 ) );
	planes[5].SetNormal( arcVec3(  0,  0, -1 ) );
	planes[0][3] = -5.3f;
	planes[1][3] = 5.3f;
	planes[2][3] = -4.4f;
	planes[3][3] = 4.4f;
	planes[4][3] = -3.5f;
	planes[5][3] = 3.5f;

	for ( i = 0; i < COUNT; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			drawVerts[i].xyz[j] = srnd.CRandomFloat() * 10.0f;
		}
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->DecalPointCull( cullBits1, planes, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->DecalPointCull()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->DecalPointCull( cullBits2, planes, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( cullBits1[i] != cullBits2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->DecalPointCull() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestOverlayPointCull
============
*/
void TestOverlayPointCull( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( arcPlane planes[2] );
	ALIGN16( arcDrawVert drawVerts[COUNT] );
	ALIGN16( byte cullBits1[COUNT] );
	ALIGN16( byte cullBits2[COUNT] );
	ALIGN16( arcVec2 texCoords1[COUNT] );
	ALIGN16( arcVec2 texCoords2[COUNT] );
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	planes[0].SetNormal( arcVec3( 0.3f, 0.2f, 0.9f ) );
	planes[1].SetNormal( arcVec3( 0.9f, 0.2f, 0.3f ) );
	planes[0][3] = -5.3f;
	planes[1][3] = -4.3f;

	for ( i = 0; i < COUNT; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			drawVerts[i].xyz[j] = srnd.CRandomFloat() * 10.0f;
		}
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->OverlayPointCull( cullBits1, texCoords1, planes, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->OverlayPointCull()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->OverlayPointCull( cullBits2, texCoords2, planes, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( cullBits1[i] != cullBits2[i] ) {
			break;
		}
		if ( !texCoords1[i].Compare( texCoords2[i], 1e-4f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->OverlayPointCull() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestDeriveTriPlanes
============
*/
void TestDeriveTriPlanes( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( arcDrawVert drawVerts1[COUNT] );
	ALIGN16( arcDrawVert drawVerts2[COUNT] );
	ALIGN16( arcPlane planes1[COUNT] );
	ALIGN16( arcPlane planes2[COUNT] );
	ALIGN16( int indexes[COUNT*3] );
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			drawVerts1[i].xyz[j] = srnd.CRandomFloat() * 10.0f;
		}
		for ( j = 0; j < 2; j++ ) {
			drawVerts1[i].st[j] = srnd.CRandomFloat();
		}
		drawVerts2[i] = drawVerts1[i];
	}

	for ( i = 0; i < COUNT; i++ ) {
		indexes[i*3+0] = ( i + 0 ) % COUNT;
		indexes[i*3+1] = ( i + 1 ) % COUNT;
		indexes[i*3+2] = ( i + 2 ) % COUNT;
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->DeriveTriPlanes( planes1, drawVerts1, COUNT, indexes, COUNT*3 );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->DeriveTriPlanes()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->DeriveTriPlanes( planes2, drawVerts2, COUNT, indexes, COUNT*3 );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !planes1[i].Compare( planes2[i], 1e-1f, 1e-1f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->DeriveTriPlanes() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestDeriveTangents
============
*/
void TestDeriveTangents( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( arcDrawVert drawVerts1[COUNT] );
	ALIGN16( arcDrawVert drawVerts2[COUNT] );
	ALIGN16( arcPlane planes1[COUNT] );
	ALIGN16( arcPlane planes2[COUNT] );
	ALIGN16( int indexes[COUNT*3] );
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			drawVerts1[i].xyz[j] = srnd.CRandomFloat() * 10.0f;
		}
		for ( j = 0; j < 2; j++ ) {
			drawVerts1[i].st[j] = srnd.CRandomFloat();
		}
		drawVerts2[i] = drawVerts1[i];
	}

	for ( i = 0; i < COUNT; i++ ) {
		indexes[i*3+0] = ( i + 0 ) % COUNT;
		indexes[i*3+1] = ( i + 1 ) % COUNT;
		indexes[i*3+2] = ( i + 2 ) % COUNT;
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->DeriveTangents( planes1, drawVerts1, COUNT, indexes, COUNT*3 );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->DeriveTangents()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->DeriveTangents( planes2, drawVerts2, COUNT, indexes, COUNT*3 );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		arcVec3 v1, v2;

		v1 = drawVerts1[i].normal;
		v1.Normalize();
		v2 = drawVerts2[i].normal;
		v2.Normalize();
		if ( !v1.Compare( v2, 1e-1f ) ) {
			arcLibrary::common->Printf( "DeriveTangents: broken at normal %i\n -- expecting %s got %s", i, v1.ToString(), v2.ToString() );
			break;
		}
		v1 = drawVerts1[i].tangents[0];
		v1.Normalize();
		v2 = drawVerts2[i].tangents[0];
		v2.Normalize();
		if ( !v1.Compare( v2, 1e-1f ) ) {
			arcLibrary::common->Printf( "DeriveTangents: broken at tangent0 %i -- expecting %s got %s\n", i, v1.ToString(), v2.ToString() );
			break;
		}
		v1 = drawVerts1[i].tangents[1];
		v1.Normalize();
		v2 = drawVerts2[i].tangents[1];
		v2.Normalize();
		if ( !v1.Compare( v2, 1e-1f ) ) {
			arcLibrary::common->Printf( "DeriveTangents: broken at tangent1 %i -- expecting %s got %s\n", i, v1.ToString(), v2.ToString() );
			break;
		}
		if ( !planes1[i].Compare( planes2[i], 1e-1f, 1e-1f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->DeriveTangents() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestDeriveUnsmoothedTangents
============
*/
void TestDeriveUnsmoothedTangents( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( arcDrawVert drawVerts1[COUNT] );
	ALIGN16( arcDrawVert drawVerts2[COUNT] );
	ALIGN16( dominantTri_s dominantTris[COUNT] );
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			drawVerts1[i].xyz[j] = srnd.CRandomFloat() * 10.0f;
		}
		for ( j = 0; j < 2; j++ ) {
			drawVerts1[i].st[j] = srnd.CRandomFloat();
		}
		drawVerts2[i] = drawVerts1[i];

		dominantTris[i].v2 = ( i + 1 + srnd.RandomInt( 8 ) ) % COUNT;
		dominantTris[i].v3 = ( i + 9 + srnd.RandomInt( 8 ) ) % COUNT;
		dominantTris[i].normalizationScale[0] = srnd.CRandomFloat();
		dominantTris[i].normalizationScale[1] = srnd.CRandomFloat();
		dominantTris[i].normalizationScale[2] = srnd.CRandomFloat();
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->DeriveUnsmoothedTangents( drawVerts1, dominantTris, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->DeriveUnsmoothedTangents()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->DeriveUnsmoothedTangents( drawVerts2, dominantTris, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		arcVec3 v1, v2;

		v1 = drawVerts1[i].normal;
		v1.Normalize();
		v2 = drawVerts2[i].normal;
		v2.Normalize();
		if ( !v1.Compare( v2, 1e-1f ) ) {
			break;
		}
		v1 = drawVerts1[i].tangents[0];
		v1.Normalize();
		v2 = drawVerts2[i].tangents[0];
		v2.Normalize();
		if ( !v1.Compare( v2, 1e-1f ) ) {
			break;
		}
		v1 = drawVerts1[i].tangents[1];
		v1.Normalize();
		v2 = drawVerts2[i].tangents[1];
		v2.Normalize();
		if ( !v1.Compare( v2, 1e-1f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->DeriveUnsmoothedTangents() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestNormalizeTangents
============
*/
void TestNormalizeTangents( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( arcDrawVert drawVerts1[COUNT] );
	ALIGN16( arcDrawVert drawVerts2[COUNT] );
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			drawVerts1[i].normal[j] = srnd.CRandomFloat() * 10.0f;
			drawVerts1[i].tangents[0][j] = srnd.CRandomFloat() * 10.0f;
			drawVerts1[i].tangents[1][j] = srnd.CRandomFloat() * 10.0f;
		}
		drawVerts2[i] = drawVerts1[i];
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->NormalizeTangents( drawVerts1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->NormalizeTangents()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->NormalizeTangents( drawVerts2, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !drawVerts1[i].normal.Compare( drawVerts2[i].normal, 1e-2f ) ) {
			break;
		}
		if ( !drawVerts1[i].tangents[0].Compare( drawVerts2[i].tangents[0], 1e-2f ) ) {
			break;
		}
		if ( !drawVerts1[i].tangents[1].Compare( drawVerts2[i].tangents[1], 1e-2f ) ) {
			break;
		}

		// since we're doing a lot of unaligned work, added this check to
		// make sure xyz wasn't getting overwritten
		if ( !drawVerts1[i].xyz.Compare( drawVerts2[i].xyz, 1e-2f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->NormalizeTangents() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestGetTextureSpaceLightVectors
============
*/
void TestGetTextureSpaceLightVectors( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( arcDrawVert drawVerts[COUNT] );
	ALIGN16( arcVec4 texCoords1[COUNT] );
	ALIGN16( arcVec4 texCoords2[COUNT] );
	ALIGN16( int indexes[COUNT*3] );
	ALIGN16( arcVec3 lightVectors1[COUNT] );
	ALIGN16( arcVec3 lightVectors2[COUNT] );
	arcVec3 lightOrigin;
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			drawVerts[i].xyz[j] = srnd.CRandomFloat() * 100.0f;
			drawVerts[i].normal[j] = srnd.CRandomFloat();
			drawVerts[i].tangents[0][j] = srnd.CRandomFloat();
			drawVerts[i].tangents[1][j] = srnd.CRandomFloat();
		}
	}

	for ( i = 0; i < COUNT; i++ ) {
		indexes[i*3+0] = ( i + 0 ) % COUNT;
		indexes[i*3+1] = ( i + 1 ) % COUNT;
		indexes[i*3+2] = ( i + 2 ) % COUNT;
	}

	lightOrigin[0] = srnd.CRandomFloat() * 100.0f;
	lightOrigin[1] = srnd.CRandomFloat() * 100.0f;
	lightOrigin[2] = srnd.CRandomFloat() * 100.0f;

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->CreateTextureSpaceLightVectors( lightVectors1, lightOrigin, drawVerts, COUNT, indexes, COUNT*3 );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CreateTextureSpaceLightVectors()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->CreateTextureSpaceLightVectors( lightVectors2, lightOrigin, drawVerts, COUNT, indexes, COUNT*3 );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !lightVectors1[i].Compare( lightVectors2[i], 1e-4f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CreateTextureSpaceLightVectors() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestGetSpecularTextureCoords
============
*/
void TestGetSpecularTextureCoords( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( arcDrawVert drawVerts[COUNT] );
	ALIGN16( arcVec4 texCoords1[COUNT] );
	ALIGN16( arcVec4 texCoords2[COUNT] );
	ALIGN16( int indexes[COUNT*3] );
	ALIGN16( arcVec3 lightVectors1[COUNT] );
	ALIGN16( arcVec3 lightVectors2[COUNT] );
	arcVec3 lightOrigin, viewOrigin;
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			drawVerts[i].xyz[j] = srnd.CRandomFloat() * 100.0f;
			drawVerts[i].normal[j] = srnd.CRandomFloat();
			drawVerts[i].tangents[0][j] = srnd.CRandomFloat();
			drawVerts[i].tangents[1][j] = srnd.CRandomFloat();
		}
	}

	for ( i = 0; i < COUNT; i++ ) {
		indexes[i*3+0] = ( i + 0 ) % COUNT;
		indexes[i*3+1] = ( i + 1 ) % COUNT;
		indexes[i*3+2] = ( i + 2 ) % COUNT;
	}

	lightOrigin[0] = srnd.CRandomFloat() * 100.0f;
	lightOrigin[1] = srnd.CRandomFloat() * 100.0f;
	lightOrigin[2] = srnd.CRandomFloat() * 100.0f;
	viewOrigin[0] = srnd.CRandomFloat() * 100.0f;
	viewOrigin[1] = srnd.CRandomFloat() * 100.0f;
	viewOrigin[2] = srnd.CRandomFloat() * 100.0f;

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->CreateSpecularTextureCoords( texCoords1, lightOrigin, viewOrigin, drawVerts, COUNT, indexes, COUNT*3 );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CreateSpecularTextureCoords()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->CreateSpecularTextureCoords( texCoords2, lightOrigin, viewOrigin, drawVerts, COUNT, indexes, COUNT*3 );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !texCoords1[i].Compare( texCoords2[i], 1e-2f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CreateSpecularTextureCoords() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestCreateShadowCache
============
*/
void TestCreateShadowCache( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( arcDrawVert drawVerts[COUNT] );
	ALIGN16( arcVec4 vertexCache1[COUNT*2] );
	ALIGN16( arcVec4 vertexCache2[COUNT*2] );
	ALIGN16( int originalVertRemap[COUNT] );
	ALIGN16( int vertRemap1[COUNT] );
	ALIGN16( int vertRemap2[COUNT] );
	ALIGN16( arcVec3 lightOrigin );
	int numVerts1 = 0, numVerts2 = 0;
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		drawVerts[i].xyz[0] = srnd.CRandomFloat() * 100.0f;
		drawVerts[i].xyz[1] = srnd.CRandomFloat() * 100.0f;
		drawVerts[i].xyz[2] = srnd.CRandomFloat() * 100.0f;
		originalVertRemap[i] = ( srnd.CRandomFloat() > 0.0f ) ? -1 : 0;
	}
	lightOrigin[0] = srnd.CRandomFloat() * 100.0f;
	lightOrigin[1] = srnd.CRandomFloat() * 100.0f;
	lightOrigin[2] = srnd.CRandomFloat() * 100.0f;

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < COUNT; j++ ) {
			vertRemap1[j] = originalVertRemap[j];
		}
		StartRecordTime( start );
		numVerts1 =p_generic->CreateShadowCache( vertexCache1, vertRemap1, lightOrigin, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CreateShadowCache()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < COUNT; j++ ) {
			vertRemap2[j] = originalVertRemap[j];
		}
		StartRecordTime( start );
		numVerts2 = p_simd->CreateShadowCache( vertexCache2, vertRemap2, lightOrigin, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( i < ( numVerts1 / 2 ) ) {
			if ( !vertexCache1[i*2+0].Compare( vertexCache2[i*2+0], 1e-2f ) ) {
				break;
			}
			if ( !vertexCache1[i*2+1].Compare( vertexCache2[i*2+1], 1e-2f ) ) {
				break;
			}
		}
		if ( vertRemap1[i] != vertRemap2[i] ) {
			break;
		}
	}

	result = ( i >= COUNT && numVerts1 == numVerts2 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CreateShadowCache() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_generic->CreateVertexProgramShadowCache( vertexCache1, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->CreateVertexProgramShadowCache()", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		p_simd->CreateVertexProgramShadowCache( vertexCache2, drawVerts, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( !vertexCache1[i*2+0].Compare( vertexCache2[i*2+0], 1e-2f ) ) {
			break;
		}
		if ( !vertexCache1[i*2+1].Compare( vertexCache2[i*2+1], 1e-2f ) ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->CreateVertexProgramShadowCache() %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestSoundUpSampling
============
*/
#define SOUND_UPSAMPLE_EPSILON		1.0f

void TestSoundUpSampling( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( short pcm[MIXBUFFER_SAMPLES*2] );
	ALIGN16( float ogg0[MIXBUFFER_SAMPLES*2] );
	ALIGN16( float ogg1[MIXBUFFER_SAMPLES*2] );
	ALIGN16( float samples1[MIXBUFFER_SAMPLES*2] );
	ALIGN16( float samples2[MIXBUFFER_SAMPLES*2] );
	float *ogg[2];
	int kHz, numSpeakers;
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < MIXBUFFER_SAMPLES*2; i++ ) {
		pcm[i] = srnd.RandomInt( (1<<16) ) - (1<<15);
		ogg0[i] = srnd.RandomFloat();
		ogg1[i] = srnd.RandomFloat();
	}

	ogg[0] = ogg0;
	ogg[1] = ogg1;

	for ( numSpeakers = 1; numSpeakers <= 2; numSpeakers++ ) {

		for ( kHz = 11025; kHz <= 44100; kHz *= 2 ) {
			bestClocksGeneric = 0;
			for ( i = 0; i < NUMTESTS; i++ ) {
				StartRecordTime( start );
				p_generic->UpSamplePCMTo44kHz( samples1, pcm, MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, kHz, numSpeakers );
				StopRecordTime( end );
				GetBest( start, end, bestClocksGeneric );
			}
			PrintClocks( va( "generic->UpSamplePCMTo44kHz( %d, %d )", kHz, numSpeakers ), MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, bestClocksGeneric );

			bestClocksSIMD = 0;
			for ( i = 0; i < NUMTESTS; i++ ) {
				StartRecordTime( start );
				p_simd->UpSamplePCMTo44kHz( samples2, pcm, MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, kHz, numSpeakers );
				StopRecordTime( end );
				GetBest( start, end, bestClocksSIMD );
			}

			for ( i = 0; i < MIXBUFFER_SAMPLES*numSpeakers; i++ ) {
				if ( arcMath::Fabs( samples1[i] - samples2[i] ) > SOUND_UPSAMPLE_EPSILON ) {
					break;
				}
			}
			result = ( i >= MIXBUFFER_SAMPLES*numSpeakers ) ? "ok" : S_COLOR_RED"X";
			PrintClocks( va( "   simd->UpSamplePCMTo44kHz( %d, %d ) %s", kHz, numSpeakers, result ), MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, bestClocksSIMD, bestClocksGeneric );
		}
	}

	for ( numSpeakers = 1; numSpeakers <= 2; numSpeakers++ ) {

		for ( kHz = 11025; kHz <= 44100; kHz *= 2 ) {
			bestClocksGeneric = 0;
			for ( i = 0; i < NUMTESTS; i++ ) {
				StartRecordTime( start );
				p_generic->UpSampleOGGTo44kHz( samples1, ogg, MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, kHz, numSpeakers );
				StopRecordTime( end );
				GetBest( start, end, bestClocksGeneric );
			}
			PrintClocks( va( "generic->UpSampleOGGTo44kHz( %d, %d )", kHz, numSpeakers ), MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, bestClocksGeneric );

			bestClocksSIMD = 0;
			for ( i = 0; i < NUMTESTS; i++ ) {
				StartRecordTime( start );
				p_simd->UpSampleOGGTo44kHz( samples2, ogg, MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, kHz, numSpeakers );
				StopRecordTime( end );
				GetBest( start, end, bestClocksSIMD );
			}

			for ( i = 0; i < MIXBUFFER_SAMPLES*numSpeakers; i++ ) {
				if ( arcMath::Fabs( samples1[i] - samples2[i] ) > SOUND_UPSAMPLE_EPSILON ) {
					break;
				}
			}
			result = ( i >= MIXBUFFER_SAMPLES ) ? "ok" : S_COLOR_RED"X";
			PrintClocks( va( "   simd->UpSampleOGGTo44kHz( %d, %d ) %s", kHz, numSpeakers, result ), MIXBUFFER_SAMPLES*numSpeakers*kHz/44100, bestClocksSIMD, bestClocksGeneric );
		}
	}
}

/*
============
TestSoundMixing
============
*/
#define SOUND_MIX_EPSILON		2.0f

void TestSoundMixing( void ) {
	int i, j;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float origMixBuffer[MIXBUFFER_SAMPLES*6] );
	ALIGN16( float mixBuffer1[MIXBUFFER_SAMPLES*6] );
	ALIGN16( float mixBuffer2[MIXBUFFER_SAMPLES*6] );
	ALIGN16( float samples[MIXBUFFER_SAMPLES*6] );
	ALIGN16( short outSamples1[MIXBUFFER_SAMPLES*6] );
	ALIGN16( short outSamples2[MIXBUFFER_SAMPLES*6] );
	float lastV[6];
	float currentV[6];
	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < 6; i++ ) {
		lastV[i] = srnd.CRandomFloat();
		currentV[i] = srnd.CRandomFloat();
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*6; i++ ) {
		origMixBuffer[i] = srnd.CRandomFloat();
		samples[i] = srnd.RandomInt( (1<<16) ) - (1<<15);
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*6; j++ ) {
			mixBuffer1[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_generic->MixSoundTwoSpeakerMono( mixBuffer1, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MixSoundTwoSpeakerMono()", MIXBUFFER_SAMPLES, bestClocksGeneric );


	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*6; j++ ) {
			mixBuffer2[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_simd->MixSoundTwoSpeakerMono( mixBuffer2, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*6; i++ ) {
		if ( arcMath::Fabs( mixBuffer1[i] - mixBuffer2[i] ) > SOUND_MIX_EPSILON ) {
			break;
		}
	}
	result = ( i >= MIXBUFFER_SAMPLES*6 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MixSoundTwoSpeakerMono() %s", result ), MIXBUFFER_SAMPLES, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*6; j++ ) {
			mixBuffer1[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_generic->MixSoundTwoSpeakerStereo( mixBuffer1, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MixSoundTwoSpeakerStereo()", MIXBUFFER_SAMPLES, bestClocksGeneric );


	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*6; j++ ) {
			mixBuffer2[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_simd->MixSoundTwoSpeakerStereo( mixBuffer2, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*6; i++ ) {
		if ( arcMath::Fabs( mixBuffer1[i] - mixBuffer2[i] ) > SOUND_MIX_EPSILON ) {
			break;
		}
	}
	result = ( i >= MIXBUFFER_SAMPLES*6 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MixSoundTwoSpeakerStereo() %s", result ), MIXBUFFER_SAMPLES, bestClocksSIMD, bestClocksGeneric );


	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*6; j++ ) {
			mixBuffer1[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_generic->MixSoundSixSpeakerMono( mixBuffer1, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MixSoundSixSpeakerMono()", MIXBUFFER_SAMPLES, bestClocksGeneric );


	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*6; j++ ) {
			mixBuffer2[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_simd->MixSoundSixSpeakerMono( mixBuffer2, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*6; i++ ) {
		if ( arcMath::Fabs( mixBuffer1[i] - mixBuffer2[i] ) > SOUND_MIX_EPSILON ) {
			break;
		}
	}
	result = ( i >= MIXBUFFER_SAMPLES*6 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MixSoundSixSpeakerMono() %s", result ), MIXBUFFER_SAMPLES, bestClocksSIMD, bestClocksGeneric );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*6; j++ ) {
			mixBuffer1[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_generic->MixSoundSixSpeakerStereo( mixBuffer1, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MixSoundSixSpeakerStereo()", MIXBUFFER_SAMPLES, bestClocksGeneric );


	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*6; j++ ) {
			mixBuffer2[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_simd->MixSoundSixSpeakerStereo( mixBuffer2, samples, MIXBUFFER_SAMPLES, lastV, currentV );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*6; i++ ) {
		if ( arcMath::Fabs( mixBuffer1[i] - mixBuffer2[i] ) > SOUND_MIX_EPSILON ) {
			break;
		}
	}
	result = ( i >= MIXBUFFER_SAMPLES*6 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MixSoundSixSpeakerStereo() %s", result ), MIXBUFFER_SAMPLES, bestClocksSIMD, bestClocksGeneric );


	for ( i = 0; i < MIXBUFFER_SAMPLES*6; i++ ) {
		origMixBuffer[i] = srnd.RandomInt( (1<<17) ) - (1<<16);
	}

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*6; j++ ) {
			mixBuffer1[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_generic->MixedSoundToSamples( outSamples1, mixBuffer1, MIXBUFFER_SAMPLES*6 );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->MixedSoundToSamples()", MIXBUFFER_SAMPLES, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		for ( j = 0; j < MIXBUFFER_SAMPLES*6; j++ ) {
			mixBuffer2[j] = origMixBuffer[j];
		}
		StartRecordTime( start );
		p_simd->MixedSoundToSamples( outSamples2, mixBuffer2, MIXBUFFER_SAMPLES*6 );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < MIXBUFFER_SAMPLES*6; i++ ) {
		if ( outSamples1[i] != outSamples2[i] ) {
			break;
		}
	}
	result = ( i >= MIXBUFFER_SAMPLES*6 ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->MixedSoundToSamples() %s", result ), MIXBUFFER_SAMPLES, bestClocksSIMD, bestClocksGeneric );
}

/*
============
TestMath
============
*/
void TestMath( void ) {
	int i;
	TIME_TYPE start, end, bestClocks;

	arcLibrary::common->Printf( "====================================\n" );

	float tst = -1.0f;
	float tst2 = 1.0f;
	float testvar = 1.0f;
	arcRandom rnd;

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = fabs( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "            fabs( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		int tmp = * ( int * ) &tst;
		tmp &= 0x7FFFFFFF;
		tst = * ( float * ) &tmp;
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    arcMath::Fabs( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = 10.0f + 100.0f * rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = sqrt( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.01f;
		tst = 10.0f + 100.0f * rnd.RandomFloat();
	}
	PrintClocks( "            sqrt( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = arcMath::Sqrt( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.RandomFloat();
	}
	PrintClocks( "    arcMath::Sqrt( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = arcMath::Sqrt16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.RandomFloat();
	}
	PrintClocks( "  arcMath::Sqrt16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = arcMath::Sqrt64( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.RandomFloat();
	}
	PrintClocks( "  arcMath::Sqrt64( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.RandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = tst * arcMath::RSqrt( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.RandomFloat();
	}
	PrintClocks( "   arcMath::RSqrt( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = arcMath::Sin( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "     arcMath::Sin( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = arcMath::Sin16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "   arcMath::Sin16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = arcMath::Cos( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "     arcMath::Cos( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = arcMath::Cos16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "   arcMath::Cos16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		arcMath::SinCos( tst, tst, tst2 );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  arcMath::SinCos( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		arcMath::SinCos16( tst, tst, tst2 );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "arcMath::SinCos16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = arcMath::Tan( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "     arcMath::Tan( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = arcMath::Tan16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "   arcMath::Tan16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = arcMath::ASin( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * ( 1.0f / arcMath::PI );
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    arcMath::ASin( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = arcMath::ASin16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * ( 1.0f / arcMath::PI );
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  arcMath::ASin16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = arcMath::ACos( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * ( 1.0f / arcMath::PI );
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    arcMath::ACos( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = arcMath::ACos16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * ( 1.0f / arcMath::PI );
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  arcMath::ACos16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = arcMath::ATan( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    arcMath::ATan( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = arcMath::ATan16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  arcMath::ATan16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = arcMath::Pow( 2.7f, tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    arcMath::Pow( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = arcMath::Pow16( 2.7f, tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  arcMath::Pow16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = arcMath::Exp( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    arcMath::Exp( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		tst = arcMath::Exp16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst * 0.1f;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  arcMath::Exp16( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		tst = fabs( tst ) + 1.0f;
		StartRecordTime( start );
		tst = arcMath::Log( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "    arcMath::Log( tst )", 1, bestClocks );

	bestClocks = 0;
	tst = rnd.CRandomFloat();
	for ( i = 0; i < NUMTESTS; i++ ) {
		tst = fabs( tst ) + 1.0f;
		StartRecordTime( start );
		tst = arcMath::Log16( tst );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
		testvar = ( testvar + tst ) * tst;
		tst = rnd.CRandomFloat();
	}
	PrintClocks( "  arcMath::Log16( tst )", 1, bestClocks );

	arcLibrary::common->Printf( "testvar = %f\n", testvar );

	arcMat3 resultMat3;
	arcQuats fromQuat, toQuat, resultQuat;
	arcCQuats cq;
	arcAngles ang;

	fromQuat = arcAngles( 30, 45, 0 ).ToQuat();
	toQuat = arcAngles( 45, 0, 0 ).ToQuat();
	cq = arcAngles( 30, 45, 0 ).ToQuat().ToCQuat();
	ang = arcAngles( 30, 40, 50 );

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		resultMat3 = fromQuat.ToMat3();
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "       arcQuats::ToMat3()", 1, bestClocks );

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		resultQuat.Slerp( fromQuat, toQuat, 0.3f );
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "        arcQuats::Slerp()", 1, bestClocks );

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		resultQuat = cq.ToQuat();
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "      arcCQuats::ToQuat()", 1, bestClocks );

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		resultQuat = ang.ToQuat();
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "     arcAngles::ToQuat()", 1, bestClocks );

	bestClocks = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {
		StartRecordTime( start );
		resultMat3 = ang.ToMat3();
		StopRecordTime( end );
		GetBest( start, end, bestClocks );
	}
	PrintClocks( "     arcAngles::ToMat3()", 1, bestClocks );
}

/*
============
TestNegate
============
*/

// this wasn't previously in the test
void TestNegate( void ) {
	int i;
	TIME_TYPE start, end, bestClocksGeneric, bestClocksSIMD;
	ALIGN16( float fsrc0[COUNT] );
	ALIGN16( float fsrc1[COUNT] );
	ALIGN16( float fsrc2[COUNT] );

	const char *result;

	arcRandom srnd( RANDOM_SEED );

	for ( i = 0; i < COUNT; i++ ) {
		fsrc0[i] = fsrc1[i] = fsrc2[i] = srnd.CRandomFloat() * 10.0f;
		//fsrc1[i] = srnd.CRandomFloat() * 10.0f;
	}

	arcLibrary::common->Printf( "====================================\n" );

	bestClocksGeneric = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {

		memcpy( &fsrc1[0], &fsrc0[0], COUNT * sizeof( float ) );

		StartRecordTime( start );
		p_generic->Negate16( fsrc1, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksGeneric );
	}
	PrintClocks( "generic->Negate16( float[] )", COUNT, bestClocksGeneric );

	bestClocksSIMD = 0;
	for ( i = 0; i < NUMTESTS; i++ ) {

		memcpy( &fsrc2[0], &fsrc0[0], COUNT * sizeof( float ) );

		StartRecordTime( start );
		p_simd->Negate16( fsrc2, COUNT );
		StopRecordTime( end );
		GetBest( start, end, bestClocksSIMD );
	}

	for ( i = 0; i < COUNT; i++ ) {
		if ( fsrc1[i] != fsrc2[i] ) {
			break;
		}
	}
	result = ( i >= COUNT ) ? "ok" : S_COLOR_RED"X";
	PrintClocks( va( "   simd->Negate16( float[] ) %s", result ), COUNT, bestClocksSIMD, bestClocksGeneric );
}


/*
============
arcSIMD::Test_f
============
*/
void arcSIMD::Test_f( const arcCommandArgs &args ) {

#ifdef _WIN32
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );
#endif /* _WIN32 */

	p_simd = processor;
	p_generic = generic;

	if ( arcNetString::Length( args.Argv( 1 ) ) != 0 ) {
		cpuid_t cpuid = arcLibrary::sys->GetProcessorId();
		arcNetString argString = args.Args();

		argString.Replace( " ", "" );

		if ( arcNetString::Icmp( argString, "MMX" ) == 0 ) {
			if ( !( cpuid & CPUID_MMX ) ) {
				common->Printf( "CPU does not support MMX\n" );
				return;
			}
			p_simd = new arcSIMD_MMX;
		} else if ( arcNetString::Icmp( argString, "3DNow" ) == 0 ) {
			if ( !( cpuid & CPUID_MMX ) || !( cpuid & CPUID_3DNOW ) ) {
				common->Printf( "CPU does not support MMX & 3DNow\n" );
				return;
			}
			p_simd = new idSIMD_3DNow;
		} else if ( arcNetString::Icmp( argString, "SSE" ) == 0 ) {
			if ( !( cpuid & CPUID_MMX ) || !( cpuid & CPUID_SSE ) ) {
				common->Printf( "CPU does not support MMX & SSE\n" );
				return;
			}
			p_simd = new idSIMD_SSE;
		} else if ( arcNetString::Icmp( argString, "SSE2" ) == 0 ) {
			if ( !( cpuid & CPUID_MMX ) || !( cpuid & CPUID_SSE ) || !( cpuid & CPUID_SSE2 ) ) {
				common->Printf( "CPU does not support MMX & SSE & SSE2\n" );
				return;
			}
			p_simd = new idSIMD_SSE2;
		} else if ( arcNetString::Icmp( argString, "SSE3" ) == 0 ) {
			if ( !( cpuid & CPUID_MMX ) || !( cpuid & CPUID_SSE ) || !( cpuid & CPUID_SSE2 ) || !( cpuid & CPUID_SSE3 ) ) {
				common->Printf( "CPU does not support MMX & SSE & SSE2 & SSE3\n" );
				return;
			}
			p_simd = new arcSIMD_SSE3();
		} else if ( arcNetString::Icmp( argString, "AltiVec" ) == 0 ) {
			if ( !( cpuid & CPUID_ALTIVEC ) ) {
				common->Printf( "CPU does not support AltiVec\n" );
				return;
			}
			p_simd = new arcSIMD_AltiVec();
		} else {
			common->Printf( "invalid argument, use: MMX, 3DNow, SSE, SSE2, SSE3, AltiVec\n" );
			return;
		}
	}

	arcLibrary::common->SetRefreshOnPrint( true );

	arcLibrary::common->Printf( "using %s for SIMD processing\n", p_simd->GetName() );

	GetBaseClocks();

	TestMath();
	TestAdd();
	TestSub();
	TestMul();
	TestDiv();
	TestMulAdd();
	TestMulSub();
	TestDot();
	TestCompare();
	TestMinMax();
	TestClamp();
	TestMemcpy();
	TestMemset();
	TestNegate();

	TestMatXMultiplyVecX();
	TestMatXMultiplyAddVecX();
	TestMatXTransposeMultiplyVecX();
	TestMatXTransposeMultiplyAddVecX();
	TestMatXMultiplyMatX();
	TestMatXTransposeMultiplyMatX();
	TestMatXLowerTriangularSolve();
	TestMatXLowerTriangularSolveTranspose();
	TestMatXLDLTFactor();

	arcLibrary::common->Printf( "====================================\n" );

	TestBlendJoints();
	TestConvertJointQuatsToJointMats();
	TestConvertJointMatsToJointQuats();
	TestTransformJoints();
	TestUntransformJoints();
	TestTransformVerts();
	TestTracePointCull();
	TestDecalPointCull();
	TestOverlayPointCull();
	TestDeriveTriPlanes();
	TestDeriveTangents();
	TestDeriveUnsmoothedTangents();
	TestNormalizeTangents();
	TestGetTextureSpaceLightVectors();
	TestGetSpecularTextureCoords();
	TestCreateShadowCache();

	arcLibrary::common->Printf( "====================================\n" );

	TestSoundUpSampling();
	TestSoundMixing();

	arcLibrary::common->SetRefreshOnPrint( false );

	if ( p_simd != processor ) {
		delete p_simd;
	}
	p_simd = NULL;
	p_generic = NULL;

#ifdef _WIN32
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_NORMAL );
#endif /* _WIN32 */
}
