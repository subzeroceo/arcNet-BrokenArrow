
#ifndef __AAS_LOCAL_H__
#define __AAS_LOCAL_H__

#include "AAS.h"
#include "../Pvs.h"


class idRoutingCache {
	friend class idAASLocal;

public:
								idRoutingCache( int size );
								~idRoutingCache( void );

	int							Size( void ) const;

private:
	int							type;					// portal or area cache
	int							size;					// size of cache
	int							cluster;				// cluster of the cache
	int							areaNum;				// area of the cache
	int							travelFlags;			// combinations of the travel flags
	idRoutingCache *			next;					// next in list
	idRoutingCache *			prev;					// previous in list
	idRoutingCache *			time_next;				// next in time based list
	idRoutingCache *			time_prev;				// previous in time based list
	unsigned short				startTravelTime;		// travel time to start with
	unsigned char *				reachabilities;			// reachabilities used for routing
	unsigned short *			travelTimes;			// travel time for every area
};


class idRoutingUpdate {
	friend class idAASLocal;

private:
	int							cluster;				// cluster number of this update
	int							areaNum;				// area number of this update
	unsigned short				tmpTravelTime;			// temporary travel time
	unsigned short *			areaTravelTimes;		// travel times within the area
	arcVec3						start;					// start point into area
	idRoutingUpdate *			next;					// next in list
	idRoutingUpdate *			prev;					// prev in list
	bool						isInList;				// true if the update is in the list
};


class idRoutingObstacle {
	friend class idAASLocal;
								idRoutingObstacle( void ) { }

private:
	arcBounds					bounds;					// obstacle bounds
	idList<int>					areas;					// areas the bounds are in
};

class idAASLocal : public idAAS {
public:
								idAASLocal( void );
	virtual						~idAASLocal( void );
	virtual bool				Init( const idStr &mapName, unsigned int mapFileCRC );
	virtual void				Shutdown( void );
// RAVEN BEGIN
// jscott: added summary flag
	virtual size_t				StatsSummary( void ) const;
// RAVEN END
	virtual void				Stats( void ) const;
	virtual void				Test( const arcVec3 &origin );
	virtual const idAASSettings *GetSettings( void ) const;
	virtual int					PointAreaNum( const arcVec3 &origin ) const;
	virtual int					PointReachableAreaNum( const arcVec3 &origin, const arcBounds &searchBounds, const int areaFlags ) const;
	virtual int					BoundsReachableAreaNum( const arcBounds &bounds, const int areaFlags ) const;
	virtual void				PushPointIntoAreaNum( int areaNum, arcVec3 &origin ) const;
	virtual arcVec3				AreaCenter( int areaNum ) const;
// RAVEN BEGIN
// bdube: added
	virtual float				AreaRadius( int areaNum ) const;
	virtual arcBounds &			AreaBounds( int areaNum ) const;
	virtual float				AreaCeiling( int areaNum ) const;
// RAVEN END
	virtual int					AreaFlags( int areaNum ) const;
	virtual int					AreaTravelFlags( int areaNum ) const;
	virtual bool				Trace( aasTrace_t &trace, const arcVec3 &start, const arcVec3 &end ) const;
	virtual const idPlane &		GetPlane( int planeNum ) const;
	virtual int					GetWallEdges( int areaNum, const arcBounds &bounds, int travelFlags, int *edges, int maxEdges ) const;
	virtual void				SortWallEdges( int *edges, int numEdges ) const;
	virtual void				GetEdgeVertexNumbers( int edgeNum, int verts[2] ) const;
	virtual void				GetEdge( int edgeNum, arcVec3 &start, arcVec3 &end ) const;
	virtual bool				SetAreaState( const arcBounds &bounds, const int areaContents, bool disabled );
	virtual aasHandle_t			AddObstacle( const arcBounds &bounds );
	virtual void				RemoveObstacle( const aasHandle_t handle );
	virtual void				RemoveAllObstacles( void );
	virtual int					TravelTimeToGoalArea( int areaNum, const arcVec3 &origin, int goalAreaNum, int travelFlags ) const;
	virtual bool				RouteToGoalArea( int areaNum, const arcVec3 origin, int goalAreaNum, int travelFlags, int &travelTime, idReachability **reach ) const;
	virtual bool				WalkPathToGoal( aasPath_t &path, int areaNum, const arcVec3 &origin, int goalAreaNum, const arcVec3 &goalOrigin, int travelFlags ) const;
	virtual bool				WalkPathValid( int areaNum, const arcVec3 &origin, int goalAreaNum, const arcVec3 &goalOrigin, int travelFlags, arcVec3 &endPos, int &endAreaNum ) const;
	virtual bool				FlyPathToGoal( aasPath_t &path, int areaNum, const arcVec3 &origin, int goalAreaNum, const arcVec3 &goalOrigin, int travelFlags ) const;
	virtual bool				FlyPathValid( int areaNum, const arcVec3 &origin, int goalAreaNum, const arcVec3 &goalOrigin, int travelFlags, arcVec3 &endPos, int &endAreaNum ) const;
	virtual void				ShowWalkPath( const arcVec3 &origin, int goalAreaNum, const arcVec3 &goalOrigin ) const;
	virtual void				ShowFlyPath( const arcVec3 &origin, int goalAreaNum, const arcVec3 &goalOrigin ) const;
	virtual bool				FindNearestGoal( aasGoal_t &goal, int areaNum, const arcVec3 origin, const arcVec3 &target, int travelFlags, float minDistance, float maxDistance, aasObstacle_t *obstacles, int numObstacles, idAASCallback &callback ) const;
// RAVEN BEGIN
// creed: Added Area Wall Extraction For AASTactical
	virtual idAASFile*			GetFile( void ) {return file;}
// cdr: Alternate Routes Bug
	 virtual void				SetReachabilityState( idReachability* reach, bool enable );
// rjohnson: added more debug drawing
	virtual bool				IsValid( void ) const;
	virtual void				ShowAreas( const arcVec3 &origin, bool ShowProblemAreas = false ) const;
// RAVEN END


private:
	idAASFile *					file;
	idStr						name;

private:	// routing data
	idRoutingCache ***			areaCacheIndex;			// for each area in each cluster the travel times to all other areas in the cluster
	int							areaCacheIndexSize;		// number of area cache entries
	idRoutingCache **			portalCacheIndex;		// for each area in the world the travel times from each portal
	int							portalCacheIndexSize;	// number of portal cache entries
	idRoutingUpdate *			areaUpdate;				// memory used to update the area routing cache
	idRoutingUpdate *			portalUpdate;			// memory used to update the portal routing cache
	unsigned short *			goalAreaTravelTimes;	// travel times to goal areas
	unsigned short *			areaTravelTimes;		// travel times through the areas
	int							numAreaTravelTimes;		// number of area travel times
	mutable idRoutingCache *	cacheListStart;			// start of list with cache sorted from oldest to newest
	mutable idRoutingCache *	cacheListEnd;			// end of list with cache sorted from oldest to newest
	mutable int					totalCacheMemory;		// total cache memory used
	idList<idRoutingObstacle *>	obstacleList;			// list with obstacles

private:	// routing
	bool						SetupRouting( void );
	void						ShutdownRouting( void );
	unsigned short				AreaTravelTime( int areaNum, const arcVec3 &start, const arcVec3 &end ) const;
	void						CalculateAreaTravelTimes( void );
	void						DeleteAreaTravelTimes( void );
	void						SetupRoutingCache( void );
	void						DeleteClusterCache( int clusterNum );
	void						DeletePortalCache( void );
	void						ShutdownRoutingCache( void );
	void						RoutingStats( void ) const;
	void						LinkCache( idRoutingCache *cache ) const;
	void						UnlinkCache( idRoutingCache *cache ) const;
	void						DeleteOldestCache( void ) const;
	idReachability *			GetAreaReachability( int areaNum, int reachabilityNum ) const;
	int							ClusterAreaNum( int clusterNum, int areaNum ) const;
	void						UpdateAreaRoutingCache( idRoutingCache *areaCache ) const;
	idRoutingCache *			GetAreaRoutingCache( int clusterNum, int areaNum, int travelFlags ) const;
	void						UpdatePortalRoutingCache( idRoutingCache *portalCache ) const;
	idRoutingCache *			GetPortalRoutingCache( int clusterNum, int areaNum, int travelFlags ) const;
	void						RemoveRoutingCacheUsingArea( int areaNum );
	void						DisableArea( int areaNum );
	void						EnableArea( int areaNum );
	bool						SetAreaState_r( int nodeNum, const arcBounds &bounds, const int areaContents, bool disabled );
	void						GetBoundsAreas_r( int nodeNum, const arcBounds &bounds, idList<int> &areas ) const;
	void						SetObstacleState( const idRoutingObstacle *obstacle, bool enable );

private:	// pathing
	bool						EdgeSplitPoint( arcVec3 &split, int edgeNum, const idPlane &plane ) const;
	bool						FloorEdgeSplitPoint( arcVec3 &split, int areaNum, const idPlane &splitPlane, const idPlane &frontPlane, bool closest ) const;
	arcVec3						SubSampleWalkPath( int areaNum, const arcVec3 &origin, const arcVec3 &start, const arcVec3 &end, int travelFlags, int &endAreaNum ) const;
	arcVec3						SubSampleFlyPath( int areaNum, const arcVec3 &origin, const arcVec3 &start, const arcVec3 &end, int travelFlags, int &endAreaNum ) const;

private:	// debug
	const arcBounds &			DefaultSearchBounds( void ) const;
	void						DrawCone( const arcVec3 &origin, const arcVec3 &dir, float radius, const arcVec4 &color ) const;
	void						DrawAreaBounds( int areaNum ) const;
	void						DrawArea( int areaNum ) const;
	void						DrawFace( int faceNum, bool side ) const;
	void						DrawEdge( int edgeNum, bool arrow ) const;
	void						DrawReachability( const idReachability *reach ) const;
	void						ShowArea( const arcVec3 &origin ) const;
	void						ShowWallEdges( const arcVec3 &origin ) const;
	void						ShowHideArea( const arcVec3 &origin, int targerAreaNum ) const;
	bool						PullPlayer( const arcVec3 &origin, int toAreaNum ) const;
	void						RandomPullPlayer( const arcVec3 &origin ) const;
	void						ShowPushIntoArea( const arcVec3 &origin ) const;

// RAVEN BEGIN
// rjohnson: added more debug drawing
	void						DrawSimpleEdge( int edgeNum ) const;
	void						DrawSimpleFace( int faceNum, bool visited ) const;
	void						DrawSimpleArea( int areaNum ) const;
	void						ShowProblemEdge( int edgeNum ) const;
	void						ShowProblemFace( int faceNum ) const;
	void						ShowProblemArea( int areaNum ) const;
	void						ShowProblemArea( const arcVec3 &origin ) const;
// RAVEN END
};

#endif /* !__AAS_LOCAL_H__ */
