#ifndef __RENDERWORLD_H__
#define __RENDERWORLD_H__

/*
===============================================================================

	Render World

===============================================================================
*/

#define PROC_FILE_EXT				"proc"
#define	PROC_FILE_ID				"mapProcFile003"

// shader parms
const int MAX_TOTAL_SP	= 12;

const int SP_RED			= 0;
const int SP_GREEN			= 1;
const int SP_BLUE			= 2;
const int SP_APLHA			= 3;
const int SP_TIMESCALE		= 3;
const int SP_TIMEOFFSET		= 4;
const int SP_DIVERSITY		= 5;	// random between 0.0 and 1.0 for some effects (muzzle flashes, etc)
const int SP_MODE			= 7;	// for selecting which shader passes to enable
const int SP_EXPIRATIONTIME	= 7;	// for the monster skin-burn-away effect enable and time offset

// model parms
const int SP_MD5_SKINSCALE	= 8;	// for scaling vertex offsets on md5 models (jack skellington effect)

const int SP_MD3_FRAME		= 8;
const int SP_MD3_LASTFRAME	= 9;
const int SP_MD3_BACKLERP	= 10;

const int SP_BEAM_END_X		= 8;	// for _beam models
const int SP_BEAM_END_Y		= 9;
const int SP_BEAM_END_Z		= 10;
const int SP_BEAM_WIDTH		= 11;

const int SP_SPRITE_WIDTH	= 8;
const int SP_SPRITE_HEIGHT	= 9;

const int SP_PARTICLE_STOPTIME = 8;	// don't spawn any more particles after this time

// guis
const int MAX_RENDERENTITY_GUI = 3;

typedef bool(*deferredEntityCallback_t)( renderEntity_s *, const renderView_s * );

const int MAX_LIGHT_AREAS = 8;
const int MAX_PRELIGHTS = 5;

////const int MAX_SURFACE_BITS = 64;

//const unsigned short MIRROR_VIEW_ID			= 0xFFFF;
//const unsigned short FAST_MIRROR_VIEW_ID	= 0xFFFE;
const		   int	 WORLD_SPAWN_ID			= 0xFFFF0000;

typedef struct renderEntity_s {
	anRenderModel					*hModel;				// this can only be null if callback is set

	int								entityNum;
	int								bodyId;
	int								spawnID;//entityNum;
	int								mapId;

	// Entities that are expensive to generate, like skeletal models, can be
	// deferred until their bounds are found to be in view, in the frustum
	// of a shadowing light that is in view, or contacted by a trace / overlay test.
	// This is also used to do visual cueing on items in the view
	// The renderView may be NULL if the callback is being issued for a non-view related
	// source.
	// The callback function should clear renderEntity->callback if it doesn't
	// want to be called again next time the entity is referenced (ie, if the
	// callback has now made the entity valid until the next updateEntity)
	anBounds					bounds;					// only needs to be set for deferred models and md5s
	deferredEntityCallback_t 		callback;
	void							*callbackData;			// used for whatever the callback wants

	// player bodies and possibly player shadows should be suppressed in views from
	// that player's eyes, but will show up in mirrors and other subviews
	// security cameras could suppress their model in their subviews if we add a way
	// of specifying a view number for a remoteRenderMap view
	int								suppressSurfInViewID;
	int								suppressShadowInViewID;

	// world models for the player and weapons will not cast shadows from view weapon
	// muzzle flashes
	int								suppressShadowInLightID;

	// if non-zero, the surface and shadow (if it casts one)
	// will only show up in the specific view, ie: player weapons
	int								allowSurfInViewID;
	// if non-zero, the surface will act as a noSelfShadow in the specified view
	unsigned short					noSelfShadowInViewID;

	unsigned char					drawSpec;
	unsigned char					shadowSpec;
	// positioning
	// axis rotation vectors must be unit length for many
	// R_LocalToGlobal functions to work, so don't scale models!
	// axis vectors are [0] = forward, [1] = left, [2] = up
	anVec3							origin;
	anMat3							axis;

	// texturing
	const anMaterial 				*customShader;	// if non-0, all surfaces will use this
	const anMaterial 				*shaderRef;		// used so flares can reference the proper light shader
	const arcDeclSkin 				*customSkin;		// 0 for no remappings
	class ARCSoundEmitter			*sndRef;			// for shader sound tables, allowing effects to vary with sounds
	float							shaderParms[ MAX_ENTITY_SHADER_PARMS ];	// can be used in any way by shader or model generation

	// networking: see WriteGUIToSnapshot / ReadGUIFromSnapshot
	class anUserInterfaces			*gui[ MAX_RENDERENTITY_GUI ];

	struct renderView_s				*remoteRenderView;		// any remote camera surfaces will use this

	int								numJoints;
	arcJointMat 					*joints;					// array of joints that will modify vertices.
													// NULL if non-deformable model.  NOT freed by renderer

	float							modelDepthHack;			// squash depth range so particle effects don't clip into walls

	// options to override surface shader flags (replace with material parameters?)
	bool							noSelfShadow;			// cast shadows onto other objects,but not self
	bool							noShadow;				// no shadow at all

	bool							noDynamicInteractions;	// don't create any light / shadow interactions after
													// the level load is completed.  This is a performance hack
													// for the gigantic outdoor meshes in the monorail map, so
													// all the lights in the moving monorail don't touch the meshes

	bool							ViewDepthHack;			// squash depth range so view weapons don't poke into walls
													// this automatically implies noShadow
	bool							foliageDepthHack;
	int								forceUpdate;			// force an update (NOTE: not a bool to keep this struct a multiple of 4 bytes)
	int								timeGroup;
	int								xrayIndex;

	float							sortOffset;				// Override material sort form render entity

	float							coverage;				// used when flag overridencoverage is specified

	short							minGpuSpec;
	float							shadowLODDistance;
	int								suppressLOD;

	//int								numVisDummies;
	//anVec3							*dummies;

	int								numAreas;
	int								*areas;
} renderEntity_t;

typedef struct renderLight_s {
	anMat3					axis;				// rotation vectors, must be unit length
	anVec3					origin;

	// if non-zero, the light will not show up in the specific view,
	// which may be used if we want to have slightly different muzzle
	// flash lights for the player and other views
	int						suppressLightInPOV;

	// if non-zero, the light will only show up in the specific view
	// which can allow player gun gui lights and such to not effect everyone
	int						allowLightInPOV;

	// I am sticking the four bools together so there are no unused gaps in
	// the padded structure, which could confuse the memcmp that checks for redundant
	// updates
	bool					noShadows;			// (should we replace this with material parameters on the shader?)
	bool					noSpecular;
	bool					noDynamicShadows;
	bool					noSelfShadow;		// cast shadows onto other objects,but not self

	bool					pointLight;			// otherwise a projection light (should probably invert the sense of this, because points are way more common)
	bool					parallel;			// lightCenter gives the direction to the light at infinity
	anVec3					lightRadius;		// xyz radius for point lights
	anVec3					lightCenter;		// offset the lighting direction for shading and
												// shadows, relative to origin
	// frustum definition for projected lights, all reletive to origin
	// FIXME: we should probably have real plane equations here, and offer
	// a helper function for conversion from this format
	anPlane				target;
	anPlane				right;
	anPlane				up;
	anPlane				start;
	anPlane				end;

	// Dmap will generate an optimized shadow volume named _prelight_<lightName>
	// for the light against all the _area* models in the map.  The renderer will
	// ignore this value if the light has been moved after initial creation
	anRenderModel 			*prelightModel;

	// muzzle flash lights will not cast shadows from player and weapon world models
	int						lightId;
	int						maxVisDist;

	//unsigned int			minSpecShadowColor;
	// light polytope doesn't get pushed down the tree, instead, only add to the specified areas
	int						numAreas;
	int						areas[ MAX_LIGHT_AREAS ];

	//struct atmosLightProjection_t *atmosLightProjection;

	const anMaterial		*shader;				// NULL = either lights/defaultPointLight or lights/defaultProjectedLight
	float					shaderParms[MAX_ENTITY_SHADER_PARMS];		// can be used in any way by shader
	ARCSoundEmitter			*sndRef;		// for shader sound tables, allowing effects to vary with sounds
	//int						ApplyChanges( const renderLight_t &other );
} renderLight_t;

typedef struct renderView_s {
	// player views will set this to a non-zero integer for model suppress / allow
	// subviews (mirrors, cameras, etc) will always clear it to zero
	int						inViewID;

	// sized from 0 to SCREEN_WIDTH / SCREEN_HEIGHT (640/480), not actual resolution
	int						x, y, width, height;

	float					fov_x, fov_y;
	anVec3					viewOrg;
	anMat3					viewAxis;			// transformation matrix, view looks down the positive X axis

	bool					cramZNear;			// for cinematics, we want to set ZNear much lower
	bool					forceUpdate;		// for an update

	// time in milliseconds for shader effects and other time dependent rendering issues
	int						time;
	float					shaderParms[MAX_TOTAL_SP]; // can be used in any way by shader
	const anMaterial		*globalMaterial;		// used to override everything draw
} renderView_t;

// exitPortal_t is returned by anRenderWorld::GetPortal()
typedef struct {
	int						areas[2];		// areas connected by this portal
	const anWinding		*w;				// winding points have counter clockwise ordering seen from areas[0]
	int						blockingBits;	// PS_BLOCK_VIEW, PS_BLOCK_AIR, etc
	arcNetHandle_t			portalHandle;
} exitPortal_t;

// guiPoint_t is returned by anRenderWorld::GuiTrace()
typedef struct {
	float					x, y;			// 0.0 to 1.0 range if trace hit a gui, otherwise -1
	int						guiId;			// id of gui ( 0, 1, or 2 ) that the trace happened against
} guiPoint_t;

// modelTrace_t is for tracing vs. visual geometry
typedef struct modelTrace_s {
	float						fraction;		// fraction of trace completed
	anVec3						point;			// end point of trace in global space
	anVec3						normal;			// hit triangle normal vector in global space
	anVec2						st;					// texture coordinate
	const anMaterial			*material;		// material of hit surface
	const srfTriangles_t	*	geometry;
	anVec3						surfaceColor;
	const renderEntity_t		*entity;			// render entity that was hit
	int							jointNumber;	// md5 joint nearest to the hit triangle
	const class idRenderEntity	*def;
	const anDeclSurfaceType		*surfaceType; // for material types/classes
} modelTrace_t;

static const int NUM_PORTAL_ATTRIBUTES = 4;

typedef enum {
	PS_BLOCK_NONE	= 0,
	PS_BLOCK_VIEW	= 1,
	PS_BLOCK_LOCATION = 2,		// game map location strings often stop in hallways
	PS_BLOCK_AIR	= 4,			// windows between pressurized and unpresurized areas
	PS_BLOCK_GRAVITY = 8,
	PS_BLOCK_ALL	= ( 1<<NUM_PORTAL_ATTRIBUTES )-1
	//PS_BLOCK_ALL = PS_BLOCK_VIEW|PS_BLOCK_LOCATION|PS_BLOCK_AIR
} portalConnection_t;

class anRenderWorld {
public:
	virtual					~anRenderWorld() {};

	// The same render world can be reinitialized as often as desired
	// a NULL or empty mapName will create an empty, single area world
	virtual bool			InitFromMap( const char *mapName ) = 0;
	virtual void			LinkCullSectorsToArea( int area ) = 0;

	virtual void			Flush() = 0;

	//-------------- Entity and Light Defs -----------------

	// entityDefs and lightDefs are added to a given world to determine
	// what will be drawn for a rendered scene.  Most update work is defered
	// until it is determined that it is actually needed for a given view.
	virtual	arcNetHandle_t	AddEntityDef( const renderEntity_t *re ) = 0;
	virtual	void			UpdateEntityDef( arcNetHandle_t entityHandle, const renderEntity_t *re ) = 0;
	virtual	void			FreeEntityDef( arcNetHandle_t entityHandle ) = 0;
	virtual const renderEntity_t *GetRenderEntity( arcNetHandle_t entityHandle ) const = 0;

	virtual	arcNetHandle_t	AddLightDef( const renderLight_t *rlight ) = 0;
	virtual	void			UpdateLightDef( arcNetHandle_t lightHandle, const renderLight_t *rlight ) = 0;
	virtual	void			FreeLightDef( arcNetHandle_t lightHandle ) = 0;
	virtual const renderLight_t *GetRenderLight( arcNetHandle_t lightHandle ) const = 0;

	// Force the generation of all light / surface interactions at the start of a level
	// If this isn't called, they will all be dynamically generated
	virtual	void			GenerateAllInteractions() = 0;

	// returns true if this area model needs portal sky to draw
	virtual bool			CheckAreaForPortalSky( int areaNum ) = 0;

	virtual anRenderModel	*GetEntityHandleDynamicModel( qhandle_t entityHandle ) = 0;

	//-------------- Decals and Overlays  -----------------
	//virtual anRenderModel *CreateDecalModel() = 0;
	//virtual void 			AddToProjectedDecal( const idFixedWinding& winding, const anVec3 &projectionOrigin, const bool parallel, const anVec4& color, idRenderModel* model, int entityNum, const anMaterial** onlyMaterials = NULL, const int numOnlyMaterials = 1 ) = 0;
	//virtual void			ResetDecalModel( idRenderModel* model ) = 0;
	//virtual void			FinishDecal( idRenderModel* model ) = 0;

	// Creates decals on all world surfaces that the winding projects onto.
	// The projection origin should be infront of the winding plane.
	// The decals are projected onto world geometry between the winding plane and the projection origin.
	// The decals are depth faded from the winding plane to a certain distance infront of the
	// winding plane and the same distance from the projection origin towards the winding.
	virtual void			ProjectDecalOntoWorld( const arcFixedWinding &winding, const anVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const anMaterial *material, const int startTime ) = 0;

	// Creates decals on static models.
	virtual void			ProjectDecal( arcNetHandle_t entityHandle, const arcFixedWinding &winding, const anVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const anMaterial *material, const int startTime ) = 0;

	// Creates overlays on dynamic models.
	virtual void			ProjectOverlay( arcNetHandle_t entityHandle, const anPlane localTextureAxis[2], const anMaterial *material ) = 0;

	// Removes all decals and overlays from the given entity def.
	virtual void			RemoveDecals( arcNetHandle_t entityHandle ) = 0;

	virtual void			ClearDecals( void ) = 0;
	//-------------- Scene Rendering -----------------
	//virtual void			AddEnvBounds( anVec3 const &origin, anVec3 const &scale, const char *cubemap ) = 0;

	// some calls to material functions use the current renderview time when servicing cinematics.  this function
	// ensures that any parms accessed (such as time) are properly set.
	//virtual void			SetRenderView( const renderView_t *renderView ) = 0;
	virtual void			RenderScene( const renderView_t *renderView, int renderFlags = RF_NORMAL ) = 0;
	// rendering a scene may actually render multiple subviews for mirrors and portals, and
	// may render composite textures for gui console screens and light projections
	// It would also be acceptable to render a scene multiple times, for "rear view mirrors", etc
	virtual void			RenderScene( const renderView_t *renderView ) = 0;
	virtual void			PerformRenderScene( const renderView_t *renderView ) = 0;
	//-------------- Portal Area Information -----------------

	// returns the number of portals
	virtual int				NumPortals( void ) const = 0;

	// returns 0 if no portal contacts the bounds
	// This is used by the game to identify portals that are contained
	// inside doors, so the connection between areas can be topologically
	// terminated when the door shuts.
	virtual	arcNetHandle_t	FindPortal( const anBounds &b ) const = 0;

	// doors explicitly close off portals when shut
	// multiple bits can be set to block multiple things, ie: ( PS_VIEW | PS_LOCATION | PS_AIR )
	virtual	void			SetPortalState( arcNetHandle_t portal, int blockingBits ) = 0;
	virtual int				GetPortalState( arcNetHandle_t portal ) = 0;

	// returns true only if a chain of portals without the given connection bits set
	// exists between the two areas (a door doesn't separate them, etc)
	virtual	bool			AreasAreConnected( int areaNum1, int areaNum2, portalConnection_t connection ) = 0;

	// returns the number of portal areas in a map, so game code can build information
	// tables for the different areas
	virtual	int				NumAreas( void ) const = 0;

	// Will return -1 if the point is not in an area, otherwise
	// it will return 0 <= value < NumAreas()
	virtual int				PointInArea( const anVec3 &point ) const = 0;

	// fills the *areas array with the numbers of the areas the bounds cover
	// returns the total number of areas the bounds cover
	virtual int				BoundsInAreas( const anBounds &bounds, int *areas, int maxAreas ) const = 0;

	// Used by the sound system to do area flowing
	virtual	int				NumPortalsInArea( int areaNum ) = 0;

	// returns one portal from an area
	virtual exitPortal_t	GetPortal( int areaNum, int portalNum ) = 0;

	//-------------- Tracing  -----------------

	// Checks a ray trace against any gui surfaces in an entity, returning the
	// fraction location of the trace on the gui surface, or -1,-1 if no hit.
	// This doesn't do any occlusion testing, simply ignoring non-gui surfaces.
	// start / end are in global world coordinates.
	virtual guiPoint_t		GuiTrace( arcNetHandle_t entityHandle, const anVec3 start, const anVec3 end ) const = 0;

	// Traces vs the render model, possibly instantiating a dynamic version, and returns true if something was hit
	virtual bool			ModelTrace( modelTrace_t &trace, arcNetHandle_t entityHandle, const anVec3 &start, const anVec3 &end, const float radius ) const = 0;

	// Traces vs the whole rendered world. FIXME: we need some kind of material flags.
	virtual bool			Trace( modelTrace_t &trace, const anVec3 &start, const anVec3 &end, const float radius, bool skipDynamic = true, bool skipPlayer = false ) const = 0;

	// Traces vs the world model bsp tree.
	virtual bool			FastWorldTrace( modelTrace_t &trace, const anVec3 &start, const anVec3 &end ) const = 0;

	// this is used to regenerate all interactions ( which is currently only done during influences ), there may be a less
	// expensive way to do it
	virtual void			RegenerateWorld() = 0;

	//-------------- Debug Visualization  -----------------

	// Line drawing for debug visualization
	virtual void			DebugClearLines( int time ) = 0;		// a time of 0 will clear all lines and text
	virtual void			DebugLine( const anVec4 &color, const anVec3 &start, const anVec3 &end, const int lifeTime = 0, const bool depthTest = false ) = 0;
	virtual void			DebugArrow( const anVec4 &color, const anVec3 &start, const anVec3 &end, int size, const int lifeTime = 0 ) = 0;
	virtual void			DebugWinding( const anVec4 &color, const anWinding &w, const anVec3 &origin, const anMat3 &axis, const int lifeTime = 0, const bool depthTest = false ) = 0;
	virtual void			DebugCircle( const anVec4 &color, const anVec3 &origin, const anVec3 &dir, const float radius, const int numSteps, const int lifeTime = 0, const bool depthTest = false ) = 0;
	virtual void			DebugSphere( const anVec4 &color, const anSphere &sphere, const int lifeTime = 0, bool depthTest = false ) = 0;
	virtual void			DebugBounds( const anVec4 &color, const anBounds &bounds, const anVec3 &org = vec3_origin, const int lifeTime = 0 ) = 0;
	virtual void			DebugBoundsDepthTest( const anVec4 &color, const anBounds &bounds, const anVec3 &org = vec3_origin, const int lifetime = 0, bool depthTest = false ) = 0;

	virtual void			DebugBox( const anVec4 &color, const anBox &box, const int lifeTime = 0 ) = 0;
	virtual void			DebugFrustum( const anVec4 &color, const anFrustum &frustum, const bool showFromOrigin = false, const int lifeTime = 0 ) = 0;
	virtual void			DebugCone( const anVec4 &color, const anVec3 &apex, const anVec3 &dir, float radius1, float radius2, const int lifeTime = 0 ) = 0;
	virtual void			DebugAxis( const anVec3 &origin, const anMat3 &axis ) = 0;

	// Polygon drawing for debug visualization.
	virtual void			DebugClearPolygons( int time ) = 0;		// a time of 0 will clear all polygons
	virtual void			DebugPolygon( const anVec4 &color, const anWinding &winding, const int lifeTime = 0, const bool depthTest = false ) = 0;

 	virtual void			DebugFOV( const anVec4 &color, const anVec3 &origin, const anVec3 &dir, float farDot, float farDist, float nearDot = 1.0f, float nearDist = 0.0f, float alpha = 0.3f, int lifetime = 0 ) = 0;

	virtual size_t			MemorySummary( const anCommandArgs &args ) = 0;
	virtual void			ShowDebugLines( void ) = 0;
	virtual void			ShowDebugPolygons( void ) = 0;
	virtual void			ShowDebugText( void ) = 0;

	// Text drawing for debug visualization.
	virtual void			DrawText( const char *text, const anVec3 &origin, float scale, const anVec4 &color, const anMat3 &viewAxis, const int align = 1, const int lifeTime = 0, bool depthTest = false ) = 0;

	//virtual const arcDeclAtmosphere*	GetAtmosphere() const = 0;
	//virtual void			SetAtmosphere( const sdDeclAtmosphere* atmosphere ) = 0;
	//virtual void			SetupMatrices( const renderView_t* renderView, float* projectionMatrix, float* modelViewMatrix, const bool allowJitter ) = 0;
	//virtual struct atmosLightProjection_t *FindAtmosLightProjection( int lightID ) = 0;
};

#endif