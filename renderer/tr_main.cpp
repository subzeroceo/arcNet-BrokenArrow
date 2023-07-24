#include "/idlib/precompiled.h"
#pragma hdrstop

#include "tr_local.h"
#ifdef __ppc__
#include <vecLib/vecLib.h>
#endif
#if defined(MACOS_X) && defined(__i386__)
#include <xmmintrin.h>
#endif

//====================================================================
  /*
    qglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    qglEnable( GL_BLEND);
    qglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
    qglColor4f(1.0, 0.0, 0.0, 1.0 );
    qglBegin( GL_POLYGON);

    qglVertex2f(0.0, 0.0 );
    qglVertex2f(1.0, 0.0 );
    qglVertex2f(1.0, 1.0 );
    qglVertex2f(0.0, 1.0 );
    qglEnd();
    qglDisable( GL_BLEND);
    # print the debug overdraw message
    print( "glPolygonMode( GL_FRONT_AND_BACK, GL_FILL)" );
    // end of debug overdraw
*/
/*
======================
ARCScreenRect::Clear
======================
*/
void ARCScreenRect::Clear() {
	x1 = y1 = 32000;
	x2 = y2 = -32000;
	zMin = 0.0f;
	zMaz = 1.0f;
}

/*
======================
ARCScreenRect::AddPoint
======================
*/
void ARCScreenRect::AddPoint( float x, float y ) {
	int	ix = arcMath::FtoiFast( x );
	int iy = arcMath::FtoiFast( y );

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

/*
======================
ARCScreenRect::Expand
======================
*/
void ARCScreenRect::Expand() {
	x1--;
	y1--;
	x2++;
	y2++;
}

/*
======================
ARCScreenRect::Intersect
======================
*/
void ARCScreenRect::Intersect( const ARCScreenRect &rect ) {
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

/*
======================
ARCScreenRect::Union
======================
*/
void ARCScreenRect::Union( const ARCScreenRect &rect ) {
	if ( rect.x1 < x1 ) {
		x1 = rect.x1;
	}
	if ( rect.x2 > x2 ) {
		x2 = rect.x2;
	}
	if ( rect.y1 < y1 ) {
		y1 = rect.y1;
	}
	if ( rect.y2 > y2 ) {
		y2 = rect.y2;
	}
}

/*
======================
ARCScreenRect::Equals
======================
*/
bool ARCScreenRect::Equals( const ARCScreenRect &rect ) const {
	return ( x1 == rect.x1 && x2 == rect.x2 && y1 == rect.y1 && y2 == rect.y2 );
}

/*
======================
ARCScreenRect::IsEmpty
======================
*/
bool ARCScreenRect::IsEmpty() const {
	return ( x1 > x2 || y1 > y2 );
}

/*
======================
R_ScreenRectFromViewFrustumBounds
======================
*/
ARCScreenRect R_ScreenRectFromViewFrustumBounds( const arcBounds &bounds ) {
	ARCScreenRect screenRect;

	// convert to virtual screen coordinates
	screenRect.x1 = arcMath::FtoiFast( 0.5f * ( 1.0f - bounds[1].y ) * ( tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1 ) );
	screenRect.x2 = arcMath::FtoiFast( 0.5f * ( 1.0f - bounds[0].y ) * ( tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1 ) );
	screenRect.y1 = arcMath::FtoiFast( 0.5f * ( 1.0f + bounds[0].z ) * ( tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1 ) );
	screenRect.y2 = arcMath::FtoiFast( 0.5f * ( 1.0f + bounds[1].z ) * ( tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1 ) );

	// in order for depth bounds testing to work right, we need to transform the z values
	if ( r_useDepthBoundsTest.GetInteger() ) {
		// transform from world space to eye space
		TransformEyeZToWin( -bounds[0].x, tr.viewDef->projectionMatrix, screenRect.zMin );
		TransformEyeZToWin( -bounds[1].x, tr.viewDef->projectionMatrix, screenRect.zMaz );
	}

	return screenRect;
}

/*
======================
R_ShowColoredScreenRect
======================
*/
void R_ShowColoredScreenRect( const ARCScreenRect &rect, int colorIndex ) {
	if ( !rect.IsEmpty() ) {
		static arcVec4 colors[] = { colorRed, colorGreen, colorBlue, colorYellow, colorMagenta, colorCyan, colorWhite, colorPurple };
		tr.viewDef->renderWorld->DebugScreenRect( colors[colorIndex & 7], rect, tr.viewDef );
	}
}


/*
=================
R_RadiusCullLocalBox

A fast, conservative center-to-corner culling test
Returns true if the box is outside the given global frustum, (positive sides are out)
=================
*/
bool R_RadiusCullLocalBox( const arcBounds &bounds, const float modelMatrix[16], int numPlanes, const arcPlane *planes ) {
	arcVec3	worldOrigin;

	if ( r_useCulling.GetInteger() == 0 ) {
		return false;
	}

	// transform the surface bounds into world space
	arcVec3 localOrigin = ( bounds[0] + bounds[1] ) * 0.5;

	LocalPointToGlobal( modelMatrix, localOrigin, worldOrigin );

	float worldRadius = ( bounds[0] - localOrigin ).Length();	// FIXME: won't be correct for scaled objects

	for ( int i = 0; i < numPlanes; i++ ) {
		const arcPlane	*frust = planes + i;
		float d = frust->Distance( worldOrigin );
		if ( d > worldRadius ) {
			return true;	// culled
		}
	}

	return false;		// no culled
}

/*
=================
R_CornerCullLocalBox

Tests all corners against the frustum.
Can still generate a few false positives when the box is outside a corner.
Returns true if the box is outside the given global frustum, (positive sides are out)
=================
*/
bool R_CornerCullLocalBox( const arcBounds &bounds, const float modelMatrix[16], int numPlanes, const arcPlane *planes ) {
	// we can disable box culling for experimental timing purposes
	if ( r_useCulling.GetInteger() < 2 ) {
		return false;
	}

	// transform into world space
	for ( int i = 0; i < 8; i++ ) {
		arcVec3 v[0] = bounds[i&1][0];
		arcVec3 v[1] = bounds[( i>>1 )&1][1];
		arcVec3 v[2] = bounds[( i>>2 )&1][2];

		LocalPointToGlobal( modelMatrix, v, transformed[i] );
	}

	// check against frustum planes
	for ( i = 0; i < numPlanes; i++ ) {
		const arcPlane *frust = planes + i;
		for ( int j = 0; j < 8; j++ ) {
			arcVec3 transformed[8];
			float dists[8];
			dists[j] = frust->Distance( transformed[j] );
			if ( dists[j] < 0 ) {
				break;
			}
		}
		if ( j == 8 ) {
			// all points were behind one of the planes
			tr.pc.boxCullOut++;
			return true;
		}
	}

	tr.pc.boxCullIn++;

	return false;		// not culled
}

/*
=================
R_CullLocalBox

Performs quick test before expensive test
Returns true if the box is outside the given global frustum, (positive sides are out)
=================
*/
bool R_CullLocalBox( const arcBounds &bounds, const float modelMatrix[16], int numPlanes, const arcPlane *planes ) {
	if ( R_RadiusCullLocalBox( bounds, modelMatrix, numPlanes, planes ) ) {
		return true;
	}
	return R_CornerCullLocalBox( bounds, modelMatrix, numPlanes, planes );
}

/*
=================
R_SetupViewFrustum

Setup that culling frustum planes for the current view
FIXME: derive from modelview matrix times projection matrix
=================
*/
static void R_SetupViewFrustum( void ) {
	float	xs, xc;

	float ang = DEG2RAD( tr.viewDef->renderView.fov_x ) * 0.5f;
	arcMath::SinCos( ang, xs, xc );

	tr.viewDef->frustum[0] = xs * tr.viewDef->renderView.viewAxis[0] + xc * tr.viewDef->renderView.viewAxis[1];
	tr.viewDef->frustum[1] = xs * tr.viewDef->renderView.viewAxis[0] - xc * tr.viewDef->renderView.viewAxis[1];

	ang = DEG2RAD( tr.viewDef->renderView.fov_y ) * 0.5f;
	arcMath::SinCos( ang, xs, xc );

	tr.viewDef->frustum[2] = xs * tr.viewDef->renderView.viewAxis[0] + xc * tr.viewDef->renderView.viewAxis[2];
	tr.viewDef->frustum[3] = xs * tr.viewDef->renderView.viewAxis[0] - xc * tr.viewDef->renderView.viewAxis[2];

	// plane four is the front clipping plane
	tr.viewDef->frustum[4] = /* vec3_origin - */ tr.viewDef->renderView.viewAxis[0];

	for ( int i = 0; i < 5; i++ ) {
		// flip direction so positive side faces out (FIXME: globally unify this)
		tr.viewDef->frustum[i] = -tr.viewDef->frustum[i].Normal();
		tr.viewDef->frustum[i][3] = -( tr.viewDef->renderView.vieworg * tr.viewDef->frustum[i].Normal() );
	}

	// eventually, plane five will be the rear clipping plane for fog
	float dNear = r_znear.GetFloat();
	if ( tr.viewDef->renderView.cramZNear ) {
		dNear *= 0.25f;
	}

	float dFar = MAX_WORLD_SIZE;
	float dLeft = dFar * tan( DEG2RAD( tr.viewDef->renderView.fov_x * 0.5f ) );
	float dUp = dFar * tan( DEG2RAD( tr.viewDef->renderView.fov_y * 0.5f ) );
	tr.viewDef->viewFrustum.SetOrigin( tr.viewDef->renderView.vieworg );
	tr.viewDef->viewFrustum.SetAxis( tr.viewDef->renderView.viewAxis );
	tr.viewDef->viewFrustum.SetSize( dNear, dFar, dLeft, dUp );
}

/*
===================
R_ConstrainViewFrustum
===================
*/
static void R_ConstrainViewFrustum( void ) {
	// constrain the view frustum to the total bounds of all visible lights and visible entities
	arcBounds bounds.Clear();

	for ( viewLight_t *vLight = tr.viewDef->viewLights; vLight; vLight = vLight->next ) {
		bounds.AddBounds( vLight->lightDef->frustumTris->bounds );
	}

	for ( viewEntity_t *vEntity = tr.viewDef->viewEntitys; vEntity; vEntity = vEntity->next ) {
		bounds.AddBounds( vEntity->entityDef->referenceBounds );
	}

	tr.viewDef->viewFrustum.ConstrainToBounds( bounds );

	if ( r_useFrustumFarDistance.GetFloat() > 0.0f ) {
		tr.viewDef->viewFrustum.MoveFarDistance( r_useFrustumFarDistance.GetFloat() );
	}
}

/*
==========================================================================================

DRAWSURF SORTING

==========================================================================================
*/

/*
=======================
R_QSortSurfaces
=======================
*/
static int R_QSortSurfaces( const void *a, const void *b ) {
	const drawSurf_t *ea = *( drawSurf_t ** )a;
	const drawSurf_t *eb = *( drawSurf_t ** )b;

	if ( ea->sort < eb->sort ) {
		return -1;
	}

	if ( ea->sort > eb->sort ) {
		return 1;
	}

	return 0;
}

/*
=================
R_SortDrawSurfs
=================
*/
static void R_SortDrawSurfs( void ) {
	// sort the drawsurfs by sort type, then orientation, then shader
	qsort( tr.viewDef->drawSurfs, tr.viewDef->numDrawSurfs, sizeof( tr.viewDef->drawSurfs[0] ), R_QSortSurfaces( 0 ) );
}

/*
================
R_RenderPostProcess

Because R_RenderView may be called by subviews we have to make sure the post process
pass happens after the active view and its subviews is done rendering.
================
*/
void R_RenderPostProcess( viewDef_t *parms ) {
	viewDef_t *oldView = tr.viewDef;

	R_AddDrawPostProcess( parms );

	tr.viewDef = oldView;
}