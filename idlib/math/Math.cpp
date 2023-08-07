#include "../Lib.h"
#pragma hdrstop

const float	anMath::PI				= 3.14159265358979323846f;
const float	anMath::TWO_PI			= 2.0f * PI;
const float	anMath::HALF_PI			= 0.5f * PI;
const float	anMath::ONEFOURTH_PI	= 0.25f * PI;
const float	anMath::THREEFOURTHS_PI= 0.75f * PI;

const float anMath::E				= 2.71828182845904523536f;
const float	anMath::EXP				= 2.718281828459045f;

const float anMath::SQRT_TWO		= 1.41421356237309504880f;
const float anMath::SQRT_THREE		= 1.73205080756887729352f;

const float	anMath::SQRT_1OVER2		= 0.70710678118654752440f;
const float	anMath::SQRT_1OVER3		= 0.57735026918962576450f;

const float	anMath::M_DEG2RAD		= PI / 180.0f;
const float	anMath::M_RAD2DEG		= 180.0f / PI;

const float	anMath::M_SEC2MS		= 1000.0f;
const float	anMath::M_MS2SEC		= 0.001f;
const float	anMath::INFINITY		= 1e30f;

const float anMath::FLT_EPSILON		= 1.192092896e-07f;
const float anMath::FLOAT_EPSILON	= 1.192092896e-07f;
const int	anMath::INT_MIN			= (-2147483647 - 1 );
const int	anMath::INT_MAX			= 2147483647;

bool		anMath::initialized	= false;
dword		anMath::iSqrt[SQRT_TABLE_SIZE];		// inverse square root lookup table

const float anMath::SSE_FLOAT_ZERO	= 0.0f;
const float anMath::SSE_FLOAT_255	= 255.0f;

/*
===============
anMath::Init
===============
*/
void anMath::Init( void ) {
    union _flint fi, fo;

    for ( int i = 0; i < SQRT_TABLE_SIZE; i++ ) {
        fi.i	 = ( ( EXP_BIAS-1 ) << EXP_POS) | ( i << LOOKUP_POS );
        fo.f	 = ( float )( 1.0 / sqrt( fi.f ) );
        iSqrt[i] = ( ( dword )( ( (fo.i + ( 1<<( SEED_POS-2 ) ) ) >> SEED_POS ) & 0xFF ) )<<SEED_POS;
    }

	iSqrt[SQRT_TABLE_SIZE / 2] = ( ( dword ) ( 0xFF ) )<<( SEED_POS );

	initialized = true;
}

/*
================
anMath::FloatToBits
================
*/
int anMath::FloatToBits( float f, int exponentBits, int mantissaBits ) {
	int i, sign, exponent, mantissa, value;

	assert( exponentBits >= 2 && exponentBits <= 8 );
	assert( mantissaBits >= 2 && mantissaBits <= 23 );

	int maxBits = ( ( ( 1 << ( exponentBits - 1 ) ) - 1 ) << mantissaBits ) | ( ( 1 << mantissaBits ) - 1 );
	int minBits = ( ( ( 1 <<   exponentBits       ) - 2 ) << mantissaBits ) | 1;

	float max = BitsToFloat( maxBits, exponentBits, mantissaBits );
	float min = BitsToFloat( minBits, exponentBits, mantissaBits );

	if ( f >= 0.0f ) {
		if ( f >= max ) {
			return maxBits;
		} else if ( f <= min ) {
			return minBits;
		}
	} else {
		if ( f <= -max ) {
			return ( maxBits | ( 1 << ( exponentBits + mantissaBits ) ) );
		} else if ( f >= -min ) {
			return ( minBits | ( 1 << ( exponentBits + mantissaBits ) ) );
		}
	}

	exponentBits--;
	i = *reinterpret_cast<int *>( &f );
	sign = ( i >> IEEE_FLT_SIGN_BIT ) & 1;
	exponent = ( ( i >> IEEE_FLT_MANTISSA_BITS ) & ( ( 1 << IEEE_FLT_EXPONENT_BITS ) - 1 ) ) - IEEE_FLT_EXPONENT_BIAS;
	mantissa = i & ( ( 1 << IEEE_FLT_MANTISSA_BITS ) - 1 );
	value = sign << ( 1 + exponentBits + mantissaBits );
	value |= ( ( INTSIGNBITSET( exponent ) << exponentBits ) | ( abs( exponent ) & ( ( 1 << exponentBits ) - 1 ) ) ) << mantissaBits;
	value |= mantissa >> ( IEEE_FLT_MANTISSA_BITS - mantissaBits );
	return value;
}

/*
================
anMath::BitsToFloat
================
*/
float anMath::BitsToFloat( int i, int exponentBits, int mantissaBits ) {
	static int exponentSign[2] = { 1, -1 };
	int sign, exponent, mantissa, value;

	assert( exponentBits >= 2 && exponentBits <= 8 );
	assert( mantissaBits >= 2 && mantissaBits <= 23 );

	exponentBits--;
	sign = i >> ( 1 + exponentBits + mantissaBits );
	exponent = ( ( i >> mantissaBits ) & ( ( 1 << exponentBits ) - 1 ) ) * exponentSign[( i >> ( exponentBits + mantissaBits ) ) & 1];
	mantissa = ( i & ( ( 1 << mantissaBits ) - 1 ) ) << ( IEEE_FLT_MANTISSA_BITS - mantissaBits );
	value = sign << IEEE_FLT_SIGN_BIT | ( exponent + IEEE_FLT_EXPONENT_BIAS ) << IEEE_FLT_MANTISSA_BITS | mantissa;
	return *reinterpret_cast<float *>( &value );
}

/*
================
anMath::BitCheck
================
*/
bool anMath::BitCheck( const int array[], int bitNum ) {
	int i;

	i = 0;
	while ( bitNum > 31 ) {
		i++;
		bitNum -= 32;
	}

	return ( ( array[i] & ( 1 << bitNum ) ) != 0 );
}

/*
================
anMath::BitSet
================
*/
void anMath::BitSet( int array[], int bitNum ) {
	int i = 0;

	while ( bitNum > 31 ) {
		i++;
		bitNum -= 32;
	}

	array[i] |= ( 1 << bitNum );
}

/*
================
anMath::BitClear
================
*/
void anMath::BitClear( int array[], int bitNum ) {
	int i = 0;

	while ( bitNum > 31 ) {
		i++;
		bitNum -= 32;
	}

	array[i] &= ~( 1 << bitNum );
}

static int anMath::IsFinite( float f ) {
	floatint_t fi;
	fi.f = f;

	if ( fi.u == 0xFF800000 || fi.u == 0x7F800000 )
		return 0; // -INF or +INF

	fi.u = 0x7F800000 - ( fi.u & 0x7FFFFFFF );
	if ( (int)( fi.u >> 31 ) )
		return 0; // -NAN or +NAN

	return 1;
}

/*
===============
anMath::Pow

This calculates the fractional part of a given float number.
It takes a float number A as input and returns the fractional part of that number.
===============
*/
float anMath::FractionalPart( const float a ) {
	// Subtracting the integer part of A from a gives us the fractional part.
	return a - static_cast<int>( a );//return a - ( ( int )a );
}

/*
===============
anMath::Pow
===============
*/
float anMath::CalcPow( const float num, const float exp ) {
	return pow( num, exp );
}

/*
===============
anMath::ArbitraryBase
===============
*/
float anMath::ArbitraryBase( float base, float x ) {
	// Compute algorithm for arbitrary base by equation:
	// Lx = Lx / log10f(B)
	//    b         c         c			for c
	return log10f( x ) / log10f( base );
}

/*
================
anMath::ArtesianFromPolar
================
*/
void anMath::ArtesianFromPolar( anVec3 &result, anVec3 view ) {
	float	s1, c1, s2, c2;

	anMath::SinCos( view[1], s1, c1 );
	anMath::SinCos( view[2], s2, c2 );

	result[0] = c1 * s2 * view[0];
	result[1] = s1 * s2 * view[0];
	result[2] = c2 * view[0];
}

/*
================
anMath::PolarFromArtesian
================
*/
void anMath::PolarFromArtesian( anVec3 &view, anVec3 artesian ) {
	view[0] = artesian.Length();
	view[1] = anMath::ATan( artesian[1], artesian[0] );

	float length = sqrtf( ( artesian[0] * artesian[0] ) + ( artesian[1] * artesian[1] ) );

	view[2] = anMath::ATan( length, artesian[2] );
}

/*
================
anMath::FloatRand
================
*/
unsigned long anMath::mSeed;
float anMath::FloatRand( float min, float max ) {
	mSeed = ( mSeed * 214013L ) + 2531011;

	// Note: the shift and divide cannot be combined as this breaks the routine
	float result = ( float )( mSeed >> 17 );						// 0 - 32767 range
	float result = ( ( result * ( max - min ) ) * ( 1.0f / 32768.0f ) ) + min;

	return ( result );
}

/*
================
anMath::FloatRand
================
*/
float anMath::FloatRand() {
	return FloatRand( 0.0f, 1.0f );
}

float anMath::FloatRand( const anVec2& v ) {
	return FloatRand( v[0], v[1] );
}

+
/*
================
anMath::IntRand
================
*/
int anMath::IntRand( int min, int max ) {
	max++;

	mSeed = ( mSeed * 214013L ) + 2531011;

	int result = mSeed >> 17;
	result = ( ( result * ( max - min ) ) >> 15 ) + min;

	return ( result );
}

/*
================
anMath::Init
================
*/
int anMath::InitSeed( void ) {
	mSeed *= ( unsigned long )sys->Milliseconds();

	return ( mSeed );
}

/*
================
anMath::BarycentricTriArea
================
*/
float anMath::BarycentricTriArea( const anVec3 &normal, const anVec3 &a, const anVec3 &b, const anVec3 &c ) {
	anVec3 v1 = b - a;
	anVec3 v2 = c - a;
	anVec3 cross = v1.Cross( v2 );
	float area = 0.5f * v1.Dot( cross, normal );

	return area;
}

/*
================
anMath::BarycentricEvaluate
================
*/
void anMath::BarycentricEvaluate( anVec2 &result, const anVec3 &point, const anVec3 &normal, const float area, const anVec3 t[3], const anVec2 tc[3] ) {
	float b1 = anMath::BarycentricTriArea( normal, point, t[1], t[2] ) / area;
	float b2 = anMath::BarycentricTriArea( normal, t[0], point, t[2] ) / area;
	float b3 = anMath::BarycentricTriArea( normal, t[0], t[1], point ) / area;

	result[0] = ( b1 * tc[0][0] ) + ( b2 * tc[1][0] ) + ( b3 * tc[2][0] );
	result[1] = ( b1 * tc[0][1] ) + ( b2 * tc[1][1] ) + ( b3 * tc[2][1] );
}

/*
================
anMath::BarycentricTriArea
================
*/

float BarycentricTriArea2D( const anVec3 &normal, const anVec3 &a, const anVec3 &b, const anVec3 &c ) {
	anVec2 v1 = b - a;
	anVec2 v2 = c - a;
	float cross = v1.x * v2.y - v1.y * v2.x;
	float area = 0.5f * cross;

	return area;
}

/*
================
anMath::BarycentricEvaluate
================
*/
void BarycentricEvaluate2D( anVec2 &result, const anVec2 &point, const anVec2 &normal, const float area, const anVec2 t[3], const anVec2 tc[3] ) {
	float b1 = anMath::BarycentricTriArea2D( normal, point, t[1], t[2] ) / area;
	float b2 = anMath::BarycentricTriArea2D( normal, t[0], point, t[2] ) / area;
	float b3 = anMath::BarycentricTriArea2D( normal, t[0], t[1], point ) / area;

	result.x = ( b1 * tc[0][0] ) + ( b2 * tc[1][0] ) + ( b3 * tc[2][0] );
	result.y = ( b1 * tc[0][1] ) + ( b2 * tc[1][1] ) + ( b3 * tc[2][1] );
}

/*
================
anMath::Lerp
================
*/
float anMath::DifferentialValueLerp( float start, float end, float frac ) {
	if ( frac >= 1.0f ) {
		return end;
	}

	if ( frac <= 0.0f ) {
		return start;
	}

	return start + ( end - start ) * frac;
}

/*
================
anMath::Lerp
================
*/
float anMath::DifferentialValueLerp2( const anVec2& range, float frac ) {
	return Lerp( range[0], range[1], frac );
}

/*
================
anMath::MidPointToLerp
================
*/
float anMath::MidPointToLerp( float start, float mid, float end, float frac ) {
	if ( frac < 0.5f ) {
		return Lerp( start, mid, 2.0f * frac );
	}

	return Lerp( mid, end, 2.0f * ( frac - 0.5f ) );
}

/*
================
anMath::MidPointToLerp
================
*/
float anMath::NewMidPointLerp( const float startVal, const float midVal, const float endVal, const float frac ) {
	if ( frac <= 0.0f ) {
		return startVal;
	}

	if ( frac >= 1.0f ) {
		return endVal;
	}

	return ( frac < 0.5f ) ? Lerp( startVal, midVal, 2.0f * frac ) : Lerp( midVal, endVal, 2.0f * ( frac - 0.5f ) );
}

/*
================
anMath::DBToScale
================
*/
float anMath::DBToScale( float db ) {
	if ( db < -60.0f ) {
		return ( 0.0f );
	} else {
		return ( powf( 2.0f, db * ( 1.0f / 6.0f ) ) );
	}
}

/*
================
anMath::ScaleToDb
================
*/
float anMath::ScaleToDb( float scale ) {
	if ( scale <= 0.0f ) {
		return ( -60.0f );
	} else {
		return ( 6.0f * anMath::Log( scale ) / anMath::Log( 2 ) );
	}
}

/*
================
anMath::TestDBToScale
================
*/
float anMath::TstDBToScale( float dB ) {
	if ( dB == 0.0f ) {
		return 1.0f;
	} else if ( dB <= -60.0f ) {
		return 0.0f;
	}
	return ( Pow( 2, ( dB / 6.0f ) ) );
}

/*
================
anMath::ScaleToDb
================
*/
float anMath::TstScaleToDB( float scale ) {
	if ( scale <= 0.0f ) {
		return -60.0f;
	} else if ( scale == 1.0f ) {
		return 0.0f;
	}
	return 6.0f * anMath::ArbitraryBase( scale ) / anMath::ArbitraryBase( 2.0f, scale );
}

/*
================
anMath::NormalToLatLong
================
*/
void anMath::NormalToLatLong( const vec3_origin &normal, byte bytes[2] ) {
	// check for singularities
	if ( normal[0] == 0 && normal[1] == 0 ) {
		if ( normal[2] > 0 ) {
			bytes[0] = 0;
			bytes[1] = 0;		// lat = 0, long = 0
		} else {
			bytes[0] = 128;
			bytes[1] = 0;		// lat = 0, long = 128
		}
	} else {
		// use a suitable quake style polar coordinate system
		//anVec3 normal2, latLong;

		//normal2[0] = dv->normal[0] * cos( DEG2RAD( normal[2] ) );
		//normal2[1] = dv->normal[0] * sin( DEG2RAD( normal[2] ) );
		//normal2[2] = dv->normal[1];
		int a = RAD2DEG( ATan2( normal[1], normal[0] ) ) * ( 255.0f / 360.0f );
		a &= 0xff;

		int b = RAD2DEG( ACos( normal[2] ) ) * ( 255.0f / 360.0f );
		b &= 0xff;

		bytes[0] = b;	// longitude
		bytes[1] = a;	// lattitude
	}
}

// TODO: place this in bounds class
anVec3	anMath::NearestBoundaryPoint( const anVec3 &pt, const anBounds &bounds ) {
	anVec3 ul = bounds[0];
	anVec3 lr = bounds[1];

	// We are INSIDE looking for closest boundary
	if ( bounds.ContainsPoint( pt ) ) {
		anVec3 nearPt = pt;
		int nearestSideToPt[3];
		float nearestSideDistFromPt[3];
		// Find side nearest to point
		for ( int i = 1; i < 3; i++ ) {
			float ulDist = pt[i] - ul[i];
			float lrDist = lr[i] - pt[i];
			nearestSideToPt[i] = ( ulDist < lrDist ) ? 0 : 1;
			nearestSideDistFromPt[i] = ( ulDist < lrDist ) ? ulDist : lrDist;
        }

		// Now find nearest axis
		int nearestAxis = 0;
		for ( int i = 1; i < 3; i++ ) {
            if ( nearestSideDistFromPt[i] < nearestSideDistFromPt[nearestAxis] ) {
				nearestAxis = i;
			}
		}

        nearPt[nearestAxis] = ( nearestSideToPt[nearestAxis] == 0 ) ? ul[nearestAxis] : lr[nearestAxis];
    } else {
        for ( int i = 0; i < 3; i++ ) {
            nearPt[i] = ( pt[i] < ul[i] ) ? ul[i] : ( ( pt[i] > lr[i] ) ? lr[i] : pt[i] );
        }
    }

	return nearPt;
};

/*
================
anMath::ProjectPointOntoLine
================
*/
anVec3 anMath::ProjectPointOntoLine( const anVec3 &point, const anVec3 &line, const anVec3 &lineStartPoint ) {
	anVec3 lineDir = line;
	lineDir.Normalize();
	float dot = ( point - lineStartPoint ) * lineDir;

	return ( lineDir * dot ) + lineStartPoint;
}

/*
================
anMath::DistFromPointToLine
================
*/
float anMath::DistFromPointToLine( const anVec3 &point, const anVec3 &line, const anVec3 &lineStartPoint ) {
	assert( line.Length() );

	return ( ( point - lineStartPoint ).Cross( line ) ).Length() / line.Length();
}


/*
================
anMat3::RotatryMatrix

TODO: move this to Matrix 3 class
================
*/
void anMath::RotatryMatrix( float phi, int axis, anMat3 &mat ) {
	mat.Identity();

	switch ( axis ) {
	case 0:
		mat[1][0] = 0.0f;
		mat[1][1] = Cos( phi );
		mat[1][2] = Sin( phi );
		mat[2][0] = 0.0f;
		mat[2][1] = -Sin( phi );
		mat[2][2] = Cos( phi );
		break;
	case 1:
		mat[0][0] = Cos( phi );
		mat[0][1] = 0.0f;
		mat[0][2] = Sin( phi );
		mat[2][0] = -Sin( phi );
		mat[2][1] = 0.0f;
		mat[2][2] = Cos( phi );
		break;
	case 2:
		mat[0][0] = Cos( phi );
		mat[0][1] = Sin( phi );
		mat[0][2] = 0.0f;
		mat[1][0] = -Sin( phi );
		mat[1][1] = Cos( phi );
		mat[1][2] = 0.0f;
		break;
	default:
		break;
	}
}