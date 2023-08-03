#define MAYA_DEFAULT_CAMERA		"camera1"

#define	ANIM_TX			BIT( 0 )
#define	ANIM_TY			BIT( 1 )
#define	ANIM_TZ			BIT( 2 )
#define	ANIM_QX			BIT( 3 )
#define	ANIM_QY			BIT( 4 )
#define	ANIM_QZ			BIT( 5 )

typedef enum {
	WRITE_MESH,
	WRITE_ANIM,
	WRITE_CAMERA
} exportType_t;

typedef struct {
	anCQuats				q;
	anVec3				t;
} jointFrame_t;

typedef struct {
	anCQuats				q;
	anVec3				t;
	float				fov;
} cameraFrame_t;

/*
==============================================================================================

	arcTokenizer

==============================================================================================
*/

class arcTokenizer {
private:
	int					currentToken;
	anStringList		tokens;

public:
						arcTokenizer()			{ Clear(); };
	void				Clear( void )			{ currentToken = 0;	tokens.Clear(); };

	int					SetTokens( const char *buffer );
	const char			*NextToken( const char *errorstring = NULL );

	bool				TokenAvailable( void )	{ return currentToken < tokens.Num(); };
	int					Num( void ) 			{ return tokens.Num(); };
	void				UnGetToken( void )		{ if ( currentToken > 0 ) { currentToken--; } };
	const char			*GetToken( int index )	{ if ( ( index >= 0 ) && ( index < tokens.Num() ) ) { return tokens[index]; } else { return NULL; } };
	const char			*CurrentToken( void )	{ return GetToken( currentToken ); };
};

/*
==============================================================================================

	arcExportOptions

==============================================================================================
*/

class arcNamePair {
public:
	anString	from;
	anString	to;
};

class arcAnimGroup {
public:
	anString		name;
	anStringList	joints;
};

class arcExportOptions {
private:
	arcTokenizer				tokens;

	void					Reset( const char *commandline );

public:
	anString					commandLine;
	anString					src;
	anString					dest;
	anString					game;
	anString					prefix;
	float					scale;
	exportType_t			type;
	bool					ignoreMeshes;
	bool					clearOrigin;
	bool					clearOriginAxis;
	bool					ignoreScale;
	int						startframe;
	int						endframe;
	int						framerate;
	float					xyzPrecision;
	float					quatPrecision;
	anString					align;
	anList<arcNamePair>	renamejoints;
	anList<arcNamePair>	remapjoints;
	anStringList			keepjoints;
	anStringList			skipmeshes;
	anStringList			keepmeshes;
	anList<arcAnimGroup *>	exportgroups;
	anList<arcAnimGroup>	groups;
	float					rotate;
	float					jointThreshold;
	int						cycleStart;

							arcExportOptions( const char *commandline, const char *ospath );

	bool					jointInExportGroup( const char *jointname );
};

/*
==============================================================================

arcExportJoint

==============================================================================
*/

class arcExportJoint {
public:
	anString						name;
	anString						realname;
	anString						longname;
	int							index;
	int							exportNum;
	bool						keep;

	float						scale;
	float						invscale;

	MFnDagNode					*dagnode;

	arcHierarchy<arcExportJoint>mayaNode;
	arcHierarchy<arcExportJoint>exportNode;

	anVec3						t;
	anMat3						wm;

	anVec3						idt;
	anMat3						idwm;

	anVec3						bindpos;
	anMat3						bindmat;

	int							animBits;
	int							firstComponent;
	jointFrame_t				baseFrame;
	int							depth;

								arcExportJoint();
	arcExportJoint				&operator=( const arcExportJoint &other );
};

/*
==============================================================================

misc structures

==============================================================================
*/

typedef struct {
	arcExportJoint			*joint;
	float					jointWeight;
	anVec3					offset;
} exportWeight_t;

typedef struct {
	anVec3					pos;
	anVec2					texCoords;
	int						startweight;
	int						numWeights;
} exportVertex_t;

typedef struct {
	int						indexes[ 3 ];
} exportTriangle_t;

typedef struct {
	anVec2					uv[ 3 ];
} exportUV_t;

ARC_INLINE int operator==( exportVertex_t a, exportVertex_t b ) {
	if ( a.pos != b.pos ) {
		return false;
	}

	if ( ( a.texCoords[ 0 ] != b.texCoords[ 0 ] ) || ( a.texCoords[ 1 ] != b.texCoords[ 1 ] ) ) {
		return false;
	}

	if ( ( a.startweight != b.startweight ) || ( a.numWeights != b.numWeights ) ) {
		return false;
	}

	return true;
}

/*
========================================================================

.MD3 triangle model file format

========================================================================
*/

#define MD3_IDENT			(('3'<<24)+('P'<<16)+('D'<<8)+'I')
#define MD3_VERSION			15

// limits
#define MD3_MAX_LODS		4
#define	MD3_MAX_TRIANGLES	8192	// per surface
#define MD3_MAX_VERTS		4096	// per surface
#define MD3_MAX_SHADERS		256		// per surface
#define MD3_MAX_FRAMES		1024	// per model
#define	MD3_MAX_SURFACES	32		// per model
#define MD3_MAX_TAGS		16		// per frame

// vertex scales
#define	MD3_XYZ_SCALE		(1.0/64)

// surface geometry should not exceed these limits
#define	SHADER_MAX_VERTEXES	1000
#define	SHADER_MAX_INDEXES	(6*SHADER_MAX_VERTEXES)

// the maximum size of game reletive pathnames
#define	MAX_Q3PATH		64

typedef struct md3Frame_s {
	anVec3		bounds[2];
	anVec3		localOrigin;
	float		radius;
	char		name[16];
} md3Frame_t;

typedef struct md3Tag_s {
	char		name[MAX_Q3PATH];	// tag name
	anVec3		origin;
	anVec3		axis[3];
} md3Tag_t;

/*
** md3Surface_t
**
** CHUNK			SIZE
** header			sizeof( md3Surface_t )
** shaders			sizeof( md3Shader_t ) * numShaders
** triangles[0]		sizeof( md3Triangle_t ) * numTriangles
** st				sizeof( md3St_t ) * numVerts
** XyzNormals		sizeof( md3XyzNormal_t ) * numVerts * numFrames
*/
typedef struct {
	int			ident;				//

	char		name[MAX_Q3PATH];	// polyset name

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
	char		name[MAX_Q3PATH];
	int			shaderIndex;	// for in-game use
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

typedef struct {
	int			ident;
	int			version;

	char		name[MAX_Q3PATH];	// model name

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

/*
==============================================================================

arcExportMesh

==============================================================================
*/

class arcExportMesh {
public:

	anString						name;
	anString						shader;

	bool						keep;

	anList<exportVertex_t>		verts;
	anList<exportTriangle_t>	tris;
	anList<exportWeight_t>		weights;
	anList<exportUV_t>			uv;

								arcExportMesh() { keep = true; };
	void						ShareVerts( void );
	void						GetBounds( anBounds &bounds ) const;
	void						Merge( arcExportMesh *mesh );
};

/*
==============================================================================

arcExportModel

==============================================================================
*/

class arcExportModel {
public:
	arcExportJoint				*exportOrigin;
	anList<arcExportJoint>		joints;
	arcHierarchy<arcExportJoint>	mayaHead;
	arcHierarchy<arcExportJoint>	exportHead;
	anList<int>					cameraCuts;
	anList<cameraFrame_t>		camera;
	anList<anBounds>			bounds;
	anList<jointFrame_t>		jointFrames;
	anList<jointFrame_t	*>		frames;
	int							frameRate;
	int							numFrames;
	int							skipjoints;
	int							export_joints;
	anList<arcExportMesh *>		meshes;

								arcExportModel();
								~arcExportModel();
	arcExportJoint				*FindJointReal( const char *name );
	arcExportJoint				*FindJoint( const char *name );
	bool						WriteMesh( const char *filename, arcExportOptions &options );
	bool						WriteAnim( const char *filename, arcExportOptions &options );
	bool						WriteCamera( const char *filename, arcExportOptions &options );
};

/*
==============================================================================

Maya

==============================================================================
*/

class arcMayaExport {
private:
	arcExportModel			model;
	arcExportOptions			&options;

	void					FreeDagNodes( void );

	float					TimeForFrame( int num ) const;
	int						GetMayaFrameNum( int num ) const;
	void					SetFrame( int num );


	void					GetBindPose( MObject &jointNode, arcExportJoint *joint, float scale );
	void					GetLocalTransform( arcExportJoint *joint, anVec3 &pos, anMat3 &mat );
	void					GetWorldTransform( arcExportJoint *joint, anVec3 &pos, anMat3 &mat, float scale );

	void					CreateJoints( float scale );
	void					PruneJoints( anStringList &keepjoints, anString &prefix );
	void					RenameJoints( anList<arcNamePair> &renamejoints, anString &prefix );
	bool					RemapParents( anList<arcNamePair> &remapjoints );

	MObject					FindShader( MObject& setNode );
	void					GetTextureForMesh( arcExportMesh *mesh, MFnDagNode &dagNode );

	arcExportMesh			*CopyMesh( MFnSkinCluster &skinCluster, float scale );
	void					CreateMesh( float scale );
	void					CombineMeshes( void );

	void					GetAlignment( anString &alignName, anMat3 &align, float rotate, int startframe );

	const char				*GetObjectType( MObject object );

	float					GetCameraFov( arcExportJoint *joint );
	void					GetCameraFrame( arcExportJoint *camera, anMat3 &align, cameraFrame_t *cam );
	void					CreateCameraAnim( anMat3 &align );

	void					GetDefaultPose( anMat3 &align );
	void					CreateAnimation( anMat3 &align );

public:
							arcMayaExport( arcExportOptions &exportOptions ) : options( exportOptions ) { };
							~arcMayaExport();

	void					ConvertModel( void );
	void					ConvertToMD3( void );
};
