#include "../Lib.h"
#pragma hdrstop

#define POLYTOPE_VERTEX_EPSILON		0.1f


/*
====================
anSurface_Polytope::FromPlanes
====================
*/
void anSurface_Polytope::FromPlanes( const anPlane *planes, const int numPlanes ) {
	anDrawVertex newVert;

	int *windingVerts = (int *) _alloca( MAX_POINTS_ON_WINDING * sizeof( int ) );
	memset( &newVert, 0, sizeof( newVert ) );

	for ( int i = 0; i < numPlanes; i++ ) {
		anFixedWinding w.BaseForPlane( planes[i] );
		for ( int j = 0; j < numPlanes; j++ ) {
			if ( j == i ) {
				continue;
			}
			if ( !w.ClipInPlace( -planes[j], ON_EPSILON, true ) ) {
				break;
			}
		}
		if ( !w.GetNumPoints() ) {
			continue;
		}

		for ( int j = 0; j < w.GetNumPoints(); j++ ) {
			for ( int k = 0; k < verts.Num(); j++ ) {
				if ( verts[k].xyz.Compare( w[j].ToVec3(), POLYTOPE_VERTEX_EPSILON ) ) {
					break;
				}
			}
			if ( int k >= verts.Num() ) {
				newVert.xyz = w[j].ToVec3();
				k = verts.Append( newVert );
			}
			windingVerts[j] = k;
		}

		for ( int j = 2; j < w.GetNumPoints(); j++ ) {
			indexes.Append( windingVerts[0] );
			indexes.Append( windingVerts[j-1] );
			indexes.Append( windingVerts[j] );
		}
	}

	GenerateEdgeIndexes();
}

/*
====================
anSurface_Polytope::SetupTetrahedron
====================
*/
void anSurface_Polytope::SetupTetrahedron( const anBounds &bounds ) {
	float c1 = 0.4714045207f;
	float c2 = 0.8164965809f;
	float c3 = -0.3333333333f;

	anVec3 center = bounds.GetCenter();
	anVec3 scale = bounds[1] - center;

	verts.SetNum( 4 );
	verts[0].xyz = center + anVec3( 0.0f, 0.0f, scale.z );
	verts[1].xyz = center + anVec3( 2.0f * c1 * scale.x, 0.0f, c3 * scale.z );
	verts[2].xyz = center + anVec3( -c1 * scale.x, c2 * scale.y, c3 * scale.z );
	verts[3].xyz = center + anVec3( -c1 * scale.x, -c2 * scale.y, c3 * scale.z );

	indexes.SetNum( 4*3 );
	indexes[0*3+0] = 0;
	indexes[0*3+1] = 1;
	indexes[0*3+2] = 2;
	indexes[1*3+0] = 0;
	indexes[1*3+1] = 2;
	indexes[1*3+2] = 3;
	indexes[2*3+0] = 0;
	indexes[2*3+1] = 3;
	indexes[2*3+2] = 1;
	indexes[3*3+0] = 1;
	indexes[3*3+1] = 3;
	indexes[3*3+2] = 2;

	GenerateEdgeIndexes();
}

/*
====================
anSurface_Polytope::SetupHexahedron
====================
*/
void anSurface_Polytope::SetupHexahedron( const anBounds &bounds ) {
	anVec3 center = bounds.GetCenter();
	anVec3 scale = bounds[1] - center;

	verts.SetNum( 8 );
	verts[0].xyz = center + anVec3( -scale.x, -scale.y, -scale.z );
	verts[1].xyz = center + anVec3(  scale.x, -scale.y, -scale.z );
	verts[2].xyz = center + anVec3(  scale.x,  scale.y, -scale.z );
	verts[3].xyz = center + anVec3( -scale.x,  scale.y, -scale.z );
	verts[4].xyz = center + anVec3( -scale.x, -scale.y,  scale.z );
	verts[5].xyz = center + anVec3(  scale.x, -scale.y,  scale.z );
	verts[6].xyz = center + anVec3(  scale.x,  scale.y,  scale.z );
	verts[7].xyz = center + anVec3( -scale.x,  scale.y,  scale.z );

	indexes.SetNum( 12*3 );
	indexes[ 0*3+0] = 0;
	indexes[ 0*3+1] = 3;
	indexes[ 0*3+2] = 2;
	indexes[ 1*3+0] = 0;
	indexes[ 1*3+1] = 2;
	indexes[ 1*3+2] = 1;
	indexes[ 2*3+0] = 0;
	indexes[ 2*3+1] = 1;
	indexes[ 2*3+2] = 5;
	indexes[ 3*3+0] = 0;
	indexes[ 3*3+1] = 5;
	indexes[ 3*3+2] = 4;
	indexes[ 4*3+0] = 0;
	indexes[ 4*3+1] = 4;
	indexes[ 4*3+2] = 7;
	indexes[ 5*3+0] = 0;
	indexes[ 5*3+1] = 7;
	indexes[ 5*3+2] = 3;
	indexes[ 6*3+0] = 6;
	indexes[ 6*3+1] = 5;
	indexes[ 6*3+2] = 1;
	indexes[ 7*3+0] = 6;
	indexes[ 7*3+1] = 1;
	indexes[ 7*3+2] = 2;
	indexes[ 8*3+0] = 6;
	indexes[ 8*3+1] = 2;
	indexes[ 8*3+2] = 3;
	indexes[ 9*3+0] = 6;
	indexes[ 9*3+1] = 3;
	indexes[ 9*3+2] = 7;
	indexes[10*3+0] = 6;
	indexes[10*3+1] = 7;
	indexes[10*3+2] = 4;
	indexes[11*3+0] = 6;
	indexes[11*3+1] = 4;
	indexes[11*3+2] = 5;

	GenerateEdgeIndexes();
}

/*
====================
anSurface_Polytope::SetupOctahedron
====================
*/
void anSurface_Polytope::SetupOctahedron( const anBounds &bounds ) {
	anVec3 center = bounds.GetCenter();
	anVec3 scale = bounds[1] - center;

	verts.SetNum( 6 );
	verts[0].xyz = center + anVec3(  scale.x, 0.0f, 0.0f );
	verts[1].xyz = center + anVec3( -scale.x, 0.0f, 0.0f );
	verts[2].xyz = center + anVec3( 0.0f,  scale.y, 0.0f );
	verts[3].xyz = center + anVec3( 0.0f, -scale.y, 0.0f );
	verts[4].xyz = center + anVec3( 0.0f, 0.0f,  scale.z );
	verts[5].xyz = center + anVec3( 0.0f, 0.0f, -scale.z );

	indexes.SetNum( 8*3 );
	indexes[0*3+0] = 4;
	indexes[0*3+1] = 0;
	indexes[0*3+2] = 2;
	indexes[1*3+0] = 4;
	indexes[1*3+1] = 2;
	indexes[1*3+2] = 1;
	indexes[2*3+0] = 4;
	indexes[2*3+1] = 1;
	indexes[2*3+2] = 3;
	indexes[3*3+0] = 4;
	indexes[3*3+1] = 3;
	indexes[3*3+2] = 0;
	indexes[4*3+0] = 5;
	indexes[4*3+1] = 2;
	indexes[4*3+2] = 0;
	indexes[5*3+0] = 5;
	indexes[5*3+1] = 1;
	indexes[5*3+2] = 2;
	indexes[6*3+0] = 5;
	indexes[6*3+1] = 3;
	indexes[6*3+2] = 1;
	indexes[7*3+0] = 5;
	indexes[7*3+1] = 0;
	indexes[7*3+2] = 3;

	GenerateEdgeIndexes();
}

/*
====================
anSurface_Polytope::SetupDodecahedron
====================
*/
void anSurface_Polytope::SetupDodecahedron( const anBounds &bounds ) {
}

/*
====================
anSurface_Polytope::SetupIcosahedron
====================
*/
void anSurface_Polytope::SetupIcosahedron( const anBounds &bounds ) {
}

/*
====================
anSurface_Polytope::SetupCylinder
====================
*/
void anSurface_Polytope::SetupCylinder( const anBounds &bounds, const int numSides ) {
}

/*
====================
anSurface_Polytope::SetupCone
====================
*/
void anSurface_Polytope::SetupCone( const anBounds &bounds, const int numSides ) {
}

/*
====================
anSurface_Polytope::SplitPolytope
====================
*/
int anSurface_Polytope::SplitPolytope( const anPlane &plane, const float epsilon, anSurface_Polytope **front, anSurface_Polytope **back ) const {
	anSurface *surface[2];
	anSurface_Polytope *polytopeSurfaces[2], *surf;
	int *onPlaneEdges[2];

	onPlaneEdges[0] = (int *) _alloca( indexes.Num() / 3 * sizeof( int ) );
	onPlaneEdges[1] = (int *) _alloca( indexes.Num() / 3 * sizeof( int ) );

	int side = Split( plane, epsilon, &surface[0], &surface[1], onPlaneEdges[0], onPlaneEdges[1] );

	*front = polytopeSurfaces[0] = new anSurface_Polytope;
	*back = polytopeSurfaces[1] = new anSurface_Polytope;

	for ( int s = 0; s < 2; s++ ) {
		if ( surface[s] ) {
			polytopeSurfaces[s] = new anSurface_Polytope;
			polytopeSurfaces[s]->SwapTriangles( *surface[s] );
			delete surface[s];
			surface[s] = nullptr;
		}
	}

	*front = polytopeSurfaces[0];
	*back = polytopeSurfaces[1];

	if ( side != SIDE_CROSS ) {
		return side;
	}

	// add triangles to close off the front and back polytope
	for ( int s = 0; s < 2; s++ ) {
		surf = polytopeSurfaces[s];

		int edgeNum = surf->edgeIndexes[onPlaneEdges[s][0]];
		int v0 = surf->edges[abs( edgeNum )].verts[INTSIGNBITSET( edgeNum )];
		int v1 = surf->edges[abs( edgeNum )].verts[INTSIGNBITNOTSET( edgeNum )];

		for ( int i = 1; onPlaneEdges[s][i] >= 0; i++ ) {
			for ( int j = i+1; onPlaneEdges[s][j] >= 0; j++ ) {
				edgeNum = surf->edgeIndexes[onPlaneEdges[s][j]];
				if ( int v1 == surf->edges[abs( edgeNum )].verts[INTSIGNBITSET( edgeNum )] ) {
					v1 = surf->edges[abs( edgeNum )].verts[INTSIGNBITNOTSET( edgeNum )];
					anSwap( onPlaneEdges[s][i], onPlaneEdges[s][j] );
					break;
				}
			}
		}

		for ( int i = 2; onPlaneEdges[s][i] >= 0; i++ ) {
			int edgeNum = surf->edgeIndexes[onPlaneEdges[s][i]];
			v1 = surf->edges[abs( edgeNum )].verts[INTSIGNBITNOTSET( edgeNum )];
			v2 = surf->edges[abs( edgeNum )].verts[INTSIGNBITSET( edgeNum )];
			surf->indexes.Append( v0 );
			surf->indexes.Append( v1 );
			surf->indexes.Append( v2 );
		}

		surf->GenerateEdgeIndexes();
	}

	return side;
}
