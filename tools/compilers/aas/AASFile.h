/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __AASFILE_H__
#define __AASFILE_H__

/*
===============================================================================

	AAS File

===============================================================================
*/

#define SEAS_FILEID					"DewmAAS"
#define SEAS_FILEVERSION				"1.07"

// travel flags
#define TFL_INVALID					BIT(0 )		// not valid
#define TFL_WALK					BIT(1 )		// walking
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
#define TFL_WATER					BIT(21)		// travel through water
#define TFL_AIR						BIT(22)		// travel through air

// face flags
#define FACE_SOLID					BIT(0 )		// solid at the other side
#define FACE_LADDER					BIT(1 )		// ladder surface
#define FACE_FLOOR					BIT(2)		// standing on floor when on this face
#define FACE_LIQUID					BIT(3)		// face seperating two areas with liquid
#define FACE_LIQUIDSURFACE			BIT(4)		// face seperating liquid and air

// area flags
#define AREA_FLOOR					BIT(0 )		// AI can stand on the floor in this area
#define AREA_GAP					BIT(1 )		// area has a gap
#define AREA_LEDGE					BIT(2)		// if entered the AI bbox partly floats above a ledge
#define AREA_LADDER					BIT(3)		// area contains one or more ladder faces
#define AREA_LIQUID					BIT(4)		// area contains a liquid
#define AREA_CROUCH					BIT(5)		// AI cannot walk but can only crouch in this area
#define AREA_REACHABLE_WALK			BIT(6)		// area is reachable by walking or swimming
#define AREA_REACHABLE_FLY			BIT(7)		// area is reachable by flying

// area contents flags
#define AREACONTENTS_SOLID			BIT(0 )		// solid, not a valid area
#define AREACONTENTS_WATER			BIT(1 )		// area contains water
#define AREACONTENTS_CLUSTERPORTAL	BIT(2)		// area is a cluster portal
#define AREACONTENTS_OBSTACLE		BIT(3)		// area contains (part of) a dynamic obstacle
#define AREACONTENTS_TELEPORTER		BIT(4)		// area contains (part of) a teleporter trigger

// bits for different bboxes
#define AREACONTENTS_BBOX_BIT		24

#define MAX_REACH_PER_AREA			256
#define SEAS_MAX_TREE_DEPTH			128

#define SEAS_MAX_BOUNDING_BOXES		4

// reachability to another area
class anReachability {
public:
	int							travelType;			// type of travel required to get to the area
	short						toAreaNum;			// number of the reachable area
	short						fromAreaNum;		// number of area the reachability starts
	anVec3						start;				// start point of inter area movement
	anVec3						end;				// end point of inter area movement
	int							edgeNum;			// edge crossed by this reachability
	unsigned short				travelTime;			// travel time of the inter area movement
	byte						number;				// reachability number within the fromAreaNum (must be < 256)
	byte						disableCount;		// number of times this reachability has been disabled
	anReachability *			next;				// next reachability in list
	anReachability *			rev_next;			// next reachability in reversed list
	unsigned short *			areaTravelTimes;	// travel times within the fromAreaNum from reachabilities that lead towards this area
public:
	void						CopyBase( anReachability &reach );
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
public:
	anDict						dict;
};

// index
typedef int seasIndex_t;

// vertex
typedef anVec3 seasVertex_t;

// edge
typedef struct seasEdge_s {
	int							vertexNum[2];		// numbers of the vertexes of this edge
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
	unsigned short				flags;				// several area flags
	unsigned short				contents;			// contents of the area
	short						cluster;			// cluster the area belongs to, if negative it's a portal
	short						clusterAreaNum;		// number of the area in the cluster
	int							travelFlags;		// travel flags for traveling through this area
	anReachability *			reach;				// reachabilities that start from this area
	anReachability *			rev_reach;			// reachabilities that lead to this area
} seasArea_t;

// nodes of the bsp tree
typedef struct seasNode_s {
	unsigned short				planeNum;			// number of the plane that splits the subspace at this node
	int							children[2];		// child nodes, zero is solid, negative is -(area number)
} seasNode_t;

// cluster portal
typedef struct seasPortal_s {
	short						areaNum;			// number of the area that is the actual portal
	short						clusters[2];		// number of cluster at the front and back of the portal
	short						clusterAreaNum[2];	// number of this portal area in the front and back cluster
	unsigned short				maxAreaTravelTime;	// maximum travel time through the portal area
} seasPortal_t;

// cluster
typedef struct seasCluster_s {
	int							numAreas;			// number of areas in the cluster
	int							numReachableAreas;	// number of areas with reachabilities
	int							numPortals;			// number of cluster portals
	int							firstPortal;		// first cluster portal in the index
} seasCluster_t;

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
	bool						usePatches;
	bool						writeBrushMap;
	bool						playerFlood;
	bool						noOptimize;
	bool						allowSwimReachabilities;
	bool						allowFlyReachabilities;
	anString						fileExtension;
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
								// fixed travel times
	int							tt_barrierJump;
	int							tt_startCrouching;
	int							tt_waterJump;
	int							tt_startWalkOffLedge;

public:
								anSEASSettings( void );

	bool						FromFile( const anString &fileName );
	bool						FromParser( anLexer &src );
	bool						FromDict( const char *name, const anDict *dict );
	bool						WriteToFile( anFile *fp ) const;
	bool						ValidForBounds( const anBounds &bounds ) const;
	bool						ValarcEntity( const char *classname ) const;

private:
	bool						ParseBool( anLexer &src, bool &b );
	bool						ParseInt( anLexer &src, int &i );
	bool						ParseFloat( anLexer &src, float &f );
	bool						ParseVector( anLexer &src, anVec3 &vec );
	bool						ParseBBoxes( anLexer &src );
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


class anSEASFile {
public:
	virtual 					~anSEASFile( void ) {}

	const char *				GetName( void ) const { return name.c_str(); }
	unsigned int				GetCRC( void ) const { return crc; }

	int							GetNumPlanes( void ) const { return planeList.Num(); }
	const anPlane &				GetPlane( int index ) const { return planeList[index]; }
	int							GetNumVertices( void ) const { return vertices.Num(); }
	const seasVertex_t &			GetVertex( int index ) const { return vertices[index]; }
	int							GetNumEdges( void ) const { return edges.Num(); }
	const seasEdge_t &			GetEdge( int index ) const { return edges[index]; }
	int							GetNumEdgeIndexes( void ) const { return edgeIndex.Num(); }
	const seasIndex_t &			GetEdgeIndex( int index ) const { return edgeIndex[index]; }
	int							GetNumFaces( void ) const { return faces.Num(); }
	const seasFace_t &			GetFace( int index ) const { return faces[index]; }
	int							GetNumFaceIndexes( void ) const { return faceIndex.Num(); }
	const seasIndex_t &			GetFaceIndex( int index ) const { return faceIndex[index]; }
	int							GetNumAreas( void ) const { return areas.Num(); }
	const seasArea_t &			GetArea( int index ) { return areas[index]; }
	int							GetNumNodes( void ) const { return nodes.Num(); }
	const seasNode_t &			GetNode( int index ) const { return nodes[index]; }
	int							GetNumPortals( void ) const { return portals.Num(); }
	const seasPortal_t &			GetPortal( int index ) { return portals[index]; }
	int							GetNumPortalIndexes( void ) const { return portalIndex.Num(); }
	const seasIndex_t &			GetPortalIndex( int index ) const { return portalIndex[index]; }
	int							GetNumClusters( void ) const { return clusters.Num(); }
	const seasCluster_t &		GetCluster( int index ) const { return clusters[index]; }

	const anSEASSettings &		GetSettings( void ) const { return settings; }

	void						SetPortalMaxTravelTime( int index, int time ) { portals[index].maxAreaTravelTime = time; }
	void						SetAreaTravelFlag( int index, int flag ) { areas[index].travelFlags |= flag; }
	void						RemoveAreaTravelFlag( int index, int flag ) { areas[index].travelFlags &= ~flag; }

	virtual anVec3				EdgeCenter( int edgeNum ) const = 0;
	virtual anVec3				FaceCenter( int faceNum ) const = 0;
	virtual anVec3				AreaCenter( int areaNum ) const = 0;

	virtual anBounds			EdgeBounds( int edgeNum ) const = 0;
	virtual anBounds			FaceBounds( int faceNum ) const = 0;
	virtual anBounds			AreaBounds( int areaNum ) const = 0;

	virtual int					PointAreaNum( const anVec3 &origin ) const = 0;
	virtual int					PointReachableAreaNum( const anVec3 &origin, const anBounds &searchBounds, const int areaFlags, const int excludeTravelFlags ) const = 0;
	virtual int					BoundsReachableAreaNum( const anBounds &bounds, const int areaFlags, const int excludeTravelFlags ) const = 0;
	virtual void				PushPointIntoAreaNum( int areaNum, anVec3 &point ) const = 0;
	virtual bool				Trace( seasTrace_t &trace, const anVec3 &start, const anVec3 &end ) const = 0;
	virtual void				PrintInfo( void ) const = 0;

protected:
	anString						name;
	unsigned int				crc;

	aRcPlaneSet					planeList;
	anList<seasVertex_t>			vertices;
	anList<seasEdge_t>			edges;
	anList<seasIndex_t>			edgeIndex;
	anList<seasFace_t>			faces;
	anList<seasIndex_t>			faceIndex;
	anList<seasArea_t>			areas;
	anList<seasNode_t>			nodes;
	anList<seasPortal_t>			portals;
	anList<seasIndex_t>			portalIndex;
	anList<seasCluster_t>		clusters;
	anSEASSettings				settings;
};

#endif /* !__AASFILE_H__ */
