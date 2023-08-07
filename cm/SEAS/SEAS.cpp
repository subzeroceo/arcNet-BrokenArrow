#include "../../idlib/Lib.h"
#pragma hdrstop


#include "../Game_local.h"

#include "SEAS_local.h"

/*
============
anSEAS::Alloc
============
*/
anSEAS *anSEAS::Alloc( void ) {
	MEM_SCOPED_TAG(tag,MA_AAS);
	return new anSEASLocal;
}

/*
============
anSEAS::anSEAS
============
*/
anSEAS::~anSEAS( void ) {
}

/*
============
anSEASLocal::anSEASLocal
============
*/
anSEASLocal::anSEASLocal( void ) {
	file = nullptr;
}

/*
============
anSEASLocal::~anSEASLocal
============
*/
anSEASLocal::~anSEASLocal( void ) {
	Shutdown();
}

/*
============
anSEASLocal::Init
============
*/
bool anSEASLocal::Init( const anStr &mapName, unsigned int mapFileCRC ) {
	if ( file && mapName.Icmp( file->GetName() ) == 0 && mapFileCRC == file->GetCRC() ) {
		gameLocal.Printf( "Keeping %s\n", file->GetName() );
		RemoveAllObstacles();
	} else {
		Shutdown();

		file = SEASFileManager->LoadSEAS( mapName, mapFileCRC );
		if ( !file ) {
			common->DWarning( "Couldn't load AAS file: '%s'", mapName.c_str() );
			return false;
		} else if ( file->IsDummyFile( mapFileCRC ) ) {
			SEASFileManager->FreeSEAS( file );
			file = nullptr;
			return false;
		}
		SetupRouting();
	}
	return true;
}

/*
============
anSEASLocal::Shutdown
============
*/
void anSEASLocal::Shutdown( void ) {
	if ( file ) {
		ShutdownRouting();
		RemoveAllObstacles();
		SEASFileManager->FreeSEAS( file );
		file = nullptr;
	}
}

/*
============
anSEASLocal::Stats
============
*/
void anSEASLocal::Stats( void ) const {
	if ( !file ) {
		return;
	}
	common->Printf( "[%s]\n", file->GetName() );
	file->PrintInfo();
	RoutingStats();
}

/*
============
anSEASLocal::StatsSummary
============
*/
size_t anSEASLocal::StatsSummary( void ) const {
	if ( !file ) {
		return( 0 );
	}

	int size = ( numAreaTravelTimes * sizeof( unsigned short ) )
			+ ( areaCacheIndexSize * sizeof( SEASRouteCache * ) )
			+ ( portalCacheIndexSize * sizeof( SEASRouteCache * ) );

	return( file->GetMemorySize() + size );
}

/*
============
anSEASLocal::GetSettings
============
*/
const anSEASSettings *anSEASLocal::GetSettings( void ) const {
	if ( !file ) {
		return nullptr;
	}
	return &file->GetSettings();
}

/*
============
anSEASLocal::PointAreaNum
============
*/
int anSEASLocal::PointAreaNum( const anVec3 &origin ) const {
	if ( !file ) {
		return 0;
	}
	return file->PointAreaNum( origin );
}

/*
============
anSEASLocal::PointReachableAreaNum
============
*/
int anSEASLocal::PointReachableAreaNum( const anVec3 &origin, const anBounds &searchBounds, const int areaFlags ) const {
	if ( !file ) {
		return 0;
	}

	return file->PointReachableAreaNum( origin, searchBounds, areaFlags, TFL_INVALID );
}

/*
============
anSEASLocal::BoundsReachableAreaNum
============
*/
int anSEASLocal::BoundsReachableAreaNum( const anBounds &bounds, const int areaFlags ) const {
	if ( !file ) {
		return 0;
	}

	return file->BoundsReachableAreaNum( bounds, areaFlags, TFL_INVALID );
}

/*
============
anSEASLocal::PushPointIntoAreaNum
============
*/
void anSEASLocal::PushPointIntoAreaNum( int areaNum, anVec3 &origin ) const {
	if ( !file ) {
		return;
	}
	file->PushPointIntoAreaNum( areaNum, origin );
}

/*
============
anSEASLocal::AreaCenter
============
*/
anVec3 anSEASLocal::AreaCenter( int areaNum ) const {
	if ( !file ) {
		return vec3_origin;
	}
	return file->GetArea( areaNum ).center;
}

/*
============
anSEASLocal::AreaRadius
============
*/
float anSEASLocal::AreaRadius( int areaNum ) const {
	if ( !file ) {
		return 0;
	}
	return file->GetArea( areaNum ).bounds.GetRadius();
}

/*
============
anSEASLocal::AreaBounds
============
*/
anBounds & anSEASLocal::AreaBounds( int areaNum ) const {
	return file->GetArea( areaNum ).bounds;
}

/*
============
anSEASLocal::AreaCeiling
============
*/
float anSEASLocal::AreaCeiling( int areaNum ) const {
	return file->GetArea( areaNum ).ceiling;
}

/*
============
anSEASLocal::AreaFlags
============
*/
int anSEASLocal::AreaFlags( int areaNum ) const {
	if ( !file ) {
		return 0;
	}
	return file->GetArea( areaNum ).flags;
}

/*
============
anSEASLocal::AreaTravelFlags
============
*/
int anSEASLocal::AreaTravelFlags( int areaNum ) const {
	if ( !file ) {
		return 0;
	}
	return file->GetArea( areaNum ).travelFlags;
}

/*
============
anSEASLocal::Trace
============
*/
bool anSEASLocal::Trace( seasTrace_t &trace, const anVec3 &start, const anVec3 &end ) const {
	if ( !file ) {
		trace.fraction = 0.0f;
		trace.lastAreaNum = 0;
		trace.numAreas = 0;
		return true;
	}
	return file->Trace( trace, start, end );
}

/*
============
anSEASLocal::GetPlane
============
*/
const anPlane &anSEASLocal::GetPlane( int planeNum ) const {
	if ( !file ) {
		static anPlane dummy;
		return dummy;
	}
	return file->GetPlane( planeNum );
}

/*
============
anSEASLocal::GetEdgeVertexNumbers
============
*/
void anSEASLocal::GetEdgeVertexNumbers( int edgeNum, int verts[2] ) const {
	if ( !file ) {
		verts[0] = verts[1] = 0;
		return;
	}
	const int *v = file->GetEdge( abs(edgeNum) ).vertexNum;
	verts[0] = v[INTSIGNBITSET(edgeNum)];
	verts[1] = v[INTSIGNBITNOTSET(edgeNum)];
}

/*
============
anSEASLocal::GetEdge
============
*/
void anSEASLocal::GetEdge( int edgeNum, anVec3 &start, anVec3 &end ) const {
	if ( !file ) {
		start.Zero();
		end.Zero();
		return;
	}
	const int *v = file->GetEdge( abs(edgeNum) ).vertexNum;
	start = file->GetVertex( v[INTSIGNBITSET(edgeNum)] );
	end = file->GetVertex( v[INTSIGNBITNOTSET(edgeNum)] );
}


/*
===============================================================================

	anSEASCallback

===============================================================================
*/

/*
============
anSEASCallback::~anSEASCallback
============
*/
anSEASCallback::~anSEASCallback( void ) {
}

/*
============
anSEASCallback::Test
============
*/
anSEASCallback::testResult_t anSEASCallback::Test( class anSEAS *aas, int areaNum, const anVec3 &origin, float minDistance, float maxDistance, const anVec3* point, seasGoal_t& goal ) {
	// Get AAS file
	anSEASFile* file = ( ( anSEAS & )*aas ).GetFile();
	if ( !file ) {
		return TEST_BADAREA;
	}

	// Get area for edges
	seasArea_t& area = file->GetArea( areaNum );

	if ( ai_debugTactical.GetInteger() > 1 ) {
		gameRenderWorld->DebugLine( colorYellow, area.center, area.center + anVec3(0,0,80.0f), 10000 );
	}

	// Make sure the area itself is valid
	if ( !TestArea ( aas, areaNum, area ) ) {
		return TEST_BADAREA;
	}

	if ( ai_debugTactical.GetInteger() > 1 && point ) {
		gameRenderWorld->DebugLine( colorMagenta, *point, *point + anVec3(0,0,64.0f), 10000 );
	}

	// Test the original origin first
	if ( point && TestPointDistance( origin, *point, minDistance, maxDistance) && TestPoint ( aas, *point ) ) {
		goal.areaNum = areaNum;
		goal.origin  = *point;
		return TEST_OK;
	}

	if ( ai_debugTactical.GetInteger() > 1 ) {
		gameRenderWorld->DebugLine( colorCyan, area.center, area.center + anVec3(0,0,64.0f), 10000 );
	}

	// Test the center of the area
	if ( TestPointDistance( origin, area.center, minDistance, maxDistance) && TestPoint ( aas, area.center, area.ceiling ) ) {
		goal.areaNum = areaNum;
		goal.origin  = area.center;
		return TEST_OK;
	}

	// For each face test all available edges
	int	f;
	int	e;
	for ( f = 0; f < area.numFaces; f ++ ) {
		seasFace_t& face = file->GetFace( abs( file->GetFaceIndex(area.firstFace + f ) ) );
		// for each edge test a point between the center of the edge and the center
		for ( e = 0; e < face.numEdges; e ++ ) {
			anVec3 edgeCenter = file->EdgeCenter( abs( file->GetEdgeIndex( face.firstEdge + e ) ) );
			anVec3 dir = area.center - edgeCenter;
			float  dist;
			for ( dist = dir.Normalize() - 64.0f; dist > 0.0f; dist -= 64.0f ) {
				anVec3 testPoint = edgeCenter + dir * dist;
				if ( ai_debugTactical.GetInteger() > 1 ) {
					gameRenderWorld->DebugLine( colorPurple, testPoint, testPoint + anVec3( 0,0,64.0f ), 10000 );
				}

				if ( TestPointDistance( origin, testPoint, minDistance, maxDistance ) && TestPoint( aas, testPoint, area.ceiling ) ) {
					goal.areaNum = areaNum;
					goal.origin  = testPoint;
					return TEST_OK;
				}
			}
		}
	}

	return TEST_BADPOINT;
}

/*
============
anSEASCallback::Init
============
*/
bool anSEASCallback::TestPointDistance( const anVec3 &origin, const anVec3 &point, float minDistance, float maxDistance ) {
	float dist = ( origin - point ).LengthFast();
	if ( minDistance > 0.0f && dist < minDistance ) {
		return false;
	}
	if ( maxDistance > 0.0f && dist > maxDistance ) {
		return false;
	}
	return true;
}

/*
============
anSEASCallback::Init
============
*/
void anSEASCallback::Init( void ) {
}

/*
============
anSEASCallback::Finish
============
*/
void anSEASCallback::Finish( void ) {
}

/*
============
anSEASCallback::TestArea
============
*/
bool anSEASCallback::TestArea( class anSEAS *aas, int areaNum, const seasArea_t& area ) {
	return true;
}

/*
============
anSEASCallback::TestPoint
============
*/
bool anSEASCallback::TestPoint( class anSEAS *aas, const anVec3 &pos, const float zAllow ) {
	return true;
}