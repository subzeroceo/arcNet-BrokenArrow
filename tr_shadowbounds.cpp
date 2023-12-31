#include "/idlib/precompiled.h"
#pragma hdrstop

#include "tr_local.h"

#define MAX_POLYTOPE_PLANES		6

int	c_turboUsedVerts;
int c_turboUnusedVerts;

/*
=====================
R_CreateVertexProgramTurboShadowVolume

are dangling edges that are outside the light frustum still making planes?
=====================
*/
surfTriangles_t *R_CreateVertexProgramTurboShadowVolume( const anRenderEntityLocal *ent, const surfTriangles_t *tri, const anRenderLightsLocal *light, srfCullInfo_t &cullInfo ) {
	int		i, j;
	surfTriangles_t	*newTri;
	silEdge_t	*sil;
	const qglIndex_t *indexes;
	const byte *facing;

	R_CalcInteractionFacing( ent, tri, light, cullInfo );
	if ( r_useShadowProjectedCull.GetBool() ) {
		R_CalcInteractionCullBits( ent, tri, light, cullInfo );
	}

	int numFaces = tri->numIndexes / 3;
	int	numShadowingFaces = 0;
	facing = cullInfo.facing;

	// if all the triangles are inside the light frustum
	if ( cullInfo.cullBits == LIGHT_CULL_ALL_FRONT || !r_useShadowProjectedCull.GetBool() ) {
		// count the number of shadowing faces
		for ( i = 0; i < numFaces; i++ ) {
			numShadowingFaces += facing[i];
		}
		numShadowingFaces = numFaces - numShadowingFaces;
	} else {
		// make all triangles that are outside the light frustum "facing", so they won't cast shadows
		indexes = tri->indexes;
		byte *modifyFacing = cullInfo.facing;
		const byte *cullBits = cullInfo.cullBits;
		for ( j = i = 0; i < tri->numIndexes; i += 3, j++ ) {
			if ( !modifyFacing[j] ) {
				int	i1 = indexes[i+0];
				int	i2 = indexes[i+1];
				int	i3 = indexes[i+2];
				if ( cullBits[i1] & cullBits[i2] & cullBits[i3] ) {
					modifyFacing[j] = 1;
				} else {
					numShadowingFaces++;
				}
			}
		}
	}

	if ( !numShadowingFaces ) {
		// no faces are inside the light frustum and still facing the right way
		return NULL;
	}

	// shadowVerts will be NULL on these surfaces, so the shadowVerts will be taken from the ambient surface
	newTri = R_AllocStaticTriSurf();

	newTri->numVerts = tri->numVerts * 2;

	// alloc the max possible size
#ifdef USE_TRI_DATA_ALLOCATOR
	R_AllocStaticTriSurfIndexes( newTri, ( numShadowingFaces + tri->numSilEdges ) * 6 );
	qglIndex_t *tempIndexes = newTri->indexes;
	qglIndex_t *shadowIndexes = newTri->indexes;
#else
	qglIndex_t *tempIndexes = (qglIndex_t *)_alloca16( tri->numSilEdges * 6 * sizeof( tempIndexes[0] ) );
	qglIndex_t *shadowIndexes = tempIndexes;
#endif

	// create new triangles along sil planes
	for ( sil = tri->silEdges, i = tri->numSilEdges; i > 0; i--, sil++ ) {
		int f1 = facing[sil->p1];
		int f2 = facing[sil->p2];
		if ( !( f1 ^ f2 ) ) {
			continue;
		}

		int v1 = sil->v1 << 1;
		int v2 = sil->v2 << 1;

		// set the two triangle winding orders based on facing
		// without using a poorly-predictable branch
		shadowIndexes[0] = v1;
		shadowIndexes[1] = v2 ^ f1;
		shadowIndexes[2] = v2 ^ f2;
		shadowIndexes[3] = v1 ^ f2;
		shadowIndexes[4] = v1 ^ f1;
		shadowIndexes[5] = v2 ^ 1;

		shadowIndexes += 6;
	}

	int	numShadowIndexes = shadowIndexes - tempIndexes;

	// we aren't bothering to separate front and back caps on these
	newTri->numIndexes = newTri->numShadowIndexesNoFrontCaps = numShadowIndexes + numShadowingFaces * 6;
	newTri->numShadowIndexesNoCaps = numShadowIndexes;
	newTri->shadowCapPlaneBits = SHADOW_CAP_INFINITE;

#ifdef USE_TRI_DATA_ALLOCATOR
	// decrease the size of the memory block to only store the used indexes
	R_ResizeStaticTriSurfIndexes( newTri, newTri->numIndexes );
#else
	// allocate memory for the indexes
	R_AllocStaticTriSurfIndexes( newTri, newTri->numIndexes );
	// copy the indexes we created for the sil planes
	SIMDProcessor->Memcpy( newTri->indexes, tempIndexes, numShadowIndexes * sizeof( tempIndexes[0] ) );
#endif

	// these have no effect, because they extend to infinity
	newTri->bounds.Clear();

	// put some faces on the model and some on the distant projection
	indexes = tri->indexes;
	shadowIndexes = newTri->indexes + numShadowIndexes;
	for ( i = 0, j = 0; i < tri->numIndexes; i += 3, j++ ) {
		if ( facing[j] ) {
			continue;
		}

		int i0 = indexes[i+0] << 1;
		shadowIndexes[2] = i0;
		shadowIndexes[3] = i0 ^ 1;
		int i1 = indexes[i+1] << 1;
		shadowIndexes[1] = i1;
		shadowIndexes[4] = i1 ^ 1;
		int i2 = indexes[i+2] << 1;
		shadowIndexes[0] = i2;
		shadowIndexes[5] = i2 ^ 1;

		shadowIndexes += 6;
	}

	return newTri;
}

/*
=====================
R_CreateTurboShadowVolume
=====================
*/
surfTriangles_t *R_CreateTurboShadowVolume( const anRenderEntityLocal *ent, const surfTriangles_t *tri, const anRenderLightsLocal *light, srfCullInfo_t &cullInfo ) {
	int		i, j;
	anVec3	localLightOrigin;
	surfTriangles_t	*newTri;
	silEdge_t	*sil;
	const qglIndex_t *indexes;
	const byte *facing;

	R_CalcInteractionFacing( ent, tri, light, cullInfo );
	if ( r_useShadowProjectedCull.GetBool() ) {
		R_CalcInteractionCullBits( ent, tri, light, cullInfo );
	}

	int numFaces = tri->numIndexes / 3;
	int	numShadowingFaces = 0;
	facing = cullInfo.facing;

	// if all the triangles are inside the light frustum
	if ( cullInfo.cullBits == LIGHT_CULL_ALL_FRONT || !r_useShadowProjectedCull.GetBool() ) {
		// count the number of shadowing faces
		for ( i = 0; i < numFaces; i++ ) {
			numShadowingFaces += facing[i];
		}
		numShadowingFaces = numFaces - numShadowingFaces;
	} else {
		// make all triangles that are outside the light frustum "facing", so they won't cast shadows
		indexes = tri->indexes;
		byte *modifyFacing = cullInfo.facing;
		const byte *cullBits = cullInfo.cullBits;
		for ( j = i = 0; i < tri->numIndexes; i += 3, j++ ) {
			if ( !modifyFacing[j] ) {
				int	i1 = indexes[i+0];
				int	i2 = indexes[i+1];
				int	i3 = indexes[i+2];
				if ( cullBits[i1] & cullBits[i2] & cullBits[i3] ) {
					modifyFacing[j] = 1;
				} else {
					numShadowingFaces++;
				}
			}
		}
	}

	if ( !numShadowingFaces ) {
		// no faces are inside the light frustum and still facing the right way
		return NULL;
	}

	newTri = R_AllocStaticTriSurf();

#ifdef USE_TRI_DATA_ALLOCATOR
	R_AllocStaticTriSurfShadowVerts( newTri, tri->numVerts * 2 );
	anShadowCache *shadowVerts = newTri->shadowVertexes;
#else
	anShadowCache *shadowVerts = (anShadowCache *)_alloca16( tri->numVerts * 2 * sizeof( shadowVerts[0] ) );
#endif

	R_GlobalPointToLocal( ent->modelMatrix, light->globalLightOrigin, localLightOrigin );

	int	*vertRemap = ( int * )_alloca16( tri->numVerts * sizeof( vertRemap[0] ) );

	SIMDProcessor->Memset( vertRemap, -1, tri->numVerts * sizeof( vertRemap[0] ) );

	for ( i = 0, j = 0; i < tri->numIndexes; i += 3, j++ ) {
		if ( facing[j] ) {
			continue;
		}
		// this may pull in some vertexes that are outside
		// the frustum, because they connect to vertexes inside
		vertRemap[tri->silIndexes[i+0]] = 0;
		vertRemap[tri->silIndexes[i+1]] = 0;
		vertRemap[tri->silIndexes[i+2]] = 0;
	}

	newTri->numVerts = SIMDProcessor->CreateShadowCache( &shadowVerts->xyz, vertRemap, localLightOrigin, tri->verts, tri->numVerts );

	c_turboUsedVerts += newTri->numVerts;
	c_turboUnusedVerts += tri->numVerts * 2 - newTri->numVerts;

#ifdef USE_TRI_DATA_ALLOCATOR
	R_ResizeStaticTriSurfShadowVerts( newTri, newTri->numVerts );
#else
	R_AllocStaticTriSurfShadowVerts( newTri, newTri->numVerts );
	SIMDProcessor->Memcpy( newTri->shadowVertexes, shadowVerts, newTri->numVerts * sizeof( shadowVerts[0] ) );
#endif

	// alloc the max possible size
#ifdef USE_TRI_DATA_ALLOCATOR
	R_AllocStaticTriSurfIndexes( newTri, ( numShadowingFaces + tri->numSilEdges ) * 6 );
	qglIndex_t *tempIndexes = newTri->indexes;
	qglIndex_t *shadowIndexes = newTri->indexes;
#else
	qglIndex_t *tempIndexes = (qglIndex_t *)_alloca16( tri->numSilEdges * 6 * sizeof( tempIndexes[0] ) );
	qglIndex_t *shadowIndexes = tempIndexes;
#endif

	// create new triangles along sil planes
	for ( sil = tri->silEdges, i = tri->numSilEdges; i > 0; i--, sil++ ) {
		int f1 = facing[sil->p1];
		int f2 = facing[sil->p2];
		if ( !( f1 ^ f2 ) ) {
			continue;
		}

		int v1 = vertRemap[sil->v1];
		int v2 = vertRemap[sil->v2];

		// set the two triangle winding orders based on facing
		// without using a poorly-predictable branch

		shadowIndexes[0] = v1;
		shadowIndexes[1] = v2 ^ f1;
		shadowIndexes[2] = v2 ^ f2;
		shadowIndexes[3] = v1 ^ f2;
		shadowIndexes[4] = v1 ^ f1;
		shadowIndexes[5] = v2 ^ 1;

		shadowIndexes += 6;
	}

	int numShadowIndexes = shadowIndexes - tempIndexes;

	// we aren't bothering to separate front and back caps on these
	newTri->numIndexes = newTri->numShadowIndexesNoFrontCaps = numShadowIndexes + numShadowingFaces * 6;
	newTri->numShadowIndexesNoCaps = numShadowIndexes;
	newTri->shadowCapPlaneBits = SHADOW_CAP_INFINITE;

#ifdef USE_TRI_DATA_ALLOCATOR
	// decrease the size of the memory block to only store the used indexes
	R_ResizeStaticTriSurfIndexes( newTri, newTri->numIndexes );
#else
	// allocate memory for the indexes
	R_AllocStaticTriSurfIndexes( newTri, newTri->numIndexes );
	// copy the indexes we created for the sil planes
	SIMDProcessor->Memcpy( newTri->indexes, tempIndexes, numShadowIndexes * sizeof( tempIndexes[0] ) );
#endif

	// these have no effect, because they extend to infinity
	newTri->bounds.Clear();

	// put some faces on the model and some on the distant projection
	indexes = tri->silIndexes;
	shadowIndexes = newTri->indexes + numShadowIndexes;
	for ( i = 0, j = 0; i < tri->numIndexes; i += 3, j++ ) {
		if ( facing[j] ) {
			continue;
		}

		int i0 = vertRemap[indexes[i+0]];
		shadowIndexes[2] = i0;
		shadowIndexes[3] = i0 ^ 1;
		int i1 = vertRemap[indexes[i+1]];
		shadowIndexes[1] = i1;
		shadowIndexes[4] = i1 ^ 1;
		int i2 = vertRemap[indexes[i+2]];
		shadowIndexes[0] = i2;
		shadowIndexes[5] = i2 ^ 1;

		shadowIndexes += 6;
	}

	return newTri;
}

/*
=====================
R_PolytopeSurface

Generate vertexes and indexes for a polytope, and optionally returns the polygon windings.
The positive sides of the planes will be visible.
=====================
*/
surfTriangles_t *R_PolytopeSurface( int numPlanes, const anPlane *planes, anWinding **windings ) {
	arcFixedWinding planeWindings[MAX_POLYTOPE_PLANES];

	if ( numPlanes > MAX_POLYTOPE_PLANES ) {
		common->Error( "R_PolytopeSurface: more than %d planes", MAX_POLYTOPE_PLANES );
	}

	int numVerts = 0;
	int numIndexes = 0;
	for ( int i = 0; i < numPlanes; i++ ) {
		const anPlane &plane = planes[i];
		arcFixedWinding &w = planeWindings[i];

		w.BaseForPlane( plane );
		for ( int j = 0; j < numPlanes; j++ ) {
			const anPlane &plane2 = planes[j];
			if ( j == i ) {
				continue;
			}
			if ( !w.ClipInPlace( -plane2, ON_EPSILON ) ) {
				break;
			}
		}
		if ( w.GetNumPoints() <= 2 ) {
			continue;
		}
		numVerts += w.GetNumPoints();
		numIndexes += ( w.GetNumPoints() - 2 ) * 3;
	}

	// allocate the surface
	surfTriangles_t *tri = R_AllocStaticTriSurf();
	R_AllocStaticTriSurfVerts( tri, numVerts );
	R_AllocStaticTriSurfIndexes( tri, numIndexes );

	// copy the data from the windings
	for ( i = 0; i < numPlanes; i++ ) {
		arcFixedWinding &w = planeWindings[i];
		if ( !w.GetNumPoints() ) {
			continue;
		}
		for ( j = 0; j < w.GetNumPoints(); j++ ) {
			tri->verts[tri->numVerts + j ].Clear();
			tri->verts[tri->numVerts + j ].xyz = w[j].ToVec3();
		}

		for ( j = 1; j < w.GetNumPoints() - 1; j++ ) {
			tri->indexes[ tri->numIndexes + 0 ] = tri->numVerts;
			tri->indexes[ tri->numIndexes + 1 ] = tri->numVerts + j;
			tri->indexes[ tri->numIndexes + 2 ] = tri->numVerts + j + 1;
			tri->numIndexes += 3;
		}
		tri->numVerts += w.GetNumPoints();

		// optionally save the winding
		if ( windings ) {
			windings[i] = new anWinding( w.GetNumPoints() );
			*windings[i] = w;
		}
	}

	R_BoundTriSurf( tri );

	return tri;
}

// Compute conservative shadow bounds as the intersection
// of the object's bounds' shadow volume and the light's bounds.
template <class T, int N> struct arcArray {
arcArray() : s( 0 ) {
}

	arcArray( const arcArray<T,N> & cpy ) : s( cpy.s ) {
		for ( int i=0; i < s; i++ ) {
			v[i] = cpy.v[i];
		}
	}

	void PushBack( const T & i ) {
		v[s] = i;
		s++;
		//if (s > maxSize) {
			//maxSize = int(s);
		//}
	}

	T & operator[]( int i ) {
		return v[i];
	}

	const T & operator[]( int i ) const {
		return v[i];
	}

	unsigned int Size() const {
		return s;
	}

	void IsEmpty() {
		s = 0;
	}

	T v[N];
	int s;
//	static int maxSize;
};

typedef arcArray<int, 4> arcArrayInt;
//int arcArrayInt::maxSize = 0;
typedef arcArray<anVec4, 16> arcArrayVec4;
//int arcArrayVec4::maxSize = 0;

struct poly {
    arcArrayInt vi;
    arcArrayInt ni;
    anVec4 plane;
}; typedef arcArray<poly, 9> arcArrayPoly;
//int arcArrayPoly::maxSize = 0;

struct edge {
    int vi[2];
    int pi[2];
};

typedef arcArray<edge, 15> arcArrayEdge;
//int arcArrayEdge::maxSize = 0;

arcArrayInt FourIntegers( int a, int b, int c, int d ) {
    arcArrayInt vi;
    vi.PushBack( a );
    vi.PushBack( b );
    vi.PushBack( c );
    vi.PushBack( d );
    return vi;
}

anVec3 HomogeneousDifference( anVec4 a, anVec4 b) {
    anVec3 v;
	v.x = b.x * a.w - a.x * b.w;
	v.y = b.y * a.w - a.y * b.w;
	v.z = b.z * a.w - a.z * b.w;
    return v;
}

// handles positive w only
anVec4 ComputeHomogeneousPlane( anVec4 a, anVec4 b, anVec4 c ) {
	anVec4 v, t;

    if ( a[3] == 0 ) {
		t = a; a = b; b = c; c = t;
	}
	if ( a[3] == 0 ) {
		t = a; a = b; b = c; c = t;
	}

    // can't handle 3 infinite points
    if ( a[3] == 0 ) {
        return v;
	}

    anVec3 vb = HomogeneousDifference( a, b );
    anVec3 vc = HomogeneousDifference( a, c );

    anVec3 n = vb.Cross(vc);
    n.Normalize();

	v.x = n.x;
	v.y = n.y;
	v.z = n.z;

	v.w = - ( n * anVec3( a.x, a.y, a.z ) ) / a.w ;

    return v;
}

struct idPolyhedron {
    arcArrayVec4 v;
    arcArrayPoly  p;
    arcArrayEdge  e;

    void AddQuad( int va, int vb, int vc, int vd ) {
        poly pg;
        pg.vi = FourIntegers( va, vb, vc, vd );
        pg.ni = FourIntegers( -1, -1, -1, -1 );
        pg.plane = ComputeHomogeneousPlane( v[va], v[vb], v[vc] );
        p.PushBack( pg);
    }

    void DiscardNeighborInfo() {
        for ( unsigned int i = 0; i < p.Size(); i++ ) {
            arcArrayInt & ni = p[i].ni;
            for ( unsigned int j = 0; j < ni.Size(); j++ ) {
                ni[j] = -1;
			}
        }
    }

    void ComputeNeighbor() {
		e.IsEmpty();

        DiscardNeighborInfo();

        bool found;
        int P = p.Size();
        // for each polygon
        for ( int i = 0; i < P-1; i++ ) {
            const arcArrayInt & vi = p[i].vi;
            arcArrayInt & ni = p[i].ni;
            int Si = vi.Size();

            // for each edge of that polygon
            for ( int ii=0; ii < Si; ii++ ) {
                int ii0 = ii;
                int ii1 = ( ii+1 ) % Si;

                // continue if we've already found this neighbor
                if ( ni[ii] != -1 ) {
                    continue;
				}
                found = false;

                // check all remaining polygons
                for ( int j = i+1; j < P; j++ ) {
                    const arcArrayInt & vj = p[j].vi;
                    arcArrayInt & nj = p[j].ni;
                    int Sj = vj.Size();
                    for ( int jj = 0; jj < Sj; jj++ ) {
                        int jj0 = jj;
                        int ( j + 1 ) = (jj+1 ) % Sj;
                        if ( vi[ii0] == vj[( j + 1 )] && vi[ii1] == vj[jj0] ) {
                            edge ed;
                            ed.vi[0] = vi[ii0];
                            ed.vi[1] = vi[ii1];
                            ed.pi[0] = i;
                            ed.pi[1] = j;
                            e.PushBack( ed );
                            ni[ii] = j;
                            nj[jj] = i;
                            found = true;
                            break;
                        } else if ( vi[ii0] == vj[jj0] && vi[ii1] == vj[( j + 1 )] ) {
                            fprintf( stderr,"why am I here?\n" );
                        }
                    }
                    if ( found ) {
                        break;
					}
                }
            }
        }
    }

    void RecomputePlane() {
        // for each polygon
        for  ( unsigned int i = 0; i < p.Size(); i++ ) {
            p[i].plane = ComputeHomogeneousPlane( v[p[i].vi[0]], v[p[i].vi[1]], v[p[i].vi[2]] );
        }
    }

    void Transform( const anMat4 & m ) {
        for ( unsigned int i=0; i < v.Size(); i++ ) {
            v[i] = m * v[i];
		}
        RecomputePlane();
    }
};

// make a unit cube
idPolyhedron PolyhedronFromBounds( const anBounds &b ) {
	static idPolyhedron p;

	if ( p.e.Size() == 0 ) {
		p.v.PushBack( anVec4( -1, -1,  1, 1 ) );
		p.v.PushBack( anVec4(  1, -1,  1, 1 ) );
		p.v.PushBack( anVec4(  1,  1,  1, 1 ) );
		p.v.PushBack( anVec4( -1,  1,  1, 1 ) );
		p.v.PushBack( anVec4( -1, -1, -1, 1 ) );
		p.v.PushBack( anVec4(  1, -1, -1, 1 ) );
		p.v.PushBack( anVec4(  1,  1, -1, 1 ) );
		p.v.PushBack( anVec4( -1,  1, -1, 1 ) );

		p.AddQuad( 0, 1, 2, 3 );
		p.AddQuad( 7, 6, 5, 4 );
		p.AddQuad( 1, 0, 4, 5 );
		p.AddQuad( 2, 1, 5, 6 );
		p.AddQuad( 3, 2, 6, 7 );
		p.AddQuad( 0, 3, 7, 4 );

		p.ComputeNeighbor();
		p.RecomputePlane();
		p.v.IsEmpty(); // no need to copy this data since it'll be replaced
	}

	idPolyhedron p2( p);

	const anVec3 & min = b[0];
	const anVec3 & max = b[1];

	p2.v.IsEmpty();
	p2.v.PushBack( anVec4( min.x, min.y, max.z, 1 ) );
	p2.v.PushBack( anVec4( max.x, min.y, max.z, 1 ) );
	p2.v.PushBack( anVec4( max.x, max.y, max.z, 1 ) );
	p2.v.PushBack( anVec4( min.x, max.y, max.z, 1 ) );
	p2.v.PushBack( anVec4( min.x, min.y, min.z, 1 ) );
	p2.v.PushBack( anVec4( max.x, min.y, min.z, 1 ) );
	p2.v.PushBack( anVec4( max.x, max.y, min.z, 1 ) );
	p2.v.PushBack( anVec4( min.x, max.y, min.z, 1 ) );

	p2.RecomputePlane();
    return p2;
}

idPolyhedron MakeShadowVolume( const idPolyhedron & oc, anVec4 light ) {
	static idPolyhedron lut[64];
	int index = 0;

	for ( unsigned int i = 0; i < 6; i++ ) {
		if ( ( oc.p[i].plane * light ) > 0 )
			index |= 1<<i;
	}

	if ( lut[index].e.Size() == 0 ) {
		idPolyhedron & ph = lut[index];
		ph = oc;

		int V = ph.v.Size();
		for ( int j = 0; j < V; j++ ) {
			anVec3 proj = HomogeneousDifference( light, ph.v[j] );
			ph.v.PushBack( anVec4( proj.x, proj.y, proj.z, 0 ) );
		}

		ph.p.IsEmpty();

		for ( unsigned int i=0; i < oc.p.Size(); i++ ) {
			if ( ( oc.p[i].plane * light ) > 0 ) {
				ph.p.PushBack( oc.p[i] );
			}
		}

		if ( ph.p.Size() == 0 ) {
			return ph = idPolyhedron();
		}
		ph.ComputeNeighbor();

		arcArrayPoly vpg;
		int I = ph.p.Size();
		for ( int i=0; i < I; i++ ) {
			arcArrayInt & vi = ph.p[i].vi;
			arcArrayInt & ni = ph.p[i].ni;
			int S = vi.Size();

			for ( int j = 0; j < S; j++ ) {
				if ( ni[j] == -1 ) {
					poly pg;
					int a = vi[( j+1 )%S];
					int b = vi[j];
					pg.vi = FourIntegers( a, b, b+V, a+V );
					pg.ni = FourIntegers( -1, -1, -1, -1 );
					vpg.PushBack( pg );
				}
			}
		}
		for ( unsigned int i = 0; i < vpg.Size(); i++ ) {
			ph.p.PushBack(vpg[i] );
		}

		ph.ComputeNeighbor();
		ph.v.IsEmpty(); // no need to copy this data since it'll be replaced
	}

	idPolyhedron ph2 = lut[index];

	// initalize vertices
	ph2.v = oc.v;
	int V = ph2.v.Size();
	for ( int j = 0; j < V; j++ ) {
		anVec3 proj = HomogeneousDifference( light, ph2.v[j] );
		ph2.v.PushBack( anVec4( proj.x, proj.y, proj.z, 0 ) );
	}

    // need to compute planes for the shadow volume (sv)
    ph2.RecomputePlane();

    return ph2;
}

typedef arcArray<anVec4, 36> idSegments;
//int idSegments::maxSize = 0;

void idPolyhedronEdges( idPolyhedron & a, idSegments & e ) {
	e.IsEmpty();
   if ( a.e.Size() == 0 && a.p.Size() != 0 ) {
        a.ComputeNeighbor();
   }

    for ( unsigned int i = 0; i < a.e.Size(); i++ ) {
        e.PushBack( a.v[a.e[i].vi[0]] );
        e.PushBack( a.v[a.e[i].vi[1]] );
    }
}

// clip the segments of e by the planes of idPolyhedron a.
void ClipSegments(const idPolyhedron & ph, idSegments & is, idSegments & os) {
    const arcArrayPoly & p = ph.p;
    for ( unsigned int i = 0; i < is.Size(); i+=2 ) {
        anVec4 a = is[i  ];
        anVec4 b = is[i+1];
        anVec4 c;

        bool discard = false;

        for ( unsigned int j = 0; j < p.Size(); j++ ) {
            float da = a * p[j].plane;
            float db = b * p[j].plane;
            float rdw = 1/( da - db );

			int code = 0;

			if ( da > 0 ) {
				code = 2;
			}
			if ( db > 0 ) {
				code |= 1;
			}

            switch ( code ) {
            case 3:
                discard = true;
                break;

            case 2:
                c = -db * rdw * a + da * rdw * b;
                a = c;
                break;

            case 1:
                c = -db * rdw * a + da * rdw * b;
                b = c;
                break;

            case 0:
                break;

            default:
                common->Printf( "bad clip code!\n" );
                break;
            }

           if ( discard ) {
                break;
		   }
        }

       if ( ! discard ) {
            os.PushBack( a );
            os.PushBack( b );
        }
    }
}

anMat4 MakeMatrix4( const float * m ) {
	return anMat4( m[ 0], m[ 4], m[ 8], m[12],
				   m[ 1], m[ 5], m[ 9], m[13],
				   m[ 2], m[ 6], m[10], m[14],
				   m[ 3], m[ 7], m[11], m[15] );
}

void DrawPolyhedron( const viewDef_t *viewDef, const idPolyhedron & p, anVec4 color ) {
	for ( unsigned int i = 0; i < p.e.Size(); i++ ) {
		viewDef->renderWorld->DebugLine( color, anVec4::4ToVec3( p.v[p.e[i].vi[0]] ), anVec4::4ToVec3( p.v[p.e[i].vi[1]] ) );
	}
}

void DrawSegments( const viewDef_t *viewDef, const idSegments & s, anVec4 color ) {
	for ( unsigned int i = 0; i < s.Size(); i += 2 ) {
		viewDef->renderWorld->DebugLine( color, anVec4::4ToVec3( s[i] ), anVec4::4ToVec3( s[i+1] ) );
	}
}

void R_GlobalToHClip( const viewDef_t *viewDef, const anVec4 &global, anVec4 &clip ) {
	//const arcRenderMatrices & projectionMatrix = viewDef->worldSpace.modelViewMatrix;
	//const arcRenderMatrices::Multiply( anMat4( anVec4( 1.0f ), anVec4( 0.0f, 0.0f, 0.0f, 0.0f ) ), anMat4( anVec4( 0.0f, 1.0f ), id )

	for ( int i = 0; i < 4; i ++ ) {
		anVec4 view[i] =
			global[0] * viewDef->worldSpace.modelViewMatrix[ i + 0 * 4 ] +
			global[1] * viewDef->worldSpace.modelViewMatrix[ i + 1 * 4 ] +
			global[2] * viewDef->worldSpace.modelViewMatrix[ i + 2 * 4 ] +
			global[3] *	viewDef->worldSpace.modelViewMatrix[ i + 3 * 4 ];
	}

	for ( int i = 0; i < 4; i ++ ) {
		clip[i] =
			anVec4 view[0] * viewDef->projectionMatrix[ i + 0 * 4 ] +
			anVec4 view[1] * viewDef->projectionMatrix[ i + 1 * 4 ] +
			anVec4 view[2] * viewDef->projectionMatrix[ i + 2 * 4 ] +
			anVec4 view[3] * viewDef->projectionMatrix[ i + 3 * 4 ];
	}
}

anScreenRect R_CalcIntersectionScissor( const anRenderLightsLocal * lightDef, const anRenderEntityLocal * entityDef, const viewDef_t * viewDef ) {
	anMat4 a = MakeMatrix4( entityDef->modelMatrix );
	anMat4 b = MakeMatrix4( lightDef->modelMatrix );

	// compute light idPolyhedron
	idPolyhedron lightVol = idPolyhedronFromBounds( lightDef->frustumTris->bounds );
	// Transform it into world space
	//lightVol.Transform( b );

	// debug //
	if ( r_useInteractionScissors.GetInteger() == -2 ) {
		DrawPolyhedron( viewDef, lightVol, colorRed );
	}

	// compute object idPolyhedron
	idPolyhedron vol = idPolyhedronFromBounds( entityDef->referenceBounds );

	//viewDef->renderWorld->DebugBounds( colorRed, lightDef->frustumTris->bounds );
	//viewDef->renderWorld->DebugBox( colorBlue, anBox( model->Bounds(), entityDef->parms.origin, entityDef->parms.axis ) );

	// Transform it into world space
    vol.Transform( a );

	// debug //
	if ( r_useInteractionScissors.GetInteger() == -2 ) {
		DrawPolyhedron( viewDef, vol, colorBlue );
	}

	// Transform light position into world space
	anVec4 lighPos = anVec4( lightDef->globalLightOrigin.x, lightDef->globalLightOrigin.y, lightDef->globalLightOrigin.z, 1.0f );

	// generate shadow volume "idPolyhedron"
    idPolyhedron sv = MakeShadowVolume( vol, lighPos );

    idSegments inSegs, outSegs;

	// get shadow volume edges
    idPolyhedronEdges( sv, inSegs );
	// clip them against light bounds planes
    ClipSegments( lightVol, inSegs, outSegs );

	// get light bounds edges
	idPolyhedronEdges( lightVol, inSegs );
	// clip them by the shadow volume
    ClipSegments( sv, inSegs, outSegs);

	// debug //
	if ( r_useInteractionScissors.GetInteger() == -2 ) {
		DrawSegments( viewDef, outSegs, colorGreen );
	}

	anBounds outBounds;
	outBounds.Clear();
	for ( unsigned int i = 0; i < outSegs.Size(); i++ ) {
		anVec4 v;
		R_GlobalToHClip( viewDef, outSegs[i], v );

		if ( v.w <= 0.0f ) {
			return lightDef->viewLight->scissorRect;
		}

		anVec3 rv( v.x, v.y, v.z );
		rv /= v.w;

		outBounds.AddPoint( rv );
	}

	// limit the bounds to avoid an inside out scissor rectangle due to floating point to short conversion
	if ( outBounds[0].x < -1.0f ) {
		outBounds[0].x = -1.0f;
	}
	if ( outBounds[1].x > 1.0f ) {
		outBounds[1].x = 1.0f;
	}
	if ( outBounds[0].y < -1.0f ) {
		outBounds[0].y = -1.0f;
	}
	if ( outBounds[1].y > 1.0f ) {
		outBounds[1].y = 1.0f;
	}

	float w2 = ( viewDef->viewport.x2 - viewDef->viewport.x1 + 1 ) / 2.0f;
	float x = viewDef->viewport.x1;
	float h2 = ( viewDef->viewport.y2 - viewDef->viewport.y1 + 1 ) / 2.0f;
	float y = viewDef->viewport.y1;

	anScreenRect rect;
	rect.x1 = outBounds[0].x * w2 + w2 + x;
	rect.x2 = outBounds[1].x * w2 + w2 + x;
	rect.y1 = outBounds[0].y * h2 + h2 + y;
	rect.y2 = outBounds[1].y * h2 + h2 + y;
	rect.Expand();

	rect.Intersect( lightDef->viewLight->scissorRect );

	// debug //
	if ( r_useInteractionScissors.GetInteger() == -2 && !rect.IsEmpty() ) {
		viewDef->renderWorld->DebugScreenRect( colorYellow, rect, viewDef );
	}

	return rect;
}

// tr_stencilShadow.c -- creaton of stencil shadow volumes
/*Should we split shadow volume surfaces when they exceed max verts
  or max indexes?

  a problem is that the number of vertexes needed for the
  shadow volume will be twice the number in the original,
  and possibly up to 8/3 when near plane clipped.

  The maximum index count is 7x when not clipped and all
  triangles are completely discrete.  Near plane clipping
  can increase this to 10x.

  The maximum expansions are always with discrete triangles.
  Meshes of triangles will result in less index expansion because
  there will be less silhouette edges, although it will always be
  greater than the source if a cap is present.

  can't just project onto a plane if some surface points are
  behind the light.

  The cases when a face is edge on to a light is robustly handled
  with closed volumes, because only a single one of it's neighbors
  will pass the edge test.  It may be an issue with non-closed models.

  It is crucial that the shadow volumes be completely enclosed.
  The triangles identified as shadow sources will be projected
  directly onto the light far plane.
  The sil edges must be handled carefully.
  A partially clipped explicit sil edge will still generate a sil
  edge.
  EVERY new edge generated by clipping the triangles to the view
  will generate a sil edge.

  If a triangle has no points inside the frustum, it is completely
  culled away.  If a sil edge is either in or on the frustum, it is
  added.
  If a triangle has no points outside the frustum, it does not
  need to be clipped.

  USING THE STENCIL BUFFER FOR SHADOWING

  basic triangle property

  view plane inside shadow volume problem

  quad triangulation issue

  issues with silhouette optimizations

  the shapes of shadow projections are poor for sphere or box culling

  the gouraud shading problem


  // epsilon culling rules:

// the positive side of the frustum is inside
d = tri->verts[i].xyz * frustum[j].Normal() + frustum[j][3];
if ( d < LIGHT_CLIP_EPSILON ) {
	pointCull[i] |= ( 1 << j );
}
if ( d > -LIGHT_CLIP_EPSILON ) {
	pointCull[i] |= ( 1 << (6+j ) );
}

If a low order bit is set, the point is on or outside the plane
If a high order bit is set, the point is on or inside the plane
If a low order bit is clear, the point is inside the plane (definately positive)
If a high order bit is clear, the point is outside the plane (definately negative)
*/

#define TRIANGLE_CULLED( p1, p2, p3 ) ( pointCull[p1] & pointCull[p2] & pointCull[p3] & 0x3f )

//#define TRIANGLE_CLIPPED( p1, p2, p3 ) ( ( pointCull[p1] | pointCull[p2] | pointCull[p3] ) & 0xfc0 )
#define TRIANGLE_CLIPPED( p1, p2, p3 ) ( ( ( pointCull[p1] & pointCull[p2] & pointCull[p3] ) & 0xfc0 ) != 0xfc0 )

// an edge that is on the plane is NOT culled
#define EDGE_CULLED( p1, p2 ) ( ( pointCull[p1] ^ 0xfc0 ) & ( pointCull[p2] ^ 0xfc0 ) & 0xfc0 )
#define EDGE_CLIPPED( p1, p2 ) ( ( pointCull[p1] & pointCull[p2] & 0xfc0 ) != 0xfc0 )

// a point that is on the plane is NOT culled
//#define	POINT_CULLED( p1 ) ( ( pointCull[p1] ^ 0xfc0 ) & 0xfc0 )
#define	POINT_CULLED( p1 ) ( ( pointCull[p1] & 0xfc0 ) != 0xfc0 )

//#define	LIGHT_CLIP_EPSILON	0.001f
#define	LIGHT_CLIP_EPSILON		0.1f
#define	MAX_CLIP_SIL_EDGES		2048
static int			numClipSilEdges;
static int			clipSilEdges[MAX_CLIP_SIL_EDGES][2];

// facing will be 0 if forward facing, 1 if backwards facing
// grabbed with alloca
static byte			*globalFacing;

// faceCastsShadow will be 1 if the face is in the projection
// and facing the apropriate direction
static byte			*faceCastsShadow;

static int			*remap;

#define	MAX_SHADOW_INDEXES		0x18000
#define	MAX_SHADOW_VERTS		0x18000
static int			numShadowIndexes;
static qglIndex_t	shadowIndexes[MAX_SHADOW_INDEXES];
static int			numShadowVerts;
static anVec4		shadowVerts[MAX_SHADOW_VERTS];
static bool			overflowed;

anPlane	pointLightFrustums[6][6] = {
	{	anPlane( 1,0,0,0 ),
		anPlane( 1,1,0,0 ),
		anPlane( 1,-1,0,0 ),
		anPlane( 1,0,1,0 ),
		anPlane( 1,0,-1,0 ),
		anPlane( -1,0,0,0 ),
	},{
		anPlane( -1,0,0,0 ),
		anPlane( -1,1,0,0 ),
		anPlane( -1,-1,0,0 ),
		anPlane( -1,0,1,0 ),
		anPlane( -1,0,-1,0 ),
		anPlane( 1,0,0,0 ),
	},{
		anPlane( 0,1,0,0 ),
		anPlane( 0,1,1,0 ),
		anPlane( 0,1,-1,0 ),
		anPlane( 1,1,0,0 ),
		anPlane( -1,1,0,0 ),
		anPlane( 0,-1,0,0 ),
	},{
		anPlane( 0,-1,0,0 ),
		anPlane( 0,-1,1,0 ),
		anPlane( 0,-1,-1,0 ),
		anPlane( 1,-1,0,0 ),
		anPlane( -1,-1,0,0 ),
		anPlane( 0,1,0,0 ),
	},{
		anPlane( 0,0,1,0 ),
		anPlane( 1,0,1,0 ),
		anPlane( -1,0,1,0 ),
		anPlane( 0,1,1,0 ),
		anPlane( 0,-1,1,0 ),
		anPlane( 0,0,-1,0 ),
	},{
		anPlane( 0,0,-1,0 ),
		anPlane( 1,0,-1,0 ),
		anPlane( -1,0,-1,0 ),
		anPlane( 0,1,-1,0 ),
		anPlane( 0,-1,-1,0 ),
		anPlane( 0,0,1,0 ),},
};

int			c_caps, c_sils;
static bool	callOptimizer;			// call the preprocessor optimizer after clipping occluders

typedef struct {
	int		frontCapStart;
	int		rearCapStart;
	int		silStart;
	int		end;
} indexRef_t;
static indexRef_t	indexRef[6];
static int indexFrustumNumber;		// which shadow generating side of a light the indexRef is for

/*
===============
PointsOrdered

To make sure the triangulations of the sil edges is consistant,
we need to be able to order two points.  We don't care about how
they compare with any other points, just that when the same two
points are passed in (in either order), they will always specify
the same one as leading.

Currently we need to have separate faces in different surfaces
order the same way, so we must look at the actual coordinates.
If surfaces are ever guaranteed to not have to edge match with
other surfaces, we could just compare indexes.
===============
*/
static bool PointsOrdered( const anVec3 &a, const anVec3 &b ) {
	float	i, j;

	// vectors that wind up getting an equal hash value will
	// potentially cause a misorder, which can show as a couple
	// crack pixels in a shadow

	// scale by some odd numbers so -8, 8, 8 will not be equal
	// to 8, -8, 8

	// in the very rare case that these might be equal, all that would
	// happen is an oportunity for a tiny rasterization shadow crack
	i = a[0] + a[1]*127 + a[2]*1023;
	j = b[0] + b[1]*127 + b[2]*1023;

	return (bool)( i < j);
}

/*
====================
R_LightProjectionMatrix

====================
*/
void R_LightProjectionMatrix( const anVec3 &origin, const anPlane &rearPlane, anVec4 mat[4] ) {
	anVec4		lv;
	float		lg;

	// calculate the homogenious light vector
	lv.x = origin.x;
	lv.y = origin.y;
	lv.z = origin.z;
	lv.w = 1;

	lg = rearPlane.ToVec4() * lv;

	// outer product
	mat[0][0] = lg -rearPlane[0] * lv[0];
	mat[0][1] = -rearPlane[1] * lv[0];
	mat[0][2] = -rearPlane[2] * lv[0];
	mat[0][3] = -rearPlane[3] * lv[0];

	mat[1][0] = -rearPlane[0] * lv[1];
	mat[1][1] = lg -rearPlane[1] * lv[1];
	mat[1][2] = -rearPlane[2] * lv[1];
	mat[1][3] = -rearPlane[3] * lv[1];

	mat[2][0] = -rearPlane[0] * lv[2];
	mat[2][1] = -rearPlane[1] * lv[2];
	mat[2][2] = lg -rearPlane[2] * lv[2];
	mat[2][3] = -rearPlane[3] * lv[2];

	mat[3][0] = -rearPlane[0] * lv[3];
	mat[3][1] = -rearPlane[1] * lv[3];
	mat[3][2] = -rearPlane[2] * lv[3];
	mat[3][3] = lg -rearPlane[3] * lv[3];
}

/*
===================
R_ProjectPointsToFarPlane

make a projected copy of the even verts into the odd spots
that is on the far light clip plane
===================
*/
static void R_ProjectPointsToFarPlane( const anRenderEntityLocal *ent, const anRenderLightsLocal *light, const anPlane &lightPlaneLocal, int firstShadowVert, int numShadowVerts ) {
	anVec3		lv;
	anVec4		mat[4];
	int			i;
	anVec4		*in;

	R_GlobalPointToLocal( ent->modelMatrix, light->globalLightOrigin, lv );
	R_LightProjectionMatrix( lv, lightPlaneLocal, mat );

#if 1
	// make a projected copy of the even verts into the odd spots
	in = &shadowVerts[firstShadowVert];
	for ( i = firstShadowVert; i < numShadowVerts; i+= 2, in += 2 ) {
		float	w, oow;

		in[0].w = 1;

		w = in->ToVec3() * mat[3].ToVec3() + mat[3][3];
		if ( w == 0 ) {
			in[1] = in[0];
			continue;
		}

		oow = 1.0 / w;
		in[1].x = ( in->ToVec3() * mat[0].ToVec3() + mat[0][3] ) * oow;
		in[1].y = ( in->ToVec3() * mat[1].ToVec3() + mat[1][3] ) * oow;
		in[1].z = ( in->ToVec3() * mat[2].ToVec3() + mat[2][3] ) * oow;
		in[1].w = 1;
	}
#else
	// messing with W seems to cause some depth precision problems
	// make a projected copy of the even verts into the odd spots
	in = &shadowVerts[firstShadowVert];
	for ( i = firstShadowVert; i < numShadowVerts; i+= 2, in += 2 ) {
		in[0].w = 1;
		in[1].x = *in * mat[0].ToVec3() + mat[0][3];
		in[1].y = *in * mat[1].ToVec3() + mat[1][3];
		in[1].z = *in * mat[2].ToVec3() + mat[2][3];
		in[1].w = *in * mat[3].ToVec3() + mat[3][3];
	}
#endif
}

#define	MAX_CLIPPED_POINTS	20
typedef struct {
	int		numVerts;
	anVec3	verts[MAX_CLIPPED_POINTS];
	int		edgeFlags[MAX_CLIPPED_POINTS];
} clipTri_t;

/*
=============
R_ChopWinding

Clips a triangle from one buffer to another, setting edge flags
The returned buffer may be the same as inNum if no clipping is done
If entirely clipped away, clipTris[returned].numVerts == 0

I have some worries about edge flag cases when polygons are clipped
multiple times near the epsilon.
=============
*/
static int R_ChopWinding( clipTri_t clipTris[2], int inNum, const anPlane &plane ) {
	clipTri_t	*in, *out;
	float	dists[MAX_CLIPPED_POINTS];
	int		sides[MAX_CLIPPED_POINTS];
	int		counts[3];
	float	dot;
	int		i, j;
	anVec3	*p1, *p2;
	anVec3	mid;

	in = &clipTris[inNum];
	out = &clipTris[inNum^1];
	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	for ( i = 0; i < in->numVerts; i++ ) {
		dot = plane.Distance( in->verts[i] );
		dists[i] = dot;
		if ( dot < -LIGHT_CLIP_EPSILON ) {
			sides[i] = SIDE_BACK;
		} else if ( dot > LIGHT_CLIP_EPSILON ) {
			sides[i] = SIDE_FRONT;
		} else {
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}

	// if none in front, it is completely clipped away
	if ( !counts[SIDE_FRONT] ) {
		in->numVerts = 0;
		return inNum;
	}
	if ( !counts[SIDE_BACK] ) {
		return inNum;		// inout stays the same
	}

	// avoid wrapping checks by duplicating first value to end
	sides[i] = sides[0];
	dists[i] = dists[0];
	in->verts[in->numVerts] = in->verts[0];
	in->edgeFlags[in->numVerts] = in->edgeFlags[0];

	out->numVerts = 0;
	for ( i = 0; i < in->numVerts; i++ ) {
		p1 = &in->verts[i];
		if ( sides[i] != SIDE_BACK ) {
			out->verts[out->numVerts] = *p1;
			if ( sides[i] == SIDE_ON && sides[i+1] == SIDE_BACK ) {
				out->edgeFlags[out->numVerts] = 1;
			} else {
				out->edgeFlags[out->numVerts] = in->edgeFlags[i];
			}
			out->numVerts++;
		}

		if ( ( sides[i] == SIDE_FRONT && sides[i+1] == SIDE_BACK )
			|| ( sides[i] == SIDE_BACK && sides[i+1] == SIDE_FRONT ) ) {
			// generate a split point
			p2 = &in->verts[i+1];
			dot = dists[i] / ( dists[i]-dists[i+1] );
			for ( j = 0; j<3; j++ ) {
				mid[j] = ( *p1 )[j] + dot*( ( *p2 )[j]-( *p1 )[j] );
			}

			out->verts[out->numVerts] = mid;

			// set the edge flag
			if ( sides[i+1] != SIDE_FRONT ) {
				out->edgeFlags[out->numVerts] = 1;
			} else {
				out->edgeFlags[out->numVerts] = in->edgeFlags[i];
			}

			out->numVerts++;
		}
	}

	return inNum ^ 1;
}

/*
===================
R_ClipTriangleToLight

Returns false if nothing is left after clipping
===================
*/
static bool	R_ClipTriangleToLight( const anVec3 &a, const anVec3 &b, const anVec3 &c, int planeBits, const anPlane frustum[6] ) {
	int			i;
	int			base;
	clipTri_t	pingPong[2], *ct;
	int			p;

	pingPong[0].numVerts = 3;
	pingPong[0].edgeFlags[0] = 0;
	pingPong[0].edgeFlags[1] = 0;
	pingPong[0].edgeFlags[2] = 0;
	pingPong[0].verts[0] = a;
	pingPong[0].verts[1] = b;
	pingPong[0].verts[2] = c;

	p = 0;
	for ( i = 0; i < 6; i++ ) {
		if ( planeBits & ( 1 << i ) ) {
			p = R_ChopWinding( pingPong, p, frustum[i] );
			if ( pingPong[p].numVerts < 1 ) {
				return false;
			}
		}
	}
	ct = &pingPong[p];

	// copy the clipped points out to shadowVerts
	if ( numShadowVerts + ct->numVerts * 2 > MAX_SHADOW_VERTS ) {
		overflowed = true;
		return false;
	}

	base = numShadowVerts;
	for ( i = 0; i < ct->numVerts; i++ ) {
		shadowVerts[ base + i*2 ].ToVec3() = ct->verts[i];
	}
	numShadowVerts += ct->numVerts * 2;

	if ( numShadowIndexes + 3 * ( ct->numVerts - 2 ) > MAX_SHADOW_INDEXES ) {
		overflowed = true;
		return false;
	}

	for ( i = 2; i < ct->numVerts; i++ ) {
		shadowIndexes[numShadowIndexes++] = base + i * 2;
		shadowIndexes[numShadowIndexes++] = base + ( i - 1 ) * 2;
		shadowIndexes[numShadowIndexes++] = base;
	}

	// any edges that were created by the clipping process will
	// have a silhouette quad created for it, because it is one
	// of the exterior bounds of the shadow volume
	for ( i = 0; i < ct->numVerts; i++ ) {
		if ( ct->edgeFlags[i] ) {
			if ( numClipSilEdges == MAX_CLIP_SIL_EDGES ) {
				break;
			}
			clipSilEdges[ numClipSilEdges ][0] = base + i * 2;
			if ( i == ct->numVerts - 1 ) {
				clipSilEdges[ numClipSilEdges ][1] = base;
			} else {
				clipSilEdges[ numClipSilEdges ][1] = base + ( i + 1 ) * 2;
			}
			numClipSilEdges++;
		}
	}

	return true;
}

/*
===================
R_ClipLineToLight

If neither point is clearly behind the clipping
plane, the edge will be passed unmodified.  A sil edge that
is on a border plane must be drawn.

If one point is clearly clipped by the plane and the
other point is on the plane, it will be completely removed.
===================
*/
static bool R_ClipLineToLight(	const anVec3 &a, const anVec3 &b, const anPlane frustum[4], anVec3 &p1, anVec3 &p2 ) {
	float	*clip;
	int		j;
	float	d1, d2;
	float	f;

	p1 = a;
	p2 = b;

	// clip it
	for ( j = 0; j < 6; j++ ) {
		d1 = frustum[j].Distance( p1 );
		d2 = frustum[j].Distance( p2 );

		// if both on or in front, not clipped to this plane
		if ( d1 > -LIGHT_CLIP_EPSILON && d2 > -LIGHT_CLIP_EPSILON ) {
			continue;
		}

		// if one is behind and the other isn't clearly in front, the edge is clipped off
		if ( d1 <= -LIGHT_CLIP_EPSILON && d2 < LIGHT_CLIP_EPSILON ) {
			return false;
		}
		if ( d2 <= -LIGHT_CLIP_EPSILON && d1 < LIGHT_CLIP_EPSILON ) {
			return false;
		}

		// clip it, keeping the negative side
		if ( d1 < 0 ) {
			clip = p1.ToFloatPtr();
		} else {
			clip = p2.ToFloatPtr();
		}

#if 0
		if ( anMath::Fabs(d1 - d2) < 0.001 ) {
			d2 = d1 - 0.1;
		}
#endif

		f = d1 / ( d1 - d2 );
		clip[0] = p1[0] + f * ( p2[0] - p1[0] );
		clip[1] = p1[1] + f * ( p2[1] - p1[1] );
		clip[2] = p1[2] + f * ( p2[2] - p1[2] );
	}

	return true;	// retain a fragment
}

/*
==================
R_AddClipSilEdges

Add sil edges for each triangle clipped to the side of
the frustum.

Only done for simple projected lights, not point lights.
==================
*/
static void R_AddClipSilEdges( void ) {
	int		v1, v2;
	int		v1_back, v2_back;
	int		i;

	// don't allow it to overflow
	if ( numShadowIndexes + numClipSilEdges * 6 > MAX_SHADOW_INDEXES ) {
		overflowed = true;
		return;
	}

	for ( i = 0; i < numClipSilEdges; i++ ) {
		v1 = clipSilEdges[i][0];
		v2 = clipSilEdges[i][1];
		v1_back = v1 + 1;
		v2_back = v2 + 1;
		if ( PointsOrdered( shadowVerts[ v1 ].ToVec3(), shadowVerts[ v2 ].ToVec3() ) ) {
			shadowIndexes[numShadowIndexes++] = v1;
			shadowIndexes[numShadowIndexes++] = v2;
			shadowIndexes[numShadowIndexes++] = v1_back;
			shadowIndexes[numShadowIndexes++] = v2;
			shadowIndexes[numShadowIndexes++] = v2_back;
			shadowIndexes[numShadowIndexes++] = v1_back;
		} else {
			shadowIndexes[numShadowIndexes++] = v1;
			shadowIndexes[numShadowIndexes++] = v2;
			shadowIndexes[numShadowIndexes++] = v2_back;
			shadowIndexes[numShadowIndexes++] = v1;
			shadowIndexes[numShadowIndexes++] = v2_back;
			shadowIndexes[numShadowIndexes++] = v1_back;
		}
	}
}

/*
=================
R_AddSilEdges

Add quads from the front points to the projected points
for each silhouette edge in the light
=================
*/
static void R_AddSilEdges( const surfTriangles_t *tri, unsigned short *pointCull, const anPlane frustum[6] ) {
	int		v1, v2;
	int		i;
	silEdge_t	*sil;
	int		numPlanes;

	numPlanes = tri->numIndexes / 3;

	// add sil edges for any true silhouette boundaries on the surface
	for ( i = 0; i < tri->numSilEdges; i++ ) {
		sil = tri->silEdges + i;
		if ( sil->p1 < 0 || sil->p1 > numPlanes || sil->p2 < 0 || sil->p2 > numPlanes ) {
			common->Error( "Bad sil planes" );
		}

		// an edge will be a silhouette edge if the face on one side
		// casts a shadow, but the face on the other side doesn't.
		// "casts a shadow" means that it has some surface in the projection,
		// not just that it has the correct facing direction
		// This will cause edges that are exactly on the frustum plane
		// to be considered sil edges if the face inside casts a shadow.
		if ( !( faceCastsShadow[ sil->p1 ] ^ faceCastsShadow[ sil->p2 ] ) ) {
			continue;
		}

		// if the edge is completely off the negative side of
		// a frustum plane, don't add it at all.  This can still
		// happen even if the face is visible and casting a shadow
		// if it is partially clipped
		if ( EDGE_CULLED( sil->v1, sil->v2 ) ) {
			continue;
		}

		// see if the edge needs to be clipped
		if ( EDGE_CLIPPED( sil->v1, sil->v2 ) ) {
			if ( numShadowVerts + 4 > MAX_SHADOW_VERTS ) {
				overflowed = true;
				return;
			}
			v1 = numShadowVerts;
			v2 = v1 + 2;
			if ( !R_ClipLineToLight( tri->verts[ sil->v1 ].xyz, tri->verts[ sil->v2 ].xyz,
				frustum, shadowVerts[v1].ToVec3(), shadowVerts[v2].ToVec3() ) ) {
				continue;	// clipped away
			}

			numShadowVerts += 4;
		} else {
			// use the entire edge
			v1 = remap[ sil->v1 ];
			v2 = remap[ sil->v2 ];
			if ( v1 < 0 || v2 < 0 ) {
				common->Error( "R_AddSilEdges: bad remap[]" );
			}
		}

		// don't overflow
		if ( numShadowIndexes + 6 > MAX_SHADOW_INDEXES ) {
			overflowed = true;
			return;
		}

		// we need to choose the correct way of triangulating the silhouette quad
		// consistantly between any two points, no matter which order they are specified.
		// If this wasn't done, slight rasterization cracks would show in the shadow
		// volume when two sil edges were exactly coincident
		if ( faceCastsShadow[ sil->p2 ] ) {
			if ( PointsOrdered( shadowVerts[ v1 ].ToVec3(), shadowVerts[ v2 ].ToVec3() ) ) {
				shadowIndexes[numShadowIndexes++] = v1;
				shadowIndexes[numShadowIndexes++] = v1+1;
				shadowIndexes[numShadowIndexes++] = v2;
				shadowIndexes[numShadowIndexes++] = v2;
				shadowIndexes[numShadowIndexes++] = v1+1;
				shadowIndexes[numShadowIndexes++] = v2+1;
			} else {
				shadowIndexes[numShadowIndexes++] = v1;
				shadowIndexes[numShadowIndexes++] = v2+1;
				shadowIndexes[numShadowIndexes++] = v2;
				shadowIndexes[numShadowIndexes++] = v1;
				shadowIndexes[numShadowIndexes++] = v1+1;
				shadowIndexes[numShadowIndexes++] = v2+1;
			}
		} else {
			if ( PointsOrdered( shadowVerts[ v1 ].ToVec3(), shadowVerts[ v2 ].ToVec3() ) ) {
				shadowIndexes[numShadowIndexes++] = v1;
				shadowIndexes[numShadowIndexes++] = v2;
				shadowIndexes[numShadowIndexes++] = v1+1;
				shadowIndexes[numShadowIndexes++] = v2;
				shadowIndexes[numShadowIndexes++] = v2+1;
				shadowIndexes[numShadowIndexes++] = v1+1;
			} else {
				shadowIndexes[numShadowIndexes++] = v1;
				shadowIndexes[numShadowIndexes++] = v2;
				shadowIndexes[numShadowIndexes++] = v2+1;
				shadowIndexes[numShadowIndexes++] = v1;
				shadowIndexes[numShadowIndexes++] = v2+1;
				shadowIndexes[numShadowIndexes++] = v1+1;
			}
		}
	}
}

/*
================
R_CalcPointCull

Also inits the remap[] array to all -1
================
*/
static void R_CalcPointCull( const surfTriangles_t *tri, const anPlane frustum[6], unsigned short *pointCull ) {
	int i;
	int frontBits;
	float *planeSide;
	byte *side1, *side2;

	SIMDProcessor->Memset( remap, -1, tri->numVerts * sizeof( remap[0] ) );

	for ( frontBits = 0, i = 0; i < 6; i++ ) {
		// get front bits for the whole surface
		if ( tri->bounds.PlaneDistance( frustum[i] ) >= LIGHT_CLIP_EPSILON ) {
			frontBits |= 1<<( i+6);
		}
	}

	// initialize point cull
	for ( i = 0; i < tri->numVerts; i++ ) {
		pointCull[i] = frontBits;
	}

	// if the surface is not completely inside the light frustum
	if ( frontBits == ( ( ( 1 << 6 ) - 1 ) ) << 6 ) {
		return;
	}

	planeSide = (float *) _alloca16( tri->numVerts * sizeof( float ) );
	side1 = ( byte * ) _alloca16( tri->numVerts * sizeof( byte ) );
	side2 = ( byte * ) _alloca16( tri->numVerts * sizeof( byte ) );
	SIMDProcessor->Memset( side1, 0, tri->numVerts * sizeof( byte ) );
	SIMDProcessor->Memset( side2, 0, tri->numVerts * sizeof( byte ) );

	for ( i = 0; i < 6; i++ ) {

		if ( frontBits & (1<<( i+6) ) ) {
			continue;
		}

		SIMDProcessor->Dot( planeSide, frustum[i], tri->verts, tri->numVerts );
		SIMDProcessor->CmpLT( side1, i, planeSide, LIGHT_CLIP_EPSILON, tri->numVerts );
		SIMDProcessor->CmpGT( side2, i, planeSide, -LIGHT_CLIP_EPSILON, tri->numVerts );
	}
	for ( i = 0; i < tri->numVerts; i++ ) {
		pointCull[i] |= side1[i] | (side2[i] << 6);
	}
}

/*
=================
R_CreateShadowVolumeInFrustum

Adds new verts and indexes to the shadow volume.

If the frustum completely defines the projected light,
makeClippedPlanes should be true, which will cause sil quads to
be added along all clipped edges.

If the frustum is just part of a point light, clipped planes don't
need to be added.
=================
*/
static void R_CreateShadowVolumeInFrustum( const anRenderEntityLocal *ent, const surfTriangles_t *tri, const anRenderLightsLocal *light, const anVec3 lightOrigin, const anPlane frustum[6], const anPlane &farPlane, bool makeClippedPlanes ) {
	int		i;
	int		numTris;
	unsigned short		*pointCull;
	int		numCapIndexes;
	int		firstShadowIndex;
	int		firstShadowVert;
	int		cullBits;

	pointCull = (unsigned short *)_alloca16( tri->numVerts * sizeof( pointCull[0] ) );

	// test the vertexes for inside the light frustum, which will allow
	// us to completely cull away some triangles from consideration.
	R_CalcPointCull( tri, frustum, pointCull );

	// this may not be the first frustum added to the volume
	firstShadowIndex = numShadowIndexes;
	firstShadowVert = numShadowVerts;

	// decide which triangles front shadow volumes, clipping as needed
	numClipSilEdges = 0;
	numTris = tri->numIndexes / 3;
	for ( i = 0; i < numTris; i++ ) {
		int		i1, i2, i3;

		faceCastsShadow[i] = 0;	// until shown otherwise

		// if it isn't facing the right way, don't add it
		// to the shadow volume
		if ( globalFacing[i] ) {
			continue;
		}

		i1 = tri->silIndexes[ i*3 + 0 ];
		i2 = tri->silIndexes[ i*3 + 1 ];
		i3 = tri->silIndexes[ i*3 + 2 ];

		// if all the verts are off one side of the frustum,
		// don't add any of them
		if ( TRIANGLE_CULLED( i1, i2, i3 ) ) {
			continue;
		}

		// make sure the verts that are not on the negative sides
		// of the frustum are copied over.
		// we need to get the original verts even from clipped triangles
		// so the edges reference correctly, because an edge may be unclipped
		// even when a triangle is clipped.
		if ( numShadowVerts + 6 > MAX_SHADOW_VERTS ) {
			overflowed = true;
			return;
		}

		if ( !POINT_CULLED(i1) && remap[i1] == -1 ) {
			remap[i1] = numShadowVerts;
			shadowVerts[ numShadowVerts ].ToVec3() = tri->verts[i1].xyz;
			numShadowVerts+=2;
		}
		if ( !POINT_CULLED(i2) && remap[i2] == -1 ) {
			remap[i2] = numShadowVerts;
			shadowVerts[ numShadowVerts ].ToVec3() = tri->verts[i2].xyz;
			numShadowVerts+=2;
		}
		if ( !POINT_CULLED(i3) && remap[i3] == -1 ) {
			remap[i3] = numShadowVerts;
			shadowVerts[ numShadowVerts ].ToVec3() = tri->verts[i3].xyz;
			numShadowVerts+=2;
		}

		// clip the triangle if any points are on the negative sides
		if ( TRIANGLE_CLIPPED( i1, i2, i3 ) ) {
			cullBits = ( ( pointCull[ i1 ] ^ 0xfc0 ) | ( pointCull[ i2 ] ^ 0xfc0 ) | ( pointCull[ i3 ] ^ 0xfc0 ) ) >> 6;
			// this will also define clip edges that will become
			// silhouette planes
			if ( R_ClipTriangleToLight( tri->verts[i1].xyz, tri->verts[i2].xyz,
				tri->verts[i3].xyz, cullBits, frustum ) ) {
				faceCastsShadow[i] = 1;
			}
		} else {
			// instead of overflowing or drawing a streamer shadow, don't draw a shadow at all
			if ( numShadowIndexes + 3 > MAX_SHADOW_INDEXES ) {
				overflowed = true;
				return;
			}
			if ( remap[i1] == -1 || remap[i2] == -1 || remap[i3] == -1 ) {
				common->Error( "R_CreateShadowVolumeInFrustum: bad remap[]" );
			}
			shadowIndexes[numShadowIndexes++] = remap[i3];
			shadowIndexes[numShadowIndexes++] = remap[i2];
			shadowIndexes[numShadowIndexes++] = remap[i1];
			faceCastsShadow[i] = 1;
		}
	}

	// add indexes for the back caps, which will just be reversals of the
	// front caps using the back vertexes
	numCapIndexes = numShadowIndexes - firstShadowIndex;

	// if no faces have been defined for the shadow volume,
	// there won't be anything at all
	if ( numCapIndexes == 0 ) {
		return;
	}

	//--------------- off-line processing ------------------
	// if we are running from dmap, perform the (very) expensive shadow optimizations
	// to remove internal sil edges and optimize the caps
	if ( callOptimizer ) {
		// project all of the vertexes to the shadow plane, generating
		// an equal number of back vertexes
		//R_ProjectPointsToFarPlane( ent, light, farPlane, firstShadowVert, numShadowVerts );
		optimizedShadow_t opt = SuperOptimizeOccluders( shadowVerts, shadowIndexes + firstShadowIndex, numCapIndexes, farPlane, lightOrigin );

		// pull off the non-optimized data
		numShadowIndexes = firstShadowIndex;
		numShadowVerts = firstShadowVert;

		// add the optimized data
		if ( numShadowIndexes + opt.totalIndexes > MAX_SHADOW_INDEXES
			|| numShadowVerts + opt.numVerts > MAX_SHADOW_VERTS ) {
			overflowed = true;
			common->Printf( "WARNING: overflowed MAX_SHADOW tables, shadow discarded\n" );
			Mem_Free( opt.verts );
			Mem_Free( opt.indexes );
			return;
		}

		for ( i = 0; i < opt.numVerts; i++ ) {
			shadowVerts[numShadowVerts+i][0] = opt.verts[i][0];
			shadowVerts[numShadowVerts+i][1] = opt.verts[i][1];
			shadowVerts[numShadowVerts+i][2] = opt.verts[i][2];
			shadowVerts[numShadowVerts+i][3] = 1;
		}
		for ( i = 0; i < opt.totalIndexes; i++ ) {
			int	index = opt.indexes[i];
			if ( index < 0 || index > opt.numVerts ) {
				common->Error( "optimized shadow index out of range" );
			}
			shadowIndexes[numShadowIndexes+i] = index + numShadowVerts;
		}

		numShadowVerts += opt.numVerts;
		numShadowIndexes += opt.totalIndexes;

		// note the index distribution so we can sort all the caps after all the sils
		indexRef[indexFrustumNumber].frontCapStart = firstShadowIndex;
		indexRef[indexFrustumNumber].rearCapStart = firstShadowIndex+opt.numFrontCapIndexes;
		indexRef[indexFrustumNumber].silStart = firstShadowIndex+opt.numFrontCapIndexes+opt.numRearCapIndexes;
		indexRef[indexFrustumNumber].end = numShadowIndexes;
		indexFrustumNumber++;

		Mem_Free( opt.verts );
		Mem_Free( opt.indexes );
		return;
	}

	//--------------- real-time processing ------------------
	// the dangling edge "face" is never considered to cast a shadow,
	// so any face with dangling edges that casts a shadow will have
	// it's dangling sil edge trigger a sil plane
	faceCastsShadow[numTris] = 0;

	// instead of overflowing or drawing a streamer shadow, don't draw a shadow at all
	// if we ran out of space
	if ( numShadowIndexes + numCapIndexes > MAX_SHADOW_INDEXES ) {
		overflowed = true;
		return;
	}
	for ( i = 0; i < numCapIndexes; i += 3 ) {
		shadowIndexes[ numShadowIndexes + i + 0 ] = shadowIndexes[ firstShadowIndex + i + 2 ] + 1;
		shadowIndexes[ numShadowIndexes + i + 1 ] = shadowIndexes[ firstShadowIndex + i + 1 ] + 1;
		shadowIndexes[ numShadowIndexes + i + 2 ] = shadowIndexes[ firstShadowIndex + i + 0 ] + 1;
	}
	numShadowIndexes += numCapIndexes;

	c_caps += numCapIndexes * 2;

	int preSilIndexes = numShadowIndexes;

	// if any triangles were clipped, we will have a list of edges
	// on the frustum which must now become sil edges
	if ( makeClippedPlanes ) {
		R_AddClipSilEdges();
	}

	// any edges that are a transition between a shadowing and
	// non-shadowing triangle will cast a silhouette edge
	R_AddSilEdges( tri, pointCull, frustum );

	c_sils += numShadowIndexes - preSilIndexes;

	// project all of the vertexes to the shadow plane, generating
	// an equal number of back vertexes
	R_ProjectPointsToFarPlane( ent, light, farPlane, firstShadowVert, numShadowVerts );

	// note the index distribution so we can sort all the caps after all the sils
	indexRef[indexFrustumNumber].frontCapStart = firstShadowIndex;
	indexRef[indexFrustumNumber].rearCapStart = firstShadowIndex+numCapIndexes;
	indexRef[indexFrustumNumber].silStart = preSilIndexes;
	indexRef[indexFrustumNumber].end = numShadowIndexes;
	indexFrustumNumber++;
}

/*
===================
R_MakeShadowFrustums

Called at definition derivation time
===================
*/
void R_MakeShadowFrustums( anRenderLightsLocal *light ) {
	int		i, j;

	if ( light->parms.pointLight ) {
#if 0
		anVec3	adjustedRadius;

		// increase the light radius to cover any origin offsets.
		// this will cause some shadows to extend out of the exact light
		// volume, but is simpler than adjusting all the frustums
		adjustedRadius[0] = light->parms.lightRadius[0] + anMath::Fabs( light->parms.lightCenter[0] );
		adjustedRadius[1] = light->parms.lightRadius[1] + anMath::Fabs( light->parms.lightCenter[1] );
		adjustedRadius[2] = light->parms.lightRadius[2] + anMath::Fabs( light->parms.lightCenter[2] );

		light->numShadowFrustums = 0;
		// a point light has to project against six planes
		for ( i = 0; i < 6; i++ ) {
			shadowFrustum_t	*frust = &light->shadowFrustums[ light->numShadowFrustums ];

			frust->numPlanes = 6;
			frust->makeClippedPlanes = false;
			for ( j = 0; j < 6; j++ ) {
				anPlane &plane = frust->planes[j];
				plane[0] = pointLightFrustums[i][j][0] / adjustedRadius[0];
				plane[1] = pointLightFrustums[i][j][1] / adjustedRadius[1];
				plane[2] = pointLightFrustums[i][j][2] / adjustedRadius[2];
				plane.Normalize();
				plane[3] = -( plane.Normal() * light->globalLightOrigin );
				if ( j == 5 ) {
					plane[3] += adjustedRadius[i>>1];
				}
			}

			light->numShadowFrustums++;
		}
#else
		// exact projection,taking into account asymetric frustums when
		// globalLightOrigin isn't centered
		static int	faceCorners[6][4] = {
			{ 7, 5, 1, 3 },		// positive X side
			{ 4, 6, 2, 0 },		// negative X side
			{ 6, 7, 3, 2 },		// positive Y side
			{ 5, 4, 0, 1 },		// negative Y side
			{ 6, 4, 5, 7 },		// positive Z side
			{ 3, 1, 0, 2 }		// negative Z side
		};
		static int	faceEdgeAdjacent[6][4] = {
			{ 4, 4, 2, 2 },		// positive X side
			{ 7, 7, 1, 1 },		// negative X side
			{ 5, 5, 0, 0 },		// positive Y side
			{ 6, 6, 3, 3 },		// negative Y side
			{ 0, 0, 3, 3 },		// positive Z side
			{ 5, 5, 6, 6 }		// negative Z side
		};

		bool	centerOutside = false;

		// if the light center of projection is outside the light bounds,
		// we will need to build the planes a little differently
		if ( fabs( light->parms.lightCenter[0] ) > light->parms.lightRadius[0]
			|| fabs( light->parms.lightCenter[1] ) > light->parms.lightRadius[1]
			|| fabs( light->parms.lightCenter[2] ) > light->parms.lightRadius[2] ) {
			centerOutside = true;
		}

		// make the corners
		anVec3	corners[8];

		for ( i = 0; i < 8; i++ ) {
			anVec3	temp;
			for ( j = 0; j < 3; j++ ) {
				if ( i & ( 1 << j ) ) {
					temp[j] = light->parms.lightRadius[j];
				} else {
					temp[j] = -light->parms.lightRadius[j];
				}
			}

			// transform to global space
			corners[i] = light->parms.origin + light->parms.axis * temp;
		}

		light->numShadowFrustums = 0;
		for ( int side = 0; side < 6; side++ ) {
			shadowFrustum_t	*frust = &light->shadowFrustums[ light->numShadowFrustums ];
			anVec3 &p1 = corners[faceCorners[side][0]];
			anVec3 &p2 = corners[faceCorners[side][1]];
			anVec3 &p3 = corners[faceCorners[side][2]];
			anPlane backPlane;

			// plane will have positive side inward
			backPlane.FromPoints( p1, p2, p3 );

			// if center of projection is on the wrong side, skip
			float d = backPlane.Distance( light->globalLightOrigin );
			if ( d < 0 ) {
				continue;
			}

			frust->numPlanes = 6;
			frust->planes[5] = backPlane;
			frust->planes[4] = backPlane;	// we don't really need the extra plane

			// make planes with positive side facing inwards in light local coordinates
			for ( int edge = 0; edge < 4; edge++ ) {
				anVec3 &p1 = corners[faceCorners[side][edge]];
				anVec3 &p2 = corners[faceCorners[side][(edge+1 )&3]];

				// create a plane that goes through the center of projection
				frust->planes[edge].FromPoints( p2, p1, light->globalLightOrigin );

				// see if we should use an adjacent plane instead
				if ( centerOutside ) {
					anVec3 &p3 = corners[faceEdgeAdjacent[side][edge]];
					anPlane sidePlane;

					sidePlane.FromPoints( p2, p1, p3 );
					d = sidePlane.Distance( light->globalLightOrigin );
					if ( d < 0 ) {
						// use this plane instead of the edged plane
						frust->planes[edge] = sidePlane;
					}
					// we can't guarantee a neighbor, so add sill planes at edge
					light->shadowFrustums[ light->numShadowFrustums ].makeClippedPlanes = true;
				}
			}
			light->numShadowFrustums++;
		}

#endif
		return;
	}

	// projected light
	light->numShadowFrustums = 1;
	shadowFrustum_t	*frust = &light->shadowFrustums[ 0 ];

	// flip and transform the frustum planes so the positive side faces
	// inward in local coordinates

	// it is important to clip against even the near clip plane, because
	// many projected lights that are faking area lights will have their
	// origin behind solid surfaces.
	for ( i = 0; i < 6; i++ ) {
		anPlane &plane = frust->planes[i];

		plane.SetNormal( -light->frustum[i].Normal() );
		plane.SetDist( -light->frustum[i].Dist() );
	}

	frust->numPlanes = 6;

	frust->makeClippedPlanes = true;
	// projected lights don't have shared frustums, so any clipped edges
	// right on the planes must have a sil plane created for them
}

/*
=================
R_CreateShadowVolume

The returned surface will have a valid bounds and radius for culling.

Triangles are clipped to the light frustum before projecting.

A single triangle can clip to as many as 7 vertexes, so
the worst case expansion is 2*(numindexes/3)*7 verts when counting both
the front and back caps, although it will usually only be a modest
increase in vertexes for closed modesl

The worst case index count is much larger, when the 7 vertex clipped triangle
needs 15 indexes for the front, 15 for the back, and 42 (a quad on seven sides)
for the sides, for a total of 72 indexes from the original 3.  Ouch.

NULL may be returned if the surface doesn't create a shadow volume at all,
as with a single face that the light is behind.

If an edge is within an epsilon of the border of the volume, it must be treated
as if it is clipped for triangles, generating a new sil edge, and act
as if it was culled for edges, because the sil edge will have been
generated by the triangle irregardless of if it actually was a sil edge.
=================
*/
surfTriangles_t *R_CreateShadowVolume( const anRenderEntityLocal *ent, const surfTriangles_t *tri, const anRenderLightsLocal *light, shadowGen_t optimize, srfCullInfo_t &cullInfo ) {
	int		i, j;
	anVec3	lightOrigin;
	surfTriangles_t	*newTri;
	int		capPlaneBits;

	if ( !r_shadows.GetBool() ) {
		return NULL;
	}

	if ( tri->numSilEdges == 0 || tri->numIndexes == 0 || tri->numVerts == 0 ) {
		return NULL;
	}

	if ( tri->numIndexes < 0 ) {
		common->Error( "R_CreateShadowVolume: tri->numIndexes = %i", tri->numIndexes );
	}

	if ( tri->numVerts < 0 ) {
		common->Error( "R_CreateShadowVolume: tri->numVerts = %i", tri->numVerts );
	}

	tr.pc.c_createShadowVolumes++;

	// use the fast infinite projection in dynamic situations, which
	// trades somewhat more overdraw and no cap optimizations for
	// a very simple generation process
	if ( optimize == SG_DYNAMIC && r_useTurboShadow.GetBool() ) {
		if ( tr.backEndRendererHasVertexPrograms && r_useShadowVertexProgram.GetBool() ) {
			return R_CreateVertexProgramTurboShadowVolume( ent, tri, light, cullInfo );
		} else {
			return R_CreateTurboShadowVolume( ent, tri, light, cullInfo );
		}
	}

	R_CalcInteractionFacing( ent, tri, light, cullInfo );

	int numFaces = tri->numIndexes / 3;
	int allFront = 1;
	for ( i = 0; i < numFaces && allFront; i++ ) {
		allFront &= cullInfo.facing[i];
	}
	if ( allFront ) {
		// if no faces are the right direction, don't make a shadow at all
		return NULL;
	}

	// clear the shadow volume
	numShadowIndexes = 0;
	numShadowVerts = 0;
	overflowed = false;
	indexFrustumNumber = 0;
	capPlaneBits = 0;
	callOptimizer = (optimize == SG_OFFLINE);

	// the facing information will be the same for all six projections
	// from a point light, as well as for any directed lights
	globalFacing = cullInfo.facing;
	faceCastsShadow = ( byte * )_alloca16( tri->numIndexes / 3 + 1 );	// + 1 for fake dangling edge face
	remap = ( int * )_alloca16( tri->numVerts * sizeof( remap[0] ) );

	R_GlobalPointToLocal( ent->modelMatrix, light->globalLightOrigin, lightOrigin );

	// run through all the shadow frustums, which is one for a projected light,
	// and usually six for a point light, but point lights with centers outside
	// the box may have less
	for ( int frustumNum = 0; frustumNum < light->numShadowFrustums; frustumNum++ ) {
		const shadowFrustum_t	*frust = &light->shadowFrustums[frustumNum];
		ALIGN16( anPlane frustum[6] );

		// transform the planes into entity space
		// we could share and reverse some of the planes between frustums for a minor
		// speed increase

		// the cull test is redundant for a single shadow frustum projected light, because
		// the surface has already been checked against the main light frustums
		for ( j = 0; j < frust->numPlanes; j++ ) {
			R_GlobalPlaneToLocal( ent->modelMatrix, frust->planes[j], frustum[j] );

			// try to cull the entire surface against this frustum
			float d = tri->bounds.PlaneDistance( frustum[j] );
			if ( d < -LIGHT_CLIP_EPSILON ) {
				break;
			}
		}
		if ( j != frust->numPlanes ) {
			continue;
		}
		// we need to check all the triangles
		int		oldFrustumNumber = indexFrustumNumber;

		R_CreateShadowVolumeInFrustum( ent, tri, light, lightOrigin, frustum, frustum[5], frust->makeClippedPlanes );

		// if we couldn't make a complete shadow volume, it is better to
		// not draw one at all, avoiding streamer problems
		if ( overflowed ) {
			return NULL;
		}

		if ( indexFrustumNumber != oldFrustumNumber ) {
			// note that we have caps projected against this frustum,
			// which may allow us to skip drawing the caps if all projected
			// planes face away from the viewer and the viewer is outside the light volume
			capPlaneBits |= 1<<frustumNum;
		}
	}

	// if no faces have been defined for the shadow volume,
	// there won't be anything at all
	if ( numShadowIndexes == 0 ) {
		return NULL;
	}

	// this should have been prevented by the overflowed flag, so if it ever happens,
	// it is a code error
	if ( numShadowVerts > MAX_SHADOW_VERTS || numShadowIndexes > MAX_SHADOW_INDEXES ) {
		common->FatalError( "Shadow volume exceeded allocation" );
	}

	// allocate a new surface for the shadow volume
	newTri = R_AllocStaticTriSurf();

	// we might consider setting this, but it would only help for
	// large lights that are partially off screen
	newTri->bounds.Clear();

	// copy off the verts and indexes
	newTri->numVerts = numShadowVerts;
	newTri->numIndexes = numShadowIndexes;

	// the shadow verts will go into a main memory buffer as well as a vertex
	// cache buffer, so they can be copied back if they are purged
	R_AllocStaticTriSurfShadowVerts( newTri, newTri->numVerts );
	SIMDProcessor->Memcpy( newTri->shadowVertexes, shadowVerts, newTri->numVerts * sizeof( newTri->shadowVertexes[0] ) );

	R_AllocStaticTriSurfIndexes( newTri, newTri->numIndexes );

	if ( 1 /* sortCapIndexes */ ) {
		newTri->shadowCapPlaneBits = capPlaneBits;

		// copy the sil indexes first
		newTri->numShadowIndexesNoCaps = 0;
		for ( i = 0; i < indexFrustumNumber; i++ ) {
			int	c = indexRef[i].end - indexRef[i].silStart;
			SIMDProcessor->Memcpy( newTri->indexes+newTri->numShadowIndexesNoCaps,
									shadowIndexes+indexRef[i].silStart, c * sizeof( newTri->indexes[0] ) );
			newTri->numShadowIndexesNoCaps += c;
		}
		// copy rear cap indexes next
		newTri->numShadowIndexesNoFrontCaps = newTri->numShadowIndexesNoCaps;
		for ( i = 0; i < indexFrustumNumber; i++ ) {
			int	c = indexRef[i].silStart - indexRef[i].rearCapStart;
			SIMDProcessor->Memcpy( newTri->indexes+newTri->numShadowIndexesNoFrontCaps,
									shadowIndexes+indexRef[i].rearCapStart, c * sizeof( newTri->indexes[0] ) );
			newTri->numShadowIndexesNoFrontCaps += c;
		}
		// copy front cap indexes last
		newTri->numIndexes = newTri->numShadowIndexesNoFrontCaps;
		for ( i = 0; i < indexFrustumNumber; i++ ) {
			int	c = indexRef[i].rearCapStart - indexRef[i].frontCapStart;
			SIMDProcessor->Memcpy( newTri->indexes+newTri->numIndexes,
									shadowIndexes+indexRef[i].frontCapStart, c * sizeof( newTri->indexes[0] ) );
			newTri->numIndexes += c;
		}
	} else {
		newTri->shadowCapPlaneBits = 63;	// we don't have optimized index lists
		SIMDProcessor->Memcpy( newTri->indexes, shadowIndexes, newTri->numIndexes * sizeof( newTri->indexes[0] ) );
	}

	if ( optimize == SG_OFFLINE ) {
		CleanupOptimizedShadowTris( newTri );
	}

	return newTri;
}
