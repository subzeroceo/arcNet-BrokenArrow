#include "../idlib/Lib.h"
#pragma hdrstop

#include "tr_local.h"
#include "Model_local.h"

/*

This is a simple dynamic model that just creates a stretched quad between
two points that faces the view, like a dynamic deform tube.

*/

static const char *beam_SnapshotName = "_beam_Snapshot_";

/*
===============
idRenderModelBeam::IsDynamicModel
===============
*/
dynamicModel_t idRenderModelBeam::IsDynamicModel() const {
	return DM_CONTINUOUS;	// regenerate for every view
}

/*
===============
idRenderModelBeam::IsLoaded
===============
*/
bool idRenderModelBeam::IsLoaded() const {
	return true;	// don't ever need to load
}

/*
===============
idRenderModelBeam::InstantiateDynamicModel
===============
*/
anRenderModel *idRenderModelBeam::InstantiateDynamicModel( const struct renderEntity_s *renderEntity, const struct viewDef_s *viewDef, anRenderModel *cachedModel ) {
	anModelStatic *staticModel;
	srfTriangles_t *tri;
	modelSurface_t surf;

	if ( cachedModel ) {
		delete cachedModel;
		cachedModel = nullptr;
	}

	if ( renderEntity == nullptr || viewDef == nullptr ) {
		delete cachedModel;
		return nullptr;
	}

	if ( cachedModel != nullptr ) {
		assert( dynamic_cast<anModelStatic *>( cachedModel ) != nullptr );
		assert( anString::Icmp( cachedModel->Name(), beam_SnapshotName ) == 0 );

		staticModel = static_cast<anModelStatic *>( cachedModel );
		surf = *staticModel->Surface( 0 );
		tri = surf.geometry;
	} else {
		staticModel = new anModelStatic;
		staticModel->InitEmpty( beam_SnapshotName );

		tri = R_AllocStaticTriSurf();
		R_AllocStaticTriSurfVerts( tri, 4 );
		R_AllocStaticTriSurfIndexes( tri, 6 );

		tri->verts[0].Clear();
		tri->verts[0].st[0] = 0;
		tri->verts[0].st[1] = 0;

		tri->verts[1].Clear();
		tri->verts[1].st[0] = 0;
		tri->verts[1].st[1] = 1;

		tri->verts[2].Clear();
		tri->verts[2].st[0] = 1;
		tri->verts[2].st[1] = 0;

		tri->verts[3].Clear();
		tri->verts[3].st[0] = 1;
		tri->verts[3].st[1] = 1;

		tri->indexes[0] = 0;
		tri->indexes[1] = 2;
		tri->indexes[2] = 1;
		tri->indexes[3] = 2;
		tri->indexes[4] = 3;
		tri->indexes[5] = 1;

		tri->numVerts = 4;
		tri->numIndexes = 6;

		surf.geometry = tri;
		surf.id = 0;
		surf.shader = tr.defaultMaterial;
		staticModel->AddSurface( surf );
	}

	anVec3	target = *reinterpret_cast<const anVec3 *>( &renderEntity->shaderParms[SHADERPARM_BEAM_END_X] );

	// we need the view direction to project the minor axis of the tube
	// as the view changes
	anVec3	localView, localTarget;
	float	modelMatrix[16];
	R_AxisToModelMatrix( renderEntity->axis, renderEntity->origin, modelMatrix );
	R_GlobalPointToLocal( modelMatrix, viewDef->renderView.vieworg, localView );
	R_GlobalPointToLocal( modelMatrix, target, localTarget );

	anVec3	major = localTarget;
	anVec3	minor;

	anVec3	mid = 0.5f * localTarget;
	anVec3	dir = mid - localView;
	minor.Cross( major, dir );
	minor.Normalize();
	if ( renderEntity->shaderParms[SHADERPARM_BEAM_WIDTH] != 0.0f ) {
		minor *= renderEntity->shaderParms[SHADERPARM_BEAM_WIDTH] * 0.5f;
	}

	int red		= anMath::FtoiFast( renderEntity->shaderParms[SP_RED] * 255.0f );
	int green	= anMath::FtoiFast( renderEntity->shaderParms[SP_GREEN] * 255.0f );
	int blue	= anMath::FtoiFast( renderEntity->shaderParms[SS_BLUE] * 255.0f );
	int alpha	= anMath::FtoiFast( renderEntity->shaderParms[SS_APLHA] * 255.0f );

	tri->verts[0].xyz = minor;
	tri->verts[0].color[0] = red;
	tri->verts[0].color[1] = green;
	tri->verts[0].color[2] = blue;
	tri->verts[0].color[3] = alpha;

	tri->verts[1].xyz = -minor;
	tri->verts[1].color[0] = red;
	tri->verts[1].color[1] = green;
	tri->verts[1].color[2] = blue;
	tri->verts[1].color[3] = alpha;

	tri->verts[2].xyz = localTarget + minor;
	tri->verts[2].color[0] = red;
	tri->verts[2].color[1] = green;
	tri->verts[2].color[2] = blue;
	tri->verts[2].color[3] = alpha;

	tri->verts[3].xyz = localTarget - minor;
	tri->verts[3].color[0] = red;
	tri->verts[3].color[1] = green;
	tri->verts[3].color[2] = blue;
	tri->verts[3].color[3] = alpha;

	R_BoundTriSurf( tri );

	staticModel->bounds = tri->bounds;

	return staticModel;
}

/*
===============
idRenderModelBeam::Bounds
===============
*/
anBounds idRenderModelBeam::Bounds( const struct renderEntity_s *renderEntity ) const {
	anBounds	b;

	b.Zero();
	if ( !renderEntity ) {
		b.ExpandSelf( 8.0f );
	} else {
		anVec3	target = *reinterpret_cast<const anVec3 *>( &renderEntity->shaderParms[SHADERPARM_BEAM_END_X] );
		anVec3	localTarget;
		float	modelMatrix[16];
		R_AxisToModelMatrix( renderEntity->axis, renderEntity->origin, modelMatrix );
		R_GlobalPointToLocal( modelMatrix, target, localTarget );

		b.AddPoint( localTarget );
		if ( renderEntity->shaderParms[SHADERPARM_BEAM_WIDTH] != 0.0f ) {
			b.ExpandSelf( renderEntity->shaderParms[SHADERPARM_BEAM_WIDTH] * 0.5f );
		}
	}
	return b;
}