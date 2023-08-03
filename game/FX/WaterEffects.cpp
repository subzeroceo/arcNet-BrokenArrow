#include "../Lib.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "WaterEffects.h"
#include "Wakes.h"

WaterFX::WaterFX ( void ) {
	maxVelocity = 300.0f;
	atten = 1.0f;
}

WaterFX::~WaterFX( void ) {
}

void WaterFX::Init( const char *splashEffectName, const char *wakeEffectName, anVec3 &offset, const anDictonary &wakeArgs ) {
	renderEffect_t &wakeEffect = wake.GetRenderEffect();
	if ( wakeEffectName ) {
		wakeEffect.declEffect	= gameLocal.FindEffect( wakeEffectName );
		wakeEffect.axis			= mat3_identity;
		wakeEffect.attenuation	= 1.f;
		wakeEffect.hasEndOrigin	= false;
		wakeEffect.loop			= true;
		wakeEffect.shaderParms[SHADERPARM_RED]			= 1.0f;
		wakeEffect.shaderParms[SHADERPARM_GREEN]		= 1.0f;
		wakeEffect.shaderParms[SHADERPARM_BLUE]			= 1.0f;
		wakeEffect.shaderParms[SHADERPARM_ALPHA]		= 1.0f;
		wakeEffect.shaderParms[SHADERPARM_BRIGHTNESS]	= 1.0f;
	} else {
		wakeEffect.declEffect	= nullptr;
	}

	renderEffect_t &splashEffect = splash.GetRenderEffect();
	if ( splashEffectName ) {
		splashEffect.declEffect		= gameLocal.FindEffect( splashEffectName );
		splashEffect.axis			= mat3_identity;
		splashEffect.attenuation	= 1.f;
		splashEffect.hasEndOrigin	= false;
		splashEffect.loop			= false;
		splashEffect.shaderParms[SHADERPARM_RED]		= 1.0f;
		splashEffect.shaderParms[SHADERPARM_GREEN]		= 1.0f;
		splashEffect.shaderParms[SHADERPARM_BLUE]		= 1.0f;
		splashEffect.shaderParms[SHADERPARM_ALPHA]		= 1.0f;
		splashEffect.shaderParms[SHADERPARM_BRIGHTNESS]	= 1.0f;
	} else {
		splashEffect.declEffect	= nullptr;
	}

	inWater = false;
	origin = vec3_origin;
	axis = mat3_identity;
	wakeStopped = true;
	this->offset = offset;

	wakeHandle = -1;
	wakeParms.ParseFromDict( wakeArgs );
}

void WaterFX::CheckWater( arcEntity *ent, const anVec3 &waterBodyOrg, const anMat3 &waterBodyAxis, arcCollisionModel *waterBodyModel, bool showWake ) {
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	const anBounds& waterBounds = waterBodyModel->GetBounds();
	anVec3 pos = origin + axis * offset;
	pos -= waterBodyOrg;
	pos *= waterBodyAxis.Transpose();
	bool newInWater = waterBounds.ContainsPoint( pos );

	anBounds bodybb;
	bool submerged = false;
	bodybb = ent->GetPhysics()->GetBounds();
	anBounds worldbb;
	worldbb.FromTransformedBounds( bodybb, ent->GetPhysics()->GetOrigin(), ent->GetPhysics()->GetAxis() );
	submerged = worldbb.GetMaxs()[2] < (waterBounds.GetMaxs()[2] + waterBodyOrg.z);

	CheckWater( ent, newInWater, waterBounds.GetMaxs()[2] + waterBodyOrg.z, submerged, showWake );
}

void WaterFX::CheckWater( arcEntity *ent, bool newInWater, float waterlevel, bool submerged, bool showWake ) {
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	// Update the wake
	if ( !newInWater || ( atten < 10e-4f ) || submerged ) {
		wake.StopDetach();
		wakeStopped = true;
	} else {
		renderEffect_t &wakeEff = wake.GetRenderEffect();
		if ( wakeEff.declEffect ) {
			wakeEff.origin = origin;
			wakeEff.origin.z = waterlevel + 3.0f;
			wakeEff.axis = axis;
			wakeEff.suppressSurfaceInViewID = ent->GetRenderEntity()->suppressSurfaceInViewID;
			wakeEff.attenuation = atten;
			if ( wakeStopped ) {
				wake.FreeRenderEffect();
				wake.Start( gameLocal.time );
				wakeStopped = false;
			}
		}
	}

	if ( newInWater ) {
		anVec3 wakeOrigin = origin;
		wakeOrigin.z = waterlevel + 10.0f;
		if ( !submerged && showWake ) {
			if ( wakeHandle < 0 ) {
				wakeHandle = arcWakeManager::GetInstance().AllocateWake( wakeParms );
			}
			if ( !arcWakeManager::GetInstance().UpdateWake( wakeHandle, velocity, wakeOrigin, axis ) ) {
				wakeHandle = -1; //All the elements in the wake faded or it was stopped for some reason make sure we allocate a new one next time
			}
		} else {
			wakeHandle = -1; //We left the water let it die naturally
		}
	} else {
		wakeHandle = -1; //We left the water let it die naturally
	}

	if ( !submerged || 1 ) {
		// Fire off a splash
		renderEffect_t &splashEff = splash.GetRenderEffect();
		if ( splashEff.declEffect && ( newInWater != inWater ) ) {
			splashEff.suppressSurfaceInViewID = ent->GetRenderEntity()->suppressSurfaceInViewID;
			splashEff.origin = origin;
			splashEff.origin.z = waterlevel + 4.f;
			splashEff.axis = mat3_identity;
			splashEff.attenuation = atten;
			splash.FreeRenderEffect();
			splash.Start( gameLocal.time );
		}
	}

	splash.Update();
	wake.Update();
	inWater = newInWater;
}

void WaterFX::CheckWaterEffectsOnly( void ) {
	// we are not in water
	wake.StopDetach();
	wakeStopped = true;
	wakeHandle = -1;
	inWater = false;
	splash.Update();
	wake.Update();
}

void WaterFX::SetOrigin( const anVec3 &origin ) {
	this->origin = origin;
}

void WaterFX::SetAxis( const anMat3 &axis ) {
	this->axis = axis;
}

// Velocity to scale effects
void WaterFX::SetVelocity( const anVec3 &velocity ) {
	this->velocity = velocity;

	float size = velocity.Length();
	if ( maxVelocity ) {
		float minVelocity = 50.0f;
		if ( size < minVelocity ) {
			atten = 0;
			return;
		}

		// Go from 0->1 between min and max velocity
		float intervVelocity = maxVelocity - minVelocity;
		atten = Min( size-minVelocity, intervVelocity ) / intervVelocity;
	} else {
		atten = 1.0f;
	}
}

WaterFX *WaterFX::SetupFromSpawnArgs( const anDictonary &args  ) {
	const char *wakeKey = args.GetString( "fx_wake", nullptr );
	const char *splashKey = args.GetString( "fx_splash", nullptr );
	anVec3 offset = args.GetVector( "water_offset", "0 0 0" );
	if ( wakeKey || splashKey ) {
		WaterFX *waterEffects = new WaterFX();
		waterEffects->Init( splashKey, wakeKey, offset, args );
		return waterEffects;
	} else {
		return  nullptr;
	}
}
