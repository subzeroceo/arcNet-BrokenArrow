#include "../precompiled.h"
#pragma hdrstop

#define POLYTOPE_VERTEX_EPSILON		0.1f


/*
====================
arcSurface_Polytope::FromPlanes
====================
*/
void arcSurface_Polytope::FromPlanes( const arcPlane *planes, const int numPlanes ) {
	arcDrawVert newVert;

	int *windingVerts = ( int * ) _alloca( MAX_POINTS_ON_WINDING * sizeof( int ) );
	memset( &newVert, 0, sizeof( newVert ) );

	for ( int i = 0; i < numPlanes; i++ ) {
		arcFixedWinding w.BaseForPlane( planes[i] );
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
arcSurface_Polytope::SetupTetrahedron
====================
*/
void arcSurface_Polytope::SetupTetrahedron( const arcBounds &bounds ) {
	float c1 = 0.4714045207f;
	float c2 = 0.8164965809f;
	float c3 = -0.3333333333f;

	arcVec3 center = bounds.GetCenter();
	arcVec3 scale = bounds[1] - center;

	verts.SetNum( 4 );
	verts[0].xyz = center + arcVec3( 0.0f, 0.0f, scale.z );
	verts[1].xyz = center + arcVec3( 2.0f * c1 * scale.x, 0.0f, c3 * scale.z );
	verts[2].xyz = center + arcVec3( -c1 * scale.x, c2 * scale.y, c3 * scale.z );
	verts[3].xyz = center + arcVec3( -c1 * scale.x, -c2 * scale.y, c3 * scale.z );

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
arcSurface_Polytope::SetupHexahedron
====================
*/
void arcSurface_Polytope::SetupHexahedron( const arcBounds &bounds ) {
	arcVec3 center = bounds.GetCenter();
	arcVec3 scale = bounds[1] - center;

	verts.SetNum( 8 );
	verts[0].xyz = center + arcVec3( -scale.x, -scale.y, -scale.z );
	verts[1].xyz = center + arcVec3(  scale.x, -scale.y, -scale.z );
	verts[2].xyz = center + arcVec3(  scale.x,  scale.y, -scale.z );
	verts[3].xyz = center + arcVec3( -scale.x,  scale.y, -scale.z );
	verts[4].xyz = center + arcVec3( -scale.x, -scale.y,  scale.z );
	verts[5].xyz = center + arcVec3(  scale.x, -scale.y,  scale.z );
	verts[6].xyz = center + arcVec3(  scale.x,  scale.y,  scale.z );
	verts[7].xyz = center + arcVec3( -scale.x,  scale.y,  scale.z );

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
arcSurface_Polytope::SetupOctahedron
====================
*/
void arcSurface_Polytope::SetupOctahedron( const arcBounds &bounds ) {
	arcVec3 center = bounds.GetCenter();
	arcVec3 scale = bounds[1] - center;

	verts.SetNum( 6 );
	verts[0].xyz = center + arcVec3(  scale.x, 0.0f, 0.0f );
	verts[1].xyz = center + arcVec3( -scale.x, 0.0f, 0.0f );
	verts[2].xyz = center + arcVec3( 0.0f,  scale.y, 0.0f );
	verts[3].xyz = center + arcVec3( 0.0f, -scale.y, 0.0f );
	verts[4].xyz = center + arcVec3( 0.0f, 0.0f,  scale.z );
	verts[5].xyz = center + arcVec3( 0.0f, 0.0f, -scale.z );

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
arcSurface_Polytope::SetupDodecahedron
====================
*/
void arcSurface_Polytope::SetupDodecahedron( const arcBounds &bounds ) {
}

/*
====================
arcSurface_Polytope::SetupIcosahedron
====================
*/
void arcSurface_Polytope::SetupIcosahedron( const arcBounds &bounds ) {
}

/*
====================
arcSurface_Polytope::SetupCylinder
====================
*/
void arcSurface_Polytope::SetupCylinder( const arcBounds &bounds, const int numSides ) {
}

/*
====================
arcSurface_Polytope::SetupCone
====================
*/
void arcSurface_Polytope::SetupCone( const arcBounds &bounds, const int numSides ) {
}

/*
====================
arcSurface_Polytope::SplitPolytope
====================
*/
int arcSurface_Polytope::SplitPolytope( const arcPlane &plane, const float epsilon, arcSurface_Polytope **front, arcSurface_Polytope **back ) const {
	arcSurface *surface[2];
	arcSurface_Polytope *polytopeSurfaces[2], *surf;
	int *onPlaneEdges[2];

	onPlaneEdges[0] = ( int * ) _alloca( indexes.Num() / 3 * sizeof( int ) );
	onPlaneEdges[1] = ( int * ) _alloca( indexes.Num() / 3 * sizeof( int ) );

	int side = Split( plane, epsilon, &surface[0], &surface[1], onPlaneEdges[0], onPlaneEdges[1] );

	*front = polytopeSurfaces[0] = new arcSurface_Polytope;
	*back = polytopeSurfaces[1] = new arcSurface_Polytope;

	for ( int s = 0; s < 2; s++ ) {
		if ( surface[s] ) {
			polytopeSurfaces[s] = new arcSurface_Polytope;
			polytopeSurfaces[s]->SwapTriangles( *surface[s] );
			delete surface[s];
			surface[s] = NULL;
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
					idSwap( onPlaneEdges[s][i], onPlaneEdges[s][j] );
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
