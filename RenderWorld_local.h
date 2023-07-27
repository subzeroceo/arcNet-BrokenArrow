#ifndef __RENDERWORLDLOCAL_H__
#define __RENDERWORLDLOCAL_H__

// assume any lightDef or entityDef index above this is an internal error
const int LUDICROUS_INDEX	= 10000;

typedef struct portal_s {
	int							intoArea;		// area this portal leads to
	anWinding					*w;				// winding points have counter clockwise ordering seen this area
	anPlane					plane;			// view must be on the positive side of the plane to cross
	struct portal_s				*next;			// next portal of the area
	struct doublePortal_s		*doublePortal;
} portal_t;

typedef struct doublePortal_s {
	struct portal_s	*			portals[2];
	int							blockingBits;	// PS_BLOCK_VIEW, PS_BLOCK_AIR, etc, set by doors that shut them off

	// A portal will be considered closed if it is past the
	// fog-out point in a fog volume.  We only support a single
	// fog volume over each portal.
	anRenderLightsLocal		*fogLight;
	struct doublePortal_s		*nextFoggedPortal;
} doublePortal_t;

typedef struct portalArea_s {
	int							areaNum;
	int							connectedAreaNum[NUM_PORTAL_ATTRIBUTES];	// if two areas have matching connectedAreaNum, they are
									// not separated by a portal with the apropriate PS_BLOCK_* blockingBits
	int							viewCount;		// set by R_FindViewLightsAndEntities
	portal_t					*portals;		// never changes after load
	areaReference_t				entityRefs;		// head/tail of doubly linked list, may change
	areaReference_t				lightReferences;		// head/tail of doubly linked list, may change
} portalArea_t;

static const int				CHILDREN_HAVE_MULTIPLE_AREAS = -2;
static const int				CHILDREN_AREANUM_SOLID = -1;

typedef struct {
	anPlane					plane;
	int							children[2];		// negative numbers are (-1 - areaNumber), 0 = solid
	int							commonChildrenArea;	// if all children are either solid or a single area,
										// this is the area number, else CHILDREN_HAVE_MULTIPLE_AREAS
} areaNode_t;

class anRenderWorldLocal : public anRenderWorld {
public:
							anRenderWorldLocal();
	virtual					~anRenderWorldLocal();

	virtual	arcNetHandle_t	AddEntityDef( const renderEntity_t *re );
	virtual	void			UpdateEntityDef( arcNetHandle_t entityHandle, const renderEntity_t *re );
	virtual	void			FreeEntityDef( arcNetHandle_t entityHandle );
	virtual const renderEntity_t *GetRenderEntity( arcNetHandle_t entityHandle ) const;

	virtual	arcNetHandle_t	AddLightDef( const renderLight_t *rlight );
	virtual	void			UpdateLightDef( arcNetHandle_t lightHandle, const renderLight_t *rlight );
	virtual	void			FreeLightDef( arcNetHandle_t lightHandle );
	virtual const renderLight_t *GetRenderLight( arcNetHandle_t lightHandle ) const;

	virtual bool			CheckAreaForPortalSky( int areaNum );

	virtual	void			GenerateAllInteractions();
	virtual void			RegenerateWorld();

	virtual void			ProjectDecalOntoWorld( const arcFixedWinding &winding, const anVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const anMaterial *material, const int startTime );
	virtual void			ProjectDecal( arcNetHandle_t entityHandle, const arcFixedWinding &winding, const anVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const anMaterial *material, const int startTime );
	virtual void			ProjectOverlay( arcNetHandle_t entityHandle, const anPlane localTextureAxis[2], const anMaterial *material );
	virtual void			RemoveDecals( arcNetHandle_t entityHandle );

	virtual void			SetRenderView( const renderView_t *renderView );
	virtual	void			RenderScene( const renderView_t *renderView );

	virtual	int				NumAreas( void ) const;
	virtual int				PointInArea( const anVec3 &point ) const;
	virtual int				BoundsInAreas( const anBounds &bounds, int *areas, int maxAreas ) const;
	virtual	int				NumPortalsInArea( int areaNum );
	virtual exitPortal_t	GetPortal( int areaNum, int portalNum );

	virtual	guiPoint_t		GuiTrace( arcNetHandle_t entityHandle, const anVec3 start, const anVec3 end ) const;
	virtual bool			ModelTrace( modelTrace_t &trace, arcNetHandle_t entityHandle, const anVec3 &start, const anVec3 &end, const float radius ) const;
	virtual bool			Trace( modelTrace_t &trace, const anVec3 &start, const anVec3 &end, const float radius, bool skipDynamic = true, bool skipPlayer = false ) const;
	virtual bool			FastWorldTrace( modelTrace_t &trace, const anVec3 &start, const anVec3 &end ) const;

	virtual void			DebugClearLines( int time );
	virtual void			DebugLine( const anVec4 &color, const anVec3 &start, const anVec3 &end, const int lifetime = 0, const bool depthTest = false );
	virtual void			DebugArrow( const anVec4 &color, const anVec3 &start, const anVec3 &end, int size, const int lifetime = 0 );
	virtual void			DebugWinding( const anVec4 &color, const anWinding &w, const anVec3 &origin, const anMat3 &axis, const int lifetime = 0, const bool depthTest = false );
	virtual void			DebugCircle( const anVec4 &color, const anVec3 &origin, const anVec3 &dir, const float radius, const int numSteps, const int lifetime = 0, const bool depthTest = false );
	virtual void			DebugSphere( const anVec4 &color, const anSphere &sphere, const int lifetime = 0, bool depthTest = false );
	virtual void			DebugBounds( const anVec4 &color, const anBounds &bounds, const anVec3 &org = vec3_origin, const int lifetime = 0 );
	virtual void			DebugBox( const anVec4 &color, const anBox &box, const int lifetime = 0 );
	virtual void			DebugFrustum( const anVec4 &color, const anFrustum &frustum, const bool showFromOrigin = false, const int lifetime = 0 );
	virtual void			DebugCone( const anVec4 &color, const anVec3 &apex, const anVec3 &dir, float radius1, float radius2, const int lifetime = 0 );
	virtual void			DebugScreenRect( const anVec4 &color, const anScreenRect &rect, const viewDef_t *viewDef, const int lifetime = 0 );
	virtual void			DebugAxis( const anVec3 &origin, const anMat3 &axis );

	virtual void			DebugClearPolygons( int time );
	virtual void			DebugPolygon( const anVec4 &color, const anWinding &winding, const int lifeTime = 0, const bool depthTest = false );

	virtual void			DrawText( const char *text, const anVec3 &origin, float scale, const anVec4 &color, const anMat3 &viewAxis, const int align = 1, const int lifetime = 0, bool depthTest = false );

	//-----------------------

	anString				mapName;				// ie: maps/tim_dm2.proc, written to demoFile
	ARC_TIME_T				mapTimeStamp;			// for fast reloads of the same level

	areaNode_t 				*areaNodes;
	int						numAreaNodes;

	portalArea_t			*portalAreas;
	int						numPortalAreas;
	int						connectedAreaNum;		// incremented every time a door portal state changes

	anScreenRect			*areaScreenRect;

	doublePortal_t 			*doublePortals;
	int						numInterAreaPortals;

	anList<anRenderModel*> localModels;

	anList<anRenderEntityLocal*> entityDefs;
	anList<anRenderLightsLocal*> lightDefs;

	arcBlockAlloc<areaReference_t, 1024> areaReferenceAllocator;
	arcBlockAlloc<an Interaction, 256> interactionAllocator;
	arcBlockAlloc<areaNumRef_t, 1024> areaNumRefAllocator;

	// all light / entity interactions are referenced here for fast lookup without
	// having to crawl the doubly linked lists.  EnntityDefs are sequential for better
	// cache access, because the table is accessed by light in anRenderWorldLocal::CreateLightDefInteractions()
	// Growing this table is time consuming, so we add a pad value to the number
	// of entityDefs and lightDefs
	an Interaction **		interactionTable;
	int						interactionTableWidth;		// entityDefs
	int						interactionTableHeight;		// lightDefs


	bool					generateAllInteractionsCalled;

	//-----------------------
	// RenderWorld_load.cpp

	anRenderModel 			*ParseModel( anLexer *src );
	anRenderModel			*ParseShadowModel( anLexer *src );
	void					SetupAreaRefs();
	void					ParseInterAreaPortals( anLexer *src );
	void					ParseNodes( anLexer *src );
	int						CommonChildrenArea_r( areaNode_t *node );
	void					FreeWorld();
	void					ClearWorld();
	void					FreeDefs();
	void					TouchWorldModels( void );
	void					AddWorldModelEntities();
	void					ClearPortalStates();
	virtual	bool			InitFromMap( const char *mapName );

	//--------------------------
	// RenderWorld_portals.cpp

	anScreenRect			ScreenRectFromWinding( const anWinding *w, viewEntity_t *space );
	bool					PortalIsFoggedOut( const portal_t *p );
	void					FloodViewThroughArea_r( const anVec3 origin, int areaNum, const struct portalStack_s *ps );
	void					FlowViewThroughPortals( const anVec3 origin, int numPlanes, const anPlane *planes );
	void					FloodLightThroughArea_r( anRenderLightsLocal *light, int areaNum, const struct portalStack_s *ps );
	void					FlowLightThroughPortals( anRenderLightsLocal *light );
	areaNumRef_t *			FloodFrustumAreas_r( const anFrustum &frustum, const int areaNum, const anBounds &bounds, areaNumRef_t *areas );
	areaNumRef_t *			FloodFrustumAreas( const anFrustum &frustum, areaNumRef_t *areas );
	bool					CullEntityByPortals( const anRenderEntityLocal *entity, const struct portalStack_s *ps );
	void					AddAreaEntityRefs( int areaNum, const struct portalStack_s *ps );
	bool					CullLightByPortals( const anRenderLightsLocal *light, const struct portalStack_s *ps );
	void					AddAreaLightRefs( int areaNum, const struct portalStack_s *ps );
	void					AddAreaRefs( int areaNum, const struct portalStack_s *ps );
	void					BuildConnectedAreas_r( int areaNum );
	void					BuildConnectedAreas( void );
	void					FindViewLightsAndEntities( void );

	int						NumPortals( void ) const;
	arcNetHandle_t			FindPortal( const anBounds &b ) const;
	void					SetPortalState( arcNetHandle_t portal, int blockingBits );
	int						GetPortalState( arcNetHandle_t portal );
	bool					AreasAreConnected( int areaNum1, int areaNum2, portalConnection_t connection );
	void					FloodConnectedAreas( portalArea_t *area, int portalAttributeIndex );
	anScreenRect &			GetAreaScreenRect( int areaNum ) const { return areaScreenRect[areaNum]; }
	void					ShowPortals();

	void					WriteLoadMap();
	void					WriteRenderView( const renderView_t *renderView );
	void					WriteVisibleDefs( const viewDef_t *viewDef );
	void					WriteFreeLight( arcNetHandle_t handle );
	void					WriteFreeEntity( arcNetHandle_t handle );
	void					WriteRenderLight( arcNetHandle_t handle, const renderLight_t *light );
	void					WriteRenderEntity( arcNetHandle_t handle, const renderEntity_t *ent );
	void					ReadRenderEntity();
	void					ReadRenderLight();

	//--------------------------
	// RenderWorld.cpp

	void					ResizeInteractionTable();

	void					AddEntityRefToArea( anRenderEntityLocal *def, portalArea_t *area );
	void					AddLightRefToArea( anRenderLightsLocal *light, portalArea_t *area );

	void					RecurseProcBSP_r( modelTrace_t *results, int parentNodeNum, int nodeNum, float p1f, float p2f, const anVec3 &p1, const anVec3 &p2 ) const;

	void					BoundsInAreas_r( int nodeNum, const anBounds &bounds, int *areas, int *numAreas, int maxAreas ) const;

	float					DrawTextLength( const char *text, float scale, int len = 0 );

	void					FreeInteractions();

	void					PushVolumeIntoTree_r( anRenderEntityLocal *def, anRenderLightsLocal *light, const anSphere *sphere, int numPoints, const anVec3 (*points), int nodeNum );

	void					PushVolumeIntoTree( anRenderEntityLocal *def, anRenderLightsLocal *light, int numPoints, const anVec3 (*points) );

	//-------------------------------
	// tr_light.c
	void					CreateLightDefInteractions( anRenderLightsLocal *ldef );
};

#endif
