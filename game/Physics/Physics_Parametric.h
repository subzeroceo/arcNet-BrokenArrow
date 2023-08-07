
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
	anVec3									origin;					// world origin
	anAngles								angles;					// world angles
	anMat3									axis;					// world axis
	anVec3									localOrigin;			// local origin
	anAngles								localAngles;			// local angles
	anExtrapolate<anVec3>					linearExtrapolation;	// extrapolation based description of the position over time
	anExtrapolate<anAngles>					angularExtrapolation;	// extrapolation based description of the orientation over time
	idInterpolateAccelDecelLinear<anVec3>	linearInterpolation;	// interpolation based description of the position over time
	idInterpolateAccelDecelLinear<anAngles>	angularInterpolation;	// interpolation based description of the orientation over time
	anCurve_Spline<anVec3> *				spline;					// spline based description of the position over time
	idInterpolateAccelDecelLinear<float>	splineInterpolate;		// position along the spline over time
	bool									useSplineAngles;		// set the orientation using the spline
} parametricPState_t;

class anPhysics_Parametric : public anPhysics_Base {

public:
	CLASS_PROTOTYPE( anPhysics_Parametric );

							anPhysics_Parametric( void );
							~anPhysics_Parametric( void );

	void					Save( anSaveGame *savefile ) const;
	void					Restore( anRestoreGame *savefile );

	void					SetPusher( int flags );
	bool					IsPusher( void ) const;

	void					SetLinearExtrapolation( extrapolation_t type, int time, int duration, const anVec3 &base, const anVec3 &speed, const anVec3 &baseSpeed );
	void					SetAngularExtrapolation( extrapolation_t type, int time, int duration, const anAngles &base, const anAngles &speed, const anAngles &baseSpeed );
	extrapolation_t			GetLinearExtrapolationType( void ) const;
	extrapolation_t			GetAngularExtrapolationType( void ) const;

	void					SetLinearInterpolation( int time, int accelTime, int decelTime, int duration, const anVec3 &startPos, const anVec3 &endPos );
	void					SetAngularInterpolation( int time, int accelTime, int decelTime, int duration, const anAngles &startAng, const anAngles &endAng );

	void					SetSpline( anCurve_Spline<anVec3> *spline, int accelTime, int decelTime, bool useSplineAngles );
	anCurve_Spline<anVec3> *GetSpline( void ) const;
	int						GetSplineAcceleration( void ) const;
	int						GetSplineDeceleration( void ) const;
	bool					UsingSplineAngles( void ) const;

	void					GetLocalOrigin( anVec3 &curOrigin ) const;
	void					GetLocalAngles( anAngles &curAngles ) const;

	void					GetAngles( anAngles &curAngles ) const;


// abahr: a method for hiding gimblelock
	void					SetAxisOffset( const anMat3 &offset ) { axisOffset = offset; useAxisOffset = true; }
	const anMat3&			GetAxisOffset() const { return axisOffset; }
	anMat3&					GetAxisOffset() { return axisOffset; }
	bool					UseAxisOffset() const { return useAxisOffset; }


public:	// common physics interface
	void					SetClipModel( anClipModel *model, float density, int id = 0, bool freeOld = true );
	anClipModel *			GetClipModel( int id = 0 ) const;
	int						GetNumClipModels( void ) const;

	void					SetMass( float mass, int id = -1 );
	float					GetMass( int id = -1 ) const;

	void					SetContents( int contents, int id = -1 );
	int						GetContents( int id = -1 ) const;

	const anBounds &		GetBounds( int id = -1 ) const;
	const anBounds &		GetAbsBounds( int id = -1 ) const;

	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	void					Activate( void );
	bool					IsAtRest( void ) const;
	int						GetRestStartTime( void ) const;
	bool					IsPushable( void ) const;

	void					SaveState( void );
	void					RestoreState( void );

	void					SetOrigin( const anVec3 &newOrigin, int id = -1 );
	void					SetAxis( const anMat3 &newAxis, int id = -1 );

	void					Translate( const anVec3 &translation, int id = -1 );
	void					Rotate( const anRotation &rotation, int id = -1 );

	const anVec3 &			GetOrigin( int id = 0 ) const;
	const anMat3 &			GetAxis( int id = 0 ) const;

	void					SetLinearVelocity( const anVec3 &newLinearVelocity, int id = 0 );
	void					SetAngularVelocity( const anVec3 &newAngularVelocity, int id = 0 );

	const anVec3 &			GetLinearVelocity( int id = 0 ) const;
	const anVec3 &			GetAngularVelocity( int id = 0 ) const;

	void					DisableClip( void );
	void					EnableClip( void );

	void					UnlinkClip( void );
	void					LinkClip( void );

	void					SetMaster( anEntity *master, const bool orientated = true );

	const trace_t *			GetBlockingInfo( void ) const;
	anEntity *				GetBlockingEntity( void ) const;

	int						GetLinearEndTime( void ) const;
	int						GetAngularEndTime( void ) const;

	void					WriteToSnapshot( anBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const anBitMsgDelta &msg );

private:
	// parametric physics state
	parametricPState_t		current;
	parametricPState_t		saved;

	// pusher
	bool					isPusher;
	anClipModel *			clipModel;
	int						pushFlags;

	// results of last evaluate
	trace_t					pushResults;
	bool					isBlocked;

	// master
	bool					hasMaster;
	bool					isOrientated;


// abahr: a method for hiding gimblelock
	bool					useAxisOffset;
	anMat3					axisOffset;


private:
	bool					TestIfAtRest( void ) const;
	void					Rest( void );
};

#endif /* !__PHYSICS_PARAMETRIC_H__ */
