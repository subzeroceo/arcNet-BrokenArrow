#pragma hdrstop

void GL_Cull( int cullType ) {
	if ( backEnd.qglState.faceCulling == cullType ) {
		return;
	}

	if ( cullType == CT_TWO_SIDED ) {
		qglDisable( GL_CULL_FACE );
	} else {
		if ( backEnd.qglState.faceCulling == CT_TWO_SIDED ) {
			qglEnable( GL_CULL_FACE );
		}

		if ( cullType == CT_BACK_SIDED ) {
			if ( backEnd.viewDef->isMirror ) {
				qglCullFace( GL_FRONT );
			} else {
				qglCullFace( GL_BACK );
			}
		} else {
			if ( backEnd.viewDef->isMirror ) {
				qglCullFace( GL_BACK );
			} else {
				qglCullFace( GL_FRONT );
			}
		}
	}
	backEnd.qglState.faceCulling = cullType;
}

void GL_Scissor( int x, int y, int w, int h ) {
	qglScissor( x, y, w, h );
}

void GL_Viewport( int x, int y, int w, int h ) {
	qglViewport( x, y, w, h );
}

void GL_PolygonOffset( float scale, float bias ) {
	backEnd.qglState.polyOfsScale = scale;
	backEnd.qglState.polyOfsBias = bias;
	if ( backEnd.qglState.glStateBits & GLS_POLYGON_OFFSET ) {
		qglPolygonOffset( scale, bias );
	}
}

void GL_DepthBoundsTest( const float zMin, const float zMaz ) {
	if ( !qglConfig.depthBoundsTest || zMin > zMaz ) {
		return;
	}

	if ( zMin == 0.0f && zMaz == 0.0f ) {
		qglDisable( GL_DEPTH_BOUNDS_TEST_EXT );
	} else {
		qglEnable( GL_DEPTH_BOUNDS_TEST_EXT );
		qglDepthBoundsEXT( zMin, zMaz );
	}
}

void GL_StartDepthPass( const ARCScreenRect & rect ) {
}

void GL_FinishDepthPass() {
}

void GL_GetDepthPassRect( ARCScreenRect & rect ) {
	rect.Clear();
}

void GL_Color( float * color ) {
	if ( color == NULL ) {
		return;
	}
	GL_Color( color[0], color[1], color[2], color[3] );
}

void GL_Color( float r, float g, float b ) {
	GL_Color( r, g, b, 1.0f );
}

void GL_Color( float r, float g, float b, float a ) {
	float parm[4];
	parm[0] = ::ClampFloat( 0.0f, 1.0f, r );
	parm[1] = ::ClampFloat( 0.0f, 1.0f, g );
	parm[2] = ::ClampFloat( 0.0f, 1.0f, b );
	parm[3] = ::ClampFloat( 0.0f, 1.0f, a );
	//renderProgManager.SetRenderParm( RENDERPARM_COLOR, parm );
}

void GL_Clear( bool color, bool depth, bool stencil, byte stencilValue, float r, float g, float b, float a ) {
	int clearFlags = 0;
	if ( color ) {
		qglClearColor( r, g, b, a );
		clearFlags |= GL_COLOR_BUFFER_BIT;
	}
	if ( depth ) {
		clearFlags |= GL_DEPTH_BUFFER_BIT;
	}
	if ( stencil ) {
		qglClearStencil( stencilValue );
		clearFlags |= GL_STENCIL_BUFFER_BIT;
	}
	qglClear( clearFlags );
}


// We want to exit this with the GPU idle, right at vsync
const void GL_BlockingSwapBuffers() {
    RENDERLOG_PRINTF( "***************** GL_BlockingSwapBuffers *****************\n\n\n" );

	const int beforeFinish = RB_Milliseconds();

	if ( !qglConfig.isSynchronized ) {
		glFinish();
	}

	const int beforeSwap = RB_Milliseconds();
	if ( r_showSwapBuffers.GetBool() && beforeSwap - beforeFinish > 1 ) {
		common->Printf( "%i msec to qglFinish\n", beforeSwap - beforeFinish );
	}

	GLimp_SwapBuffers();

	const int beforeFence = RB_Milliseconds();
	if ( r_showSwapBuffers.GetBool() && beforeFence - beforeSwap > 1 ) {
		common->Printf( "%i msec to swapBuffers\n", beforeFence - beforeSwap );
	}

	if ( qglConfig.isSynchronized ) {
		swapIndex ^= 1;
		if ( qglIsSync( renderSync[swapIndex] ) ) {
			qglDeleteSync( renderSync[swapIndex] );
		}
		// draw something tiny to ensure the sync is after the swap
		const int start = RB_Milliseconds();
		qglScissor( 0, 0, 1, 1 );
		qglEnable( GL_SCISSOR_TEST );
		qglClear( GL_COLOR_BUFFER_BIT );
		renderSync[swapIndex] = qglFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
		const int end = RB_Milliseconds();
		if ( r_showSwapBuffers.GetBool() && end - start > 1 ) {
			common->Printf( "%i msec to start fence\n", end - start );
		}

		GLsync	syncToWaitOn;
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

	const int afterFence = RB_Milliseconds();
	if ( r_showSwapBuffers.GetBool() && afterFence - beforeFence > 1 ) {
		common->Printf( "%i msec to wait on fence\n", afterFence - beforeFence );
	}

	const int64 exitBlockTime = Sys_Microseconds();

	static int64 prevBlockTime;
	if ( r_showSwapBuffers.GetBool() && prevBlockTime ) {
		const int delta = ( int ) ( exitBlockTime - prevBlockTime );
		common->Printf( "blockToBlock: %i\n", delta );
	}
	prevBlockTime = exitBlockTime;
}

void GL_SetDefaultState() {
	RENDERLOG_PRINTF( "--- GL_SetDefaultState ---\n" );

	qglClearDepth( 1.0f );

	// make sure our GL state vector is set correctly
	memset( &backEnd.qglState, 0, sizeof( backEnd.qglState ) );
	//backEnd.qglState.forceGlState = true;
	GL_State( 0, true );
	// These are changed by GL_Cull
	/*qglCullFace( GL_FRONT_AND_BACK );
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

	qglShadeModel( GL_SMOOTH );
	qglEnable( GL_DEPTH_TEST );
	qglEnable( GL_BLEND );
	qglEnable( GL_SCISSOR_TEST );
	qglDrawBuffer( GL_BACK );
	qglReadBuffer( GL_BACK );*/
	qglColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

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

	for ( i = qglConfig.maxImageUnits - 1; i >= 0; i-- ) {
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
	}
}

void RB_SetGL2D( void ) {
	backEnd.projection2D = true;

	// set 2D virtual screen size
	qglViewport( 0, 0, qglConfig.vidWidth, qglConfig.vidHeight );
	qglScissor( 0, 0, qglConfig.vidWidth, qglConfig.vidHeight );
	qglMatrixMode( GL_PROJECTION );
	qglLoadIdentity();
	qglOrtho( 0, qglConfig.vidWidth, qglConfig.vidHeight, 0, 0, 1 );
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity();

	GL_State( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	qglDisable( GL_FOG );

	GL_Cull( CT_TWO_SIDED );
	qglDisable( GL_CLIP_PLANE0 );

	// set time for 2D shaders
	backEnd.refdef.time = RB_Milliseconds();
	backEnd.refdef.floatTime = backEnd.refdef.time * 0.001;
}

void GL_State( uint64 stateBits, bool forceGlState ) {
	uint64 diff = stateBits ^ backEnd.qglState.glStateBits;

	if ( !r_useStateCaching.GetBool() || forceGlState ) {
		// make sure everything is set all the time, so we
		// can see if our delta checking is screwing up
		diff = 0xFFFFFFFFFFFFFFFF;
	} else if ( diff == 0 ) {
		return;
	}

	//
	// check depthFunc bits
	//
	if ( diff & GLS_DEPTHFUNC_BITS ) {
		switch ( stateBits & GLS_DEPTHFUNC_BITS ) {
			case GLS_DEPTHFUNC_EQUAL:	qglDepthFunc( GL_EQUAL ); break;
			case GLS_DEPTHFUNC_ALWAYS:	qglDepthFunc( GL_ALWAYS ); break;
			case GLS_DEPTHFUNC_LESS:	qglDepthFunc( GL_LEQUAL ); break;
			case GLS_DEPTHFUNC_GREATER:	qglDepthFunc( GL_GEQUAL ); break;
		}
	}

	//
	// check blend bits
	//
	if ( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) {
		GLenum srcFactor = GL_ONE;
		GLenum dstFactor = GL_ZERO;

		switch ( stateBits & GLS_SRCBLEND_BITS ) {
			case GLS_SRCBLEND_ZERO:					srcFactor = GL_ZERO; break;
			case GLS_SRCBLEND_ONE:					srcFactor = GL_ONE; break;
			case GLS_SRCBLEND_DST_COLOR:			srcFactor = GL_DST_COLOR; break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:	srcFactor = GL_ONE_MINUS_DST_COLOR; break;
			case GLS_SRCBLEND_SRC_ALPHA:			srcFactor = GL_SRC_ALPHA; break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:	srcFactor = GL_ONE_MINUS_SRC_ALPHA; break;
			case GLS_SRCBLEND_DST_ALPHA:			srcFactor = GL_DST_ALPHA; break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:	srcFactor = GL_ONE_MINUS_DST_ALPHA; break;
			default:
				assert( !"GL_State: invalid src blend state bits\n" );
				break;
		}

		switch ( stateBits & GLS_DSTBLEND_BITS ) {
			case GLS_DSTBLEND_ZERO:					dstFactor = GL_ZERO; break;
			case GLS_DSTBLEND_ONE:					dstFactor = GL_ONE; break;
			case GLS_DSTBLEND_SRC_COLOR:			dstFactor = GL_SRC_COLOR; break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:	dstFactor = GL_ONE_MINUS_SRC_COLOR; break;
			case GLS_DSTBLEND_SRC_ALPHA:			dstFactor = GL_SRC_ALPHA; break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:	dstFactor = GL_ONE_MINUS_SRC_ALPHA; break;
			case GLS_DSTBLEND_DST_ALPHA:			dstFactor = GL_DST_ALPHA; break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:  dstFactor = GL_ONE_MINUS_DST_ALPHA; break;
			default:
				assert( !"GL_State: invalid dst blend state bits\n" );
				break;
		}

		// Only actually update GL's blend func if blending is enabled.
		if ( srcFactor == GL_ONE && dstFactor == GL_ZERO ) {
			qglDisable( GL_BLEND );
		} else {
			qglEnable( GL_BLEND );
			qglBlendFunc( srcFactor, dstFactor );
		}
	}

	//
	// check depthmask
	//
	if ( diff & GLS_DEPTHMASK ) {
		if ( stateBits & GLS_DEPTHMASK ) {
			qglDepthMask( GL_FALSE );
		} else {
			qglDepthMask( GL_TRUE );
		}
	}

	//
	// check colormask
	//
	if ( diff & ( GLS_REDMASK|GLS_GREENMASK|GLS_BLUEMASK|GLS_ALPHAMASK) ) {
		GLboolean r = ( stateBits & GLS_REDMASK ) ? GL_FALSE : GL_TRUE;
		GLboolean g = ( stateBits & GLS_GREENMASK ) ? GL_FALSE : GL_TRUE;
		GLboolean b = ( stateBits & GLS_BLUEMASK ) ? GL_FALSE : GL_TRUE;
		GLboolean a = ( stateBits & GLS_ALPHAMASK ) ? GL_FALSE : GL_TRUE;
		qglColorMask( r, g, b, a );
	}

	//
	// fill/line mode
	//
	if ( diff & GLS_POLYMODE_LINE ) {
		if ( stateBits & GLS_POLYMODE_LINE ) {
			qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		} else {
			qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	//
	// polygon offset
	//
	if ( diff & GLS_POLYGON_OFFSET ) {
		if ( stateBits & GLS_POLYGON_OFFSET ) {
			qglPolygonOffset( backEnd.qglState.polyOfsScale, backEnd.qglState.polyOfsBias );
			qglEnable( GL_POLYGON_OFFSET_FILL );
			qglEnable( GL_POLYGON_OFFSET_LINE );
		} else {
			qglDisable( GL_POLYGON_OFFSET_FILL );
			qglDisable( GL_POLYGON_OFFSET_LINE );
		}
	}

#if !defined( USE_CORE_PROFILE )
	//
	// alpha test
	//
	if ( diff & ( GLS_ALPHATEST_FUNC_BITS | GLS_ALPHATEST_FUNC_REF_BITS ) ) {
		if ( ( stateBits & GLS_ALPHATEST_FUNC_BITS ) != 0 ) {
			qglEnable( GL_ALPHA_TEST );

			GLenum func = GL_ALWAYS;
			switch ( stateBits & GLS_ALPHATEST_FUNC_BITS ) {
				case GLS_ALPHATEST_FUNC_LESS:		func = GL_LESS; break;
				case GLS_ALPHATEST_FUNC_EQUAL:		func = GL_EQUAL; break;
				case GLS_ALPHATEST_FUNC_GREATER:	func = GL_GEQUAL; break;
				default: assert( false );
			}
			GLclampf ref = ( ( stateBits & GLS_ALPHATEST_FUNC_REF_BITS ) >> GLS_ALPHATEST_FUNC_REF_SHIFT ) / ( float )0xFF;
			qglAlphaFunc( func, ref );
		} else {
			qglDisable( GL_ALPHA_TEST );
		}
	}
#endif

	// stencil
	if ( diff & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS ) ) {
		if ( ( stateBits & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_OP_BITS ) ) != 0 ) {
			qglEnable( GL_STENCIL_TEST );
		} else {
			qglDisable( GL_STENCIL_TEST );
		}
	}

	if ( diff & ( GLS_STENCIL_FUNC_BITS | GLS_STENCIL_FUNC_REF_BITS | GLS_STENCIL_FUNC_MASK_BITS ) ) {
		GLuint ref = GLuint( ( stateBits & GLS_STENCIL_FUNC_REF_BITS ) >> GLS_STENCIL_FUNC_REF_SHIFT );
		GLuint mask = GLuint( ( stateBits & GLS_STENCIL_FUNC_MASK_BITS ) >> GLS_STENCIL_FUNC_MASK_SHIFT );
		GLenum func = 0;
		switch ( stateBits & GLS_STENCIL_FUNC_BITS ) {
			case GLS_STENCIL_FUNC_NEVER:		func = GL_NEVER; break;
			case GLS_STENCIL_FUNC_LESS:			func = GL_LESS; break;
			case GLS_STENCIL_FUNC_EQUAL:		func = GL_EQUAL; break;
			case GLS_STENCIL_FUNC_LEQUAL:		func = GL_LEQUAL; break;
			case GLS_STENCIL_FUNC_GREATER:		func = GL_GREATER; break;
			case GLS_STENCIL_FUNC_NOTEQUAL:		func = GL_NOTEQUAL; break;
			case GLS_STENCIL_FUNC_GEQUAL:		func = GL_GEQUAL; break;
			case GLS_STENCIL_FUNC_ALWAYS:		func = GL_ALWAYS; break;
		}
		qglStencilFunc( func, ref, mask );
	}

	if ( diff & ( GLS_STENCIL_OP_FAIL_BITS | GLS_STENCIL_OP_ZFAIL_BITS | GLS_STENCIL_OP_PASS_BITS ) ) {
		GLenum sFail = 0;
		GLenum zFail = 0;
		GLenum pass = 0;
		switch ( stateBits & GLS_STENCIL_OP_FAIL_BITS ) {
			case GLS_STENCIL_OP_FAIL_KEEP:		sFail = GL_KEEP; break;
			case GLS_STENCIL_OP_FAIL_ZERO:		sFail = GL_ZERO; break;
			case GLS_STENCIL_OP_FAIL_REPLACE:	sFail = GL_REPLACE; break;
			case GLS_STENCIL_OP_FAIL_INCR:		sFail = GL_INCR; break;
			case GLS_STENCIL_OP_FAIL_DECR:		sFail = GL_DECR; break;
			case GLS_STENCIL_OP_FAIL_INVERT:	sFail = GL_INVERT; break;
			case GLS_STENCIL_OP_FAIL_INCR_WRAP: sFail = GL_INCR_WRAP; break;
			case GLS_STENCIL_OP_FAIL_DECR_WRAP: sFail = GL_DECR_WRAP; break;
		}
		switch ( stateBits & GLS_STENCIL_OP_ZFAIL_BITS ) {
			case GLS_STENCIL_OP_ZFAIL_KEEP:		zFail = GL_KEEP; break;
			case GLS_STENCIL_OP_ZFAIL_ZERO:		zFail = GL_ZERO; break;
			case GLS_STENCIL_OP_ZFAIL_REPLACE:	zFail = GL_REPLACE; break;
			case GLS_STENCIL_OP_ZFAIL_INCR:		zFail = GL_INCR; break;
			case GLS_STENCIL_OP_ZFAIL_DECR:		zFail = GL_DECR; break;
			case GLS_STENCIL_OP_ZFAIL_INVERT:	zFail = GL_INVERT; break;
			case GLS_STENCIL_OP_ZFAIL_INCR_WRAP:zFail = GL_INCR_WRAP; break;
			case GLS_STENCIL_OP_ZFAIL_DECR_WRAP:zFail = GL_DECR_WRAP; break;
		}
		switch ( stateBits & GLS_STENCIL_OP_PASS_BITS ) {
			case GLS_STENCIL_OP_PASS_KEEP:		pass = GL_KEEP; break;
			case GLS_STENCIL_OP_PASS_ZERO:		pass = GL_ZERO; break;
			case GLS_STENCIL_OP_PASS_REPLACE:	pass = GL_REPLACE; break;
			case GLS_STENCIL_OP_PASS_INCR:		pass = GL_INCR; break;
			case GLS_STENCIL_OP_PASS_DECR:		pass = GL_DECR; break;
			case GLS_STENCIL_OP_PASS_INVERT:	pass = GL_INVERT; break;
			case GLS_STENCIL_OP_PASS_INCR_WRAP:	pass = GL_INCR_WRAP; break;
			case GLS_STENCIL_OP_PASS_DECR_WRAP:	pass = GL_DECR_WRAP; break;
		}
		qglStencilOp( sFail, zFail, pass );
	}
	backEnd.qglState.glStateBits = stateBits;
}

uint64 GL_GetCurrentState() {
	//return backEnd.qglState.glStateBits;
}

uint64 GL_GetCurrentStateMinusStencil() {
	return GL_GetCurrentState() & ~( GLS_STENCIL_OP_BITS|GLS_STENCIL_FUNC_BITS|GLS_STENCIL_FUNC_REF_BITS|GLS_STENCIL_FUNC_MASK_BITS);
}

bool R_CheckExtension( char *name ) {
	if ( !strstr( qglConfig.qglExtStrOutput, name ) ) {
		printf( "X..%s not found\n", name );
		return false;
	}
	printf( "...using %s\n", name );
	return true;
}

static void R_CheckPortableExtensions() {
	qglConfig.qglVersion = atof( qglConfig.versionOutput );
	const char * badVideoCard = ARCLocalization::GetString( "#str_" );
	if ( qglConfig.qglVersion < 2.0f ) {
		arcLibrary::FatalError( badVideoCard );
	}

	if ( icmpn( qglConfig.rendererOutput, "ATI ", 4 ) == 0 || arcNetString::Icmpn( qglConfig.rendererOutput, "AMD ", 4 ) == 0 ) {
		qglConfig.vendor = VENDOR_AMD;
	}else if ( qglConfig.rendererOutput, "AMD64", 4 == 0 || arcNetString::Icmpn( qglConfig.rendererOutput, "RADEON VEGA", 4 ) == 0 ) {
		qglConfig.vendor = VENDOR_AMD64;
	} else if ( ::Icmpn( qglConfig.rendererOutput, "NVIDIA", 6 ) == 0 ) {
		qglConfig.vendor = VENDOR_NVIDIA;
	} else if ( ::Icmpn( qglConfig.rendererOutput, "Intel", 5 ) == 0 ) {
		qglConfig.vendor = VENDOR_INTEL;
	}

	// GL_ARB_multitexture
	qglConfig.multitextureAvailable = R_CheckExtension( "GL_ARB_multitexture" );
	if ( qglConfig.multitextureAvailable ) {
		qglActiveTextureARB = (void(APIENTRY *)( GLenum) )GLimp_ExtensionPointer( "glActiveTextureARB" );
		qglClientActiveTextureARB = (void(APIENTRY *)( GLenum) )GLimp_ExtensionPointer( "glClientActiveTextureARB" );
	}

	// GL_EXT_direct_state_access
	qglConfig.directStateAccess = R_CheckExtension( "GL_EXT_direct_state_access" );
	if ( qglConfig.directStateAccess ) {
		qglBindMultiTextureEXT = (PFNGLBINDMULTITEXTUREEXTPROC)GLimp_ExtensionPointer( "glBindMultiTextureEXT" );
	} else {
		qglBindMultiTextureEXT = glBindMultiTextureEXT;
	}

	// GL_ARB_texture_compression + GL_S3_s3tc
	// DRI drivers may have GL_ARB_texture_compression but no GL_EXT_texture_compression_s3tc
	qglConfig.textureCompression = R_CheckExtension( "GL_ARB_texture_compression" ) && R_CheckExtension( "GL_EXT_texture_compression_s3tc" );
	if ( qglConfig.textureCompression ) {
		qglCompressedTexImage2DARB = (PFNGLCOMPRESSEDTEXIMAGE2DARBPROC)GLimp_ExtensionPointer( "glCompressedTexImage2DARB" );
		qglCompressedTexSubImage2DARB = (PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC)GLimp_ExtensionPointer( "glCompressedTexSubImage2DARB" );
		qglGetCompressedTexImageARB = (PFNGLGETCOMPRESSEDTEXIMAGEARBPROC)GLimp_ExtensionPointer( "glGetCompressedTexImageARB" );
	}

	// GL_EXT_texture_filter_anisotropic
	qglConfig.anisotropicFilterAvailable = R_CheckExtension( "GL_EXT_texture_filter_anisotropic" );
	if ( qglConfig.anisotropicFilterAvailable ) {
		qglGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &qglConfig.maxTextureAnisotropy );
		printf( "   maxTextureAnisotropy: %f\n", qglConfig.maxTextureAnisotropy );
	} else {
		qglConfig.maxTextureAnisotropy = 1;
	}

	// GL_EXT_texture_lod_bias
	// The actual extension is broken as specificed, storing the state in the texture unit instead
	// of the texture object.  The behavior in GL 1.4 is the behavior we use.
	qglConfig.useTextureLODBias = ( qglConfig.qglVersion >= 1.4 || R_CheckExtension( "GL_EXT_texture_lod_bias" ) );
	if ( qglConfig.useTextureLODBias ) {
		printf( "...using %s\n", "GL_EXT_texture_lod_bias" );
	} else {
		printf( "X..%s not found\n", "GL_EXT_texture_lod_bias" );
	}

	// GL_ARB_seamless_cube_map
	qglConfig.useSeamlessCubeMap = R_CheckExtension( "GL_ARB_seamless_cube_map" );
	r_useSeamlessCubeMap.SetModified();		// the CheckCvars() next frame will enable / disable it

	// GL_ARB_framebuffer_sRGB
	qglConfig.useSRGBFramebuffer = R_CheckExtension( "GL_ARB_framebuffer_sRGB" );
	r_useSRGB.SetModified();		// the CheckCvars() next frame will enable / disable it

	// GL_ARB_vertex_buffer_object
	qglConfig.vboAvailable = R_CheckExtension( "GL_ARB_vertex_buffer_object" );
	if ( qglConfig.vboAvailable ) {
		qglBindBufferARB = (PFNGLBINDBUFFERARBPROC)GLimp_ExtensionPointer( "glBindBufferARB" );
		qglBindBufferRange = (PFNGLBINDBUFFERRANGEPROC)GLimp_ExtensionPointer( "glBindBufferRange" );
		qglDeleteBuffersARB = (PFNGLDELETEBUFFERSARBPROC)GLimp_ExtensionPointer( "glDeleteBuffersARB" );
		qglGenBuffersARB = (PFNGLGENBUFFERSARBPROC)GLimp_ExtensionPointer( "glGenBuffersARB" );
		qglIsBufferARB = (PFNGLISBUFFERARBPROC)GLimp_ExtensionPointer( "glIsBufferARB" );
		qglBufferDataARB = (PFNGLBUFFERDATAARBPROC)GLimp_ExtensionPointer( "glBufferDataARB" );
		qglBufferSubDataARB = (PFNGLBUFFERSUBDATAARBPROC)GLimp_ExtensionPointer( "glBufferSubDataARB" );
		qglGetBufferSubDataARB = (PFNGLGETBUFFERSUBDATAARBPROC)GLimp_ExtensionPointer( "glGetBufferSubDataARB" );
		qglMapBufferARB = (PFNGLMAPBUFFERARBPROC)GLimp_ExtensionPointer( "glMapBufferARB" );
		qglUnmapBufferARB = (PFNGLUNMAPBUFFERARBPROC)GLimp_ExtensionPointer( "glUnmapBufferARB" );
		qglGetBufferParameterivARB = (PFNGLGETBUFFERPARAMETERIVARBPROC)GLimp_ExtensionPointer( "glGetBufferParameterivARB" );
		qglGetBufferPointervARB = (PFNGLGETBUFFERPOINTERVARBPROC)GLimp_ExtensionPointer( "glGetBufferPointervARB" );
	}

	// GL_ARB_map_buffer_range, map a section of a buffer object's data store
	qglConfig.mapBufferRangeAvailable = R_CheckExtension( "GL_ARB_map_buffer_range" );
	if ( qglConfig.mapBufferRangeAvailable ) {
		qglMapBufferRange = (PFNGLMAPBUFFERRANGEPROC)GLimp_ExtensionPointer( "glMapBufferRange" );
	}

	// GL_ARB_vertex_array_object
	qglConfig.isVertexArrayEnabled = R_CheckExtension( "GL_ARB_vertex_array_object" );
	if ( qglConfig.isVertexArrayEnabled ) {
		qglGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)GLimp_ExtensionPointer( "glGenVertexArrays" );
		qglBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)GLimp_ExtensionPointer( "glBindVertexArray" );
		qglDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)GLimp_ExtensionPointer( "glDeleteVertexArrays" );
	}

	// GL_ARB_draw_elements_base_vertex
	qglConfig.drawElementsBaseVertex = R_CheckExtension( "GL_ARB_draw_elements_base_vertex" );
	if ( qglConfig.drawElementsBaseVertex ) {
		qglDrawElementsBaseVertex = (PFNGLDRAWELEMENTSBASEVERTEXPROC)GLimp_ExtensionPointer( "glDrawElementsBaseVertex" );
	}

	// GL_ARB_vertex_program / GL_ARB_fragment_program
	qglConfig.fragmentProgramOn = R_CheckExtension( "GL_ARB_fragment_program" );
	if ( qglConfig.fragmentProgramOn ) {
		qglVertexAttribPointerARB = (PFNGLVERTEXATTRIBPOINTERARBPROC)GLimp_ExtensionPointer( "glVertexAttribPointerARB" );
		qglEnableVertexAttribArrayARB = (PFNGLENABLEVERTEXATTRIBARRAYARBPROC)GLimp_ExtensionPointer( "glEnableVertexAttribArrayARB" );
		qglDisableVertexAttribArrayARB = (PFNGLDISABLEVERTEXATTRIBARRAYARBPROC)GLimp_ExtensionPointer( "glDisableVertexAttribArrayARB" );
		qglProgramStringARB = (PFNGLPROGRAMSTRINGARBPROC)GLimp_ExtensionPointer( "glProgramStringARB" );
		qglBindProgramARB = (PFNGLBINDPROGRAMARBPROC)GLimp_ExtensionPointer( "glBindProgramARB" );
		qglGenProgramsARB = (PFNGLGENPROGRAMSARBPROC)GLimp_ExtensionPointer( "glGenProgramsARB" );
		qglDeleteProgramsARB = (PFNGLDELETEPROGRAMSARBPROC)GLimp_ExtensionPointer( "glDeleteProgramsARB" );
		qglProgramEnvParameter4fvARB = (PFNGLPROGRAMENVPARAMETER4FVARBPROC)GLimp_ExtensionPointer( "glProgramEnvParameter4fvARB" );
		qglProgramLocalParameter4fvARB = (PFNGLPROGRAMLOCALPARAMETER4FVARBPROC)GLimp_ExtensionPointer( "glProgramLocalParameter4fvARB" );

		qglGetIntegerv( GL_MAX_TEXTURE_COORDS_ARB, (GLint *)&qglConfig.maxImageCoords );
		qglGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS_ARB, (GLint *)&qglConfig.maxTextureImageUnits );
	}

	// GLSL, core in OpenGL > 2.0
	qglConfig.enableGLsl = ( qglConfig.qglVersion >= 2.0f );
	if ( qglConfig.enableGLsl ) {
		qglCreateShader = (PFNGLCREATESHADERPROC)GLimp_ExtensionPointer( "glCreateShader" );
		qglDeleteShader = (PFNGLDELETESHADERPROC)GLimp_ExtensionPointer( "glDeleteShader" );
		qglShaderSource = (PFNGLSHADERSOURCEPROC)GLimp_ExtensionPointer( "glShaderSource" );
		qglCompileShader = (PFNGLCOMPILESHADERPROC)GLimp_ExtensionPointer( "glCompileShader" );
		qglGetShaderiv = (PFNGLGETSHADERIVPROC)GLimp_ExtensionPointer( "glGetShaderiv" );
		qglGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)GLimp_ExtensionPointer( "glGetShaderInfoLog" );
		qglCreateProgram = (PFNGLCREATEPROGRAMPROC)GLimp_ExtensionPointer( "glCreateProgram" );
		qglDeleteProgram = (PFNGLDELETEPROGRAMPROC)GLimp_ExtensionPointer( "glDeleteProgram" );
		qglAttachShader = (PFNGLATTACHSHADERPROC)GLimp_ExtensionPointer( "glAttachShader" );
		qglDetachShader = (PFNGLDETACHSHADERPROC)GLimp_ExtensionPointer( "glDetachShader" );
		qglLinkProgram = (PFNGLLINKPROGRAMPROC)GLimp_ExtensionPointer( "glLinkProgram" );
		qglUseProgram = (PFNGLUSEPROGRAMPROC)GLimp_ExtensionPointer( "glUseProgram" );
		qglGetProgramiv = (PFNGLGETPROGRAMIVPROC)GLimp_ExtensionPointer( "glGetProgramiv" );
		qglGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)GLimp_ExtensionPointer( "glGetProgramInfoLog" );
		qglBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)GLimp_ExtensionPointer( "glBindAttribLocation" );
		qglGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)GLimp_ExtensionPointer( "glGetUniformLocation" );
		qglUniform1i = (PFNGLUNIFORM1IPROC)GLimp_ExtensionPointer( "glUniform1i" );
		qglUniform4fv = (PFNGLUNIFORM4FVPROC)GLimp_ExtensionPointer( "glUniform4fv" );
	}

	// GL_ARB_uniform_buffer_object
	qglConfig.uniformBufferEnabled = R_CheckExtension( "GL_ARB_uniform_buffer_object" );
	if ( qglConfig.uniformBufferEnabled ) {
		qglGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)GLimp_ExtensionPointer( "glGetUniformBlockIndex" );
		qglUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)GLimp_ExtensionPointer( "glUniformBlockBinding" );

		qglGetIntegerv( GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, (GLint *)&qglConfig.uniformBufferOffsetAlignment );
		if ( qglConfig.uniformBufferOffsetAlignment < 256 ) {
			qglConfig.uniformBufferOffsetAlignment = 256;
		}
	}

	// ATI_separate_stencil / OpenGL 2.0 separate stencil
	qglConfig.isDoubleEdgeStencil = ( qglConfig.qglVersion >= 2.0f ) || R_CheckExtension( "GL_ATI_separate_stencil" );
	if ( qglConfig.isDoubleEdgeStencil ) {
		qglStencilOpSeparate = (PFNGLSTENCILOPSEPARATEATIPROC)GLimp_ExtensionPointer( "glStencilOpSeparate" );
		qglStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEATIPROC)GLimp_ExtensionPointer( "glStencilFuncSeparate" );
	}

	// GL_EXT_depth_bounds_test
 	qglConfig.depthBoundsTest = R_CheckExtension( "GL_EXT_depth_bounds_test" );
 	if ( qglConfig.depthBoundsTest ) {
 		qglDepthBoundsEXT = (PFNGLDEPTHBOUNDSEXTPROC)GLimp_ExtensionPointer( "glDepthBoundsEXT" );
 	}

	// GL_ARB_sync
	qglConfig.isSynchronized = R_CheckExtension( "GL_ARB_sync" ) &&
		// as of 5/24/2012 (driver version 15.26.12.64.2761) sync objects
		// do not appear to work for the Intel HD 4000 graphics
		( qglConfig.vendor != VENDOR_INTEL || r_skipIntelWorkarounds.GetBool() );
	if ( qglConfig.isSynchronized ) {
		qglFenceSync = (PFNGLFENCESYNCPROC)GLimp_ExtensionPointer( "glFenceSync" );
		qglIsSync = (PFNGLISSYNCPROC)GLimp_ExtensionPointer( "glIsSync" );
		qglClientWaitSync = (PFNGLCLIENTWAITSYNCPROC)GLimp_ExtensionPointer( "glClientWaitSync" );
		qglDeleteSync = (PFNGLDELETESYNCPROC)GLimp_ExtensionPointer( "glDeleteSync" );
	}

	// GL_ARB_occlusion_query
	qglConfig.occlusionQueryAvailable = R_CheckExtension( "GL_ARB_occlusion_query" );
	if ( qglConfig.occlusionQueryAvailable ) {
		// defined in GL_ARB_occlusion_query, which is required for GL_EXT_timer_query
		qglGenQueriesARB = (PFNGLGENQUERIESARBPROC)GLimp_ExtensionPointer( "glGenQueriesARB" );
		qglDeleteQueriesARB = (PFNGLDELETEQUERIESARBPROC)GLimp_ExtensionPointer( "glDeleteQueriesARB" );
		qglIsQueryARB = (PFNGLISQUERYARBPROC)GLimp_ExtensionPointer( "glIsQueryARB" );
		qglBeginQueryARB = (PFNGLBEGINQUERYARBPROC)GLimp_ExtensionPointer( "glBeginQueryARB" );
		qglEndQueryARB = (PFNGLENDQUERYARBPROC)GLimp_ExtensionPointer( "glEndQueryARB" );
		qglGetQueryivARB = (PFNGLGETQUERYIVARBPROC)GLimp_ExtensionPointer( "glGetQueryivARB" );
		qglGetQueryObjectivARB = (PFNGLGETQUERYOBJECTIVARBPROC)GLimp_ExtensionPointer( "glGetQueryObjectivARB" );
		qglGetQueryObjectuivARB = (PFNGLGETQUERYOBJECTUIVARBPROC)GLimp_ExtensionPointer( "glGetQueryObjectuivARB" );
	}

	// GL_ARB_timer_query
	qglConfig.isTimerQueryActive = R_CheckExtension( "GL_ARB_timer_query" ) || R_CheckExtension( "GL_EXT_timer_query" );
	if ( qglConfig.isTimerQueryActive ) {
		qglGetQueryObjectui64vEXT = (PFNGLGETQUERYOBJECTUI64VEXTPROC)GLimp_ExtensionPointer( "glGetQueryObjectui64vARB" );
		if ( qglGetQueryObjectui64vEXT == NULL ) {
			qglGetQueryObjectui64vEXT = (PFNGLGETQUERYOBJECTUI64VEXTPROC)GLimp_ExtensionPointer( "glGetQueryObjectui64vEXT" );
		}
	}

	// GL_ARB_debug_output
	qglConfig.debugOutput = R_CheckExtension( "GL_ARB_debug_output" );
	if ( qglConfig.debugOutput ) {
		qglDebugMessageControlARB   = (PFNGLDEBUGMESSAGECONTROLARBPROC)GLimp_ExtensionPointer( "glDebugMessageControlARB" );
		qglDebugMessageInsertARB    = (PFNGLDEBUGMESSAGEINSERTARBPROC)GLimp_ExtensionPointer( "glDebugMessageInsertARB" );
		qglDebugMessageCallbackARB  = (PFNGLDEBUGMESSAGECALLBACKARBPROC)GLimp_ExtensionPointer( "glDebugMessageCallbackARB" );
		qglGetDebugMessageLogARB    = (PFNGLGETDEBUGMESSAGELOGARBPROC)GLimp_ExtensionPointer( "glGetDebugMessageLogARB" );
		if ( r_debugContext.GetInteger() >= 1 ) {
			qglDebugMessageCallbackARB( DebugCallback, NULL );
		}
		if ( r_debugContext.GetInteger() >= 2 ) {
			// force everything to happen in the main thread instead of in a separate driver thread
			glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB );
		}
		if ( r_debugContext.GetInteger() >= 3 ) {
			// enable all the low priority messages
			qglDebugMessageControlARB( GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW_ARB, 0, NULL, true );
		}
	}

	// GL_ARB_multitexture
	if ( !qglConfig.multitextureAvailable ) {
		message( "GL_ARB_multitexture not available" );
	}
	// GL_ARB_texture_compression + GL_EXT_texture_compression_s3tc
	if ( !qglConfig.textureCompression ) {
		message( "GL_ARB_texture_compression or GL_EXT_texture_compression_s3tc not available" );
	}
	// GL_ARB_vertex_buffer_object
	if ( !qglConfig.vboAvailable ) {
		message( "GL_ARB_vertex_buffer_object not available" );
	}
	// GL_ARB_map_buffer_range
	if ( !qglConfig.mapBufferRangeAvailable ) {
		message( "GL_ARB_map_buffer_range not available" );
	}
	// GL_ARB_vertex_array_object
	if ( !qglConfig.isVertexArrayEnabled ) {
		message( "GL_ARB_vertex_array_object not available" );
	}
	// GL_ARB_draw_elements_base_vertex
	if ( !qglConfig.drawElementsBaseVertex ) {
		message( "GL_ARB_draw_elements_base_vertex not available" );
	}
	// GL_ARB_vertex_program / GL_ARB_fragment_program
	if ( !qglConfig.fragmentProgramOn ) {
		message( "GL_ARB_fragment_program not available" );
	}
	// GLSL
	if ( !qglConfig.enableGLsl ) {
		message( "GLSL not available" );
	}
	// GL_ARB_uniform_buffer_object
	if ( !qglConfig.uniformBufferEnabled ) {
		message( "GL_ARB_uniform_buffer_object not available" );
	}
	// GL_EXT_stencil_two_side
	if ( !qglConfig.isDoubleEdgeStencil ) {
		message( "GL_ATI_separate_stencil not available" );
	}

	// generate one global Vertex Array Object (VAO)
	qglGenVertexArrays( 1, &qglConfig.global_vao );
	qglBindVertexArray( qglConfig.global_vao );
}

static bool r_initialized = false;

R_IsInitialized() {
	return r_initialized;
}

R_GLCheckErrors() {
	int	err;
	char s[64];
	int i;

	for ( i = 0; i < 10; i++ ) {
		err = qglGetError();
		if ( err == GL_NO_ERROR ) {
			return;
		}
		switch ( err ) {
			case GL_INVALID_ENUM:
			strpcy( s, 'GL_INVALID_ENUM')
			break;
			case GL_INVALID_VALUE:
			strpcy( s, 'GL_INVALID_VALUE')
			break;
			case GL_INVALID_OPERATION:
			strpcy( s, 'GL_INVALID_OPERATION' )
			break;
			case GL_STACK_OVERFLOW:
			strpcy( s, 'GL_STACK_OVERFLOW')
			break;
			case GL_STACK_UNDERFLOW:
			strpcy( s, 'GL_STACK_UNDERFLOW')
			break;
			case GL_OUT_OF_MEMORY:
				strcpy( s, "GL_OUT_OF_MEMORY" );
				break;
			default:
			break;
		}
	}
}

void GL_SetCurrentTextureUnit( int unit ) {
	if ( qglState.currenttmu == unit ) {
		return;
	}

	if ( unit == 0 ) {
		qglActiveTextureARB( GL_TEXTURE0_ARB );
		//GLimp_LogComment( "glActiveTextureARB( GL_TEXTURE0_ARB )\n" );
		qglClientActiveTextureARB( GL_TEXTURE0_ARB );
		//GLimp_LogComment( "glClientActiveTextureARB( GL_TEXTURE0_ARB )\n" );
	} else if ( unit == 1 )   {
		qglActiveTextureARB( GL_TEXTURE1_ARB );
		//GLimp_LogComment( "glActiveTextureARB( GL_TEXTURE1_ARB )\n" );
		qglClientActiveTextureARB( GL_TEXTURE1_ARB );
		//GLimp_LogComment( "glClientActiveTextureARB( GL_TEXTURE1_ARB )\n" );
	} else {
		Error( ERR_DROP, "GL_SetCurrentTextureUnit: unit = %i", unit );
	}

	qglState.currenttmu = unit;
}


void R_InitOpenGL() {
	if ( !R_IsInitialized() ) {
		R_InitOpenGL();
	}

	int err = qglGetError();

	if ( err = GK==GL_NO_ERROR ) {
		printf( "qglGetError() = 0x%x\n" err );
	}
}

void R_ShutdownOpenGL() {
	//R_ShutdownFrameData();
	GLimp_Shutdown();
	r_initialized = false;
}

void GLimp_EnableLogging( bool enable ) {
}

/*
void R_Console_GLDraw( int x, int y, int w, int h) {
	qglPushAttrib( GL_ALL_ATTRIB_BITS );
	qglClearColor( 0.1f, 0.1f, 0.1f, 0.0f );
	qglScissor( 0, 0, w, h );
	qglClear( GL_COLOR_BUFFER_BIT );
	//renderSystem->BeginFrame( w, h );

	//console->Draw( true );

	//renderSystem->EndFrame( NULL, NULL );
	qglPopAttrib();
}*/

void GLDraw( int x, int y, int w, int h ) {
	GL_State( GLS_DEFAULT );
	qglViewport( x, y, w, h );
	qglScissor( x, y, w, h );

	qglMatrixMode( GL_PROJECTION );

	//float clearColor[] = { 0.1f, 0.1f, 0.1f, 0.0f };
	//glClearBufferfv( GL_COLOR, 0, clearColor );

	qglClearColor( 0.1f, 0.1f, 0.1f, 0.0f );
	qglClear( GL_COLOR_BUFFER_BIT );
	qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	qglLineWidth( 0.5 );
	qglColor3f( 1, 1, 1 )

	//globalImages->BindNull();

	qglBegin( GL_LINE_LOOP );
	qglColor3f( 1, 0, 0 );
	qglVertex2f( x + 3, y + 3 );
	qglColor3f( 0, 1, 0 );
	qglVertex2f( x + 3, h - 3 );
	qglColor3f( 0, 0, 1 );
	qglVertex2f ( w - 3, h - 3 );
	qglColor3f( 1, 1, 1 );
	qglVertex2f( w - 3, y + 3 );
	qglEnd();
}

/*
==========================================================================================

Some Math and matrix stuff for GL renderer

==========================================================================================
*/
void R_QGlMultMatrix( const float *a, const float *b, float *out ) {
	int i, j;

	for ( i = 0; i < 4; i++ ) {
		for ( j = 0; j < 4; j++ ) {
			out[ i * 4 + j ] =
				a [ i * 4 + 0 ] * b [ 0 * 4 + j ]
				+ a [ i * 4 + 1 ] * b [ 1 * 4 + j ]
				+ a [ i * 4 + 2 ] * b [ 2 * 4 + j ]
				+ a [ i * 4 + 3 ] * b [ 3 * 4 + j ];
		}
	}
}

/*void R_TransformClipToWindow( const arcVec4 clip, const viewDef *view, arcVec4 normalized, arcVec4 window ) {
	normalized[0] = clip[0] / clip[3];
	normalized[1] = clip[1] / clip[3];
	normalized[2] = ( clip[2] + clip[3] ) / ( 2 * clip[3] );

	window[0] = 0.5f * ( 1.0f + normalized[0] ) * view->viewportWidth;
	window[1] = 0.5f * ( 1.0f + normalized[1] ) * view->viewportHeight;
	window[2] = normalized[2];

	window[0] = ( int ) ( window[0] + 0.5 );
	window[1] = ( int ) ( window[1] + 0.5 );
}*/

void R_AxisToModelMatrix( const arcMat3 &axis, const arcVec3 &origin, float modelMatrix[16] ) {
	modelMatrix[0 * 4 + 0] = axis[0][0];
	modelMatrix[1 * 4 + 0] = axis[1][0];
	modelMatrix[2 * 4 + 0] = axis[2][0];
	modelMatrix[3 * 4 + 0] = origin[0];

	modelMatrix[0 * 4 + 1] = axis[0][1];
	modelMatrix[1 * 4 + 1] = axis[1][1];
	modelMatrix[2 * 4 + 1] = axis[2][1];
	modelMatrix[3 * 4 + 1] = origin[1];

	modelMatrix[0 * 4 + 2] = axis[0][2];
	modelMatrix[1 * 4 + 2] = axis[1][2];
	modelMatrix[2 * 4 + 2] = axis[2][2];
	modelMatrix[3 * 4 + 2] = origin[2];

	modelMatrix[0 * 4 + 3] = 0.0f;
	modelMatrix[1 * 4 + 3] = 0.0f;
	modelMatrix[2 * 4 + 3] = 0.0f;
	modelMatrix[3 * 4 + 3] = 1.0f;
}

void R_MatrixTranspose( const float in[16], float out[16] ) {
	for ( int i = 0; i < 4; i++ ) {
		for ( int j = 0; j < 4; j++ ) {
			out[i*4+j] = in[j*4+i];
		}
	}
}

void R_LocalPointToGlobal( const float modelMatrix[16], const arcVec3 &in, arcVec3 &out ) {
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 0] + in[2] * modelMatrix[2 * 4 + 0] + modelMatrix[3 * 4 + 0];
	out[1] = in[0] * modelMatrix[0 * 4 + 1] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 1] + modelMatrix[3 * 4 + 1];
	out[2] = in[0] * modelMatrix[0 * 4 + 2] + in[1] * modelMatrix[1 * 4 + 2] + in[2] * modelMatrix[2 * 4 + 2] + modelMatrix[3 * 4 + 2];
}

void R_GlobalPointToLocal( const float modelMatrix[16], const arcVec3 &in, arcVec3 &out ) {
	arcVec3 temp;

	temp[0] = in[0] - modelMatrix[3 * 4 + 0];
	temp[1] = in[1] - modelMatrix[3 * 4 + 1];
	temp[2] = in[2] - modelMatrix[3 * 4 + 2];

	out[0] = temp[0] * modelMatrix[0 * 4 + 0] + temp[1] * modelMatrix[0 * 4 + 1] + temp[2] * modelMatrix[0 * 4 + 2];
	out[1] = temp[0] * modelMatrix[1 * 4 + 0] + temp[1] * modelMatrix[1 * 4 + 1] + temp[2] * modelMatrix[1 * 4 + 2];
	out[2] = temp[0] * modelMatrix[2 * 4 + 0] + temp[1] * modelMatrix[2 * 4 + 1] + temp[2] * modelMatrix[2 * 4 + 2];
}

void R_LocalVecToGlobal( const float modelMatrix[16], const arcVec3 &in, arcVec3 &out ) {
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 0] + in[2] * modelMatrix[2 * 4 + 0];
	out[1] = in[0] * modelMatrix[0 * 4 + 1] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 1];
	out[2] = in[0] * modelMatrix[0 * 4 + 2] + in[1] * modelMatrix[1 * 4 + 2] + in[2] * modelMatrix[2 * 4 + 2];
}

void R_GlobalVecToLocal( const float modelMatrix[16], const arcVec3 &in, arcVec3 &out ) {
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[0 * 4 + 1] + in[2] * modelMatrix[0 * 4 + 2];
	out[1] = in[0] * modelMatrix[1 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[1 * 4 + 2];
	out[2] = in[0] * modelMatrix[2 * 4 + 0] + in[1] * modelMatrix[2 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 2];
}

void R_GlobalPlaneToLocal( const float modelMatrix[16], const arcPlane &in, arcPlane &out ) {
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[0 * 4 + 1] + in[2] * modelMatrix[0 * 4 + 2];
	out[1] = in[0] * modelMatrix[1 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[1 * 4 + 2];
	out[2] = in[0] * modelMatrix[2 * 4 + 0] + in[1] * modelMatrix[2 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 2];
	out[3] = in[0] * modelMatrix[3 * 4 + 0] + in[1] * modelMatrix[3 * 4 + 1] + in[2] * modelMatrix[3 * 4 + 2] + in[3];
}

void R_LocalPlaneToGlobal( const float modelMatrix[16], const arcPlane &in, arcPlane &out ) {
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 0] + in[2] * modelMatrix[2 * 4 + 0];
	out[1] = in[0] * modelMatrix[0 * 4 + 1] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 1];
	out[2] = in[0] * modelMatrix[0 * 4 + 2] + in[1] * modelMatrix[1 * 4 + 2] + in[2] * modelMatrix[2 * 4 + 2];
	out[3] = in[3] - modelMatrix[3 * 4 + 0] * out[0] - modelMatrix[3 * 4 + 1] * out[1] - modelMatrix[3 * 4 + 2] * out[2];
}

void R_LocalNormalToWorld( arcVec3 &local, arcVec3 modelMatrix ) {
	const arcMat3 &axis
	modelMatrix[0] = local[0] * axis[0][0] + local[1] * axis[1][0] + local[2] * axis[2][0];
	modelMatrix[1] = local[0] * axis[0][1] + local[1] * axis[1][1] + local[2] * axis[2][1];
	modelMatrix[2] = local[0] * axis[0][2] + local[1] * axis[1][2] + local[2] * axis[2][2];
}

void R_ScreenRectClear() {
	x1 = y1 = 32000;
	x2 = y2 = -32000;
	zMin = 0.0f;
	zmax = 1.0f;
}

void R_ScreenRectAddPoint( float x, float y ) {
	int	ix = arcMath::Ftoi( x );
	int iy = arcMath::Ftoi( y );

	if ( ix < x1 ) {
		x1 = ix;
	}
	if ( ix > x2 ) {
		x2 = ix;
	}
	if ( iy < y1 ) {
		y1 = iy;
	}
	if ( iy > y2 ) {
		y2 = iy;
	}
}

void R_ScreenRectExpand() {
	x1--;
	y1--;
	x2++;
	y2++;
}

// FIXME:
void R_ScreenRectIntersect( const r_screenRect &rect ) {
	if ( rect.x1 > x1 ) {
		x1 = rect.x1;
	}
	if ( rect.x2 < x2 ) {
		x2 = rect.x2;
	}
	if ( rect.y1 > y1 ) {
		y1 = rect.y1;
	}
	if ( rect.y2 < y2 ) {
		y2 = rect.y2;
	}
}
void R_ScreenRectUnion( const int &rect ) {
	if ( rect.x1 > x1 ) {
		x1 = rect.x1;
	}
	if ( rect.x2 < x2 ) {
		x2 = rect.x2;
	}
	if ( rect.y1 > y1 ) {
		y1 = rect.y1;
	}
	if ( rect.y2 < y2 ) {
		y2 = rect.y2;
	}
}

bool R_ScreenRectEquals( const r_screenRect &rect ) const {
	return ( x1 == rect.x1 && x2 == rect.x2 && y1 == rect.y1 && y2 == rect.y2 );
}

bool R_ScreenRectIsEmpty() const {
	return ( x1 > x2 || y1 > y2 );
}

void R_ShowColoredScreenRect( /*const r_screenRect &rect,*/ int colorIndex ) {
	if ( !R_ScreenRectIsEmpty() ) {
		static arcVec4 colors[] = { colorRed, colorGreen, colorBlue, colorYellow, colorMagenta, colorCyan, colorWhite, colorPurple };
	}
}