#ifndef __MATH_PLUECKER_H__
#define __MATH_PLUECKER_H__

/*
===============================================================================

	Pluecker coordinate

===============================================================================
*/

class arcPluecker {
public:
					arcPluecker( void );
					explicit arcPluecker( const float *a );
					explicit arcPluecker( const anVec3 &start, const anVec3 &end );
					explicit arcPluecker( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	arcPluecker		operator-() const;											// flips the direction
	arcPluecker		operator*( const float a ) const;
	arcPluecker		operator/( const float a ) const;
	float			operator*( const arcPluecker &a ) const;						// permuted inner product
	arcPluecker		operator-( const arcPluecker &a ) const;
	arcPluecker		operator+( const arcPluecker &a ) const;
	arcPluecker &	operator*=( const float a );
	arcPluecker &	operator/=( const float a );
	arcPluecker &	operator+=( const arcPluecker &a );
	arcPluecker &	operator-=( const arcPluecker &a );

	bool			Compare( const arcPluecker &a ) const;						// exact compare, no epsilon
	bool			Compare( const arcPluecker &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const arcPluecker &a ) const;					// exact compare, no epsilon
	bool			operator!=(	const arcPluecker &a ) const;					// exact compare, no epsilon

	void 			Set( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 );
	void			Zero( void );

	void			FromLine( const anVec3 &start, const anVec3 &end );			// pluecker from line
	void			FromRay( const anVec3 &start, const anVec3 &dir );			// pluecker from ray
	bool			FromPlanes( const anPlane &p1, const anPlane &p2 );			// pluecker from intersection of planes
	bool			ToLine( anVec3 &start, anVec3 &end ) const;					// pluecker to line
	bool			ToRay( anVec3 &start, anVec3 &dir ) const;					// pluecker to ray
	void			ToDir( anVec3 &dir ) const;									// pluecker to direction
	float			PermutedInnerProduct( const arcPluecker &a ) const;			// pluecker permuted inner product
	float			Distance3DSqr( const arcPluecker &a ) const;					// pluecker line distance

	float			Length( void ) const;										// pluecker length
	float			LengthSqr( void ) const;									// pluecker squared length
	arcPluecker		Normalize( void ) const;									// pluecker normalize
	float			NormalizeSelf( void );										// pluecker normalize

	int				GetDimension( void ) const;

	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

private:
	float			p[6];
};

extern arcPluecker pluecker_origin;
#define pluecker_zero pluecker_origin

ARC_INLINE arcPluecker::arcPluecker( void ) {
}

ARC_INLINE arcPluecker::arcPluecker( const float *a ) {
	memcpy( p, a, 6 * sizeof( float ) );
}

ARC_INLINE arcPluecker::arcPluecker( const anVec3 &start, const anVec3 &end ) {
	FromLine( start, end );
}

ARC_INLINE arcPluecker::arcPluecker( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 ) {
	p[0] = a1;
	p[1] = a2;
	p[2] = a3;
	p[3] = a4;
	p[4] = a5;
	p[5] = a6;
}

ARC_INLINE arcPluecker arcPluecker::operator-() const {
	return arcPluecker( -p[0], -p[1], -p[2], -p[3], -p[4], -p[5] );
}

ARC_INLINE float arcPluecker::operator[]( const int index ) const {
	return p[index];
}

ARC_INLINE float &arcPluecker::operator[]( const int index ) {
	return p[index];
}

ARC_INLINE arcPluecker arcPluecker::operator*( const float a ) const {
	return arcPluecker( p[0]*a, p[1]*a, p[2]*a, p[3]*a, p[4]*a, p[5]*a );
}

ARC_INLINE float arcPluecker::operator*( const arcPluecker &a ) const {
	return p[0] * a.p[4] + p[1] * a.p[5] + p[2] * a.p[3] + p[4] * a.p[0] + p[5] * a.p[1] + p[3] * a.p[2];
}

ARC_INLINE arcPluecker arcPluecker::operator/( const float a ) const {
	float inva;

	assert( a != 0.0f );
	inva = 1.0f / a;
	return arcPluecker( p[0]*inva, p[1]*inva, p[2]*inva, p[3]*inva, p[4]*inva, p[5]*inva );
}

ARC_INLINE arcPluecker arcPluecker::operator+( const arcPluecker &a ) const {
	return arcPluecker( p[0] + a[0], p[1] + a[1], p[2] + a[2], p[3] + a[3], p[4] + a[4], p[5] + a[5] );
}

ARC_INLINE arcPluecker arcPluecker::operator-( const arcPluecker &a ) const {
	return arcPluecker( p[0] - a[0], p[1] - a[1], p[2] - a[2], p[3] - a[3], p[4] - a[4], p[5] - a[5] );
}

ARC_INLINE arcPluecker &arcPluecker::operator*=( const float a ) {
	p[0] *= a;
	p[1] *= a;
	p[2] *= a;
	p[3] *= a;
	p[4] *= a;
	p[5] *= a;
	return *this;
}

ARC_INLINE arcPluecker &arcPluecker::operator/=( const float a ) {
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

ARC_INLINE arcPluecker &arcPluecker::operator+=( const arcPluecker &a ) {
	p[0] += a[0];
	p[1] += a[1];
	p[2] += a[2];
	p[3] += a[3];
	p[4] += a[4];
	p[5] += a[5];
	return *this;
}

ARC_INLINE arcPluecker &arcPluecker::operator-=( const arcPluecker &a ) {
	p[0] -= a[0];
	p[1] -= a[1];
	p[2] -= a[2];
	p[3] -= a[3];
	p[4] -= a[4];
	p[5] -= a[5];
	return *this;
}

ARC_INLINE bool arcPluecker::Compare( const arcPluecker &a ) const {
	return ( ( p[0] == a[0] ) && ( p[1] == a[1] ) && ( p[2] == a[2] ) &&
			( p[3] == a[3] ) && ( p[4] == a[4] ) && ( p[5] == a[5] ) );
}

ARC_INLINE bool arcPluecker::Compare( const arcPluecker &a, const float epsilon ) const {
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

ARC_INLINE bool arcPluecker::operator==( const arcPluecker &a ) const {
	return Compare( a );
}

ARC_INLINE bool arcPluecker::operator!=( const arcPluecker &a ) const {
	return !Compare( a );
}

ARC_INLINE void arcPluecker::Set( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 ) {
	p[0] = a1;
	p[1] = a2;
	p[2] = a3;
	p[3] = a4;
	p[4] = a5;
	p[5] = a6;
}

ARC_INLINE void arcPluecker::Zero( void ) {
	p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = 0.0f;
}

ARC_INLINE void arcPluecker::FromLine( const anVec3 &start, const anVec3 &end ) {
	p[0] = start[0] * end[1] - end[0] * start[1];
	p[1] = start[0] * end[2] - end[0] * start[2];
	p[2] = start[0] - end[0];
	p[3] = start[1] * end[2] - end[1] * start[2];
	p[4] = start[2] - end[2];
	p[5] = end[1] - start[1];
}

ARC_INLINE void arcPluecker::FromRay( const anVec3 &start, const anVec3 &dir ) {
	p[0] = start[0] * dir[1] - dir[0] * start[1];
	p[1] = start[0] * dir[2] - dir[0] * start[2];
	p[2] = -dir[0];
	p[3] = start[1] * dir[2] - dir[1] * start[2];
	p[4] = -dir[2];
	p[5] = dir[1];
}

ARC_INLINE bool arcPluecker::ToLine( anVec3 &start, anVec3 &end ) const {
	anVec3 dir1, dir2;
	float d;

	dir1[0] = p[3];
	dir1[1] = -p[1];
	dir1[2] = p[0];

	dir2[0] = -p[2];
	dir2[1] = p[5];
	dir2[2] = -p[4];

	d = dir2 * dir2;
	if ( d == 0.0f ) {
		return false; // pluecker coordinate does not represent a line
	}

	start = dir2.Cross(dir1) * (1.0f / d);
	end = start + dir2;
	return true;
}

ARC_INLINE bool arcPluecker::ToRay( anVec3 &start, anVec3 &dir ) const {
	anVec3 dir1;
	float d;

	dir1[0] = p[3];
	dir1[1] = -p[1];
	dir1[2] = p[0];

	dir[0] = -p[2];
	dir[1] = p[5];
	dir[2] = -p[4];

	d = dir * dir;
	if ( d == 0.0f ) {
		return false; // pluecker coordinate does not represent a line
	}

	start = dir.Cross(dir1) * (1.0f / d);
	return true;
}

ARC_INLINE void arcPluecker::ToDir( anVec3 &dir ) const {
	dir[0] = -p[2];
	dir[1] = p[5];
	dir[2] = -p[4];
}

ARC_INLINE float arcPluecker::PermutedInnerProduct( const arcPluecker &a ) const {
	return p[0] * a.p[4] + p[1] * a.p[5] + p[2] * a.p[3] + p[4] * a.p[0] + p[5] * a.p[1] + p[3] * a.p[2];
}

ARC_INLINE float arcPluecker::Length( void ) const {
	return ( float )anMath::Sqrt( p[5] * p[5] + p[4] * p[4] + p[2] * p[2] );
}

ARC_INLINE float arcPluecker::LengthSqr( void ) const {
	return ( p[5] * p[5] + p[4] * p[4] + p[2] * p[2] );
}

ARC_INLINE float arcPluecker::NormalizeSelf( void ) {
	float l, d;

	l = LengthSqr();
	if ( l == 0.0f ) {
		return l; // pluecker coordinate does not represent a line
	}
	d = anMath::InvSqrt( l );
	p[0] *= d;
	p[1] *= d;
	p[2] *= d;
	p[3] *= d;
	p[4] *= d;
	p[5] *= d;
	return d * l;
}

ARC_INLINE arcPluecker arcPluecker::Normalize( void ) const {
	float d;

	d = LengthSqr();
	if ( d == 0.0f ) {
		return *this; // pluecker coordinate does not represent a line
	}
	d = anMath::InvSqrt( d );
	return arcPluecker( p[0]*d, p[1]*d, p[2]*d, p[3]*d, p[4]*d, p[5]*d );
}

ARC_INLINE int arcPluecker::GetDimension( void ) const {
	return 6;
}

ARC_INLINE const float *arcPluecker::ToFloatPtr( void ) const {
	return p;
}

ARC_INLINE float *arcPluecker::ToFloatPtr( void ) {
	return p;
}

#endif /* !__MATH_PLUECKER_H__ */
