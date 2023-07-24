#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Physics_Parabola.h"

CLASS_DECLARATION( arcPhysics_Base, arcPhysics_Parabola )
END_CLASS

/*
================
arcPhysics_Parabola::arcPhysics_Parabola
================
*/
arcPhysics_Parabola::arcPhysics_Parabola( void ) {
	current.origin.Zero();
	current.velocity.Zero();
	current.time = 0;
	clipModel = NULL;
	baseOrg.Zero();
	baseVelocity.Zero();
	baseAcceleration.Zero();
	baseAxes.Identity();}

/*
================
arcPhysics_Parabola::~arcPhysics_Parabola
================
*/
arcPhysics_Parabola::~arcPhysics_Parabola( void ) {
	gameLocal.clip.DeleteClipModel( clipModel );
}

/*
================
arcPhysics_Parabola::MakeDefault
================
*/
void arcPhysics_Parabola::MakeDefault( void ) {
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
arcPhysics_Parabola::CalcProperties
================
*/
void arcPhysics_Parabola::CalcProperties( arcVec3& origin, arcVec3& velocity, int time ) const {
	if ( time < startTime ) {
		time = startTime;
	}

	if ( endTime != -1 ) {
		if ( time > endTime ) {
			time = endTime;
		}
	}

	float elapsed = MS2SEC( time - startTime );

	arcVec3 acc = baseAcceleration + gravityVector;

	velocity		= baseVelocity + ( acc * elapsed );
	origin			= baseOrg + ( baseVelocity * elapsed ) + ( 0.5f * acc * Square( elapsed ) );
}

/*
================
arcPhysics_Parabola::Init
================
*/
void arcPhysics_Parabola::Init( const arcVec3& origin, const arcVec3& velocity, const arcVec3& acceleration, const arcMat3& axes, int _startTime, int _endTime ) {
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
arcPhysics_Parabola::EvaluatePosition
================
*/
arcVec3 arcPhysics_Parabola::EvaluatePosition( void ) const {
	arcVec3 org, vel;
	CalcProperties( org, vel, gameLocal.time );
	return org;
}

/*
================
arcPhysics_Parabola::Evaluate
================
*/
bool arcPhysics_Parabola::Evaluate( int timeStepMSec, int endTimeMSec ) {
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
arcPhysics_Parabola::LinkClip
================
*/
void arcPhysics_Parabola::LinkClip( void ) {
	if ( !clipModel ) {
		return;
	}

	clipModel->Link( gameLocal.clip, self, 0, current.origin, baseAxes );
}

/*
================
arcPhysics_Parabola::SetClipMask
================
*/
void arcPhysics_Parabola::SetContents( int contents, int id ) {
	if ( clipModel ) {
		clipModel->SetContents( contents );
	}
}

/*
================
arcPhysics_Parabola::Evaluate
================
*/
bool arcPhysics_Parabola::CollisionResponse( trace_t& collision ) {
	arcEntity* ent = gameLocal.entities[ collision.c.entityNum ];
	if ( !ent ) {
		gameLocal.Warning( "[Physics Parabola Collision Response] collision against an unknown entity" );
		return false;
	}

	impactInfo_t info;
	ent->GetImpactInfo( self, collision.c.id, collision.c.point, &info );

	arcVec3 velocity = current.velocity - info.velocity;

	ent->Hit( collision, velocity, self );
	return self->Collide( collision, velocity, -1 );
}

/*
==============
arcPhysics_Parabola::CheckWater
==============
*/
void arcPhysics_Parabola::CheckWater( void ) {
	waterLevel = 0.0f;

	const arcBounds &absBounds = GetAbsBounds( -1 );

	const arcClipModel *clipModel;
	int count = gameLocal.clip.ClipModelsTouchingBounds( CLIP_DEBUG_PARMS absBounds, CONTENTS_WATER, &clipModel, 1, NULL );
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

	const arcBounds& modelBounds = model->GetBounds();
	arcBounds worldbb;
	worldbb.FromTransformedBounds( GetBounds(), GetOrigin( 0 ), GetAxis( 0 ) );
	bool submerged = worldbb.GetMaxs()[2] < (modelBounds.GetMaxs()[2] + clipModel->GetOrigin().z);

	if ( submerged ) {
		waterLevel = 1.f;
	}
}

/*
================
arcPhysics_Parabola::CheckForCollisions
================
*/
bool arcPhysics_Parabola::CheckForCollisions( parabolaPState_t& next, trace_t& collision ) {
	if ( gameLocal.clip.Translation( CLIP_DEBUG_PARMS collision, current.origin, next.origin, clipModel, baseAxes, clipMask, self ) ) {
		next.origin		= collision.endpos;
		next.velocity	= current.velocity;
		return true;
	}

	return false;
}

/*
================
arcPhysics_Parabola::SetClipModel
================
*/
void arcPhysics_Parabola::SetClipModel( arcClipModel* model, float density, int id, bool freeOld ) {
	assert( self );
	assert( model );					// we need a clip model
	assert( model->IsTraceModel() );	// and it should be a trace model

	if ( clipModel != NULL && clipModel != model && freeOld ) {
		gameLocal.clip.DeleteClipModel( clipModel );
	}
	clipModel = model;
	LinkClip();
}

/*
================
arcPhysics_Parabola::GetClipModel
================
*/
arcClipModel *arcPhysics_Parabola::GetClipModel( int id ) const {
	return clipModel;
}

/*
================
arcPhysics_Parabola::SetAxis
================
*/
void arcPhysics_Parabola::SetAxis( const arcMat3& newAxis, int id ) {
	baseAxes = newAxis;

	LinkClip();
}

/*
================
arcPhysics_Parabola::SetOrigin
================
*/
void arcPhysics_Parabola::SetOrigin( const arcVec3& newOrigin, int id ) {
	current.origin = newOrigin;

	LinkClip();
}

const float	PARABOLA_ORIGIN_MAX				= 32767.0f;
const int	PARABOLA_ORIGIN_TOTAL_BITS		= 24;
const int	PARABOLA_ORIGIN_EXPONENT_BITS	= arcMath::BitsForInteger( arcMath::BitsForFloat( PARABOLA_ORIGIN_MAX ) ) + 1;
const int	PARABOLA_ORIGIN_MANTISSA_BITS	= PARABOLA_ORIGIN_TOTAL_BITS - 1 - PARABOLA_ORIGIN_EXPONENT_BITS;

const float	PARABOLA_VELOCITY_MAX			= 4000;
const int	PARABOLA_VELOCITY_TOTAL_BITS	= 16;
const int	PARABOLA_VELOCITY_EXPONENT_BITS	= arcMath::BitsForInteger( arcMath::BitsForFloat( PARABOLA_VELOCITY_MAX ) ) + 1;
const int	PARABOLA_VELOCITY_MANTISSA_BITS	= PARABOLA_VELOCITY_TOTAL_BITS - 1 - PARABOLA_VELOCITY_EXPONENT_BITS;

/*
================
arcPhysics_Parabola::GetBounds
================
*/
const arcBounds& arcPhysics_Parabola::GetBounds( int id ) const {
	if ( clipModel ) {
		return clipModel->GetBounds();
	}
	return arcPhysics_Base::GetBounds();
}

/*
================
arcPhysics_Parabola::GetAbsBounds
================
*/
const arcBounds& arcPhysics_Parabola::GetAbsBounds( int id ) const {
	if ( clipModel ) {
		return clipModel->GetAbsBounds();
	}
	return arcPhysics_Base::GetAbsBounds();
}

