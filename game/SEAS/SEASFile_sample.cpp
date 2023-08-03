#pragma hdrstop
#include "../idlib/Lib.h"


#include "SEASFile.h"
#include "SEASFile_local.h"


//===============================================================
//
//	Environment Sampling
//
//===============================================================

/*
================
anSEASFileLocal::EdgeCenter
================
*/
anVec3 anSEASFileLocal::EdgeCenter( int edgeNum ) const {
	const seasEdge_t *edge;
	edge = &edges[edgeNum];
	return ( vertices[edge->vertexNum[0]] + vertices[edge->vertexNum[1]] ) * 0.5f;
}

/*
================
anSEASFileLocal::FaceCenter
================
*/
anVec3 anSEASFileLocal::FaceCenter( int faceNum ) const {
	int i, edgeNum;
	const seasFace_t *face;
	const seasEdge_t *edge;
	anVec3 center;

	center = vec3_origin;

	face = &faces[faceNum];
	if ( face->numEdges > 0 ) {
		for ( i = 0; i < face->numEdges; i++ ) {
			edgeNum = edgeIndex[ face->firstEdge + i ];
			edge = &edges[ abs( edgeNum ) ];
			center += vertices[ edge->vertexNum[ INT32_SIGNBITSET(edgeNum) ] ];
		}
		center /= face->numEdges;
	}
	return center;
}

/*
================
anSEASFileLocal::AreaCenter
================
*/
anVec3 anSEASFileLocal::AreaCenter( int areaNum ) const {
	int i, faceNum;
	const seasArea_t *area;
	anVec3 center;

	center = vec3_origin;

	area = &areas[areaNum];
	if ( area->numFaces > 0 ) {
		for ( i = 0; i < area->numFaces; i++ ) {
			faceNum = faceIndex[area->firstFace + i];
			center += FaceCenter( abs(faceNum) );
		}
		center /= area->numFaces;
	}
	return center;
}

/*
============
anSEASFileLocal::AreaReachableGoal
============
*/
anVec3 anSEASFileLocal::AreaReachableGoal( int areaNum ) const {
	int i, faceNum, numFaces;
	const seasArea_t *area;
	anVec3 center;
	anVec3 start, end;
	seasTrace_t trace;

	area = &areas[areaNum];

	if ( !(area->flags & (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY)) || (area->flags & AREA_LIQUID) ) {
		return AreaCenter( areaNum );
	}

	center = vec3_origin;

	numFaces = 0;
	for ( i = 0; i < area->numFaces; i++ ) {
		faceNum = faceIndex[area->firstFace + i];
		if ( !(faces[abs(faceNum)].flags & FACE_FLOOR) ) {
			continue;
		}
		center += FaceCenter( abs(faceNum) );
		numFaces++;
	}
	if ( numFaces > 0 ) {
		center /= numFaces;
	}
	center[2] += 1.0f;
	end = center;
	end[2] -= 1024;
	Trace( trace, center, end );

	return trace.endpos;
}

/*
================
anSEASFileLocal::EdgeBounds
================
*/
anBounds anSEASFileLocal::EdgeBounds( int edgeNum ) const {
	const seasEdge_t *edge;
	anBounds bounds;

	edge = &edges[ abs( edgeNum ) ];
	bounds[0] = bounds[1] = vertices[ edge->vertexNum[0] ];
	bounds += vertices[ edge->vertexNum[1] ];
	return bounds;
}

/*
================
anSEASFileLocal::FaceBounds
================
*/
anBounds anSEASFileLocal::FaceBounds( int faceNum ) const {
	int i, edgeNum;
	const seasFace_t *face;
	const seasEdge_t *edge;
	anBounds bounds;

	face = &faces[faceNum];
	bounds.Clear();

	for ( i = 0; i < face->numEdges; i++ ) {
		edgeNum = edgeIndex[ face->firstEdge + i ];
		edge = &edges[ abs( edgeNum ) ];
		bounds.AddPoint( vertices[ edge->vertexNum[ INT32_SIGNBITSET(edgeNum) ] ] );
	}
	return bounds;
}

/*
================
anSEASFileLocal::AreaBounds
================
*/
anBounds anSEASFileLocal::AreaBounds( int areaNum ) const {
	int i, faceNum;
	const seasArea_t *area;
	anBounds bounds;

	area = &areas[areaNum];
	bounds.Clear();

	for ( i = 0; i < area->numFaces; i++ ) {
		faceNum = faceIndex[area->firstFace + i];
		bounds += FaceBounds( abs(faceNum) );
	}
	return bounds;
}

/*
============
anSEASFileLocal::PointAreaNum
============
*/
int anSEASFileLocal::PointAreaNum( const anVec3 &origin ) const {
	int nodeNum;
	const seasNode_t *node;

	nodeNum = 1;
	do {
		node = &nodes[nodeNum];
		if ( planeList[node->planeNum].Side( origin ) == PLANESIDE_BACK ) {
			nodeNum = node->children[1];
		}
		else {
			nodeNum = node->children[0];
		}
		if ( nodeNum < 0 ) {
			return -nodeNum;
		}
	} while( nodeNum );

	return 0;
}

/*
============
anSEASFileLocal::PointReachableAreaNum
============
*/
int anSEASFileLocal::PointReachableAreaNum( const anVec3 &origin, const anBounds &searchBounds, const int areaFlags, const int excludeTravelFlags ) const {
	int areaList[32], areaNum, i;
	anVec3 start, end, pointList[32];
	seasTrace_t trace;
	anBounds bounds;
	float frac;

	start = origin;

	trace.areas = areaList;
	trace.points = pointList;
	trace.maxAreas = sizeof( areaList ) / sizeof( int );
	trace.getOutOfSolid = true;

	areaNum = PointAreaNum( start );
	if ( areaNum ) {
		if ( ( areas[areaNum].flags & areaFlags ) && ( ( areas[areaNum].travelFlags & excludeTravelFlags ) == 0 ) ) {
			return areaNum;
		}
	} else {
		// trace up
		end = start;
		end[2] += 32.0f;
		Trace( trace, start, end );
		if ( trace.numAreas >= 1 ) {
			if ( ( areas[0].flags & areaFlags ) && ( ( areas[0].travelFlags & excludeTravelFlags ) == 0 ) ) {
				return areaList[0];
			}
			start = pointList[0];
			start[2] += 1.0f;
		}
	}

	// trace down
	end = start;
	end[2] -= 32.0f;
	Trace( trace, start, end );
	if ( trace.lastAreaNum ) {
		if ( ( areas[trace.lastAreaNum].flags & areaFlags ) && ( ( areas[trace.lastAreaNum].travelFlags & excludeTravelFlags ) == 0 ) ) {
			return trace.lastAreaNum;
		}
		start = trace.endpos;
	}

	// expand bounds until an area is found
	for ( i = 1; i <= 12; i++ ) {
		frac = i * ( 1.0f / 12.0f );
		bounds[0] = origin + searchBounds[0] * frac;
		bounds[1] = origin + searchBounds[1] * frac;
		areaNum = BoundsReachableAreaNum( bounds, areaFlags, excludeTravelFlags );
		if ( areaNum && ( areas[areaNum].flags & areaFlags ) && ( ( areas[areaNum].travelFlags & excludeTravelFlags ) == 0 ) ) {
			return areaNum;
		}
	}
	return 0;
}

/*
============
anSEASFileLocal::BoundsReachableAreaNum_r
============
*/
int anSEASFileLocal::BoundsReachableAreaNum_r( int nodeNum, const anBounds &bounds, const int areaFlags, const int excludeTravelFlags ) const {
	int res;
	const seasNode_t *node;

	while( nodeNum ) {
		if ( nodeNum < 0 ) {
			if ( ( areas[-nodeNum].flags & areaFlags ) && ( ( areas[-nodeNum].travelFlags & excludeTravelFlags ) == 0 ) ) {
				return -nodeNum;
			}
			return 0;
		}
		node = &nodes[nodeNum];
		res = bounds.PlaneSide( planeList[node->planeNum] );
		if ( res == PLANESIDE_BACK ) {
			nodeNum = node->children[1];
		}
		else if ( res == PLANESIDE_FRONT ) {
			nodeNum = node->children[0];
		}
		else {
			nodeNum = BoundsReachableAreaNum_r( node->children[1], bounds, areaFlags, excludeTravelFlags );
			if ( nodeNum ) {
				return nodeNum;
			}
			nodeNum = node->children[0];
		}
	}

	return 0;
}

/*
============
anSEASFileLocal::BoundsReachableAreaNum
============
*/
int anSEASFileLocal::BoundsReachableAreaNum( const anBounds &bounds, const int areaFlags, const int excludeTravelFlags ) const {

	return BoundsReachableAreaNum_r( 1, bounds, areaFlags, excludeTravelFlags );
}

/*
============
anSEASFileLocal::PushPointIntoAreaNum
============
*/
void anSEASFileLocal::PushPointIntoAreaNum( int areaNum, anVec3 &point ) const {
	int i, faceNum;
	const seasArea_t *area;
	const seasFace_t *face;

	area = &areas[areaNum];

	// push the point to the right side of all area face planes
	for ( i = 0; i < area->numFaces; i++ ) {
		faceNum = faceIndex[area->firstFace + i];
		face = &faces[abs( faceNum )];

		const anPlane &plane = planeList[face->planeNum ^ INT32_SIGNBITSET( faceNum )];
		float dist = plane.Distance( point );

		// project the point onto the face plane if it is on the wrong side
		if ( dist < 0.0f ) {
			point -= dist * plane.Normal();
		}
	}
}

/*
============
anSEASFileLocal::Trace
============
*/
#define TRACEPLANE_EPSILON		0.125f

typedef struct aasTraceStack_s
{
	anVec3			start;
	anVec3			end;
	int				planeNum;
	int				nodeNum;
} aasTraceStack_t;

bool anSEASFileLocal::Trace( seasTrace_t &trace, const anVec3 &start, const anVec3 &end ) const {
	int side, nodeNum, tmpPlaneNum;
	double front, back, frac;
	anVec3 cur_start, cur_end, cur_mid, v1, v2;
	aasTraceStack_t tracestack[SEAS_MAX_TREE_DEPTH];
	aasTraceStack_t *tstack_p;
	const seasNode_t *node;
	const anPlane *plane;

	trace.numAreas = 0;
	trace.lastAreaNum = 0;
	trace.blockingAreaNum = 0;

	tstack_p = tracestack;
	tstack_p->start = start;
	tstack_p->end = end;
	tstack_p->planeNum = 0;
	tstack_p->nodeNum = 1;		//start with the root of the tree
	tstack_p++;

	while( 1 ) {

		tstack_p--;
		// if the trace stack is empty
		if ( tstack_p < tracestack ) {
			if ( !trace.lastAreaNum ) {
				// completely in solid
				trace.fraction = 0.0f;
				trace.endpos = start;
			}
			else {
				// nothing was hit
				trace.fraction = 1.0f;
				trace.endpos = end;
			}
			trace.planeNum = 0;
			return false;
		}

		// number of the current node to test the line against
		nodeNum = tstack_p->nodeNum;

		// if it is an area
		if ( nodeNum < 0) {
			// if can't enter the area
			if ( ( areas[-nodeNum].flags & trace.flags ) || ( areas[-nodeNum].travelFlags & trace.travelFlags ) ) {
				if ( !trace.lastAreaNum ) {
					trace.fraction = 0.0f;
					v1 = vec3_origin;
				} else {
					v1 = end - start;
					v2 = tstack_p->start - start;
					trace.fraction = v2.Length() / v1.Length();
				}
				trace.endpos = tstack_p->start;
				trace.blockingAreaNum = -nodeNum;
				trace.planeNum = tstack_p->planeNum;
				// always take the plane with normal facing towards the trace start
				plane = &planeList[trace.planeNum];
				if ( v1 * plane->Normal() > 0.0f ) {
					trace.planeNum ^= 1;
				}
				return true;
			}
			trace.lastAreaNum = -nodeNum;
			if ( trace.numAreas < trace.maxAreas ) {
				if ( trace.areas ) {
					trace.areas[trace.numAreas] = -nodeNum;
				}
				if ( trace.points ) {
					trace.points[trace.numAreas] = tstack_p->start;
				}
				trace.numAreas++;
			}
			continue;
		}

		// if it is a solid leaf
		if ( !nodeNum ) {
			if ( !trace.lastAreaNum ) {
				trace.fraction = 0.0f;
				v1 = vec3_origin;
			} else {
				v1 = end - start;
				v2 = tstack_p->start - start;
				trace.fraction = v2.Length() / v1.Length();
			}
			trace.endpos = tstack_p->start;
			trace.blockingAreaNum = 0;	// hit solid leaf
			trace.planeNum = tstack_p->planeNum;
			// always take the plane with normal facing towards the trace start
			plane = &planeList[trace.planeNum];
			if ( v1 * plane->Normal() > 0.0f ) {
				trace.planeNum ^= 1;
			}
			if ( !trace.lastAreaNum && trace.getOutOfSolid ) {
				continue;
			}
			else {
				return true;
			}
		}

		// the node to test against
		node = &nodes[nodeNum];
		// start point of current line to test against node
		cur_start = tstack_p->start;
		// end point of the current line to test against node
		cur_end = tstack_p->end;
		// the current node plane
		plane = &planeList[node->planeNum];

		front = plane->Distance( cur_start );
		back = plane->Distance( cur_end );

		// if the whole to be traced line is totally at the front of this node
		// only go down the tree with the front child
		if ( front >= -ON_EPSILON && back >= -ON_EPSILON ) {
			// keep the current start and end point on the stack and go down the tree with the front child
			tstack_p->nodeNum = node->children[0];
			tstack_p++;
			if ( tstack_p >= &tracestack[SEAS_MAX_TREE_DEPTH] ) {
				common->Error( "anSEASFileLocal::Trace: stack overflow\n" );
				return false;
			}
		}
		// if the whole to be traced line is totally at the back of this node
		// only go down the tree with the back child
		else if ( front < ON_EPSILON && back < ON_EPSILON ) {
			// keep the current start and end point on the stack and go down the tree with the back child
			tstack_p->nodeNum = node->children[1];
			tstack_p++;
			if ( tstack_p >= &tracestack[SEAS_MAX_TREE_DEPTH] ) {
				common->Error( "anSEASFileLocal::Trace: stack overflow\n" );
				return false;
			}
		}
		// go down the tree both at the front and back of the node
		else {
			tmpPlaneNum = tstack_p->planeNum;
			// calculate the hit point with the node plane
			// put the cross point TRACEPLANE_EPSILON on the near side
			if (front < 0) {
				frac = (front + TRACEPLANE_EPSILON) / ( front - back );
			}
			else {
				frac = (front - TRACEPLANE_EPSILON) / ( front - back );
			}

			if (frac < 0) {
				frac = 0.001f; //0
			}
			else if (frac > 1) {
				frac = 0.999f; //1
			}

			cur_mid = cur_start + ( cur_end - cur_start ) * frac;

			// side the front part of the line is on
			side = front < 0;

			// first put the end part of the line on the stack (back side)
			tstack_p->start = cur_mid;
			tstack_p->planeNum = node->planeNum;
			tstack_p->nodeNum = node->children[!side];
			tstack_p++;
			if ( tstack_p >= &tracestack[SEAS_MAX_TREE_DEPTH] ) {
				common->Error( "anSEASFileLocal::Trace: stack overflow\n" );
				return false;
			}
			// now put the part near the start of the line on the stack so we will
			// continue with that part first.
			tstack_p->start = cur_start;
			tstack_p->end = cur_mid;
			tstack_p->planeNum = tmpPlaneNum;
			tstack_p->nodeNum = node->children[side];
			tstack_p++;
			if ( tstack_p >= &tracestack[SEAS_MAX_TREE_DEPTH] ) {
				common->Error( "anSEASFileLocal::Trace: stack overflow\n" );
				return false;
			}
		}
	}
	return false;
}

/*
============
anSEASLocal::AreaContentsTravelFlags
============
*/
int anSEASFileLocal::AreaContentsTravelFlags( int areaNum ) const {
	if ( areas[areaNum].contents & AREACONTENTS_WATER ) {
		return TFL_WATER;
	}
	return TFL_AIR;
}

/*
============
anSEASFileLocal::MaxTreeDepth_r
============
*/
void anSEASFileLocal::MaxTreeDepth_r( int nodeNum, int &depth, int &maxDepth ) const {
	const seasNode_t *node;

	if ( nodeNum <= 0 ) {
		return;
	}

	depth++;
	if ( depth > maxDepth ) {
		maxDepth = depth;
	}

	node = &nodes[nodeNum];
	MaxTreeDepth_r( node->children[0], depth, maxDepth );
	MaxTreeDepth_r( node->children[1], depth, maxDepth );

	depth--;
}

/*
============
anSEASFileLocal::MaxTreeDepth
============
*/
int anSEASFileLocal::MaxTreeDepth() const {
	int depth, maxDepth;

	depth = maxDepth = 0;
	MaxTreeDepth_r( 1, depth, maxDepth );
	return maxDepth;
}