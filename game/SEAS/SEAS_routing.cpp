
#include "../../idlib/Lib.h"
#pragma hdrstop

#include "SEAS_local.h"
#include "../Game_local.h"		// for print and error

#define CACHETYPE_AREA				1
#define CACHETYPE_PORTAL			2

#define MAX_ROUTING_CACHE_MEMORY	(2*1024*1024)

#define LEDGE_TRAVELTIME_PANALTY	250

/*
============
SEASRouteCache::SEASRouteCache
============
*/
SEASRouteCache::SEASRouteCache( int size ) {
	areaNum = 0;
	cluster = 0;
	next = prev = nullptr;
	time_next = time_prev = nullptr;
	travelFlags = 0;
	startTravelTime = 0;
	type = 0;
	this->size = size;
	reachabilities = new byte[size];
	memset( reachabilities, 0, size * sizeof( reachabilities[0] ) );
	travelTimes = new unsigned short[size];
	memset( travelTimes, 0, size * sizeof( travelTimes[0] ) );
}

/*
============
SEASRouteCache::~SEASRouteCache
============
*/
SEASRouteCache::~SEASRouteCache( void ) {
	delete [] reachabilities;
	delete [] travelTimes;
}

/*
============
SEASRouteCache::Size
============
*/
int SEASRouteCache::Size( void ) const {
	return sizeof( SEASRouteCache ) + size * sizeof( reachabilities[0] ) + size * sizeof( travelTimes[0] );
}

/*
============
anSEASLocal::AreaTravelTime
============
*/
unsigned short anSEASLocal::AreaTravelTime( int areaNum, const anVec3 &start, const anVec3 &end ) const {
	float dist;

	dist = ( end - start ).Length();

	if ( file->GetArea( areaNum ).travelFlags & TFL_CROUCH ) {
		dist *= 100.0f / 100.0f;
	} else if ( file->GetArea( areaNum ).travelFlags & TFL_WATER ) {
		dist *= 100.0f / 150.0f;
	} else {
		dist *= 100.0f / 300.0f;
	}
	if ( dist < 1.0f ) {
		return 1;
	}
	return (unsigned short) anMath::FtoiFast( dist );
}

/*
============
anSEASLocal::CalculateAreaTravelTimes
============
*/
void anSEASLocal::CalculateAreaTravelTimes( void ) {
	int n, i, j, numReach, numRevReach, t, maxt;
	byte *bytePtr;
	anReachability *reach, *rev_reach;

	// get total memory for all area travel times
	numAreaTravelTimes = 0;
	for ( n = 0; n < file->GetNumAreas(); n++ ) {

		if ( !(file->GetArea( n ).flags & (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY)) ) {
			continue;
		}

		numReach = 0;
		for ( reach = file->GetArea( n ).reach; reach; reach = reach->next ) {
			numReach++;
		}

		numRevReach = 0;
		for ( rev_reach = file->GetArea( n ).rev_reach; rev_reach; rev_reach = rev_reach->rev_next ) {
			numRevReach++;
		}
		numAreaTravelTimes += numReach * numRevReach;
	}

	areaTravelTimes = (unsigned short *) Mem_Alloc( numAreaTravelTimes * sizeof( unsigned short ),MA_AAS );
	bytePtr = (byte *) areaTravelTimes;

	for ( n = 0; n < file->GetNumAreas(); n++ ) {
		if ( !(file->GetArea( n ).flags & (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY)) ) {
			continue;
		}
		// for each reachability that starts in this area calculate the travel time
		// towards all the reachabilities that lead towards this area
		for ( maxt = i = 0, reach = file->GetArea( n ).reach; reach; reach = reach->next, i++ ) {
			assert( i < MAX_REACH_PER_AREA );
			if ( i >= MAX_REACH_PER_AREA ) {
				gameLocal.Error( "i >= MAX_REACH_PER_AREA on area %d at %g,%g,%g", n, file->GetArea( n ).center.x, file->GetArea( n ).center.y, file->GetArea( n ).center.z );
			}
			reach->number = i;
			reach->disableCount = 0;
			reach->areaTravelTimes = (unsigned short *) bytePtr;
			for ( j = 0, rev_reach = file->GetArea( n ).rev_reach; rev_reach; rev_reach = rev_reach->rev_next, j++ ) {
				t = AreaTravelTime( n, reach->start, rev_reach->end );
				reach->areaTravelTimes[j] = t;
				if ( t > maxt ) {
					maxt = t;
				}
			}
			bytePtr += j * sizeof( unsigned short );
		}

		// if this area is a portal
		if ( file->GetArea( n ).cluster < 0 ) {
			// set the maximum travel time through this portal
			file->SetPortalMaxTravelTime( -file->GetArea( n ).cluster, maxt );
		}
	}

	assert( ( (unsigned int) bytePtr - (unsigned int) areaTravelTimes ) <= numAreaTravelTimes * sizeof( unsigned short ) );
}

/*
============
anSEASLocal::DeleteAreaTravelTimes
============
*/
void anSEASLocal::DeleteAreaTravelTimes( void ) {
	Mem_Free( areaTravelTimes );
	areaTravelTimes = nullptr;
	numAreaTravelTimes = 0;
}

/*
============
anSEASLocal::SetupRoutingCache
============
*/
void anSEASLocal::SetupRoutingCache( void ) {
	int i;
	byte *bytePtr;

	areaCacheIndexSize = 0;
	for ( i = 0; i < file->GetNumClusters(); i++ ) {
		areaCacheIndexSize += file->GetCluster( i ).numReachableAreas;
	}

	areaCacheIndex = (SEASRouteCache ***) Mem_ClearedAlloc( file->GetNumClusters() * sizeof( SEASRouteCache ** ) +
													areaCacheIndexSize * sizeof( SEASRouteCache *), MA_AAS );

	bytePtr = ((byte *)areaCacheIndex) + file->GetNumClusters() * sizeof( SEASRouteCache ** );
	for ( i = 0; i < file->GetNumClusters(); i++ ) {
		areaCacheIndex[i] = ( SEASRouteCache ** ) bytePtr;
		bytePtr += file->GetCluster( i ).numReachableAreas * sizeof( SEASRouteCache * );
	}

	portalCacheIndexSize = file->GetNumAreas();
	portalCacheIndex = (SEASRouteCache **) Mem_ClearedAlloc( portalCacheIndexSize * sizeof( SEASRouteCache * ), MA_AAS );

	areaUpdate = (SEASRouteUpdate *) Mem_ClearedAlloc( file->GetNumAreas() * sizeof( SEASRouteUpdate ),MA_AAS );
	portalUpdate = (SEASRouteUpdate *) Mem_ClearedAlloc( (file->GetNumPortals()+1) * sizeof( SEASRouteUpdate ),MA_AAS );

	goalAreaTravelTimes = (unsigned short *) Mem_ClearedAlloc( file->GetNumAreas() * sizeof( unsigned short ),MA_AAS );
	cacheListStart = cacheListEnd = nullptr;
	totalCacheMemory = 0;
}

/*
============
anSEASLocal::DeleteClusterCache
============
*/
void anSEASLocal::DeleteClusterCache( int clusterNum ) {
	int i;
	SEASRouteCache *cache;

	if ( !areaCacheIndex ) {
		return;
	}

	for ( i = 0; i < file->GetCluster( clusterNum ).numReachableAreas; i++ ) {
		for ( cache = areaCacheIndex[clusterNum][i]; cache; cache = areaCacheIndex[clusterNum][i] ) {
			areaCacheIndex[clusterNum][i] = cache->next;
			UnlinkCache( cache );
			delete cache;
		}
	}
}

/*
============
anSEASLocal::DeletePortalCache
============
*/
void anSEASLocal::DeletePortalCache( void ) {
	SEASRouteCache *cache;

	if ( !portalCacheIndex ) {
		return;
	}

	for ( int i = 0; i < file->GetNumAreas(); i++ ) {
		for ( cache = portalCacheIndex[i]; cache; cache = portalCacheIndex[i] ) {
			portalCacheIndex[i] = cache->next;
			UnlinkCache( cache );
			delete cache;
		}
	}
}

/*
============
anSEASLocal::ShutdownRoutingCache
============
*/
void anSEASLocal::ShutdownRoutingCache( void ) {
	for ( int i = 0; i < file->GetNumClusters(); i++ ) {
		DeleteClusterCache( i );
	}

	DeletePortalCache();

	Mem_Free( areaCacheIndex );
	areaCacheIndex = nullptr;
	areaCacheIndexSize = 0;
	Mem_Free( portalCacheIndex );
	portalCacheIndex = nullptr;
	portalCacheIndexSize = 0;
	if ( areaUpdate ) {
		Mem_Free( areaUpdate );
		areaUpdate = nullptr;
	}
	if ( portalUpdate ) {
		Mem_Free( portalUpdate );
		portalUpdate = nullptr;
	}
	if ( goalAreaTravelTimes ) {
		Mem_Free( goalAreaTravelTimes );
		goalAreaTravelTimes = nullptr;
	}

	cacheListStart = cacheListEnd = nullptr;
	totalCacheMemory = 0;
}

/*
============
anSEASLocal::SetupRouting
============
*/
bool anSEASLocal::SetupRouting( void ) {
	areaCacheIndex = nullptr;
	portalCacheIndex = nullptr;
	areaUpdate = nullptr;
	portalUpdate = nullptr;
	goalAreaTravelTimes = nullptr;

	CalculateAreaTravelTimes();
	SetupRoutingCache();
	return true;
}

/*
============
anSEASLocal::ShutdownRouting
============
*/
void anSEASLocal::ShutdownRouting( void ) {
	DeleteAreaTravelTimes();
	ShutdownRoutingCache();
}

/*
============
anSEASLocal::RoutingStats
============
*/
void anSEASLocal::RoutingStats( void ) const {
	SEASRouteCache *cache;
	int numAreaCache, numPortalCache;
	int totalAreaCacheMemory, totalPortalCacheMemory;

	numAreaCache = numPortalCache = 0;
	totalAreaCacheMemory = totalPortalCacheMemory = 0;
	for ( cache = cacheListStart; cache; cache = cache->time_next ) {
		if ( cache->type == CACHETYPE_AREA ) {
			numAreaCache++;
			totalAreaCacheMemory += sizeof( SEASRouteCache ) + cache->size * ( sizeof( unsigned short ) + sizeof( byte ) );
		} else {
			numPortalCache++;
			totalPortalCacheMemory += sizeof( SEASRouteCache ) + cache->size * ( sizeof( unsigned short ) + sizeof( byte ) );
		}
	}

	gameLocal.Printf( "%6d area cache (%d KB)\n", numAreaCache, totalAreaCacheMemory >> 10 );
	gameLocal.Printf( "%6d portal cache (%d KB)\n", numPortalCache, totalPortalCacheMemory >> 10 );
	gameLocal.Printf( "%6d total cache (%d KB)\n", numAreaCache + numPortalCache, totalCacheMemory >> 10 );
	gameLocal.Printf( "%6d area travel times (%d KB)\n", numAreaTravelTimes, ( numAreaTravelTimes * sizeof( unsigned short ) ) >> 10 );
	gameLocal.Printf( "%6d area cache entries (%d KB)\n", areaCacheIndexSize, ( areaCacheIndexSize * sizeof( SEASRouteCache * ) ) >> 10 );
	gameLocal.Printf( "%6d portal cache entries (%d KB)\n", portalCacheIndexSize, ( portalCacheIndexSize * sizeof( SEASRouteCache * ) ) >> 10 );
}

/*
============
anSEASLocal::RemoveRoutingCacheUsingArea
============
*/
void anSEASLocal::RemoveRoutingCacheUsingArea( int areaNum ) {
	int clusterNum;

	clusterNum = file->GetArea( areaNum ).cluster;
	if ( clusterNum > 0 ) {
		// remove all the cache in the cluster the area is in
		DeleteClusterCache( clusterNum );
	} else {
		// if this is a portal remove all cache in both the front and back cluster
		DeleteClusterCache( file->GetPortal( -clusterNum ).clusters[0] );
		DeleteClusterCache( file->GetPortal( -clusterNum ).clusters[1] );
	}
	DeletePortalCache();
}

/*
============
anSEASLocal::DisableArea
============
*/
void anSEASLocal::DisableArea( int areaNum ) {
	assert( areaNum > 0 && areaNum < file->GetNumAreas() );

	if ( file->GetArea( areaNum ).travelFlags & TFL_INVALID ) {
		return;
	}

	file->SetAreaTravelFlag( areaNum, TFL_INVALID );

	RemoveRoutingCacheUsingArea( areaNum );
}

/*
============
anSEASLocal::EnableArea
============
*/
void anSEASLocal::EnableArea( int areaNum ) {
	assert( areaNum > 0 && areaNum < file->GetNumAreas() );

	if ( !( file->GetArea( areaNum ).travelFlags & TFL_INVALID ) ) {
		return;
	}

	file->RemoveAreaTravelFlag( areaNum, TFL_INVALID );

	RemoveRoutingCacheUsingArea( areaNum );
}

/*
============
anSEASLocal::SetAreaState_r
============
*/
bool anSEASLocal::SetAreaState_r( int nodeNum, const anBounds &bounds, const int areaContents, bool disabled ) {
	int res;
	const seasNode_t *node;
	bool foundClusterPortal = false;

	while( nodeNum != 0 ) {
		if ( nodeNum < 0 ) {
			// if this area is a cluster portal
			if ( file->GetArea( -nodeNum ).contents & areaContents ) {
				if ( disabled ) {
					DisableArea( -nodeNum );
				} else {
					EnableArea( -nodeNum );
				}
				foundClusterPortal |= true;
			}
			break;
		}
		node = &file->GetNode( nodeNum );
		res = bounds.PlaneSide( file->GetPlane( node->planeNum ) );
		if ( res == PLANESIDE_BACK ) {
			nodeNum = node->children[1];
		} else if ( res == PLANESIDE_FRONT ) {
			nodeNum = node->children[0];
		} else {
			foundClusterPortal |= SetAreaState_r( node->children[1], bounds, areaContents, disabled );
			nodeNum = node->children[0];
		}
	}

	return foundClusterPortal;
}

/*
============
anSEASLocal::SetAreaState
============
*/
bool anSEASLocal::SetAreaState( const anBounds &bounds, const int areaContents, bool disabled ) {
	anBounds expBounds;

	if ( !file ) {
		return false;
	}

	expBounds[0] = bounds[0] - file->GetSettings().boundingBoxes[0][1];
	expBounds[1] = bounds[1] - file->GetSettings().boundingBoxes[0][0];

	// find all areas within or touching the bounds with the given contents and disable/enable them for routing
	return SetAreaState_r( 1, expBounds, areaContents, disabled );
}

/*
============
anSEASLocal::GetBoundsAreas_r
============
*/
void anSEASLocal::GetBoundsAreas_r( int nodeNum, const anBounds &bounds, anList<int> &areas ) const {
	int res;
	const seasNode_t *node;

	while( nodeNum != 0 ) {
		if ( nodeNum < 0 ) {
			areas.Append( -nodeNum );
			break;
		}
		node = &file->GetNode( nodeNum );
		res = bounds.PlaneSide( file->GetPlane( node->planeNum ) );
		if ( res == PLANESIDE_BACK ) {
			nodeNum = node->children[1];
		} else if ( res == PLANESIDE_FRONT ) {
			nodeNum = node->children[0];
		} else {
			GetBoundsAreas_r( node->children[1], bounds, areas );
			nodeNum = node->children[0];
		}
	}
}

/*
============
anSEASLocal::SetObstacleState
============
*/
void anSEASLocal::SetObstacleState( const SEASRouteObstacle *obstacle, bool enable ) {
	int i;
	const seasArea_t *area;
	anReachability *reach, *rev_reach;
	bool inside;

	for ( i = 0; i < obstacle->areas.Num(); i++ ) {
		RemoveRoutingCacheUsingArea( obstacle->areas[i] );
		area = &file->GetArea( obstacle->areas[i] );
		for ( rev_reach = area->rev_reach; rev_reach; rev_reach = rev_reach->rev_next ) {
			if ( rev_reach->travelType & TFL_INVALID ) {
				continue;
			}

			inside = false;

			if ( obstacle->bounds.ContainsPoint( rev_reach->end ) ) {
				inside = true;
			} else {
				for ( reach = area->reach; reach; reach = reach->next ) {
					if ( obstacle->bounds.LineIntersection( rev_reach->end, reach->start ) ) {
						inside = true;
						break;
					}
				}
			}

			if ( inside ) {
				if ( enable ) {
					rev_reach->disableCount--;
					if ( rev_reach->disableCount <= 0 ) {
						rev_reach->travelType &= ~TFL_INVALID;
						rev_reach->disableCount = 0;
					}
				} else {
					rev_reach->travelType |= TFL_INVALID;
					rev_reach->disableCount++;
				}
			}
		}
	}
}

/*
============
anSEASLocal::SetReachabilityState
============
*/
void anSEASLocal::SetReachabilityState( anReachability *reach, bool enable ) {
 	if ( enable &&  reach->travelType&TFL_INVALID) {
 		reach->disableCount--;
 		if ( reach->disableCount <= 0 ) {
 			reach->travelType &= ~TFL_INVALID;
 			reach->disableCount = 0;
 			RemoveRoutingCacheUsingArea( reach->fromAreaNum );
 		}
 	} else if ( !enable && !( reach->travelType&TFL_INVALID ) ) {
 		reach->travelType |= TFL_INVALID;
 		reach->disableCount++;
 		RemoveRoutingCacheUsingArea( reach->fromAreaNum );
 	}
}

/*
============
anSEASLocal::AddObstacle
============
*/
seasHandle_t anSEASLocal::AddObstacle( const anBounds &bounds ) {
	SEASRouteObstacle *obstacle;

	if ( !file ) {
		return -1;
	}

	obstacle = new SEASRouteObstacle;
	obstacle->bounds[0] = bounds[0] - file->GetSettings().boundingBoxes[0][1];
	obstacle->bounds[1] = bounds[1] - file->GetSettings().boundingBoxes[0][0];
	GetBoundsAreas_r( 1, obstacle->bounds, obstacle->areas );
	SetObstacleState( obstacle, true );

	obstacleList.Append( obstacle );
	return obstacleList.Num() - 1;
}

/*
============
anSEASLocal::RemoveObstacle
============
*/
void anSEASLocal::RemoveObstacle( const seasHandle_t handle ) {
	if ( !file ) {
		return;
	}
	if ( ( handle >= 0 ) && ( handle < obstacleList.Num() ) ) {
		SetObstacleState( obstacleList[handle], false );
		delete obstacleList[handle];
		obstacleList.RemoveIndex( handle );
	}
}

/*
============
anSEASLocal::RemoveAllObstacles
============
*/
void anSEASLocal::RemoveAllObstacles( void ) {
	if ( !file ) {
		return;
	}

	for ( int i = 0; i < obstacleList.Num(); i++ ) {
		SetObstacleState( obstacleList[i], false );
		delete obstacleList[i];
	}
	obstacleList.Clear();
}

/*
============
anSEASLocal::LinkCache

  link the cache in the cache list sorted from oldest to newest cache
============
*/
void anSEASLocal::LinkCache( SEASRouteCache *cache ) const {
	// if the cache is already linked
	if ( cache->time_next || cache->time_prev || cacheListStart == cache ) {
		UnlinkCache( cache );
	}

	totalCacheMemory += cache->Size();

	// add cache to the end of the list
	cache->time_next = nullptr;
	cache->time_prev = cacheListEnd;
	if ( cacheListEnd ) {
		cacheListEnd->time_next = cache;
	}
	cacheListEnd = cache;
	if ( !cacheListStart ) {
		cacheListStart = cache;
	}
}

/*
============
anSEASLocal::UnlinkCache
============
*/
void anSEASLocal::UnlinkCache( SEASRouteCache *cache ) const {
	totalCacheMemory -= cache->Size();

	// unlink the cache
	if ( cache->time_next ) {
		cache->time_next->time_prev = cache->time_prev;
	} else {
		cacheListEnd = cache->time_prev;
	}
	if ( cache->time_prev ) {
		cache->time_prev->time_next = cache->time_next;
	} else {
		cacheListStart = cache->time_next;
	}
	cache->time_next = cache->time_prev = nullptr;
}

/*
============
anSEASLocal::DeleteOldestCache
============
*/
void anSEASLocal::DeleteOldestCache( void ) const {
	SEASRouteCache *cache;

	assert( cacheListStart );

	// unlink the oldest cache
	cache = cacheListStart;
	UnlinkCache( cache );

	// unlink the oldest cache from the area or portal cache index
	if ( cache->next ) {
		cache->next->prev = cache->prev;
	}
	if ( cache->prev ) {
		cache->prev->next = cache->next;
	} else if ( cache->type == CACHETYPE_AREA ) {
		areaCacheIndex[cache->cluster][ClusterAreaNum( cache->cluster, cache->areaNum )] = cache->next;
	} else if ( cache->type == CACHETYPE_PORTAL ) {
		portalCacheIndex[cache->areaNum] = cache->next;
	}

	delete cache;
}

/*
============
anSEASLocal::GetAreaReachability
============
*/
anReachability *anSEASLocal::GetAreaReachability( int areaNum, int reachabilityNum ) const {
	anReachability *reach;

	for ( reach = file->GetArea( areaNum ).reach; reach; reach = reach->next ) {
		if ( --reachabilityNum < 0 ) {
			return reach;
		}
	}
	return nullptr;
}

/*
============
anSEASLocal::ClusterAreaNum
============
*/
inline int anSEASLocal::ClusterAreaNum( int clusterNum, int areaNum ) const {
	int side, areaCluster;

	areaCluster = file->GetArea( areaNum ).cluster;
	if ( areaCluster > 0 ) {
		return file->GetArea( areaNum ).clusterAreaNum;
	} else {
		side = file->GetPortal( -areaCluster ).clusters[0] != clusterNum;
		return file->GetPortal( -areaCluster ).clusterAreaNum[side];
	}
}

/*
============
anSEASLocal::UpdateAreaRoutingCache
============
*/
void anSEASLocal::UpdateAreaRoutingCache( SEASRouteCache *areaCache ) const {
	int i, nextAreaNum, cluster, badTravelFlags, clusterAreaNum, numReachableAreas;
	unsigned short t, startAreaTravelTimes[MAX_REACH_PER_AREA];
	SEASRouteUpdate *updateListStart, *updateListEnd, *curUpdate, *nextUpdate;
	anReachability *reach;
	const seasArea_t *nextArea;

	// number of reachability areas within this cluster
	numReachableAreas = file->GetCluster( areaCache->cluster ).numReachableAreas;

	// number of the start area within the cluster
	clusterAreaNum = ClusterAreaNum( areaCache->cluster, areaCache->areaNum );
	if ( clusterAreaNum >= numReachableAreas ) {
		return;
	}

	areaCache->travelTimes[clusterAreaNum] = areaCache->startTravelTime;
	badTravelFlags = ~areaCache->travelFlags;
	memset( startAreaTravelTimes, 0, sizeof( startAreaTravelTimes ) );

	// initialize first update
	curUpdate = &areaUpdate[clusterAreaNum];
	curUpdate->areaNum = areaCache->areaNum;
	curUpdate->areaTravelTimes = startAreaTravelTimes;
	curUpdate->tmpTravelTime = areaCache->startTravelTime;
	curUpdate->next = nullptr;
	curUpdate->prev = nullptr;
	updateListStart = curUpdate;
	updateListEnd = curUpdate;

	// while there are updates in the list
	while( updateListStart ) {
		curUpdate = updateListStart;
		if ( curUpdate->next ) {
			curUpdate->next->prev = nullptr;
		} else {
			updateListEnd = nullptr;
		}
		updateListStart = curUpdate->next;

		curUpdate->isInList = false;

		for ( i = 0, reach = file->GetArea( curUpdate->areaNum ).rev_reach; reach; reach = reach->rev_next, i++ ) {
			// if the reachability uses an undesired travel type
			if ( reach->travelType & badTravelFlags ) {
				continue;
			}

			// next area the reversed reachability leads to
			nextAreaNum = reach->fromAreaNum;
			nextArea = &file->GetArea( nextAreaNum );

			// if traveling through the next area requires an undesired travel flag
			if ( nextArea->travelFlags & badTravelFlags ) {
				continue;
			}

			// get the cluster number of the area
			cluster = nextArea->cluster;
			// don't leave the cluster, however do flood into cluster portals
			if ( cluster > 0 && cluster != areaCache->cluster ) {
				continue;
			}

			// get the number of the area in the cluster
			clusterAreaNum = ClusterAreaNum( areaCache->cluster, nextAreaNum );
			if ( clusterAreaNum >= numReachableAreas ) {
				continue;	// should never happen
			}

			assert( clusterAreaNum < areaCache->size );

			// time already travelled plus the traveltime through the current area
			// plus the travel time of the reachability towards the next area
			t = curUpdate->tmpTravelTime + curUpdate->areaTravelTimes[i] + reach->travelTime;

			if ( !areaCache->travelTimes[clusterAreaNum] || t < areaCache->travelTimes[clusterAreaNum] ) {

				areaCache->travelTimes[clusterAreaNum] = t;
				areaCache->reachabilities[clusterAreaNum] = reach->number; // reversed reachability used to get into this area
				nextUpdate = &areaUpdate[clusterAreaNum];
				nextUpdate->areaNum = nextAreaNum;
				nextUpdate->tmpTravelTime = t;
				nextUpdate->areaTravelTimes = reach->areaTravelTimes;

				// if we are not allowed to fly
				if ( badTravelFlags & TFL_FLY ) {
					// avoid areas near ledges
					if ( file->GetArea( nextAreaNum ).flags & AREA_LEDGE ) {
						nextUpdate->tmpTravelTime += LEDGE_TRAVELTIME_PANALTY;
					}
				}

				if ( !nextUpdate->isInList ) {
					nextUpdate->next = nullptr;
					nextUpdate->prev = updateListEnd;
					if ( updateListEnd ) {
						updateListEnd->next = nextUpdate;
					}
					else {
						updateListStart = nextUpdate;
					}
					updateListEnd = nextUpdate;
					nextUpdate->isInList = true;
				}
			}
		}
	}
}

/*
============
anSEASLocal::GetAreaRoutingCache
============
*/
SEASRouteCache *anSEASLocal::GetAreaRoutingCache( int clusterNum, int areaNum, int travelFlags ) const {
	int clusterAreaNum;
	SEASRouteCache *cache, *clusterCache;

	// number of the area in the cluster
	clusterAreaNum = ClusterAreaNum( clusterNum, areaNum );
	// pointer to the cache for the area in the cluster
	clusterCache = areaCacheIndex[clusterNum][clusterAreaNum];
	// check if cache without undesired travel flags already exists
	for ( cache = clusterCache; cache; cache = cache->next ) {
		if ( cache->travelFlags == travelFlags ) {
			break;
		}
	}
	// if no cache found
	if ( !cache ) {
		cache = new SEASRouteCache( file->GetCluster( clusterNum ).numReachableAreas );
		cache->type = CACHETYPE_AREA;
		cache->cluster = clusterNum;
		cache->areaNum = areaNum;
		cache->startTravelTime = 1;
		cache->travelFlags = travelFlags;
		cache->prev = nullptr;
		cache->next = clusterCache;
		if ( clusterCache ) {
			clusterCache->prev = cache;
		}
		areaCacheIndex[clusterNum][clusterAreaNum] = cache;
		UpdateAreaRoutingCache( cache );
	}
	LinkCache( cache );
	return cache;
}

/*
============
anSEASLocal::UpdatePortalRoutingCache
============
*/
void anSEASLocal::UpdatePortalRoutingCache( SEASRouteCache *portalCache ) const {
	int i, portalNum, clusterAreaNum;
	unsigned short t;
	const seasPortal_t *portal;
	const seasCluster_t *cluster;
	SEASRouteCache *cache;
	SEASRouteUpdate *updateListStart, *updateListEnd, *curUpdate, *nextUpdate;

	curUpdate = &portalUpdate[ file->GetNumPortals() ];
	curUpdate->cluster = portalCache->cluster;
	curUpdate->areaNum = portalCache->areaNum;
	curUpdate->tmpTravelTime = portalCache->startTravelTime;

	//put the area to start with in the current read list
	curUpdate->next = nullptr;
	curUpdate->prev = nullptr;
	updateListStart = curUpdate;
	updateListEnd = curUpdate;

	// while there are updates in the current list
	while( updateListStart ) {
		curUpdate = updateListStart;
		// remove the current update from the list
		if ( curUpdate->next ) {
			curUpdate->next->prev = nullptr;
		} else {
			updateListEnd = nullptr;
		}
		updateListStart = curUpdate->next;
		// current update is removed from the list
		curUpdate->isInList = false;

		cluster = &file->GetCluster( curUpdate->cluster );
		cache = GetAreaRoutingCache( curUpdate->cluster, curUpdate->areaNum, portalCache->travelFlags );

		// take all portals of the cluster
		for ( i = 0; i < cluster->numPortals; i++ ) {
			portalNum = file->GetPortalIndex( cluster->firstPortal + i );
			assert( portalNum < portalCache->size );
			portal = &file->GetPortal( portalNum );

			clusterAreaNum = ClusterAreaNum( curUpdate->cluster, portal->areaNum );
			if ( clusterAreaNum >= cluster->numReachableAreas ) {
				continue;
			}

			t = cache->travelTimes[clusterAreaNum];
			if ( t == 0 ) {
				continue;
			}
			t += curUpdate->tmpTravelTime;

			if ( !portalCache->travelTimes[portalNum] || t < portalCache->travelTimes[portalNum] ) {
				portalCache->travelTimes[portalNum] = t;
				portalCache->reachabilities[portalNum] = cache->reachabilities[clusterAreaNum];
				nextUpdate = &portalUpdate[portalNum];
				if ( portal->clusters[0] == curUpdate->cluster ) {
					nextUpdate->cluster = portal->clusters[1];
				} else {
					nextUpdate->cluster = portal->clusters[0];
				}
				nextUpdate->areaNum = portal->areaNum;
				// add travel time through the actual portal area for the next update
				nextUpdate->tmpTravelTime = t + portal->maxAreaTravelTime;

				if ( !nextUpdate->isInList ) {
					nextUpdate->next = nullptr;
					nextUpdate->prev = updateListEnd;
					if ( updateListEnd ) {
						updateListEnd->next = nextUpdate;
					} else {
						updateListStart = nextUpdate;
					}
					updateListEnd = nextUpdate;
					nextUpdate->isInList = true;
				}
			}
		}
	}
}

/*
============
anSEASLocal::GetPortalRoutingCache
============
*/
SEASRouteCache *anSEASLocal::GetPortalRoutingCache( int clusterNum, int areaNum, int travelFlags ) const {
	SEASRouteCache *cache;

	// check if cache without undesired travel flags already exists
	for ( cache = portalCacheIndex[areaNum]; cache; cache = cache->next ) {
		if ( cache->travelFlags == travelFlags ) {
			break;
		}
	}
	// if no cache found
	if ( !cache ) {
		cache = new SEASRouteCache( file->GetNumPortals() );
		cache->type = CACHETYPE_PORTAL;
		cache->cluster = clusterNum;
		cache->areaNum = areaNum;
		cache->startTravelTime = 1;
		cache->travelFlags = travelFlags;
		cache->prev = nullptr;
		cache->next = portalCacheIndex[areaNum];
		if ( portalCacheIndex[areaNum] ) {
			portalCacheIndex[areaNum]->prev = cache;
		}
		portalCacheIndex[areaNum] = cache;
		UpdatePortalRoutingCache( cache );
	}
	LinkCache( cache );
	return cache;
}

/*
============
anSEASLocal::RouteToGoalArea
============
*/
bool anSEASLocal::RouteToGoalArea( int areaNum, const anVec3 origin, int goalAreaNum, int travelFlags, int &travelTime, anReachability **reach ) const {
	int clusterNum, goalClusterNum, portalNum, i, clusterAreaNum;
	unsigned short int t, bestTime;
	const seasPortal_t *portal;
	const seasCluster_t *cluster;
	SEASRouteCache *areaCache, *portalCache, *clusterCache;
	anReachability *bestReach, *r, *nextr;

	travelTime = 0;
	*reach = nullptr;

	if ( !file ) {
		return false;
	}

	if ( areaNum == goalAreaNum ) {
		return true;
	}

	if ( areaNum <= 0 || areaNum >= file->GetNumAreas() ) {
		gameLocal.Printf( "RouteToGoalArea: areaNum %d out of range\n", areaNum );
		return false;
	}
	if ( goalAreaNum <= 0 || goalAreaNum >= file->GetNumAreas() ) {
		gameLocal.Printf( "RouteToGoalArea: goalAreaNum %d out of range\n", goalAreaNum );
		return false;
	}

	while( totalCacheMemory > MAX_ROUTING_CACHE_MEMORY ) {
		DeleteOldestCache();
	}

	clusterNum = file->GetArea( areaNum ).cluster;
	goalClusterNum = file->GetArea( goalAreaNum ).cluster;

	// if the source area is a cluster portal, read directly from the portal cache
	if ( clusterNum < 0 ) {
		// if the goal area is a portal
		if ( goalClusterNum < 0 ) {
			// just assume the goal area is part of the front cluster
			portal = &file->GetPortal( -goalClusterNum );
			goalClusterNum = portal->clusters[0];
		}
		// get the portal routing cache
		portalCache = GetPortalRoutingCache( goalClusterNum, goalAreaNum, travelFlags );
		*reach = GetAreaReachability( areaNum, portalCache->reachabilities[-clusterNum] );
		travelTime = portalCache->travelTimes[-clusterNum] + AreaTravelTime( areaNum, origin, (*reach)->start );
		return true;
	}

	bestTime = 0;
	bestReach = nullptr;

	// check if the goal area is a portal of the source area cluster
	if ( goalClusterNum < 0 ) {
		portal = &file->GetPortal( -goalClusterNum );
		if ( portal->clusters[0] == clusterNum || portal->clusters[1] == clusterNum) {
			goalClusterNum = clusterNum;
		}
	}

	// if both areas are in the same cluster
	if ( clusterNum > 0 && goalClusterNum > 0 && clusterNum == goalClusterNum ) {
		clusterCache = GetAreaRoutingCache( clusterNum, goalAreaNum, travelFlags );
		clusterAreaNum = ClusterAreaNum( clusterNum, areaNum );
		if ( clusterCache->travelTimes[clusterAreaNum] ) {
			bestReach = GetAreaReachability( areaNum, clusterCache->reachabilities[clusterAreaNum] );
			bestTime = clusterCache->travelTimes[clusterAreaNum] + AreaTravelTime( areaNum, origin, bestReach->start );
		} else {
			clusterCache = nullptr;
		}
	} else {
		clusterCache = nullptr;
	}

	clusterNum = file->GetArea( areaNum ).cluster;
	goalClusterNum = file->GetArea( goalAreaNum ).cluster;

	// if the goal area is a portal
	if ( goalClusterNum < 0 ) {
		// just assume the goal area is part of the front cluster
		portal = &file->GetPortal( -goalClusterNum );
		goalClusterNum = portal->clusters[0];
	}
	// get the portal routing cache
	portalCache = GetPortalRoutingCache( goalClusterNum, goalAreaNum, travelFlags );

	// the cluster the area is in
	cluster = &file->GetCluster( clusterNum );
	// current area inside the current cluster
	clusterAreaNum = ClusterAreaNum( clusterNum, areaNum );
	// if the area is not a reachable area
	if ( clusterAreaNum >= cluster->numReachableAreas) {
		return false;
	}

	// find the portal of the source area cluster leading towards the goal area
	for ( i = 0; i < cluster->numPortals; i++ ) {
		portalNum = file->GetPortalIndex( cluster->firstPortal + i );
		// if the goal area isn't reachable from the portal
		if ( !portalCache->travelTimes[portalNum] ) {
			continue;
		}

		portal = &file->GetPortal( portalNum );
		// get the cache of the portal area
		areaCache = GetAreaRoutingCache( clusterNum, portal->areaNum, travelFlags );
		// if the portal is not reachable from this area
		if ( !areaCache->travelTimes[clusterAreaNum] ) {
			continue;
		}

		r = GetAreaReachability( areaNum, areaCache->reachabilities[clusterAreaNum] );

		if ( clusterCache ) {
			// if the next reachability from the portal leads back into the cluster
			nextr = GetAreaReachability( portal->areaNum, portalCache->reachabilities[portalNum] );
			if ( file->GetArea( nextr->toAreaNum ).cluster < 0 || file->GetArea( nextr->toAreaNum ).cluster == clusterNum ) {
				continue;
			}
		}

		// the total travel time is the travel time from the portal area to the goal area
		// plus the travel time from the source area towards the portal area
		t = portalCache->travelTimes[portalNum] + areaCache->travelTimes[clusterAreaNum];
		// NOTE:	Should add the exact travel time through the portal area.
		//			However we add the largest travel time through the portal area.
		//			We cannot directly calculate the exact travel time through the portal area
		//			because the reachability used to travel into the portal area is not known.
		t += portal->maxAreaTravelTime;

		// if the time is better than the one already found
		if ( !bestTime || t < bestTime ) {
			bestReach = r;
			bestTime = t;
		}
	}

	if ( !bestReach ) {
		return false;
	}

	*reach = bestReach;
	travelTime = bestTime;

	return true;
}

/*
============
anSEASLocal::TravelTimeToGoalArea
============
*/
int anSEASLocal::TravelTimeToGoalArea( int areaNum, const anVec3 &origin, int goalAreaNum, int travelFlags ) const {
	int travelTime;
	anReachability *reach;

	if ( !file ) {
		return 0;
	}

	if ( !RouteToGoalArea( areaNum, origin, goalAreaNum, travelFlags, travelTime, &reach ) ) {
		return 0;
	}
	return travelTime;
}

/*
============
anSEASLocal::FindNearestGoal
============
*/
bool anSEASLocal::FindNearestGoal( seasGoal_t &goal, int areaNum, const anVec3 origin, const anVec3 &target, int travelFlags, float minDistance, float maxDistance, seasObstructs_t *obstacles, int numObstacles, anSEASCallback &callback ) const {
	int i, j, k, badTravelFlags, nextAreaNum;
	seasGoal_t			bestGoal;
	unsigned short t, bestTravelTime;
	SEASRouteUpdate *updateListStart, *updateListEnd, *curUpdate, *nextUpdate;
	anReachability *reach;
	const seasArea_t *nextArea;
	anVec3 v1, v2, p;
	float targetDist, dist;

	if ( file == nullptr || areaNum <= 0 ) {
		goal.areaNum = areaNum;
		goal.origin = origin;
		return false;
	}

	// setup obstacles
	for ( k = 0; k < numObstacles; k++ ) {
		obstacles[k].expAbsBounds[0] = obstacles[k].absBounds[0] - file->GetSettings().boundingBoxes[0][1];
		obstacles[k].expAbsBounds[1] = obstacles[k].absBounds[1] - file->GetSettings().boundingBoxes[0][0];
	}

	badTravelFlags = ~travelFlags;
	SIMDProcessor->Memset( goalAreaTravelTimes, 0, file->GetNumAreas() * sizeof( unsigned short ) );

	targetDist = (target - origin).Length();

	// initialize first update
	curUpdate = &areaUpdate[areaNum];
	curUpdate->areaNum = areaNum;
	curUpdate->tmpTravelTime = 0;
	curUpdate->start = origin;
	curUpdate->next = nullptr;
	curUpdate->prev = nullptr;

	callback.Init();

	// if the first area is valid goal, just return the origin
	curUpdate->cluster = (int)callback.Test ( (anSEASLocal*)this, areaNum, origin, minDistance, maxDistance, &origin, goal );
	if ( curUpdate->cluster == anSEASCallback::TEST_OK ) {
		callback.Finish();
		return true;
	}

	updateListStart = curUpdate;
	updateListEnd = curUpdate;

	bestTravelTime = 0;
	bestGoal.areaNum = 0;

	// while there are updates in the list
	while ( updateListStart ) {
		curUpdate = updateListStart;
		if ( curUpdate->next ) {
			curUpdate->next->prev = nullptr;
		} else {
			updateListEnd = nullptr;
		}
		updateListStart = curUpdate->next;

		curUpdate->isInList = false;

		// if we already found a closer location
		if ( bestTravelTime && curUpdate->tmpTravelTime >= bestTravelTime ) {
			continue;
		}

		for ( i = 0, reach = file->GetArea( curUpdate->areaNum ).reach; reach; reach = reach->next, i++ ) {
			// if the reachability uses an undesired travel type
			if ( reach->travelType & badTravelFlags ) {
				continue;
			}

			// next area the reversed reachability leads to
			nextAreaNum = reach->toAreaNum;
			nextArea = &file->GetArea( nextAreaNum );

			// if traveling through the next area requires an undesired travel flag
			if ( nextArea->travelFlags & badTravelFlags ) {
				continue;
			}

			t = curUpdate->tmpTravelTime + AreaTravelTime( curUpdate->areaNum, curUpdate->start, reach->start ) +
						reach->travelTime;

			// project target origin onto movement vector through the area
			v1 = reach->end - curUpdate->start;
			v1.Normalize();
			v2 = target - curUpdate->start;
			p = curUpdate->start + (v2 * v1) * v1;

			// get the point on the path closest to the target
			for ( j = 0; j < 3; j++ ) {
				if ( (p[j] > curUpdate->start[j] + 0.1f && p[j] > reach->end[j] + 0.1f) ||
					(p[j] < curUpdate->start[j] - 0.1f && p[j] < reach->end[j] - 0.1f) ) {
					break;
				}
			}
			if ( j >= 3 ) {
				dist = (target - p).Length();
			} else {
				dist = (target - reach->end).Length();
			}

			// avoid moving closer to the target
			if ( dist < targetDist ) {
				t += ( targetDist - dist ) * 10;
			}

			// if we already found a closer location
			if ( bestTravelTime && t >= bestTravelTime ) {
				continue;
			}

			// if this is not the best path towards the next area
			if ( goalAreaTravelTimes[nextAreaNum] && t >= goalAreaTravelTimes[nextAreaNum] ) {
				continue;
			}

			// path may not go through any obstacles
			for ( k = 0; k < numObstacles; k++ ) {
				// If the start of the movement vector is inside the expanded bounds then we are already too
				// close to the obstacle, so use its unexpanded bounds instead.
				if ( obstacles[k].expAbsBounds.ContainsPoint ( curUpdate->start ) ) {
					if ( obstacles[k].absBounds.LineIntersection( curUpdate->start, reach->end ) ) {
						break;
					}
				// if the movement vector intersects the expanded obstacle bounds
				} else if ( obstacles[k].expAbsBounds.LineIntersection( curUpdate->start, reach->end ) ) {
					break;
				}
			}
			if ( k < numObstacles ) {
				continue;
			}

			goalAreaTravelTimes[nextAreaNum] = t;
			nextUpdate = &areaUpdate[nextAreaNum];
			nextUpdate->areaNum = nextAreaNum;
			nextUpdate->tmpTravelTime = t;
			nextUpdate->start = reach->end;

			// if we are not allowed to fly
			if ( badTravelFlags & TFL_FLY ) {
				// avoid areas near ledges
				if ( file->GetArea( nextAreaNum ).flags & AREA_LEDGE ) {
					nextUpdate->tmpTravelTime += LEDGE_TRAVELTIME_PANALTY;
				}
			}

			// If outside of max distance skip this area
			anVec3 point = origin;
			float  areaDist;
			file->PushPointIntoAreaNum ( nextAreaNum, point );
			areaDist = (origin-point).LengthFast();
			if ( maxDistance > 0.0f && areaDist > maxDistance ) {
				curUpdate->cluster = anSEASCallback::TEST_BADAREA;
				continue;
			}

			// don't put goal near a ledge
			if ( !( nextArea->flags & AREA_LEDGE ) ) {
				// add travel time through the area
				t += AreaTravelTime( reach->toAreaNum, reach->end, nextArea->center );
				if ( !bestTravelTime || t < bestTravelTime ) {
					// if the area is not visible to the target
					nextUpdate->cluster = (int)callback.Test ( (anSEASLocal*)this, reach->toAreaNum, origin, minDistance, maxDistance, nullptr, bestGoal );
					switch ( nextUpdate->cluster ) {
						case anSEASCallback::TEST_OK:
							bestTravelTime = t;
							break;
						case anSEASCallback::TEST_BADAREA:
							if ( curUpdate->cluster != anSEASCallback::TEST_BADAREA ) {
								continue;
							}
							break;
					}
				}
			}

			if ( !nextUpdate->isInList ) {
				nextUpdate->next = nullptr;
				nextUpdate->prev = updateListEnd;
				if ( updateListEnd ) {
					updateListEnd->next = nextUpdate;
				} else {
					updateListStart = nextUpdate;
				}
				updateListEnd = nextUpdate;
				nextUpdate->isInList = true;
			}
		}
	}

	callback.Finish();

	if ( bestGoal.areaNum ) {
		goal = bestGoal;
		return true;
	}

	return false;
}
