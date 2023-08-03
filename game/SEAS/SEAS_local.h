
#ifndef __SEAS_LOCAL_H__
#define __SEAS_LOCAL_H__

#include "SEAS.h"
#include "../Pvs.h"


class SEASRouteCache {
	friend class anSEASLocal;

public:
								SEASRouteCache( int size );
								~SEASRouteCache( void );

	int							Size( void ) const;

private:
	int							type;					// portal or area cache
	int							size;					// size of cache
	int							cluster;				// cluster of the cache
	int							areaNum;				// area of the cache
	int							travelFlags;			// combinations of the travel flags
	SEASRouteCache *			next;					// next in list
	SEASRouteCache *			prev;					// previous in list
	SEASRouteCache *			time_next;				// next in time based list
	SEASRouteCache *			time_prev;				// previous in time based list
	unsigned short				startTravelTime;		// travel time to start with
	unsigned char *				reachabilities;			// reachabilities used for routing
	unsigned short *			travelTimes;			// travel time for every area
};


class SEASRouteUpdate {
	friend class anSEASLocal;

private:
	int							cluster;				// cluster number of this update
	int							areaNum;				// area number of this update
	unsigned short				tmpTravelTime;			// temporary travel time
	unsigned short *			areaTravelTimes;		// travel times within the area
	anVec3						start;					// start point into area
	SEASRouteUpdate *			next;					// next in list
	SEASRouteUpdate *			prev;					// prev in list
	bool						isInList;				// true if the update is in the list
};


class SEASRouteObstacle {
	friend class anSEASLocal;
								SEASRouteObstacle( void ) { }

private:
	anBounds					bounds;					// obstacle bounds
	anList<int>					areas;					// areas the bounds are in
};

class anSEASLocal : public anSEAS {
public:
								anSEASLocal( void );
	virtual						~anSEASLocal( void );
	virtual bool				Init( const anString &mapName, unsigned int mapFileCRC );
	virtual void				Shutdown( void );

	virtual size_t				StatsSummary( void ) const;

	virtual void				Stats( void ) const;
	virtual void				Test( const anVec3 &origin );
	virtual const anSEASSettings *GetSettings( void ) const;
	virtual int					PointAreaNum( const anVec3 &origin ) const;
	virtual int					PointReachableAreaNum( const anVec3 &origin, const anBounds &searchBounds, const int areaFlags ) const;
	virtual int					BoundsReachableAreaNum( const anBounds &bounds, const int areaFlags ) const;
	virtual void				PushPointIntoAreaNum( int areaNum, anVec3 &origin ) const;
	virtual anVec3				AreaCenter( int areaNum ) const;

	virtual float				AreaRadius( int areaNum ) const;
	virtual anBounds &			AreaBounds( int areaNum ) const;
	virtual float				AreaCeiling( int areaNum ) const;

	virtual int					AreaFlags( int areaNum ) const;
	virtual int					AreaTravelFlags( int areaNum ) const;
	virtual bool				Trace( seasTrace_t &trace, const anVec3 &start, const anVec3 &end ) const;
	virtual const anPlane &		GetPlane( int planeNum ) const;
	virtual int					GetWallEdges( int areaNum, const anBounds &bounds, int travelFlags, int *edges, int maxEdges ) const;
	virtual void				SortWallEdges( int *edges, int numEdges ) const;
	virtual void				GetEdgeVertexNumbers( int edgeNum, int verts[2] ) const;
	virtual void				GetEdge( int edgeNum, anVec3 &start, anVec3 &end ) const;
	virtual bool				SetAreaState( const anBounds &bounds, const int areaContents, bool disabled );
	virtual seasHandle_t		AddObstacle( const anBounds &bounds );
	virtual void				RemoveObstacle( const seasHandle_t handle );
	virtual void				RemoveAllObstacles( void );
	virtual int					TravelTimeToGoalArea( int areaNum, const anVec3 &origin, int goalAreaNum, int travelFlags ) const;
	virtual bool				RouteToGoalArea( int areaNum, const anVec3 origin, int goalAreaNum, int travelFlags, int &travelTime, anReachability **reach ) const;
	virtual bool				WalkPathToGoal( seasPath_t &path, int areaNum, const anVec3 &origin, int goalAreaNum, const anVec3 &goalOrigin, int travelFlags ) const;
	virtual bool				WalkPathValid( int areaNum, const anVec3 &origin, int goalAreaNum, const anVec3 &goalOrigin, int travelFlags, anVec3 &endPos, int &endAreaNum ) const;
	virtual bool				FlyPathToGoal( seasPath_t &path, int areaNum, const anVec3 &origin, int goalAreaNum, const anVec3 &goalOrigin, int travelFlags ) const;
	virtual bool				FlyPathValid( int areaNum, const anVec3 &origin, int goalAreaNum, const anVec3 &goalOrigin, int travelFlags, anVec3 &endPos, int &endAreaNum ) const;
	virtual void				ShowWalkPath( const anVec3 &origin, int goalAreaNum, const anVec3 &goalOrigin ) const;
	virtual void				ShowFlyPath( const anVec3 &origin, int goalAreaNum, const anVec3 &goalOrigin ) const;
	virtual bool				FindNearestGoal( seasGoal_t &goal, int areaNum, const anVec3 origin, const anVec3 &target, int travelFlags, float minDistance, float maxDistance, seasObstructs_t *obstacles, int numObstacles, anSEASCallback &callback ) const;
// Added Area Wall Extraction For SEASTactical
	virtual anSEASFile*			GetFile( void ) {return file;}
	 virtual void				SetReachabilityState( anReachability* reach, bool enable );
	virtual bool				IsValid( void ) const;
	virtual void				ShowAreas( const anVec3 &origin, bool ShowProblemAreas = false ) const;


private:
	anSEASFile *					file;
	anString						name;

private:	// routing data
	SEASRouteCache ***			areaCacheIndex;			// for each area in each cluster the travel times to all other areas in the cluster
	int							areaCacheIndexSize;		// number of area cache entries
	SEASRouteCache **			portalCacheIndex;		// for each area in the world the travel times from each portal
	int							portalCacheIndexSize;	// number of portal cache entries
	SEASRouteUpdate *			areaUpdate;				// memory used to update the area routing cache
	SEASRouteUpdate *			portalUpdate;			// memory used to update the portal routing cache
	unsigned short *			goalAreaTravelTimes;	// travel times to goal areas
	unsigned short *			areaTravelTimes;		// travel times through the areas
	int							numAreaTravelTimes;		// number of area travel times
	mutable SEASRouteCache *	cacheListStart;			// start of list with cache sorted from oldest to newest
	mutable SEASRouteCache *	cacheListEnd;			// end of list with cache sorted from oldest to newest
	mutable int					totalCacheMemory;		// total cache memory used
	anList<SEASRouteObstacle *>	obstacleList;			// list with obstacles

private:	// routing
	bool						SetupRouting( void );
	void						ShutdownRouting( void );
	unsigned short				AreaTravelTime( int areaNum, const anVec3 &start, const anVec3 &end ) const;
	void						CalculateAreaTravelTimes( void );
	void						DeleteAreaTravelTimes( void );
	void						SetupRoutingCache( void );
	void						DeleteClusterCache( int clusterNum );
	void						DeletePortalCache( void );
	void						ShutdownRoutingCache( void );
	void						RoutingStats( void ) const;
	void						LinkCache( SEASRouteCache *cache ) const;
	void						UnlinkCache( SEASRouteCache *cache ) const;
	void						DeleteOldestCache( void ) const;
	anReachability *			GetAreaReachability( int areaNum, int reachabilityNum ) const;
	int							ClusterAreaNum( int clusterNum, int areaNum ) const;
	void						UpdateAreaRoutingCache( SEASRouteCache *areaCache ) const;
	SEASRouteCache *			GetAreaRoutingCache( int clusterNum, int areaNum, int travelFlags ) const;
	void						UpdatePortalRoutingCache( SEASRouteCache *portalCache ) const;
	SEASRouteCache *			GetPortalRoutingCache( int clusterNum, int areaNum, int travelFlags ) const;
	void						RemoveRoutingCacheUsingArea( int areaNum );
	void						DisableArea( int areaNum );
	void						EnableArea( int areaNum );
	bool						SetAreaState_r( int nodeNum, const anBounds &bounds, const int areaContents, bool disabled );
	void						GetBoundsAreas_r( int nodeNum, const anBounds &bounds, anList<int> &areas ) const;
	void						SetObstacleState( const SEASRouteObstacle *obstacle, bool enable );

private:	// pathing
	bool						EdgeSplitPoint( anVec3 &split, int edgeNum, const anPlane &plane ) const;
	bool						FloorEdgeSplitPoint( anVec3 &split, int areaNum, const anPlane &splitPlane, const anPlane &frontPlane, bool closest ) const;
	anVec3						SubSampleWalkPath( int areaNum, const anVec3 &origin, const anVec3 &start, const anVec3 &end, int travelFlags, int &endAreaNum ) const;
	anVec3						SubSampleFlyPath( int areaNum, const anVec3 &origin, const anVec3 &start, const anVec3 &end, int travelFlags, int &endAreaNum ) const;

private:	// debug
	const anBounds &			DefaultSearchBounds( void ) const;
	void						DrawCone( const anVec3 &origin, const anVec3 &dir, float radius, const anVec4 &color ) const;
	void						DrawAreaBounds( int areaNum ) const;
	void						DrawArea( int areaNum ) const;
	void						DrawFace( int faceNum, bool side ) const;
	void						DrawEdge( int edgeNum, bool arrow ) const;
	void						DrawReachability( const anReachability *reach ) const;
	void						ShowArea( const anVec3 &origin ) const;
	void						ShowWallEdges( const anVec3 &origin ) const;
	void						ShowHideArea( const anVec3 &origin, int targerAreaNum ) const;
	bool						PullPlayer( const anVec3 &origin, int toAreaNum ) const;
	void						RandomPullPlayer( const anVec3 &origin ) const;
	void						ShowPushIntoArea( const anVec3 &origin ) const;

	void						DrawSimpleEdge( int edgeNum ) const;
	void						DrawSimpleFace( int faceNum, bool visited ) const;
	void						DrawSimpleArea( int areaNum ) const;
	void						ShowProblemEdge( int edgeNum ) const;
	void						ShowProblemFace( int faceNum ) const;
	void						ShowProblemArea( int areaNum ) const;
	void						ShowProblemArea( const anVec3 &origin ) const;
};

#endif // !__SEAS_LOCAL_H__
