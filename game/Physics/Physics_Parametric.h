
#ifndef __PHYSICS_PARAMETRIC_H__
#define __PHYSICS_PARAMETRIC_H__

/*
===================================================================================

	Parametric physics

	Used for predefined or scripted motion. The motion of an object is completely
	parametrized. By adjusting the parameters an object is forced to follow a
	predefined path. The parametric physics is typically used for doors, bridges,
	rotating fans etc.

===================================================================================
*/

typedef struct parametricPState_s {
	int										time;					// physics time
	int										atRest;					// set when simulation is suspended
	arcVec3									origin;					// world origin
	idAngles								angles;					// world angles
	arcMat3									axis;					// world axis
	arcVec3									localOrigin;			// local origin
	idAngles								localAngles;			// local angles
	idExtrapolate<arcVec3>					linearExtrapolation;	// extrapolation based description of the position over time
	idExtrapolate<idAngles>					angularExtrapolation;	// extrapolation based description of the orientation over time
	idInterpolateAccelDecelLinear<arcVec3>	linearInterpolation;	// interpolation based description of the position over time
	idInterpolateAccelDecelLinear<idAngles>	angularInterpolation;	// interpolation based description of the orientation over time
	idCurve_Spline<arcVec3> *				spline;					// spline based description of the position over time
	idInterpolateAccelDecelLinear<float>	splineInterpolate;		// position along the spline over time
	bool									useSplineAngles;		// set the orientation using the spline
} parametricPState_t;

class idPhysics_Parametric : public idPhysics_Base {

public:
	CLASS_PROTOTYPE( idPhysics_Parametric );

							idPhysics_Parametric( void );
							~idPhysics_Parametric( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					SetPusher( int flags );
	bool					IsPusher( void ) const;

	void					SetLinearExtrapolation( extrapolation_t type, int time, int duration, const arcVec3 &base, const arcVec3 &speed, const arcVec3 &baseSpeed );
	void					SetAngularExtrapolation( extrapolation_t type, int time, int duration, const idAngles &base, const idAngles &speed, const idAngles &baseSpeed );
	extrapolation_t			GetLinearExtrapolationType( void ) const;
	extrapolation_t			GetAngularExtrapolationType( void ) const;

	void					SetLinearInterpolation( int time, int accelTime, int decelTime, int duration, const arcVec3 &startPos, const arcVec3 &endPos );
	void					SetAngularInterpolation( int time, int accelTime, int decelTime, int duration, const idAngles &startAng, const idAngles &endAng );

	void					SetSpline( idCurve_Spline<arcVec3> *spline, int accelTime, int decelTime, bool useSplineAngles );
	idCurve_Spline<arcVec3> *GetSpline( void ) const;
	int						GetSplineAcceleration( void ) const;
	int						GetSplineDeceleration( void ) const;
	bool					UsingSplineAngles( void ) const;

	void					GetLocalOrigin( arcVec3 &curOrigin ) const;
	void					GetLocalAngles( idAngles &curAngles ) const;

	void					GetAngles( idAngles &curAngles ) const;

// RAVEN BEGIN
// abahr: a method for hiding gimblelock
	void					SetAxisOffset( const arcMat3& offset ) { axisOffset = offset; useAxisOffset = true; }
	const arcMat3&			GetAxisOffset() const { return axisOffset; }
	arcMat3&					GetAxisOffset() { return axisOffset; }
	bool					UseAxisOffset() const { return useAxisOffset; }
// RAVEN END

public:	// common physics interface
	void					SetClipModel( idClipModel *model, float density, int id = 0, bool freeOld = true );
	idClipModel *			GetClipModel( int id = 0 ) const;
	int						GetNumClipModels( void ) const;

	void					SetMass( float mass, int id = -1 );
	float					GetMass( int id = -1 ) const;

	void					SetContents( int contents, int id = -1 );
	int						GetContents( int id = -1 ) const;

	const arcBounds &		GetBounds( int id = -1 ) const;
	const arcBounds &		GetAbsBounds( int id = -1 ) const;

	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	void					Activate( void );
	bool					IsAtRest( void ) const;
	int						GetRestStartTime( void ) const;
	bool					IsPushable( void ) const;

	void					SaveState( void );
	void					RestoreState( void );

	void					SetOrigin( const arcVec3 &newOrigin, int id = -1 );
	void					SetAxis( const arcMat3 &newAxis, int id = -1 );

	void					Translate( const arcVec3 &translation, int id = -1 );
	void					Rotate( const idRotation &rotation, int id = -1 );

	const arcVec3 &			GetOrigin( int id = 0 ) const;
	const arcMat3 &			GetAxis( int id = 0 ) const;

	void					SetLinearVelocity( const arcVec3 &newLinearVelocity, int id = 0 );
	void					SetAngularVelocity( const arcVec3 &newAngularVelocity, int id = 0 );

	const arcVec3 &			GetLinearVelocity( int id = 0 ) const;
	const arcVec3 &			GetAngularVelocity( int id = 0 ) const;

	void					DisableClip( void );
	void					EnableClip( void );

	void					UnlinkClip( void );
	void					LinkClip( void );

	void					SetMaster( idEntity *master, const bool orientated = true );

	const trace_t *			GetBlockingInfo( void ) const;
	idEntity *				GetBlockingEntity( void ) const;

	int						GetLinearEndTime( void ) const;
	int						GetAngularEndTime( void ) const;

	void					WriteToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const idBitMsgDelta &msg );

private:
	// parametric physics state
	parametricPState_t		current;
	parametricPState_t		saved;

	// pusher
	bool					isPusher;
	idClipModel *			clipModel;
	int						pushFlags;

	// results of last evaluate
	trace_t					pushResults;
	bool					isBlocked;

	// master
	bool					hasMaster;
	bool					isOrientated;

// RAVEN BEGIN
// abahr: a method for hiding gimblelock
	bool					useAxisOffset;
	arcMat3					axisOffset;
// RAVEN END

private:
	bool					TestIfAtRest( void ) const;
	void					Rest( void );
};

#endif /* !__PHYSICS_PARAMETRIC_H__ */
