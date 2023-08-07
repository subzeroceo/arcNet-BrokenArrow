#ifndef __WINDING_H__
#define __WINDING_H__

/*
===============================================================================

	A winding is an arbitrary convex polygon defined by an array of points.

===============================================================================
*/

class anWinding {

public:
					anWinding( void );
					explicit anWinding( const int n );								// allocate for n points
					explicit anWinding( const anVec3 *verts, const int n );			// winding from points
					explicit anWinding( const anVec3 &normal, const float dist );	// base winding for plane
					explicit anWinding( const anPlane &plane );						// base winding for plane
					explicit anWinding( const anWinding &winding );
	virtual			~anWinding( void );

	anWinding &	operator=( const anWinding &winding );
	const anVec5 &	operator[]( const int index ) const;
	anVec5 &		operator[]( const int index );

					// add a point to the end of the winding point array
	anWinding &	operator+=( const anVec3 &v );
	anWinding &	operator+=( const anVec5 &v );
	void			AddPoint( const anVec3 &v );
	void			AddPoint( const anVec5 &v );

					// number of points on winding
	int				GetNumPoints( void ) const;
	void			SetNumPoints( int n );
	virtual void	Clear( void );

					// huge winding for plane, the points go counter clockwise when facing the front of the plane
	void			BaseForPlane( const anVec3 &normal, const float dist );
	void			BaseForPlane( const anPlane &plane );

					// splits the winding into a front and back winding, the winding itself stays unchanged
					// returns a SIDE_?
	int				Split( const anPlane &plane, const float epsilon, anWinding **front, anWinding **back ) const;
					// returns the winding fragment at the front of the clipping plane,
					// if there is nothing at the front the winding itself is destroyed and nullptr is returned
	anWinding *	Clip( const anPlane &plane, const float epsilon = ON_EPSILON, const bool keepOn = false );
					// cuts off the part at the back side of the plane, returns true if some part was at the front
					// if there is nothing at the front the number of points is set to zero
	bool			ClipInPlace( const anPlane &plane, const float epsilon = ON_EPSILON, const bool keepOn = false );

					// returns a copy of the winding
	anWinding *	Copy( void ) const;
	anWinding *	Reverse( void ) const;
	void			ReverseSelf( void );
	void			RemoveEqualPoints( const float epsilon = ON_EPSILON );
	void			RemoveColinearPoints( const anVec3 &normal, const float epsilon = ON_EPSILON );
	void			RemovePoint( int point );
	void			InsertPoint( const anVec3 &point, int spot );
	bool			InsertPointIfOnEdge( const anVec3 &point, const anPlane &plane, const float epsilon = ON_EPSILON );
					// add a winding to the convex hull
	void			AddToConvexHull( const anWinding *winding, const anVec3 &normal, const float epsilon = ON_EPSILON );
					// add a point to the convex hull
	void			AddToConvexHull( const anVec3 &point, const anVec3 &normal, const float epsilon = ON_EPSILON );
					// tries to merge 'this' with the given winding, returns nullptr if merge fails, both 'this' and 'w' stay intact
					// 'keep' tells if the contacting points should stay even if they create colinear edges
	anWinding *	TryMerge( const anWinding &w, const anVec3 &normal, int keep = false ) const;
					// check whether the winding is valid or not
	bool			Check( bool print = true ) const;

	float			GetArea( void ) const;
	anVec3			GetCenter( void ) const;
	float			GetRadius( const anVec3 &center ) const;
	void			GetPlane( anVec3 &normal, float &dist ) const;
	void			GetPlane( anPlane &plane ) const;
	void			GetBounds( anBounds &bounds ) const;

	bool			IsTiny( void ) const;
	bool			IsHuge( void ) const;	// base winding for a plane is typically huge
	void			Print( void ) const;

	float			PlaneDistance( const anPlane &plane ) const;
	int				PlaneSide( const anPlane &plane, const float epsilon = ON_EPSILON ) const;

	bool			PlanesConcave( const anWinding &w2, const anVec3 &normal1, const anVec3 &normal2, float dist1, float dist2 ) const;

	bool			PointInside( const anVec3 &normal, const anVec3 &point, const float epsilon ) const;
					// returns true if the line or ray intersects the winding
	bool			LineIntersection( const anPlane &windingPlane, const anVec3 &start, const anVec3 &end, bool backFaceCull = false ) const;
					// intersection point is start + dir * scale
	bool			RayIntersection( const anPlane &windingPlane, const anVec3 &start, const anVec3 &dir, float &scale, bool backFaceCull = false ) const;

	static float	TriangleArea( const anVec3 &a, const anVec3 &b, const anVec3 &c );

protected:
	int				numPoints;				// number of points
	anVec5 *		p;						// pointer to point data
	int				allocedSize;

	bool			EnsureAlloced( int n, bool keep = false );
	virtual bool	ReAllocate( int n, bool keep = false );
};

inline anWinding::anWinding( void ) {
	numPoints = allocedSize = 0;
	p = nullptr;
}

inline anWinding::anWinding( int n ) {
	numPoints = allocedSize = 0;
	p = nullptr;
	EnsureAlloced( n );
}

inline anWinding::anWinding( const anVec3 *verts, const int n ) {
	int i;

	numPoints = allocedSize = 0;
	p = nullptr;
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

inline anWinding::anWinding( const anVec3 &normal, const float dist ) {
	numPoints = allocedSize = 0;
	p = nullptr;
	BaseForPlane( normal, dist );
}

inline anWinding::anWinding( const anPlane &plane ) {
	numPoints = allocedSize = 0;
	p = nullptr;
	BaseForPlane( plane );
}

inline anWinding::anWinding( const anWinding &winding ) {
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

inline anWinding::~anWinding( void ) {
	delete[] p;
	p = nullptr;
}

inline anWinding &anWinding::operator=( const anWinding &winding ) {
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

inline const anVec5 &anWinding::operator[]( const int index ) const {
	//assert( index >= 0 && index < numPoints );
	return p[index];
}

inline anVec5 &anWinding::operator[]( const int index ) {
	//assert( index >= 0 && index < numPoints );
	return p[index];
}

inline anWinding &anWinding::operator+=( const anVec3 &v ) {
	AddPoint( v );
	return *this;
}

inline anWinding &anWinding::operator+=( const anVec5 &v ) {
	AddPoint( v );
	return *this;
}

inline void anWinding::AddPoint( const anVec3 &v ) {
	if ( !EnsureAlloced(numPoints+1, true) ) {
		return;
	}
	p[numPoints] = v;
	numPoints++;
}

inline void anWinding::AddPoint( const anVec5 &v ) {
	if ( !EnsureAlloced(numPoints+1, true) ) {
		return;
	}
	p[numPoints] = v;
	numPoints++;
}

inline int anWinding::GetNumPoints( void ) const {
	return numPoints;
}

inline void anWinding::SetNumPoints( int n ) {
	if ( !EnsureAlloced( n, true ) ) {
		return;
	}
	numPoints = n;
}

inline void anWinding::Clear( void ) {
	numPoints = 0;
	delete[] p;
	p = nullptr;
}

inline void anWinding::BaseForPlane( const anPlane &plane ) {
	BaseForPlane( plane.Normal(), plane.Dist() );
}

inline bool anWinding::EnsureAlloced( int n, bool keep ) {
	if ( n > allocedSize ) {
		return ReAllocate( n, keep );
	}
	return true;
}


/*
===============================================================================

	anFixedWinding is a fixed buffer size winding not using
	memory allocations.

	When an operation would overflow the fixed buffer a warning
	is printed and the operation is safely cancelled.

===============================================================================
*/

#define	MAX_POINTS_ON_WINDING	64

class anFixedWinding : public anWinding {

public:
					anFixedWinding( void );
					explicit anFixedWinding( const int n );
					explicit anFixedWinding( const anVec3 *verts, const int n );
					explicit anFixedWinding( const anVec3 &normal, const float dist );
					explicit anFixedWinding( const anPlane &plane );
					explicit anFixedWinding( const anWinding &winding );
					explicit anFixedWinding( const anFixedWinding &winding );
	virtual			~anFixedWinding( void );

	anFixedWinding &operator=( const anWinding &winding );

	virtual void	Clear( void );

					// splits the winding in a back and front part, 'this' becomes the front part
					// returns a SIDE_?
	int				Split( anFixedWinding *back, const anPlane &plane, const float epsilon = ON_EPSILON );

protected:
	anVec5			data[MAX_POINTS_ON_WINDING];	// point data

	virtual bool	ReAllocate( int n, bool keep = false );
};

inline anFixedWinding::anFixedWinding( void ) {
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
}

inline anFixedWinding::anFixedWinding( int n ) {
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
}

inline anFixedWinding::anFixedWinding( const anVec3 *verts, const int n ) {
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

inline anFixedWinding::anFixedWinding( const anVec3 &normal, const float dist ) {
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
	BaseForPlane( normal, dist );
}

inline anFixedWinding::anFixedWinding( const anPlane &plane ) {
	numPoints = 0;
	p = data;
	allocedSize = MAX_POINTS_ON_WINDING;
	BaseForPlane( plane );
}

inline anFixedWinding::anFixedWinding( const anWinding &winding ) {
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

inline anFixedWinding::anFixedWinding( const anFixedWinding &winding ) {
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

inline anFixedWinding::~anFixedWinding( void ) {
	p = nullptr;	// otherwise it tries to free the fixed buffer
}

inline anFixedWinding &anFixedWinding::operator=( const anWinding &winding ) {
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

inline void anFixedWinding::Clear( void ) {
	numPoints = 0;
}
#endif	/* !__WINDING_H__ */
