
#ifndef __COMPILER_PUBLIC_H__
#define __COMPILER_PUBLIC_H__

/*
===============================================================================

	Compilers for map, model, video etc. processing.

===============================================================================
*/

// map processing (also see SuperOptimizeOccluders in tr_local.h)
void Dmap_f( const arcCommandArgs &args );

// bump map generation
void RenderBump_f( const arcCommandArgs &args );
void RenderBumpFlat_f( const arcCommandArgs &args );

// AAS file compiler
void RunAAS_f( const arcCommandArgs &args );
void RunAASDir_f( const arcCommandArgs &args );
void RunReach_f( const arcCommandArgs &args );

// video file encoding
void RoQFileEncode_f( const arcCommandArgs &args );

#endif	/* !__COMPILER_PUBLIC_H__ */
