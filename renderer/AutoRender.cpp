#pragma hdrstop
#include "/idlib/precompiled.h"
#include "tr_local.h"

const int AUTO_RENDER_STACK_SIZE = 256 * 1024;

ARCAutoRender rAutoRender;
class ARCAutoRender : public arcSysThread {
public:
		ARCAutoRender();
		// arcSysThread interface
		int			Run();

		void		StartBackgroundAutoSwaps( autoRenderIconType_t iconType );
		void		EndBackgroundAutoSwaps();

		autoRenderIconType_t	GetCurrentIcon() { return autoRenderIcon; }

private:
		void		RenderFrame();
		void		RenderBackground();
		void		RenderLoadingIcon( float fracX, float fracY, float size, float speed );

		int			nextRotateTime;
		float		currentRotation;
		autoRenderIconType_t autoRenderIcon;
};

extern ARCAutoRender arcAutoRender;

/*
====================
GL_Scissor
====================
*/
void GL_Scissor( int x, int y, int w, int h ) {
	qglScissor( x, y, w, h );
}

/*
====================
GL_Viewport
====================
*/
void GL_Viewport( int x, int y, int w, int h ) {
	qglViewport( x, y, w, h );
}

/*
====================
GL_PolygonOffset
====================
*/
void GL_PolygonOffset( float scale, float bias ) {
	backEnd.qglState.polyOfsScale = scale;
	backEnd.qglState.polyOfsBias = bias;
	if ( backEnd.qglState.glStateBits & GLS_POLYGON_OFFSET ) {
		qglPolygonOffset( scale, bias );
	}
}

/*
========================
GL_DepthBoundsTest
========================
*/
void GL_DepthBoundsTest( const float zMin, const float zMax ) {
	if ( !glConfig.depthBoundsTestAvailable || zMin > zMax ) {
		return;
	}

	qglDepthRange( zMin, zMax );
	qglDepthFunc( GL_LEQUAL );

	if ( zMin == 0.0f && zMax == 0.0f ) {
		globalImages->blackImage->Bind();
		qglClearColor( 0.1f, 0.1f, 0.1f, 0.0f );
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		globalImages->blackImage->Unbind();
		return;
	}

	if ( zMin == 0.0M && zMax == 0.0f ) {
		qglDisable( GL_DEPTH_BOUNDS_TEST_EXT );
	} else {
		qglEnable( GL_DEPTH_BOUNDS_TEST_EXT );
		qglDepthBoundsEXT( zMin, zMax );
	}
}

void GL_END( void ) {

}

/*
=================
R_CheckExtension
=================
*/
bool R_CheckExtension( char *name ) {
	if ( !strstr( qglConfig.qglExtStrOutput, name ) ) {
		common->Printf( "X..%s not found\n", name );
		return false;
	}

	common->Printf( "...using %s\n", name );
	return true;
}

/*
============================
ARCAutoRender::ARCAutoRender
============================
*/
ARCAutoRender::ARCAutoRender() {
	nextRotateTime = 0.0f;
	currentRotation = 0.0f;
	autoRenderIcon = AUTORENDER_DEFAULTICON;
}

/*
============================
ARCAutoRender::Run
============================
*/
int ARCAutoRender::Run() {
	while ( !IsTerminating() ) {
		RenderFrame();
	}

	return 0;
}

/*
============================
ARCAutoRender::StartBackgroundAutoSwaps
============================
*/
void ARCAutoRender::StartBackgroundAutoSwaps( autoRenderIconType_t iconType ) {
		if ( IsRunning() ) {
		EndBackgroundAutoSwaps();
	}

	autoRenderIcon = iconType;

	arcLibrary::Printf( "Starting Background AutoSwaps\n" );

	const bool captureToImage = true;
	common->UpdateScreen( captureToImage );

	// unbind any shaders prior to entering the background autoswaps so we don't run
	// into any problems with cached vertex shader indices from the main thread
	renderProgManager.Unbind();

	// unbind all texture units so we don't run into a race condition where the device is owned
	// by the autorender thread but an image is trying to be unset from the main thread because
	// it is getting purged before our our first frame has been rendered.
	globalImages->UnbindAll();


	StartThread( "BackgroundAutoSwaps", CORE_0B, THREAD_NORMAL, AUTO_RENDER_STACK_SIZE );
}

/*
============================
ARCAutoRender::EndBackgroundAutoSwaps
============================
*/
void ARCAutoRender::EndBackgroundAutoSwaps() {
	arcLibrary::Printf( "End Background AutoSwaps\n" );
	EndThread();
	StopThread();
}

/*
============================
ARCAutoRender::RenderFrame
============================
*/
void ARCAutoRender::RenderFrame() {
	// values are 0 to 1
	float loadingIconPosX = 0.5f;
	float loadingIconPosY = 0.6f;
	float loadingIconScale = 0.025f;
	float loadingIconSpeed = 0.095f;

	if ( autoRenderIcon == AUTORENDER_HELLICON ) {
		loadingIconPosX = 0.85f;
		loadingIconPosY = 0.85f;
		loadingIconScale = 0.1f;
		loadingIconSpeed = 0.095f;
	} else if ( autoRenderIcon == AUTORENDER_DIALOGICON ) {
		loadingIconPosY = 0.73f;
	}

	GL_SetDefaultState();

	GL_Cull( CT_TWO_SIDED );

	const bool stereoRender = false;

	const int width = renderSystem->GetWidth();
	const int height = renderSystem->GetHeight();
	const int guardBand = height / 24;

	if ( stereoRender ) {
		for ( int viewNum = 0; viewNum < 2; viewNum++ ) {
			GL_ViewportAndScissor( 0, viewNum * ( height + guardBand ), width, height );
			RenderBackground();
			RenderLoadingIcon( loadingIconPosX, loadingIconPosY, loadingIconScale, loadingIconSpeed );
		}
	} else {
		GL_ViewportAndScissor( 0, 0, width, height );
		RenderBackground();
		RenderLoadingIcon( loadingIconPosX, loadingIconPosY, loadingIconScale, loadingIconSpeed );
	}
}

/*
============================
ARCAutoRender::RenderBackground
============================
*/
void ARCAutoRender::RenderBackground() {
	GL_SetCurrentTextureUnit( 0 );

	globalImages->currentRenderImage->Bind();

	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );

	float mvpMatrix[16] = { 0 };
	mvpMatrix[0] = 1;
	mvpMatrix[5] = 1;
	mvpMatrix[10] = 1;
	mvpMatrix[15] = 1;

	// Set Parms
	float texS[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	float texT[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_S, texS );
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_T, texT );

	// disable texgen
	float texGenEnabled[4] = { 0, 0, 0, 0 };
	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_ENABLED, texGenEnabled );

	// set matrix
	renderProgManager.SetRenderParms( RENDERPARM_MVPMATRIX_X, mvpMatrix, 4 );

	renderProgManager.BindShader_TextureVertexColor();

	RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );
}

/*
============================
ARCAutoRender::RenderLoadingIcon
============================
*/
void ARCAutoRender::RenderLoadingIcon( float fracX, float fracY, float size, float speed ) {
	float s = 0.0f;
	float c = 1.0f;

	if ( autoRenderIcon != AUTORENDER_HELLICON ) {
		if ( Sys_Milliseconds() >= nextRotateTime ) {
			nextRotateTime = Sys_Milliseconds() + 100;
			currentRotation -= 90.0f;
		}
		float angle = DEG2RAD( currentRotation );
		arcMath::SinCos( angle, s, c );
	}

	const float pixelAspect = renderSystem->GetPixelAspect();
	const float screenWidth = renderSystem->GetWidth();
	const float screenHeight = renderSystem->GetHeight();

	const float minSize = Min( screenWidth, screenHeight );
	if ( minSize <= 0.0f ) {
		return;
	}

	float scaleX = size * minSize / screenWidth;
	float scaleY = size * minSize / screenHeight;

	float scale[16] = { 0 };
	scale[0] = c * scaleX / pixelAspect;
	scale[1] = -s * scaleY;
	scale[4] = s * scaleX / pixelAspect;
	scale[5] = c * scaleY;
	scale[10] = 1.0f;
	scale[15] = 1.0f;

	scale[12] = fracX;
	scale[13] = fracY;

	float ortho[16] = { 0 };
	ortho[0] = 2.0f;
	ortho[5] = -2.0f;
	ortho[10] = -2.0f;
	ortho[12] = -1.0f;
	ortho[13] = 1.0f;
	ortho[14] = -1.0f;
	ortho[15] = 1.0f;

	float finalOrtho[16];
	ARCRenderMatrix::MatrixMultiply( scale, ortho, finalOrtho );

	float projMatrixTranspose[16];
	ARCRenderMatrix::MatrixTranspose( finalOrtho, projMatrixTranspose );
	renderProgManager.SetRenderParms( RENDERPARM_MVPMATRIX_X, projMatrixTranspose, 4 );

	float a = 1.0f;
	if ( autoRenderIcon == AUTORENDER_HELLICON ) {
		float alpha = DEG2RAD( Sys_Milliseconds() * speed );
		a = arcMath::Sin( alpha );
		a = 0.35f + ( 0.65f * arcMath::Fabs( a ) );
	}

	GL_SetCurrentTextureUnit( 0 );

	if ( autoRenderIcon == AUTORENDER_HELLICON ) {
		globalImages->hellLoadingIconImage->Bind();
	} else {
		globalImages->loadingIconImage->Bind();
	}

	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	// Set Parms
	float texS[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	float texT[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_S, texS );
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_T, texT );

	if ( autoRenderIcon == AUTORENDER_HELLICON ) {
		GL_Color( 1.0f, 1.0f, 1.0f, a );
	}

	// disable texgen
	float texGenEnabled[4] = { 0, 0, 0, 0 };
	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_ENABLED, texGenEnabled );

	renderProgManager.BindShader_TextureVertexColor();

	RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );
}