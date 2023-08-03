#ifndef __PHYSICS_PARABOLA_H__
#define __PHYSICS_PARABOLA_H__

#include "Physics_Base.h"

class anPhysics_Parabola : public anPhysics_Base {
public:
	typedef struct parabolaPState_s {
		int					time;
		anVec3				origin;
		anVec3				velocity;
	} parabolaPState_t;

	CLASS_PROTOTYPE( anPhysics_Parabola );

							anPhysics_Parabola( void );
	virtual					~anPhysics_Parabola( void );

	void					Init( const anVec3 &origin, const anVec3 &velocity, const anVec3 &acceleration, const anMat3 &axes, int _startTime, int _endTime );

	virtual void			MakeDefault( void );

	virtual int				GetNumClipModels( void ) const { return clipModel ? 1 : 0; }

	virtual anVec3			EvaluatePosition( void ) const;
	virtual bool			Evaluate( int timeStepMSec, int endTimeMSec );

	void					AdjustForMaster( anVec3 &org, anMat3 &axes );
	void					CalcProperties( anVec3 &origin, anVec3 &velocity, int time ) const;

	void					CheckWater( void );
	bool					CheckForCollisions( parabolaPState_t &next, trace_t &collision );
	bool					CollisionResponse( trace_t &collision );

	virtual void			SetClipModel( anClipModel *model, float density = 0.f, int id = 0, bool freeOld = true );
	virtual anClipModel *GetClipModel( int id = 0 ) const;

	virtual void			SetAxis( const anMat3 &newAxis, int id );
	virtual void			SetOrigin( const anVec3 &newOrigin, int id );

	virtual void			LinkClip( void );

	virtual const anVec3 &	GetLinearVelocity( int id ) const { return current.velocity; }
	virtual const anVec3 &	GetOrigin( int id ) const { return current.origin; }
	virtual const anMat3 &	GetAxis( int id ) const { return baseAxes; }

	virtual const anBounds &	GetBounds( int id = -1 ) const;
	virtual const anBounds &	GetAbsBounds( int id = -1 ) const;

	virtual void			SetContents( int mask, int id = -1 );

	virtual float			InWater( void ) const { return waterLevel; }

private:
	parabolaPState_t		current;

	anVec3					baseOrg;
	anVec3					baseVelocity;
	anVec3					baseAcceleration;
	anMat3					baseAxes;
	int						startTime;
	int						endTime;
	float					waterLevel;
	anCQuat					orientation;
	anVec3					position;
	anVec3					velocity;
	anVec3					acceleration;

	int						startTime;
	int						endTime;
	anClipModel *			clipModel;
};

#endif // __PHYSICS_PARABOLA_H__
