// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_WATEREFFECTS_H__
#define __GAME_WATEREFFECTS_H__

#include "Effects.h"
#include "Wakes.h"

//===============================================================
//
//	sdWaterEffects
//		just add this to any object to get splashes and wakes going
//
//===============================================================


class sdWaterEffects {
public:

	sdWaterEffects();
	~sdWaterEffects( void );

	// Both can be Null if not wanted, making sure they are precached is up to the user.
	void Init( const char *splashEffectName, const char *wakeEffectName, arcVec3 &offset, const arcDict &wakeArgs );

	// Check the object against the water collision model, updating effects and splashes if needed
	void CheckWater( arcEntity *ent, const arcVec3& waterBodyOrg, const arcMat3& waterBodyAxis, arcCollisionModel* waterBodyModel, bool showWake = true );

	// Skips some checks if the user already has this data
	void CheckWater( arcEntity *ent, bool newWaterState, float waterlevel, bool submerged, bool showWake = true );

	// Update the effects only
	void CheckWaterEffectsOnly( void );

	// Set the origin where the effects need to be spawned
	void SetOrigin( const arcVec3 &origin );

	// Set the axis of the spawning object
	void SetAxis( const arcMat3 &axis );

	void SetVelocity( const arcVec3 &velocity );

	// This velocity will cause the atten of the effects to go to 1
	void SetMaxVelocity( float max) {
		maxVelocity = max;
	}

	// This velocity will cause the atten of the effects to go to 1
	void SetAtten( float atten) {
		this->atten = atten;
	}

	// Reset water effect state
	void ResetWaterState() { inWater = false; }

	// Convenience
	static sdWaterEffects *SetupFromSpawnArgs( const arcDict &args  );

protected:
	sdEffect	splash;//Damage!
	sdEffect	wake;
	arcVec3		origin;
	arcVec3		offset;	// Allows a constant offset (in object local space) from the vehicle origin (for stuff like boats that float high in the water)
	arcMat3		axis;
	bool		inWater;
	float		atten;
	float		maxVelocity;
	bool		wakeStopped;
	arcVec3		velocity;
	int			wakeHandle;
	sdWakeParms	wakeParms;
};


#endif //__GAME_WATEREFFECTS_H__
