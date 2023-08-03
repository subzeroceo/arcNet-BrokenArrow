#ifndef __JOINTTRANSFORM_H__
#define __JOINTTRANSFORM_H__

/*
===============================================================================

  Joint Quaternion

===============================================================================
*/

class anJointQuat {
public:

	anQuats		q;
	anVec3			t;
};


/*
===============================================================================

  Joint Matrix

  anMat3 m;
  anVec3 t;

  m[0][0], m[1][0], m[2][0], t[0]
  m[0][1], m[1][1], m[2][1], t[1]
  m[0][2], m[1][2], m[2][2], t[2]

===============================================================================
*/

class arcJointMat {
public:

	void			SetRotation( const anMat3 &m );
	void			SetTranslation( const anVec3 &t );

	anVec3			operator*( const anVec3 &v ) const;							// only rotate
	anVec3			operator*( const anVec4 &v ) const;							// rotate and translate

	arcJointMat &	operator*=( const arcJointMat &a );							// transform
	arcJointMat &	operator/=( const arcJointMat &a );							// untransform

	bool			Compare( const arcJointMat &a ) const;						// exact compare, no epsilon
	bool			Compare( const arcJointMat &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const arcJointMat &a ) const;					// exact compare, no epsilon
	bool			operator!=(	const arcJointMat &a ) const;					// exact compare, no epsilon

	anMat3			ToMat3( void ) const;
	anVec3			ToVec3( void ) const;
	anJointQuat		ToJointQuat( void ) const;
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );

private:
	float			mat[3*4];
};

ARC_INLINE void arcJointMat::SetRotation( const anMat3 &m ) {
	// NOTE: anMat3 is transposed because it is column-major
	mat[0 * 4 + 0] = m[0][0];
	mat[0 * 4 + 1] = m[1][0];
	mat[0 * 4 + 2] = m[2][0];
	mat[1 * 4 + 0] = m[0][1];
	mat[1 * 4 + 1] = m[1][1];
	mat[1 * 4 + 2] = m[2][1];
	mat[2 * 4 + 0] = m[0][2];
	mat[2 * 4 + 1] = m[1][2];
	mat[2 * 4 + 2] = m[2][2];
}

ARC_INLINE void arcJointMat::SetTranslation( const anVec3 &t ) {
	mat[0 * 4 + 3] = t[0];
	mat[1 * 4 + 3] = t[1];
	mat[2 * 4 + 3] = t[2];
}

ARC_INLINE anVec3 arcJointMat::operator*( const anVec3 &v ) const {
	return anVec3(	mat[0 * 4 + 0] * v[0] + mat[0 * 4 + 1] * v[1] + mat[0 * 4 + 2] * v[2],
					mat[1 * 4 + 0] * v[0] + mat[1 * 4 + 1] * v[1] + mat[1 * 4 + 2] * v[2],
					mat[2 * 4 + 0] * v[0] + mat[2 * 4 + 1] * v[1] + mat[2 * 4 + 2] * v[2] );
}

ARC_INLINE anVec3 arcJointMat::operator*( const anVec4 &v ) const {
	return anVec3(	mat[0 * 4 + 0] * v[0] + mat[0 * 4 + 1] * v[1] + mat[0 * 4 + 2] * v[2] + mat[0 * 4 + 3] * v[3],
					mat[1 * 4 + 0] * v[0] + mat[1 * 4 + 1] * v[1] + mat[1 * 4 + 2] * v[2] + mat[1 * 4 + 3] * v[3],
					mat[2 * 4 + 0] * v[0] + mat[2 * 4 + 1] * v[1] + mat[2 * 4 + 2] * v[2] + mat[2 * 4 + 3] * v[3] );
}

ARC_INLINE arcJointMat &arcJointMat::operator*=( const arcJointMat &a ) {
	float dst[3];

	dst[0] = mat[0 * 4 + 0] * a.mat[0 * 4 + 0] + mat[1 * 4 + 0] * a.mat[0 * 4 + 1] + mat[2 * 4 + 0] * a.mat[0 * 4 + 2];
	dst[1] = mat[0 * 4 + 0] * a.mat[1 * 4 + 0] + mat[1 * 4 + 0] * a.mat[1 * 4 + 1] + mat[2 * 4 + 0] * a.mat[1 * 4 + 2];
	dst[2] = mat[0 * 4 + 0] * a.mat[2 * 4 + 0] + mat[1 * 4 + 0] * a.mat[2 * 4 + 1] + mat[2 * 4 + 0] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 0] = dst[0];
	mat[1 * 4 + 0] = dst[1];
	mat[2 * 4 + 0] = dst[2];

	dst[0] = mat[0 * 4 + 1] * a.mat[0 * 4 + 0] + mat[1 * 4 + 1] * a.mat[0 * 4 + 1] + mat[2 * 4 + 1] * a.mat[0 * 4 + 2];
	dst[1] = mat[0 * 4 + 1] * a.mat[1 * 4 + 0] + mat[1 * 4 + 1] * a.mat[1 * 4 + 1] + mat[2 * 4 + 1] * a.mat[1 * 4 + 2];
	dst[2] = mat[0 * 4 + 1] * a.mat[2 * 4 + 0] + mat[1 * 4 + 1] * a.mat[2 * 4 + 1] + mat[2 * 4 + 1] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 1] = dst[0];
	mat[1 * 4 + 1] = dst[1];
	mat[2 * 4 + 1] = dst[2];

	dst[0] = mat[0 * 4 + 2] * a.mat[0 * 4 + 0] + mat[1 * 4 + 2] * a.mat[0 * 4 + 1] + mat[2 * 4 + 2] * a.mat[0 * 4 + 2];
	dst[1] = mat[0 * 4 + 2] * a.mat[1 * 4 + 0] + mat[1 * 4 + 2] * a.mat[1 * 4 + 1] + mat[2 * 4 + 2] * a.mat[1 * 4 + 2];
	dst[2] = mat[0 * 4 + 2] * a.mat[2 * 4 + 0] + mat[1 * 4 + 2] * a.mat[2 * 4 + 1] + mat[2 * 4 + 2] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 2] = dst[0];
	mat[1 * 4 + 2] = dst[1];
	mat[2 * 4 + 2] = dst[2];

	dst[0] = mat[0 * 4 + 3] * a.mat[0 * 4 + 0] + mat[1 * 4 + 3] * a.mat[0 * 4 + 1] + mat[2 * 4 + 3] * a.mat[0 * 4 + 2];
	dst[1] = mat[0 * 4 + 3] * a.mat[1 * 4 + 0] + mat[1 * 4 + 3] * a.mat[1 * 4 + 1] + mat[2 * 4 + 3] * a.mat[1 * 4 + 2];
	dst[2] = mat[0 * 4 + 3] * a.mat[2 * 4 + 0] + mat[1 * 4 + 3] * a.mat[2 * 4 + 1] + mat[2 * 4 + 3] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 3] = dst[0];
	mat[1 * 4 + 3] = dst[1];
	mat[2 * 4 + 3] = dst[2];

	mat[0 * 4 + 3] += a.mat[0 * 4 + 3];
	mat[1 * 4 + 3] += a.mat[1 * 4 + 3];
	mat[2 * 4 + 3] += a.mat[2 * 4 + 3];

	return *this;
}

ARC_INLINE arcJointMat &arcJointMat::operator/=( const arcJointMat &a ) {
	float dst[3];

	mat[0 * 4 + 3] -= a.mat[0 * 4 + 3];
	mat[1 * 4 + 3] -= a.mat[1 * 4 + 3];
	mat[2 * 4 + 3] -= a.mat[2 * 4 + 3];

	dst[0] = mat[0 * 4 + 0] * a.mat[0 * 4 + 0] + mat[1 * 4 + 0] * a.mat[1 * 4 + 0] + mat[2 * 4 + 0] * a.mat[2 * 4 + 0];
	dst[1] = mat[0 * 4 + 0] * a.mat[0 * 4 + 1] + mat[1 * 4 + 0] * a.mat[1 * 4 + 1] + mat[2 * 4 + 0] * a.mat[2 * 4 + 1];
	dst[2] = mat[0 * 4 + 0] * a.mat[0 * 4 + 2] + mat[1 * 4 + 0] * a.mat[1 * 4 + 2] + mat[2 * 4 + 0] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 0] = dst[0];
	mat[1 * 4 + 0] = dst[1];
	mat[2 * 4 + 0] = dst[2];

	dst[0] = mat[0 * 4 + 1] * a.mat[0 * 4 + 0] + mat[1 * 4 + 1] * a.mat[1 * 4 + 0] + mat[2 * 4 + 1] * a.mat[2 * 4 + 0];
	dst[1] = mat[0 * 4 + 1] * a.mat[0 * 4 + 1] + mat[1 * 4 + 1] * a.mat[1 * 4 + 1] + mat[2 * 4 + 1] * a.mat[2 * 4 + 1];
	dst[2] = mat[0 * 4 + 1] * a.mat[0 * 4 + 2] + mat[1 * 4 + 1] * a.mat[1 * 4 + 2] + mat[2 * 4 + 1] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 1] = dst[0];
	mat[1 * 4 + 1] = dst[1];
	mat[2 * 4 + 1] = dst[2];

	dst[0] = mat[0 * 4 + 2] * a.mat[0 * 4 + 0] + mat[1 * 4 + 2] * a.mat[1 * 4 + 0] + mat[2 * 4 + 2] * a.mat[2 * 4 + 0];
	dst[1] = mat[0 * 4 + 2] * a.mat[0 * 4 + 1] + mat[1 * 4 + 2] * a.mat[1 * 4 + 1] + mat[2 * 4 + 2] * a.mat[2 * 4 + 1];
	dst[2] = mat[0 * 4 + 2] * a.mat[0 * 4 + 2] + mat[1 * 4 + 2] * a.mat[1 * 4 + 2] + mat[2 * 4 + 2] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 2] = dst[0];
	mat[1 * 4 + 2] = dst[1];
	mat[2 * 4 + 2] = dst[2];

	dst[0] = mat[0 * 4 + 3] * a.mat[0 * 4 + 0] + mat[1 * 4 + 3] * a.mat[1 * 4 + 0] + mat[2 * 4 + 3] * a.mat[2 * 4 + 0];
	dst[1] = mat[0 * 4 + 3] * a.mat[0 * 4 + 1] + mat[1 * 4 + 3] * a.mat[1 * 4 + 1] + mat[2 * 4 + 3] * a.mat[2 * 4 + 1];
	dst[2] = mat[0 * 4 + 3] * a.mat[0 * 4 + 2] + mat[1 * 4 + 3] * a.mat[1 * 4 + 2] + mat[2 * 4 + 3] * a.mat[2 * 4 + 2];
	mat[0 * 4 + 3] = dst[0];
	mat[1 * 4 + 3] = dst[1];
	mat[2 * 4 + 3] = dst[2];

	return *this;
}

ARC_INLINE bool arcJointMat::Compare( const arcJointMat &a ) const {
	for ( int i = 0; i < 12; i++ ) {
		if ( mat[i] != a.mat[i] ) {
			return false;
		}
	}
	return true;
}

ARC_INLINE bool arcJointMat::Compare( const arcJointMat &a, const float epsilon ) const {
	for ( int i = 0; i < 12; i++ ) {
		if ( anMath::Fabs( mat[i] - a.mat[i] ) > epsilon ) {
			return false;
		}
	}
	return true;
}

ARC_INLINE bool arcJointMat::operator==( const arcJointMat &a ) const {
	return Compare( a );
}

ARC_INLINE bool arcJointMat::operator!=( const arcJointMat &a ) const {
	return !Compare( a );
}

ARC_INLINE anMat3 arcJointMat::ToMat3( void ) const {
	return anMat3(	mat[0 * 4 + 0], mat[1 * 4 + 0], mat[2 * 4 + 0],
					mat[0 * 4 + 1], mat[1 * 4 + 1], mat[2 * 4 + 1],
					mat[0 * 4 + 2], mat[1 * 4 + 2], mat[2 * 4 + 2] );
}

ARC_INLINE anVec3 arcJointMat::ToVec3( void ) const {
	return anVec3( mat[0 * 4 + 3], mat[1 * 4 + 3], mat[2 * 4 + 3] );
}

ARC_INLINE const float *arcJointMat::ToFloatPtr( void ) const {
	return mat;
}

ARC_INLINE float *arcJointMat::ToFloatPtr( void ) {
	return mat;
}

#endif // !__JOINTTRANSFORM_H__