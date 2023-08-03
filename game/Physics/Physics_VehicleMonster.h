
#ifndef __PHYSICS_VEHICLE_MONSTER_H__
#define __PHYSICS_VEHICLE_MONSTER_H__

/*
===================================================================================

	Vehicle Monster Physics

	Employs an impulse based dynamic simulation which is not very accurate but
	relatively fast and still reliable due to the continuous collision detection.
	Extents particle physics with the ability to apply impulses.

===================================================================================
*/

class anPhysics_VehicleMonster : public anPhysics_RigidBody {
public:
	CLASS_PROTOTYPE( anPhysics_VehicleMonster );

	bool					Evaluate						( int timeStepMSec, int endTimeMSec );
	void					SetGravity						( const anVec3 & v );
};

#endif /* !__PHYSICS_VEHICLE_MONSTER_H__ */
