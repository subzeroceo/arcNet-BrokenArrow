#ifndef __WINDING2D_H__
#define __WINDING2D_H__

/*
===============================================================================

	A 2D winding is an arbitrary convex 2D polygon defined by an array of points.

===============================================================================
*/

#define	MAX_POINTS_ON_WINDING_2D		16


class idWinding2D {
public:
					idWinding2D( void );

	idWinding2D &	operator=( const idWinding2D &winding );
	const anVec2 &	operator[]( const int index ) const;
	anVec2 &		operator[]( const int index );

	void			Clear( void );
	void			AddPoint( const anVec2 &point );
	int				GetNumPoints( void ) const;

	void			Expand( const float d );
	void			ExpandForAxialBox( const anVec2 bounds[2] );

					// splits the winding into a front and back winding, the winding itself stays unchanged
					// returns a SIDE_?
	int				Split( const anVec3 &plane, const float epsilon, idWinding2D **front, idWinding2D **back ) const;
					// cuts off the part at the back side of the plane, returns true if some part was at the front
					// if there is nothing at the front the number of points is set to zero
	bool			ClipInPlace( const anVec3 &plane, const float epsilon = ON_EPSILON, const bool keepOn = false );

	idWinding2D *	Copy( void ) const;
	idWinding2D *	Reverse( void ) const;

	float			GetArea( void ) const;
	anVec2			GetCenter( void ) const;
	float			GetRadius( const anVec2 &center ) const;
	void			GetBounds( anVec2 bounds[2] ) const;

	bool			IsTiny( void ) const;
	bool			IsHuge( void ) const;	// base winding for a plane is typically huge
	void			Print( void ) const;

	float			PlaneDistance( const anVec3 &plane ) const;
	int				PlaneSide( const anVec3 &plane, const float epsilon = ON_EPSILON ) const;

	bool			PointInside( const anVec2 &point, const float epsilon ) const;
	bool			LineIntersection( const anVec2 &start, const anVec2 &end ) const;
	bool			RayIntersection( const anVec2 &start, const anVec2 &dir, float &scale1, float &scale2, int *edgeNums = nullptr ) const;

	static anVec3	Plane2DFromPoints( const anVec2 &start, const anVec2 &end, const bool normalize = false );
	static anVec3	Plane2DFromVecs( const anVec2 &start, const anVec2 &dir, const bool normalize = false );
	static bool		Plane2DIntersection( const anVec3 &plane1, const anVec3 &plane2, anVec2 &point );

private:
	int				numPoints;
	anVec2			p[MAX_POINTS_ON_WINDING_2D];
};

inline idWinding2D::idWinding2D( void ) {
	numPoints = 0;
}

inline idWinding2D &idWinding2D::operator=( const idWinding2D &winding ) {
	int i;

	for ( i = 0; i < winding.numPoints; i++ ) {
		p[i] = winding.p[i];
	}
	numPoints = winding.numPoints;
	return *this;
}

inline const anVec2 &idWinding2D::operator[]( const int index ) const {
	return p[index];
}

inline anVec2 &idWinding2D::operator[]( const int index ) {
	return p[index];
}

inline void idWinding2D::Clear( void ) {
	numPoints = 0;
}

inline void idWinding2D::AddPoint( const anVec2 &point ) {
	p[numPoints++] = point;
}

inline int idWinding2D::GetNumPoints( void ) const {
	return numPoints;
}

inline anVec3 idWinding2D::Plane2DFromPoints( const anVec2 &start, const anVec2 &end, const bool normalize ) {
	anVec3 plane;
	plane.x = start.y - end.y;
	plane.y = end.x - start.x;
	if ( normalize ) {
		plane.ToVec2().Normalize();
	}
	plane.z = - ( start.x * plane.x + start.y * plane.y );
	return plane;
}

inline anVec3 idWinding2D::Plane2DFromVecs( const anVec2 &start, const anVec2 &dir, const bool normalize ) {
	anVec3 plane;
	plane.x = -dir.y;
	plane.y = dir.x;
	if ( normalize ) {
		plane.ToVec2().Normalize();
	}
	plane.z = - ( start.x * plane.x + start.y * plane.y );
	return plane;
}

inline bool idWinding2D::Plane2DIntersection( const anVec3 &plane1, const anVec3 &plane2, anVec2 &point ) {
	float n00, n01, n11, det, invDet, f0, f1;

	n00 = plane1.x * plane1.x + plane1.y * plane1.y;
	n01 = plane1.x * plane2.x + plane1.y * plane2.y;
	n11 = plane2.x * plane2.x + plane2.y * plane2.y;
	det = n00 * n11 - n01 * n01;

	if ( anMath::Fabs(det) < 1e-6f ) {
		return false;
	}

	invDet = 1.0f / det;
	f0 = ( n01 * plane2.z - n11 * plane1.z ) * invDet;
	f1 = ( n01 * plane1.z - n00 * plane2.z ) * invDet;
	point.x = f0 * plane1.x + f1 * plane2.x;
	point.y = f0 * plane1.y + f1 * plane2.y;
	return true;
}

#endif /* !__WINDING2D_H__ */
