#ifndef __MODEL_ASE_H__
#define __MODEL_ASE_H__

/*
===============================================================================

	ASE loader. (3D Studio Max ASCII Export)

===============================================================================
*/

typedef struct {
	int						vertexNum[3];
	int						tVertexNum[3];
	arcVec3					faceNormal;
	arcVec3					vertexNormals[3];
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

	arcVec3					transform[4];			// applied to normals

	bool					colorsParsed;
	bool					normalsParsed;
	arcVec3 *				vertexes;
	arcVec2 *				tvertexes;
	arcVec3 *				cvertexes;
	aseFace_t *				faces;
} aseMesh_t;

typedef struct {
	char					name[128];
	float					uOffset, vOffset;		// max lets you offset by material without changing texCoords
	float					uTiling, vTiling;		// multiply tex coords by this
	float					angle;					// in clockwise radians
} aseMaterial_t;

typedef struct {
	char					name[128];
	int						materialRef;

	aseMesh_t				mesh;

	// frames are only present with animations
	arcNetList<aseMesh_t*>		frames;			// aseMesh_t
} aseObject_t;

typedef struct aseModel_s {
	ARC_TIME_T					timeStamp;
	arcNetList<aseMaterial_t *>	materials;
	arcNetList<aseObject_t *>	objects;
} aseModel_t;


aseModel_t *ASE_Load( const char *fileName );
void		ASE_Free( aseModel_t *ase );

#endif /* !__MODEL_ASE_H__ */
