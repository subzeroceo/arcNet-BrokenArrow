#ifndef __MATH_QUAT_H__
#define __MATH_QUAT_H__

/*
===============================================================================

	Quaternion

===============================================================================
*/


class anVec3;
class arcAngles;
class anRotation;
class anMat3;
class anMat4;
class arcCQuats;

class anQuats {
public:
	float			x;
	float			y;
	float			z;
	float			w;

					anQuats( void );
					anQuats( float x, float y, float z, float w );

	void 			Set( float x, float y, float z, float w );

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	anQuats		operator-() const;
	anQuats &		operator=( const anQuats &a );
	anQuats		operator+( const anQuats &a ) const;
	anQuats &		operator+=( const anQuats &a );
	anQuats		operator-( const anQuats &a ) const;
	anQuats &		operator-=( const anQuats &a );
	anQuats		operator*( const anQuats &a ) const;
	anVec3			operator*( const anVec3 &a ) const;
	anQuats		operator*( float a ) const;
	anQuats &		operator*=( const anQuats &a );
	anQuats &		operator*=( float a );

	friend anQuats	operator*( const float a, const anQuats &b );
	friend anVec3	operator*( const anVec3 &a, const anQuats &b );

	bool			Compare( const anQuats &a ) const;						// exact compare, no epsilon
	bool			Compare( const anQuats &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const anQuats &a ) const;					// exact compare, no epsilon
	bool			operator!=(	const anQuats &a ) const;					// exact compare, no epsilon

	anQuats		Inverse( void ) const;
	float			Length( void ) const;
	anQuats &		Normalize( void );

	float			CalcW( void ) const;
	int				GetDimension( void ) const;

	arcAngles		ToAngles( void ) const;
	anRotation		ToRotation( void ) const;
	anVec3			AngleTo( const anQuats &ang ) const;
	anMat3			ToMat3( void ) const;
	anMat4			ToMat4( void ) const;
	arcCQuats		ToCQuat( void ) const;
	anVec3			ToAngularVelocity( void ) const;
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

	anQuats &		Slerp( const anQuats &from, const anQuats &to, float t );

	arcCQuats 			Exp() const;
	arcCQuats			Log() const;
	arcCQuats			SphericalCubicInterpolate( const arcCQuats &bb, const arcCQuats &pa, const arcCQuats &pb, const anVec3 &weight ) const;
};

ARC_INLINE anQuats::anQuats( void ) {
}

ARC_INLINE anQuats::anQuats( float x, float y, float z, float w ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

ARC_INLINE float anQuats::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index < 4 ) );
	return ( &x )[index];
}

ARC_INLINE float& anQuats::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index < 4 ) );
	return ( &x )[index];
}

ARC_INLINE anQuats anQuats::operator-() const {
	return anQuats( -x, -y, -z, -w );
}

ARC_INLINE anQuats &anQuats::operator=( const anQuats &a ) {
	x = a.x;
	y = a.y;
	z = a.z;
	w = a.w;

	return *this;
}

ARC_INLINE anQuats anQuats::operator+( const anQuats &a ) const {
	return anQuats( x + a.x, y + a.y, z + a.z, w + a.w );
}

ARC_INLINE anQuats& anQuats::operator+=( const anQuats &a ) {
	x += a.x;
	y += a.y;
	z += a.z;
	w += a.w;

	return *this;
}

ARC_INLINE anQuats anQuats::operator-( const anQuats &a ) const {
	return anQuats( x - a.x, y - a.y, z - a.z, w - a.w );
}

ARC_INLINE anQuats& anQuats::operator-=( const anQuats &a ) {
	x -= a.x;
	y -= a.y;
	z -= a.z;
	w -= a.w;

	return *this;
}

ARC_INLINE anQuats anQuats::operator*( const anQuats &a ) const {
	return anQuats( w*a.x + x*a.w + y*a.z - z*a.y,
					w*a.y + y*a.w + z*a.x - x*a.z,
					w*a.z + z*a.w + x*a.y - y*a.x,
					w*a.w - x*a.x - y*a.y - z*a.z );
}

ARC_INLINE anVec3 anQuats::operator*( const anVec3 &a ) const {
#if 0
	// it's faster to do the conversion to a 3x3 matrix and multiply the vector by this 3x3 matrix
	return ( ToMat3() * a );
#else
	// result = this->Inverse() * anQuats( a.x, a.y, a.z, 0.0f ) * (*this)
	float xxzz = x*x - z*z;
	float wwyy = w*w - y*y;

	float xw2 = x*w*2.0f;
	float xy2 = x*y*2.0f;
	float xz2 = x*z*2.0f;
	float yw2 = y*w*2.0f;
	float yz2 = y*z*2.0f;
	float zw2 = z*w*2.0f;

	return anVec3(
		( xxzz + wwyy )*a.x	+ ( xy2 + zw2 )*a.y		+ ( xz2 - yw2 )*a.z,
		( xy2 - zw2 )*a.x		+ ( y*y+w*w-x*x-z*z )*a.y	+ ( yz2 + xw2 )*a.z,
		( xz2 + yw2 )*a.x		+ ( yz2 - xw2 )*a.y		+ ( wwyy - xxzz )*a.z
	);
#endif
}

ARC_INLINE anQuats anQuats::operator*( float a ) const {
	return anQuats( x * a, y * a, z * a, w * a );
}

ARC_INLINE anQuats operator*( const float a, const anQuats &b ) {
	return b * a;
}

ARC_INLINE anVec3 operator*( const anVec3 &a, const anQuats &b ) {
	return b * a;
}

ARC_INLINE anQuats& anQuats::operator*=( const anQuats &a ) {
	*this = *this * a;

	return *this;
}

ARC_INLINE anQuats& anQuats::operator*=( float a ) {
	x *= a;
	y *= a;
	z *= a;
	w *= a;

	return *this;
}

ARC_INLINE bool anQuats::Compare( const anQuats &a ) const {
	return ( ( x == a.x ) && ( y == a.y ) && ( z == a.z ) && ( w == a.w ) );
}

ARC_INLINE bool anQuats::Compare( const anQuats &a, const float epsilon ) const {
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

ARC_INLINE bool anQuats::operator==( const anQuats &a ) const {
	return Compare( a );
}

ARC_INLINE bool anQuats::operator!=( const anQuats &a ) const {
	return !Compare( a );
}

ARC_INLINE void anQuats::Set( float x, float y, float z, float w ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

ARC_INLINE anQuats anQuats::Inverse( void ) const {
	return anQuats( -x, -y, -z, w );
}

ARC_INLINE float anQuats::Length( void ) const {
	float len;

	len = x * x + y * y + z * z + w * w;
	return anMath::Sqrt( len );
}

ARC_INLINE anQuats& anQuats::Normalize( void ) {
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

ARC_INLINE float anQuats::CalcW( void ) const {
	// take the absolute value because floating point rounding may cause the dot of x,y,z to be larger than 1
	return sqrt( fabs( 1.0f - ( x * x + y * y + z * z ) ) );
}

ARC_INLINE int anQuats::GetDimension( void ) const {
	return 4;
}

ARC_INLINE const float *anQuats::ToFloatPtr( void ) const {
	return &x;
}

ARC_INLINE float *anQuats::ToFloatPtr( void ) {
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
	anRotation		ToRotation( void ) const;
	anMat3			ToMat3( void ) const;
	anMat4			ToMat4( void ) const;
	anQuats			ToQuat( void ) const;
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

ARC_INLINE bool arcCQuats::operator==( const arcCQuats &a ) const {
	return Compare( a );
}

ARC_INLINE bool arcCQuats::operator!=( const arcCQuats &a ) const {
	return !Compare( a );
}

ARC_INLINE int arcCQuats::GetDimension( void ) const {
	return 3;
}

ARC_INLINE anQuats arcCQuats::ToQuat( void ) const {
	// take the absolute value because floating point rounding may cause the dot of x,y,z to be larger than 1
	return anQuats( x, y, z, sqrt( fabs( 1.0f - ( x * x + y * y + z * z ) ) ) );
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