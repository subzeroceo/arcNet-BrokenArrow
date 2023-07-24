#ifndef __INTERACTION_H__
#define __INTERACTION_H__

/*
===============================================================================

	Interaction between entityDef surfaces and a lightDef.

	Interactions with no lightTris and no shadowTris are still
	valid, because they show that a given entityDef / lightDef
	do not interact, even though they share one or more areas.

===============================================================================
*/

#define LIGHT_TRIS_DEFERRED			((surfTriangles_t *)-1 )
#define LIGHT_CULL_ALL_FRONT		(( byte * )-1 )
#define	LIGHT_CLIP_EPSILON			0.1f


typedef struct {
	// For each triangle a byte set to 1 if facing the light origin.
	byte *					facing;

	// For each vertex a byte with the bits [0-5] set if the
	// vertex is at the back side of the corresponding clip plane.
	// If the 'cullBits' pointer equals LIGHT_CULL_ALL_FRONT all
	// vertices are at the front of all the clip planes.
	byte *					cullBits;

	// Clip planes in surface space used to calculate the cull bits.
	arcPlane					localClipPlanes[6];
} srfCullInfo_t;


typedef struct {
	// if lightTris == LIGHT_TRIS_DEFERRED, then the calculation of the
	// lightTris has been deferred, and must be done if ambientTris is visible
	surfTriangles_t *		lightTris;

	// shadow volume triangle surface
	surfTriangles_t *		shadowTris;

	// so we can check ambientViewCount before adding lightTris, and get
	// at the shared vertex and possibly shadowVertex caches
	surfTriangles_t *		ambientTris;

	const arcMaterial *		shader;

	int						expCulled;			// only for the experimental shadow buffer renderer

	srfCullInfo_t			cullInfo;
} surfaceInteraction_t;


typedef struct areaNumRef_s {
	struct areaNumRef_s *	next;
	int						areaNum;
} areaNumRef_t;


class ARCRenderEntityLocal;
class ARCRenderLightsLocal;

class ARCInteraction {
public:
	// this may be 0 if the light and entity do not actually intersect
	// -1 = an untested interaction
	int						numSurfaces;

	// if there is a whole-entity optimized shadow hull, it will
	// be present as a surfaceInteraction_t with a NULL ambientTris, but
	// possibly having a shader to specify the shadow sorting order
	surfaceInteraction_t *	surfaces;

	// get space from here, if NULL, it is a pre-generated shadow volume from dmap
	ARCRenderEntityLocal *	entityDef;
	ARCRenderLightsLocal *	lightDef;

	ARCInteraction *			lightNext;				// for lightDef chains
	ARCInteraction *			lightPrev;
	ARCInteraction *			entityNext;				// for entityDef chains
	ARCInteraction *			entityPrev;

public:
							ARCInteraction( void );

	// because these are generated and freed each game tic for active elements all
	// over the world, we use a custom pool allocater to avoid memory allocation overhead
	// and fragmentation
	static ARCInteraction *	AllocAndLink( ARCRenderEntityLocal *edef, ARCRenderLightsLocal *ldef );

	// unlinks from the entity and light, frees all surfaceInteractions,
	// and puts it back on the free list
	void					UnlinkAndFree( void );

	// free the interaction surfaces
	void					FreeSurfaces( void );

	// makes the interaction empty for when the light and entity do not actually intersect
	// all empty interactions are linked at the end of the light's and entity's interaction list
	void					MakeEmpty( void );

	// returns true if the interaction is empty
	bool					IsEmpty( void ) const { return ( numSurfaces == 0 ); }

	// returns true if the interaction is not yet completely created
	bool					IsDeferred( void ) const { return ( numSurfaces == -1 ); }

	// returns true if the interaction has shadows
	bool					HasShadows( void ) const;

	// counts up the memory used by all the surfaceInteractions, which
	// will be used to determine when we need to start purging old interactions
	int						MemoryUsed( void );

	// makes sure all necessary light surfaces and shadow surfaces are created, and
	// calls R_LinkLightSurf() for each one
	void					AddActiveInteraction( void );

private:
	enum {
		FRUSTUM_UNINITIALIZED,
		FRUSTUM_INVALID,
		FRUSTUM_VALID,
		FRUSTUM_VALIDAREAS,
	}						frustumState;
	ARCFrustum				frustum;				// frustum which contains the interaction
	areaNumRef_t *			frustumAreas;			// numbers of the areas the frustum touches

	int						dynamicModelFrameCount;	// so we can tell if a callback model animated

private:
	// actually create the interaction
	void					CreateInteraction( const ARCRenderModel *model );

	// unlink from entity and light lists
	void					Unlink( void );

	// try to determine if the entire interaction, including shadows, is guaranteed
	// to be outside the view frustum
	bool					CullInteractionByViewFrustum( const ARCFrustum &viewFrustum );

	// determine the minimum scissor rect that will include the interaction shadows
	// projected to the bounds of the light
	ARCScreenRect			CalcInteractionScissorRectangle( const ARCFrustum &viewFrustum );
};


void R_CalcInteractionFacing( const ARCRenderEntityLocal *ent, const surfTriangles_t *tri, const ARCRenderLightsLocal *light, srfCullInfo_t &cullInfo );
void R_CalcInteractionCullBits( const ARCRenderEntityLocal *ent, const surfTriangles_t *tri, const ARCRenderLightsLocal *light, srfCullInfo_t &cullInfo );
void R_FreeInteractionCullInfo( srfCullInfo_t &cullInfo );

void R_ShowInteractionMemory_f( const arcCommandArgs &args );

#endif /* !__INTERACTION_H__ */
