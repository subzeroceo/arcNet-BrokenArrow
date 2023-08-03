#ifndef __MODEL_ASE_H__
#define __MODEL_ASE_H__

/*
===============================================================================

	ASE loader. (3D Studio Max ASCII Export)

===============================================================================
*/

#define MAX_ASE_MATERIALS			32
#define MAX_ASE_OBJECTS				64
#define MAX_ASE_ANIMATIONS			32
#define MAX_ASE_ANIMATION_FRAMES	512

#define VERBOSE( x ) { if ( ase.verbose ) { printf x ; } }

typedef struct {
	int				numMaterials;
	aseMaterial_t	materials[MAX_ASE_MATERIALS];
	aseGeomObject_t objects[MAX_ASE_OBJECTS];

	char *					buffer;
	char *					curpos;
	int						len;

	int						currentObject;
	bool					verbose;
	bool				grabAnims;
} ase_t;

typedef struct {
	int						vertexNum[3];
	int						tVertexNum[3];
	anVec3					faceNormal;
	anVec3					vertexNormals[3];
	byte					vertexColors[3][4];
} aseFace_t;

typedef struct {
	int						timeValue;

	int						numVertexes;
	int						numTVertexes;
	int						numCVertexes;
	int						numFaces;
	int						numTVFaces;
	int						numCVFaces;

	anVec3					transform[4];			// applied to normals

	bool					colorsParsed;
	bool					normalsParsed;
	anVec3 *				vertexes;
	anVec2 *				tvertexes;
	anVec3 *				cvertexes;
	aseVertex_t				*verts;
	aseTVertex_t			*tangVerts;

	aseFace_t *				tfaces;
	aseFace_t *				faces;
	int currentFace, currentVertex;
} aseMesh_t;

typedef struct {
	char					name[128];
	float					uOffset, vOffset;		// max lets you offset by material without changing texCoords
	float					uTiling, vTiling;		// multiply tex coords by this
	float					angle;					// in clockwise radians
} aseMaterial_t;

// contains the animate sequence of a single surface
// using a single material
typedef struct {
	char					name[128];
	int						materialRef;

	aseMesh_t				mesh;
	int numAnimations;

	// frames are only present with animations
	anList<aseMesh_t*>		frames;			// aseMesh_t
	aseMeshAnimation_t		anim;
} aseObject_t;

typedef struct aseModel_s {
	ARC_TIME_T				timeStamp;
	anList<aseMaterial_t *>	materials;
	anList<aseObject_t *>	objects;
} aseModel_t;

typedef struct {
	int						numFrames;
	aseMesh_t				frames[MAX_ASE_ANIMATION_FRAMES];

	int						currentFrame;
} aseMeshAnimation_t;

typedef struct {
	float x, y, z;
	float nx, ny, nz;
	float s, t;
} aseVertex_t;

typedef struct {
	float s, t;
} aseTVertex_t;

typedef int aseFace_t[3];
static char s_token[1024];
static ase_t ase;

aseModel_t *ASE_Load( const char *fileName );
void		ASE_Free( aseModel_t *ase );

static void ASE_Process( void );
static void ASE_FreeGeomObject( int ndx );

#endif // !__MODEL_ASE_H__
