
#include "../../idlib/Lib.h"
#pragma hdrstop

#include "../Game_local.h"

CLASS_DECLARATION( anForce, anForce_Spring )
END_CLASS

/*
================
anForce_Spring::anForce_Spring
================
*/
anForce_Spring::anForce_Spring( void ) {
	Kstretch		= 100.0f;
	Kcompress		= 100.0f;
	damping			= 0.0f;
	restLength		= 0.0f;
	physics1		= nullptr;
	id1				= 0;
	p1				= vec3_zero;
	physics2		= nullptr;
	id2				= 0;
	p2				= vec3_zero;
}

/*
================
anForce_Spring::~anForce_Spring
================
*/
anForce_Spring::~anForce_Spring( void ) {
}

/*
================
anForce_Spring::InitSpring
================
*/
void anForce_Spring::InitSpring( float Kstretch, float Kcompress, float damping, float restLength ) {
	this->Kstretch = Kstretch;
	this->Kcompress = Kcompress;
	this->damping = damping;
	this->restLength = restLength;
}

/*
================
anForce_Spring::SetPosition
================
*/
void anForce_Spring::SetPosition( anPhysics *physics1, int id1, const anVec3 &p1, anPhysics *physics2, int id2, const anVec3 &p2 ) {
	this->physics1 = physics1;
	this->id1 = id1;
	this->p1 = p1;
	this->physics2 = physics2;
	this->id2 = id2;
	this->p2 = p2;
}

/*
================
anForce_Spring::Evaluate
================
*/
void anForce_Spring::Evaluate( int time ) {
	float length;
	anMat3 axis;
	anVec3 pos1, pos2, velocity1, velocity2, force, dampingForce;
	impactInfo_t info;

	pos1 = p1;
	pos2 = p2;
	velocity1 = velocity2 = vec3_origin;

	if ( physics1 ) {
		axis = physics1->GetAxis( id1 );
		pos1 = physics1->GetOrigin( id1 );
		pos1 += p1 * axis;
		if ( damping > 0.0f ) {
			physics1->GetImpactInfo( id1, pos1, &info );
			velocity1 = info.velocity;
		}
	}

	if ( physics2 ) {
		axis = physics2->GetAxis( id2 );
		pos2 = physics2->GetOrigin( id2 );
		pos2 += p2 * axis;
		if ( damping > 0.0f ) {
			physics2->GetImpactInfo( id2, pos2, &info );
			velocity2 = info.velocity;
		}
	}

	force = pos2 - pos1;
	dampingForce = ( damping * ( ((velocity2 - velocity1) * force) / (force * force) ) ) * force;
	length = force.Normalize();

	// if the spring is stretched
	if ( length > restLength ) {
		if ( Kstretch > 0.0f ) {
			force = ( Square( length - restLength ) * Kstretch ) * force - dampingForce;
			if ( physics1 ) {
				physics1->AddForce( id1, pos1, force );
			}
			if ( physics2 ) {
				physics2->AddForce( id2, pos2, -force );
			}
		}
	} else {
		if ( Kcompress > 0.0f ) {
			force = ( Square( length - restLength ) * Kcompress ) * force - dampingForce;
			if ( physics1 ) {
				physics1->AddForce( id1, pos1, -force );
			}
			if ( physics2 ) {
				physics2->AddForce( id2, pos2, force );
			}
		}
	}
}

/*
================
anForce_Spring::RemovePhysics
================
*/
void anForce_Spring::RemovePhysics( const anPhysics *phys ) {
	if ( physics1 == phys ) {
		physics1 = nullptr;
	}
	if ( physics2 == phys ) {
		physics2 = nullptr;
	}
}

/*
================
anForce_Spring::Save
================
*/
void anForce_Spring::Save( anSaveGame *savefile ) const {
	savefile->WriteFloat( Kstretch );
	savefile->WriteFloat( Kcompress );
	savefile->WriteFloat( damping );
	savefile->WriteFloat( restLength );
}

/*
================
anForce_Spring::Restore
================
*/
void anForce_Spring::Restore( anRestoreGame *savefile ) {
	savefile->ReadFloat( Kstretch );
	savefile->ReadFloat( Kcompress );
	savefile->ReadFloat( damping );
	savefile->ReadFloat( restLength );
}

