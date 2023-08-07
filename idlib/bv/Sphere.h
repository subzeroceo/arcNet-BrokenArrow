#ifndef __BV_SPHERE_H__
#define __BV_SPHERE_H__

/*
===============================================================================

	Sphere

===============================================================================
*/

class anSphere {
public:
					anSphere( void );
					explicit anSphere( const anVec3 &point );
					explicit anSphere( const anVec3 &point, const float r );

	float			operator[]( const int index ) const;
	float &			operator[]( const int index );
	anSphere		operator+( const anVec3 &t ) const;				// returns tranlated sphere
	anSphere &		operator+=( const anVec3 &t );					// translate the sphere
	anSphere		operator+( const anSphere &s ) const;
	anSphere &		operator+=( const anSphere &s );

	bool			Compare( const anSphere &a ) const;							// exact compare, no epsilon
	bool			Compare( const anSphere &a, const float epsilon ) const;	// compare with epsilon
	bool			operator==(	const anSphere &a ) const;						// exact compare, no epsilon
	bool			operator!=(	const anSphere &a ) const;						// exact compare, no epsilon

	void			Clear( void );									// inside out sphere
	void			Zero( void );									// single point at origin
	void			SetOrigin( const anVec3 &o );					// set origin of sphere
	void			SetRadius( const float r );						// set square radius

	const anVec3 &	GetOrigin( void ) const;						// returns origin of sphere
	float			GetRadius( void ) const;						// returns sphere radius
	bool			IsCleared( void ) const;						// returns true if sphere is inside out

	bool			AddPoint( const anVec3 &p );					// add the point, returns true if the sphere expanded
	bool			AddSphere( const anSphere &s );					// add the sphere, returns true if the sphere expanded
	anSphere		Expand( const float d ) const;					// return bounds expanded in all directions with the given value
	anSphere &		ExpandSelf( const float d );					// expand bounds in all directions with the given value
	anSphere		Translate( const anVec3 &translation ) const;
	anSphere &		TranslateSelf( const anVec3 &translation );

	float			PlaneDistance( const anPlane &plane ) const;
	int				PlaneSide( const anPlane &plane, const float epsilon = ON_EPSILON ) const;

	bool			ContainsPoint( const anVec3 &p ) const;			// includes touching
	bool			IntersectsSphere( const anSphere &s ) const;	// includes touching
	bool			LineIntersection( const anVec3 &start, const anVec3 &end ) const;
					// intersection points are ( start + dir * scale1) and ( start + dir * scale2)
	bool			RayIntersection( const anVec3 &start, const anVec3 &dir, float &scale1, float &scale2 ) const;

					// Tight sphere for a point set.
	void			FromPoints( const anVec3 *points, const int numPoints );
					// Most tight sphere for a translation.
	void			FromPointTranslation( const anVec3 &point, const anVec3 &translation );
	void			FromSphereTranslation( const anSphere &sphere, const anVec3 &start, const anVec3 &translation );
					// Most tight sphere for a rotation.
	void			FromPointRotation( const anVec3 &point, const anRotation &rotation );
	void			FromSphereRotation( const anSphere &sphere, const anVec3 &start, const anRotation &rotation );

	void			AxisProjection( const anVec3 &dir, float &min, float &max ) const;

private:
	anVec3			origin;
	float			radius;
};

extern anSphere	sphere_zero;

inline anSphere::anSphere( void ) {
}

inline anSphere::anSphere( const anVec3 &point ) {
	origin = point;
	radius = 0.0f;
}

inline anSphere::anSphere( const anVec3 &point, const float r ) {
	origin = point;
	radius = r;
}

inline float anSphere::operator[]( const int index ) const {
	return ((float *) &origin)[index];
}

inline float &anSphere::operator[]( const int index ) {
	return ((float *) &origin)[index];
}

inline anSphere anSphere::operator+( const anVec3 &t ) const {
	return anSphere( origin + t, radius );
}

inline anSphere &anSphere::operator+=( const anVec3 &t ) {
	origin += t;
	return *this;
}

inline bool anSphere::Compare( const anSphere &a ) const {
	return ( origin.Compare( a.origin ) && radius == a.radius );
}

inline bool anSphere::Compare( const anSphere &a, const float epsilon ) const {
	return ( origin.Compare( a.origin, epsilon ) && anMath::Fabs( radius - a.radius ) <= epsilon );
}

inline bool anSphere::operator==( const anSphere &a ) const {
	return Compare( a );
}

inline bool anSphere::operator!=( const anSphere &a ) const {
	return !Compare( a );
}

inline void anSphere::Clear( void ) {
	origin.Zero();
	radius = -1.0f;
}

inline void anSphere::Zero( void ) {
	origin.Zero();
	radius = 0.0f;
}

inline void anSphere::SetOrigin( const anVec3 &o ) {
	origin = o;
}

inline void anSphere::SetRadius( const float r ) {
	radius = r;
}

inline const anVec3 &anSphere::GetOrigin( void ) const {
	return origin;
}

inline float anSphere::GetRadius( void ) const {
	return radius;
}

inline bool anSphere::IsCleared( void ) const {
	return ( radius < 0.0f );
}

inline bool anSphere::AddPoint( const anVec3 &p ) {
	if ( radius < 0.0f ) {
		origin = p;
		radius = 0.0f;
		return true;
	}
	else {
		float r = ( p - origin ).LengthSqr();
		if ( r > radius * radius ) {
			r = anMath::Sqrt( r );
			origin += ( p - origin ) * 0.5f * (1.0f - radius / r );
			radius += 0.5f * ( r - radius );
			return true;
		}
		return false;
	}
}

inline bool anSphere::AddSphere( const anSphere &s ) {
	if ( radius < 0.0f ) {
		origin = s.origin;
		radius = s.radius;
		return true;
	}
	else {
		float r = ( s.origin - origin ).LengthSqr();
		if ( r > ( radius + s.radius ) * ( radius + s.radius ) ) {
			r = anMath::Sqrt( r );
			origin += ( s.origin - origin ) * 0.5f * (1.0f - radius / ( r + s.radius ) );
			radius += 0.5f * ( ( r + s.radius ) - radius );
			return true;
		}
		return false;
	}
}

inline anSphere anSphere::Expand( const float d ) const {
	return anSphere( origin, radius + d );
}

inline anSphere &anSphere::ExpandSelf( const float d ) {
	radius += d;
	return *this;
}

inline anSphere anSphere::Translate( const anVec3 &translation ) const {
	return anSphere( origin + translation, radius );
}

inline anSphere &anSphere::TranslateSelf( const anVec3 &translation ) {
	origin += translation;
	return *this;
}

inline bool anSphere::ContainsPoint( const anVec3 &p ) const {
	if ( ( p - origin ).LengthSqr() > radius * radius ) {
		return false;
	}
	return true;
}

inline bool anSphere::IntersectsSphere( const anSphere &s ) const {
	float r = s.radius + radius;
	if ( ( s.origin - origin ).LengthSqr() > r * r ) {
		return false;
	}
	return true;
}

inline void anSphere::FromPointTranslation( const anVec3 &point, const anVec3 &translation ) {
	origin = point + 0.5f * translation;
	radius = anMath::Sqrt( 0.5f * translation.LengthSqr() );
}

inline void anSphere::FromSphereTranslation( const anSphere &sphere, const anVec3 &start, const anVec3 &translation ) {
	origin = start + sphere.origin + 0.5f * translation;
	radius = anMath::Sqrt( 0.5f * translation.LengthSqr() ) + sphere.radius;
}

inline void anSphere::FromPointRotation( const anVec3 &point, const anRotation &rotation ) {
	anVec3 end = rotation * point;
	origin = ( point + end ) * 0.5f;
	radius = anMath::Sqrt( 0.5f * ( end - point ).LengthSqr() );
}

inline void anSphere::FromSphereRotation( const anSphere &sphere, const anVec3 &start, const anRotation &rotation ) {
	anVec3 end = rotation * sphere.origin;
	origin = start + ( sphere.origin + end ) * 0.5f;
	radius = anMath::Sqrt( 0.5f * ( end - sphere.origin ).LengthSqr() ) + sphere.radius;
}

inline void anSphere::AxisProjection( const anVec3 &dir, float &min, float &max ) const {
	float d;
	d = dir * origin;
	min = d - radius;
	max = d + radius;
}

#endif /* !__BV_SPHERE_H__ */
