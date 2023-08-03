
#include "../../idlib/Lib.h"
#pragma hdrstop

#include "../Game_local.h"

CLASS_DECLARATION( anPhysics_Base, anPhysics_RigidBody )
END_CLASS

const float STOP_SPEED		= 10.0f;


#undef RB_TIMINGS

#ifdef RB_TIMINGS
static int lastTimerReset = 0;
static int numRigidBodies = 0;
static anTimer timer_total, timer_collision;
#endif


/*
================
RigidBodyDerivatives
================
*/
void RigidBodyDerivatives( const float t, const void *clientData, const float *state, float *derivatives ) {
	const anPhysics_RigidBody *p = (anPhysics_RigidBody *) clientData;
	rigidBodyIState_t *s = (rigidBodyIState_t *) state;
	// NOTE: this struct should be build conform rigidBodyIState_t
	struct rigidBodyDerivatives_s {
		anVec3				linearVelocity;
		anMat3				angularMatrix;
		anVec3				force;
		anVec3				torque;
	} *d = ( struct rigidBodyDerivatives_s *) derivatives;
	anVec3 angularVelocity;
	anMat3 inverseWorldInertiaTensor;

	inverseWorldInertiaTensor = s->orientation * p->inverseInertiaTensor * s->orientation.Transpose();
	angularVelocity = inverseWorldInertiaTensor * s->angularMomentum;
	// derivatives
	d->linearVelocity = p->inverseMass * s->linearMomentum;
	d->angularMatrix = SkewSymmetric( angularVelocity ) * s->orientation;
	d->force = - p->linearFriction * s->linearMomentum + p->current.externalForce;
	d->torque = - p->angularFriction * s->angularMomentum + p->current.externalTorque;
}

/*
================
anPhysics_RigidBody::Integrate

  Calculate next state from the current state using an integrator.
================
*/
void anPhysics_RigidBody::Integrate( float deltaTime, rigidBodyPState_t &next ) {
	anVec3 position;

	position = current.i.position;
	current.i.position += centerOfMass * current.i.orientation;

	current.i.orientation.TransposeSelf();

	integrator->Evaluate( (float *) &current.i, (float *) &next.i, 0, deltaTime );
	next.i.orientation.OrthoNormalizeSelf();

	// apply gravity
	next.i.linearMomentum += deltaTime * gravityVector * mass;

	current.i.orientation.TransposeSelf();
	next.i.orientation.TransposeSelf();

	current.i.position = position;
	next.i.position -= centerOfMass * next.i.orientation;

	next.atRest = current.atRest;
}

/*
================
anPhysics_RigidBody::CollisionImpulse

  Calculates the collision impulse using the velocity relative to the collision object.
  The current state should be set to the moment of impact.
================
*/
bool anPhysics_RigidBody::CollisionImpulse( const trace_t &collision, anVec3 &impulse ) {
	anVec3 r, linearVelocity, angularVelocity, velocity;
	anMat3 inverseWorldInertiaTensor;
	float impulseNumerator, impulseDenominator, vel;
	impactInfo_t info;
	anEntity *ent;

	// get info from other entity involved
	ent = gameLocal.entities[collision.c.entityNum];
	ent->GetImpactInfo( self, collision.c.id, collision.c.point, &info );

	// once in water take out the water flag	and increase friction
	if ( ent->GetPhysics()->GetContents() & CONTENTS_WATER ) {
		clipMask &= ~CONTENTS_WATER;
		linearFriction *= 20.0f;
		angularFriction *= 20.0f;
	}

	// collision point relative to the body center of mass
	r = collision.c.point - (current.i.position + centerOfMass * current.i.orientation);

	// the velocity at the collision point
	linearVelocity = inverseMass * current.i.linearMomentum;
	inverseWorldInertiaTensor = current.i.orientation.Transpose() * inverseInertiaTensor * current.i.orientation;
	angularVelocity = inverseWorldInertiaTensor * current.i.angularMomentum;
	velocity = linearVelocity + angularVelocity.Cross(r);
	// subtract velocity of other entity
	velocity -= info.velocity;

	// velocity in normal direction
	vel = velocity * collision.c.normal;

	if ( vel > -STOP_SPEED ) {
		impulseNumerator = STOP_SPEED;
	} else {
		impulseNumerator = -( 1.0f + bouncyness ) * vel;
	}
	impulseDenominator = inverseMass + ( ( inverseWorldInertiaTensor * r.Cross( collision.c.normal ) ).Cross( r ) * collision.c.normal );
	if ( info.invMass ) {
		impulseDenominator += info.invMass + ( ( info.invInertiaTensor * info.position.Cross( collision.c.normal ) ).Cross( info.position ) * collision.c.normal );
	}
	impulse = (impulseNumerator / impulseDenominator) * collision.c.normal;

	// update linear and angular momentum with impulse
	current.i.linearMomentum += impulse;
	current.i.angularMomentum += r.Cross(impulse);

	// if no movement at all don't blow up
	if ( collision.fraction < 0.0001f ) {
		current.i.linearMomentum *= 0.5f;
		current.i.angularMomentum *= 0.5f;
	}

	// callback to self to let the entity know about the collision
	return self->Collide( collision, velocity );
}

/*
================
anPhysics_RigidBody::CheckForCollisions

  Check for collisions between the current and next state.
  If there is a collision the next state is set to the state at the moment of impact.
================
*/
bool anPhysics_RigidBody::CheckForCollisions( const float deltaTime, rigidBodyPState_t &next, trace_t &collision ) {
//#define TEST_COLLISION_DETECTION
	anMat3 axis;
	anRotation rotation;
	bool collided = false;

#ifdef TEST_COLLISION_DETECTION
	bool startsolid;
	if ( gameLocal.Contents( self, current.i.position, clipModel, current.i.orientation, clipMask, self ) ) {
		startsolid = true;
	}
#endif

	TransposeMultiply( current.i.orientation, next.i.orientation, axis );
	rotation = axis.ToRotation();
	rotation.SetOrigin( current.i.position );

	// if there was a collision
	if ( gameLocal.Motion( self, collision, current.i.position, next.i.position, rotation, clipModel, current.i.orientation, clipMask, self ) ) {
		// set the next state to the state at the moment of impact
		next.i.position = collision.endpos;
		next.i.orientation = collision.endAxis;
		next.i.linearMomentum = current.i.linearMomentum;
		next.i.angularMomentum = current.i.angularMomentum;
		collided = true;
	}

#ifdef TEST_COLLISION_DETECTION
	if ( gameLocal.Contents( self, next.i.position, clipModel, next.i.orientation, clipMask, self ) ) {
		if ( !startsolid ) {
			int bah = 1;
		}
	}
#endif
	return collided;
}

/*
================
anPhysics_RigidBody::ContactFriction

  Does not solve friction for multiple simultaneous contacts but applies contact friction in isolation.
  Uses absolute velocity at the contact points instead of the velocity relative to the contact object.
================
*/
void anPhysics_RigidBody::ContactFriction( float deltaTime ) {
	int i;
	float magnitude, impulseNumerator, impulseDenominator;
	anMat3 inverseWorldInertiaTensor;
	anVec3 linearVelocity, angularVelocity;
	anVec3 massCenter, r, velocity, normal, impulse, normalVelocity;

	inverseWorldInertiaTensor = current.i.orientation.Transpose() * inverseInertiaTensor * current.i.orientation;

	massCenter = current.i.position + centerOfMass * current.i.orientation;

	for ( i = 0; i < contacts.Num(); i++ ) {

		r = contacts[i].point - massCenter;

		// calculate velocity at contact point
		linearVelocity = inverseMass * current.i.linearMomentum;
		angularVelocity = inverseWorldInertiaTensor * current.i.angularMomentum;
		velocity = linearVelocity + angularVelocity.Cross(r);

		// velocity along normal vector
		normalVelocity = ( velocity * contacts[i].normal ) * contacts[i].normal;

		// calculate friction impulse
		normal = -( velocity - normalVelocity );
		magnitude = normal.Normalize();
		impulseNumerator = contactFriction * magnitude;
		impulseDenominator = inverseMass + ( ( inverseWorldInertiaTensor * r.Cross( normal ) ).Cross( r ) * normal );
		impulse = (impulseNumerator / impulseDenominator) * normal;

		// apply friction impulse
		current.i.linearMomentum += impulse;
		current.i.angularMomentum += r.Cross(impulse);

		// if moving towards the surface at the contact point
		if ( normalVelocity * contacts[i].normal < 0.0f ) {
			// calculate impulse
			normal = -normalVelocity;
			impulseNumerator = normal.Normalize();
			impulseDenominator = inverseMass + ( ( inverseWorldInertiaTensor * r.Cross( normal ) ).Cross( r ) * normal );
			impulse = (impulseNumerator / impulseDenominator) * normal;

			// apply impulse
			current.i.linearMomentum += impulse;
			current.i.angularMomentum += r.Cross( impulse );
		}
	}
}

/*
================
anPhysics_RigidBody::TestIfAtRest

  Returns true if the body is considered at rest.
  Does not catch all cases where the body is at rest but is generally good enough.
================
*/
bool anPhysics_RigidBody::TestIfAtRest( void ) const {
	int i;
	float gv;
	anVec3 v, av, normal, point;
	anMat3 inverseWorldInertiaTensor;
	anFixedWinding contactWinding;

	if ( current.atRest >= 0 ) {
		return true;
	}

	// need at least 3 contact points to come to rest
	if ( contacts.Num() < 3 ) {
		return false;
	}

	// get average contact plane normal
	normal.Zero();
	for ( i = 0; i < contacts.Num(); i++ ) {
		normal += contacts[i].normal;
	}
	normal /= ( float ) contacts.Num();
	normal.Normalize();

	// if on a too steep surface
	if ( (normal * gravityNormal) > -0.7f ) {
		return false;
	}

	// create bounds for contact points
	contactWinding.Clear();
	for ( i = 0; i < contacts.Num(); i++ ) {
		// project point onto plane through origin orthogonal to the gravity
		point = contacts[i].point - (contacts[i].point * gravityNormal) * gravityNormal;
		contactWinding.AddToConvexHull( point, gravityNormal );
	}

	// need at least 3 contact points to come to rest
	if ( contactWinding.GetNumPoints() < 3 ) {
		return false;
	}

	// center of mass in world space
	point = current.i.position + centerOfMass * current.i.orientation;
	point -= (point * gravityNormal) * gravityNormal;

	// if the point is not inside the winding
	if ( !contactWinding.PointInside( gravityNormal, point, 0 ) ) {
		return false;
	}

	// linear velocity of body
	v = inverseMass * current.i.linearMomentum;
	// linear velocity in gravity direction
	gv = v * gravityNormal;
	// linear velocity orthogonal to gravity direction
	v -= gv * gravityNormal;

	// if too much velocity orthogonal to gravity direction
	if ( v.Length() > STOP_SPEED ) {
		return false;
	}
	// if too much velocity in gravity direction
	if ( gv > 2.0f * STOP_SPEED || gv < -2.0f * STOP_SPEED ) {
		return false;
	}

	// calculate rotational velocity
	inverseWorldInertiaTensor = current.i.orientation * inverseInertiaTensor * current.i.orientation.Transpose();
	av = inverseWorldInertiaTensor * current.i.angularMomentum;

	// if too much rotational velocity
	if ( av.LengthSqr() > STOP_SPEED ) {
		return false;
	}

	return true;
}

/*
================
anPhysics_RigidBody::DropToFloorAndRest

  Drops the object straight down to the floor and verifies if the object is at rest on the floor.
================
*/
void anPhysics_RigidBody::DropToFloorAndRest( void ) {
	anVec3 down;
	trace_t tr;

	if ( testSolid ) {
		testSolid = false;

// ddynerman: multiple collision worlds
		if ( gameLocal.Contents( self, current.i.position, clipModel, current.i.orientation, clipMask, self ) ) {

			gameLocal.DWarning( "rigid body in solid for entity '%s' type '%s' at (%s)",
								self->name.c_str(), self->GetType()->classname, current.i.position.ToString(0) );
			Rest();
			dropToFloor = false;
			return;
		}
	}

	// put the body on the floor
	down = current.i.position + gravityNormal * 128.0f;


	gameLocal.Translation( self, tr, current.i.position, down, clipModel, current.i.orientation, clipMask, self );

	current.i.position = tr.endpos;


	clipModel->Link( self, clipModel->GetId(), tr.endpos, current.i.orientation );


	// if on the floor already
	if ( tr.fraction == 0.0f ) {
		// test if we are really at rest
		EvaluateContacts();
		if ( !TestIfAtRest() ) {
			gameLocal.DWarning( "rigid body not at rest for entity '%s' type '%s' at (%s)",
								self->name.c_str(), self->GetType()->classname, current.i.position.ToString(0) );
		}
		Rest();
		dropToFloor = false;
	} else if ( IsOutsideWorld() ) {
		gameLocal.Warning( "rigid body outside world bounds for entity '%s' type '%s' at (%s)",
							self->name.c_str(), self->GetType()->classname, current.i.position.ToString(0) );
		Rest();
		dropToFloor = false;
	}
}

/*
================
anPhysics_RigidBody::DebugDraw
================
*/
void anPhysics_RigidBody::DebugDraw( void ) {
	if ( rb_showBodies.GetBool() || ( rb_showActive.GetBool() && current.atRest < 0 ) ) {
		collisionModelManager->DrawModel( clipModel->GetCollisionModel(), clipModel->GetOrigin(), clipModel->GetAxis(), vec3_origin, mat3_identity, 0.0f );
	}

	if ( rb_showMass.GetBool() ) {
		// draw center of mass at the center of mass
		gameRenderWorld->DrawText( va( "\n%1.2f", mass ), current.i.position + centerOfMass * current.i.orientation, 0.08f, colorCyan, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1 );
	}

	if ( rb_showInertia.GetBool() ) {
		anMat3 &I = inertiaTensor;
		gameRenderWorld->DrawText( va( "\n\n\n( %.1f %.1f %.1f )\n( %.1f %.1f %.1f )\n( %.1f %.1f %.1f )",
									I[0].x, I[0].y, I[0].z,
									I[1].x, I[1].y, I[1].z,
									I[2].x, I[2].y, I[2].z ),
									current.i.position, 0.05f, colorCyan, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1 );
	}

	if ( rb_showVelocity.GetBool() ) {
		DrawVelocity( clipModel->GetId(), 0.1f, 4.0f );
	}

	if ( rb_showContacts.GetBool() ) {
		int i;
		for ( i = 0; i < contacts.Num(); i ++ ) {
			anVec3 x, y;
			contacts[i].normal.NormalVectors( x, y );
			gameRenderWorld->DebugLine( colorWhite, contacts[i].point, contacts[i].point + 6.0f * contacts[i].normal );
			gameRenderWorld->DebugLine( colorWhite, contacts[i].point - 2.0f * x, contacts[i].point + 2.0f * x );
			gameRenderWorld->DebugLine( colorWhite, contacts[i].point - 2.0f * y, contacts[i].point + 2.0f * y );
		}
	}
}

/*
================
anPhysics_RigidBody::anPhysics_RigidBody
================
*/
anPhysics_RigidBody::anPhysics_RigidBody( void ) {
	// set default rigid body properties
	SetClipMask( MASK_SOLID );
	SetBouncyness( 0.6f );
	SetFriction( 0.6f, 0.6f, 0.0f );
	clipModel = nullptr;

	memset( &current, 0, sizeof( current ) );

	current.atRest = -1;

// bdube: use GetMSec access rather than USERCMD_TIME
	current.lastTimeStep = gameLocal.GetMSec();


	current.i.position.Zero();
	current.i.orientation.Identity();

	current.i.linearMomentum.Zero();
	current.i.angularMomentum.Zero();

	saved = current;

	mass = 1.0f;
	inverseMass = 1.0f;
	centerOfMass.Zero();
	inertiaTensor.Identity();
	inverseInertiaTensor.Identity();

	// use the least expensive euler integrator
	integrator = new anODE_Euler( sizeof(rigidBodyIState_t) / sizeof( float ), RigidBodyDerivatives, this );

	dropToFloor = false;
	noImpact = false;
	noContact = false;

	hasMaster = false;
	isOrientated = false;

#ifdef RB_TIMINGS
	lastTimerReset = 0;
#endif
}

/*
================
anPhysics_RigidBody::~anPhysics_RigidBody
================
*/
anPhysics_RigidBody::~anPhysics_RigidBody( void ) {
	if ( clipModel ) {
		delete clipModel;
		clipModel = nullptr;
	}
	delete integrator;
}

/*
================
anPhysics_RigidBody_SavePState
================
*/
void anPhysics_RigidBody_SavePState( anSaveGame *savefile, const rigidBodyPState_t &state ) {
	savefile->WriteInt( state.atRest );
	savefile->WriteFloat( state.lastTimeStep );
	savefile->WriteVec3( state.localOrigin );
	savefile->WriteMat3( state.localAxis );
	savefile->Write( &state.pushVelocity, sizeof( state.pushVelocity ) );
	savefile->WriteVec3( state.externalForce );
	savefile->WriteVec3( state.externalTorque );

	savefile->WriteVec3( state.i.position );
	savefile->WriteMat3( state.i.orientation );
	savefile->WriteVec3( state.i.linearMomentum );
	savefile->WriteVec3( state.i.angularMomentum );
}

/*
================
anPhysics_RigidBody_RestorePState
================
*/
void anPhysics_RigidBody_RestorePState( anRestoreGame *savefile, rigidBodyPState_t &state ) {
	savefile->ReadInt( state.atRest );
	savefile->ReadFloat( state.lastTimeStep );
	savefile->ReadVec3( state.localOrigin );
	savefile->ReadMat3( state.localAxis );
	savefile->Read( &state.pushVelocity, sizeof( state.pushVelocity ) );
	savefile->ReadVec3( state.externalForce );
	savefile->ReadVec3( state.externalTorque );

	savefile->ReadVec3( state.i.position );
	savefile->ReadMat3( state.i.orientation );
	savefile->ReadVec3( state.i.linearMomentum );
	savefile->ReadVec3( state.i.angularMomentum );
}

/*
================
anPhysics_RigidBody::Save
================
*/
void anPhysics_RigidBody::Save( anSaveGame *savefile ) const {

	anPhysics_RigidBody_SavePState( savefile, current );
	anPhysics_RigidBody_SavePState( savefile, saved );

	savefile->WriteFloat( linearFriction );
	savefile->WriteFloat( angularFriction );
	savefile->WriteFloat( contactFriction );
	savefile->WriteFloat( bouncyness );
	savefile->WriteClipModel( clipModel );

	savefile->WriteFloat( mass );
	savefile->WriteFloat( inverseMass );
	savefile->WriteVec3( centerOfMass );
	savefile->WriteMat3( inertiaTensor );
	savefile->WriteMat3( inverseInertiaTensor );

	savefile->WriteBool( dropToFloor );
	savefile->WriteBool( testSolid );
	savefile->WriteBool( noImpact );
	savefile->WriteBool( noContact );

	savefile->WriteBool( hasMaster );
	savefile->WriteBool( isOrientated );
}

/*
================
anPhysics_RigidBody::Restore
================
*/
void anPhysics_RigidBody::Restore( anRestoreGame *savefile ) {
	anPhysics_RigidBody_RestorePState( savefile, current );
	anPhysics_RigidBody_RestorePState( savefile, saved );

	savefile->ReadFloat( linearFriction );
	savefile->ReadFloat( angularFriction );
	savefile->ReadFloat( contactFriction );
	savefile->ReadFloat( bouncyness );
	savefile->ReadClipModel( clipModel );

	savefile->ReadFloat( mass );
	savefile->ReadFloat( inverseMass );
	savefile->ReadVec3( centerOfMass );
	savefile->ReadMat3( inertiaTensor );
	savefile->ReadMat3( inverseInertiaTensor );

	savefile->ReadBool( dropToFloor );
	savefile->ReadBool( testSolid );
	savefile->ReadBool( noImpact );
	savefile->ReadBool( noContact );

	savefile->ReadBool( hasMaster );
	savefile->ReadBool( isOrientated );
}

/*
================
anPhysics_RigidBody::SetClipModel
================
*/
#define MAX_INERTIA_SCALE		10.0f
void anPhysics_RigidBody::SetClipModel( anClipModel *model, const float density, int id, bool freeOld ) {
	int minIndex;
	anMat3 inertiaScale;

	assert( self );
	assert( model );					// we need a clip model
	assert( model->IsTraceModel() );	// and it should be a trace model
	assert( density > 0.0f );			// density should be valid

	if ( clipModel && clipModel != model && freeOld ) {
		delete clipModel;
	}
	clipModel = model;
	clipModel->Link( self, 0, current.i.position, current.i.orientation );

	// get mass properties from the trace model
	clipModel->GetMassProperties( density, mass, centerOfMass, inertiaTensor );

	// check whether or not the clip model has valid mass properties
	if ( mass <= 0.0f || FLOAT_IS_NAN( mass ) ) {
		gameLocal.Warning( "anPhysics_RigidBody::SetClipModel: invalid mass for entity '%s' type '%s'",
							self->name.c_str(), self->GetType()->classname );
		mass = 1.0f;
		centerOfMass.Zero();
		inertiaTensor.Identity();
	}

	// check whether or not the inertia tensor is balanced
	minIndex = Min3Index( inertiaTensor[0][0], inertiaTensor[1][1], inertiaTensor[2][2] );
	inertiaScale.Identity();
	inertiaScale[0][0] = inertiaTensor[0][0] / inertiaTensor[minIndex][minIndex];
	inertiaScale[1][1] = inertiaTensor[1][1] / inertiaTensor[minIndex][minIndex];
	inertiaScale[2][2] = inertiaTensor[2][2] / inertiaTensor[minIndex][minIndex];

	if ( inertiaScale[0][0] > MAX_INERTIA_SCALE || inertiaScale[1][1] > MAX_INERTIA_SCALE || inertiaScale[2][2] > MAX_INERTIA_SCALE ) {
		gameLocal.DWarning( "anPhysics_RigidBody::SetClipModel: unbalanced inertia tensor for entity '%s' type '%s'",
							self->name.c_str(), self->GetType()->classname );
		float min = inertiaTensor[minIndex][minIndex] * MAX_INERTIA_SCALE;
		inertiaScale[(minIndex+1)%3][(minIndex+1)%3] = min / inertiaTensor[(minIndex+1)%3][(minIndex+1)%3];
		inertiaScale[(minIndex+2)%3][(minIndex+2)%3] = min / inertiaTensor[(minIndex+2)%3][(minIndex+2)%3];
		inertiaTensor *= inertiaScale;
	}

	inverseMass = 1.0f / mass;
	inverseInertiaTensor = inertiaTensor.Inverse() * ( 1.0f / 6.0f );

	current.i.linearMomentum.Zero();
	current.i.angularMomentum.Zero();
}

/*
================
anPhysics_RigidBody::GetClipModel
================
*/
anClipModel *anPhysics_RigidBody::GetClipModel( int id ) const {
	return clipModel;
}

/*
================
anPhysics_RigidBody::GetNumClipModels
================
*/
int anPhysics_RigidBody::GetNumClipModels( void ) const {
	return 1;
}

/*
================
anPhysics_RigidBody::SetMass
================
*/
void anPhysics_RigidBody::SetMass( float mass, int id ) {
	assert( mass > 0.0f );
	inertiaTensor *= mass / this->mass;
	inverseInertiaTensor = inertiaTensor.Inverse() * (1.0f / 6.0f);
	this->mass = mass;
	inverseMass = 1.0f / mass;
}

/*
================
anPhysics_RigidBody::GetMass
================
*/
float anPhysics_RigidBody::GetMass( int id ) const {
	return mass;
}
/*
================
anPhysics_RigidBody::GetCenterMass
================
*/
anVec3 anPhysics_RigidBody::GetCenterMass( int id ) const {
	return ( current.i.position + centerOfMass * current.i.orientation );
}

/*
================
anPhysics_RigidBody::SetFriction
================
*/
void anPhysics_RigidBody::SetFriction( const float linear, const float angular, const float contact ) {
	if ( linear < 0.0f || linear > 1.0f || angular < 0.0f || angular > 1.0f || contact < 0.0f || contact > 1.0f ) {
		return;
	}
	linearFriction = linear;
	angularFriction = angular;
	contactFriction = contact;
}

/*
================
anPhysics_RigidBody::SetBouncyness
================
*/
void anPhysics_RigidBody::SetBouncyness( const float b ) {
	if ( b < 0.0f || b > 1.0f ) {
		return;
	}
	bouncyness = b;
}

/*
================
anPhysics_RigidBody::Rest
================
*/
void anPhysics_RigidBody::Rest( void ) {
	current.atRest = gameLocal.time;
	current.i.linearMomentum.Zero();
	current.i.angularMomentum.Zero();
	self->BecomeInactive( TH_PHYSICS );
}

/*
================
anPhysics_RigidBody::DropToFloor
================
*/
void anPhysics_RigidBody::DropToFloor( void ) {
	dropToFloor = true;
	testSolid = true;
}

/*
================
anPhysics_RigidBody::NoContact
================
*/
void anPhysics_RigidBody::NoContact( void ) {
	noContact = true;
}

/*
================
anPhysics_RigidBody::Activate
================
*/
void anPhysics_RigidBody::Activate( void ) {
	current.atRest = -1;
	self->BecomeActive( TH_PHYSICS );
}

/*
================
anPhysics_RigidBody::PutToRest

  put to rest untill something collides with this physics object
================
*/
void anPhysics_RigidBody::PutToRest( void ) {
	Rest();
}

/*
================
anPhysics_RigidBody::EnableImpact
================
*/
void anPhysics_RigidBody::EnableImpact( void ) {
	noImpact = false;
}

/*
================
anPhysics_RigidBody::DisableImpact
================
*/
void anPhysics_RigidBody::DisableImpact( void ) {
	noImpact = true;
}

/*
================
anPhysics_RigidBody::SetContents
================
*/
void anPhysics_RigidBody::SetContents( int contents, int id ) {
	clipModel->SetContents( contents );
}

/*
================
anPhysics_RigidBody::GetContents
================
*/
int anPhysics_RigidBody::GetContents( int id ) const {
	return clipModel->GetContents();
}

/*
================
anPhysics_RigidBody::GetBounds
================
*/
const anBounds &anPhysics_RigidBody::GetBounds( int id ) const {
	return clipModel->GetBounds();
}

/*
================
anPhysics_RigidBody::GetAbsBounds
================
*/
const anBounds &anPhysics_RigidBody::GetAbsBounds( int id ) const {
	return clipModel->GetAbsBounds();
}

/*
================
anPhysics_RigidBody::Evaluate

  Evaluate the impulse based rigid body physics.
  When a collision occurs an impulse is applied at the moment of impact but
  the remaining time after the collision is ignored.
================
*/
bool anPhysics_RigidBody::Evaluate( int timeStepMSec, int endTimeMSec ) {
	rigidBodyPState_t next;
	anAngles angles;
	trace_t collision;
	anVec3 impulse;
	anEntity *ent;
	anVec3 oldOrigin, masterOrigin;
	anMat3 oldAxis, masterAxis;
	float timeStep;
	bool collided, cameToRest = false;

	timeStep = MS2SEC( timeStepMSec );
	current.lastTimeStep = timeStep;

	if ( hasMaster ) {
		oldOrigin = current.i.position;
		oldAxis = current.i.orientation;
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.i.position = masterOrigin + current.localOrigin * masterAxis;
		if ( isOrientated ) {
			current.i.orientation = current.localAxis * masterAxis;
		}
		else {
			current.i.orientation = current.localAxis;
		}
		clipModel->Link( self, clipModel->GetId(), current.i.position, current.i.orientation );
		current.i.linearMomentum = mass * ( ( current.i.position - oldOrigin ) / timeStep );
		current.i.angularMomentum = inertiaTensor * ( ( current.i.orientation * oldAxis.Transpose() ).ToAngularVelocity() / timeStep );
		current.externalForce.Zero();
		current.externalTorque.Zero();

		return ( current.i.position != oldOrigin || current.i.orientation != oldAxis );
	}

	// if the body is at rest
	if ( current.atRest >= 0 || timeStep <= 0.0f ) {
		DebugDraw();
		return false;
	}

	// if putting the body to rest
	if ( dropToFloor ) {
		DropToFloorAndRest();
		current.externalForce.Zero();
		current.externalTorque.Zero();
		return true;
	}

#ifdef RB_TIMINGS
	if ( rb_showTimings->integer != 0 ) {
		timer_total.Start();
	}
#endif

	// move the rigid body velocity into the frame of a pusher
//	current.i.linearMomentum -= current.pushVelocity.SubVec3( 0 ) * mass;
//	current.i.angularMomentum -= current.pushVelocity.SubVec3( 1 ) * inertiaTensor;
	clipModel->Unlink();

	next = current;

	// calculate next position and orientation
	Integrate( timeStep, next );

#ifdef RB_TIMINGS
	if ( rb_showTimings->integer != 0 ) {
		timer_collision.Start();
	}
#endif

	// check for collisions from the current to the next state
	collided = CheckForCollisions( timeStep, next, collision );

#ifdef RB_TIMINGS
	if ( rb_showTimings->integer != 0 ) {
		timer_collision.Stop();
	}
#endif

	// set the new state
	current = next;

//	trace_t					pushResults;
//	gameLocal.push.ClipPush( pushResults, self, PUSHFL_CRUSH | PUSHFL_CLIP, saved.localOrigin, saved.localAxis, current.localOrigin, current.localAxis );

	if ( collided ) {
		// apply collision impulse
		if ( CollisionImpulse( collision, impulse ) ) {
			current.atRest = gameLocal.time;
		}
	}

	// update the position of the clip model
	clipModel->Link( self, clipModel->GetId(), current.i.position, current.i.orientation );
	DebugDraw();

	if ( !noContact ) {

#ifdef RB_TIMINGS
		if ( rb_showTimings->integer != 0 ) {
			timer_collision.Start();
		}
#endif
		// get contacts
		EvaluateContacts();

#ifdef RB_TIMINGS
		if ( rb_showTimings->integer != 0 ) {
			timer_collision.Stop();
		}
#endif

		// check if the body has come to rest
		if ( TestIfAtRest() ) {
			// put to rest
			Rest();
			cameToRest = true;
		}  else {
			// apply contact friction
			ContactFriction( timeStep );
		}
	}

	if ( current.atRest < 0 ) {
		ActivateContactEntities();
	}

	if ( collided ) {
		// if the rigid body didn't come to rest or the other entity is not at rest
		ent = gameLocal.entities[collision.c.entityNum];
		if ( ent && ( !cameToRest || !ent->IsAtRest() ) ) {
			// apply impact to other entity
			ent->ApplyImpulse( self, collision.c.id, collision.c.point, -impulse );
		}
	}

	// move the rigid body velocity back into the world frame
//	current.i.linearMomentum += current.pushVelocity.SubVec3( 0 ) * mass;
//	current.i.angularMomentum += current.pushVelocity.SubVec3( 1 ) * inertiaTensor;
	current.pushVelocity.Zero();

	current.lastTimeStep = timeStep;
	current.externalForce.Zero();
	current.externalTorque.Zero();

	if ( IsOutsideWorld() ) {
		gameLocal.Warning( "rigid body moved outside world bounds for entity '%s' type '%s' at (%s)",
					self->name.c_str(), self->GetType()->classname, current.i.position.ToString(0) );
		Rest();
	}

#ifdef RB_TIMINGS
	if ( rb_showTimings->integer != 0 ) {
		timer_total.Stop();

		if ( rb_showTimings->integer == 1 ) {
			gameLocal.Printf( "%12s: t %1.4f cd %1.4f\n",
							self->name.c_str(),
							timer_total.Milliseconds(), timer_collision.Milliseconds() );
			lastTimerReset = 0;
		}
		else if ( rb_showTimings->integer == 2 ) {
			numRigidBodies++;
			if ( endTimeMSec > lastTimerReset ) {
				gameLocal.Printf( "rb %d: t %1.4f cd %1.4f\n",
								numRigidBodies,
								timer_total.Milliseconds(), timer_collision.Milliseconds() );
			}
		}
		if ( endTimeMSec > lastTimerReset ) {
			lastTimerReset = endTimeMSec;
			numRigidBodies = 0;
			timer_total.Clear();
			timer_collision.Clear();
		}
	}
#endif

	return true;
}

/*
================
anPhysics_RigidBody::UpdateTime
================
*/
void anPhysics_RigidBody::UpdateTime( int endTimeMSec ) {
}

/*
================
anPhysics_RigidBody::GetTime
================
*/
int anPhysics_RigidBody::GetTime( void ) const {
	return gameLocal.time;
}

/*
================
anPhysics_RigidBody::GetImpactInfo
================
*/
void anPhysics_RigidBody::GetImpactInfo( const int id, const anVec3 &point, impactInfo_t *info ) const {
	anVec3 linearVelocity, angularVelocity;
	anMat3 inverseWorldInertiaTensor;

	linearVelocity = inverseMass * current.i.linearMomentum;
	inverseWorldInertiaTensor = current.i.orientation.Transpose() * inverseInertiaTensor * current.i.orientation;
	angularVelocity = inverseWorldInertiaTensor * current.i.angularMomentum;

	info->invMass = inverseMass;
	info->invInertiaTensor = inverseWorldInertiaTensor;
	info->position = point - ( current.i.position + centerOfMass * current.i.orientation );
	info->velocity = linearVelocity + angularVelocity.Cross( info->position );
}

/*
================
anPhysics_RigidBody::ApplyImpulse
================
*/
void anPhysics_RigidBody::ApplyImpulse( const int id, const anVec3 &point, const anVec3 &impulse ) {
	if ( noImpact ) {
		return;
	}
	current.i.linearMomentum += impulse;
	current.i.angularMomentum += ( point - ( current.i.position + centerOfMass * current.i.orientation ) ).Cross( impulse );
	Activate();
}

/*
================
anPhysics_RigidBody::AddForce
================
*/
void anPhysics_RigidBody::AddForce( const int id, const anVec3 &point, const anVec3 &force ) {
	if ( noImpact ) {
		return;
	}
	current.externalForce += force;
	current.externalTorque += ( point - ( current.i.position + centerOfMass * current.i.orientation ) ).Cross( force );
	Activate();
}

/*
================
anPhysics_RigidBody::IsAtRest
================
*/
bool anPhysics_RigidBody::IsAtRest( void ) const {
	return current.atRest >= 0;
}

/*
================
anPhysics_RigidBody::GetRestStartTime
================
*/
int anPhysics_RigidBody::GetRestStartTime( void ) const {
	return current.atRest;
}

/*
================
anPhysics_RigidBody::IsPushable
================
*/
bool anPhysics_RigidBody::IsPushable( void ) const {
	return ( !noImpact && !hasMaster );
}

/*
================
anPhysics_RigidBody::SaveState
================
*/
void anPhysics_RigidBody::SaveState( void ) {
	saved = current;
}

/*
================
anPhysics_RigidBody::RestoreState
================
*/
void anPhysics_RigidBody::RestoreState( void ) {
	current = saved;
	clipModel->Link( self, clipModel->GetId(), current.i.position, current.i.orientation );
	EvaluateContacts();
}

/*
================
anPhysics::SetOrigin
================
*/
void anPhysics_RigidBody::SetOrigin( const anVec3 &newOrigin, int id ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	current.localOrigin = newOrigin;
	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.i.position = masterOrigin + newOrigin * masterAxis;
	} else {
		current.i.position = newOrigin;
	}
	clipModel->Link( self, clipModel->GetId(), current.i.position, clipModel->GetAxis() );

	Activate();
}

/*
================
anPhysics::SetAxis
================
*/
void anPhysics_RigidBody::SetAxis( const anMat3 &newAxis, int id ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	current.localAxis = newAxis;
	if ( hasMaster && isOrientated ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.i.orientation = newAxis * masterAxis;
	} else {
		current.i.orientation = newAxis;
	}
	clipModel->Link( self, clipModel->GetId(), clipModel->GetOrigin(), current.i.orientation );

	Activate();
}

/*
================
anPhysics::Move
================
*/
void anPhysics_RigidBody::Translate( const anVec3 &translation, int id ) {
	current.localOrigin += translation;
	current.i.position += translation;
	clipModel->Link( self, clipModel->GetId(), current.i.position, clipModel->GetAxis() );
	Activate();
}

/*
================
anPhysics::Rotate
================
*/
void anPhysics_RigidBody::Rotate( const anRotation &rotation, int id ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	current.i.orientation *= rotation.ToMat3();
	current.i.position *= rotation;

	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.localAxis *= rotation.ToMat3();
		current.localOrigin = ( current.i.position - masterOrigin ) * masterAxis.Transpose();
	} else {
		current.localAxis = current.i.orientation;
		current.localOrigin = current.i.position;
	}
	clipModel->Link( self, clipModel->GetId(), current.i.position, current.i.orientation );
	Activate();
}

/*
================
anPhysics_RigidBody::GetOrigin
================
*/
const anVec3 &anPhysics_RigidBody::GetOrigin( int id ) const {
	return current.i.position;
}

/*
================
anPhysics_RigidBody::GetAxis
================
*/
const anMat3 &anPhysics_RigidBody::GetAxis( int id ) const {
	return current.i.orientation;
}

/*
================
anPhysics_RigidBody::SetLinearVelocity
================
*/
void anPhysics_RigidBody::SetLinearVelocity( const anVec3 &newLinearVelocity, int id ) {
	current.i.linearMomentum = newLinearVelocity * mass;
	Activate();
}

/*
================
anPhysics_RigidBody::SetAngularVelocity
================
*/
void anPhysics_RigidBody::SetAngularVelocity( const anVec3 &newAngularVelocity, int id ) {
	current.i.angularMomentum = newAngularVelocity * inertiaTensor;
	Activate();
}

/*
================
anPhysics_RigidBody::GetLinearVelocity
================
*/
const anVec3 &anPhysics_RigidBody::GetLinearVelocity( int id ) const {
	static anVec3 curLinearVelocity;
	curLinearVelocity = current.i.linearMomentum * inverseMass;
	return curLinearVelocity;
}

/*
================
anPhysics_RigidBody::GetAngularVelocity
================
*/
const anVec3 &anPhysics_RigidBody::GetAngularVelocity( int id ) const {
	static anVec3 curAngularVelocity;
	anMat3 inverseWorldInertiaTensor;

	inverseWorldInertiaTensor = current.i.orientation.Transpose() * inverseInertiaTensor * current.i.orientation;
	curAngularVelocity = inverseWorldInertiaTensor * current.i.angularMomentum;
	return curAngularVelocity;
}

/*
================
anPhysics_RigidBody::ClipTranslation
================
*/
void anPhysics_RigidBody::ClipTranslation( trace_t &results, const anVec3 &translation, const anClipModel *model ) const {
	if ( model ) {
		gameLocal.TranslationModel( self, results, clipModel->GetOrigin(), clipModel->GetOrigin() + translation,
		clipModel, clipModel->GetAxis(), clipMask,
		model->GetCollisionModel(), model->GetOrigin(), model->GetAxis() );
	} else {
		gameLocal.Translation( self, results, clipModel->GetOrigin(), clipModel->GetOrigin() + translation,
		clipModel, clipModel->GetAxis(), clipMask, self );
	}
}

/*
================
anPhysics_RigidBody::ClipRotation
================
*/
void anPhysics_RigidBody::ClipRotation( trace_t &results, const anRotation &rotation, const anClipModel *model ) const {
	if ( model ) {
		gameLocal.RotationModel( self, results, clipModel->GetOrigin(), rotation,
			clipModel, clipModel->GetAxis(), clipMask,
			model->GetCollisionModel(), model->GetOrigin(), model->GetAxis() );
	} else {
		gameLocal.Rotation( self, results, clipModel->GetOrigin(), rotation,
		clipModel, clipModel->GetAxis(), clipMask, self );
	}
}

/*
================
anPhysics_RigidBody::ClipContents
================
*/
int anPhysics_RigidBody::ClipContents( const anClipModel *model ) const {
	if ( model ) {
		return gameLocal.ContentsModel( self, clipModel->GetOrigin(), clipModel, clipModel->GetAxis(), -1, model->GetCollisionModel(), model->GetOrigin(), model->GetAxis() );
	} else {
		return gameLocal.Contents( self, clipModel->GetOrigin(), clipModel, clipModel->GetAxis(), -1, nullptr );
	}
}

/*
================
anPhysics_RigidBody::DisableClip
================
*/
void anPhysics_RigidBody::DisableClip( void ) {
	clipModel->Disable();
}

/*
================
anPhysics_RigidBody::EnableClip
================
*/
void anPhysics_RigidBody::EnableClip( void ) {
	clipModel->Enable();
}

/*
================
anPhysics_RigidBody::UnlinkClip
================
*/
void anPhysics_RigidBody::UnlinkClip( void ) {
	clipModel->Unlink();
}

/*
================
anPhysics_RigidBody::LinkClip
================
*/
void anPhysics_RigidBody::LinkClip( void ) {
	clipModel->Link( self, clipModel->GetId(), current.i.position, current.i.orientation );
}

/*
================
anPhysics_RigidBody::EvaluateContacts
================
*/
bool anPhysics_RigidBody::EvaluateContacts( void ) {
	anVec6 dir;
	int num;

	ClearContacts();

	contacts.SetNum( 10, false );

	dir.SubVec3(0) = current.i.linearMomentum + current.lastTimeStep * gravityVector * mass;
	dir.SubVec3( 1 ) = current.i.angularMomentum;
	dir.SubVec3(0).Normalize();
	dir.SubVec3( 1 ).Normalize();
	num = gameLocal.Contacts( self, &contacts[0], 10, clipModel->GetOrigin(),
					dir, CONTACT_EPSILON, clipModel, clipModel->GetAxis(), clipMask, self );
	contacts.SetNum( num, false );

	AddContactEntitiesForContacts();

	return ( contacts.Num() != 0 );
}

/*
================
anPhysics_RigidBody::SetPushed
================
*/
void anPhysics_RigidBody::SetPushed( int deltaTime ) {
	anRotation rotation;
	rotation = ( saved.i.orientation * current.i.orientation ).ToRotation();

	// velocity with which the af is pushed
	current.pushVelocity.SubVec3(0) += ( current.i.position - saved.i.position ) / ( deltaTime * anMath::M_MS2SEC );
	current.pushVelocity.SubVec3( 1 ) += rotation.GetVec() * -DEG2RAD( rotation.GetAngle() ) / ( deltaTime * anMath::M_MS2SEC );
}

/*
================
anPhysics_RigidBody::GetPushedLinearVelocity
================
*/
const anVec3 &anPhysics_RigidBody::GetPushedLinearVelocity( const int id ) const {
	return current.pushVelocity.SubVec3(0);
}

/*
================
anPhysics_RigidBody::GetPushedAngularVelocity
================
*/
const anVec3 &anPhysics_RigidBody::GetPushedAngularVelocity( const int id ) const {
	return current.pushVelocity.SubVec3( 1 );
}

/*
================
anPhysics_RigidBody::SetMaster
================
*/
void anPhysics_RigidBody::SetMaster( anEntity *master, const bool orientated ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	if ( master ) {
		if ( !hasMaster ) {
			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			current.localOrigin = ( current.i.position - masterOrigin ) * masterAxis.Transpose();
			if ( orientated ) {
				current.localAxis = current.i.orientation * masterAxis.Transpose();
			} else {
				current.localAxis = current.i.orientation;
			}
			hasMaster = true;
			isOrientated = orientated;
			ClearContacts();
		}
	} else {
		if ( hasMaster ) {
			hasMaster = false;
			Activate();
		}
	}
}

const float	RB_VELOCITY_MAX				= 16000;
const int	RB_VELOCITY_TOTAL_BITS		= 16;
const int	RB_VELOCITY_EXPONENT_BITS	= anMath::BitsForInteger( anMath::BitsForFloat( RB_VELOCITY_MAX ) ) + 1;
const int	RB_VELOCITY_MANTISSA_BITS	= RB_VELOCITY_TOTAL_BITS - 1 - RB_VELOCITY_EXPONENT_BITS;
const float	RB_MOMENTUM_MAX				= 1e20f;
const int	RB_MOMENTUM_TOTAL_BITS		= 16;
const int	RB_MOMENTUM_EXPONENT_BITS	= anMath::BitsForInteger( anMath::BitsForFloat( RB_MOMENTUM_MAX ) ) + 1;
const int	RB_MOMENTUM_MANTISSA_BITS	= RB_MOMENTUM_TOTAL_BITS - 1 - RB_MOMENTUM_EXPONENT_BITS;
const float	RB_FORCE_MAX				= 1e20f;
const int	RB_FORCE_TOTAL_BITS			= 16;
const int	RB_FORCE_EXPONENT_BITS		= anMath::BitsForInteger( anMath::BitsForFloat( RB_FORCE_MAX ) ) + 1;
const int	RB_FORCE_MANTISSA_BITS		= RB_FORCE_TOTAL_BITS - 1 - RB_FORCE_EXPONENT_BITS;

/*
================
anPhysics_RigidBody::WriteToSnapshot
================
*/
void anPhysics_RigidBody::WriteToSnapshot( anBitMsgDelta &msg ) const {
	anCQuat quat, localQuat;

	quat = current.i.orientation.ToCQuat();
	localQuat = current.localAxis.ToCQuat();

	msg.WriteLong( current.atRest );
	msg.WriteFloat( current.i.position[0] );
	msg.WriteFloat( current.i.position[1] );
	msg.WriteFloat( current.i.position[2] );
	msg.WriteFloat( quat.x );
	msg.WriteFloat( quat.y );
	msg.WriteFloat( quat.z );
	msg.WriteFloat( current.i.linearMomentum[0], RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	msg.WriteFloat( current.i.linearMomentum[1], RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	msg.WriteFloat( current.i.linearMomentum[2], RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	msg.WriteFloat( current.i.angularMomentum[0], RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	msg.WriteFloat( current.i.angularMomentum[1], RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	msg.WriteFloat( current.i.angularMomentum[2], RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	msg.WriteDeltaFloat( current.i.position[0], current.localOrigin[0] );
	msg.WriteDeltaFloat( current.i.position[1], current.localOrigin[1] );
	msg.WriteDeltaFloat( current.i.position[2], current.localOrigin[2] );
	msg.WriteDeltaFloat( quat.x, localQuat.x );
	msg.WriteDeltaFloat( quat.y, localQuat.y );
	msg.WriteDeltaFloat( quat.z, localQuat.z );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[0], RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[1], RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[2], RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.externalForce[0], RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.externalForce[1], RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.externalForce[2], RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.externalTorque[0], RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.externalTorque[1], RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.externalTorque[2], RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
}

/*
================
anPhysics_RigidBody::ReadFromSnapshot
================
*/
void anPhysics_RigidBody::ReadFromSnapshot( const anBitMsgDelta &msg ) {
	anCQuat quat, localQuat;

	current.atRest = msg.ReadLong();
	current.i.position[0] = msg.ReadFloat();
	current.i.position[1] = msg.ReadFloat();
	current.i.position[2] = msg.ReadFloat();
	quat.x = msg.ReadFloat();
	quat.y = msg.ReadFloat();
	quat.z = msg.ReadFloat();
	current.i.linearMomentum[0] = msg.ReadFloat( RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	current.i.linearMomentum[1] = msg.ReadFloat( RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	current.i.linearMomentum[2] = msg.ReadFloat( RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	current.i.angularMomentum[0] = msg.ReadFloat( RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	current.i.angularMomentum[1] = msg.ReadFloat( RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	current.i.angularMomentum[2] = msg.ReadFloat( RB_MOMENTUM_EXPONENT_BITS, RB_MOMENTUM_MANTISSA_BITS );
	current.localOrigin[0] = msg.ReadDeltaFloat( current.i.position[0] );
	current.localOrigin[1] = msg.ReadDeltaFloat( current.i.position[1] );
	current.localOrigin[2] = msg.ReadDeltaFloat( current.i.position[2] );
	localQuat.x = msg.ReadDeltaFloat( quat.x );
	localQuat.y = msg.ReadDeltaFloat( quat.y );
	localQuat.z = msg.ReadDeltaFloat( quat.z );
	current.pushVelocity[0] = msg.ReadDeltaFloat( 0.0f, RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS );
	current.pushVelocity[1] = msg.ReadDeltaFloat( 0.0f, RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS );
	current.pushVelocity[2] = msg.ReadDeltaFloat( 0.0f, RB_VELOCITY_EXPONENT_BITS, RB_VELOCITY_MANTISSA_BITS );
	current.externalForce[0] = msg.ReadDeltaFloat( 0.0f, RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
	current.externalForce[1] = msg.ReadDeltaFloat( 0.0f, RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
	current.externalForce[2] = msg.ReadDeltaFloat( 0.0f, RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
	current.externalTorque[0] = msg.ReadDeltaFloat( 0.0f, RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
	current.externalTorque[1] = msg.ReadDeltaFloat( 0.0f, RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );
	current.externalTorque[2] = msg.ReadDeltaFloat( 0.0f, RB_FORCE_EXPONENT_BITS, RB_FORCE_MANTISSA_BITS );

	current.i.orientation = quat.ToMat3();
	current.localAxis = localQuat.ToMat3();

	if ( clipModel ) {
		clipModel->Link( self, clipModel->GetId(), current.i.position, current.i.orientation );
	}
}
