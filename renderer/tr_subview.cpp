#include "/idlib/Lib.h"
#pragma hdrstop

#include "tr_local.h"

typedef struct {
	anVec3		origin;
	anMat3		axis;
} orientation_t;

/*
=================
R_MirrorPoint
=================
*/
static void R_MirrorPoint( const anVec3 in, orientation_t *surface, orientation_t *camera, anVec3 &out ) {
	int		i;
	anVec3	local;
	anVec3	transformed;
	float	d;

	local = in - surface->origin;

	transformed = vec3_origin;
	for ( i = 0; i < 3; i++ ) {
		d = local * surface->axis[i];
		transformed += d * camera->axis[i];
	}

	out = transformed + camera->origin;
}

/*
=================
R_MirrorVector
=================
*/
static void R_MirrorVector( const anVec3 in, orientation_t *surface, orientation_t *camera, anVec3 &out ) {
	out = vec3_origin;
	for ( int i = 0; i < 3; i++ ) {
		float d = in * surface->axis[i];
		out += d * camera->axis[i];
	}
}

/*
=============
R_PlaneForSurface

Returns the plane for the first triangle in the surface
FIXME: check for degenerate triangle?
=============
*/
static void R_PlaneForSurface( const srfTriangles_t *tri, anPlane &plane ) {
	anDrawVertex *v1 = tri->verts + tri->indexes[0];
	anDrawVertex *v2 = tri->verts + tri->indexes[1];
	anDrawVertex *v3 = tri->verts + tri->indexes[2];
	plane.FromPoints( v1->xyz, v2->xyz, v3->xyz );
}

/*
=========================
R_PreciseCullSurface

Check the surface for visibility on a per-triangle basis
for cases when it is going to be VERY expensive to draw ( subviews)

If not culled, also returns the bounding box of the surface in
Normalized Device Coordinates, so it can be used to crop the scissor rect.

OPTIMIZE: we could also take exact portal passing into consideration
=========================
*/
bool R_PreciseCullSurface( const drawSurf_t *drawSurf, anBounds &ndcBounds ) {
	anPlane eye;
	anVec3 localView;

	const srfTriangles_t *tri = drawSurf->geo;

	unsigned int pointOr = 0;
	unsigned int pointAnd = (unsigned int)~0;

	// get an exact bounds of the triangles for scissor cropping
	ndcBounds.Clear();

	for ( int i = 0; i < tri->numVerts; i++ ) {
		unsigned int pointFlags = 0;
		R_TransformModelToClip( tri->verts[i].xyz, drawSurf->space->modelViewMatrix, tr.viewDef->projectionMatrix, eye, clip );		for ( int j = 0; j < 3; j++ ) {
			if ( anPlane clip[j] >= clip[3] ) {
				pointFlags |= (1 << ( j*2 ) );
			} else if ( clip[j] <= -clip[3] ) {
				pointFlags |= ( 1 << ( j*2+1 ) );
			}
		}

		pointAnd &= pointFlags;
		pointOr |= pointFlags;
	}

	// trivially reject
	if ( pointAnd ) {
		return true;
	}

	// backface and frustum cull
	int numTriangles = tri->numIndexes / 3;

	R_GlobalPointToLocal( drawSurf->space->modelMatrix, tr.viewDef->renderView.vieworg, localView );

	for ( int i = 0; i < tri->numIndexes; i += 3 ) {
		const anVec3 &v1 = tri->verts[tri->indexes[i]].xyz;
		const anVec3 &v2 = tri->verts[tri->indexes[i+1]].xyz;
		const anVec3 &v3 = tri->verts[tri->indexes[i+2]].xyz;
		// this is a hack, because R_GlobalPointToLocal doesn't work with the non-normalized
		// axis that we get from the gui view transform.  It doesn't hurt anything, because
		// we know that all gui generated surfaces are front facing
		if ( tr.guiRecursionLevel == 0 ) {
			// we don't care that it isn't normalized,
			// all we want is the sign
			anVec3 v1 = v2 - v1;
			anVec3 d2 = v3 - v1;
			anVec3 normal = d2.Cross( d1 );

			anVec3 dir = v1 - localView;

			float dot = normal * dir;
			if ( dot >= 0.0f ) {
				return true;
			}
		}

		// now find the exact screen bounds of the clipped triangle
		anFixedWinding w.SetNumPoints( 3 );
		R_LocalPointToGlobal( drawSurf->space->modelMatrix, v1, w[0].ToVec3() );
		R_LocalPointToGlobal( drawSurf->space->modelMatrix, v2, w[1].ToVec3() );
		R_LocalPointToGlobal( drawSurf->space->modelMatrix, v3, w[2].ToVec3() );
		w[0].s = w[0].t = w[1].s = w[1].t = w[2].s = w[2].t = 0.0f;

		for ( int j = 0; j < 4; j++ ) {
			if ( !w.ClipInPlace( -tr.viewDef->frustum[j], 0.1f ) ) {
				break;
			}
		}
		for ( int j = 0; j < w.GetNumPoints(); j++ ) {
			anVec3	screen;
			R_GlobalToNormalizedDeviceCoordinates( w[j].ToVec3(), screen );
			ndcBounds.AddPoint( screen );
		}
	}

	// if we don't enclose any area, return
	if ( ndcBounds.IsCleared() ) {
		return true;
	}

	return false;
}

/*
========================
R_MirrorViewBySurface
========================
*/
static viewDef_t *R_MirrorViewBySurface( drawSurf_t *drawSurf ) {
	anPlane originalPlane, plane;

	// copy the viewport size from the original
	viewDef_t *parms = (viewDef_t *)R_FrameAlloc( sizeof( *parms ) );
	*parms = *tr.viewDef;
	parms->renderView.viewID = 0;	// clear to allow player bodies to show up, and suppress view weapons

	parms->isSubview = true;
	parms->isMirror = true;

	// create plane axis for the portal we are seeing
	R_PlaneForSurface( drawSurf->geo, originalPlane );
	R_LocalPlaneToGlobal( drawSurf->space->modelMatrix, originalPlane, plane );

	orientation_t surface.origin = plane.Normal() * -plane[3];
	surface.axis[0] = plane.Normal();
	surface.axis[0].NormalVectors( surface.axis[1], surface.axis[2] );
	surface.axis[2] = -surface.axis[2];

	orientation_t camera.origin = surface.origin;
	camera.axis[0] = -surface.axis[0];
	camera.axis[1] = surface.axis[1];
	camera.axis[2] = surface.axis[2];

	// set the mirrored origin and axis
	R_MirrorPoint( tr.viewDef->renderView.vieworg, &surface, &camera, parms->renderView.vieworg );

	R_MirrorVector( tr.viewDef->renderView.viewAxis[0], &surface, &camera, parms->renderView.viewAxis[0] );
	R_MirrorVector( tr.viewDef->renderView.viewAxis[1], &surface, &camera, parms->renderView.viewAxis[1] );
	R_MirrorVector( tr.viewDef->renderView.viewAxis[2], &surface, &camera, parms->renderView.viewAxis[2] );

	// make the view origin 16 units away from the center of the surface
	anVec3	viewOrigin = ( drawSurf->geo->bounds[0] + drawSurf->geo->bounds[1] ) * 0.5;
	viewOrigin += ( originalPlane.Normal() * 16 );

	R_LocalPointToGlobal( drawSurf->space->modelMatrix, viewOrigin, parms->initialViewAreaOrigin );

	// set the mirror clip plane
	parms->numClipPlanes = 1;
	parms->clipPlanes[0] = -camera.axis[0];

	parms->clipPlanes[0][3] = -( camera.origin * parms->clipPlanes[0].Normal() );

	return parms;
}

/*
========================
R_XrayViewBySurface
========================
*/
static viewDef_t *R_XrayViewBySurface( drawSurf_t *drawSurf ) {
	// copy the viewport size from the original
	viewDef_t *parms = (viewDef_t *)R_FrameAlloc( sizeof( *parms ) );
	*parms = *tr.viewDef;
	parms->renderView.viewID = 0;	// clear to allow player bodies to show up, and suppress view weapons

	parms->isSubview = true;
	parms->isXraySubview = true;

	return parms;
}

/*
===============
R_RemoteRender
===============
*/
static void R_RemoteRender( drawSurf_t *surf, textureStage_t *stage ) {
	// remote views can be reused in a single frame
	if ( stage->dynamicFrameCount == tr.frameCount ) {
		return;
	}

	// if the entity doesn't have a remoteRenderView, do nothing
	if ( !surf->space->entityDef->parms.remoteRenderView ) {
		return;
	}

	// copy the viewport size from the original
		viewDef_t *parms = (viewDef_t *)R_FrameAlloc( sizeof( *parms ) );
	*parms = *tr.viewDef;

	parms->isSubview = true;
	parms->isMirror = false;

	parms->renderView = *surf->space->entityDef->parms.remoteRenderView;
	parms->renderView.viewID = 0;	// clear to allow player bodies to show up, and suppress view weapons
	parms->initialViewAreaOrigin = parms->renderView.vieworg;

	tr.CropRenderSize( stage->width, stage->height, true );

	parms->renderView.x = 0;
	parms->renderView.y = 0;
	parms->renderView.width = SCREEN_WIDTH;
	parms->renderView.height = SCREEN_HEIGHT;

	tr.RenderViewToViewport( &parms->renderView, &parms->viewport );

	parms->scissor.x1 = 0;
	parms->scissor.y1 = 0;
	parms->scissor.x2 = parms->viewport.x2 - parms->viewport.x1;
	parms->scissor.y2 = parms->viewport.y2 - parms->viewport.y1;

	parms->superView = tr.viewDef;
	parms->subviewSurface = surf;

	// generate render commands for it
	R_RenderView(parms);

	// copy this rendering to the image
	stage->dynamicFrameCount = tr.frameCount;
	if ( !stage->image) {
		stage->image = globalImages->scratchImage;
	}

	tr.CaptureRenderToImage( stage->image->imgName );
	tr.UnCrop();
}

/*
=================
R_MirrorRender
=================
*/
void R_MirrorRender( drawSurf_t *surf, textureStage_t *stage, anScreenRect scissor ) {
	// remote views can be reused in a single frame
	if ( stage->dynamicFrameCount == tr.frameCount ) {
		return;
	}

	// issue a new view command
	viewDef_t *parms = R_MirrorViewBySurface( surf );
	if ( !parms ) {
		return;
	}

	tr.CropRenderSize( stage->width, stage->height, true );

	parms->renderView.x = 0;
	parms->renderView.y = 0;
	parms->renderView.width = SCREEN_WIDTH;
	parms->renderView.height = SCREEN_HEIGHT;

	tr.RenderViewToViewport( &parms->renderView, &parms->viewport );

	parms->scissor.x1 = 0;
	parms->scissor.y1 = 0;
	parms->scissor.x2 = parms->viewport.x2 - parms->viewport.x1;
	parms->scissor.y2 = parms->viewport.y2 - parms->viewport.y1;

	parms->superView = tr.viewDef;
	parms->subviewSurface = surf;

	// triangle culling order changes with mirroring
	parms->isMirror = ( ( ( int )parms->isMirror ^ ( int )tr.viewDef->isMirror ) != 0 );

	// generate render commands for it
	R_RenderView( parms );

	// copy this rendering to the image
	stage->dynamicFrameCount = tr.frameCount;
	stage->image = globalImages->scratchImage;

	tr.CaptureRenderToImage( stage->image->imgName );
	tr.UnCrop();
}

/*
=================
R_XrayRender
=================
*/
void R_XrayRender( drawSurf_t *surf, textureStage_t *stage, anScreenRect scissor ) {
	// remote views can be reused in a single frame
	if ( stage->dynamicFrameCount == tr.frameCount ) {
		return;
	}

	// issue a new view command
	viewDef_t *parms = R_XrayViewBySurface( surf );
	if ( !parms ) {
		return;
	}

	tr.CropRenderSize( stage->width, stage->height, true );

	parms->renderView.x = 0;
	parms->renderView.y = 0;
	parms->renderView.width = SCREEN_WIDTH;
	parms->renderView.height = SCREEN_HEIGHT;

	tr.RenderViewToViewport( &parms->renderView, &parms->viewport );

	parms->scissor.x1 = 0;
	parms->scissor.y1 = 0;
	parms->scissor.x2 = parms->viewport.x2 - parms->viewport.x1;
	parms->scissor.y2 = parms->viewport.y2 - parms->viewport.y1;

	parms->superView = tr.viewDef;
	parms->subviewSurface = surf;

	// triangle culling order changes with mirroring
	parms->isMirror = ( ( ( int )parms->isMirror ^ ( int )tr.viewDef->isMirror ) != 0 );

	// generate render commands for it
	R_RenderView( parms );

	// copy this rendering to the image
	stage->dynamicFrameCount = tr.frameCount;
	stage->image = globalImages->scratchImage2;

	tr.CaptureRenderToImage( stage->image->imgName );
	tr.UnCrop();
}

/*
==================
R_GenerateSurfaceSubview
==================
*/
bool R_GenerateSurfaceSubview( drawSurf_t *drawSurf ) {
	anBounds ndcBounds;

	// for testing the performance hit
	if ( r_skipSubviews.GetBool() ) {
		return false;
	}

	if ( R_PreciseCullSurface( drawSurf, ndcBounds ) ) {
		return false;
	}

	const anMaterial *shader = drawSurf->material;
	viewDef_t *parms;

	// never recurse through a subview surface that we are
	// already seeing through
	for ( parms = tr.viewDef; parms; parms = parms->superView ) {
		if ( parms->subviewSurface
			&& parms->subviewSurface->geo == drawSurf->geo
			&& parms->subviewSurface->space->entityDef == drawSurf->space->entityDef ) {
			break;
		}
	}
	if ( parms ) {
		return false;
	}

	// crop the scissor bounds based on the precise cull
	anScreenRect	scissor;

	anScreenRect	*v = &tr.viewDef->viewport;
	scissor.x1 = v->x1 + ( int )( ( v->x2 - v->x1 + 1 ) * 0.5f * ( ndcBounds[0][0] + 1.0f ) );
	scissor.y1 = v->y1 + ( int )( ( v->y2 - v->y1 + 1 ) * 0.5f * ( ndcBounds[0][1] + 1.0f ) );
	scissor.x2 = v->x1 + ( int )( ( v->x2 - v->x1 + 1 ) * 0.5f * ( ndcBounds[1][0] + 1.0f ) );
	scissor.y2 = v->y1 + ( int )( ( v->y2 - v->y1 + 1 ) * 0.5f * ( ndcBounds[1][1] + 1.0f ) );

	// nudge a bit for safety
	scissor.Expand();

	scissor.Intersect( tr.viewDef->scissor );

	if ( scissor.IsEmpty() ) {
		// cropped out
		return false;
	}

	// see what kind of subview we are making
	if ( shader->GetSort() != SS_SUBVIEW ) {
		for ( int i = 0; i < shader->GetNumStages(); i++ ) {
			const materialStage_t	*stage = shader->GetStage( i );
			switch ( stage->texture.dynamic ) {
			case DI_REMOTE_RENDER:
				R_RemoteRender( drawSurf, const_cast<textureStage_t *>( &stage->texture ) );
				break;
			case DI_MIRROR_RENDER:
				R_MirrorRender( drawSurf, const_cast<textureStage_t *>( &stage->texture ), scissor );
				break;
			case DI_XRAY_RENDER:
				R_XrayRender( drawSurf, const_cast<textureStage_t *>( &stage->texture ), scissor );
				break;
			case DI_REFLECTION_RENDER:
				R_MirrorRender( drawSurf, const_cast<textureStage_t *>( &stage->texture ), scissor );
				break;
			case DI_REFRACTION_RENDER:
				R_MirrorRender( drawSurf, const_cast<textureStage_t *>( &stage->texture ), scissor );
				break;
			}
		}
		return true;
	}

	// issue a new view command
	parms = R_MirrorViewBySurface( drawSurf );
	if ( !parms ) {
		return false;
	}

	parms->scissor = scissor;
	parms->superView = tr.viewDef;
	parms->subviewSurface = drawSurf;

	// triangle culling order changes with mirroring
	parms->isMirror = ( ( ( int )parms->isMirror ^ ( int )tr.viewDef->isMirror ) != 0 );

	// generate render commands for it
	R_RenderView( parms );

	return true;
}

/*
================
R_GenerateSubViews

If we need to render another view to complete the current view,
generate it first.

It is important to do this after all drawSurfs for the current
view have been generated, because it may create a subview which
would change tr.viewCount.
================
*/
bool R_GenerateSubViews( void ) {
	// for testing the performance hit
	if ( r_skipSubviews.GetBool() ) {
		return false;
	}

	bool subviews = false;

	// scan the surfaces until we either find a subview, or determine
	// there are no more subview surfaces.
	for ( int i = 0; i < tr.viewDef->numDrawSurfs; i++ ) {
		drawSurf_t *drawSurf; = tr.viewDef->drawSurfs[i];
		const anMaterial *shader = drawSurf->material;
		if ( !shader || !shader->HasSubview() ) {
			continue;
		}

		if ( R_GenerateSurfaceSubview( drawSurf ) ) {
			subviews = true;
		}
	}

	return subviews;
}
