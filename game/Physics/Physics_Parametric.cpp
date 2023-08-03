
#include "../../idlib/Lib.h"
#pragma hdrstop

#include "../Game_local.h"

CLASS_DECLARATION( anPhysics_Base, anPhysics_Parametric )
END_CLASS


/*
================
anPhysics_Parametric::Activate
================
*/
void anPhysics_Parametric::Activate( void ) {
	current.atRest = -1;
	self->BecomeActive( TH_PHYSICS );
}

/*
================
anPhysics_Parametric::TestIfAtRest
================
*/
bool anPhysics_Parametric::TestIfAtRest( void ) const {

	if ( ( current.linearExtrapolation.GetExtrapolationType() & ~EXTRAPOLATION_NOSTOP ) == EXTRAPOLATION_NONE &&
			( current.angularExtrapolation.GetExtrapolationType() & ~EXTRAPOLATION_NOSTOP ) == EXTRAPOLATION_NONE &&
				current.linearInterpolation.GetDuration() == 0 &&
					current.angularInterpolation.GetDuration() == 0 &&
						current.spline == nullptr ) {
		return true;
	}

	if ( !current.linearExtrapolation.IsDone( current.time ) ) {
		return false;
	}

	if ( !current.angularExtrapolation.IsDone( current.time ) ) {
		return false;
	}

	if ( !current.linearInterpolation.IsDone( current.time ) ) {
		return false;
	}

	if ( !current.angularInterpolation.IsDone( current.time ) ) {
		return false;
	}

	if ( current.spline != nullptr && !current.spline->IsDone( current.time ) ) {
		return false;
	}

	return true;
}

/*
================
anPhysics_Parametric::Rest
================
*/
void anPhysics_Parametric::Rest( void ) {
	current.atRest = gameLocal.time;
	self->BecomeInactive( TH_PHYSICS );
}

/*
================
anPhysics_Parametric::anPhysics_Parametric
================
*/
anPhysics_Parametric::anPhysics_Parametric( void ) {

	current.time = gameLocal.time;
	current.atRest = -1;
	current.useSplineAngles = false;
	current.origin.Zero();
	current.angles.Zero();
	current.axis.Identity();
	current.localOrigin.Zero();
	current.localAngles.Zero();
	current.linearExtrapolation.Init( 0, 0, vec3_zero, vec3_zero, vec3_zero, EXTRAPOLATION_NONE );
	current.angularExtrapolation.Init( 0, 0, ang_zero, ang_zero, ang_zero, EXTRAPOLATION_NONE );
	current.linearInterpolation.Init( 0, 0, 0, 0, vec3_zero, vec3_zero );
	current.angularInterpolation.Init( 0, 0, 0, 0, ang_zero, ang_zero );
	current.spline = nullptr;
	current.splineInterpolate.Init( 0, 1, 1, 2, 0, 0 );

	saved = current;

	isPusher = false;
	pushFlags = 0;
	clipModel = nullptr;
	isBlocked = false;
	memset( &pushResults, 0, sizeof( pushResults ) );

	hasMaster = false;
	isOrientated = false;


// abahr:
	useAxisOffset = false;
	axisOffset.Identity();

}

/*
================
anPhysics_Parametric::~anPhysics_Parametric
================
*/
anPhysics_Parametric::~anPhysics_Parametric( void ) {
	if ( clipModel != nullptr ) {
		delete clipModel;
		clipModel = nullptr;
	}
	if ( current.spline != nullptr ) {
		delete current.spline;
		current.spline = nullptr;
	}
}

/*
================
anPhysics_Parametric_SavePState
================
*/
void anPhysics_Parametric_SavePState( anSaveGame *savefile, const parametricPState_t &state ) {
	savefile->WriteInt( state.time );
	savefile->WriteInt( state.atRest );
	savefile->WriteBool( state.useSplineAngles );
	savefile->WriteVec3( state.origin );
	savefile->WriteAngles( state.angles );
	savefile->WriteMat3( state.axis );
	savefile->WriteVec3( state.localOrigin );
	savefile->WriteAngles( state.localAngles );

	savefile->WriteInt( (int)state.linearExtrapolation.GetExtrapolationType() );
	savefile->WriteFloat( state.linearExtrapolation.GetStartTime() );
	savefile->WriteFloat( state.linearExtrapolation.GetDuration() );
	savefile->WriteVec3( state.linearExtrapolation.GetStartValue() );
	savefile->WriteVec3( state.linearExtrapolation.GetBaseSpeed() );
	savefile->WriteVec3( state.linearExtrapolation.GetSpeed() );

	savefile->WriteInt( (int)state.angularExtrapolation.GetExtrapolationType() );
	savefile->WriteFloat( state.angularExtrapolation.GetStartTime() );
	savefile->WriteFloat( state.angularExtrapolation.GetDuration() );
	savefile->WriteAngles( state.angularExtrapolation.GetStartValue() );
	savefile->WriteAngles( state.angularExtrapolation.GetBaseSpeed() );
	savefile->WriteAngles( state.angularExtrapolation.GetSpeed() );

	savefile->WriteFloat( state.linearInterpolation.GetStartTime() );
	savefile->WriteFloat( state.linearInterpolation.GetAcceleration() );
	savefile->WriteFloat( state.linearInterpolation.GetDeceleration() );
	savefile->WriteFloat( state.linearInterpolation.GetDuration() );
	savefile->WriteVec3( state.linearInterpolation.GetStartValue() );
	savefile->WriteVec3( state.linearInterpolation.GetEndValue() );

	savefile->WriteFloat( state.angularInterpolation.GetStartTime() );
	savefile->WriteFloat( state.angularInterpolation.GetAcceleration() );
	savefile->WriteFloat( state.angularInterpolation.GetDeceleration() );
	savefile->WriteFloat( state.angularInterpolation.GetDuration() );
	savefile->WriteAngles( state.angularInterpolation.GetStartValue() );
	savefile->WriteAngles( state.angularInterpolation.GetEndValue() );

	// spline is handled by owner

	savefile->WriteFloat( state.splineInterpolate.GetStartTime() );
	savefile->WriteFloat( state.splineInterpolate.GetAcceleration() );
	savefile->WriteFloat( state.splineInterpolate.GetDuration() );
	savefile->WriteFloat( state.splineInterpolate.GetDeceleration() );
	savefile->WriteFloat( state.splineInterpolate.GetStartValue() );
	savefile->WriteFloat( state.splineInterpolate.GetEndValue() );
}

/*
================
anPhysics_Parametric_RestorePState
================
*/
void anPhysics_Parametric_RestorePState( anRestoreGame *savefile, parametricPState_t &state ) {
	extrapolation_t etype;
	float startTime, duration, accelTime, decelTime, startValue, endValue;
	anVec3 linearStartValue, linearBaseSpeed, linearSpeed, startPos, endPos;
	anAngles angularStartValue, angularBaseSpeed, angularSpeed, startAng, endAng;

	savefile->ReadInt( state.time );
	savefile->ReadInt( state.atRest );
	savefile->ReadBool( state.useSplineAngles );
	savefile->ReadVec3( state.origin );
	savefile->ReadAngles( state.angles );
	savefile->ReadMat3( state.axis );
	savefile->ReadVec3( state.localOrigin );
	savefile->ReadAngles( state.localAngles );

	savefile->ReadInt( ( int&)etype );
	savefile->ReadFloat( startTime );
	savefile->ReadFloat( duration );
	savefile->ReadVec3( linearStartValue );
	savefile->ReadVec3( linearBaseSpeed );
	savefile->ReadVec3( linearSpeed );

	state.linearExtrapolation.Init( startTime, duration, linearStartValue, linearBaseSpeed, linearSpeed, etype );

	savefile->ReadInt( ( int&)etype );
	savefile->ReadFloat( startTime );
	savefile->ReadFloat( duration );
	savefile->ReadAngles( angularStartValue );
	savefile->ReadAngles( angularBaseSpeed );
	savefile->ReadAngles( angularSpeed );

	state.angularExtrapolation.Init( startTime, duration, angularStartValue, angularBaseSpeed, angularSpeed, etype );

	savefile->ReadFloat( startTime );
	savefile->ReadFloat( accelTime );
	savefile->ReadFloat( decelTime );
	savefile->ReadFloat( duration );
	savefile->ReadVec3( startPos );
	savefile->ReadVec3( endPos );

	state.linearInterpolation.Init( startTime, accelTime, decelTime, duration, startPos, endPos );

	savefile->ReadFloat( startTime );
	savefile->ReadFloat( accelTime );
	savefile->ReadFloat( decelTime );
	savefile->ReadFloat( duration );
	savefile->ReadAngles( startAng );
	savefile->ReadAngles( endAng );

	state.angularInterpolation.Init( startTime, accelTime, decelTime, duration, startAng, endAng );

	// spline is handled by owner

	savefile->ReadFloat( startTime );
	savefile->ReadFloat( accelTime );
	savefile->ReadFloat( duration );
	savefile->ReadFloat( decelTime );
	savefile->ReadFloat( startValue );
	savefile->ReadFloat( endValue );

	state.splineInterpolate.Init( startTime, accelTime, decelTime, duration, startValue, endValue );
}

/*
================
anPhysics_Parametric::Save
================
*/
void anPhysics_Parametric::Save( anSaveGame *savefile ) const {

	anPhysics_Parametric_SavePState( savefile, current );
	anPhysics_Parametric_SavePState( savefile, saved );

	savefile->WriteBool( isPusher );
	savefile->WriteClipModel( clipModel );
	savefile->WriteInt( pushFlags );

	savefile->WriteTrace( pushResults );
	savefile->WriteBool( isBlocked );

	savefile->WriteBool( hasMaster );
	savefile->WriteBool( isOrientated );

	savefile->WriteBool ( useAxisOffset );	// cnicholson: Added unsaved var
	savefile->WriteMat3 ( axisOffset );		// cnicholson: Added unsaved var
}

/*
================
anPhysics_Parametric::Restore
================
*/
void anPhysics_Parametric::Restore( anRestoreGame *savefile ) {

	anPhysics_Parametric_RestorePState( savefile, current );
	anPhysics_Parametric_RestorePState( savefile, saved );

	savefile->ReadBool( isPusher );
	savefile->ReadClipModel( clipModel );
	savefile->ReadInt( pushFlags );

	savefile->ReadTrace( pushResults );
	savefile->ReadBool( isBlocked );

	savefile->ReadBool( hasMaster );
	savefile->ReadBool( isOrientated );

	savefile->ReadBool ( useAxisOffset );
	savefile->ReadMat3 ( axisOffset );

}

/*
================
anPhysics_Parametric::SetPusher
================
*/
void anPhysics_Parametric::SetPusher( int flags ) {
	assert( clipModel );
	isPusher = true;
	pushFlags = flags;
}

/*
================
anPhysics_Parametric::IsPusher
================
*/
bool anPhysics_Parametric::IsPusher( void ) const {
	return isPusher;
}

/*
================
anPhysics_Parametric::SetLinearExtrapolation
================
*/
void anPhysics_Parametric::SetLinearExtrapolation( extrapolation_t type, int time, int duration, const anVec3 &base, const anVec3 &speed, const anVec3 &baseSpeed ) {
	current.time = gameLocal.time;
	current.linearExtrapolation.Init( time, duration, base, baseSpeed, speed, type );
	current.localOrigin = base;
	Activate();
}

/*
================
anPhysics_Parametric::SetAngularExtrapolation
================
*/
void anPhysics_Parametric::SetAngularExtrapolation( extrapolation_t type, int time, int duration, const anAngles &base, const anAngles &speed, const anAngles &baseSpeed ) {
	current.time = gameLocal.time;
	current.angularExtrapolation.Init( time, duration, base, baseSpeed, speed, type );
	current.localAngles = base;
	Activate();
}

/*
================
anPhysics_Parametric::GetLinearExtrapolationType
================
*/
extrapolation_t anPhysics_Parametric::GetLinearExtrapolationType( void ) const {
	return current.linearExtrapolation.GetExtrapolationType();
}

/*
================
anPhysics_Parametric::GetAngularExtrapolationType
================
*/
extrapolation_t anPhysics_Parametric::GetAngularExtrapolationType( void ) const {
	return current.angularExtrapolation.GetExtrapolationType();
}

/*
================
anPhysics_Parametric::SetLinearInterpolation
================
*/
void anPhysics_Parametric::SetLinearInterpolation( int time, int accelTime, int decelTime, int duration, const anVec3 &startPos, const anVec3 &endPos ) {
	current.time = gameLocal.time;
	current.linearInterpolation.Init( time, accelTime, decelTime, duration, startPos, endPos );
	current.localOrigin = startPos;
	Activate();
}

/*
================
anPhysics_Parametric::SetAngularInterpolation
================
*/
void anPhysics_Parametric::SetAngularInterpolation( int time, int accelTime, int decelTime, int duration, const anAngles &startAng, const anAngles &endAng ) {
	current.time = gameLocal.time;
	current.angularInterpolation.Init( time, accelTime, decelTime, duration, startAng, endAng );
	current.localAngles = startAng;
	Activate();
}

/*
================
anPhysics_Parametric::SetSpline
================
*/
void anPhysics_Parametric::SetSpline( anCurve_Spline<anVec3> *spline, int accelTime, int decelTime, bool useSplineAngles ) {
	if ( current.spline != nullptr ) {
		delete current.spline;
		current.spline = nullptr;
	}
	current.spline = spline;
	if ( current.spline != nullptr ) {
		float startTime = current.spline->GetTime( 0 );
		float endTime = current.spline->GetTime( current.spline->GetNumValues() - 1 );
		float length = current.spline->GetLengthForTime( endTime );
		current.splineInterpolate.Init( startTime, accelTime, decelTime, endTime - startTime, 0.0f, length );
	}
	current.useSplineAngles = useSplineAngles;
	Activate();
}

/*
================
anPhysics_Parametric::GetSpline
================
*/
anCurve_Spline<anVec3> *anPhysics_Parametric::GetSpline( void ) const {
	return current.spline;
}

/*
================
anPhysics_Parametric::GetSplineAcceleration
================
*/
int anPhysics_Parametric::GetSplineAcceleration( void ) const {
	return current.splineInterpolate.GetAcceleration();
}

/*
================
anPhysics_Parametric::GetSplineDeceleration
================
*/
int anPhysics_Parametric::GetSplineDeceleration( void ) const {
	return current.splineInterpolate.GetDeceleration();
}

/*
================
anPhysics_Parametric::UsingSplineAngles
================
*/
bool anPhysics_Parametric::UsingSplineAngles( void ) const {
	return current.useSplineAngles;
}

/*
================
anPhysics_Parametric::GetLocalOrigin
================
*/
void anPhysics_Parametric::GetLocalOrigin( anVec3 &curOrigin ) const {
	curOrigin = current.localOrigin;
}

/*
================
anPhysics_Parametric::GetLocalAngles
================
*/
void anPhysics_Parametric::GetLocalAngles( anAngles &curAngles ) const {
	curAngles = current.localAngles;
}

/*
================
anPhysics_Parametric::SetClipModel
================
*/
void anPhysics_Parametric::SetClipModel( anClipModel *model, float density, int id, bool freeOld ) {

	assert( self );
	assert( model );

	if ( clipModel && clipModel != model && freeOld ) {
		delete clipModel;
	}
	clipModel = model;


	clipModel->Link( self, 0, current.origin, current.axis );

}

/*
================
anPhysics_Parametric::GetClipModel
================
*/
anClipModel *anPhysics_Parametric::GetClipModel( int id ) const {
	return clipModel;
}

/*
================
anPhysics_Parametric::GetNumClipModels
================
*/
int anPhysics_Parametric::GetNumClipModels( void ) const {
	return ( clipModel != nullptr );
}

/*
================
anPhysics_Parametric::SetMass
================
*/
void anPhysics_Parametric::SetMass( float mass, int id ) {
}

/*
================
anPhysics_Parametric::GetMass
================
*/
float anPhysics_Parametric::GetMass( int id ) const {
	return 0.0f;
}

/*
================
anPhysics_Parametric::SetClipMask
================
*/
void anPhysics_Parametric::SetContents( int contents, int id ) {
	if ( clipModel ) {
		clipModel->SetContents( contents );
	}
}

/*
================
anPhysics_Parametric::SetClipMask
================
*/
int anPhysics_Parametric::GetContents( int id ) const {
	if ( clipModel ) {
		return clipModel->GetContents();
	}
	return 0;
}

/*
================
anPhysics_Parametric::GetBounds
================
*/
const anBounds &anPhysics_Parametric::GetBounds( int id ) const {
	if ( clipModel ) {
		return clipModel->GetBounds();
	}
	return anPhysics_Base::GetBounds();
}

/*
================
anPhysics_Parametric::GetAbsBounds
================
*/
const anBounds &anPhysics_Parametric::GetAbsBounds( int id ) const {
	if ( clipModel ) {
		return clipModel->GetAbsBounds();
	}
	return anPhysics_Base::GetAbsBounds();
}

/*
================
anPhysics_Parametric::Evaluate
================
*/
bool anPhysics_Parametric::Evaluate( int timeStepMSec, int endTimeMSec ) {
	anVec3 oldLocalOrigin, oldOrigin, masterOrigin;
	anAngles oldLocalAngles, oldAngles;
	anMat3 oldAxis, masterAxis;

	isBlocked = false;
	oldLocalOrigin = current.localOrigin;
	oldOrigin = current.origin;
	oldLocalAngles = current.localAngles;
	oldAngles = current.angles;
	oldAxis = current.axis;

	current.localOrigin.Zero();
	current.localAngles.Zero();

	if ( current.spline != nullptr ) {
		float length = current.splineInterpolate.GetCurrentValue( endTimeMSec );
		float t = current.spline->GetTimeForLength( length, 0.01f );
		current.localOrigin = current.spline->GetCurrentValue( t );
		if ( current.useSplineAngles ) {
			current.localAngles = current.spline->GetCurrentFirstDerivative( t ).ToAngles();
		}
	} else if ( current.linearInterpolation.GetDuration() != 0 ) {
		current.localOrigin += current.linearInterpolation.GetCurrentValue( endTimeMSec );
	} else {
		current.localOrigin += current.linearExtrapolation.GetCurrentValue( endTimeMSec );
	}

	if ( current.angularInterpolation.GetDuration() != 0 ) {
		current.localAngles += current.angularInterpolation.GetCurrentValue( endTimeMSec );
	} else {
		current.localAngles += current.angularExtrapolation.GetCurrentValue( endTimeMSec );
	}

	current.localAngles.Normalize360();
	current.origin = current.localOrigin;
	current.angles = current.localAngles;
	current.axis = current.localAngles.ToMat3();

	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		if ( masterAxis.IsRotated() ) {
			current.origin = current.origin * masterAxis + masterOrigin;
			if ( isOrientated ) {
				current.axis *= masterAxis;
				current.angles = current.axis.ToAngles();
			}
		}
		else {
			current.origin += masterOrigin;
		}
	}

	if ( isPusher ) {

		gameLocal.push.ClipPush( pushResults, self, pushFlags, oldOrigin, oldAxis, current.origin, current.axis );
		if ( pushResults.fraction < 1.0f ) {


			clipModel->Link( self, 0, oldOrigin, oldAxis );

			current.localOrigin = oldLocalOrigin;
			current.origin = oldOrigin;
			current.localAngles = oldLocalAngles;
			current.angles = oldAngles;
			current.axis = oldAxis;
			isBlocked = true;
			return false;
		}

		current.angles = current.axis.ToAngles();
	}

	if ( clipModel ) {

// abahr: a hack way of hiding gimble lock from movers.
		clipModel->Link( self, 0, current.origin, UseAxisOffset() ? GetAxisOffset() * current.axis : current.axis );

	}

	current.time = endTimeMSec;

	if ( TestIfAtRest() ) {
		Rest();
	}

	return ( current.origin != oldOrigin || current.axis != oldAxis );
}

/*
================
anPhysics_Parametric::UpdateTime
================
*/
void anPhysics_Parametric::UpdateTime( int endTimeMSec ) {
	int timeLeap = endTimeMSec - current.time;

	current.time = endTimeMSec;
	// move the trajectory start times to sync the trajectory with the current endTime
	current.linearExtrapolation.SetStartTime( current.linearExtrapolation.GetStartTime() + timeLeap );
	current.angularExtrapolation.SetStartTime( current.angularExtrapolation.GetStartTime() + timeLeap );
	current.linearInterpolation.SetStartTime( current.linearInterpolation.GetStartTime() + timeLeap );
	current.angularInterpolation.SetStartTime( current.angularInterpolation.GetStartTime() + timeLeap );
	if ( current.spline != nullptr ) {
		current.spline->ShiftTime( timeLeap );
		current.splineInterpolate.SetStartTime( current.splineInterpolate.GetStartTime() + timeLeap );
	}
}

/*
================
anPhysics_Parametric::GetTime
================
*/
int anPhysics_Parametric::GetTime( void ) const {
	return current.time;
}

/*
================
anPhysics_Parametric::IsAtRest
================
*/
bool anPhysics_Parametric::IsAtRest( void ) const {
	return current.atRest >= 0;
}

/*
================
anPhysics_Parametric::GetRestStartTime
================
*/
int anPhysics_Parametric::GetRestStartTime( void ) const {
	return current.atRest;
}

/*
================
anPhysics_Parametric::IsPushable
================
*/
bool anPhysics_Parametric::IsPushable( void ) const {
	return false;
}

/*
================
anPhysics_Parametric::SaveState
================
*/
void anPhysics_Parametric::SaveState( void ) {
	saved = current;
}

/*
================
anPhysics_Parametric::RestoreState
================
*/
void anPhysics_Parametric::RestoreState( void ) {

	current = saved;

	if ( clipModel ) {


		clipModel->Link( self, 0, current.origin, current.axis );

	}
}

/*
================
anPhysics_Parametric::SetOrigin
================
*/
void anPhysics_Parametric::SetOrigin( const anVec3 &newOrigin, int id ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	current.linearExtrapolation.SetStartValue( newOrigin );
	current.linearInterpolation.SetStartValue( newOrigin );

	current.localOrigin = current.linearExtrapolation.GetCurrentValue( current.time );
	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + current.localOrigin * masterAxis;
	}
	else {
		current.origin = current.localOrigin;
	}
	if ( clipModel ) {


		clipModel->Link( self, 0, current.origin, current.axis );

	}
	Activate();
}

/*
================
anPhysics_Parametric::SetAxis
================
*/
void anPhysics_Parametric::SetAxis( const anMat3 &newAxis, int id ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	current.localAngles = newAxis.ToAngles();

	current.angularExtrapolation.SetStartValue( current.localAngles );
	current.angularInterpolation.SetStartValue( current.localAngles );

	current.localAngles = current.angularExtrapolation.GetCurrentValue( current.time );
	if ( hasMaster && isOrientated ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.axis = current.localAngles.ToMat3() * masterAxis;
		current.angles = current.axis.ToAngles();
	}
	else {
		current.axis = current.localAngles.ToMat3();
		current.angles = current.localAngles;
	}
	if ( clipModel ) {


		clipModel->Link( self, 0, current.origin, current.axis );

	}
	Activate();
}

/*
================
anPhysics_Parametric::Move
================
*/
void anPhysics_Parametric::Translate( const anVec3 &translation, int id ) {
}

/*
================
anPhysics_Parametric::Rotate
================
*/
void anPhysics_Parametric::Rotate( const anRotation &rotation, int id ) {
}

/*
================
anPhysics_Parametric::GetOrigin
================
*/
const anVec3 &anPhysics_Parametric::GetOrigin( int id ) const {
	return current.origin;
}

/*
================
anPhysics_Parametric::GetAxis
================
*/
const anMat3 &anPhysics_Parametric::GetAxis( int id ) const {
	return current.axis;
}

/*
================
anPhysics_Parametric::GetAngles
================
*/
void anPhysics_Parametric::GetAngles( anAngles &curAngles ) const {
	curAngles = current.angles;
}

/*
================
anPhysics_Parametric::SetLinearVelocity
================
*/
void anPhysics_Parametric::SetLinearVelocity( const anVec3 &newLinearVelocity, int id ) {
	SetLinearExtrapolation( extrapolation_t(EXTRAPOLATION_LINEAR|EXTRAPOLATION_NOSTOP), gameLocal.time, 0, current.origin, newLinearVelocity, vec3_origin );
	current.linearInterpolation.Init( 0, 0, 0, 0, vec3_zero, vec3_zero );
	Activate();
}

/*
================
anPhysics_Parametric::SetAngularVelocity
================
*/
void anPhysics_Parametric::SetAngularVelocity( const anVec3 &newAngularVelocity, int id ) {
	anRotation rotation;
	anVec3 vec;
	float angle;

	vec = newAngularVelocity;
	angle = vec.Normalize();
	rotation.Set( vec3_origin, vec, ( float ) RAD2DEG( angle ) );

	SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_LINEAR|EXTRAPOLATION_NOSTOP), gameLocal.time, 0, current.angles, rotation.ToAngles(), ang_zero );
	current.angularInterpolation.Init( 0, 0, 0, 0, ang_zero, ang_zero );
	Activate();
}

/*
================
anPhysics_Parametric::GetLinearVelocity
================
*/
const anVec3 &anPhysics_Parametric::GetLinearVelocity( int id ) const {
	static anVec3 curLinearVelocity;

	curLinearVelocity = current.linearExtrapolation.GetCurrentSpeed( gameLocal.time );
	return curLinearVelocity;
}

/*
================
anPhysics_Parametric::GetAngularVelocity
================
*/
const anVec3 &anPhysics_Parametric::GetAngularVelocity( int id ) const {
	static anVec3 curAngularVelocity;
	anAngles angles;

	angles = current.angularExtrapolation.GetCurrentSpeed( gameLocal.time );
	curAngularVelocity = angles.ToAngularVelocity();
	return curAngularVelocity;
}

/*
================
anPhysics_Parametric::DisableClip
================
*/
void anPhysics_Parametric::DisableClip( void ) {
	if ( clipModel ) {
		clipModel->Disable();
	}
}

/*
================
anPhysics_Parametric::EnableClip
================
*/
void anPhysics_Parametric::EnableClip( void ) {
	if ( clipModel ) {
		clipModel->Enable();
	}
}

/*
================
anPhysics_Parametric::UnlinkClip
================
*/
void anPhysics_Parametric::UnlinkClip( void ) {
	if ( clipModel ) {
		clipModel->Unlink();
	}
}

/*
================
anPhysics_Parametric::LinkClip
================
*/
void anPhysics_Parametric::LinkClip( void ) {
	if ( clipModel ) {


		clipModel->Link( self, 0, current.origin, current.axis );

	}
}

/*
================
anPhysics_Parametric::GetBlockingInfo
================
*/
const trace_t *anPhysics_Parametric::GetBlockingInfo( void ) const {
	return ( isBlocked ? &pushResults : nullptr );
}

/*
================
anPhysics_Parametric::GetBlockingEntity
================
*/
anEntity *anPhysics_Parametric::GetBlockingEntity( void ) const {
	if ( isBlocked ) {
		return gameLocal.entities[ pushResults.c.entityNum ];
	}
	return nullptr;
}

/*
================
anPhysics_Parametric::SetMaster
================
*/
void anPhysics_Parametric::SetMaster( anEntity *master, const bool orientated ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	if ( master ) {
		if ( !hasMaster ) {

			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
			if ( orientated ) {
				current.localAngles = ( current.axis * masterAxis.Transpose() ).ToAngles();
			}
			else {
				current.localAngles = current.axis.ToAngles();
			}

			current.linearExtrapolation.SetStartValue( current.localOrigin );
			current.angularExtrapolation.SetStartValue( current.localAngles );
			hasMaster = true;
			isOrientated = orientated;
		}
	}
	else {
		if ( hasMaster ) {
			// transform from master space to world space
			current.localOrigin = current.origin;
			current.localAngles = current.angles;
			SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, current.origin, vec3_origin, vec3_origin );
			SetAngularExtrapolation( EXTRAPOLATION_NONE, 0, 0, current.angles, ang_zero, ang_zero );
			hasMaster = false;
		}
	}
}

/*
================
anPhysics_Parametric::GetLinearEndTime
================
*/
int anPhysics_Parametric::GetLinearEndTime( void ) const {
	if ( current.spline != nullptr ) {
		if ( current.spline->GetBoundaryType() != anCurve_Spline<anVec3>::BT_CLOSED ) {
			return current.spline->GetTime( current.spline->GetNumValues() - 1 );
		} else {
			return 0;
		}
	} else if ( current.linearInterpolation.GetDuration() != 0 ) {
		return current.linearInterpolation.GetEndTime();
	} else {
		return current.linearExtrapolation.GetEndTime();
	}
}

/*
================
anPhysics_Parametric::GetAngularEndTime
================
*/
int anPhysics_Parametric::GetAngularEndTime( void ) const {
	if ( current.angularInterpolation.GetDuration() != 0 ) {
		return current.angularInterpolation.GetEndTime();
	} else {
		return current.angularExtrapolation.GetEndTime();
	}
}

/*
================
anPhysics_Parametric::WriteToSnapshot
================
*/
void anPhysics_Parametric::WriteToSnapshot( anBitMsgDelta &msg ) const {
	msg.WriteLong( current.time );
	msg.WriteLong( current.atRest );
	msg.WriteFloat( current.origin[0] );
	msg.WriteFloat( current.origin[1] );
	msg.WriteFloat( current.origin[2] );
	msg.WriteFloat( current.angles[0] );
	msg.WriteFloat( current.angles[1] );
	msg.WriteFloat( current.angles[2] );
	msg.WriteDeltaFloat( current.origin[0], current.localOrigin[0] );
	msg.WriteDeltaFloat( current.origin[1], current.localOrigin[1] );
	msg.WriteDeltaFloat( current.origin[2], current.localOrigin[2] );
	msg.WriteDeltaFloat( current.angles[0], current.localAngles[0] );
	msg.WriteDeltaFloat( current.angles[1], current.localAngles[1] );
	msg.WriteDeltaFloat( current.angles[2], current.localAngles[2] );

	msg.WriteBits( current.linearExtrapolation.GetExtrapolationType(), 8 );
	msg.WriteDeltaFloat( 0.0f, current.linearExtrapolation.GetStartTime() );
	msg.WriteDeltaFloat( 0.0f, current.linearExtrapolation.GetDuration() );
	msg.WriteDeltaFloat( 0.0f, current.linearExtrapolation.GetStartValue()[0] );
	msg.WriteDeltaFloat( 0.0f, current.linearExtrapolation.GetStartValue()[1] );
	msg.WriteDeltaFloat( 0.0f, current.linearExtrapolation.GetStartValue()[2] );
	msg.WriteDeltaFloat( 0.0f, current.linearExtrapolation.GetSpeed()[0] );
	msg.WriteDeltaFloat( 0.0f, current.linearExtrapolation.GetSpeed()[1] );
	msg.WriteDeltaFloat( 0.0f, current.linearExtrapolation.GetSpeed()[2] );
	msg.WriteDeltaFloat( 0.0f, current.linearExtrapolation.GetBaseSpeed()[0] );
	msg.WriteDeltaFloat( 0.0f, current.linearExtrapolation.GetBaseSpeed()[1] );
	msg.WriteDeltaFloat( 0.0f, current.linearExtrapolation.GetBaseSpeed()[2] );

	msg.WriteBits( current.angularExtrapolation.GetExtrapolationType(), 8 );
	msg.WriteDeltaFloat( 0.0f, current.angularExtrapolation.GetStartTime() );
	msg.WriteDeltaFloat( 0.0f, current.angularExtrapolation.GetDuration() );
	msg.WriteDeltaFloat( 0.0f, current.angularExtrapolation.GetStartValue()[0] );
	msg.WriteDeltaFloat( 0.0f, current.angularExtrapolation.GetStartValue()[1] );
	msg.WriteDeltaFloat( 0.0f, current.angularExtrapolation.GetStartValue()[2] );
	msg.WriteDeltaFloat( 0.0f, current.angularExtrapolation.GetSpeed()[0] );
	msg.WriteDeltaFloat( 0.0f, current.angularExtrapolation.GetSpeed()[1] );
	msg.WriteDeltaFloat( 0.0f, current.angularExtrapolation.GetSpeed()[2] );
	msg.WriteDeltaFloat( 0.0f, current.angularExtrapolation.GetBaseSpeed()[0] );
	msg.WriteDeltaFloat( 0.0f, current.angularExtrapolation.GetBaseSpeed()[1] );
	msg.WriteDeltaFloat( 0.0f, current.angularExtrapolation.GetBaseSpeed()[2] );

	msg.WriteDeltaFloat( 0.0f, current.linearInterpolation.GetStartTime() );
	msg.WriteDeltaFloat( 0.0f, current.linearInterpolation.GetAcceleration() );
	msg.WriteDeltaFloat( 0.0f, current.linearInterpolation.GetDeceleration() );
	msg.WriteDeltaFloat( 0.0f, current.linearInterpolation.GetDuration() );
	msg.WriteDeltaFloat( 0.0f, current.linearInterpolation.GetStartValue()[0] );
	msg.WriteDeltaFloat( 0.0f, current.linearInterpolation.GetStartValue()[1] );
	msg.WriteDeltaFloat( 0.0f, current.linearInterpolation.GetStartValue()[2] );
	msg.WriteDeltaFloat( 0.0f, current.linearInterpolation.GetEndValue()[0] );
	msg.WriteDeltaFloat( 0.0f, current.linearInterpolation.GetEndValue()[1] );
	msg.WriteDeltaFloat( 0.0f, current.linearInterpolation.GetEndValue()[2] );

	msg.WriteDeltaFloat( 0.0f, current.angularInterpolation.GetStartTime() );
	msg.WriteDeltaFloat( 0.0f, current.angularInterpolation.GetAcceleration() );
	msg.WriteDeltaFloat( 0.0f, current.angularInterpolation.GetDeceleration() );
	msg.WriteDeltaFloat( 0.0f, current.angularInterpolation.GetDuration() );
	msg.WriteDeltaFloat( 0.0f, current.angularInterpolation.GetStartValue()[0] );
	msg.WriteDeltaFloat( 0.0f, current.angularInterpolation.GetStartValue()[1] );
	msg.WriteDeltaFloat( 0.0f, current.angularInterpolation.GetStartValue()[2] );
	msg.WriteDeltaFloat( 0.0f, current.angularInterpolation.GetEndValue()[0] );
	msg.WriteDeltaFloat( 0.0f, current.angularInterpolation.GetEndValue()[1] );
	msg.WriteDeltaFloat( 0.0f, current.angularInterpolation.GetEndValue()[2] );
}

/*
================
anPhysics_Parametric::ReadFromSnapshot
================
*/
void anPhysics_Parametric::ReadFromSnapshot( const anBitMsgDelta &msg ) {
	extrapolation_t linearType, angularType;
	float startTime, duration, accelTime, decelTime;
	anVec3 linearStartValue, linearSpeed, linearBaseSpeed, startPos, endPos;
	anAngles angularStartValue, angularSpeed, angularBaseSpeed, startAng, endAng;

	current.time = msg.ReadLong();
	current.atRest = msg.ReadLong();
	current.origin[0] = msg.ReadFloat();
	current.origin[1] = msg.ReadFloat();
	current.origin[2] = msg.ReadFloat();
	current.angles[0] = msg.ReadFloat();
	current.angles[1] = msg.ReadFloat();
	current.angles[2] = msg.ReadFloat();
	current.localOrigin[0] = msg.ReadDeltaFloat( current.origin[0] );
	current.localOrigin[1] = msg.ReadDeltaFloat( current.origin[1] );
	current.localOrigin[2] = msg.ReadDeltaFloat( current.origin[2] );
	current.localAngles[0] = msg.ReadDeltaFloat( current.angles[0] );
	current.localAngles[1] = msg.ReadDeltaFloat( current.angles[1] );
	current.localAngles[2] = msg.ReadDeltaFloat( current.angles[2] );

	linearType = (extrapolation_t) msg.ReadBits( 8 );
	startTime = msg.ReadDeltaFloat( 0.0f );
	duration = msg.ReadDeltaFloat( 0.0f );
	linearStartValue[0] = msg.ReadDeltaFloat( 0.0f );
	linearStartValue[1] = msg.ReadDeltaFloat( 0.0f );
	linearStartValue[2] = msg.ReadDeltaFloat( 0.0f );
	linearSpeed[0] = msg.ReadDeltaFloat( 0.0f );
	linearSpeed[1] = msg.ReadDeltaFloat( 0.0f );
	linearSpeed[2] = msg.ReadDeltaFloat( 0.0f );
	linearBaseSpeed[0] = msg.ReadDeltaFloat( 0.0f );
	linearBaseSpeed[1] = msg.ReadDeltaFloat( 0.0f );
	linearBaseSpeed[2] = msg.ReadDeltaFloat( 0.0f );
	current.linearExtrapolation.Init( startTime, duration, linearStartValue, linearBaseSpeed, linearSpeed, linearType );

	angularType = (extrapolation_t) msg.ReadBits( 8 );
	startTime = msg.ReadDeltaFloat( 0.0f );
	duration = msg.ReadDeltaFloat( 0.0f );
	angularStartValue[0] = msg.ReadDeltaFloat( 0.0f );
	angularStartValue[1] = msg.ReadDeltaFloat( 0.0f );
	angularStartValue[2] = msg.ReadDeltaFloat( 0.0f );
	angularSpeed[0] = msg.ReadDeltaFloat( 0.0f );
	angularSpeed[1] = msg.ReadDeltaFloat( 0.0f );
	angularSpeed[2] = msg.ReadDeltaFloat( 0.0f );
	angularBaseSpeed[0] = msg.ReadDeltaFloat( 0.0f );
	angularBaseSpeed[1] = msg.ReadDeltaFloat( 0.0f );
	angularBaseSpeed[2] = msg.ReadDeltaFloat( 0.0f );
	current.angularExtrapolation.Init( startTime, duration, angularStartValue, angularBaseSpeed, angularSpeed, angularType );

	startTime = msg.ReadDeltaFloat( 0.0f );
	accelTime = msg.ReadDeltaFloat( 0.0f );
	decelTime = msg.ReadDeltaFloat( 0.0f );
	duration = msg.ReadDeltaFloat( 0.0f );
	startPos[0] = msg.ReadDeltaFloat( 0.0f );
	startPos[1] = msg.ReadDeltaFloat( 0.0f );
	startPos[2] = msg.ReadDeltaFloat( 0.0f );
	endPos[0] = msg.ReadDeltaFloat( 0.0f );
	endPos[1] = msg.ReadDeltaFloat( 0.0f );
	endPos[2] = msg.ReadDeltaFloat( 0.0f );
	current.linearInterpolation.Init( startTime, accelTime, decelTime, duration, startPos, endPos );

	startTime = msg.ReadDeltaFloat( 0.0f );
	accelTime = msg.ReadDeltaFloat( 0.0f );
	decelTime = msg.ReadDeltaFloat( 0.0f );
	duration = msg.ReadDeltaFloat( 0.0f );
	startAng[0] = msg.ReadDeltaFloat( 0.0f );
	startAng[1] = msg.ReadDeltaFloat( 0.0f );
	startAng[2] = msg.ReadDeltaFloat( 0.0f );
	endAng[0] = msg.ReadDeltaFloat( 0.0f );
	endAng[1] = msg.ReadDeltaFloat( 0.0f );
	endAng[2] = msg.ReadDeltaFloat( 0.0f );
	current.angularInterpolation.Init( startTime, accelTime, decelTime, duration, startAng, endAng );

	current.axis = current.angles.ToMat3();

	if ( clipModel ) {


		clipModel->Link( self, 0, current.origin, current.axis );

	}
}
