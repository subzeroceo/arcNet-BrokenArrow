
#ifndef __COMPILER_PUBLIC_H__
#define __COMPILER_PUBLIC_H__

/*
===============================================================================

	Compilers for map, model, video etc. processing.

===============================================================================
*/

// map processing (also see SuperOptimizeOccluders in tr_local.h)
void Dmap_f( const anCommandArgs &args );

// bump map generation
void RenderBump_f( const anCommandArgs &args );
void RenderBumpFlat_f( const anCommandArgs &args );

// AAS file compiler
void RunAAS_f( const anCommandArgs &args );
void RunAASDir_f( const anCommandArgs &args );
void RunReach_f( const anCommandArgs &args );

// video file encoding
void RoQFileEncode_f( const anCommandArgs &args );

#endif	/* !__COMPILER_PUBLIC_H__ */
