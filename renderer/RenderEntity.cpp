#include "/idlib/precompiled.h"
#pragma hdrstop
#include "tr_local.h"

ARCRenderEntityLocal::ARCRenderEntityLocal() {
	memset( &parms, 0, sizeof( parms ) );
	memset( modelMatrix, 0, sizeof( modelMatrix ) );

	world					= NULL;
	index					= 0;
	lastModifiedFrameNum	= 0;
	archived				= false;
	dynamicModel			= NULL;
	dynamicModelFrameCount	= 0;
	cachedDynamicModel		= NULL;
	referenceBounds			= bounds_zero;
	viewCount				= 0;
	viewEntity				= NULL;
	visibleCount			= 0;
	decals					= NULL;
	overlay					= NULL;
	entityRefs				= NULL;
	firstInteraction		= NULL;
	lastInteraction			= NULL;
	needsPortalSky			= false;
}

void ARCRenderEntityLocal::FreeRenderEntity() {
	#ifdef _DEBUG
	assert( 0 );
	#endif
}

void ARCRenderEntityLocal::UpdateRenderEntity( const renderEntity_t *re, bool forceUpdate ) {
}

void ARCRenderEntityLocal::GetRenderEntity( renderEntity_t *re ) {
}

void ARCRenderEntityLocal::ForceUpdate() {
}

int ARCRenderEntityLocal::GetIndex() {
	return index;
}

void ARCRenderEntityLocal::ProjectOverlay( const arcPlane localTextureAxis[2], const arcMaterial *material ) {
}
void ARCRenderEntityLocal::RemoveDecals() {
}

ARCRenderLightsLocal::ARCRenderLightsLocal() {
	memset( &parms, 0, sizeof( parms ) );
	memset( modelMatrix, 0, sizeof( modelMatrix ) );
	memset( shadowFrustums, 0, sizeof( shadowFrustums ) );
	memset( lightProject, 0, sizeof( lightProject ) );
	memset( frustum, 0, sizeof( frustum ) );
	memset( frustumWindings, 0, sizeof( frustumWindings ) );

	lightHasMoved			= false;
	world					= NULL;
	index					= 0;
	areaNum					= 0;
	lastModifiedFrameNum	= 0;
	archived				= false;
	lightShader				= NULL;
	falloffImage			= NULL;
	globalLightOrigin		= vec3_zero;
	frustumTris				= NULL;
	numShadowFrustums		= 0;
	viewCount				= 0;
	viewLight				= NULL;
	references				= NULL;
	foggedPortals			= NULL;
	firstInteraction		= NULL;
	lastInteraction			= NULL;
}

void ARCRenderLightsLocal::FreeRenderLight() {
}
void ARCRenderLightsLocal::UpdateRenderLight( const renderLight_t *re, bool forceUpdate ) {
}
void ARCRenderLightsLocal::GetRenderLight( renderLight_t *re ) {
}
void ARCRenderLightsLocal::ForceUpdate() {
}
int ARCRenderLightsLocal::GetIndex() {
	return index;
}
