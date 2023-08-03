
#include "../../idlib/Lib.h"
#pragma hdrstop


#include "../Game_local.h"

#include "SEAS_local.h"

#define SUBSAMPLE_WALK_PATH		1
#define SUBSAMPLE_FLY_PATH		0

const int		maxWalkPathIterations		= 10;
const float		maxWalkPathDistance			= 500.0f;
const float		walkPathSampleDistance		= 8.0f;

const int		maxFlyPathIterations		= 10;
const float		maxFlyPathDistance			= 500.0f;
const float		flyPathSampleDistance		= 8.0f;


/*
============
anSEASLocal::EdgeSplitPoint

  calculates split point of the edge with the plane
  returns true if the split point is between the edge vertices
============
*/
bool anSEASLocal::EdgeSplitPoint( anVec3 &split, int edgeNum, const anPlane &plane ) const {
	const seasEdge_t *edge;
	anVec3 v1, v2;
	float d1, d2;

	edge = &file->GetEdge( edgeNum );
	v1 = file->GetVertex( edge->vertexNum[0] );
	v2 = file->GetVertex( edge->vertexNum[1] );
	d1 = v1 * plane.Normal() - plane.Dist();
	d2 = v2 * plane.Normal() - plane.Dist();

	//if ( (d1 < CM_CLIP_EPSILON && d2 < CM_CLIP_EPSILON) || (d1 > -CM_CLIP_EPSILON && d2 > -CM_CLIP_EPSILON) ) {
	if ( FLOATSIGNBITSET( d1 ) == FLOATSIGNBITSET( d2 ) ) {
		return false;
	}
	split = v1 + (d1 / (d1 - d2)) * (v2 - v1);
	return true;
}


// rjohnson: optimized function
/*
============
anSEASLocal::FloorEdgeSplitPoint

  calculates either the closest or furthest point on the floor of the area which also lies on the pathPlane
  the point has to be on the front side of the frontPlane to be valid
============
*/
bool anSEASLocal::FloorEdgeSplitPoint( anVec3 &bestSplit, int areaNum, const anPlane &pathPlane, const anPlane &frontPlane, bool closest ) const {
	int i, j, faceNum, edgeNum;
	const seasArea_t *area;
	const seasFace_t *face;
	anVec3 split;
	float dist, bestDist;
	const seasEdge_t *edge;
	anVec3 v1, v2;
	float d1, d2;

	area = &file->GetArea( areaNum );
	if ( closest ) {
		bestDist = maxWalkPathDistance;

		for ( i = area->numFaces-1; i >= 0; i-- ) {
			faceNum = file->GetFaceIndex( area->firstFace + i );
			face = &file->GetFace( abs(faceNum) );

			if ( !(face->flags & FACE_FLOOR ) ) {
				continue;
			}

			for ( j = face->numEdges-1; j >= 0; j-- ) {
				edgeNum = file->GetEdgeIndex( face->firstEdge + j );

				edge = &file->GetEdge( abs( edgeNum ) );
				v1 = file->GetVertex( edge->vertexNum[0] );
				v2 = file->GetVertex( edge->vertexNum[1] );
				d1 = v1 * pathPlane.Normal() - pathPlane.Dist();
				d2 = v2 * pathPlane.Normal() - pathPlane.Dist();

				//if ( (d1 < CM_CLIP_EPSILON && d2 < CM_CLIP_EPSILON) || (d1 > -CM_CLIP_EPSILON && d2 > -CM_CLIP_EPSILON) ) {
				if ( FLOATSIGNBITSET( d1 ) == FLOATSIGNBITSET( d2 ) ) {
					continue;
				}

				split = v1 + (d1 / (d1 - d2)) * (v2 - v1);
				dist = frontPlane.Distance( split );
				if ( dist >= -0.1f && dist < bestDist ) {
					bestDist = dist;
					bestSplit = split;
				}
			}
		}

		return ( bestDist < maxWalkPathDistance );

	} else {
		bestDist = -0.1f;

		for ( i = area->numFaces-1; i >= 0; i-- ) {
			faceNum = file->GetFaceIndex( area->firstFace + i );
			face = &file->GetFace( abs(faceNum) );

			if ( !(face->flags & FACE_FLOOR ) ) {
				continue;
			}

			for ( j = face->numEdges-1; j >= 0; j-- ) {
				edgeNum = file->GetEdgeIndex( face->firstEdge + j );

				edge = &file->GetEdge( abs( edgeNum ) );
				v1 = file->GetVertex( edge->vertexNum[0] );
				v2 = file->GetVertex( edge->vertexNum[1] );
				d1 = v1 * pathPlane.Normal() - pathPlane.Dist();
				d2 = v2 * pathPlane.Normal() - pathPlane.Dist();

				//if ( (d1 < CM_CLIP_EPSILON && d2 < CM_CLIP_EPSILON) || (d1 > -CM_CLIP_EPSILON && d2 > -CM_CLIP_EPSILON) ) {
				if ( FLOATSIGNBITSET( d1 ) == FLOATSIGNBITSET( d2 ) ) {
					continue;
				}

				split = v1 + (d1 / (d1 - d2)) * (v2 - v1);
				dist = frontPlane.Distance( split );
				if ( dist > bestDist ) {
					bestDist = dist;
					bestSplit = split;
				}
			}
		}

		return ( bestDist > -0.1f );
	}
}


/*
============
anSEASLocal::WalkPathValid

  returns true if one can walk in a straight line between origin and goalOrigin
============
*/
bool anSEASLocal::WalkPathValid( int areaNum, const anVec3 &origin, int goalAreaNum, const anVec3 &goalOrigin, int travelFlags, anVec3 &endPos, int &endAreaNum ) const {
	int curAreaNum, lastAreaNum, lastAreas[4], lastAreaIndex;
	anPlane pathPlane, frontPlane, farPlane;
	anReachability *reach;
	const seasArea_t *area;
	anVec3 p, dir;

	if ( file == nullptr ) {
		endPos = goalOrigin;
		endAreaNum = 0;
		return true;
	}

	lastAreas[0] = lastAreas[1] = lastAreas[2] = lastAreas[3] = areaNum;
	lastAreaIndex = 0;

	pathPlane.SetNormal( (goalOrigin - origin).Cross( file->GetSettings().gravityDir ) );
	pathPlane.Normalize();
	pathPlane.FitThroughPoint( origin );

	frontPlane.SetNormal( goalOrigin - origin );
	frontPlane.Normalize();
	frontPlane.FitThroughPoint( origin );

	farPlane.SetNormal( frontPlane.Normal() );
	farPlane.FitThroughPoint( goalOrigin );

	curAreaNum = areaNum;
	lastAreaNum = curAreaNum;

	while ( 1 ) {

		// find the furthest floor face split point on the path
		if ( !FloorEdgeSplitPoint( endPos, curAreaNum, pathPlane, frontPlane, false ) ) {
			endPos = origin;
		}

		// if we found a point near or further than the goal we're done
		if ( farPlane.Distance( endPos ) > -0.5f ) {
			break;
		}

		// if we reached the goal area we're done
		if ( curAreaNum == goalAreaNum ) {
			break;
		}

		frontPlane.SetDist( frontPlane.Normal() * endPos );

		area = &file->GetArea( curAreaNum );

		for ( reach = area->reach; reach; reach = reach->next ) {
			if ( reach->travelType != TFL_WALK ) {
				continue;
			}

			// if the reachability goes back to a previous area
			if ( reach->toAreaNum == lastAreas[0] || reach->toAreaNum == lastAreas[1] ||
					reach->toAreaNum == lastAreas[2] || reach->toAreaNum == lastAreas[3] ) {
				continue;
			}

			// if undesired travel flags are required to travel through the area
			if ( file->GetArea( reach->toAreaNum ).travelFlags & ~travelFlags ) {
				continue;
			}

			// don't optimize through an area near a ledge
			if ( file->GetArea( reach->toAreaNum ).flags & AREA_LEDGE ) {
				continue;
			}

			// find the closest floor face split point on the path
			if ( !FloorEdgeSplitPoint( p, reach->toAreaNum, pathPlane, frontPlane, true ) ) {
				continue;
			}

			// direction parallel to gravity
			dir = ( file->GetSettings().gravityDir * endPos * file->GetSettings().gravityDir ) -
						( file->GetSettings().gravityDir * p * file->GetSettings().gravityDir );
			if ( dir.LengthSqr() > Square( file->GetSettings().maxStepHeight ) ) {
				continue;
			}

			// direction orthogonal to gravity
			dir = endPos - p - dir;
			if ( dir.LengthSqr() > Square( 0.2f ) ) {
				continue;
			}

			break;
		}

		if ( !reach ) {
			return false;
		}

		lastAreas[lastAreaIndex] = curAreaNum;
		lastAreaIndex = ( lastAreaIndex + 1 ) & 3;

		curAreaNum = reach->toAreaNum;
	}

	endAreaNum = curAreaNum;

	return true;
}

/*
============
anSEASLocal::SubSampleWalkPath
============
*/
anVec3 anSEASLocal::SubSampleWalkPath( int areaNum, const anVec3 &origin, const anVec3 &start, const anVec3 &end, int travelFlags, int &endAreaNum ) const {
	int i, numSamples, curAreaNum;
	anVec3 dir, point, nextPoint, endPos;

	dir = end - start;
	numSamples = (int) (dir.Length() / walkPathSampleDistance) + 1;

	point = start;
	for ( i = 1; i < numSamples; i++ ) {
		nextPoint = start + dir * (( float ) i / numSamples);
		if ( (point - nextPoint).LengthSqr() > Square( maxWalkPathDistance ) ) {
			return point;
		}
		if ( !anSEASLocal::WalkPathValid( areaNum, origin, 0, nextPoint, travelFlags, endPos, curAreaNum ) ) {
			return point;
		}
		point = nextPoint;
		endAreaNum = curAreaNum;
	}
	return point;
}

/*
============
anSEASLocal::WalkPathToGoal

  FIXME: don't stop optimizing on first failure ?
============
*/
bool anSEASLocal::WalkPathToGoal( seasPath_t &path, int areaNum, const anVec3 &origin, int goalAreaNum, const anVec3 &goalOrigin, int travelFlags ) const {
	int i, travelTime, curAreaNum, lastAreas[4], lastAreaIndex, endAreaNum;
	anReachability *reach = nullptr;
	anVec3 endPos;

	path.type = PATHTYPE_WALK;
	path.moveGoal = origin;
	path.moveAreaNum = areaNum;
	path.secondaryGoal = origin;
	path.reachability = nullptr;

	if ( file == nullptr || areaNum == goalAreaNum ) {
		path.moveGoal = goalOrigin;
		return true;
	}

	lastAreas[0] = lastAreas[1] = lastAreas[2] = lastAreas[3] = areaNum;
	lastAreaIndex = 0;

	curAreaNum = areaNum;

	for ( i = 0; i < maxWalkPathIterations; i++ ) {

		if ( !anSEASLocal::RouteToGoalArea( curAreaNum, path.moveGoal, goalAreaNum, travelFlags, travelTime, &reach ) ) {
			break;
		}

		if ( !reach ) {
			return false;
		}


 		// cdr: Alternate Routes Bug
 		path.reachability = reach;


		// no need to check through the first area
		if ( areaNum != curAreaNum ) {
			// only optimize a limited distance ahead
			if ( (reach->start - origin).LengthSqr() > Square( maxWalkPathDistance ) ) {
#if SUBSAMPLE_WALK_PATH
				path.moveGoal = SubSampleWalkPath( areaNum, origin, path.moveGoal, reach->start, travelFlags, path.moveAreaNum );
#endif
				return true;
			}

			if ( !anSEASLocal::WalkPathValid( areaNum, origin, 0, reach->start, travelFlags, endPos, endAreaNum ) ) {
#if SUBSAMPLE_WALK_PATH
				path.moveGoal = SubSampleWalkPath( areaNum, origin, path.moveGoal, reach->start, travelFlags, path.moveAreaNum );
#endif
				return true;
			}
		}

		path.moveGoal = reach->start;
		path.moveAreaNum = curAreaNum;

		if ( reach->travelType != TFL_WALK ) {
			break;
		}

		if ( !anSEASLocal::WalkPathValid( areaNum, origin, 0, reach->end, travelFlags, endPos, endAreaNum ) ) {
			return true;
		}

		path.moveGoal = reach->end;
		path.moveAreaNum = reach->toAreaNum;

		if ( reach->toAreaNum == goalAreaNum ) {
			if ( !anSEASLocal::WalkPathValid( areaNum, origin, 0, goalOrigin, travelFlags, endPos, endAreaNum ) ) {
#if SUBSAMPLE_WALK_PATH
				path.moveGoal = SubSampleWalkPath( areaNum, origin, path.moveGoal, goalOrigin, travelFlags, path.moveAreaNum );
#endif
				return true;
			}
			path.moveGoal = goalOrigin;
			path.moveAreaNum = goalAreaNum;
			return true;
		}

		lastAreas[lastAreaIndex] = curAreaNum;
		lastAreaIndex = ( lastAreaIndex + 1 ) & 3;

		curAreaNum = reach->toAreaNum;

		if ( curAreaNum == lastAreas[0] || curAreaNum == lastAreas[1] ||
				curAreaNum == lastAreas[2] || curAreaNum == lastAreas[3] ) {
			common->Warning( "anSEASLocal::WalkPathToGoal: local routing minimum going from area %d to area %d", areaNum, goalAreaNum );
			break;
		}
	}

	if ( !reach ) {
		return false;
	}

	switch ( reach->travelType ) {
		case TFL_WALKOFFLEDGE:
			path.type = PATHTYPE_WALKOFFLEDGE;
			path.secondaryGoal = reach->end;
			path.reachability = reach;
			break;
		case TFL_BARRIERJUMP:
			path.type |= PATHTYPE_BARRIERJUMP;
			path.secondaryGoal = reach->end;
			path.reachability = reach;
			break;
		case TFL_JUMP:
			path.type |= PATHTYPE_JUMP;
			path.secondaryGoal = reach->end;
			path.reachability = reach;
			break;
		default:
			break;
	}

	return true;
}

/*
============
anSEASLocal::FlyPathValid

  returns true if one can fly in a straight line between origin and goalOrigin
============
*/
bool anSEASLocal::FlyPathValid( int areaNum, const anVec3 &origin, int goalAreaNum, const anVec3 &goalOrigin, int travelFlags, anVec3 &endPos, int &endAreaNum ) const {
	seasTrace_t trace;

	if ( file == nullptr ) {
		endPos = goalOrigin;
		endAreaNum = 0;
		return true;
	}

	file->Trace( trace, origin, goalOrigin );

	endPos = trace.endpos;
	endAreaNum = trace.lastAreaNum;

	if ( trace.fraction >= 1.0f ) {
		return true;
	}

	return false;
}

/*
============
anSEASLocal::SubSampleFlyPath
============
*/
anVec3 anSEASLocal::SubSampleFlyPath( int areaNum, const anVec3 &origin, const anVec3 &start, const anVec3 &end, int travelFlags, int &endAreaNum ) const {
	int i, numSamples, curAreaNum;
	anVec3 dir, point, nextPoint, endPos;

	dir = end - start;
	numSamples = (int) (dir.Length() / flyPathSampleDistance) + 1;

	point = start;
	for ( i = 1; i < numSamples; i++ ) {
		nextPoint = start + dir * (( float ) i / numSamples);
		if ( (point - nextPoint).LengthSqr() > Square( maxFlyPathDistance ) ) {
			return point;
		}
		if ( !anSEASLocal::FlyPathValid( areaNum, origin, 0, nextPoint, travelFlags, endPos, curAreaNum ) ) {
			return point;
		}
		point = nextPoint;
		endAreaNum = curAreaNum;
	}
	return point;
}

/*
============
anSEASLocal::FlyPathToGoal

  FIXME: don't stop optimizing on first failure ?
============
*/
bool anSEASLocal::FlyPathToGoal( seasPath_t &path, int areaNum, const anVec3 &origin, int goalAreaNum, const anVec3 &goalOrigin, int travelFlags ) const {
	int i, travelTime, curAreaNum, lastAreas[4], lastAreaIndex, endAreaNum;
	anReachability *reach = nullptr;
	anVec3 endPos;

	path.type = PATHTYPE_WALK;
	path.moveGoal = origin;
	path.moveAreaNum = areaNum;
	path.secondaryGoal = origin;
	path.reachability = nullptr;

	if ( file == nullptr || areaNum == goalAreaNum ) {
		path.moveGoal = goalOrigin;
		return true;
	}

	lastAreas[0] = lastAreas[1] = lastAreas[2] = lastAreas[3] = areaNum;
	lastAreaIndex = 0;

	curAreaNum = areaNum;

	for ( i = 0; i < maxFlyPathIterations; i++ ) {

		if ( !anSEASLocal::RouteToGoalArea( curAreaNum, path.moveGoal, goalAreaNum, travelFlags, travelTime, &reach ) ) {
			break;
		}

		if ( !reach ) {
			return false;
		}

		// no need to check through the first area
		if ( areaNum != curAreaNum ) {
			if ( (reach->start - origin).LengthSqr() > Square( maxFlyPathDistance ) ) {
#if SUBSAMPLE_FLY_PATH
				path.moveGoal = SubSampleFlyPath( areaNum, origin, path.moveGoal, reach->start, travelFlags, path.moveAreaNum );
#endif
				return true;
			}

			if ( !anSEASLocal::FlyPathValid( areaNum, origin, 0, reach->start, travelFlags, endPos, endAreaNum ) ) {
#if SUBSAMPLE_FLY_PATH
				path.moveGoal = SubSampleFlyPath( areaNum, origin, path.moveGoal, reach->start, travelFlags, path.moveAreaNum );
#endif
				return true;
			}
		}

		path.moveGoal = reach->start;
		path.moveAreaNum = curAreaNum;

		if ( !anSEASLocal::FlyPathValid( areaNum, origin, 0, reach->end, travelFlags, endPos, endAreaNum ) ) {
			return true;
		}

		path.moveGoal = reach->end;
		path.moveAreaNum = reach->toAreaNum;

		if ( reach->toAreaNum == goalAreaNum ) {
			if ( !anSEASLocal::FlyPathValid( areaNum, origin, 0, goalOrigin, travelFlags, endPos, endAreaNum ) ) {
#if SUBSAMPLE_FLY_PATH
				path.moveGoal = SubSampleFlyPath( areaNum, origin, path.moveGoal, goalOrigin, travelFlags, path.moveAreaNum );
#endif
				return true;
			}
			path.moveGoal = goalOrigin;
			path.moveAreaNum = goalAreaNum;
			return true;
		}

		lastAreas[lastAreaIndex] = curAreaNum;
		lastAreaIndex = ( lastAreaIndex + 1 ) & 3;

		curAreaNum = reach->toAreaNum;

		if ( curAreaNum == lastAreas[0] || curAreaNum == lastAreas[1] ||
				curAreaNum == lastAreas[2] || curAreaNum == lastAreas[3] ) {
			common->Warning( "anSEASLocal::FlyPathToGoal: local routing minimum going from area %d to area %d", areaNum, goalAreaNum );
			break;
		}
	}

	if ( !reach ) {
		return false;
	}

	return true;
}

typedef struct wallEdge_s {
	int					edgeNum;
	int					verts[2];
	struct wallEdge_s *	next;
} wallEdge_t;

/*
============
anSEASLocal::SortWallEdges
============
*/
void anSEASLocal::SortWallEdges( int *edges, int numEdges ) const {
	int i, j, k, numSequences;
	wallEdge_t **sequenceFirst, **sequenceLast, *wallEdges, *wallEdge;

	wallEdges = (wallEdge_t *) _alloca16( numEdges * sizeof( wallEdge_t ) );
	sequenceFirst = (wallEdge_t **)_alloca16( numEdges * sizeof( wallEdge_t * ) );
	sequenceLast = (wallEdge_t **)_alloca16( numEdges * sizeof( wallEdge_t * ) );

	for ( i = 0; i < numEdges; i++ ) {
		wallEdges[i].edgeNum = edges[i];
		GetEdgeVertexNumbers( edges[i], wallEdges[i].verts );
		wallEdges[i].next = nullptr;
		sequenceFirst[i] = &wallEdges[i];
		sequenceLast[i] = &wallEdges[i];
	}
	numSequences = numEdges;

	for ( i = 0; i < numSequences; i++ ) {
		for ( j = i+1; j < numSequences; j++ ) {
			if ( sequenceFirst[i]->verts[0] == sequenceLast[j]->verts[1] ) {
				sequenceLast[j]->next = sequenceFirst[i];
				sequenceFirst[i] = sequenceFirst[j];
				break;
			}
			if ( sequenceLast[i]->verts[1] == sequenceFirst[j]->verts[0] ) {
				sequenceLast[i]->next = sequenceFirst[j];
				break;
			}
		}
		if ( j < numSequences ) {
			numSequences--;
			for ( k = j; k < numSequences; k++ ) {
				sequenceFirst[k] = sequenceFirst[k+1];
				sequenceLast[k] = sequenceLast[k+1];
			}
			i = -1;
		}
	}

	k = 0;
	for ( i = 0; i < numSequences; i++ ) {
		for ( wallEdge = sequenceFirst[i]; wallEdge; wallEdge = wallEdge->next ) {
			edges[k++] = wallEdge->edgeNum;
		}
	}
}

/*
============
anSEASLocal::GetWallEdges
============
*/
int anSEASLocal::GetWallEdges( int areaNum, const anBounds &bounds, int travelFlags, int *edges, int maxEdges ) const {
	int i, j, k, l, face1Num, face2Num, edge1Num, edge2Num, numEdges, absEdge1Num;
	int *areaQueue, curArea, queueStart, queueEnd;
	byte *areasVisited;
	const seasArea_t *area;
	const seasFace_t *face1, *face2;
	anReachability *reach;

	if ( !file ) {
		return 0;
	}

	numEdges = 0;

	areasVisited = (byte *) _alloca16( file->GetNumAreas() );
	memset( areasVisited, 0, file->GetNumAreas() * sizeof( byte ) );
	areaQueue = ( int*) _alloca16( file->GetNumAreas() * sizeof( int ) );

	queueStart = -1;
	queueEnd = 0;
	areaQueue[0] = areaNum;
	areasVisited[areaNum] = true;

	for ( curArea = areaNum; queueStart < queueEnd; curArea = areaQueue[++queueStart] ) {

		area = &file->GetArea( curArea );

		for ( i = 0; i < area->numFaces; i++ ) {
			face1Num = file->GetFaceIndex( area->firstFace + i );
			face1 = &file->GetFace( abs(face1Num) );

			if ( !(face1->flags & FACE_FLOOR ) ) {
				continue;
			}

			for ( j = 0; j < face1->numEdges; j++ ) {
				edge1Num = file->GetEdgeIndex( face1->firstEdge + j );
				absEdge1Num = abs( edge1Num );

				// test if the edge is shared by another floor face of this area
				for ( k = 0; k < area->numFaces; k++ ) {
					if ( k == i ) {
						continue;
					}
					face2Num = file->GetFaceIndex( area->firstFace + k );
					face2 = &file->GetFace( abs(face2Num) );

					if ( !(face2->flags & FACE_FLOOR ) ) {
						continue;
					}

					for ( l = 0; l < face2->numEdges; l++ ) {
						edge2Num = abs( file->GetEdgeIndex( face2->firstEdge + l ) );
						if ( edge2Num == absEdge1Num ) {
							break;
						}
					}
					if ( l < face2->numEdges ) {
						break;
					}
				}
				if ( k < area->numFaces ) {
					continue;
				}

				// test if the edge is used by a reachability
				for ( reach = area->reach; reach; reach = reach->next ) {
					if ( reach->travelType & travelFlags ) {
						if ( reach->edgeNum == absEdge1Num ) {
							break;
						}
					}
				}
				if ( reach ) {
					continue;
				}

				// test if the edge is already in the list
				for ( k = 0; k < numEdges; k++ ) {
					if ( edge1Num == edges[k] ) {
						break;
					}
				}
				if ( k < numEdges ) {
					continue;
				}

				// add the edge to the list
				edges[numEdges++] = edge1Num;
				if ( numEdges >= maxEdges ) {
					return numEdges;
				}
			}
		}

		// add new areas to the queue
		for ( reach = area->reach; reach; reach = reach->next ) {
			if ( reach->travelType & travelFlags ) {
				// if the area the reachability leads to hasn't been visited yet and the area bounds touch the search bounds
				if ( !areasVisited[reach->toAreaNum] && bounds.IntersectsBounds( file->GetArea( reach->toAreaNum ).bounds ) ) {
					areaQueue[queueEnd++] = reach->toAreaNum;
					areasVisited[reach->toAreaNum] = true;
				}
			}
		}
	}
	return numEdges;
}
