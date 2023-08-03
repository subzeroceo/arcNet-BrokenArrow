#include "../Lib.h"
#ifndef __MATH_ANGLES_H__
#define __MATH_ANGLES_H__

/*
===============================================================================

	Euler angles

===============================================================================
*/

// angle indexes
#include <cstddef>
#define	PITCH				0		// up / down
#define	YAW					1		// left / right
#define	ROLL				2		// fall over
const int A_ROLL			= 0;
const int A_PITCH			= 1;
const int A_YAW				= 2;

class anMath;
class anVec3;
class anQuats;
class anRotation;
class anMat3;
class anMat4;

class anAngles {
public:
	float			pitch;
	float			yaw;
	float			roll;

					anAngles( void );
					anAngles( float pitch, float yaw, float roll );
					explicit anAngles( const anVec3 &v );
	float			AngleMod( float a );
	void 			Set( float pitch, float yaw, float roll );
	anAngles &		Zero( void );

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	anAngles		operator-() const;			// negate angles, in general not the inverse rotation
	anAngles &		operator=( const anAngles &a );
	anAngles		operator+( const anAngles &a ) const;
	anAngles &		operator+=( const anAngles &a );
	anAngles		operator-( const anAngles &a ) const;
	anAngles &		operator-=( const anAngles &a );
	anAngles		operator*( const float a ) const;
	anAngles &		operator*=( const float a );
	anAngles		operator/( const float a ) const;
	anAngles &		operator/=( const float a );

	friend anAngles operator*( const float a, const anAngles &b );

	bool			Compare( const anAngles &a ) const;							// exact compare, no epsilon
	bool			Compare( const anAngles &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const anAngles &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const anAngles &a ) const;						// exact compare, no epsilon

	anAngles &		Normalize360( void );	// normalizes 'this'
	anAngles &		Normalize180( void );	// normalizes 'this'

	static float 	LerpAngle( float from, float to, float frac );

	static float	AngleNormalize360( float angle );
	static float	AngleNormalize180( float angle );
	static float	AngleDelta( float angle1, float angle2 );

	static float	Subtract( float a1, float a2 );

	void			Clamp( const anAngles &min, const anAngles &max );

	int				GetDimension( void ) const;

	void			ToVectors( anVec3 *forward, anVec3 *right = nullptr, anVec3 *up = nullptr ) const;
	anVec3			ToForward( void ) const;
	anQuats			ToQuat( void ) const;
	anRotation 		ToRotation( void ) const;
	anMat3			ToMat3( void ) const;

	void			ToMat3NoRoll( anMat3 &mat ) const;
	static anMat3 &	YawToMat3( float yaw, anMat3 &mat );
	static anMat3 &	PitchToMat3( float pitch, anMat3 &mat );
	static anMat3 &	RollToMat3( float roll, anMat3 &mat );

	anMat4			ToMat4( void ) const;
	anVec3			ToAngularVelocity( void ) const;

	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

	anMat3			ToMat3Maya( void ) const;

	bool			FixDenormals( float epsilon = anMath::FLT_EPSILON );
};

extern anAngles ang_zero;

ARC_INLINE anAngles::anAngles( void ) {
}

ARC_INLINE anAngles::anAngles( float pitch, float yaw, float roll ) {
	this->pitch = pitch;
	this->yaw	= yaw;
	this->roll	= roll;
}

ARC_INLINE anAngles::anAngles( const anVec3 &v ) {
	this->pitch = v[0];
	this->yaw	= v[1];
	this->roll	= v[2];
}

ARC_INLINE float anAngles::AngleMod( float a ) {
	return( ( 360.0f / 65536 ) * ( ( int )( a * ( 65536 / 360.0f ) ) & 65535 ) );
}

ARC_INLINE void anAngles::Set( float pitch, float yaw, float roll ) {
	this->pitch = pitch;
	this->yaw	= yaw;
	this->roll	= roll;
}

ARC_INLINE anAngles &anAngles::Zero( void ) {
	pitch = yaw = roll = 0.0f;
	return *this;
}

ARC_INLINE float anAngles::AngleNormalize360( float angle ) {
	if ( ( angle >= 360.0f ) || ( angle < 0.0f ) ) {
		angle -= floor( angle / 360.0f ) * 360.0f;
	}
	return angle;
}

ARC_INLINE float anAngles::AngleNormalize180( float angle ) {
	angle = AngleNormalize360( angle );
	if ( angle > 180.0f ) {
		angle -= 360.0f;
	}
	return angle;
}

ARC_INLINE float anAngles::AngleDelta( float angle1, float angle2 ) {
	return AngleNormalize180( angle1 - angle2 );
}

ARC_INLINE float anAngles::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( &pitch )[index];
}

ARC_INLINE float &anAngles::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( &pitch )[index];
}

ARC_INLINE anAngles anAngles::operator-() const {
	return anAngles( -pitch, -yaw, -roll );
}

ARC_INLINE anAngles &anAngles::operator=( const anAngles &a ) {
	pitch	= a.pitch;
	yaw		= a.yaw;
	roll	= a.roll;
	return *this;
}

ARC_INLINE anAngles anAngles::operator+( const anAngles &a ) const {
	return anAngles( pitch + a.pitch, yaw + a.yaw, roll + a.roll );
}

ARC_INLINE anAngles& anAngles::operator+=( const anAngles &a ) {
	pitch	+= a.pitch;
	yaw		+= a.yaw;
	roll	+= a.roll;

	return *this;
}

ARC_INLINE anAngles anAngles::operator-( const anAngles &a ) const {
	return anAngles( pitch - a.pitch, yaw - a.yaw, roll - a.roll );
}

ARC_INLINE anAngles& anAngles::operator-=( const anAngles &a ) {
	pitch	-= a.pitch;
	yaw		-= a.yaw;
	roll	-= a.roll;

	return *this;
}

ARC_INLINE anAngles anAngles::operator*( const float a ) const {
	return anAngles( pitch * a, yaw * a, roll * a );
}

ARC_INLINE anAngles& anAngles::operator*=( float a ) {
	pitch	*= a;
	yaw		*= a;
	roll	*= a;
	return *this;
}

ARC_INLINE anAngles anAngles::operator/( const float a ) const {
	float inva = 1.0f / a;
	return anAngles( pitch * inva, yaw * inva, roll * inva );
}

ARC_INLINE anAngles& anAngles::operator/=( float a ) {
	float inva = 1.0f / a;
	pitch	*= inva;
	yaw		*= inva;
	roll	*= inva;
	return *this;
}

ARC_INLINE anAngles operator*( const float a, const anAngles &b ) {
	return anAngles( a * b.pitch, a * b.yaw, a * b.roll );
}

ARC_INLINE bool anAngles::Compare( const anAngles &a ) const {
	return ( ( a.pitch == pitch ) && ( a.yaw == yaw ) && ( a.roll == roll ) );
}

ARC_INLINE bool anAngles::Compare( const anAngles &a, const float epsilon ) const {
	if ( anMath::Fabs( pitch - a.pitch ) > epsilon ) {
		return false;
	}

	if ( anMath::Fabs( yaw - a.yaw ) > epsilon ) {
		return false;
	}

	if ( anMath::Fabs( roll - a.roll ) > epsilon ) {
		return false;
	}

	return true;
}

ARC_INLINE bool anAngles::operator==( const anAngles &a ) const {
	return Compare( a );
}

ARC_INLINE bool anAngles::operator!=( const anAngles &a ) const {
	return !Compare( a );
}

ARC_INLINE void anAngles::Clamp( const anAngles &min, const anAngles &max ) {
	if ( pitch < min.pitch ) {
		pitch = min.pitch;
	} else if ( pitch > max.pitch ) {
		pitch = max.pitch;
	}
	if ( yaw < min.yaw ) {
		yaw = min.yaw;
	} else if ( yaw > max.yaw ) {
		yaw = max.yaw;
	}
	if ( roll < min.roll ) {
		roll = min.roll;
	} else if ( roll > max.roll ) {
		roll = max.roll;
	}
}

ARC_INLINE int anAngles::GetDimension( void ) const {
	return 3;
}

ARC_INLINE const float *anAngles::ToFloatPtr( void ) const {
	return &pitch;
}

ARC_INLINE float *anAngles::ToFloatPtr( void ) {
	return &pitch;
}

ARC_INLINE anMat3 &anAngles::YawToMat3( float yaw, anMat3 &mat ) {
	float sy, cy;

	anMath::SinCos( DEG2RAD( yaw ), sy, cy );

	mat[ 0 ].Set( cy, sy, 0 );
	mat[ 1 ].Set( -sy, cy, 0 );
	mat[ 2 ].Set( 0, 0, 1 );

	return mat;
}

ARC_INLINE anMat3 &anAngles::PitchToMat3( float pitch, anMat3 &mat ) {
	float sp, cp;

	anMath::SinCos( DEG2RAD( pitch ), sp, cp );

	mat[ 0 ].Set( cp, 0, -sp );
	mat[ 1 ].Set( 0, 1, 0 );
	mat[ 2 ].Set( sp, 0, cp );

	return mat;
}

ARC_INLINE anMat3 &anAngles::RollToMat3( float roll, anMat3 &mat ) {
	float sr, cr;

	anMath::SinCos( DEG2RAD( roll ), sr, cr );

	mat[ 0 ].Set( 1, 0, 0 );
	mat[ 1 ].Set( 0, cr, sr );
	mat[ 2 ].Set( 0, -sr, cr );

	return mat;
}

ARC_INLINE bool anAngles::FixDenormals( float epsilon ) {
	bool denormal = false;
	if ( fabs( yaw ) < epsilon ) {
		yaw = 0.0f;
		denormal = true;
	}
	if ( fabs( pitch ) < epsilon ) {
		pitch = 0.0f;
		denormal = true;
	}
	if ( fabs( roll ) < epsilon ) {
		roll = 0.0f;
		denormal = true;
	}
	return denormal;
}
#endif