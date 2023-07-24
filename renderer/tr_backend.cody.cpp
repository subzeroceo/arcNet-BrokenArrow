#pragma hdrstop
#include "../idlib/precompiled.h"

#include "tr_local.h"
backEndState_t	backEnd;

idCVar r_drawEyeColor( "r_drawEyeColor", "0", CVAR_RENDERER | CVAR_BOOL, "Draw a colored box, red = left eye, blue = right eye, grey = non-stereo" );
idCVar r_motionBlur( "r_motionBlur", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "1 - 5, log2 of the number of motion blur samples" );
idCVar r_forceZPassStencilShadows( "r_forceZPassStencilShadows", "0", CVAR_RENDERER | CVAR_BOOL, "force Z-pass rendering for performance testing" );
idCVar r_useStencilShadowPreload( "r_useStencilShadowPreload", "1", CVAR_RENDERER | CVAR_BOOL, "use stencil shadow preload algorithm instead of Z-fail" );
idCVar r_skipShaderPasses( "r_skipShaderPasses", "0", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_skipInteractionFastPath( "r_skipInteractionFastPath", "1", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_useLightStencilSelect( "r_useLightStencilSelect", "0", CVAR_RENDERER | CVAR_BOOL, "use stencil select pass" );

/*
==================
GL_CheckErrors
==================
*/
void GL_CheckErrors( void ) {
    GLenum	err;
    char	stack[64];

	// check for up to 10 errors pending
	for ( int i = 0; i < 10; i++ ) {
		err = qglGetError();
		if ( err == GL_NO_ERROR ) {
			return;
		}
		switch( err ) {
			case GL_INVALID_ENUM:
				strncpy( stack, "GL_INVALID_ENUM" );
				break;
			case GL_INVALID_VALUE:
				strncpy( stack, "GL_INVALID_VALUE" );
				break;
			case GL_INVALID_OPERATION:
				strncpy( stack, "GL_INVALID_OPERATION" );
				break;
			case GL_STACK_OVERFLOW:
				strncpy( stack, "GL_STACK_OVERFLOW" );
				break;
			case GL_STACK_UNDERFLOW:
				strncpy( stack, "GL_STACK_UNDERFLOW" );
				break;
			case GL_OUT_OF_MEMORY:
				strncpy( stack, "GL_OUT_OF_MEMORY" );
				break;
			default:
				arcNetString::snPrintf( stack, sizeof( stack ), "%i", err );
				break;
		}

		if ( !r_ignoreGLErrors.GetBool() ) {
			common->Printf( "GL_CheckErrors: %s\n", stack );
		}
	}
}

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

oid GL_TexEnv( GLint env ) {
	if ( env == qglState.texEnv[ qglState.currenttmu ] ) {
		return;
	}
	qglState.texEnv[ qglState.currenttmu ] = env;

	switch ( env ) {
	case GL_MODULATE:
	case GL_REPLACE:
	case GL_DECAL:
	case GL_ADD:
		qglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, env );
		break;
	default:
		Error( ERR_DROP, "GL_TexEnv: invalid env '%d' passed", env );
		break;
	}
}

d GL_Bind( ARCImage *image ) {
	GLuint texnum;

	if ( !image ) {
		RB_LogComment( PRINT_WARNING, "GL_Bind: NULL image\n" );
		texnum = tr.defaultImage->texnum;
	} else {
		texnum = image->texnum;
	}

	if ( qglState.currenttextures[glState.currenttmu] != texnum ) {
		if ( image ) {
			image->frameUsed = tr.frameCount;
		}
		qglState.currenttextures[qglState.currenttmu] = texnum;
		qglBindTexture( GL_TEXTURE_2D, texnum );
	}
}

/*
========================
GL_Cull

Updated to support additional culling modes and OpenGL 4.6
========================
*/

void GL_Cull(int cullType) {
  if (cullType == CT_TWO_SIDED) {
    // Two sided culling disabled
    glDisable(GL_CULL_FACE);

  } else {

    // Enable culling
    glEnable(GL_CULL_FACE);

    if (cullType == CT_BACK_SIDED) {
      // Back face culling
      glCullFace(GL_BACK);

    } else if (cullType == CT_FRONT_SIDED) {
      // Front face culling
      glCullFace(GL_FRONT);

    } else if (cullType == CT_OCCLUSION_CULL) {
      // Occlusion culling using query objects
      glCullFace(GL_BACK);

      GLuint queryId = glGenQueries(1);
      glBeginQuery(GL_ANY_SAMPLES_PASSED, queryId);

      // Draw occluder geometry...

      glEndQuery(GL_ANY_SAMPLES_PASSED);
      GLuint numSamples = 0;
      glGetQueryObjectuiv(queryId, GL_QUERY_RESULT, &numSamples);

      if (numSamples > 0) {
        // Visible, render normally
      } else {
        // Occluded, skip rendering
      }

      glDeleteQueries(1, &queryId);

    } else {
      // Unsupported cull type
      assert(false);
    }
  }

  // Update state
  backEnd.qglState.faceCulling = cullType;
}
```

static const void *GL_DrawBuffer( const void *data ) {
	const drawBufferCommand_t *cmd = (const drawBufferCommand_t *)data;

	for ( int i = 0; i < buffer.size(); i++ ) {
		buffer[i] = { 255, 255, 255, 255 }; // White color
	}
	if ( qglConfig.useVertexBufferObject ) {
		BindFrameBuffer( cmd->buffer );
		qglDrawBuffer( GL_COLOR_ATTACHMENT0 );
	} else {
		qglDrawBuffer( cmd->buffer );
	}

	// clear screen for debugging
	if ( r_clear.GetInteger() ) {
		qglClearColor( 1, 0, 0.5, 1 );
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}

	return (const void *)( cmd + 1 );
}

/*
========================
GL_ClearDepth

Updated to support additional culling modes and OpenGL 4.6
========================
*/
static const void *GL_ClearDepthBuffer( const void *data ) {
	const clearDepthCommand_t *cmd = data;

	RB_EndSurface();
	qglClear( GL_DEPTH_BUFFER_BIT );

	return (const void *)(cmd + 1);
}

static const void *RB_ClearColor( const void *data ) {
	const clearColorCommand_t *cmd = data;

	if ( cmd->fullscreen ) {
		qglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
		qglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	}

	if ( cmd->colorMask ) {
		qglColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
	}

	qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );

	if ( cmd->frontAndBack ) {
		qglDrawBuffer( GL_FRONT );
		qglClear( GL_COLOR_BUFFER_BIT );
		qglDrawBuffer( GL_BACK );
	}

	qglClear( GL_COLOR_BUFFER_BIT );

	return (const void *)( cmd + 1 );
}