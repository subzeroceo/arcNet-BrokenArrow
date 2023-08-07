
#include "../../idlib/Lib.h"
#pragma hdrstop

#include "SEAS_local.h"
#include "../Game_local.h"		// for cvars and debug drawing
#include "AI.h"
#include "SEAS_Find.h"


/*
============
anSEASLocal::DrawCone
============
*/
void anSEASLocal::DrawCone( const anVec3 &origin, const anVec3 &dir, float radius, const anVec4 &color ) const {
	int i;
	anMat3 axis;
	anVec3 center, top, p, lastp;

	axis[2] = dir;
	axis[2].NormalVectors( axis[0], axis[1] );
	axis[1] = -axis[1];

	center = origin + dir;
	top = center + dir * (3.0f * radius);
	lastp = center + radius * axis[1];

	for ( i = 20; i <= 360; i += 20 ) {
		p = center + anMath::Sin( DEG2RAD( i ) ) * radius * axis[0] + anMath::Cos( DEG2RAD( i ) ) * radius * axis[1];
		gameRenderWorld->DebugLine( color, lastp, p, 0 );
		gameRenderWorld->DebugLine( color, p, top, 0 );
		lastp = p;
	}
}

/*
============
anSEASLocal::DrawReachability
============
*/
void anSEASLocal::DrawReachability( const anReachability *reach ) const {
	gameRenderWorld->DebugArrow( colorCyan, reach->start, reach->end, 2 );

	if ( gameLocal.GetLocalPlayer() ) {
		gameRenderWorld->DrawText( va( "%d", reach->edgeNum ), ( reach->start + reach->end ) * 0.5f, 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAxis );
	}

	switch ( reach->travelType ) {
		case TFL_WALK: {
//			const anReachability_Walk *walk = static_cast<const anReachability_Walk *>(reach);
			break;
		}
		default: {
			break;
		}
	}
}

/*
============
anSEASLocal::DrawEdge
============
*/
void anSEASLocal::DrawEdge( int edgeNum, bool arrow ) const {
	const seasEdge_t *edge;
	anVec4 *color;

	if ( !file ) {
		return;
	}

	edge = &file->GetEdge( edgeNum );
	color = &colorRed;
	if ( arrow ) {
		gameRenderWorld->DebugArrow( *color, file->GetVertex( edge->vertexNum[0] ), file->GetVertex( edge->vertexNum[1] ), 1 );
	} else {
		gameRenderWorld->DebugLine( *color, file->GetVertex( edge->vertexNum[0] ), file->GetVertex( edge->vertexNum[1] ) );
	}

	if ( gameLocal.GetLocalPlayer() ) {
		gameRenderWorld->DrawText( va( "%d", edgeNum ), ( file->GetVertex( edge->vertexNum[0] ) + file->GetVertex( edge->vertexNum[1] ) ) * 0.5f + anVec3(0,0,4), 0.1f, colorRed, gameLocal.GetLocalPlayer()->viewAxis );
	}
}

/*
============
anSEASLocal::DrawFace
============
*/
void anSEASLocal::DrawFace( int faceNum, bool side ) const {
	int i, j, numEdges, firstEdge;
	const seasFace_t *face;
	anVec3 mid, end;

	if ( !file ) {
		return;
	}

	face = &file->GetFace( faceNum );
	numEdges = face->numEdges;
	firstEdge = face->firstEdge;

	if ( !numEdges ) {//wtf?  A face with no edges?!
		return;
	}

	mid = vec3_origin;
	for ( i = 0; i < numEdges; i++ ) {
		DrawEdge( abs( file->GetEdgeIndex( firstEdge + i ) ), ( face->flags & FACE_FLOOR ) != 0 );
		j = file->GetEdgeIndex( firstEdge + i );
		mid += file->GetVertex( file->GetEdge( abs( j ) ).vertexNum[ j < 0 ] );
	}

	mid /= numEdges;
	if ( side ) {
		end = mid - 5.0f * file->GetPlane( file->GetFace( faceNum ).planeNum ).Normal();
	} else {
		end = mid + 5.0f * file->GetPlane( file->GetFace( faceNum ).planeNum ).Normal();
	}
	gameRenderWorld->DebugArrow( colorGreen, mid, end, 1 );
}

/*
============
anSEASLocal::DrawAreaBounds
============
*/
void anSEASLocal::DrawAreaBounds( int areaNum ) const {
	const seasArea_t *area;
	if ( !file ) {
		return;
	}
	area = &file->GetArea( areaNum );

	anVec3 points[8];
	bool	drawn[8][8];
	memset( drawn, false, sizeof( drawn ) );

	area->bounds.ToPoints( points );
	for ( int p1 = 0; p1 < 8; p1++ ) {
		for ( int p2 = 0; p2 < 8; p2++ ) {
			if ( !drawn[p2][p1] ) {
				if ( (points[p1].x == points[p2].x && (points[p1].y == points[p2].y||points[p1].z == points[p2].z))
					|| (points[p1].y == points[p2].y && (points[p1].x == points[p2].x||points[p1].z == points[p2].z))
					|| (points[p1].z == points[p2].z && (points[p1].x == points[p2].x||points[p1].y == points[p2].y)) ) {
					//an edge
					gameRenderWorld->DebugLine( colorRed, points[p1], points[p2] );
					drawn[p1][p2] = true;
				}
			}
		}
	}
}

/*
============
anSEASLocal::DrawArea
============
*/
void anSEASLocal::DrawArea( int areaNum ) const {
	int i, numFaces, firstFace;
	const seasArea_t *area;
	anReachability *reach;
	if ( !file ) {
		return;
	}

	area = &file->GetArea( areaNum );

	if ( aas_showAreaBounds.GetBool() ) {
		if ( (area->flags&AREA_FLOOR)  ) {
			anVec3 areaTop = area->center;
			areaTop.z = area->ceiling;
			gameRenderWorld->DebugArrow( colorCyan, area->center, areaTop, 1 );
			gameRenderWorld->DrawText( va( "%4.2f", floor(area->ceiling-area->center.z) ), ( area->center + areaTop ) * 0.5f, 0.1f, colorCyan, gameLocal.GetLocalPlayer()->viewAxis );
		} else {//air area
			DrawAreaBounds( areaNum );
		}
	}

	numFaces = area->numFaces;
	firstFace = area->firstFace;

	for ( i = 0; i < numFaces; i++ ) {
		DrawFace( abs( file->GetFaceIndex( firstFace + i ) ), file->GetFaceIndex( firstFace + i ) < 0 );
	}

	if (aas_showRevReach.GetInteger()) {
 		for ( reach = area->rev_reach; reach; reach = reach->rev_next ) {
 			DrawReachability( reach );
 		}
 	} else {
 		for ( reach = area->reach; reach; reach = reach->next ) {
 			DrawReachability( reach );
 		}
	}
}

/*
============
anSEASLocal::DefaultSearchBounds
============
*/
const anBounds &anSEASLocal::DefaultSearchBounds( void ) const {
	return file->GetSettings().boundingBoxes[0];
}

/*
============
anSEASLocal::ShowArea
============
*/
void anSEASLocal::ShowArea( const anVec3 &origin ) const {
	static int lastAreaNum;
	int areaNum;
	const seasArea_t *area;
	anVec3 org;

	areaNum = PointReachableAreaNum( origin, DefaultSearchBounds(), (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY) );
	org = origin;
	PushPointIntoAreaNum( areaNum, org );

	if ( aas_goalArea.GetInteger() ) {
		int travelTime;
		anReachability *reach;

		RouteToGoalArea( areaNum, org, aas_goalArea.GetInteger(), TFL_WALK|TFL_AIR, travelTime, &reach );
		gameLocal.Printf( "\rtt = %4d", travelTime );
		if ( reach ) {
			gameLocal.Printf( " to area %4d", reach->toAreaNum );
			DrawArea( reach->toAreaNum );
		}
	}

	if ( areaNum != lastAreaNum ) {
		area = &file->GetArea( areaNum );
		gameLocal.Printf( "area %d: ", areaNum );
		if ( area->flags & AREA_LEDGE ) {
			gameLocal.Printf( "AREA_LEDGE " );
		}
		if ( area->flags & AREA_REACHABLE_WALK ) {
			gameLocal.Printf( "AREA_REACHABLE_WALK " );
		}
		if ( area->flags & AREA_REACHABLE_FLY ) {
			gameLocal.Printf( "AREA_REACHABLE_FLY " );
		}
		if ( area->contents & AREACONTENTS_CLUSTERPORTAL ) {
			gameLocal.Printf( "AREACONTENTS_CLUSTERPORTAL " );
		}
		if ( area->contents & AREACONTENTS_OBSTACLE ) {
			gameLocal.Printf( "AREACONTENTS_OBSTACLE " );
		}
		gameLocal.Printf( "\n" );
		lastAreaNum = areaNum;
	}

	if ( org != origin ) {
		anBounds bnds = file->GetSettings().boundingBoxes[0];
		bnds[1].z = bnds[0].z;
		gameRenderWorld->DebugBounds( colorYellow, bnds, org );
	}

	DrawArea( areaNum );
}

/*
============
anSEASLocal::DrawSimpleEdge
============
*/
void anSEASLocal::DrawSimpleEdge( int edgeNum ) const {
	const seasEdge_t *edge;
	const anVec4	*color;

	if ( !file ) {
		return;
	}

	edge = &file->GetEdge( edgeNum );
	color = &file->GetSettings().debugColor;

	gameRenderWorld->DebugLine( *color, file->GetVertex( edge->vertexNum[0] ), file->GetVertex( edge->vertexNum[1] ) );
}

/*
============
anSEASLocal::DrawSimpleFace
============
*/
const int 	MAX_AAS_WALL_EDGES			= 256;
void anSEASLocal::DrawSimpleFace( int faceNum, bool visited ) const {
	int				i, numEdges, firstEdge, edgeNum;
	const			seasFace_t *face;
	anVec3			sides[MAX_AAS_WALL_EDGES];
	const seasEdge_t *edge, *nextEdge;
	anVec4			color2;

	if ( !file ) {
		return;
	}

	face = &file->GetFace( faceNum );
	numEdges = face->numEdges;
	firstEdge = face->firstEdge;

	for ( i = 0; i < numEdges; i++ ) {
		DrawSimpleEdge( abs( file->GetEdgeIndex( firstEdge + i ) ) );
	}

	if ( numEdges >= 2 && numEdges <= MAX_AAS_WALL_EDGES ) {
		edgeNum = abs( file->GetEdgeIndex( firstEdge ) );
		edge = &file->GetEdge( abs( edgeNum ) );
		edgeNum = abs( file->GetEdgeIndex( firstEdge + 1 ) );
		nextEdge = &file->GetEdge( abs( edgeNum ) );
		// need to find the first common edge so that we go form the polygon in the right direction
		if ( file->GetVertex( edge->vertexNum[0] ) == file->GetVertex( nextEdge->vertexNum[0] ) ||
			file->GetVertex( edge->vertexNum[0] ) == file->GetVertex( nextEdge->vertexNum[1] ) ) {
			sides[0] = file->GetVertex( edge->vertexNum[0] );
		} else {
			sides[0] = file->GetVertex( edge->vertexNum[1] );
		}

		for ( i = 1; i < numEdges; i++ ) {
			edgeNum = abs( file->GetEdgeIndex( firstEdge + i ) );
			edge = &file->GetEdge( abs( edgeNum ) );
			if ( sides[ i-1 ] == file->GetVertex( edge->vertexNum[0] ) ) {
				sides[ i ] = file->GetVertex( edge->vertexNum[1] );
			} else {
				sides[ i ] = file->GetVertex( edge->vertexNum[0] );
			}
		}

		color2 = file->GetSettings().debugColor;
		color2[3] = 0.20f;
		if ( !visited) {
			color2[3] = 0.05f;
		}

		idWinding winding( sides, numEdges );
		gameRenderWorld->DebugPolygon( color2, winding, 0, true );
	}
}

/*
============
anSEASLocal::DrawSimpleArea
============
*/
void anSEASLocal::DrawSimpleArea( int areaNum ) const {
	int				i, numFaces, firstFace;
	const seasArea_t *area;

	if ( !file ) {
		return;
	}

	area = &file->GetArea( areaNum );

	if ( aas_showAreaBounds.GetBool() ) {
		if ( (area->flags&AREA_FLOOR) ) {
			anVec3 areaTop = area->center;
			areaTop.z = area->ceiling;
			gameRenderWorld->DebugArrow( colorCyan, area->center, areaTop, 1 );
			gameRenderWorld->DrawText( va( "%4.2f", floor(area->ceiling-area->center.z) ), ( area->center + areaTop ) * 0.5f, 0.1f, colorCyan, gameLocal.GetLocalPlayer()->viewAxis );
		} else {//air area
			DrawAreaBounds( areaNum );
		}
	}

	numFaces = area->numFaces;
	firstFace = area->firstFace;

	for ( i = 0; i < numFaces; i++ ) {
		DrawSimpleFace( abs( file->GetFaceIndex( firstFace + i ) ), true );
	}
}

/*
============
anSEASLocal::IsValid
============
*/
bool anSEASLocal::IsValid( void ) const {
	if ( !file ) {
		return false;
	}

	return true;
}

/*
============
anSEASLocal::ShowAreas
============
*/
void anSEASLocal::ShowAreas( const anVec3 &origin, bool ShowProblemAreas ) const {
	int				i, areaNum;
	anBasePlayer		*player;
	int				*areaQueue, curArea, queueStart, queueEnd;
	byte			*areasVisited;
	const seasArea_t *area;
	anReachability	*reach;
	int				travelFlags = (TFL_WALK);
	const anBounds	bounds = anBounds( origin ).Expand( 256.0f );

	if ( !file ) {
		return;
	}

	player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		return;
	}

	areaNum = PointReachableAreaNum( origin, DefaultSearchBounds(), (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY) );

	areasVisited = (byte *) _alloca16( file->GetNumAreas() );
	memset( areasVisited, 0, file->GetNumAreas() * sizeof( byte ) );
	areaQueue = (int *) _alloca16( file->GetNumAreas() * sizeof( int ) );

	queueStart = 0;
	queueEnd = 1;
	areaQueue[0] = areaNum;
	areasVisited[areaNum] = true;

	for ( curArea = areaNum; queueStart < queueEnd; curArea = areaQueue[++queueStart] ) {
		area = &file->GetArea( curArea );

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

	if ( ShowProblemAreas ) {
		for ( i = 0; i < queueEnd; i++ ) {
			ShowProblemArea( areaQueue[ i ] );
		}
	} else {
		for ( i = 0; i < queueEnd; i++ ) {
			DrawSimpleArea( areaQueue[ i ] );
		}
	}
}

/*
============
anSEASLocal::ShowProblemEdge
============
*/
void anSEASLocal::ShowProblemEdge( int edgeNum ) const {
	const seasEdge_t	*edge;
	const anVec4	*color;
	anVec4			color2;
	anTraceModel	trm;
	anClipModel		mdl;
	anVec3			sides[4];
	trace_t			results;
	anVec3			forward, forwardNormal, left, down, start, end;
	float			hullSize, hullHeight;

	if ( !file ) {
		return;
	}

	edge = &file->GetEdge( edgeNum );

	start = (anVec3)file->GetVertex( edge->vertexNum[0] );
	end = (anVec3)file->GetVertex( edge->vertexNum[1] );
	forward =  end - start ;
	forward.Normalize();
	forwardNormal = forward;
	forward.NormalVectors( left, down );

	hullSize = ( ( file->GetSettings().boundingBoxes[0][1][0] - file->GetSettings().boundingBoxes[0][0][0] ) / 2.0f ) - 1.0f;	// assumption that x and y are the same size
	hullHeight = ( file->GetSettings().boundingBoxes[0][1][2] - file->GetSettings().boundingBoxes[0][0][2] );

	left *= hullSize;
	forward *= hullSize;

	sides[0] = -left + anVec3( 0.0f, 0.0f, file->GetSettings().boundingBoxes[0][0][2] + 1.0f );
	sides[1] = left + anVec3( 0.0f, 0.0f, file->GetSettings().boundingBoxes[0][0][2] + 1.0f );
	sides[2] = left + anVec3( 0.0f, 0.0f, file->GetSettings().boundingBoxes[0][1][2] - 1.0f );
	sides[3] = -left + anVec3( 0.0f, 0.0f, file->GetSettings().boundingBoxes[0][1][2] - 1.0f );
	trm.SetupPolygon( sides, 4 );
	mdl.LoadModel( trm, nullptr );

	gameLocal.Translation( gameLocal.GetLocalPlayer(), results, start, end, &mdl, mat3_identity, MASK_MONSTERSOLID, gameLocal.GetLocalPlayer() );

	if ( results.fraction != 1.0f ) {
		color = &file->GetSettings().debugColor;
		gameRenderWorld->DebugLine( *color, start - left, end - left );
		gameRenderWorld->DebugLine( *color, start + left, end + left );

		color = &colorYellow;
		gameRenderWorld->DebugLine( *color, start, end );

		color2 = colorOrange;
		color2[3] = 0.25f;
		sides[0] += results.endpos;
		sides[1] += results.endpos;
		sides[2] += results.endpos;
		sides[3] += results.endpos;
		idWinding winding( sides, 4 );
		gameRenderWorld->DebugPolygon( color2, winding );
	}
}

/*
============
anSEASLocal::ShowProblemFace
============
*/
void anSEASLocal::ShowProblemFace( int faceNum ) const {
	int		i, numEdges, firstEdge;
	const	seasFace_t *face;

	if ( !file ) {
		return;
	}

	face = &file->GetFace( faceNum );
	numEdges = face->numEdges;
	firstEdge = face->firstEdge;

	for ( i = 0; i < numEdges; i++ ) {
		ShowProblemEdge( abs( file->GetEdgeIndex( firstEdge + i ) ) );
	}
}

/*
============
anSEASLocal::ShowProblemArea
============
*/
void anSEASLocal::ShowProblemArea( int areaNum ) const {
	int				i, numFaces, firstFace;
	const seasArea_t *area;
	anEntity *		entityList[ MAX_GENTITIES ];
	int				numListedEntities;
	anBounds		bounds;
	float			hullSize;

	if ( !file ) {
		return;
	}

	area = &file->GetArea( areaNum );
	numFaces = area->numFaces;
	firstFace = area->firstFace;

	hullSize = ( ( file->GetSettings().boundingBoxes[0][1][0] - file->GetSettings().boundingBoxes[0][0][0] ) / 2.0f );	// assumption that x and y are the same size

	bounds = area->bounds;
	bounds.Expand( hullSize );
	numListedEntities = gameLocal.EntitiesTouchingBounds( gameLocal.GetLocalPlayer(), bounds, -1, entityList, MAX_GENTITIES );

	for ( i = 0; i < numListedEntities; i++ ) {
		if ( entityList[ i ]->IsType( idMoveable::GetClassType() )  ||
			 entityList[ i ]->IsType( idMover::GetClassType() ) ||
			 entityList[ i ]->IsType( anSAAI::GetClassType() ) ) {
			entityList[ i ]->GetPhysics()->DisableClip();
			continue;
		}

		entityList[ i ] = nullptr;
	}

	for ( i = 0; i < numFaces; i++ ) {
		ShowProblemFace( abs( file->GetFaceIndex( firstFace + i ) ) );
	}

	for ( i = 0; i < numListedEntities; i++ ) {
		if ( entityList[ i ] ) {
			entityList[ i ]->GetPhysics()->EnableClip();
		}
	}
}

/*
============
anSEASLocal::ShowProblemArea
============
*/
void anSEASLocal::ShowProblemArea( const anVec3 &origin ) const {
	static int	lastAreaNum;
	int			areaNum;
	anVec3		org;

	areaNum = PointReachableAreaNum( origin, DefaultSearchBounds(), (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY) );

	ShowProblemArea( areaNum );
}

/*
============
anSEASLocal::ShowWalkPath
============
*/
void anSEASLocal::ShowWalkPath( const anVec3 &origin, int goalAreaNum, const anVec3 &goalOrigin ) const {
	int i, areaNum, curAreaNum, travelTime;
	anReachability *reach;
	anVec3 org, areaCenter;
	seasPath_t path;

	if ( !file ) {
		return;
	}

	org = origin;
	areaNum = PointReachableAreaNum( org, DefaultSearchBounds(), AREA_REACHABLE_WALK );
	PushPointIntoAreaNum( areaNum, org );
	curAreaNum = areaNum;

	for ( i = 0; i < 100; i++ ) {

		if ( !RouteToGoalArea( curAreaNum, org, goalAreaNum, TFL_WALK|TFL_AIR, travelTime, &reach ) ) {
			break;
		}

		if ( !reach ) {
			break;
		}

		gameRenderWorld->DebugArrow( colorGreen, org, reach->start, 2 );
		DrawReachability( reach );

		if ( reach->toAreaNum == goalAreaNum ) {
			break;
		}

		curAreaNum = reach->toAreaNum;
		org = reach->end;
	}

	if ( WalkPathToGoal( path, areaNum, origin, goalAreaNum, goalOrigin, TFL_WALK|TFL_AIR ) ) {
		gameRenderWorld->DebugArrow( colorBlue, origin, path.moveGoal, 2 );
	}
}

/*
============
anSEASLocal::ShowFlyPath
============
*/
void anSEASLocal::ShowFlyPath( const anVec3 &origin, int goalAreaNum, const anVec3 &goalOrigin ) const {
	int i, areaNum, curAreaNum, travelTime;
	anReachability *reach;
	anVec3 org, areaCenter;
	seasPath_t path;

	if ( !file ) {
		return;
	}

	org = origin;
	areaNum = PointReachableAreaNum( org, DefaultSearchBounds(), AREA_REACHABLE_FLY );
	PushPointIntoAreaNum( areaNum, org );
	curAreaNum = areaNum;

	for ( i = 0; i < 100; i++ ) {

		if ( !RouteToGoalArea( curAreaNum, org, goalAreaNum, TFL_WALK|TFL_FLY|TFL_AIR, travelTime, &reach ) ) {
			break;
		}

		if ( !reach ) {
			break;
		}

		gameRenderWorld->DebugArrow( colorPurple, org, reach->start, 2 );
		DrawReachability( reach );

		if ( reach->toAreaNum == goalAreaNum ) {
			break;
		}

		curAreaNum = reach->toAreaNum;
		org = reach->end;
	}

	if ( FlyPathToGoal( path, areaNum, origin, goalAreaNum, goalOrigin, TFL_WALK|TFL_FLY|TFL_AIR ) ) {
		gameRenderWorld->DebugArrow( colorBlue, origin, path.moveGoal, 2 );
	}
}

/*
============
anSEASLocal::ShowWallEdges
============
*/
void anSEASLocal::ShowWallEdges( const anVec3 &origin ) const {
	int i, areaNum, numEdges, edges[1024];
	anVec3 start, end;
	anBasePlayer *player;

	player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		return;
	}

	areaNum = PointReachableAreaNum( origin, DefaultSearchBounds(), (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY) );
	numEdges = GetWallEdges( areaNum, anBounds( origin ).Expand( 256.0f ), TFL_WALK, edges, 1024 );
	for ( i = 0; i < numEdges; i++ ) {
		GetEdge( edges[i], start, end );
		gameRenderWorld->DebugLine( colorRed, start, end );
		gameRenderWorld->DrawText( va( "%d", edges[i] ), ( start + end ) * 0.5f, 0.1f, colorWhite, player->viewAxis );
	}
}

/*
============
anSEASLocal::ShowHideArea
============
*/
void anSEASLocal::ShowHideArea( const anVec3 &origin, int targetAreaNum ) const {
	int areaNum, numObstacles;
	anVec3 target;
	seasGoal_t goal;
	seasObstructs_t obstacles[10];

	areaNum = PointReachableAreaNum( origin, DefaultSearchBounds(), (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY) );
	target = AreaCenter( targetAreaNum );

	// consider the target an obstacle
	obstacles[0].absBounds = anBounds( target ).Expand( 16 );
	numObstacles = 1;

	DrawCone( target, anVec3(0,0,1), 16.0f, colorYellow );

	anSEASTacticalManager findHide ( target );
	if ( FindNearestGoal( goal, areaNum, origin, target, TFL_WALK|TFL_AIR, 0.0f, 0.0f, obstacles, numObstacles, findHide ) ) {
		DrawArea( goal.areaNum );
		ShowWalkPath( origin, goal.areaNum, goal.origin );
		DrawCone( goal.origin, anVec3(0,0,1), 16.0f, colorWhite );
	}
}

/*
============
anSEASLocal::PullPlayer
============
*/
bool anSEASLocal::PullPlayer( const anVec3 &origin, int toAreaNum ) const {
	int areaNum;
	anVec3 areaCenter, dir, vel;
	anAngles delta;
	seasPath_t path;
	anBasePlayer *player;

	player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		return true;
	}

	anPhysics *physics = player->GetPhysics();
	if ( !physics ) {
		return true;
	}

	if ( !toAreaNum ) {
		return false;
	}

	areaNum = PointReachableAreaNum( origin, DefaultSearchBounds(), (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY) );
	areaCenter = AreaCenter( toAreaNum );
	if ( player->GetPhysics()->GetAbsBounds().Expand( 8 ).ContainsPoint( areaCenter ) ) {
		return false;
	}
	if ( WalkPathToGoal( path, areaNum, origin, toAreaNum, areaCenter, TFL_WALK|TFL_AIR ) ) {
		dir = path.moveGoal - origin;
		dir[2] *= 0.5f;
		dir.Normalize();
		delta = dir.ToAngles() - player->cmdAngles - player->GetDeltaViewAngles();
		delta.Normalize180();
		player->SetDeltaViewAngles( player->GetDeltaViewAngles() + delta * 0.1f );
		dir[2] = 0.0f;
		dir.Normalize();
		dir *= 100.0f;
		vel = physics->GetLinearVelocity();
		dir[2] = vel[2];
		physics->SetLinearVelocity( dir );
		return true;
	} else {
		return false;
	}
}

/*
============
anSEASLocal::RandomPullPlayer
============
*/
void anSEASLocal::RandomPullPlayer( const anVec3 &origin ) const {
	if ( !PullPlayer( origin, aas_pullPlayer.GetInteger() ) ) {
		int rnd = gameLocal.random.RandomFloat() * file->GetNumAreas();
		for ( int i = 0; i < file->GetNumAreas(); i++ ) {
			int n = (rnd + i) % file->GetNumAreas();
			if ( file->GetArea( n ).flags & (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY) ) {
				aas_pullPlayer.SetInteger( n );
			}
		}
	} else {
		ShowWalkPath( origin, aas_pullPlayer.GetInteger(), AreaCenter( aas_pullPlayer.GetInteger() ) );
	}
}

/*
============
anSEASLocal::ShowPushIntoArea
============
*/
void anSEASLocal::ShowPushIntoArea( const anVec3 &origin ) const {
	int areaNum;
	anVec3 target;

	target = origin;
	areaNum = PointReachableAreaNum( target, DefaultSearchBounds(), (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY) );
	PushPointIntoAreaNum( areaNum, target );
	gameRenderWorld->DebugArrow( colorGreen, origin, target, 1 );
}

/*
============
anSEASLocal::Test
============
*/
void anSEASLocal::Test( const anVec3 &origin ) {
	if ( !file ) {
		return;
	}

	if ( aas_randomPullPlayer.GetBool() ) {
		RandomPullPlayer( origin );
	}
	if ( ( aas_pullPlayer.GetInteger() > 0 ) && ( aas_pullPlayer.GetInteger() < file->GetNumAreas() ) ) {
		ShowWalkPath( origin, aas_pullPlayer.GetInteger(), AreaCenter( aas_pullPlayer.GetInteger() ) );
		PullPlayer( origin, aas_pullPlayer.GetInteger() );
	}
	if ( ( aas_showPath.GetInteger() > 0 ) && ( aas_showPath.GetInteger() < file->GetNumAreas() ) ) {
		ShowWalkPath( origin, aas_showPath.GetInteger(), AreaCenter( aas_showPath.GetInteger() ) );
	}
	if ( ( aas_showFlyPath.GetInteger() > 0 ) && ( aas_showFlyPath.GetInteger() < file->GetNumAreas() ) ) {
		ShowFlyPath( origin, aas_showFlyPath.GetInteger(), AreaCenter( aas_showFlyPath.GetInteger() ) );
	}
	if ( ( aas_showHideArea.GetInteger() > 0 ) && ( aas_showHideArea.GetInteger() < file->GetNumAreas() ) ) {
		ShowHideArea( origin, aas_showHideArea.GetInteger() );
	}

	if ( aas_showAreas.GetInteger() == 1 ) {
		ShowArea( origin );
	} else if ( aas_showAreas.GetInteger() == 2 ) {
		ShowAreas( origin );
	}
	if ( aas_showProblemAreas.GetInteger() == 1 ) {
		ShowProblemArea( origin );
	} else if ( aas_showProblemAreas.GetInteger() == 2 ) {
		ShowAreas( origin, true );
	}

	if ( aas_showWallEdges.GetBool() ) {
		ShowWallEdges( origin );
	}
	if ( aas_showPushIntoArea.GetBool() ) {
		ShowPushIntoArea( origin );
	}
}
