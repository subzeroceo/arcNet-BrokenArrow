
#ifndef __SEAS_H__
#define __SEAS_H__

/*
===============================================================================
	Situational Environment Awareness System	Situational Environment Awareness System
	Situational Environment Awareness System	Situational Environment Awareness System
	Situational Environment Awareness System	Situational Environment Awareness System
===============================================================================
*/

enum {
	PATHTYPE_WALK,
	PATHTYPE_WALKOFFLEDGE,
	PATHTYPE_BARRIERJUMP,
	PATHTYPE_JUMP
};

typedef struct seasPath_s {
	int							type;			// path type
	anVec3						moveGoal;		// point the AI should move towards
	int							moveAreaNum;	// number of the area the AI should move towards
	anVec3						secondaryGoal;	// secondary move goal for complex navigation
	const anReachability *		reachability;	// reachability used for navigation
} seasPath_t;


typedef struct seasGoal_s {
	int							areaNum;		// area the goal is in
	anVec3						origin;			// position of goal
} seasGoal_t;


typedef struct seasObstructs_s {
	anBounds					absBounds;		// absolute bounds of obstacle
	anBounds					expAbsBounds;	// expanded absolute bounds of obstacle
} seasObstructs_t;

class anSEASCallback {
public:
	virtual	~anSEASCallback ( void );

	enum testResult_t {
		TEST_OK,
		TEST_BADAREA,
		TEST_BADPOINT
	};

	virtual void				Init		( void );
	virtual void				Finish		( void );

	testResult_t				Test		( class anSEAS *aas, int areaNum, const anVec3& origin, float minDistance, float maxDistance, const anVec3* point, seasGoal_t& goal );

protected:

	virtual bool				TestArea	( class anSEAS *aas, int areaNum, const seasArea_t& area );
	virtual	bool				TestPoint	( class anSEAS *aas, const anVec3& pos, const float zAllow=0.0f );

private:

	bool		TestPointDistance		( const anVec3& origin, const anVec3& point, float minDistance, float maxDistance );
};

typedef int seasHandle_t;

class anSEAS {
public:
	static anSEAS *				Alloc( void );
	virtual						~anSEAS( void ) = 0;
								// Initialize for the given map.
	virtual bool				Init( const anString &mapName, unsigned int mapFileCRC ) = 0;
								// Prints out the memory used by this AAS
	virtual size_t				StatsSummary( void ) const = 0;

#if defined(_MEM_SYS_SUPPORT)
	virtual void				Shutdown( void ) = 0;
#endif

								// Print AAS stats.
	virtual void				Stats( void ) const = 0;
								// Test from the given origin.
	virtual void				Test( const anVec3 &origin ) = 0;
								// Get the AAS settings.
	virtual const anSEASSettings *GetSettings( void ) const = 0;
								// Returns the number of the area the origin is in.
	virtual int					PointAreaNum( const anVec3 &origin ) const = 0;
								// Returns the number of the nearest reachable area for the given point.
	virtual int					PointReachableAreaNum( const anVec3 &origin, const anBounds &bounds, const int areaFlags ) const = 0;
								// Returns the number of the first reachable area in or touching the bounds.
	virtual int					BoundsReachableAreaNum( const anBounds &bounds, const int areaFlags ) const = 0;
								// Push the point into the area.
	virtual void				PushPointIntoAreaNum( int areaNum, anVec3 &origin ) const = 0;
								// Returns a reachable point inside the given area.
	virtual anVec3				AreaCenter( int areaNum ) const = 0;

								// Returns a reachable point inside the given area.
	virtual float				AreaRadius( int areaNum ) const = 0;
	virtual anBounds &			AreaBounds( int areaNum ) const = 0;
	virtual float				AreaCeiling( int areaNum ) const = 0;

								// Returns the area flags.
	virtual int					AreaFlags( int areaNum ) const = 0;
								// Returns the travel flags for traveling through the area.
	virtual int					AreaTravelFlags( int areaNum ) const = 0;
								// Trace through the areas and report the first collision.
	virtual bool				Trace( seasTrace_t &trace, const anVec3 &start, const anVec3 &end ) const = 0;
								// Get a plane for a trace.
	virtual const anPlane &		GetPlane( int planeNum ) const = 0;
								// Get wall edges.
	virtual int					GetWallEdges( int areaNum, const anBounds &bounds, int travelFlags, int *edges, int maxEdges ) const = 0;
								// Sort the wall edges to create continuous sequences of walls.
	virtual void				SortWallEdges( int *edges, int numEdges ) const = 0;
								// Get the vertex numbers for an edge.
	virtual void				GetEdgeVertexNumbers( int edgeNum, int verts[2] ) const = 0;
								// Get an edge.
	virtual void				GetEdge( int edgeNum, anVec3 &start, anVec3 &end ) const = 0;
								// Find all areas within or touching the bounds with the given contents and disable/enable them for routing.
	virtual bool				SetAreaState( const anBounds &bounds, const int areaContents, bool disabled ) = 0;
								// Add an obstacle to the routing system.
	virtual seasHandle_t			AddObstacle( const anBounds &bounds ) = 0;
								// Remove an obstacle from the routing system.
	virtual void				RemoveObstacle( const seasHandle_t handle ) = 0;
								// Remove all obstacles from the routing system.
	virtual void				RemoveAllObstacles( void ) = 0;
								// Returns the travel time towards the goal area in 100th of a second.
	virtual int					TravelTimeToGoalArea( int areaNum, const anVec3 &origin, int goalAreaNum, int travelFlags ) const = 0;
								// Get the travel time and first reachability to be used towards the goal, returns true if there is a path.
	virtual bool				RouteToGoalArea( int areaNum, const anVec3 origin, int goalAreaNum, int travelFlags, int &travelTime, anReachability **reach ) const = 0;
								// Creates a walk path towards the goal.
	virtual bool				WalkPathToGoal( seasPath_t &path, int areaNum, const anVec3 &origin, int goalAreaNum, const anVec3 &goalOrigin, int travelFlags ) const = 0;
								// Returns true if one can walk along a straight line from the origin to the goal origin.
	virtual bool				WalkPathValid( int areaNum, const anVec3 &origin, int goalAreaNum, const anVec3 &goalOrigin, int travelFlags, anVec3 &endPos, int &endAreaNum ) const = 0;
								// Creates a fly path towards the goal.
	virtual bool				FlyPathToGoal( seasPath_t &path, int areaNum, const anVec3 &origin, int goalAreaNum, const anVec3 &goalOrigin, int travelFlags ) const = 0;
								// Returns true if one can fly along a straight line from the origin to the goal origin.
	virtual bool				FlyPathValid( int areaNum, const anVec3 &origin, int goalAreaNum, const anVec3 &goalOrigin, int travelFlags, anVec3 &endPos, int &endAreaNum ) const = 0;
								// Show the walk path from the origin towards the area.
	virtual void				ShowWalkPath( const anVec3 &origin, int goalAreaNum, const anVec3 &goalOrigin ) const = 0;
								// Show the fly path from the origin towards the area.
	virtual void				ShowFlyPath( const anVec3 &origin, int goalAreaNum, const anVec3 &goalOrigin ) const = 0;
								// Find the nearest goal which satisfies the callback.
	virtual bool				FindNearestGoal( seasGoal_t &goal, int areaNum, const anVec3 origin, const anVec3 &target, int travelFlags, float minDistance, float maxDistance, seasObstructs_t *obstacles, int numObstacles, anSEASCallback &callback ) const = 0;

	virtual anSEASFile*			GetFile( void ) = 0;
	virtual void				SetReach
	virtual void				ShowAreas( const anVec3 &origin, bool ShowProblemAreas = false ) const = 0;
	virtual bool				IsValid( void ) const = 0;

};

#endif // !__SEAS_H__
