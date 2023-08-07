#ifndef __MODEL_MDSERIES_H__
#define __MODEL_MDSERIES_H__
#include "tr_local.h"
#include "Model_local.h"

#define COLLISION_JOINT_HANDLE( id )	( ( id ) >= 0 ? INVALID_JOINT : ((jointHandle_t) ( -1 - id )) )
#define JOINTHANDLE_FOR_TRACE( trace )	( trace ? COLLISION_JOINT_HANDLE( trace->c.id ) : INVALID_JOINT )

/*
===============================================================================

	MD6 animated model

===============================================================================
*/

class anMD6Mesh_Reform {
	friend class				anRenderModelMD6;
public:
	anMD6Mesh_Reform();
	~anMD6Mesh_Reform();

	void						ParseMesh( anLexer &parser, int numJoints, const anJointMat *joints );

	anBounds					CalcBounds( const anJointMat *joints );

	int							NearestJoint( int a, int b, int c ) const;
	int							NumVerts( void ) const;
	int							NumTris( void ) const;
	int							NumWeights( void ) const;

	const anMaterial *			GetShader( void ) const { return shader; }

public:
	struct deformInfo_s * 		deformInfo;			// used to create srfTriangles_t from base frames and new vertexes

private:
	anList<anVec2>				texCoords;			// texture coordinates
	int							numWeights;			// number of weights
	anVec4 * 					scaledWeights;		// joint weights
	int *						weightIndex; 		// pairs of: joint offset + bool true if next weight is for next vertex
	const anMaterial * 			shader;				// material applied to mesh
	int							numTris;			// number of triangles	
	int							surfaceNum;			// number of the static surface created for this mesh

	int							numMeshJoints;
	float						maxJointVertDist;
	byte *						meshJoints;
	anBounds					bounds;
};

class anRenderModelMD6 : public idRenderModelStatic {
public:
								anRenderModelMD6();
								~anRenderModelMD6();

	void						InitFromFile( const char *fileName );

	dynamicModel_t				IsDynamicModel() const;
	anBounds					Bounds( const class anRenderEntity *ent ) const;

	void						Print() const;
	void						List() const;
	void						TouchData();

	anRenderModel * 			InstantiateDynamicModel( const class anRenderEntity *ent, const struct viewDef_s *view, anRenderModel *cachedModel );
	void						PurgeModel();

	void						LoadModel();
	int							Memory() const;

	int							NumJoints( void ) const;
	const anMD6Joint *			GetJoints( void ) const;

	jointHandle_t				GetJointHandle( const char *name )  const;
	const char *				GetJointName( jointHandle_t handle ) const;

	const anJointQuat *			GetDefaultPose( void ) const;
	int							NearestJoint( int surfaceNum, int a, int b, int c ) const;

	float						DepthHack() const;
private:
	void						ParseInitBlock( anLexer &parser );

	anJointBuffer *				jointBuffer;

	anList<anMD6Joint>			joints;
	anList<anJointQuat>			defaultPose;
	anList<anMD6Mesh_Reform>			meshes;

	anJointMat *				poseMat3;
	anList<anJointMat>			invertDefaultPose;

	anBounds					expandedBounds;

	void						CalculateBounds( const anJointMat *joints );
	void						DrawJoints( const anRenderEntityParms *ent, const struct viewDef_s *view ) const;
	void						ParseJoint( anLexer &parser, anMD6Joint *joint, anJointQuat *defaultPose );
};

class idRenderModelMD6Instance : public idRenderModelStatic {
public:
	bool						IsSkeletalMesh() const override;
	void						CreateStaticMeshSurfaces( const anList<anMD6Mesh_Reform> &meshes );

	void						UpdateGPUSurface( struct deformInfo_s *deformInfo, const const anRenderEntityParms *ent, modelSurface_t *surf, const anMaterial *shader );
public:
								// This is shared across all models of the same instance!
	anJointBuffer *				jointBuffer;
};


/*
===============================================================================

	MD5 animated model

===============================================================================
*/
typedef struct {
	int numSurfaces;
	int ofsSurfaces;                // first surface, others follow
	int ofsEnd;                     // next lod follows
} md5LOD_t;

class anMD5Mesh {
	friend class				anRenderModelM8D;

public:
								anMD5Mesh();
								~anMD5Mesh();

 	void						ParseMesh( anLexer &parser, int numJoints, const anJointMat *joints );
	void						UpdateSurface( const struct renderEntity_s *ent, const anJointMat *joints, modelSurface_t *surf );
	anBounds					CalcBounds( const anJointMat *joints );
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
	struct deformInfo_s *		deformInfo;			// used to create srfTriangles_t from base frames and new vertexes
	int							surfaceNum;			// number of the static surface created for this mesh

	void						TransformVerts( anDrawVertex *verts, const anJointMat *joints );
	void						TransformScaledVerts( anDrawVertex *verts, const anJointMat *joints, float scale );
};

class anRenderModelM8D : public anModelStatic {
public:
	void						InitFromFile( const char *fileName );
	dynamicModel_t				IsDynamicModel() const;
	anBounds					Bounds( const struct renderEntity_s *ent ) const;
	void						Print() const;
	void						List() const;
	void						TouchData();
	void						PurgeModel();
	void						LoadModel();
	int							Memory() const;
	anRenderModel *				InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, anRenderModel *cachedModel );
	int							NumJoints( void ) const;
	const anMD6Joint *			GetJoints( void ) const;
	jointHandle_t				GetJointHandle( const char *name ) const;
	const char *				GetJointName( jointHandle_t handle ) const;
	const anJointQuat *			GetDefaultPose( void ) const;
	int							NearestJoint( int surfaceNum, int a, int b, int c ) const;

private:
	anList<anMD6Joint>			joints;
	anList<anJointQuat>			defaultPose;
	anList<anMD5Mesh>			meshes;

	void						CalculateBounds( const anJointMat *joints );
	void						GetFrameBounds( const renderEntity_t *ent, anBounds &bounds ) const;
	void						DrawJoints( const renderEntity_t *ent, const struct viewDef_s *view ) const;
	void						ParseJoint( anLexer &parser, anMD6Joint *joint, anJointQuat *defaultPose );
};


/*
===============================================================================

	MD3 animated triangle model and file format

Private structures used by the MD3 loader.

 md3Surface_t

 CHUNK			SIZE
 header			sizeof( md3Surface_t )
 shaders		sizeof( md3Shader_t )	 * numShaders
 triangles[0]	sizeof( md3Triangle_t ) * numTriangles
 st				sizeof( md3St_t ) 		* numVerts
 XyzNormals		sizeof( md3XyzNormal_t ) * numVerts * numFrames

===============================================================================
*/

#define MD3_IDENT			(('3'<<24)+('P'<<16)+('D'<<8)+'I')
#define MD3_VERSION			15

// surface geometry should not exceed these limits
#define	SHADER_MAX_VERTEXES	1000
#define	SHADER_MAX_INDEXES	(6*SHADER_MAX_VERTEXES)

// limits
#define MD3_MAX_LODS		4
#define	MD3_MAX_TRIANGLES	8192	// per surface
#define MD3_MAX_VERTS		4096	// per surface
#define MD3_MAX_SHADERS		256		// per surface
#define MD3_MAX_FRAMES		1024	// per model
#define	MD3_MAX_SURFACES	32		// per model
#define MD3_MAX_TAGS		16		// per frame
#define MAX_MD3PATH			64		// from quake3

// vertex scales
#define	MD3_XYZ_SCALE		(1.0/64)

typedef struct md3Frame_s {
	anVec3 				bounds[2];
	anVec3 				localOrigin;
	float 				radius;
	char 				name[16];
} md3Frame_t;

typedef struct md3Tag_s {
	char 				name[MAX_MD3PATH];	// tag name
	anVec3 				origin;
	anVec3 				axis[3];
} md3Tag_t;

typedef struct md3Surface_s {
	int			ident;				//

	char		name[MAX_MD3PATH];	// polyset name

	int			flags;
	int			numFrames;			// all surfaces in a model should have the same

	int			numShaders;			// all surfaces in a model should have the same
	int			numVerts;

	int			numTriangles;
	int			ofsTriangles;

	int			ofsShaders;			// offset from start of md3Surface_t
	int			ofsSt;				// texture coords are common for all frames
	int			ofsXyzNormals;		// numVerts * numFrames

	int			ofsEnd;				// next surface follows
} md3Surface_t;

typedef struct {
	char				name[MAX_MD3PATH];
	const anMaterial *	shader;			// for in-game use
} md3Shader_t;

typedef struct {
	int			indexes[3];
} md3Triangle_t;

typedef struct {
	float		st[2];
} md3St_t;

typedef struct {
	short		xyz[3];
	short		normal;
} md3XyzNormal_t;

typedef struct md3Header_s {
	int			ident;
	int			version;

	char		name[MAX_MD3PATH];	// model name

	int			flags;

	int			numFrames;
	int			numTags;
	int			numSurfaces;

	int			numSkins;

	int			ofsFrames;			// offset for first frame
	int			ofsTags;			// numFrames * numTags
	int			ofsSurfaces;		// first surface, others follow

	int			ofsEnd;				// end of file
} md3Header_t;

struct md3Header_s;
struct md3Surface_s;

class idRenderModelMD3_Legacy : public anModelStatic {
public:
	void						InitFromFile( const char *fileName );
	dynamicModel_t				IsDynamicModel() const;
	//bool						IsLoaded();
	anRenderModel *				InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, anRenderModel *cachedModel );
	anBounds					Bounds( const struct renderEntity_s *ent ) const;

private:
	int							index;			// model = tr.models[model->index]
	int							dataSize;		// just for listing purposes
	struct md3Header_s *		md3;			// only if type == MOD_MESH
	int							numLods;

	void						LerpMeshVertexes( srfTriangles_t *tri, const struct md3Surface_s *surf, const float backlerp, const int frame, const int oldframe ) const;
};
/*
===============================================================================

	Raven MDR animated model

===============================================================================
*/

class idRenderModelMDR_Legacy : public idRenderModelStatic {
public:
								idRenderModelMDR_Legacy();
								~idRenderModelMDR_Legacy();

	virtual void				InitFromFile( const char *fileName );
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual anBounds			Bounds( const class anRenderEntity *ent ) const;
	virtual void				TouchData();
	virtual void				LoadModel();
	virtual int					Memory() const;
	virtual idRenderModel *		InstantiateDynamicModel( const class anRenderEntity *ent, const struct viewDef_s *view, anRenderModel *cachedModel );
	virtual int					NumJoints( void ) const;
	virtual const anMD6Joint *	GetJoints( void ) const;
	virtual jointHandle_t		GetJointHandle( const char *name ) const;
	virtual const char *		GetJointName( jointHandle_t handle ) const;
	virtual const anointQuat *	GetDefaultPose( void ) const;
	virtual int					NearestJoint( int surfaceNum, int a, int b, int c ) const;
private:
	bool						LoadMDRFile( void *buffer, int filesize );
	void						RenderFramesToModel( anRenderModelStatic *staticModel, const struct anRenderEntityParms *ent, const struct viewDef_s *view, const anMaterial *overrideMaterial, int scale );

	int							numLods;

	anBounds					bounds;

	anList<struct mdrFrame_t *>	frames;
	anList<const anMaterial *>	materials;

	int							dataSize;
	struct mdrHeader_t *		modelData;

	anRenderModelStatic *		cachedModel;
};

#endif // !__MODEL_MDSERIES_H__