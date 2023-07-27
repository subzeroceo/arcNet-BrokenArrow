#pragma hdrstop
#include "/idlib/precompiled.h"
#include "tr_local.h"

anRenderSystemLocal	tr;
anRenderSystem *renderSystem = &tr;

void _qglTexImage2D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
    printf( "target: %d\n", target );
    printf( "level: %d\n", level );
    printf( "internalformat: %d\n", internalformat );
    printf( "width: %d\n", width );
    printf( "height: %d\n", height );
    printf( "border: %d\n", border );
    printf( "format: %d\n", format );
    printf( "type: %d\n", type );
    // Rest of the function body
}

/*
=====================
R_PerformanceCounters

This prints both front and back end counters, so it should
only be called when the back end thread is idle.
=====================
*/
static void R_PerformanceCounters( void ) {
	if ( r_showPrimitives.GetInteger() != 0 ) {
		float megaBytes = globalImages->SumOfUsedImages() / ( 1024*1024.0 );
		if ( r_showPrimitives.GetInteger() > 1 ) {
			common->Printf( "v:%i ds:%i t:%i/%i v:%i/%i st:%i sv:%i image:%5.1f MB\n",
				tr.pc.numViews,
				backEnd.pc.drawElements + backEnd.pc.shdwElements,
				backEnd.pc.drawIndexes / 3,
				( backEnd.pc.drawIndexes - backEnd.pc.drawRefIndexes ) / 3,
				backEnd.pc.drawVerts,
				( backEnd.pc.drawVerts - backEnd.pc.drawRefVerts ),
				backEnd.pc.shdwIndexes / 3,
				backEnd.pc.shdwVerts,
				megaBytes
				);
		} else {
			common->Printf( "views:%i draws:%i tris:%i (shdw:%i) (vbo:%i) image:%5.1f MB\n",
				tr.pc.numViews,
				backEnd.pc.drawElements + backEnd.pc.shdwElements,
				( backEnd.pc.drawIndexes + backEnd.pc.shdwIndexes ) / 3,
				backEnd.pc.shdwIndexes / 3,
				backEnd.pc.vboIndexes / 3,
				megaBytes
				);
		}
	}

	if ( r_showDynamic.GetBool() ) {
		common->Printf( "callback:%i md5:%i dfrmVerts:%i dfrmTris:%i tangTris:%i guis:%i\n",
			tr.pc.entityDefCallbacks,
			tr.pc.generateMD5,
			tr.pc.deformedVerts,
			tr.pc.deformedIndexes/3,
			tr.pc.tangentIndexes/3,
			tr.pc.guiSurfaces
			);
	}

	if ( r_showCull.GetBool() ) {
		common->Printf( "%i sin %i sclip  %i sout %i bin %i bout\n",
			tr.pc.sphereCull, tr.pc.sphereClip, tr.pc.sphereCullOut,
			tr.pc.boxCull, tr.pc.boxCullOut );
	}

	if ( r_showAlloc.GetBool() ) {
		common->Printf( "alloc:%i free:%i\n", tr.pc.allocCount, tr.pc.freeCount );
	}

	if ( r_showInteractions.GetBool() ) {
		common->Printf( "createInteractions:%i createLightTris:%i createShadowVolumes:%i\n",
			tr.pc.createInteractions, tr.pc.createLightTris, tr.pc.createShadowVolumes );
 	}
	if ( r_showDefs.GetBool() ) {
		common->Printf( "viewEntities:%i  shadowEntities:%i  viewLights:%i\n", tr.pc.visibleViewEntities,
			tr.pc.shadowViewEntities, tr.pc.viewLights );
	}
	if ( r_showUpdates.GetBool() ) {
		common->Printf( "entityUpdates:%i  entityRefs:%i  lightUpdates:%i  lightReferences:%i\n",
			tr.pc.entityUpdates, tr.pc.entityReferences,
			tr.pc.lightUpdates, tr.pc.lightReferences );
	}
	if ( r_showMemory.GetBool() ) {
		int	m1 = frameData ? frameData->memoryHighwater : 0;
		common->Printf( "frameData: %i (%i)\n", R_CountFrameData(), m1 );
	}
	if ( r_showLightScale.GetBool() ) {
		common->Printf( "lightScale: %f\n", backEnd.pc.maxLightVal );
	}

	memset( &tr.pc, 0, sizeof( tr.pc ) );
	memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
}

void idRenderBackend::ExecuteBackEndCommands( const setBufferCommand_t *cmds ) {
}

/*
====================
R_IssueCommandBuffer
====================
*/
void anRenderSystemLocal::RenderCommandBuffers( const setBufferCommand_t * const cmdHead ) {
	// if there isn't a draw view command, do nothing to avoid swapping a bad frame
	bool	hasView = false;

	for ( const setBufferCommand_t * cmd = cmdHead; cmd; cmd = (const setBufferCommand_t *)cmd->next ) {
		//if ( frameData->cmdHead->commandId == RC_NOP && !frameData->cmdHead->next ) {
		if ( cmd->commandId == RC_DRAW_VIEW_3D || cmd->commandId == RC_DRAW_VIEW_GUI ) {
			//return;
			hasView = true;
			break;
		}
	}
	if ( !hasView ) {
		return;
	}

	// r_skipBackEnd allows the entire time of the back end
	// to be removed from performance measurements, although
	// nothing will be drawn to the screen.  If the prints
	// are going to a file, or r_skipBackEnd is later disabled,
	// usefull data can be received.

	// r_skipRender is usually more usefull, because it will still
	// draw 2D graphics
	if ( !r_skipBackEnd.GetBool() ) {
		if ( qglConfig.timerQueryAvail ) {
			if ( tr.timerQueryId == 0 ) {
				qglGenQueriesARB( 1, & tr.timerQueryId );
			}
			qglBeginQueryARB( GL_TIME_ELAPSED_EXT, tr.timerQueryId );
			RB_ExecuteBackEndCommands( cmdHead );
			qglEndQueryARB( GL_TIME_ELAPSED_EXT );
			qglFlush();
		} else {
			RB_ExecuteBackEndCommands( cmdHead );
		}
	}
	R_ClearCommandChain();
	// pass in null for now - we may need to do some map specific hackery in the future
	//resolutionScale.InitForMap( NULL );
}

/*
============
R_GetCommandBuffer

Returns memory for a command buffer (stretchPicCommand_t,
setBufferCommand_t, etc) and links it to the end of the
current command chain.
============
*/
void *R_GetCommandBuffer( int bytes ) {
	setBufferCommand_t	*cmd;

	cmd = (setBufferCommand_t *)R_FrameAlloc( bytes );
	cmd->next = NULL;
	frameData->cmdTail->next = &cmd->commandId;
	frameData->cmdTail = cmd;

	return (void *)cmd;
}

void RB_CheckOverflow( int verts, int indexes ) {
	surfTriangles_t *tris;
		if ( tris.numVertexes + verts < SHADER_MAX_VERTEXES && tris.numIndexes + indexes < SHADER_MAX_INDEXES ) {
		return;
	}

	RB_EndFrame();

	if ( verts >= SHADER_MAX_VERTEXES ) {
		common->Error( "RB_CheckOverflow: verts > MAX (%d > %d)", verts, SHADER_MAX_VERTEXES );
	}

	if ( indexes >= SHADER_MAX_INDEXES ) {
		common->Error( "RB_CheckOverflow: indices > MAX (%d > %d)", indexes, SHADER_MAX_INDEXES );
	}

	RB_BeginFrame( tris.shader, tris.fogNum );
	// write a function that draws and checks OpenGL Overflow for debugging
    qglPushMatrix();
    qglPopMatrix();
}

void RB_ShowOverdraw() {

}

/*
====================
R_ClearCommandChain

Called after every buffer submission
and by R_ToggleSmpFrame
====================
*/
void R_ClearCommandChain( void ) {
	// clear the command chain
	frameData->cmdHead = frameData->cmdTail = (setBufferCommand_t *)R_FrameAlloc( sizeof( *frameData->cmdHead ) );
	frameData->cmdHead->commandId = RC_NOP;
	frameData->cmdHead->next = NULL;
}

/*
=================
R_ViewStatistics
=================
*/
static void R_ViewStatistics( viewDef_t *parms ) {
	// report statistics about this view
	if ( !r_showSurfaces.GetBool() ) {
		return;
	}
	common->Printf( "view:%p surfs:%i\n", parms, parms->numDrawSurfs );
}

/*
=============
R_AddDrawViewCmd

This is the main 3D rendering command.  A single scene may
have multiple views if a mirror, portal, or dynamic texture is present.
=============
*/
void R_AddDrawViewCmd( viewDef_t *parms ) {//, bool guiOnly ) {
	setBufferCommand_t	*cmd = (setBufferCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	//cmd->commandId = ( guiOnly ) ? RC_DRAW_VIEW_GUI : RC_DRAW_VIEW_3D;
	cmd->commandId = RC_DRAW_VIEW;
	cmd->viewDef = parms;

	if ( parms->viewEntitys ) {
		// save the command for r_lockSurfaces debugging
		tr.lockSurfacesCmd = *cmd;
	}

	tr.pc.numViews++;

	R_ViewStatistics( parms );
}

/*
=============
R_AddPostProcess

This issues the command to do a post process after all the views have
been rendered.
=============
*/
void R_AddDrawPostProcess( viewDef_t *parms ) {
	postProcessCommand_t * cmd = (postProcessCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	cmd->commandId = RC_POST_PROCESS;
	cmd->viewDef = parms;
}

/*
======================
R_LockSurfaceScene

r_lockSurfaces allows a developer to move around
without changing the composition of the scene, including
culling.  The only thing that is modified is the
view position and axis, no front end work is done at all

Add the stored off command again, so the new rendering will use EXACTLY
the same surfaces, including all the culling, even though the transformation
matricies have been changed.  This allow the culling tightness to be
evaluated interactively.
======================
*/
void R_LockSurfaceScene( viewDef_t *parms ) {
	setBufferCommand_t	*cmd;
	viewEntity_t			*vModel;

	// set the matrix for world space to eye space
	R_SetViewMatrix( parms );
	tr.lockSurfacesCmd.viewDef->worldSpace = parms->worldSpace;

	// update the view origin and axis, and all
	// the entity matricies
	for ( vModel = tr.lockSurfacesCmd.viewDef->viewEntitys; vModel; vModel = vModel->next ) {
		GL_MultMatrix( vModel->modelMatrix,
			tr.lockSurfacesCmd.viewDef->worldSpace.modelViewMatrix,
			vModel->modelViewMatrix );
	}

	// add the stored off surface commands again
	cmd = (setBufferCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	*cmd = tr.lockSurfacesCmd;
}

/*
=============
R_CheckCvars

See if some cvars that we watch have changed
=============
*/
static void R_CheckCvars( void ) {
	globalImages->CheckCvars();

	// gamma stuff
	if ( r_gamma.IsModified() || r_brightness.IsModified() ) {
		r_gamma.ClearModified();
		r_brightness.ClearModified();
		R_SetColorMappings();
	}
	// filtering
	// NOTE: dont forget to define these cvars in the primary headers later
	if ( r_maxAnisotropicFiltering.IsModified() || r_useTrilinearFiltering.IsModified() ) {//|| r_lodBias.IsModified() ) {
		anLibrary::Printf( "Updating texture filter parameters.\n" );
		r_maxAnisotropicFiltering.ClearModified();
		r_useTrilinearFiltering.ClearModified();
		//r_lodBias.ClearModified();
		for ( int i = 0; i < globalImages->images.Num(); i++ ) {
			if ( globalImages->images[i] ) {
				globalImages->images[i]->Bind();
				globalImages->images[i]->SetTexParameters();
			}
		}
	}

	// NOTE: dont forget to define these cvars in the primary headers later
	extern anCVarSystem r_useSeamlessCubeMap;
	if ( r_useSeamlessCubeMap.IsModified() ) {
		r_useSeamlessCubeMap.ClearModified();
		if ( qglConfig.useSeamlessCubeMap ) {
			if ( r_useSeamlessCubeMap.GetBool() ) {
				qglEnable( GL_TEXTURE_CUBE_MAP_SEAMLESS );
			} else {
				qglDisable( GL_TEXTURE_CUBE_MAP_SEAMLESS );
			}
		}
	}

	// NOTE: dont forget to define these cvars in the primary headers later
	extern anCVarSystem r_useSRGB;
	if ( r_useSRGB.IsModified() ) {
		r_useSRGB.ClearModified();
		if ( qglConfig.useSRGBFramebuffer ) {
			if ( r_useSRGB.GetBool() ) {
				qglEnable( GL_FRAMEBUFFER_SRGB );
			} else {
				qglDisable( GL_FRAMEBUFFER_SRGB );
			}
		}
	}

	// NOTE: dont forget to define these cvars in the primary headers later
	if ( r_multiSamples.IsModified() ) {
		if ( r_multiSamples.GetInteger() > 0 ) {
			qglEnable( GL_MULTISAMPLE_ARB );
		} else {
			qglDisable( GL_MULTISAMPLE_ARB );
		}
	}
	// check for changes to logging state
	GLimp_EnableLogging( r_logFile.GetInteger() != 0 );
}

anRenderSystemLocal::anRenderSystemLocal() : unitSquareTriangles( NULL ), zeroOneCubeTriangles( NULL ), testImageTriangles( NULL ) {
}

anRenderSystemLocal::~anRenderSystemLocal( void ) {
}

/*
=============
SetColor

This can be used to pass general information to the current material, not
just colors
=============
*/
void anRenderSystemLocal::SetColor( const anVec4 &rgba ) {
	guiModel->SetColor( rgba[0], rgba[1], rgba[2], rgba[3] );
}

void anRenderSystemLocal::SetColor2( const anVec4 & rgba ) {
	currentColorNativeBytesOrder = LittleLong( PackColor( rgba ) );
}

void anRenderSystemLocal::SetColor4( float r, float g, float b, float a ) {
	guiModel->SetColor( r, g, b, a );
}

uint32 anRenderSystemLocal::GetColor2() {
	return LittleLong( currentColorNativeBytesOrder );
}

/*
=============
DrawStretchPic
=============
*/
void anRenderSystemLocal::DrawStretchPic( const anDrawVertex *verts, const qglIndex_t *indexes, int vertCount, int indexCount, const anMaterial *material, bool clip, float min_x, float min_y, float max_x, float max_y ) {
	guiModel->DrawStretchPic( verts, indexes, vertCount, indexCount, material,
		clip, min_x, min_y, max_x, max_y );
}

void anRenderSystemLocal::DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const anMaterial *material ) {
	guiModel->DrawStretchPic( x, y, w, h, s1, t1, s2, t2, material );
}

void anRenderSystemLocal::DrawStretchTri( anVec2 p1, anVec2 p2, anVec2 p3, anVec2 t1, anVec2 t2, anVec2 t3, const anMaterial *material ) {
	tr.guiModel->DrawStretchTri( p1, p2, p3, t1, t2, t3, material );
}

/*
=============
GlobalToNormalizedDeviceCoordinates
=============
*/
void anRenderSystemLocal::GlobalToNormalizedDeviceCoordinates( const anVec3 &global, anVec3 &ndc ) {
	ARCRenderMatrixes->GlobalToNormalizedDeviceCoordinates( global, ndc );
}

/*
=====================
anRenderSystemLocal::GetGLSettings
=====================
*/
//void anRenderSystemLocal::GetGLSettings( int& width, int& height ) {
//	width = qglConfig.vidWidth;
//	height = qglConfig.vidHeight;
//}
void anRenderSystemLocal::SetGLState( const GLuint glState ) {
	glState = qglState;
}

/*
=====================
anRenderSystemLocal::DrawFilled
=====================
*/
void anRenderSystemLocal::DrawFilled( const anVec4 & color, float x, float y, float w, float h ) {
	SetColor2( color );
	DrawStretchPic( x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, whiteMaterial );
}

/*
=============
idRenderSystemLocal::DrawStretchPic
=============
*/
static triIndex_t quadPicIndexes[6] = { 3, 0, 2, 2, 0, 1 };
void anRenderSystemLocal::DrawStretchPic( const anVec4 & topLeft, const anVec4 & topRight, const anVec4 & bottomRight, const anVec4 & bottomLeft, const anMaterial * material ) {
	if ( !R_IsInitialized() ) {
		return;
	}
	if ( material == NULL ) {
		return;
	}

	anDrawVertex * verts = guiModel->AllocTris( 4, quadPicIndexes, 6, material, currentGLState, STEREO_DEPTH_TYPE_NONE );
	if ( verts == NULL ) {
		return;
	}

	ALIGNTYPE16 anDrawVertex localVerts[4];

	localVerts[0].Clear();
	localVerts[0].xyz[0] = topLeft.x;
	localVerts[0].xyz[1] = topLeft.y;
	localVerts[0].SetTexCoord( topLeft.z, topLeft.w );
	localVerts[0].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[0].ClearColor2();

	localVerts[1].Clear();
	localVerts[1].xyz[0] = topRight.x;
	localVerts[1].xyz[1] = topRight.y;
	localVerts[1].SetTexCoord( topRight.z, topRight.w );
	localVerts[1].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[1].ClearColor2();

	localVerts[2].Clear();
	localVerts[2].xyz[0] = bottomRight.x;
	localVerts[2].xyz[1] = bottomRight.y;
	localVerts[2].SetTexCoord( bottomRight.z, bottomRight.w );
	localVerts[2].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[2].ClearColor2();

	localVerts[3].Clear();
	localVerts[3].xyz[0] = bottomLeft.x;
	localVerts[3].xyz[1] = bottomLeft.y;
	localVerts[3].SetTexCoord( bottomLeft.z, bottomLeft.w );
	localVerts[3].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[3].ClearColor2();

	WriteDrawVerts16( verts, localVerts, 4 );
}

/*
=============
idRenderSystemLocal::DrawStretchTri
=============
*/
void anRenderSystemLocal::DrawStretchTri( const anVec2 & p1, const anVec2 & p2, const anVec2 & p3, const anVec2 & t1, const anVec2 & t2, const anVec2 & t3, const anMaterial *material ) {
	if ( !R_IsInitialized() && material == NULL ) {
		return;
	}

	triIndex_t tempIndexes[3] = { 1, 0, 2 };

	anDrawVertex * verts = guiModel->AllocTris( 3, tempIndexes, 3, material, currentGLState, STEREO_DEPTH_TYPE_NONE );
	if ( verts == NULL ) {
		return;
	}

	ALIGNTYPE16 anDrawVertex localVerts[3];

	localVerts[0].Clear();
	localVerts[0].xyz[0] = p1.x;
	localVerts[0].xyz[1] = p1.y;
	localVerts[0].SetTexCoord( t1 );
	localVerts[0].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[0].ClearColor2();

	localVerts[1].Clear();
	localVerts[1].xyz[0] = p2.x;
	localVerts[1].xyz[1] = p2.y;
	localVerts[1].SetTexCoord( t2 );
	localVerts[1].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[1].ClearColor2();

	localVerts[2].Clear();
	localVerts[2].xyz[0] = p3.x;
	localVerts[2].xyz[1] = p3.y;
	localVerts[2].SetTexCoord( t3 );
	localVerts[2].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[2].ClearColor2();

	WriteDrawVerts16( verts, localVerts, 3 );
}

/*
=============
idRenderSystemLocal::AllocTris
=============
*/
anDrawVertex *anRenderSystemLocal::AllocTris( int numVerts, const triIndex_t * indexes, int numIndexes, const anMaterial * material, const stereoDepthType_t stereoType ) {
	return guiModel->AllocTris( numVerts, indexes, numIndexes, material, currentGLState, stereoType );
}

/*
=====================
anRenderSystemLocal::DrawSmallChar

small chars are drawn at native screen resolution
=====================
*/
void anRenderSystemLocal::DrawSmallChar( int x, int y, int ch, const anMaterial *material ) {
	int row, col;
	float frow, fcol;
	float size;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	if ( y < -SMALLCHAR_HEIGHT ) {
		return;
	}

	row = ch >> 4;
	col = ch & 15;

	frow = row * 0.0625f;
	fcol = col * 0.0625f;
	size = 0.0625f;

	DrawStretchPic( x, y, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT,
					   fcol, frow, fcol + size, frow + size, material );
}

/*
==================
anRenderSystemLocal::DrawSmallString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void anRenderSystemLocal::DrawSmallStringExt( int x, int y, const char *string, const anVec4 &setColor, bool forceColor, const anMaterial *material ) {
	//const anMaterial	*mtlOveride = NULL;
	// draw the colored text
	const unsigned char *s = ( const unsigned char* )string;
	int xx = x;

	SetColor( setColor );

	while ( *s ) {
		if ( anString::IsColor( ( const char* )s ) ) {
			if ( !forceColor ) {
				if ( *( s+1 ) == C_COLOR_DEFAULT ) {
					SetColor( setColor );
				} else {
					color = anString::ColorForIndex( *( s+1 ) );
					anVec4 color[3] = setColor[3];
					SetColor( color );
				}
			}

			s += 2;
			continue;
		}

		DrawSmallChar( xx, y, *s, material );

		xx += SMALLCHAR_WIDTH;
		s++;
	}

	SetColor( colorWhite );
}

/*
=====================
anRenderSystemLocal::DrawBigChar
=====================
*/
void anRenderSystemLocal::DrawBigChar( int x, int y, int ch, const anMaterial *material ) {
	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	if ( y < -BIGCHAR_HEIGHT ) {
		return;
	}

	int row = ch >> 4;
	int col = ch & 15;

	float frow = row * 0.0625f;
	float fcol = col * 0.0625f;
	float size = 0.0625f;

	DrawStretchPic( x, y, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, fcol, frow, fcol + size, frow + size, material );
}

/*
==================
anRenderSystemLocal::DrawBigString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void anRenderSystemLocal::DrawBigStringExt( int x, int y, const char *string, const anVec4 &setColor, bool forceColor, const anMaterial *material ) {
	// draw the colored text
	const char *s = string;
	int xx = x;
	SetColor( setColor );
	while ( *s ) {
		if ( anString::IsColor( s ) ) {
			if ( !forceColor ) {
				if ( *( s+1 ) == C_COLOR_DEFAULT ) {
					SetColor( setColor );
				} else {
					anVec4 color = anString::ColorForIndex( *( s+1 ) );
					color[3] = setColor[3];
					SetColor( color );
				}
			}
			s += 2;
			continue;
		}
		DrawBigChar( xx, y, *s, material );
		xx += BIGCHAR_WIDTH;
		s++;
	}
	SetColor( colorWhite );
}


/*
====================
anRenderSystemLocal::SwapCommandBuffers

Performs final closeout of any gui models being defined.

Waits for the previous GPU rendering to complete and vsync.

Returns the head of the linked command list that was just closed off.

Returns timing information from the previous frame.

After this is called, new command buffers can be built up in parallel
with the rendering of the closed off command buffers by RenderCommandBuffers()
====================
*/
const setBufferCommand_t *anRenderSystemLocal::SwapCommandBuffers( GLuint64 *frontEndMicroSec, GLuint64 *backEndMicroSec, GLuint64 *shadowMicroSec, GLuint64 *gpuMicroSec ) {
	RB_LogComment( "SwapCommandBuffers" );

	if ( gpuMicroSec != NULL ) {
		*gpuMicroSec = 0;		// until shown otherwise
	}

	if ( !R_IsInitialized() ) {
		return nullptr;
	}


	// After coming back from an autoswap, we won't have anything to render
	if ( frameData->cmdHead->next != NULL ) {
		// wait for our fence to hit, which means the swap has actually happened
		// We must do this before clearing any resources the GPU may be using
		void GL_BlockingSwapBuffers();
		GL_BlockingSwapBuffers();
	}

	// read back the start and end timer queries from the previous frame
	if ( qglConfig.timerQueryAvailable ) {
		GLuint drawingTimeNanoseconds = 0;
		if ( tr.timerQueryId != 0 ) {
			qglGetQueryObjectui64vEXT( tr.timerQueryId, GL_QUERY_RESULT, &drawingTimeNanoseconds );
		}
		if ( gpuMicroSec != NULL ) {
			*gpuMicroSec = drawingTimeNanoseconds / 1000;
		}
	}

	//------------------------------

	// save out timing information
	if ( frontEndMicroSec != NULL && backEndMicroSec != NULL && shadowMicroSec != NULL ) {
		*frontEndMicroSec = pc.frontEndMicroSec;
		*backEndMicroSec = backEnd.pc.totalMicroSec;
		*shadowMicroSec = backEnd.pc.shadowMicroSec;
	}

	// print any other statistics and clear all of them
	R_PerformanceCounters();

	// check for dynamic changes that require some initialization
	R_CheckCvars();

    // check for errors
	GL_CheckErrors();
	return nullptr
}
/*
==================
SetBackEndRenderer

Check for changes in the back end renderSystem, possibly invalidating cached data
==================
*/
void anRenderSystemLocal::SetBackEndRenderer() {
	if ( !r_renderer.IsModified() ) {
		return;
	}

	bool oldVPstate = beRenderVertProgs;

	backEndRenderer = BE_BAD;

	if ( anString::Icmp( r_renderer.GetString(), "arb" ) == 0 ) {
		backEndRenderer = BE_ARB;
	} else if ( anString::Icmp( r_renderer.GetString(), "arb2" ) == 0 ) {
		if ( qglConfig.ARB2Path ) {
			backEndRenderer = BE_ARB2;
		}
	} else if ( anString::Icmp( r_renderer.GetString(), "nv10" ) == 0 ) {
		if ( qglConfig.NV10Path ) {
			backEndRenderer = BE_NV10;
		}
	} else if ( anString::Icmp( r_renderer.GetString(), "nv20" ) == 0 ) {
		if ( qglConfig.NV20Path ) {
			backEndRenderer = BE_NV20;
		}
	} else if ( anString::Icmp( r_renderer.GetString(), "r200" ) == 0 ) {
		if ( qglConfig.R200Path ) {
			backEndRenderer = BE_R200;
		}
	} else if ( anString::Icmp( r_renderer.GetString(), "GL_4_6" ) == 0 ) {
		if ( qglConfig.ARB_GLARBpath ) {
			backEndRenderer = BE_GLARB;
		}
	}

	// fallback
	if ( backEndRenderer == BE_BAD ) {
		// choose the best
		if ( qglConfig.ARB2Path ) {
			backEndRenderer = BE_ARB2;
		} else if ( qglConfig.R200Path ) {
			backEndRenderer = BE_R200;
		} else if ( qglConfig.NV20Path ) {
			backEndRenderer = BE_NV20;
		} else if ( qglConfig.NV10Path ) {
			backEndRenderer = BE_NV10;
		} else {
			// the others are considered experimental
			backEndRenderer = BE_ARB;
		}
	}

	beRenderVertProgs = false;
	beRenderMaxLight = 1.0;

	switch( backEndRenderer ) {
	case BE_GLARB:
		common->Printf( "using GL 4.6 renderSystem\n" );
		break;
	case BE_ARB:
		common->Printf( "using ARB renderSystem\n" );
		break;
	case BE_NV10:
		common->Printf( "using NV10 renderSystem\n" );
		break;
	case BE_NV20:
		common->Printf( "using NV20 renderSystem\n" );
		beRenderVertProgs = true;
		break;
	case BE_R200:
		common->Printf( "using R200 renderSystem\n" );
		beRenderVertProgs = true;
		break;
	case BE_ARB2:
		common->Printf( "using ARB2 renderSystem\n" );
		beRenderVertProgs = true;
		beRenderMaxLight = 999;
		break;
	default:
		common->FatalError( "SetbackEndRenderer: bad back end" );
	}

	// clear the vertex cache if we are changing between
	// using vertex programs and not, because specular and
	// shadows will be different data
	if ( oldVPstate != beRenderVertProgs ) {
		vertexCache.PurgeAll();
		if ( primaryWorld ) {
			primaryWorld->FreeInteractions();
		}
	}

	r_renderer.ClearModified();
}

/*
====================
BeginFrame
====================
*/
void anRenderSystemLocal::BeginFrame( int winWidth, int winHeight ) {
	setBufferCommand_t	*cmd;

	if ( !qglConfig.isInitialized ) {
		return;
	}

	// determine which back end we will use
	SetBackEndRenderer();

	guiModel->Clear();

	// for the larger-than-window tiled rendering screenshots
	if ( tiledViewport[0] ) {
		winWidth = tiledViewport[0];
		winHeight = tiledViewport[1];
	}

	qglConfig.vidWidth = winWidth;
	qglConfig.vidHeight = winHeight;

	renderCrops[0].x = 0;
	renderCrops[0].y = 0;
	renderCrops[0].width = winWidth;
	renderCrops[0].height = winHeight;
	currentRenderCrop = 0;

	// screenFraction is just for quickly testing fill rate limitations
	if ( r_screenFraction.GetInteger() != 100 ) {
		int	w = SCREEN_WIDTH * r_screenFraction.GetInteger() / 100.0f;
		int h = SCREEN_HEIGHT * r_screenFraction.GetInteger() / 100.0f;
		CropRenderSize( w, h );
	}
	// this is the ONLY place this is modified
	frameCount++;

	// just in case we did a common->Error while this
	// was set
	guiRecursionLevel = 0;

	// the first rendering will be used for commands like
	// screenshot, rather than a possible subsequent remote
	// or mirror render
	// primaryWorld = NULL;

	// set the time for shader effects in 2D rendering
	frameShaderTime = eventLoop->Milliseconds() * 0.001;

	//
	// draw buffer stuff
	//
	cmd = (setBufferCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	cmd->commandId = RC_SET_BUFFER;
	cmd->frameCount = frameCount;

	if ( r_frontBuffer.GetBool() ) {
		cmd->buffer = ( int )GL_FRONT;
	} else {
		cmd->buffer = ( int )GL_BACK;
	}
}
void anRenderSystemLocal::RenderLightFrustum( const struct renderLight_s &renderLight, anPlane lightFrustum[6] ) {
	R_RenderLightFrustum( renderLight, lightFrustum );
}

void anRenderSystemLocal::LightProjectionMatrix( const anVec3 &origin, const anPlane &rearPlane, anVec4 mat[4] ) {
	LightProjectionMatrix();
}


void anRenderSystemLocal::ToggleSmpFrame( void ) {
	// note that this does not necessarily mean we're going to be finishing
	// a render job in parallel
	//
	// DO NOT turn on the light shader puzzle in the example!  The default
	//
	// GL_PROGRAM_ERROR_CHECK macro will cause a GL_INVALID_OPERATION error
	//
	guiRecursionLevel--;
	if ( r_skipGuiShaders.GetBool() ) {
		return;
	}
	if ( guiSmpToggle ) {
		if ( frameCount.GetValue() < oldFrameCount ) {
			guiSmpToggle = false;
		}
	}
}

/*
=============
EndFrame

This function ends the current frame of the render engine and performs various tasks such as saving timing information,
clearing statistics, checking for dynamic changes, checking for errors, adding a swapbuffers command, starting the back end again,
toggling the frame for SMP rendering, and releasing vertexes used in the frame.

Parameters:
- frontEndMsec: a pointer to an integer to store the number of milliseconds spent in the front end (optional)
- backEndMsec: a pointer to an integer to store the number of milliseconds spent in the back end (optional)
- numVerts: a pointer to an integer to store the number of vertices (optional)
- numIndexes: a pointer to an integer to store the number of indexes (optional)
=============
*/
void anRenderSystemLocal::EndFrame( int *frontEndMsec, int *backEndMsec, int *numVerts, int *numIndexes ) {
    if ( !qglConfig.isInitialized || !qglDrawBuffer( setBufferCommand_t *cmd ) ) {
        return R_IsInitialized();
	} else {
		if ( !qglConfig.isInitialized == NULL ) {
			return;
		}
    }

	// After coming back from an autoswap, we won't have anything to render
	if ( frameData->cmdHead->next != NULL ) {
		// wait for our fence to hit, which means the swap has actually happened
		// We must do this before clearing any resources the GPU may be using
		void GL_BlockingSwapBuffers();
		GL_BlockingSwapBuffers();
	}

    // Close any GUI drawing
    guiModel->EmitFullScreen();
    guiModel->Clear();
	if ( numVerts != NULL ) {
		// *numVerts = pc.frontEndMicroSec;
	}

	//if ( gpuMicroSec != NULL ) {
		// *gpuMicroSec = 0;		// until shown otherwise
	//}

    //
	// The function takes in several optional parameters to store
	// the number of milliseconds spent in the front end saving the timing information, the
	// back end the number of vertices, and the number of indexes.
	if ( frontEndMsec != NULL ) {
		*frontEndMsec = pc.frontEndMsec;
	}
	if ( backEndMsec != NULL ) {
		*backEndMsec = backEnd.pc.mSec;
	}

    // Print any other statistics and clear all of them
    R_PerformanceCounters();

    // Check for dynamic changes that require some initialization
    R_CheckCvars();

    // Check for errors
    GL_CheckErrors();

    // Add the swapbuffers command
    setBufferCommand_t *cmd = (setBufferCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
    cmd->commandId = RC_SWAP_BUFFERS;

    // Start the back end up again with the new command list
    RenderCommandBuffers();

    // Use the other buffers next frame, because another CPU
    // may still be rendering into the current buffers
    R_ToggleSmpFrame();
	//if ( !Sys_LowPhysicalMemory() ) {}}

    // We can now release the vertices used this frame
    vertexCache.EndFrame();
}

/*
=====================
RenderViewToViewport

Converts from SCREEN_WIDTH / SCREEN_HEIGHT coordinates to current cropped pixel coordinates
=====================
*/
void anRenderSystemLocal::RenderViewToViewport( const renderView_t *renderView, anScreenRect *viewport ) {
	renderCrop_t *rc = &renderCrops[currentRenderCrop];

	float wRatio = ( float )rc->width / SCREEN_WIDTH;
	float hRatio = ( float )rc->height / SCREEN_HEIGHT;

	viewport->x1 = anMath::Ftoi( rc->x + renderView->x * wRatio );
	viewport->x2 = anMath::Ftoi( rc->x + floor( ( renderView->x + renderView->width ) * wRatio + 0.5f ) - 1 );
	viewport->y1 = anMath::Ftoi( ( rc->y + rc->height ) - floor( ( renderView->y + renderView->height ) * hRatio + 0.5f ) );
	viewport->y2 = anMath::Ftoi( ( rc->y + rc->height ) - floor( renderView->y * hRatio + 0.5f ) - 1 );
}

static int RoundDownToPowerOfTwo( int v ) {
	for ( int i = 0; i < 20; i++ ) {
		if ( ( 1 << i ) == v ) {
			return v;
		}
		if ( ( 1 << i ) > v ) {
			return 1 << ( i-1 );
		}
	}
	return 1<<i;
}

/*
================
CropRenderSize

This automatically halves sizes until it fits in the current window size,
so if you specify a power of two size for a texture copy, it may be shrunk
down, but still valid.
================
*/
void anRenderSystemLocal::CropRenderSize( int width, int height, bool makePowerOfTwo, bool forceDimensions ) {
	if ( !qglConfig.isInitialized ) {
		return;
	}

	// close any gui drawing before changing the size
	guiModel->EmitFullScreen();
	guiModel->Clear();

	if ( width < 1 || height < 1 ) {
		common->Error( "CropRenderSize: bad sizes" );
	}

	// convert from virtual SCREEN_WIDTH/SCREEN_HEIGHT coordinates to physical OpenGL pixels
	renderView_t renderView;
	renderView.x = 0;
	renderView.y = 0;
	renderView.width = width;
	renderView.height = height;

	anScreenRect r;
	RenderViewToViewport( &renderView, &r );

	width = r.x2 - r.x1 + 1;
	height = r.y2 - r.y1 + 1;

	if ( forceDimensions ) {
		// just give exactly what we ask for
		width = renderView.width;
		height = renderView.height;
	}

	// if makePowerOfTwo, drop to next lower power of two after scaling to physical pixels
	if ( makePowerOfTwo ) {
		width = RoundDownToPowerOfTwo( width );
		height = RoundDownToPowerOfTwo( height );
		// FIXME: megascreenshots with offset viewports don't work right with this yet
	}

	renderCrop_t	*rc = &renderCrops[currentRenderCrop];

	// we might want to clip these to the crop window instead
	while ( width > qglConfig.vidWidth ) {
		width >>= 1;
	}
	while ( height > qglConfig.vidHeight ) {
		height >>= 1;
	}

	if ( currentRenderCrop == MAX_RENDER_CROPS ) {
		common->Error( "anRenderSystemLocal::CropRenderSize: currentRenderCrop == MAX_RENDER_CROPS" );
	}

	currentRenderCrop++;

	rc = &renderCrops[currentRenderCrop];

	rc->x = 0;
	rc->y = 0;
	rc->width = width;
	rc->height = height;
}

/*
=====================
anRenderSystemLocal::WriteDemoPics
=====================
*/
void anRenderSystemLocal::WriteDemoPics() {
	//common->WriteDemo()->WriteInt( DS_RENDER );
	//common->WriteDemo()->WriteInt( DC_GUI_MODEL );
}

/*
=====================
anRenderSystemLocal::DrawDemoPics
=====================
*/
void anRenderSystemLocal::DrawDemoPics() {
}

/*
=====================
anRenderSystemLocal::GetCroppedViewport

Returns the current cropped pixel coordinates
=====================
*/
void anRenderSystemLocal::GetCroppedViewport( idScreenRect * viewport ) {
	*viewport = renderCrops[currentRenderCrop];
}

/*
================
UnCrop
================
*/
void anRenderSystemLocal::UnCrop() {
	if ( !qglConfig.isInitialized ) {// && IsInitialized() ) {
		return;
	}

	if ( currentRenderCrop < 1 ) {
		common->Error( "anRenderSystemLocal::UnCrop: currentRenderCrop < 1" );
	}

	// close any gui drawing
	guiModel->EmitFullScreen();
	guiModel->Clear();

	currentRenderCrop--;
}

/*
================
CaptureRenderToImage
================
*/
void anRenderSystemLocal::CaptureRenderToImage( const char *imageName ) {
	if ( !qglConfig.isInitialized ) {// && IsInitialized() ) {
		return;
	}
	guiModel->EmitFullScreen();
	guiModel->Clear();

	// look up the image before we create the render command, because it
	// may need to sync to create the image
	anImage	*image = globalImages->ImageFromFile( imageName, TF_DEFAULT, true, TR_REPEAT, TD_DEFAULT );

	renderCrop_t *rc = &renderCrops[currentRenderCrop];

	setBufferCommand_t *cmd = (setBufferCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	cmd->commandId = RC_COPY_RENDER;
	cmd->x = rc->x;
	cmd->y = rc->y;
	cmd->imageWidth = rc->width;
	cmd->imageHeight = rc->height;
	cmd->image = image;

	guiModel->Clear();
}

/*
==============
CaptureRenderToFile
==============
*/
void anRenderSystemLocal::CaptureRenderToFile( const char *fileName, bool fixAlpha ) {
	if ( !qglConfig.isInitialized ) {// && IsInitialized() ) {
		return;
	}

	renderCrop_t *rc = &renderCrops[currentRenderCrop];

	guiModel->EmitFullScreen();
	guiModel->Clear();
	RenderCommandBuffers();

	qglReadBuffer( GL_BACK );

	// include extra space for OpenGL padding to word boundaries
	int	c = ( rc->width + 3 ) * rc->height;
	byte *data = ( byte * )R_StaticAlloc( c * 3 );

	qglReadPixels( rc->x, rc->y, rc->width, rc->height, GL_RGB, GL_UNSIGNED_BYTE, data );

	/*byte *data2 = ( byte * )R_StaticAlloc( c * 4 );

	for ( int i = 0; i < c; i++ ) {
		data2[ i * 4 ] = data[ i * 3 ];
		data2[ i * 4 + 1 ] = data[ i * 3 + 1 ];
		data2[ i * 4 + 2 ] = data[ i * 3 + 2 ];
		data2[ i * 4 + 3 ] = 0xff;
	}*/
	byte *pData = data;
	int dataStride = ( rc->width * 3 + 3 ) & ~3; // take into account the OpenGL padding to dword boundaries; see R_ReadTiledPixels
	int dataRowSize = rc->width * 3; // row size without padding
	int dataRowSkip = dataStride - dataRowSize;

	byte *data2 = ( byte * )R_StaticAlloc( rc->width * rc->height * 4 );
	byte *pData2 = data2;
	byte *pData2PastLast = data2 + ( rc->width * rc->height * 4 );

	while ( pData2 < pData2PastLast ) {
		byte *pDataRowPastLast = pData + dataRowSize; // past last non-padded element of the current row of "data"
		while ( pData < pDataRowPastLast ) {
			*pData2++ = *pData++;
			*pData2++ = *pData++;
			*pData2++ = *pData++;
			*pData2++ = 0xff;
		}
		pData += dataRowSkip; // go to the next row by skipping the padded elements of this row
	}

	R_WriteTGA( fileName, data2, rc->width, rc->height, true );

	R_StaticFree( data );
	R_StaticFree( data2 );
}

/*
==============
AllocRenderWorld
==============
*/
anRenderWorld *anRenderSystemLocal::AllocRenderWorld() {
	anRenderWorldLocal *rw;
	rw = new anRenderWorldLocal;
	worlds.Append( rw );
	return rw;
}

/*
==============
FreeRenderWorld
==============
*/
void anRenderSystemLocal::FreeRenderWorld( anRenderWorld *rw ) {
	if ( primaryWorld == rw ) {
		primaryWorld = NULL;
	}
	worlds.Remove( static_cast<anRenderWorldLocal *>( rw ) );
	delete rw;
}

/*
==============
PrintMemInfo
==============
*/
void anRenderSystemLocal::PrintMemInfo( MemInfo_t *mi ) {
	// sum up image totals
	globalImages->PrintMemInfo( mi );

	// sum up model totals
	renderModelManager->PrintMemInfo( mi );
	// compute render totals
}

/*
===============
anRenderSystemLocal::UploadImage
===============
*/
bool anRenderSystemLocal::UploadImage( const char *imageName, const byte *data, int width, int height  ) {
	anImage *image = globalImages->GetImage( imageName );
	if ( !image ) {
		return false;
	}
	image->UploadScratch( data, width, height );
	image->SetImageFilterAndRepeat();
	return true;
}
