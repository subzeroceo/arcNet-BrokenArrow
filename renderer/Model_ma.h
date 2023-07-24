/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

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
	arcVec3					translate;
	arcVec3					rotate;
	arcVec3					scale;
	maTransform_s*			parent;
} maTransform_t;

typedef struct {
	int						edge[3];
	int						vertexNum[3];
	int						tVertexNum[3];
	int						vertexColors[3];
	arcVec3					vertexNormals[3];
} maFace_t;

typedef struct {

	//Transform to be applied
	maTransform_t*			transform;

	//Verts
	int						numVertexes;
	arcVec3 *				vertexes;
	int						numVertTransforms;
	arcVec4 *				vertTransforms;
	int						nextVertTransformIndex;

	//Texture Coordinates
	int						numTVertexes;
	arcVec2 *				tvertexes;

	//Edges
	int						numEdges;
	arcVec3 *				edges;

	//Colors
	int						numColors;
	byte*					colors;

	//Faces
	int						numFaces;
	maFace_t *				faces;

	//Normals
	int						numNormals;
	arcVec3 *				normals;
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
	arcNetList<maMaterial_t *>		materials;
	arcNetList<maObject_t *>		objects;
	arcHashTable<maTransform_t*> transforms;

	//Material Resolution
	arcHashTable<maFileNode_t*>		fileNodes;
	arcHashTable<maMaterialNode_t*>	materialNodes;

} maModel_t;

maModel_t	*MA_Load( const char *fileName );
void		MA_Free( maModel_t *ma );

#endif /* !__MODEL_MA_H__ */
