#ifndef __COLLISIONMODELMANAGER_H__
#define __COLLISIONMODELMANAGER_H__

/*
===============================================================================

	Collision model, Trace model vs. polygonal model collision detection.

===============================================================================
*/

#define MIN_NODE_SIZE						64.0f
#define MAX_NODE_POLYGONS					128
#define CM_MAX_POLYGON_EDGES				64
#define CIRCLE_APPROXIMATION_LENGTH			64.0f

#define	MAX_SUBMODELS						2048
#define	TRACE_MODEL_HANDLE					MAX_SUBMODELS

#define VERTEX_HASH_BOXSIZE					(1<<6)	// must be power of 2
#define VERTEX_HASH_SIZE					(VERTEX_HASH_BOXSIZE*VERTEX_HASH_BOXSIZE)
#define EDGE_HASH_SIZE						(1<<14)

#define NODE_BLOCK_SIZE_SMALL				8
#define NODE_BLOCK_SIZE_LARGE				256
#define REFERENCE_BLOCK_SIZE_SMALL			8
#define REFERENCE_BLOCK_SIZE_LARGE			256

#define MAX_WINDING_LIST					128		// quite a few are generated at times
#define INTEGRAL_EPSILON					0.01f
#define VERTEX_EPSILON						0.1f
#define CHOP_EPSILON						0.1f


typedef struct cm_windingList_s {
	int					numWindings;			// number of windings
	anFixedWinding		w[MAX_WINDING_LIST];	// windings
	anVec3				normal;					// normal for all windings
	anBounds			bounds;					// bounds of all windings in list
	anVec3				origin;					// origin for radius
	float				radius;					// radius relative to origin for all windings
	int					contents;				// winding surface contents
	int					primitiveNum;			// number of primitive the windings came from
} cm_windingList_t;

typedef struct cm_vertex_s {
	anVec3					p;					// vertex point
	int						checkcount;			// for multi-check avoidance
	unsigned long			side;				// each bit tells at which side this vertex passes one of the trace model edges
	unsigned long			sideSet;			// each bit tells if sidedness for the trace model edge has been calculated yet
} cm_vertex_t;

typedef struct cm_edge_s {
	int						checkcount;			// for multi-check avoidance
	unsigned short			internal;			// a trace model can never collide with internal edges
	unsigned short			numUsers;			// number of polygons using this edge
	unsigned long			side;				// each bit tells at which side of this edge one of the trace model vertices passes
	unsigned long			sideSet;			// each bit tells if sidedness for the trace model vertex has been calculated yet
	int						vertexNum[2];		// start and end point of edge
	anVec3					normal;				// edge normal
} cm_edge_t;

typedef struct cm_polygonBlock_s {
	int						bytesRemaining;
	byte *					next;
} cm_polygonBlock_t;

typedef struct cm_polygon_s {
	anBounds				bounds;				// polygon bounds
	int						checkcount;			// for multi-check avoidance
	int						contents;			// contents behind polygon
	const anMaterial *		material;			// material
	anPlane					plane;				// polygon plane
	int						numEdges;			// number of edges
	int						edges[1];			// variable sized, indexes into cm_edge_t list
} cm_polygon_t;

typedef struct cm_polygonRef_s {
	cm_polygon_t *			p;					// pointer to polygon
	struct cm_polygonRef_s *next;				// next polygon in chain
} cm_polygonRef_t;

typedef struct cm_polygonRefBlock_s {
	cm_polygonRef_t *		nextRef;			// next polygon reference in block
	struct cm_polygonRefBlock_s *next;			// next block with polygon references
} cm_polygonRefBlock_t;

typedef struct cm_brushBlock_s {
	int						bytesRemaining;
	byte *					next;
} cm_brushBlock_t;

typedef struct cm_brush_s {
	int						checkcount;			// for multi-check avoidance
	anBounds				bounds;				// brush bounds
	int						contents;			// contents of brush
	const anMaterial *		material;			// material
	int						primitiveNum;		// number of brush primitive
	int						numPlanes;			// number of bounding planes
	anPlane					planes[1];			// variable sized
} cm_brush_t;

typedef struct cm_brushRef_s {
	cm_brush_t *			b;					// pointer to brush
	struct cm_brushRef_s *	next;				// next brush in chain
} cm_brushRef_t;

typedef struct cm_brushRefBlock_s {
	cm_brushRef_t *			nextRef;			// next brush reference in block
	struct cm_brushRefBlock_s *next;			// next block with brush references
} cm_brushRefBlock_t;

typedef struct cm_node_s {
	int						planeType;			// node axial plane type
	float					planeDist;			// node plane distance
	cm_polygonRef_t *		polygons;			// polygons in node
	cm_brushRef_t *			brushes;			// brushes in node
	struct cm_node_s *		parent;				// parent of this node
	struct cm_node_s *		children[2];		// node children
} cm_node_t;

typedef struct cm_nodeBlock_s {
	cm_node_t *				nextNode;			// next node in block
	struct cm_nodeBlock_s *next;				// next block with nodes
} cm_nodeBlock_t;

typedef struct cm_model_s {
	anString				name;				// model name
	anBounds				bounds;				// model bounds
	int						contents;			// all contents of the model ored together
	bool					isConvex;			// set if model is convex
	// model geometry
	int						maxVertices;		// size of vertex array
	int						numVertices;		// number of vertices
	cm_vertex_t *			vertices;			// array with all vertices used by the model
	int						maxEdges;			// size of edge array
	int						numEdges;			// number of edges
	cm_edge_t *				edges;				// array with all edges used by the model
	cm_node_t *				node;				// first node of spatial subdivision
	// blocks with allocated memory
	cm_nodeBlock_t *		nodeBlocks;			// list with blocks of nodes
	cm_polygonRefBlock_t *	polygonRefBlocks;	// list with blocks of polygon references
	cm_brushRefBlock_t *	brushRefBlocks;		// list with blocks of brush references
	cm_polygonBlock_t *		polygonBlock;		// memory block with all polygons
	cm_brushBlock_t *		brushBlock;			// memory block with all brushes
	// statistics
	int						numPolygons;
	int						polygonMemory;
	int						numBrushes;
	int						brushMemory;
	int						numNodes;
	int						numBrushRefs;
	int						numPolygonRefs;
	int						numInternalEdges;
	int						numSharpEdges;
	int						numRemovedPolys;
	int						numMergedPolys;
	int						usedMemory;
} cm_model_t;

/*
===============================================================================

Data used during collision detection calculations

===============================================================================
*/

typedef struct cm_trmVertex_s {
	int used;										// true if this vertex is used for collision detection
	anVec3 p;										// vertex position
	anVec3 endp;									// end point of vertex after movement
	int polygonSide;								// side of polygon this vertex is on (rotational collision)
	anPluecker pl;									// pluecker coordinate for vertex movement
	anVec3 rotationOrigin;							// rotation origin for this vertex
	anBounds rotationBounds;						// rotation bounds for this vertex
} cm_trmVertex_t;

typedef struct cm_trmEdge_s {
	int used;										// true when vertex is used for collision detection
	anVec3 start;									// start of edge
	anVec3 end;										// end of edge
	int vertexNum[2];								// indexes into cm_traceWork_t->vertices
	anPluecker pl;									// pluecker coordinate for edge
	anVec3 cross;									// (z,-y,x) of cross product between edge dir and movement dir
	anBounds rotationBounds;						// rotation bounds for this edge
	anPluecker plzaxis;								// pluecker coordinate for rotation about the z-axis
	unsigned short bitNum;							// vertex bit number
} cm_trmEdge_t;

typedef struct cm_trmPolygon_s {
	int used;
	anPlane plane;									// polygon plane
	int numEdges;									// number of edges
	int edges[MAX_TRACEMODEL_POLYEDGES];			// index into cm_traceWork_t->edges
	anBounds rotationBounds;						// rotation bounds for this polygon
} cm_trmPolygon_t;

typedef struct cm_traceWork_s {
	int numVerts;
	cm_trmVertex_t vertices[MAX_TRACEMODEL_VERTS];	// trm vertices
	int numEdges;
	cm_trmEdge_t edges[MAX_TRACEMODEL_EDGES+1];		// trm edges
	int numPolys;
	cm_trmPolygon_t polys[MAX_TRACEMODEL_POLYS];	// trm polygons
	cm_model_t *model;								// model colliding with
	anVec3 start;									// start of trace
	anVec3 end;										// end of trace
	anVec3 dir;										// trace direction
	anBounds bounds;								// bounds of full trace
	anBounds size;									// bounds of transformed trm relative to start
	anVec3 extents;									// largest of abs( size[0]) and abs( size[1]) for BSP trace
	int contents;									// ignore polygons that do not have any of these contents flags
	trace_t trace;									// collision detection result

	bool rotation;									// true if calculating rotational collision
	bool pointTrace;								// true if only tracing a point
	bool positionTest;								// true if not tracing but doing a position test
	bool isConvex;									// true if the trace model is convex
	bool axisIntersectsTrm;							// true if the rotation axis intersects the trace model
	bool getContacts;								// true if retrieving contacts
	bool quickExit;									// set to quickly stop the collision detection calculations

	anVec3 origin;									// origin of rotation in model space
	anVec3 axis;									// rotation axis in model space
	anMat3 matrix;									// rotates axis of rotation to the z-axis
	float angle;									// angle for rotational collision
	float maxTan;									// max tangent of half the positive angle used instead of fraction
	float radius;									// rotation radius of trm start
	anRotation modelVertexRotation;					// inverse rotation for model vertices

	contactInfo_t *contacts;						// array with contacts
	int maxContacts;								// max size of contact array
	int numContacts;								// number of contacts found

	anPlane heartPlane1;							// polygons should be near anough the trace heart planes
	float maxDistFromHeartPlane1;
	anPlane heartPlane2;
	float maxDistFromHeartPlane2;
	anPluecker polygonEdgePlueckerCache[CM_MAX_POLYGON_EDGES];
	anPluecker polygonVertexPlueckerCache[CM_MAX_POLYGON_EDGES];
	anVec3 polygonRotationOriginCache[CM_MAX_POLYGON_EDGES];
} cm_traceWork_t;

/*
===============================================================================

Collision Map

===============================================================================
*/

typedef struct cm_procNode_s {
	anPlane plane;
	int children[2];				// negative numbers are (-1 - areaNumber), 0 = solid
} cm_procNode_t;
/*
===============================================================================

	Trace model vs. polygonal model collision detection.

	Short translations are the least expensive. Retrieving contact points is
	about as cheap as a short translation. Position tests are more expensive
	and rotations are most expensive.

	There is no position test at the start of a translation or rotation. In other
	words if a translation with start != end or a rotation with angle != 0 starts
	in solid, this goes unnoticed and the collision result is undefined.

	A translation with start == end or a rotation with angle == 0 performs
	a position test and fills in the trace_t structure accordingly.

===============================================================================
*/

struct springPhysics_t {
	int vertex1;
    int vertex2;
    float restLength;
  };

// SoftBodyVertex struct
typedef struct {
	anVec3			pos;
	anVec3			velocity;
	anVec3			force;
	anVec3			origin;
	anMat3			axis;
	anVec3			localOrigin;
	anMat3			localAxis;
	//int numSBVerts, total;
} softBodyVertex_t;

// contact type
typedef enum {
	CONTACT_NONE,							// no contact
	CONTACT_EDGE,							// trace model edge hits model edge
	CONTACT_MODELVERTEX,					// model vertex hits trace model polygon
	CONTACT_TRMVERTEX,
	CONTACT_SBVERTEX,						// trace model vertex hits model polygon
	CONTACT_SB
} contactType_t;

// contact info
typedef struct {
	contactType_t			type;			// contact type
	anVec3					point;			// point of contact
	anVec3					normal;			// contact plane normal
	float					dist;			// contact plane distance
	float					separation;		// contact feature separation at initial position
	int						contents;		// contents at other side of surface
	const anMaterial *		material;		// surface material
	const anDeclSurfaceType *surfaceType;	// surface shader/material type
	int 					surfaceColor	// surface color
	int						modelFeature;	// contact feature on model
	int						trmFeature;		// contact feature on trace model
	int						entityNum;		// entity the contact surface is a part of
	int						id;				// id of clip model the contact surface is part of
	int						selfId;
} contactInfo_t;

// trace result
typedef struct trace_s {
	float					fraction;		// fraction of movement completed, 1.0 = didn't hit anything
	anVec3					endpos;			// final position of trace model
	anMat3					endAxis;		// final axis of trace model
	contactInfo_t			c;				// contact information, only valid if fraction < 1.0
} trace_t;

typedef int cmHandle_t;

#define CM_CLIP_EPSILON		0.25f			// always stay this distance away from any model
#define CM_BOX_EPSILON		1.0f			// should always be larger than clip epsilon
#define CM_MAX_TRACE_DIST	4096.0f			// maximum distance a trace model may be traced, point traces are unlimited

// collision model
class anCollisionModel {
public:
						~anCollisionModel() { }
								// Returns the name of the model.
	const char *				GetName( void ) const = 0;

								// Gets the bounds of the model.
	const anBounds &			GetBounds( void ) const = 0;

								// Gets the bounds of the model, excluding/including surfaces of the appropriate surface type
	void						GetBounds( anBounds& bounds, int surfaceMask, bool inclusive ) const = 0;

								// Gets all contents flags of brushes and polygons of the model ored together.
	int							GetContents( void ) const = 0;

								// Gets a vertex of the model.
	const anVec3 &				GetVertex( int vertexNum ) const = 0;

								// Gets an edge of the model.
	void						GetEdge( int edgeNum, anVec3& start, anVec3& end ) const = 0;

								// Gets a polygon of the model.
	void						GetPolygon( int polygonNum, anFixedWinding &winding ) const = 0;

								// Draws surfaces which don't/do have the surface mask
	void						Draw( int surfaceMask, bool inclusive ) const = 0;

	int							GetNumBrushPlanes( void ) const = 0;
	const anPlane&				GetBrushPlane( int planeNum ) const = 0;

	const anMaterial *			GetPolygonMaterial( int polygonNum ) const = 0;
	const anPlane&				GetPolygonPlane( int polygonNum ) const = 0;
	int							GetNumPolygons( void ) const = 0;

	void						GetNumSoftBodyVerts() const { return numVertices;//softBodyVerts.Num(); }

	bool						IsTraceModel( void ) const = 0;
	bool						IsConvex( void ) const = 0;

	bool						IsWorld( void ) const = 0;
	void						SetWorld( bool tf ) = 0;

	bool						IsColliding( void ) const = 0;
	//CalculateCollisionForce(anvec3, model);
								// arC-Net CollisionDetection
	 bool 						CheckSoftBodyCollisions( anSoftBody *softBody ) const = 0;
	// integrate softbodies
	softBodyVertex_t *			GetSoftBodies() const = 0;

								// Gets a soft vertex of the soft body model.
	softBodyVertex_t *			GetSoftBodyVertex( int vertexNum ) const = 0;
	void		 				IntegrateSoftBodies() const = 0;

	softBodyVertex_t *			softBodyVerts;
	softBodyVertex_t *			vertices;
	int 						numVertices;
	springPhysics_t *			springs;
	int numSprings;
	float mass;
	float stiffness;
	float damping;
};

class anCollisionModelManager {
public:
	virtual					~anCollisionModelManager( void ) {}

	void						Init( void ) = 0;
	void						Shutdown( void ) = 0;

	void						AllocThread( void ) = 0;
	void						FreeThread( void ) = 0;
	int						GetThreadId( void ) = 0;
	int						GetThreadCount( void ) = 0;

							// Loads collision models from a map file.
	void					LoadMap( const anMapFile *mapFile ) = 0;
							// Frees all the collision models.
	void					FreeMap( void ) = 0;

								// Gets the clip handle for a model.
	cmHandle_t				LoadModel( const char *modelName, const bool precache ) = 0;

	//virtual anCollisionModel *ExtractSBCollisionModel( anRenderModel *renderModel, const char *modelName ) = 0;

							// Precaches a collision model.
	void					PreCacheModel( const char *mapName, const char *modelName ) = 0;

							// Free the given model.
	void					FreeModel( anCollisionModel *model ) = 0;

							// Purge all unused models.
	void					PurgeModels( void ) = 0;

	// Sets up a trace model for collision with other trace models.
	cmHandle_t				SetupTrmModel( const anTraceModel &trm, const anMaterial *material ) = 0;

	// Creates a trace model from a collision model, returns true if succesfull.
	bool					TrmFromModel( const char *modelName, anTraceModel &trm ) = 0;

							// Sets up a trace model for collision with other trace models.
	anCollisionModel *		ModelFromTrm( const char *mapName, const char *modelName, const anTraceModel &trm, const anMaterial *material ) = 0;
							// Creates a trace model from a collision model, returns true if succesfull.
	bool					TrmFromModel( const char *mapName, const char *modelName, anTraceModel &trm ) = 0;
							// Creates a trace model for each primitive of the collision model, returns the number of trace models.
	int						CompoundTrmFromModel( const char *mapName, const char *modelName, anTraceModel *trms, int maxTrms ) = 0;

	// Gets the name of a model.
	const char *			GetModelName( cmHandle_t model ) const = 0;

	// Gets the bounds of a model.
	bool					GetModelBounds( cmHandle_t model, anBounds &bounds ) const = 0;

	// Gets all contents flags of brushes and polygons of a model ored together.
	bool					GetModelContents( cmHandle_t model, int &contents ) const = 0;

	// Gets a vertex of a model.
	bool					GetModelVertex( cmHandle_t model, int vertexNum, anVec3 &vertex ) const = 0;

	// Gets an edge of a model.
	bool					GetModelEdge( cmHandle_t model, int edgeNum, anVec3 &start, anVec3 &end ) const = 0;

	// Gets a polygon of a model.
	bool					GetModelPolygon( cmHandle_t model, int polygonNum, anFixedWinding &winding ) const = 0;

	const anMaterial *		GetPolygonMaterial( int polygonNum ) const = 0;
	const anPlane &			GetPolygonPlane( int polygonNum ) const = 0;
	int						GetNumPolygons( void ) const = 0;

	int						GetNumBrushPlanes( void ) const = 0;
	const anPlane &			GetBrushPlane( int planeNum ) const = 0;

	// Translates a trace model and reports the first collision if any.
	void					Translation( trace_t *results, const anVec3 &start, const anVec3 &end, const anTraceModel *trm, const anMat3 &trmAxis, int contentMask, cmHandle_t model, const anVec3 &modelOrigin, const anMat3 &modelAxis ) = 0;

	// Rotates a trace model and reports the first collision if any.
	void					Rotation( trace_t *results, const anVec3 &start, const anRotation &rotation, const anTraceModel *trm, const anMat3 &trmAxis, int contentMask, cmHandle_t model, const anVec3 &modelOrigin, const anMat3 &modelAxis ) = 0;

	// Returns the contents touched by the trace model or 0 if the trace model is in free space.
	int						Contents( const anVec3 &start, const anTraceModel *trm, const anMat3 &trmAxis, int contentMask, cmHandle_t model, const anVec3 &modelOrigin, const anMat3 &modelAxis ) = 0;

	// Stores all contact points of the trace model with the model, returns the number of contacts.
	int						Contacts( contactInfo_t *contacts, const int maxContacts, const anVec3 &start, const anVec6 &dir, const float depth, const anTraceModel *trm, const anMat3 &trmAxis, int contentMask, cmHandle_t model, const anVec3 &modelOrigin, const anMat3 &modelAxis ) = 0;

	bool					IsTraceModel( void ) const = 0;
	bool					IsConvex( void ) const = 0;

	bool					IsWorld( void ) const = 0;
	void					SetWorld( bool tf ) = 0;

	// Tests collision detection.
	void					DebugOutput( const anVec3 &origin ) = 0;

	// Tests soft bodies collision model, and debug.
	bool					TestSoftBodyCollision( anCollisionModel *model ) = 0;

	// Draws a model.
	void					DrawModel( cmHandle_t model, const anVec3 &modelOrigin, const anMat3 &modelAxis, const anVec3 &viewOrigin, const float radius ) = 0;

	// Prints model information, use -1 handle for accumulated model info.
	void					ModelInfo( cmHandle_t model ) = 0;

	// Lists all loaded models.
	void			ListModels( void ) = 0;

	// Writes a collision model file for the given map entity.
	bool			WriteCollisionModelForMapEntity( const anMapEntity *mapEnt, const char *filename, const bool testTraceModel = true ) = 0;

private:				// collision map data
	anString			mapName;
	ARC_TIME_T			mapFileTime;
	int					loaded;
						// for multi-check avoidance
	int					checkCount;
							// models
	int					maxModels;
	int					numModels;
	cm_model_t **		models;
						// polygons and brush for trm model
	cm_polygonRef_t *	trmPolygons[MAX_TRACEMODEL_POLYS];
	cm_brushRef_t *		trmBrushes[1];
	const anMaterial *	trmMaterial;
						// for data pruning
	int					numProcNodes;
	cm_procNode_t *		procNodes;
						// for retrieving contact points
	bool				getContacts;
	contactInfo_t *		contacts;
	int					maxContacts;
	int					numContacts;

};

extern anCollisionModelManager *	collisionModelManager;
extern anCVarSystem					cm_debugCollision;

#endif // !__COLLISIONMODELMANAGER_H__
