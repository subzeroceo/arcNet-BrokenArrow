#ifndef __SURFACE_H__
#define __SURFACE_H__

/*
===============================================================================

	Surface base class.

	A surface is tesselated to a triangle mesh with each edge shared by
	at most two triangles.

===============================================================================
*/

typedef struct surfaceEdge_s {
	int						verts[2];	// edge vertices always with ( verts[0] < verts[1] )
	int						tris[2];	// edge triangles
} surfaceEdge_t;


class arcSurface {
public:
							arcSurface( void );
							explicit arcSurface( const arcSurface &surf );
							explicit arcSurface( const arcDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes );
							~arcSurface( void );

	const arcDrawVert &		operator[]( const int index ) const;
	arcDrawVert &			operator[]( const int index );
	arcSurface &				operator+=( const arcSurface &surf );

	int						GetNumIndexes( void ) const { return indexes.Num(); }
	const int *				GetIndexes( void ) const { return indexes.Ptr(); }
	int						GetNumVertices( void ) const { return verts.Num(); }
	const arcDrawVert *		GetVertices( void ) const { return verts.Ptr(); }
	const int *				GetEdgeIndexes( void ) const { return edgeIndexes.Ptr(); }
	const surfaceEdge_t *	GetEdges( void ) const { return edges.Ptr(); }

	void					Clear( void );
	void					SwapTriangles( arcSurface &surf );
	void					TranslateSelf( const arcVec3 &translation );
	void					RotateSelf( const arcMat3 &rotation );

							// splits the surface into a front and back surface, the surface itself stays unchanged
							// frontOnPlaneEdges and backOnPlaneEdges optionally store the indexes to the edges that lay on the split plane
							// returns a SIDE_?
	int						Split( const arcPlane &plane, const float epsilon, arcSurface **front, arcSurface **back, int *frontOnPlaneEdges = NULL, int *backOnPlaneEdges = NULL ) const;
							// cuts off the part at the back side of the plane, returns true if some part was at the front
							// if there is nothing at the front the number of points is set to zero
	bool					ClipInPlace( const arcPlane &plane, const float epsilon = ON_EPSILON, const bool keepOn = false );

							// returns true if each triangle can be reached from any other triangle by a traversal
	bool					IsConnected( void ) const;
							// returns true if the surface is closed
	bool					IsClosed( void ) const;
							// returns true if the surface is a convex hull
	bool					IsPolytope( const float epsilon = 0.1f ) const;

	float					PlaneDistance( const arcPlane &plane ) const;
	int						PlaneSide( const arcPlane &plane, const float epsilon = ON_EPSILON ) const;

							// returns true if the line intersects one of the surface triangles
	bool					LineIntersection( const arcVec3 &start, const arcVec3 &end, bool backFaceCull = false ) const;
							// intersection point is start + dir * scale
	bool					RayIntersection( const arcVec3 &start, const arcVec3 &dir, float &scale, bool backFaceCull = false ) const;

protected:
	arcNetList<arcDrawVert>		verts;			// vertices
	arcNetList<int>				indexes;		// 3 references to vertices for each triangle
	arcNetList<surfaceEdge_t>	edges;			// edges
	arcNetList<int>				edgeIndexes;	// 3 references to edges for each triangle, may be negative for reversed edge

protected:
	void					GenerateEdgeIndexes( void );
	int						FindEdge( int v1, int v2 ) const;
};

/*
====================
arcSurface::arcSurface
====================
*/
ARC_INLINE arcSurface::arcSurface( void ) {
}

/*
=================
arcSurface::arcSurface
=================
*/
ARC_INLINE arcSurface::arcSurface( const arcDrawVert *verts, const int numVerts, const int *indexes, const int numIndexes ) {
	assert( verts != NULL && indexes != NULL && numVerts > 0 && numIndexes > 0 );
	this->verts.SetNum( numVerts );
	memcpy( this->verts.Ptr(), verts, numVerts * sizeof( verts[0] ) );
	this->indexes.SetNum( numIndexes );
	memcpy( this->indexes.Ptr(), indexes, numIndexes * sizeof( indexes[0] ) );
	GenerateEdgeIndexes();
}

/*
====================
arcSurface::arcSurface
====================
*/
ARC_INLINE arcSurface::arcSurface( const arcSurface &surf ) {
	this->verts = surf.verts;
	this->indexes = surf.indexes;
	this->edges = surf.edges;
	this->edgeIndexes = surf.edgeIndexes;
}

/*
====================
arcSurface::~arcSurface
====================
*/
ARC_INLINE arcSurface::~arcSurface( void ) {
}

/*
=================
arcSurface::operator[]
=================
*/
ARC_INLINE const arcDrawVert &arcSurface::operator[]( const int index ) const {
	return verts[index];
};

/*
=================
arcSurface::operator[]
=================
*/
ARC_INLINE arcDrawVert &arcSurface::operator[]( const int index ) {
	return verts[index];
};

/*
=================
arcSurface::operator+=
=================
*/
ARC_INLINE arcSurface &arcSurface::operator+=( const arcSurface &surf ) {
	int i, m, n;
	n = verts.Num();
	m = indexes.Num();
	verts.Append( surf.verts );			// merge verts where possible ?
	indexes.Append( surf.indexes );
	for ( i = m; i < indexes.Num(); i++ ) {
		indexes[i] += n;
	}
	GenerateEdgeIndexes();
	return *this;
}

/*
=================
arcSurface::Clear
=================
*/
ARC_INLINE void arcSurface::Clear( void ) {
	verts.Clear();
	indexes.Clear();
	edges.Clear();
	edgeIndexes.Clear();
}

/*
=================
arcSurface::SwapTriangles
=================
*/
ARC_INLINE void arcSurface::SwapTriangles( arcSurface &surf ) {
	verts.Swap( surf.verts );
	indexes.Swap( surf.indexes );
	edges.Swap( surf.edges );
	edgeIndexes.Swap( surf.edgeIndexes );
}

/*
=================
arcSurface::TranslateSelf
=================
*/
ARC_INLINE void arcSurface::TranslateSelf( const arcVec3 &translation ) {
	for ( int i = 0; i < verts.Num(); i++ ) {
		verts[i].xyz += translation;
	}
}

/*
=================
arcSurface::RotateSelf
=================
*/
ARC_INLINE void arcSurface::RotateSelf( const arcMat3 &rotation ) {
	for ( int i = 0; i < verts.Num(); i++ ) {
		verts[i].xyz *= rotation;
		verts[i].normal *= rotation;
		verts[i].tangents[0] *= rotation;
		verts[i].tangents[1] *= rotation;
	}
}

#endif /* !__SURFACE_H__ */
