#include "../idlib/Lib.h"
#pragma hdrstop

#include "Game_local.h"
#include "spawner.h"

const anEventDef EV_OnAcceleration( "<onAcceleration>" );
const anEventDef EV_OnDeceleration( "<onDeceleration>" );
const anEventDef EV_OnCruising( "<onCruising>" );

const anEventDef EV_OnStartMoving( "<onStartMoving>" );
const anEventDef EV_OnStopMoving( "<onStopMoving>" );

//=======================================================
//
//	anPhysics_Spline
//
//=======================================================
CLASS_DECLARATION( idPhysics_Base, anPhysics_Spline )
	EVENT( EV_PostRestore,			anPhysics_Spline::Event_PostRestore )
END_CLASS

void splinePState_t::ApplyAccelerationDelta( float timeStepSec ) {
	speed = SignZero(idealSpeed) * Min( anMath::Fabs(idealSpeed), anMath::Fabs( speed) + acceleration * timeStepSec );
}

void splinePState_t::ApplyDecelerationDelta( float timeStepSec ) {
	speed = SignZero( speed) * Max( anMath::Fabs(idealSpeed), anMath::Fabs( speed) - deceleration * timeStepSec );
}

void splinePState_t::UpdateDist( float timeStepSec ) {
	dist += speed * timeStepSec;
}

bool splinePState_t::ShouldAccelerate() const {
	if ( Sign(idealSpeed) == Sign( speed) ) {
		return anMath::Fabs( speed) < anMath::Fabs(idealSpeed);
	} else if ( !Sign( speed) ) {
		return true;
	}
	
	return false;
}

bool splinePState_t::ShouldDecelerate() const {
	if ( Sign( speed) == Sign(idealSpeed) ) {
		return anMath::Fabs( speed) > anMath::Fabs(idealSpeed);
	} else if ( !Sign(idealSpeed) ) {
		return true;
	}

	return false;
}

void splinePState_t::Clear() {
	origin.Zero();
	localOrigin.Zero();
	axis.Identity();
	localAxis.Identity();
	speed = 0.0f;
	idealSpeed = 0.0f;
	dist = 0.0f;
	acceleration = 0.0f;
	deceleration = 0.0f;
}

void splinePState_t::WriteToSnapshot( anBitMsgDelta &msg ) const {
	msg.WriteDeltaVec3( vec3_zero, origin );
	msg.WriteDeltaVec3( vec3_zero, localOrigin );
	msg.WriteDeltaMat3( mat3_identity, axis );
	msg.WriteDeltaMat3( mat3_identity, localAxis );
	msg.WriteDeltaFloat( 0.0f, speed );
	msg.WriteDeltaFloat( 0.0f, idealSpeed );
	msg.WriteDeltaFloat( 0.0f, dist );
	msg.WriteDeltaFloat( 0.0f, acceleration );
	msg.WriteDeltaFloat( 0.0f, deceleration );
}

void splinePState_t::ReadFromSnapshot( const anBitMsgDelta &msg ) {
	origin = msg.ReadDeltaVec3( vec3_zero );
	localOrigin = msg.ReadDeltaVec3( vec3_zero );
	axis = msg.ReadDeltaMat3( mat3_identity );
	localAxis = msg.ReadDeltaMat3( mat3_identity );
	speed = msg.ReadDeltaFloat( 0.0f );
	idealSpeed = msg.ReadDeltaFloat( 0.0f );
	dist = msg.ReadDeltaFloat( 0.0f );
	acceleration = msg.ReadDeltaFloat( 0.0f );
	deceleration = msg.ReadDeltaFloat( 0.0f );
}

void splinePState_t::Save( anSaveGame *savefile ) const {
	savefile->WriteVec3( origin );
	savefile->WriteVec3( localOrigin );
	savefile->WriteMat3( axis );
	savefile->WriteMat3( localAxis );
	savefile->WriteFloat( speed );
	savefile->WriteFloat( idealSpeed );
	savefile->WriteFloat( dist );
	savefile->WriteFloat( acceleration );
	savefile->WriteFloat( deceleration );
}

void splinePState_t::Restore( anRestoreGame *savefile ) {
	savefile->ReadVec3( origin );
	savefile->ReadVec3( localOrigin );
	savefile->ReadMat3( axis );
	savefile->ReadMat3( localAxis );
	savefile->ReadFloat( speed );
	savefile->ReadFloat( idealSpeed );
	savefile->ReadFloat( dist );
	savefile->ReadFloat( acceleration );
	savefile->ReadFloat( deceleration );
}

splinePState_t&	splinePState_t::Assign( const splinePState_t* state ) {
	SIMDProcessor->Memcpy( this, state, sizeof( splinePState_t) );
	return *this;
}

splinePState_t&	splinePState_t::operator=( const splinePState_t& state ) {
	return Assign( &state );
}

splinePState_t&	splinePState_t::operator=( const splinePState_t* state ) {
	return Assign( state );
}

/*
================
anPhysics_Spline::anPhysics_Spline
================
*/
anPhysics_Spline::anPhysics_Spline( void ) {
	accelDecelStateThread.SetName( "AccelDecel" );
	accelDecelStateThread.SetOwner( this );
	accelDecelStateThread.SetState( "Cruising" );

	clipModel = nullptr;

	spline = nullptr;
	SetSplineEntity( nullptr );

	memset( &pushResults, 0, sizeof(trace_t) );
	pushResults.fraction = 1.0f;

	current.Clear();
	SaveState();
}

/*
================
anPhysics_Spline::~anPhysics_Spline
================
*/
anPhysics_Spline::~anPhysics_Spline( void ) {
	SAFE_DELETE_PTR( clipModel );

	SAFE_DELETE_PTR( spline );
}

/*
================
anPhysics_Spline::Save
================
*/
void anPhysics_Spline::Save( anSaveGame *savefile ) const {

	current.Save( savefile );
	saved.Save( savefile );

	savefile->WriteFloat( splineLength );
	// This spline was retored as nullptr, so there's no reason to save it.
	//savefile->WriteInt( spline != nullptr ? spline->GetTime( 0 ) : -1 );	// cnicholson: Added unsaved var
	splineEntity.Save( savefile );

	savefile->WriteTrace( pushResults );

	savefile->WriteClipModel( clipModel );

	accelDecelStateThread.Save( savefile );
}

/*
================
anPhysics_Spline::Restore
================
*/
void anPhysics_Spline::Event_PostRestore( void ) {

	if ( splineEntity.IsValid() ) {
		spline = splineEntity->GetSpline();
	}
}

void anPhysics_Spline::Restore( anRestoreGame *savefile ) {

	current.Restore( savefile );
	saved.Restore( savefile );

	savefile->ReadFloat( splineLength );
	SAFE_DELETE_PTR( spline );
	splineEntity.Restore( savefile );

	savefile->ReadTrace( pushResults );
	
	savefile->ReadClipModel( clipModel );

	accelDecelStateThread.Restore( savefile, this );
}

/*
================
anPhysics_Spline::SetClipModel
================
*/
void anPhysics_Spline::SetClipModel( anClipModel *model, const float density, int id, bool freeOld ) {

	assert( self );
	assert( model );					// we need a clip model

	if ( clipModel && clipModel != model && freeOld ) {
		delete clipModel;
	}
	
	clipModel = model;

	LinkClip();
}

/*
================
anPhysics_Spline::GetClipModel
================
*/
anClipModel *anPhysics_Spline::GetClipModel( int id ) const {
	return clipModel;
}

/*
================
anPhysics_Spline::SetContents
================
*/
void anPhysics_Spline::SetContents( int contents, int id ) {
	clipModel->SetContents( contents );
}

/*
================
anPhysics_Spline::GetContents
================
*/
int anPhysics_Spline::GetContents( int id ) const {
	return clipModel->GetContents();
}

/*
================
anPhysics_Spline::GetBounds
================
*/
const anBounds &anPhysics_Spline::GetBounds( int id ) const {
	return clipModel->GetBounds();
}

/*
================
anPhysics_Spline::GetAbsBounds
================
*/
const anBounds &anPhysics_Spline::GetAbsBounds( int id ) const {
	return clipModel->GetAbsBounds();
}

/*
================
anPhysics_Spline::SetSpline
================
*/
void anPhysics_Spline::SetSpline( anCurve_Spline<anVec3>* spline ) {
	SAFE_DELETE_PTR( this->spline );

	//Keep any left over dist from last spline to minimize hitches
	if ( GetSpeed() >= 0.0f ) {
		current.dist = Max( 0.0f, current.dist - splineLength );
	}

	if ( !spline ) {
		splineLength = 0.0f;
		return;
	}

	this->spline = spline;
 
	splineLength = spline->GetLengthForTime( spline->GetTime( spline->GetNumValues() - 1) );
	if ( GetSpeed() < 0.0f ) {
		current.dist = splineLength - current.dist;
	}

	Activate();
}

/*
================
anPhysics_Spline::SetSplineEntity
================
*/
void anPhysics_Spline::SetSplineEntity( anSplinePath* spline ) {
	splineEntity = spline;
	SetSpline( ( spline) ? spline->GetSpline() : nullptr );
}

/*
================
anPhysics_Spline::ComputeDecelFromSpline
================
*/
float anPhysics_Spline::ComputeDecelFromSpline() const {
	// FIXME: merge this in better.  It seems very special case
	float numerator = GetSpeed() * GetSpeed();
	float denomonator = 2.0f * ((GetSpeed() >= 0.0f) ? ( splineLength - current.dist) : current.dist);

	assert( denomonator > VECTOR_EPSILON );

	return numerator / denomonator;
}

/*
================
anPhysics_Spline::SetLinearAcceleration
================
*/
void anPhysics_Spline::SetLinearAcceleration( const float accel ) {
	current.acceleration = accel;
}

/*
================
anPhysics_Spline::SetLinearDeceleration
================
*/
void anPhysics_Spline::SetLinearDeceleration( const float decel ) {
	current.deceleration = decel;
}

/*
================
anPhysics_Spline::SetSpeed
================
*/
void anPhysics_Spline::SetSpeed( float speed ) {
	if ( IsAtRest() || StoppedMoving() ) {
		current.dist = ( speed < 0.0f) ? splineLength - current.dist : current.dist;
	}

	current.idealSpeed = speed;
	Activate();
}

/*
================
anPhysics_Spline::GetSpeed
================
*/
float anPhysics_Spline::GetSpeed() const {
	return current.speed;
}

/*
================
anPhysics_Spline::Evaluate
================
*/
bool anPhysics_Spline::Evaluate( int timeStepMSec, int endTimeMSec ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	splinePState_t previous = current;

	if ( HasValidSpline() ) {
		if ( StoppedMoving() ) {
			Rest();
			return false;
		}

		accelDecelStateThread.Execute();

		// FIXME: clean this up
		if ( IsAtBeginningOfSpline() || IsAtEndOfSpline() ) {
			current = previous;
			Rest();
			self->ProcessEvent( &EV_DoneMoving );
			
			if ( gameLocal.program.GetReturnedBool() ) {
				current.speed = 0.0f;
				return false;
			} else {
				return true;
			}
		}
	
		float currentTime = splineEntity->GetSampledTime ( current.dist );
		if (  currentTime ==  -1.0f ) {
			currentTime = spline->GetTimeForLength( Min(current.dist, splineLength), 0.01f );
		}

		current.axis = spline->GetCurrentFirstDerivative(currentTime).ToAngles().Normalize360().ToMat3();
		current.origin = spline->GetCurrentValue( currentTime );
		current.localOrigin = current.origin;
		current.localAxis = current.axis;
	} else if ( self->IsBound() ) {	
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.axis = current.localAxis * masterAxis;
	    current.origin = masterOrigin + current.localOrigin * masterAxis;
	} else {
		Rest();
		return false;
	}

	gameLocal.push.ClipPush( pushResults, self, 0, previous.origin, previous.axis, current.origin, current.axis );
	if ( pushResults.fraction < 1.0f ) {
		current = previous;
		LinkClip();
		current.speed = 0.0f;
		return false;
	}

	LinkClip();

	if ( StoppedMoving() && !self->IsBound() ) {
		Rest();
		self->ProcessEvent( &EV_DoneMoving );
		return !gameLocal.program.GetReturnedBool();
	}

	return true;
}

/*
================
anPhysics_Spline::Activate
================
*/
void anPhysics_Spline::Activate( void ) {
	assert( self );
	self->BecomeActive( TH_PHYSICS );
}

/*
================
anPhysics_Spline::Rest
================
*/
void anPhysics_Spline::Rest( void ) {
	assert( self );
	self->BecomeInactive( TH_PHYSICS );
}

/*
================
anPhysics_Spline::IsAtRest
================
*/
bool anPhysics_Spline::IsAtRest( void ) const {
	assert( self );
	return !self->IsActive( TH_PHYSICS );
}

/*
================
anPhysics_Spline::IsAtEndOfSpline
================
*/
bool anPhysics_Spline::IsAtEndOfSpline( void ) const {
	return current.dist >= splineLength;
}

/*
================
anPhysics_Spline::IsAtBeginningOfSpline
================
*/
bool anPhysics_Spline::IsAtBeginningOfSpline( void ) const {
	return current.dist <= 0.0f;
}

/*
================
anPhysics_Spline::IsPushable
================
*/
bool anPhysics_Spline::IsPushable( void ) const {
	return !HasValidSpline() && idPhysics_Base::IsPushable();
}

/*
================
anPhysics_Spline::StartingToMove
================
*/
bool anPhysics_Spline::StartingToMove( void ) const {
	float firstDeltaSpeed = current.acceleration * MS2SEC(gameLocal.GetMSec());
	return anMath::Fabs(current.idealSpeed) > VECTOR_EPSILON && anMath::Fabs(current.speed) <= firstDeltaSpeed;
}

/*
================
anPhysics_Spline::StoppedMoving
================
*/
bool anPhysics_Spline::StoppedMoving( void ) const {
	return anMath::Fabs(current.idealSpeed) < VECTOR_EPSILON && anMath::Fabs(current.speed) < VECTOR_EPSILON;
}

/*
================
anPhysics_Spline::HasValidSpline
================
*/
bool anPhysics_Spline::HasValidSpline() const {
	return spline && splineLength > VECTOR_EPSILON;
}

/*
================
anPhysics_Spline::SaveState
================
*/
void anPhysics_Spline::SaveState( void ) {
	saved = current;
}

/*
================
anPhysics_Spline::RestoreState
================
*/
void anPhysics_Spline::RestoreState( void ) {
	current = saved;

	LinkClip();
}

/*
================
idPhysics::SetOrigin
================
*/
void anPhysics_Spline::SetOrigin( const anVec3 &newOrigin, int id ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	current.localOrigin = newOrigin;
	if ( self->IsBound() ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + current.localOrigin * masterAxis;
	}
	else {
		current.origin = current.localOrigin;
	}

	LinkClip();
	Activate();
}

/*
================
idPhysics::SetAxis
================
*/
void anPhysics_Spline::SetAxis( const anMat3 &newAxis, int id ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	current.localAxis = newAxis;
	if ( self->IsBound() ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.axis = newAxis * masterAxis;
	}
	else {
		current.axis = newAxis;
	}

	LinkClip();
	Activate();
}

/*
================
anPhysics_Spline::Translate
================
*/
void anPhysics_Spline::Translate( const anVec3 &translation, int id ) {
	SetOrigin( GetLocalOrigin() + translation );
}

/*
================
anPhysics_Spline::Rotate
================
*/
void anPhysics_Spline::Rotate( const anRotation &rotation, int id ) {
	SetAxis( GetLocalAxis() * rotation.ToMat3() );
	SetOrigin( GetLocalOrigin() * rotation );
}

/*
================
anPhysics_Spline::GetOrigin
================
*/
const anVec3 &anPhysics_Spline::GetOrigin( int id ) const {
	return current.origin;
}

/*
================
anPhysics_Spline::GetAxis
================
*/
const anMat3 &anPhysics_Spline::GetAxis( int id ) const {
	return current.axis;
}

/*
================
anPhysics_Spline::GetOrigin
================
*/
anVec3 &anPhysics_Spline::GetOrigin( int id ) {
	return current.origin;
}

/*
================
anPhysics_Spline::GetAxis
================
*/
anMat3 &anPhysics_Spline::GetAxis( int id ) {
	return current.axis;
}

/*
================
anPhysics_Spline::GetLocalOrigin
================
*/
const anVec3 &anPhysics_Spline::GetLocalOrigin( int id ) const {
	return current.localOrigin;
}

/*
================
anPhysics_Spline::GetLocalAxis
================
*/
const anMat3 &anPhysics_Spline::GetLocalAxis( int id ) const {
	return current.localAxis;
}

/*
================
anPhysics_Spline::GetLocalOrigin
================
*/
anVec3 &anPhysics_Spline::GetLocalOrigin( int id ) {
	return current.localOrigin;
}

/*
================
anPhysics_Spline::GetLocalAxis
================
*/
anMat3 &anPhysics_Spline::GetLocalAxis( int id ) {
	return current.localAxis;
}

/*
================
anPhysics_Spline::SetMaster
================
*/
void anPhysics_Spline::SetMaster( anEntity *master, const bool orientated ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	if ( master ) {
		if ( self->IsBound() ) {
			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			current.localOrigin = ( GetOrigin() - masterOrigin ) * masterAxis.Transpose();
			current.localAxis = GetAxis() * masterAxis.Transpose();
		}
	}
}

/*
================
anPhysics_Spline::ClipTranslation
================
*/
void anPhysics_Spline::ClipTranslation( trace_t &results, const anVec3 &translation, const anClipModel *model ) const {
	if ( model ) {
		gameLocal.TranslationModel( self, results, GetOrigin(), GetOrigin() + translation,
											clipModel, GetAxis(), clipMask,
											model->GetCollisionModel(), model->GetOrigin(), model->GetAxis() );
	}
	else {
		gameLocal.Translation( self, results, GetOrigin(), GetOrigin() + translation,
											clipModel, GetAxis(), clipMask, self );
	}
}

/*
================
anPhysics_Spline::ClipRotation
================
*/
void anPhysics_Spline::ClipRotation( trace_t &results, const anRotation &rotation, const anClipModel *model ) const {
	if ( model ) {
		gameLocal.RotationModel( self, results, GetOrigin(), rotation,
											clipModel, GetAxis(), clipMask,
											model->GetCollisionModel(), model->GetOrigin(), model->GetAxis() );
	}
	else {
		gameLocal.Rotation( self, results, GetOrigin(), rotation,
											clipModel, GetAxis(), clipMask, self );
	}
}

/*
================
anPhysics_Spline::ClipContents
================
*/
int anPhysics_Spline::ClipContents( const anClipModel *model ) const {
	if ( model ) {
// RAVEN BEGIN
// ddynerman: multiple clip worlds
		return gameLocal.ContentsModel( self, GetOrigin(), clipModel, GetAxis(), -1,
									model->GetCollisionModel(), model->GetOrigin(), model->GetAxis() );
	}
	else {
		return gameLocal.Contents( self, GetOrigin(), clipModel, GetAxis(), -1, nullptr );
// RAVEN END
	}
}

/*
================
anPhysics_Spline::DisableClip
================
*/
void anPhysics_Spline::DisableClip( void ) {
	if ( clipModel ) {
		clipModel->Disable();
	}
}

/*
================
anPhysics_Spline::EnableClip
================
*/
void anPhysics_Spline::EnableClip( void ) {
	if ( clipModel ) {
		clipModel->Enable();
	}
}

/*
================
anPhysics_Spline::UnlinkClip
================
*/
void anPhysics_Spline::UnlinkClip( void ) {
	if ( clipModel ) {
		clipModel->Unlink();
	}
}

/*
================
anPhysics_Spline::LinkClip
================
*/
void anPhysics_Spline::LinkClip( void ) {
	if ( clipModel ) {
// RAVEN BEGIN
// ddynerman: multiple clip worlds
		clipModel->Link( self, clipModel->GetId(), GetOrigin(), GetAxis() );
// RAVEN END
	}
}

/*
================
anPhysics_Spline::GetBlockingInfo
================
*/
const trace_t* anPhysics_Spline::GetBlockingInfo( void ) const {
	return (pushResults.fraction < 1.0f) ? &pushResults : nullptr;
}

/*
================
anPhysics_Spline::GetBlockingEntity
================
*/
anEntity *anPhysics_Spline::GetBlockingEntity( void ) const {
	return (pushResults.fraction < 1.0f) ? gameLocal.entities[ pushResults.c.entityNum ] : nullptr;
}

/*
================
anPhysics_Spline::WriteToSnapshot
================
*/
void anPhysics_Spline::WriteToSnapshot( anBitMsgDelta &msg ) const {
	current.WriteToSnapshot( msg );
}

/*
================
anPhysics_Spline::ReadFromSnapshot
================
*/
void anPhysics_Spline::ReadFromSnapshot( const anBitMsgDelta &msg ) {
	current.ReadFromSnapshot( msg );

	LinkClip();
}

CLASS_STATES_DECLARATION( anPhysics_Spline )
	STATE( "Accelerating",		anPhysics_Spline::State_Accelerating )
	STATE( "Decelerating",		anPhysics_Spline::State_Decelerating )
	STATE( "Cruising",			anPhysics_Spline::State_Cruising )
END_CLASS_STATES

/*
================
anPhysics_Spline::State_Accelerating
================
*/
stateResult_t anPhysics_Spline::State_Accelerating( const stateParms_t& parms ) {
	stateResult_t returnResult = SRESULT_WAIT;

	if ( !current.ShouldAccelerate() ) {
		accelDecelStateThread.SetState( current.ShouldDecelerate() ? "Decelerating" : "Cruising" );
		return SRESULT_DONE;
	}

	if ( !parms.stage ) {
		if ( StartingToMove() ) {
			self->ProcessEvent( &EV_OnStartMoving );
		}
		self->ProcessEvent( &EV_OnAcceleration );

		returnResult = SRESULT_STAGE( parms.stage + 1 );
	}

	float timeStepSec = MS2SEC( gameLocal.GetMSec() );
	current.ApplyAccelerationDelta( timeStepSec );
	current.UpdateDist( timeStepSec );

	return returnResult;
}

/*
================
anPhysics_Spline::State_Decelerating
================
*/
stateResult_t anPhysics_Spline::State_Decelerating( const stateParms_t& parms ) {
	if ( !current.ShouldDecelerate() ) {
		accelDecelStateThread.SetState( current.ShouldAccelerate() ? "Accelerating" : "Cruising" );
		return SRESULT_DONE;
	}

	float timeStepSec = MS2SEC( gameLocal.GetMSec() );
	current.ApplyDecelerationDelta( timeStepSec );
	current.UpdateDist( timeStepSec );

	if ( !parms.stage ) {
		self->ProcessEvent( &EV_OnDeceleration );
		return SRESULT_STAGE( parms.stage + 1 );
	}

	if ( StoppedMoving() ) {
		self->ProcessEvent( &EV_OnStopMoving );
	}

	return SRESULT_WAIT;
}

/*
================
anPhysics_Spline::State_Cruising
================
*/
stateResult_t anPhysics_Spline::State_Cruising( const stateParms_t& parms ) {
	if ( current.ShouldAccelerate() ) {
		accelDecelStateThread.SetState( "Accelerating" );
		return SRESULT_DONE;
	} else if ( current.ShouldDecelerate() ) {
		accelDecelStateThread.SetState( "Decelerating" );
		return SRESULT_DONE;
	}

	current.UpdateDist( MS2SEC(gameLocal.GetMSec()) );

	if ( !parms.stage ) {
		self->ProcessEvent( &EV_OnCruising );
		return SRESULT_STAGE( parms.stage + 1 );
	}

	return SRESULT_WAIT;
}



/*
===============================================================

	anPhysics_SimpleSpline

===============================================================
*/

CLASS_DECLARATION( idPhysics_Base, anPhysics_SimpleSpline )
END_CLASS

/*
================
anPhysics_SimpleSpline::anPhysics_SimpleSpline
================
*/
anPhysics_SimpleSpline::anPhysics_SimpleSpline( void ) {
	current.worldAxes.Identity();
	current.worldOrigin.Zero();
	current.acceleration.Zero();
	current.velocity.Zero();
	current.time = 0;

	saved			= current;

	clipModel		= NULL;
	isOrientated	= false;
	startTime		= -1;
	totalTime		= 0;
}

/*
================
anPhysics_SimpleSpline::~anPhysics_SimpleSpline
================
*/
anPhysics_SimpleSpline::~anPhysics_SimpleSpline( void ) {
}

/*
================
anPhysics_SimpleSpline::Evaluate
================
*/
anVec3 anPhysics_SimpleSpline::EvaluatePosition( void ) const {
	if ( !spline.Num() ) {
		return vec3_origin;
	}

	float frac;
	const splineType_t& section = GetSplineInfo( gameLocal.time, frac );

	return section.GetCurrentValue( frac );
}

/*
================
anPhysics_SimpleSpline::Evaluate
================
*/
bool anPhysics_SimpleSpline::Evaluate( int timeStepMSec, int endTimeMSec ) {
	if ( !spline.Num() ) {
		return false;
	}

/*	int i;
	for ( i = 0; i < spline.Num(); i++ ) {
		const idCurve_CubicBezier< anVec3 >& section = spline[ i ];
		float index = 0.f;
		anVec3 lastPos = section.GetCurrentValue( index );
		anVec3 pos;
		for ( index += 0.01f; index <= 1.f; index += 0.01f ) {
			pos = section.GetCurrentValue( index );
			gameRenderWorld->DebugLine( colorGreen, lastPos, pos );
			lastPos = pos;
		}
	}*/


	anVec3 oldOrigin		= current.worldOrigin;
	anMat3 oldWorldAxes		= current.worldAxes;

	float frac;
	const splineType_t& section = GetSplineInfo( endTimeMSec, frac );

	current.acceleration	= section.GetCurrentSecondDerivative( frac );
	current.velocity		= section.GetCurrentFirstDerivative( frac );
	current.worldOrigin		= section.GetCurrentValue( frac );

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.worldOrigin, current.worldAxes );
	}

	current.time = endTimeMSec;

	return ( current.worldOrigin != oldOrigin ) || ( current.worldAxes != oldWorldAxes );
}

/*
================
anPhysics_SimpleSpline::SetSpline
================
*/
void anPhysics_SimpleSpline::SetSpline( int _startTime, const anList< splineType_t >& _spline, const anList< int >& _splineTimes ) {
	startTime	= _startTime;
	splineTimes	= _splineTimes;
	spline		= _spline;
	totalTime	= 0;

	int i;
	for ( i = 0; i < _splineTimes.Num(); i++ ) {
		totalTime += _splineTimes[ i ];
	}

	assert( splineTimes.Num() == spline.Num() );
}

/*
================
anPhysics_SimpleSpline::GetSplineInfo
================
*/
const anPhysics_SimpleSpline::splineType_t& anPhysics_SimpleSpline::GetSplineInfo( int endTimeMSec, float& fraction ) const {
	int timeSinceStart = endTimeMSec - startTime;
	int i;
	for ( i = 0; i < spline.Num(); i++ ) {
		if ( timeSinceStart > splineTimes[ i ] ) {
			timeSinceStart -= splineTimes[ i ];
			continue;
		}

		fraction = anMath::ClampFloat( 0.f, 1.f, timeSinceStart / ( float )splineTimes[ i ] );
		return spline[ i ];
	}

	fraction = 1.f;
	return spline[ spline.Num() - 1 ];
}

/*
================
anPhysics_SimpleSpline::SetAxis
================
*/
void anPhysics_SimpleSpline::SetAxis( const anMat3 &newAxis, int id ) {
	current.worldAxes = newAxis;

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.worldOrigin, current.worldAxes );
	}
}

/*
================
anPhysics_SimpleSpline::SetOrigin
================
*/
void anPhysics_SimpleSpline::SetOrigin( const anVec3 &newOrigin, int id ) {
	current.worldOrigin = newOrigin;

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.worldOrigin, current.worldAxes );
	}
}

/*
================
anPhysics_SimpleSpline::IsAtRest
================
*/
bool anPhysics_SimpleSpline::IsAtRest( void ) const {
	return current.time >= ( startTime + totalTime );
}

/*
================
anPhysics_SimpleSpline::GetAbsBounds
================
*/
const anBounds& anPhysics_SimpleSpline::GetAbsBounds( int id ) const {
	current.absBounds.Clear();
	current.absBounds.AddPoint( current.mins + current.worldOrigin );
	current.absBounds.AddPoint( current.maxs + current.worldOrigin );

	return current.absBounds;
}

/*
================
anPhysics_SimpleSpline::SetBounds
================
*/
void anPhysics_SimpleSpline::SetBounds( const anVec3 &mins, const anVec3 &maxs ) {
	current.mins = mins;
	current.maxs = maxs;
}



const anEventDef EV_SetSpline( "setSpline", "E" );

const anEventDef EV_SetAccel( "setAccel", "f" );
const anEventDef EV_SetDecel( "setDecel", "f" );

const anEventDef EV_SetSpeed( "setSpeed", "f" );
const anEventDef EV_GetSpeed( "getSpeed", "", 'f' );

const anEventDef EV_TramCar_SetIdealSpeed( "setIdealSpeed", "f" );
const anEventDef EV_TramCar_GetIdealSpeed( "getIdealSpeed", "", 'f' );

const anEventDef EV_TramCar_ApplySpeedScale( "applySpeedScale", "f" );

const anEventDef EV_GetCurrentTrackInfo( "getCurrentTrackInfo", "", 's' );
const anEventDef EV_GetTrackInfo( "getTrackInfo", "e", 's' );

const anEventDef EV_DoneMoving( "<doneMoving>", "", 'd' );
const anEventDef EV_StartSoundPeriodic( "<startSoundPeriodic>", "sddd" );

anLinkList<anPhysics_SplineMover> anPhysics_SplineMover::splineMovers;

//=======================================================
//
//	anPhysics_SplineMover
//
//=======================================================
CLASS_DECLARATION( anAnimatedEntity, anPhysics_SplineMover )
	EVENT( EV_PostSpawn,				anPhysics_SplineMover::Event_PostSpawn )
	EVENT( EV_Activate,					anPhysics_SplineMover::Event_Activate )
	EVENT( EV_SetSpline,				anPhysics_SplineMover::Event_SetSpline )
	EVENT( EV_SetAccel,					anPhysics_SplineMover::Event_SetAcceleration )
	EVENT( EV_SetDecel,					anPhysics_SplineMover::Event_SetDeceleration )
	EVENT( EV_SetSpeed,					anPhysics_SplineMover::Event_SetSpeed )
	EVENT( EV_GetSpeed,					anPhysics_SplineMover::Event_GetSpeed )
	EVENT( EV_Thread_SetCallback,		anPhysics_SplineMover::Event_SetCallBack )
	EVENT( EV_DoneMoving,				anPhysics_SplineMover::Event_DoneMoving )
	EVENT( EV_GetSplineEntity,			anPhysics_SplineMover::Event_GetSpline )
	EVENT( EV_GetCurrentTrackInfo,		anPhysics_SplineMover::Event_GetCurrentTrackInfo )
	EVENT( EV_GetTrackInfo,				anPhysics_SplineMover::Event_GetTrackInfo )
	EVENT( EV_TramCar_SetIdealSpeed,	anPhysics_SplineMover::Event_SetIdealSpeed )
	EVENT( EV_TramCar_GetIdealSpeed,	anPhysics_SplineMover::Event_GetIdealSpeed )
	EVENT( EV_TramCar_ApplySpeedScale,	anPhysics_SplineMover::Event_ApplySpeedScale )
	EVENT( EV_OnAcceleration,			anPhysics_SplineMover::Event_OnAcceleration )
	EVENT( EV_OnDeceleration,			anPhysics_SplineMover::Event_OnDeceleration )
	EVENT( EV_OnCruising,				anPhysics_SplineMover::Event_OnCruising )
	EVENT( EV_OnStartMoving,			anPhysics_SplineMover::Event_OnStartMoving )
	EVENT( EV_OnStopMoving,				anPhysics_SplineMover::Event_OnStopMoving )
	EVENT( EV_StartSoundPeriodic,		anPhysics_SplineMover::Event_StartSoundPeriodic )
	EVENT( EV_PartBlocked,				anPhysics_SplineMover::Event_PartBlocked )
END_CLASS

/*
================
anPhysics_SplineMover::Spawn
================
*/
void anPhysics_SplineMover::Spawn() {
	waitThreadId = -1;

	physicsObj.SetSelf( this );
	PushHeapMemory(this);
	physicsObj.SetClipModel( new anClipModel(GetPhysics()->GetClipModel()), 1.0f );
	PopSystemHeap();
	physicsObj.SetContents( spawnArgs.GetBool( "solid", "1" ) ? CONTENTS_SOLID : 0 );
	physicsObj.SetClipMask( spawnArgs.GetBool( "solidClip" ) ? CONTENTS_SOLID : 0 );
	physicsObj.SetLinearVelocity( GetPhysics()->GetLinearVelocity() );
	physicsObj.SetLinearAcceleration( spawnArgs.GetFloat( "accel", "50" ) );
	physicsObj.SetLinearDeceleration( spawnArgs.GetFloat( "decel", "50" ) );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );

	SetPhysics( &physicsObj );

	AddSelfToGlobalList();

	// This is needed so we get sorted correctly
	BecomeInactive( TH_PHYSICS );
	BecomeActive( TH_PHYSICS );

	PlayAnim( ANIMCHANNEL_ALL, "idle", 0 );

	PostEventMS( &EV_PostSpawn, 0 );
}

/*
================
anPhysics_SplineMover::~anPhysics_SplineMover
================
*/
anPhysics_SplineMover::~anPhysics_SplineMover() {
	RemoveSelfFromGlobalList();
	SetPhysics( nullptr );
}

/*
================
anPhysics_SplineMover::SetSpeed
================
*/
void anPhysics_SplineMover::SetSpeed( float newSpeed ) {
	physicsObj.SetSpeed( newSpeed );
}

/*
================
anPhysics_SplineMover::GetSpeed
================
*/
float anPhysics_SplineMover::GetSpeed() const {
	return physicsObj.GetSpeed();
}

/*
================
anPhysics_SplineMover::SetIdealSpeed
================
*/
void anPhysics_SplineMover::SetIdealSpeed( float newIdealSpeed ) {
	idealSpeed = newIdealSpeed;
	SetSpeed( newIdealSpeed );
}

/*
================
anPhysics_SplineMover::GetIdealSpeed
================
*/
float anPhysics_SplineMover::GetIdealSpeed() const {
	return idealSpeed;
}

/*
================
anPhysics_SplineMover::SetSpline
================
*/
void anPhysics_SplineMover::SetSpline( anSplinePath* spline ) {
	physicsObj.SetSplineEntity( spline );
	CheckSplineForOverrides( physicsObj.GetSpline(), &spline->spawnArgs );
}

/*
================
anPhysics_SplineMover::GetSpline
================
*/
const anSplinePath* anPhysics_SplineMover::GetSpline() const {
	return physicsObj.GetSplineEntity();
}

/*
================
anPhysics_SplineMover::GetSpline
================
*/
anSplinePath* anPhysics_SplineMover::GetSpline() {
	return physicsObj.GetSplineEntity();
}

/*
================
anPhysics_SplineMover::SetAcceleration
================
*/
void anPhysics_SplineMover::SetAcceleration( float accel ) {
	physicsObj.SetLinearAcceleration( accel );
}

/*
================
anPhysics_SplineMover::SetDeceleration
================
*/
void anPhysics_SplineMover::SetDeceleration( float decel ) {
	physicsObj.SetLinearDeceleration( decel );
}

/*
================
anPhysics_SplineMover::CheckSplineForOverrides
================
*/
void anPhysics_SplineMover::CheckSplineForOverrides( const anCurve_Spline<anVec3>* spline, const anDict* args ) {
	if ( !spline || !args ) {
		return;
	}

	int endSpline = args->GetInt( "end_spline" );
	if ( endSpline && Sign(endSpline) == Sign(GetSpeed()) ) {
		physicsObj.SetLinearDeceleration( physicsObj.ComputeDecelFromSpline() );
		SetIdealSpeed( 0.0f );
	}
}

/*
================
anPhysics_SplineMover::RestoreFromOverrides
================
*/
void anPhysics_SplineMover::RestoreFromOverrides( const anDict* args ) {
	if ( !args ) {
		return;
	}

	physicsObj.SetLinearDeceleration( args->GetFloat( "decel" ) );
}

/*
================
anPhysics_SplineMover::PlayAnim
================
*/
int anPhysics_SplineMover::PlayAnim( int channel, const char *animName, int blendFrames ) {
	int animIndex = GetAnimator()->GetAnim( animName );
	if ( !animIndex ) {
		return 0;
	}

	GetAnimator()->PlayAnim( channel, animIndex, gameLocal.GetTime(), FRAME2MS(blendFrames) );
	return GetAnimator()->CurrentAnim( channel )->Length();
}

/*
================
anPhysics_SplineMover::CycleAnim
================
*/
void anPhysics_SplineMover::CycleAnim( int channel, const char *animName, int blendFrames ) {
	int animIndex = GetAnimator()->GetAnim( animName );
	if ( !animIndex ) {
		return;
	}

	GetAnimator()->CycleAnim( channel, animIndex, gameLocal.GetTime(), FRAME2MS(blendFrames) );
}

/*
================
anPhysics_SplineMover::ClearChannel
================
*/
void anPhysics_SplineMover::ClearChannel( int channel, int clearFrame ) {
	GetAnimator()->Clear( channel, gameLocal.GetTime(), FRAME2MS( clearFrame ) );
}

/*
================
anPhysics_SplineMover::PreBind
================
*/
void anPhysics_SplineMover::PreBind() {
	anAnimatedEntity::PreBind();
	SetSpline( nullptr );
}

/*
================
anPhysics_SplineMover::Save
================
*/
void anPhysics_SplineMover::Save( anSaveGame *savefile ) const {
	savefile->WriteStaticObject( physicsObj );
	savefile->WriteFloat( idealSpeed );
	savefile->WriteInt( waitThreadId );
}

/*
================
anPhysics_SplineMover::Restore
================
*/
void anPhysics_SplineMover::Restore( anRestoreGame *savefile ) {
	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );
	savefile->ReadFloat( idealSpeed );
	savefile->ReadInt( waitThreadId );
	AddSelfToGlobalList();
}

/*
================
anPhysics_SplineMover::WriteToSnapshot
================
*/
void anPhysics_SplineMover::WriteToSnapshot( anBitMsgDelta &msg ) const {
	physicsObj.WriteToSnapshot( msg );
}

/*
================
anPhysics_SplineMover::ReadFromSnapshot
================
*/
void anPhysics_SplineMover::ReadFromSnapshot( const anBitMsgDelta &msg ) {
	physicsObj.ReadFromSnapshot( msg );
}

/*
================
anPhysics_SplineMover::AddSelfToGlobalList
================
*/
void anPhysics_SplineMover::AddSelfToGlobalList() {
	splineMoverNode.SetOwner( this );

	if ( !InGlobalList() ) {
		splineMoverNode.AddToEnd( splineMovers );
	}
}

/*
================
anPhysics_SplineMover::RemoveSelfFromGlobalList
================
*/
void anPhysics_SplineMover::RemoveSelfFromGlobalList() {
	splineMoverNode.Remove();
}

/*
================
anPhysics_SplineMover::InGlobalList
================
*/
bool anPhysics_SplineMover::InGlobalList() const {
	return splineMoverNode.InList();
}

/*
================
anPhysics_SplineMover::WhosVisible
================
*/
bool anPhysics_SplineMover::WhosVisible( const anFrustum& frustum, anList<anPhysics_SplineMover*>& list ) const {
	list.Clear();

	if ( !frustum.IsValid() ) {
		return false;
	}

	for ( anPhysics_SplineMover* node = splineMovers.Next(); node; node = node->splineMoverNode.Next() ) {
		if ( node == this ) {
			continue;
		}

		if ( frustum.IntersectsBounds(node->GetPhysics()->GetAbsBounds()) ) {
			list.AddUnique( node );
		}
	}

	return list.Num() > 0;
}

/*
================
anPhysics_SplineMover::GetTrackInfo
================
*/
anStr anPhysics_SplineMover::GetTrackInfo( const anSplinePath* track ) const {
	if ( !track ) {
		return anStr( "" );
	}

	anStr info( track->GetName() );
	return info.Mid( info.Last('_') - 1, 1 );
}

/*
================
anPhysics_SplineMover::ConvertToMover
================
*/
anPhysics_SplineMover *anPhysics_SplineMover::ConvertToMover( anEntity *mover ) const {
	return mover && mover->IsType(anPhysics_SplineMover::Type) ? static_cast<anPhysics_SplineMover*>(mover) : nullptr;
}

/*
================
anPhysics_SplineMover::ConvertToSplinePath
================
*/
anSplinePath* anPhysics_SplineMover::ConvertToSplinePath( anEntity *spline ) const {
	return ( spline && spline->IsType(anSplinePath::GetClassType())) ? static_cast<anSplinePath*>( spline) : nullptr;
}

/*
================
anPhysics_SplineMover::PreDoneMoving
================
*/
void anPhysics_SplineMover::PreDoneMoving() {
	if ( waitThreadId >= 0 ) {
		anThread::ObjectMoveDone( waitThreadId, this );
		waitThreadId = -1;
	}

	RestoreFromOverrides( &spawnArgs );
}

/*
================
anPhysics_SplineMover::PostDoneMoving
================
*/
void anPhysics_SplineMover::PostDoneMoving() {
	CallScriptEvents( physicsObj.GetSplineEntity(), "call_doneMoving", this );
}

/*
==============
anPhysics_SplineMover::CallScriptEvents
==============
*/
// FIXME: very similier code is in the spawner...if possible try and make one function for both to call
void anPhysics_SplineMover::CallScriptEvents( const anSplinePath* spline, const char *prefixKey, anEntity *parm ) {
	if ( !spline || !prefixKey || !prefixKey[0] ) {
		return;
	}

	rvScriptFuncUtility func;
	for ( const anKeyValue *kv = spline->spawnArgs.MatchPrefix(prefixKey); kv; kv = spline->spawnArgs.MatchPrefix(prefixKey, kv) ) {
		if ( !kv->GetValue().Length() ) {
			continue;
		}

		if ( func.Init(kv->GetValue()) <= SFU_ERROR ) {
			continue;
		}

		func.InsertEntity( spline, 0 );
		func.InsertEntity( parm, 1 );
		func.CallFunc( &spawnArgs );
	}
}

/*
================
anPhysics_SplineMover::Event_PostSpawn
================
*/
void anPhysics_SplineMover::Event_PostSpawn() {
	anEntityPtr<anEntity> target;
	for ( int ix = targets.Num() - 1; ix >= 0; --ix ) {
		target = targets[ix];

		if ( target.IsValid() && target->IsType(anSplinePath::GetClassType()) ) {
			SetSpline( static_cast<anSplinePath*>(target.GetEntity()) );
			break;
		}
	}

	SetIdealSpeed( spawnArgs.GetBool( "waitForTrigger" ) ? 0.0f : spawnArgs.GetFloat( "speed", "50" ) );
}

/*
===============
anPhysics_SplineMover::Event_PartBlocked
===============
*/
void anPhysics_SplineMover::Event_PartBlocked( anEntity *blockingEntity ) {
	assert( blockingEntity );

	float damage = spawnArgs.GetFloat( "damage" );
	if ( damage > 0.0f ) {
		blockingEntity->Damage( this, this, vec3_origin, "damage_moverCrush", damage, INVALID_JOINT );
	}
	if ( g_debugMover.GetBool() ) {
		gameLocal.Printf( "%d: '%s' blocked by '%s'\n", gameLocal.GetTime(), GetName(), blockingEntity->GetName() );
	}
}

/*
================
anPhysics_SplineMover::Event_SetSpline
================
*/
void anPhysics_SplineMover::Event_SetSpline( anEntity *spline ) {
	SetSpline( ConvertToSplinePath( spline) );
}

/*
================
anPhysics_SplineMover::Event_GetSpline
================
*/
void anPhysics_SplineMover::Event_GetSpline() {
	anThread::ReturnEntity( GetSpline() );
}

/*
================
anPhysics_SplineMover::Event_SetAcceleration
================
*/
void anPhysics_SplineMover::Event_SetAcceleration( float accel ) {
	SetAcceleration( accel );
}

/*
================
anPhysics_SplineMover::Event_SetDeceleration
================
*/
void anPhysics_SplineMover::Event_SetDeceleration( float decel ) {
	SetDeceleration( decel );
}

/*
================
anPhysics_SplineMover::Event_SetSpeed
================
*/
void anPhysics_SplineMover::Event_SetSpeed( float speed ) {
	SetIdealSpeed( speed );
	SetSpeed( speed );
}

/*
================
anPhysics_SplineMover::Event_GetSpeed
================
*/
void anPhysics_SplineMover::Event_GetSpeed() {
	anThread::ReturnFloat( GetSpeed() );
}

/*
================
anPhysics_SplineMover::Event_SetIdealSpeed
================
*/
void anPhysics_SplineMover::Event_SetIdealSpeed( float speed ) {
	SetIdealSpeed( speed );
}

/*
================
anPhysics_SplineMover::Event_GetIdealSpeed
================
*/
void anPhysics_SplineMover::Event_GetIdealSpeed() {
	anThread::ReturnFloat( GetIdealSpeed() );
}

/*
================
anPhysics_SplineMover::Event_ApplySpeedScale
================
*/
void anPhysics_SplineMover::Event_ApplySpeedScale( float scale ) {
	SetIdealSpeed( spawnArgs.GetFloat( "speed", "50" ) * scale );
}

/*
================
anPhysics_SplineMover::Event_SetCallBack
================
*/
void anPhysics_SplineMover::Event_SetCallBack() {
	if ( waitThreadId >= 0 ) {
		anThread::ReturnInt( false );
	}

	waitThreadId = anThread::CurrentThreadNum();
	anThread::ReturnInt( true );
}

/*
================
anPhysics_SplineMover::Event_DoneMoving
================
*/
void anPhysics_SplineMover::Event_DoneMoving() {
	PreDoneMoving();
	PostDoneMoving();
	anThread::ReturnInt( !physicsObj.HasValidSpline() );
}

/*
================
anPhysics_SplineMover::Event_GetCurrentTrackInfo
================
*/
void anPhysics_SplineMover::Event_GetCurrentTrackInfo() {
	Event_GetTrackInfo( physicsObj.GetSplineEntity() );
}

/*
================
anPhysics_SplineMover::Event_GetTrackInfo
================
*/
void anPhysics_SplineMover::Event_GetTrackInfo( anEntity *track ) {
	anThread::ReturnString( GetTrackInfo(ConvertToSplinePath(track)) );
}

/*
================
anPhysics_SplineMover::Event_Activate
================
*/
void anPhysics_SplineMover::Event_Activate( anEntity *activator ) {
	// This is for my special case in tram1b
	//if ( physicsObj.StoppedMoving() ) {
	//	SetIdealSpeed( spawnArgs.GetFloat( "speed", "50" ) );
	//}
}

/*
================
anPhysics_SplineMover::Event_OnAcceleration
================
*/
void anPhysics_SplineMover::Event_OnAcceleration() {
	StartSound( "snd_accel", SND_CHANNEL_ANY, 0, false, nullptr );
}

/*
================
anPhysics_SplineMover::Event_OnDeceleration
================
*/
void anPhysics_SplineMover::Event_OnDeceleration() {
	StartSound( "snd_decel", SND_CHANNEL_ANY, 0, false, nullptr );
}

/*
================
anPhysics_SplineMover::Event_OnCruising
================
*/
void anPhysics_SplineMover::Event_OnCruising() {
	anVec2 range( spawnArgs.GetVec2( "noisePeriodRange" ) * anMath::M_SEC2MS );
	if ( !EventIsPosted(&EV_StartSoundPeriodic) && range.Length() > VECTOR_EPSILON ) {
		ProcessEvent( &EV_StartSoundPeriodic, "snd_noise", (int)SND_CHANNEL_ANY, (int)range[0], (int)range[1] );
	}
}

/*
================
anPhysics_SplineMover::Event_OnStopMoving
================
*/
void anPhysics_SplineMover::Event_OnStopMoving() {
	StopSound( SND_CHANNEL_ANY, false );
	CancelEvents( &EV_StartSoundPeriodic );
}

/*
================
anPhysics_SplineMover::Event_OnStartMoving
================
*/
void anPhysics_SplineMover::Event_OnStartMoving() {
}

/*
================
anPhysics_SplineMover::Event_StartSoundPeriodic
================
*/
void anPhysics_SplineMover::Event_StartSoundPeriodic( const char *sndKey, const s_channelType channel, int minDelay, int maxDelay ) {
	CancelEvents( &EV_StartSoundPeriodic );
	if ( physicsObj.StoppedMoving() ) {
		return;
	}

	int length;
	StartSound( sndKey, channel, 0, false, &length );
	PostEventMS( &EV_StartSoundPeriodic, Max(anRandom::irand(minDelay, maxDelay), length), sndKey, (int)channel, minDelay, maxDelay );
}


const anEventDef EV_TramCar_RadiusDamage( "<tramCar_radiusDamage>", "vs" );
const anEventDef EV_TramCar_SetIdealTrack( "setIdealTrack", "s" );
const anEventDef EV_TramCar_DriverSpeak( "driverSpeak", "s", 'e' );
const anEventDef EV_TramCar_GetDriver( "getDriver", "", 'E' );

const anEventDef EV_TramCar_OpenDoors( "openDoors" );
const anEventDef EV_TramCar_CloseDoors( "closeDoors" );

/*
================
anTramCar::HeadTowardsIdealTrack
================
*/
void anTramCar::HeadTowardsIdealTrack() {
	anSplinePath *ideal = FindSplineToIdealTrack( GetSpline() );
	if ( !ideal ) {
		ideal = GetRandomSpline(GetSpline());
	}

	SetSpline( ideal );
}

/*
================
anTramCar::FindSplineToTrack
================
*/
enum {
	LOOK_LEFT = -1,
	LOOK_RIGHT = 1
};
anSplinePath* anTramCar::FindSplineToTrack( anSplinePath* spline, const anStr& track ) const {
	anSplinePath*	target = nullptr;
	anEntity*		ent = nullptr;
	anStr			trackInfo;
	anList<anPhysics_SplineMover*> list;

	if ( !spline ) {
		return nullptr;
	}

	for ( int ix = SortSplineTargets( spline) - 1; ix >= 0; --ix ) {
		ent = GetSplineTarget( spline, ix );
		target = static_cast<anSplinePath*>( ent );
		assert( target->IsActive() );

		trackInfo = GetTrackInfo( target );
		if ( -1 >= trackInfo.Find(track) ) {
			continue;
		}

		// HACK: I hate switch statements
		switch ( ConvertToTrackNumber(trackInfo) - GetCurrentTrack() ) {
			case LOOK_LEFT: {
				if ( !LookLeft(list) ) {
					return target; 
				}
				break;
			}

			case LOOK_RIGHT: {
				if ( !LookRight(list) ) {
					return target; 
				}
				break;
			}

			default: {
				return target;
			}
		}
		// HACK
	}

	return nullptr;
}

/*
================
anTramCar::SortSplineTargets
================
*/
int	anTramCar::SortSplineTargets( anSplinePath *spline ) const {
	assert( spline );
	return (SignZero(GetSpeed()) >= 0) ? spline->SortTargets() : spline->SortBackwardsTargets();
}

/*
================
anTramCar::GetSplineTarget
================
*/
anEntity *anTramCar::GetSplineTarget( anSplinePath* spline, int index ) const {
	assert( spline );
	return (SignZero(GetSpeed()) >= 0) ? spline->targets[index].GetEntity() : spline->backwardPathTargets[index].GetEntity();
}

/*
================
anTramCar::FindSplineToIdealTrack
================
*/
anSplinePath* anTramCar::FindSplineToIdealTrack( anSplinePath* spline ) const {
	int trackDelta = anMath::ClampInt( -1, 1, GetIdealTrack() - GetCurrentTrack() );
	anStr trackLetter( ConvertToTrackLetter(GetCurrentTrack() + trackDelta) );
	anSplinePath *s = nullptr;

	if ( idealTrackTag.Length() && !trackDelta ) {
		s = FindSplineToTrack( spline, idealTrackTag + trackLetter );
	}
	if ( !s ) {
		s = FindSplineToTrack( spline, trackLetter );
	}
	return s;
}

/*
================
anTramCar::GetRandomSpline
================
*/
anSplinePath* anTramCar::GetRandomSpline( anSplinePath* spline ) const {
	if ( !spline ) {
		return nullptr;
	}

	int numActiveTargets = SortSplineTargets( spline );
	if ( !numActiveTargets ) {
		return nullptr;
	}

	anEntity *target = GetSplineTarget( spline, anRandom::irand(0, numActiveTargets - 1) );
	return (target && target->IsType(anSplinePath::GetClassType())) ? static_cast<anSplinePath*>(target) : nullptr;
}
