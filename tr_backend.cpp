#include "GLIncludes/qgl.h"
#include <glcore.h>
#include <glad.h>
#include "tr_local.h"
#include "RenderSystem.h"
#include "RenderWorld.h"
#include "/framework/Common_local.h"
#include "/framework/CVarSystem.h"
#pragma hdrstop

 frameData_t	*frameData;
 backEndState_t	backEnd;

anCVarSystem r_drawFlickerBox( "r_drawFlickerBox", "0", CVAR_RENDERER | CVAR_BOOL, "visual test for dropping frames" );
anCVarSystem r_showSwapBuffers( "r_showSwapBuffers", "0", CVAR_BOOL, "Show timings from GL_BlockingSwapBuffers" );
anCVarSystem r_syncEveryFrame( "r_syncEveryFrame", "1", CVAR_BOOL, "Don't let the GPU buffer execution past swapbuffers" );

typedef struct __GLsync *GLsync;
static int		swapIndex;		// 0 or 1 into renderSync
static GLsync	renderSync[2];

void GLimp_SwapBuffers();

void RB_SurfaceAxis( void ) {
	GL_Bind( tr.whiteImage );
	qglLineWidth( 3 );
	qglBegin( GL_LINES );
	qglColor3f( 1, 0, 0 );
	qglVertex3f( 0, 0, 0 );
	qglVertex3f( 16, 0, 0 );
	qglColor3f( 0, 1, 0 );
	qglVertex3f( 0, 0, 0 );
	qglVertex3f( 0, 16, 0 );
	qglColor3f( 0, 0, 1 );
	qglVertex3f( 0, 0, 0 );
	qglVertex3f( 0, 0, 16 );
	qglEnd();
	qglLineWidth( 1 );
}

/*
================
RB_SetVertexColorParms
================
*/
static const float zero[4] = { 0, 0, 0, 0 };
static const float one[4] = { 1, 1, 1, 1 };
static const float negOne[4] = { -1, -1, -1, -1 };
static void RB_SetVertexColorParms( stageVertexColor_t svc ) {
	switch ( svc ) {
		case SVC_IGNORE:
			SetVertexParm( RENDERPARM_VERTEXCOLOR_MODULATE, zero );
			SetVertexParm( RENDERPARM_VERTEXCOLOR_ADD, one );
			break;
		case SVC_MODULATE:
			SetVertexParm( RENDERPARM_VERTEXCOLOR_MODULATE, one );
			SetVertexParm( RENDERPARM_VERTEXCOLOR_ADD, zero );
			break;
		case SVC_INVERSE_MODULATE:
			SetVertexParm( RENDERPARM_VERTEXCOLOR_MODULATE, negOne );
			SetVertexParm( RENDERPARM_VERTEXCOLOR_ADD, one );
			break;
	}
}

/*
===============
GL_TextureAnisotropy

clean this up along with everything else in the damn world in here
grabbed wrong version code instead of vanilla doom 3 now the mess it was already is the mess it is now nice uneven and not
steady. lol =( this is a mess and needs updating.
===============
*/
void GL_TextureAnisotropy( float aniso ) {
	anImage *glt;

	if ( r_maxAnisotropicFiltering.GetInteger() == 1 ) {
		if ( aniso < 1.0 || aniso > qglConfig.maxTextureAnisotropy ) {
			RB_LogComment( "anisotropy out of range\n" );
			return;
		}
	}

	globalImages->textureAnisotropy = aniso;
	if ( !qglConfig.useAnisotropyFilter ) {
		return;
	}

	// change all the existing texture objects
	for ( int i = 0 ; i < tr.numImages ; i++ ) {
		glt = tr.images[ i ];
		GL_Bind( glt );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso );
	}
}

/*
===============
R_DeleteTextures
===============
*/
void R_DeleteTextures( void ) {
	for ( int i = 0; i < tr.numImages ; i++ ) {
		qglDeleteTextures( 1, &tr.images->texnum );
	}
	memset( tr.images, 0, sizeof( tr.images ) );
	// Ridah
	//%	R_InitTexnumImages(qtrue);
	// done.

	memset( qglState.currenttmu, 0, sizeof( qglState.currenttmu ) );
	if ( qglBindTexture ) {
		if ( qglActiveTextureARB ) {
			GL_SetCurrentTextureUnit( 1 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
			GL_SetCurrentTextureUnit( 0 );
			qglBindTexture( GL_TEXTURE_2D, 0 );
		} else {
			qglBindTexture( GL_TEXTURE_2D, 0 );
		}
	}
}

/*
======================
RB_SetDefaultGLState

This should initialize all GL state(s) that any part of the entire program
may touch, including the editor.
======================
*/
void RB_SetDefaultGLState( void ) {
	RB_LogComment( "--- R_SetDefaultGLState ---\n" );

	qglClearDepth( 1.0f );
	qglColor4f( 1, 1, 1, 1 );

	// the vertex array is always enabled
	qglEnableClientState( GL_VERTEX_ARRAY );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	qglDisableClientState( GL_COLOR_ARRAY );

	//
	// make sure our GL state vector is set correctly
	//
	memset( &backEnd.qglState, 0, sizeof( backEnd.qglState ) );
	backEnd.qglState.forceGlState = true;

	qglColorMask( 1, 1, 1, 1 );

	qglEnable( GL_DEPTH_TEST );
	qglEnable( GL_BLEND );
	qglEnable( GL_SCISSOR_TEST );
	qglEnable( GL_CULL_FACE );
	qglDisable( GL_LIGHTING );
	qglDisable( GL_LINE_STIPPLE );
	qglDisable( GL_STENCIL_TEST );

	qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	qglDepthMask( GL_TRUE );
	qglDepthFunc( GL_ALWAYS );

	qglCullFace( GL_FRONT_AND_BACK );
	qglShadeModel( GL_SMOOTH );

	if ( r_useScissor.GetBool() ) {
		qglScissor( 0, 0, qglConfig.vidWidth, qglConfig.vidHeight );
	}

	for ( int i = qglConfig.maxImageUnits - 1; i >= 0; i-- ) {
		GL_SetCurrentTextureUnit( i );

		// object linear texgen is our default
		qglTexGenf( GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
		qglTexGenf( GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
		qglTexGenf( GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
		qglTexGenf( GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );

		GL_TexEnv( GL_MODULATE );
		qglDisable( GL_TEXTURE_2D );

		if ( qglConfig.3DImagesActive ) {
			qglDisable( GL_TEXTURE_3D );
		}

		if ( qglConfig.useCubeMap ) {
			qglDisable( GL_TEXTURE_CUBE_MAP_EXT );
		}
	}
}

/*
========================
GL_SetDefaultState

This should initialize all GL state that any part of the entire program
may touch, including the editor.
========================
*/
void GL_SetDefaultState() {
	RENDERLOG_PRINTF( "--- GL_SetDefaultState ---\n" );

	qglClearDepth( 1.0f );

	// make sure our GL state vector is set correctly
	memset( &backEnd.qglState, 0, sizeof( backEnd.qglState ) );
	GL_State( 0, true );

	// These are changed by GL_Cull
	qglCullFace( GL_FRONT_AND_BACK );
	qglEnable( GL_CULL_FACE );

	// These are changed by GL_State
	qglColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
	qglBlendFunc( GL_ONE, GL_ZERO );
	qglDepthMask( GL_TRUE );
	qglDepthFunc( GL_LESS );
	qglDisable( GL_STENCIL_TEST );
	qglDisable( GL_POLYGON_OFFSET_FILL );
	qglDisable( GL_POLYGON_OFFSET_LINE );
	qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	// These should never be changed
	qglShadeModel( GL_SMOOTH );
	qglEnable( GL_DEPTH_TEST );
	qglEnable( GL_BLEND );
	qglEnable( GL_SCISSOR_TEST );
	qglDrawBuffer( GL_BACK );
	qglReadBuffer( GL_BACK );

	if ( r_useScissor.GetBool() ) {
		qglScissor( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
	}
}

/*
========================
GL_GetCurrentStateMinusStencil
========================
*/
uint64 GL_GetCurrentStateMinusStencil() {
	return GL_GetCurrentState() & ~( GLS_STENCIL_OP_BITS|GLS_STENCIL_FUNC_BITS|GLS_STENCIL_FUNC_REF_BITS|GLS_STENCIL_FUNC_MASK_BITS );
}

/*
====================
RB_LogComment
====================
*/
void RB_LogComment( const char *cmt, ... ) {
	// if we don't have a log file, don't do anything
	if ( !tr.logFile ) {
		return;
	}
    // Write the comment to the log file
	fprintf( tr.logFile, "// " );
	// Get the variable arguments
	va_start( va_list marker, cmt );
	// Write the formatted comment to the log file
	vfprintf( tr.logFile, cmt, marker );
	va_end( va_list marker ); // End the variable argument
}

// we want this directly to the console/terminal instead of logfile.
void RenderPrintf( const char *comment, ... ) {
	fprintf( tr.logFile, "// " );
	va_start( va_list marker, fmt );
	vfprintf(, const char *__restrict format, __va_list_tag *arg )
	va_end( va_list marker )
}

//=============================================================================
void GLDSA_BindNullTextures( void ) {
	if ( backEnd.qglRefConfig.directStateAccess ) {
		for ( int i = 0; i < NUM_TEXTURE_BUNDLES; i++ ) {
			qglBindMultiTextureEXT( GL_TEXTURE0 + i, GL_TEXTURE_2D, 0 );
			backEnd.qglState.textures[i] = 0;
		}
	} else {
		for ( int i = 0; i < NUM_TEXTURE_BUNDLES; i++ ) {
			qglActiveTexture( GL_TEXTURE0 + i );
			qglBindTexture( GL_TEXTURE_2D, 0 );
			backEnd.qglState.textures[i] = 0;
		}

		qglActiveTexture( GL_TEXTURE0 );
		backEnd.qglState.texUnit = GL_TEXTURE0;
	}
}

int GLDSA_BindMultiTexture( GLenum texUnit, GLenum target, GLuint texture ) {
	GLuint tmu = texUnit - GL_TEXTURE0;

	if ( backEnd.qglState.textures[tmu] == texture ) {
		return 0;
	}

	if ( target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z ) {
		target = GL_TEXTURE_CUBE_MAP;
	}
	qglBindMultiTextureEXT( texUnit, target, texture );
	backEnd.qglState.textures[tmu] = texture;
	return 1;
}

GLvoid APIENTRY GLDSA_BindMultiTextureEXT( GLenum texUnit, GLenum target, GLuint texture ) {
	if ( backEnd.qglState.texUnit != texUnit ) {
		qglActiveTexture( texUnit );
		backEnd.qglState.texUnit = texUnit;
	}

	qglBindTexture( target, texture );
}
void GLDSA_BindNullFramebuffers( void ) {
	qglBindFramebuffer( GL_FRAMEBUFFER, 0 );
	backEnd.qglState.drawFramebuffer = backEnd.qglState.readFramebuffer = 0;
	qglBindRenderbuffer( GL_RENDERBUFFER, 0 );
	backEnd.qglState.renderbuffer = 0;
}

void GLDSA_BindFramebuffer( GLenum target, GLuint framebuffer ) {
	switch ( target ) {
		case GL_FRAMEBUFFER:
			if ( framebuffer != backEnd.qglState.drawFramebuffer || framebuffer != backEnd.qglState.readFramebuffer ) {
				qglBindFramebuffer( target, framebuffer );
				qglState.drawFramebuffer = qglState.readFramebuffer = framebuffer;
			}
			break;

		case GL_DRAW_FRAMEBUFFER:
			if ( framebuffer != backEnd.qglState.drawFramebuffer ) {
				qglBindFramebuffer( target, framebuffer );
				backEnd.qglState.drawFramebuffer = framebuffer;
			}
			break;

		case GL_READ_FRAMEBUFFER:
			if ( framebuffer != backEnd.qglState.readFramebuffer ) {
				qglBindFramebuffer( target, framebuffer ) ;
				backEnd.qglState.readFramebuffer = framebuffer;
			}
			break;
	}
}

void GLDSA_BindRenderbuffer( GLuint renderbuffer ) {
	if ( renderbuffer != backEnd.qglState.renderbuffer ) {
		qglBindRenderbuffer( GL_RENDERBUFFER, renderbuffer );
		backEnd.qglState.renderbuffer = renderbuffer;
	}
}

void GL_BindNullProgram( void ) {
	qglUseProgram( 0 );
	backEnd.qglState.program = 0;
}

/*
====================
GL_SetCurrentTextureUnit

This code defines a function called GL_SetCurrentTextureUnit that takes an integer parameter unit.
It checks if the current texture unit is already set to the specified unit, and if so,
it returns early. Otherwise, it checks if the unit is within a valid range.
If it is not, it logs a warning and returns.

Finally, it sets the active texture unit using the qglActiveTextureARB and qglClientActiveTextureARB
functions and updates the current texture unit in the qglState structure.
====================
*/
void GL_SetCurrentTextureUnit( int unit ) {
	if ( backEnd.qglState.currenttmu == unit ) {
		return;
	}

	if ( unit < 0 || unit >= MAX_TEXTURE_IMAGE_UNITS ) {
		common->Warning( "GL_SetCurrentTextureUnit: unit = %i", unit );
		return;
	}
	if ( backEnd.qglState.currenttmu == unit ) {
		return;
	}
	qglActiveTextureARB( GL_TEXTURE0_ARB + unit );

	RB_LogComment( "qglActiveTextureARB( %i );\n", unit, unit );
}

/*
====================
GL_UseProgram
====================
*/
void GL_UseProgram( shaderProgram_t *program ) {
	if ( backEnd.qglState.program == program ) {
		return;//0;
	}

	RB_LogComment( "Last shader program: %s, %p\n", backEnd.qglState.program ? backEnd.qglState.program->name : "NULL", backEnd.qglState.program );
	RB_LogComment( "Current shader program: %s, %p\n", program ? program->name : "NULL", program );

	qglUseProgram( program ? program->program : 0 );
	backEnd.qglState.program = program;

	GL_CheckErrors();
	//return 1;
}

/*
====================
GL_Uniform1fv
====================
*/
void GL_Uniform1fv( GLint location, const GLfloat *value ) {
	if ( !backEnd.qglState.program ) {
		common->Printf("GL_Uniform1fv: no current program object\n");
		__builtin_trap();
		return;
	}

	qglUniform1fv( *(GLint *)( (char *)backEnd.qglState.program + location ), 1, value );

	GL_CheckErrors();
}

/*
====================
GL_Uniform4fv
====================
*/
void GL_Uniform4fv( GLint location, const GLfloat *value) {
	if ( !backEnd.qglState.program ) {
		common->Printf("GL_Uniform4fv: no current program object\n");
		__builtin_trap();
		return;
	}

	qglUniform4fv(*(GLint *)( (char *)backEnd.qglState.program + location), 1, value);
	GL_CheckErrors();
}

/*
====================
GL_UniformMatrix4fv
====================
*/
void GL_UniformMatrix4fv( GLint location, const GLfloat *value) {
	if ( !backEnd.qglState.program ) {
		common->Printf("GL_Uniform4fv: no current program object\n");
		__builtin_trap();
		return;
	}

	qglUniformMatrix4fv(*(GLint *)( (char *)backEnd.qglState.program + location), 1, GL_FALSE, value);
	GL_CheckErrors();
}

/*
====================
GL_EnableVertexAttribArray
====================
*/
void GL_EnableVertexAttribArray( GLuint index ) {
	if ( !backEnd.qglState.program ) {
		common->Printf( "GL_EnableVertexAttribArray: no current program object\n");
		__builtin_trap();
		return;
	}

	if ( (*(GLint *)( (char *)backEnd.qglState.program + index ) ) == -1 ) {
		common->Printf("GL_EnableVertexAttribArray: unbound attribute index\n" );
#ifdef _HARM_SHADER_NAME
		RB_LogComment( "Current shader program: %s, index: %d\n", backEnd.qglState.program->name, index );
#endif
		__builtin_trap();
		return;
	}

	//RB_LogComment("glEnableVertexAttribArray( %i );\n", index);
	qglEnableVertexAttribArray( *(GLint *)( (char *)backEnd.qglState.program + index ) );
	GL_CheckErrors();
}

/*
====================
GL_DisableVertexAttribArray
====================
*/
void GL_DisableVertexAttribArray( GLuint index ) {
	if ( !backEnd.qglState.program ) {
		common->Printf( "GL_DisableVertexAttribArray: no current program object\n" );
		qglDisableVertexAttribArray( index );
		__builtin_trap();
		return;
	}

	if ( ( *(GLint *)( (char *)backEnd.qglState.program + index ) ) == -1 ) {
		common->Printf( "GL_DisableVertexAttribArray: unbound attribute index\n" );
		RB_LogComment( "Current shader program: %s, index: %d\n", backEnd.qglState.program->name, index );
		__builtin_trap();
		return;
	}

	qglDisableVertexAttribArray( *(GLint *)( (char *)backEnd.qglState.program + index ) );
	GL_CheckErrors();
}

/*
====================
GL_VertexAttribPointer
====================
*/
void GL_VertexAttribPointer( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer ) {
	if ( !backEnd.qglState.program  ) {
		common->Printf( "GL_VertexAttribPointer: no current program object\n" );
		__builtin_trap();
		return;
	}

	if ( ( *(GLint *)( (char *)backEnd.qglState.program + index ) ) == -1 ) {
		common->Printf( "GL_VertexAttribPointer: unbound attribute index\n" );
		__builtin_trap();
		return;
	}

	// RB_LogComment( "qglVertexAttribPointer( %i, %i, %i, %i, %i, %p );\n", index, size, type, normalized, stride, pointer );
	qglVertexAttribPointer( *(GLint *)( (char *)backEnd.qglState.program + index ), size, type, normalized, stride, pointer );
	GL_CheckErrors();
}
/*
====================
GL_Cull

This handles the flipping needed when the view being
rendered is a mirored view.
====================
*/
void GL_Cull( int cullType ) {
	if ( backEnd.qglState.faceCulling == cullType ) {
		if ( cullType == CT_TWO_SIDED ) {
			// Two sided culling disabled
			qglDisable( GL_CULL_FACE );
		}
		return;
	} else {
		// Enable culling
		qglEnable( GL_CULL_FACE );
		if ( cullType == CT_BACK_SIDED ) {
			// Back face culling
			qglCullFace( GL_BACK );
		} else if ( cullType == CT_FRONT_SIDED ) {
			// Front face culling
			qglCullFace( GL_FRONT );
		} else if ( cullType == CT_OCCLUSION_CULL ) {
			// Occlusion culling using query objects
			qglCullFace( GL_BACK );

			GLuint queryId = qglGenQueries( 1 );
			qglBeginQuery( GL_ANY_SAMPLES_PASSED, queryId );

			// Draw occluder geometry...
			qglEndQuery( GL_ANY_SAMPLES_PASSED );
			GLuint numSamples = 0;
			qglGetQueryObjectuiv( queryId, GL_QUERY_RESULT, &numSamples );

			if ( numSamples > 0 ) {
				// Visible, render normally
			} else {
				// Occluded, skip rendering
			}
			qglDeleteQueries( 1, &queryId );
		} else {
			// Unsupported cull type
			assert( false );
		}
	}
	// Update state
	backEnd.qglState.faceCulling = cullType;
}

/*
=================
GL_ClearStateDelta

Clears the state delta bits, so the next GL_State
will set every item
=================
*/
void GL_ClearStateDelta( void ) {
	backEnd.qglState.forceGlState = true;
}

/*
====================
GL_TexEnv
====================
*/
void GL_TexEnv( int env ) {
	tmu_t *tmu = &backEnd.qglState.tmu[backEnd.qglState.currenttmu];
	if ( env == tmu->texEnv ) {
		return;
	}

	tmu->texEnv = env;

	switch ( env ) {
	case GL_COMBINE_EXT:
	case GL_MODULATE:
	case GL_REPLACE:
	case GL_DECAL:
	case GL_ADD:
		qglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, env );
		break;
	default:
		common->Error( "GL_TexEnv: invalid env '%d' passed\n", env );
		break;
	}
}
//bool forceGlState = false ) {
/*
====================
GL_State

This routine is responsible for setting the most commonly changed state
====================
*/
void GL_State( GLuint stateBits ) {
	if ( !r_useStateCaching.GetBool() || backEnd.qglState.forceGlState ) {
		// make sure everything is set all the time, so we
		// can see if our delta checking is screwing up
		GLuint diff = -1;
		backEnd.qglState.forceGlState = false;
	} else {
		GLuint diff = stateBits ^ backEnd.qglState.glStateBits;
		if ( !diff ) {
			return;
		}
	}

	//
	// check depthFunc bits
	//
	if ( diff & ( GLS_DEPTHFUNC_EQUAL | GLS_DEPTHFUNC_LESS | GLS_DEPTHFUNC_ALWAYS ) ) {
		GLenum depthFunc;
		if ( stateBits & GLS_DEPTHFUNC_EQUAL ) {
			//qglDepthFunc( GL_EQUAL );
			depthFunc = GL_EQUAL;
		} else if ( stateBits & GLS_DEPTHFUNC_ALWAYS ) {
			//qglDepthFunc( GL_ALWAYS );
			depthFunc = GL_ALWAYS;
		} else {
			//qglDepthFunc( GL_LEQUAL );
			depthFunc = GL_LEQUAL;
		}
		qglDepthFunc( depthFunc );
	}

	//
	// check blend bits
	//
	if ( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) {
		GLenum srcFactor, dstFactor;
		switch ( stateBits & GLS_SRCBLEND_BITS ) {
		case GLS_SRCBLEND_ZERO:
			srcFactor = GL_ZERO;
			break;
		case GLS_SRCBLEND_ONE:
			srcFactor = GL_ONE;
			break;
		case GLS_SRCBLEND_DST_COLOR:
			srcFactor = GL_DST_COLOR;
			break;
		case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
			srcFactor = GL_ONE_MINUS_DST_COLOR;
			break;
		case GLS_SRCBLEND_SRC_ALPHA:
			srcFactor = GL_SRC_ALPHA;
			break;
		case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
			srcFactor = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case GLS_SRCBLEND_DST_ALPHA:
			srcFactor = GL_DST_ALPHA;
			break;
		case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
			srcFactor = GL_ONE_MINUS_DST_ALPHA;
			break;
		case GLS_SRCBLEND_ALPHA_SATURATE:
			srcFactor = GL_SRC_ALPHA_SATURATE;
			break;
		case GLS_SRCBLEND_SRC_COLOR:
			srcFactor = GL_SRC_COLOR;
			break;
		default:
			srcFactor = GL_ONE;		// to get warning to shut up
			common->Error( "GL_State: invalid src blend state bits\n" );
			break;
		}

		switch ( stateBits & GLS_DSTBLEND_BITS ) {
		case GLS_DSTBLEND_ZERO:
			dstFactor = GL_ZERO;
			break;
		case GLS_DSTBLEND_ONE:
			dstFactor = GL_ONE;
			break;
		case GLS_DSTBLEND_SRC_COLOR:
			dstFactor = GL_SRC_COLOR;
			break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
			dstFactor = GL_ONE_MINUS_SRC_COLOR;
			break;
		case GLS_DSTBLEND_SRC_ALPHA:
			dstFactor = GL_SRC_ALPHA;
			break;
		case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
			dstFactor = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case GLS_DSTBLEND_DST_ALPHA:
			dstFactor = GL_DST_ALPHA;
			break;
		case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
			dstFactor = GL_ONE_MINUS_DST_ALPHA;
			break;
		default:
			dstFactor = GL_ONE;		// to get warning to shut up
			common->Error( "GL_State: invalid dst blend state bits\n" );
			break;
		}

		qglBlendFunc( srcFactor, dstFactor );
	}

	// check depthmask
	//
	if ( diff & GLS_DEPTHMASK && stateBits & GLS_DEPTHMASK ) {
		GLboolean depthMask = ( stateBits & GLS_DEPTHMASK ) ? GL_FALSE : GL_TRUE;
		qglDepthMask( depthMask );
	}

	// check colormask
	//
	diff &= (GLS_REDMASK | GLS_GREENMASK | GLS_BLUEMASK | GLS_ALPHAMASK);

	if ( diff & ( GLS_REDMASK|GLS_GREENMASK|GLS_BLUEMASK|GLS_ALPHAMASK ) ) {
		GLboolean r = ( stateBits & GLS_REDMASK ) ? 0 : 1;
		GLboolean g = ( stateBits & GLS_GREENMASK ) ? 0 : 1;
		GLboolean b = ( stateBits & GLS_BLUEMASK ) ? 0 : 1;
		GLboolean a = ( stateBits & GLS_ALPHAMASK ) ? 0 : 1;
		qglColorMask( r, g, b, a );
	}

	// fill/line mode
	//
	if ( diff & GLS_POLYMODE_LINE ) {
		GLenum mode = ( stateBits & GLS_POLYMODE_LINE ) ? GL_LINE : GL_FILL;
		qglPolygonMode( L_FRONT_AND_BACK, mode );
	}

	// alpha test
	//
	if ( ( diff & GLS_ATEST_BITS ) || (stateBits & GLS_ATEST_BITS ) == 0 ) {
		qglDisable( GL_ALPHA_TEST );
	} else if ( stateBits & GLS_ATEST_BITS == GLS_ATEST_EQ_255 ) {
		qglEnable( GL_ALPHA_TEST );
		qglAlphaFunc( GL_EQUAL, 1.0f );
	} else if ( stateBits & GLS_ATEST_BITS == GLS_ATEST_LT_128 ) {
		qglEnable( GL_ALPHA_TEST );
		qglAlphaFunc( GL_LESS, 0.5f );
	} else if ( stateBits & GLS_ATEST_BITS == GLS_ATEST_GE_128 ) {
		qglEnable( GL_ALPHA_TEST );
		qglAlphaFunc( GL_GEQUAL, 0.5f );
	} else {
		common->assert( 0 );
	}

	backEnd.qglState.glStateBits = stateBits;
}

void GL_SetCurrentTextureUnitEXP( int unit ) {
    if ( backEnd.qglState.currenttmu == unit ) {
        return;
    }

    if ( unit < 0 || unit >= GL_MAX_TEXTURE_IMAGE_UNITS ) {
        RB_LogWarning( "GL_SetCurrentTextureUnit: unit = %i", unit );
        return;
    }
	// Select texture unit
    qglActiveTextureARB( GL_TEXTURE0 + unit );

    RB_LogComment( "qglActiveTextureARB( %i );\n", unit, unit );
	// Avoid redundant state changes
	if ( backEnd.qglState.currenttmu == unit ) {
		return;
	}
	// update state tracking
	backEnd.qglState.currenttmu = unit;

	// bind the texture
	qglBindTexture( GL_TEXTURE_3D, 0 );

    // Check if modifications are enabled
    bool enableDepthModifications = true; // Set this to false to disable the modifications

    // Bind the 3D texture and apply the modifications if enabled
    if ( enableDepthModifications ) {
        qglActiveTexture( GL_TEXTURE0 + unit ) ;
        qglBindTexture( GL_TEXTURE_3D, 0 ) ;

        // Set the depth comparison function
        qglTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
        qglTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

        // Set the number of depth bits (optional)
        qglTexParameteri( GL_TEXTURE_3D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY ); // or GL_LUMINANCE, GL_RED, etc.
        qglTexParameteri( GL_TEXTURE_3D, GL_DEPTH_BITS, 24 ); // or the desired number of bits

        // Enable anisotropic filtering (optional)
        qglTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy );
    } else {
        // Fallback to standard glBindTexture if modifications are disabled
        qglBindTexture( GL_TEXTURE_2D, 0 );
    }
}

/*
============================================================================

RENDER BACK END THREAD FUNCTIONS

============================================================================
*/
static void SetViewportAndScissor( void ) {
	const float projectionMatrix;
	qglMatrixMode( GL_PROJECTION );
	qglLoadMatrixf( backEnd.viewParms.projectionMatrix );
	qglMatrixMode( GL_MODELVIEW );

	// set the window clipping
	qglViewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
	//qglScissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
	if ( r_useScissor.GetBool() ) {
		qglScissor( 0, 0, qglConfig.vidWidth, qglConfig.vidHeight );
	}
}

/*
=============
RB_SetGL2D

This is not used by the normal game paths, just by some tools
=============
*/
void RB_SetGL2D( void ) {
	// set 2D virtual screen size
	qglViewport( 0, 0, qglConfig.vidWidth, qglConfig.vidHeight );
	if ( r_useScissor.GetBool() ) {
		qglScissor( 0, 0, qglConfig.vidWidth, qglConfig.vidHeight );
	}

	//float	mat[16];
	//QGLMultMatrix( backEnd.viewDef->worldSpace.modelViewMatrix, backEnd.viewDef->projectionMatrix, mat );
	qglMatrixMode( GL_PROJECTION | GL_MODELVIEW );
	qglOrtho( 0, 640, 480, 0, 0, 1 );		// always assume 640x480 virtual coordinates
    qglLoadIdentity();

	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	GL_Cull( CT_TWO_SIDED );

	qglDisable( GL_DEPTH_TEST | GL_STENCIL_TEST );
}

/*
=============
RB_DrawFlickerBox
=============
*/
static void RB_DrawFlickerBox() {
	if ( !r_drawFlickerBox.GetBool() ) {
		return;
	}
	if ( tr.frameCount & 1 ) {
		qglClearColor( 1, 0, 0, 1 );
	} else {
		qglClearColor( 0, 1, 0, 1 );
	}
	qglScissor( 0, 0, 256, 256 );
	qglClear( GL_COLOR_BUFFER_BIT );
}

const void *RB_DrawBuffer( const void *data ) {
	const drawBufferCommand_t *cmd = (const drawBufferCommand_t *)data;

	qglDrawBuffer( cmd->buffer );

	// clear screen for debugging
	if ( r_clear.GetInteger() ) {
		qglClearColor( 1, 0, 0.5, 1 );
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}

	return (const void *)( cmd + 1 );
}

/*
=============
GL_BlockingSwapBuffers

We want to exit this with the GPU idle, right at vsync
=============
*/
const void GL_BlockingSwapBuffers() {
    RB_LogComment( "***************** GL_BlockingSwapBuffers *****************\n\n\n" );

	const int beforeFinish = Sys_Milliseconds();

	if ( !qglConfig.isSynchronized ) {
		qglFinish();
	}

	const int beforeSwap = Sys_Milliseconds();
	if ( r_showSwapBuffers.GetBool() && beforeSwap - beforeFinish > 1 ) {
		common->Printf( "%i msec to glFinish\n", beforeSwap - beforeFinish );
	}

	GLimp_SwapBuffers();

	const int beforeFence = Sys_Milliseconds();
	if ( r_showSwapBuffers.GetBool() && beforeFence - beforeSwap > 1 ) {
		common->Printf( "%i msec to swapBuffers\n", beforeFence - beforeSwap );
	}

	if ( qglConfig.isSynchronized ) {
		swapIndex ^= 1;
		if ( r_showSync.GetBool() ) {
			common->Printf( "%i msec to sync\n", Sys_Milliseconds() - beforeFence );
			if ( qglIsSync( renderSync[swapIndex] ) ) {
				for ( GLenum r = GL_TIMEOUT_EXPIRED; r == GL_TIMEOUT_EXPIRED; ) {
					r = qglWaitSync( renderSync[swapIndex], 0, 0 );
					common->Printf( "waitSync error: %i\n", r );
					swapIndex ^= 1;
					if ( r == GL_TIMEOUT_EXPIRED ) {
						break;
					}
					Sys_Sleep( 1 );
					r = qglIsSync( renderSync[swapIndex] );
					common->Error( "qglIsSync failed:%i\n", r );
				}
			qglDeleteSync( renderSync[swapIndex] );
		}

		// make sure the sync is after the swap
		// draw something tiny to ensure the sync is after the swap
		const unsigned int syncAfter = Sys_Milliseconds() + 1;
		unsigned char *syncByte = new unsigned char[4];
		*( (unsigned char *) syncByte ) = syncAfter;
		qglXWaitSync( dpy, syncAfter, 0 );
		delete[] syncByte;
		renderSync[swapIndex] = qglGenSync( GL_SYNC_SEQUENTIAL );
		qglXClientSync( dpy, syncAfter, GL_EXCLUSIVE_SYNC );
		common->Printf( "client sync at %i msec\n", syncAfter );
	} else {
		renderSync[swapIndex] = 0;
		common->Printf( "no sync available\n" );
		swapIndex ^= 1;
		if ( qglXMakeCurrent( dpy, None, NULL ) ) {
			common->Error( "qglXMakeCurrent failed\n" );
		}
	}

	glDrawBuffer  = GL_BACK;
	glReadBuffer = GL_BACK;
	c_cull_bits = cullType;

	qglState.faceCulling = faceCulling;
	qglState.cullType = cullType;
	qglState.cullFace = faceCulling;
	qglState.depthFunc = depthFunc;
	qglState.alphaTest = alphaTest;
	qglState.depthTest = depthTest;
	qglState.stencilTest = (qglIsEnabled(GL_STENCIL_TEST))?GL_TRUE:GL_FALSE;
	qglState.stencilOp = GL_KEEP;
	qglState.stencilFail = GL_KEEP;
	qglState.stencilZFail = GL_KEEP;
	qglState.stencilPass = GL_KEEP;
	qglState.depthFail = GL_KEEP;
	qglState.depthPass = GL_KEEP;
	qglState.depthMask = GL_TRUE;
	qglState.blend = GL_TRUE;
	qglState.blendSrc = GL_SRC_ALPHA;

		const int start = Sys_Milliseconds();
		qglScissor( 0, 0, 1, 1 );
		qglEnable( GL_SCISSOR_TEST );
		qglClear( GL_COLOR_BUFFER_BIT );
		renderSync[swapIndex] = qglFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
		const int end = Sys_Milliseconds();
		if ( r_showSwapBuffers.GetBool() && end - start > 1 ) {
			common->Printf( "%i msec to start fence\n", end - start );
		}

		GLsync syncToWaitOn;
		if ( r_syncEveryFrame.GetBool() ) {
			syncToWaitOn = renderSync[swapIndex];
		} else {
			syncToWaitOn = renderSync[!swapIndex];
		}

		if ( qglIsSync( syncToWaitOn ) ) {
			for ( GLenum r = GL_TIMEOUT_EXPIRED; r == GL_TIMEOUT_EXPIRED; ) {
				r = qglClientWaitSync( syncToWaitOn, GL_SYNC_FLUSH_COMMANDS_BIT, 1000 * 1000 );
			}
		}
	}

	const int afterFence = Sys_Milliseconds();
	if ( r_showSwapBuffers.GetBool() && afterFence - beforeFence > 1 ) {
		common->Printf( "%i msec to wait on fence\n", afterFence - beforeFence );
	}

	const int64 exitBlockTime = Sys_Microseconds();
	static int64 prevBlockTime;
	if ( r_showSwapBuffers.GetBool() && prevBlockTime ) {
		const int delta = (int) ( exitBlockTime - prevBlockTime );
		common->Printf( "blockToBlock: %i\n", delta );
	}
	prevBlockTime = exitBlockTime;
}

void idRenderBackend::DrawGlobalIllumination( const viewDef_t* _viewDef ) {

}

/*
=============
RB_RenderThread
=============
*/
void RB_RenderThread( void ) {
	const void *data;
	int readPixelPasses, drawPixelPasses, readDrawPasses;
	int writePixelPasses, writeDrawPassesnumDrawPasses;
	int numDrawPasses = 0;
	int numDrawPasses2 = 0;
	int numDrawPasses3 = 0;
	int numDrawPasses4 = 0;

	common->FrameStart();
	common->Printf( "---------- RB_RenderThread ----------\n" );
	common->FrameTime(Sys_Microseconds(), true);
	common->FrameEnd();
	common->FrameTime(Sys_Microseconds(), false);
	common->FrameEnd();
	common->Printf( "----------\n" );
	common->FrameTime(Sys_Microseconds(), true);

	// wait for either a rendering command or a quit command
	while ( 1 ) {
		// sleep until we have work to do
		data = GLimp_RendererSleep();
		if ( !data ) {
			return; // all done, renderer is shutting down
		}

		renderThreadActive = true;

		RB_ExecuteRenderCommands( data );

		renderThreadActive = false;
	}
}

/*
=============
RB_SetBuffer
=============
*/
static void	RB_SetBuffer( const void *data ) {
	// see which draw buffer we want to render the frame to
	const setBufferCommand_t *cmd = (const setBufferCommand_t *)data;

	backEnd.frameCount = cmd->frameCount;
	qglDrawBuffer( cmd->buffer );

	RB_LogComment( "---------- [RB_SetBuffer] ---------- to buffer # %d\n", cmd->buffer );

	GL_Scissor( 0, 0, tr.GetWidth(), tr.GetHeight() );

	// clear screen for debugging
	// automatically enable this with several other debug tools
	// that might leave unrendered portions of the screen
	if ( r_clear.GetFloat() || anString::Length( r_clear.GetString() ) != 1 || r_lockSurfaces.GetBool() || r_singleArea.GetBool() || r_showOverDraw.GetBool() ) {
		float c[3];
		switch ( r_clear.GetInteger() ) {
			case 2:
				qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
			break;
		default:
			if ( sscanf( r_clear.GetString(), "%f %f %f", &c[0], &c[1], &c[2] ) == 3 ) {
				qglClearColor( c[0], c[1], c[2], 1 );
			} else if ( r_showOverDraw.GetBool() ) {
				qglClearColor( 1.0f, 1.0f, 1.0f, 1.0f );
			} else {
				qglClearColor( 0.4f, 0.0f, 0.25f, 1.0f );
			}
			break;
		}
		qglClear( GL_COLOR_BUFFER_BIT );
	}
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.
===============
*/
void RB_ShowImages( void ) {
	RB_SetGL2D();

	// Setup orthographic projection
	qglMatrixMode( GL_PROJECTION );
	qglLoadIdentity();
	qglOrtho( 0.0f, ( GLdouble )qglConfig.vidWidth, 0.0, ( GLdouble )qglConfig.vidHeight, -1.0f, 1.0f );

	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();

	int start = Sys_Milliseconds();
	const float w = qglConfig.vidWidth / 20;
	const float h = qglConfig.vidHeight / 15;

	//for ( int m = 0 ; m < MAX_MIP_LEVELS ; m++ ) {
	for ( int i = 0 ; i < globalImages->images.Num() ; i++ ) {
		anImage *image = globalImages->images[i];
		if ( images->partialImage == NULL && images->texnum == anImage::TEXTURE_NOT_LOADED ) {
			continue;
		}

		float x = i % 20 * w;
		float y = i / 20 * h;

		// show in proportional size in mode 2
		if ( r_showImages.GetInteger() == 2 ) {
			w *= images->uploadWidth / 512.0f;
			h *= images->uploadHeight / 512.0f;
		}

		images->Bind();
		qglBindTexture(GL_TEXTURE_2D, image->texId);

		qglBegin( GL_QUADS );
		qglTexCoord2f( 0, 0 );
		qglVertex2f( x, y );
		qglTexCoord2f( 1, 0 );
		qglVertex2f( x + w, y );
		qglTexCoord2f( 1, 1 );
		qglVertex2f( x + w, y + h );
		qglTexCoord2f( 0, 1 );
		qglVertex2f( x, y + h );
		qglEnd();
	}

	qglFinish();

	int end = Sys_Milliseconds();
	common->Printf( "%i msec to draw all images\n", end - start );
}

/*
=============
RB_SwapBuffers
=============
*/
const void	RB_SwapBuffers( const void *data ) {
	// texture swapping test
	if ( r_showImages.GetInteger() != 0 ) {
		RB_ShowImages();
	}

	// force a gl sync if requested
	if ( r_finish.GetBool() ) {
		qglFinish();
	}

    RB_LogComment( "***************** RB_SwapBuffers *****************\n\n\n" );

	// don't flip if drawing to front buffer
	if ( !r_frontBuffer.GetBool() ) {
	    GLimp_SwapBuffers();
	}
}

/*
=============
RB_CopyRender

Copy part of the current framebuffer to an image
=============
*/
const void RB_CopyRender( const void *data ) {
	const setBufferCommand_t *cmd = (const setBufferCommand_t *)data;

	if ( r_skipCopyTexture.GetBool() ) {
		return;
	}

    RB_LogComment( "***************** RB_CopyRender *****************\n" );

	if ( cmd->image ) {
		cmd->image->CopyFramebuffer( cmd->x, cmd->y, cmd->imageWidth, cmd->imageHeight, false );
	}
}

/*
====================
RB_StereoRenderExecuteBackEndCommands

Renders the draw list twice, with slight modifications for left eye / right eye
====================
*/
void RB_StereoRenderExecuteBackEndCommands( const setBufferCommand_t * const allCmds ) {
	GLint backEndStartTime = Sys_Microseconds();

	// If we are in a monoscopic context, this draws to the only buffer, and is
	// the same as GL_BACK.  In a quad-buffer stereo context, this is necessary
	// to prevent GL from forcing the rendering to go to both BACK_LEFT and
	// BACK_RIGHT at a performance penalty.
	// To allow stereo deghost processing, the views have to be copied to separate
	// textures anyway, so there isn't any benefit to rendering to BACK_RIGHT for
	// that eye.
	qglDrawBuffer( GL_BACK_LEFT );

	// create the stereoRenderImage if we haven't already
	static anImage * stereoRenderImages[2];
	for ( int i = 0; i < 2; i++ ) {
		if ( stereoRenderImages[i] == NULL ) {
			stereoRenderImages[i] = globalImages->ImageFromFunction( va("_stereoRender%i",i), R_MakeStereoRenderImage );
		}

		// resize the stereo render image if the main window has changed size
		if ( stereoRenderImages[i]->GetUploadWidth() != renderSystem->GetWidth() ||
			 stereoRenderImages[i]->GetUploadHeight() != renderSystem->GetHeight() ) {
			stereoRenderImages[i]->Resize( renderSystem->GetWidth(), renderSystem->GetHeight() );
		}
	}

	// In stereoRender mode, the front end has generated two RC_DRAW_VIEW commands
	// with slightly different origins for each eye.

	// TODO: only do the copy after the final view has been rendered, not mirror subviews?

	// Render the 3D draw views from the screen origin so all the screen relative
	// texture mapping works properly, then copy the portion we are going to use
	// off to a texture.
	bool foundEye[2] = { false, false };

	for ( int stereoEye = 1; stereoEye >= -1; stereoEye -= 2 ) {
		// set up the target texture we will draw to
		const int targetEye = ( stereoEye == 1 ) ? 1 : 0;

		// Set the back end into a known default state to fix any stale render state issues
		GL_SetDefaultState();
		renderProgManager.Unbind();
		renderProgManager.ZeroUniforms();

		for ( const setBufferCommand_t * cmds = allCmds; cmds != NULL; cmds = (const setBufferCommand_t *)cmds->next ) {
			switch ( cmds->commandId ) {
			case RC_NOP:
				break;
			case RC_DRAW_VIEW_GUI:
			case RC_DRAW_VIEW_3D: {
					const setBufferCommand_t *const dsc = (const setBufferCommand_t *)cmds;
					const viewDef_t		&eyeViewDef = *dsc->viewDef;
					if ( eyeViewDef.renderView.viewEyeBuffer && eyeViewDef.renderView.viewEyeBuffer != stereoEye ) {
						// this is the render view for the other eye
						continue;
					}

					foundEye[ targetEye ] = true;
					RB_DrawView( dsc, stereoEye );
					if ( cmds->commandId == RC_DRAW_VIEW_GUI ) {
					}
				}
				break;
			case RC_SET_BUFFER:
				RB_SetBuffer( cmds );
				break;
			case RC_COPY_RENDER:
				RB_CopyRender( cmds );
				break;
			case RC_POST_PROCESS: {
					postProcessCommand_t * cmd = (postProcessCommand_t *)cmds;
					if ( cmd->viewDef->renderView.viewEyeBuffer != stereoEye ) {
						break;
					}
					RB_PostProcess( cmds );
				}
				break;
			default:
				common->Error( "RB_ExecuteBackEndCommands: bad commandId" );
				break;
			}
		}

		// copy to the target
		stereoRenderImages[ targetEye ]->CopyFramebuffer( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
	}

	// perform the final compositing / warping / deghosting to the actual framebuffer(s)
	assert( foundEye[0] && foundEye[1] );

	GL_SetDefaultState();

	//RB_SetMVP( renderMatrix_identity );

	// If we are in quad-buffer pixel format but testing another 3D mode,
	// make sure we draw to both eyes.  This is likely to be sub-optimal
	// performance on most cards and drivers, but it is better than getting
	// a confusing, half-ghosted view.
	if ( renderSystem->GetStereo3DMode() != STEREO3D_QUAD_BUFFER ) {
		qglDrawBuffer( GL_BACK );
	}

	GL_State( GLS_DEPTHFUNC_ALWAYS );
	GL_Cull( CT_TWO_SIDED );

	// We just want to do a quad pass - so make sure we disable any texgen and
	// set the texture matrix to the identity so we don't get anomalies from
	// any stale uniform data being present from a previous draw call
	const float texS[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	const float texT[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_S, texS );
	renderProgManager.SetRenderParm( RENDERPARM_TEXTUREMATRIX_T, texT );

	// disable any texgen
	const float texGenEnabled[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	renderProgManager.SetRenderParm( RENDERPARM_TEXGEN_0_ENABLED, texGenEnabled );

	renderProgManager.BindShader_Texture();
	GL_Color( 1, 1, 1, 1 );

	switch( renderSystem->GetStereo3DMode() ) {
	case STEREO3D_QUAD_BUFFER:
		glDrawBuffer( GL_BACK_RIGHT );
		GL_SetCurrentTextureUnit( 0 );
		stereoRenderImages[1]->Bind();
		GL_SetCurrentTextureUnit( 1 );
		stereoRenderImages[0]->Bind();
		RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );

		glDrawBuffer( GL_BACK_LEFT );
		GL_SetCurrentTextureUnit( 1 );
		stereoRenderImages[1]->Bind();
		GL_SetCurrentTextureUnit( 0 );
		stereoRenderImages[0]->Bind();
		RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );

		break;
	case STEREO3D_HDMI_720:
		// HDMI 720P 3D
		GL_SetCurrentTextureUnit( 0 );
		stereoRenderImages[1]->Bind();
		GL_SetCurrentTextureUnit( 1 );
		stereoRenderImages[0]->Bind();
		GL_ViewportAndScissor( 0, 0, 1280, 720 );
		RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );

		GL_SetCurrentTextureUnit( 0 );
		stereoRenderImages[0]->Bind();
		GL_SetCurrentTextureUnit( 1 );
		stereoRenderImages[1]->Bind();
		GL_ViewportAndScissor( 0, 750, 1280, 720 );
		RB_DrawElementsWithCounters( &backEnd.unitSquareSurface );

		// force the HDMI 720P 3D guard band to a constant color
		qglScissor( 0, 720, 1280, 30 );
		qglClear( GL_COLOR_BUFFER_BIT );
		break;
	default:

	// debug tool
	RB_DrawFlickerBox();

	// make sure the drawing is actually started
	qglFlush();

	// we may choose to sync to the swapbuffers before the next frame

	// stop rendering on this thread
	GLuint backEndFinishTime = Sys_Microseconds();
	backEnd.pc.totalMicroSec = backEndFinishTime - backEndStartTime;
}

/*
====================
RB_ExecuteBackEndCommands

This function will be called syncronously if running without
smp extensions, or asyncronously by another thread.
====================
*/
int backEndStartTime, backEndFinishTime;
void RB_ExecuteBackEndCommands( const setBufferCommand_t *cmds ) {
	//resolutionScale.SetCurrentGPUFrameTime( commonLocal.GetRendererGPUMicroseconds() );
	GL_StartFrame();

	// r_debugRenderToTexture
	if ( cmds->commandId == RC_NOP && !cmds->next ) {
		return;
	}

	GLint backEndStartTime = Sys_Milliseconds();

	// needed for editor rendering
	RB_SetDefaultGLState();

	// upload any image loads that have completed
	globalImages->CompleteBackgroundImageLoads();

	// If we have a stereo pixel format, this will draw to both
	// the back left and back right buffers, which will have a
	// performance penalty.
	qglDrawBuffer( GL_BACK );

	for ( ; cmds; cmds = (const setBufferCommand_t *)cmds->next ) {
		switch ( cmds->commandId ) {
		case RC_NOP:
			break;
		case RC_DRAW_VIEW:
		//break; //noView, or a noDraw?
		case RC_DRAW_VIEW_3D:
			RB_DrawView( cmds );
			if ( ( (const setBufferCommand_t *)cmds )->viewDef->viewEntitys ) {
				int	pc_draw3D = 0;
				pc_draw3D++;
			} else {
				int	pc_draw2D = 0;
				pc_draw2D++;
			}
			break;
		case RC_DRAW_VIEW_GUI:
			RB_DrawView( cmds, 0 );
			if ( ( (const setBufferCommand_t *)cmds )->viewDef->viewEntitys ) {
				pc_draw3D++;
			} else {
				pc_draw2D++;
			}
			break;
		case RC_SET_BUFFER:
			RB_SetBuffer( cmds );
			int	pc_setBuffers = 0;
			pc_setBuffers++;
			break;
		case RC_SWAP_BUFFERS:
			RB_SwapBuffers( cmds );
			int	pc_swapBuffers = 0;
			pc_swapBuffers++;
			break;
		case RC_COPY_RENDER:
			RB_CopyRender( cmds );
			int pc_copyRenders = 0
			pc_copyRenders++;
			break;
		case RC_POST_PROCESS:
			RB_PostProcess( cmds );
			break;
		default:
			common->Error( "RB_ExecuteBackEndCommands: bad commandId" );
			break;
		}
	}

	RB_DrawFlickerBox();
	// Fix for the steam overlay not showing up while in game without Shell/Debug/Console/Menu also rendering
	qglColorMask( 1, 1, 1, 1 );
	qglFlush();
	//GL_EndFrame();

	// go back to the default texture so the editor doesn't mess up a bound image
	qglBindTexture( GL_TEXTURE_2D, 0 );
	backEnd.qglState.tmu[0].current2DMap = -1;

	// stop rendering on this thread
	GLiunt backEndFinishTime = Sys_Milliseconds();
	backEnd.pc.mSec = backEndFinishTime - backEndStartTime;

	if ( r_debugRenderToTexture.GetInteger() == 1 ) {
		common->Printf( "3D: %i, 2D: %i, SetBuf: %i, SwpBuf: %i, CpyRenders: %i, CpyFrameBuf: %i\n", pc_draw3D, pc_draw2D, pc_setBuffers, pc_swapBuffers, pc_copyRenders, backEnd.copyFrameBufferPC );
		backEnd.copyFrameBufferPC = 0;
	}
	EndFrame();
}
