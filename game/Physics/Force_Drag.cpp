
#include "../../idlib/Lib.h"
#pragma hdrstop

#include "../Game_local.h"

CLASS_DECLARATION( anForce, anForce_Drag )END_CLASS

/*
================
anForce_Drag::anForce_Drag
================
*/
anForce_Drag::anForce_Drag( void ) {
	damping			= 0.5f;
	dragPosition	= vec3_zero;
	physics			= nullptr;
	id				= 0;
	p				= vec3_zero;
	dragPosition	= vec3_zero;
}

/*
================
anForce_Drag::~anForce_Drag
================
*/
anForce_Drag::~anForce_Drag( void ) {
}

/*
================
anForce_Drag::Init
================
*/
void anForce_Drag::Init( float damping ) {
	if ( damping >= 0.0f && damping < 1.0f ) {
		this->damping = damping;
	}
}

/*
================
anForce_Drag::SetPhysics
================
*/
void anForce_Drag::SetPhysics( anPhysics *phys, int id, const anVec3 &p ) {
	this->physics = phys;
	this->id = id;
	this->p = p;
}

/*
================
anForce_Drag::SetDragPosition
================
*/
void anForce_Drag::SetDragPosition( const anVec3 &pos ) {
	this->dragPosition = pos;
}

/*
================
anForce_Drag::GetDragPosition
================
*/
const anVec3 &anForce_Drag::GetDragPosition( void ) const {
	return this->dragPosition;
}

/*
================
anForce_Drag::GetDraggedPosition
================
*/
const anVec3 anForce_Drag::GetDraggedPosition( void ) const {
	return ( physics->GetOrigin( id ) + p * physics->GetAxis( id ) );
}

/*
================
anForce_Drag::Evaluate
================
*/
void anForce_Drag::Evaluate( int time ) {
	float l1, l2, mass;
	anVec3 dragOrigin, dir1, dir2, velocity, centerOfMass;
	anMat3 inertiaTensor;
	anRotation rotation;
	anClipModel *clipModel;

	if ( !physics ) {
		return;
	}

	clipModel = physics->GetClipModel( id );
	if ( clipModel != nullptr && clipModel->IsTraceModel() ) {
		clipModel->GetMassProperties( 1.0f, mass, centerOfMass, inertiaTensor );
	} else {
		centerOfMass.Zero();
	}

	centerOfMass = physics->GetOrigin( id ) + centerOfMass * physics->GetAxis( id );
	dragOrigin = physics->GetOrigin( id ) + p * physics->GetAxis( id );

	dir1 = dragPosition - centerOfMass;
	dir2 = dragOrigin - centerOfMass;
	l1 = dir1.Normalize();
	l2 = dir2.Normalize();

	rotation.Set( centerOfMass, dir2.Cross( dir1 ), RAD2DEG( anMath::ACos( dir1 * dir2 ) ) );
	physics->SetAngularVelocity( rotation.ToAngularVelocity() / MS2SEC( gameLocal.GetMSec() ), id );


// bdube: use GetMSec access rather than USERCMD_TIME
	velocity = physics->GetLinearVelocity( id ) * damping + dir1 * ( ( l1 - l2 ) * ( 1.0f - damping ) / MS2SEC( gameLocal.GetMSec() ) );

	physics->SetLinearVelocity( velocity, id );
}

/*
================
anForce_Drag::RemovePhysics
================
*/
void anForce_Drag::RemovePhysics( const anPhysics *phys ) {
	if ( physics == phys ) {
		physics = nullptr;
	}
}
