#ifndef __PHYSICS_PARABOLA_H__
#define __PHYSICS_PARABOLA_H__

#include "Physics_Base.h"

class arcPhysics_Parabola : public arcPhysics_Base {
public:
	typedef struct parabolaPState_s {
		int					time;
		arcVec3				origin;
		arcVec3				velocity;
	} parabolaPState_t;

	CLASS_PROTOTYPE( arcPhysics_Parabola );

							arcPhysics_Parabola( void );
	virtual					~arcPhysics_Parabola( void );

	void					Init( const arcVec3 &origin, const arcVec3 &velocity, const arcVec3 &acceleration, const arcMat3 &axes, int _startTime, int _endTime );

	virtual void			MakeDefault( void );

	virtual int				GetNumClipModels( void ) const { return clipModel ? 1 : 0; }

	virtual arcVec3			EvaluatePosition( void ) const;
	virtual bool			Evaluate( int timeStepMSec, int endTimeMSec );

	void					AdjustForMaster( arcVec3 &org, arcMat3 &axes );
	void					CalcProperties( arcVec3 &origin, arcVec3 &velocity, int time ) const;

	void					CheckWater( void );
	bool					CheckForCollisions( parabolaPState_t &next, trace_t &collision );
	bool					CollisionResponse( trace_t &collision );

	virtual void			SetClipModel( arcClipModel *model, float density = 0.f, int id = 0, bool freeOld = true );
	virtual arcClipModel *GetClipModel( int id = 0 ) const;

	virtual void			SetAxis( const arcMat3 &newAxis, int id );
	virtual void			SetOrigin( const arcVec3 &newOrigin, int id );

	virtual void			LinkClip( void );

	virtual const arcVec3 &	GetLinearVelocity( int id ) const { return current.velocity; }
	virtual const arcVec3 &	GetOrigin( int id ) const { return current.origin; }
	virtual const arcMat3 &	GetAxis( int id ) const { return baseAxes; }

	virtual const arcBounds &	GetBounds( int id = -1 ) const;
	virtual const arcBounds &	GetAbsBounds( int id = -1 ) const;
	
	virtual void			SetContents( int mask, int id = -1 );

	virtual float			InWater( void ) const { return waterLevel; }

private:
	parabolaPState_t		current;

	arcVec3					baseOrg;
	arcVec3					baseVelocity;
	arcVec3					baseAcceleration;
	arcMat3					baseAxes;
	int						startTime;
	int						endTime;
	float					waterLevel;
	arcCQuat					orientation;
	arcVec3					position;
	arcVec3					velocity;
	arcVec3					acceleration;

	int						startTime;
	int						endTime;
	arcClipModel *			clipModel;
};

#endif // __PHYSICS_PARABOLA_H__
