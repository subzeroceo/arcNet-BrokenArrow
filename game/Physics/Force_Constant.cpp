
#include "../../idlib/Lib.h"
#pragma hdrstop

#include "../Game_local.h"

CLASS_DECLARATION( anForce, anForce_Constant )
END_CLASS

/*
================
anForce_Constant::anForce_Constant
================
*/
anForce_Constant::anForce_Constant( void ) {
	force		= vec3_zero;
	physics		= nullptr;
	id			= 0;
	point		= vec3_zero;
}

/*
================
anForce_Constant::~anForce_Constant
================
*/
anForce_Constant::~anForce_Constant( void ) {
}

/*
================
anForce_Constant::Save
================
*/
void anForce_Constant::Save( anSaveGame *savefile ) const {
	savefile->WriteVec3( force );
	// TOSAVE: anPhysics *		physics
	savefile->WriteInt( id );
	savefile->WriteVec3( point );
}

/*
================
anForce_Constant::Restore
================
*/
void anForce_Constant::Restore( anRestoreGame *savefile ) {
	// Owner needs to call SetPhysics!!
	savefile->ReadVec3( force );
	savefile->ReadInt( id );
	savefile->ReadVec3( point );
}

/*
================
anForce_Constant::SetPosition
================
*/
void anForce_Constant::SetPosition( anPhysics *physics, int id, const anVec3 &point ) {
	this->physics = physics;
	this->id = id;
	this->point = point;
}

/*
================
anForce_Constant::SetForce
================
*/
void anForce_Constant::SetForce( const anVec3 &force ) {
	this->force = force;
}

/*
================
anForce_Constant::SetPhysics
================
*/
void anForce_Constant::SetPhysics( anPhysics *physics ) {
	this->physics = physics;
}

/*
================
anForce_Constant::Evaluate
================
*/
void anForce_Constant::Evaluate( int time ) {
	anVec3 p;

	if ( !physics ) {
		return;
	}

	p = physics->GetOrigin( id ) + point * physics->GetAxis( id );

	physics->AddForce( id, p, force );
}

/*
================
anForce_Constant::RemovePhysics
================
*/
void anForce_Constant::RemovePhysics( const anPhysics *phys ) {
	if ( physics == phys ) {
		physics = nullptr;
	}
}
