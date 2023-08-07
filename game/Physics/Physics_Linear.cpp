#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Physics_Linear.h"
#include "../Entity.h"

CLASS_DECLARATION( idPhysics_Base, anPhysics_Linear )
END_CLASS

/*
================
anPhysics_Linear::Activate
================
*/
void anPhysics_Linear::Activate( void ) {
	current.atRest = -1;
	self->BecomeActive( TH_PHYSICS );
}

/*
================
anPhysics_Linear::TestIfAtRest
================
*/
bool anPhysics_Linear::TestIfAtRest( void ) const {
	if ( ( current.linearExtrapolation.GetExtrapolationType() & ~EXTRAPOLATION_NOSTOP ) == EXTRAPOLATION_NONE ) {
		return true;
	}

	if ( !current.linearExtrapolation.IsDone( static_cast<float>( current.time ) ) ) {
		return false;
	}

	return true;
}

/*
================
anPhysics_Linear::Rest
================
*/
void anPhysics_Linear::Rest( void ) {
	current.atRest = gameLocal.time;
//	self->BecomeInactive( TH_PHYSICS );
}

/*
================
anPhysics_Linear::anPhysics_Linear
================
*/
anPhysics_Linear::anPhysics_Linear( void ) {
	current.time = gameLocal.time;
	current.atRest = -1;
	current.origin.Zero();
	current.localOrigin.Zero();
	current.linearExtrapolation.Init( 0, 0, vec3_zero, vec3_zero, vec3_zero, EXTRAPOLATION_NONE );

	axis.Identity();

	saved			= current;
	isPusher		= false;
	pushFlags		= 0;
	clipModel		= NULL;
	isBlocked		= false;
	hasMaster		= false;
	isOrientated	= false;

	memset( &pushResults, 0, sizeof( pushResults ) );
}

/*
================
anPhysics_Linear::~anPhysics_Linear
================
*/
anPhysics_Linear::~anPhysics_Linear( void ) {
	gameLocal.clip.DeleteClipModel( clipModel );
}


/*
================
anPhysics_Linear::SetPusher
================
*/
void anPhysics_Linear::SetPusher( int flags ) {
	assert( clipModel );
	isPusher = true;
	pushFlags = flags;
}

/*
================
anPhysics_Linear::IsPusher
================
*/
bool anPhysics_Linear::IsPusher( void ) const {
	return isPusher;
}

/*
================
anPhysics_Linear::SetLinearExtrapolation
================
*/
void anPhysics_Linear::SetLinearExtrapolation( extrapolation_t type, int time, int duration, const anVec3 &base, const anVec3 &speed, const anVec3 &baseSpeed ) {
	current.time = gameLocal.time;
	current.linearExtrapolation.Init( time, duration, base, baseSpeed, speed, type );
	current.localOrigin = base;
	Activate();
}

/*
================
anPhysics_Linear::GetLocalOrigin
================
*/
void anPhysics_Linear::GetLocalOrigin( anVec3 &curOrigin ) const {
	curOrigin = current.localOrigin;
}

/*
================
anPhysics_Linear::SetClipModel
================
*/
void anPhysics_Linear::SetClipModel( anClipModel *model, float density, int id, bool freeOld ) {
	assert( self );
	assert( model );

	if ( clipModel != NULL && clipModel != model && freeOld ) {
		gameLocal.clip.DeleteClipModel( clipModel );
	}
	clipModel = model;
	clipModel->Link( gameLocal.clip, self, 0, current.origin, axis );
}

/*
================
anPhysics_Linear::GetClipModel
================
*/
anClipModel *anPhysics_Linear::GetClipModel( int id ) const {
	return clipModel;
}

/*
================
anPhysics_Linear::GetNumClipModels
================
*/
int anPhysics_Linear::GetNumClipModels( void ) const {
	return ( clipModel != NULL );
}

/*
================
anPhysics_Linear::SetMass
================
*/
void anPhysics_Linear::SetMass( float mass, int id ) {
}

/*
================
anPhysics_Linear::GetMass
================
*/
float anPhysics_Linear::GetMass( int id ) const {
	return 0.0f;
}

/*
================
anPhysics_Linear::SetClipMask
================
*/
void anPhysics_Linear::SetContents( int contents, int id ) {
	if ( clipModel ) {
		clipModel->SetContents( contents );
	}
}

/*
================
anPhysics_Linear::SetClipMask
================
*/
int anPhysics_Linear::GetContents( int id ) const {
	if ( clipModel ) {
		return clipModel->GetContents();
	}
	return 0;
}

/*
================
anPhysics_Linear::GetBounds
================
*/
const anBounds& anPhysics_Linear::GetBounds( int id ) const {
	return clipModel ? clipModel->GetBounds() : idPhysics_Base::GetBounds();
}

/*
================
anPhysics_Linear::GetAbsBounds
================
*/
const anBounds& anPhysics_Linear::GetAbsBounds( int id ) const {
	return clipModel ? clipModel->GetAbsBounds() : idPhysics_Base::GetAbsBounds();
}

/*
================
anPhysics_Linear::Evaluate
================
*/
bool anPhysics_Linear::Evaluate( int timeStepMSec, int endTimeMSec ) {
	current.origin.FixDenormals();

	anVec3 oldLocalOrigin, oldOrigin, masterOrigin;
	anMat3 oldAxis, masterAxis;

	isBlocked		= false;
	oldLocalOrigin	= current.localOrigin;
	oldOrigin		= current.origin;
	oldAxis			= axis;

	current.localOrigin = current.linearExtrapolation.GetCurrentValue( endTimeMSec );
	current.origin		= current.localOrigin;

	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		if ( masterAxis.IsRotated() ) {
			current.origin = current.origin * masterAxis + masterOrigin;
			if ( isOrientated ) {
				axis *= masterAxis;
			}
		} else {
			current.origin += masterOrigin;
		}
	}

	if ( isPusher && ( oldOrigin != current.origin ) ) {
		gameLocal.push.ClipPush( pushResults, self, pushFlags, oldOrigin, oldAxis, current.origin, axis, GetClipModel() );
		if ( pushResults.fraction < 1.0f ) {
			clipModel->Link( gameLocal.clip, self, 0, oldOrigin, oldAxis );
			current.localOrigin = oldLocalOrigin;
			current.origin		= oldOrigin;
			axis				= oldAxis;
			isBlocked			= true;
			return false;
		}
	}

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, axis );
	}

	current.time = endTimeMSec;

	if ( TestIfAtRest() ) {
		Rest();
	}

	return ( current.origin != oldOrigin );
}

/*
================
anPhysics_Linear::UpdateTime
================
*/
void anPhysics_Linear::UpdateTime( int endTimeMSec ) {
	int timeLeap = endTimeMSec - current.time;
	current.time = endTimeMSec;
	// move the trajectory start times to sync the trajectory with the current endTime
	current.linearExtrapolation.SetStartTime( current.linearExtrapolation.GetStartTime() + timeLeap );
}

/*
================
anPhysics_Linear::GetTime
================
*/
int anPhysics_Linear::GetTime( void ) const {
	return current.time;
}

/*
================
anPhysics_Linear::IsAtRest
================
*/
bool anPhysics_Linear::IsAtRest( void ) const {
	return current.atRest >= 0;
}

/*
================
anPhysics_Linear::GetRestStartTime
================
*/
int anPhysics_Linear::GetRestStartTime( void ) const {
	return current.atRest;
}

/*
================
anPhysics_Linear::IsPushable
================
*/
bool anPhysics_Linear::IsPushable( void ) const {
	return false;
}

/*
================
anPhysics_Linear::SaveState
================
*/
void anPhysics_Linear::SaveState( void ) {
	saved = current;
}

/*
================
anPhysics_Linear::RestoreState
================
*/
void anPhysics_Linear::RestoreState( void ) {
	current = saved;

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, axis );
	}
}

/*
================
anPhysics_Linear::SetOrigin
================
*/
void anPhysics_Linear::SetOrigin( const anVec3 &newOrigin, int id ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	current.linearExtrapolation.SetStartValue( newOrigin );
	current.localOrigin = current.linearExtrapolation.GetCurrentValue( current.time );
	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + current.localOrigin * masterAxis;
	} else {
		current.origin = current.localOrigin;
	}
	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, axis );
	}
	Activate();
}

/*
================
anPhysics_Linear::SetAxis
================
*/
void anPhysics_Linear::SetAxis( const anMat3 &newAxis, int id ) {
	axis = newAxis;

	if ( hasMaster && isOrientated ) {
		anVec3 masterOrigin;
		anMat3 masterAxis;
		self->GetMasterPosition( masterOrigin, masterAxis );
		axis *= masterAxis;
	}

	if ( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, axis );
	}
	Activate();
}

/*
================
anPhysics_Linear::Move
================
*/
void anPhysics_Linear::Translate( const anVec3 &translation, int id ) {
}

/*
================
anPhysics_Linear::Rotate
================
*/
void anPhysics_Linear::Rotate( const idRotation &rotation, int id ) {
}

/*
================
anPhysics_Linear::GetOrigin
================
*/
const anVec3 &anPhysics_Linear::GetOrigin( int id ) const {
	return current.origin;
}

/*
================
anPhysics_Linear::GetAxis
================
*/
const anMat3 &anPhysics_Linear::GetAxis( int id ) const {
	return axis;
}

/*
================
anPhysics_Linear::SetLinearVelocity
================
*/
void anPhysics_Linear::SetLinearVelocity( const anVec3 &newLinearVelocity, int id ) {
	SetLinearExtrapolation( extrapolation_t( EXTRAPOLATION_LINEAR | EXTRAPOLATION_NOSTOP ), gameLocal.time, 0, current.origin, newLinearVelocity, vec3_origin );
	Activate();
}

/*
================
anPhysics_Linear::SetAngularVelocity
================
*/
void anPhysics_Linear::SetAngularVelocity( const anVec3 &newAngularVelocity, int id ) {
}

/*
================
anPhysics_Linear::GetLinearVelocity
================
*/
const anVec3 &anPhysics_Linear::GetLinearVelocity( int id ) const {
	static anVec3 curLinearVelocity;
	curLinearVelocity = current.linearExtrapolation.GetCurrentSpeed( gameLocal.time );
	return curLinearVelocity;
}

/*
================
anPhysics_Linear::GetAngularVelocity
================
*/
const anVec3 &anPhysics_Linear::GetAngularVelocity( int id ) const {
	return vec3_zero;
}

/*
================
anPhysics_Linear::UnlinkClip
================
*/
void anPhysics_Linear::UnlinkClip( void ) {
	if ( clipModel != NULL ) {
		clipModel->Unlink( gameLocal.clip );
	}
}

/*
================
anPhysics_Linear::LinkClip
================
*/
void anPhysics_Linear::LinkClip( void ) {
	if ( clipModel != NULL ) {
		clipModel->Link( gameLocal.clip, self, 0, current.origin, axis );
	}
}

/*
================
anPhysics_Linear::DisableClip
================
*/
void anPhysics_Linear::DisableClip( bool activateContacting ) {
	if ( clipModel != NULL ) {
		if ( activateContacting ) {
			WakeEntitiesContacting( self, clipModel );
		}
		clipModel->Disable();
	}
}

/*
================
anPhysics_Linear::EnableClip
================
*/
void anPhysics_Linear::EnableClip( void ) {
	if ( clipModel != NULL ) {
		clipModel->Enable();
	}
}

/*
================
anPhysics_Linear::GetBlockingInfo
================
*/
const trace_t *anPhysics_Linear::GetBlockingInfo( void ) const {
	return ( isBlocked ? &pushResults : NULL );
}

/*
================
anPhysics_Linear::GetBlockingEntity
================
*/
anEntity *anPhysics_Linear::GetBlockingEntity( void ) const {
	if ( isBlocked ) {
		return gameLocal.entities[ pushResults.c.entityNum ];
	}
	return NULL;
}

/*
================
anPhysics_Linear::SetMaster
================
*/
void anPhysics_Linear::SetMaster( anEntity *master, const bool orientated ) {
	if ( master ) {
		if ( !hasMaster ) {
			anVec3 masterOrigin;
			anMat3 masterAxis;

			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
			current.linearExtrapolation.SetStartValue( current.localOrigin );

			hasMaster			= true;
			isOrientated		= orientated;
		}
	} else {
		if ( hasMaster ) {
			// transform from master space to world space
			current.localOrigin = current.origin;
			SetLinearExtrapolation( EXTRAPOLATION_NONE, 0, 0, current.origin, vec3_origin, vec3_origin );
			hasMaster = false;
		}
	}
}

/*
================
anPhysics_Linear::GetLinearEndTime
================
*/
int anPhysics_Linear::GetLinearEndTime( void ) const {
	return current.linearExtrapolation.GetEndTime();
}
