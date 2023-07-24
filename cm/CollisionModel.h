#ifndef __COLLISIONMODELMANAGER_H__
#define __COLLISIONMODELMANAGER_H__

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

// contact type
typedef enum {
	CONTACT_NONE,							// no contact
	CONTACT_EDGE,							// trace model edge hits model edge
	CONTACT_MODELVERTEX,					// model vertex hits trace model polygon
	CONTACT_TRMVERTEX						// trace model vertex hits model polygon
} contactType_t;

// contact info
typedef struct {
	contactType_t			type;			// contact type
	arcVec3					point;			// point of contact
	arcVec3					normal;			// contact plane normal
	float					dist;			// contact plane distance
	float					separation;		// contact feature separation at initial position
	int						contents;		// contents at other side of surface
	const arcMaterial *		material;		// surface material
	const arcDeclSurfType *	surfaceType;	// surface shader/material type
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
	arcVec3					endpos;			// final position of trace model
	arcMat3					endAxis;		// final axis of trace model
	contactInfo_t			c;				// contact information, only valid if fraction < 1.0
} trace_t;

typedef int cmHandle_t;

#define CM_CLIP_EPSILON		0.25f			// always stay this distance away from any model
#define CM_BOX_EPSILON		1.0f			// should always be larger than clip epsilon
#define CM_MAX_TRACE_DIST	4096.0f			// maximum distance a trace model may be traced, point traces are unlimited

// collision model
class idCollisionModel {
public:
	virtual						~idCollisionModel() { }
								// Returns the name of the model.
	virtual const char *		GetName( void ) const = 0;
								// Gets the bounds of the model.
	virtual const arcBounds&		GetBounds( void ) const = 0;
								// Gets the bounds of the model, excluding/including surfaces of the appropriate surface type
	virtual void				GetBounds( arcBounds& bounds, int surfaceMask, bool inclusive ) const = 0;
								// Gets all contents flags of brushes and polygons of the model ored together.
	virtual int					GetContents( void ) const = 0;
								// Gets a vertex of the model.
	virtual const arcVec3&		GetVertex( int vertexNum ) const = 0;
								// Gets an edge of the model.
	virtual void				GetEdge( int edgeNum, arcVec3& start, arcVec3& end ) const = 0;
								// Gets a polygon of the model.
	virtual void				GetPolygon( int polygonNum, idFixedWinding &winding ) const = 0;
								// Draws surfaces which don't/do have the surface mask
	virtual void				Draw( int surfaceMask, bool inclusive ) const = 0;

	virtual int					GetNumBrushPlanes( void ) const = 0;
	virtual const arcPlane&		GetBrushPlane( int planeNum ) const = 0;

	virtual const arcMaterial*	GetPolygonMaterial( int polygonNum ) const = 0;
	virtual const arcPlane&		GetPolygonPlane( int polygonNum ) const = 0;
	virtual int					GetNumPolygons( void ) const = 0;

	virtual bool				IsTraceModel( void ) const = 0;
	virtual bool				IsConvex( void ) const = 0;

	virtual bool				IsWorld( void ) const = 0;
	virtual void				SetWorld( bool tf ) = 0;
};

class arcCollisionModelManager {
public:
	virtual					~arcCollisionModelManager( void ) {}

	virtual void				Init( void ) = 0;
	virtual void				Shutdown( void ) = 0;

	virtual void				AllocThread( void ) = 0;
	virtual void				FreeThread( void ) = 0;
	virtual int					GetThreadId( void ) = 0;
	virtual int					GetThreadCount( void ) = 0;

	// Loads collision models from a map file.
	virtual void			LoadMap( const idMapFile *mapFile ) = 0;
	// Frees all the collision models.
	virtual void			FreeMap( void ) = 0;

	// Gets the clip handle for a model.
	virtual cmHandle_t		LoadModel( const char *modelName, const bool precache ) = 0;
							// Precaches a collision model.
	virtual void			PreCacheModel( const char *mapName, const char *modelName ) = 0;
							// Free the given model.
	virtual void			FreeModel( idCollisionModel *model ) = 0;
							// Purge all unused models.
	virtual void			PurgeModels( void ) = 0;

	// Sets up a trace model for collision with other trace models.
	virtual cmHandle_t		SetupTrmModel( const arcTraceModel &trm, const arcMaterial *material ) = 0;
	// Creates a trace model from a collision model, returns true if succesfull.
	virtual bool			TrmFromModel( const char *modelName, arcTraceModel &trm ) = 0;

							// Sets up a trace model for collision with other trace models.
	virtual idCollisionModel *ModelFromTrm( const char *mapName, const char *modelName, const arcTraceModel &trm, const arcMaterial *material ) = 0;
							// Creates a trace model from a collision model, returns true if succesfull.
	virtual bool			TrmFromModel( const char *mapName, const char *modelName, arcTraceModel &trm ) = 0;
							// Creates a trace model for each primitive of the collision model, returns the number of trace models.
	virtual int				CompoundTrmFromModel( const char *mapName, const char *modelName, arcTraceModel *trms, int maxTrms ) = 0;
	// Gets the name of a model.
	virtual const char *	GetModelName( cmHandle_t model ) const = 0;
	// Gets the bounds of a model.
	virtual bool			GetModelBounds( cmHandle_t model, arcBounds &bounds ) const = 0;
	// Gets all contents flags of brushes and polygons of a model ored together.
	virtual bool			GetModelContents( cmHandle_t model, int &contents ) const = 0;
	// Gets a vertex of a model.
	virtual bool			GetModelVertex( cmHandle_t model, int vertexNum, arcVec3 &vertex ) const = 0;
	// Gets an edge of a model.
	virtual bool			GetModelEdge( cmHandle_t model, int edgeNum, arcVec3 &start, arcVec3 &end ) const = 0;
	// Gets a polygon of a model.
	virtual bool			GetModelPolygon( cmHandle_t model, int polygonNum, arcFixedWinding &winding ) const = 0;

	virtual const arcMaterial*	GetPolygonMaterial( int polygonNum ) const = 0;
	virtual const arcPlane&		GetPolygonPlane( int polygonNum ) const = 0;
	virtual int					GetNumPolygons( void ) const = 0;

	virtual int				GetNumBrushPlanes( void ) const = 0;
	virtual const arcPlane	&GetBrushPlane( int planeNum ) const = 0;

	// Translates a trace model and reports the first collision if any.
	virtual void			Translation( trace_t *results, const arcVec3 &start, const arcVec3 &end,
								const arcTraceModel *trm, const arcMat3 &trmAxis, int contentMask,
								cmHandle_t model, const arcVec3 &modelOrigin, const arcMat3 &modelAxis ) = 0;
	// Rotates a trace model and reports the first collision if any.
	virtual void			Rotation( trace_t *results, const arcVec3 &start, const arcRotation &rotation,
								const arcTraceModel *trm, const arcMat3 &trmAxis, int contentMask,
								cmHandle_t model, const arcVec3 &modelOrigin, const arcMat3 &modelAxis ) = 0;
	// Returns the contents touched by the trace model or 0 if the trace model is in free space.
	virtual int				Contents( const arcVec3 &start,
								const arcTraceModel *trm, const arcMat3 &trmAxis, int contentMask,
								cmHandle_t model, const arcVec3 &modelOrigin, const arcMat3 &modelAxis ) = 0;
	// Stores all contact points of the trace model with the model, returns the number of contacts.
	virtual int				Contacts( contactInfo_t *contacts, const int maxContacts, const arcVec3 &start, const arcVec6 &dir, const float depth,
								const arcTraceModel *trm, const arcMat3 &trmAxis, int contentMask,
								cmHandle_t model, const arcVec3 &modelOrigin, const arcMat3 &modelAxis ) = 0;

	virtual bool				IsTraceModel( void ) const = 0;
	virtual bool				IsConvex( void ) const = 0;

	virtual bool				IsWorld( void ) const = 0;
	virtual void				SetWorld( bool tf ) = 0;
	// Tests collision detection.
	virtual void			DebugOutput( const arcVec3 &origin ) = 0;
	// Draws a model.
	virtual void			DrawModel( cmHandle_t model, const arcVec3 &modelOrigin, const arcMat3 &modelAxis,
												const arcVec3 &viewOrigin, const float radius ) = 0;
	// Prints model information, use -1 handle for accumulated model info.
	virtual void			ModelInfo( cmHandle_t model ) = 0;
	// Lists all loaded models.
	virtual void			ListModels( void ) = 0;
	// Writes a collision model file for the given map entity.
	virtual bool			WriteCollisionModelForMapEntity( const idMapEntity *mapEnt, const char *filename, const bool testTraceModel = true ) = 0;
};

extern arcCollisionModelManager *		collisionModelManager;

#endif // !__COLLISIONMODELMANAGER_H__
