#include "../idlib/Lib.h"
#include <X11/X.h>
#pragma hdrstop

#include "tr_local.h"

static const int	FRAME_MEMORY_BYTES = 0x200000;
static const int	EXPAND_HEADERS = 1024;

anCVarSystem anVertexCache::r_showVertexCache( "r_showVertexCache", "0", CVAR_INTEGER|CVAR_RENDERER, "" );
anCVarSystem anVertexCache::r_vertexBufferMegs( "r_vertexBufferMegs", "32", CVAR_INTEGER|CVAR_RENDERER, "" );

anVertexCache		vertexCache;
#include "tr_local.h"

anCVar r_showBuffers( "r_showBuffers", "0", CVAR_INTEGER, "" );

/*
==================
IsWriteCombined
==================
*/
bool IsWriteCombined( void * base ) {
	MEMORY_BASIC_INFORMATION info;
	SIZE_T size = VirtualQueryEx( GetCurrentProcess(), base, &info, sizeof( info ) );
	if ( size == 0 ) {
		DWORD error = GetLastError();
		error = error;
		return false;
	}
	bool isWriteCombined = ( ( info.AllocationProtect & PAGE_WRITECOMBINE ) != 0 );
	return isWriteCombined;
}

/*
========================
anRenderCache::anRenderCache
========================
*/
anRenderCache::Init() {
	size = 0;
	// Initialize the frame buffer, vertex buffer, and VAO
	InitFrameBuffer();
	InitVertexBuffer();
	InitIndexBuffer();
	InitVertexArrayObject();
}

/*
=====================
R_InitFrameData
=====================
*/
void R_InitFrameData( void ) {
	ShutdownFrameData();

	frameData_t *frameData = (frameData_t *)Mem_ClearedAlloc( sizeof( *frameData ) ) ;
	frame = frameData;
	int size = MEMORY_BLOCK_SIZE;
	frameData_t *block = (frameData_t *)Mem_Alloc( size + sizeof( *block ) );

	if ( !block ) {
		common->FatalError( "R_InitFrameData: Mem_Alloc() failed" );
		//} else for ( int i = 0; i < NUM_FRAME_DATA; i++ ) {
			//smpFrameData[i].frameMemory = (byte *) Mem_Alloc16( MAX_FRAME_MEMORY, TAG_RENDER );
		//}
	}
	block->size = size;
	block->used = 0;
	block->next = nullptr;
	frame->memory = block;
	frame->memoryHighwater = 0;
	// must be set before calling R_ToggleSmpFrame()
	//frameData = &smpFrameData[0];

	R_ToggleSmpFrame();
}

/*
========================
anRenderCache::InitFrameBuffer
========================
*/
void anRenderCache::InitFrameBuffer() {
	// Create and configure the frame buffer
	if ( qglConfig.useVertexBufferObject ) {
		qglGenFramebuffers( 1, &frameBuffe r);
		qglBindFramebuffer( GL_FRAMEBUFFER, frameBuffer );
		// Configure frame buffer attachments and settings
		qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, vertexBuffer, 0 );
		//qglFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, indexBuffer, 0 );
		qglFramebufferRenderbuffer
	}
}

/*
========================
anRenderCache::InitVertexArrayObject
========================
*/
void anRenderCache::InitVertexArrayObject( const anDrawVertex *drawVerts ) {
	int vertSize, indexSize;
	int offset = 0;

	int vertArrayNum = 0;

	// Create and configure the VAO
	qglGenVertexArrays( 1, &vertexArrayObject );
	qglBindVertexArray( vertexArrayObject );
	// Set up vertex attribute pointers

	vertSize  = sizeof( drawVerts.xyz[0] );
	vertSize += sizeof( drawVerts.normal[0] );
	vertSize += sizeof( drawVerts.tangent[0] );
	vertSize += sizeof( drawVerts.color[0] );
	vertSize += sizeof( drawVerts.texCoords[0] );
	//vertSize += sizeof( drawVerts.lightCoords[0] );
	//vertSize += sizeof( drawVerts.lightdir[0] );
	vertSize *= SHADER_MAX_VERTEXES;

	indexesSize = sizeof( drawVerts.indexes[0] ) * SHADER_MAX_INDEXES;
	BindVertexArrayObject();
	GL_CheckErrors();
}

/*
========================
anRenderCache::InitVertexBuffer
========================
*/
void anRenderCache::InitVertexBuffer() {
	// Create and Set up vertex buffer data and configure the vertex buffer
	qglGenBuffers( 1, &vertexBuffer );
	qglBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );

	// Set up the vertex attributes
	qglEnableVertexAttribArray( 0 );
	qglVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof( GLfloat ), (GLfloat *)0 );
	qglEnableVertexAttribArray( 1 );
	qglVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof( GLfloat ), (GLfloat *)3 * sizeof( GLfloat ) );
	qglEnableVertexAttribArray(2);
	qglVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof( GLfloat ), (GLfloat *)6 * sizeof( GLfloat ) );

 	// Unbind the vertex buffer object
	qglBindBuffer( GL_ARRAY_BUFFER, 0 );
	qglGenBuffers( 1, &vertexBuffer );

	GL_CheckErrors();
}

/*
========================
anRenderCache::InitVertexBuffer
========================
*/
void AllocVertexArray() {
	buffered_t *indexSet;
	int *batchLength;
	queuedSurface_t *surf, *end = vcq.surfaces + vcq.numSurfaces;

	R_BindVao( vc.vao );

	// Search for a matching batch
	// FIXME: Use faster search
	indexSet = vc.surfaceIndexSets;
	batchLength = vc.batchLengths;
	for ( ; batchLength < vc.batchLengths + vc.numBatches; batchLength++ ) {
		if ( *batchLength == vcq.numSurfaces ) {
			buffered_t *indexSet2 = indexSet;
			for ( surf = vcq.surfaces; surf < end; surf++, indexSet2++) {
				if ( surf->indexes != indexSet2->data || (surf->numIndexes * sizeof( glIndex_t ) ) != indexSet2->size ) {
					break;
				}
			}

			if ( surf == end ) {
				break;
			}
		}


		indexSet += *batchLength;
	}

	// If found, use it
	if ( indexSet < vc.surfaceIndexSets + vc.numSurfaces ) {
		tess.firstIndex = indexSet->bufferOffset / sizeof( glIndex_t );
		//Printf(PRINT_ALL, "firstIndex %d numIndexes %d as %d\n", tess.firstIndex, tess.numIndexes, (int)(batchLength - vc.batchLengths));
		//Printf(PRINT_ALL, "vc.numSurfaces %d vc.numBatches %d\n", vc.numSurfaces, vc.numBatches );
	// If not, rebuffer the batch
	// FIXME: keep track of the vertexes so we don't have to reupload them every time
	} else {
		srfVert_t *dstVertex = vcq.vertexes;
		glIndex_t *dstIndex = vcq.indexes;

		batchLength = vc.batchLengths + vc.numBatches;
		*batchLength = vcq.numSurfaces;
		vc.numBatches++;

		tess.firstIndex = vc.indexOffset / sizeof(glIndex_t);
		vcq.vertexCommitSize = 0;
		vcq.indexCommitSize = 0;
		for ( surf = vcq.surfaces; surf < end; surf++ ) {
			glIndex_t *srcIndex = surf->indexes;
			int vertexesSize = surf->numVerts * sizeof( srfVert_t );
			int indexesSize = surf->numIndexes * sizeof( glIndex_t );
			int i, indexOffset = ( vc.vertexOffset + vcq.vertexCommitSize ) / sizeof( srfVert_t );

			Memcpy( dstVertex, surf->vertexes, vertexesSize );
			dstVertex += surf->numVerts;

			vcq.vertexCommitSize += vertexesSize;

			indexSet = vc.surfaceIndexSets + vc.numSurfaces;
			indexSet->data = surf->indexes;
			indexSet->size = indexesSize;
			indexSet->bufferOffset = vc.indexOffset + vcq.indexCommitSize;
			vc.numSurfaces++;

			for ( i = 0; i < surf->numIndexes; i++ ) {
				*dstIndex++ = *srcIndex++ + indexOffset;
			}
			vcq.indexCommitSize += indexesSize;
		}

		//ri.Printf(PRINT_ALL, "committing %d to %d, %d to %d as %d\n", vcq.vertexCommitSize, vc.vertexOffset, vcq.indexCommitSize, vc.indexOffset, (int)(batchLength - vc.batchLengths));

		if ( vcq.vertexCommitSize ) {
			qglBindBuffer( GL_ARRAY_BUFFER, vc.vao->vertexesVBO );
			qglBufferSubData( GL_ARRAY_BUFFER, vc.vertexOffset, vcq.vertexCommitSize, vcq.vertexes );
			vc.vertexOffset += vcq.vertexCommitSize;
		}

		if ( vcq.indexCommitSize ) {
			qglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vc.vao->indexesIBO );
			qglBufferSubData( GL_ELEMENT_ARRAY_BUFFER, vc.indexOffset, vcq.indexCommitSize, vcq.indexes );
			vc.indexOffset += vcq.indexCommitSize;
		}
	}
}

/*
========================
anRenderCache::InitIndexBuffer
========================
*/
void anRenderCache::InitIndexBuffer() {
}
void anRenderCache::AddDepthBuffer( GLuint format ) {
	depthFormat = format;
	bool notCreatedYet = depthBuffer == 0;
	if ( notCreatedYet ) {
		qglGenRenderbuffers( 1, &depthBuffer );
	}

	qglBindRenderbuffer( GL_RENDERBUFFER, depthBuffer );

	if ( useMsaa ) {
		qglRenderbufferStorageMultisample( GL_RENDERBUFFER, msaaSamples, format, width, height );
	} else {
		qglRenderbufferStorage( GL_RENDERBUFFER, format, width, height );
	}

	qglBindRenderbuffer( GL_RENDERBUFFER, 0 );

	if ( notCreatedYet ) {
		qglBindFramebuffer( GL_FRAMEBUFFER, frameBuffer );
		qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer );
	}
}

void anRenderCache::AddDepthStencilBuffer( GLuint format ) {
	depthFormat = format;
	AddDepthBuffer( depthFormat );

	bool qglConfig = stencilBuffer == 0;

	if ( notCreatedYet ) {
		qglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthBuffer );
		stencilBuffer = depthBuffer;
		stencilFormat = depthFormat;
	}
}
/*
========================
anRenderCache::AllocVertexBuffer
========================
*/
void anRenderCache::AllocVertexBuffer( GLuint size ) {
	qglGenBuffers( 1, &vertexBuffer );
	qglBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );

	// Set up the vertex attributes
	qglEnableVertexAttribArray( 0 );
	qglVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, size * sizeof( GLfloat ), (GLfloat *)0 );
	qglEnableVertexAttribArray(  1);
	qglVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, size * sizeof( GLfloat ), (GLfloat *)size * sizeof( GLfloat ) );
	qglEnableVertexAttribArray( 2 );
	qglVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, size * sizeof( GLfloat ), (GLfloat *)2 * size * sizeof( GLfloat ) );

	   // Unbind the vertex buffer object
	qglBindBuffer( GL_ARRAY_BUFFER, 0 );
}

/*
=================
R_StaticAlloc
=================
*/
void *anRenderCache::StaticAlloc( int bytes ) {
	tr.pc.c_alloc++;
	tr.staticAllocCount += bytes;

	void *buf = Mem_Alloc( bytes );

	// don't exit on failure on zero length allocations since the old code didn't
	if ( !buf && ( bytes != 0 ) ) {
		common->FatalError( "R_StaticAlloc failed on %i bytes", bytes );
	}
	return buf;
}

/*
=================
R_StaticFree
=================
*/
void anRenderCache::StaticFree( void *data ) {
	tr.pc.c_free++;
	Mem_Free( data );
}

/*
================
R_FrameAlloc

This data will be automatically freed when the
current frame's back end completes.

This should only be called by the front end.  The
back end shouldn't need to allocate memory.

If we passed smpFrame in, the back end could
alloc memory, because it will always be a
different frameData than the front end is using.

All temporary data, like dynamic tesselations
and local spaces are allocated here.

The memory will not move, but it may not be
contiguous with previous allocations even
from this frame.

The memory is NOT zero filled.
Should part of this be inlined in a macro?
================
*/
void *anRenderCache::FrameAlloc( int bytes ) {
	bytes = ( bytes+16 ) & ~15;
	// see if it can be satisfied in the current block
	frameData_t *frame = frameData;
	frameData_t *block = frame->alloc;

	if ( block->size - block->used >= bytes ) {
		void *buf = block->base + block->used;
		block->used += bytes;
		return buf;
	}

	// advance to the next memory block if available
	block = block->next;

	// create a new block if we are at the end of
	// the chain
	if ( !block ) {
		int size = MEMORY_BLOCK_SIZE;
		block = (frameData_t *)Mem_Alloc( size + sizeof(* block) );
		if ( !block ) {
			common->FatalError( "R_FrameAlloc: Mem_Alloc failed" );
		}
		block->size = size;
		block->used = 0;
		block->next = nullptr;
		frame->alloc->next = block;
	}

	// we could fix this if we needed to...
	if ( bytes > block->size ) {
		common->FatalError( "R_FrameAlloc of %i exceeded MEMORY_BLOCK_SIZE", bytes );
	}

	frame->alloc = block;
	block->used = bytes;

	return block->base;
}

/*
========================
anRenderCache::BindFrameBuffer
========================
*/
void anRenderCache::BindFrameBuffer() {
	// Bind the frame buffer
	qglBindFramebuffer( GL_FRAMEBUFFER, frameBuffer );
	//BindVertexBuffer();
	//BindIndexBuffer();BindVertexArrayObject();
}

/*
========================
anRenderCache::BindVertexBuffer
========================
*/
void anRenderCache::BindVertexBuffer( GLfloat *vertices, int numVerts/*, GLfloat size, anRenderCache *buf*/ ) {
	// Bind the vertex buffer
   qglGenBuffers( 1, &buffer );
   qglBindBuffer( GL_ARRAY_BUFFER, vertexBuffer);
   qglBufferData( GL_ARRAY_BUFFER, sizeof( GLfloat ) * numVerts, vertices, GL_STATIC_DRAW );
   return buffer;
}

/*
========================
anRenderCache::BindIndexBuffer
========================
*/
void anRenderCache::BindIndexBuffer( GLint numIndices ) {
	qglGenBuffers( 1, &indexBuffer );
	qglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );
	qglBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( GLuint ) * numIndices, indices, GL_STATIC_DRAW );

	qglDrawElements( GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0 );
}

/*
========================
anRenderCache::BindVertexArrayObject
========================
*/
void anRenderCache::BindVertexArrayObject() {
	if ( !vao ) {
		//R_BindNullVao();
		Error( "BindVertexArrayObject: nullptr vao");
		return;
	}

	if ( r_logFile->integer) {
		// don't just call LogComment, or we will get a call to va() every frame!
		GLimp_LogComment(va("--- R_BindVao( %s ) ---\n", vao->name));
	}

	if ( qglState.currentVao != vao ) {
		qglState.currentVao = vao;
		qglState.vertexAttribsInterpolation = 0;
		qglState.vertexAnimation = false;
		backEnd.pc.c_vaoBinds++;
		if ( qglConfig.vertexArrayObject ) {
			qglBindVertexArray( vao->vao );
			// Intel Graphics doesn't save GL_ELEMENT_ARRAY_BUFFER binding with VAO binding.
			if ( qglRefConfig.intelGraphics || vao == tess.vao ) {
				qglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vao->indexesIBO );
			}
			// tess VAO always has buffers bound
			if ( vao == dominantTri_s.vao )
				qglBindBuffer( GL_ARRAY_BUFFER, vao->vertexesVBO );
		} else {
			qglBindBuffer( GL_ARRAY_BUFFER, vao->vertexesVBO );
			qglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vao->indexesIBO );

		// tess VAO doesn't have vertex pointers set until data is uploaded
		if ( vao != tess.vao ) {
			Vao_SetVertexPointers( vao );

		}
	}
}
	// Bind the VAO
	qglBindVertexArray( vertexArrayObject );
}

/*
============
R_VaoList_f
============
*/
void R_VaoList_f( void ) {
	vao_t          *vao;
	int             vertexesSize = 0;
	int             indexesSize = 0;

	ri.Printf( " size          name\n");
	ri.Printf( "----------------------------------------------------------\n" );

	for ( int i = 0; i < tr.numVaos; i++ ) {
		vao = tr.vaos[i];
		RB_Printf( "%d.%02d MB %s\n", vao->vertexesSize / ( 1024 * 1024 ), ( vao->vertexesSize % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ), vao->name );
		vertexesSize += vao->vertexesSize;
	}

	for ( i = 0; i < tr.numVaos; i++ ) {
		vao = tr.vaos[i];

		RB_Printf( "%d.%02d MB %s\n", vao->indexesSize / ( 1024 * 1024 ), ( vao->indexesSize % ( 1024 * 1024 ) ) * 100 / (1 024 * 1024 ), vao->name );

		indexesSize += vao->indexesSize;
	}

	RB_Printf( " %i total VAOs\n", tr.numVaos );
	RB_Printf( " %d.%02d MB total vertices memory\n", vertexesSize / ( 1024 * 1024 ), ( vertexesSize % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ) );
	RB_Printf( " %d.%02d MB total triangle indices memory\n", indexesSize / ( 1024 * 1024 ), ( indexesSize % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ) );
}

void Framebuffer::Error( int status ) {
	if ( status == GL_FRAMEBUFFER_COMPLETE ) {
		return;
	}

	// something went wrong
	switch ( status ) {
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, incomplete attachment", fboName.c_str() );
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, missing attachment", fboName.c_str() );
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, missing draw buffer", fboName.c_str() );
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, missing read buffer", fboName.c_str() );
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, missing layer targets", fboName.c_str() );
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		common->Error( "Framebuffer::Check( %s ): Framebuffer incomplete, missing multisample", fboName.c_str() );
		break;

	case GL_FRAMEBUFFER_UNSUPPORTED:
		common->Error( "Framebuffer::Check( %s ): Unsupported framebuffer format", fboName.c_str() );
		break;

	default:
		common->Error( "Framebuffer::Check( %s ): Unknown error 0x%X", fboName.c_str(), status );
		break;
	};
}

/*
====================
R_ToggleSmpFrame
====================
*/
void anRenderCache::ToggleSmpFrame( void ) {
	if ( r_lockSurfaces.GetBool() ) {
		return;
	}
	R_FreeDeferredTriSurfs( frameData );
	// update the highwater mark
	anRenderCache::CountFrameData();

	frameData_t *frame = frameData;

	// reset the memory allocation to the first block
	frameData->alloc = frameData->memory;

	// clear all the blocks
	for ( frameData_t *block = frame->memory; block; block = block->next ) {
		block->used = 0;
	}

	R_ClearCommandChain();
}

/*
========================
anRenderCache::UnbindFrameBuffer
========================
*/
void anRenderCache::UnbindFrameBuffer() {
	// Unbind the frame buffer
	qglBindFramebuffer( GL_FRAMEBUFFER, 0 ) ;
}

/*
========================
anRenderCache::UnbindVertexBuffer
========================
*/
void anRenderCache::UnbindVertexBuffer() {
	// Unbind the vertex buffer
	qglBindBuffer( GL_ARRAY_BUFFER, 0 );
}

/*
========================
anRenderCache::UnbindIndexBuffer
========================
*/
void anRenderCache::UnbindIndexBuffer() {
	qglGenBuffers( 0, &indexBuffer );
	qglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );
	qglBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( GLuint ) * numIndices, indices, GL_STATIC_DRAW );

	qglDrawElements( GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0 );
}

/*
========================
anRenderCache::UnbindVertexArrayObject
========================
*/
void anRenderCache::UnbindVertexArrayObject() {
	// Unbind the VAO
	qglBindVertexArray( 0 );
}

/*
========================
anRenderCache::
========================
*/
void UnbindAllBufferObjects() {

}

/*
========================
anRenderCache::ActuallyFreeVertexBuffers
========================
*/
void anRenderCache::ActuallyFreeVertexBuffers() {
	// Ensure the memory deallocation is only performed when it is actually needed.
	// This can help with avoiding unnecessary deallocations.
	if ( vertexBuffer.base == nullptr ) {
		if ( r_useWindows ) {
			VirtualFree( vertexBuffer.base );
		} else if ( !IsWriteCombined( vertexBuffer.base ) ) {
			Mem_Free( vertexBuffer.base ); // Use your own memory deallocation function
		}
		return;
	}

	vertexBuffer.base = nullptr;
	vertexBuffer.size = 0;
	vertexBuffer.buffer = 0;//= verterxCache->ActuallyFree( vertexBuffer.base );

	qglBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );

	// Set up the vertex attributes
	//qglEnableVertexAttribArray( 0 );
//	qglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLfloat*)0);
	//qglEnableVertexAttribArray( 1 );
	//qglVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLfloat*)3 * sizeof(GLfloat));
	//qglEnableVertexAttribArray(2);
	///qglVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLfloat*)6 * sizeof(GLfloat));

	   // Unbind the vertex buffer object
	qglBindBuffer( GL_ARRAY_BUFFER, 0 );
}

/*
========================
anRenderCache::DeleteFrameBuffer
========================
*/
void anRenderCache::DeleteFrameBuffer() {
	// Delete the frame buffer
	qglDeleteFramebuffers( 1, &frameBuffer );
}

/*
========================
anRenderCache::DeleteVertexBuffer
========================
*/
void anRenderCache::DeleteVertexBuffer() {
	// Delete the vertex buffer
	qglDeleteBuffers( 1, &vertexBuffer );
}

/*
========================
anRenderCache::DeleteIndexBuffer
========================
*/
void anRenderCache::DeleteIndexBuffer() {
}

/*
========================
anRenderCache::DeleteVertexArrayObject
========================
*/
void anRenderCache::DeleteVertexArrayObject() {
	// Delete the VAO
	qglDeleteVertexArrays( 1, &vertexArrayObject );
}

/*
================
R_RenderView

A view may be either the actual camera view,
a mirror / remote location, or a 3D view on a gui surface.

Parms will typically be allocated with R_FrameAlloc
================
*/
void anRenderCache::RenderView( viewDef_t *parms ) {
	if ( parms->renderView.width <= 0 || parms->renderView.height <= 0 ) {
		return;
	}
	// Bind the frame buffer, vertex buffer, and VAO
	BindFrameBuffer();
	BindVertexBuffer();
	BindIndexBuffer();
	BindVertexArrayObject();

	// Perform rendering here

	// Unbind the frame buffer, vertex buffer, and VAO
	UnbindFrameBuffer();
	UnbindVertexBuffer();
	UnbindVertexArrayObject();
	tr.viewCount++;
	viewDef_t* oldView = tr.viewDef;	// save view in case we are a subview
	tr.viewDef = parms;
	tr.sortOffset = 0;

	// set the matrix for world space to eye space
	R_SetViewMatrix( tr.viewDef );

	// we need to set the projection matrix before doing
	// portal-to-screen scissor calculations
	R_SetupProjectionMatrix( tr.viewDef );//R_SetupViewFrustum();

	// constrain the view frustum to the view lights and entities
	//R_ConstrainViewFrustum();

	// setup render matrices for faster culling
	ARCRenderMatrix::Transpose( *(ARCRenderMatrix *)tr.viewDef->projectionMatrix, tr.viewDef->projectionRenderMatrix );
	ARCRenderMatrix viewRenderMatrix;
	ARCRenderMatrix::Transpose( *(ARCRenderMatrix *)tr.viewDef->worldSpace.modelViewMatrix, viewRenderMatrix );
	ARCRenderMatrix::Multiply( tr.viewDef->projectionRenderMatrix, viewRenderMatrix, tr.viewDef->worldSpace.mvp );

	// the planes of the view frustum are needed for portal visibility culling
	ARCRenderMatrix::GetFrustumPlanes( tr.viewDef->frustum, tr.viewDef->worldSpace.mvp, false, true );

	// frustum planes used to point outside the frustum
	for ( int i = 0; i < 6; i++ ) {
		tr.viewDef->frustum[i] = - tr.viewDef->frustum[i];
	}
	// remove the Z-near to avoid portals from being near clipped
	tr.viewDef->frustum[4][3] -= r_znear.GetFloat();

	// identify all the visible portal areas, and create view lights and view entities
	// for all the the entityDefs and lightDefs that are in the visible portal areas
	static_cast<anRenderWorldLocal *>( parms->renderWorld )->FindViewLightsAndEntities();

	// wait for any shadow volume jobs from the previous frame to finish
	tr.frontEndJobList->Wait();

	// make sure that interactions exist for all light / entity combinations
	// that are visible
	// add any pre-generated light shadows, and calculate the light shader values
	R_AddLightSurfaces();//R_AddLights();

	// adds ambient surfaces and create any necessary interaction surfaces to add to the light
	// lists
	R_AddModelSurfaces();//R_AddModels();

	// build up the GUIs on world surfaces
	R_AddInGameGuis( tr.viewDef->drawSurfs, tr.viewDef->numDrawSurfs );

	// any viewLight that didn't have visible surfaces can have it's shadows removed
	R_RemoveUnecessaryViewLights();//R_OptimizeViewLightsList();

	// sort all the ambient surfaces for translucency ordering
	R_QSortDrawSurfs();

	// generate any subviews (mirrors, cameras, etc) before adding this view
	if ( R_GenerateSubViews( tr.viewDef->drawSurfs, tr.viewDef->numDrawSurfs ) ) {
	//if ( R_GenerateSubViews() ) {
		// if we are debugging subviews, allow the skipping of the
		// main view draw
		if ( r_subviewOnly.GetBool() ) {
			return;
		}
	}

	//static_cast<anRenderWorldLocal *>( parms->renderWorld )->WriteVisibleDefs( tr.viewDef );

	// add the rendering commands for this viewDef
	R_AddDrawViewCmd( parms );

	// restore view in case we are a subview
	tr.viewDef = oldView;
}

/*
==============
R_ListVertexCache_f
==============
*/
static void anRenderCache::ListVertexCache_f( const anCommandArgs &args ) {
	vertexCache.List();
}

#define	MEMORY_BLOCK_SIZE	0x100000

/*
=====================
R_ShutdownFrameData
=====================
*/
void anRenderCache::ShutdownFrameData( void ) {
	// free any current data
	frameData_t *frame = frameData;
	if ( !frame && qglConfig.useFramebufferObject ) {
		if ( !frame ) {
			return;
		}
	}
	//if ( r_useARBpath.GetBool() ) {
		//qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
	//	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
	//} else {
		//qglBindBuffer( GL_ARRAY_BUFFER, 0 );
		//qglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	//}
	//UnbindFrameBuffer();
	// free all the deferred triangle surfaces
	R_FreeDeferredTriSurfs( frame );
	//Mem_Free( frame );
	frameData = nullptr;

	// free all the memory blocks
	for ( frameData_t *block = frame->memory; block; block = frameData_t *nextBlock ) {
		nextBlock = block->next;
		Mem_Free( block );
	} else for ( int i = 0; i < NUM_FRAME_DATA; i++ ) {
		Mem_Free16( smpFrameData[i].frameMemory );
		smpFrameData[i].frameMemory = nullptr;
	}
}

/*
================
R_CountFrameData
================
*/
int anRenderCache::CountFrameData( void ) {
	int count = 0;
	frameData_t *frame = frameData;
	for ( frameData_t * lock = frame->memory; block; block = block->next ) {
		count += block->used;
		if ( block == frame->alloc ) {
			break;
		}
	}

	// note if this is a new highwater mark
	if ( count > frame->memoryHighwater ) {
		frame->memoryHighwater = count;
	}

	return count;
}

/*
==================
R_ClearedFrameAlloc
==================
*/
void *anRenderCache::ClearedFrameAlloc( int bytes ) {
	void *r = R_FrameAlloc( bytes );
	SIMDProcessor->Memset( r, 0, bytes );
	return r;
}

/*
=================
R_ClearedStaticAlloc
=================
*/
void *anRenderCache::ClearedStaticAlloc( int bytes ) {
	void *buf = R_StaticAlloc( bytes );
	SIMDProcessor->Memset( buf, 0, bytes );
	return buf;
}

/*
==================
R_FrameFree

This does nothing at all, as the frame data is reused every frame
and can only be stack allocated.

The only reason for it's existance is so functions that can
use either static or frame memory can set function pointers
to both alloc and free.
==================
*/
void anRenderCache::FrameFree( void *data ) {
	// free the frame data
}

/*
==============
anVertexCache::ActuallyFree
==============
*/
void anVertexCache::ActuallyFree( vertCache_t *block ) {
	if ( !block ) {
		common->Error( "[VertCache Free]: nullptr pointer" );
	}

	if ( block->user ) {
		// let the owner know we have purged it
		*block->user = nullptr;
		block->user = nullptr;
	}

	// temp blocks are in a shared space that won't be freed
	if ( block->tag != TAG_TEMP ) {
		staticAllocTotal -= block->size;
		staticCountTotal--;
		if ( block->vbo ) {
			// this isn't really necessary, it will be reused soon enough
			// filling with zero length data is the equivalent of freeing
			//qglBindBufferARB( GL_ARRAY_BUFFER_ARB, block->vbo );
			//qglBufferDataARB( GL_ARRAY_BUFFER_ARB, 0, 0, GL_DYNAMIC_DRAW_ARB );
			qglNamedBufferData( block->vbo, 0, nullptr, GL_DYNAMIC_DRAW )
		} else if ( block->virtMem ) {
			Mem_Free( block->virtMem );
			block->virtMem = nullptr;
		}
	}
	block->tag = TAG_FREE;		// mark as free

	// unlink stick it back on the free list
	block->next->prev = block->prev;
	block->prev->next = block->next;

	// stick it on the front of the free list so it will be reused immediately
	block->next = freeStaticHeaders.next;
	block->prev = &freeStaticHeaders;

	// stick it on the back of the free list so it won't be reused soon (just for debugging)
	//block->next = &freeStaticHeaders;
	//block->prev = freeStaticHeaders.prev;

	block->next->prev = block;
	block->prev->next = block;
}

/*
==============
anVertexCache::Position

this will be a real pointer with virtual memory,
but it will be an int offset cast to a pointer with
ARB_vertex_buffer_object

The ARB_vertex_buffer_object will be bound
==============
*/
void *anVertexCache::Position( vertCache_t *buffer ) {
	if ( !buffer || buffer->tag == TAG_FREE ) {
		common->FatalError( "anVertexCache::Position: bad vertCache_t" );
	}

	// the ARB vertex object just uses an offset
	if ( buffer->vbo ) {
		if ( r_showVertexCache.GetInteger() == 2 ) {
			if ( buffer->tag == TAG_TEMP ) {
				common->Printf( "GL_ARRAY_BUFFER_ARB = %i + %i (%i bytes)\n", buffer->vbo, buffer->offset, buffer->size );
			} else {
				common->Printf( "GL_ARRAY_BUFFER_ARB = %i (%i bytes)\n", buffer->vbo, buffer->size );
			}
		}
		if ( buffer->indexBuffer ) {
			qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, buffer->vbo );
		} else {
			qglBindBufferARB( GL_ARRAY_BUFFER_ARB, buffer->vbo );
		}
		return (void *)buffer->offset;
	}

	// virtual memory is a real pointer
	return (void *)((byte *)buffer->virtMem + buffer->offset);
}

void anVertexCache::UnbindIndex() {
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
}


//================================================================================

/*
===========
anVertexCache::Init
===========
*/
void anVertexCache::Init() {
	cmdSystem->AddCommand( "listVertexCache", R_ListVertexCache_f, CMD_FL_RENDERER, "lists vertex cache" );

	if ( r_vertexBufferMegs.GetInteger() < 8 ) {
		r_vertexBufferMegs.SetInteger( 8 );
	}

	virtualMemory = false;

	// use ARB_vertex_buffer_object unless explicitly disabled
	if ( r_useVertexBuffers.GetInteger() && qglConfig.useVertexBufferObject ) {
		common->Printf( "using ARB_vertex_buffer_object memory\n" );
		qglConfig.useVertexBufferObject = true;

		qglGenBuffers( 1, &qglConfig.useVertexBufferObject );
		qglBindBuffer( GL_ARRAY_BUFFER, qglConfig.useVertexBufferObject );
        // Allocate buffer memory and upload data to the VBO
        qglBufferData( GL_ARRAY_BUFFER, bufferSizeInBytes, nullptr, GL_DYNAMIC_DRAW );
		qglEnableVertexAttribArray( attributeIndex )
		qglVertexAttribPointer( attributeIndex, numComponants, GL_FLOAT, GL_FALSE, strideInBytes, nullptr)
	} else {
		virtualMemory = true;
		r_useIndexBuffers.SetBool( false );
		common->Printf( "WARNING: vertex array range in virtual memory (SLOW)\n" );
	}
	//if ( i ) {
		///freeStaticHeaderss[i].next = freeStaticHeaderss[i].prev = &freeStaticHeaderss[i];
		//staticHeaderss[i].next = staticHeaderss[i].prev = &staticHeaderss[i];
	//}

	// initialize the cache memory blocks
	freeStaticHeaders.next = freeStaticHeaders.prev = &freeStaticHeaders;
	staticHeaders.next = staticHeaders.prev = &staticHeaders;
	freeDynamicHeaders.next = freeDynamicHeaders.prev = &freeDynamicHeaders;
	dynamicHeaders.next = dynamicHeaders.prev = &dynamicHeaders;
	deferredFreeList.next = deferredFreeList.prev = &deferredFreeList;

	// set up the dynamic frame memory
	frameBytes = FRAME_MEMORY_BYTES;
	staticAllocTotal = 0;

	byte *junk = (byte *)Mem_Alloc( frameBytes );
	for ( int i = 0; i < NUM_VERTEX_FRAMES; i++ ) {
		allocatingTempBuffer = true;	// force the alloc to use GL_STREAM_DRAW_ARB
		Alloc( junk, frameBytes, &tempBuffers[i] );
		allocatingTempBuffer = false;
		tempBuffers[i]->tag = TAG_FIXED;
		// unlink these from the static list, so they won't ever get purged
		tempBuffers[i]->next->prev = tempBuffers[i]->prev;
		tempBuffers[i]->prev->next = tempBuffers[i]->next;
	}
	Mem_Free( junk );
	EndFrame();
}

/*
===========
anVertexCache::PurgeAll

Used when toggling vertex programs on or off, because
the cached data isn't valid
===========
*/
void anVertexCache::PurgeAll() {
	//( i ) {
		//vertCache_t &staticHeaders = staticHeaderss[i];
	while ( staticHeaders.next != &staticHeaders ) {
		ActuallyFree( staticHeaders.next );
	}
}

/*
===========
anVertexCache::Shutdown
===========
*/
void anVertexCache::Shutdown() {
//	PurgeAll();	// !@#: also purge the temp buffers
	headerAllocator.Shutdown();
}

void anVertexCache::BeginBackEnd() {
	mostUsedVertex = Max( mostUsedVertex, frameData[listNum].vertexMemUsed.GetValue() );
	mostUsedIndex = Max( mostUsedIndex, frameData[listNum].indexMemUsed.GetValue() );
	if ( r_showVertexCache.GetBool() ) {
		idLib::Printf( "%08d: %d allocations, %dkB vertex, %dkB index : %dkB vertex, %dkB index\n",
					   currentFrame, frameData[listNum].allocations,
					   frameData[listNum].vertexMemUsed.GetValue() / 1024,
					   frameData[listNum].indexMemUsed.GetValue() / 1024,
					   mostUsedVertex / 1024, mostUsedIndex / 1024 );
	}

	if ( endUnmap - startUnmap > 1 ) {
		ANLib::PrintfIf( r_showVertexCacheTimings.GetBool(), "idVertexCache::unmap took %i msec\n", endUnmap - startUnmap );
	}
	drawListNum = listNum;
	// prepare the next frame for writing to by the CPU
	currentFrame++;
	listNum = currentFrame % VERTCACHE_NUM_FRAMES;

	const int startBind = Sys_Milliseconds();
	qglBindBuffer( GL_ARRAY_BUFFER, ( GLuint )frameData[drawListNum].vertexBuffer.GetAPIObject() );
	qglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ( GLuint )frameData[drawListNum].indexBuffer.GetAPIObject() );
	const int endBind = Sys_Milliseconds();
	if ( endBind - startBind > 1 ) {
		anLib::Printf( "VertexCache::bind took %i msec\n", endBind - startBind );
	}
}
/*
===========
anVertexCache::Alloc
===========
*/
void anVertexCache::Alloc( void *data, int size, vertCache_t **buffer, bool indexBuffer ) {
	vertCache_t	*block;

	if ( size <= 0 ) {
		common->Error( "anVertexCache::Alloc: size = %i\n", size );
	}

	// if we can't find anything, it will be nullptr
	*buffer = nullptr;

	// if we don't have any remaining unused headers, allocate some more
	if ( freeStaticHeaders.next == &freeStaticHeaders ) {
		for ( int i = 0; i < EXPAND_HEADERS; i++ ) {
			block = headerAllocator.Alloc();
			block->next = freeStaticHeaders.next;
			block->prev = &freeStaticHeaders;
			block->next->prev = block;
			block->prev->next = block;
			if ( !virtualMemory ) {
				qglGenBuffersARB( 1, & block->vbo );
			}
		}
	}

	// move it from the freeStaticHeaders list to the staticHeaders list
	block = freeStaticHeaders.next;
	block->next->prev = block->prev;
	block->prev->next = block->next;
	block->next = staticHeaders.next;
	block->prev = &staticHeaders;
	block->next->prev = block;
	block->prev->next = block;

	block->size = size;
	block->offset = 0;
	block->tag = TAG_USED;

	// save data for debugging
	staticAllocThisFrame += block->size;
	staticCountThisFrame++;
	staticCountTotal++;
	staticAllocTotal += block->size;

	// this will be set to zero when it is purged
	block->user = buffer;
	*buffer = block;

	// allocation doesn't imply used-for-drawing, because at level
	// load time lots of things may be created, but they aren't
	// referenced by the GPU yet, and can be purged if needed.
	block->frameUsed = currentFrame - NUM_VERTEX_FRAMES;
	block->indexBuffer = indexBuffer;

	// copy the data
	if ( block->vbo ) {
		if ( indexBuffer ) {
			qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, block->vbo );
			qglBufferDataARB( GL_ELEMENT_ARRAY_BUFFER_ARB, ( GLsizeiptrARB)size, data, GL_STATIC_DRAW_ARB );
		} else {
			qglBindBufferARB( GL_ARRAY_BUFFER_ARB, block->vbo );
			if ( allocatingTempBuffer ) {
				qglBufferDataARB( GL_ARRAY_BUFFER_ARB, ( GLsizeiptrARB )size, data, GL_STREAM_DRAW_ARB );
			} else {
				qglBufferDataARB( GL_ARRAY_BUFFER_ARB, ( GLsizeiptrARB )size, data, GL_STATIC_DRAW_ARB );
			}
		}
	} else {
		block->virtMem = Mem_Alloc( size );
		SIMDProcessor->Memcpy( block->virtMem, data, size );
	}
}

/*
===========
anVertexCache::AllocStaticVertex
===========
*/
vertCacheHandle_t anVertexCache::AllocStaticVertex( const void *data, int bytes ) {
	if ( staticData.vertexMemUsed.GetValue() + bytes > STATIC_VERTEX_MEMORY ) {
		anLib::FatalError( "anVertexCache::Alloc: size = %i update or increase STATIC_VERTEX_MEMORY\n ", bytes );
	}
	return ActuallyAlloc( staticData, data, bytes, CACHE_VERTEX );
}

/*
===========
anVertexCache::AllocStaticIndex
===========
*/
vertCacheHandle_t anVertexCache::AllocStaticIndex( const void *data, int bytes ) {
	if ( staticData.indexMemUsed.GetValue() + bytes > STATIC_INDEX_MEMORY ) {
		anLib::FatalError( "AllocStaticIndex failed, increase STATIC_INDEX_MEMORY" );
	}
	return ActuallyAlloc( staticData, data, bytes, CACHE_INDEX );
}

/*
===========
anVertexCache::Touch
===========
*/
void anVertexCache::Touch( vertCache_t *block ) {
	if ( !block ) {
		common->Error( "anVertexCache Touch: nullptr pointer" );
	}

	if ( block->tag == TAG_FREE ) {
		common->FatalError( "anVertexCache Touch: freed pointer" );
	}
	if ( block->tag == TAG_TEMP ) {
		common->FatalError( "anVertexCache Touch: temporary pointer" );
	}

	block->frameUsed = currentFrame;

	// move to the head of the LRU list
	block->next->prev = block->prev;
	block->prev->next = block->next;

	block->next = staticHeaders.next;
	block->prev = &staticHeaders;

	staticHeaders.next->prev = block;
	staticHeaders.next = block;
}

/*
===========
anVertexCache::Free
===========
*/
void anVertexCache::Free( vertCache_t *block ) {
	if ( !block ) {
		return;
	}

	if ( block->tag == TAG_FREE ) {
		common->FatalError( "anVertexCache Free: freed pointer" );
	}
	if ( block->tag == TAG_TEMP ) {
		common->FatalError( "anVertexCache Free: temporary pointer" );
	}

	// this block still can't be purged until the frame count has expired,
	// but it won't need to clear a user pointer when it is
	block->user = nullptr;

	block->next->prev = block->prev;
	block->prev->next = block->next;

	block->next = deferredFreeList.next;
	block->prev = &deferredFreeList;

	deferredFreeList.next->prev = block;
	deferredFreeList.next = block;
}

/*
===========
anVertexCache::AllocFrameTemp

A frame temp allocation must never be allowed to fail due to overflow.
We can't simply sync with the GPU and overwrite what we have, because
there may still be future references to dynamically created surfaces.
===========
*/
vertCache_t	*anVertexCache::AllocFrameTemp( void *data, int size ) {
	if ( size <= 0 ) {
		common->Error( "anVertexCache::AllocFrameTemp: size = %i\n", size );
	}

	vertCache_t *block = AllocBlock( sizeof( vertCache_t ) + size );
	if ( !block ) {
		return nullptr;
	}

	if ( !freeTempVerts.Exists( size ) ) {
		return block;
	}

	if ( dynamicAllocThisFrame + size > frameBytes ) {
		// if we don't have enough room in the temp block, allocate a static block,
		// but immediately free it so it will get freed at the next frame
		tempOverflow = true;
		Alloc( data, size, &block );
		Free( block);
		return block;
	}

	// this data is just going on the shared dynamic list

	// if we don't have any remaining unused headers, allocate some more
	if ( freeDynamicHeaders.next == &freeDynamicHeaders ) {
		for ( int i = 0; i < EXPAND_HEADERS; i++ ) {
			block = headerAllocator.Alloc();
			block->next = freeDynamicHeaders.next;
			block->prev = &freeDynamicHeaders;
			block->next->prev = block;
			block->prev->next = block;
		}
	}

	// move it from the freeDynamicHeaders list to the dynamicHeaders list
	block = freeDynamicHeaders.next;
	block->next->prev = block->prev;
	block->prev->next = block->next;
	block->next = dynamicHeaders.next;
	block->prev = &dynamicHeaders;
	block->next->prev = block;
	block->prev->next = block;

	block->size = size;
	block->tag = TAG_TEMP;
	block->indexBuffer = false;
	block->offset = dynamicAllocThisFrame;

	dynamicAllocThisFrame += block->size;
	dynamicCountThisFrame++;

	block->user = nullptr;
	block->frameUsed = 0;

	// copy the data
	block->virtMem = tempBuffers[listNum]->virtMem;
	block->vbo = tempBuffers[listNum]->vbo;

	if ( block->vbo ) {
		qglBindBufferARB( GL_ARRAY_BUFFER_ARB, block->vbo );
		qglBufferSubDataARB( GL_ARRAY_BUFFER_ARB, block->offset, ( GLsizeiptrARB)size, data );
	} else {
		SIMDProcessor->Memcpy( (byte *)block->virtMem + block->offset, data, size );
	}

	return block;
}

/*
==============
anVertexCache::GetVertexBuffer
==============
*/
bool anVertexCache::GetVertexBuffer( vertCacheHandle_t handle, anVertexBuffer* vb ) {
	const int isStatic = handle & VERTCACHE_STATIC;
	const unsigned int size = ( int )( handle >> VERTCACHE_SIZE_SHIFT ) & VERTCACHE_SIZE_MASK;
	const unsigned int offset = ( int )( handle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	const unsigned int frameNum = ( int )( handle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
	if ( isStatic ) {
		vb->Reference( staticData.vertexBuffer, offset, size );
		return true;
	}
	if ( frameNum != ( ( currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) ) {
		return false;
	}
	vb->Reference( frameData[drawListNum].vertexBuffer, offset, size );
	return true;
}

/*
==============
anVertexCache::GetIndexBuffer
==============
*/
bool anVertexCache::GetIndexBuffer( vertCacheHandle_t handle, anIndexBuffer *ib ) {
	const int isStatic = handle & VERTCACHE_STATIC;
	const unsigned int size = ( int )( handle >> VERTCACHE_SIZE_SHIFT ) & VERTCACHE_SIZE_MASK;
	const unsigned int offset = ( int )( handle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	const unsigned int frameNum = ( int )( handle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
	if ( isStatic ) {
		ib->Reference( staticData.indexBuffer, offset, size );
		return true;
	}
	if ( frameNum != ( ( currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) ) {
		return false;
	}
	ib->Reference( frameData[drawListNum].indexBuffer, offset, size );
	return true;
}

/*
===========
anVertexCache::CacheIsCurrent
===========
*/
bool anVertexCache::CacheIsCurrent( const vertCacheHandle_t handle ) {
	const int isStatic = handle & VERTCACHE_STATIC;

	if ( isStatic ) {
		return true;
	}

	const unsigned int frameNum = ( int )( handle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;

	if ( frameNum != ( currentFrame & VERTCACHE_FRAME_MASK ) ) {
		return false;
	}

	return true;
}

/*
===========
anVertexCache::EndFrame
===========
*/
void anVertexCache::EndFrame() {
	// display debug information
	if ( r_showVertexCache.GetBool() ) {
		int	staticUseCount = 0;
		int staticUseSize = 0;
		for ( vertCache_t *block = staticHeaders.next; block != &staticHeaders; block = block->next ) {
			if ( block->frameUsed == currentFrame ) {
				staticUseCount++;
				staticUseSize += block->size;
			}
		}

		const char *frameOverflow = tempOverflow ? "(OVERFLOW)" : "";

		common->Printf( "vertex dynamic:%i=%ik%s, static alloc:%i=%ik used:%i=%ik total:%i=%ik\n",
			dynamicCountThisFrame, dynamicAllocThisFrame/1024, frameOverflow,
			staticCountThisFrame, staticAllocThisFrame/1024,
			staticUseCount, staticUseSize/1024,
			staticCountTotal, staticAllocTotal/1024 );
	}

#if 0
	// if our total static count is above our working memory limit, start purging things
	while ( staticAllocTotal > r_vertexBufferMegs.GetInteger() * 1024 * 1024 ) {
		// free the least recently used

	}
#endif

	if ( !virtualMemory ) {
		// unbind vertex buffers so normal virtual memory will be used in case
		// r_useVertexBuffers / r_useIndexBuffers
		qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
		qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
	}

	currentFrame = tr.frameCount;
	listNum = currentFrame % NUM_VERTEX_FRAMES;
	staticAllocThisFrame = 0;
	staticCountThisFrame = 0;
	dynamicAllocThisFrame = 0;
	dynamicCountThisFrame = 0;
	tempOverflow = false;

	// free all the deferred free headers
	while( deferredFreeList.next != &deferredFreeList ) {
		ActuallyFree( deferredFreeList.next );
	}

	// free all the frame temp headers
	vertCache_t	*block = dynamicHeaders.next;
	if ( block != &dynamicHeaders ) {
		block->prev = &freeDynamicHeaders;
		dynamicHeaders.prev->next = freeDynamicHeaders.next;
		freeDynamicHeaders.next->prev = dynamicHeaders.prev;
		freeDynamicHeaders.next = block;
		dynamicHeaders.next = dynamicHeaders.prev = &dynamicHeaders;
	}
}

/*
=============
anVertexCache::List
=============
*/
void anVertexCache::List( void ) {
	int	numActive = 0;
	int	numDeferred = 0;
	int frameStatic = 0;
	int	totalStatic = 0;
	int	deferredSpace = 0;
	for ( vertCache_t *block = staticHeaders.next; block != &staticHeaders; block = block->next ) {
		numActive++;
		totalStatic += block->size;
		if ( block->frameUsed == currentFrame ) {
			frameStatic += block->size;
		}
	}

	int	numFreeStaticHeaders = 0;
	for ( vertCache_t *block = freeStaticHeaders.next; block != &freeStaticHeaders; block = block->next ) {
		numFreeStaticHeaders++;
	}

	int	numFreeDynamicHeaders = 0;
	for ( vertCache_t *block = freeDynamicHeaders.next; block != &freeDynamicHeaders; block = block->next ) {
		numFreeDynamicHeaders++;
	}

	common->Printf( "%i megs working set\n", r_vertexBufferMegs.GetInteger() );
	common->Printf( "%i dynamic temp buffers of %ik\n", NUM_VERTEX_FRAMES, frameBytes / 1024 );
	common->Printf( "%5i active static headers\n", numActive );
	common->Printf( "%5i free static headers\n", numFreeStaticHeaders );
	common->Printf( "%5i free dynamic headers\n", numFreeDynamicHeaders );

	if ( !virtualMemory  ) {
		common->Printf( "Vertex cache is in ARB_vertex_buffer_object memory (FAST).\n" );
	} else {
		common->Printf( "Vertex cache is in virtual memory (SLOW)\n" );
	}

	if ( r_useIndexBuffers.GetBool() ) {
		common->Printf( "Index buffers are accelerated.\n" );
	} else {
		common->Printf( "Index buffers are not used.\n" );
	}
}

vertCache_t *anVertexCache::CreateTempVbo( int bytes, bool indexBuffer ) {
	vertCache_t *block = headerAllocator.Alloc();

	block->next = nullptr;
	block->prev = nullptr;
	block->virtMem = nullptr;
	block->offset = 0;
	block->tag = TAG_FIXED;
	block->indexBuffer = indexBuffer;
	block->virtMemDirty = false;

	block->size = bytes;
	block->user = nullptr;

	block->virtMem = Mem_Alloc( bytes );

	qglGenBuffers( 1, & block->vbo );

	if (indexBuffer) {
		qglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, block->vbo );
		qglBufferData( GL_ELEMENT_ARRAY_BUFFER, ( GLsizeiptr )bytes, 0, GL_STREAM_DRAW );
	} else{
		qglBindBuffer( GL_ARRAY_BUFFER, block->vbo );
		qglBufferData( GL_ARRAY_BUFFER, ( GLsizeiptr )bytes, 0, GL_STREAM_DRAW );
	}

	return block;
}

int anVertexCache::GetNextListNum( void ) const {
	return ( listNum + 1 ) % NUM_VERTEX_FRAMES;
}

/*
=============
anVertexCache::IsFast

just for gfxinfo printing
=============
*/
bool anVertexCache::IsFast() {
	if ( virtualMemory ) {
		return false;
	}
	return true;
}
