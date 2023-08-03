// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_ATMOSPHERERENDERABLE_H__
#define __GAME_ATMOSPHERERENDERABLE_H__

#include "../framework/CVarSystem.h"

class anRenderWorld;

class sdAtmosphereRenderable {
public:
	struct parms_t {
									parms_t() :
										mapId( 0 ) {
									}

		const sdDeclAtmosphere*		atmosphere;
		anRenderModel*				boxDomeModel;
		anRenderModel*				oldDomeModel;
		anVec3						skyOrigin;

		int							mapId;
	};

								sdAtmosphereRenderable( anRenderWorld* renderWorld );
	virtual						~sdAtmosphereRenderable();

	void						UpdateAtmosphere( parms_t& parms );

	void						DrawPostProcess( const renderView_t* view, float x, float y, float w, float h ) const;

	void						FreeModelDef();
	void						FreeLightDef();

	bool						IsLightValid() const { return skyLightHandle != -1; }

public:
	static anCVar				a_sun;
	static anCVar				a_glowScale;
	static anCVar				a_glowBaseScale;
	static anCVar				a_glowThresh;
	static anCVar				a_glowLuminanceDependency;
	static anCVar				a_glowSunPower;
	static anCVar				a_glowSunScale;
	static anCVar				a_glowSunBaseScale;

private:
	void						UpdateCelestialBody( parms_t& parms );
	void						UpdateCloudLayers( parms_t& parms );

	bool _glowSpriteCB( renderEntity_t*, const renderView_s*, int& lastModifiedGameTime );


private:
	anRenderWorld*				renderWorld;
	int							Uid;

	anList< renderEntity_t >	renderEnts;
	anList< qhandle_t >			renderHandles;

	renderLight_t				skyLight;
	qhandle_t					skyLightHandle;

	renderEntity_t				skyLightSprite;
	qhandle_t					skyLightSpriteHandle;

	renderEntity_t				skyLightGlowSprite;
	qhandle_t					skyLightGlowSpriteHandle;

	qhandle_t					occtestHandle;

	const anMaterial*			postProcessMaterial;
	anRenderModel*				spriteModel;

	float						currentScale;
	float						currentAlpha;

	float						sunFlareMaxSize;
	float						sunFlareTime;

	static bool glowSpriteCB( renderEntity_t*, const renderView_s*, int& lastModifiedGameTime );
};

#endif /* __GAME_ATMOSPHERERENDERABLE_H__ */
