// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_VEHICLE_VEHICLE_RIGIDBODY_H__
#define __GAME_VEHICLE_VEHICLE_RIGIDBODY_H__

#include "Transport.h"
#include "VehicleIK.h"

class anVehicle_RigidBody : public anTransport_RB {
public:
	CLASS_PROTOTYPE( anVehicle_RigidBody );

							anVehicle_RigidBody( void );
							~anVehicle_RigidBody( void );

	virtual void			DoLoadVehicleScript( void );

	void					Spawn( void );

	virtual bool			UpdateAnimationControllers( void );
	virtual bool			Collide( const trace_t &collision, const anVec3 &velocity, int bodyId );
	virtual void			CollideFatal( anEntity *other );
	virtual idIK*			GetIK( void ) { return &ik; }

	virtual void			FreezePhysics( bool freeze );

	void					Event_SetDamageDealtScale( float scale );
	virtual void			SetDamageDealtScale( float scale );
	float					GetDamageDealtScale() const { return collideDamageDealtScale; }

	const sdDeclDamage*		GetCollideDamage() const { return collideDamage; }

	static void				IncRoadKillStats( anBasePlayer *player );

protected:
	int						nextSelfCollisionTime;
	int						nextJumpSound;
	int						nextCollisionTime;
	int						nextCollisionSound;

	const sdDeclDamage*		collideDamage;
	const sdDeclDamage*		collideFatalDamage;
	float					collideDamageDealtScale;
	float					collideDotLimit;

	sdIK_WheeledVehicle		ik;

	const sdProgram::sdFunction*		onCollisionFunc;
	const sdProgram::sdFunction*		onCollisionSideScrapeFunc;

protected:
	virtual void			HandleCollision( const trace_t &collision, const float velocity );
};

#endif // __GAME_VEHICLE_VEHICLE_RIGIDBODY_H__
