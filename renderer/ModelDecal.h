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
	arcVec3						projectionOrigin;
	arcBounds				projectionBounds;
	arcPlane					boundingPlanes[6];
	arcPlane					fadePlanes[2];
	arcPlane					textureAxis[2];
	const arcMaterial *			material;
	bool						parallel;
	float						fadeDepth;
	int							startTime;
	bool						force;
} decalProjectionInfo_t;


class ARCRenderModelDecal {
public:
								ARCRenderModelDecal( void );
								~ARCRenderModelDecal( void );

	static ARCRenderModelDecal * Alloc( void );
	static void					Free( ARCRenderModelDecal *decal );

								// Creates decal projection info.
	static bool					CreateProjectionInfo( decalProjectionInfo_t &info, const arcFixedWinding &winding, const arcVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const arcMaterial *material, const int startTime );

								// Transform the projection info from global space to local.
	static void					GlobalProjectionInfoToLocal( decalProjectionInfo_t &localInfo, const decalProjectionInfo_t &info, const arcVec3 &origin, const arcMat3 &axis );

								// Creates a deal on the given model.
	void						CreateDecal( const ARCRenderModel *model, const decalProjectionInfo_t &localInfo );

								// Remove decals that are completely faded away.
	static ARCRenderModelDecal * RemoveFadedDecals( ARCRenderModelDecal *decals, int time );

								// Updates the vertex colors, removing any faded indexes,
								// then copy the verts to temporary vertex cache and adds a drawSurf.
	void						AddDecalDrawSurf( struct viewEntity_s *space );

								// Returns the next decal in the chain.
	ARCRenderModelDecal *		Next( void ) const { return nextDecal; }

	//void						ReadFromDemoFile( class ARCDemoFile *f );
	//void						WriteToDemoFile( class ARCDemoFile *f ) const;

private:
	static const int			MAX_DECAL_VERTS = 40;
	static const int			MAX_DECAL_INDEXES = 60;

	const arcMaterial *			material;
	surfTriangles_t					tri;
	arcDrawVert				verts[MAX_DECAL_VERTS];
	float						vertDepthFade[MAX_DECAL_VERTS];
	qglIndex_t					indexes[MAX_DECAL_INDEXES];
	int							indexStartTime[MAX_DECAL_INDEXES];
	ARCRenderModelDecal *		nextDecal;

								// Adds the winding triangles to the appropriate decal in the
								// chain, creating a new one if necessary.
	void						AddWinding( const arcWinding &w, const arcMaterial *decalMaterial, const arcPlane fadePlanes[2], float fadeDepth, int startTime );

								// Adds depth faded triangles for the winding to the appropriate
								// decal in the chain, creating a new one if necessary.
								// The part of the winding at the front side of both fade planes is not faded.
								// The parts at the back sides of the fade planes are faded with the given depth.
	void						AddDepthFadedWinding( const arcWinding &w, const arcMaterial *decalMaterial, const arcPlane fadePlanes[2], float fadeDepth, int startTime );
};

#endif