#ifndef __MATH_QUAT_H__
#define __MATH_QUAT_H__

/*
===============================================================================

	Quaternion

===============================================================================
*/


class arcVec3;
class arcAngles;
class arcRotate;
class arcMat3;
class arcMat4;
class arcCQuats;

class arcQuats {
public:
	float			x;
	float			y;
	float			z;
	float			w;

					arcQuats( void );
					arcQuats( float x, float y, float z, float w );

	void 			Set( float x, float y, float z, float w );

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	arcQuats		operator-() const;
	arcQuats &		operator=( const arcQuats &a );
	arcQuats		operator+( const arcQuats &a ) const;
	arcQuats &		operator+=( const arcQuats &a );
	arcQuats		operator-( const arcQuats &a ) const;
	arcQuats &		operator-=( const arcQuats &a );
	arcQuats		operator*( const arcQuats &a ) const;
	arcVec3			operator*( const arcVec3 &a ) const;
	arcQuats		operator*( float a ) const;
	arcQuats &		operator*=( const arcQuats &a );
	arcQuats &		operator*=( float a );

	friend arcQuats	operator*( const float a, const arcQuats &b );
	friend arcVec3	operator*( const arcVec3 &a, const arcQuats &b );

	bool			Compare( const arcQuats &a ) const;						// exact compare, no epsilon
	bool			Compare( const arcQuats &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const arcQuats &a ) const;					// exact compare, no epsilon
	bool			operator!=(	const arcQuats &a ) const;					// exact compare, no epsilon

	arcQuats		Inverse( void ) const;
	float			Length( void ) const;
	arcQuats &		Normalize( void );

	float			CalcW( void ) const;
	int				GetDimension( void ) const;

	arcAngles		ToAngles( void ) const;
	arcRotate		ToRotation( void ) const;
	arcVec3			AngleTo( const arcQuats &ang ) const;
	arcMat3			ToMat3( void ) const;
	arcMat4			ToMat4( void ) const;
	arcCQuats		ToCQuat( void ) const;
	arcVec3			ToAngularVelocity( void ) const;
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

	arcQuats &		Slerp( const arcQuats &from, const arcQuats &to, float t );

	arcCQuats 			Exp() const;
	arcCQuats			Log() const;
	arcCQuats			SphericalCubicInterpolate( const arcCQuats &bb, const arcCQuats &pa, const arcCQuats &pb, const arcVec3 &weight ) const;
};

ARC_INLINE arcQuats::arcQuats( void ) {
}

ARC_INLINE arcQuats::arcQuats( float x, float y, float z, float w ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

ARC_INLINE float arcQuats::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index < 4 ) );
	return ( &x )[index];
}

ARC_INLINE float& arcQuats::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index < 4 ) );
	return ( &x )[index];
}

ARC_INLINE arcQuats arcQuats::operator-() const {
	return arcQuats( -x, -y, -z, -w );
}

ARC_INLINE arcQuats &arcQuats::operator=( const arcQuats &a ) {
	x = a.x;
	y = a.y;
	z = a.z;
	w = a.w;

	return *this;
}

ARC_INLINE arcQuats arcQuats::operator+( const arcQuats &a ) const {
	return arcQuats( x + a.x, y + a.y, z + a.z, w + a.w );
}

ARC_INLINE arcQuats& arcQuats::operator+=( const arcQuats &a ) {
	x += a.x;
	y += a.y;
	z += a.z;
	w += a.w;

	return *this;
}

ARC_INLINE arcQuats arcQuats::operator-( const arcQuats &a ) const {
	return arcQuats( x - a.x, y - a.y, z - a.z, w - a.w );
}

ARC_INLINE arcQuats& arcQuats::operator-=( const arcQuats &a ) {
	x -= a.x;
	y -= a.y;
	z -= a.z;
	w -= a.w;

	return *this;
}

ARC_INLINE arcQuats arcQuats::operator*( const arcQuats &a ) const {
	return arcQuats( w*a.x + x*a.w + y*a.z - z*a.y,
					w*a.y + y*a.w + z*a.x - x*a.z,
					w*a.z + z*a.w + x*a.y - y*a.x,
					w*a.w - x*a.x - y*a.y - z*a.z );
}

ARC_INLINE arcVec3 arcQuats::operator*( const arcVec3 &a ) const {
#if 0
	// it's faster to do the conversion to a 3x3 matrix and multiply the vector by this 3x3 matrix
	return ( ToMat3() * a );
#else
	// result = this->Inverse() * arcQuats( a.x, a.y, a.z, 0.0f ) * (*this)
	float xxzz = x*x - z*z;
	float wwyy = w*w - y*y;

	float xw2 = x*w*2.0f;
	float xy2 = x*y*2.0f;
	float xz2 = x*z*2.0f;
	float yw2 = y*w*2.0f;
	float yz2 = y*z*2.0f;
	float zw2 = z*w*2.0f;

	return arcVec3(
		( xxzz + wwyy )*a.x	+ ( xy2 + zw2 )*a.y		+ ( xz2 - yw2 )*a.z,
		( xy2 - zw2 )*a.x		+ ( y*y+w*w-x*x-z*z )*a.y	+ ( yz2 + xw2 )*a.z,
		( xz2 + yw2 )*a.x		+ ( yz2 - xw2 )*a.y		+ ( wwyy - xxzz )*a.z
	);
#endif
}

ARC_INLINE arcQuats arcQuats::operator*( float a ) const {
	return arcQuats( x * a, y * a, z * a, w * a );
}

ARC_INLINE arcQuats operator*( const float a, const arcQuats &b ) {
	return b * a;
}

ARC_INLINE arcVec3 operator*( const arcVec3 &a, const arcQuats &b ) {
	return b * a;
}

ARC_INLINE arcQuats& arcQuats::operator*=( const arcQuats &a ) {
	*this = *this * a;

	return *this;
}

ARC_INLINE arcQuats& arcQuats::operator*=( float a ) {
	x *= a;
	y *= a;
	z *= a;
	w *= a;

	return *this;
}

ARC_INLINE bool arcQuats::Compare( const arcQuats &a ) const {
	return ( ( x == a.x ) && ( y == a.y ) && ( z == a.z ) && ( w == a.w ) );
}

ARC_INLINE bool arcQuats::Compare( const arcQuats &a, const float epsilon ) const {
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

ARC_INLINE bool arcQuats::operator==( const arcQuats &a ) const {
	return Compare( a );
}

ARC_INLINE bool arcQuats::operator!=( const arcQuats &a ) const {
	return !Compare( a );
}

ARC_INLINE void arcQuats::Set( float x, float y, float z, float w ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

ARC_INLINE arcQuats arcQuats::Inverse( void ) const {
	return arcQuats( -x, -y, -z, w );
}

ARC_INLINE float arcQuats::Length( void ) const {
	float len;

	len = x * x + y * y + z * z + w * w;
	return arcMath::Sqrt( len );
}

ARC_INLINE arcQuats& arcQuats::Normalize( void ) {
	float len;
	float ilength;

	len = this->Length();
	if ( len ) {
		ilength = 1 / len;
		x *= ilength;
		y *= ilength;
		z *= ilength;
		w *= ilength;
	}
	return *this;
}

ARC_INLINE float arcQuats::CalcW( void ) const {
	// take the absolute value because floating point rounding may cause the dot of x,y,z to be larger than 1
	return sqrt( fabs( 1.0f - ( x * x + y * y + z * z ) ) );
}

ARC_INLINE int arcQuats::GetDimension( void ) const {
	return 4;
}

ARC_INLINE const float *arcQuats::ToFloatPtr( void ) const {
	return &x;
}

ARC_INLINE float *arcQuats::ToFloatPtr( void ) {
	return &x;
}

/*
===============================================================================

	Compressed quaternion

===============================================================================
*/

class arcCQuats {
public:
	float			x;
	float			y;
	float			z;

					arcCQuats( void );
					arcCQuats( float x, float y, float z );

	void 			Set( float x, float y, float z );

	float			operator[]( int index ) const;
	float &			operator[]( int index );

	bool			Compare( const arcCQuats &a ) const;						// exact compare, no epsilon
	bool			Compare( const arcCQuats &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const arcCQuats &a ) const;					// exact compare, no epsilon
	bool			operator!=(	const arcCQuats &a ) const;					// exact compare, no epsilon

	int				GetDimension( void ) const;

	arcAngles		ToAngles( void ) const;
	arcRotate		ToRotation( void ) const;
	arcMat3			ToMat3( void ) const;
	arcMat4			ToMat4( void ) const;
	arcQuats			ToQuat( void ) const;
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;
};

ARC_INLINE arcCQuats::arcCQuats( void ) {
}

ARC_INLINE arcCQuats::arcCQuats( float x, float y, float z ) {
	this->x = x;
	this->y = y;
	this->z = z;
}

ARC_INLINE void arcCQuats::Set( float x, float y, float z ) {
	this->x = x;
	this->y = y;
	this->z = z;
}

ARC_INLINE float arcCQuats::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( &x )[index];
}

ARC_INLINE float& arcCQuats::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index < 3 ) );
	return ( &x )[index];
}

ARC_INLINE bool arcCQuats::Compare( const arcCQuats &a ) const {
	return ( ( x == a.x ) && ( y == a.y ) && ( z == a.z ) );
}

ARC_INLINE bool arcCQuats::Compare( const arcCQuats &a, const float epsilon ) const {
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

ARC_INLINE bool arcCQuats::operator==( const arcCQuats &a ) const {
	return Compare( a );
}

ARC_INLINE bool arcCQuats::operator!=( const arcCQuats &a ) const {
	return !Compare( a );
}

ARC_INLINE int arcCQuats::GetDimension( void ) const {
	return 3;
}

ARC_INLINE arcQuats arcCQuats::ToQuat( void ) const {
	// take the absolute value because floating point rounding may cause the dot of x,y,z to be larger than 1
	return arcQuats( x, y, z, sqrt( fabs( 1.0f - ( x * x + y * y + z * z ) ) ) );
}

ARC_INLINE const float *arcCQuats::ToFloatPtr( void ) const {
	return &x;
}

ARC_INLINE float *arcCQuats::ToFloatPtr( void ) {
	return &x;
}

/*
===============================================================================

	Specialization to get size of an arcCQuats generically.

===============================================================================
*/
template<>
struct arcTupleSize< arcCQuat > {
	enum { value = 3 };
};


#endif // !__MATH_QUAT_H__