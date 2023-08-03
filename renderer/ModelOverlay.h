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

#endif