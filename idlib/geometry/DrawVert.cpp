#include "../Lib.h"
#include "DrawVert.h"
#pragma hdrstop

/*
=============
anDrawVertex::Normalize
=============
*/
void anDrawVertex::Normalize( void ) {
	normal.Normalize();
	tangents[1].Cross( normal, tangents[0] );
	tangents[1].Normalize();
	tangents[0].Cross( tangents[1], normal );
	tangents[0].Normalize();
}

/*
============
anShadowCache::CreateShadowCache
============
*/
int anShadowCache::CreateShadowCache( anShadowCache *vertexCache, const anDrawVertex *verts, const int numVerts ) {
	for ( int i = 0; i < numVerts; i++ ) {
		vertexCache[i*2+0].xyzw[0] = verts[i].xyz[0];
		vertexCache[i*2+0].xyzw[1] = verts[i].xyz[1];
		vertexCache[i*2+0].xyzw[2] = verts[i].xyz[2];
		vertexCache[i*2+0].xyzw[3] = 1.0f;

		vertexCache[i*2+1].xyzw[0] = verts[i].xyz[0];
		vertexCache[i*2+1].xyzw[1] = verts[i].xyz[1];
		vertexCache[i*2+1].xyzw[2] = verts[i].xyz[2];
		vertexCache[i*2+1].xyzw[3] = 0.0f;
	}
	return numVerts * 2;
}

/*
===================
anShadowCache::CreateShadowCache
===================
*/
int anShadowCache::CreateShadowCache( anShadowCache *vertexCache, const anDrawVertex *verts, const int numVerts ) {
	for ( int i = 0; i < numVerts; i++ ) {
		vertexCache[0].xyzw[0] = verts[i].xyz[0];
		vertexCache[0].xyzw[1] = verts[i].xyz[1];
		vertexCache[0].xyzw[2] = verts[i].xyz[2];
		vertexCache[0].xyzw[3] = 1.0f;
		*(unsigned int *)vertexCache[0].color = *(unsigned int *)verts[i].color;
		*(unsigned int *)vertexCache[0].color2 = *(unsigned int *)verts[i].color2;

		vertexCache[1].xyzw[0] = verts[i].xyz[0];
		vertexCache[1].xyzw[1] = verts[i].xyz[1];
		vertexCache[1].xyzw[2] = verts[i].xyz[2];
		vertexCache[1].xyzw[3] = 0.0f;
		*(unsigned int *)vertexCache[1].color = *(unsigned int *)verts[i].color;
		*(unsigned int *)vertexCache[1].color2 = *(unsigned int *)verts[i].color2;

		vertexCache += 2;
	}
	return numVerts * 2;
}
