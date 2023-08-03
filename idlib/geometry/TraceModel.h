
#ifndef __TRACEMODEL_H__
#define __TRACEMODEL_H__

/*
===============================================================================

	Traceable surface.

===============================================================================
*/

class anSurface_Traceable : public anSurface {
public:
								anSurface_Traceable( void );
								anSurface_Traceable( const anSurface &surf ) :
									anSurface( surf ),
									hash( nullptr ),
									traceCount( 0 ),
									triPlanes( nullptr ),
									lastTriTrace( nullptr ) {
									bounds.Clear();
								}
								~anSurface_Traceable( void );

	const anBounds&				GetBounds( void );
	void						SetTraceFraction( const float traceFraction );

	void						OptimizeForTracing( const float traceFraction = 1.f );

								// intersection point is start + dir * scale
	bool						RayIntersection( /*arcList< int >& tracedTris,*/ const anVec3 &start, const anVec3 &dir, float &scale, anDrawVertex& dv, bool backFaceCull = false ) const;

protected:
	class TraceableTriHash {
	public:
		struct hashTriangle_t {
			int				triIndex;
			hashTriangle_t*	next;
		};

		struct hashBin_t {
			size_t			lastTrace;
			hashTriangle_t*	triangleList;
		};

							TraceableTriHash( anSurface_Traceable &surface, const int binsPerAxis = 50, const int snapFractions = 32 );
							~TraceableTriHash( void );

		const anBounds&		GetBounds( void ) const { return bounds; }
		hashBin_t*			GetHashBin( const anVec3& point );

	private:
		int					binsPerAxis;
		int					snapFractions;
		anBounds			bounds;

		hashBin_t *			bins;
		anVec3				scale;
		int					intMins[ 3 ];
		int					intScale[ 3 ];

		anBlockAlloc< hashTriangle_t, 128 >	triangleAllocator;

	private:
		int					BinIndex( const int x, const int y, const int z ) const { return ( ( binsPerAxis * binsPerAxis * z ) + ( binsPerAxis * y ) + x ); }
	};

	anBounds					bounds;
	arcTraceableTriHash*	hash;

	float						traceDist;

	mutable size_t				traceCount;

	anPlane*					triPlanes;
	size_t*						lastTriTrace;

protected:
	void						GenerateHash( void );
	void						GenerateIntersectionDrawVert( const anVec3& intersection, const int intersectedTriIndex, anDrawVertex& dv ) const;
};

/*
====================
anSurface_Traceable::TraceableTriHash::~TraceableTriHash
====================
*/
ARC_INLINE anSurface_Traceable::TraceableTriHash::~TraceableTriHash( void ) {
	delete [] bins;
	triangleAllocator.Shutdown();
}

/*
====================
anSurface_Traceable::TraceableTriHash::GetHashBin
====================
*/
ARC_INLINE anSurface_Traceable::TraceableTriHash::hashBin_t* anSurface_Traceable::TraceableTriHash::GetHashBin( const anVec3& point ) {
	int block[ 3 ];

	// snap the point to integral values
	for ( int i = 0; i < 3; i++ ) {
		block[i] = anMath::Ftoi( anMath::Floor( ( point[i] + .5f / snapFractions ) * snapFractions ) );
		block[i] = ( block[i] - intMins[i] ) / intScale[i];
		if ( block[i] < 0 ) {
			block[i] = 0;
		} else if ( block[i] >= binsPerAxis ) {
			block[i] = binsPerAxis - 1;
		}
	}

	return &bins[ BinIndex( block[ 0 ], block[ 1 ], block[ 2 ] ) ];
}

/*
====================
anSurface_Traceable::anSurface_Traceable
====================
*/
ARC_INLINE anSurface_Traceable::anSurface_Traceable( void ) :
	hash( nullptr ),
	traceCount( 0 ),
	triPlanes( nullptr ),
	lastTriTrace( nullptr ) {
	bounds.Clear();
}

/*
====================
anSurface_Traceable::~anSurface_Traceable
====================
*/
ARC_INLINE anSurface_Traceable::~anSurface_Traceable( void ) {
	delete hash;
	delete [] triPlanes;
	delete [] lastTriTrace;
}

/*
====================
anSurface_Traceable::GetBounds
====================
*/
ARC_INLINE const anBounds& anSurface_Traceable::GetBounds( void ) {
	if ( bounds.IsCleared() ) {
		//SIMDProcessor->MinMax( bounds[0], bounds[1], verts.Begin(), indexes.Begin(), indexes.Num() );
	}

	return bounds;
}

/*
====================
anSurface_Traceable::SetTraceFraction
====================
*/
ARC_INLINE void anSurface_Traceable::SetTraceFraction( const float traceFraction ) {
	traceDist = 0.f;

	// the traceDist will be the traceFrac times the largest bounds axis
	for ( int i = 0; i < 3; i++ ) {
		float d;
		d = traceFraction * ( GetBounds()[1][i] - GetBounds()[0][i] );
		if ( d > traceDist ) {
			traceDist = d;
		}
	}
}

/*
====================
anSurface_Traceable::OptimizeForTracing
====================
*/
ARC_INLINE void anSurface_Traceable::OptimizeForTracing( const float traceFraction ) {
	delete hash;
	delete [] triPlanes;
	delete [] lastTriTrace;

	hash = new TraceableTriHash( *this );

	triPlanes = new anPlane[ indexes.Num() / 3 ];
	SIMDProcessor->DeriveTriPlanes( triPlanes, verts.Begin(), verts.Num(), indexes.Begin(), indexes.Num() );

	lastTriTrace = new size_t[ indexes.Num() / 3 ];
	memset( lastTriTrace, 0, indexes.Num() / 3 * sizeof( size_t ) );

	SetTraceFraction( traceFraction );
}

/*
===============================================================================

	A trace model is an arbitrary polygonal model which is used by the
	collision detection system to find collisions, contacts or the contents
	of a volume. For collision detection speed reasons the number of vertices
	and edges are limited. The trace model can have any shape. However convex
	models are usually preferred.

===============================================================================
*/

class anVec3;
class anMat3;
class anBounds;

// trace model type
typedef enum {
	TRM_INVALID,		// invalid trm
	TRM_BOX,			// box
	TRM_OCTAHEDRON,		// octahedron
	TRM_DODECAHEDRON,	// dodecahedron
	TRM_CYLINDER,		// cylinder approximation
	TRM_CONE,			// cone approximation
	TRM_BONE,			// two tetrahedrons attached to each other
	TRM_POLYGON,		// arbitrary convex polygon
	TRM_POLYGONVOLUME,	// volume for arbitrary convex polygon
	TRM_CUSTOM			// loaded from map model or ASE/LWO
} traceModel_t;

// these are bit cache limits
#define MAX_TRACEMODEL_VERTS		32
#define MAX_TRACEMODEL_EDGES		32
#define MAX_TRACEMODEL_POLYS		16
#define MAX_TRACEMODEL_POLYEDGES	16

typedef anVec3 traceModelVert_t;

typedef struct {
	int					v[2];
	anVec3				normal;
} traceModelEdge_t;

typedef struct {
	anVec3				normal;
	float				dist;
	anBounds			bounds;
	int					numEdges;
	int					edges[MAX_TRACEMODEL_POLYEDGES];
} traceModelPoly_t;

class anTraceModel {

public:
	traceModel_t		type;
	int					numVerts;
	traceModelVert_t	verts[MAX_TRACEMODEL_VERTS];
	int					numEdges;
	traceModelEdge_t	edges[MAX_TRACEMODEL_EDGES+1];
	int					numPolys;
	traceModelPoly_t	polys[MAX_TRACEMODEL_POLYS];
	anVec3				offset;			// offset to center of model
	anBounds			bounds;			// bounds of model
	bool				isConvex;		// true when model is convex

public:
						anTraceModel( void );
						// axial bounding box
						anTraceModel( const anBounds &boxBounds );
						// cylinder approximation
						anTraceModel( const anBounds &cylBounds, const int numSides );
						// bone
						anTraceModel( const float length, const float width );

						// axial box
	void				SetupBox( const anBounds &boxBounds );
	void				SetupBox( const float size );
						// octahedron
	void				SetupOctahedron( const anBounds &octBounds );
	void				SetupOctahedron( const float size );
						// dodecahedron
	void				SetupDodecahedron( const anBounds &dodBounds );
	void				SetupDodecahedron( const float size );
						// cylinder approximation
	void				SetupCylinder( const anBounds &cylBounds, const int numSides );
	void				SetupCylinder( const float height, const float width, const int numSides );
						// cone approximation
	void				SetupCone( const anBounds &coneBounds, const int numSides );
	void				SetupCone( const float height, const float width, const int numSides );
						// two tetrahedrons attached to each other
	void				SetupBone( const float length, const float width );
						// arbitrary convex polygon
	void				SetupPolygon( const anVec3 *v, const int count );
	void				SetupPolygon( const anWinding &w );
						// generate edge normals
	int					GenerateEdgeNormals( void );
						// translate the trm
	void				Translate( const anVec3 &translation );
						// rotate the trm
	void				Rotate( const anMat3 &rotation );
						// shrink the model m units on all sides
	void				Shrink( const float m );
						// compare
	bool				Compare( const anTraceModel &trm ) const;
	bool				operator==(	const anTraceModel &trm ) const;
	bool				operator!=(	const anTraceModel &trm ) const;
						// get the area of one of the polygons
	float				GetPolygonArea( int polyNum ) const;
						// get the silhouette edges
	int					GetProjectionSilhouetteEdges( const anVec3 &projectionOrigin, int silEdges[MAX_TRACEMODEL_EDGES] ) const;
	int					GetParallelProjectionSilhouetteEdges( const anVec3 &projectionDir, int silEdges[MAX_TRACEMODEL_EDGES] ) const;
						// calculate mass properties assuming an uniform density
	void				GetMassProperties( const float density, float &mass, anVec3 &centerOfMass, anMat3 &inertiaTensor ) const;

private:
	void				InitBox( void );
	void				InitOctahedron( void );
	void				InitDodecahedron( void );
	void				InitBone( void );

	void				ProjectionIntegrals( int polyNum, int a, int b, struct projectionIntegrals_s &integrals ) const;
	void				PolygonIntegrals( int polyNum, int a, int b, int c, struct polygonIntegrals_s &integrals ) const;
	void				VolumeIntegrals( struct volumeIntegrals_s &integrals ) const;
	void				VolumeFromPolygon( anTraceModel &trm, float thickness ) const;
	int					GetOrderedSilhouetteEdges( const int edgeIsSilEdge[MAX_TRACEMODEL_EDGES+1], int silEdges[MAX_TRACEMODEL_EDGES] ) const;
};


ARC_INLINE anTraceModel::anTraceModel( void ) {
	type = TRM_INVALID;
	numVerts = numEdges = numPolys = 0;
	bounds.Zero();
}

ARC_INLINE anTraceModel::anTraceModel( const anBounds &boxBounds ) {
	InitBox();
	SetupBox( boxBounds );
}

ARC_INLINE anTraceModel::anTraceModel( const anBounds &cylBounds, const int numSides ) {
	SetupCylinder( cylBounds, numSides );
}

ARC_INLINE anTraceModel::anTraceModel( const float length, const float width ) {
	InitBone();
	SetupBone( length, width );
}

ARC_INLINE bool anTraceModel::operator==( const anTraceModel &trm ) const {
	return Compare( trm );
}

ARC_INLINE bool anTraceModel::operator!=( const anTraceModel &trm ) const {
	return !Compare( trm );
}

#endif // !__TRACEMODEL_H__