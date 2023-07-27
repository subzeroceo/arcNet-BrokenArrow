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

class anVec3;
class anQuats;
class anRotation;
class anMat3;
class anMat4;

class arcAngles {
public:
	float			pitch;
	float			yaw;
	float			roll;

					arcAngles( void );
					arcAngles( float pitch, float yaw, float roll );
					explicit arcAngles( const anVec3 &v );
	float			AngleMod( float a );
	void 			Set( float pitch, float yaw, float roll );
	arcAngles &		Zero( void );

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	arcAngles		operator-() const;			// negate angles, in general not the inverse rotation
	arcAngles &		operator=( const arcAngles &a );
	arcAngles		operator+( const arcAngles &a ) const;
	arcAngles &		operator+=( const arcAngles &a );
	arcAngles		operator-( const arcAngles &a ) const;
	arcAngles &		operator-=( const arcAngles &a );
	arcAngles		operator*( const float a ) const;
	arcAngles &		operator*=( const float a );
	arcAngles		operator/( const float a ) const;
	arcAngles &		operator/=( const float a );

	friend arcAngles operator*( const float a, const arcAngles &b );

	bool			Compare( const arcAngles &a ) const;							// exact compare, no epsilon
	bool			Compare( const arcAngles &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const arcAngles &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const arcAngles &a ) const;						// exact compare, no epsilon

	arcAngles &		Normalize360( void );	// normalizes 'this'
	arcAngles &		Normalize180( void );	// normalizes 'this'

	static float 	LerpAngle( float from, float to, float frac );

	static float	AngleNormalize360( float angle );
	static float	AngleNormalize180( float angle );
	static float	AngleDelta( float angle1, float angle2 );

	static float	Subtract( float a1, float a2 );

	void			Clamp( const arcAngles &min, const arcAngles &max );

	int				GetDimension( void ) const;

	void			ToVectors( anVec3 *forward, anVec3 *right = NULL, anVec3 *up = NULL ) const;
	anVec3			ToForward( void ) const;
	anQuats		ToQuat( void ) const;
	anRotation ToRotation( void ) const;
	anMat3			ToMat3( void ) const;
	anMat4			ToMat4( void ) const;
	anVec3			ToAngularVelocity( void ) const;

	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;
};

extern arcAngles ang_zero;

ARC_INLINE arcAngles::arcAngles( void ) {
}

ARC_INLINE arcAngles::arcAngles( float pitch, float yaw, float roll ) {
	this->pitch = pitch;
	this->yaw	= yaw;
	this->roll	= roll;
}

ARC_INLINE arcAngles::arcAngles( const anVec3 &v ) {
	this->pitch = v[0];
	this->yaw	= v[1];
	this->roll	= v[2];
}

ARC_INLINE float arcAngles::AngleMod( float a ) {
	return( ( 360.0f / 65536 ) * ( ( int )( a * ( 65536 / 360.0f ) ) & 65535 ) );
}

ARC_INLINE void arcAngles::Set( float pitch, float yaw, float roll ) {
	this->pitch = pitch;
	this->yaw	= yaw;
	this->roll	= roll;
}

ARC_INLINE arcAngles &arcAngles::Zero( void ) {
	pitch = yaw = roll = 0.0f;
	return *this;
}

ARC_INLINE float arcAngles::AngleNormalize360( float angle ) {
	if ( ( angle >= 360.0f ) || ( angle < 0.0f ) ) {
		angle -= floor( angle / 360.0f ) * 360.0f;
	}
	return angle;
}

ARC_INLINE float arcAngles::AngleNormalize180( float angle ) {
	angle = AngleNormalize360( angle );
	if ( angle > 180.0f ) {
		angle -= 360.0f;
	}
	return angle;
}

ARC_INLINE float arcAngles::AngleDelta( float angle1, float angle2 ) {
	return AngleNormalize180( angle1 - angle2 );
}

ARC_INLINE float arcAngles::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( &pitch )[index];
}

ARC_INLINE float &arcAngles::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( &pitch )[index];
}

ARC_INLINE arcAngles arcAngles::operator-() const {
	return arcAngles( -pitch, -yaw, -roll );
}

ARC_INLINE arcAngles &arcAngles::operator=( const arcAngles &a ) {
	pitch	= a.pitch;
	yaw		= a.yaw;
	roll	= a.roll;
	return *this;
}

ARC_INLINE arcAngles arcAngles::operator+( const arcAngles &a ) const {
	return arcAngles( pitch + a.pitch, yaw + a.yaw, roll + a.roll );
}

ARC_INLINE arcAngles& arcAngles::operator+=( const arcAngles &a ) {
	pitch	+= a.pitch;
	yaw		+= a.yaw;
	roll	+= a.roll;

	return *this;
}

ARC_INLINE arcAngles arcAngles::operator-( const arcAngles &a ) const {
	return arcAngles( pitch - a.pitch, yaw - a.yaw, roll - a.roll );
}

ARC_INLINE arcAngles& arcAngles::operator-=( const arcAngles &a ) {
	pitch	-= a.pitch;
	yaw		-= a.yaw;
	roll	-= a.roll;

	return *this;
}

ARC_INLINE arcAngles arcAngles::operator*( const float a ) const {
	return arcAngles( pitch * a, yaw * a, roll * a );
}

ARC_INLINE arcAngles& arcAngles::operator*=( float a ) {
	pitch	*= a;
	yaw		*= a;
	roll	*= a;
	return *this;
}

ARC_INLINE arcAngles arcAngles::operator/( const float a ) const {
	float inva = 1.0f / a;
	return arcAngles( pitch * inva, yaw * inva, roll * inva );
}

ARC_INLINE arcAngles& arcAngles::operator/=( float a ) {
	float inva = 1.0f / a;
	pitch	*= inva;
	yaw		*= inva;
	roll	*= inva;
	return *this;
}

ARC_INLINE arcAngles operator*( const float a, const arcAngles &b ) {
	return arcAngles( a * b.pitch, a * b.yaw, a * b.roll );
}

ARC_INLINE bool arcAngles::Compare( const arcAngles &a ) const {
	return ( ( a.pitch == pitch ) && ( a.yaw == yaw ) && ( a.roll == roll ) );
}

ARC_INLINE bool arcAngles::Compare( const arcAngles &a, const float epsilon ) const {
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

ARC_INLINE bool arcAngles::operator==( const arcAngles &a ) const {
	return Compare( a );
}

ARC_INLINE bool arcAngles::operator!=( const arcAngles &a ) const {
	return !Compare( a );
}

ARC_INLINE void arcAngles::Clamp( const arcAngles &min, const arcAngles &max ) {
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

ARC_INLINE int arcAngles::GetDimension( void ) const {
	return 3;
}

ARC_INLINE const float *arcAngles::ToFloatPtr( void ) const {
	return &pitch;
}

ARC_INLINE float *arcAngles::ToFloatPtr( void ) {
	return &pitch;
}

#endif