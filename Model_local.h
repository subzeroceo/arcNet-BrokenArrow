#ifndef __MODEL_LOCAL_H__
#define __MODEL_LOCAL_H__

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
	virtual						~anModelStatic();

	virtual void				InitFromFile( const char *fileName );
	virtual void				PartialInitFromFile( const char *fileName );
	virtual void				PurgeModel();
	virtual void				Reset() {};
	virtual void				LoadModel();
	virtual bool				IsLoaded();
	virtual void				SetLevelLoadReferenced( bool referenced );
	virtual bool				IsLevelLoadReferenced();
	virtual void				TouchData();
	virtual void				InitEmpty( const char *name );
	virtual void				AddSurface( modelSurface_t surface );
	virtual void				FinishSurfaces();
	virtual void				FreeVertexCache();
	virtual const char *		Name() const;
	virtual void				Print() const;
	virtual void				List() const;
	virtual int					Memory() const;
	virtual ARC_TIME_T				Timestamp() const;
	virtual int					NumSurfaces() const;
	virtual int					NumBaseSurfaces() const;
	virtual const modelSurface_t *Surface( int surfaceNum ) const;
	virtual surfTriangles_t *	AllocSurfaceTriangles( int numVerts, int numIndexes ) const;
	virtual void				FreeSurfaceTriangles( surfTriangles_t *tris ) const;
	virtual surfTriangles_t *	ShadowHull() const;
	virtual bool				IsStaticWorldModel() const;
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual bool				IsDefaultModel() const;
	virtual bool				IsReloadable() const;
	virtual anRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, anRenderModel *cachedModel );
	virtual int					NumJoints( void ) const;
	virtual const anM8DJoint *	GetJoints( void ) const;
	virtual jointHandle_t		GetJointHandle( const char *name ) const;
	virtual const char *		GetJointName( jointHandle_t handle ) const;
	virtual const anJointQuat *	GetDefaultPose( void ) const;
	virtual int					NearestJoint( int surfaceNum, int a, int b, int c ) const;
	virtual anBounds			Bounds( const struct renderEntity_s *ent ) const;
	virtual void				ReadFromDemoFile( class anDemoFile *f );
	virtual void				WriteToDemoFile( class anDemoFile *f );
	virtual float				DepthHack() const;

	void						MakeDefaultModel();

	bool						LoadASE( const char *fileName );
	bool						LoadLWO( const char *fileName );
	bool						LoadFLT( const char *fileName );
	bool						LoadMA( const char *filename );

	bool						ConvertASEToModelSurfaces( const struct aseModel_s *ase );
	bool						ConvertLWOToModelSurfaces( const struct st_lwObject *lwo );
	bool						ConvertMAToModelSurfaces (const struct maModel_s *ma );

	struct aseModel_s *			ConvertLWOToASE( const struct st_lwObject *obj, const char *fileName );

	bool						DeleteSurfaceWithId( int id );
	void						DeleteSurfacesWithNegativeId( void );
	bool						FindSurfaceWithId( int id, int &surfaceNum );

public:
	anList<modelSurface_t>		surfaces;
	anBounds					bounds;
	int							overlaysAdded;

protected:
	int							lastModifiedFrame;
	int							lastArchivedFrame;

	anString						name;
	surfTriangles_t *			shadowHull;
	bool						isStaticWorldModel;
	bool						defaulted;
	bool						purged;					// eventually we will have dynamic reloading
	bool						fastLoad;				// don't generate tangents and shadow data
	bool						reloadable;				// if not, reloadModels won't check timestamp
	bool						levelLoadReferenced;	// for determining if it needs to be freed
	ARC_TIME_T						timeStamp;

	static anCVarSystem				r_mergeModelSurfaces;	// combine model surfaces with the same material
	static anCVarSystem				r_slopVertex;			// merge xyz coordinates this far apart
	static anCVarSystem				r_slopTexCoord;			// merge texture coordinates this far apart
	static anCVarSystem				r_slopNormal;			// merge normals that dot less than this
};

/*
===============================================================================

	MD5 animated model

===============================================================================
*/

class anM8DMesh {
	friend class				anRenderModelM8D;

public:
								anM8DMesh();
								~anM8DMesh();

 	void						ParseMesh( anLexer &parser, int numJoints, const arcJointMat *joints );
	void						UpdateSurface( const struct renderEntity_s *ent, const arcJointMat *joints, modelSurface_t *surf );
	anBounds					CalcBounds( const arcJointMat *joints );
	int							NearestJoint( int a, int b, int c ) const;
	int							NumVerts( void ) const;
	int							NumTris( void ) const;
	int							NumWeights( void ) const;

private:
	anList<anVec2>				texCoords;			// texture coordinates
	int							numWeights;			// number of weights
	anVec4 *					scaledWeights;		// joint weights
	int *						weightIndex;		// pairs of: joint offset + bool true if next weight is for next vertex
	const anMaterial *			shader;				// material applied to mesh
	int							numTris;			// number of triangles
	struct deformInfo_s *		deformInfo;			// used to create surfTriangles_t from base frames and new vertexes
	int							surfaceNum;			// number of the static surface created for this mesh

	void						TransformVerts( anDrawVertex *verts, const arcJointMat *joints );
	void						TransformScaledVerts( anDrawVertex *verts, const arcJointMat *joints, float scale );
};

class anRenderModelM8D : public anModelStatic {
public:
	virtual void				InitFromFile( const char *fileName );
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual anBounds			Bounds( const struct renderEntity_s *ent ) const;
	virtual void				Print() const;
	virtual void				List() const;
	virtual void				TouchData();
	virtual void				PurgeModel();
	virtual void				LoadModel();
	virtual int					Memory() const;
	virtual anRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, anRenderModel *cachedModel );
	virtual int					NumJoints( void ) const;
	virtual const anM8DJoint *	GetJoints( void ) const;
	virtual jointHandle_t		GetJointHandle( const char *name ) const;
	virtual const char *		GetJointName( jointHandle_t handle ) const;
	virtual const anJointQuat *	GetDefaultPose( void ) const;
	virtual int					NearestJoint( int surfaceNum, int a, int b, int c ) const;

private:
	anList<anM8DJoint>			joints;
	anList<anJointQuat>			defaultPose;
	anList<anM8DMesh>			meshes;

	void						CalculateBounds( const arcJointMat *joints );
	void						GetFrameBounds( const renderEntity_t *ent, anBounds &bounds ) const;
	void						DrawJoints( const renderEntity_t *ent, const struct viewDef_s *view ) const;
	void						ParseJoint( anLexer &parser, anM8DJoint *joint, anJointQuat *defaultPose );
};

/*
===============================================================================

	MD3 animated model

===============================================================================
*/

struct md3Header_s;
struct md3Surface_s;

class idRenderModelMD3 : public anModelStatic {
public:
	virtual void				InitFromFile( const char *fileName );
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual anRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, anRenderModel *cachedModel );
	virtual anBounds			Bounds( const struct renderEntity_s *ent ) const;

private:
	int							index;			// model = tr.models[model->index]
	int							dataSize;		// just for listing purposes
	struct md3Header_s *		md3;			// only if type == MOD_MESH
	int							numLods;

	void						LerpMeshVertexes( surfTriangles_t *tri, const struct md3Surface_s *surf, const float backlerp, const int frame, const int oldframe ) const;
};

/*
===============================================================================

	Liquid model

===============================================================================
*/

class ARCLiquidModel : public anModelStatic {
public:
								ARCLiquidModel();

	virtual void				InitFromFile( const char *fileName );
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual anRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, anRenderModel *cachedModel );
	virtual anBounds			Bounds( const struct renderEntity_s *ent ) const;

	virtual void				Reset();
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

	arcRandom					random;

	const anMaterial *			shader;
	struct deformInfo_s	*		deformInfo;		// used to create surfTriangles_t from base frames
											// and new vertexes

	float						density;
	float						drop_height;
	int							drop_radius;
	float						drop_delay;

	anList<float>				pages;
	float *						page1;
	float *						page2;

	anList<anDrawVertex>			verts;

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

	virtual void				InitFromFile( const char *fileName );
	virtual void				TouchData();
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual anRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, anRenderModel *cachedModel );
	virtual anBounds			Bounds( const struct renderEntity_s *ent ) const;
	virtual float				DepthHack() const;
	virtual int					Memory() const;

private:
	const arcDeclParticle *		particleSystem;
};

/*
===============================================================================

	Beam model

===============================================================================
*/

class idRenderModelBeam : public anModelStatic {
public:
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual bool				IsLoaded() const;
	virtual anRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, anRenderModel *cachedModel );
	virtual anBounds			Bounds( const struct renderEntity_s *ent ) const;
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

	virtual dynamicModel_t		IsDynamicModel() const;
	virtual bool				IsLoaded() const;
	virtual anRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, anRenderModel *cachedModel );
	virtual anBounds			Bounds( const struct renderEntity_s *ent ) const;

	int							NewTrail( anVec3 pt, int duration );
	void						UpdateTrail( int index, anVec3 pt );
	void						DrawTrail( int index, const struct renderEntity_s *ent, surfTriangles_t *tri, float globalAlpha );
};

/*
===============================================================================

	Lightning model

===============================================================================
*/

class idRenderModelLightning : public anModelStatic {
public:
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual bool				IsLoaded() const;
	virtual anRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, anRenderModel *cachedModel );
	virtual anBounds			Bounds( const struct renderEntity_s *ent ) const;
};

/*
================================================================================

	idRenderModelSprite

================================================================================
*/
class idRenderModelSprite : public anModelStatic {
public:
	virtual	dynamicModel_t	IsDynamicModel() const;
	virtual	bool			IsLoaded() const;
	virtual	anRenderModel *	InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, anRenderModel *cachedModel );
	virtual	anBounds		Bounds( const struct renderEntity_s *ent ) const;
};

#endif // !__MODEL_LOCAL_H__
