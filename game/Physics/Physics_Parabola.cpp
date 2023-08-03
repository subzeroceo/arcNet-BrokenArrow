#include "../Lib.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Physics_Parabola.h"

CLASS_DECLARATION( anPhysics_Base, anPhysics_Parabola )
END_CLASS

/*
================
anPhysics_Parabola::anPhysics_Parabola
================
*/
anPhysics_Parabola::anPhysics_Parabola( void ) {
	current.origin.Zero();
	current.velocity.Zero();
	current.time = 0;
	clipModel = nullptr;
	baseOrg.Zero();
	baseVelocity.Zero();
	baseAcceleration.Zero();
	baseAxes.Identity();}

/*
================
anPhysics_Parabola::~anPhysics_Parabola
================
*/
anPhysics_Parabola::~anPhysics_Parabola( void ) {
	gameLocal.clip.DeleteClipModel( clipModel );
}

/*
================
anPhysics_Parabola::MakeDefault
================
*/
void anPhysics_Parabola::MakeDefault( void ) {
	orientation.x	= 0.f;
	orientation.y	= 0.f;
	orientation.z	= 0.f;

	position		= vec3_origin;
	velocity		= vec3_zero;
	acceleration	= vec3_zero;

	startTime		= 0;
	endTime			= 0;
}

/*
================
anPhysics_Parabola::CalcProperties
================
*/
void anPhysics_Parabola::CalcProperties( anVec3& origin, anVec3& velocity, int time ) const {
	if ( time < startTime ) {
		time = startTime;
	}

	if ( endTime != -1 ) {
		if ( time > endTime ) {
			time = endTime;
		}
	}

	float elapsed = MS2SEC( time - startTime );

	anVec3 acc = baseAcceleration + gravityVector;

	velocity		= baseVelocity + ( acc * elapsed );
	origin			= baseOrg + ( baseVelocity * elapsed ) + ( 0.5f * acc * Square( elapsed ) );
}

/*
================
anPhysics_Parabola::Init
================
*/
void anPhysics_Parabola::Init( const anVec3& origin, const anVec3& velocity, const anVec3& acceleration, const anMat3& axes, int _startTime, int _endTime ) {
	baseAxes			= axes;
	baseOrg				= origin;
	baseVelocity		= velocity;
	baseAcceleration	= acceleration;
	startTime			= _startTime;
	endTime				= _endTime;

	CalcProperties( current.origin, current.velocity, gameLocal.time );
	current.time = gameLocal.time;
}

/*
================
anPhysics_Parabola::EvaluatePosition
================
*/
anVec3 anPhysics_Parabola::EvaluatePosition( void ) const {
	anVec3 org, vel;
	CalcProperties( org, vel, gameLocal.time );
	return org;
}

/*
================
anPhysics_Parabola::Evaluate
================
*/
bool anPhysics_Parabola::Evaluate( int timeStepMSec, int endTimeMSec ) {
	parabolaPState_t next;

	CalcProperties( current.origin, current.velocity, endTimeMSec - timeStepMSec );
	current.time = endTimeMSec - timeStepMSec;
	CalcProperties( next.origin, next.velocity, endTimeMSec );
	next.time = endTimeMSec;

	CheckWater();

	trace_t collision;
	bool collided = CheckForCollisions( next, collision );

	current = next;

	if ( collided ) {
		if ( CollisionResponse( collision ) ) {
			startTime		= endTimeMSec;
			endTime			= endTimeMSec;
			baseOrg			= current.origin;
			current.velocity.Zero();
			baseVelocity.Zero();
			baseAcceleration.Zero();
		}
	}

	LinkClip();

	return true;
}

/*
================
anPhysics_Parabola::LinkClip
================
*/
void anPhysics_Parabola::LinkClip( void ) {
	if ( !clipModel ) {
		return;
	}

	clipModel->Link( gameLocal.clip, self, 0, current.origin, baseAxes );
}

/*
================
anPhysics_Parabola::SetClipMask
================
*/
void anPhysics_Parabola::SetContents( int contents, int id ) {
	if ( clipModel ) {
		clipModel->SetContents( contents );
	}
}

/*
================
anPhysics_Parabola::Evaluate
================
*/
bool anPhysics_Parabola::CollisionResponse( trace_t &collision ) {
	arcEntity* ent = gameLocal.entities[ collision.c.entityNum ];
	if ( !ent ) {
		gameLocal.Warning( "[Parabola-Physics] Collision Response against unknown entity" );
		return false;
	}

	impactInfo_t info;
	ent->GetImpactInfo( self, collision.c.id, collision.c.point, &info );

	anVec3 velocity = current.velocity - info.velocity;

	ent->Hit( collision, velocity, self );
	return self->Collide( collision, velocity, -1 );
}

/*
==============
anPhysics_Parabola::CheckWater
==============
*/
void anPhysics_Parabola::CheckWater( void ) {
	waterLevel = 0.0f;

	const anBounds &absBounds = GetAbsBounds( -1 );

	const anClipModel *clipModel;
	int count = gameLocal.clip.ClipModelsTouchingBounds( CLIP_DEBUG_PARMS absBounds, CONTENTS_WATER, &clipModel, 1, nullptr );
	if ( !count ) {
		return;
	}

	if ( !clipModel->GetNumCollisionModels() ) {
		return;
	}

	arcCollisionModel *model = clipModel->GetCollisionModel( 0 );
	int numPlanes = model->GetNumBrushPlanes();
	if ( !numPlanes ) {
		return;
	}

	self->CheckWater( clipModel->GetOrigin(), clipModel->GetAxis(), model );

	const anBounds& modelBounds = model->GetBounds();
	anBounds worldbb;
	worldbb.FromTransformedBounds( GetBounds(), GetOrigin( 0 ), GetAxis( 0 ) );
	bool submerged = worldbb.GetMaxs()[2] < (modelBounds.GetMaxs()[2] + clipModel->GetOrigin().z);

	if ( submerged ) {
		waterLevel = 1.f;
	}
}

/*
================
anPhysics_Parabola::CheckForCollisions
================
*/
bool anPhysics_Parabola::CheckForCollisions( parabolaPState_t& next, trace_t& collision ) {
	if ( gameLocal.clip.Translation( CLIP_DEBUG_PARMS collision, current.origin, next.origin, clipModel, baseAxes, clipMask, self ) ) {
		next.origin		= collision.endpos;
		next.velocity	= current.velocity;
		return true;
	}

	return false;
}

/*
================
anPhysics_Parabola::SetClipModel
================
*/
void anPhysics_Parabola::SetClipModel( anClipModel* model, float density, int id, bool freeOld ) {
	assert( self );
	assert( model );					// we need a clip model
	assert( model->IsTraceModel() );	// and it should be a trace model

	if ( clipModel != nullptr && clipModel != model && freeOld ) {
		gameLocal.clip.DeleteClipModel( clipModel );
	}
	clipModel = model;
	LinkClip();
}

/*
================
anPhysics_Parabola::GetClipModel
================
*/
anClipModel *anPhysics_Parabola::GetClipModel( int id ) const {
	return clipModel;
}

/*
================
anPhysics_Parabola::SetAxis
================
*/
void anPhysics_Parabola::SetAxis( const anMat3& newAxis, int id ) {
	baseAxes = newAxis;
	LinkClip();
}

/*
================
anPhysics_Parabola::SetOrigin
================
*/
void anPhysics_Parabola::SetOrigin( const anVec3& newOrigin, int id ) {
	current.origin = newOrigin;
	LinkClip();
}

const float	PARABOLA_ORIGIN_MAX				= 32767.0f;
const int	PARABOLA_ORIGIN_TOTAL_BITS		= 24;
const int	PARABOLA_ORIGIN_EXPONENT_BITS	= anMath::BitsForInteger( anMath::BitsForFloat( PARABOLA_ORIGIN_MAX ) ) + 1;
const int	PARABOLA_ORIGIN_MANTISSA_BITS	= PARABOLA_ORIGIN_TOTAL_BITS - 1 - PARABOLA_ORIGIN_EXPONENT_BITS;

const float	PARABOLA_VELOCITY_MAX			= 4000;
const int	PARABOLA_VELOCITY_TOTAL_BITS	= 16;
const int	PARABOLA_VELOCITY_EXPONENT_BITS	= anMath::BitsForInteger( anMath::BitsForFloat( PARABOLA_VELOCITY_MAX ) ) + 1;
const int	PARABOLA_VELOCITY_MANTISSA_BITS	= PARABOLA_VELOCITY_TOTAL_BITS - 1 - PARABOLA_VELOCITY_EXPONENT_BITS;

/*
================
anPhysics_Parabola::GetBounds
================
*/
const anBounds& anPhysics_Parabola::GetBounds( int id ) const {
	if ( clipModel ) {
		return clipModel->GetBounds();
	}
	return anPhysics_Base::GetBounds();
}

/*
================
anPhysics_Parabola::GetAbsBounds
================
*/
const anBounds& anPhysics_Parabola::GetAbsBounds( int id ) const {
	if ( clipModel ) {
		return clipModel->GetAbsBounds();
	}
	return anPhysics_Base::GetAbsBounds();
}

