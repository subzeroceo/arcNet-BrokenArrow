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
class anMat3;

//===============================================================
//
//	anVec2 - 2D vector
//
//===============================================================

class anVec2 {
public:
	float			x;
	float			y;

					anVec2( void );
					explicit anVec2( const float x, const float y );

	void 			Set( const float x, const float y );
	void			Zero( void );

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	anVec2		operator-() const;
	float			operator*( const anVec2 &a ) const;
	anVec2		oerator*( const float a ) const;
	anVec2		operator/( const float a ) const;
	anVec2		operator+( const anVec2 &a ) const;
	anVec2		operator-( const anVec2 &a ) const;
	anVec2 &		operator+=( const anVec2 &a );
	anVec2 &		operator-=( const anVec2 &a );
	anVec2 &		operator/=( const anVec2 &a );
	anVec2 &		operator/=( const float a );
	anVec2 &		operator*=( const float a );

	friend anVec2	operator*( const float a, const anVec2 b );

	bool			Compare( const anVec2 &a ) const;							// exact compare, no epsilon
	bool			Compare( const anVec2 &a, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const anVec2 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const anVec2 &a ) const;						// exact compare, no epsilon

	float			Length( void ) const;
	float			LengthFast( void ) const;
	float			LengthSqr( void ) const;
	float			Normalize( void );			// returns length
	float			NormalizeFast( void );		// returns length
	anVec2 &		Truncate( float length );	// cap length
	void			Clamp( const anVec2 &min, const anVec2 &max );
	void			Snap( void );				// snap to closest integer value
	void			SnapInt( void );			// snap towards integer (floor)

	int				GetDimension( void ) const;

	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

	void			Lerp( const anVec2 &v1, const anVec2 &v2, const float l );
};

extern anVec2 vec2_origin;
#define vec2_zero vec2_origin

ARC_INLINE anVec2::anVec2( void ) {
}

ARC_INLINE anVec2::anVec2( const float x, const float y ) {
	this->x = x;
	this->y = y;
}

ARC_INLINE void anVec2::Set( const float x, const float y ) {
	this->x = x;
	this->y = y;
}

ARC_INLINE void anVec2::Zero( void ) {
	x = y = 0.0f;
}

ARC_INLINE bool anVec2::Compare( const anVec2 &a ) const {
	return ( ( x == a.x ) && ( y == a.y ) );
}

ARC_INLINE bool anVec2::Compare( const anVec2 &a, const float epsilon ) const {
	if ( anMath::Fabs( x - a.x ) > epsilon ) {
		return false;
	}

	if ( anMath::Fabs( y - a.y ) > epsilon ) {
		return false;
	}

	return true;
}

ARC_INLINE bool anVec2::operator==( const anVec2 &a ) const {
	return Compare( a );
}

ARC_INLINE bool anVec2::operator!=( const anVec2 &a ) const {
	return !Compare( a );
}

ARC_INLINE float anVec2::operator[]( int index ) const {
	return ( &x )[index];
}

ARC_INLINE float& anVec2::operator[]( int index ) {
	return ( &x )[index];
}

ARC_INLINE float anVec2::Length( void ) const {
	return ( float )anMath::Sqrt( x * x + y * y );
}

ARC_INLINE float anVec2::LengthFast( void ) const {
	float sqrLength;

	sqrLength = x * x + y * y;
	return sqrLength * anMath::RSqrt( sqrLength );
}

ARC_INLINE float anVec2::LengthSqr( void ) const {
	return ( x * x + y * y );
}

ARC_INLINE float anVec2::Normalize( void ) {
	float sqrLength, invLength;

	sqrLength = x * x + y * y;
	invLength = anMath::InvSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	return invLength * sqrLength;
}

ARC_INLINE float anVec2::NormalizeFast( void ) {
	float lengthSqr, invLength;

	lengthSqr = x * x + y * y;
	invLength = anMath::RSqrt( lengthSqr );
	x *= invLength;
	y *= invLength;
	return invLength * lengthSqr;
}

ARC_INLINE anVec2 &anVec2::Truncate( float length ) {
	float length2;
	float ilength;

	if ( !length ) {
		Zero();
	} else {
		length2 = LengthSqr();
		if ( length2 > length * length ) {
			ilength = length * anMath::InvSqrt( length2 );
			x *= ilength;
			y *= ilength;
		}
	}

	return *this;
}

ARC_INLINE void anVec2::Clamp( const anVec2 &min, const anVec2 &max ) {
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

ARC_INLINE void anVec2::Snap( void ) {
	x = floor( x + 0.5f );
	y = floor( y + 0.5f );
}

ARC_INLINE void anVec2::SnapInt( void ) {
	x = float( int( x ) );
	y = float( int( y ) );
}

ARC_INLINE anVec2 anVec2::operator-() const {
	return anVec2( -x, -y );
}

ARC_INLINE anVec2 anVec2::operator-( const anVec2 &a ) const {
	return anVec2( x - a.x, y - a.y );
}

ARC_INLINE float anVec2::operator*( const anVec2 &a ) const {
	return x * a.x + y * a.y;
}

ARC_INLINE anVec2 anVec2::operator*( const float a ) const {
	return anVec2( x * a, y * a );
}

ARC_INLINE anVec2 anVec2::operator/( const float a ) const {
	float inva = 1.0f / a;
	return anVec2( x * inva, y * inva );
}

ARC_INLINE anVec2 operator*( const float a, const anVec2 b ) {
	return anVec2( b.x * a, b.y * a );
}

ARC_INLINE anVec2 anVec2::operator+( const anVec2 &a ) const {
	return anVec2( x + a.x, y + a.y );
}

ARC_INLINE anVec2 &anVec2::operator+=( const anVec2 &a ) {
	x += a.x;
	y += a.y;

	return *this;
}

ARC_INLINE anVec2 &anVec2::operator/=( const anVec2 &a ) {
	x /= a.x;
	y /= a.y;

	return *this;
}

ARC_INLINE anVec2 &anVec2::operator/=( const float a ) {
	float inva = 1.0f / a;
	x *= inva;
	y *= inva;

	return *this;
}

ARC_INLINE anVec2 &anVec2::operator-=( const anVec2 &a ) {
	x -= a.x;
	y -= a.y;

	return *this;
}

ARC_INLINE anVec2 &anVec2::operator*=( const float a ) {
	x *= a;
	y *= a;

	return *this;
}

ARC_INLINE int anVec2::GetDimension( void ) const {
	return 2;
}

ARC_INLINE const float *anVec2::ToFloatPtr( void ) const {
	return &x;
}

ARC_INLINE float *anVec2::ToFloatPtr( void ) {
	return &x;
}

//===============================================================
//
//	anVec3 - 3D vector
//
//===============================================================
#define anVec3 anVec3

class anVec3 {
public:
	float			x;
	float			y;
	float			z;

					anVec3( void );
					explicit anVec3( const float x, const float y, const float z );

	void 			Set( const float x, const float y, const float z );
	void			Zero( void );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	anVec3			operator-() const;
	anVec3 &		operator=( const anVec3 &a );		// required because of a msvc 6 & 7 bug
	float			operator*( const anVec3 &a ) const;
	anVec3			operator*( const float a ) const;
	anVec3			operator/( const float a ) const;
	anVec3			operator+( const anVec3 &a ) const;
	anVec3			operator-( const anVec3 &a ) const;
	anVec3 &		operator+=( const anVec3 &a );
	anVec3 &		operator-=( const anVec3 &a );
	anVec3 &		operator/=( const anVec3 &a );
	anVec3 &		operator/=( const float a );
	anVec3 &		operator*=( const float a );

	friend anVec3	operator*( const float a, const anVec3 b );

	bool			Compare( const anVec3 &a ) const;							// exact compare, no epsilon
	bool			Compare( const anVec3 &a, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const anVec3 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const anVec3 &a ) const;						// exact compare, no epsilon

	bool			FixDegenerateNormal( void );	// fix degenerate axial cases
	bool			FixDenormals( void );			// change tiny numbers to zero

	anVec3			Cross( const anVec3 &a ) const;
	anVec3 &		Cross( const anVec3 &a, const anVec3 &b );
	float 			Dot( const anVec3 &v1, const anVec3 &v2 ) const;
	anVec3			Dot( const anVec3 v1, const anVec3 v2 );
	float			Length( void ) const;
	float			LengthSqr( void ) const;
	float			LengthFast( void ) const;
	float			Normalize( void );				// returns length
	float			NormalizeFast( void );			// returns length
	anVec3 		VectorNormalize( anVec3 v );
	anVec3 &		Truncate( float length );		// cap length
	void			Clamp( const anVec3 &min, const anVec3 &max );
	void			Snap( void );					// snap to closest integer value
	void			SnapInt( void );				// snap towards integer (floor)

	int				GetDimension( void ) const;

	void			Rotate( anVec3 in, anMat3 matrix[3], anVec3 out );
	float			ToYaw( void ) const;
	float			ToPitch( void ) const;
	arcAngles		ToAngles( void ) const;
	idPolar3		ToPolar( void ) const;
	anMat3			ToMat3( void ) const;		// vector should be normalized
	const anVec2 &	ToVec2( void ) const;
	anVec2 &		ToVec2( void );
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

	void			NormalVectors( anVec3 &left, anVec3 &down ) const;	// vector should be normalized
	void			OrthogonalBasis( anVec3 &left, anVec3 &up ) const;

	void			ProjectOntoPlane( const anVec3 &normal, const float overBounce = 1.0f );
	bool			ProjectAlongPlane( const anVec3 &normal, const float epsilon, const float overBounce = 1.0f );
	void			ProjectSelfOntoSphere( const float radius );

	void			Lerp( const anVec3 &v1, const anVec3 &v2, const float l );
	void			SLerp( const anVec3 &v1, const anVec3 &v2, const float l );
	float			LerpAngle( const float from, const float to, const float frac );
	void			Inverse( const anVec3 v );
	void 			MA( const anVec3 a, float scale, const anVec3 b, anVec3 c );
	void			Subtract( const anVec3 a, const anVec3 b, anVec3 out );
	void			Add( const anVec3 a, const anVec3 b, anVec3 out );
	void			Copy( const anVec3 in, anVec3 out );
	void			Scale( const anVec3 in, const anVec3 scale, anVec3 out );

	//bool  			ProjectAlongPlane( const anVec3 &normal, const float epsilon, const float overBounce ) const;
	//void			ProjectOntoPlane( const anVec3 &normal, const float overBounce );

	anVec2			Octahedron( void );

	anVec3			OctahedronDecode( const anVec2 &oct );
	anVec3			Octahedron_Tangent( const float sign ) const;


	anVec3			Octahedron_TangentDecode( const anVec2 &oct, float *sign );

 	bool			IsFinite() const;
};

#define anVec3_x Vector( -1, -1, -1 )
#define anVec3_y Vector( -2, -1, -1 )
#define anVec3_z Vector( -3, -1, -1 )

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

extern anVec3 vec3_origin;
#define vec3_zero vec3_origin

ARC_INLINE anVec3::anVec3( void ) {
}

ARC_INLINE anVec3::anVec3( const float x, const float y, const float z ) {
	this->x = x;
	this->y = y;
	this->z = z;
}

ARC_INLINE float anVec3::operator[]( const int index ) const {
	return ( &x )[index];
}

ARC_INLINE float &anVec3::operator[]( const int index ) {
	return ( &x )[index];
}

ARC_INLINE void anVec3::Set( const float x, const float y, const float z ) {
	this->x = x; this->y = y; this->z = z;
}

ARC_INLINE void anVec3::Zero( void ) {
	x = y = z = 0.0f;
}

ARC_INLINE anVec3 anVec3::operator-() const {
	return anVec3( -x, -y, -z );
}

ARC_INLINE anVec3 &anVec3::operator=( const anVec3 &a ) {
	x = a.x; y = a.y; z = a.z;
	return *this;
}

ARC_INLINE anVec3 anVec3::operator-( const anVec3 &a ) const {
	return anVec3( x - a.x, y - a.y, z - a.z );
}

ARC_INLINE float anVec3::operator*( const anVec3 &a ) const {
	return x * a.x + y * a.y + z * a.z;
}

ARC_INLINE anVec3 anVec3::operator*( const float a ) const {
	return anVec3( x * a, y * a, z * a );
}

ARC_INLINE anVec3 anVec3::operator/( const float a ) const {
	float inva = 1.0f / a;
	return anVec3( x * inva, y * inva, z * inva );
}

ARC_INLINE anVec3 operator*( const float a, const anVec3 b ) {
	return anVec3( b.x * a, b.y * a, b.z * a );
}

ARC_INLINE anVec3 anVec3::operator+( const anVec3 &a ) const {
	return anVec3( x + a.x, y + a.y, z + a.z );
}

ARC_INLINE anVec3 &anVec3::operator+=( const anVec3 &a ) {
	x += a.x; y += a.y; z += a.z;
	return *this;
}

ARC_INLINE anVec3 &anVec3::operator/=( const anVec3 &a ) {
	x /= a.x; y /= a.y; z /= a.z;
	return *this;
}

ARC_INLINE anVec3 &anVec3::operator/=( const float a ) {
	float inva = 1.0f / a;
	x *= inva; y *= inva; z *= inva;
	return *this;
}

ARC_INLINE anVec3 &anVec3::operator-=( const anVec3 &a ) {
	x -= a.x; y -= a.y; z -= a.z;
	return *this;
}

ARC_INLINE anVec3 &anVec3::operator*=( const float a ) {
	x *= a; y *= a; z *= a;
	return *this;
}

ARC_INLINE bool anVec3::Compare( const anVec3 &a ) const {
	return ( ( x == a.x ) && ( y == a.y ) && ( z == a.z ) );
}

ARC_INLINE bool anVec3::Compare( const anVec3 &a, const float epsilon ) const {
	if ( anMath::Fabs( x - a.x ) > epsilon ) {
		return false;
	}

	if ( anMath::Fabs( y - a.y ) > epsilon ) {
		return false;
	}

	if ( anMath::Fabs( z - a.z ) > epsilon ) {
		return false;
	}

	return true;
}

ARC_INLINE bool anVec3::operator==( const anVec3 &a ) const {
	return Compare( a );
}

ARC_INLINE bool anVec3::operator!=( const anVec3 &a ) const {
	return !Compare( a );
}

ARC_INLINE float anVec3::NormalizeFast( void ) {
	float sqrLength = x * x + y * y + z * z;
	float invLength = anMath::RSqrt( sqrLength );
	x *= invLength; y *= invLength; z *= invLength;
	return invLength * sqrLength;
}

ARC_INLINE bool anVec3::FixDegenerateNormal( void ) {
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
	if ( anMath::Fabs( x ) == 1.0f ) {
		if ( y != 0.0f || z != 0.0f ) {
			y = z = 0.0f;
			return true;
		}
		return false;
	} else if ( anMath::Fabs( y ) == 1.0f ) {
		if ( x != 0.0f || z != 0.0f ) {
			x = z = 0.0f;
			return true;
		}
		return false;
	} else if ( anMath::Fabs( z ) == 1.0f ) {
		if ( x != 0.0f || y != 0.0f ) {
			x = y = 0.0f;
			return true;
		}
		return false;
	}
	return false;
}

ARC_INLINE bool anVec3::FixDenormals( void ) {
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

// Calculate the cross product of two vectors
ARC_INLINE anVec3 anVec3::Cross( const anVec3 &a ) const {
	return anVec3( y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x );
}

ARC_INLINE anVec3 &anVec3::Cross( const anVec3 &a, const anVec3 &b ) {
	x = a.y * b.z - a.z * b.y;
	y = a.z * b.x - a.x * b.z;
	z = a.x * b.y - a.y * b.x;
	return *this;
}

// Calculate the dot product of two vectors
ARC_INLINE float Dot( const anVec3 &v1, const anVec3 &v2 ) const {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

ARC_INLINE anVec3 anVec3::Dot( const anVec3 v1, const anVec3 v2 ) {
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

ARC_INLINE float anVec3::Length( void ) const {
	return ( float )anMath::Sqrt( x * x + y * y + z * z );
}

ARC_INLINE float anVec3::LengthSqr( void ) const {
	return ( x * x + y * y + z * z );
}

ARC_INLINE float anVec3::LengthFast( void ) const {
	float sqrLength;
	sqrLength = x * x + y * y + z * z;
	return sqrLength * anMath::RSqrt( sqrLength );
}

ARC_INLINE float anVec3::Normalize( void ) {
	float sqrLength = x * x + y * y + z * z;
	float invLength = anMath::InvSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	z *= invLength;
	return invLength * sqrLength;
}

ARC_INLINE anVec3 &anVec3::Truncate( float length ) {
	if ( !length ) {
		Zero();
	} else {
		float length2 = LengthSqr();
		if ( length2 > length * length ) {
			float ilength = length * anMath::InvSqrt( length2 );
			x *= ilength;
			y *= ilength;
			z *= ilength;
		}
	}

	return *this;
}

ARC_INLINE void anVec3::Clamp( const anVec3 &min, const anVec3 &max ) {
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

ARC_INLINE void anVec3::Snap( void ) {
	x = floor( x + 0.5f );
	y = floor( y + 0.5f );
	z = floor( z + 0.5f );
}

ARC_INLINE void anVec3::SnapInt( void ) {
	x = float( int( x ) );
	y = float( int( y ) );
	z = float( int( z ) );
}

ARC_INLINE int anVec3::GetDimension( void ) const {
	return 3;
}

ARC_INLINE const anVec2 &anVec3::ToVec2( void ) const {
	return *reinterpret_cast<const anVec2 *>( this );
}

ARC_INLINE anVec2 &anVec3::ToVec2( void ) {
	return *reinterpret_cast<anVec2 *>( this );
}

ARC_INLINE const float *anVec3::ToFloatPtr( void ) const {
	return &x;
}

ARC_INLINE float *anVec3::ToFloatPtr( void ) {
	return &x;
}

ARC_INLINE void anVec3::NormalVectors( anVec3 &left, anVec3 &down ) const {
	float d = x * x + y * y;
	if ( !d ) {
		left[0] = 1;
		left[1] = 0;
		left[2] = 0;
	} else {
		d = anMath::InvSqrt( d );
		left[0] = -y * d;
		left[1] = x * d;
		left[2] = 0;
	}
	down = left.Cross( *this );
}

ARC_INLINE void anVec3::OrthogonalBasis( anVec3 &left, anVec3 &up ) const {
	if ( anMath::Fabs( z ) > 0.7f ) {
		float l = y * y + z * z;
		float s = anMath::InvSqrt( l );
		up[0] = 0;
		up[1] = z * s;
		up[2] = -y * s;
		left[0] = l * s;
		left[1] = -x * up[2];
		left[2] = x * up[1];
	} else {
		float l = x * x + y * y;
		float s = anMath::InvSqrt( l );
		left[0] = -y * s;
		left[1] = x * s;
		left[2] = 0;
		up[0] = -z * left[1];
		up[1] = z * left[0];
		up[2] = l * s;
	}
}

ARC_INLINE void anVec3::ProjectOntoPlane( const anVec3 &normal, const float overBounce ) {
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

ARC_INLINE bool anVec3::ProjectAlongPlane( const anVec3 &normal, const float epsilon, const float overBounce ) const {
	anVec3 cross = this->Cross( normal ).Cross( (* this) );
	// normalize so a fixed epsilon can be used
	cross.Normalize();
	float len = normal * cross;
	if ( anMath::Fabs( len ) < epsilon ) {
		return false;
	}
	cross *= overBounce * ( normal * (*this) ) / len;
	(*this) -= cross;
	return true;
}

ARC_INLINE anVec3 anVec3::OctahedronDecode( const anVec2 &oct ) {
	anVec2 f( oct.x * 2.0f - 1.0f, oct.y * 2.0f - 1.0f );
	anVec3 n( f.x, f.y, 1.0f - anMath::Abs( f.x ) - anMath::Abs( f.y ) );
	float t = Clamp( -n.z, 0.0f, 1.0f );
	n.x += n.x >= 0 ? -t : t;
	n.y += n.y >= 0 ? -t : t;
	return n.normalized();
}

ARC_INLINE anVec2 anVec3::Octahedron_Tangent( const float sign ) const {
	const float bias = 1.0f / 32767.0f;
	anVec2 res = this->Octahedron();
	res.y = Max( res.y, bias );
	res.y = res.y * 0.5f + 0.5f;
	res.y = sign >= 0.0f ? res.y : 1 - res.y;
	return res;
}

ARC_INLINE anVec3 anVec3::Octahedron_TangentDecode( const anVec2 &oct, float *sign ) {
	anVec2 compressed = oct;
	compressed.y = compressed.y * 2 - 1;
	*sign = compressed.y >= 0.0f ? 1.0f : -1.0f;
	compressed.y = anMath::abs( compressed.y );
	anVec3 res = anVec3::OctahedronDecode( compressed );
	return res;
}


ARC_INLINE anVec2 anVec3::Octahedron( void ) {
	anVec3 n = *this;
	n /= anMath::Abs( n.x ) + anMath::Abs( n.y ) + anMath::Abs( n.z );
	if ( n.z >= 0.0f ) {
		anVec2 o.x = n.x;
		anVec2 o.y = n.y;
	} else {
		anVec2 o.x = ( 1.0f - anMath::Abs( n.y ) ) * ( n.x >= 0.0f ? 1.0f : -1.0f );
		anVec2 o.y = ( 1.0f - anMath::Abs( n.x ) ) * ( n.y >= 0.0f ? 1.0f : -1.0f );
	}
	anVec2 o.x = o.x * 0.5f + 0.5f;
	anVec2 o.y = o.y * 0.5f + 0.5f;
	return o;
}

ARC_INLINE bool anVec3::IsFinite() const {
	return anMath::IsFinite( x ) && anMath::IsFinite( y ) && anMath::IsFinite( z );
}

//===============================================================
//
//	anVec4 - 4D vector
//
//===============================================================
#define anVec4 anVec4

class anVec4 {
public:
	float			x;
	float			y;
	float			z;
	float			w;

					anVec4( void );
					explicit anVec4( const float x, const float y, const float z, const float w );

	void 			Set( const float x, const float y, const float z, const float w );
	void			Zero( void );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	anVec4			operator-() const;
	float			operator*( const anVec4 &a ) const;
	anVec4			operator*( const float a ) const;
	anVec4			operator/( const float a ) const;
	anVec4			operator+( const anVec4 &a ) const;
	anVec4			operator-( const anVec4 &a ) const;
	anVec4 &		operator+=( const anVec4 &a );
	anVec4 &		operator-=( const anVec4 &a );
	anVec4 &		operator/=( const anVec4 &a );
	anVec4 &		operator/=( const float a );
	anVec4 &		operator*=( const float a );

	friend anVec4	operator*( const float a, const anVec4 b );

	bool			Compare( const anVec4 &a ) const;							// exact compare, no epsilon
	bool			Compare( const anVec4 &a, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const anVec4 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const anVec4 &a ) const;						// exact compare, no epsilon

	float			Length( void ) const;
	float			LengthSqr( void ) const;
	float			Normalize( void );			// returns length
	float			NormalizeFast( void );		// returns length

	int				GetDimension( void ) const;

	const anVec2 &	ToVec2( void ) const;
	anVec2 &		ToVec2( void );
	const anVec3 &	ToVec3( void ) const;
	anVec3 &		ToVec3( void );
	anVec3 &		4ToVec3( const anVec4 & v );
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

	void			Lerp( const anVec4 &v1, const anVec4 &v2, const float l );
	void			Scale( const anVec4 in, const anVec4 scale, anVec4 out );
};

extern anVec4 vec4_origin;
#define vec4_zero vec4_origin

ARC_INLINE anVec4::anVec4( void ) {
}

ARC_INLINE anVec4::anVec4( const float x, const float y, const float z, const float w ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

ARC_INLINE void anVec4::Set( const float x, const float y, const float z, const float w ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

ARC_INLINE void anVec4::Zero( void ) {
	x = y = z = w = 0.0f;
}

ARC_INLINE float anVec4::operator[]( int index ) const {
	return ( &x )[index];
}

ARC_INLINE float &anVec4::operator[]( int index ) {
	return ( &x )[index];
}

ARC_INLINE anVec4 anVec4::operator-() const {
	return anVec4( -x, -y, -z, -w );
}

ARC_INLINE anVec4 anVec4::operator-( const anVec4 &a ) const {
	return anVec4( x - a.x, y - a.y, z - a.z, w - a.w );
}

ARC_INLINE float anVec4::operator*( const anVec4 &a ) const {
	return x * a.x + y * a.y + z * a.z + w * a.w;
}

ARC_INLINE anVec4 anVec4::operator*( const float a ) const {
	return anVec4( x * a, y * a, z * a, w * a );
}

ARC_INLINE anVec4 anVec4::operator/( const float a ) const {
	float inva = 1.0f / a;
	return anVec4( x * inva, y * inva, z * inva, w * inva );
}

ARC_INLINE anVec4 operator*( const float a, const anVec4 b ) {
	return anVec4( b.x * a, b.y * a, b.z * a, b.w * a );
}

ARC_INLINE anVec4 anVec4::operator+( const anVec4 &a ) const {
	return anVec4( x + a.x, y + a.y, z + a.z, w + a.w );
}

ARC_INLINE anVec4 &anVec4::operator+=( const anVec4 &a ) {
	x += a.x;
	y += a.y;
	z += a.z;
	w += a.w;

	return *this;
}

ARC_INLINE anVec4 &anVec4::operator/=( const anVec4 &a ) {
	x /= a.x;
	y /= a.y;
	z /= a.z;
	w /= a.w;

	return *this;
}

ARC_INLINE anVec4 &anVec4::operator/=( const float a ) {
	float inva = 1.0f / a;
	x *= inva;
	y *= inva;
	z *= inva;
	w *= inva;

	return *this;
}

ARC_INLINE anVec4 &anVec4::operator-=( const anVec4 &a ) {
	x -= a.x;
	y -= a.y;
	z -= a.z;
	w -= a.w;

	return *this;
}

ARC_INLINE anVec4 &anVec4::operator*=( const float a ) {
	x *= a;
	y *= a;
	z *= a;
	w *= a;

	return *this;
}

ARC_INLINE bool anVec4::Compare( const anVec4 &a ) const {
	return ( ( x == a.x ) && ( y == a.y ) && ( z == a.z ) && w == a.w );
}

ARC_INLINE bool anVec4::Compare( const anVec4 &a, const float epsilon ) const {
	if ( anMath::Fabs( x - a.x ) > epsilon ) {
		return false;
	}

	if ( anMath::Fabs( y - a.y ) > epsilon ) {
		return false;
	}

	if ( anMath::Fabs( z - a.z ) > epsilon ) {
		return false;
	}

	if ( anMath::Fabs( w - a.w ) > epsilon ) {
		return false;
	}

	return true;
}

ARC_INLINE bool anVec4::operator==( const anVec4 &a ) const {
	return Compare( a );
}

ARC_INLINE bool anVec4::operator!=( const anVec4 &a ) const {
	return !Compare( a );
}

ARC_INLINE float anVec4::Length( void ) const {
	return ( float )anMath::Sqrt( x * x + y * y + z * z + w * w );
}

ARC_INLINE float anVec4::LengthSqr( void ) const {
	return ( x * x + y * y + z * z + w * w );
}

ARC_INLINE float anVec4::Normalize( void ) {
	float sqrLength, invLength;

	sqrLength = x * x + y * y + z * z + w * w;
	invLength = anMath::InvSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	z *= invLength;
	w *= invLength;
	return invLength * sqrLength;
}

ARC_INLINE float anVec4::NormalizeFast( void ) {
	float sqrLength, invLength;

	sqrLength = x * x + y * y + z * z + w * w;
	invLength = anMath::RSqrt( sqrLength );
	x *= invLength;
	y *= invLength;
	z *= invLength;
	w *= invLength;
	return invLength * sqrLength;
}

ARC_INLINE int anVec4::GetDimension( void ) const {
	return 4;
}

ARC_INLINE const anVec2 &anVec4::ToVec2( void ) const {
	return *reinterpret_cast<const anVec2 *>( this );
}

ARC_INLINE anVec2 &anVec4::ToVec2( void ) {
	return *reinterpret_cast<anVec2 *>( this );
}

ARC_INLINE const anVec3 &anVec4::ToVec3( void ) const {
	return *reinterpret_cast<const anVec3 *>( this );
}

ARC_INLINE anVec3 &anVec4::ToVec3( void ) {
	return *reinterpret_cast<anVec3 *>( this );
}

ARC_INLINE anVec3 &anVec4::4ToVec3( const anVec4 & v ) {
	return anVec3( v.x / v.w, v.y / v.w, v.z / v.w );
}

ARC_INLINE const float *anVec4::ToFloatPtr( void ) const {
	return &x;
}

ARC_INLINE float *anVec4::ToFloatPtr( void ) {
	return &x;
}


//===============================================================
//
//	anVec5 - 5D vector
//
//===============================================================

class anVec5 {
public:
	float			x;
	float			y;
	float			z;
	float			s;
	float			t;

					anVec5( void );
					explicit anVec5( const anVec3 ( &xyz ), const anVec2 &st );
					explicit anVec5( const float x, const float y, const float z, const float s, const float t );

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	anVec5 &		operator=( const anVec3 &a );

	int				GetDimension( void ) const;

	const anVec3 &	ToVec3( void ) const;
	anVec3 &		ToVec3( void );
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

	void			Lerp( const anVec5 &v1, const anVec5 &v2, const float l );
};

extern anVec5 vec5_origin;
#define vec5_zero vec5_origin

ARC_INLINE anVec5::anVec5( void ) {
}

ARC_INLINE anVec5::anVec5( const anVec3 ( &xyz ), const anVec2 &st ) {
	x = xyz.x;
	y = xyz.y;
	z = xyz.z;
	s = st[0];
	t = st[1];
}

ARC_INLINE anVec5::anVec5( const float x, const float y, const float z, const float s, const float t ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->s = s;
	this->t = t;
}

ARC_INLINE float anVec5::operator[]( int index ) const {
	return ( &x )[index];
}

ARC_INLINE float& anVec5::operator[]( int index ) {
	return ( &x )[index];
}

ARC_INLINE anVec5 &anVec5::operator=( const anVec3 &a ) {
	x = a.x;
	y = a.y;
	z = a.z;
	s = t = 0;
	return *this;
}

ARC_INLINE int anVec5::GetDimension( void ) const {
	return 5;
}

ARC_INLINE const anVec3 &anVec5::ToVec3( void ) const {
	return *reinterpret_cast<const anVec3 *>( this );
}

ARC_INLINE anVec3 &anVec5::ToVec3( void ) {
	return *reinterpret_cast<anVec3 *>( this );
}

ARC_INLINE const float *anVec5::ToFloatPtr( void ) const {
	return &x;
}

ARC_INLINE float *anVec5::ToFloatPtr( void ) {
	return &x;
}


//===============================================================
//
//	anVec6 - 6D vector
//
//===============================================================

class anVec6 {
public:
					anVec6( void );
					explicit anVec6( const float *a );
					explicit anVec6( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 );

	void 			Set( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 );
	void			Zero( void );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	anVec6			operator-() const;
	anVec6			operator*( const float a ) const;
	anVec6			operator/( const float a ) const;
	float			operator*( const anVec6 &a ) const;
	anVec6			operator-( const anVec6 &a ) const;
	anVec6			operator+( const anVec6 &a ) const;
	anVec6 &		operator*=( const float a );
	anVec6 &		operator/=( const float a );
	anVec6 &		operator+=( const anVec6 &a );
	anVec6 &		operator-=( const anVec6 &a );

	friend anVec6	operator*( const float a, const anVec6 b );

	bool			Compare( const anVec6 &a ) const;							// exact compare, no epsilon
	bool			Compare( const anVec6 &a, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const anVec6 &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const anVec6 &a ) const;						// exact compare, no epsilon

	float			Length( void ) const;
	float			LengthSqr( void ) const;
	float			Normalize( void );			// returns length
	float			NormalizeFast( void );		// returns length

	int				GetDimension( void ) const;

	const anVec3 &	SubVec3( int index ) const;
	anVec3 &		SubVec3( int index );
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

private:
	float			p[6];
};

extern anVec6 vec6_origin;
#define vec6_zero vec6_origin
extern anVec6 vec6_infinity;

ARC_INLINE anVec6::anVec6( void ) {
}

ARC_INLINE anVec6::anVec6( const float *a ) {
	memcpy( p, a, 6 * sizeof( float ) );
}

ARC_INLINE anVec6::anVec6( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 ) {
	p[0] = a1;
	p[1] = a2;
	p[2] = a3;
	p[3] = a4;
	p[4] = a5;
	p[5] = a6;
}

ARC_INLINE anVec6 anVec6::operator-() const {
	return anVec6( -p[0], -p[1], -p[2], -p[3], -p[4], -p[5] );
}

ARC_INLINE float anVec6::operator[]( const int index ) const {
	return p[index];
}

ARC_INLINE float &anVec6::operator[]( const int index ) {
	return p[index];
}

ARC_INLINE anVec6 anVec6::operator*( const float a ) const {
	return anVec6( p[0]*a, p[1]*a, p[2]*a, p[3]*a, p[4]*a, p[5]*a );
}

ARC_INLINE float anVec6::operator*( const anVec6 &a ) const {
	return p[0] * a[0] + p[1] * a[1] + p[2] * a[2] + p[3] * a[3] + p[4] * a[4] + p[5] * a[5];
}

ARC_INLINE anVec6 anVec6::operator/( const float a ) const {
	float inva;

	assert( a != 0.0f );
	inva = 1.0f / a;
	return anVec6( p[0]*inva, p[1]*inva, p[2]*inva, p[3]*inva, p[4]*inva, p[5]*inva );
}

ARC_INLINE anVec6 anVec6::operator+( const anVec6 &a ) const {
	return anVec6( p[0] + a[0], p[1] + a[1], p[2] + a[2], p[3] + a[3], p[4] + a[4], p[5] + a[5] );
}

ARC_INLINE anVec6 anVec6::operator-( const anVec6 &a ) const {
	return anVec6( p[0] - a[0], p[1] - a[1], p[2] - a[2], p[3] - a[3], p[4] - a[4], p[5] - a[5] );
}

ARC_INLINE anVec6 &anVec6::operator*=( const float a ) {
	p[0] *= a;
	p[1] *= a;
	p[2] *= a;
	p[3] *= a;
	p[4] *= a;
	p[5] *= a;
	return *this;
}

ARC_INLINE anVec6 &anVec6::operator/=( const float a ) {
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

ARC_INLINE anVec6 &anVec6::operator+=( const anVec6 &a ) {
	p[0] += a[0];
	p[1] += a[1];
	p[2] += a[2];
	p[3] += a[3];
	p[4] += a[4];
	p[5] += a[5];
	return *this;
}

ARC_INLINE anVec6 &anVec6::operator-=( const anVec6 &a ) {
	p[0] -= a[0];
	p[1] -= a[1];
	p[2] -= a[2];
	p[3] -= a[3];
	p[4] -= a[4];
	p[5] -= a[5];
	return *this;
}

ARC_INLINE anVec6 operator*( const float a, const anVec6 b ) {
	return b * a;
}

ARC_INLINE bool anVec6::Compare( const anVec6 &a ) const {
	return ( ( p[0] == a[0] ) && ( p[1] == a[1] ) && ( p[2] == a[2] ) &&
			( p[3] == a[3] ) && ( p[4] == a[4] ) && ( p[5] == a[5] ) );
}

ARC_INLINE bool anVec6::Compare( const anVec6 &a, const float epsilon ) const {
	if ( anMath::Fabs( p[0] - a[0] ) > epsilon ) {
		return false;
	}

	if ( anMath::Fabs( p[1] - a[1] ) > epsilon ) {
		return false;
	}

	if ( anMath::Fabs( p[2] - a[2] ) > epsilon ) {
		return false;
	}

	if ( anMath::Fabs( p[3] - a[3] ) > epsilon ) {
		return false;
	}

	if ( anMath::Fabs( p[4] - a[4] ) > epsilon ) {
		return false;
	}

	if ( anMath::Fabs( p[5] - a[5] ) > epsilon ) {
		return false;
	}

	return true;
}

ARC_INLINE bool anVec6::operator==( const anVec6 &a ) const {
	return Compare( a );
}

ARC_INLINE bool anVec6::operator!=( const anVec6 &a ) const {
	return !Compare( a );
}

ARC_INLINE void anVec6::Set( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 ) {
	p[0] = a1;
	p[1] = a2;
	p[2] = a3;
	p[3] = a4;
	p[4] = a5;
	p[5] = a6;
}

ARC_INLINE void anVec6::Zero( void ) {
	p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = 0.0f;
}

ARC_INLINE float anVec6::Length( void ) const {
	return ( float )anMath::Sqrt( p[0] * p[0] + p[1] * p[1] + p[2] * p[2] + p[3] * p[3] + p[4] * p[4] + p[5] * p[5] );
}

ARC_INLINE float anVec6::LengthSqr( void ) const {
	return ( p[0] * p[0] + p[1] * p[1] + p[2] * p[2] + p[3] * p[3] + p[4] * p[4] + p[5] * p[5] );
}

ARC_INLINE float anVec6::Normalize( void ) {
	float sqrLength, invLength;

	sqrLength = p[0] * p[0] + p[1] * p[1] + p[2] * p[2] + p[3] * p[3] + p[4] * p[4] + p[5] * p[5];
	invLength = anMath::InvSqrt( sqrLength );
	p[0] *= invLength;
	p[1] *= invLength;
	p[2] *= invLength;
	p[3] *= invLength;
	p[4] *= invLength;
	p[5] *= invLength;
	return invLength * sqrLength;
}

ARC_INLINE float anVec6::NormalizeFast( void ) {
	float sqrLength, invLength;

	sqrLength = p[0] * p[0] + p[1] * p[1] + p[2] * p[2] + p[3] * p[3] + p[4] * p[4] + p[5] * p[5];
	invLength = anMath::RSqrt( sqrLength );
	p[0] *= invLength;
	p[1] *= invLength;
	p[2] *= invLength;
	p[3] *= invLength;
	p[4] *= invLength;
	p[5] *= invLength;
	return invLength * sqrLength;
}

ARC_INLINE int anVec6::GetDimension( void ) const {
	return 6;
}

ARC_INLINE const anVec3 &anVec6::SubVec3( int index ) const {
	return *reinterpret_cast<const anVec3 *>(p + index * 3);
}

ARC_INLINE anVec3 &anVec6::SubVec3( int index ) {
	return *reinterpret_cast<anVec3 *>(p + index * 3);
}

ARC_INLINE const float *anVec6::ToFloatPtr( void ) const {
	return p;
}

ARC_INLINE float *anVec6::ToFloatPtr( void ) {
	return p;
}


//===============================================================
//
//	anVecX - arbitrary sized vector
//
//  The vector lives on 16 byte aligned and 16 byte padded memory.
//
//	NOTE: due to the temporary memory pool anVecX cannot be used by multiple threads
//
//===============================================================

#define VECX_MAX_TEMP		1024
#define VECX_QUAD( x )		( ( ( ( x ) + 3 ) & ~3 ) * sizeof( float ) )
#define VECX_CLEAREND()		int s = size; while( s < ( ( s + 3) & ~3 ) ) { p[s++] = 0.0f; }
#define VECX_ALLOCA( n )	( (float *) _alloca16( VECX_QUAD( n ) ) )
#define VECX_SIMD

class anVecX {
	friend class anMatX;

public:
					anVecX( void );
					explicit anVecX( int length );
					explicit anVecX( int length, float *data );
					~anVecX( void );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	anVecX			operator-() const;
	anVecX &		operator=( const anVecX &a );
	anVecX			operator*( const float a ) const;
	anVecX			operator/( const float a ) const;
	float			operator*( const anVecX &a ) const;
	anVecX			operator-( const anVecX &a ) const;
	anVecX			operator+( const anVecX &a ) const;
	anVecX &		operator*=( const float a );
	anVecX &		operator/=( const float a );
	anVecX &		operator+=( const anVecX &a );
	anVecX &		operator-=( const anVecX &a );

	friend anVecX	operator*( const float a, const anVecX b );

	bool			Compare( const anVecX &a ) const;							// exact compare, no epsilon
	bool			Compare( const anVecX &a, const float epsilon ) const;		// compare with epsilon
	bool			operator==(	const anVecX &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const anVecX &a ) const;						// exact compare, no epsilon

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
	anVecX &		SwapElements( int e1, int e2 );

	float			Length( void ) const;
	float			LengthSqr( void ) const;
	anVecX			Normalize( void ) const;
	float			NormalizeSelf( void );

	int				GetDimension( void ) const;

	const anVec3 &	SubVec3( int index ) const;
	anVec3 &		SubVec3( int index );
	const anVec6 &	SubVec6( int index ) const;
	anVec6 &		SubVec6( int index );
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


ARC_INLINE anVecX::anVecX( void ) {
	size = alloced = 0;
	p = NULL;
}

ARC_INLINE anVecX::anVecX( int length ) {
	size = alloced = 0;
	p = NULL;
	SetSize( length );
}

ARC_INLINE anVecX::anVecX( int length, float *data ) {
	size = alloced = 0;
	p = NULL;
	SetData( length, data );
}

ARC_INLINE anVecX::~anVecX( void ) {
	// if not temp memory
	if ( p && ( p < anVecX::tempPtr || p >= anVecX::tempPtr + VECX_MAX_TEMP ) && alloced != -1 ) {
		Mem_Free16( p );
	}
}

ARC_INLINE float anVecX::operator[]( const int index ) const {
	assert( index >= 0 && index < size );
	return p[index];
}

ARC_INLINE float &anVecX::operator[]( const int index ) {
	assert( index >= 0 && index < size );
	return p[index];
}

ARC_INLINE anVecX anVecX::operator-() const {
	int i;
	anVecX m;

	m.SetTempSize( size );
	for ( int i = 0; i < size; i++ ) {
		m.p[i] = -p[i];
	}
	return m;
}

ARC_INLINE anVecX &anVecX::operator=( const anVecX &a ) {
	SetSize( a.size );
#ifdef VECX_SIMD
	SIMDProcessor->Copy16( p, a.p, a.size );
#else
	memcpy( p, a.p, a.size * sizeof( float ) );
#endif
	anVecX::tempIndex = 0;
	return *this;
}

ARC_INLINE anVecX anVecX::operator+( const anVecX &a ) const {
	anVecX m;

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

ARC_INLINE anVecX anVecX::operator-( const anVecX &a ) const {
	anVecX m;

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

ARC_INLINE anVecX &anVecX::operator+=( const anVecX &a ) {
	assert( size == a.size );
#ifdef VECX_SIMD
	SIMDProcessor->AddAssign16( p, a.p, size );
#else
	for ( int i = 0; i < size; i++ ) {
		p[i] += a.p[i];
	}
#endif
	anVecX::tempIndex = 0;
	return *this;
}

ARC_INLINE anVecX &anVecX::operator-=( const anVecX &a ) {
	assert( size == a.size );
#ifdef VECX_SIMD
	SIMDProcessor->SubAssign16( p, a.p, size );
#else
	for ( int i = 0; i < size; i++ ) {
		p[i] -= a.p[i];
	}
#endif
	anVecX::tempIndex = 0;
	return *this;
}

ARC_INLINE anVecX anVecX::operator*( const float a ) const {
	anVecX m;

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

ARC_INLINE anVecX &anVecX::operator*=( const float a ) {
#ifdef VECX_SIMD
	SIMDProcessor->MulAssign16( p, a, size );
#else
	for ( int i = 0; i < size; i++ ) {
		p[i] *= a;
	}
#endif
	return *this;
}

ARC_INLINE anVecX anVecX::operator/( const float a ) const {
	assert( a != 0.0f );
	return (*this) * ( 1.0f / a );
}

ARC_INLINE anVecX &anVecX::operator/=( const float a ) {
	assert( a != 0.0f );
	(*this) *= ( 1.0f / a );
	return *this;
}

ARC_INLINE anVecX operator*( const float a, const anVecX b ) {
	return b * a;
}

ARC_INLINE float anVecX::operator*( const anVecX &a ) const {
	float sum = 0.0f;

	assert( size == a.size );
	for ( int i = 0; i < size; i++ ) {
		sum += p[i] * a.p[i];
	}
	return sum;
}

ARC_INLINE bool anVecX::Compare( const anVecX &a ) const {
	assert( size == a.size );
	for ( int i = 0; i < size; i++ ) {
		if ( p[i] != a.p[i] ) {
			return false;
		}
	}
	return true;
}

ARC_INLINE bool anVecX::Compare( const anVecX &a, const float epsilon ) const {
	assert( size == a.size );
	for ( int i = 0; i < size; i++ ) {
		if ( anMath::Fabs( p[i] - a.p[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

ARC_INLINE bool anVecX::operator==( const anVecX &a ) const {
	return Compare( a );
}

ARC_INLINE bool anVecX::operator!=( const anVecX &a ) const {
	return !Compare( a );
}

ARC_INLINE void anVecX::SetSize( int newSize ) {
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

ARC_INLINE void anVecX::ChangeSize( int newSize, bool makeZero ) {
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

ARC_INLINE void anVecX::SetTempSize( int newSize ) {
	size = newSize;
	alloced = ( newSize + 3 ) & ~3;
	assert( alloced < VECX_MAX_TEMP );
	if ( anVecX::tempIndex + alloced > VECX_MAX_TEMP ) {
		anVecX::tempIndex = 0;
	}
	p = anVecX::tempPtr + anVecX::tempIndex;
	anVecX::tempIndex += alloced;
	VECX_CLEAREND();
}

ARC_INLINE void anVecX::SetData( int length, float *data ) {
	if ( p && ( p < anVecX::tempPtr || p >= anVecX::tempPtr + VECX_MAX_TEMP ) && alloced != -1 ) {
		Mem_Free16( p );
	}
	assert( ( ( ( int ) data ) & 15 ) == 0 ); // data must be 16 byte aligned
	p = data;
	size = length;
	alloced = -1;
	VECX_CLEAREND();
}

ARC_INLINE void anVecX::Zero( void ) {
#ifdef VECX_SIMD
	SIMDProcessor->Zero16( p, size );
#else
	memset( p, 0, size * sizeof( float ) );
#endif
}

ARC_INLINE void anVecX::Zero( int length ) {
	SetSize( length );
#ifdef VECX_SIMD
	SIMDProcessor->Zero16( p, length );
#else
	memset( p, 0, size * sizeof( float ) );
#endif
}

ARC_INLINE void anVecX::Random( int seed, float l, float u ) {
	float c;
	arcRandom rnd( seed );

	c = u - l;
	for ( int i = 0; i < size; i++ ) {
		p[i] = l + rnd.RandomFloat() * c;
	}
}

ARC_INLINE void anVecX::Random( int length, int seed, float l, float u ) {
	arcRandom rnd( seed );

	SetSize( length );
	float c = u - l;
	for ( int i = 0; i < size; i++ ) {
		p[i] = l + rnd.RandomFloat() * c;
	}
}

ARC_INLINE void anVecX::Negate( void ) {
#ifdef VECX_SIMD
	SIMDProcessor->Negate16( p, size );
#else
	for ( int i = 0; i < size; i++ ) {
		p[i] = -p[i];
	}
#endif
}

ARC_INLINE void anVecX::Clamp( float min, float max ) {
	for ( int i = 0; i < size; i++ ) {
		if ( p[i] < min ) {
			p[i] = min;
		} else if ( p[i] > max ) {
			p[i] = max;
		}
	}
}

ARC_INLINE anVecX &anVecX::SwapElements( int e1, int e2 ) {
	float tmp = p[e1];
	p[e1] = p[e2];
	p[e2] = tmp;
	return *this;
}

ARC_INLINE float anVecX::Length( void ) const {
	float sum = 0.0f;

	for ( int i = 0; i < size; i++ ) {
		sum += p[i] * p[i];
	}
	return anMath::Sqrt( sum );
}

ARC_INLINE float anVecX::LengthSqr( void ) const {
	float sum = 0.0f;

	for ( int i = 0; i < size; i++ ) {
		sum += p[i] * p[i];
	}
	return sum;
}

ARC_INLINE anVecX anVecX::Normalize( void ) const {
	float invSqrt, sum = 0.0f;

	anVecX m.SetTempSize( size );
	for ( int i = 0; i < size; i++ ) {
		sum += p[i] * p[i];
	}
	invSqrt = anMath::InvSqrt( sum );
	for ( int i = 0; i < size; i++ ) {
		m.p[i] = p[i] * invSqrt;
	}
	return m;
}

ARC_INLINE float anVecX::NormalizeSelf( void ) {
	float invSqrt, sum = 0.0f;

	//invSqrt = anMath::InvSqrt( LengthSqr() );
	for ( int i = 0; i < size; i++ ) {
		sum += p[i] * p[i];
	}
	invSqrt = anMath::InvSqrt( sum );
	for ( int i = 0; i < size; i++ ) {
		p[i] *= invSqrt;
	}
	return invSqrt * sum;
}

ARC_INLINE int anVecX::GetDimension( void ) const {
	return size;
}

ARC_INLINE anVec3 &anVecX::SubVec3( int index ) {
	assert( index >= 0 && index * 3 + 3 <= size );
	return *reinterpret_cast<anVec3 *>( p + index * 3 );
}

ARC_INLINE const anVec3 &anVecX::SubVec3( int index ) const {
	assert( index >= 0 && index * 3 + 3 <= size );
	return *reinterpret_cast<const anVec3 *>(p + index * 3);
}

ARC_INLINE anVec6 &anVecX::SubVec6( int index ) {
	assert( index >= 0 && index * 6 + 6 <= size );
	return *reinterpret_cast<anVec6 *>( p + index * 6 );
}

ARC_INLINE const anVec6 &anVecX::SubVec6( int index ) const {
	assert( index >= 0 && index * 6 + 6 <= size );
	return *reinterpret_cast<const anVec6 *>( p + index * 6 );
}

ARC_INLINE const float *anVecX::ToFloatPtr( void ) const {
	return p;
}

ARC_INLINE float *anVecX::ToFloatPtr( void ) {
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

	anVec3			ToVec3( void ) const;
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

ARC_INLINE anVec3 idPolar3::ToVec3( void ) const {
	float sp, cp, st, ct;
	anMath::SinCos( phi, sp, cp );
	anMath::SinCos( theta, st, ct );
 	return anVec3( cp * radius * ct, cp * radius * st, radius * sp );
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
