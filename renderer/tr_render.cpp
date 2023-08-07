#include "../idlib/Lib.h"
#include "Color/ColorSpace.h"
#pragma hdrstop

#include "tr_local.h"

//back end scene + lights rendering functions

/*
=================
RB_DrawElementsImmediate

Draws with immediate mode commands, which is going to be very slow.
This should never happen if the vertex cache is operating properly.
=================
*/
void RB_DrawElementsImmediate( const srfTriangles_t *tri ) {
	backEnd.pc.drawElements++;
	backEnd.pc.drawIndexes += tri->numIndexes;
	backEnd.pc.drawVerts += tri->numVerts;

	if ( tri->ambientSurface != nullptr  ) {
		if ( tri->indexes == tri->ambientSurface->indexes ) {
			backEnd.pc.drawRefIndexes += tri->numIndexes;
		}
		if ( tri->verts == tri->ambientSurface->verts ) {
			backEnd.pc.drawRefVerts += tri->numVerts;
		}
	}

	qglBegin( GL_TRIANGLES );
	for ( int i = 0; i < tri->numIndexes; i++ ) {
		qglTexCoord2fv( tri->verts[ tri->indexes[i] ].st );
		qglVertex3fv( tri->verts[ tri->indexes[i] ].xyz );
	}
	qglEnd();
}

/*
================
RB_DrawElementsWithCounters
================
*/
void RB_DrawElementsWithCounters( const srfTriangles_t *tri ) {
	backEnd.pc.drawElements++;
	backEnd.pc.drawIndexes += tri->numIndexes;
	backEnd.pc.drawVerts += tri->numVerts;

	if ( tri->ambientSurface != nullptr  ) {
		if ( tri->indexes == tri->ambientSurface->indexes ) {
			backEnd.pc.drawRefIndexes += tri->numIndexes;
		}
		if ( tri->verts == tri->ambientSurface->verts ) {
			backEnd.pc.drawRefVerts += tri->numVerts;
		}
	}

	if ( tri->indexCache && r_useIndexBuffers.GetBool() ) {
		qglDrawElements( GL_TRIANGLES, r_singleTriangle.GetBool() ? 3 : tri->numIndexes, GL_INDEX_TYPE, (int *)vertexCache.Position( tri->indexCache ) );
		backEnd.pc.c_vboIndexes += tri->numIndexes;
	} else {
		if ( r_useIndexBuffers.GetBool() ) {
			vertexCache.UnbindIndex();
		}
		qglDrawElements( GL_TRIANGLES,
						r_singleTriangle.GetBool() ? 3 : tri->numIndexes,
						GL_INDEX_TYPE,
						tri->indexes );
	}
}

/*
================
RB_DrawShadowElementsWithCounters

May not use all the indexes in the surface if caps are skipped
================
*/
void RB_DrawShadowElementsWithCounters( const srfTriangles_t *tri, int numIndexes ) {
	backEnd.pc.shadowElements++;
	backEnd.pc.shadowIndexes += numIndexes;
	backEnd.pc.shadowVerts += tri->numVerts;

	if ( tri->indexCache && r_useIndexBuffers.GetBool() ) {
		qglDrawElements( GL_TRIANGLES, r_singleTriangle.GetBool() ? 3 : numIndexes, GL_INDEX_TYPE, (int *)vertexCache.Position( tri->indexCache ) );
		backEnd.pc.c_vboIndexes += numIndexes;
	} else {
		if ( r_useIndexBuffers.GetBool() ) {
			vertexCache.UnbindIndex();
		}
		qglDrawElements( GL_TRIANGLES, r_singleTriangle.GetBool() ? 3 : numIndexes, GL_INDEX_TYPE, tri->indexes );
	}
}


/*
===============
RB_RenderTriangleSurface

Sets texcoord and vertex pointers
===============
*/
void RB_RenderTriangleSurface( const srfTriangles_t *tri ) {
	if ( !tri->ambientCache ) {
		RB_DrawElementsImmediate( tri );
		return;
	}

	//anDrawVertex *ac = (anDrawVertex *)vertexCache.Position( tri->ambientCache );
	const anDrawVertex *ac = static_cast<const anDrawVertex*>( vertexCache.Position( tri->ambientCache ) );
	qglVertexPointer( 3, GL_FLOAT, sizeof( anDrawVertex ), ac->xyz );
	qglTexCoordPointer( 2, GL_FLOAT, sizeof( anDrawVertex ), ac->st );

	RB_DrawElementsWithCounters( tri );
}

/*
===============
RB_T_RenderTriangleSurface
===============
*/
void RB_T_RenderTriangleSurface( const drawSurf_t *surf ) {
	RB_RenderTriangleSurface( surf->geo );
}

/*
===============
RB_EnterPrimaryViewDepthHack
===============
*/
void RB_EnterPrimaryViewDepthHack() {
	qglDepthRange( 0.0f, 0.5f );

	float matrix[16];
	for ( int i = 0; i < 16; i++ ) {
		matrix[i] = backEnd.viewDef->projectionMatrix[i];
	}

	matrix[14] *= 0.25f;

	qglMatrixMode( GL_PROJECTION );
	qglLoadMatrixf( matrix );
	qglMatrixMode( GL_MODELVIEW );
}

/*
===============
RB_EnterModelDepthHack
===============
*/
void RB_EnterModelDepthHack( float depth ) {
	qglDepthRange( 0.0f, 1.0f );

	float matrix[16];
	for ( int i = 0; i < 16; i++ ) {
		matrix[i] = backEnd.viewDef->projectionMatrix[i];
	}

	matrix[14] -= depth;

	qglMatrixMode( GL_PROJECTION );
	qglLoadMatrixf( matrix );
	qglMatrixMode( GL_MODELVIEW );
}

/*
===============
RB_LeaveDepthHack
===============
*/
void RB_LeaveDepthHack() {
	qglDepthRange( 0, 1 );

	qglMatrixMode( GL_PROJECTION );
	qglLoadMatrixf( backEnd.viewDef->projectionMatrix );
	qglMatrixMode( GL_MODELVIEW );
}

/*
====================
RB_RenderDrawSurfListWithFunction

The triangle functions can check backEnd.currentSpace != surf->space
to see if they need to perform any new matrix setup.  The modelview
matrix will already have been loaded, and backEnd.currentSpace will
be updated after the triangle function completes.
====================
*/
void RB_RenderDrawSurfListWithFunction( drawSurf_t **drawSurfs, int numDrawSurfs, void (*triFunc_)( const drawSurf_t *) ) {
	backEnd.currentSpace = nullptr;

	for ( int i = 0 ; i < numDrawSurfs; i++ ) {
		const drawSurf_t *drawSurf = drawSurfs[i];
		// change the matrix if needed
		if ( drawSurf->space != backEnd.currentSpace ) {
			qglLoadMatrixf( drawSurf->space->modelViewMatrix );
		}

		// change the scissor if needed
		if ( r_useScissor.GetBool() && !backEnd.currentScissor.Equals( drawSurf->scissorRect ) ) {
			backEnd.currentScissor = drawSurf->scissorRect;
			qglScissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1, backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1, backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
		}

		if ( drawSurf->space->primaryViewDepthHack ) {
			RB_EnterPrimaryViewDepthHack();
		}

		if ( drawSurf->space->modelDepthHack != 0.0f ) {
			RB_EnterModelDepthHack( drawSurf->space->modelDepthHack );
		}

		// render it
		triFunc_( drawSurf );

		if ( drawSurf->space->primaryViewDepthHack || drawSurf->space->modelDepthHack != 0.0f ) {
			RB_LeaveDepthHack();
		}

		backEnd.currentSpace = drawSurf->space;
	}
}

/*
======================
RB_RenderDrawSurfChainWithFunction
======================
*/
void RB_RenderDrawSurfChainWithFunction( const drawSurf_t *drawSurfs, void ( *triFunc_ )( const drawSurf_t *) ) {
	backEnd.currentSpace = nullptr;

	for ( const drawSurf_t *drawSurf = drawSurfs; drawSurf; drawSurf = drawSurf->nextOnLight ) {
		// change the matrix if needed
		if ( drawSurf->space != backEnd.currentSpace ) {
			qglLoadMatrixf( drawSurf->space->modelViewMatrix );
		}

		// change the scissor if needed
		if ( r_useScissor.GetBool() && !backEnd.currentScissor.Equals( drawSurf->scissorRect ) ) {
			backEnd.currentScissor = drawSurf->scissorRect;
			qglScissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
				backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
				backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
				backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
		}

		if ( drawSurf->space->primaryViewDepthHack ) {
			RB_EnterPrimaryViewDepthHack();
		}

		if ( drawSurf->space->modelDepthHack ) {
			RB_EnterModelDepthHack( drawSurf->space->modelDepthHack );
		}

		// render it
		triFunc_( drawSurf );

		if ( drawSurf->space->primaryViewDepthHack || drawSurf->space->modelDepthHack != 0.0f ) {
			RB_LeaveDepthHack();
		}

		backEnd.currentSpace = drawSurf->space;
	}
}

/*
======================
RB_GetShaderTextureMatrix
======================
*/
void RB_GetShaderTextureMatrix( const float *shaderRegisters, const textureStage_t *texture, float matrix[16] ) {
	matrix[0] = shaderRegisters[ texture->matrix[0][0] ];
	matrix[4] = shaderRegisters[ texture->matrix[0][1] ];
	matrix[8] = 0;
	matrix[12] = shaderRegisters[ texture->matrix[0][2] ];

	// we attempt to keep scrolls from generating incredibly large texture values, but
	// center rotations and center scales can still generate offsets that need to be > 1
	if ( matrix[12] < -40 || matrix[12] > 40 ) {
		matrix[12] -= ( int )matrix[12];
	}

	matrix[1] = shaderRegisters[ texture->matrix[1][0] ];
	matrix[5] = shaderRegisters[ texture->matrix[1][1] ];
	matrix[9] = 0;
	matrix[13] = shaderRegisters[ texture->matrix[1][2] ];
	if ( matrix[13] < -40 || matrix[13] > 40 ) {
		matrix[13] -= ( int )matrix[13];
	}

	matrix[2] = 0;
	matrix[6] = 0;
	matrix[10] = 1;
	matrix[14] = 0;

	matrix[3] = 0;
	matrix[7] = 0;
	matrix[11] = 0;
	matrix[15] = 1;
}

/*
======================
RB_LoadShaderTextureMatrix
======================
*/
void RB_LoadShaderTextureMatrix( const float *shaderRegisters, const textureStage_t *texture ) {
	float matrix[16];

	RB_GetShaderTextureMatrix( shaderRegisters, texture, matrix );
	qglMatrixMode( GL_TEXTURE );
	qglLoadMatrixf( matrix );
	qglMatrixMode( GL_MODELVIEW );
}

/*
======================
RB_BindVariableStageImage

Handles generating a cinematic frame if needed
======================
*/
void RB_BindVariableStageImage( const textureStage_t *texture, const float *shaderRegisters ) {
	if ( texture->cinematic ) {
		cinData_t cin;
		if ( r_skipDynamicTextures.GetBool() ) {
			globalImages->defaultImage->Bind();
			return;
		}

		//globalImages->blackImage->Bind();
		//globalImages = texture->cinematic->ImageForTime( ( int )(1000 * ( backEnd.viewDef->floatTime + backEnd.viewDef->renderView.shaderParms[11] ) ) );

		if ( globalImages.image ) {
			globalImages->cinematicImage->UploadScratch( cin.image, cin.imageWidth, cin.imageHeight );
		} else {
			globalImages->blackImage->Bind();
		}
	} else {
		// FIXME: see why image is invalid
		if ( texture->image ) {
			texture->image->Bind();
		}
	}
}

/*
======================
RB_BindStageTexture
======================
*/
void RB_BindStageTexture( const float *shaderRegisters, const textureStage_t *texture, const drawSurf_t *surf ) {
	// image
	RB_BindVariableStageImage( texture, shaderRegisters );

	// texgens
	if ( texture->texgen == TG_DIFFUSE_CUBE ) {
		qglTexCoordPointer( 3, GL_FLOAT, sizeof( anDrawVertex ), ( (anDrawVertex *)vertexCache.Position( surf->geo->ambientCache ) )->normal.ToFloatPtr() );
	}
	if ( texture->texgen == TG_SKYBOX_CUBE || texture->texgen == TG_WOBBLESKY_CUBE ) {
		qglTexCoordPointer( 3, GL_FLOAT, 0, vertexCache.Position( surf->dynamicTexCoords ) );
	}
	if ( texture->texgen == TG_REFLECT_CUBE ) {
		qglEnable( GL_TEXTURE_GEN_S );
		qglEnable( GL_TEXTURE_GEN_T );
		qglEnable( GL_TEXTURE_GEN_R );
		qglTexGenf( GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_EXT );
		qglTexGenf( GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_EXT );
		qglTexGenf( GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_EXT );
		qglEnableClientState( GL_NORMAL_ARRAY );
		qglNormalPointer( GL_FLOAT, sizeof( anDrawVertex ), ( (anDrawVertex *)vertexCache.Position( surf->geo->ambientCache ) )->normal.ToFloatPtr() );

		qglMatrixMode( GL_TEXTURE );
		float	mat[16];

		TransposeGLMatrix( backEnd.viewDef->worldSpace.modelViewMatrix, mat );

		qglLoadMatrixf( mat );
		qglMatrixMode( GL_MODELVIEW );
	}

	// matrix
	if ( texture->hasMatrix ) {
		RB_LoadShaderTextureMatrix( shaderRegisters, texture );
	}
}

/*
======================
RB_FinishStageTexture
======================
*/
void RB_FinishStageTexture( const textureStage_t *texture, const drawSurf_t *surf ) {
	if ( texture->texgen == TG_DIFFUSE_CUBE || texture->texgen == TG_SKYBOX_CUBE || texture->texgen == TG_WOBBLESKY_CUBE ) {
		qglTexCoordPointer( 2, GL_FLOAT, sizeof( anDrawVertex ), (void *)&( ( (anDrawVertex *)vertexCache.Position( surf->geo->ambientCache ) )->st) );
	}

	if ( texture->texgen == TG_REFLECT_CUBE ) {
		qglDisable( GL_TEXTURE_GEN_S );
		qglDisable( GL_TEXTURE_GEN_T );
		qglDisable( GL_TEXTURE_GEN_R );
		qglTexGenf( GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
		qglTexGenf( GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
		qglTexGenf( GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
		for ( !dev_useVertexArrayObject ) {
			qglBindVertexArray( qglConfig.global_vao );
			qglBIndBuffer( GL_ARRAY_BUFFER, qglConfig.global_vao );
		} else if ( dev_useVertexBuffer ) {
			qglVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( anDrawVertex ), &(anDrawVertex *)vertexCache.Position( surf->geo->ambientCache)->st );
		}
		qglMatrixMode( GL_TEXTURE );
		qglLoadIdentity();
		qglMatrixMode( GL_MODELVIEW );
	}

	if ( texture->hasMatrix ) {
		qglMatrixMode( GL_TEXTURE );
		qglLoadIdentity();
		qglMatrixMode( GL_MODELVIEW );
	}
}

//=============================================================================================

/*
=================
RB_DetermineLightScale

Sets:
backEnd.lightScale
backEnd.overBright

Find out how much we are going to need to overscale the lighting, so we
can down modulate the pre-lighting passes.

We only look at light calculations, but an argument could be made that
we should also look at surface evaluations, which would let surfaces
overbright past 1.0
=================
*/
void RB_DetermineLightScale( void ) {
	// the light scale will be based on the largest color component of any surface
	// that will be drawn.
	// should we consider separating rgb scales?

	// if there are no lights, this will remain at 1.0, so GUI-only
	// rendering will not lose any bits of precision
	float max = 1.0;

	for ( viewLight_t *vLight = backEnd.viewDef->viewLights; vLight; vLight = vLight->next ) {
		// lights with no surfaces or shaderparms may still be present
		// for debug display
		if ( !vLight->localInteractions && !vLight->globalInteractions
			&& !vLight->translucentInteractions ) {
			continue;
		}

		const anMaterial *shader = vLight->lightShader;
		int numStages = shader->GetNumStages();
		for ( int i = 0; i < numStages; i++ ) {
			const materialStage_t *stage = shader->GetStage( i );
			for ( int j = 0; j < 3; j++ ) {
				float	v = r_lightScale.GetFloat() * vLight->shaderRegisters[ stage->color.registers[j] ];
				if ( v > max ) {
					max = v;
				}
			}
		}
	}

	backEnd.pc.maxLightValue = max;
	if ( max <= tr.backEndRendererMaxLight ) {
		backEnd.lightScale = r_lightScale.GetFloat();
		backEnd.overBright = 1.0;
	} else {
		backEnd.lightScale = r_lightScale.GetFloat() * tr.backEndRendererMaxLight / max;
		backEnd.overBright = max / tr.backEndRendererMaxLight;
	}
}

/*
=================
RB_BeginDrawingView

Any mirrored or portaled views have already been drawn, so prepare
to actually render the visible surfaces for this view
=================
*/
void RB_BeginDrawingView( void ) {
	// set the modelview matrix for the viewer
	qglMatrixMode( GL_PROJECTION );
	qglLoadMatrixf( backEnd.viewDef->projectionMatrix );
	qglMatrixMode( GL_MODELVIEW );

	// set the window clipping
	qglViewport( tr.viewportOffset[0] + backEnd.viewDef->viewport.x1,
				tr.viewportOffset[1] + backEnd.viewDef->viewport.y1,
				backEnd.viewDef->viewport.x2 + 1 - backEnd.viewDef->viewport.x1,
				backEnd.viewDef->viewport.y2 + 1 - backEnd.viewDef->viewport.y1 );

	// the scissor may be smaller than the viewport for subviews
	qglScissor( tr.viewportOffset[0] + backEnd.viewDef->viewport.x1 + backEnd.viewDef->scissor.x1,
				tr.viewportOffset[1] + backEnd.viewDef->viewport.y1 + backEnd.viewDef->scissor.y1,
				backEnd.viewDef->scissor.x2 + 1 - backEnd.viewDef->scissor.x1
				backEnd.viewDef->scissor.y2 + 1 - backEnd.viewDef->scissor.y1 );
	backEnd.currentScissor = backEnd.viewDef->scissor;

	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );

	// we don't have to clear the depth / stencil buffer for 2D rendering
	if ( backEnd.viewDef->viewEntitys ) {
		qglStencilMask( 0xff );
		qglClearStencil( 1<<( qglConfig.stencilBits-1 ) );
		qglClear( GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
		qglEnable( GL_DEPTH_TEST );
	} else {
		qglDisable( GL_DEPTH_TEST );
		qglDisable( GL_STENCIL_TEST );
	}

	backEnd.qglState.faceCulling = -1;		// force face culling to set next time
	GL_Cull( CT_FRONT_SIDED );
}

/*
==================
R_SetDrawInteractions
==================
*/
void R_SetDrawInteraction( const materialStage_t *surfaceStage, const float *surfaceRegs, anImage **image, anVec4 matrix[2], anVec4 color[4] ) {
	*image = surfaceStage->texture.image;
		// we attempt to keep scrolls from generating incredibly large texture values, but
		// center rotations and center scales can still generate offsets that need to be > 1
	if ( surfaceStage->texture.hasMatrix ) {
		matrix[0].Set( 0, surfaceRegs[surfaceStage->texture.matrix[0][0]] );
		matrix[0].Set( 1, surfaceRegs[surfaceStage->texture.matrix[0][1]] );
		matrix[0].Set( 2, 0 );
		matrix[0].Set( 3, surfaceRegs[surfaceStage->texture.matrix[0][2]] );

		matrix[1].Set( 0, surfaceRegs[surfaceStage->texture.matrix[1][0]] );
		matrix[1].Set( 1, surfaceRegs[surfaceStage->texture.matrix[1][1]] );
		matrix[1].Set( 2, 0 );
		matrix[1].Set( 3, surfaceRegs[surfaceStage->texture.matrix[1][2]] );
	} else {
		if ( matrix[0].Get( 3 ) < -40 || matrix[0].Get( 3 ) > 40 ) {
			matrix[0].Set( 3, matrix[0].Get( 3 ) - static_cast<int>( matrix[0].Get( 3 ) ) );
		}
		if ( matrix[1].Get( 3 ) < -40 || matrix[1].Get( 3 ) > 40 ) {
			matrix[1].Set( 3, matrix[1].Get( 3 ) - static_cast<int>( matrix[1].Get( 3 ) ) );
		}
	} else {
		matrix[0].Set(0, 1);
		matrix[0].Set(1, 0);
		matrix[0].Set(2, 0);
		matrix[0].Set(3, 0);

		matrix[1].Set(0, 0);
		matrix[1].Set(1, 1);
		matrix[1].Set(2, 0);
		vmatrix[1].Set(3, 0);
	}

	if ( color != nullptr ) {
		for ( int i = 0; i < 4; i++ ) {
			color[i].Set( i, surfaceRegs[surfaceStage->color.registers[i]] );
			// clamp here, so card with greater range don't look different.
			// we could perform overbrighting like we do for lights, but
			// it doesn't currently look worth it.
			if ( color[i].Get( i ) < 0.0f ) {
				color[i].Set( i, 0.0f );
			} else if ( color[i].Get( i ) > 1.0f ) {
				color[i].Set( i, 1.0 f );
			}
		}
	}
}

/*
=================
RB_SubmittInteraction
=================
*/
static void RB_SubmittInteraction( drawInteraction_t *din, void (*DrawInteraction)(const drawInteraction_t *) ) {
	if ( !din->bumpImage ) {
		return;
	}

	if ( !din->diffuseImage || r_skipDiffuse.GetBool() ) {
		din->diffuseImage = globalImages->blackImage;
	}
	if ( !din->specularImage || r_skipSpecular.GetBool() || din->ambientLight ) {
		din->specularImage = globalImages->blackImage;
	}
	if ( !din->bumpImage || r_skipBump.GetBool() ) {
		din->bumpImage = globalImages->flatNormalMap;
	}

	bool shouldDraw = false;
	for ( int i = 0; i < 3; i++ ) {
		if ( din->diffuseColor[i] > 0 || din->specularColor[i] > 0 ) {
			shouldDraw = true;
			break;
		}
	}

	// if we wouldn't draw anything, don't call the Draw function
if ( shouldDraw && ( din->diffuseImage != globalImages->blackImage || din->specularImage != globalImages->blackImage ) ) {
	//if ( ( ( din->diffuseColor[0] > 0 || din->diffuseColor[1] > 0 || din->diffuseColor[2] > 0 ) && din->diffuseImage != globalImages->blackImage ) ||
	//( ( din->specularColor[0] > 0 || din->specularColor[1] > 0 || din->specularColor[2] > 0 ) && din->specularImage != globalImages->blackImage ) ) {
		DrawInteraction( din );
	}
}

/*
=============
RB_CreateSingleDrawInteractions

This can be used by different draw_* backends to decompose a complex light / surface
interaction into primitive interactions
=============
*/
void RB_CreateSingleDrawInteractions( const drawSurf_t *surf, void (*DrawInteraction)(const drawInteraction_t *) ) {
	const anMaterial	*surfaceShader = surf->material;
	const float			*surfaceRegs = surf->shaderRegisters;
	const viewLight_t	*vLight = backEnd.vLight;
	const anMaterial	*lightShader = vLight->lightShader;
	const float			*lightRegs = vLight->shaderRegisters;

	if ( r_skipInteractions.GetBool() || !surf->geo || !surf->geo->ambientCache ) {
		return;
	}

	if ( tr.logFile ) {
		RB_LogComment( "---------- RB_CreateSingleDrawInteractions %s on %s ----------\n", lightShader->GetName(), surfaceShader->GetName() );
	}
	// checks if the current space is different from the one
	// that the surface is in, and if it is, it sets the current space
	// to the one that the surface is in, loads the model view matrix of
	// the current space into the current model view matrix, and changes
	// the scissor if it is needed.

	// change the matrix and light projection vectors if needed
	if ( surf->space != backEnd.currentSpace ) {
		backEnd.currentSpace = surf->space;
		qglLoadMatrixf( surf->space->modelViewMatrix );
		// update the scissor rectangle
		//if ( r_useScissor.GetBool() && !backEnd.currentScissor.Equals( surf->scissorRect ) ) {
		backEnd.currentScissor = surf->scissorRect;
		qglScissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
		backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
		backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
		backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
		// hack depth range if needed
		if ( surf->space->primaryViewDepthHack || surf->space->modelDepthHack ) {
			RB_EnterPrimaryViewDepthHack();
			RB_EnterModelDepthHack( surf->space->modelDepthHack );
		}
	}
	drawInteraction_t inter;
	anPlane lightProject[4];
	//inter.surf = surf;
	//inter.lightFalloffImage = vLight->falloffImage;

	R_GlobalPointToLocal( surf->space->modelMatrix, vLight->globalLightOrigin, inter.localLightOrigin.ToVec3() );
	R_GlobalPointToLocal( surf->space->modelMatrix, backEnd.viewDef->renderView.viewOrg, inter.localViewOrigin.ToVec3() );
	inter.localLightOrigin[3] = 0;
	inter.localViewOrigin[3] = 1;
	inter.ambientLight = lightShader->IsAmbientLight();

	// the base projections may be modified by texture matrix on light stages
	for ( int i = 0; i < 4; i++ ) {
		R_GlobalPlaneToLocal( surf->space->modelMatrix, backEnd.vLight->lightProject[i], lightProject[i] );
		for ( int lightStageNum = 0; lightStageNum < lightShader->GetNumStages(); lightStageNum++ ) {
			const materialStage_t *lightStage = lightShader->GetStage( lightStageNum );
			// ignore stages that fail the condition
			if ( !lightRegs[ lightStage->conditionRegister ] ) {
				continue;
			}

			inter.lightImage = lightStage->texture.image;
			//for ( int j = 0; j < sizeof( inter.lightProjection ) / sizeof( inter.lightProjection[0] ); j++ ) {
			for ( int j = 0; j < 4; j++ )  {
				inter.lightProjection[j] = lightProject[i];
				// now multiply the texgen by the light texture matrix
				if ( lightStage->texture.hasMatrix ) {
					RB_GetShaderTextureMatrix( lightRegs, &lightStage->texture, backEnd.lightTextureMatrix );
					RB_BakeTextureMatrixIntoTexgen( reinterpret_cast<class anPlane *>( inter.lightProjection ), backEnd.lightTextureMatrix );
				}

				inter.bumpImage = nullptr;
				inter.specularImage = nullptr;
				inter.diffuseImage = nullptr;
				inter.diffuseColor[0] = inter.diffuseColor[1] = inter.diffuseColor[2] = inter.diffuseColor[3] = 0;
				inter.specularColor[0] = inter.specularColor[1] = inter.specularColor[2] = inter.specularColor[3] = 0;

				float lightColor[4];

				// backEnd.lightScale is calculated so that lightColor[] will never exceed
				// tr.backEndRendererMaxLight
				lightColor[0] = backEnd.lightScale * lightRegs[ lightStage->color.registers[0] ];
				lightColor[1] = backEnd.lightScale * lightRegs[ lightStage->color.registers[1] ];
				lightColor[2] = backEnd.lightScale * lightRegs[ lightStage->color.registers[2] ];
				lightColor[3] = lightRegs[ lightStage->color.registers[3] ];

				// go through the individual stages
				for ( int surfaceStageNum = 0; surfaceStageNum < surfaceShader->GetNumStages(); surfaceStageNum++ ) {
					const materialStage_t	*surfaceStage = surfaceShader->GetStage( surfaceStageNum );
					bool shouldDrawInteraction = surfaceRegs[surfaceStage->conditionRegister];

					switch ( surfaceStage->lighting ) {
						case SL_AMBIENT: {
							// ignore ambient stages while drawing interactions
							break;
						}
						case SL_BUMP: {
							// ignore stage that fails the condition
						if (0 == surfaceRegs[surfaceStage.conditionRegister]) {
							break;
						}
							if ( shouldDrawInteraction ) {
								RB_SubmittInteraction( &inter, DrawInteraction );
								R_SetDrawInteraction( surfaceStage, surfaceRegs, &inter.bumpImage, inter.bumpMatrix, nullptr );
							}
							break;
						}
						case SL_DIFFUSE: {
							// ignore stage that fails the condition
							if ( shouldDrawInteraction ) {
								if (inter.diffuseImage) {
									RB_SubmittInteraction( &inter, DrawInteraction );
								}
								R_SetDrawInteraction( surfaceStage, surfaceRegs, &inter.diffuseImage, inter.diffuseMatrix, inter.diffuseColor.ToFloatPtr() );
								float tempDiffuseColor[4] = {
									inter.diffuseColor[0] * lightColor[0],
									inter.diffuseColor[1] * lightColor[1],
									inter.diffuseColor[2] * lightColor[2],
									inter.diffuseColor[3] * lightColor[3]
								};
								inter.diffuseColor = Color4(tempDiffuseColor);
								inter.vertexColor = surfaceStage->vertexColor;
							}
							break;
						}
						case SL_SPECULAR: {
							if ( shouldDrawInteraction ) {
								if ( inter.specularImage ) {
									RB_SubmittInteraction( &inter, DrawInteraction );
								}
								R_SetDrawInteraction( surfaceStage, surfaceRegs, &inter.specularImage, inter.specularMatrix, inter.specularColor.ToFloatPtr() );
								float tempSpecularColor[4] = {
									inter.specularColor[0] * lightColor[0],
									inter.specularColor[1] * lightColor[1],
									inter.specularColor[2] * lightColor[2],
									inter.specularColor[3] * lightColor[3]
								};
								inter.specularColor = Color4( tempSpecularColor );
								inter.vertexColor = surfaceStage->vertexColor;
							}
							break;
						}
					}
				}

				// draw the final interaction
				RB_SubmittInteraction( &inter, DrawInteraction );
			}
		}
	}

	// unhack depth range if needed
	if ( surf->space->primaryViewDepthHack || surf->space->modelDepthHack != 0.0f ) {
		RB_LeaveDepthHack();
	}
}

/*
=============
RB_DrawView
=============
*/
void RB_DrawView( const void *data ) {
	const setBufferCommand_t *cmd;

	cmd = (const setBufferCommand_t *)data;

	backEnd.viewDef = cmd->viewDef;

	// we will need to do a new copyTexSubImage of the screen
	// when a SS_POST_PROCESS material is used
	backEnd.currentRenderCopied = false;

	// if there aren't any drawsurfs, do nothing
	if ( !backEnd.viewDef->numDrawSurfs ) {
		return;
	}

	// skip render bypasses everything that has models, assuming
	// them to be 3D views, but leaves 2D rendering visible
	if ( r_skipRender.GetBool() && backEnd.viewDef->viewEntitys ) {
		return;
	}

	// skip render context sets the wgl context to nullptr,
	// which should factor out the API cost, under the assumption
	// that all gl calls just return if the context isn't valid
	if ( r_skipRenderContext.GetBool() && backEnd.viewDef->viewEntitys ) {
		GLimp_DeactivateContext();
	}

	backEnd.pc.surfCounts += backEnd.viewDef->numDrawSurfs;

	RB_ShowOverdraw();

	// render the scene, jumping to the hardware specific interaction renderers
	RB_STD_DrawView();

	// restore the context for 2D drawing if we were stubbing it out
	if ( r_skipRenderContext.GetBool() && backEnd.viewDef->viewEntitys ) {
		GLimp_ActivateContext();
		RB_SetDefaultGLState();
	}
}
