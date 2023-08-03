#ifndef __MODEL_MA_H__
#define __MODEL_MA_H__

/*
===============================================================================

	MA loader. (Maya Ascii Format)

===============================================================================
*/

typedef struct {
	char					name[128];
	char					parent[128];
} maNodeHeader_t;

typedef struct {
	char					name[128];
	int						size;
} maAttribHeader_t;

typedef struct maTransform_s {
	anVec3					translate;
	anVec3					rotate;
	anVec3					scale;
	maTransform_s*			parent;
} maTransform_t;

typedef struct {
	int						edge[3];
	int						vertexNum[3];
	int						tVertexNum[3];
	int						vertexColors[3];
	anVec3					vertexNormals[3];
} maFace_t;

typedef struct {
	//Transform to be applied
	maTransform_t*			transform;

	//Verts
	int						numVertexes;
	anVec3 *				vertexes;
	int						numVertTransforms;
	anVec4 *				vertTransforms;
	int						nextVertTransformIndex;

	//Texture Coordinates
	int						numTVertexes;
	anVec2 *				tvertexes;

	//Edges
	int						numEdges;
	anVec3 *				edges;

	//Colors
	int						numColors;
	byte*					colors;

	//Faces
	int						numFaces;
	maFace_t *				faces;

	//Normals
	int						numNormals;
	anVec3 *				normals;
	bool					normalsParsed;
	int						nextNormal;
} maMesh_t;

typedef struct {
	char					name[128];
	float					uOffset, vOffset;		// max lets you offset by material without changing texCoords
	float					uTiling, vTiling;		// multiply tex coords by this
	float					angle;					// in clockwise radians
} maMaterial_t;

typedef struct {
	char					name[128];
	int						materialRef;
	char					materialName[128];

	maMesh_t				mesh;
} maObject_t;


typedef struct {
	char					name[128];
	char					path[1024];
} maFileNode_t;

typedef struct maMaterialNode_s {
	char					name[128];

	maMaterialNode_s*		child;
	maFileNode_t*				file;

} maMaterialNode_t;

typedef struct maModel_s {
	ARC_TIME_T						timeStamp;
	anList<maMaterial_t *>		materials;
	anList<maObject_t *>		objects;
	anHashTable<maTransform_t*> transforms;

	//Material Resolution
	anHashTable<maFileNode_t*>		fileNodes;
	anHashTable<maMaterialNode_t*>	materialNodes;

} maModel_t;

maModel_t	*MA_Load( const char *fileName );
void		MA_Free( maModel_t *ma );

#endif // !__MODEL_MA_H__
