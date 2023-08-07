#ifndef __MATH_ROTATION_H__
#define __MATH_ROTATION_H__

/*
===============================================================================

	Describes a complete rotation in degrees about an abritray axis.
	A local rotation matrix is stored for fast rotation of multiple points.

===============================================================================
*/


class anAngles;
class anQuats;
class anMat3;

class anRotation {

	friend class anAngles;
	friend class anQuats;
	friend class anMat3;

public:
						anRotation( void );
						anRotation( const anVec3 &rotationOrigin, const anVec3 &rotationVec, const float rotationAngle );

	void				Set( const anVec3 &rotationOrigin, const anVec3 &rotationVec, const float rotationAngle );
	void				SetOrigin( const anVec3 &rotationOrigin );
	void				SetVec( const anVec3 &rotationVec );					// has to be normalized
	void				SetVec( const float x, const float y, const float z );	// has to be normalized
	void				SetAngle( const float rotationAngle );
	void				Scale( const float s );
	void				ReCalculateMatrix( void );
	const anVec3 &		GetOrigin( void ) const;
	const anVec3 &		GetVec( void ) const;
	float				GetAngle( void ) const;

	anRotation			operator-() const;										// flips rotation
	anRotation			operator*( const float s ) const;						// scale rotation
	anRotation			operator/( const float s ) const;						// scale rotation
	anRotation &			operator*=( const float s );							// scale rotation
	anRotation &			operator/=( const float s );							// scale rotation
	anVec3				operator*( const anVec3 &v ) const;					// rotate vector

	friend anRotation	operator*( const float s, const anRotation &r );			// scale rotation
	friend anVec3		operator*( const anVec3 &v, const anRotation &r );		// rotate vector
	friend anVec3 &	operator*=( anVec3 &v, const anRotation &r );			// rotate vector

	anAngles			ToAngles( void ) const;
	anQuats			ToQuat( void ) const;
	const anMat3 &		ToMat3( void ) const;
	anMat4				ToMat4( void ) const;
	anVec3				ToAngularVelocity( void ) const;

	void				RotatePoint( anVec3 &point ) const;

	void				Normalize180( void );
	void				Normalize360( void );

private:
	anVec3				origin;			// origin of rotation
	anVec3				vec;			// normalized vector to rotate around
	float				angle;			// angle of rotation in degrees
	mutable anMat3		axis;			// rotation axis
	mutable bool		axisValid;		// true if rotation axis is valid
};


inline anRotation::anRotation( void ) {
}

inline anRotation::anRotation( const anVec3 &rotationOrigin, const anVec3 &rotationVec, const float rotationAngle ) {
	origin = rotationOrigin;
	vec = rotationVec;
	angle = rotationAngle;
	axisValid = false;
}

inline void anRotation::Set( const anVec3 &rotationOrigin, const anVec3 &rotationVec, const float rotationAngle ) {
	origin = rotationOrigin;
	vec = rotationVec;
	angle = rotationAngle;
	axisValid = false;
}

inline void anRotation::SetOrigin( const anVec3 &rotationOrigin ) {
	origin = rotationOrigin;
}

inline void anRotation::SetVec( const anVec3 &rotationVec ) {
	vec = rotationVec;
	axisValid = false;
}

inline void anRotation::SetVec( float x, float y, float z ) {
	vec[0] = x;
	vec[1] = y;
	vec[2] = z;
	axisValid = false;
}

inline void anRotation::SetAngle( const float rotationAngle ) {
	angle = rotationAngle;
	axisValid = false;
}

inline void anRotation::Scale( const float s ) {
	angle *= s;
	axisValid = false;
}

inline void anRotation::ReCalculateMatrix( void ) {
	axisValid = false;
	ToMat3();
}

inline const anVec3 &anRotation::GetOrigin( void ) const {
	return origin;
}

inline const anVec3 &anRotation::GetVec( void ) const  {
	return vec;
}

inline float anRotation::GetAngle( void ) const  {
	return angle;
}

inline anRotation anRotation::operator-() const {
	return anRotation( origin, vec, -angle );
}

inline anRotation anRotation::operator*( const float s ) const {
	return anRotation( origin, vec, angle * s );
}

inline anRotation anRotation::operator/( const float s ) const {
	assert( s != 0.0f );
	return anRotation( origin, vec, angle / s );
}

inline anRotation &anRotation::operator*=( const float s ) {
	angle *= s;
	axisValid = false;
	return *this;
}

inline anRotation &anRotation::operator/=( const float s ) {
	assert( s != 0.0f );
	angle /= s;
	axisValid = false;
	return *this;
}

inline anVec3 anRotation::operator*( const anVec3 &v ) const {
	if ( !axisValid ) {
		ToMat3();
	}
	return ( ( v - origin) * axis + origin);
}

inline anRotation operator*( const float s, const anRotation &r ) {
	return r * s;
}

inline anVec3 operator*( const anVec3 &v, const anRotation &r ) {
	return r * v;
}

inline anVec3 &operator*=( anVec3 &v, const anRotation &r ) {
	v = r * v;
	return v;
}

inline void anRotation::RotatePoint( anVec3 &point ) const {
	if ( !axisValid ) {
		ToMat3();
	}
	point = ((point - origin) * axis + origin);
}

#endif
