
#include "../../idlib/Lib.h"
#pragma hdrstop

#include "../Game_local.h"

CLASS_DECLARATION( anPhysics_RigidBody, anPhysics_VehicleMonster )
END_CLASS

/*
================
anPhysics_VehicleMonster::Evaluate

  Evaluate the impulse based rigid body physics.
  When a collision occurs an impulse is applied at the moment of impact but
  the remaining time after the collision is ignored.
================
*/
bool anPhysics_VehicleMonster::Evaluate( int timeStepMSec, int endTimeMSec ) {
	if ( anPhysics_RigidBody::Evaluate( timeStepMSec, endTimeMSec ) ) {

		anAngles euler			= current.i.orientation.ToAngles();
		euler.pitch				= 0.0f;
		euler.roll				= 0.0f;
		current.i.orientation	= euler.ToMat3();

		return true;
	}

	return false;
}

void anPhysics_VehicleMonster::SetGravity ( const anVec3 & v ) {
	gravityVector = v;
	gravityNormal = gameLocal.GetGravity( );
	gravityNormal.Normalize();
}
