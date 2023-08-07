#ifndef __MATH_PLUECKER_H__
#define __MATH_PLUECKER_H__

/*
===============================================================================

	Pluecker coordinate

===============================================================================
*/

class anPluecker {
public:
					anPluecker( void );
					explicit anPluecker( const float *a );
					explicit anPluecker( const anVec3 &start, const anVec3 &end );
					explicit anPluecker( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	anPluecker		operator-() const;											// flips the direction
	anPluecker		operator*( const float a ) const;
	anPluecker		operator/( const float a ) const;
	float			operator*( const anPluecker &a ) const;						// permuted inner product
	anPluecker		operator-( const anPluecker &a ) const;
	anPluecker		operator+( const anPluecker &a ) const;
	anPluecker &	operator*=( const float a );
	anPluecker &	operator/=( const float a );
	anPluecker &	operator+=( const anPluecker &a );
	anPluecker &	operator-=( const anPluecker &a );

	bool			Compare( const anPluecker &a ) const;						// exact compare, no epsilon
	bool			Compare( const anPluecker &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const anPluecker &a ) const;					// exact compare, no epsilon
	bool			operator!=(	const anPluecker &a ) const;					// exact compare, no epsilon

	void 			Set( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 );
	void			Zero( void );

	void			FromLine( const anVec3 &start, const anVec3 &end );			// pluecker from line
	void			FromRay( const anVec3 &start, const anVec3 &dir );			// pluecker from ray
	bool			FromPlanes( const anPlane &p1, const anPlane &p2 );			// pluecker from intersection of planes
	bool			ToLine( anVec3 &start, anVec3 &end ) const;					// pluecker to line
	bool			ToRay( anVec3 &start, anVec3 &dir ) const;					// pluecker to ray
	void			ToDir( anVec3 &dir ) const;									// pluecker to direction
	float			PermutedInnerProduct( const anPluecker &a ) const;			// pluecker permuted inner product
	float			Distance3DSqr( const anPluecker &a ) const;					// pluecker line distance

	float			Length( void ) const;										// pluecker length
	float			LengthSqr( void ) const;									// pluecker squared length
	anPluecker		Normalize( void ) const;									// pluecker normalize
	float			NormalizeSelf( void );										// pluecker normalize

	int				GetDimension( void ) const;

	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

private:
	float			p[6];
};

extern anPluecker pluecker_origin;
#define pluecker_zero pluecker_origin

inline anPluecker::anPluecker( void ) {
}

inline anPluecker::anPluecker( const float *a ) {
	memcpy( p, a, 6 * sizeof( float ) );
}

inline anPluecker::anPluecker( const anVec3 &start, const anVec3 &end ) {
	FromLine( start, end );
}

inline anPluecker::anPluecker( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 ) {
	p[0] = a1;
	p[1] = a2;
	p[2] = a3;
	p[3] = a4;
	p[4] = a5;
	p[5] = a6;
}

inline anPluecker anPluecker::operator-() const {
	return anPluecker( -p[0], -p[1], -p[2], -p[3], -p[4], -p[5] );
}

inline float anPluecker::operator[]( const int index ) const {
	return p[index];
}

inline float &anPluecker::operator[]( const int index ) {
	return p[index];
}

inline anPluecker anPluecker::operator*( const float a ) const {
	return anPluecker( p[0]*a, p[1]*a, p[2]*a, p[3]*a, p[4]*a, p[5]*a );
}

inline float anPluecker::operator*( const anPluecker &a ) const {
	return p[0] * a.p[4] + p[1] * a.p[5] + p[2] * a.p[3] + p[4] * a.p[0] + p[5] * a.p[1] + p[3] * a.p[2];
}

inline anPluecker anPluecker::operator/( const float a ) const {
	float inva;

	assert( a != 0.0f );
	inva = 1.0f / a;
	return anPluecker( p[0]*inva, p[1]*inva, p[2]*inva, p[3]*inva, p[4]*inva, p[5]*inva );
}

inline anPluecker anPluecker::operator+( const anPluecker &a ) const {
	return anPluecker( p[0] + a[0], p[1] + a[1], p[2] + a[2], p[3] + a[3], p[4] + a[4], p[5] + a[5] );
}

inline anPluecker anPluecker::operator-( const anPluecker &a ) const {
	return anPluecker( p[0] - a[0], p[1] - a[1], p[2] - a[2], p[3] - a[3], p[4] - a[4], p[5] - a[5] );
}

inline anPluecker &anPluecker::operator*=( const float a ) {
	p[0] *= a;
	p[1] *= a;
	p[2] *= a;
	p[3] *= a;
	p[4] *= a;
	p[5] *= a;
	return *this;
}

inline anPluecker &anPluecker::operator/=( const float a ) {
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

inline anPluecker &anPluecker::operator+=( const anPluecker &a ) {
	p[0] += a[0];
	p[1] += a[1];
	p[2] += a[2];
	p[3] += a[3];
	p[4] += a[4];
	p[5] += a[5];
	return *this;
}

inline anPluecker &anPluecker::operator-=( const anPluecker &a ) {
	p[0] -= a[0];
	p[1] -= a[1];
	p[2] -= a[2];
	p[3] -= a[3];
	p[4] -= a[4];
	p[5] -= a[5];
	return *this;
}

inline bool anPluecker::Compare( const anPluecker &a ) const {
	return ( ( p[0] == a[0] ) && ( p[1] == a[1] ) && ( p[2] == a[2] ) &&
			( p[3] == a[3] ) && ( p[4] == a[4] ) && ( p[5] == a[5] ) );
}

inline bool anPluecker::Compare( const anPluecker &a, const float epsilon ) const {
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

inline bool anPluecker::operator==( const anPluecker &a ) const {
	return Compare( a );
}

inline bool anPluecker::operator!=( const anPluecker &a ) const {
	return !Compare( a );
}

inline void anPluecker::Set( const float a1, const float a2, const float a3, const float a4, const float a5, const float a6 ) {
	p[0] = a1;
	p[1] = a2;
	p[2] = a3;
	p[3] = a4;
	p[4] = a5;
	p[5] = a6;
}

inline void anPluecker::Zero( void ) {
	p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = 0.0f;
}

inline void anPluecker::FromLine( const anVec3 &start, const anVec3 &end ) {
	p[0] = start[0] * end[1] - end[0] * start[1];
	p[1] = start[0] * end[2] - end[0] * start[2];
	p[2] = start[0] - end[0];
	p[3] = start[1] * end[2] - end[1] * start[2];
	p[4] = start[2] - end[2];
	p[5] = end[1] - start[1];
}

inline void anPluecker::FromRay( const anVec3 &start, const anVec3 &dir ) {
	p[0] = start[0] * dir[1] - dir[0] * start[1];
	p[1] = start[0] * dir[2] - dir[0] * start[2];
	p[2] = -dir[0];
	p[3] = start[1] * dir[2] - dir[1] * start[2];
	p[4] = -dir[2];
	p[5] = dir[1];
}

inline bool anPluecker::ToLine( anVec3 &start, anVec3 &end ) const {
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

inline bool anPluecker::ToRay( anVec3 &start, anVec3 &dir ) const {
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

inline void anPluecker::ToDir( anVec3 &dir ) const {
	dir[0] = -p[2];
	dir[1] = p[5];
	dir[2] = -p[4];
}

inline float anPluecker::PermutedInnerProduct( const anPluecker &a ) const {
	return p[0] * a.p[4] + p[1] * a.p[5] + p[2] * a.p[3] + p[4] * a.p[0] + p[5] * a.p[1] + p[3] * a.p[2];
}

inline float anPluecker::Length( void ) const {
	return ( float )anMath::Sqrt( p[5] * p[5] + p[4] * p[4] + p[2] * p[2] );
}

inline float anPluecker::LengthSqr( void ) const {
	return ( p[5] * p[5] + p[4] * p[4] + p[2] * p[2] );
}

inline float anPluecker::NormalizeSelf( void ) {
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

inline anPluecker anPluecker::Normalize( void ) const {
	float d;

	d = LengthSqr();
	if ( d == 0.0f ) {
		return *this; // pluecker coordinate does not represent a line
	}
	d = anMath::InvSqrt( d );
	return anPluecker( p[0]*d, p[1]*d, p[2]*d, p[3]*d, p[4]*d, p[5]*d );
}

inline int anPluecker::GetDimension( void ) const {
	return 6;
}

inline const float *anPluecker::ToFloatPtr( void ) const {
	return p;
}

inline float *anPluecker::ToFloatPtr( void ) {
	return p;
}

#endif /* !__MATH_PLUECKER_H__ */
