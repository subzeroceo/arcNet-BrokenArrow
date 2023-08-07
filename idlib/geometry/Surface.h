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


class anSurface {
public:
							anSurface( void );
							explicit anSurface( const anSurface &surf );
							explicit anSurface( const anDrawVertex *verts, const int numVerts, const int *indexes, const int numIndexes );
							~anSurface( void );

	const anDrawVertex &		operator[]( const int index ) const;
	anDrawVertex &			operator[]( const int index );
	anSurface &				operator+=( const anSurface &surf );

	int						GetNumIndexes( void ) const { return indexes.Num(); }
	const int *				GetIndexes( void ) const { return indexes.Ptr(); }
	int						GetNumVertices( void ) const { return verts.Num(); }
	const anDrawVertex *		GetVertices( void ) const { return verts.Ptr(); }
	const int *				GetEdgeIndexes( void ) const { return edgeIndexes.Ptr(); }
	const surfaceEdge_t *	GetEdges( void ) const { return edges.Ptr(); }

	void					Clear( void );
	void					SwapTriangles( anSurface &surf );
	void					TranslateSelf( const anVec3 &translation );
	void					RotateSelf( const anMat3 &rotation );

							// splits the surface into a front and back surface, the surface itself stays unchanged
							// frontOnPlaneEdges and backOnPlaneEdges optionally store the indexes to the edges that lay on the split plane
							// returns a SIDE_?
	int						Split( const anPlane &plane, const float epsilon, anSurface **front, anSurface **back, int *frontOnPlaneEdges = nullptr, int *backOnPlaneEdges = nullptr ) const;
							// cuts off the part at the back side of the plane, returns true if some part was at the front
							// if there is nothing at the front the number of points is set to zero
	bool					ClipInPlace( const anPlane &plane, const float epsilon = ON_EPSILON, const bool keepOn = false );

							// returns true if each triangle can be reached from any other triangle by a traversal
	bool					IsConnected( void ) const;
							// returns true if the surface is closed
	bool					IsClosed( void ) const;
							// returns true if the surface is a convex hull
	bool					IsPolytope( const float epsilon = 0.1f ) const;

	float					PlaneDistance( const anPlane &plane ) const;
	int						PlaneSide( const anPlane &plane, const float epsilon = ON_EPSILON ) const;

							// returns true if the line intersects one of the surface triangles
	bool					LineIntersection( const anVec3 &start, const anVec3 &end, bool backFaceCull = false ) const;
							// intersection point is start + dir * scale
	bool					RayIntersection( const anVec3 &start, const anVec3 &dir, float &scale, bool backFaceCull = false ) const;

protected:
	anList<anDrawVertex>		verts;			// vertices
	anList<int>				indexes;		// 3 references to vertices for each triangle
	anList<surfaceEdge_t>	edges;			// edges
	anList<int>				edgeIndexes;	// 3 references to edges for each triangle, may be negative for reversed edge

protected:
	void					GenerateEdgeIndexes( void );
	int						FindEdge( int v1, int v2 ) const;
};

/*
====================
anSurface::anSurface
====================
*/
inline anSurface::anSurface( void ) {
}

/*
=================
anSurface::anSurface
=================
*/
inline anSurface::anSurface( const anDrawVertex *verts, const int numVerts, const int *indexes, const int numIndexes ) {
	assert( verts != nullptr && indexes != nullptr && numVerts > 0 && numIndexes > 0 );
	this->verts.SetNum( numVerts );
	memcpy( this->verts.Ptr(), verts, numVerts * sizeof( verts[0] ) );
	this->indexes.SetNum( numIndexes );
	memcpy( this->indexes.Ptr(), indexes, numIndexes * sizeof( indexes[0] ) );
	GenerateEdgeIndexes();
}

/*
====================
anSurface::anSurface
====================
*/
inline anSurface::anSurface( const anSurface &surf ) {
	this->verts = surf.verts;
	this->indexes = surf.indexes;
	this->edges = surf.edges;
	this->edgeIndexes = surf.edgeIndexes;
}

/*
====================
anSurface::~anSurface
====================
*/
inline anSurface::~anSurface( void ) {
}

/*
=================
anSurface::operator[]
=================
*/
inline const anDrawVertex &anSurface::operator[]( const int index ) const {
	return verts[index];
};

/*
=================
anSurface::operator[]
=================
*/
inline anDrawVertex &anSurface::operator[]( const int index ) {
	return verts[index];
};

/*
=================
anSurface::operator+=
=================
*/
inline anSurface &anSurface::operator+=( const anSurface &surf ) {
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
anSurface::Clear
=================
*/
inline void anSurface::Clear( void ) {
	verts.Clear();
	indexes.Clear();
	edges.Clear();
	edgeIndexes.Clear();
}

/*
=================
anSurface::SwapTriangles
=================
*/
inline void anSurface::SwapTriangles( anSurface &surf ) {
	verts.Swap( surf.verts );
	indexes.Swap( surf.indexes );
	edges.Swap( surf.edges );
	edgeIndexes.Swap( surf.edgeIndexes );
}

/*
=================
anSurface::TranslateSelf
=================
*/
inline void anSurface::TranslateSelf( const anVec3 &translation ) {
	for ( int i = 0; i < verts.Num(); i++ ) {
		verts[i].xyz += translation;
	}
}

/*
=================
anSurface::RotateSelf
=================
*/
inline void anSurface::RotateSelf( const anMat3 &rotation ) {
	for ( int i = 0; i < verts.Num(); i++ ) {
		verts[i].xyz *= rotation;
		verts[i].normal *= rotation;
		verts[i].tangents[0] *= rotation;
		verts[i].tangents[1] *= rotation;
	}
}

#endif /* !__SURFACE_H__ */
