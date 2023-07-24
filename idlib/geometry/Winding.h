#ifndef __WINDING_H__
#define __WINDING_H__

/*
===============================================================================

	A winding is an arbitrary convex polygon defined by an array of points.

===============================================================================
*/

class arcWinding {

public:
					arcWinding( void );
					explicit arcWinding( const int n );								// allocate for n points
					explicit arcWinding( const arcVec3 *verts, const int n );			// winding from points
					explicit arcWinding( const arcVec3 &normal, const float dist );	// base winding for plane
					explicit arcWinding( const arcPlane &plane );						// base winding for plane
					explicit arcWinding( const arcWinding &winding );
	virtual			~arcWinding( void );

	arcWinding &	operator=( const arcWinding &winding );
	const arcVec5 &	operator[]( const int index ) const;
	arcVec5 &		operator[]( const int index );

					// add a point to the end of the winding point array
	arcWinding &	operator+=( const arcVec3 &v );
	arcWinding &	operator+=( const arcVec5 &v );
	void			AddPoint( const arcVec3 &v );
	void			AddPoint( const arcVec5 &v );

					// number of points on winding
	int				GetNumPoints( void ) const;
	void			SetNumPoints( int n );
	virtual void	Clear( void );

					// huge winding for plane, the points go counter clockwise when facing the front of the plane
	void			BaseForPlane( const arcVec3 &normal, const float dist );
	void			BaseForPlane( const arcPlane &plane );

					// splits the winding into a front and back winding, the winding itself stays unchanged
					// returns a SIDE_?
	int				Split( const arcPlane &plane, const float epsilon, arcWinding **front, arcWinding **back ) const;
					// returns the winding fragment at the front of the clipping plane,
					// if there is nothing at the front the winding itself is destroyed and NULL is returned
	arcWinding *	Clip( const arcPlane &plane, const float epsilon = ON_EPSILON, const bool keepOn = false );
					// cuts off the part at the back side of the plane, returns true if some part was at the front
					// if there is nothing at the front the number of points is set to zero
	bool			ClipInPlace( const arcPlane &plane, const float epsilon = ON_EPSILON, const bool keepOn = false );

					// returns a copy of the winding
	arcWinding *	Copy( void ) const;
	arcWinding *	Reverse( void ) const;
	void			ReverseSelf( void );
	void			RemoveEqualPoints( const float epsilon = ON_EPSILON );
	void			RemoveColinearPoints( const arcVec3 &normal, const float epsilon = ON_EPSILON );
	void			RemovePoint( int point );
	void			InsertPoint( const arcVec3 &point, int spot );
	bool			InsertPointIfOnEdge( const arcVec3 &point, const arcPlane &plane, const float epsilon = ON_EPSILON );
					// add a winding to the convex hull
	void			AddToConvexHull( const arcWinding *winding, const arcVec3 &normal, const float epsilon = ON_EPSILON );
					// add a point to the convex hull
	void			AddToConvexHull( const arcVec3 &point, const arcVec3 &normal, const float epsilon = ON_EPSILON );
					// tries to merge 'this' with the given winding, returns NULL if merge fails, both 'this' and 'w' stay intact
					// 'keep' tells if the contacting points should stay even if they create colinear edges
	arcWinding *	TryMerge( const arcWinding &w, const arcVec3 &normal, int keep = false ) const;
					// check whether the winding is valid or not
	bool			Check( bool print = true ) const;

	float			GetArea( void ) const;
	arcVec3			GetCenter( void ) const;
	float			GetRadius( const arcVec3 &center ) const;
	void			GetPlane( arcVec3 &normal, float &dist ) const;
	void			GetPlane( arcPlane &plane ) const;
	void			GetBounds( arcBounds &bounds ) const;

	bool			IsTiny( void ) const;
	bool			IsHuge( void ) const;	// base winding for a plane is typically huge
	void			Print( void ) const;

	float			PlaneDistance( const arcPlane &plane ) const;
	int				PlaneSide( const arcPlane &plane, const float epsilon = ON_EPSILON ) const;

	bool			PlanesConcave( const arcWinding &w2, const arcVec3 &normal1, const arcVec3 &normal2, float dist1, float dist2 ) const;

	bool			PointInside( const arcVec3 &normal, const arcVec3 &point, const float epsilon ) const;
					// returns true if the line or ray intersects the winding
	bool			LineIntersection( const arcPlane &windingPlane, const arcVec3 &start, const arcVec3 &end, bool backFaceCull = false ) const;
					// intersection point is start + dir * scale
	bool			RayIntersection( const arcPlane &windingPlane, const arcVec3 &start, const arcVec3 &dir, float &scale, bool backFaceCull = false ) const;

	static float	TriangleArea( const arcVec3 &a, const arcVec3 &b, const arcVec3 &c );

protected:
	int				numPoints;				// number of points
	arcVec5 *		p;						// pointer to point data
	int				allocedSize;

	bool			EnsureAlloced( int n, bool keep = false );
	virtual bool	ReAllocate( int n, bool keep = false );
};

ARC_INLINE arcWinding::arcWinding( void ) {
	numPoints = allocedSize = 0;
	p = NULL;
}

ARC_INLINE arcWinding::arcWinding( int n ) {
	numPoints = allocedSize = 0;
	p = NULL;
	EnsureAlloced( n );
}

ARC_INLINE arcWinding::arcWinding( const arcVec3 *verts, const int n ) {
	int i;

	numPoints = allocedSize = 0;
	p = NULL;
	if ( !EnsureAlloced( n ) ) {
		numPoints = 0;
		return;
	}
	for ( i = 0; i < n; i++ ) {
		p[i].ToVec3() = verts[i];
		p[i].s = p[i].t = 0.0f;
	}
	numPoints = n;
}

ARC_INLINE arcWinding::arcWinding( const arcVec3 &normal, const float dist ) {
	numPoints = allocedSize = 0;
	p = NULL;
	BaseForPlane( normal, dist );
}

ARC_INLINE arcWinding::arcWinding( const arcPlane &plane ) {
	numPoints = allocedSize = 0;
	p = NULL;
	BaseForPlane( plane );
}

ARC_INLINE arcWinding::arcWinding( const arcWinding &winding ) {
	int i;
	if ( !EnsureAlloced( winding.GetNumPoints() ) ) {
		numPoints = 0;
		return;
	}
	for ( i = 0; i < winding.GetNumPoints(); i++ ) {
		p[i] = winding[i];
	}
	numPoints = winding.GetNumPoints();
}

ARC_INLINE arcWinding::~arcWinding( void ) {
	delete[] p;
	p = NULL;
}

ARC_INLINE arcWinding &arcWinding::operator=( const arcWinding &winding ) {
	int i;

	if ( !EnsureAlloced( winding.numPoints ) ) {
		numPoints = 0;
		return *this;
	}
	for ( i = 0; i < winding.numPoints; i++ ) {
		p[i] = winding.p[i];
	}
	numPoints = winding.numPoints;
	return *this;
}

ARC_INLINE const arcVec5 &arcWinding::operator[]( const int index ) const {
	//assert( index >= 0 && index < numPoints );
	return p[index];
}

ARC_INLINE arcVec5 &arcWinding::operator[]( const int index ) {
	//assert( index >= 0 && index < numPoints );
	return p[index];
}

ARC_INLINE arcWinding &arcWinding::operator+=( const arcVec3 &v ) {
	AddPoint( v );
	return *this;
}

ARC_INLINE arcWinding &arcWinding::operator+=( const arcVec5 &v ) {
	AddPoint( v );
	return *this;
}

ARC_INLINE void arcWinding::AddPoint( const arcVec3 &v ) {
	if ( !EnsureAlloced(numPoints+1, true) ) {
		return;
	}
	p[numPoints] = v;
	numPoints++;
}

ARC_INLINE void arcWinding::AddPoint( const arcVec5 &v ) {
	if ( !EnsureAlloced(numPoints+1, true) ) {
		return;
	}
	p[numPoints] = v;
	numPoints++;
}

ARC_INLINE int arcWinding::GetNumPoints( void ) const {
	return numPoints;
}

ARC_INLINE void arcWinding::SetNumPoints( int n ) {
	if ( !EnsureAlloced( n, true ) ) {
		return;
	}
	numPoints = n;
}

ARC_INLINE void arcWinding::Clear( void ) {
	numPoints = 0;
	delete[] p;
	p = NULL;
}

ARC_INLINE void arcWinding::BaseForPlane( const arcPlane &plane ) {
	BaseForPlane( plane.Normal(), plane.Dist() );
}

ARC_INLINE bool arcWinding::EnsureAlloced( int n, bool keep ) {
	if ( n > allocedSize ) {
		return ReAllocate( n, keep );
	}
	return true;
}


/*
===============================================================================

	arcFixedWinding is a fixed buffer size winding not using
	memory allocations.

	When an operation would overflow the fixed buffer a warning
	is printed and the operation is safely cancelled.

===============================================================================
*/

#define	MAX_POINTS_ON_WINDING	64

class arcFixedWinding : public arcWinding {

public:
					arcFixedWinding( void );
					explicit arcFixedWinding( const int n );
					explicit arcFixedWinding( const arcVec3 *verts, const int n );
					explicit arcFixedWinding( const arcVec3 &normal, const float dist );
					explicit arcFixedWinding( const arcPlane &plane );
					explicit arcFixedWinding( const arcWinding &winding );
					explicit arcFixedWinding( const arcFixedWinding &winding );
	virtual			~arcFixedWinding( void );

	arcFixedWinding &operator=( const arcWinding &winding );

	virtual void	Clear( void );

					// splits the winding in a back and front part, 'this' becomes the front part
					// returns a SIDE_?
	int				Split( arcFixedWinding *back, const arcPlane &plane, const float epsilon = ON_EPSILON );

protected:
	arcVec5			data[MAX_POINTS_ON_WINDING];	// point data

	virtual bool	ReAllocate( int n, bool keep = false );
};

ARC_INLINE arcFixedWinding::arcFixedWinding( void ) {
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
}

ARC_INLINE arcFixedWinding::arcFixedWinding( int n ) {
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
}

ARC_INLINE arcFixedWinding::arcFixedWinding( const arcVec3 *verts, const int n ) {
	int i;

	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
	if ( !EnsureAlloced( n ) ) {
		numPoints = 0;
		return;
	}
	for ( i = 0; i < n; i++ ) {
		p[i].ToVec3() = verts[i];
		p[i].s = p[i].t = 0;
	}
	numPoints = n;
}

ARC_INLINE arcFixedWinding::arcFixedWinding( const arcVec3 &normal, const float dist ) {
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
	BaseForPlane( normal, dist );
}

ARC_INLINE arcFixedWinding::arcFixedWinding( const arcPlane &plane ) {
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
	BaseForPlane( plane );
}

ARC_INLINE arcFixedWinding::arcFixedWinding( const arcWinding &winding ) {
	int i;

	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
	if ( !EnsureAlloced( winding.GetNumPoints() ) ) {
		numPoints = 0;
		return;
	}
	for ( i = 0; i < winding.GetNumPoints(); i++ ) {
		p[i] = winding[i];
	}
	numPoints = winding.GetNumPoints();
}

ARC_INLINE arcFixedWinding::arcFixedWinding( const arcFixedWinding &winding ) {
	int i;

	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
	if ( !EnsureAlloced( winding.GetNumPoints() ) ) {
		numPoints = 0;
		return;
	}
	for ( i = 0; i < winding.GetNumPoints(); i++ ) {
		p[i] = winding[i];
	}
	numPoints = winding.GetNumPoints();
}

ARC_INLINE arcFixedWinding::~arcFixedWinding( void ) {
	p = NULL;	// otherwise it tries to free the fixed buffer
}

ARC_INLINE arcFixedWinding &arcFixedWinding::operator=( const arcWinding &winding ) {
	int i;

	if ( !EnsureAlloced( winding.GetNumPoints() ) ) {
		numPoints = 0;
		return *this;
	}
	for ( i = 0; i < winding.GetNumPoints(); i++ ) {
		p[i] = winding[i];
	}
	numPoints = winding.GetNumPoints();
	return *this;
}

ARC_INLINE void arcFixedWinding::Clear( void ) {
	numPoints = 0;
}
#endif	/* !__WINDING_H__ */
