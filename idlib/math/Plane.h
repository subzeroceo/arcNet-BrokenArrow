#ifndef __MATH_PLANE_H__
#define __MATH_PLANE_H__

/*
===============================================================================

	3D plane with equation: a * x + b * y + c * z + d = 0

===============================================================================
*/


class arcVec3;
class arcMat3;

#define	ON_EPSILON					0.1f
#define DEGENERATE_DIST_EPSILON		1e-4f

#define	SIDE_FRONT					0
#define	SIDE_BACK					1
#define	SIDE_ON						2
#define	SIDE_CROSS					3

// plane sides
#define PLANESIDE_FRONT				0
#define PLANESIDE_BACK				1
#define PLANESIDE_ON				2
#define PLANESIDE_CROSS				3

// plane types
#define PLANETYPE_X					0
#define PLANETYPE_Y					1
#define PLANETYPE_Z					2
#define PLANETYPE_NEGX				3
#define PLANETYPE_NEGY				4
#define PLANETYPE_NEGZ				5
#define PLANETYPE_TRUEAXIAL			6	// all types < 6 are true axial planes
#define PLANETYPE_ZEROX				6
#define PLANETYPE_ZEROY				7
#define PLANETYPE_ZEROZ				8
#define PLANETYPE_NONAXIAL			9

class arcPlane {
public:
					arcPlane( void );
					arcPlane( float a, float b, float c, float d );
					arcPlane( const arcVec3 &normal, const float dist );

	float			operator[]( int index ) const;
	float &			operator[]( int index );
	arcPlane		operator-() const;						// flips plane
	arcPlane &		operator=( const arcVec3 &v );			// sets normal and sets arcPlane::d to zero
	arcPlane		operator+( const arcPlane &p ) const;	// add plane equations
	arcPlane		operator-( const arcPlane &p ) const;	// subtract plane equations
	arcPlane &		operator*=( const arcMat3 &m );			// Normal() *= m

	bool			Compare( const arcPlane &p ) const;						// exact compare, no epsilon
	bool			Compare( const arcPlane &p, const float epsilon ) const;	// compare with epsilon
	bool			Compare( const arcPlane &p, const float normalEps, const float distEps ) const;	// compare with epsilon
	bool			operator==(	const arcPlane &p ) const;					// exact compare, no epsilon
	bool			operator!=(	const arcPlane &p ) const;					// exact compare, no epsilon

	void			Zero( void );							// zero plane
	void			SetNormal( const arcVec3 &normal );		// sets the normal
	const arcVec3 &	Normal( void ) const;					// reference to const normal
	arcVec3 &		Normal( void );							// reference to normal
	float			Normalize( bool fixDegenerate = true );	// only normalizes the plane normal, does not adjust d
	bool			FixDegenerateNormal( void );			// fix degenerate normal
	bool			FixDegeneracies( float distEpsilon );	// fix degenerate normal and dist
	float			Dist( void ) const;						// returns: -d
	void			SetDist( const float dist );			// sets: d = -dist
	int				Type( void ) const;						// returns plane type

	bool			FromPoints( const arcVec3 &p1, const arcVec3 &p2, const arcVec3 &p3, bool fixDegenerate = true );
	bool			FromVecs( const arcVec3 &dir1, const arcVec3 &dir2, const arcVec3 &p, bool fixDegenerate = true );
	void			FitThroughPoint( const arcVec3 &p );	// assumes normal is valid
	bool			HeightFit( const arcVec3 *points, const int numPoints );
	arcPlane		Translate( const arcVec3 &translation ) const;
	arcPlane &		TranslateSelf( const arcVec3 &translation );
	arcPlane		Rotate( const arcVec3 &origin, const arcMat3 &axis ) const;
	arcPlane &		RotateSelf( const arcVec3 &origin, const arcMat3 &axis );

	float			Distance( const arcVec3 &v ) const;
	int				Side( const arcVec3 &v, const float epsilon = 0.0f ) const;

	bool			LineIntersection( const arcVec3 &start, const arcVec3 &end ) const;
					// intersection point is start + dir * scale
	bool			RayIntersection( const arcVec3 &start, const arcVec3 &dir, float &scale ) const;
	bool			PlaneIntersection( const arcPlane &plane, arcVec3 &start, arcVec3 &dir ) const;

	int				GetDimension( void ) const;

	const arcVec4 &	ToVec4( void ) const;
	arcVec4 &		ToVec4( void );
	const float *	ToFloatPtr( void ) const;
	float *			ToFloatPtr( void );
	const char *	ToString( int precision = 2 ) const;

private:
	float			a;
	float			b;
	float			c;
	float			d;
};

extern arcPlane plane_origin;
#define plane_zero plane_origin

ARC_INLINE arcPlane::arcPlane( void ) {
}

ARC_INLINE arcPlane::arcPlane( float a, float b, float c, float d ) {
	this->a = a;
	this->b = b;
	this->c = c;
	this->d = d;
}

ARC_INLINE arcPlane::arcPlane( const arcVec3 &normal, const float dist ) {
	this->a = normal.x;
	this->b = normal.y;
	this->c = normal.z;
	this->d = -dist;
}

ARC_INLINE float arcPlane::operator[]( int index ) const {
	return ( &a )[index];
}

ARC_INLINE float& arcPlane::operator[]( int index ) {
	return ( &a )[index];
}

ARC_INLINE arcPlane arcPlane::operator-() const {
	return arcPlane( -a, -b, -c, -d );
}

ARC_INLINE arcPlane &arcPlane::operator=( const arcVec3 &v ) {
	a = v.x;
	b = v.y;
	c = v.z;
	d = 0;
	return *this;
}

ARC_INLINE arcPlane arcPlane::operator+( const arcPlane &p ) const {
	return arcPlane( a + p.a, b + p.b, c + p.c, d + p.d );
}

ARC_INLINE arcPlane arcPlane::operator-( const arcPlane &p ) const {
	return arcPlane( a - p.a, b - p.b, c - p.c, d - p.d );
}

ARC_INLINE arcPlane &arcPlane::operator*=( const arcMat3 &m ) {
	Normal() *= m;
	return *this;
}

ARC_INLINE bool arcPlane::Compare( const arcPlane &p ) const {
	return ( a == p.a && b == p.b && c == p.c && d == p.d );
}

ARC_INLINE bool arcPlane::Compare( const arcPlane &p, const float epsilon ) const {
	if ( arcMath::Fabs( a - p.a ) > epsilon ) {
		return false;
	}

	if ( arcMath::Fabs( b - p.b ) > epsilon ) {
		return false;
	}

	if ( arcMath::Fabs( c - p.c ) > epsilon ) {
		return false;
	}

	if ( arcMath::Fabs( d - p.d ) > epsilon ) {
		return false;
	}

	return true;
}

ARC_INLINE bool arcPlane::Compare( const arcPlane &p, const float normalEps, const float distEps ) const {
	if ( arcMath::Fabs( d - p.d ) > distEps ) {
		return false;
	}
	if ( !Normal().Compare( p.Normal(), normalEps ) ) {
		return false;
	}
	return true;
}

ARC_INLINE bool arcPlane::operator==( const arcPlane &p ) const {
	return Compare( p );
}

ARC_INLINE bool arcPlane::operator!=( const arcPlane &p ) const {
	return !Compare( p );
}

ARC_INLINE void arcPlane::Zero( void ) {
	a = b = c = d = 0.0f;
}

ARC_INLINE void arcPlane::SetNormal( const arcVec3 &normal ) {
	a = normal.x;
	b = normal.y;
	c = normal.z;
}

ARC_INLINE const arcVec3 &arcPlane::Normal( void ) const {
	return *reinterpret_cast<const arcVec3 *>(&a);
}

ARC_INLINE arcVec3 &arcPlane::Normal( void ) {
	return *reinterpret_cast<arcVec3 *>(&a);
}

ARC_INLINE float arcPlane::Normalize( bool fixDegenerate ) {
	float length = reinterpret_cast<arcVec3 *>(&a)->Normalize();

	if ( fixDegenerate ) {
		FixDegenerateNormal();
	}
	return length;
}

ARC_INLINE bool arcPlane::FixDegenerateNormal( void ) {
	return Normal().FixDegenerateNormal();
}

ARC_INLINE bool arcPlane::FixDegeneracies( float distEpsilon ) {
	bool fixedNormal = FixDegenerateNormal();
	// only fix dist if the normal was degenerate
	if ( fixedNormal ) {
		if ( arcMath::Fabs( d - arcMath::Rint( d ) ) < distEpsilon ) {
			d = arcMath::Rint( d );
		}
	}
	return fixedNormal;
}

ARC_INLINE float arcPlane::Dist( void ) const {
	return -d;
}

ARC_INLINE void arcPlane::SetDist( const float dist ) {
	d = -dist;
}

ARC_INLINE bool arcPlane::FromPoints( const arcVec3 &p1, const arcVec3 &p2, const arcVec3 &p3, bool fixDegenerate ) {
	Normal() = (p1 - p2).Cross( p3 - p2 );
	if ( Normalize( fixDegenerate ) == 0.0f ) {
		return false;
	}
	d = -( Normal() * p2 );
	return true;
}

ARC_INLINE bool arcPlane::FromVecs( const arcVec3 &dir1, const arcVec3 &dir2, const arcVec3 &p, bool fixDegenerate ) {
	Normal() = dir1.Cross( dir2 );
	if ( Normalize( fixDegenerate ) == 0.0f ) {
		return false;
	}
	d = -( Normal() * p );
	return true;
}

ARC_INLINE void arcPlane::FitThroughPoint( const arcVec3 &p ) {
	d = -( Normal() * p );
}

ARC_INLINE arcPlane arcPlane::Translate( const arcVec3 &translation ) const {
	return arcPlane( a, b, c, d - translation * Normal() );
}

ARC_INLINE arcPlane &arcPlane::TranslateSelf( const arcVec3 &translation ) {
	d -= translation * Normal();
	return *this;
}

ARC_INLINE arcPlane arcPlane::Rotate( const arcVec3 &origin, const arcMat3 &axis ) const {
	arcPlane p;
	p.Normal() = Normal() * axis;
	p.d = d + origin * Normal() - origin * p.Normal();
	return p;
}

ARC_INLINE arcPlane &arcPlane::RotateSelf( const arcVec3 &origin, const arcMat3 &axis ) {
	d += origin * Normal();
	Normal() *= axis;
	d -= origin * Normal();
	return *this;
}

ARC_INLINE float arcPlane::Distance( const arcVec3 &v ) const {
	return a * v.x + b * v.y + c * v.z + d;
}

ARC_INLINE int arcPlane::Side( const arcVec3 &v, const float epsilon ) const {
	float dist = Distance( v );
	if ( dist > epsilon ) {
		return PLANESIDE_FRONT;
	}
	else if ( dist < -epsilon ) {
		return PLANESIDE_BACK;
	}
	else {
		return PLANESIDE_ON;
	}
}

ARC_INLINE bool arcPlane::LineIntersection( const arcVec3 &start, const arcVec3 &end ) const {
	float d1, d2, fraction;

	d1 = Normal() * start + d;
	d2 = Normal() * end + d;
	if ( d1 == d2 ) {
		return false;
	}
	if ( d1 > 0.0f && d2 > 0.0f ) {
		return false;
	}
	if ( d1 < 0.0f && d2 < 0.0f ) {
		return false;
	}
	fraction = ( d1 / ( d1 - d2 ) );
	return ( fraction >= 0.0f && fraction <= 1.0f );
}

ARC_INLINE bool arcPlane::RayIntersection( const arcVec3 &start, const arcVec3 &dir, float &scale ) const {
	float d1, d2;

	d1 = Normal() * start + d;
	d2 = Normal() * dir;
	if ( d2 == 0.0f ) {
		return false;
	}
	scale = -( d1 / d2 );
	return true;
}

ARC_INLINE int arcPlane::GetDimension( void ) const {
	return 4;
}

ARC_INLINE const arcVec4 &arcPlane::ToVec4( void ) const {
	return *reinterpret_cast<const arcVec4 *>(&a);
}

ARC_INLINE arcVec4 &arcPlane::ToVec4( void ) {
	return *reinterpret_cast<arcVec4 *>(&a);
}

ARC_INLINE const float *arcPlane::ToFloatPtr( void ) const {
	return reinterpret_cast<const float *>(&a);
}

ARC_INLINE float *arcPlane::ToFloatPtr( void ) {
	return reinterpret_cast<float *>(&a);
}

#endif /* !__MATH_PLANE_H__ */
