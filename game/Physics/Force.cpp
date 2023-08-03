
#include "../../idlib/Lib.h"
#pragma hdrstop
#include "../Game_local.h"
CLASS_DECLARATION( anClass, anForce )END_CLASS

anList<anForce *> anForce::forceList;

/*
================
anForce::anForce
================
*/
anForce::anForce( void ) {
	forceList.Append( this );
}

/*
================
anForce::~anForce
================
*/
anForce::~anForce( void ) {
	forceList.Remove( this );
}

/*
================
anForce::DeletePhysics
================
*/
void anForce::DeletePhysics( const anPhysics *phys ) {
	int i;

	for ( i = 0; i < forceList.Num(); i++ ) {
		forceList[i]->RemovePhysics( phys );
	}
}

/*
================
anForce::ClearForceList
================
*/
void anForce::ClearForceList( void ) {
	forceList.Clear();
}

/*
================
anForce::Evaluate
================
*/
void anForce::Evaluate( int time ) {
}

/*
================
anForce::RemovePhysics
================
*/
void anForce::RemovePhysics( const anPhysics *phys ) {
}
