#ifndef __MATH_VECTOR_H__
#define __MATH_VECTOR_H__

/*
===============================================================================

  Vector classes

===============================================================================
*/

#include "Math.h"
#define VECTOR_EPSILON		0.001f

class arcAngles;
class idPolar3;
class arcMat3;

//===============================================================
//
//	arcVec2 - 2D vector
//
//===============================================================

class arcVec2 {
public:
	float			x;
	float			y;

					arcVec2( void );
					explicit arcVec2( const float x, const float y );

	void 			Set( const float x, const float y );
	void			Zero( void );

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	arcVec2		operator-() const;
	float			operator*( const arcVec2 &a ) const;
	arcVec2		oerator*( const float a ) const;
	arcVec2		operator/( const float a ) const;
	arcVec2		operator+( const arcVec2 &a ) const;
	arcVec2		operator-( const arcVec2 &a ) const;
	arcVec2 &		operator+=( const arcVec2 &a );
	arcVec2 &		operator-=( const arcVec2 &a );
	arcVec2 &		operator/=( const arcVec2 &a );
	arcVec2 &		operator/=( const float a );
	arcVec2 &		operator*=( const float a );

	friend arcVec2	operator*( const float a, const arcVec2 b );

	bool			Compare( const arcVec2 &a ) const;							// exact compare, no epsilon
	bool			Compare( const arcVec2 &a, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const arcVec2 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const arcVec2 &a ) const;						// exact compare, no epsilon

	float			Length( void ) const;
	float			LengthFast( void ) const;
	float			LengthSqr( void ) const;
	float			Normalize( void );			// returns length
	float			NormalizeFast( void );		// returns length
	arcVec2 &		Truncate( float length );	// cap length
	void			Clamp( const arcVec2 &min, const arcVec2 &max );
	void			Snap( void );				// snap to closest integer value
	void			SnapInt( void );			// snap towards integer (floor)

	int				GetDimension( void ) const;

	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

	void			Lerp( const arcVec2 &v1, const arcVec2 &v2, const float l );
};

extern arcVec2 vec2_origin;
#define vec2_zero vec2_origin

ARC_INLINE arcVec2::arcVec2( void ) {
}

ARC_INLINE arcVec2::arcVec2( const float x, const float y ) {
	this->x = x;
	this->y = y;
}

ARC_INLINE void arcVec2::Set( const float x, const float y ) {
	this->x = x;
	this->y = y;
}

ARC_INLINE void arcVec2::Zero( void ) {
	x = y = 0.0f;
}

ARC_INLINE bool arcVec2::Compare( const arcVec2 &a ) const {
	return ( ( x == a.x ) && ( y == a.y ) );
}

ARC_INLINE bool arcVec2::Compare( const arcVec2 &a, const float epsilon ) const {
	if ( arcMath::Fabs( x - a.x ) > epsilon ) {
		return false;
	}

	if ( arcMath::Fabs( y - a.y ) > epsilon ) {
		return false;
	}

	return true;
}

ARC_INLINE bool arcVec2::operator==( const arcVec2 &a ) const {
	return Compare( a );
}

ARC_INLINE bool arcVec2::operator!=( const arcVec2 &a ) const {
	return !Compare( a );
}

ARC_INLINE float arcVec2::operator[]( int index ) const {
	return ( &x )[index];
}

ARC_INLINE float& arcVec2::operator[]( int index ) {
	return ( &x )[index];
}

ARC_INLINE float arcVec2::Length( void ) const {
	return ( float )arcMath::Sqrt( x * x + y * y );
}

ARC_INLINE float arcVec2::LengthFast( void ) const {
	float sqrLength;

	sqrLength = x * x + y * y;
	return sqrLength * arcMath::RSqrt( sqrLength );
}

ARC_INLINE float arcVec2::LengthSqr( void ) const {
	return ( x * x + y * y );
}

ARC_INLINE float arcVec2::Normalize( void ) {
	float sqrLength, invLength;

	sqrLength = x * x + y * y;
	invLength = arcMath::InvSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	return invLength * sqrLength;
}

ARC_INLINE float arcVec2::NormalizeFast( void ) {
	float lengthSqr, invLength;

	lengthSqr = x * x + y * y;
	invLength = arcMath::RSqrt( lengthSqr );
	x *= invLength;
	y *= invLength;
	return invLength * lengthSqr;
}

ARC_INLINE arcVec2 &arcVec2::Truncate( float length ) {
	float length2;
	float ilength;

	if ( !length ) {
		Zero();
	} else {
		length2 = LengthSqr();
		if ( length2 > length * length ) {
			ilength = length * arcMath::InvSqrt( length2 );
			x *= ilength;
			y *= ilength;
		}
	}

	return *this;
}

ARC_INLINE void arcVec2::Clamp( const arcVec2 &min, const arcVec2 &max ) {
	if ( x < min.x ) {
		x = min.x;
	} else if ( x > max.x ) {
		x = max.x;
	}
	if ( y < min.y ) {
		y = min.y;
	} else if ( y > max.y ) {
		y = max.y;
	}
}

ARC_INLINE void arcVec2::Snap( void ) {
	x = floor( x + 0.5f );
	y = floor( y + 0.5f );
}

ARC_INLINE void arcVec2::SnapInt( void ) {
	x = float( int( x ) );
	y = float( int( y ) );
}

ARC_INLINE arcVec2 arcVec2::operator-() const {
	return arcVec2( -x, -y );
}

ARC_INLINE arcVec2 arcVec2::operator-( const arcVec2 &a ) const {
	return arcVec2( x - a.x, y - a.y );
}

ARC_INLINE float arcVec2::operator*( const arcVec2 &a ) const {
	return x * a.x + y * a.y;
}

ARC_INLINE arcVec2 arcVec2::operator*( const float a ) const {
	return arcVec2( x * a, y * a );
}

ARC_INLINE arcVec2 arcVec2::operator/( const float a ) const {
	float inva = 1.0f / a;
	return arcVec2( x * inva, y * inva );
}

ARC_INLINE arcVec2 operator*( const float a, const arcVec2 b ) {
	return arcVec2( b.x * a, b.y * a );
}

ARC_INLINE arcVec2 arcVec2::operator+( const arcVec2 &a ) const {
	return arcVec2( x + a.x, y + a.y );
}

ARC_INLINE arcVec2 &arcVec2::operator+=( const arcVec2 &a ) {
	x += a.x;
	y += a.y;

	return *this;
}

ARC_INLINE arcVec2 &arcVec2::operator/=( const arcVec2 &a ) {
	x /= a.x;
	y /= a.y;

	return *this;
}

ARC_INLINE arcVec2 &arcVec2::operator/=( const float a ) {
	float inva = 1.0f / a;
	x *= inva;
	y *= inva;

	return *this;
}

ARC_INLINE arcVec2 &arcVec2::operator-=( const arcVec2 &a ) {
	x -= a.x;
	y -= a.y;

	return *this;
}

ARC_INLINE arcVec2 &arcVec2::operator*=( const float a ) {
	x *= a;
	y *= a;

	return *this;
}

ARC_INLINE int arcVec2::GetDimension( void ) const {
	return 2;
}

ARC_INLINE const float *arcVec2::ToFloatPtr( void ) const {
	return &x;
}

ARC_INLINE float *arcVec2::ToFloatPtr( void ) {
	return &x;
}

//===============================================================
//
//	arcVec3 - 3D vector
//
//===============================================================
#define idVec3 arcVec3

class arcVec3 {
public:
	float			x;
	float			y;
	float			z;

					arcVec3( void );
					explicit arcVec3( const float x, const float y, const float z );

	void 			Set( const float x, const float y, const float z );
	void			Zero( void );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	arcVec3			operator-() const;
	arcVec3 &		operator=( const arcVec3 &a );		// required because of a msvc 6 & 7 bug
	float			operator*( const arcVec3 &a ) const;
	arcVec3			operator*( const float a ) const;
	arcVec3			operator/( const float a ) const;
	arcVec3			operator+( const arcVec3 &a ) const;
	arcVec3			operator-( const arcVec3 &a ) const;
	arcVec3 &		operator+=( const arcVec3 &a );
	arcVec3 &		operator-=( const arcVec3 &a );
	arcVec3 &		operator/=( const arcVec3 &a );
	arcVec3 &		operator/=( const float a );
	arcVec3 &		operator*=( const float a );

	friend arcVec3	operator*( const float a, const arcVec3 b );

	bool			Compare( const arcVec3 &a ) const;							// exact compare, no epsilon
	bool			Compare( const arcVec3 &a, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const arcVec3 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const arcVec3 &a ) const;						// exact compare, no epsilon

	bool			FixDegenerateNormal( void );	// fix degenerate axial cases
	bool			FixDenormals( void );			// change tiny numbers to zero

	arcVec3			Cross( const arcVec3 &a ) const;
	arcVec3 &		Cross( const arcVec3 &a, const arcVec3 &b );
	arcVec3			Dot( const arcVec3 v1, const arcVec3 v2 );
	float			Length( void ) const;
	float			LengthSqr( void ) const;
	float			LengthFast( void ) const;
	float			Normalize( void );				// returns length
	float			NormalizeFast( void );			// returns length
	arcVec3 		VectorNormalize( arcVec3 v );
	arcVec3 &		Truncate( float length );		// cap length
	void			Clamp( const arcVec3 &min, const arcVec3 &max );
	void			Snap( void );					// snap to closest integer value
	void			SnapInt( void );				// snap towards integer (floor)

	int				GetDimension( void ) const;

	void			Rotate( arcVec3 in, arcMat3 matrix[3], arcVec3 out );
	float			ToYaw( void ) const;
	float			ToPitch( void ) const;
	arcAngles		ToAngles( void ) const;
	idPolar3		ToPolar( void ) const;
	arcMat3			ToMat3( void ) const;		// vector should be normalized
	const arcVec2 &	ToVec2( void ) const;
	arcVec2 &		ToVec2( void );
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

	void			NormalVectors( arcVec3 &left, arcVec3 &down ) const;	// vector should be normalized
	void			OrthogonalBasis( arcVec3 &left, arcVec3 &up ) const;

	void			ProjectOntoPlane( const arcVec3 &normal, const float overBounce = 1.0f );
	bool			ProjectAlongPlane( const arcVec3 &normal, const float epsilon, const float overBounce = 1.0f );
	void			ProjectSelfOntoSphere( const float radius );

	void			Lerp( const arcVec3 &v1, const arcVec3 &v2, const float l );
	void			SLerp( const arcVec3 &v1, const arcVec3 &v2, const float l );
	float			LerpAngle( const float from, const float to, const float frac );
	void			Inverse( const arcVec3 v );
	void 			MA( const arcVec3 a, float scale, const arcVec3 b, arcVec3 c );
	void			Subtract( const arcVec3 a, const arcVec3 b, arcVec3 out );
	void			Add( const arcVec3 a, const arcVec3 b, arcVec3 out );
	void			Copy( const arcVec3 in, arcVec3 out );
	void			Scale( const arcVec3 in, const arcVec3 scale, arcVec3 out );

	//bool  			ProjectAlongPlane( const arcVec3 &normal, const float epsilon, const float overBounce ) const;
	//void			ProjectOntoPlane( const arcVec3 &normal, const float overBounce );

	arcVec2			Octahedron( void );

	arcVec3			OctahedronDecode( const arcVec2 &oct );
	arcVec3			Octahedron_Tangent( const float sign ) const;


	arcVec3			Octahedron_TangentDecode( const arcVec2 &oct, float *sign );

 	bool			IsFinite() const;
};

#define arcVec3_x Vector( -1, -1, -1 )
#define arcVec3_y Vector( -2, -1, -1 )
#define arcVec3_z Vector( -3, -1, -1 )

#define VectorSet( v, x, y, z ) ( v.x = ( x ), v.y = ( y ), v.z = ( z ) )
#define VectorSet( v, x, y, z ) ( v.x = ( ( x ) < ( y )? x : y ), v.y = ( ( y ) < ( z )? y : z ), v.z = ( ( z ) < ( x )? z : x ) )
#define VectorZero( v ) ( v.x = v.y = v.z = 0.0f )

#define Vector4Set( v, x, y, z, w ) ( v.x = ( x ), v.y = ( y ), v.z = ( z ), v.w = ( w ) )
#define Vector4Zero( v ) ( v.x = v.y = v.z = v.w = 0.0f )

#define Vector4DSet( v, x, y, z, w ) ( v.x = ( x ), v.y = ( y ), v.z = ( z ), v.w = ( w ) )

#define Vector4DAdd( v1, v2, v3 ) ( v1.x += v2.x, v1.y += v2.y, v1.z += v2.z, v1.w += v2.w )
#define Vector4DSubtract( v1, v2, v3 ) ( v1.x -= v2.x, v1.y -= v2.y, v1.z -= v2.z, v1.w -= v2.w )
#define Vector4DMultiply( v1, v2, v3 ) ( v1.x *= v2.x, v1.y *= v2.y, v1.z *= v2.z, v1.w *= v2.w )
#define Vector4DDivide( v1, v2, v3 ) ( v1.x /= v2.x, v1.y /= v2.y, v1.z /= v2.z, v1.w /= v2.w )
#define Vector4DIsZero( v ) ( ( v ).x > -0.0001f && ( v ).x < 0.0001f && ( v ).y > -0.0001f && ( v ).y < 0.0001f && ( v ).z > -0.0001f && ( v ).z < 0.0001f && ( v ).w > -0.0001f && ( v ).w < 0.0001f )

extern arcVec3 vec3_origin;
#define vec3_zero vec3_origin

ARC_INLINE arcVec3::arcVec3( void ) {
}

ARC_INLINE arcVec3::arcVec3( const float x, const float y, const float z ) {
	this->x = x;
	this->y = y;
	this->z = z;
}

ARC_INLINE float arcVec3::operator[]( const int index ) const {
	return ( &x )[index];
}

ARC_INLINE float &arcVec3::operator[]( const int index ) {
	return ( &x )[index];
}

ARC_INLINE void arcVec3::Set( const float x, const float y, const float z ) {
	this->x = x; this->y = y; this->z = z;
}

ARC_INLINE void arcVec3::Zero( void ) {
	x = y = z = 0.0f;
}

ARC_INLINE arcVec3 arcVec3::operator-() const {
	return arcVec3( -x, -y, -z );
}

ARC_INLINE arcVec3 &arcVec3::operator=( const arcVec3 &a ) {
	x = a.x; y = a.y; z = a.z;
	return *this;
}

ARC_INLINE arcVec3 arcVec3::operator-( const arcVec3 &a ) const {
	return arcVec3( x - a.x, y - a.y, z - a.z );
}

ARC_INLINE float arcVec3::operator*( const arcVec3 &a ) const {
	return x * a.x + y * a.y + z * a.z;
}

ARC_INLINE arcVec3 arcVec3::operator*( const float a ) const {
	return arcVec3( x * a, y * a, z * a );
}

ARC_INLINE arcVec3 arcVec3::operator/( const float a ) const {
	float inva = 1.0f / a;
	return arcVec3( x * inva, y * inva, z * inva );
}

ARC_INLINE arcVec3 operator*( const float a, const arcVec3 b ) {
	return arcVec3( b.x * a, b.y * a, b.z * a );
}

ARC_INLINE arcVec3 arcVec3::operator+( const arcVec3 &a ) const {
	return arcVec3( x + a.x, y + a.y, z + a.z );
}

ARC_INLINE arcVec3 &arcVec3::operator+=( const arcVec3 &a ) {
	x += a.x; y += a.y; z += a.z;
	return *this;
}

ARC_INLINE arcVec3 &arcVec3::operator/=( const arcVec3 &a ) {
	x /= a.x; y /= a.y; z /= a.z;
	return *this;
}

ARC_INLINE arcVec3 &arcVec3::operator/=( const float a ) {
	float inva = 1.0f / a;
	x *= inva; y *= inva; z *= inva;
	return *this;
}

ARC_INLINE arcVec3 &arcVec3::operator-=( const arcVec3 &a ) {
	x -= a.x; y -= a.y; z -= a.z;
	return *this;
}

ARC_INLINE arcVec3 &arcVec3::operator*=( const float a ) {
	x *= a; y *= a; z *= a;
	return *this;
}

ARC_INLINE bool arcVec3::Compare( const arcVec3 &a ) const {
	return ( ( x == a.x ) && ( y == a.y ) && ( z == a.z ) );
}

ARC_INLINE bool arcVec3::Compare( const arcVec3 &a, const float epsilon ) const {
	if ( arcMath::Fabs( x - a.x ) > epsilon ) {
		return false;
	}

	if ( arcMath::Fabs( y - a.y ) > epsilon ) {
		return false;
	}

	if ( arcMath::Fabs( z - a.z ) > epsilon ) {
		return false;
	}

	return true;
}

ARC_INLINE bool arcVec3::operator==( const arcVec3 &a ) const {
	return Compare( a );
}

ARC_INLINE bool arcVec3::operator!=( const arcVec3 &a ) const {
	return !Compare( a );
}

ARC_INLINE float arcVec3::NormalizeFast( void ) {
	float sqrLength = x * x + y * y + z * z;
	float invLength = arcMath::RSqrt( sqrLength );
	x *= invLength; y *= invLength; z *= invLength;
	return invLength * sqrLength;
}

ARC_INLINE bool arcVec3::FixDegenerateNormal( void ) {
	if ( x == 0.0f ) {
		if ( y == 0.0f ) {
			if ( z > 0.0f ) {
				if ( z != 1.0f ) {
					z = 1.0f;
					return true;
				}
			} else {
				if ( z != -1.0f ) {
					z = -1.0f;
					return true;
				}
			}
			return false;
		} else if ( z == 0.0f ) {
			if ( y > 0.0f ) {
				if ( y != 1.0f ) {
					y = 1.0f;
					return true;
				}
			} else {
				if ( y != -1.0f ) {
					y = -1.0f;
					return true;
				}
			}
			return false;
		}
	} else if ( y == 0.0f ) {
		if ( z == 0.0f ) {
			if ( x > 0.0f ) {
				if ( x != 1.0f ) {
					x = 1.0f;
					return true;
				}
			} else {
				if ( x != -1.0f ) {
					x = -1.0f;
					return true;
				}
			}
			return false;
		}
	}
	if ( arcMath::Fabs( x ) == 1.0f ) {
		if ( y != 0.0f || z != 0.0f ) {
			y = z = 0.0f;
			return true;
		}
		return false;
	} else if ( arcMath::Fabs( y ) == 1.0f ) {
		if ( x != 0.0f || z != 0.0f ) {
			x = z = 0.0f;
			return true;
		}
		return false;
	} else if ( arcMath::Fabs( z ) == 1.0f ) {
		if ( x != 0.0f || y != 0.0f ) {
			x = y = 0.0f;
			return true;
		}
		return false;
	}
	return false;
}

ARC_INLINE bool arcVec3::FixDenormals( void ) {
	bool denormal = false;
	if ( fabs( x ) < 1e-30f ) {
		x = 0.0f;
		denormal = true;
	}
	if ( fabs( y ) < 1e-30f ) {
		y = 0.0f;
		denormal = true;
	}
	if ( fabs( z ) < 1e-30f ) {
		z = 0.0f;
		denormal = true;
	}
	return denormal;
}

ARC_INLINE arcVec3 arcVec3::Cross( const arcVec3 &a ) const {
	return arcVec3( y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x );
}

ARC_INLINE arcVec3 &arcVec3::Cross( const arcVec3 &a, const arcVec3 &b ) {
	x = a.y * b.z - a.z * b.y;
	y = a.z * b.x - a.x * b.z;
	z = a.x * b.y - a.y * b.x;
	return *this;
}

ARC_INLINE arcVec3 arcVec3::Dot( const arcVec3 v1, const arcVec3 v2 ) {
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

ARC_INLINE float arcVec3::Length( void ) const {
	return ( float )arcMath::Sqrt( x * x + y * y + z * z );
}

ARC_INLINE float arcVec3::LengthSqr( void ) const {
	return ( x * x + y * y + z * z );
}

ARC_INLINE float arcVec3::LengthFast( void ) const {
	float sqrLength;
	sqrLength = x * x + y * y + z * z;
	return sqrLength * arcMath::RSqrt( sqrLength );
}

ARC_INLINE float arcVec3::Normalize( void ) {
	float sqrLength = x * x + y * y + z * z;
	float invLength = arcMath::InvSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	z *= invLength;
	return invLength * sqrLength;
}

ARC_INLINE arcVec3 &arcVec3::Truncate( float length ) {
	if ( !length ) {
		Zero();
	} else {
		float length2 = LengthSqr();
		if ( length2 > length * length ) {
			float ilength = length * arcMath::InvSqrt( length2 );
			x *= ilength;
			y *= ilength;
			z *= ilength;
		}
	}

	return *this;
}

ARC_INLINE void arcVec3::Clamp( const arcVec3 &min, const arcVec3 &max ) {
	if ( x < min.x ) {
		x = min.x;
	} else if ( x > max.x ) {
		x = max.x;
	}
	if ( y < min.y ) {
		y = min.y;
	} else if ( y > max.y ) {
		y = max.y;
	}
	if ( z < min.z ) {
		z = min.z;
	} else if ( z > max.z ) {
		z = max.z;
	}
}

ARC_INLINE void arcVec3::Snap( void ) {
	x = floor( x + 0.5f );
	y = floor( y + 0.5f );
	z = floor( z + 0.5f );
}

ARC_INLINE void arcVec3::SnapInt( void ) {
	x = float( int( x ) );
	y = float( int( y ) );
	z = float( int( z ) );
}

ARC_INLINE int arcVec3::GetDimension( void ) const {
	return 3;
}

ARC_INLINE const arcVec2 &arcVec3::ToVec2( void ) const {
	return *reinterpret_cast<const arcVec2 *>( this );
}

ARC_INLINE arcVec2 &arcVec3::ToVec2( void ) {
	return *reinterpret_cast<arcVec2 *>( this );
}

ARC_INLINE const float *arcVec3::ToFloatPtr( void ) const {
	return &x;
}

ARC_INLINE float *arcVec3::ToFloatPtr( void ) {
	return &x;
}

ARC_INLINE void arcVec3::NormalVectors( arcVec3 &left, arcVec3 &down ) const {
	float d = x * x + y * y;
	if ( !d ) {
		left[0] = 1;
		left[1] = 0;
		left[2] = 0;
	} else {
		d = arcMath::InvSqrt( d );
		left[0] = -y * d;
		left[1] = x * d;
		left[2] = 0;
	}
	down = left.Cross( *this );
}

ARC_INLINE void arcVec3::OrthogonalBasis( arcVec3 &left, arcVec3 &up ) const {
	if ( arcMath::Fabs( z ) > 0.7f ) {
		float l = y * y + z * z;
		float s = arcMath::InvSqrt( l );
		up[0] = 0;
		up[1] = z * s;
		up[2] = -y * s;
		left[0] = l * s;
		left[1] = -x * up[2];
		left[2] = x * up[1];
	} else {
		float l = x * x + y * y;
		float s = arcMath::InvSqrt( l );
		left[0] = -y * s;
		left[1] = x * s;
		left[2] = 0;
		up[0] = -z * left[1];
		up[1] = z * left[0];
		up[2] = l * s;
	}
}

ARC_INLINE void arcVec3::ProjectOntoPlane( const arcVec3 &normal, const float overBounce ) {
	float backoff = *this * normal;

	if ( overBounce != 1.0 ) {
		if ( backoff < 0 ) {
			backoff *= overBounce;
		} else {
			backoff /= overBounce;
		}
	}

	*this -= backoff * normal;
}

ARC_INLINE bool arcVec3::ProjectAlongPlane( const arcVec3 &normal, const float epsilon, const float overBounce ) const {
	arcVec3 cross = this->Cross( normal ).Cross( (* this) );
	// normalize so a fixed epsilon can be used
	cross.Normalize();
	float len = normal * cross;
	if ( arcMath::Fabs( len ) < epsilon ) {
		return false;
	}
	cross *= overBounce * ( normal * (*this) ) / len;
	(*this) -= cross;
	return true;
}

ARC_INLINE arcVec3 arcVec3::OctahedronDecode( const arcVec2 &oct ) {
	arcVec2 f( oct.x * 2.0f - 1.0f, oct.y * 2.0f - 1.0f );
	arcVec3 n( f.x, f.y, 1.0f - arcMath::Abs( f.x ) - arcMath::Abs( f.y ) );
	float t = Clamp( -n.z, 0.0f, 1.0f );
	n.x += n.x >= 0 ? -t : t;
	n.y += n.y >= 0 ? -t : t;
	return n.normalized();
}

ARC_INLINE arcVec2 arcVec3::Octahedron_Tangent( const float sign ) const {
	const float bias = 1.0f / 32767.0f;
	arcVec2 res = this->Octahedron();
	res.y = Max( res.y, bias );
	res.y = res.y * 0.5f + 0.5f;
	res.y = sign >= 0.0f ? res.y : 1 - res.y;
	return res;
}

ARC_INLINE arcVec3 arcVec3::Octahedron_TangentDecode( const arcVec2 &oct, float *sign ) {
	arcVec2 compressed = oct;
	compressed.y = compressed.y * 2 - 1;
	*sign = compressed.y >= 0.0f ? 1.0f : -1.0f;
	compressed.y = arcMath::abs( compressed.y );
	arcVec3 res = arcVec3::OctahedronDecode( compressed );
	return res;
}


ARC_INLINE arcVec2 arcVec3::Octahedron( void ) {
	arcVec3 n = *this;
	n /= arcMath::Abs( n.x ) + arcMath::Abs( n.y ) + arcMath::Abs( n.z );
	if ( n.z >= 0.0f ) {
		arcVec2 o.x = n.x;
		arcVec2 o.y = n.y;
	} else {
		arcVec2 o.x = ( 1.0f - arcMath::Abs( n.y ) ) * ( n.x >= 0.0f ? 1.0f : -1.0f );
		arcVec2 o.y = ( 1.0f - arcMath::Abs( n.x ) ) * ( n.y >= 0.0f ? 1.0f : -1.0f );
	}
	arcVec2 o.x = o.x * 0.5f + 0.5f;
	arcVec2 o.y = o.y * 0.5f + 0.5f;
	return o;
}

ARC_INLINE bool arcVec3::IsFinite() const {
	return arcMath::IsFinite( x ) && arcMath::IsFinite( y ) && arcMath::IsFinite( z );
}

//===============================================================
//
//	arcVec4 - 4D vector
//
//===============================================================
#define idVec4 arcVec4

class arcVec4 {
public:
	float			x;
	float			y;
	float			z;
	float			w;

					arcVec4( void );
					explicit arcVec4( const float x, const float y, const float z, const float w );

	void 			Set( const float x, const float y, const float z, const float w );
	void			Zero( void );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	arcVec4			operator-() const;
	float			operator*( const arcVec4 &a ) const;
	arcVec4			operator*( const float a ) const;
	arcVec4			operator/( const float a ) const;
	arcVec4			operator+( const arcVec4 &a ) const;
	arcVec4			operator-( const arcVec4 &a ) const;
	arcVec4 &		operator+=( const arcVec4 &a );
	arcVec4 &		operator-=( const arcVec4 &a );
	arcVec4 &		operator/=( const arcVec4 &a );
	arcVec4 &		operator/=( const float a );
	arcVec4 &		operator*=( const float a );

	friend arcVec4	operator*( const float a, const arcVec4 b );

	bool			Compare( const arcVec4 &a ) const;							// exact compare, no epsilon
	bool			Compare( const arcVec4 &a, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const arcVec4 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const arcVec4 &a ) const;						// exact compare, no epsilon

	float			Length( void ) const;
	float			LengthSqr( void ) const;
	float			Normalize( void );			// returns length
	float			NormalizeFast( void );		// returns length

	int				GetDimension( void ) const;

	const arcVec2 &	ToVec2( void ) const;
	arcVec2 &		ToVec2( void );
	const arcVec3 &	ToVec3( void ) const;
	arcVec3 &		ToVec3( void );
	arcVec3 &		4ToVec3( const arcVec4 & v );
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

	void			Lerp( const arcVec4 &v1, const arcVec4 &v2, const float l );
	void			Scale( const arcVec4 in, const arcVec4 scale, arcVec4 out );
};

extern arcVec4 vec4_origin;
#define vec4_zero vec4_origin

ARC_INLINE arcVec4::arcVec4( void ) {
}

ARC_INLINE arcVec4::arcVec4( const float x, const float y, const float z, const float w ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

ARC_INLINE void arcVec4::Set( const float x, const float y, const float z, const float w ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

ARC_INLINE void arcVec4::Zero( void ) {
	x = y = z = w = 0.0f;
}

ARC_INLINE float arcVec4::operator[]( int index ) const {
	return ( &x )[index];
}

ARC_INLINE float& arcVec4::operator[]( int index ) {
	return ( &x )[index];
}

ARC_INLINE arcVec4 arcVec4::operator-() const {
	return arcVec4( -x, -y, -z, -w );
}

ARC_INLINE arcVec4 arcVec4::operator-( const arcVec4 &a ) const {
	return arcVec4( x - a.x, y - a.y, z - a.z, w - a.w );
}

ARC_INLINE float arcVec4::operator*( const arcVec4 &a ) const {
	return x * a.x + y * a.y + z * a.z + w * a.w;
}

ARC_INLINE arcVec4 arcVec4::operator*( const float a ) const {
	return arcVec4( x * a, y * a, z * a, w * a );
}

ARC_INLINE arcVec4 arcVec4::operator/( const float a ) const {
	float inva = 1.0f / a;
	return arcVec4( x * inva, y * inva, z * inva, w * inva );
}

ARC_INLINE arcVec4 operator*( const float a, const arcVec4 b ) {
	return arcVec4( b.x * a, b.y * a, b.z * a, b.w * a );
}

ARC_INLINE arcVec4 arcVec4::operator+( const arcVec4 &a ) const {
	return arcVec4( x + a.x, y + a.y, z + a.z, w + a.w );
}

ARC_INLINE arcVec4 &arcVec4::operator+=( const arcVec4 &a ) {
	x += a.x;
	y += a.y;
	z += a.z;
	w += a.w;

	return *this;
}

ARC_INLINE arcVec4 &arcVec4::operator/=( const arcVec4 &a ) {
	x /= a.x;
	y /= a.y;
	z /= a.z;
	w /= a.w;

	return *this;
}

ARC_INLINE arcVec4 &arcVec4::operator/=( const float a ) {
	float inva = 1.0f / a;
	x *= inva;
	y *= inva;
	z *= inva;
	w *= inva;

	return *this;
}

ARC_INLINE arcVec4 &arcVec4::operator-=( const arcVec4 &a ) {
	x -= a.x;
	y -= a.y;
	z -= a.z;
	w -= a.w;

	return *this;
}

ARC_INLINE arcVec4 &arcVec4::operator*=( const float a ) {
	x *= a;
	y *= a;
	z *= a;
	w *= a;

	return *this;
}

ARC_INLINE bool arcVec4::Compare( const arcVec4 &a ) const {
	return ( ( x == a.x ) && ( y == a.y ) && ( z == a.z ) && w == a.w );
}

ARC_INLINE bool arcVec4::Compare( const arcVec4 &a, const float epsilon ) const {
	if ( arcMath::Fabs( x - a.x ) > epsilon ) {
		return false;
	}

	if ( arcMath::Fabs( y - a.y ) > epsilon ) {
		return false;
	}

	if ( arcMath::Fabs( z - a.z ) > epsilon ) {
		return false;
	}

	if ( arcMath::Fabs( w - a.w ) > epsilon ) {
		return false;
	}

	return true;
}

ARC_INLINE bool arcVec4::operator==( const arcVec4 &a ) const {
	return Compare( a );
}

ARC_INLINE bool arcVec4::operator!=( const arcVec4 &a ) const {
	return !Compare( a );
}

ARC_INLINE float arcVec4::Length( void ) const {
	return ( float )arcMath::Sqrt( x * x + y * y + z * z + w * w );
}

ARC_INLINE float arcVec4::LengthSqr( void ) const {
	return ( x * x + y * y + z * z + w * w );
}

ARC_INLINE float arcVec4::Normalize( void ) {
	float sqrLength, invLength;

	sqrLength = x * x + y * y + z * z + w * w;
	invLength = arcMath::InvSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	z *= invLength;
	w *= invLength;
	return invLength * sqrLength;
}

ARC_INLINE float arcVec4::NormalizeFast( void ) {
	float sqrLength, invLength;

	sqrLength = x * x + y * y + z * z + w * w;
	invLength = arcMath::RSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	z *= invLength;
	w *= invLength;
	return invLength * sqrLength;
}

ARC_INLINE int arcVec4::GetDimension( void ) const {
	return 4;
}

ARC_INLINE const arcVec2 &arcVec4::ToVec2( void ) const {
	return *reinterpret_cast<const arcVec2 *>( this );
}

ARC_INLINE arcVec2 &arcVec4::ToVec2( void ) {
	return *reinterpret_cast<arcVec2 *>( this );
}

ARC_INLINE const arcVec3 &arcVec4::ToVec3( void ) const {
	return *reinterpret_cast<const arcVec3 *>( this );
}

ARC_INLINE arcVec3 &arcVec4::ToVec3( void ) {
	return *reinterpret_cast<arcVec3 *>( this );
}

ARC_INLINE arcVec3 &arcVec4::4ToVec3( const arcVec4 & v ) {
	return arcVec3( v.x / v.w, v.y / v.w, v.z / v.w );
}

ARC_INLINE const float *arcVec4::ToFloatPtr( void ) const {
	return &x;
}

ARC_INLINE float *arcVec4::ToFloatPtr( void ) {
	return &x;
}


//===============================================================
//
//	arcVec5 - 5D vector
//
//===============================================================

class arcVec5 {
public:
	float			x;
	float			y;
	float			z;
	float			s;
	float			t;

					arcVec5( void );
					explicit arcVec5( const arcVec3 ( &xyz ), const arcVec2 &st );
					explicit arcVec5( const float x, const float y, const float z, const float s, const float t );

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	arcVec5 &		operator=( const arcVec3 &a );

	int				GetDimension( void ) const;

	const arcVec3 &	ToVec3( void ) const;
	arcVec3 &		ToVec3( void );
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

	void			Lerp( const arcVec5 &v1, const arcVec5 &v2, const float l );
};

extern arcVec5 vec5_origin;
#define vec5_zero vec5_origin

ARC_INLINE arcVec5::arcVec5( void ) {
}

ARC_INLINE arcVec5::arcVec5( const arcVec3 ( &xyz ), const arcVec2 &st ) {
	x = xyz.x;
	y = xyz.y;
	z = xyz.z;
	s = st[0];
	t = st[1];
}

ARC_INLINE arcVec5::arcVec5( const float x, const float y, const float z, const float s, const float t ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->s = s;
	this->t = t;
}

ARC_INLINE float arcVec5::operator[]( int index ) const {
	return ( &x )[index];
}

ARC_INLINE float& arcVec5::operator[]( int index ) {
	return ( &x )[index];
}

ARC_INLINE arcVec5 &arcVec5::operator=( const arcVec3 &a ) {
	x = a.x;
	y = a.y;
	z = a.z;
	s = t = 0;
	return *this;
}

ARC_INLINE int arcVec5::GetDimension( void ) const {
	return 5;
}

ARC_INLINE const arcVec3 &arcVec5::ToVec3( void ) const {
	return *reinterpret_cast<const arcVec3 *>( this );
}

ARC_INLINE arcVec3 &arcVec5::ToVec3( void ) {
	return *reinterpret_cast<arcVec3 *>( this );
}

ARC_INLINE const float *arcVec5::ToFloatPtr( void ) const {
	return &x;
}

ARC_INLINE float *arcVec5::ToFloatPtr( void ) {
	return &x;
}


//===============================================================
//
//	arcVec6 - 6D vector
//
//===============================================================

class arcVec6 {
public:
					arcVec6( void );
					explicit arcVec6( const float *a );
					explicit arcVec6( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 );

	void 			Set( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 );
	void			Zero( void );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	arcVec6			operator-() const;
	arcVec6			operator*( const float a ) const;
	arcVec6			operator/( const float a ) const;
	float			operator*( const arcVec6 &a ) const;
	arcVec6			operator-( const arcVec6 &a ) const;
	arcVec6			operator+( const arcVec6 &a ) const;
	arcVec6 &		operator*=( const float a );
	arcVec6 &		operator/=( const float a );
	arcVec6 &		operator+=( const arcVec6 &a );
	arcVec6 &		operator-=( const arcVec6 &a );

	friend arcVec6	operator*( const float a, const arcVec6 b );

	bool			Compare( const arcVec6 &a ) const;							// exact compare, no epsilon
	bool			Compare( const arcVec6 &a, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const arcVec6 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const arcVec6 &a ) const;						// exact compare, no epsilon

	float			Length( void ) const;
	float			LengthSqr( void ) const;
	float			Normalize( void );			// returns length
	float			NormalizeFast( void );		// returns length

	int				GetDimension( void ) const;

	const arcVec3 &	SubVec3( int index ) const;
	arcVec3 &		SubVec3( int index );
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

private:
	float			p[6];
};

extern arcVec6 vec6_origin;
#define vec6_zero vec6_origin
extern arcVec6 vec6_infinity;

ARC_INLINE arcVec6::arcVec6( void ) {
}

ARC_INLINE arcVec6::arcVec6( const float *a ) {
	memcpy( p, a, 6 * sizeof( float ) );
}

ARC_INLINE arcVec6::arcVec6( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 ) {
	p[0] = a1;
	p[1] = a2;
	p[2] = a3;
	p[3] = a4;
	p[4] = a5;
	p[5] = a6;
}

ARC_INLINE arcVec6 arcVec6::operator-() const {
	return arcVec6( -p[0], -p[1], -p[2], -p[3], -p[4], -p[5] );
}

ARC_INLINE float arcVec6::operator[]( const int index ) const {
	return p[index];
}

ARC_INLINE float &arcVec6::operator[]( const int index ) {
	return p[index];
}

ARC_INLINE arcVec6 arcVec6::operator*( const float a ) const {
	return arcVec6( p[0]*a, p[1]*a, p[2]*a, p[3]*a, p[4]*a, p[5]*a );
}

ARC_INLINE float arcVec6::operator*( const arcVec6 &a ) const {
	return p[0] * a[0] + p[1] * a[1] + p[2] * a[2] + p[3] * a[3] + p[4] * a[4] + p[5] * a[5];
}

ARC_INLINE arcVec6 arcVec6::operator/( const float a ) const {
	float inva;

	assert( a != 0.0f );
	inva = 1.0f / a;
	return arcVec6( p[0]*inva, p[1]*inva, p[2]*inva, p[3]*inva, p[4]*inva, p[5]*inva );
}

ARC_INLINE arcVec6 arcVec6::operator+( const arcVec6 &a ) const {
	return arcVec6( p[0] + a[0], p[1] + a[1], p[2] + a[2], p[3] + a[3], p[4] + a[4], p[5] + a[5] );
}

ARC_INLINE arcVec6 arcVec6::operator-( const arcVec6 &a ) const {
	return arcVec6( p[0] - a[0], p[1] - a[1], p[2] - a[2], p[3] - a[3], p[4] - a[4], p[5] - a[5] );
}

ARC_INLINE arcVec6 &arcVec6::operator*=( const float a ) {
	p[0] *= a;
	p[1] *= a;
	p[2] *= a;
	p[3] *= a;
	p[4] *= a;
	p[5] *= a;
	return *this;
}

ARC_INLINE arcVec6 &arcVec6::operator/=( const float a ) {
	float inva;

	assert( a != 0.0f );
	inva = 1.0f / a;
	p[0] *= inva;
	p[1] *= inva;
	p[2] *= inva;
	p[3] *= inva;
	p[4] *= inva;
	p[5] *= inva;
	return *this;
}

ARC_INLINE arcVec6 &arcVec6::operator+=( const arcVec6 &a ) {
	p[0] += a[0];
	p[1] += a[1];
	p[2] += a[2];
	p[3] += a[3];
	p[4] += a[4];
	p[5] += a[5];
	return *this;
}

ARC_INLINE arcVec6 &arcVec6::operator-=( const arcVec6 &a ) {
	p[0] -= a[0];
	p[1] -= a[1];
	p[2] -= a[2];
	p[3] -= a[3];
	p[4] -= a[4];
	p[5] -= a[5];
	return *this;
}

ARC_INLINE arcVec6 operator*( const float a, const arcVec6 b ) {
	return b * a;
}

ARC_INLINE bool arcVec6::Compare( const arcVec6 &a ) const {
	return ( ( p[0] == a[0] ) && ( p[1] == a[1] ) && ( p[2] == a[2] ) &&
			( p[3] == a[3] ) && ( p[4] == a[4] ) && ( p[5] == a[5] ) );
}

ARC_INLINE bool arcVec6::Compare( const arcVec6 &a, const float epsilon ) const {
	if ( arcMath::Fabs( p[0] - a[0] ) > epsilon ) {
		return false;
	}

	if ( arcMath::Fabs( p[1] - a[1] ) > epsilon ) {
		return false;
	}

	if ( arcMath::Fabs( p[2] - a[2] ) > epsilon ) {
		return false;
	}

	if ( arcMath::Fabs( p[3] - a[3] ) > epsilon ) {
		return false;
	}

	if ( arcMath::Fabs( p[4] - a[4] ) > epsilon ) {
		return false;
	}

	if ( arcMath::Fabs( p[5] - a[5] ) > epsilon ) {
		return false;
	}

	return true;
}

ARC_INLINE bool arcVec6::operator==( const arcVec6 &a ) const {
	return Compare( a );
}

ARC_INLINE bool arcVec6::operator!=( const arcVec6 &a ) const {
	return !Compare( a );
}

ARC_INLINE void arcVec6::Set( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 ) {
	p[0] = a1;
	p[1] = a2;
	p[2] = a3;
	p[3] = a4;
	p[4] = a5;
	p[5] = a6;
}

ARC_INLINE void arcVec6::Zero( void ) {
	p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = 0.0f;
}

ARC_INLINE float arcVec6::Length( void ) const {
	return ( float )arcMath::Sqrt( p[0] * p[0] + p[1] * p[1] + p[2] * p[2] + p[3] * p[3] + p[4] * p[4] + p[5] * p[5] );
}

ARC_INLINE float arcVec6::LengthSqr( void ) const {
	return ( p[0] * p[0] + p[1] * p[1] + p[2] * p[2] + p[3] * p[3] + p[4] * p[4] + p[5] * p[5] );
}

ARC_INLINE float arcVec6::Normalize( void ) {
	float sqrLength, invLength;

	sqrLength = p[0] * p[0] + p[1] * p[1] + p[2] * p[2] + p[3] * p[3] + p[4] * p[4] + p[5] * p[5];
	invLength = arcMath::InvSqrt( sqrLength );
	p[0] *= invLength;
	p[1] *= invLength;
	p[2] *= invLength;
	p[3] *= invLength;
	p[4] *= invLength;
	p[5] *= invLength;
	return invLength * sqrLength;
}

ARC_INLINE float arcVec6::NormalizeFast( void ) {
	float sqrLength, invLength;

	sqrLength = p[0] * p[0] + p[1] * p[1] + p[2] * p[2] + p[3] * p[3] + p[4] * p[4] + p[5] * p[5];
	invLength = arcMath::RSqrt( sqrLength );
	p[0] *= invLength;
	p[1] *= invLength;
	p[2] *= invLength;
	p[3] *= invLength;
	p[4] *= invLength;
	p[5] *= invLength;
	return invLength * sqrLength;
}

ARC_INLINE int arcVec6::GetDimension( void ) const {
	return 6;
}

ARC_INLINE const arcVec3 &arcVec6::SubVec3( int index ) const {
	return *reinterpret_cast<const arcVec3 *>(p + index * 3);
}

ARC_INLINE arcVec3 &arcVec6::SubVec3( int index ) {
	return *reinterpret_cast<arcVec3 *>(p + index * 3);
}

ARC_INLINE const float *arcVec6::ToFloatPtr( void ) const {
	return p;
}

ARC_INLINE float *arcVec6::ToFloatPtr( void ) {
	return p;
}


//===============================================================
//
//	arcVecX - arbitrary sized vector
//
//  The vector lives on 16 byte aligned and 16 byte padded memory.
//
//	NOTE: due to the temporary memory pool arcVecX cannot be used by multiple threads
//
//===============================================================

#define VECX_MAX_TEMP		1024
#define VECX_QUAD( x )		( ( ( ( x ) + 3 ) & ~3 ) * sizeof( float ) )
#define VECX_CLEAREND()		int s = size; while( s < ( ( s + 3) & ~3 ) ) { p[s++] = 0.0f; }
#define VECX_ALLOCA( n )	( (float *) _alloca16( VECX_QUAD( n ) ) )
#define VECX_SIMD

class arcVecX {
	friend class arcMatX;

public:
					arcVecX( void );
					explicit arcVecX( int length );
					explicit arcVecX( int length, float *data );
					~arcVecX( void );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	arcVecX			operator-() const;
	arcVecX &		operator=( const arcVecX &a );
	arcVecX			operator*( const float a ) const;
	arcVecX			operator/( const float a ) const;
	float			operator*( const arcVecX &a ) const;
	arcVecX			operator-( const arcVecX &a ) const;
	arcVecX			operator+( const arcVecX &a ) const;
	arcVecX &		operator*=( const float a );
	arcVecX &		operator/=( const float a );
	arcVecX &		operator+=( const arcVecX &a );
	arcVecX &		operator-=( const arcVecX &a );

	friend arcVecX	operator*( const float a, const arcVecX b );

	bool			Compare( const arcVecX &a ) const;							// exact compare, no epsilon
	bool			Compare( const arcVecX &a, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const arcVecX &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const arcVecX &a ) const;						// exact compare, no epsilon

	void			SetSize( int size );
	void			ChangeSize( int size, bool makeZero = false );
	int				GetSize( void ) const { return size; }
	void			SetData( int length, float *data );
	void			Zero( void );
	void			Zero( int length );
	void			Random( int seed, float l = 0.0f, float u = 1.0f );
	void			Random( int length, int seed, float l = 0.0f, float u = 1.0f );
	void			Negate( void );
	void			Clamp( float min, float max );
	arcVecX &		SwapElements( int e1, int e2 );

	float			Length( void ) const;
	float			LengthSqr( void ) const;
	arcVecX			Normalize( void ) const;
	float			NormalizeSelf( void );

	int				GetDimension( void ) const;

	const arcVec3 &	SubVec3( int index ) const;
	arcVec3 &		SubVec3( int index );
	const arcVec6 &	SubVec6( int index ) const;
	arcVec6 &		SubVec6( int index );
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

private:
	int				size;					// size of the vector
	int				alloced;				// if -1 p points to data set with SetData
	float *			p;						// memory the vector is stored

	static float	temp[VECX_MAX_TEMP+4];	// used to store intermediate results
	static float *	tempPtr;				// pointer to 16 byte aligned temporary memory
	static int		tempIndex;				// index into memory pool, wraps around

private:
	void			SetTempSize( int size );
};


ARC_INLINE arcVecX::arcVecX( void ) {
	size = alloced = 0;
	p = NULL;
}

ARC_INLINE arcVecX::arcVecX( int length ) {
	size = alloced = 0;
	p = NULL;
	SetSize( length );
}

ARC_INLINE arcVecX::arcVecX( int length, float *data ) {
	size = alloced = 0;
	p = NULL;
	SetData( length, data );
}

ARC_INLINE arcVecX::~arcVecX( void ) {
	// if not temp memory
	if ( p && ( p < arcVecX::tempPtr || p >= arcVecX::tempPtr + VECX_MAX_TEMP ) && alloced != -1 ) {
		Mem_Free16( p );
	}
}

ARC_INLINE float arcVecX::operator[]( const int index ) const {
	assert( index >= 0 && index < size );
	return p[index];
}

ARC_INLINE float &arcVecX::operator[]( const int index ) {
	assert( index >= 0 && index < size );
	return p[index];
}

ARC_INLINE arcVecX arcVecX::operator-() const {
	int i;
	arcVecX m;

	m.SetTempSize( size );
	for ( int i = 0; i < size; i++ ) {
		m.p[i] = -p[i];
	}
	return m;
}

ARC_INLINE arcVecX &arcVecX::operator=( const arcVecX &a ) {
	SetSize( a.size );
#ifdef VECX_SIMD
	SIMDProcessor->Copy16( p, a.p, a.size );
#else
	memcpy( p, a.p, a.size * sizeof( float ) );
#endif
	arcVecX::tempIndex = 0;
	return *this;
}

ARC_INLINE arcVecX arcVecX::operator+( const arcVecX &a ) const {
	arcVecX m;

	assert( size == a.size );
	m.SetTempSize( size );
#ifdef VECX_SIMD
	SIMDProcessor->Add16( m.p, p, a.p, size );
#else
	for ( int i = 0; i < size; i++ ) {
		m.p[i] = p[i] + a.p[i];
	}
#endif
	return m;
}

ARC_INLINE arcVecX arcVecX::operator-( const arcVecX &a ) const {
	arcVecX m;

	assert( size == a.size );
	m.SetTempSize( size );
#ifdef VECX_SIMD
	SIMDProcessor->Sub16( m.p, p, a.p, size );
#else
	for ( int i = 0; i < size; i++ ) {
		m.p[i] = p[i] - a.p[i];
	}
#endif
	return m;
}

ARC_INLINE arcVecX &arcVecX::operator+=( const arcVecX &a ) {
	assert( size == a.size );
#ifdef VECX_SIMD
	SIMDProcessor->AddAssign16( p, a.p, size );
#else
	for ( int i = 0; i < size; i++ ) {
		p[i] += a.p[i];
	}
#endif
	arcVecX::tempIndex = 0;
	return *this;
}

ARC_INLINE arcVecX &arcVecX::operator-=( const arcVecX &a ) {
	assert( size == a.size );
#ifdef VECX_SIMD
	SIMDProcessor->SubAssign16( p, a.p, size );
#else
	for ( int i = 0; i < size; i++ ) {
		p[i] -= a.p[i];
	}
#endif
	arcVecX::tempIndex = 0;
	return *this;
}

ARC_INLINE arcVecX arcVecX::operator*( const float a ) const {
	arcVecX m;

	m.SetTempSize( size );
#ifdef VECX_SIMD
	SIMDProcessor->Mul16( m.p, p, a, size );
#else
	for ( int i = 0; i < size; i++ ) {
		m.p[i] = p[i] * a;
	}
#endif
	return m;
}

ARC_INLINE arcVecX &arcVecX::operator*=( const float a ) {
#ifdef VECX_SIMD
	SIMDProcessor->MulAssign16( p, a, size );
#else
	for ( int i = 0; i < size; i++ ) {
		p[i] *= a;
	}
#endif
	return *this;
}

ARC_INLINE arcVecX arcVecX::operator/( const float a ) const {
	assert( a != 0.0f );
	return (*this) * ( 1.0f / a );
}

ARC_INLINE arcVecX &arcVecX::operator/=( const float a ) {
	assert( a != 0.0f );
	(*this) *= ( 1.0f / a );
	return *this;
}

ARC_INLINE arcVecX operator*( const float a, const arcVecX b ) {
	return b * a;
}

ARC_INLINE float arcVecX::operator*( const arcVecX &a ) const {
	float sum = 0.0f;

	assert( size == a.size );
	for ( int i = 0; i < size; i++ ) {
		sum += p[i] * a.p[i];
	}
	return sum;
}

ARC_INLINE bool arcVecX::Compare( const arcVecX &a ) const {
	assert( size == a.size );
	for ( int i = 0; i < size; i++ ) {
		if ( p[i] != a.p[i] ) {
			return false;
		}
	}
	return true;
}

ARC_INLINE bool arcVecX::Compare( const arcVecX &a, const float epsilon ) const {
	assert( size == a.size );
	for ( int i = 0; i < size; i++ ) {
		if ( arcMath::Fabs( p[i] - a.p[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

ARC_INLINE bool arcVecX::operator==( const arcVecX &a ) const {
	return Compare( a );
}

ARC_INLINE bool arcVecX::operator!=( const arcVecX &a ) const {
	return !Compare( a );
}

ARC_INLINE void arcVecX::SetSize( int newSize ) {
	int alloc = ( newSize + 3 ) & ~3;
	if ( alloc > alloced && alloced != -1 ) {
		if ( p ) {
			Mem_Free16( p );
		}
		p = (float *) Mem_Alloc16( alloc * sizeof( float ) );
		alloced = alloc;
	}
	size = newSize;
	VECX_CLEAREND();
}

ARC_INLINE void arcVecX::ChangeSize( int newSize, bool makeZero ) {
	int alloc = ( newSize + 3 ) & ~3;
	if ( alloc > alloced && alloced != -1 ) {
		float *oldVec = p;
		p = (float *) Mem_Alloc16( alloc * sizeof( float ) );
		alloced = alloc;
		if ( oldVec ) {
			for ( int i = 0; i < size; i++ ) {
				p[i] = oldVec[i];
			}
			Mem_Free16( oldVec );
		}
		if ( makeZero ) {
			// zero any new elements
			for ( int i = size; i < newSize; i++ ) {
				p[i] = 0.0f;
			}
		}
	}
	size = newSize;
	VECX_CLEAREND();
}

ARC_INLINE void arcVecX::SetTempSize( int newSize ) {
	size = newSize;
	alloced = ( newSize + 3 ) & ~3;
	assert( alloced < VECX_MAX_TEMP );
	if ( arcVecX::tempIndex + alloced > VECX_MAX_TEMP ) {
		arcVecX::tempIndex = 0;
	}
	p = arcVecX::tempPtr + arcVecX::tempIndex;
	arcVecX::tempIndex += alloced;
	VECX_CLEAREND();
}

ARC_INLINE void arcVecX::SetData( int length, float *data ) {
	if ( p && ( p < arcVecX::tempPtr || p >= arcVecX::tempPtr + VECX_MAX_TEMP ) && alloced != -1 ) {
		Mem_Free16( p );
	}
	assert( ( ( ( int ) data ) & 15 ) == 0 ); // data must be 16 byte aligned
	p = data;
	size = length;
	alloced = -1;
	VECX_CLEAREND();
}

ARC_INLINE void arcVecX::Zero( void ) {
#ifdef VECX_SIMD
	SIMDProcessor->Zero16( p, size );
#else
	memset( p, 0, size * sizeof( float ) );
#endif
}

ARC_INLINE void arcVecX::Zero( int length ) {
	SetSize( length );
#ifdef VECX_SIMD
	SIMDProcessor->Zero16( p, length );
#else
	memset( p, 0, size * sizeof( float ) );
#endif
}

ARC_INLINE void arcVecX::Random( int seed, float l, float u ) {
	float c;
	arcRandom rnd( seed );

	c = u - l;
	for ( int i = 0; i < size; i++ ) {
		p[i] = l + rnd.RandomFloat() * c;
	}
}

ARC_INLINE void arcVecX::Random( int length, int seed, float l, float u ) {
	arcRandom rnd( seed );

	SetSize( length );
	float c = u - l;
	for ( int i = 0; i < size; i++ ) {
		p[i] = l + rnd.RandomFloat() * c;
	}
}

ARC_INLINE void arcVecX::Negate( void ) {
#ifdef VECX_SIMD
	SIMDProcessor->Negate16( p, size );
#else
	for ( int i = 0; i < size; i++ ) {
		p[i] = -p[i];
	}
#endif
}

ARC_INLINE void arcVecX::Clamp( float min, float max ) {
	for ( int i = 0; i < size; i++ ) {
		if ( p[i] < min ) {
			p[i] = min;
		} else if ( p[i] > max ) {
			p[i] = max;
		}
	}
}

ARC_INLINE arcVecX &arcVecX::SwapElements( int e1, int e2 ) {
	float tmp = p[e1];
	p[e1] = p[e2];
	p[e2] = tmp;
	return *this;
}

ARC_INLINE float arcVecX::Length( void ) const {
	float sum = 0.0f;

	for ( int i = 0; i < size; i++ ) {
		sum += p[i] * p[i];
	}
	return arcMath::Sqrt( sum );
}

ARC_INLINE float arcVecX::LengthSqr( void ) const {
	float sum = 0.0f;

	for ( int i = 0; i < size; i++ ) {
		sum += p[i] * p[i];
	}
	return sum;
}

ARC_INLINE arcVecX arcVecX::Normalize( void ) const {
	float invSqrt, sum = 0.0f;

	arcVecX m.SetTempSize( size );
	for ( int i = 0; i < size; i++ ) {
		sum += p[i] * p[i];
	}
	invSqrt = arcMath::InvSqrt( sum );
	for ( int i = 0; i < size; i++ ) {
		m.p[i] = p[i] * invSqrt;
	}
	return m;
}

ARC_INLINE float arcVecX::NormalizeSelf( void ) {
	float invSqrt, sum = 0.0f;

	//invSqrt = arcMath::InvSqrt( LengthSqr() );
	for ( int i = 0; i < size; i++ ) {
		sum += p[i] * p[i];
	}
	invSqrt = arcMath::InvSqrt( sum );
	for ( int i = 0; i < size; i++ ) {
		p[i] *= invSqrt;
	}
	return invSqrt * sum;
}

ARC_INLINE int arcVecX::GetDimension( void ) const {
	return size;
}

ARC_INLINE arcVec3 &arcVecX::SubVec3( int index ) {
	assert( index >= 0 && index * 3 + 3 <= size );
	return *reinterpret_cast<arcVec3 *>( p + index * 3 );
}

ARC_INLINE const arcVec3 &arcVecX::SubVec3( int index ) const {
	assert( index >= 0 && index * 3 + 3 <= size );
	return *reinterpret_cast<const arcVec3 *>(p + index * 3);
}

ARC_INLINE arcVec6 &arcVecX::SubVec6( int index ) {
	assert( index >= 0 && index * 6 + 6 <= size );
	return *reinterpret_cast<arcVec6 *>( p + index * 6 );
}

ARC_INLINE const arcVec6 &arcVecX::SubVec6( int index ) const {
	assert( index >= 0 && index * 6 + 6 <= size );
	return *reinterpret_cast<const arcVec6 *>( p + index * 6 );
}

ARC_INLINE const float *arcVecX::ToFloatPtr( void ) const {
	return p;
}

ARC_INLINE float *arcVecX::ToFloatPtr( void ) {
	return p;
}

//===============================================================
//
//	idPolar3
//
//===============================================================

class idPolar3 {
public:
	float			radius, theta, phi;

					idPolar3( void );
					explicit idPolar3( const float radius, const float theta, const float phi );

	void 			Set( const float radius, const float theta, const float phi );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	idPolar3		operator-() const;
	idPolar3 &		operator=( const idPolar3 &a );

	arcVec3			ToVec3( void ) const;
};

ARC_INLINE idPolar3::idPolar3( void ) {
}

ARC_INLINE idPolar3::idPolar3( const float radius, const float theta, const float phi ) {
	assert( radius > 0 );
	this->radius = radius;
	this->theta = theta;
	this->phi = phi;
}

ARC_INLINE void idPolar3::Set( const float radius, const float theta, const float phi ) {
	assert( radius > 0 );
	this->radius = radius;
	this->theta = theta;
	this->phi = phi;
}

ARC_INLINE float idPolar3::operator[]( const int index ) const {
	return ( &radius )[index];
}

ARC_INLINE float &idPolar3::operator[]( const int index ) {
	return ( &radius )[index];
}

ARC_INLINE idPolar3 idPolar3::operator-() const {
	return idPolar3( radius, -theta, -phi );
}

ARC_INLINE idPolar3 &idPolar3::operator=( const idPolar3 &a ) {
	radius = a.radius;
	theta = a.theta;
	phi = a.phi;
	return *this;
}

ARC_INLINE arcVec3 idPolar3::ToVec3( void ) const {
	float sp, cp, st, ct;
	arcMath::SinCos( phi, sp, cp );
	arcMath::SinCos( theta, st, ct );
 	return arcVec3( cp * radius * ct, cp * radius * st, radius * sp );
}


/*
===============================================================================

	Old 3D vector macros, should no longer be used.

===============================================================================
*/

#define DotProduct( a, b)			( ( a )[0]*( b )[0]+( a )[1]*( b )[1]+( a )[2]*( b )[2] )
#define VectorSubtract( a, b, c )	( ( c )[0]=( a )[0]-( b )[0],( c )[1]=( a )[1]-( b )[1],( c )[2]=( a )[2]-( b )[2] )
#define VectorAdd( a, b, c )		( ( c )[0]=( a )[0]+( b )[0],( c )[1]=( a )[1]+( b )[1],( c )[2]=( a )[2]+( b )[2] )
#define	VectorScale( v, s, o )		((o)[0]=( v)[0]*(s),(o)[1]=( v)[1]*(s),(o)[2]=( v)[2]*(s) )
#define	VectorMA( v, s, b, o )		((o)[0]=( v)[0]+( b )[0]*(s),(o)[1]=( v)[1]+( b )[1]*(s),(o)[2]=( v)[2]+( b )[2]*(s) )
#define VectorCopy( a, b )			( ( b )[0]=( a )[0],( b )[1]=( a )[1],( b )[2]=( a )[2] )


#endif /* !__MATH_VECTOR_H__ */
