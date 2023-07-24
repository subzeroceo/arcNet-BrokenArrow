#ifndef __BV_SPHERE_H__
#define __BV_SPHERE_H__

/*
===============================================================================

	Sphere

===============================================================================
*/

class ARCSphere {
public:
					ARCSphere( void );
					explicit ARCSphere( const arcVec3 &point );
					explicit ARCSphere( const arcVec3 &point, const float r );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	ARCSphere		operator+( const arcVec3 &t ) const;				// returns tranlated sphere
	ARCSphere &		operator+=( const arcVec3 &t );					// translate the sphere
	ARCSphere		operator+( const ARCSphere &s ) const;
	ARCSphere &		operator+=( const ARCSphere &s );

	bool			Compare( const ARCSphere &a ) const;							// exact compare, no epsilon
	bool			Compare( const ARCSphere &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const ARCSphere &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const ARCSphere &a ) const;						// exact compare, no epsilon

	void			Clear( void );									// inside out sphere
	void			Zero( void );									// single point at origin
	void			SetOrigin( const arcVec3 &o );					// set origin of sphere
	void			SetRadius( const float r );						// set square radius

	const arcVec3 &	GetOrigin( void ) const;						// returns origin of sphere
	float			GetRadius( void ) const;						// returns sphere radius
	bool			IsCleared( void ) const;						// returns true if sphere is inside out

	bool			AddPoint( const arcVec3 &p );					// add the point, returns true if the sphere expanded
	bool			AddSphere( const ARCSphere &s );					// add the sphere, returns true if the sphere expanded
	ARCSphere		Expand( const float d ) const;					// return bounds expanded in all directions with the given value
	ARCSphere &		ExpandSelf( const float d );					// expand bounds in all directions with the given value
	ARCSphere		Translate( const arcVec3 &translation ) const;
	ARCSphere &		TranslateSelf( const arcVec3 &translation );

	float			PlaneDistance( const arcPlane &plane ) const;
	int				PlaneSide( const arcPlane &plane, const float epsilon = ON_EPSILON ) const;

	bool			ContainsPoint( const arcVec3 &p ) const;			// includes touching
	bool			IntersectsSphere( const ARCSphere &s ) const;	// includes touching
	bool			LineIntersection( const arcVec3 &start, const arcVec3 &end ) const;
					// intersection points are (start + dir * scale1) and (start + dir * scale2)
	bool			RayIntersection( const arcVec3 &start, const arcVec3 &dir, float &scale1, float &scale2 ) const;

					// Tight sphere for a point set.
	void			FromPoints( const arcVec3 *points, const int numPoints );
					// Most tight sphere for a translation.
	void			FromPointTranslation( const arcVec3 &point, const arcVec3 &translation );
	void			FromSphereTranslation( const ARCSphere &sphere, const arcVec3 &start, const arcVec3 &translation );
					// Most tight sphere for a rotation.
	void			FromPointRotation( const arcVec3 &point, const arcRotate &rotation );
	void			FromSphereRotation( const ARCSphere &sphere, const arcVec3 &start, const arcRotate &rotation );

	void			AxisProjection( const arcVec3 &dir, float &min, float &max ) const;

private:
	arcVec3			origin;
	float			radius;
};

extern ARCSphere	sphere_zero;

ARC_INLINE ARCSphere::ARCSphere( void ) {
}

ARC_INLINE ARCSphere::ARCSphere( const arcVec3 &point ) {
	origin = point;
	radius = 0.0f;
}

ARC_INLINE ARCSphere::ARCSphere( const arcVec3 &point, const float r ) {
	origin = point;
	radius = r;
}

ARC_INLINE float ARCSphere::operator[]( const int index ) const {
	return ((float *) &origin)[index];
}

ARC_INLINE float &ARCSphere::operator[]( const int index ) {
	return ((float *) &origin)[index];
}

ARC_INLINE ARCSphere ARCSphere::operator+( const arcVec3 &t ) const {
	return ARCSphere( origin + t, radius );
}

ARC_INLINE ARCSphere &ARCSphere::operator+=( const arcVec3 &t ) {
	origin += t;
	return *this;
}

ARC_INLINE bool ARCSphere::Compare( const ARCSphere &a ) const {
	return ( origin.Compare( a.origin ) && radius == a.radius );
}

ARC_INLINE bool ARCSphere::Compare( const ARCSphere &a, const float epsilon ) const {
	return ( origin.Compare( a.origin, epsilon ) && arcMath::Fabs( radius - a.radius ) <= epsilon );
}

ARC_INLINE bool ARCSphere::operator==( const ARCSphere &a ) const {
	return Compare( a );
}

ARC_INLINE bool ARCSphere::operator!=( const ARCSphere &a ) const {
	return !Compare( a );
}

ARC_INLINE void ARCSphere::Clear( void ) {
	origin.Zero();
	radius = -1.0f;
}

ARC_INLINE void ARCSphere::Zero( void ) {
	origin.Zero();
	radius = 0.0f;
}

ARC_INLINE void ARCSphere::SetOrigin( const arcVec3 &o ) {
	origin = o;
}

ARC_INLINE void ARCSphere::SetRadius( const float r ) {
	radius = r;
}

ARC_INLINE const arcVec3 &ARCSphere::GetOrigin( void ) const {
	return origin;
}

ARC_INLINE float ARCSphere::GetRadius( void ) const {
	return radius;
}

ARC_INLINE bool ARCSphere::IsCleared( void ) const {
	return ( radius < 0.0f );
}

ARC_INLINE bool ARCSphere::AddPoint( const arcVec3 &p ) {
	if ( radius < 0.0f ) {
		origin = p;
		radius = 0.0f;
		return true;
	}
	else {
		float r = ( p - origin ).LengthSqr();
		if ( r > radius * radius ) {
			r = arcMath::Sqrt( r );
			origin += ( p - origin ) * 0.5f * (1.0f - radius / r );
			radius += 0.5f * ( r - radius );
			return true;
		}
		return false;
	}
}

ARC_INLINE bool ARCSphere::AddSphere( const ARCSphere &s ) {
	if ( radius < 0.0f ) {
		origin = s.origin;
		radius = s.radius;
		return true;
	}
	else {
		float r = ( s.origin - origin ).LengthSqr();
		if ( r > ( radius + s.radius ) * ( radius + s.radius ) ) {
			r = arcMath::Sqrt( r );
			origin += ( s.origin - origin ) * 0.5f * (1.0f - radius / ( r + s.radius ) );
			radius += 0.5f * ( ( r + s.radius ) - radius );
			return true;
		}
		return false;
	}
}

ARC_INLINE ARCSphere ARCSphere::Expand( const float d ) const {
	return ARCSphere( origin, radius + d );
}

ARC_INLINE ARCSphere &ARCSphere::ExpandSelf( const float d ) {
	radius += d;
	return *this;
}

ARC_INLINE ARCSphere ARCSphere::Translate( const arcVec3 &translation ) const {
	return ARCSphere( origin + translation, radius );
}

ARC_INLINE ARCSphere &ARCSphere::TranslateSelf( const arcVec3 &translation ) {
	origin += translation;
	return *this;
}

ARC_INLINE bool ARCSphere::ContainsPoint( const arcVec3 &p ) const {
	if ( ( p - origin ).LengthSqr() > radius * radius ) {
		return false;
	}
	return true;
}

ARC_INLINE bool ARCSphere::IntersectsSphere( const ARCSphere &s ) const {
	float r = s.radius + radius;
	if ( ( s.origin - origin ).LengthSqr() > r * r ) {
		return false;
	}
	return true;
}

ARC_INLINE void ARCSphere::FromPointTranslation( const arcVec3 &point, const arcVec3 &translation ) {
	origin = point + 0.5f * translation;
	radius = arcMath::Sqrt( 0.5f * translation.LengthSqr() );
}

ARC_INLINE void ARCSphere::FromSphereTranslation( const ARCSphere &sphere, const arcVec3 &start, const arcVec3 &translation ) {
	origin = start + sphere.origin + 0.5f * translation;
	radius = arcMath::Sqrt( 0.5f * translation.LengthSqr() ) + sphere.radius;
}

ARC_INLINE void ARCSphere::FromPointRotation( const arcVec3 &point, const arcRotate &rotation ) {
	arcVec3 end = rotation * point;
	origin = ( point + end ) * 0.5f;
	radius = arcMath::Sqrt( 0.5f * ( end - point ).LengthSqr() );
}

ARC_INLINE void ARCSphere::FromSphereRotation( const ARCSphere &sphere, const arcVec3 &start, const arcRotate &rotation ) {
	arcVec3 end = rotation * sphere.origin;
	origin = start + ( sphere.origin + end ) * 0.5f;
	radius = arcMath::Sqrt( 0.5f * ( end - sphere.origin ).LengthSqr() ) + sphere.radius;
}

ARC_INLINE void ARCSphere::AxisProjection( const arcVec3 &dir, float &min, float &max ) const {
	float d;
	d = dir * origin;
	min = d - radius;
	max = d + radius;
}

#endif /* !__BV_SPHERE_H__ */
