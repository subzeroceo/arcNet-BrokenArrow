
#include "CollisionModel.h"


class anSoftBodiesPhysicsManager {
public:
	// load collision models from a map file
	void			LoadMap( const anMapFile *mapFile );
	// frees all the collision models
	void			FreeMap( void );

	// get clip handle for model
	cmHandle_t		LoadModel( const char *modelName, const bool precache );
	// sets up a trace model for collision with other trace models
	cmHandle_t		SetupTrmModel( const anTraceModel &trm, const anMaterial *material );
	// create trace model from a collision model, returns true if succesfull
	bool			TrmFromModel( const char *modelName, anTraceModel &trm );

	// name of the model
	const char *	GetModelName( cmHandle_t model ) const;
	// bounds of the model
	bool			GetModelBounds( cmHandle_t model, anBounds &bounds ) const;
	// all contents flags of brushes and polygons ored together
	bool			GetModelContents( cmHandle_t model, int &contents ) const;
	// get the vertex of a model
	bool			GetModelVertex( cmHandle_t model, int vertexNum, anVec3 &vertex ) const;
	// get the edge of a model
	bool			GetModelEdge( cmHandle_t model, int edgeNum, anVec3 &start, anVec3 &end ) const;
	// get the polygon of a model
	bool			GetModelPolygon( cmHandle_t model, int polygonNum, anFixedWinding &winding ) const;

	// translates a trm and reports the first collision if any
	void			Translation( trace_t *results, const anVec3 &start, const anVec3 &end,
								const anTraceModel *trm, const anMat3 &trmAxis, int contentMask,
								cmHandle_t model, const anVec3 &modelOrigin, const anMat3 &modelAxis );
	// rotates a trm and reports the first collision if any
	void			Rotation( trace_t *results, const anVec3 &start, const anRotation &rotation,
								const anTraceModel *trm, const anMat3 &trmAxis, int contentMask,
								cmHandle_t model, const anVec3 &modelOrigin, const anMat3 &modelAxis );
	// returns the contents the trm is stuck in or 0 if the trm is in free space
	int				Contents( const anVec3 &start,
								const anTraceModel *trm, const anMat3 &trmAxis, int contentMask,
								cmHandle_t model, const anVec3 &modelOrigin, const anMat3 &modelAxis );
	// stores all contact points of the trm with the model, returns the number of contacts
	int				Contacts( contactInfo_t *contacts, const int maxContacts, const anVec3 &start, const anVec6 &dir, const float depth,
								const anTraceModel *trm, const anMat3 &trmAxis, int contentMask,
								cmHandle_t model, const anVec3 &modelOrigin, const anMat3 &modelAxis );
	// test collision detection
	void			DebugOutput( const anVec3 &origin );
	// draw a model
	void			DrawModel( cmHandle_t model, const anVec3 &origin, const anMat3 &axis,
											const anVec3 &viewOrigin, const float radius );
	// print model information, use -1 handle for accumulated model info
	void			ModelInfo( cmHandle_t model );
	// list all loaded models
	void			ListModels( void );
	// write a collision model file for the map entity
	bool			WriteCollisionModelForMapEntity( const anMapEntity *mapEnt, const char *filename, const bool testTraceModel = true );

private:			// CollisionMap_translate.cpp
	int				TranslateEdgeThroughEdge( anVec3 &cross, anPluecker &l1, anPluecker &l2, float *fraction );
	void			TranslateTrmEdgeThroughPolygon( cm_traceWork_t *tw, cm_polygon_t *poly, cm_trmEdge_t *trmEdge );
	void			TranslateTrmVertexThroughPolygon( cm_traceWork_t *tw, cm_polygon_t *poly, cm_trmVertex_t *v, int bitNum );
	void			TranslatePointThroughPolygon( cm_traceWork_t *tw, cm_polygon_t *poly, cm_trmVertex_t *v );
	void			TranslateVertexThroughTrmPolygon( cm_traceWork_t *tw, cm_trmPolygon_t *trmpoly, cm_polygon_t *poly, cm_vertex_t *v, anVec3 &endp, anPluecker &pl );
	bool			TranslateTrmThroughPolygon( cm_traceWork_t *tw, cm_polygon_t *p );
	void			SetupTranslationHeartPlanes( cm_traceWork_t *tw );
	void			SetupTrm( cm_traceWork_t *tw, const anTraceModel *trm );

private:			// CollisionMap_rotate.cpp
	int				CollisionBetweenEdgeBounds( cm_traceWork_t *tw, const anVec3 &va, const anVec3 &vb,
											const anVec3 &vc, const anVec3 &vd, float tanHalfAngle,
											anVec3 &collisionPoint, anVec3 &collisionNormal );
	int				RotateEdgeThroughEdge( cm_traceWork_t *tw, const anPluecker &pl1,
											const anVec3 &vc, const anVec3 &vd,
											const float minTan, float &tanHalfAngle );
	int				EdgeFurthestFromEdge( cm_traceWork_t *tw, const anPluecker &pl1,
											const anVec3 &vc, const anVec3 &vd,
											float &tanHalfAngle, float &dir );
	void			RotateTrmEdgeThroughPolygon( cm_traceWork_t *tw, cm_polygon_t *poly, cm_trmEdge_t *trmEdge );
	int				RotatePointThroughPlane( const cm_traceWork_t *tw, const anVec3 &point, const anPlane &plane,
											const float angle, const float minTan, float &tanHalfAngle );
	int				PointFurthestFromPlane( const cm_traceWork_t *tw, const anVec3 &point, const anPlane &plane,
											const float angle, float &tanHalfAngle, float &dir );
	int				RotatePointThroughEpsilonPlane( const cm_traceWork_t *tw, const anVec3 &point, const anVec3 &endPoint,
											const anPlane &plane, const float angle, const anVec3 &origin,
											float &tanHalfAngle, anVec3 &collisionPoint, anVec3 &endDir );
	void			RotateTrmVertexThroughPolygon( cm_traceWork_t *tw, cm_polygon_t *poly, cm_trmVertex_t *v, int vertexNum);
	void			RotateVertexThroughTrmPolygon( cm_traceWork_t *tw, cm_trmPolygon_t *trmpoly, cm_polygon_t *poly,
											cm_vertex_t *v, anVec3 &rotationOrigin );
	bool			RotateTrmThroughPolygon( cm_traceWork_t *tw, cm_polygon_t *p );
	void			BoundsForRotation( const anVec3 &origin, const anVec3 &axis, const anVec3 &start, const anVec3 &end, anBounds &bounds );
	void			Rotation180( trace_t *results, const anVec3 &rorg, const anVec3 &axis,
									const float startAngle, const float endAngle, const anVec3 &start,
									const anTraceModel *trm, const anMat3 &trmAxis, int contentMask,
									cmHandle_t model, const anVec3 &origin, const anMat3 &modelAxis );

private:			// CollisionMap_contents.cpp
	bool			TestTrmVertsInBrush( cm_traceWork_t *tw, cm_brush_t *b );
	bool			TestTrmInPolygon( cm_traceWork_t *tw, cm_polygon_t *p );
	cm_node_t *		PointNode( const anVec3 &p, cm_model_t *model );
	int				PointContents( const anVec3 p, cmHandle_t model );
	int				TransformedPointContents( const anVec3 &p, cmHandle_t model, const anVec3 &origin, const anMat3 &modelAxis );
	int				ContentsTrm( trace_t *results, const anVec3 &start,
									const anTraceModel *trm, const anMat3 &trmAxis, int contentMask,
									cmHandle_t model, const anVec3 &modelOrigin, const anMat3 &modelAxis );

private:			// CollisionMap_trace.cpp
	void			TraceTrmThroughNode( cm_traceWork_t *tw, cm_node_t *node );
	void			TraceThroughAxialBSPTree_r( cm_traceWork_t *tw, cm_node_t *node, float p1f, float p2f, anVec3 &p1, anVec3 &p2);
	void			TraceThroughModel( cm_traceWork_t *tw );
	void			RecurseProcBSP_r( trace_t *results, int parentNodeNum, int nodeNum, float p1f, float p2f, const anVec3 &p1, const anVec3 &p2 );

private:			// CollisionMap_load.cpp
	void			Clear( void );
	void			FreeTrmModelStructure( void );

					// model deallocation
	void			RemovePolygonReferences_r( cm_node_t *node, cm_polygon_t *p );
	void			RemoveBrushReferences_r( cm_node_t *node, cm_brush_t *b );
	void			FreeNode( cm_node_t *node );
	void			FreePolygonReference( cm_polygonRef_t *pref );
	void			FreeBrushReference( cm_brushRef_t *bref );
	void			FreePolygon( cm_model_t *model, cm_polygon_t *poly );
	void			FreeBrush( cm_model_t *model, cm_brush_t *brush );
	void			FreeTree_r( cm_model_t *model, cm_node_t *headNode, cm_node_t *node );
	void			FreeModel( cm_model_t *model );

					// merging polygons
	void			ReplacePolygons( cm_model_t *model, cm_node_t *node, cm_polygon_t *p1, cm_polygon_t *p2, cm_polygon_t *newp );
	cm_polygon_t *	TryMergePolygons( cm_model_t *model, cm_polygon_t *p1, cm_polygon_t *p2 );
	bool			MergePolygonWithTreePolygons( cm_model_t *model, cm_node_t *node, cm_polygon_t *polygon );
	void			MergeTreePolygons( cm_model_t *model, cm_node_t *node );

					// finding internal edges
	bool			PointInsidePolygon( cm_model_t *model, cm_polygon_t *p, anVec3 &v );
	void			FindInternalEdgesOnPolygon( cm_model_t *model, cm_polygon_t *p1, cm_polygon_t *p2 );
	void			FindInternalPolygonEdges( cm_model_t *model, cm_node_t *node, cm_polygon_t *polygon );
	void			FindInternalEdges( cm_model_t *model, cm_node_t *node );
	void			FindContainedEdges( cm_model_t *model, cm_polygon_t *p );

					// loading of proc BSP tree
	void			ParseProcNodes( anLexer *src );
	void			LoadProcBSP( const char *name );

					// removal of contained polygons
	int				R_ChoppedAwayByProcBSP( int nodeNum, anFixedWinding *w, const anVec3 &normal, const anVec3 &origin, const float radius );
	int				ChoppedAwayByProcBSP( const anFixedWinding &w, const anPlane &plane, int contents );
	void			ChopWindingListWithBrush( cm_windingList_t *list, cm_brush_t *b );
	void			R_ChopWindingListWithTreeBrushes( cm_windingList_t *list, cm_node_t *node );
	anFixedWinding *WindingOutsideBrushes( anFixedWinding *w, const anPlane &plane, int contents, int patch, cm_node_t *headNode );

					// creation of axial BSP tree
	cm_model_t *	AllocModel( void );
	cm_node_t *		AllocNode( cm_model_t *model, int blockSize );
	cm_polygonRef_t*AllocPolygonReference( cm_model_t *model, int blockSize );
	cm_brushRef_t *	AllocBrushReference( cm_model_t *model, int blockSize );
	cm_polygon_t *	AllocPolygon( cm_model_t *model, int numEdges );
	cm_brush_t *	AllocBrush( cm_model_t *model, int numPlanes );
	void			AddPolygonToNode( cm_model_t *model, cm_node_t *node, cm_polygon_t *p );
	void			AddBrushToNode( cm_model_t *model, cm_node_t *node, cm_brush_t *b );
	void			SetupTrmModelStructure( void );
	void			R_FilterPolygonIntoTree( cm_model_t *model, cm_node_t *node, cm_polygonRef_t *pref, cm_polygon_t *p );
	void			R_FilterBrushIntoTree( cm_model_t *model, cm_node_t *node, cm_brushRef_t *pref, cm_brush_t *b );
	cm_node_t *		R_CreateAxialBSPTree( cm_model_t *model, cm_node_t *node, const anBounds &bounds );
	cm_node_t *		CreateAxialBSPTree( cm_model_t *model, cm_node_t *node );

					// creation of raw polygons
	void			SetupHash( void );
	void			ShutdownHash( void );
	void			ClearHash( anBounds &bounds );
	int				HashVec(const anVec3 &vec);
	int				GetVertex( cm_model_t *model, const anVec3 &v, int *vertexNum );
	int				GetEdge( cm_model_t *model, const anVec3 &v1, const anVec3 &v2, int *edgeNum, int v1num );
	void			CreatePolygon( cm_model_t *model, anFixedWinding *w, const anPlane &plane, const anMaterial *material, int primitiveNum );
	void			PolygonFromWinding( cm_model_t *model, anFixedWinding *w, const anPlane &plane, const anMaterial *material, int primitiveNum );
	void			CalculateEdgeNormals( cm_model_t *model, cm_node_t *node );
	void			CreatePatchPolygons( cm_model_t *model, anSurface_Patch &mesh, const anMaterial *material, int primitiveNum );
	void			ConvertPatch( cm_model_t *model, const anMapPatch *patch, int primitiveNum );
	void			ConvertBrushSides( cm_model_t *model, const anMapBrush *mapBrush, int primitiveNum );
	void			ConvertBrush( cm_model_t *model, const anMapBrush *mapBrush, int primitiveNum );
	void			PrintModelInfo( const cm_model_t *model );
	void			AccumulateModelInfo( cm_model_t *model );
	void			RemapEdges( cm_node_t *node, int *edgeRemap );
	void			OptimizeArrays( cm_model_t *model );
	void			FinishModel( cm_model_t *model );
	void			BuildModels( const anMapFile *mapFile );
	cmHandle_t		FindModel( const char *name );
	cm_model_t *	CollisionModelForMapEntity( const anMapEntity *mapEnt );	// brush/patch model from .map
	cm_model_t *	LoadRenderModel( const char *fileName );					// ASE/LWO models
	bool			TrmFromModel_r( anTraceModel &trm, cm_node_t *node );
	bool			TrmFromModel( const cm_model_t *model, anTraceModel &trm );

private:			// CollisionMap_files.cpp
					// writing
	void			WriteNodes( anFile *fp, cm_node_t *node );
	int				CountPolygonMemory( cm_node_t *node ) const;
	void			WritePolygons( anFile *fp, cm_node_t *node );
	int				CountBrushMemory( cm_node_t *node ) const;
	void			WriteBrushes( anFile *fp, cm_node_t *node );
	void			WriteCollisionModel( anFile *fp, cm_model_t *model );
	void			WriteCollisionModelsToFile( const char *filename, int firstModel, int lastModel, unsigned int mapFileCRC );

					// loading
	cm_node_t *		ParseNodes( anLexer *src, cm_model_t *model, cm_node_t *parent );
	void			ParseVertices( anLexer *src, cm_model_t *model );
	void			ParseEdges( anLexer *src, cm_model_t *model );
	void			ParsePolygons( anLexer *src, cm_model_t *model );
	void			ParseBrushes( anLexer *src, cm_model_t *model );
	bool			ParseCollisionModel( anLexer *src );
	bool			LoadCollisionModelFile( const char *name, unsigned int mapFileCRC );

private:			// CollisionMap_debug
	int				ContentsFromString( const char *string ) const;
	const char *	StringFromContents( const int contents ) const;
	void			DrawEdge( cm_model_t *model, int edgeNum, const anVec3 &origin, const anMat3 &axis );
	void			DrawPolygon( cm_model_t *model, cm_polygon_t *p, const anVec3 &origin, const anMat3 &axis,
								const anVec3 &viewOrigin );
	void			DrawNodePolygons( cm_model_t *model, cm_node_t *node, const anVec3 &origin, const anMat3 &axis,
								const anVec3 &viewOrigin, const float radius );

};

