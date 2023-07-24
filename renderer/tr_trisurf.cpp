#include "/idlib/precompiled.h"
#pragma hdrstop

#include "tr_local.h"

/*
==============================================================================

TRIANGLE MESH PROCESSING

The functions in this file have no vertex / index count limits.

Truly identical vertexes that match in position, normal, and texcoord can
be merged away.

Vertexes that match in position and texcoord, but have distinct normals will
remain distinct for all purposes.  This is usually a poor choice for models,
as adding a bevel face will not add any more vertexes, and will tend to
look better.

Match in position and normal, but differ in texcoords are referenced together
for calculating tangent vectors for bump mapping.
Artists should take care to have identical texels in all maps (bump/diffuse/specular)
in this case

Vertexes that only match in position are merged for shadow edge finding.

Degenerate triangles.

Overlapped triangles, even if normals or texcoords differ, must be removed.
for the silhoette based stencil shadow algorithm to function properly.
Is this true???
Is the overlapped triangle problem just an example of the trippled edge problem?

Interpenetrating triangles are not currently clipped to surfaces.
Do they effect the shadows?

if vertexes are intended to deform apart, make sure that no vertexes
are on top of each other in the base frame, or the sil edges may be
calculated incorrectly.

We might be able to identify this from topology.

Dangling edges are acceptable, but three way edges are not.

Are any combinations of two way edges unacceptable, like one facing
the backside of the other?


Topology is determined by a collection of triangle indexes.

The edge list can be built up from this, and stays valid even under
deformations.

Somewhat non-intuitively, concave edges cannot be optimized away, or the
stencil shadow algorithm miscounts.

Face normals are needed for generating shadow volumes and for calculating
the silhouette, but they will change with any deformation.

Vertex normals and vertex tangents will change with each deformation,
but they may be able to be transformed instead of recalculated.

bounding volume, both box and sphere will change with deformation.

silhouette indexes
shade indexes
texture indexes

  shade indexes will only be > silhouette indexes if there is facet shading present

	lookups from texture to sil and texture to shade?

The normal and tangent vector smoothing is simple averaging, no attempt is
made to better handle the cases where the distribution around the shared vertex
is highly uneven.


  we may get degenerate triangles even with the uniquing and removal
  if the vertexes have different texcoords.

==============================================================================
*/

// this shouldn't change anything, but previously renderbumped models seem to need it
#define USE_INVA

// instead of using the texture T vector, cross the normal and S vector for an orthogonal axis
#define DERIVE_UNSMOOTHED_BITANGENT

const int MAX_SIL_EDGES			= 0x10000;
const int SILEDGE_HASH_SIZE		= 1024;

static int			numSilEdges;
static silEdge_t *	silEdges;
static ARCHashIndex	silEdgeHash( SILEDGE_HASH_SIZE, MAX_SIL_EDGES );
static int			numPlanes;

static arcBlockAlloc<surfTriangles_t, 1<<8>				srfTrianglesAllocator;

#ifdef USE_TRI_DATA_ALLOCATOR
static arcDynamicBlockAlloc<aRcnicalImageVertex, 1<<20, 1<<10>	triVertexAllocator;
static arcDynamicBlockAlloc<qglIndex_t, 1<<18, 1<<10>		triIndexAllocator;
static arcDynamicBlockAlloc<arcShadowCache, 1<<18, 1<<10>	triShadowVertexAllocator;
static arcDynamicBlockAlloc<arcPlane, 1<<17, 1<<10>		triPlaneAllocator;
static arcDynamicBlockAlloc<qglIndex_t, 1<<17, 1<<10>		triSilIndexAllocator;
static arcDynamicBlockAlloc<silEdge_t, 1<<17, 1<<10>		triSilEdgeAllocator;
static arcDynamicBlockAlloc<dominantTri_t, 1<<16, 1<<10>	triDominantTrisAllocator;
static arcDynamicBlockAlloc<int, 1<<16, 1<<10>			triMirroredVertAllocator;
static arcDynamicBlockAlloc<int, 1<<16, 1<<10>			triDupVertAllocator;
#else
static arcDynamicAlloc<aRcnicalImageVertex, 1<<20, 1<<10>			triVertexAllocator;
static arcDynamicAlloc<qglIndex_t, 1<<18, 1<<10>			triIndexAllocator;
static arcDynamicAlloc<arcShadowCache, 1<<18, 1<<10>		triShadowVertexAllocator;
static arcDynamicAlloc<arcPlane, 1<<17, 1<<10>			triPlaneAllocator;
static arcDynamicAlloc<qglIndex_t, 1<<17, 1<<10>			triSilIndexAllocator;
static arcDynamicAlloc<silEdge_t, 1<<17, 1<<10>			triSilEdgeAllocator;
static arcDynamicAlloc<dominantTri_t, 1<<16, 1<<10>		triDominantTrisAllocator;
static arcDynamicAlloc<int, 1<<16, 1<<10>				triMirroredVertAllocator;
static arcDynamicAlloc<int, 1<<16, 1<<10>				triDupVertAllocator;
#endif


/*
===============
R_InitTriSurfData
===============
*/
void R_InitTriSurfData( void ) {
	silEdges = (silEdge_t *)R_StaticAlloc( MAX_SIL_EDGES * sizeof( silEdges[0] ) );

	// initialize allocators for triangle surfaces
	triVertexAllocator.Init();
	triIndexAllocator.Init();
	triShadowVertexAllocator.Init();
	triPlaneAllocator.Init();
	triSilIndexAllocator.Init();
	triSilEdgeAllocator.Init();
	triDominantTrisAllocator.Init();
	triMirroredVertAllocator.Init();
	triDupVertAllocator.Init();

	// never swap out triangle surfaces
	triVertexAllocator.SetLockMemory( true );
	triIndexAllocator.SetLockMemory( true );
	triShadowVertexAllocator.SetLockMemory( true );
	triPlaneAllocator.SetLockMemory( true );
	triSilIndexAllocator.SetLockMemory( true );
	triSilEdgeAllocator.SetLockMemory( true );
	triDominantTrisAllocator.SetLockMemory( true );
	triMirroredVertAllocator.SetLockMemory( true );
	triDupVertAllocator.SetLockMemory( true );
}

/*
===============
R_ShutdownTriSurfData
===============
*/
void R_ShutdownTriSurfData( void ) {
	R_StaticFree( silEdges );
	silEdgeHash.Free();
	srfTrianglesAllocator.Shutdown();
	triVertexAllocator.Shutdown();
	triIndexAllocator.Shutdown();
	triShadowVertexAllocator.Shutdown();
	triPlaneAllocator.Shutdown();
	triSilIndexAllocator.Shutdown();
	triSilEdgeAllocator.Shutdown();
	triDominantTrisAllocator.Shutdown();
	triMirroredVertAllocator.Shutdown();
	triDupVertAllocator.Shutdown();
}

/*
===============
R_PurgeTriSurfData
===============
*/
void R_PurgeTriSurfData( frameData_t *frame ) {
	// free deferred triangle surfaces
	R_FreeDeferredTriSurfs( frame );

	// free empty base blocks
	triVertexAllocator.FreeEmptyBaseBlocks();
	triIndexAllocator.FreeEmptyBaseBlocks();
	triShadowVertexAllocator.FreeEmptyBaseBlocks();
	triPlaneAllocator.FreeEmptyBaseBlocks();
	triSilIndexAllocator.FreeEmptyBaseBlocks();
	triSilEdgeAllocator.FreeEmptyBaseBlocks();
	triDominantTrisAllocator.FreeEmptyBaseBlocks();
	triMirroredVertAllocator.FreeEmptyBaseBlocks();
	triDupVertAllocator.FreeEmptyBaseBlocks();
}

/*
===============
R_ShowTriMemory_f
===============
*/
void R_ShowTriSurfMemory_f( const arcCommandArgs &args ) {
	common->Printf( "%6d kB in %d triangle surfaces\n",
		( srfTrianglesAllocator.GetAllocCount() * sizeof( surfTriangles_t ) ) >> 10,
			srfTrianglesAllocator.GetAllocCount() );

	common->Printf( "%6d kB vertex memory (%d kB free in %d blocks, %d empty base blocks)\n",
		triVertexAllocator.GetBaseBlockMemory() >> 10, triVertexAllocator.GetFreeBlockMemory() >> 10,
			triVertexAllocator.GetNumFreeBlocks(), triVertexAllocator.GetNumEmptyBaseBlocks() );

	common->Printf( "%6d kB index memory (%d kB free in %d blocks, %d empty base blocks)\n",
		triIndexAllocator.GetBaseBlockMemory() >> 10, triIndexAllocator.GetFreeBlockMemory() >> 10,
			triIndexAllocator.GetNumFreeBlocks(), triIndexAllocator.GetNumEmptyBaseBlocks() );

	common->Printf( "%6d kB shadow vert memory (%d kB free in %d blocks, %d empty base blocks)\n",
		triShadowVertexAllocator.GetBaseBlockMemory() >> 10, triShadowVertexAllocator.GetFreeBlockMemory() >> 10,
			triShadowVertexAllocator.GetNumFreeBlocks(), triShadowVertexAllocator.GetNumEmptyBaseBlocks() );

	common->Printf( "%6d kB tri plane memory (%d kB free in %d blocks, %d empty base blocks)\n",
		triPlaneAllocator.GetBaseBlockMemory() >> 10, triPlaneAllocator.GetFreeBlockMemory() >> 10,
			triPlaneAllocator.GetNumFreeBlocks(), triPlaneAllocator.GetNumEmptyBaseBlocks() );

	common->Printf( "%6d kB sil index memory (%d kB free in %d blocks, %d empty base blocks)\n",
		triSilIndexAllocator.GetBaseBlockMemory() >> 10, triSilIndexAllocator.GetFreeBlockMemory() >> 10,
			triSilIndexAllocator.GetNumFreeBlocks(), triSilIndexAllocator.GetNumEmptyBaseBlocks() );

	common->Printf( "%6d kB sil edge memory (%d kB free in %d blocks, %d empty base blocks)\n",
		triSilEdgeAllocator.GetBaseBlockMemory() >> 10, triSilEdgeAllocator.GetFreeBlockMemory() >> 10,
			triSilEdgeAllocator.GetNumFreeBlocks(), triSilEdgeAllocator.GetNumEmptyBaseBlocks() );

	common->Printf( "%6d kB dominant tri memory (%d kB free in %d blocks, %d empty base blocks)\n",
		triDominantTrisAllocator.GetBaseBlockMemory() >> 10, triDominantTrisAllocator.GetFreeBlockMemory() >> 10,
			triDominantTrisAllocator.GetNumFreeBlocks(), triDominantTrisAllocator.GetNumEmptyBaseBlocks() );

	common->Printf( "%6d kB mirror vert memory (%d kB free in %d blocks, %d empty base blocks)\n",
		triMirroredVertAllocator.GetBaseBlockMemory() >> 10, triMirroredVertAllocator.GetFreeBlockMemory() >> 10,
			triMirroredVertAllocator.GetNumFreeBlocks(), triMirroredVertAllocator.GetNumEmptyBaseBlocks() );

	common->Printf( "%6d kB dup vert memory (%d kB free in %d blocks, %d empty base blocks)\n",
		triDupVertAllocator.GetBaseBlockMemory() >> 10, triDupVertAllocator.GetFreeBlockMemory() >> 10,
			triDupVertAllocator.GetNumFreeBlocks(), triDupVertAllocator.GetNumEmptyBaseBlocks() );

	common->Printf( "%6d kB total triangle memory\n",
		( srfTrianglesAllocator.GetAllocCount() * sizeof( surfTriangles_t ) +
			triVertexAllocator.GetBaseBlockMemory() +
			triIndexAllocator.GetBaseBlockMemory() +
			triShadowVertexAllocator.GetBaseBlockMemory() +
			triPlaneAllocator.GetBaseBlockMemory() +
			triSilIndexAllocator.GetBaseBlockMemory() +
			triSilEdgeAllocator.GetBaseBlockMemory() +
			triDominantTrisAllocator.GetBaseBlockMemory() +
			triMirroredVertAllocator.GetBaseBlockMemory() +
			triDupVertAllocator.GetBaseBlockMemory() ) >> 10 );
}

/*
=================
R_TriSurfMemory

For memory profiling
=================
*/
int R_TriSurfMemory( const surfTriangles_t *tri ) {
	int total = 0;

	if ( !tri ) {
		return total;
	}

	// used as a flag in interations
	if ( tri == LIGHT_TRIS_DEFERRED ) {
		return total;
	}

	if ( tri->shadowVertexes != NULL ) {
		total += tri->numVerts * sizeof( tri->shadowVertexes[0] );
	} else if ( tri->verts != NULL ) {
		if ( tri->ambientSurface == NULL || tri->verts != tri->ambientSurface->verts ) {
			total += tri->numVerts * sizeof( tri->verts[0] );
		}
	}
	if ( tri->facePlanes != NULL ) {
		total += tri->numIndexes / 3 * sizeof( tri->facePlanes[0] );
	}
	if ( tri->indexes != NULL ) {
		if ( tri->ambientSurface == NULL || tri->indexes != tri->ambientSurface->indexes ) {
			total += tri->numIndexes * sizeof( tri->indexes[0] );
		}
	}
	if ( tri->silIndexes != NULL ) {
		total += tri->numIndexes * sizeof( tri->silIndexes[0] );
	}
	if ( tri->silEdges != NULL ) {
		total += tri->numSilEdges * sizeof( tri->silEdges[0] );
	}
	if ( tri->dominantTris != NULL ) {
		total += tri->numVerts * sizeof( tri->dominantTris[0] );
	}
	if ( tri->mirroredVerts != NULL ) {
		total += tri->numMirroredVerts * sizeof( tri->mirroredVerts[0] );
	}
	if ( tri->dupVerts != NULL ) {
		total += tri->numDupVerts * sizeof( tri->dupVerts[0] );
	}

	total += sizeof( *tri );

	return total;
}

/*
==============
R_FreeStaticTriSurfVertexCaches
==============
*/
void R_FreeStaticTriSurfVertexCaches( surfTriangles_t *tri ) {
	if ( tri->ambientSurface == NULL ) {
		// this is a real model surface
		vertexCache.Free( tri->ambientCache );
		tri->ambientCache = NULL;
	} else {
		// this is a light interaction surface that references
		// a different ambient model surface
		vertexCache.Free( tri->lightingCache );
		tri->lightingCache = NULL;
	}
	if ( tri->indexCache ) {
		vertexCache.Free( tri->indexCache );
		tri->indexCache = NULL;
	}
	if ( tri->shadowCache && ( tri->shadowVertexes != NULL || tri->verts != NULL ) ) {
		// if we don't have tri->shadowVertexes, these are a reference to a
		// shadowCache on the original surface, which a vertex program
		// will take care of making unique for each light
		vertexCache.Free( tri->shadowCache );
		tri->shadowCache = NULL;
	}
}

/*
==============
R_ReallyFreeStaticTriSurf

This does the actual free
==============
*/
void R_ReallyFreeStaticTriSurf( surfTriangles_t *tri ) {
	if ( !tri ) {
		return;
	}

	R_FreeStaticTriSurfVertexCaches( tri );

	if ( tri->verts != NULL ) {
		// R_CreateLightTris points tri->verts at the verts of the ambient surface
		if ( tri->ambientSurface == NULL || tri->verts != tri->ambientSurface->verts ) {
			triVertexAllocator.Free( tri->verts );
		}
	}

	if ( !tri->deformedSurface ) {
		if ( tri->indexes != NULL ) {
			// if a surface is completely inside a light volume R_CreateLightTris points tri->indexes at the indexes of the ambient surface
			if ( tri->ambientSurface == NULL || tri->indexes != tri->ambientSurface->indexes ) {
				triIndexAllocator.Free( tri->indexes );
			}
		}
		if ( tri->silIndexes != NULL ) {
			triSilIndexAllocator.Free( tri->silIndexes );
		}
		if ( tri->silEdges != NULL ) {
			triSilEdgeAllocator.Free( tri->silEdges );
		}
		if ( tri->dominantTris != NULL ) {
			triDominantTrisAllocator.Free( tri->dominantTris );
		}
		if ( tri->mirroredVerts != NULL ) {
			triMirroredVertAllocator.Free( tri->mirroredVerts );
		}
		if ( tri->dupVerts != NULL ) {
			triDupVertAllocator.Free( tri->dupVerts );
		}
	}

	if ( tri->facePlanes != NULL ) {
		triPlaneAllocator.Free( tri->facePlanes );
	}

	if ( tri->shadowVertexes != NULL ) {
		triShadowVertexAllocator.Free( tri->shadowVertexes );
	}

#ifdef _DEBUG
	memset( tri, 0, sizeof( surfTriangles_t ) );
#endif

	srfTrianglesAllocator.Free( tri );
}

/*
==============
R_CheckStaticTriSurfMemory
==============
*/
void R_CheckStaticTriSurfMemory( const surfTriangles_t *tri ) {
	if ( !tri ) {
		return;
	}

	if ( tri->verts != NULL ) {
		// R_CreateLightTris points tri->verts at the verts of the ambient surface
		if ( tri->ambientSurface == NULL || tri->verts != tri->ambientSurface->verts ) {
			const char *error = triVertexAllocator.CheckMemory( tri->verts );
			assert( error == NULL );
		}
	}

	if ( !tri->deformedSurface ) {
		if ( tri->indexes != NULL ) {
			// if a surface is completely inside a light volume R_CreateLightTris points tri->indexes at the indexes of the ambient surface
			if ( tri->ambientSurface == NULL || tri->indexes != tri->ambientSurface->indexes ) {
				const char *error = triIndexAllocator.CheckMemory( tri->indexes );
				assert( error == NULL );
			}
		}
	}

	if ( tri->shadowVertexes != NULL ) {
		const char *error = triShadowVertexAllocator.CheckMemory( tri->shadowVertexes );
		assert( error == NULL );
	}
}

/*
==================
R_FreeDeferredTriSurfs
==================
*/
void R_FreeDeferredTriSurfs( frameData_t *frame ) {
	if ( !frame ) {
		return;
	}

	for ( surfTriangles_t **tri = frame->firstDeferredFreeTriSurf; tri; tri = next ) {
		surfTriangles_t **next = tri->nextDeferredFree;
		R_ReallyFreeStaticTriSurf( tri );
	}

	frame->firstDeferredFreeTriSurf = NULL;
	frame->lastDeferredFreeTriSurf = NULL;
}

/*
==============
R_FreeStaticTriSurf

This will defer the free until the current frame has run through the back end.
==============
*/
void R_FreeStaticTriSurf( surfTriangles_t *tri ) {
	if ( !tri ) {
		return;
	}

	if ( tri->nextDeferredFree ) {
		common->Error( "R_FreeStaticTriSurf: freed a freed triangle" );
	}
	frameData_t *frame = frameData;

	if ( !frame ) {
		// command line utility, or rendering in editor preview mode ( force )
		R_ReallyFreeStaticTriSurf( tri );
	} else {
#ifdef ID_DEBUG_MEMORY
		R_CheckStaticTriSurfMemory( tri );
#endif
		tri->nextDeferredFree = NULL;
		if ( frame->lastDeferredFreeTriSurf ) {
			frame->lastDeferredFreeTriSurf->nextDeferredFree = tri;
		} else {
			frame->firstDeferredFreeTriSurf = tri;
		}
		frame->lastDeferredFreeTriSurf = tri;
	}
}

/*
==============
R_AllocStaticTriSurf
==============
*/
surfTriangles_t *R_AllocStaticTriSurf( void ) {
	surfTriangles_t *tris = srfTrianglesAllocator.Alloc();
	memset( tris, 0, sizeof( surfTriangles_t ) );
	return tris;
}

/*
=================
R_CopyStaticTriSurf

This only duplicates the indexes and verts, not any of the derived data.
=================
*/
surfTriangles_t *R_CopyStaticTriSurf( const surfTriangles_t *tri ) {
	surfTriangles_t	*newTri = R_AllocStaticTriSurf();
	R_AllocStaticTriSurfVerts( newTri, tri->numVerts );
	R_AllocStaticTriSurfIndexes( newTri, tri->numIndexes );
	newTri->numVerts = tri->numVerts;
	newTri->numIndexes = tri->numIndexes;
	memcpy( newTri->verts, tri->verts, tri->numVerts * sizeof( newTri->verts[0] ) );
	memcpy( newTri->indexes, tri->indexes, tri->numIndexes * sizeof( newTri->indexes[0] ) );

	return newTri;
}

/*
=================
R_AllocStaticTriSurfVerts
=================
*/
void R_AllocStaticTriSurfVerts( surfTriangles_t *tri, int numVerts ) {
	assert( tri->verts == NULL );
	tri->verts = triVertexAllocator.Alloc( numVerts );
}

/*
=================
R_AllocStaticTriSurfIndexes
=================
*/
void R_AllocStaticTriSurfIndexes( surfTriangles_t *tri, int numIndexes ) {
	assert( tri->indexes == NULL );
	tri->indexes = triIndexAllocator.Alloc( numIndexes );
}

/*
=================
R_AllocStaticTriSurfShadowVerts
=================
*/
void R_AllocStaticTriSurfShadowVerts( surfTriangles_t *tri, int numVerts ) {
	assert( tri->shadowVertexes == NULL );
	tri->shadowVertexes = triShadowVertexAllocator.Alloc( numVerts );
}

/*
=================
R_AllocStaticTriSurfPlanes
=================
*/
void R_AllocStaticTriSurfPlanes( surfTriangles_t *tri, int numIndexes ) {
	if ( tri->facePlanes ) {
		triPlaneAllocator.Free( tri->facePlanes );
	}
	tri->facePlanes = triPlaneAllocator.Alloc( numIndexes / 3 );
}

static void R_ByteSwapTri( surfTriangles_t *tri ) {
	for ( int i = 0; i < sizeof( surfTriangles_t )/4 ; i++) {
		( (int *)tri )[i] = BigLong( ( (int *)tri )[i] );
	}
}

/*
=================
R_ResizeStaticTriSurfVerts
=================
*/
void R_ResizeStaticTriSurfVerts( surfTriangles_t *tri, int numVerts ) {
#ifdef USE_TRI_DATA_ALLOCATOR
	tri->verts = triVertexAllocator.Resize( tri->verts, numVerts );
#else
	assert( false );
#endif
}

/*
=================
R_ResizeStaticTriSurfIndexes
=================
*/
void R_ResizeStaticTriSurfIndexes( surfTriangles_t *tri, int numIndexes ) {
#ifdef USE_TRI_DATA_ALLOCATOR
	tri->indexes = triIndexAllocator.Resize( tri->indexes, numIndexes );
#else
	assert( false );
#endif
}

/*
=================
R_ResizeStaticTriSurfShadowVerts
=================
*/
void R_ResizeStaticTriSurfShadowVerts( surfTriangles_t *tri, int numVerts ) {
#ifdef USE_TRI_DATA_ALLOCATOR
	tri->shadowVertexes = triShadowVertexAllocator.Resize( tri->shadowVertexes, numVerts );
#else
	assert( false );
#endif
}

/*
=================
R_ReferenceStaticTriSurfVerts
=================
*/
void R_ReferenceStaticTriSurfVerts( surfTriangles_t *tri, const surfTriangles_t *reference ) {
	tri->verts = reference->verts;
}

/*
=================
R_ReferenceStaticTriSurfIndexes
=================
*/
void R_ReferenceStaticTriSurfIndexes( surfTriangles_t *tri, const surfTriangles_t *reference ) {
	tri->indexes = reference->indexes;
}

/*
=================
R_FreeStaticTriSurfSilIndexes
=================
*/
void R_FreeStaticTriSurfSilIndexes( surfTriangles_t *tri ) {
	triSilIndexAllocator.Free( tri->silIndexes );
	tri->silIndexes = NULL;
}

/*
===============
R_RangeCheckIndexes

Check for syntactically incorrect indexes, like out of range values.
Does not check for semantics, like degenerate triangles.

No vertexes is acceptable if no indexes.
No indexes is acceptable.
More vertexes than are referenced by indexes are acceptable.
===============
*/
void R_RangeCheckIndexes( const surfTriangles_t *tri ) {
	if ( tri->numIndexes < 0 ) {
		common->Error( "R_RangeCheckIndexes: numIndexes < 0" );
	}
	if ( tri->numVerts < 0 ) {
		common->Error( "R_RangeCheckIndexes: numVerts < 0" );
	}

	// must specify an integral number of triangles
	if ( tri->numIndexes % 3 != 0 ) {
		common->Error( "R_RangeCheckIndexes: numIndexes %% 3" );
	}

	for ( int i = 0; i < tri->numIndexes; i++ ) {
		if ( tri->indexes[i] < 0 || tri->indexes[i] >= tri->numVerts ) {
			common->Error( "R_RangeCheckIndexes: index out of range" );
		}
	}

	// this should not be possible unless there are unused verts
	if ( tri->numVerts > tri->numIndexes ) {
		// FIXME: find the causes of these
		// common->Printf( "R_RangeCheckIndexes: tri->numVerts > tri->numIndexes\n" );
	}
}

/*
=================
R_BoundTriSurf
=================
*/
void R_BoundTriSurf( surfTriangles_t *tri ) {
	SIMDProcessor->MinMax( tri->bounds[0], tri->bounds[1], tri->verts, tri->numVerts );
}

/*
=================
R_CreateSilRemap
=================
*/
static int *R_CreateSilRemap( const surfTriangles_t *tri ) {
	int		c_removed, c_unique;
	int		*remap;
	int		i, j, hashKey;
	const aRcnicalImageVertex *v1, *v2;

	remap = ( int * )R_ClearedStaticAlloc( tri->numVerts * sizeof( remap[0] ) );

	if ( !r_useSilRemap.GetBool() ) {
		for ( i = 0; i < tri->numVerts; i++ ) {
			remap[i] = i;
		}
		return remap;
	}

	ARCHashIndex		hash( 1024, tri->numVerts );

	c_removed = 0;
	c_unique = 0;
	for ( i = 0; i < tri->numVerts; i++ ) {
		v1 = &tri->verts[i];

		// see if there is an earlier vert that it can map to
		hashKey = hash.GenerateKey( v1->xyz );
		for ( j = hash.First( hashKey ); j >= 0; j = hash.Next( j ) ) {
			v2 = &tri->verts[j];
			if ( v2->xyz[0] == v1->xyz[0]
				&& v2->xyz[1] == v1->xyz[1]
				&& v2->xyz[2] == v1->xyz[2] ) {
				c_removed++;
				remap[i] = j;
				break;
			}
		}
		if ( j < 0 ) {
			c_unique++;
			remap[i] = i;
			hash.Add( hashKey, i );
		}
	}

	return remap;
}

/*
=================
R_CreateSilIndexes

Uniquing vertexes only on xyz before creating sil edges reduces
the edge count by about 20% on Q3 models
=================
*/
void R_CreateSilIndexes( surfTriangles_t *tri ) {
	if ( tri->silIndexes ) {
		triSilIndexAllocator.Free( tri->silIndexes );
		tri->silIndexes = NULL;
	}

	int remap = R_CreateSilRemap( tri );

	// remap indexes to the first one
	tri->silIndexes = triSilIndexAllocator.Alloc( tri->numIndexes );
	for ( int i = 0; i < tri->numIndexes; i++ ) {
		tri->silIndexes[i] = remap[tri->indexes[i]];
	}

	R_StaticFree( remap );
}

/*
=====================
R_CreateDupVerts
=====================
*/
void R_CreateDupVerts( surfTriangles_t *tri ) {
	int *remap = ( int * ) _alloca16( tri->numVerts * sizeof( remap[0] ) );

	// initialize vertex remap in case there are unused verts
	for ( int i = 0; i < tri->numVerts; i++ ) {
		remap[i] = i;
	}

	// set the remap based on how the silhouette indexes are remapped
	for ( int i = 0; i < tri->numIndexes; i++ ) {
		remap[tri->indexes[i]] = tri->silIndexes[i];
	}

	// create duplicate vertex index based on the vertex remap
	int * tempDupVerts = ( int * ) _alloca16( tri->numVerts * 2 * sizeof( tempDupVerts[0] ) );
	tri->numDupVerts = 0;
	for ( int i = 0; i < tri->numVerts; i++ ) {
		if ( remap[i] != i ) {
			tempDupVerts[tri->numDupVerts*2+0] = i;
			tempDupVerts[tri->numDupVerts*2+1] = remap[i];
			tri->numDupVerts++;
		}
	}

	tri->dupVerts = triDupVertAllocator.Alloc( tri->numDupVerts * 2 );
	memcpy( tri->dupVerts, tempDupVerts, tri->numDupVerts * 2 * sizeof( tri->dupVerts[0] ) );
}

/*
=============
R_CreateFullScreenTris
=============
*/
static surfTriangles_t * R_CreateFullScreenTris() {
	// copy verts and indexes
	surfTriangles_t * tri = (surfTriangles_t *)Mem_ClearedAlloc( sizeof( *tri ), TAG_RENDER_TOOLS );

	tri->numIndexes = 6;
	tri->numVerts = 4;

	int indexSize = tri->numIndexes * sizeof( tri->indexes[0] );
	int allocatedIndexBytes = ALIGN( indexSize, 16 );
	tri->indexes = (triIndex_t *)Mem_Alloc( allocatedIndexBytes, TAG_RENDER_TOOLS );

	int vertexSize = tri->numVerts * sizeof( tri->verts[0] );
	int allocatedVertexBytes =  ALIGN( vertexSize, 16 );
	tri->verts = (aRcnicalImageVertex *)Mem_ClearedAlloc( allocatedVertexBytes, TAG_RENDER_TOOLS );

	aRcnicalImageVertex * verts = tri->verts;

	triIndex_t tempIndexes[6] = { 3, 0, 2, 2, 0, 1 };
	memcpy( tri->indexes, tempIndexes, indexSize );

	verts[0].xyz[0] = -1.0f;
	verts[0].xyz[1] = 1.0f;
	verts[0].SetTexCoord( 0.0f, 1.0f );

	verts[1].xyz[0] = 1.0f;
	verts[1].xyz[1] = 1.0f;
	verts[1].SetTexCoord( 1.0f, 1.0f );

	verts[2].xyz[0] = 1.0f;
	verts[2].xyz[1] = -1.0f;
	verts[2].SetTexCoord( 1.0f, 0.0f );

	verts[3].xyz[0] = -1.0f;
	verts[3].xyz[1] = -1.0f;
	verts[3].SetTexCoord( 0.0f, 0.0f );

	for ( int i = 0; i < 4; i++ ) {
		verts[i].SetColor( 0xffffffff );
	}

	return tri;
}

/*
=============
R_CreateZeroOneCubeTris
=============
*/
static surfTriangles_t * R_CreateZeroOneCubeTris() {
	surfTriangles_t * tri = (surfTriangles_t *)Mem_ClearedAlloc( sizeof( *tri ), TAG_RENDER_TOOLS );

	tri->numVerts = 8;
	tri->numIndexes = 36;

	const int indexSize = tri->numIndexes * sizeof( tri->indexes[0] );
	const int allocatedIndexBytes = ALIGN( indexSize, 16 );
	tri->indexes = (triIndex_t *)Mem_Alloc( allocatedIndexBytes, TAG_RENDER_TOOLS );

	const int vertexSize = tri->numVerts * sizeof( tri->verts[0] );
	const int allocatedVertexBytes =  ALIGN( vertexSize, 16 );
	tri->verts = (aRcnicalImageVertex *)Mem_ClearedAlloc( allocatedVertexBytes, TAG_RENDER_TOOLS );

	aRcnicalImageVertex * verts = tri->verts;

	const float low = 0.0f;
	const float high = 1.0f;

	arcVec3 center( 0.0f );
	arcVec3 mx(  low, 0.0f, 0.0f );
	arcVec3 px( high, 0.0f, 0.0f );
	arcVec3 my( 0.0f,  low, 0.0f );
	arcVec3 py( 0.0f, high, 0.0f );
	arcVec3 mz( 0.0f, 0.0f,  low );
	arcVec3 pz( 0.0f, 0.0f, high );

	verts[0].xyz = center + mx + my + mz;
	verts[1].xyz = center + px + my + mz;
	verts[2].xyz = center + px + py + mz;
	verts[3].xyz = center + mx + py + mz;
	verts[4].xyz = center + mx + my + pz;
	verts[5].xyz = center + px + my + pz;
	verts[6].xyz = center + px + py + pz;
	verts[7].xyz = center + mx + py + pz;

	// bottom
	tri->indexes[ 0*3+0] = 2;
	tri->indexes[ 0*3+1] = 3;
	tri->indexes[ 0*3+2] = 0;
	tri->indexes[ 1*3+0] = 1;
	tri->indexes[ 1*3+1] = 2;
	tri->indexes[ 1*3+2] = 0;
	// back
	tri->indexes[ 2*3+0] = 5;
	tri->indexes[ 2*3+1] = 1;
	tri->indexes[ 2*3+2] = 0;
	tri->indexes[ 3*3+0] = 4;
	tri->indexes[ 3*3+1] = 5;
	tri->indexes[ 3*3+2] = 0;
	// left
	tri->indexes[ 4*3+0] = 7;
	tri->indexes[ 4*3+1] = 4;
	tri->indexes[ 4*3+2] = 0;
	tri->indexes[ 5*3+0] = 3;
	tri->indexes[ 5*3+1] = 7;
	tri->indexes[ 5*3+2] = 0;
	// right
	tri->indexes[ 6*3+0] = 1;
	tri->indexes[ 6*3+1] = 5;
	tri->indexes[ 6*3+2] = 6;
	tri->indexes[ 7*3+0] = 2;
	tri->indexes[ 7*3+1] = 1;
	tri->indexes[ 7*3+2] = 6;
	// front
	tri->indexes[ 8*3+0] = 3;
	tri->indexes[ 8*3+1] = 2;
	tri->indexes[ 8*3+2] = 6;
	tri->indexes[ 9*3+0] = 7;
	tri->indexes[ 9*3+1] = 3;
	tri->indexes[ 9*3+2] = 6;
	// top
	tri->indexes[10*3+0] = 4;
	tri->indexes[10*3+1] = 7;
	tri->indexes[10*3+2] = 6;
	tri->indexes[11*3+0] = 5;
	tri->indexes[11*3+1] = 4;
	tri->indexes[11*3+2] = 6;

	for ( int i = 0; i < 4; i++ ) {
		verts[i].SetColor( 0xffffffff );
	}

	return tri;
}

/*
================
R_CreateTestImageTris

Initializes the Test Image Triangles
================
*/
surfTriangles_t* R_CreateTestImageTris() {
	surfTriangles_t * tri = (surfTriangles_t *)Mem_ClearedAlloc( sizeof( *tri ), TAG_RENDER_TOOLS );

	tri->numIndexes = 6;
	tri->numVerts = 4;

	int indexSize = tri->numIndexes * sizeof( tri->indexes[0] );
	int allocatedIndexBytes = ALIGN( indexSize, 16 );
	tri->indexes = (triIndex_t *)Mem_Alloc( allocatedIndexBytes, TAG_RENDER_TOOLS );

	int vertexSize = tri->numVerts * sizeof( tri->verts[0] );
	int allocatedVertexBytes =  ALIGN( vertexSize, 16 );
	tri->verts = (aRcnicalImageVertex *)Mem_ClearedAlloc( allocatedVertexBytes, TAG_RENDER_TOOLS );

	ALIGNTYPE16 triIndex_t tempIndexes[6] = { 3, 0, 2, 2, 0, 1 };
	memcpy( tri->indexes, tempIndexes, indexSize );

	aRcnicalImageVertex* tempVerts = tri->verts;
	tempVerts[0].xyz[0] = 0.0f;
	tempVerts[0].xyz[1] = 0.0f;
	tempVerts[0].xyz[2] = 0;
	tempVerts[0].SetTexCoord( 0.0, 0.0f );

	tempVerts[1].xyz[0] = 1.0f;
	tempVerts[1].xyz[1] = 0.0f;
	tempVerts[1].xyz[2] = 0;
	tempVerts[1].SetTexCoord( 1.0f, 0.0f );

	tempVerts[2].xyz[0] = 1.0f;
	tempVerts[2].xyz[1] = 1.0f;
	tempVerts[2].xyz[2] = 0;
	tempVerts[2].SetTexCoord( 1.0f, 1.0f );

	tempVerts[3].xyz[0] = 0.0f;
	tempVerts[3].xyz[1] = 1.0f;
	tempVerts[3].xyz[2] = 0;
	tempVerts[3].SetTexCoord( 0.0f, 1.0f );

	for ( int i = 0; i < 4; i++ ) {
		tempVerts[i].SetColor( 0xFFFFFFFF );
	}
	return tri;
}

/*
=====================
R_DeriveFacePlanes

Writes the facePlanes values, overwriting existing ones if present
=====================
*/
void R_DeriveFacePlanes( surfTriangles_t *tri ) {
	arcPlane *	planes;

	if ( !tri->facePlanes ) {
		R_AllocStaticTriSurfPlanes( tri, tri->numIndexes );
	}
	planes = tri->facePlanes;

#if 1
	SIMDProcessor->DeriveTriPlanes( planes, tri->verts, tri->numVerts, tri->indexes, tri->numIndexes );
#else
	for ( int i = 0; i < tri->numIndexes; i+= 3, planes++ ) {
		int		i1, i2, i3;
		arcVec3	d1, d2, normal;
		arcVec3	*v1, *v2, *v3;

		i1 = tri->indexes[i + 0];
		i2 = tri->indexes[i + 1];
		i3 = tri->indexes[i + 2];

		v1 = &tri->verts[i1].xyz;
		v2 = &tri->verts[i2].xyz;
		v3 = &tri->verts[i3].xyz;

		d1[0] = v2->x - v1->x;
		d1[1] = v2->y - v1->y;
		d1[2] = v2->z - v1->z;

		d2[0] = v3->x - v1->x;
		d2[1] = v3->y - v1->y;
		d2[2] = v3->z - v1->z;

		normal[0] = d2.y * d1.z - d2.z * d1.y;
		normal[1] = d2.z * d1.x - d2.x * d1.z;
		normal[2] = d2.x * d1.y - d2.y * d1.x;

		float sqrLength, invLength;

		sqrLength = normal.x * normal.x + normal.y * normal.y + normal.z * normal.z;
		invLength = arcMath::RSqrt( sqrLength );

		(*planes)[0] = normal[0] * invLength;
		(*planes)[1] = normal[1] * invLength;
		(*planes)[2] = normal[2] * invLength;

		planes->FitThroughPoint( *v1 );
	}

#endif

	tri->facePlanesCalculated = true;
}

/*
=====================
R_CreateVertexNormals

Averages together the contributions of all faces that are
used by a vertex, creating drawVert->normal
=====================
*/
void R_CreateVertexNormals( surfTriangles_t *tri ) {
	const srfTriangles_t	*tri2;

	if ( !tri->facePlanes || !tri->facePlanesCalculated ) {
		R_DeriveFacePlanes( tri );
	}

	if ( !tri->silIndexes ) {
		R_CreateSilIndexes( tri );
	}

	for ( int i = 0; i < tri->numVerts; i++ ) {
		tri->verts[i].normal.Zero();
	}

	const arcPlane *planes = tri->facePlanes;

	for ( int i = 0; i < tri->numIndexes; i += 3, planes++ ) {
		for ( int j = 0; j < 3; j++ ) {
			int index = tri->silIndexes[i+j];
			tri->verts[index].normal += planes->Normal();
		}
	}

	// normalize and replicate from silIndexes to all indexes
	for ( int i = 0; i < tri->numIndexes; i++ ) {
		tri->verts[tri->indexes[i]].normal = tri->verts[tri->silIndexes[i]].normal;
		tri->verts[tri->indexes[i]].normal.Normalize();
	}

/*	tri->facePlanesCalculated = true;
	tri->numVerts = tri->numIndexes = tri->numSilIndexes = tri->sortedIndexes = tri->suppressInRender = 0;
	R_DeriveFacePlanes( tri );
	tri2 = tri->sortedEdges.Ptr();

	// Precompute values outside the loop
	for ( int i = 0; i < tri->numSilEdges; i++ ) {
		auto& silIndex = tri2->silIndexes[i];
		auto& vert = tri->verts[silIndex];

		silIndex.normal = vert.normal + vert.normal;
		tri2->indexes[i] = tri->indexes[i];
	}

	// Normalize the vectors once outside the loop if they don't change
	for ( int i = 0; i < tri->numSilEdges; i++ ) {
		auto& silIndex = tri2->silIndexes[i];
		silIndex.normal.Normalize();
	}*/
}

/*
===============
R_DefineEdge
===============
*/
static int c_duplicatedEdges, c_tripledEdges;
static void R_DefineEdge( int v1, int v2, int planeNum ) {
	// check for degenerate edge
	if ( v1 == v2 ) {
		return;
	}
	int hashKey = silEdgeHash.GenerateKey( v1, v2 );
	// search for a matching other side
	for ( int i = silEdgeHash.First( hashKey ); i >= 0 && i < MAX_SIL_EDGES; i = silEdgeHash.Next( i ) ) {
		if ( silEdges[i].v1 == v1 && silEdges[i].v2 == v2 ) {
			c_duplicatedEdges++;
			// allow it to still create a new edge
			continue;
		}
		if ( silEdges[i].v2 == v1 && silEdges[i].v1 == v2 ) {
			if ( silEdges[i].p2 != numPlanes )  {
				c_tripledEdges++;
				// allow it to still create a new edge
				continue;
			}
			// this is a matching back side
			silEdges[i].p2 = planeNum;
			return;
		}

	}

	// define the new edge
	if ( numSilEdges == MAX_SIL_EDGES ) {
		common->DWarning( "MAX_SIL_EDGES" );
		return;
	}

	silEdgeHash.Add( hashKey, numSilEdges );

	silEdges[numSilEdges].p1 = planeNum;
	silEdges[numSilEdges].p2 = numPlanes;
	silEdges[numSilEdges].v1 = v1;
	silEdges[numSilEdges].v2 = v2;

	numSilEdges++;
}

/*
=================
SilEdgeSort
=================
*/
static int SilEdgeSort( const void *a, const void *b ) {
	if ( ( ( silEdge_t* )a )->p1 < ( ( silEdge_t* )b )->p1 ) {
		return -1;
	}
	if ( ( ( silEdge_t* )a )->p1 > ( ( silEdge_t* )b )->p1 ) {
		return 1;
	}
	if ( ( ( silEdge_t* )a )->p2 < ( ( silEdge_t* )b )->p2 ) {
		return -1;
	}
	if ( ( ( silEdge_t* )a )->p2 > ( ( silEdge_t* )b )->p2 ) {
		return 1;
	}
	return 0;
}

/*
=================
R_IdentifySilEdges

If the surface will not deform, coplanar edges (polygon interiors)
can never create silhouette plains, and can be omited
=================
*/
int	c_coplanarSilEdges;
int	c_totalSilEdges;
void R_IdentifySilEdges( surfTriangles_t *tri, bool omitCoplanarEdges ) {
	omitCoplanarEdges = false;	// optimization doesn't work for some reason

	int numTris = tri->numIndexes / 3;

	numSilEdges = 0;
	silEdgeHash.Clear();
	numPlanes = numTris;

	c_duplicatedEdges = 0;
	c_tripledEdges = 0;

	for ( int i = 0; i < numTris; i++ ) {
		int i1 = tri->silIndexes[ i*3 + 0 ];
		int i2 = tri->silIndexes[ i*3 + 1 ];
		int i3 = tri->silIndexes[ i*3 + 2 ];

		// create the edges
		R_DefineEdge( i1, i2, i );
		R_DefineEdge( i2, i3, i );
		R_DefineEdge( i3, i1, i );
	}

	if ( c_duplicatedEdges || c_tripledEdges ) {
		common->DWarning( "%i duplicated edge directions, %i tripled edges", c_duplicatedEdges, c_tripledEdges );
	}

	// if we know that the vertexes aren't going
	// to deform, we can remove interior triangulation edges
	// on otherwise planar polygons.
	// I earlier believed that I could also remove concave
	// edges, because they are never silhouettes in the conventional sense,
	// but they are still needed to balance out all the true sil edges
	// for the shadow algorithm to function
	int c_coplanarCulled = 0;
	if ( omitCoplanarEdges ) {
		for ( i = 0; i < numSilEdges; i++ ) {
			if ( silEdges[i].p2 == numPlanes ) {	// the fake dangling edge
				continue;
			}

			int base = silEdges[i].p1 * 3;
			int i1 = tri->silIndexes[ base + 0 ];
			int i2 = tri->silIndexes[ base + 1 ];
			int i3 = tri->silIndexes[ base + 2 ];

			arcPlane plane.FromPoints( tri->verts[i1].xyz, tri->verts[i2].xyz, tri->verts[i3].xyz );

			// check to see if points of second triangle are not coplanar
			base = silEdges[i].p2 * 3;
			for ( int j = 0; j < 3; j++ ) {
				i1 = tri->silIndexes[ base + j ];
				float d = plane.Distance( tri->verts[i1].xyz );
				if ( d != 0 ) {		// even a small epsilon causes problems
					break;
				}
			}

			if ( j == 3 ) {
				// we can cull this sil edge
				memmove( &silEdges[i], &silEdges[i+1], ( numSilEdges-i-1 ) * sizeof( silEdges[i] ) );
				c_coplanarCulled++;
				numSilEdges--;
				i--;
			}
		}
		if ( c_coplanarCulled ) {
			c_coplanarSilEdges += c_coplanarCulled;
//			common->Printf( "%i of %i sil edges coplanar culled\n", c_coplanarCulled,
//				c_coplanarCulled + numSilEdges );
		}
	}
	c_totalSilEdges += numSilEdges;

	// sort the sil edges based on plane number
	qsort( silEdges, numSilEdges, sizeof( silEdges[0] ), SilEdgeSort );

	// count up the distribution.
	// a perfectly built model should only have shared
	// edges, but most models will have some interpenetration
	// and dangling edges
	int shared = 0;
	int single = 0;
	for ( int i = 0; i < numSilEdges; i++ ) {
		if ( silEdges[i].p2 == numPlanes ) {
			single++;
		} else {
			shared++;
		}
	}

	if ( !single ) {
		tri->perfectHull = true;
	} else {
		tri->perfectHull = false;
	}

	tri->numSilEdges = numSilEdges;
	tri->silEdges = triSilEdgeAllocator.Alloc( numSilEdges );
	memcpy( tri->silEdges, silEdges, numSilEdges * sizeof( tri->silEdges[0] ) );
}

/*
===============
R_FaceNegativePolarity

Returns true if the texture polarity of the face is negative, false if it is positive or zero
===============
*/
static bool R_FaceNegativePolarity( const surfTriangles_t *tri, int firstIndex ) {
	float d0[5], d1[5];

	aRcnicalImageVertex *a = tri->verts + tri->indexes[firstIndex + 0];
	aRcnicalImageVertex *b = tri->verts + tri->indexes[firstIndex + 1];
	aRcnicalImageVertex *c = tri->verts + tri->indexes[firstIndex + 2];

	d0[3] = b->st[0] - a->st[0];
	d0[4] = b->st[1] - a->st[1];

	d1[3] = c->st[0] - a->st[0];
	d1[4] = c->st[1] - a->st[1];

	float area = d0[3] * d1[4] - d0[4] * d1[3];
	if ( area >= 0 ) {
		return false;
	}
	return true;
}

/*
==================
R_DeriveFaceTangents
==================
*/
typedef struct {
	arcVec3		tangents[2];
	bool	negativePolarity;
	bool	degenerate;
} faceTangents_t;

static void	R_DeriveFaceTangents( const surfTriangles_t *tri, faceTangents_t *faceTangents ) {
	//
	// calculate tangent vectors for each face in isolation
	//
	int c_positive = 0;
	int c_negative = 0;
	int c_textureDegenerateFaces = 0;

	for ( int i = 0; i < tri->numIndexes; i += 3 ) {
		/*aRcnicalImageVertex *a = tri->verts + tri->indexes[i];
		aRcnicalImageVertex *b = tri->verts + tri->indexes[i+1];
		aRcnicalImageVertex *c = tri->verts + tri->indexes[i+2];

		faceTangents_t *ft = faceTangents + i / 3;

		// tangent vectors for the vertice tangents
		a->tangents[0] += b->tangents[0];
		a->tangents[1] += b->tangents[1];

		b->tangents[0] += c->tangents[0];
		b->tangents[1] += c->tangents[1];

		c->tangents[0] = a->tangents[0] - b->tangents[0];
		c->tangents[1] = a->tangents[1] - b->tangents[1];
		c->degenerate = false;
		if ( c->tangents[0].AlmostEqualThan( a->tangents[0] ) ) {
			c_positive++;
		}
		if ( c->tangents[1].AlmostEqualThan( a->tangents[1] ) ) {
			c_negative++;
		}
		if ( c_positive + c_negative == 0 ) {
			c_negative = 0;
			c_positive = 0;
			break;
		}
		c->needsNormalization = true;
		c_negative = 0;*/

		float		d0[5], d1[5];

		faceTangents_t *ft = &faceTangents[i/3];

		aRcnicalImageVertex *a = tri->verts + tri->indexes[i + 0];
		aRcnicalImageVertex *b = tri->verts + tri->indexes[i + 1];
		aRcnicalImageVertex *c = tri->verts + tri->indexes[i + 2];

		d0[0] = b->xyz[0] - a->xyz[0];
		d0[1] = b->xyz[1] - a->xyz[1];
		d0[2] = b->xyz[2] - a->xyz[2];
		d0[3] = b->st[0] - a->st[0];
		d0[4] = b->st[1] - a->st[1];

		d1[0] = c->xyz[0] - a->xyz[0];
		d1[1] = c->xyz[1] - a->xyz[1];
		d1[2] = c->xyz[2] - a->xyz[2];
		d1[3] = c->st[0] - a->st[0];
		d1[4] = c->st[1] - a->st[1];

		float area = d0[3] * d1[4] - d0[4] * d1[3];
		if ( fabs( area ) < 1e-20f ) {
			ft->negativePolarity = false;
			ft->degenerate = true;
			ft->tangents[0].Zero();
			ft->tangents[1].Zero();
			c_textureDegenerateFaces++;
			continue;
		}
		if ( area > 0.0f ) {
			ft->negativePolarity = false;
			c_positive++;
		} else {
			ft->negativePolarity = true;
			c_negative++;
		}
		ft->degenerate = false;

#ifdef USE_INVA
		float inva = area < 0.0f ? -1 : 1;		// was = 1.0f / area;

        arcVec3 temp[0] = (d0[0] * d1[4] - d0[4] * d1[0] ) * inva;
        arcVec3 temp[1] = (d0[1] * d1[4] - d0[4] * d1[1] ) * inva;
        arcVec3 temp[2] = (d0[2] * d1[4] - d0[4] * d1[2] ) * inva;
		temp.Normalize();
		ft->tangents[0] = temp;

        temp[0] = (d0[3] * d1[0] - d0[0] * d1[3] ) * inva;
        temp[1] = (d0[3] * d1[1] - d0[1] * d1[3] ) * inva;
        temp[2] = (d0[3] * d1[2] - d0[2] * d1[3] ) * inva;
		temp.Normalize();
		ft->tangents[1] = temp;
#else
        arcVec3 temp[0] = (d0[0] * d1[4] - d0[4] * d1[0] );
        arcVec3 temp[1] = (d0[1] * d1[4] - d0[4] * d1[1] );
        arcVec3 temp[2] = (d0[2] * d1[4] - d0[4] * d1[2] );
		temp.Normalize();
		ft->tangents[0] = temp;

        temp[0] = (d0[3] * d1[0] - d0[0] * d1[3] );
        temp[1] = (d0[3] * d1[1] - d0[1] * d1[3] );
        temp[2] = (d0[3] * d1[2] - d0[2] * d1[3] );
		temp.Normalize();
		ft->tangents[1] = temp;
#endif
	}
}



/*
===================
R_DuplicateMirroredVertexes

Modifies the surface to bust apart any verts that are shared by both positive and
negative texture polarities, so tangent space smoothing at the vertex doesn't
degenerate.

This will create some identical vertexes (which will eventually get different tangent
vectors), so never optimize the resulting mesh, or it will get the mirrored edges back.

Reallocates tri->verts and changes tri->indexes in place
Silindexes are unchanged by this.

sets mirroredVerts and mirroredVerts[]

===================
*/
typedef struct {
	bool	polarityUsed[2];
	int			negativeRemap;
} tangentVert_t;

static void	R_DuplicateMirroredVertexes( surfTriangles_t *tri ) {
	tangentVert_t	*tverts, *vert;
	int				i, j;
	int				totalVerts;
	int				numMirror;

	tverts = (tangentVert_t *)_alloca16( tri->numVerts * sizeof( *tverts ) );
	memset( tverts, 0, tri->numVerts * sizeof( *tverts ) );

	// determine texture polarity of each surface

	// mark each vert with the polarities it uses
	for ( i = 0; i < tri->numIndexes; i+=3 ) {
		int	polarity;

		polarity = R_FaceNegativePolarity( tri, i );
		for ( j = 0; j < 3; j++ ) {
			tverts[tri->indexes[i+j]].polarityUsed[ polarity ] = true;
		}
	}

	// now create new verts as needed
	totalVerts = tri->numVerts;
	for ( i = 0; i < tri->numVerts; i++ ) {
		vert = &tverts[i];
		if ( vert->polarityUsed[0] && vert->polarityUsed[1] ) {
			vert->negativeRemap = totalVerts;
			totalVerts++;
		}
	}

	tri->numMirroredVerts = totalVerts - tri->numVerts;

	// now create the new list
	if ( totalVerts == tri->numVerts ) {
		tri->mirroredVerts = NULL;
		return;
	}

	tri->mirroredVerts = triMirroredVertAllocator.Alloc( tri->numMirroredVerts );

#ifdef USE_TRI_DATA_ALLOCATOR
	tri->verts = triVertexAllocator.Resize( tri->verts, totalVerts );
#else
	aRcnicalImageVertex *oldVerts = tri->verts;
	R_AllocStaticTriSurfVerts( tri, totalVerts );
	memcpy( tri->verts, oldVerts, tri->numVerts * sizeof( tri->verts[0] ) );
	triVertexAllocator.Free( oldVerts );
#endif

	// create the duplicates
	numMirror = 0;
	for ( i = 0; i < tri->numVerts; i++ ) {
		j = tverts[i].negativeRemap;
		if ( j ) {
			tri->verts[j] = tri->verts[i];
			tri->mirroredVerts[numMirror] = i;
			numMirror++;
		}
	}

	tri->numVerts = totalVerts;
	// change the indexes
	for ( i = 0; i < tri->numIndexes; i++ ) {
		if ( tverts[tri->indexes[i]].negativeRemap &&
			R_FaceNegativePolarity( tri, 3*( i/3) ) ) {
			tri->indexes[i] = tverts[tri->indexes[i]].negativeRemap;
		}
	}

	tri->numVerts = totalVerts;
}

/*
=================
R_DeriveTangentsWithoutNormals

Build texture space tangents for bump mapping
If a surface is deformed, this must be recalculated

This assumes that any mirrored vertexes have already been duplicated, so
any shared vertexes will have the tangent spaces smoothed across.

Texture wrapping slightly complicates this, but as long as the normals
are shared, and the tangent vectors are projected onto the normals, the
separate vertexes should wind up with identical tangent spaces.

mirroring a normalmap WILL cause a slightly visible seam unless the normals
are completely flat around the edge's full bilerp support.

Vertexes which are smooth shaded must have their tangent vectors
in the same plane, which will allow a seamless
rendering as long as the normal map is even on both sides of the
seam.

A smooth shaded surface may have multiple tangent vectors at a vertex
due to texture seams or mirroring, but it should only have a single
normal vector.

Each triangle has a pair of tangent vectors in it's plane

Should we consider having vertexes point at shared tangent spaces
to save space or speed transforms?

this version only handles bilateral symetry
=================
*/
void R_DeriveTangentsWithoutNormals( surfTriangles_t *tri ) {
	int			i, j;
	faceTangents_t	*faceTangents;
	faceTangents_t	*ft;
	aRcnicalImageVertex		*vert;

	faceTangents = (faceTangents_t *)_alloca16( sizeof(faceTangents[0] ) * tri->numIndexes/3 );
	R_DeriveFaceTangents( tri, faceTangents );

	// clear the tangents
	for ( i = 0; i < tri->numVerts; i++ ) {
		tri->verts[i].tangents[0].Zero();
		tri->verts[i].tangents[1].Zero();
	}

	// sum up the neighbors
	for ( i = 0; i < tri->numIndexes; i+=3 ) {
		ft = &faceTangents[i/3];

		// for each vertex on this face
		for ( j = 0; j < 3; j++ ) {
			vert = &tri->verts[tri->indexes[i+j]];

			vert->tangents[0] += ft->tangents[0];
			vert->tangents[1] += ft->tangents[1];
		}
	}

#if 0
	// sum up both sides of the mirrored verts
	// so the S vectors exactly mirror, and the T vectors are equal
	for ( i = 0; i < tri->numMirroredVerts; i++ ) {
		aRcnicalImageVertex	*v1, *v2;

		v1 = &tri->verts[ tri->numVerts - tri->numMirroredVerts + i ];
		v2 = &tri->verts[ tri->mirroredVerts[i] ];

		v1->tangents[0] -= v2->tangents[0];
		v1->tangents[1] += v2->tangents[1];

		v2->tangents[0] = vec3_origin - v1->tangents[0];
		v2->tangents[1] = v1->tangents[1];
	}
#endif


	// project the summed vectors onto the normal plane
	// and normalize.  The tangent vectors will not necessarily
	// be orthogonal to each other, but they will be orthogonal
	// to the surface normal.
	for ( i = 0; i < tri->numVerts; i++ ) {
		vert = &tri->verts[i];
		for ( j = 0; j < 2; j++ ) {
			float	d;

			d = vert->tangents[j] * vert->normal;
			vert->tangents[j] = vert->tangents[j] - d * vert->normal;
			vert->tangents[j].Normalize();
		}
	}

	tri->tangentsCalculated = true;
}

static ARC_INLINE void VectorNormalizeFast2( const arcVec3 &v, arcVec3 &out) {
	float	ilength;

	ilength = arcMath::RSqrt( v[0]*v[0] + v[1]*v[1] + v[2]*v[2] );
	out[0] = v[0] * ilength;
	out[1] = v[1] * ilength;
	out[2] = v[2] * ilength;
}

/*
===================
R_BuildDominantTris

Find the largest triangle that uses each vertex
===================
*/
typedef struct {
	int		vertexNum;
	int		faceNum;
} indexSort_t;

static int IndexSort( const void *a, const void *b ) {
	if ( ((indexSort_t *)a)->vertexNum < ((indexSort_t *)b)->vertexNum ) {
		return -1;
	}
	if ( ((indexSort_t *)a)->vertexNum > ((indexSort_t *)b)->vertexNum ) {
		return 1;
	}
	return 0;
}

void R_BuildDominantTris( surfTriangles_t *tri ) {
	int i, j;
	dominantTri_t *dt;
	indexSort_t *ind = (indexSort_t *)R_StaticAlloc( tri->numIndexes * sizeof( *ind ) );

	for ( i = 0; i < tri->numIndexes; i++ ) {
		ind[i].vertexNum = tri->indexes[i];
		ind[i].faceNum = i / 3;
	}
	qsort( ind, tri->numIndexes, sizeof( *ind ), IndexSort );

	tri->dominantTris = dt = triDominantTrisAllocator.Alloc( tri->numVerts );
	memset( dt, 0, tri->numVerts * sizeof( dt[0] ) );

	for ( i = 0; i < tri->numIndexes; i += j ) {
		float	maxArea = 0;
		int		vertNum = ind[i].vertexNum;
		for ( j = 0; i + j < tri->numIndexes && ind[i+j].vertexNum == vertNum; j++ ) {
			float		d0[5], d1[5];
			aRcnicalImageVertex	*a, *b, *c;
			arcVec3		normal, tangent, bitangent;

			int	i1 = tri->indexes[ind[i+j].faceNum * 3 + 0];
			int	i2 = tri->indexes[ind[i+j].faceNum * 3 + 1];
			int	i3 = tri->indexes[ind[i+j].faceNum * 3 + 2];

			a = tri->verts + i1;
			b = tri->verts + i2;
			c = tri->verts + i3;

			d0[0] = b->xyz[0] - a->xyz[0];
			d0[1] = b->xyz[1] - a->xyz[1];
			d0[2] = b->xyz[2] - a->xyz[2];
			d0[3] = b->st[0] - a->st[0];
			d0[4] = b->st[1] - a->st[1];

			d1[0] = c->xyz[0] - a->xyz[0];
			d1[1] = c->xyz[1] - a->xyz[1];
			d1[2] = c->xyz[2] - a->xyz[2];
			d1[3] = c->st[0] - a->st[0];
			d1[4] = c->st[1] - a->st[1];

			normal[0] = ( d1[1] * d0[2] - d1[2] * d0[1] );
			normal[1] = ( d1[2] * d0[0] - d1[0] * d0[2] );
			normal[2] = ( d1[0] * d0[1] - d1[1] * d0[0] );

			float area = normal.Length();

			// if this is smaller than what we already have, skip it
			if ( area < maxArea ) {
				continue;
			}
			maxArea = area;

			if ( i1 == vertNum ) {
				dt[vertNum].v2 = i2;
				dt[vertNum].v3 = i3;
			} else if ( i2 == vertNum ) {
				dt[vertNum].v2 = i3;
				dt[vertNum].v3 = i1;
			} else {
				dt[vertNum].v2 = i1;
				dt[vertNum].v3 = i2;
			}

			float	len = area;
			if ( len < 0.001f ) {
				len = 0.001f;
			}
			dt[vertNum].normalizationScale[2] = 1.0f / len;		// normal

			// texture area
			area = d0[3] * d1[4] - d0[4] * d1[3];

			tangent[0] = ( d0[0] * d1[4] - d0[4] * d1[0] );
			tangent[1] = ( d0[1] * d1[4] - d0[4] * d1[1] );
			tangent[2] = ( d0[2] * d1[4] - d0[4] * d1[2] );
			len = tangent.Length();
			if ( len < 0.001f ) {
				len = 0.001f;
			}
			dt[vertNum].normalizationScale[0] = ( area > 0 ? 1 : -1 ) / len;	// tangents[0]

			bitangent[0] = ( d0[3] * d1[0] - d0[0] * d1[3] );
			bitangent[1] = ( d0[3] * d1[1] - d0[1] * d1[3] );
			bitangent[2] = ( d0[3] * d1[2] - d0[2] * d1[3] );
			len = bitangent.Length();
			if ( len < 0.001f ) {
				len = 0.001f;
			}
#ifdef DERIVE_UNSMOOTHED_BITANGENT
			dt[vertNum].normalizationScale[1] = ( area > 0 ? 1 : -1 );
#else
			dt[vertNum].normalizationScale[1] = ( area > 0 ? 1 : -1 ) / len;	// tangents[1]
#endif
		}
	}

	R_StaticFree( ind );
}

/*
====================
R_DeriveUnsmoothedTangents

Uses the single largest area triangle for each vertex, instead of smoothing over all
====================
*/
void R_DeriveUnsmoothedTangents( surfTriangles_t *tri ) {
	if ( tri->tangentsCalculated ) {
		return;
	}

#if 1

	SIMDProcessor->DeriveUnsmoothedTangents( tri->verts, tri->dominantTris, tri->numVerts );

#else

	for ( int i = 0; i < tri->numVerts; i++ ) {
		arcVec3		temp;
		float		d0[5], d1[5];
		aRcnicalImageVertex	*a, *b, *c;
		dominantTri_t	*dt = &tri->dominantTris[i];

		a = tri->verts + i;
		b = tri->verts + dt->v2;
		c = tri->verts + dt->v3;

		d0[0] = b->xyz[0] - a->xyz[0];
		d0[1] = b->xyz[1] - a->xyz[1];
		d0[2] = b->xyz[2] - a->xyz[2];
		d0[3] = b->st[0] - a->st[0];
		d0[4] = b->st[1] - a->st[1];

		d1[0] = c->xyz[0] - a->xyz[0];
		d1[1] = c->xyz[1] - a->xyz[1];
		d1[2] = c->xyz[2] - a->xyz[2];
		d1[3] = c->st[0] - a->st[0];
		d1[4] = c->st[1] - a->st[1];

		a->normal[0] = dt->normalizationScale[2] * ( d1[1] * d0[2] - d1[2] * d0[1] );
		a->normal[1] = dt->normalizationScale[2] * ( d1[2] * d0[0] - d1[0] * d0[2] );
		a->normal[2] = dt->normalizationScale[2] * ( d1[0] * d0[1] - d1[1] * d0[0] );

		a->tangents[0][0] = dt->normalizationScale[0] * ( d0[0] * d1[4] - d0[4] * d1[0] );
		a->tangents[0][1] = dt->normalizationScale[0] * ( d0[1] * d1[4] - d0[4] * d1[1] );
		a->tangents[0][2] = dt->normalizationScale[0] * ( d0[2] * d1[4] - d0[4] * d1[2] );

#ifdef DERIVE_UNSMOOTHED_BITANGENT
		// derive the bitangent for a completely orthogonal axis,
		// instead of using the texture T vector
		a->tangents[1][0] = dt->normalizationScale[1] * ( a->normal[2] * a->tangents[0][1] - a->normal[1] * a->tangents[0][2] );
		a->tangents[1][1] = dt->normalizationScale[1] * ( a->normal[0] * a->tangents[0][2] - a->normal[2] * a->tangents[0][0] );
		a->tangents[1][2] = dt->normalizationScale[1] * ( a->normal[1] * a->tangents[0][0] - a->normal[0] * a->tangents[0][1] );
#else
		// calculate the bitangent from the texture T vector
		a->tangents[1][0] = dt->normalizationScale[1] * ( d0[3] * d1[0] - d0[0] * d1[3] );
		a->tangents[1][1] = dt->normalizationScale[1] * ( d0[3] * d1[1] - d0[1] * d1[3] );
		a->tangents[1][2] = dt->normalizationScale[1] * ( d0[3] * d1[2] - d0[2] * d1[3] );
#endif
	}

#endif

	tri->tangentsCalculated = true;
}

/*
==================
R_DeriveTangents

This is called once for static surfaces, and every frame for deforming surfaces

Builds tangents, normals, and face planes
==================
*/
void R_DeriveTangents( surfTriangles_t *tri, bool allocFacePlanes ) {
	int				i;
	arcPlane			*planes;

	if ( tri->dominantTris != NULL ) {
		R_DeriveUnsmoothedTangents( tri );
		return;
	}

	if ( tri->tangentsCalculated ) {
		return;
	}

	tr.pc.c_tangentIndexes += tri->numIndexes;

	if ( !tri->facePlanes && allocFacePlanes ) {
		R_AllocStaticTriSurfPlanes( tri, tri->numIndexes );
	}
	planes = tri->facePlanes;

#if 1

	if ( !planes ) {
		planes = (arcPlane *)_alloca16( ( tri->numIndexes / 3 ) * sizeof( planes[0] ) );
	}

	SIMDProcessor->DeriveTangents( planes, tri->verts, tri->numVerts, tri->indexes, tri->numIndexes );

#else

	for ( i = 0; i < tri->numVerts; i++ ) {
		tri->verts[i].normal.Zero();
		tri->verts[i].tangents[0].Zero();
		tri->verts[i].tangents[1].Zero();
	}

	for ( i = 0; i < tri->numIndexes; i += 3 ) {
		// make face tangents
		float		d0[5], d1[5];
		aRcnicalImageVertex	*a, *b, *c;
		arcVec3		temp, normal, tangents[2];

		a = tri->verts + tri->indexes[i + 0];
		b = tri->verts + tri->indexes[i + 1];
		c = tri->verts + tri->indexes[i + 2];

		d0[0] = b->xyz[0] - a->xyz[0];
		d0[1] = b->xyz[1] - a->xyz[1];
		d0[2] = b->xyz[2] - a->xyz[2];
		d0[3] = b->st[0] - a->st[0];
		d0[4] = b->st[1] - a->st[1];

		d1[0] = c->xyz[0] - a->xyz[0];
		d1[1] = c->xyz[1] - a->xyz[1];
		d1[2] = c->xyz[2] - a->xyz[2];
		d1[3] = c->st[0] - a->st[0];
		d1[4] = c->st[1] - a->st[1];

		// normal
		temp[0] = d1[1] * d0[2] - d1[2] * d0[1];
		temp[1] = d1[2] * d0[0] - d1[0] * d0[2];
		temp[2] = d1[0] * d0[1] - d1[1] * d0[0];
		VectorNormalizeFast2( temp, normal );

#ifdef USE_INVA
		float area = d0[3] * d1[4] - d0[4] * d1[3];
		float inva = area < 0.0f ? -1 : 1;		// was = 1.0f / area;

        temp[0] = (d0[0] * d1[4] - d0[4] * d1[0] ) * inva;
        temp[1] = (d0[1] * d1[4] - d0[4] * d1[1] ) * inva;
        temp[2] = (d0[2] * d1[4] - d0[4] * d1[2] ) * inva;
		VectorNormalizeFast2( temp, tangents[0] );

        temp[0] = (d0[3] * d1[0] - d0[0] * d1[3] ) * inva;
        temp[1] = (d0[3] * d1[1] - d0[1] * d1[3] ) * inva;
        temp[2] = (d0[3] * d1[2] - d0[2] * d1[3] ) * inva;
		VectorNormalizeFast2( temp, tangents[1] );
#else
        temp[0] = (d0[0] * d1[4] - d0[4] * d1[0] );
        temp[1] = (d0[1] * d1[4] - d0[4] * d1[1] );
        temp[2] = (d0[2] * d1[4] - d0[4] * d1[2] );
		VectorNormalizeFast2( temp, tangents[0] );

        temp[0] = (d0[3] * d1[0] - d0[0] * d1[3] );
        temp[1] = (d0[3] * d1[1] - d0[1] * d1[3] );
        temp[2] = (d0[3] * d1[2] - d0[2] * d1[3] );
		VectorNormalizeFast2( temp, tangents[1] );
#endif

		// sum up the tangents and normals for each vertex on this face
		for ( int j = 0; j < 3; j++ ) {
			vert = &tri->verts[tri->indexes[i+j]];
			vert->normal += normal;
			vert->tangents[0] += tangents[0];
			vert->tangents[1] += tangents[1];
		}

		if ( planes ) {
			planes->Normal() = normal;
			planes->FitThroughPoint( a->xyz );
			planes++;
		}
	}

#endif

#if 0

	if ( tri->silIndexes != NULL ) {
		for ( i = 0; i < tri->numVerts; i++ ) {
			tri->verts[i].normal.Zero();
		}
		for ( i = 0; i < tri->numIndexes; i++ ) {
			tri->verts[tri->silIndexes[i]].normal += planes[i/3].Normal();
		}
		for ( i = 0; i < tri->numIndexes; i++ ) {
			tri->verts[tri->indexes[i]].normal = tri->verts[tri->silIndexes[i]].normal;
		}
	}

#else

	int *dupVerts = tri->dupVerts;
	aRcnicalImageVertex *verts = tri->verts;

	// add the normal of a duplicated vertex to the normal of the first vertex with the same XYZ
	for ( i = 0; i < tri->numDupVerts; i++ ) {
		verts[dupVerts[i*2+0]].normal += verts[dupVerts[i*2+1]].normal;
	}

	// copy vertex normals to duplicated vertices
	for ( i = 0; i < tri->numDupVerts; i++ ) {
		verts[dupVerts[i*2+1]].normal = verts[dupVerts[i*2+0]].normal;
	}

#endif

#if 0
	// sum up both sides of the mirrored verts
	// so the S vectors exactly mirror, and the T vectors are equal
	for ( i = 0; i < tri->numMirroredVerts; i++ ) {
		aRcnicalImageVertex	*v1, *v2;

		v1 = &tri->verts[ tri->numVerts - tri->numMirroredVerts + i ];
		v2 = &tri->verts[ tri->mirroredVerts[i] ];

		v1->tangents[0] -= v2->tangents[0];
		v1->tangents[1] += v2->tangents[1];

		v2->tangents[0] = vec3_origin - v1->tangents[0];
		v2->tangents[1] = v1->tangents[1];
	}
#endif

	// project the summed vectors onto the normal plane
	// and normalize.  The tangent vectors will not necessarily
	// be orthogonal to each other, but they will be orthogonal
	// to the surface normal.
#if 1

	SIMDProcessor->NormalizeTangents( tri->verts, tri->numVerts );

#else

	for ( i = 0; i < tri->numVerts; i++ ) {
		aRcnicalImageVertex *vert = &tri->verts[i];

		VectorNormalizeFast2( vert->normal, vert->normal );

		// project the tangent vectors
		for ( int j = 0; j < 2; j++ ) {
			float d;

			d = vert->tangents[j] * vert->normal;
			vert->tangents[j] = vert->tangents[j] - d * vert->normal;
			VectorNormalizeFast2( vert->tangents[j], vert->tangents[j] );
		}
	}

#endif

	tri->tangentsCalculated = true;
	tri->facePlanesCalculated = true;
}

/*
=================
R_RemoveDuplicatedTriangles

silIndexes must have already been calculated

silIndexes are used instead of indexes, because duplicated
triangles could have different texture coordinates.
=================
*/
void R_RemoveDuplicatedTriangles( surfTriangles_t *tri ) {
	int		c_removed;
	int		i, j, r;
	int		a, b, c;

	c_removed = 0;

	// check for completely duplicated triangles
	// any rotation of the triangle is still the same, but a mirroring
	// is considered different
	for ( i = 0; i < tri->numIndexes; i+=3 ) {
		for ( r = 0; r < 3; r++ ) {
			a = tri->silIndexes[i+r];
			b = tri->silIndexes[i+(r+1 )%3];
			c = tri->silIndexes[i+(r+2)%3];
			for ( j = i + 3; j < tri->numIndexes; j+=3 ) {
				if ( tri->silIndexes[j] == a && tri->silIndexes[j+1] == b && tri->silIndexes[j+2] == c ) {
					c_removed++;
					memmove( tri->indexes + j, tri->indexes + j + 3, ( tri->numIndexes - j - 3 ) * sizeof( tri->indexes[0] ) );
					memmove( tri->silIndexes + j, tri->silIndexes + j + 3, ( tri->numIndexes - j - 3 ) * sizeof( tri->silIndexes[0] ) );
					tri->numIndexes -= 3;
					j -= 3;
				}
			}
		}
	}

	if ( c_removed ) {
		common->Printf( "removed %i duplicated triangles\n", c_removed );
	}
}

/*
=================
R_RemoveDegenerateTriangles

silIndexes must have already been calculated
=================
*/
void R_RemoveDegenerateTriangles( surfTriangles_t *tri ) {
	int		c_removed;
	int		i;
	int		a, b, c;

	// check for completely degenerate triangles
	c_removed = 0;
	for ( i = 0; i < tri->numIndexes; i += 3 ) {
		a = tri->silIndexes[i];
		b = tri->silIndexes[i+1];
		c = tri->silIndexes[i+2];
		if ( a == b || a == c || b == c ) {
			c_removed++;
			memmove( tri->indexes + i, tri->indexes + i + 3, ( tri->numIndexes - i - 3 ) * sizeof( tri->indexes[0] ) );
			if ( tri->silIndexes ) {
				memmove( tri->silIndexes + i, tri->silIndexes + i + 3, ( tri->numIndexes - i - 3 ) * sizeof( tri->silIndexes[0] ) );
			}
			tri->numIndexes -= 3;
			i -= 3;
		}
	}

	// this doesn't free the memory used by the unused verts

	if ( c_removed ) {
		common->Printf( "removed %i degenerate triangles\n", c_removed );
	}
}

/*
=================
R_TestDegenerateTextureSpace
=================
*/
void R_TestDegenerateTextureSpace( surfTriangles_t *tri ) {
	int		c_degenerate;
	int		i;

	// check for triangles with a degenerate texture space
	c_degenerate = 0;
	for ( i = 0; i < tri->numIndexes; i += 3 ) {
		const aRcnicalImageVertex &a = tri->verts[tri->indexes[i+0]];
		const aRcnicalImageVertex &b = tri->verts[tri->indexes[i+1]];
		const aRcnicalImageVertex &c = tri->verts[tri->indexes[i+2]];

		if ( a.st == b.st || b.st == c.st || c.st == a.st ) {
			c_degenerate++;
		}
	}

	if ( c_degenerate ) {
//		common->Printf( "%d triangles with a degenerate texture space\n", c_degenerate );
	}
}

/*
=================
R_RemoveUnusedVerts
=================
*/
void R_RemoveUnusedVerts( surfTriangles_t *tri ) {
	int		i;
	int		*mark;
	int		index;
	int		used;

	mark = ( int * )R_ClearedStaticAlloc( tri->numVerts * sizeof( *mark ) );

	for ( i = 0; i < tri->numIndexes; i++ ) {
		index = tri->indexes[i];
		if ( index < 0 || index >= tri->numVerts ) {
			common->Error( "R_RemoveUnusedVerts: bad index" );
		}
		mark[index] = 1;

		if ( tri->silIndexes ) {
			index = tri->silIndexes[i];
			if ( index < 0 || index >= tri->numVerts ) {
				common->Error( "R_RemoveUnusedVerts: bad index" );
			}
			mark[index] = 1;
		}
	}

	used = 0;
	for ( i = 0; i < tri->numVerts; i++ ) {
		if ( !mark[i] ) {
			continue;
		}
		mark[i] = used + 1;
		used++;
	}

	if ( used != tri->numVerts ) {
		for ( i = 0; i < tri->numIndexes; i++ ) {
			tri->indexes[i] = mark[ tri->indexes[i] ] - 1;
			if ( tri->silIndexes ) {
				tri->silIndexes[i] = mark[ tri->silIndexes[i] ] - 1;
			}
		}
		tri->numVerts = used;

		for ( i = 0; i < tri->numVerts; i++ ) {
			index = mark[ i ];
			if ( !index ) {
				continue;
			}
			tri->verts[ index - 1 ] = tri->verts[i];
		}

		// this doesn't realloc the arrays to save the memory used by the unused verts
	}

	R_StaticFree( mark );
}

/*
=================
R_MergeSurfaceList

Only deals with vertexes and indexes, not silhouettes, planes, etc.
Does NOT perform a cleanup triangles, so there may be duplicated verts in the result.
=================
*/
surfTriangles_t	*R_MergeSurfaceList( const surfTriangles_t **surfaces, int numSurfaces ) {
	surfTriangles_t	*newTri;
	const surfTriangles_t	*tri;
	int				i, j;
	int				totalVerts;
	int				totalIndexes;

	totalVerts = 0;
	totalIndexes = 0;
	for ( i = 0; i < numSurfaces; i++ ) {
		totalVerts += surfaces[i]->numVerts;
		totalIndexes += surfaces[i]->numIndexes;
	}

	newTri = R_AllocStaticTriSurf();
	newTri->numVerts = totalVerts;
	newTri->numIndexes = totalIndexes;
	R_AllocStaticTriSurfVerts( newTri, newTri->numVerts );
	R_AllocStaticTriSurfIndexes( newTri, newTri->numIndexes );

	totalVerts = 0;
	totalIndexes = 0;
	for ( i = 0; i < numSurfaces; i++ ) {
		tri = surfaces[i];
		memcpy( newTri->verts + totalVerts, tri->verts, tri->numVerts * sizeof( *tri->verts ) );
		for ( j = 0; j < tri->numIndexes; j++ ) {
			newTri->indexes[ totalIndexes + j ] = totalVerts + tri->indexes[j];
		}
		totalVerts += tri->numVerts;
		totalIndexes += tri->numIndexes;
	}

	return newTri;
}

/*
=================
R_MergeTriangles

Only deals with vertexes and indexes, not silhouettes, planes, etc.
Does NOT perform a cleanup triangles, so there may be duplicated verts in the result.
=================
*/
surfTriangles_t	*R_MergeTriangles( const surfTriangles_t *tri1, const surfTriangles_t *tri2 ) {
	const surfTriangles_t	*tris[2];

	tris[0] = tri1;
	tris[1] = tri2;

	return R_MergeSurfaceList( tris, 2 );
}

/*
=================
R_ReverseTriangles

Lit two sided surfaces need to have the triangles actually duplicated,
they can't just turn on two sided lighting, because the normal and tangents
are wrong on the other sides.

This should be called before R_CleanupTriangles
=================
*/
void R_ReverseTriangles( surfTriangles_t *tri ) {
	int			i;

	// flip the normal on each vertex
	// If the surface is going to have generated normals, this won't matter,
	// but if it has explicit normals, this will keep it on the correct side
	for ( i = 0; i < tri->numVerts; i++ ) {
		tri->verts[i].normal = vec3_origin - tri->verts[i].normal;
	}

	// flip the index order to make them back sided
	for ( i = 0; i < tri->numIndexes; i+= 3 ) {
		qglIndex_t	temp;

		temp = tri->indexes[ i + 0 ];
		tri->indexes[ i + 0 ] = tri->indexes[ i + 1 ];
		tri->indexes[ i + 1 ] = temp;
	}
}

/*
=================
R_CleanupTriangles

FIXME: allow createFlat and createSmooth normals, as well as explicit
=================
*/
void R_CleanupTriangles( surfTriangles_t *tri, bool createNormals, bool identifySilEdges, bool useUnsmoothedTangents ) {
	R_RangeCheckIndexes( tri );

	R_CreateSilIndexes( tri );

//	R_RemoveDuplicatedTriangles( tri );	// this may remove valid overlapped transparent triangles

	R_RemoveDegenerateTriangles( tri );

	R_TestDegenerateTextureSpace( tri );

//	R_RemoveUnusedVerts( tri );

	if ( identifySilEdges ) {
		R_IdentifySilEdges( tri, true );	// assume it is non-deformable, and omit coplanar edges
	}

	// bust vertexes that share a mirrored edge into separate vertexes
	R_DuplicateMirroredVertexes( tri );

	// optimize the index order (not working?)
//	R_OrderIndexes( tri->numIndexes, tri->indexes );

	R_CreateDupVerts( tri );

	R_BoundTriSurf( tri );

	if ( useUnsmoothedTangents ) {
		R_BuildDominantTris( tri );
		R_DeriveUnsmoothedTangents( tri );
	} else if ( !createNormals ) {
		R_DeriveFacePlanes( tri );
		R_DeriveTangentsWithoutNormals( tri );
	} else {
		R_DeriveTangents( tri );
	}
}

/*
===============
R_MeshCost
===============
*/
#define	CACHE_SIZE	24
#define	STALL_SIZE	8
int	R_MeshCost( int numIndexes, qglIndex_t *indexes ) {
	int	inCache[CACHE_SIZE];
	int	i, j, v;
	int	c_stalls;
	int	c_loads;
	int	fifo;

	for ( i = 0; i < CACHE_SIZE; i++ ) {
		inCache[i] = -1;
	}

	c_loads = 0;
	c_stalls = 0;
	fifo = 0;

	for ( i = 0; i < numIndexes; i++ ) {
		v = indexes[i];
		for ( j = 0; j < CACHE_SIZE; j++ ) {
			if ( inCache[ ( fifo + j ) % CACHE_SIZE ] == v ) {
				break;
			}
		}
		if ( j == CACHE_SIZE ) {
			c_loads++;
			inCache[ fifo % CACHE_SIZE ] = v;
			fifo++;
		} else if ( j < STALL_SIZE ) {
			c_stalls++;
		}
	}

	return c_loads;
}


typedef struct vertRef_s {
	struct vertRef_s	*next;
	int			tri;
} vertRef_t;

/*
====================
R_OrderIndexes

Reorganizes the indexes so they will take best advantage
of the internal GPU vertex caches
====================
*/
void R_OrderIndexes( int numIndexes, qglIndex_t *indexes ) {
	bool	*triangleUsed;
	int			numTris;
	qglIndex_t	*oldIndexes;
	qglIndex_t	*base;
	int			numOldIndexes;
	int			tri;
	int			i;
	vertRef_t	*vref, **vrefs, *vrefTable;
	int			numVerts;
	int			v1, v2;
	int			c_starts;
	int			c_cost;

	if ( !r_orderIndexes.GetBool() ) {
		return;
	}

	// save off the original indexes
	oldIndexes = (qglIndex_t *)_alloca( numIndexes * sizeof( *oldIndexes ) );
	memcpy( oldIndexes, indexes, numIndexes * sizeof( *oldIndexes ) );
	numOldIndexes = numIndexes;

	// make a table to mark the triangles when they are emited
	numTris = numIndexes / 3;
	triangleUsed = (bool *)_alloca( numTris * sizeof( *triangleUsed ) );
	memset( triangleUsed, 0, numTris * sizeof( *triangleUsed ) );

	// find the highest vertex number
	numVerts = 0;
	for ( i = 0; i < numIndexes; i++ ) {
		if ( indexes[i] > numVerts ) {
			numVerts = indexes[i];
		}
	}
	numVerts++;

	// create a table of triangles used by each vertex
	vrefs = (vertRef_t **)_alloca( numVerts * sizeof( *vrefs ) );
	memset( vrefs, 0, numVerts * sizeof( *vrefs ) );

	vrefTable = (vertRef_t *)_alloca( numIndexes * sizeof( *vrefTable ) );
	for ( i = 0; i < numIndexes; i++ ) {
		tri = i / 3;

		vrefTable[i].tri = tri;
		vrefTable[i].next = vrefs[oldIndexes[i]];
		vrefs[oldIndexes[i]] = &vrefTable[i];
	}

	// generate new indexes
	numIndexes = 0;
	c_starts = 0;
	while ( numIndexes != numOldIndexes ) {
		// find a triangle that hasn't been used
		for ( tri = 0; tri < numTris; tri++ ) {
			if ( !triangleUsed[tri] ) {
				break;
			}
		}
		if ( tri == numTris ) {
			common->Error( "R_OrderIndexes: ran out of unused tris" );
		}

		c_starts++;

		do {
			// emit this tri
			base = oldIndexes + tri * 3;
			indexes[numIndexes+0] = base[0];
			indexes[numIndexes+1] = base[1];
			indexes[numIndexes+2] = base[2];
			numIndexes += 3;

			triangleUsed[tri] = true;

			// try to find a shared edge to another unused tri
			for ( i = 0; i < 3; i++ ) {
				v1 = base[i];
				v2 = base[( i+1 )%3];

				for ( vref = vrefs[v1]; vref; vref = vref->next ) {
					tri = vref->tri;
					if ( triangleUsed[tri] ) {
						continue;
					}

					// if this triangle also uses v2, grab it
					if ( oldIndexes[tri*3+0] == v2
						|| oldIndexes[tri*3+1] == v2
						|| oldIndexes[tri*3+2] == v2 ) {
						break;
					}
				}
				if ( vref ) {
					break;
				}
			}

			// if we couldn't chain off of any verts, we need to find a new one
			if ( i == 3 ) {
				break;
			}
		} while ( 1 );
	}

	c_cost = R_MeshCost( numIndexes, indexes );
}

/*add all triangles that can be specified by the vertexes in the last 14 cache positions

  pick a new vert to add to the cache
  don't pick one in the 24 previous cache positions
  try to pick one that will enable the creation of as many triangles as possible

  look for a vert that shares an edge with the vert about to be evicted
*/

/*
===================================================================================

DEFORMED SURFACES

===================================================================================
*/

/*
===================
R_BuildDeformInfo
===================
*/
deformInfo_t *R_BuildDeformInfo( int numVerts, const aRcnicalImageVertex *verts, int numIndexes, const int *indexes, bool useUnsmoothedTangents ) {
	deformInfo_t	*deform;
	surfTriangles_t	tri;
	int				i;

	memset( &tri, 0, sizeof( tri ) );

	tri.numVerts = numVerts;
	R_AllocStaticTriSurfVerts( &tri, tri.numVerts );
	SIMDProcessor->Memcpy( tri.verts, verts, tri.numVerts * sizeof( tri.verts[0] ) );

	tri.numIndexes = numIndexes;
	R_AllocStaticTriSurfIndexes( &tri, tri.numIndexes );

	// don't memcpy, so we can change the index type from int to short without changing the interface
	for ( i = 0; i < tri.numIndexes; i++ ) {
		tri.indexes[i] = indexes[i];
	}

	R_RangeCheckIndexes( &tri );
	R_CreateSilIndexes( &tri );

// should we order the indexes here?

//	R_RemoveDuplicatedTriangles( &tri );
//	R_RemoveDegenerateTriangles( &tri );
//	R_RemoveUnusedVerts( &tri );
	R_IdentifySilEdges( &tri, false );			// we cannot remove coplanar edges, because
												// they can deform to silhouettes

	R_DuplicateMirroredVertexes( &tri );		// split mirror points into multiple points

	R_CreateDupVerts( &tri );

	if ( useUnsmoothedTangents ) {
		R_BuildDominantTris( &tri );
	}

	deform = (deformInfo_t *)R_ClearedStaticAlloc( sizeof( *deform ) );

	deform->numSourceVerts = numVerts;
	deform->numOutputVerts = tri.numVerts;

	deform->numIndexes = numIndexes;
	deform->indexes = tri.indexes;

	deform->silIndexes = tri.silIndexes;

	deform->numSilEdges = tri.numSilEdges;
	deform->silEdges = tri.silEdges;

	deform->dominantTris = tri.dominantTris;

	deform->numMirroredVerts = tri.numMirroredVerts;
	deform->mirroredVerts = tri.mirroredVerts;

	deform->numDupVerts = tri.numDupVerts;
	deform->dupVerts = tri.dupVerts;

	if ( tri.verts ) {
		triVertexAllocator.Free( tri.verts );
	}

	if ( tri.facePlanes ) {
		triPlaneAllocator.Free( tri.facePlanes );
	}

	return deform;
}

/*
===================
R_FreeDeformInfo
===================
*/
void R_FreeDeformInfo( deformInfo_t *deformInfo ) {
	if ( deformInfo->indexes != NULL ) {
		triIndexAllocator.Free( deformInfo->indexes );
	}
	if ( deformInfo->silIndexes != NULL ) {
		triSilIndexAllocator.Free( deformInfo->silIndexes );
	}
	if ( deformInfo->silEdges != NULL ) {
		triSilEdgeAllocator.Free( deformInfo->silEdges );
	}
	if ( deformInfo->dominantTris != NULL ) {
		triDominantTrisAllocator.Free( deformInfo->dominantTris );
	}
	if ( deformInfo->mirroredVerts != NULL ) {
		triMirroredVertAllocator.Free( deformInfo->mirroredVerts );
	}
	if ( deformInfo->dupVerts != NULL ) {
		triDupVertAllocator.Free( deformInfo->dupVerts );
	}
	R_StaticFree( deformInfo );
}

/*
===================
R_DeformInfoMemoryUsed
===================
*/
int R_DeformInfoMemoryUsed( deformInfo_t *deformInfo ) {
	int total = 0;

	if ( deformInfo->indexes != NULL ) {
		total += deformInfo->numIndexes * sizeof( deformInfo->indexes[0] );
	}
	if ( deformInfo->silIndexes != NULL ) {
		total += deformInfo->numIndexes * sizeof( deformInfo->silIndexes[0] );
	}
	if ( deformInfo->silEdges != NULL ) {
		total += deformInfo->numSilEdges * sizeof( deformInfo->silEdges[0] );
	}
	if ( deformInfo->dominantTris != NULL ) {
		total += deformInfo->numSourceVerts * sizeof( deformInfo->dominantTris[0] );
	}
	if ( deformInfo->mirroredVerts != NULL ) {
		total += deformInfo->numMirroredVerts * sizeof( deformInfo->mirroredVerts[0] );
	}
	if ( deformInfo->dupVerts != NULL ) {
		total += deformInfo->numDupVerts * sizeof( deformInfo->dupVerts[0] );
	}

	total += sizeof( *deformInfo );
	return total;
}

/*
==========================================================================================

GUI SHADERS

==========================================================================================
*/

/*
================
R_SurfaceToTextureAxis

Calculates two axis for the surface sutch that a point dotted against
the axis will give a 0.0 to 1.0 range in S and T when inside the gui surface
================
*/
void R_SurfaceToTextureAxis( const surfTriangles_t *tri, arcVec3 &origin, arcVec3 axis[3] ) {
	float		area, inva;
	float		d0[5], d1[5];
	aRcnicalImageVertex	*a, *b, *c;
	float		bounds[2][2];
	float		boundsOrg[2];
	int			i, j;
	float		v;

	// find the bounds of the texture
	bounds[0][0] = bounds[0][1] = 999999;
	bounds[1][0] = bounds[1][1] = -999999;
	for ( i = 0; i < tri->numVerts; i++ ) {
		for ( j = 0; j < 2; j++ ) {
			v = tri->verts[i].st[j];
			if ( v < bounds[0][j] ) {
				bounds[0][j] = v;
			}
			if ( v > bounds[1][j] ) {
				bounds[1][j] = v;
			}
		}
	}

	// use the floor of the midpoint as the origin of the
	// surface, which will prevent a slight misalignment
	// from throwing it an entire cycle off
	boundsOrg[0] = floor( ( bounds[0][0] + bounds[1][0] ) * 0.5 );
	boundsOrg[1] = floor( ( bounds[0][1] + bounds[1][1] ) * 0.5 );


	// determine the world S and T vectors from the first drawSurf triangle
	a = tri->verts + tri->indexes[0];
	b = tri->verts + tri->indexes[1];
	c = tri->verts + tri->indexes[2];

	VectorSubtract( b->xyz, a->xyz, d0 );
	d0[3] = b->st[0] - a->st[0];
	d0[4] = b->st[1] - a->st[1];
	VectorSubtract( c->xyz, a->xyz, d1 );
	d1[3] = c->st[0] - a->st[0];
	d1[4] = c->st[1] - a->st[1];

	area = d0[3] * d1[4] - d0[4] * d1[3];
	if ( area == 0.0 ) {
		axis[0].Zero();
		axis[1].Zero();
		axis[2].Zero();
		return;	// degenerate
	}
	inva = 1.0 / area;

    axis[0][0] = (d0[0] * d1[4] - d0[4] * d1[0] ) * inva;
    axis[0][1] = (d0[1] * d1[4] - d0[4] * d1[1] ) * inva;
    axis[0][2] = (d0[2] * d1[4] - d0[4] * d1[2] ) * inva;

    axis[1][0] = (d0[3] * d1[0] - d0[0] * d1[3] ) * inva;
    axis[1][1] = (d0[3] * d1[1] - d0[1] * d1[3] ) * inva;
    axis[1][2] = (d0[3] * d1[2] - d0[2] * d1[3] ) * inva;

	arcPlane plane;
	plane.FromPoints( a->xyz, b->xyz, c->xyz );
	axis[2][0] = plane[0];
	axis[2][1] = plane[1];
	axis[2][2] = plane[2];

	// take point 0 and project the vectors to the texture origin
	arcVec3->MA( a->xyz, boundsOrg[0] - a->st[0], axis[0], origin );
	arcVec3->MA( origin, boundsOrg[1] - a->st[1], axis[1], origin );
}

/*
=================
R_RenderGuiSurf

Create a texture space on the given surface and
call the GUI generator to create quads for it.
=================
*/
void R_RenderGuiSurf( arcUserInterfaces *gui, drawSurf_t *drawSurf ) {
	arcVec3	origin, axis[3];

	// for testing the performance hit
	if ( r_skipGuiShaders.GetInteger() == 1 ) {
		return;
	}

	// don't allow an infinite recursion loop
	if ( tr.guiRecursionLevel == 4 ) {
		return;
	}

	tr.pc.guiSurfaces++;

	// create the new matrix to draw on this surface
	R_SurfaceToTextureAxis( drawSurf->geo, origin, axis );

	float	guiModelMatrix[16];
	float	modelMatrix[16];

	guiModelMatrix[0] = axis[0][0] / 640.0;
	guiModelMatrix[4] = axis[1][0] / 480.0;
	guiModelMatrix[8] = axis[2][0];
	guiModelMatrix[12] = origin[0];

	guiModelMatrix[1] = axis[0][1] / 640.0;
	guiModelMatrix[5] = axis[1][1] / 480.0;
	guiModelMatrix[9] = axis[2][1];
	guiModelMatrix[13] = origin[1];

	guiModelMatrix[2] = axis[0][2] / 640.0;
	guiModelMatrix[6] = axis[1][2] / 480.0;
	guiModelMatrix[10] = axis[2][2];
	guiModelMatrix[14] = origin[2];

	guiModelMatrix[3] = 0;
	guiModelMatrix[7] = 0;
	guiModelMatrix[11] = 0;
	guiModelMatrix[15] = 1;

	GL_MultMatrix( guiModelMatrix, drawSurf->space->modelMatrix, modelMatrix );

	tr.guiRecursionLevel++;

	// call the gui, which will call the 2D drawing functions
	tr.guiModel->Clear();
	gui->Redraw( tr.viewDef->renderView.time );
	tr.guiModel->EmitToCurrentView( modelMatrix, drawSurf->space->ViewDepthHack );
	tr.guiModel->Clear();

	tr.guiRecursionLevel--;
}

/*
================,
R_ReloadGuis_f

Reloads any guis that have had their file timeStamps changed.
An optional "all" parameter will cause all models to reload, even
if they are not out of date.

Should we also reload the map models?
================
*/
void R_ReloadGuis_f( const arcCommandArgs &args ) {
	bool all;

	if ( !arcNetString::Icmp( args.Argv(1 ), "all" ) ) {
		all = true;
		common->Printf( "Reloading all gui files...\n" );
	} else {
		all = false;
		common->Printf( "Checking for changed gui files...\n" );
	}

	uiManager->Reload( all );
}

/*
================,
R_ListGuis_f
================
*/
void R_ListGuis_f( const arcCommandArgs &args ) {
	uiManager->ListGuis();
}