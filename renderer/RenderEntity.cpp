#include "../idlib/Lib.h"
#pragma hdrstop
#include "tr_local.h"

anRenderEntityLocal::anRenderEntityLocal() {
	memset( &parms, 0, sizeof( parms ) );
	memset( modelMatrix, 0, sizeof( modelMatrix ) );

	world					= nullptr;
	index					= 0;
	lastModifiedFrameNum	= 0;
	archived				= false;
	dynamicModel			= nullptr;
	dynamicModelFrameCount	= 0;
	cachedDynamicModel		= nullptr;
	referenceBounds			= bounds_zero;
	viewCount				= 0;
	viewEntity				= nullptr;
	visibleCount			= 0;
	decals					= nullptr;
	overlay					= nullptr;
	entityRefs				= nullptr;
	firstInteraction		= nullptr;
	lastInteraction			= nullptr;
	needsPortalSky			= false;
}

void anRenderEntityLocal::FreeRenderEntity() {
	#ifdef _DEBUG
	assert( 0 );
	#endif
}

void anRenderEntityLocal::UpdateRenderEntity( const renderEntity_t *re, bool forceUpdate ) {
}

void anRenderEntityLocal::GetRenderEntity( renderEntity_t *re ) {
}

void anRenderEntityLocal::ForceUpdate() {
}

int anRenderEntityLocal::GetIndex() {
	return index;
}

void anRenderEntityLocal::ProjectOverlay( const anPlane localTextureAxis[2], const anMaterial *material ) {
}
void anRenderEntityLocal::RemoveDecals() {
}

anRenderLightsLocal::anRenderLightsLocal() {
	memset( &parms, 0, sizeof( parms ) );
	memset( modelMatrix, 0, sizeof( modelMatrix ) );
	memset( shadowFrustums, 0, sizeof( shadowFrustums ) );
	memset( lightProject, 0, sizeof( lightProject ) );
	memset( frustum, 0, sizeof( frustum ) );
	memset( frustumWindings, 0, sizeof( frustumWindings ) );

	lightHasMoved			= false;
	world					= nullptr;
	index					= 0;
	areaNum					= 0;
	lastModifiedFrameNum	= 0;
	archived				= false;
	lightShader				= nullptr;
	falloffImage			= nullptr;
	globalLightOrigin		= vec3_zero;
	frustumTris				= nullptr;
	numShadowFrustums		= 0;
	viewCount				= 0;
	viewLight				= nullptr;
	references				= nullptr;
	foggedPortals			= nullptr;
	firstInteraction		= nullptr;
	lastInteraction			= nullptr;
}

void anRenderLightsLocal::FreeRenderLight() {
}
void anRenderLightsLocal::UpdateRenderLight( const renderLight_t *re, bool forceUpdate ) {
}
void anRenderLightsLocal::GetRenderLight( renderLight_t *re ) {
}
void anRenderLightsLocal::ForceUpdate() {
}
int anRenderLightsLocal::GetIndex() {
	return index;
}
