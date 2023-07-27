#ifndef __MATH_MATH_H__
#define __MATH_MATH_H__

#ifdef MACOS_X
// for square root estimate instruction
#include <ppc_intrinsics.h>
// for FLT_MIN
#include <float.h>
#endif
/*
===============================================================================

  Math

===============================================================================
*/

#ifdef INFINITY
#undef INFINITY
#endif

#ifdef FLT_EPSILON
#undef FLT_EPSILON
#endif

#define DEG2RAD( a )				( ( a ) * anMath::M_DEG2RAD )
#define RAD2DEG( a )				( ( a ) * anMath::M_RAD2DEG )

#define SEC2MS( t )					( anMath::FtoiFast( ( t ) * anMath::M_SEC2MS ) )
#define MS2SEC( t )					( ( t ) * anMath::M_MS2SEC )

#define	ANGLE2SHORT( x )			( anMath::FtoiFast( ( x ) * 65536.0f / 360.0f ) & 65535 )
#define	SHORT2ANGLE( x )			( ( x ) * ( 360.0f / 65536.0f ) )

#define	ANGLE2BYTE( x )				( anMath::FtoiFast( ( x ) * 256.0f / 360.0f ) & 255 )
#define	BYTE2ANGLE( x )				( ( x ) * ( 360.0f / 256.0f ) )

#define FLOATSIGNBITSET( f )		( (*(const unsigned long *)&( f ) ) >> 31 )
#define FLOATSIGNBITNOTSET( f )		( (~(*(const unsigned long *)&( f ) ) ) >> 31 )
#define FLOATNOTZERO( f )			( (*(const unsigned long *)&( f ) ) & ~(1<<31 ) )
#define INTSIGNBITSET( i )			( ( ( const unsigned long)( i ) ) >> 31 )
#define INTSIGNBITNOTSET( i )		( (~( (const unsigned long)( i ) ) ) >> 31 )

#define	FLOAT_IS_NAN( x )			( ( (*(const unsigned long *)&x) & 0x7f800000) == 0x7f800000)
#define FLOAT_IS_INF( x )			( ( (*(const unsigned long *)&x) & 0x7fffffff) == 0x7f800000)
#define FLOAT_IS_IND( x )			( (*(const unsigned long *)&x) == 0xffc00000)
#define	FLOAT_IS_DENORMAL( x )		( ( (*(const unsigned long *)&x) & 0x7f800000) == 0x00000000 && \
									 ( (*(const unsigned long *)&x) & 0x007fffff) != 0x00000000 )

#define IEEE_FLT_MANTISSA_BITS	23
#define IEEE_FLT_EXPONENT_BITS	8
#define IEEE_FLT_EXPONENT_BIAS	127
#define IEEE_FLT_SIGN_BIT		31

#define IEEE_DBL_MANTISSA_BITS	52
#define IEEE_DBL_EXPONENT_BITS	11
#define IEEE_DBL_EXPONENT_BIAS	1023
#define IEEE_DBL_SIGN_BIT		63

#define IEEE_DBLE_MANTISSA_BITS	63
#define IEEE_DBLE_EXPONENT_BITS	15
#define IEEE_DBLE_EXPONENT_BIAS	0
#define IEEE_DBLE_SIGN_BIT		79

template<class T> ARC_INLINE int	MaxIndex( T x, T y ) { return  ( x > y ) ? 0 : 1; }
template<class T> ARC_INLINE int	MinIndex( T x, T y ) { return ( x < y ) ? 0 : 1; }

template<class T> ARC_INLINE T		Max3( T x, T y, T z ) { return ( x > y ) ? ( ( x > z ) ? x : z ) : ( ( y > z ) ? y : z ); }
template<class T> ARC_INLINE T		Min3( T x, T y, T z ) { return ( x < y ) ? ( ( x < z ) ? x : z ) : ( ( y < z ) ? y : z ); }
template<class T> ARC_INLINE int	Max3Index( T x, T y, T z ) { return ( x > y ) ? ( ( x > z ) ? 0 : 2 ) : ( ( y > z ) ? 1 : 2 ); }
template<class T> ARC_INLINE int	Min3Index( T x, T y, T z ) { return ( x < y ) ? ( ( x < z ) ? 0 : 2 ) : ( ( y < z ) ? 1 : 2 ); }

template<class T> ARC_INLINE T	Sign( T f ) { return ( f > 0 ) ? 1 : ( ( f < 0 ) ? -1 : 0 ); }
template<class T> ARC_INLINE T	Square( T x ) { return x * x; }
template<class T> ARC_INLINE T	Cube( T x ) { return x * x * x; }

// move from Math.h to keep gcc happy, and now returned to ease gcc's stress.
template<class T> ARC_INLINE T;	Max( T x, T y ) { return ( x > y ) ? x : y; }
template<class T> ARC_INLINE T	Min( T x, T y ) { return ( x < y ) ? x : y; }
template<class T> ARC_INLINE T	MaxElement( const T *begin, const T *end) {
T maxVal = *begin; for ( const T* it = begin + 1; it != end; ++it ) {
if ( *it > maxVal ) { maxVal = *it; } } return maxVal; }
template <class T> ARC_INLINE T MinElement( const T array[], int size ) {
if (size == 0) { return T(); } T min_element = array[0]; for (int i = 1; i < size; i++) { if ( array[i] < min_element ) { min_element = array[i]; } return min_element; }

//int intArray[] = {4, 2, 9, 5, 7};
//int minInt = MinElement<int>(intArray, 5);  // returns 2

//double doubleArray[] = {3.14, 2.718, 1.618};
//double minDouble = MinElement<double>(doubleArray, 3);  // returns 1.618

class anMath {
public:

	static void					Init( void );
	static	int					InitSeed( void );			// for a non seed based init
	static	void				InitRand( unsigned long seed ) { mSeed = seed; }// Init the seed to a unique number

	static template<class T> int arcMin( T Val1, T Val2 );
	static template<class T> int arcMax( T Val1, T Val2 );

	static float				RSqrt( float x );			// reciprocal square root, returns huge number when x == 0.0

	static float				InvSqrt( float x );			// inverse square root with 32 bits precision, returns huge number when x == 0.0
	static float				InvSqrt16( float x );		// inverse square root with 16 bits precision, returns huge number when x == 0.0
	static double				InvSqrt64( float x );		// inverse square root with 64 bits precision, returns huge number when x == 0.0

	static float				Sqrt( float x );			// square root with 32 bits precision
	static float				Sqrt16( float x );			// square root with 16 bits precision
	static double				Sqrt64( float x );			// square root with 64 bits precision

	static float				Sin( float a );				// sine with 32 bits precision
	static float				Sin16( float a );			// sine with 16 bits precision, maximum absolute error is 2.3082e-09
	static double				Sin64( float a );			// sine with 64 bits precision

	static float				Cos( float a );				// cosine with 32 bits precision
	static float				Cos16( float a );			// cosine with 16 bits precision, maximum absolute error is 2.3082e-09
	static double				Cos64( float a );			// cosine with 64 bits precision

	static void					SinCos( float a, float &s, float &c );		// sine and cosine with 32 bits precision
	static void					SinCos16( float a, float &s, float &c );	// sine and cosine with 16 bits precision
	static void					SinCos64( float a, double &s, double &c );	// sine and cosine with 64 bits precision

	static float				Tan( float a );				// tangent with 32 bits precision
	static float				Tan16( float a );			// tangent with 16 bits precision, maximum absolute error is 1.8897e-08
	static double				Tan64( float a );			// tangent with 64 bits precision

	static float				ASin( float a );			// anLexer sine with 32 bits precision, input is clamped to [-1, 1] to avoid a silent NaN
	static float				ASin16( float a );			// anLexer sine with 16 bits precision, maximum absolute error is 6.7626e-05
	static double				ASin64( float a );			// anLexer sine with 64 bits precision

	static float				ACos( float a );			// anLexer cosine with 32 bits precision, input is clamped to [-1, 1] to avoid a silent NaN
	static float				ACos16( float a );			// anLexer cosine with 16 bits precision, maximum absolute error is 6.7626e-05
	static double				ACos64( float a );			// anLexer cosine with 64 bits precision

	static float				ATan( float a );			// anLexer tangent with 32 bits precision
	static float				ATan16( float a );			// anLexer tangent with 16 bits precision, maximum absolute error is 1.3593e-08
	static double				ATan64( float a );			// anLexer tangent with 64 bits precision

	static float				ATan( float y, float x );	// anLexer tangent with 32 bits precision
	static float				ATan16( float y, float x );	// anLexer tangent with 16 bits precision, maximum absolute error is 1.3593e-08
	static double				ATan64( float y, float x );	// anLexer tangent with 64 bits precision

	static float				Pow( float x, float y );	// x raised to the power y with 32 bits precision
	static float				Pow16( float x, float y );	// x raised to the power y with 16 bits precision
	static double				Pow64( float x, float y );	// x raised to the power y with 64 bits precision

	static float				Exp( float f );				// e raised to the power f with 32 bits precision
	static float				Exp16( float f );			// e raised to the power f with 16 bits precision
	static double				Exp64( float f );			// e raised to the power f with 64 bits precision

	static float				Log( float f );				// natural logarithm with 32 bits precision
	static float				Log16( float f );			// natural logarithm with 16 bits precision
	static double				Log64( float f );			// natural logarithm with 64 bits precision

	static int					IPow( int x, int y );		// integral x raised to the power y
	static int					ILog2( float f );			// integral base-2 logarithm of the floating point value
	static int					ILog2( int i );				// integral base-2 logarithm of the integer value

	static int					BitsForFloat( float f );	// minumum number of bits required to represent ceil( f )
	static int					BitsForInteger( int i );	// minumum number of bits required to represent i
	static int					MaskForFloatSign( float f );// returns 0x00000000 if x >= 0.0f and returns 0xFFFFFFFF if x <= -0.0f
	static int					MaskForIntegerSign( int i );// returns 0x00000000 if x >= 0 and returns 0xFFFFFFFF if x < 0

	static int					FloorPowerOfTwo( int x );	// round x down to the nearest power of 2
	static int					CeilPowerOfTwo( int x );	// round x up to the nearest power of 2
	static bool					IsPowerOfTwo( int x );		// returns true if x is a power of 2

	static int					BitCount( int x );			// returns the number of 1 bits in x
	static int					BitReverse( int x );		// returns the bit reverse of x
	static bool					BitCheck( const int array[], int bitNum );
	static void					BitSet( int array[], int bitNum );
	static void 				BitClear( int array[], int bitNum );

	static void					arcRandom( void ) { mSeed = 0x89abcdef; }

	static float				FloatRand( float min, float max );// Returns a float min <= x < max (exclusive; will get max - 0.00001; but never max)
	static float				FloatRand();				// Returns a float min <= 0 < 1.0
	static float				FloatRand( const anVec2& v );
	static int					IntRand( int min, int max );// Returns an integer min <= x <= max (ie inclusive)

	static int					IsFinite( float f );

	static int					Abs( int x );				// returns the absolute value of the integer value (for reference only)
	static float				Fabs( float f );			// returns the absolute value of the floating point value
	static float				Floor( float f );			// returns the largest integer that is less than or equal to the given value
	static float				Ceil( float f );			// returns the smallest integer that is greater than or equal to the given value
	static float				Rint( float f );			// returns the nearest integer
	static int					Ftoi( float f );			// float to int conversion
	static int					FtoiFast( float f );		// fast float to int conversion but uses current FPU round mode (default round nearest)
	static unsigned long		Ftol( float f );			// float to long conversion
	static unsigned long		FtolFast( float );			// fast float to long conversion but uses current FPU round mode (default round nearest)

	static signed char			ClampChar( int i );
	static signed short			ClampShort( int i );
	static int					ClampInt( int min, int max, int value );
	static float				ClampFloat( float min, float max, float value );

	static float				AngleNormalize360( float angle );
	static float				AngleNormalize180( float angle );
	static float				AngleDelta( float angle1, float angle2 );

	static int					FloatToBits( float f, int exponentBits, int mantissaBits );
	static float				BitsToFloat( int i, int exponentBits, int mantissaBits );
	static int					FloatHash( const float *array, const int numFloats );

	static const float			PI;							// pi
	static const float			TWO_PI;						// pi * 2
	static const float			HALF_PI;					// pi / 2
	static const float			ONEFOURTH_PI;				// pi / 4
	static const float			E;							// e
	static const float			EXP;						// exponential/exp
	static const float			SQRT_TWO;					// sqrt( 2 )
	static const float			SQRT_THREE;					// sqrt( 3 )
	static const float			SQRT_1OVER2;				// sqrt( 1 / 2 )
	static const float			SQRT_1OVER3;				// sqrt( 1 / 3 )
	static const float			M_DEG2RAD;					// degrees to radians multiplier
	static const float			M_RAD2DEG;					// radians to degrees multiplier
	static const float			M_SEC2MS;					// seconds to milliseconds multiplier
	static const float			M_MS2SEC;					// milliseconds to seconds multiplier
	static const float			INFINITY;					// huge number which should be larger than any valid number used
	static const float			FLT_EPSILON;				// smallest positive number such that 1.0+FLT_EPSILON != 1.0
	static const float			FLOAT_EPSILON;				// smallest positive number such that 1.0+FLT_EPSILON != 1.0
	static const int			INT_MIN;
	static const int			INT_MAX;

	static float				FractionalPart( const float a );
	static float				CalcPow( const float num, const float exp );
	static float				ArbitraryBase( float base, float x );

	static void					ArtesianFromPolar( anVec3 &result, anVec3 view );
	static void					PolarFromArtesian( anVec3 &view, anVec3 artesian );
	static float				BarycentricTriArea( const anVec3 &normal, const anVec3 &a, const anVec3 &b, const anVec3 &c );
	static void					BarycentricEvaluate( anVec2 &result, const anVec3 &point, const anVec3 &normal, const float area, const anVec3 t[3], const anVec2 tc[3] );
	static float				BarycentricTriArea2D( const anVec3 &normal, const anVec3 &a, const anVec3 &b, const anVec3 &c );
	static void					BarycentricEvaluate2D( anVec2 &result, const anVec3 &point, const anVec3 &normal, const float area, const anVec3 t[3], const anVec2 tc[3] );

	static float		 		DifferentialValueLerp( float start, float end, float frac );
	static float				DifferentialValueLerp2( const anVec2& range, float frac );
	static float				MidPointLerp( float start, float mid, float end, float frac );
	static float				NewMidPointLerp( const float startVal, const float midVal, const float endVal, const float frac );

	static float				DBToScale( float db );
	static float				ScaleToDb( float scale );
	static float				TstDBToScale( float db );
	static float				TstScaleToDB( float scale );

	static void 				NormalToLatLong( const vec3_origin &normal, byte bytes[2] );
	static anVec3				NearestBoundaryPoint( const anVec3 &pt, const anBounds &bounds );
	static anVec3 				ProjectPointOntoLine( const anVec3 &point, const anVec3 &line, const anVec3 &lineStartPoint );

	static float 				PointToLineDist( const anVec3 &point, const anVec3 &line, const anVec3 &lineStartPoint );
	//static float 				LineToPointDist( const anVec3 &line, const anVec3 &lineStartPoint );

	// Calculate the average of an array of numbers
	float 						ArrayAvg( const float numbers[], int size );

	// Calculate the average of a vector of numbers
	float						VecAvg( const float &numbers );

	// Calculate the average of two numbers
	float						CalcAvg( float num1, float num2 );

	// Calculate the average of three numbers
	float						CalcAvg( float num1, float num2, float num3 );

private:
	enum {
		LOOKUP_BITS				= 8,
		EXP_POS					= 23,
		EXP_BIAS				= 127,
		LOOKUP_POS				= ( EXP_POS-LOOKUP_BITS ),
		SEED_POS				= ( EXP_POS-8 ),
		SQRT_TABLE_SIZE			= ( 2<<LOOKUP_BITS ),
		LOOKUP_MASK				= ( SQRT_TABLE_SIZE-1 )
	};

	union _flint {
		dword					i;
		float					f;
	};
	static unsigned long		mSeed;
	static dword				iSqrt[SQRT_TABLE_SIZE];
	static bool					initialized;
};

/*
===============
anMath::arcMin
===============
*/
template<class T>anMath::arcMin( T Val1, T Val2 ) {
	return Min( Val1, Val2 );
}

/*
===============
anMath::arcMax
===============
*/
template<class T> anMath::arcMax( T Val1, T Val2 ) {
	return Max( Val1, Val2 );
}

ARC_INLINE float anMath::RSqrt( float x ) {
	long i;
	float y, r;

	y = x * 0.5f;
	i = *reinterpret_cast<long *>( &x );
	i = 0x5f3759df - ( i >> 1 );
	r = *reinterpret_cast<float *>( &i );
	r = r * ( 1.5f - r * r * y );
	return r;
}

ARC_INLINE float anMath::InvSqrt16( float x ) {
	dword a = ( (union _flint*)( &x ) )->i;
	union _flint seed;

	assert( initialized );

	double y = x * 0.5f;
	seed.i = ( ( ( ( 3*EXP_BIAS-1 ) - ( ( a >> EXP_POS ) & 0xFF ) ) >> 1 )<<EXP_POS ) | iSqrt[ ( a >> ( EXP_POS-LOOKUP_BITS ) ) & LOOKUP_MASK];
	double r = seed.f;
	r = r * ( 1.5f - r * r * y );
	return ( float ) r;
}

ARC_INLINE float anMath::InvSqrt( float x ) {
	dword a = ( (union _flint*)( &x ) )->i;
	union _flint seed;

	assert( initialized );

	double y = x * 0.5f;
	seed.i = ( ( ( ( 3*EXP_BIAS-1 ) - ( ( a >> EXP_POS ) & 0xFF ) ) >> 1 )<<EXP_POS ) | iSqrt[( a >> ( EXP_POS-LOOKUP_BITS ) ) & LOOKUP_MASK];
	double r = seed.f;
	r = r * ( 1.5f - r * r * y );
	r = r * ( 1.5f - r * r * y );
	return ( float ) r;
}

ARC_INLINE double anMath::InvSqrt64( float x ) {
	dword a = ( (union _flint*)( &x ) )->i;
	union _flint seed;

	assert( initialized );

	double y = x * 0.5f;
	seed.i = ( ( ( ( 3*EXP_BIAS-1 ) - ( ( a >> EXP_POS ) & 0xFF ) ) >> 1 )<<EXP_POS ) | iSqrt[( a >> ( EXP_POS-LOOKUP_BITS ) ) & LOOKUP_MASK];
	double r = seed.f;
	r = r * ( 1.5f - r * r * y );
	r = r * ( 1.5f - r * r * y );
	r = r * ( 1.5f - r * r * y );
	return r;
}

ARC_INLINE float anMath::Sqrt16( float x ) {
	return x * InvSqrt16( x );
}

ARC_INLINE float anMath::Sqrt( float x ) {
	return x * InvSqrt( x );
}

ARC_INLINE double anMath::Sqrt64( float x ) {
	return x * InvSqrt64( x );
}

ARC_INLINE float anMath::Sin( float a ) {
	return sinf( a );
}

ARC_INLINE float anMath::Sin16( float a ) {
	float s;

	if ( ( a < 0.0f ) || ( a >= TWO_PI ) ) {
		a -= floorf( a / TWO_PI ) * TWO_PI;
	}
#if 1
	if ( a < PI ) {
		if ( a > HALF_PI ) {
			a = PI - a;
		}
	} else {
		if ( a > PI + HALF_PI ) {
			a = a - TWO_PI;
		} else {
			a = PI - a;
		}
	}
#else
	a = PI - a;
	if ( fabs( a ) >= HALF_PI ) {
		a = ( ( a < 0.0f ) ? -PI : PI ) - a;
	}
#endif
	s = a * a;
	return a * ( ( ( ( ( -2.39e-08f * s + 2.7526e-06f ) * s - 1.98409e-04f ) * s + 8.3333315e-03f ) * s - 1.666666664e-01f ) * s + 1.0f );
}

ARC_INLINE double anMath::Sin64( float a ) {
	return sin( a );
}

ARC_INLINE float anMath::Cos( float a ) {
	return cosf( a );
}

ARC_INLINE float anMath::Cos16( float a ) {
	float s, d;

	if ( ( a < 0.0f ) || ( a >= TWO_PI ) ) {
		a -= floorf( a / TWO_PI ) * TWO_PI;
	}
#if 1
	if ( a < PI ) {
		if ( a > HALF_PI ) {
			a = PI - a;
			d = -1.0f;
		} else {
			d = 1.0f;
		}
	} else {
		if ( a > PI + HALF_PI ) {
			a = a - TWO_PI;
			d = 1.0f;
		} else {
			a = PI - a;
			d = -1.0f;
		}
	}
#else
	a = PI - a;
	if ( fabs( a ) >= HALF_PI ) {
		a = ( ( a < 0.0f ) ? -PI : PI ) - a;
		d = 1.0f;
	} else {
		d = -1.0f;
	}
#endif
	s = a * a;
	return d * ( ( ( ( ( -2.605e-07f * s + 2.47609e-05f ) * s - 1.3888397e-03f ) * s + 4.16666418e-02f ) * s - 4.999999963e-01f ) * s + 1.0f );
}

ARC_INLINE double anMath::Cos64( float a ) {
	return cos( a );
}

ARC_INLINE void anMath::SinCos( float a, float &s, float &c ) {
#ifdef _WIN32
	_asm {
		fld		a
		fsincos
		mov		ecx, c
		mov		edx, s
		fstp	dword ptr [ecx]
		fstp	dword ptr [edx]
	}
#else
	s = sinf( a );
	c = cosf( a );
#endif
}

ARC_INLINE void anMath::SinCos16( float a, float &s, float &c ) {
	float t, d;

	if ( ( a < 0.0f ) || ( a >= anMath::TWO_PI ) ) {
		a -= floorf( a / anMath::TWO_PI ) * anMath::TWO_PI;
	}
#if 1
	if ( a < PI ) {
		if ( a > HALF_PI ) {
			a = PI - a;
			d = -1.0f;
		} else {
			d = 1.0f;
		}
	} else {
		if ( a > PI + HALF_PI ) {
			a = a - TWO_PI;
			d = 1.0f;
		} else {
			a = PI - a;
			d = -1.0f;
		}
	}
#else
	a = PI - a;
	if ( fabs( a ) >= HALF_PI ) {
		a = ( ( a < 0.0f ) ? -PI : PI ) - a;
		d = 1.0f;
	} else {
		d = -1.0f;
	}
#endif
	t = a * a;
	s = a * ( ( ( ( ( -2.39e-08f * t + 2.7526e-06f ) * t - 1.98409e-04f ) * t + 8.3333315e-03f ) * t - 1.666666664e-01f ) * t + 1.0f );
	c = d * ( ( ( ( ( -2.605e-07f * t + 2.47609e-05f ) * t - 1.3888397e-03f ) * t + 4.16666418e-02f ) * t - 4.999999963e-01f ) * t + 1.0f );
}

ARC_INLINE void anMath::SinCos64( float a, double &s, double &c ) {
#ifdef _WIN32
	_asm {
		fld		a
		fsincos
		mov		ecx, c
		mov		edx, s
		fstp	qword ptr [ecx]
		fstp	qword ptr [edx]
	}
#else
	s = sin( a );
	c = cos( a );
#endif
}

ARC_INLINE float anMath::Tan( float a ) {
	return tanf( a );
}

ARC_INLINE float anMath::Tan16( float a ) {
	float s;
	bool reciprocal;

	if ( ( a < 0.0f ) || ( a >= PI ) ) {
		a -= floorf( a / PI ) * PI;
	}
#if 1
	if ( a < HALF_PI ) {
		if ( a > ONEFOURTH_PI ) {
			a = HALF_PI - a;
			reciprocal = true;
		} else {
			reciprocal = false;
		}
	} else {
		if ( a > HALF_PI + ONEFOURTH_PI ) {
			a = a - PI;
			reciprocal = false;
		} else {
			a = HALF_PI - a;
			reciprocal = true;
		}
	}
#else
	a = HALF_PI - a;
	if ( fabs( a ) >= ONEFOURTH_PI ) {
		a = ( ( a < 0.0f ) ? -HALF_PI : HALF_PI ) - a;
		reciprocal = false;
	} else {
		reciprocal = true;
	}
#endif
	s = a * a;
	s = a * ( ( ( ( ( ( 9.5168091e-03f * s + 2.900525e-03f ) * s + 2.45650893e-02f ) * s + 5.33740603e-02f ) * s + 1.333923995e-01f ) * s + 3.333314036e-01f ) * s + 1.0f );
	if ( reciprocal ) {
		return 1.0f / s;
	} else {
		return s;
	}
}

ARC_INLINE double anMath::Tan64( float a ) {
	return tan( a );
}

ARC_INLINE float anMath::ASin( float a ) {
	if ( a <= -1.0f ) {
		return -HALF_PI;
	}
	if ( a >= 1.0f ) {
		return HALF_PI;
	}
	return asinf( a );
}

ARC_INLINE float anMath::ASin16( float a ) {
	if ( FLOATSIGNBITSET( a ) ) {
		if ( a <= -1.0f ) {
			return -HALF_PI;
		}
		a = fabs( a );
		return ( ( ( -0.0187293f * a + 0.0742610f ) * a - 0.2121144f ) * a + 1.5707288f ) * sqrt( 1.0f - a ) - HALF_PI;
	} else {
		if ( a >= 1.0f ) {
			return HALF_PI;
		}
		return HALF_PI - ( ( ( -0.0187293f * a + 0.0742610f ) * a - 0.2121144f ) * a + 1.5707288f ) * sqrt( 1.0f - a );
	}
}

ARC_INLINE double anMath::ASin64( float a ) {
	if ( a <= -1.0f ) {
		return -HALF_PI;
	}

	if ( a >= 1.0f ) {
		return HALF_PI;
	}

	return asin( a );
}

ARC_INLINE float anMath::ACos( float a ) {
	//angle = acosf( a );
	if ( a <= -1.0f ) {
		return PI;
	}

	if ( a >= 1.0f ) {
		return 0.0f;
	}

	return acosf( a );
}

ARC_INLINE float anMath::ACos16( float a ) {
	if ( FLOATSIGNBITSET( a ) ) {
		if ( a <= -1.0f ) {
			return PI;
		}
		a = fabs( a );
		return PI - ( ( ( -0.0187293f * a + 0.0742610f ) * a - 0.2121144f ) * a + 1.5707288f ) * sqrt( 1.0f - a );
	} else {
		if ( a >= 1.0f ) {
			return 0.0f;
		}
		return ( ( ( -0.0187293f * a + 0.0742610f ) * a - 0.2121144f ) * a + 1.5707288f ) * sqrt( 1.0f - a );
	}
}

ARC_INLINE double anMath::ACos64( float a ) {
	if ( a <= -1.0f ) {
		return PI;
	}
	if ( a >= 1.0f ) {
		return 0.0f;
	}
	return acos( a );
}

ARC_INLINE float anMath::ATan( float a ) {
	return atanf( a );
}

ARC_INLINE float anMath::ATan16( float a ) {
	if ( fabs( a ) > 1.0f ) {
		a = 1.0f / a;
		float s = a * a;
		s = - ( ( ( ( ( ( ( ( ( 0.0028662257f * s - 0.0161657367f ) * s + 0.0429096138f ) * s - 0.0752896400f )
				* s + 0.1065626393f ) * s - 0.1420889944f ) * s + 0.1999355085f ) * s - 0.3333314528f ) * s ) + 1.0f ) * a;
		if ( FLOATSIGNBITSET( a ) ) {
			return s - HALF_PI;
		} else {
			return s + HALF_PI;
		}
	} else {
		float s = a * a;
		return ( ( ( ( ( ( ( ( ( 0.0028662257f * s - 0.0161657367f ) * s + 0.0429096138f ) * s - 0.0752896400f )
			* s + 0.1065626393f ) * s - 0.1420889944f ) * s + 0.1999355085f ) * s - 0.3333314528f ) * s ) + 1.0f ) * a;
	}
}

ARC_INLINE double anMath::ATan64( float a ) {
	return atan( a );
}

ARC_INLINE float anMath::ATan( float y, float x ) {
	return atan2f( y, x );
}

ARC_INLINE float anMath::ATan16( float y, float x ) {
	float a, s;

	if ( fabs( y ) > fabs( x ) ) {
		a = x / y;
		s = a * a;
		s = - ( ( ( ( ( ( ( ( ( 0.0028662257f * s - 0.0161657367f ) * s + 0.0429096138f ) * s - 0.0752896400f )
				* s + 0.1065626393f ) * s - 0.1420889944f ) * s + 0.1999355085f ) * s - 0.3333314528f ) * s ) + 1.0f ) * a;
		if ( FLOATSIGNBITSET( a ) ) {
			return s - HALF_PI;
		} else {
			return s + HALF_PI;
		}
	} else {
		a = y / x;
		s = a * a;
		return ( ( ( ( ( ( ( ( ( 0.0028662257f * s - 0.0161657367f ) * s + 0.0429096138f ) * s - 0.0752896400f )
			* s + 0.1065626393f ) * s - 0.1420889944f ) * s + 0.1999355085f ) * s - 0.3333314528f ) * s ) + 1.0f ) * a;
	}
}

ARC_INLINE double anMath::ATan64( float y, float x ) {
	return atan2( y, x );
}

ARC_INLINE float anMath::Pow( float x, float y ) {
	return powf( x, y );
}

ARC_INLINE float anMath::Pow16( float x, float y ) {
	return Exp16( y * Log16( x ) );
}

ARC_INLINE double anMath::Pow64( float x, float y ) {
	return pow( x, y );
}

ARC_INLINE float anMath::Exp( float f ) {
	return expf( f );
}

ARC_INLINE float anMath::Exp16( float f ) {
	int i, s, e, m, exponent;
	float x, x2, y, p, q;

	x = f * 1.44269504088896340f;		// multiply with ( 1 / log( 2 ) )
#if 1
	i = *reinterpret_cast<int *>( &x );
	s = ( i >> IEEE_FLT_SIGN_BIT );
	e = ( ( i >> IEEE_FLT_MANTISSA_BITS ) & ( ( 1 << IEEE_FLT_EXPONENT_BITS ) - 1 ) ) - IEEE_FLT_EXPONENT_BIAS;
	m = ( i & ( ( 1 << IEEE_FLT_MANTISSA_BITS ) - 1 ) ) | ( 1 << IEEE_FLT_MANTISSA_BITS );
	i = ( ( m >> ( IEEE_FLT_MANTISSA_BITS - e ) ) & ~( e >> 31 ) ) ^ s;
#else
	i = ( int ) x;
	if ( x < 0.0f ) {
		i--;
	}
#endif
	exponent = ( i + IEEE_FLT_EXPONENT_BIAS ) << IEEE_FLT_MANTISSA_BITS;
	y = *reinterpret_cast<float *>( &exponent );
	x -= ( float ) i;
	if ( x >= 0.5f ) {
		x -= 0.5f;
		y *= 1.4142135623730950488f;	// multiply with sqrt( 2 )
	}
	x2 = x * x;
	p = x * ( 7.2152891511493f + x2 * 0.0576900723731f );
	q = 20.8189237930062f + x2;
	x = y * ( q + p ) / ( q - p );
	return x;
}

ARC_INLINE double anMath::Exp64( float f ) {
	return exp( f );
}

ARC_INLINE float anMath::Log( float f ) {
	return logf( f );
}

ARC_INLINE float anMath::Log16( float f ) {
	int i, exponent;
	float y, y2;

	i = *reinterpret_cast<int *>( &f );
	exponent = ( ( i >> IEEE_FLT_MANTISSA_BITS ) & ( ( 1 << IEEE_FLT_EXPONENT_BITS ) - 1 ) ) - IEEE_FLT_EXPONENT_BIAS;
	i -= ( exponent + 1 ) << IEEE_FLT_MANTISSA_BITS;	// get value in the range [.5, 1>
	y = *reinterpret_cast<float *>(&i);
	y *= 1.4142135623730950488f;						// multiply with sqrt( 2 )
	y = ( y - 1.0f ) / ( y + 1.0f );
	y2 = y * y;
	y = y * ( 2.000000000046727f + y2 * ( 0.666666635059382f + y2 * ( 0.4000059794795f + y2 * ( 0.28525381498f + y2 * 0.2376245609f ) ) ) );
	y += 0.693147180559945f * ( ( float )exponent + 0.5f );
	return y;
}

ARC_INLINE double anMath::Log64( float f ) {
	return log( f );
}

ARC_INLINE int anMath::IPow( int x, int y ) {
	int r; for ( r = x; y > 1; y-- ) { r *= x; } return r;
}

ARC_INLINE int anMath::ILog2( float f ) {
	return ( ( (*reinterpret_cast<int *>( &f ) ) >> IEEE_FLT_MANTISSA_BITS ) & ( ( 1 << IEEE_FLT_EXPONENT_BITS ) - 1 ) ) - IEEE_FLT_EXPONENT_BIAS;
}

ARC_INLINE int anMath::ILog2( int i ) {
	return ILog2( ( float )i );
}

ARC_INLINE int anMath::BitsForFloat( float f ) {
	return ILog2( f ) + 1;
}

ARC_INLINE int anMath::BitsForInteger( int i ) {
	return ILog2( ( float )i ) + 1;
}

ARC_INLINE int anMath::MaskForFloatSign( float f ) {
	return ( (*reinterpret_cast<int *>( &f ) ) >> 31 );
}

ARC_INLINE int anMath::MaskForIntegerSign( int i ) {
	return ( i >> 31 );
}

ARC_INLINE int anMath::FloorPowerOfTwo( int x ) {
	return CeilPowerOfTwo( x ) >> 1;
}

ARC_INLINE int anMath::CeilPowerOfTwo( int x ) {
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x++;
	return x;
}

ARC_INLINE bool anMath::IsPowerOfTwo( int x ) {
	return ( x & ( x - 1 ) ) == 0 && x > 0;
}

ARC_INLINE int anMath::BitCount( int x ) {
	x -= ( ( x >> 1 ) & 0x55555555 );
	x = ( ( ( x >> 2 ) & 0x33333333 ) + ( x & 0x33333333 ) );
	x = ( ( ( x >> 4 ) + x ) & 0x0f0f0f0f );
	x += ( x >> 8 );
	return ( ( x + ( x >> 16 ) ) & 0x0000003f );
}

ARC_INLINE int anMath::BitReverse( int x ) {
	x = ( ( ( x >> 1 ) & 0x55555555 ) | ( ( x & 0x55555555 ) << 1 ) );
	x = ( ( ( x >> 2 ) & 0x33333333 ) | ( ( x & 0x33333333 ) << 2 ) );
	x = ( ( ( x >> 4 ) & 0x0f0f0f0f ) | ( ( x & 0x0f0f0f0f ) << 4 ) );
	x = ( ( ( x >> 8 ) & 0x00ff00ff ) | ( ( x & 0x00ff00ff ) << 8 ) );
	return ( ( x >> 16 ) | ( x << 16 ) );
}

ARC_INLINE int anMath::Abs( int x ) {
   int y = x >> 31;
   return ( ( x ^ y ) - y );
}

ARC_INLINE float anMath::Fabs( float f ) {
	int tmp = *reinterpret_cast<int *>( &f );
	tmp &= 0x7FFFFFFF;
	return *reinterpret_cast<float *>( &tmp );
}

ARC_INLINE float anMath::Floor( float f ) {
	return floorf( f );
}

ARC_INLINE float anMath::Ceil( float f ) {
	return ceilf( f );
}

ARC_INLINE float anMath::Rint( float f ) {
	return floorf( f + 0.5f );
}

ARC_INLINE int anMath::Ftoi( float f ) {
	return ( int ) f;
}

ARC_INLINE int anMath::FtoiFast( float f ) {
#ifdef _WIN32
	int i;
	__asm fld		f
	__asm fistp		i		// use default rouding mode (round nearest)
	return i;
#elif 0						// round chop (C/C++ standard)
	int i, s, e, m, shift;
	i = *reinterpret_cast<int *>( &f );
	s = i >> IEEE_FLT_SIGN_BIT;
	e = ( ( i >> IEEE_FLT_MANTISSA_BITS ) & ( ( 1 << IEEE_FLT_EXPONENT_BITS ) - 1 ) ) - IEEE_FLT_EXPONENT_BIAS;
	m = ( i & ( ( 1 << IEEE_FLT_MANTISSA_BITS ) - 1 ) ) | ( 1 << IEEE_FLT_MANTISSA_BITS );
	shift = e - IEEE_FLT_MANTISSA_BITS;
	return ( ( ( ( m >> -shift ) | ( m << shift ) ) & ~( e >> 31 ) ) ^ s ) - s;
//#elif defined( __i386__ )
#elif 0
	int i = 0;
	__asm__ __volatile__ (
						  "fld %1\n" \
						  "fistp %0\n" \
						  : "=m" ( i ) \
						  : "m" ( f ) );
	return i;
#else
	return ( int ) f;
#endif
}

ARC_INLINE unsigned long anMath::Ftol( float f ) {
	return ( unsigned long ) f;
}

ARC_INLINE unsigned long anMath::FtolFast( float f ) {
#ifdef _WIN32
	// FIXME: this overflows on 31bits still .. same as FtoiFast
	unsigned long i;
	__asm fld		f
	__asm fistp		i		// use default rouding mode (round nearest)
	return i;
#elif 0						// round chop (C/C++ standard)
	int i, s, e, m, shift;
	i = *reinterpret_cast<int *>( &f );
	s = i >> IEEE_FLT_SIGN_BIT;
	e = ( ( i >> IEEE_FLT_MANTISSA_BITS ) & ( ( 1 << IEEE_FLT_EXPONENT_BITS ) - 1 ) ) - IEEE_FLT_EXPONENT_BIAS;
	m = ( i & ( ( 1 << IEEE_FLT_MANTISSA_BITS ) - 1 ) ) | ( 1 << IEEE_FLT_MANTISSA_BITS );
	shift = e - IEEE_FLT_MANTISSA_BITS;
	return ( ( ( ( m >> -shift ) | ( m << shift ) ) & ~( e >> 31 ) ) ^ s ) - s;
//#elif defined( __i386__ )
#elif 0
	// for some reason, on gcc I need to make sure i == 0 before performing a fistp
	int i = 0;
	__asm__ __volatile__ (
						  "fld %1\n" \
						  "fistp %0\n" \
						  : "=m" ( i ) \
						  : "m" ( f ) );
	return i;
#else
	return ( unsigned long ) f;
#endif
}

ARC_INLINE byte anMath::Ftob( float f ) {
#ifdef ID_WIN_X86_SSE
	// If a converted result is negative the value (0 ) is returned and if the
	// converted result is larger than the maximum byte the value (255) is returned.
	byte b;
	__asm movss		xmm0, f
	__asm maxss		xmm0, SSE_FLOAT_ZERO
	__asm minss		xmm0, SSE_FLOAT_255
	__asm cvttss2si	eax, xmm0
	__asm mov		b, al
	return b;
#else
	// If a converted result is clamped to the range [0-255].
	int i;
	i = ( int ) f;
	if ( i < 0 ) {
		return 0;
	} else if ( i > 255 ) {
		return 255;
	}
	return i;
#endif
}

ARC_INLINE signed char anMath::ClampChar( int i ) {
	if ( i < -128 ) {
		return -128;
	}
	if ( i > 127 ) {
		return 127;
	}
	return i;
}

ARC_INLINE signed short anMath::ClampShort( int i ) {
	if ( i < -32768 ) {
		return -32768;
	}
	if ( i > 32767 ) {
		return 32767;
	}
	return i;
}

ARC_INLINE int anMath::ClampInt( int min, int max, int value ) {
	if ( value < min ) {
		return min;
	}
	if ( value > max ) {
		return max;
	}
	return value;
}

ARC_INLINE float anMath::ClampFloat( float min, float max, float value ) {
	if ( value < min ) {
		return min;
	}
	if ( value > max ) {
		return max;
	}
	return value;
}

ARC_INLINE float anMath::AngleNormalize360( float angle ) {
	if ( ( angle >= 360.0f ) || ( angle < 0.0f ) ) {
		angle -= floor( angle / 360.0f ) * 360.0f;
	}
	return angle;
}

ARC_INLINE float anMath::AngleNormalize180( float angle ) {
	angle = AngleNormalize360( angle );
	if ( angle > 180.0f ) {
		angle -= 360.0f;
	}
	return angle;
}

ARC_INLINE float anMath::AngleDelta( float angle1, float angle2 ) {
	return AngleNormalize180( angle1 - angle2 );
}

ARC_INLINE int anMath::FloatHash( const float *array, const int numFloats ) {
	int i, hash = 0;
	const int *ptr;

	ptr = reinterpret_cast<const int *>( array );
	for ( i = 0; i < numFloats; i++ ) {
		hash ^= ptr[i];
	}
	return hash;
}

// Calculate the average of an array of numbers
ARC_INLINE float anMath::CalcAvg( const float numbers[], int size ) {
    float sum = 0.0f;
    for ( int i = 0; i < size; i++ )
    {
        sum += numbers[i];
    }
    return sum / size;
}

// Calculate the average of a vector of numbers
ARC_INLINE float anMath::CalcAvg( const float &numbers ) {
    float sum = 0.0f;
    for ( float number : numbers )
    {
        sum += number;
    }
    return sum / numbers.Size();
}

// Calculate the average of two numbers
ARC_INLINE float anMath::CalcAvg( float num1, float num2 ) {
    return ( num1 + num2 ) / 2;
}

// Calculate the average of three numbers
ARC_INLINE float anMath::CalcAvg( float num1, float num2, float num3 ) {
    return ( num1 + num2 + num3 ) / 3;
}

#endif