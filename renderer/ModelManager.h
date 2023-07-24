#ifndef __MODELMANAGER_H__
#define __MODELMANAGER_H__

/*
===============================================================================

	Model Manager

	Temporarily created models do not need to be added to the model manager.

===============================================================================
*/

class ARCModelManager {
public:
	virtual					~ARCModelManager() {}

	// registers console commands and clears the list
	virtual	void			Init() = 0;

	// frees all the models
	virtual	void			Shutdown() = 0;

	// called only by renderer::BeginLevelLoad
	virtual void			BeginLevelLoad() = 0;

	// called only by renderer::EndLevelLoad
	virtual void			EndLevelLoad() = 0;

	// allocates a new empty render model.
	virtual ARCRenderModel	*AllocModel() = 0;

	// frees a render model
	virtual void			FreeModel( ARCRenderModel *model ) = 0;

	// returns NULL if modelName is NULL or an empty string, otherwise
	// it will create a default model if not loadable
	virtual	ARCRenderModel	*FindModel( const char *modelName ) = 0;
	virtual srfTriangles_t	*AllocStaticTriSurf( int verts, int indices ) = 0;
	virtual void			FreeStaticTriSurf( srfTriangles_t *tris ) = 0;
	virtual srfTriangles_t	*CopyStaticTriSurf( const srfTriangles_t *tri ) = 0;
	virtual	srfTriangles_t	*PolytopeSurface( int numPlanes, const arcPlane *planes, idWinding **windings ) = 0;
	virtual void			CreateSilIndexes( srfTriangles_t *tris ) = 0;
	virtual void			DeriveFacePlanes( srfTriangles_t *tris ) = 0;
	virtual	void			BoundTriSurf( srfTriangles_t *tri ) = 0;
	virtual	void			CleanupTriangles( srfTriangles_t *tris, bool createNormals, bool identifySilEdges, bool useUnsmoothedTangents, bool needSilMultiply ) = 0;
	virtual	void			SimpleCleanupTriangles( srfTriangles_t *tri ) = 0;
	virtual srfTriangles_t	*CreateShadowVolume( const srfTriangles_t *tri, const class idRenderLight *light, int optimize ) = 0;

	virtual class idRenderLight	*CreateLightDef( void ) = 0;
	virtual void			FreeLightDef( class idRenderLight *light ) = 0;

	//virtual bool				CheckModel( ARCRenderModel *model ) = 0;
	// returns NULL if not loadable
	virtual	ARCRenderModel *CheckModel( const char *modelName ) = 0;

	// returns the default cube model
	virtual	ARCRenderModel *DefaultModel() = 0;

	// world map parsing will add all the inline models with this call
	virtual	void			AddModel( ARCRenderModel *model ) = 0;

	// when a world map unloads, it removes its internal models from the list
	// before freeing them.
	// There may be an issue with multiple renderWorlds that share data...
	virtual	void			RemoveModel( ARCRenderModel *model ) = 0;

	// the reloadModels console command calls this, but it can
	// also be explicitly invoked
	virtual	void			ReloadModels( bool forceAll = false ) = 0;

	// write "touchModel <model>" commands for each non-world-map model
	virtual	void			WritePrecacheCommands( arcNetFile *f ) = 0;

	// called during vid_restart
	virtual	void			FreeModelVertexCaches() = 0;

	// print memory info
	virtual	void			PrintMemInfo( MemInfo_t *mi ) = 0;
};

// this will be statically pointed at a private implementation
extern	ARCModelManager	*renderModelManager;

#endif