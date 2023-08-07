#ifndef __MODEL_LOCAL_H__
#define __MODEL_LOCAL_H__
#include "tr_local.h"

/*
===============================================================================

	Static model

===============================================================================
*/

class anModelStatic : public anRenderModel {
public:
	// the inherited public interface
	static anRenderModel *		Alloc();

								anModelStatic();
								~anModelStatic();

	void						InitFromFile( const char *fileName );
	void						PartialInitFromFile( const char *fileName );

	void						PurgeModel();
	void						Reset() {};

 	void						DrawEditorModel( anVec3 &origin, anMat3 &axis, bool camView );
	void						LoadModel();
	int							NumFrames() const;
	bool						IsLoaded();
	
	void						SetLevelLoadReferenced( bool referenced );
	bool						IsLevelLoadReferenced();

	void						TouchData();
	void						InitEmpty( const char *name );

	void						AddSurface( modelSurface_t surface );
	void						FinishSurfaces();

	void						FreeVertexCache();
	void						DirtyVertexAmbientCache();

	bool						IsSkeletalMesh() const;
	bool						IsWorldMesh( void ) const;
	void						MarkWorldMesh( void ) const;

	const char *				Name() const;
	void						Print() const;
	void						List() const;
	int							Memory() const;
	ARC_TIME_T					Timestamp() const;

	int							NumSurfaces() const;
	int							NumBaseSurfaces() const;

	const modelSurface_t *		Surface( int surfaceNum ) const;

	srfTriangles_t *			AllocSurfaceTriangles( int numVerts, int numIndexes ) const;
	void						FreeSurfaceTriangles( srfTriangles_t *tris ) const;

	srfTriangles_t *			ShadowHull() const;

	bool						IsStaticWorldModel() const;
	dynamicModel_t				IsDynamicModel() const;
	bool						IsDefaultModel() const;
	bool						IsReloadable() const;
	anRenderModel *				InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, anRenderModel *cachedModel );

	int							NumJoints( void ) const;
	const anMD6Joint *			GetJoints( void ) const;
	jointHandle_t				GetJointHandle( const char *name ) const;
	const char *				GetJointName( jointHandle_t handle ) const;
	const anJointQuat *			GetDefaultPose( void ) const;
	int							NearestJoint( int surfaceNum, int a, int b, int c ) const;
	anBounds					Bounds( const struct renderEntity_s *ent ) const;

	void						ReadFromDemoFile( class anDemoFile *f );
	void						WriteToDemoFile( class anDemoFile *f );

	float						DepthHack() const;

	void						MakeDefaultModel();

	bool						LoadASE( const char *fileName );
	bool						LoadFLT( const char *fileName );
	bool						LoadMA( const char *filename );
	bool						LoadObj( const char *filename );
	void						ParseOBJ( anList<anDrawVert> &drawVerts, const char *fileName, const char *objBuffer, int length );

	bool						ConvertASEToModelSurfaces( const struct aseModel_s *ase );
	bool						ConvertLWOToModelSurfaces( const struct st_lwObject *lwo );
	bool						ConvertMAToModelSurfaces( const struct maModel_s *ma );
	struct aseModel_s *			ConvertLWOToASE( const struct st_lwObject *obj, const char *fileName );

	bool						DeleteSurfaceWithId( int id );
	void						DeleteSurfacesWithNegativeId( void );
	bool						FindSurfaceWithId( int id, int &surfaceNum );

public:
	anList<modelSurface_t>		surfaces;
	anBounds					bounds;
	int							overlaysAdded;

	bool						isWorldMesh;

	// when an md5 is instantiated, the inverted joints array is stored to allow GPU skinning
	int							numInvertedJoints;
	idJointMat *				jointsInverted;

protected:
	int							lastModifiedFrame;
	int							lastArchivedFrame;

	anStr					name;
	srfTriangles_t *			shadowHull;
	bool						isStaticWorldModel;
	bool						defaulted;
	bool						purged;					// eventually we will have dynamic reloading
	bool						fastLoad;				// don't generate tangents and shadow data
	bool						reloadable;				// if not, reloadModels won't check timestamp
	bool						levelLoadReferenced;	// for determining if it needs to be freed
	ARC_TIME_T					timeStamp;

	static anCVarSystem				r_mergeModelSurfaces;	// combine model surfaces with the same material
	static anCVarSystem				r_slopVertex;			// merge xyz coordinates this far apart
	static anCVarSystem				r_slopTexCoord;			// merge texture coordinates this far apart
	static anCVarSystem				r_slopNormal;			// merge normals that dot less than this

	//idVertexBuffer *				editorVertexMesh;
	//idIndexBuffer *				editorIndexBuffer;
	//int							numEditorIndexes;
};

/*
===============================================================================

	Liquid model

===============================================================================
*/

class anLiquidModel : public anModelStatic {
public:
								anLiquidModel();

		void					InitFromFile( const char *fileName );
	dynamicModel_t				IsDynamicModel() const;
	anRenderModel *				InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, anRenderModel *cachedModel );
	anBounds					Bounds( const struct renderEntity_s *ent ) const;

	void						Reset();
	void						IntersectBounds( const anBounds &bounds, float displacement );

private:
	modelSurface_t				GenerateSurface( float lerp );
	void						WaterDrop( int x, int y, float *page );
	void						Update( void );

	int							verts_x;
	int							verts_y;
	float						scale_x;
	float						scale_y;
	int							time;
	int							liquid_type;
	int							update_tics;
	int							seed;

	anRandom					random;

	const anMaterial *			shader;
	struct deformInfo_s	*		deformInfo;		// used to create srfTriangles_t from base frames
											// and new vertexes

	float						density;
	float						drop_height;
	int							drop_radius;
	float						drop_delay;

	anList<float>				pages;
	float *						page1;
	float *						page2;

	anList<anDrawVertex>		verts;

	int							nextDropTime;

};

/*
===============================================================================

	PRT model

===============================================================================
*/

class idRenderModelPrt : public anModelStatic {
public:
								idRenderModelPrt();
								~idRenderModelPrt();

	void						InitFromFile( const char *fileName );
	void						TouchData();
	dynamicModel_t				IsDynamicModel() const;
	bool						IsLoaded() const;
	anRenderModel *				InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, anRenderModel *cachedModel );
	anBounds					Bounds( const struct renderEntity_s *ent ) const;
	int							NumFrames() const;
	float						DepthHack() const;
	int							Memory() const;

	void						CreateParticle( float simScale, anMat3 axis, int size, anDrawVert *verts ) const;
	void						LoadModel( void );
	void						ProcessGeometry( int frameNum, anMat3 axis, modelSurface_t *surf );

	void						ParseEffect( anParser *src );
	void						ParseSimulation( const char *fileName );

private:
	//int stages;
	const anDeclParticle *		particleSystem;
	int							numFrames;
	int							size;
	bool						normalizedAlpha;
	anBounds					modelBounds;
};


/*
===============================================================================

	Beam model

===============================================================================
*/

class idRenderModelBeam : public anModelStatic {
public:
	dynamicModel_t		IsDynamicModel() const;
	bool				IsLoaded() const;
	anRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, anRenderModel *cachedModel );
	anBounds			Bounds( const struct renderEntity_s *ent ) const;
};

/*
===============================================================================

	Beam model

===============================================================================
*/
#define MAX_TRAIL_PTS	20

struct Trail_t {
	int							lastUpdateTime;
	int							duration;

	anVec3						pts[MAX_TRAIL_PTS];
	int							numPoints;
};

class idRenderModelTrail : public anModelStatic {
	anList<Trail_t>				trails;
	int							numActive;
	anBounds					trailBounds;

public:
								idRenderModelTrail();

	dynamicModel_t				IsDynamicModel() const;
	bool						IsLoaded() const;
	anRenderModel *				InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, anRenderModel *cachedModel );
	anBounds					Bounds( const struct renderEntity_s *ent ) const;

	int							NewTrail( anVec3 pt, int duration );
	void						UpdateTrail( int index, anVec3 pt );
	void						DrawTrail( int index, const struct renderEntity_s *ent, srfTriangles_t *tri, float globalAlpha );
};

/*
===============================================================================

	Lightning model

===============================================================================
*/

class idRenderModelLightning : public anModelStatic {
public:
	dynamicModel_t				IsDynamicModel() const;
	bool						IsLoaded() const;
	anRenderModel *				InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, anRenderModel *cachedModel );
	anBounds					Bounds( const struct renderEntity_s *ent ) const;
};

/*
================================================================================

	idRenderModelSprite

================================================================================
*/
class idRenderModelSprite : public anModelStatic {
public:
	dynamicModel_t				IsDynamicModel() const;
	bool						IsLoaded() const;
	anRenderModel *				InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, anRenderModel *cachedModel );
	anBounds					Bounds( const struct renderEntity_s *ent ) const;
};

/*
===============================================================================

	Terrain Mesh

===============================================================================
*/

struct idHeightMapPixel_t {
	byte r;
	byte g;
	byte b;
	byte a;
};

struct idHeightMap_t {
	idHeightMap_t() {
		buffer = nullptr;
		width = 0;
		height = 0;
	}
	~idHeightMap_t() {
		if ( buffer != nullptr ) {
			R_StaticFree( buffer );
			buffer = nullptr;
		}

		width = 0;
		height = 0;
	}

	idHeightMapPixel_t *buffer;
	int width;
	int height;
};

class idRenderModelTerrain : public idRenderModelStatic {
public:
	void					InitFromFile( const char *fileName ) override;

	bool					ParseTerrainModel( const char *fileName );
	float					GetHeightForPixel( int x, int y );

	void					BuildTerrain( void );
private:
	idHeightMap_t			heightMap;
	const anMaterial *		megaTextureMaterial;
};

#endif // !__MODEL_LOCAL_H__