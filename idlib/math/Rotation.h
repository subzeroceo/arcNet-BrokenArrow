#ifndef __MATH_ROTATION_H__
#define __MATH_ROTATION_H__

/*
===============================================================================

	Describes a complete rotation in degrees about an abritray axis.
	A local rotation matrix is stored for fast rotation of multiple points.

===============================================================================
*/


class arcAngles;
class arcQuats;
class arcMat3;

class arcRotate {

	friend class arcAngles;
	friend class arcQuats;
	friend class arcMat3;

public:
						arcRotate( void );
						arcRotate( const arcVec3 &rotationOrigin, const arcVec3 &rotationVec, const float rotationAngle );

	void				Set( const arcVec3 &rotationOrigin, const arcVec3 &rotationVec, const float rotationAngle );
	void				SetOrigin( const arcVec3 &rotationOrigin );
	void				SetVec( const arcVec3 &rotationVec );					// has to be normalized
	void				SetVec( const float x, const float y, const float z );	// has to be normalized
	void				SetAngle( const float rotationAngle );
	void				Scale( const float s );
	void				ReCalculateMatrix( void );
	const arcVec3 &		GetOrigin( void ) const;
	const arcVec3 &		GetVec( void ) const;
	float				GetAngle( void ) const;

	arcRotate			operator-() const;										// flips rotation
	arcRotate			operator*( const float s ) const;						// scale rotation
	arcRotate			operator/( const float s ) const;						// scale rotation
	arcRotate &			operator*=( const float s );							// scale rotation
	arcRotate &			operator/=( const float s );							// scale rotation
	arcVec3				operator*( const arcVec3 &v ) const;					// rotate vector

	friend arcRotate	operator*( const float s, const arcRotate &r );			// scale rotation
	friend arcVec3		operator*( const arcVec3 &v, const arcRotate &r );		// rotate vector
	friend arcVec3 &	operator*=( arcVec3 &v, const arcRotate &r );			// rotate vector

	arcAngles			ToAngles( void ) const;
	arcQuats			ToQuat( void ) const;
	const arcMat3 &		ToMat3( void ) const;
	arcMat4				ToMat4( void ) const;
	arcVec3				ToAngularVelocity( void ) const;

	void				RotatePoint( arcVec3 &point ) const;

	void				Normalize180( void );
	void				Normalize360( void );

private:
	arcVec3				origin;			// origin of rotation
	arcVec3				vec;			// normalized vector to rotate around
	float				angle;			// angle of rotation in degrees
	mutable arcMat3		axis;			// rotation axis
	mutable bool		axisValid;		// true if rotation axis is valid
};


ARC_INLINE arcRotate::arcRotate( void ) {
}

ARC_INLINE arcRotate::arcRotate( const arcVec3 &rotationOrigin, const arcVec3 &rotationVec, const float rotationAngle ) {
	origin = rotationOrigin;
	vec = rotationVec;
	angle = rotationAngle;
	axisValid = false;
}

ARC_INLINE void arcRotate::Set( const arcVec3 &rotationOrigin, const arcVec3 &rotationVec, const float rotationAngle ) {
	origin = rotationOrigin;
	vec = rotationVec;
	angle = rotationAngle;
	axisValid = false;
}

ARC_INLINE void arcRotate::SetOrigin( const arcVec3 &rotationOrigin ) {
	origin = rotationOrigin;
}

ARC_INLINE void arcRotate::SetVec( const arcVec3 &rotationVec ) {
	vec = rotationVec;
	axisValid = false;
}

ARC_INLINE void arcRotate::SetVec( float x, float y, float z ) {
	vec[0] = x;
	vec[1] = y;
	vec[2] = z;
	axisValid = false;
}

ARC_INLINE void arcRotate::SetAngle( const float rotationAngle ) {
	angle = rotationAngle;
	axisValid = false;
}

ARC_INLINE void arcRotate::Scale( const float s ) {
	angle *= s;
	axisValid = false;
}

ARC_INLINE void arcRotate::ReCalculateMatrix( void ) {
	axisValid = false;
	ToMat3();
}

ARC_INLINE const arcVec3 &arcRotate::GetOrigin( void ) const {
	return origin;
}

ARC_INLINE const arcVec3 &arcRotate::GetVec( void ) const  {
	return vec;
}

ARC_INLINE float arcRotate::GetAngle( void ) const  {
	return angle;
}

ARC_INLINE arcRotate arcRotate::operator-() const {
	return arcRotate( origin, vec, -angle );
}

ARC_INLINE arcRotate arcRotate::operator*( const float s ) const {
	return arcRotate( origin, vec, angle * s );
}

ARC_INLINE arcRotate arcRotate::operator/( const float s ) const {
	assert( s != 0.0f );
	return arcRotate( origin, vec, angle / s );
}

ARC_INLINE arcRotate &arcRotate::operator*=( const float s ) {
	angle *= s;
	axisValid = false;
	return *this;
}

ARC_INLINE arcRotate &arcRotate::operator/=( const float s ) {
	assert( s != 0.0f );
	angle /= s;
	axisValid = false;
	return *this;
}

ARC_INLINE arcVec3 arcRotate::operator*( const arcVec3 &v ) const {
	if ( !axisValid ) {
		ToMat3();
	}
	return ( ( v - origin) * axis + origin);
}

ARC_INLINE arcRotate operator*( const float s, const arcRotate &r ) {
	return r * s;
}

ARC_INLINE arcVec3 operator*( const arcVec3 &v, const arcRotate &r ) {
	return r * v;
}

ARC_INLINE arcVec3 &operator*=( arcVec3 &v, const arcRotate &r ) {
	v = r * v;
	return v;
}

ARC_INLINE void arcRotate::RotatePoint( arcVec3 &point ) const {
	if ( !axisValid ) {
		ToMat3();
	}
	point = ((point - origin) * axis + origin);
}

#endif
