#ifndef __AASBUILD_LOCAL_H__
#define __AASBUILD_LOCAL_H__

#include "SEASFile.h"
#include "SEASFile_local.h"

#include "Brush.h"
#include "BrushBSP.h"
#include "AASReach.h"
#include "AASCluster.h"


//===============================================================
//
//	anSEASBuild
//
//===============================================================

typedef struct aasProcNode_s {
	anPlane plane;
	int children[2];		// negative numbers are (-1 - areaNumber), 0 = solid
} aasProcNode_t;


class idLedge {

public:
	anVec3					start;
	anVec3					end;
	idBrushBSPNode *		node;
	int						numExpandedPlanes;
	int						numSplitPlanes;
	int						numPlanes;
	anPlane					planes[8];

public:
							idLedge( void );
							idLedge( const anVec3 &v1, const anVec3 &v2, const anVec3 &gravityDir, idBrushBSPNode *n );
	void					AddPoint( const anVec3 &v );
	void					CreateBevels( const anVec3 &gravityDir );
	void					Expand( const anBounds &bounds, float maxStepHeight );
	anWinding *				ChopWinding( const anWinding *winding ) const;
	bool					PointBetweenBounds( const anVec3 &v ) const;
};

class anSEASBuild {

public:
							anSEASBuild( void );
							~anSEASBuild( void );
	bool					Build( const anString &fileName, const anSEASSettings *settings );
	bool					BuildReachability( const anString &fileName, const anSEASSettings *settings );
	void					Shutdown( void );

private:
	const anSEASSettings *	aasSettings;
	anSEASFileLocal *		file;
	aasProcNode_t *			procNodes;
	int						numProcNodes;
	int						numGravitationalSubdivisions;
	int						numMergedLeafNodes;
	int						numLedgeSubdivisions;
	anList<idLedge>			ledgeList;
	idBrushMap *			ledgeMap;

private:	// map loading
	void					ParseProcNodes( anLexer *src );
	bool					LoadProcBSP( const char *name, ARC_TIME_T minFileTime );
	void					DeleteProcBSP( void );
	bool					ChoppedAwayByProcBSP( int nodeNum, anFixedWinding *w, const anVec3 &normal, const anVec3 &origin, const float radius );
	void					ClipBrushSidesWithProcBSP( idBrushList &brushList );
	int						ContentsForAAS( int contents );
	idBrushList				AddBrushesForMapBrush( const anMapBrush *mapBrush, const anVec3 &origin, const anMat3 &axis, int entityNum, int primitiveNum, idBrushList brushList );
	idBrushList				AddBrushesForMapPatch( const anMapPatch *mapPatch, const anVec3 &origin, const anMat3 &axis, int entityNum, int primitiveNum, idBrushList brushList );
	idBrushList				AddBrushesForMapEntity( const anMapEntity *mapEnt, int entityNum, idBrushList brushList );
	idBrushList				AddBrushesForMapFile( const anMapFile * mapFile, idBrushList brushList );
	bool					CheckForEntities( const anMapFile *mapFile, anStringList &entityClassNames ) const;
	void					ChangeMultipleBoundingBoxContents_r( idBrushBSPNode *node, int mask );

private:	// gravitational subdivision
	void					SetPortalFlags_r( idBrushBSPNode *node );
	bool					PortalIsGap( idBrushBSPPortal *portal, int side );
	void					GravSubdivLeafNode( idBrushBSPNode *node );
	void					GravSubdiv_r( idBrushBSPNode *node );
	void					GravitationalSubdivision( idBrushBSP &bsp );

private:	// ledge subdivision
	void					LedgeSubdivFlood_r( idBrushBSPNode *node, const idLedge *ledge );
	void					LedgeSubdivLeafNodes_r( idBrushBSPNode *node, const idLedge *ledge );
	void					LedgeSubdiv( idBrushBSPNode *root );
	bool					IsLedgeSide_r( idBrushBSPNode *node, anFixedWinding *w, const anPlane &plane, const anVec3 &normal, const anVec3 &origin, const float radius );
	void					AddLedge( const anVec3 &v1, const anVec3 &v2, idBrushBSPNode *node );
	void					FindLeafNodeLedges( idBrushBSPNode *root, idBrushBSPNode *node );
	void					FindLedges_r( idBrushBSPNode *root, idBrushBSPNode *node );
	void					LedgeSubdivision( idBrushBSP &bsp );
	void					WriteLedgeMap( const anString &fileName, const anString &ext );

private:	// merging
	bool					AllGapsLeadToOtherNode( idBrushBSPNode *nodeWithGaps, idBrushBSPNode *otherNode );
	bool					MergeWithAdjacentLeafNodes( idBrushBSP &bsp, idBrushBSPNode *node );
	void					MergeLeafNodes_r( idBrushBSP &bsp, idBrushBSPNode *node );
	void					MergeLeafNodes( idBrushBSP &bsp );

private:	// storing file
	void					SetupHash( void );
	void					ShutdownHash( void );
	void					ClearHash( const anBounds &bounds );
	int						HashVec( const anVec3 &vec );
	bool					GetVertex( const anVec3 &v, int *vertexNum );
	bool					GetEdge( const anVec3 &v1, const anVec3 &v2, int *edgeNum, int v1num );
	bool					GetFaceForPortal( idBrushBSPPortal *portal, int side, int *faceNum );
	bool					GetAreaForLeafNode( idBrushBSPNode *node, int *areaNum );
	int						StoreTree_r( idBrushBSPNode *node );
	void					GetSizeEstimate_r( idBrushBSPNode *parent, idBrushBSPNode *node, struct sizeEstimate_s &size );
	void					SetSizeEstimate( const idBrushBSP &bsp, anSEASFileLocal *file );
	bool					StoreFile( const idBrushBSP &bsp );

};

#endif // !__AASBUILD_LOCAL_H__
