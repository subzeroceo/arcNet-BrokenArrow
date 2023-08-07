#ifndef __MODELOVERLAY_H__
#define __MODELOVERLAY_H__

/*
===============================================================================

	Render model overlay for adding decals on top of dynamic models.

===============================================================================
*/

const int MAX_OVERLAY_SURFACES	= 16;

typedef struct overlayVertex_s {
	int							vertexNum;
	float						st[2];
} overlayVertex_t;

typedef struct overlaySurface_s {
	int							surfaceNum;
	int							surfaceId;
	int							numIndexes;
	qglIndex_t *				indexes;
	int							numVerts;
	overlayVertex_t *			verts;
} overlaySurface_t;

typedef struct overlayMaterial_s {
	const anMaterial *			material;
	anList<overlaySurface_t *>	surfaces;
} overlayMaterial_t;


class anRenderModelOverlay {
public:
								anRenderModelOverlay();
								~anRenderModelOverlay();

	static anRenderModelOverlay *Alloc( void );
	static void					Free( anRenderModelOverlay *overlay );

	// Projects an overlay onto deformable geometry and can be added to
	// a render entity to allow decals on top of dynamic models.
	// This does not generate tangent vectors, so it can't be used with
	// light interaction shaders. Materials for overlays should always
	// be clamped, because the projected texcoords can run well off the
	// texture since no new clip vertexes are generated.
	void						CreateOverlay( const anRenderModel *model, const anPlane localTextureAxis[2], const anMaterial *material );

	// Creates new model surfaces for baseModel, which should be a static instantiation of a dynamic model.
	void						AddOverlaySurfacesToModel( anRenderModel *baseModel );

	// Removes overlay surfaces from the model.
	static void					RemoveOverlaySurfacesFromModel( anRenderModel *baseModel );

	void						ReadFromDemoFile( class anDemoFile *f );
	void						WriteToDemoFile( class anDemoFile *f ) const;

private:
	anList<overlayMaterial_t *>	materials;

	void						FreeSurface( overlaySurface_t *surface );
};

/*
===============================================================================

	Decals are lightweight primitives for bullet / blood marks.
	Decals with common materials will be merged together, but additional
	decals will be allocated as needed. The material should not be
	one that receives lighting, because no interactions are generated
	for these lightweight surfaces.

	FIXME:	Decals on models in portalled off areas do not get freed
			until the area becomes visible again.

===============================================================================
*/

const int NUM_DECAL_BOUNDING_PLANES = 6;

typedef struct decalProjectionInfo_s {
	anVec3						projectionOrigin;
	anBounds					projectionBounds;
	anPlane						boundingPlanes[6];
	anPlane						fadePlanes[2];
	anPlane						textureAxis[2];
	const anMaterial *			material;
	bool						parallel;
	float						fadeDepth;
	int							startTime;
	bool						force;
} decalProjectionInfo_t;


class anRenderModelDecal {
public:
								anRenderModelDecal( void );
								~anRenderModelDecal( void );

	static anRenderModelDecal * Alloc( void );
	static void					Free( anRenderModelDecal *decal );

								// Creates decal projection info.
	static bool					CreateProjectionInfo( decalProjectionInfo_t &info, const anFixedWinding &winding, const anVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const anMaterial *material, const int startTime );

								// Transform the projection info from global space to local.
	static void					GlobalProjectionInfoToLocal( decalProjectionInfo_t &localInfo, const decalProjectionInfo_t &info, const anVec3 &origin, const anMat3 &axis );

								// Creates a deal on the given model.
	void						CreateDecal( const anRenderModel *model, const decalProjectionInfo_t &localInfo );

								// Remove decals that are completely faded away.
	static anRenderModelDecal * RemoveFadedDecals( anRenderModelDecal *decals, int time );

								// Updates the vertex colors, removing any faded indexes,
								// then copy the verts to temporary vertex cache and adds a drawSurf.
	void						AddDecalDrawSurf( struct viewEntity_s *space );

								// Returns the next decal in the chain.
	anRenderModelDecal *		Next( void ) const { return nextDecal; }

	void						ReadFromDemoFile( class anDemoFile *f );
	void						WriteToDemoFile( class anDemoFile *f ) const;

private:
	static const int			MAX_DECAL_VERTS = 40;
	static const int			MAX_DECAL_INDEXES = 60;

	const anMaterial *			material;
	srfTriangles_t				tri;
	anDrawVertex				verts[MAX_DECAL_VERTS];
	float						vertDepthFade[MAX_DECAL_VERTS];
	qglIndex_t					indexes[MAX_DECAL_INDEXES];
	int							indexStartTime[MAX_DECAL_INDEXES];
	anRenderModelDecal *		nextDecal;

								// Adds the winding triangles to the appropriate decal in the
								// chain, creating a new one if necessary.
	void						AddWinding( const anWinding &w, const anMaterial *decalMaterial, const anPlane fadePlanes[2], float fadeDepth, int startTime );

								// Adds depth faded triangles for the winding to the appropriate
								// decal in the chain, creating a new one if necessary.
								// The part of the winding at the front side of both fade planes is not faded.
								// The parts at the back sides of the fade planes are faded with the given depth.
	void						AddDepthFadedWinding( const anWinding &w, const anMaterial *decalMaterial, const anPlane fadePlanes[2], float fadeDepth, int startTime );
};

#endif