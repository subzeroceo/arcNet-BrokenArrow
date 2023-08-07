#ifndef __PHYSICS_SIMPLE_H__
#define __PHYSICS_SIMPLE_H__

#include "Physics_Actor.h"

/*
===================================================================================

	Simple physics

	This is just an AABB - it has no angular info. 
	It can't be pushed, but it can push.
	Applying forces & impulses works appropriately.

===================================================================================
*/

typedef struct simplePState_s {
	anVec3					origin;
	anVec3					velocity;
	anVec3					pushVelocity;

	anMat3					axis;
	anVec3					angularVelocity;
} simplePState_t;


class sdPhysics_Simple : public idPhysics_Actor {

public:
	CLASS_PROTOTYPE( sdPhysics_Simple );

							sdPhysics_Simple( void );
							~sdPhysics_Simple( void );


public:	// common physics interface
	bool					Evaluate( int timeStepMSec, int endTimeMSec );

	void					SetClipModel( anClipModel *model, float density, int id = 0, bool freeOld = true );
	void					SetOrigin( const anVec3 &newOrigin, int id = -1 );
	const anVec3 &			GetOrigin( int id = 0 ) const;

	void					SetAxis( const anMat3 &newAxis, int id = -1 );
	const anMat3 &			GetAxis( int id = 0 ) const;

	void					SetLinearVelocity( const anVec3 &newLinearVelocity, int id = 0 );
	const anVec3 &			GetLinearVelocity( int id = 0 ) const;

	void					SetAngularVelocity( const anVec3 &newAngularVelocity, int id = 0 );
	const anVec3 &			GetAngularVelocity( int id = 0 ) const;

	void					SaveState( void );
	void					RestoreState( void );

	void					SetPushed( int deltaTime );
	const anVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;

	void					SetMaster( anEntity *master, const bool orientated = true );

	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	void					GetImpactInfo( const int id, const anVec3 &point, impactInfo_t *info ) const;
	void					ApplyImpulse( const int id, const anVec3 &point, const anVec3 &impulse );

	void					Translate( const anVec3 &translation, int id = -1 );
	void					PutToRest( void );
	void					Rest( int time );
	bool					IsPushable( void ) const;

	virtual void			Activate( void );
	virtual bool			IsAtRest( void ) const;
	virtual void			SetComeToRest( bool value );

	void					SetGroundPosition( const anVec3 &position );

	virtual void			EnableImpact( void );
	virtual void			DisableImpact( void );

private:
	int						atRest;
	bool					rotates;
	bool					locked;

	// physics state
	simplePState_t			current;
	simplePState_t			saved;

	float					groundLevel;

	anVec3					centerOfMass;
	anMat3					inertiaTensor;
	anMat3					inverseInertiaTensor;
};

#endif // !__PHYSICS_SIMPLE_H_