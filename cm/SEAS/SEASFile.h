#ifndef __SEASFILE_H__
#define __SEASFILE_H__

/*
===============================================================================

	SEAS File

===============================================================================
*/

#define SEAS_FILEID					"Tek10SEAS"
#define SEAS_FILEVERSION			"1.08"

// travel flags
#define TFL_INVALID					BIT( 0 )		// not valid
#define TFL_WALK					BIT( 1 )		// walking
#define TFL_CROUCH					BIT(2)		// crouching
#define TFL_WALKOFFLEDGE			BIT(3)		// walking of a ledge
#define TFL_BARRIERJUMP				BIT(4)		// jumping onto a barrier
#define TFL_JUMP					BIT(5)		// jumping
#define TFL_LADDER					BIT(6)		// climbing a ladder
#define TFL_SWIM					BIT(7)		// swimming
#define TFL_WATERJUMP				BIT(8)		// jump out of the water
#define TFL_TELEPORT				BIT(9)		// teleportation
#define TFL_ELEVATOR				BIT(10)		// travel by elevator
#define TFL_FLY						BIT(11)		// fly
#define TFL_SPECIAL					BIT(12)		// special
#define TFL_WATER					BIT(13)		// travel through water
#define TFL_AIR						BIT(14)		// travel through air

// face flags
#define FACE_SOLID					BIT( 0 )		// solid at the other side
#define FACE_LADDER					BIT( 1 )		// ladder surface
#define FACE_FLOOR					BIT(2)		// standing on floor when on this face
#define FACE_LIQUID					BIT(3)		// face seperating two areas with liquid
#define FACE_LIQUIDSURFACE			BIT(4)		// face seperating liquid and air

// area flags
#define AREA_FLOOR					BIT( 0 )		// AI can stand on the floor in this area
#define AREA_GAP					BIT( 1 )		// area has a gap
#define AREA_LEDGE					BIT(2)		// if entered the AI bbox partly floats above a ledge
#define AREA_LADDER					BIT(3)		// area contains one or more ladder faces
#define AREA_LIQUID					BIT(4)		// area contains a liquid
#define AREA_CROUCH					BIT(5)		// AI cannot walk but can only crouch in this area
#define AREA_REACHABLE_WALK			BIT(6)		// area is reachable by walking or swimming
#define AREA_REACHABLE_FLY			BIT(7)		// area is reachable by flying
#define AREA_REACHABLE_SPECIAL		BIT(8)		// area is reachable by special means
#define AREA_OUTSIDE				BIT(9)		// area is outside
#define AREA_HIGH_CEILING			BIT(10)		// area has a ceiling that is high enough to perform certain movements
#define AREA_NOPUSH					BIT(11)		// push into area failed because the area winding is malformed

// area contents flags
#define AREACONTENTS_SOLID			BIT( 0 )		// solid, not a valid area
#define AREACONTENTS_WATER			BIT( 1 )		// area contains water
#define AREACONTENTS_CLUSTERPORTAL	BIT(2)		// area is a cluster portal
#define AREACONTENTS_OBSTACLE		BIT(3)		// area contains (part of) a dynamic obstacle
#define AREACONTENTS_TELEPORTER		BIT(4)		// area contains (part of) a teleporter trigger
#define AREA_FLOOD_VISITED			BIT(5)		// area visited during a flood routine.  this is a temporary flag that should be removed before the routine exits

// bits for different bboxes
#define AREACONTENTS_BBOX_BIT		24

// feature bits
#define FEATURE_COVER				BIT( 0 )		// provides cover
#define FEATURE_LOOK_LEFT			BIT( 1 )		// attack by leaning left
#define FEATURE_LOOK_RIGHT			BIT(2)		// attack by leaning right
#define FEATURE_LOOK_OVER			BIT(3)		// attack by leaning over the cover
#define FEATURE_CORNER_LEFT			BIT(4)		// is a left corner
#define FEATURE_CORNER_RIGHT		BIT(5)		// is a right corner
#define FEATURE_PINCH				BIT(6)		// is a tight area connecting two larger areas
#define FEATURE_VANTAGE				BIT(7)		// provides a good view of the sampled area as a whole

// forward reference of sensor object
struct anSEASTacticalSensor;
struct anMarker;

#define MAX_REACH_PER_AREA			256
#define SEAS_MAX_TREE_DEPTH			128

#define SEAS_MAX_BOUNDING_BOXES		4

typedef enum {
	RE_WALK,
	RE_WALKOFFLEDGE,
	RE_FLY,
	RE_SWIM,
	RE_WATERJUMP,
	RE_BARRIERJUMP,
	RE_SPECIAL
};

// reachability to another area
class anReachability {
public:
	int							travelType;			// type of travel required to get to the area
	short						toAreaNum;			// number of the reachable area
	short						fromAreaNum;		// number of area the reachability starts
	anVec3						start;				// start point of inter area movement
	anVec3						end;				// end point of inter area movement
	//short						start[3];			// start point of inter area movement
	//short						end[3];				// end point of inter area movement
	int							edgeNum;			// edge crossed by this reachability
	unsigned short				travelTime;			// travel time of the inter area movement
	byte						number;				// reachability number within the fromAreaNum (must be < 256)
	byte						disableCount;		// number of times this reachability has been disabled
	anReachability *			next;				// next reachability in list
	anReachability *			rev_next;			// next reachability in reversed list
	unsigned short *			areaTravelTimes;	// travel times within the fromAreaNum from reachabilities that lead towards this area

	// v is the vector, d is the direction to snap towards
	void						SetStart( const anVec3 &v, const anVec3 &d ) { for ( int i = 0; i < 3; i++ ) start[i] = anMath::Ftoi( v[i] + anMath::Rint( d[i] ) ); }
	void						SetEnd( const anVec3 &v, const anVec3 &d )   { for ( int i = 0; i < 3; i++ ) end[i]   = anMath::Ftoi( v[i] + anMath::Rint( d[i] ) ); }

	const anVec3				GetStart() const { return anVec3( start[0], start[1], start[2] ); }
	const anVec3				GetEnd() const { return anVec3( end[0], end[1], end[2] ); }
};

class anReachability_Walk : public anReachability {
};

class anReachability_BarrierJump : public anReachability {
};

class anReachability_WaterJump : public anReachability {
};

class anReachability_WalkOffLedge : public anReachability {
};

class anReachability_Swim : public anReachability {
};

class anReachability_Fly : public anReachability {
};

class anReachability_Special : public anReachability {
	friend class anSEASFileLocal;
private:
	anDict						dict;
};

typedef int seasIndex_t;
// plane
typedef anPlane seasPlane_t;
typedef anVec3 seasVertex_t;

typedef struct seasEdge_s {
	int							vertexNum[2];		// numbers of the vertexes of this edge
	int							flags;
} seasEdge_t;

// area boundary face
typedef struct seasFace_s {
	unsigned short				planeNum;			// number of the plane this face is on
	unsigned short				flags;				// face flags
	int							numEdges;			// number of edges in the boundary of the face
	int							firstEdge;			// first edge in the edge index
	short						areas[2];			// area at the front and back of this face
} seasFace_t;

// area with a boundary of faces
typedef struct seasArea_s {
	int							numFaces;			// number of faces used for the boundary of the area
	int							firstFace;			// first face in the face index used for the boundary of the area
	anBounds					bounds;				// bounds of the area
	anVec3						center;				// center of the area an AI can move towards
	float						ceiling;			// top of the area
	int							travelFlags;		// travel flags for traveling through this area
	unsigned short				flags;				// several area flags
	unsigned short				contents;			// contents of the area
	int							numEdges;			// number of edges in the boundary of the face
	int							firstEdge;			// first edge in the edge index
	short						cluster;			// cluster the area belongs to, if negative it's a portal
	short						clusterAreaNum;		// number of the area in the cluster
	unsigned int				obstaclePVSOffset;	// offset into obstacle PVS
	anReachability *			reach;				// reachabilities that start from this area
	anReachability *			rev_reach;			// reachabilities that lead to this area

	// SEASTacticalSensor
	unsigned short				numFeatures;		// number of features in this area
	unsigned short				firstFeature;		// first feature in the feature index within this area

	// Obstacle Avoidance
	anMarker *					firstMarker;		// first obstacle avoidance threat in this area (0 if none)
} seasArea_t;

// nodes of the bsp tree
typedef struct seasNode_s {
	unsigned short				planeNum;			// number of the plane that splits the subspace at this node
	unsigned short				flags;				// node flags
	int							children[2];		// child nodes, zero is solid, negative is -(area number)
} seasNode_t;

// cluster portal
typedef struct seasPortal_s {
	short						areaNum;			// number of the area that is the actual portal
	short						clusters[2];		// number of cluster at the front and back of the portal
	short						clusterAreaNum[2];	// number of this portal area in the front and back cluster
	unsigned short				maxAreaTravelTime;	// maximum travel time through the portal area
} seasPortal_t;

typedef struct seasCluster_s {
	int							numAreas;			// number of areas in the cluster
	int							numReachableAreas;	// number of areas with reachabilities
	int							numPortals;			// number of cluster portals
	int							firstPortal;		// first cluster portal in the index
} seasCluster_t;

typedef	struct seasFeature_s {
	short						x;					// 2 Bytes
	short						y;					// 2 Bytes
	short						z;					// 2 Bytes
	unsigned short				flags;				// 2 Bytes
	unsigned char				normalx;			// 1 Byte
	unsigned char				normaly;			// 1 Byte
	unsigned char				height;				// 1 Byte
	unsigned char				weight;				// 1 Byte

	anVec3&			Normal();
	anVec3&			Origin();

	void			DrawDebugInfo( int index=-1 );
  	int				GetLookPos( anVec3 &lookPos, const anVec3 &aimAtOrigin, const float leanDistance = 16.0f );
} seasFeature_t;

// trace through the world
typedef struct seasTrace_s {
								// parameters
	int							flags;				// areas with these flags block the trace
	int							travelFlags;		// areas with these travel flags block the trace
	int							maxAreas;			// size of the 'areas' array
	int							getOutOfSolid;		// trace out of solid if the trace starts in solid
								// output
	float						fraction;			// fraction of trace completed
	anVec3						endpos;				// end position of trace
	int							planeNum;			// plane hit
	int							lastAreaNum;		// number of last area the trace went through
	int							blockingAreaNum;	// area that could not be entered
	int							numAreas;			// number of areas the trace went through
	int *						areas;				// array to store areas the trace went through
	anVec3 *					points;				// points where the trace entered each new area
								seasTrace_s( void ) { areas = nullptr; points = nullptr; getOutOfSolid = false; flags = travelFlags = maxAreas = 0; }
} seasTrace_t;

// settings
class anSEASSettings {
public:
								// collision settings
	int							numBoundingBoxes;
	anBounds					boundingBoxes[SEAS_MAX_BOUNDING_BOXES];

	bool						writeBrushMap;
	int							primitiveModeBrush;

	bool						usePatches;
	int							primitiveModePatch;

	int							primitiveModeModel;
	int							primitiveModeTerrain;

	bool						playerFlood;
	bool						noOptimize;

	bool						allowSwimReachabilities;
	bool						allowFlyReachabilities;

	bool						generateAllFaces;
	bool						generateTacticalFeatures;
	int							SEASOnly;	// 0, else 32,48,96,250 or -1 for all

	//int							type;
	anStr					fileExtension;

								// physics settings
	anVec3						gravity;
	anVec3						gravityDir;
	anVec3						invGravityDir;
	float						gravityValue;
	float						maxStepHeight;
	float						maxBarrierHeight;
	float						maxWaterJumpHeight;
	float						maxFallHeight;
	float						minFloorCos;
	float						minHighCeiling;
	float						groundSpeed;				// in units per second
	float						waterSpeed;					// in units per second
	float						ladderSpeed;				// in units per second

								// navigation settings
	float						wallCornerEdgeRadius;
	float						ledgeCornerEdgeRadius;
	float						obstaclePVSRadius;

								// fixed travel times
	int							tt_barrierJump;
	int							tt_startCrouching;
	int							tt_waterJump;
	int							tt_startWalkOffLedge;

	anVec4						debugColor;
	bool						debugDraw;
public:
								anSEASSettings( void );

	bool						FromFile( const anStr &fileName );

// changed to be anBinaryLexer instead of anLexer so that we have the ability to read binary files
	bool						FromParser( anBinaryLexer &src );
	bool						WriteToFileBinary( anFile *fp ) const;
	bool						ReadFromFileBinary( anFile *fp );

	bool						FromDict( const char *name, const anDict *dict );
	bool						WriteToFile( anFile *fp ) const;
	bool						ValidForBounds( const anBounds &bounds ) const;
	bool						ValabEntity( const char *classname, bool *needFlyReachabilities = nullptr ) const;
//	bool						ValabEntity( const anMapFile *mapFile, const char *classname ) const;

	float						Radius( float scale = 1.0f ) const;

private:

	bool						ParseBool( anBinaryLexer &src, bool &b );
	bool						ParseInt( anBinaryLexer &src, int &i );
	bool						ParseFloat( anBinaryLexer &src, float &f );
	bool						ParseVector( anBinaryLexer &src, anVec3 &vec );
	bool						ParseBBoxes( anBinaryLexer &src );
	bool						ParseBounds( anBinaryLexer &src, anBounds &bounds );

};

/*
-	when a node child is a solid leaf the node child number is zero
-	two adjacent areas (sharing a plane at opposite sides) share a face
	this face is a portal between the areas
-	when an area uses a face from the faceindex with a positive index
	then the face plane normal points into the area
-	the face edges are stored counter clockwise using the edgeindex
-	two adjacent convex areas (sharing a face) only share One face
	this is a simple result of the areas being convex
-	the areas can't have a mixture of ground and gap faces
	other mixtures of faces in one area are allowed
-	areas with the AREACONTENTS_CLUSTERPORTAL in the settings have
	the cluster number set to the negative portal number
-	edge zero is a dummy
-	face zero is a dummy
-	area zero is a dummy
-	node zero is a dummy
-	portal zero is a dummy
-	cluster zero is a dummy
*/
typedef struct sizeEstimate_s {
	int			numEdgeIndexes;
	int			numFaceIndexes;
	int			numAreas;
	int			numNodes;
} sizeEstimate_t;

class anSEASFile {
public:
	virtual 						~anSEASFile( void ) {}

	class anSEASFile	*		CreateNew( void ) = 0;
	class anSEASSettings *		CreateAASSettings( void ) = 0;
	class anReachability *		CreateReachability( int type ) = 0;
// changed to be anBinaryLexer instead of anLexer so that we have the ability to read binary files
	bool						FromParser( class anSEASSettings *edit, anBinaryLexer &src ) = 0;

	const char *				GetName( void ) const = 0;
	unsigned int				GetCRC( void ) const = 0;
	void						SetSizes( sizeEstimate_t size ) = 0;

	int							GetNumPlanes( void ) const = 0;
	anPlane &					GetPlane( int index ) = 0;
	virtual int						FindPlane( const anPlane &plane, const float normalEps, const float distEps ) = 0;

	int							GetNumVertices( void ) const = 0;
	seasVertex_t &				GetVertex( int index ) = 0;
	virtual int						AppendVertex( seasVertex_t &vert ) = 0;

	int							GetNumEdges( void ) const = 0;
	seasEdge_t &				GetEdge( int index ) = 0;
	virtual int						AppendEdge( seasEdge_t &edge ) = 0;

	int							GetNumEdgeIndexes( void ) const = 0;
	seasIndex_t &				GetEdgeIndex( int index ) = 0;
	virtual int						AppendEdgeIndex( seasIndex_t &edgeIdx ) = 0;

	int							GetNumFaces( void ) const = 0;
	seasFace_t &				GetFace( int index ) = 0;
	virtual int						AppendFace( seasFace_t &face ) = 0;

	int							GetNumFaceIndexes( void ) const = 0;
	seasIndex_t &				GetFaceIndex( int index ) = 0;
	virtual int						AppendFaceIndex( seasIndex_t &faceIdx ) = 0;

	int							GetNumAreas( void ) const = 0;
	seasArea_t &				GetArea( int index ) = 0;
	virtual int						AppendArea( seasArea_t &area ) = 0;

	int							GetNumNodes( void ) const = 0;
	seasNode_t &				GetNode( int index ) = 0;
	virtual int						AppendNode( seasNode_t &node ) = 0;
	virtual void					SetNumNodes( int num ) = 0;

	int							GetNumPortals( void ) const = 0;
	seasPortal_t &				GetPortal( int index ) = 0;
	virtual int						AppendPortal( seasPortal_t &portal ) = 0;

	int							GetNumPortalIndexes( void ) const = 0;
	seasIndex_t &				GetPortalIndex( int index ) = 0;
	virtual int						AppendPortalIndex( seasIndex_t &portalIdx, int clusterNum ) = 0;

	int							GetNumClusters( void ) const = 0;
	seasCluster_t &				GetCluster( int index ) = 0;
	virtual int						AppendCluster( seasCluster_t &cluster ) = 0;

	virtual void					ClearTactical( void ) = 0;

	int							GetNumFeatureIndexes( void ) const = 0;
	seasIndex_t &				GetFeatureIndex( int index ) = 0;
	virtual int						AppendFeatureIndex( seasIndex_t &featureIdx ) = 0;

	int							GetNumFeatures( void ) const = 0;
	seasFeature_t &				GetFeature( int index ) = 0;
	virtual int						AppendFeature( seasFeature_t &cluster ) = 0;

	anSEASSettings &			GetSettings( void ) = 0;
	void						SetSettings( const anSEASSettings &in ) = 0;

	virtual void					SetPortalMaxTravelTime( int index, int time ) = 0;
	virtual void					SetAreaTravelFlag( int index, int flag ) = 0;
	virtual void					RemoveAreaTravelFlag( int index, int flag ) = 0;

	virtual anVec3					EdgeCenter( int edgeNum ) const = 0;
	virtual anVec3					FaceCenter( int faceNum ) const = 0;
	virtual anVec3					AreaCenter( int areaNum ) const = 0;

	virtual anBounds				EdgeBounds( int edgeNum ) const = 0;
	virtual anBounds				FaceBounds( int faceNum ) const = 0;
	virtual anBounds				AreaBounds( int areaNum ) const = 0;

	virtual int						PointAreaNum( const anVec3 &origin ) const = 0;
	virtual int						PointReachableAreaNum( const anVec3 &origin, const anBounds &searchBounds, const int areaFlags, const int excludeTravelFlags ) const = 0;
	virtual int						BoundsReachableAreaNum( const anBounds &bounds, const int areaFlags, const int excludeTravelFlags ) const = 0;
	virtual void					PushPointIntoAreaNum( int areaNum, anVec3 &point ) const = 0;
	virtual bool					Trace( seasTrace_t &trace, const anVec3 &start, const anVec3 &end ) const = 0;
	virtual void					PrintInfo( void ) const = 0;

	virtual size_t					GetMemorySize( void ) = 0;

	void						Init( void ) = 0;
	virtual bool					Load( const anStr &fileName, unsigned int mapFileCRC ) = 0;
	bool						Write( const anStr &fileName, unsigned int mapFileCRC ) = 0;
	void						Clear( void ) = 0;
	void						FinishAreas( void ) = 0;
	void						ReportRoutingEfficiency( void ) const = 0;
	void						LinkReversedReachability( void ) = 0;
	void						DeleteReachabilities( void ) = 0;
	void						DeleteClusters( void ) = 0;
	void						Optimize( void ) = 0;
	virtual bool					IsDummyFile( unsigned int mapFileCRC ) = 0;

	virtual const anDict &			GetReachabilitySpecialDict( anReachability *reach ) const = 0;
	virtual void					SetReachabilitySpecialDictKeyValue( anReachability *reach, const char *key, const char *value ) = 0;
};

extern anSEASFile		*SEASFile;

#endif // !__AASFILE_H__
