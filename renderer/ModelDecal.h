#ifndef __MODELDECAL_H__
#define __MODELDECAL_H__

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
	anBounds				projectionBounds;
	anPlane					boundingPlanes[6];
	anPlane					fadePlanes[2];
	anPlane					textureAxis[2];
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

	//void						ReadFromDemoFile( class anDemoFile *f );
	//void						WriteToDemoFile( class anDemoFile *f ) const;

private:
	static const int			MAX_DECAL_VERTS = 40;
	static const int			MAX_DECAL_INDEXES = 60;

	const anMaterial *			material;
	srfTriangles_t					tri;
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