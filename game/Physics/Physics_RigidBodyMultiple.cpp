// Copyright (C) 2007 Id Software, Inc.
//


#include "../Lib.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ARC_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Physics_RigidBodyMultiple.h"
#include "Clip.h"
#include "../Player.h"
#include "../../decllib/DeclSurfaceType.h"
#include "../vehicles/Transport.h"
#include "../Game_local.h"
#include "../vehicles/VehicleControl.h"
#include "../ContentMask.h"

#include "../misc/ProfileHelper.h"

const float GRAVITY_STOP_SPEED		= 20.0f;
const float STOP_SPEED				= 0.0f;
const float CONTACT_LCP_EPSILON		= 1e-8f;
const float IMPULSE_WAKE_STRENGTH	= 15000.0f;		// this is the minimum impulse size that will wake this up

//#define RB_TIMINGS

#ifdef RB_TIMINGS
static int lastTimerReset = 0;
static int numRigidBodies = 0;
static arcTimer timer_total, timer_collision, timer_findcontacts, timer_contacts;
#endif

/*
===============================================================

	arcRigidBodyMulti_Body

===============================================================
*/

/*
================
arcRigidBodyMulti_Body::arcRigidBodyMulti_Body
================
*/
arcRigidBodyMulti_Body::arcRigidBodyMulti_Body( void ) {
	clipModel = nullptr;
	centeredClipModel = new anClipModel();

	Init();
}

/*
================
arcRigidBodyMulti_Body::arcRigidBodyMulti_Body
================
*/
arcRigidBodyMulti_Body::~arcRigidBodyMulti_Body( void ) {
	gameLocal.clip.DeleteClipModel( clipModel );
	gameLocal.clip.DeleteClipModel( centeredClipModel );
}

/*
================
arcRigidBodyMulti_Body::DebugDrawMass
================
*/
void arcRigidBodyMulti_Body::DebugDrawMass( void ) {
	if ( clipModel == nullptr ) {
		return;
	}

	gameRenderWorld->DrawText( va( "\n%1.2f", mass ), clipModel->GetOrigin(), 0.08f, colorCyan, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1 );
}

/*
================
arcRigidBodyMulti_Body::Init
================
*/
void arcRigidBodyMulti_Body::Init( void ) {
	centerOfMass.Zero();
	inertiaTensor.Identity();
	localOrigin.Zero();

	contactFriction.Set( 0.001f, 0.05f, 0.05f );
	mass					= 1.0f;
	inverseMass				= 1.0f;
	inverseInertiaTensor	= inertiaTensor.Inverse() * ( 1.0f / 6.0f );
	buoyancy				= 1.0f;
	waterDrag				= 0.0f;
}

/*
================
arcRigidBodyMulti_Body::SetClipModel
================
*/
void arcRigidBodyMulti_Body::SetClipModel( anClipModel* _clipModel, float density, int id, bool freeOld ) {
	if ( freeOld && clipModel != nullptr && ( clipModel != _clipModel ) ) {
		gameLocal.clip.DeleteClipModel( clipModel );
	}
	clipModel = _clipModel;

	if ( clipModel ) {
		clipModel->SetId( id );

		// get mass properties from the trace model
		clipModel->GetMassProperties( density, mass, centerOfMass, inertiaTensor );
		if ( mass < anMath::FLT_EPSILON || FLOAT_IS_NAN( mass ) ) {
			gameLocal.Warning( "arcRigidBodyMulti_Body::SetClipModel: invalid mass" );
			Init();
		}

		inverseMass = 1.0f / mass;
		inverseInertiaTensor = inertiaTensor.Inverse() * ( 1.0f / 6.0f );
	}
}

/*
================
arcRigidBodyMulti_Body::SetMainCenterOfMass
================
*/
void arcRigidBodyMulti_Body::SetMainCenterOfMass( const anVec3& com ) {
	if ( clipModel ) {
		assert( clipModel->GetTraceModel() != nullptr );

		anTraceModel tempTrace = *clipModel->GetTraceModel();
		tempTrace.Translate( localOrigin - com );
		centeredClipModel->LoadTraceModel( tempTrace, false );
	}
}

/*
================
arcRigidBodyMulti_Body::Link
================
*/
void arcRigidBodyMulti_Body::Link( arcEntity* self, const rigidBodyPState_t& current ) {
	if ( !clipModel ) {
		return;
	}

	anVec3 org = current.i.position + ( localOrigin * current.i.orientation );
	clipModel->Link( gameLocal.clip, self, clipModel->GetId(), org, current.i.orientation );
}

/*
================
arcRigidBodyMulti_Body::UnLink
================
*/
void arcRigidBodyMulti_Body::UnLink( void ) {
	if ( clipModel != 0) {
		clipModel->Unlink( gameLocal.clip );
	}
}

/*
================
arcRigidBodyMulti_Body::SetMass
================
*/
void arcRigidBodyMulti_Body::SetMass( float _mass ) {
	assert( _mass > 0.0f );
	inertiaTensor *= _mass / mass;
	inverseInertiaTensor = inertiaTensor.Inverse() * ( 1.0f / 6.0f );
	mass = _mass;
	inverseMass = 1.0f / mass;
}

/*
================
RigidBodyDerivatives_Multi
================
*/
void RigidBodyDerivatives_Multi( const float t, const void *clientData, const float *state, float *derivatives ) {
	const anPhysics_RigidBodyMultiple* p = reinterpret_cast< const anPhysics_RigidBodyMultiple* >( clientData );
	const rigidBodyIState_t *s = reinterpret_cast< const rigidBodyIState_t* >( state );

	struct rigidBodyDerivatives_s {
		anVec3				linearVelocity;
		anMat3				angularMatrix;
		anVec3				force;
		anVec3				torque;
	} *d = ( struct rigidBodyDerivatives_s* ) derivatives;

	anVec3 angularVelocity;
	anMat3 inverseWorldInertiaTensor;

	inverseWorldInertiaTensor = s->orientation * p->GetMainInverseInertiaTensor() * s->orientation.Transpose();
	angularVelocity = inverseWorldInertiaTensor * s->angularMomentum;

	// derivatives
	d->linearVelocity = p->GetMainInverseMass() * s->linearMomentum;
	d->angularMatrix = SkewSymmetric( angularVelocity ) * s->orientation;

	const rigidBodyPState_t& current = p->GetCurrentState();

	float linearFriction = p->InWater() ? p->GetLinearWaterFriction() : p->GetLinearFriction();
	float angularFriction = p->InWater() ? p->GetAngularWaterFriction() : p->GetAngularFriction();

	d->force = -linearFriction * s->linearMomentum + ( current.externalForce );
	d->torque = -angularFriction * s->angularMomentum + ( current.externalTorque );
}

/*
===============================================================

	anPhysics_RigidBodyMultiple

===============================================================
*/

CLASS_DECLARATION( anPhysics_Base, anPhysics_RigidBodyMultiple )
END_CLASS

/*
================
anPhysics_RigidBodyMultiple::anPhysics_RigidBodyMultiple
================
*/
anPhysics_RigidBodyMultiple::anPhysics_RigidBodyMultiple( void ) {
	// set default rigid body properties
	SetBouncyness( 0.6f );
	SetFriction( 0.6f, 0.6f );
	SetWaterRestThreshold( 1.f );

	// initialize state
	current = &state[ 0 ];
	next = &state[ 1 ];

	memset( current, 0, sizeof( *current ) );

	current->atRest = -1;
	current->lastTimeStep = gameLocal.msec;

	current->i.position.Zero();
	current->i.orientation.Identity();

	current->i.linearMomentum.Zero();
	current->i.angularMomentum.Zero();

	*next = *current;
	saved = *current;

	mainMass = 1.0f;
	mainInverseMass = 1.0f;
	mainCenterOfMass.Zero();
	mainInertiaTensor.Identity();
	mainInverseInertiaTensor.Identity();
	customInertiaTensor = false;

	waterLevel = 0.0f;

	activateEndTime = 0;

	// use the least expensive euler integrator
	integrator = new anODE_Euler( sizeof( rigidBodyIState_t ) / sizeof( float ), RigidBodyDerivatives_Multi, this );

	lcp = idLCP::AllocSymmetric();

	masterEntity = nullptr;
	mainClipModel = new anClipModel();
	centeredMainClipModel = new anClipModel();

	flags.noImpact			= false;
	flags.noContact			= false;
	flags.isOrientated		= false;
	flags.useFastPath		= false;
	flags.comeToRest		= true;
	flags.noGravity			= false;
	flags.frozen			= false;

	memset( &lastCollision, 0, sizeof( lastCollision ) );
	lastCollision.trace.fraction = 1.0f;

	blockedTime = 0;
}

/*
================
anPhysics_RigidBodyMultiple::~anPhysics_RigidBodyMultiple
================
*/
anPhysics_RigidBodyMultiple::~anPhysics_RigidBodyMultiple( void ) {
	delete integrator;
	delete lcp;
	gameLocal.clip.DeleteClipModel( mainClipModel );
	gameLocal.clip.DeleteClipModel( centeredMainClipModel );
}

/*
================
anPhysics_RigidBodyMultiple::CalculateMassProperties
================
*/
void anPhysics_RigidBodyMultiple::CalculateMassProperties( void ) {
	int i;

	mainMass = 0;

	anVec3 moments;
	moments.Zero();

	for ( i = 0; i < bodies.Num(); i++ ) {
		arcRigidBodyMulti_Body& body = bodies[i];
		if ( !body.GetClipModel() ) {
			continue;
		}

		mainMass += body.GetMass();
		moments += ( body.GetCenterOfMass() + body.GetOffset() ) * body.GetMass();
	}

	if ( mainMass ) {
		mainInverseMass = 1.f / mainMass;
		mainCenterOfMass = moments * mainInverseMass;
	} else {
		mainInverseMass = 0.f;
		mainCenterOfMass.Zero();
	}

	anMat3 oldMainInertiaTensor = mainInertiaTensor;

	mainInertiaTensor.Zero();
	for ( i = 0; i < bodies.Num(); i++ ) {
		arcRigidBodyMulti_Body& body = bodies[i];

		anMat3 localInertiaTensor = body.GetInertiaTensor();
		localInertiaTensor.InertiaTranslateSelf( body.GetMass(), body.GetCenterOfMass() + body.GetOffset(), mainCenterOfMass - ( body.GetCenterOfMass() + body.GetOffset() ) );
		mainInertiaTensor += localInertiaTensor;

		body.SetMainCenterOfMass( mainCenterOfMass );
	}

	if ( customInertiaTensor ) {
		mainInertiaTensor = oldMainInertiaTensor;
	}

	if ( bodies.Num() ) {
		mainInverseInertiaTensor = mainInertiaTensor.Inverse() * ( 1.f / 6.f );
	} else {
		mainInverseInertiaTensor.Zero();
	}

	anMat3 axis = mat3_identity;

	mainBounds.Clear();
	totalBounds.Clear();
	for ( i = 0; i < bodies.Num(); i++ ) {
		anClipModel* clipModel = bodies[i].GetClipModel();
		if ( !clipModel ) {
			continue;
		}
		anBounds bounds = clipModel->GetBounds();
		bounds.TranslateSelf( bodies[i].GetOffset() );

		totalBounds += bounds;

		if ( bodies[i].GetClipMask() && clipModel->GetContents() != MASK_HURTZONE ) {
			mainBounds += bounds;
		}
	}

	anTraceModel trm( mainBounds );
	if ( !mainBounds.IsCleared() ) {
		mainClipModel->LoadTraceModel( trm, false );

		trm.Translate( -mainCenterOfMass );
		centeredMainClipModel->LoadTraceModel( trm, false );
	}

	mainClipMask = 0;
	for ( i = 0; i < bodies.Num(); i++ ) {
		mainClipMask |= bodies[i].GetClipMask();
	}
}

/*
================
anPhysics_RigidBodyMultiple::SetBodyOffset
================
*/
void anPhysics_RigidBodyMultiple::SetBodyOffset( int id, const anVec3& offset ) {
	assert( id >= 0 && id < bodies.Num() );

	arcRigidBodyMulti_Body& body = bodies[ id ];
	body.SetOffset( offset );
	body.Link( self, *current );
}

/*
================
anPhysics_RigidBodyMultiple::SetBodyBuoyancy
================
*/
void anPhysics_RigidBodyMultiple::SetBodyBuoyancy( int id, float buoyancy ) {
	assert( id >= 0 && id < bodies.Num() );

	arcRigidBodyMulti_Body& body = bodies[ id ];
	body.SetBuoyancy( buoyancy );
}

/*
================
anPhysics_RigidBodyMultiple::SetBodyWaterDrag
================
*/
void anPhysics_RigidBodyMultiple::SetBodyWaterDrag( int id, float drag ) {
	assert( id >= 0 && id < bodies.Num() );

	arcRigidBodyMulti_Body& body = bodies[ id ];
	body.SetWaterDrag( drag );
}

/*
================
anPhysics_RigidBodyMultiple::ClearClipModels
================
*/
void anPhysics_RigidBodyMultiple::ClearClipModels( void ) {
	for ( int i = 0; i < bodies.Num(); i++ ) {
		bodies[i].SetClipModel( nullptr, 1.f, i, true );
	}
	bodies.Clear();
}

/*
================
anPhysics_RigidBodyMultiple::SetClipModel
================
*/
void anPhysics_RigidBodyMultiple::SetClipModel( anClipModel *model, float density, int id, bool freeOld ) {
	assert( self );
	assert( model );					// we need a clip model
	assert( model->IsTraceModel() );	// and it should be a trace model
	assert( density > 0.0f );			// density should be valid
	assert( id >= 0 );

	int max = Max( id + 1, bodies.Num() );
	if ( max > bodies.Num() ) {
		bodies.AssureSize( max );
	}

	arcRigidBodyMulti_Body& body = bodies[ id ];
	body.SetClipModel( model, density, id, freeOld );
	body.Link( self, *current );

	current->i.linearMomentum.Zero();
	current->i.angularMomentum.Zero();

	*next = *current;
}

/*
================
anPhysics_RigidBodyMultiple::GetClipModel
================
*/
anClipModel* anPhysics_RigidBodyMultiple::GetClipModel( int id ) const {
	if ( id < 0 || id >= bodies.Num() ) {
		return nullptr;
	}
	return bodies[ id ].GetClipModel();
}

/*
================
anPhysics_RigidBodyMultiple::GetNumClipModels
================
*/
int anPhysics_RigidBodyMultiple::GetNumClipModels( void ) const {
	return bodies.Num();
}

/*
================
anPhysics_RigidBodyMultiple::SetBouncyness
================
*/
void anPhysics_RigidBodyMultiple::SetBouncyness( const float b ) {
	if ( b < 0.0f || b > 1.0f ) {
		return;
	}
	bouncyness = b;
}

/*
================
anPhysics_RigidBodyMultiple::SetWaterRestThreshold
================
*/
void anPhysics_RigidBodyMultiple::SetWaterRestThreshold( float threshold ) {
	waterRestThreshold = threshold;
}

/*
================
anPhysics_RigidBodyMultiple::SetFriction
================
*/
void anPhysics_RigidBodyMultiple::SetFriction( const float linear, const float angular ) {
	if ( linear < 0.0f || linear > 1.0f || angular < 0.0f || angular > 1.0f ) {
		gameLocal.Warning( "anPhysics_RigidBodyMultiple::SetFriction: friction out of range, linear = %.1f, angular = %.1f", linear, angular );
		return;
	}

	linearFriction		= linear;
	angularFriction		= angular;
}

/*
================
anPhysics_RigidBodyMultiple::SetWaterFriction
================
*/
void anPhysics_RigidBodyMultiple::SetWaterFriction( const float linear, const float angular ) {
	if ( linear < 0.0f || linear > 1.0f || angular < 0.0f || angular > 1.0f ) {
		return;
	}

	linearFrictionWater		= linear;
	angularFrictionWater	= angular;
}

/*
================
anPhysics_RigidBodyMultiple::SetContactFriction
================
*/
void anPhysics_RigidBodyMultiple::SetContactFriction( const int id, const anVec3& contact ) {
	assert( id >= 0 && id < bodies.Num() );
	if ( !( id >= 0 && id < bodies.Num() ) ) {
		return;
	}

	bodies[ id ].SetContactFriction( contact );
}

/*
================
anPhysics_RigidBodyMultiple::Freeze
================
*/
void anPhysics_RigidBodyMultiple::SetFrozen( bool freeze ) {
	flags.frozen = freeze;
}

/*
================
anPhysics_RigidBodyMultiple::Integrate

  Calculate next state from the current state using an integrator.
================
*/
void anPhysics_RigidBodyMultiple::Integrate( float deltaTime ) {
	anVec3 position( current->i.position );
	current->i.position += mainCenterOfMass * current->i.orientation;

	current->i.orientation.TransposeSelf();

	integrator->Evaluate( (float *) &current->i, (float *) &next->i, 0, deltaTime );
	next->i.orientation.OrthoNormalizeSelf();

	if ( !flags.noGravity ) {
		next->i.linearMomentum += deltaTime * gravityVector * mainMass;
	}

	current->i.orientation.TransposeSelf();
	next->i.orientation.TransposeSelf();

	current->i.position = position;
	next->i.position -= mainCenterOfMass * next->i.orientation;

	next->atRest = current->atRest;
}

/*
================
anPhysics_RigidBodyMultiple::CollisionImpulse

  Calculates the collision impulse using the velocity relative to the collision object.
  The current state should be set to the moment of impact.
================
*/
#define DEBOUNCE_FACTOR		1.0f

bool anPhysics_RigidBodyMultiple::CollisionImpulse( const trace_t& collision, anVec3& impulse, anVec3& relativeVelocity, bool noCollisionDamage ) {
	// get info from other entity involved
	impactInfo_t info;
	arcEntity* ent = gameLocal.entities[collision.c.entityNum];
	anPhysics* phys = ent->GetPhysics();
	ent->GetImpactInfo( self, collision.c.id, collision.c.point, &info );


	// gather information
	float	e		= bouncyness;
	anVec3	normal	= collision.c.normal;

	anVec3	v1A		= mainInverseMass * current->i.linearMomentum;
	anMat3	invIA	= current->i.orientation.Transpose() * mainInverseInertiaTensor * current->i.orientation;
	anVec3	w1A		= invIA * current->i.angularMomentum;
	anVec3	rAP		= collision.c.point - ( current->i.position + mainCenterOfMass * current->i.orientation );
	v1A = v1A + w1A.Cross( rAP );

	float	normalVel	= v1A * normal;

	anMat3	invIB	= info.invInertiaTensor;
	anVec3	rBP		= info.position;
	anVec3	v1B		= info.velocity;

	anVec3	v1AB	= v1A - v1B;
	relativeVelocity = v1AB;

	float j = STOP_SPEED;


	if ( normalVel > -STOP_SPEED ) {
		j = -normalVel;
	} else {
		// evaluate
		float	vNorm				= v1AB * normal;

		float	invMassA			= mainInverseMass;
		float	invMassB			= info.invMass;

		float	invMassSum			= invMassA + invMassB;

		anVec3	radiusFactorA		= ( invIA * rAP.Cross( normal ) ).Cross( rAP );
		anVec3	radiusFactorB		= ( invIB * rBP.Cross( normal ) ).Cross( rBP );
		float	radiusFactors		= ( radiusFactorA + radiusFactorB ) * normal;

		float	numerator			= -( 1.0f + e ) * vNorm;
		float	denominator			= invMassSum + radiusFactors;

		j							= numerator / denominator;

		if ( j < -normalVel ) {
			j = -normalVel;
		}
	}

	if ( collision.c.separation >= 0.0f && collision.c.separation < 0.25f ) {
		// squidge impulse bounce scaling
		float separationFactor = collision.c.separation * 4.0f;
		float factor = ( 1.0f - separationFactor*separationFactor ) * DEBOUNCE_FACTOR;
		j *= ( 1.0f + factor );
	}

	impulse = j * normal;

	// if this collision isn't going to result in this body getting pushed,
	// double the impulse and leave it all for the thing we're colliding with

	// update linear and angular momentum with impulse
	if ( self->IsCollisionPushable() || info.invMass == 0.0f || !phys->IsPushable() ) {
		current->i.linearMomentum += impulse;
		current->i.angularMomentum += rAP.Cross(impulse);

		// if no movement at all don't blow up
		if ( collision.fraction < 0.0001f ) {

			float normalMomentum = current->i.linearMomentum * normal;
			if ( normalMomentum < 0.0f ) {
				// only scale the component of linear momentum that is towards the normal!!
				current->i.linearMomentum -= 0.75f * normalMomentum * normal;
				normalMomentum *= 0.25f;
			}
			current->i.angularMomentum *= 0.5f;
		}
	} else {
		impulse *= 2.0f;
	}

	// decide which entity hit which - otherwise it is entity update order dependant :(
	anVec3 rdA = rAP; rdA.NormalizeFast();
	anVec3 rdB = rBP; rdB.NormalizeFast();
	float inVel = rdA * v1AB;
	float AVel = rdA * v1A;
	float BVel = rdB * v1B;
	if ( anMath::Fabs( AVel ) > anMath::Fabs( BVel )  ) {
		// this entity hit the other
		if ( noCollisionDamage ) {
			// HACK: Zero the velocity to make no collision damage be done
			v1A.Zero();
		}

		ent->Hit( collision, v1A, self );
		return self->Collide( collision, v1A, -1 );
	} else {
		// the other entity hit this
		// set up a trace structure so that the collision code handles things happily
		if ( noCollisionDamage ) {
			// HACK: Zero the velocity to make no collision damage be done
			v1B.Zero();
		}

		trace_t fakeCollision = collision;
		fakeCollision.c.normal = -collision.c.normal;
		fakeCollision.c.id = collision.c.selfId;
		fakeCollision.c.selfId = collision.c.id;
		fakeCollision.c.entityNum = self->entityNumber;

		self->Hit( fakeCollision, v1B, ent );
		return ent->Collide( fakeCollision, v1B, -1 );
	}
}

/*
================
anPhysics_RigidBodyMultiple::CheckForCollisions

  Check for collisions between the current and next state.
  If there is a collision the next state is set to the state at the moment of impact.
================
*/
bool anPhysics_RigidBodyMultiple::CheckForCollisions( trace_t &collision ) {
	collision.fraction = 1.0f;

	// don't do anything if the position & orientation aren't changing!
	if ( current->i.position.Compare( next->i.position, anMath::FLT_EPSILON ) ) {
		if ( current->i.orientation.Compare( next->i.orientation, anMath::FLT_EPSILON ) ) {
			return false;
		}
	}

	anMat3 axis;
	anRotation rotation;
	bool collided = false;
	int i;

	if ( flags.useFastPath && !mainBounds.IsCleared() && bodies.Num() > 1 ) {
		if ( !GetTraceCollection().Contents( CLIP_DEBUG_PARMS_ENTINFO( self ) current->i.position, mainClipModel, current->i.orientation, mainClipMask ) ) {
			// calculate the position of the center of mass at current and next
			anVec3 CoMstart = current->i.position + mainCenterOfMass * current->i.orientation;
			anVec3 CoMend = next->i.position + mainCenterOfMass * next->i.orientation;

			TransposeMultiply( current->i.orientation, next->i.orientation, axis );
			rotation = axis.ToRotation();
			rotation.SetOrigin( CoMstart );

			trace_t tr;
			if ( !gameLocal.clip.Motion( CLIP_DEBUG_PARMS_ENTINFO( self ) tr, CoMstart, CoMend, rotation, centeredMainClipModel, current->i.orientation, mainClipMask, self ) ) {

				// no collision for the combined bounds, so we can early out
				return false;
			}
		}
	}

	for ( i = 0; i < bodies.Num(); i++ ) {
		arcRigidBodyMulti_Body& body = bodies[i];
		if ( body.GetClipMask() == 0 || body.GetClipMask() == MASK_HURTZONE ) {
			continue;
		}

		anClipModel* clipModel = body.GetClipModel();
		if ( !clipModel ) {
			continue;
		}

		const anVec3 worldBodyOffset = body.GetOffset() * current->i.orientation;

		anVec3 start = current->i.position + worldBodyOffset;
		anVec3 end = next->i.position + worldBodyOffset;

		trace_t tr;
		if ( GetTraceCollection().Translation( CLIP_DEBUG_PARMS_ENTINFO( self ) tr, start, end, clipModel, current->i.orientation, body.GetClipMask() ) ) {
			next->i.position = tr.endpos - worldBodyOffset;
			next->i.linearMomentum = Lerp( current->i.linearMomentum, next->i.linearMomentum, tr.fraction );
			next->i.angularMomentum = current->i.angularMomentum;
			collided = true;

			// collision is potentially a fraction of a fraction of the full move
			float oldFraction = collision.fraction;
			collision = tr;
			collision.fraction *= oldFraction;
		}
	}

	for ( i = 0; i < bodies.Num(); i++ ) {
		arcRigidBodyMulti_Body& body = bodies[i];
		if ( body.GetClipMask() == 0 || body.GetClipMask() == MASK_HURTZONE ) {
			continue;
		}

		const anClipModel* clipModel = body.GetCenteredClipModel();
		if ( !clipModel ) {
			continue;
		}

		anVec3 end = next->i.position + mainCenterOfMass * current->i.orientation;

		TransposeMultiply( current->i.orientation, next->i.orientation, axis );
		rotation = axis.ToRotation();
		if ( anMath::Fabs( rotation.GetAngle() ) < anMath::FLT_EPSILON ) {
			continue;
		}
		rotation.SetOrigin( next->i.position );

		trace_t tr;
		if ( GetTraceCollection().Rotation( CLIP_DEBUG_PARMS_ENTINFO( self ) tr, end, rotation, clipModel, current->i.orientation, body.GetClipMask() ) ) {
			next->i.orientation = tr.endAxis;
			next->i.linearMomentum = current->i.linearMomentum;
			next->i.angularMomentum = current->i.angularMomentum;
			collided = true;

			// collision is potentially a fraction of a fraction of the full move
			float oldFraction = collision.fraction;
			collision = tr;
			collision.fraction *= oldFraction;
		}
	}

	return collided;
}

/*
================
anPhysics_RigidBodyMultiple::CheckForCollisions_Simple

  Rough version of CheckForCollisions that only uses the bounds for tracing
================
*/
bool anPhysics_RigidBodyMultiple::CheckForCollisions_Simple( trace_t &collision ) {
	collision.fraction = 1.0f;
	// don't do anything if the position & orientation aren't changing!
	if ( current->i.position.Compare( next->i.position, anMath::FLT_EPSILON ) ) {
		if ( current->i.orientation.Compare( next->i.orientation, anMath::FLT_EPSILON ) ) {
			return false;
		}
	}

	if ( !mainBounds.IsCleared() ) {
		int tempClipMask = mainClipMask & ( ~GetVPushClipMask() );

		// calculate the position of the center of mass at current and next
		anVec3 CoMstart = current->i.position + mainCenterOfMass * current->i.orientation;
		anVec3 CoMend = next->i.position + mainCenterOfMass * next->i.orientation;

		anMat3 axis;
		TransposeMultiply( current->i.orientation, next->i.orientation, axis );

		anRotation rotation;
		rotation = axis.ToRotation();
		rotation.SetOrigin( CoMstart );

		trace_t tr;
		if ( gameLocal.clip.Motion( CLIP_DEBUG_PARMS_ENTINFO( self ) tr, CoMstart, CoMend, rotation, centeredMainClipModel, current->i.orientation, tempClipMask, self ) ) {
			next->i.position = tr.endpos - ( mainCenterOfMass * tr.endAxis );
			next->i.orientation = tr.endAxis;
			next->i.linearMomentum = current->i.linearMomentum;
			next->i.angularMomentum = current->i.angularMomentum;

			collision = tr;
			return true;
		}
	}

	return false;
}

/*
================
anPhysics_RigidBodyMultiple::CheckForPlayerCollisions_Simple

  Rough version of CheckForPlayerCollisions that only uses the bounds for tracing
================
*/
bool anPhysics_RigidBodyMultiple::CheckForPlayerCollisions_Simple( float timeDelta, trace_t &collision, bool& noCollisionDamage ) {
	// TODO: Implement me!
	return false;
}

/*
================
anPhysics_RigidBodyMultiple::SetupVPushCollection
================
*/
void anPhysics_RigidBodyMultiple::SetupVPushCollection( void ) {
	GetTraceCollection();

	// find which ones to ignore for various reasons
	vpushCollection.Clear();
	vpushCollection.SetSelf( self );
	anList< const anClipModel* >& collection = traceCollection.GetCollection();
	int vpushClipMask = GetVPushClipMask();

	for ( int i = 0; i < collection.Num(); i++ ) {
		const anClipModel* otherModel = collection[i];
		if ( !( otherModel->GetContents() & vpushClipMask ) ) {
			continue;
		}

		arcEntity* other = otherModel->GetEntity();
		if ( other == nullptr ) {
			continue;
		}
		if ( !other->IsCollisionPushable() ) {
			continue;
		}

		anPhysics* otherPhysics = other->GetPhysics();
		anPhysics_Actor* actorPhysics = otherPhysics->Cast< anPhysics_Actor >();

		if ( actorPhysics == nullptr ) {
			// only actor-derived things may be vpushed
			continue;
		}

		arcNetBasePlayer* playerOther = other->Cast< arcNetBasePlayer >();
		if ( playerOther != nullptr ) {
			// can't run over players that are our passenger, or the leg model of proned players
			if ( playerOther->GetProxyEntity()->Cast< sdTransport >() != nullptr || otherModel->GetId() != 0 ) {
				// don't want these to touch the normal collision path either
				traceCollection.RemoveClipModel( otherModel );
				i--;
				continue;
			}
		}

		// add it to the collection, remove it from the main collection
		vpushCollection.AddClipModel( otherModel );
		traceCollection.RemoveClipModel( otherModel );
		i--;
	}
}

/*
================
anPhysics_RigidBodyMultiple::CheckForPlayerCollisions

  Check for collisions against players only between the current and next state, by
  hurtzone bodies. These then immediately have the callbacks executed so the players
  are caused damage, but no impulse is imparted on player or vehicle.
================
*/
#define RBM_MAX_CLIP_MODELS_COLLECTED	128
bool anPhysics_RigidBodyMultiple::CheckForPlayerCollisions( float timeDelta, trace_t& collision, bool& noCollisionDamage ) {
	// don't do anything if the position & orientation aren't changing!
	if ( current->i.position.Compare( next->i.position, anMath::FLT_EPSILON ) ) {
		if ( current->i.orientation.Compare( next->i.orientation, anMath::FLT_EPSILON ) ) {
			return false;
		}
	}

	int vpushClipMask = GetVPushClipMask();

	anVec3 currentPosition = current->i.position;
	anVec3 nextPosition = next->i.position;
	anMat3 currentOrientation = current->i.orientation;
	anMat3 nextOrientation = next->i.orientation;

	anList< const anClipModel* >& clipModelList = vpushCollection.GetCollection();
	int numClipModels = vpushCollection.GetCollection().Num();

	if ( numClipModels == 0 ) {
		return false;
	}

	bool isSquisher = false;
	sdTransport* transportSelf = self->Cast< sdTransport >();
	sdTransportPositionManager* positionManager = nullptr;
	if ( transportSelf != nullptr ) {
		isSquisher = transportSelf->GetVehicleControl()->IsSquisher();
		positionManager = &transportSelf->GetPositionManager();
	}

	trace_t blockingTrace;
	blockingTrace = collision;
	// treat the final location of the true collision as the full distance travelled
	blockingTrace.fraction = 1.0f;

	anVec3 velocity		= mainInverseMass * current->i.linearMomentum;
	anMat3 invIA		= currentOrientation.Transpose() * mainInverseInertiaTensor * currentOrientation;
	anVec3 angVelocity	= invIA * current->i.angularMomentum;
	anVec3 currentCoM	= currentPosition + mainCenterOfMass * currentOrientation;
	anVec3 nextCoM		= nextPosition + mainCenterOfMass * nextOrientation;
	anVec3 CoMdelta		= nextCoM - currentCoM;

	// check each of our hurtzone bodies against each of the clip models that belong to players
	// (ie, don't care about rigidbodymultiple->rigidbodymultiple collisions)
	for ( int bodyIndex = 0; bodyIndex < bodies.Num(); bodyIndex++ ) {
		arcRigidBodyMulti_Body& body = bodies[ bodyIndex ];

		anClipModel* clipModel = body.GetClipModel();
		if ( clipModel == nullptr ) {
			continue;
		}

		anVec3 start = currentPosition + ( body.GetOffset() * currentOrientation );
		anVec3 end = nextPosition + ( body.GetOffset() * currentOrientation );
		anVec3 delta = end - start;
		anVec3 direction = delta;
		float distance = direction.Normalize();

		for ( int otherIndex = 0; otherIndex < numClipModels; otherIndex++ ) {
			int result = anPhysics_Actor::VPUSH_OK;

			const anClipModel* otherModel = clipModelList[ otherIndex ];
			assert( otherModel );
			arcEntity* other = otherModel->GetEntity();

			// paranoid check a bunch of assumptions
			if ( other == nullptr || !other->IsCollisionPushable() || other->GetPhysics() == nullptr || self == nullptr ) {
				gameLocal.Warning( "anPhysics_RigidBodyMultiple::CheckForPlayerCollisions nullptr entity" );
				assert( false );
				continue;
			}

			arcNetBasePlayer* playerOther = other->Cast< arcNetBasePlayer >();
			// purely PLAYERCLIP bodies only check against players
			if ( body.GetClipMask() == MASK_HURTZONE ) {
				if ( playerOther == nullptr ) {
					continue;
				}
			}

			bool noCollide = false;
			if ( playerOther != nullptr ) {
				if ( gameLocal.time - positionManager->GetPlayerExitTime( playerOther ) < 1000 ) {
 					noCollide = true;
				}
			}
			if ( other->GetPhysics()->IsGroundEntity( self->entityNumber ) ) {
				noCollide = true;
			}

			trace_t tr;
			gameLocal.clip.TranslationModel( CLIP_DEBUG_PARMS_ENTINFO( self ) tr, start, end, clipModel, currentOrientation, vpushClipMask, otherModel,
												otherModel->GetOrigin(), otherModel->GetAxis() );
			if ( tr.fraction < 1.0f ) {
				// do collision damage and stuff
				anVec3 radius = tr.c.point - ( currentCoM + CoMdelta * tr.fraction + mainCenterOfMass * currentOrientation );
				anVec3 hitVelocity = velocity + angVelocity.Cross( radius );

				tr.c.selfId = bodyIndex;
				tr.c.id = 0;

				if ( !noCollide ) {
					other->Hit( tr, hitVelocity, self );
					self->Collide( tr, hitVelocity, -1 );
				}

				if ( body.GetClipMask() != MASK_HURTZONE ) {
					anPhysics* otherPhysics = other->GetPhysics();
					anPhysics_Actor* actorPhysics = otherPhysics->Cast< anPhysics_Actor >();
					assert( actorPhysics != nullptr );

					if ( actorPhysics != nullptr ) {
						if ( tr.fraction > 0.0f  ) {
							// try to push the player
							// increase the remaining delta a bit, just to provide a bit of an epsilon
							float fractionLeft = 1.0f - tr.fraction;
							anVec3 remainingDelta = ( fractionLeft * ( distance + 1.0f ) ) * direction;
							result = actorPhysics->VehiclePush( false, timeDelta * fractionLeft, remainingDelta, clipModel, 0 );
						} else if ( tr.fraction == 0.0f ) {
							// the player is *inside* this clip model
							anVec3 remainingDelta = delta;
							result = actorPhysics->VehiclePush( true, timeDelta, remainingDelta, clipModel, 0 );
						}
					}
				}
			}

			if ( result != anPhysics_Actor::VPUSH_OK ) {
				if ( isSquisher && other->fl.takedamage && ( !playerOther || !playerOther->GetGodMode() ) ) {
					self->CollideFatal( other );
				} else {
					float trueFraction = tr.fraction * blockingTrace.fraction;
					blockingTrace = tr;
					blockingTrace.fraction = trueFraction;
					nextPosition = tr.endpos - ( body.GetOffset() * currentOrientation );

					end = nextPosition + ( body.GetOffset() * currentOrientation );
					delta = end - start;
					direction = delta;
					distance = direction.Normalize();

					noCollisionDamage = noCollide;
				}
			}
		}
	}

	// Now do rotation
	for ( int bodyIndex = 0; bodyIndex < bodies.Num(); bodyIndex++ ) {
		arcRigidBodyMulti_Body& body = bodies[ bodyIndex ];

		anClipModel* clipModel = body.GetCenteredClipModel();
		anClipModel* normalClipModel = body.GetClipModel();
		if ( !clipModel || !normalClipModel ) {
			continue;
		}

		bool doPush = body.GetClipMask() != MASK_HURTZONE;

		anVec3 end = nextPosition + mainCenterOfMass * currentOrientation;

		anMat3 axis;
		TransposeMultiply( currentOrientation, nextOrientation, axis );
		anRotation rotation = axis.ToRotation();
		rotation.SetOrigin( nextPosition );

		for ( int otherIndex = 0; otherIndex < numClipModels; otherIndex++ ) {
			int result = anPhysics_Actor::VPUSH_OK;

			const anClipModel* otherModel = clipModelList[ otherIndex ];
			assert( otherModel );
			arcEntity* other = otherModel->GetEntity();

			// paranoid check a bunch of assumptions
			if ( other == nullptr || !other->IsCollisionPushable() || other->GetPhysics() == nullptr || self == nullptr ) {
				gameLocal.Warning( "anPhysics_RigidBodyMultiple::CheckForPlayerCollisions nullptr entity" );
				assert( false );
				continue;
			}

			arcNetBasePlayer* playerOther = other->Cast< arcNetBasePlayer >();
			// purely PLAYERCLIP bodies only check against players
			if ( body.GetClipMask() == MASK_HURTZONE ) {
				if ( playerOther == nullptr ) {
					continue;
				}
			}

			bool noCollide = false;
			if ( playerOther != nullptr ) {
				if ( gameLocal.time - positionManager->GetPlayerExitTime( playerOther ) < 1000 ) {
					noCollide = true;
				}
			}
			if ( other->GetPhysics()->IsGroundEntity( self->entityNumber ) ) {
				noCollide = true;
			}

			if ( gameLocal.clip.ContentsModel( CLIP_DEBUG_PARMS_ENTINFO( self ) normalClipModel->GetOrigin(), normalClipModel, currentOrientation, -1, otherModel, otherModel->GetOrigin(), otherModel->GetAxis() ) ) {
				// stuck inside it
				anVec3 radius = otherModel->GetOrigin() - ( currentCoM + mainCenterOfMass * currentOrientation );
				anVec3 hitVelocity = velocity + angVelocity.Cross( radius );

				anPhysics* otherPhysics = other->GetPhysics();
				anPhysics_Actor* actorPhysics = otherPhysics->Cast< anPhysics_Actor >();
				assert( actorPhysics != nullptr );

				result = anPhysics_Actor::VPUSH_BLOCKED;
				if ( doPush && actorPhysics != nullptr ) {
					anVec3 delta = hitVelocity * timeDelta;
					// the player is *inside* this clip model
					anVec3 remainingDelta = delta;
					result = actorPhysics->VehiclePush( true, timeDelta, remainingDelta, normalClipModel, 0 );

					// check if its STILL inside
					if ( gameLocal.clip.ContentsModel( CLIP_DEBUG_PARMS_ENTINFO( self ) normalClipModel->GetOrigin(), normalClipModel, currentOrientation, -1, otherModel, otherModel->GetOrigin(), otherModel->GetAxis() ) ) {
						result = anPhysics_Actor::VPUSH_BLOCKED;
					}
				}

				if ( result != anPhysics_Actor::VPUSH_OK ) {
					if ( isSquisher && other->fl.takedamage && ( !playerOther || !playerOther->GetGodMode() ) ) {
						self->CollideFatal( other );
					}
				}
			} else if ( doPush ) {
				if ( other->GetPhysics()->IsGroundClipModel( normalClipModel->GetEntity()->entityNumber, normalClipModel->GetId() ) ) {
					// car-surfing
					// try moving the entity
					if ( body.GetClipMask() != MASK_HURTZONE ) {
						anPhysics* otherPhysics = other->GetPhysics();
						anPhysics_Actor* actorPhysics = otherPhysics->Cast< anPhysics_Actor >();
						assert( actorPhysics != nullptr );

						if ( actorPhysics != nullptr ) {
							anVec3 remainingDelta;
							GetPointVelocity( otherPhysics->GetOrigin(), remainingDelta );
							remainingDelta *= timeDelta;
							result = actorPhysics->VehiclePush( false, timeDelta, remainingDelta, clipModel, 0 );
						}
					}
					if ( result != anPhysics_Actor::VPUSH_OK ) {
						if ( isSquisher && other->fl.takedamage && ( !playerOther || !playerOther->GetGodMode() ) ) {
							self->CollideFatal( other );
						}
					}
				} else {
					trace_t tr;
					gameLocal.clip.Rotation( CLIP_DEBUG_PARMS_ENTINFO( self ) tr, end, rotation, clipModel, currentOrientation, vpushClipMask, self );
					if ( tr.fraction < 1.0f ) {
						// do collision damage and stuff
						anVec3 radius = tr.c.point - ( currentCoM + CoMdelta * tr.fraction + mainCenterOfMass * tr.endAxis );
						anVec3 hitVelocity = velocity + angVelocity.Cross( radius );

						tr.c.selfId = bodyIndex;
						tr.c.id = 0;

						if ( !noCollide ) {
							other->Hit( tr, hitVelocity, self );
							self->Collide( tr, hitVelocity, -1 );
						}

						// pushing
						anPhysics* otherPhysics = other->GetPhysics();
						anPhysics_Actor* actorPhysics = otherPhysics->Cast< anPhysics_Actor >();
						assert( actorPhysics != nullptr );

						if ( actorPhysics != nullptr ) {
							anVec3 delta = hitVelocity * timeDelta;
							if ( tr.fraction > 0.0f  ) {
								// try to push the player
								// increase the remaining delta a bit, just to provide a bit of an epsilon
								anVec3 direction = delta;
								float distance = direction.Normalize();
								float fractionLeft = 1.0f - tr.fraction;
								anVec3 remainingDelta = ( fractionLeft * ( distance + 1.0f ) ) * direction;
								result = actorPhysics->VehiclePush( false, timeDelta * fractionLeft, remainingDelta, normalClipModel, 0 );
							} else if ( tr.fraction == 0.0f ) {
								// the player is *inside* this clip model
								anVec3 remainingDelta = delta;
								result = actorPhysics->VehiclePush( true, timeDelta, remainingDelta, normalClipModel, 0 );
							}
						}
					}

					if ( result != anPhysics_Actor::VPUSH_OK ) {
						if ( isSquisher && other->fl.takedamage && ( !playerOther || !playerOther->GetGodMode() ) ) {
							self->CollideFatal( other );
						} else {
							float trueFraction = tr.fraction * blockingTrace.fraction;
							blockingTrace = tr;
							blockingTrace.fraction = trueFraction;
							nextOrientation = tr.endAxis;

							end = nextPosition + mainCenterOfMass * currentOrientation;
							noCollisionDamage = noCollide;
						}
					}
				}
			}
		}
	}

	if ( blockingTrace.fraction < 1.0f ) {
		float trueFraction = blockingTrace.fraction * collision.fraction;
		collision = blockingTrace;
		collision.fraction = trueFraction;
		next->i.position = nextPosition;
		next->i.orientation = nextOrientation;
		next->i.linearMomentum = current->i.linearMomentum;
		next->i.angularMomentum = current->i.angularMomentum;
		return true;
	}
	return false;
}

/*
================
anPhysics_RigidBodyMultiple::SolveLCPConstraints
================
*/
bool anPhysics_RigidBodyMultiple::SolveLCPConstraints( constraintInfo_t* constraints, int numConstraints, float deltaTime ) const {
	anMatX jmk;
	anVecX rhs, w, lm, lo, hi;
	int* boxIndex;

	assert( constraints && numConstraints );

	jmk.SetData( numConstraints, ( ( numConstraints + 3 ) & ~3 ), MATX_ALLOCA( numConstraints * ( ( numConstraints + 3 ) & ~3 ) ) );

	anMat3 inverseWorldInertiaTensor;

	inverseWorldInertiaTensor = current->i.orientation.Transpose() * mainInverseInertiaTensor * current->i.orientation;

	int i;
	for ( i = 0; i < numConstraints; i++ ) {
		constraintInfo_t& constraint = constraints[i];

		anVec6 t;
		t.SubVec3( 0 ) = mainInverseMass * constraint.j.SubVec3( 0 );
		t.SubVec3( 1 ) = inverseWorldInertiaTensor * constraint.j.SubVec3( 1 );

		float* dstPtr = jmk[i];

		int n;
		for ( n = 0; n < numConstraints; n++ ) {
			constraintInfo_t& otherConstraint = constraints[ n ];

			dstPtr[ n ] = ( t.SubVec3( 0 ) * otherConstraint.j.SubVec3( 0 ) ) + ( t.SubVec3( 1 ) * otherConstraint.j.SubVec3( 1 ) );
		}
	}

	float invDelta = 1.f / deltaTime;

	anVec6 acc;
	acc.SubVec3( 0 ) = ( mainInverseMass * current->i.linearMomentum * invDelta );
	acc.SubVec3( 1 ) = ( ( inverseWorldInertiaTensor * current->i.angularMomentum ) * invDelta );

// HACK: Don't take the external force/torque into account here as it can go funny
//	acc.SubVec3( 0 ) += mainInverseMass * current->externalForce;
//	acc.SubVec3( 1 ) += inverseWorldInertiaTensor * current->externalTorque;;

	rhs.SetData( numConstraints, VECX_ALLOCA( numConstraints ) );
	lo.SetData( numConstraints, VECX_ALLOCA( numConstraints ) );
	hi.SetData( numConstraints, VECX_ALLOCA( numConstraints ) );
	lm.SetData( numConstraints, VECX_ALLOCA( numConstraints ) );
	boxIndex = ( int* ) _alloca16( numConstraints * sizeof( int ) );

	for ( i = 0; i < numConstraints; i++ ) {
		constraintInfo_t& constraint = constraints[i];

		float* ptr = acc.ToFloatPtr();
		float* j1 = constraint.j.ToFloatPtr();

		rhs[i] = j1[ 0 ] * ptr[ 0 ] + j1[ 1 ] * ptr[ 1 ] + j1[ 2 ] * ptr[ 2 ] + j1[ 3 ] * ptr[ 3 ] + j1[ 4 ] * ptr[ 4 ] + j1[ 5 ] * ptr[ 5 ];
		rhs[i] += constraint.c * invDelta;

		rhs[i] = -rhs[i];
		lo[i] = constraint.lo;
		hi[i] = constraint.hi;

		boxIndex[i] = constraint.boxIndex;

		jmk[i][i] += constraint.error * invDelta;
	}

	if ( !lcp->Solve( jmk, lm, rhs, lo, hi, boxIndex ) ) {
		return false;
	}

	for ( i = 0; i < numConstraints; i++ ) {
		constraintInfo_t& constraint = constraints[i];
		constraint.lm = lm[i];
	}

	return true;
}

/*
================
anPhysics_RigidBodyMultiple::ContactFriction
================
*/
void anPhysics_RigidBodyMultiple::ContactFriction( float deltaTime, bool addEntityConstraints ) {
	int i;
	anVec3 massCenter, r;

	massCenter = current->i.position + ( mainCenterOfMass * current->i.orientation );

	constraintList_t constraintInfo;
	constraintInfo.SetNum( MAX_CONSTRAINTS );
	int numConstraints = 0;

	for ( i = 0; i < contacts.Num(); i++ ) {
		int startIndex = numConstraints;

		contactInfo_t& contact = contacts[i];
		contactInfoExt_t& contactExt = contactInfoExt[i];

		constraintInfo_t& info = constraintInfo[ numConstraints++ ];

		r = contact.point - massCenter;

		info.j.SubVec3( 0 ) = contact.normal;
		info.j.SubVec3( 1 ) = r.Cross( contact.normal );
		info.boxIndex		= -1;
		info.lo				= 0;
		info.error			= CONTACT_LCP_EPSILON;
		info.pos			= contact.point;
		if ( contact.selfId >= 0 ) {
			info.hi			= anMath::INFINITY;
			info.c			= 0;
		} else {
			info.hi			= contactExt.contactForceMax;
			info.c			= contactExt.contactForceVelocity;
		}
		info.lm			= 0;

		if ( contact.selfId >= 0 ) {
			arcRigidBodyMulti_Body& body = bodies[ contact.selfId ];
			const anMat3& axes = current->i.orientation;

			int j;
			for ( j = 0; j < 3; j++ ) {
				anVec3 dir1 = axes[ j ];
				float strength = body.GetContactFriction()[ j ];
				if ( contact.surfaceType ) {
					strength *= idGameLocal::GetSurfaceTypeForIndex( contact.surfaceType->Index() ).friction;
				}

				if ( strength > 0.0f ) {
					constraintInfo_t& otherInfo = constraintInfo[ numConstraints++ ];

					otherInfo.hi				= strength;
					otherInfo.lo				= -strength;
					otherInfo.j.SubVec3( 0 )	= dir1;
					otherInfo.j.SubVec3( 1 )	= r.Cross( dir1 );
					otherInfo.error				= CONTACT_LCP_EPSILON;
					otherInfo.boxIndex			= startIndex;
					otherInfo.c					= 0.0f;
					otherInfo.lm				= 0.0f;
					otherInfo.pos				= contact.point;
				}
			}
		} else {
			anMat3 axes = contactExt.frictionAxes * current->i.orientation;

			int j;
			for ( j = 0; j < 3; j++ ) {
				const anVec3& dir1 = axes[ j ];

				float strength = contactExt.contactFriction[ j ];
				if ( contact.surfaceType ) {
					strength *= idGameLocal::GetSurfaceTypeForIndex( contact.surfaceType->Index() ).friction;
				}

				if ( strength > 0.0f ) {
					constraintInfo_t& otherInfo = constraintInfo[ numConstraints++ ];

					otherInfo.hi				= strength;
					otherInfo.lo				= -strength;
					otherInfo.j.SubVec3( 0 )	= dir1;
					otherInfo.j.SubVec3( 1 )	= r.Cross( dir1 );
					otherInfo.error				= CONTACT_LCP_EPSILON;
					otherInfo.boxIndex			= startIndex;
					otherInfo.c					= 0.0f;
					otherInfo.lm				= 0.0f;
					otherInfo.pos				= contact.point;
				}
			}

			if ( contactExt.motorForce ) {
				anVec3 dir1 = contactExt.motorDirection * current->i.orientation;
				dir1 = dir1 - ( ( dir1 * contact.normal ) * contact.normal );

				dir1.Normalize();
				float strength = contactExt.motorForce;

				if ( strength > 0.0f ) {
					constraintInfo_t& otherInfo = constraintInfo[ numConstraints++ ];

					otherInfo.hi				= strength;
					otherInfo.lo				= -strength;
					otherInfo.j.SubVec3( 0 )	= -dir1;
					otherInfo.j.SubVec3( 1 )	= r.Cross( -dir1 );
					otherInfo.error				= CONTACT_LCP_EPSILON;
					otherInfo.boxIndex			= -1;
					otherInfo.c					= contactExt.motorSpeed;
					otherInfo.lm				= 0.0f;
					otherInfo.pos				= contact.point;
				}
			}
		}
	}

	if ( addEntityConstraints ) {
		// add custom constraints
		numConstraints += self->AddCustomConstraints( &constraintInfo[ numConstraints ], MAX_CONSTRAINTS - numConstraints );
	}

	if ( numConstraints == 0 ) {
		return;
	}

	if ( !SolveLCPConstraints( constraintInfo.Begin(), numConstraints, deltaTime ) ) {
		// burp?
		return;
	}

	for ( i = 0; i < numConstraints; i++ ) {
		constraintInfo_t& constraint = constraintInfo[i];

		current->i.linearMomentum += constraint.j.SubVec3( 0 ) * constraint.lm * deltaTime;
		current->i.angularMomentum += constraint.j.SubVec3( 1 ) * constraint.lm * deltaTime;
	}
}

/*
================
anPhysics_RigidBodyMultiple::TestIfAtRest

  Returns true if the body is considered at rest.
  Does not catch all cases where the body is at rest but is generally good enough.
================
*/
bool anPhysics_RigidBodyMultiple::TestIfAtRest( void ) const {
	if ( gameLocal.time < activateEndTime ) {
		return false;
	}

	if ( current->atRest >= 0 ) {
		return true;
	}

	float gv;
	anVec3 normal, point;

	anFixedWinding contactWinding;

	bool inWater = waterLevel > waterRestThreshold;
	bool needsGroundToRest = true;
	sdTransport* transportSelf = self->Cast< sdTransport >();
	if ( transportSelf != nullptr ) {
		const sdVehicleControlBase* control = transportSelf->GetVehicleControl();
		if ( control != nullptr ) {
			needsGroundToRest = control->RestNeedsGround();
		}
	}

	// need at least 3 contact points to come to rest
	if ( !inWater && needsGroundToRest ) {
		if ( contacts.Num() < 3 ) {
			return false;
		}

		// get average contact plane normal
		normal.Zero();
		for ( int i = 0; i < contacts.Num(); i++ ) {
			normal += contacts[i].normal;
		}
		normal.Normalize();

		// if on a too steep surface
		if ( (normal * gravityNormal) > -0.7f ) {
			return false;
		}
	}

	// create bounds for contact points
	contactWinding.Clear();

	// linear velocity of body
	anVec3 v = GetLinearVelocity();

	// linear velocity in gravity direction
	gv = v * gravityNormal;
	// linear velocity orthogonal to gravity direction
	v -= gv * gravityNormal;

	float gs = ( gravityVector * gravityNormal ) * MS2SEC( gameLocal.msec );
	gv -= gs;

	// if too much velocity in gravity direction
	if ( gv > 2.0f * GRAVITY_STOP_SPEED || gv < -2.0f * GRAVITY_STOP_SPEED ) {
		return false;
	}

	float speed = v.Length();

	// if too much velocity orthogonal to gravity direction
	if ( speed > 2.f ) {
		return false;
	}

	// calculate rotational velocity
	float avs = GetAngularVelocity().Length();
	// if too much rotational velocity
	if ( avs > 0.25f ) {
		return false;
	}

	speed = speed + avs;

	bool moving = speed > Square( 0.1 );

	for ( int i = 0; i < contacts.Num(); i++ ) {
		if ( contacts[i].selfId == -1 ) {
			if ( ( moving && !contactInfoExt[i].rested ) || ( contactInfoExt[i].motorForce && contactInfoExt[i].motorSpeed != 0.0f ) ) {
				return false;
			}
		}

		// project point onto plane through origin orthogonal to the gravity
		point = contacts[i].point - (contacts[i].point * gravityNormal) * gravityNormal;
		contactWinding.AddToConvexHull( point, gravityNormal );
	}

	if ( !inWater && needsGroundToRest ) {
		// need at least 3 contact points to come to rest
		if ( contactWinding.GetNumPoints() < 3 ) {
			return false;
		}

		// center of mass in world space
		point = current->i.position + ( mainCenterOfMass * current->i.orientation );
		point -= (point * gravityNormal) * gravityNormal;

		// if the point is not inside the winding
		if ( !contactWinding.PointInside( gravityNormal, point, 0 ) ) {
			return false;
		}
	}

	if ( current->externalForce.LengthSqr() > Square( 10000.f ) ) {
		return false;
	}

	return true;
}

/*
================
anPhysics_RigidBodyMultiple::DebugDraw
================
*/
void anPhysics_RigidBodyMultiple::DebugDraw( void ) {
	for ( int i = 0; i < bodies.Num(); i++ ) {
		arcRigidBodyMulti_Body& body = bodies[i];
		anClipModel* clipModel = body.GetClipModel();
		if ( !clipModel ) {
			continue;
		}

		arcNetBasePlayer* localPlayer = gameLocal.GetLocalPlayer();
		if ( localPlayer ) {
			if ( rb_showBodies.GetBool() || ( rb_showActive.GetBool() && current->atRest < 0 ) ) {
				clipModel->Draw();
			}
		}


		if ( rb_showVelocity.GetBool() ) {
			DrawVelocity( clipModel->GetId(), 0.1f, 4.0f );
		}
	}

	if ( rb_showBodies.GetBool() ) {
		mainClipModel->Draw( current->i.position, current->i.orientation );
	}

	if ( rb_showContacts.GetBool() ) {
		for ( int i = 0; i < contacts.Num(); i++ ) {
			contactInfo_t& contact = contacts[i];

			anVec3 x, y;
			contact.normal.NormalVectors( x, y );
			gameRenderWorld->DebugLine( colorBlue, contact.point, contact.point + 12.0f * contact.normal );
			gameRenderWorld->DebugLine( colorBlue, contact.point - 4.0f * x, contact.point + 4.0f * x );
			gameRenderWorld->DebugLine( colorBlue, contact.point - 4.0f * y, contact.point + 4.0f * y );
			if ( contact.surfaceType ) {
				gameRenderWorld->DrawText( contact.surfaceType->GetName(), contact.point, 0.2f, colorWhite, gameLocal.GetLocalPlayer()->GetRenderView()->viewaxis );
			}
		}
	}

	if ( rb_showMass.GetBool() ) {
		gameRenderWorld->DrawText( va( "\n%1.2f", mainMass ), current->i.position, 0.08f, colorCyan, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1 );

		gameRenderWorld->DebugCircle( colorYellow, current->i.position + ( mainCenterOfMass * current->i.orientation ), anVec3( 0.f, 0.f, 1.f ), 16, 16 );
		gameRenderWorld->DebugCircle( colorYellow, current->i.position + ( mainCenterOfMass * current->i.orientation ), anVec3( 0.f, 1.f, 0.f ), 16, 16 );
		gameRenderWorld->DebugCircle( colorYellow, current->i.position + ( mainCenterOfMass * current->i.orientation ), anVec3( 1.f, 0.f, 0.f ), 16, 16 );

		for ( int i = 0; i < bodies.Num(); i++ ) {
			bodies[i].DebugDrawMass();
		}
	}

	if ( rb_showInertia.GetBool() ) {
		anMat3 &I = mainInertiaTensor;
		gameRenderWorld->DrawText( va( "\n\n\n( %.1f %.1f %.1f )\n( %.1f %.1f %.1f )\n( %.1f %.1f %.1f )",
									I[0].x, I[0].y, I[0].z,
									I[1].x, I[1].y, I[1].z,
									I[2].x, I[2].y, I[2].z ),
									current->i.position, 0.05f, colorCyan, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1 );
	}
}

/*
================
anPhysics_RigidBodyMultiple::SetMass
================
*/
void anPhysics_RigidBodyMultiple::SetMass( float mass, int id ) {
	assert( id >= 0 && id < bodies.Num() );

	if ( id < 0 || id >= bodies.Num() ) {
		return;
	}

	bodies[ id ].SetMass( mass );
}

/*
================
anPhysics_RigidBodyMultiple::GetMass
================
*/
float anPhysics_RigidBodyMultiple::GetMass( int id ) const {
	if ( id < 0 || id >= bodies.Num() ) {
		return mainMass;
	}

	return bodies[ id ].GetMass();
}

/*
================
anPhysics_RigidBodyMultiple::GetInertiaTensor
================
*/
const anMat3& anPhysics_RigidBodyMultiple::GetInertiaTensor( int id ) const {
	if ( id < 0 || id >= bodies.Num() ) {
		return mainInertiaTensor;
	}

	return bodies[ id ].GetInertiaTensor();
}

/*
================
anPhysics_RigidBodyMultiple::SetInertiaTensor
================
*/
void anPhysics_RigidBodyMultiple::SetInertiaTensor( const anMat3& itt ) {
	mainInertiaTensor = itt;
	mainInverseInertiaTensor = mainInertiaTensor.Inverse() * ( 1.f / 6.f );
	customInertiaTensor = true;
}

/*
================
anPhysics_RigidBodyMultiple::Rest
================
*/
void anPhysics_RigidBodyMultiple::Rest( int time ) {
  	current->atRest = time;
	current->i.linearMomentum.Zero();
	current->i.angularMomentum.Zero();
	self->OnPhysicsRested();
}

/*
================
anPhysics_RigidBodyMultiple::PutToRest
================
*/
void anPhysics_RigidBodyMultiple::PutToRest( void ) {
	Rest( gameLocal.time );
}

/*
================
anPhysics_RigidBodyMultiple::NoContact
================
*/
void anPhysics_RigidBodyMultiple::NoContact( void ) {
	flags.noContact = true;
}

/*
================
anPhysics_RigidBodyMultiple::Activate
================
*/
void anPhysics_RigidBodyMultiple::Activate( void ) {
	if ( !IsAtRest() ) {
		return;
	}
	current->atRest = -1;
	activateEndTime = gameLocal.time + SEC2MS( 2.f );
	self->BecomeActive( TH_PHYSICS );
}

/*
================
anPhysics_RigidBodyMultiple::EnableImpact
================
*/
void anPhysics_RigidBodyMultiple::EnableImpact( void ) {
	flags.noImpact = false;
}

/*
================
anPhysics_RigidBodyMultiple::DisableImpact
================
*/
void anPhysics_RigidBodyMultiple::DisableImpact( void ) {
	flags.noImpact = true;
}



































/*
================
anPhysics_RigidBodyMultiple::SetContents
================
*/
void anPhysics_RigidBodyMultiple::SetContents( int contents, int id ) {
	assert( id >=0 && id < bodies.Num() );

	if ( id < 0 || id > bodies.Num() ) {
		return;
	}

	anClipModel* clipModel = bodies[ id ].GetClipModel();
	if ( clipModel ) {
		clipModel->SetContents( contents );
	}
}

/*
================
anPhysics_RigidBodyMultiple::GetContents
================
*/
int anPhysics_RigidBodyMultiple::GetContents( int id ) const {
//	assert( id >=0 && id < bodies.Num() );

	if ( id < 0 || id > bodies.Num() ) {
		return 0;
	}

	anClipModel* clipModel = bodies[ id ].GetClipModel();
	if ( !clipModel ) {
		return 0;
	}

	return clipModel->GetContents();
}

/*
================
anPhysics_RigidBodyMultiple::GetBounds
================
*/
const anBounds& anPhysics_RigidBodyMultiple::GetBounds( int id ) const {
	if ( id >= 0 && id < bodies.Num() ) {
		return bodies[ id ].GetClipModel()->GetBounds();
	}

	return mainBounds;
}

/*
================
anPhysics_RigidBodyMultiple::GetAbsBounds
================
*/
const anBounds &anPhysics_RigidBodyMultiple::GetAbsBounds( int id ) const {
	static anBounds absBounds;

	if ( id >= 0 && id < bodies.Num() ) {
		return bodies[ id ].GetClipModel()->GetAbsBounds();
	}

	absBounds.FromTransformedBounds( totalBounds, current->i.position, current->i.orientation );
	return absBounds;
}

/*
================
anPhysics_RigidBodyMultiple::Evaluate

  Evaluate the impulse based rigid body physics.
  When a collision occurs an impulse is applied at the moment of impact but
  the remaining time after the collision is ignored.
================
*/
bool anPhysics_RigidBodyMultiple::Evaluate( int timeStepMSec, int endTimeMSec ) {

	// eliminate any values that are too small - little bit of a hack, but prevents denormals
	current->i.angularMomentum.FixDenormals();
	current->i.linearMomentum.FixDenormals();
	current->i.orientation.FixDenormals();
	current->i.position.FixDenormals();

	if ( masterEntity != nullptr ) {
		if ( timeStepMSec <= 0 ) {
			return true;
		}

		float timeStep = MS2SEC( timeStepMSec );

		anVec3 masterOrigin;
		anMat3 masterAxis;
		anVec3 oldOrigin = current->i.position;
		anMat3 oldAxis = current->i.orientation;
		self->GetMasterPosition( masterOrigin, masterAxis );
		current->i.position = masterOrigin + localOrigin * masterAxis;
		if ( flags.isOrientated ) {
			current->i.orientation = localAxis * masterAxis;
		} else {
			current->i.orientation = localAxis;
		}

		int i;
		for ( i = 0; i < bodies.Num(); i++ ) {
			bodies[i].Link( self, *current );
		}

		current->i.linearMomentum = mainMass * ( ( current->i.position - oldOrigin ) / timeStep );
		current->i.angularMomentum = mainInertiaTensor * ( ( current->i.orientation * oldAxis.Transpose() ).ToAngularVelocity() / timeStep );
		current->externalForce.Zero();
		current->externalTorque.Zero();
		return true;
	}

	if ( pm_pausePhysics.GetBool() || flags.frozen ) {
		current->externalForce.Zero();
		current->externalTorque.Zero();
		return false;
	}

	trace_t collision;

	float timeStep = MS2SEC( timeStepMSec );
	float fullTimeStep = timeStep;
	float timeStepUsed = timeStep;
	const float minTimeStep = MS2SEC( 1 );

	int count = 0;
	int maxRepetitions = 2;
	if ( !self->IsCollisionPushable() ) {
		maxRepetitions = 5;
	}

	// HACK - increase the maximum number of repetitions for a vehicle that has people in it
	//		  and is not at rest
	if ( current->atRest < 0 ) {
		sdTransport* transportSelf = self->Cast< sdTransport >();
		if ( transportSelf != nullptr ) {
			if ( !transportSelf->GetPositionManager().IsEmpty() ) {
				maxRepetitions = 5;
			}
		}
	}

	// cut back the number of repetitions allowed based on AOR
	if ( self->GetAORPhysicsLOD() == 1 ) {
		maxRepetitions = 2;
	} else if ( self->GetAORPhysicsLOD() >= 2 ) {
		maxRepetitions = 1;
	}

	anVec3 initialPosition = current->i.position;

	// HACK - add back in the velocity from gravity that we took off after our last evaluation
	anVec3 gravityMomentum = mainMass * GetGravity() * timeStep;
	current->i.linearMomentum += gravityMomentum;

	const anClipModel *water = nullptr;
	do {
		anVec3 impulse;
		arcEntity *ent;
		bool collided = false;
		bool cameToRest = false;
		int i;

		current->lastTimeStep = timeStep;

		// if the body is at rest
		if ( current->atRest >= 0 || timeStep <= 0.0f ) {
			DebugDraw();
			self->OnPhysicsRested();

			// HACK - remove the gravity contribution from momentum outside of evaluation
			current->i.linearMomentum -= gravityMomentum;
			return false;
		}

		//
		// update the clip model collection
		//
		SetupVPushCollection();

		const anClipModel * possWater = CheckWater();
		if ( possWater != nullptr ) {
			water = possWater;
		}

#ifdef RB_TIMINGS
		timer_total.Start();
#endif

		if ( !flags.noContact ) {

#ifdef RB_TIMINGS
			timer_findcontacts.Start();
#endif
			// get contacts
			EvaluateContacts( count == 0 );

#ifdef RB_TIMINGS
			timer_findcontacts.Stop();
			timer_contacts.Start();
#endif

			// check if the body has come to rest
			if ( flags.comeToRest && TestIfAtRest() ) {
				// put to rest
				Rest( gameLocal.time );
				cameToRest = true;
			}  else {
				// apply contact friction
				ContactFriction( timeStep, count == 0 );
			}

#ifdef RB_TIMINGS
			timer_contacts.Stop();
#endif // RB_TIMINGS
		}

		if ( current->atRest < 0 ) {
			ActivateContactEntities();
		}

		if ( !cameToRest ) {
			// calculate next position and orientation
			Integrate( timeStep );

	#ifdef RB_TIMINGS
			timer_collision.Start();
	#endif

			// check for collisions from the current to the next state
			int aorLOD = self->GetAORPhysicsLOD();
			if ( aorLOD < 2 ) {
				collided = CheckForCollisions( collision );
			} else if ( aorLOD < 3 ) {
				collided = CheckForCollisions_Simple( collision );
			}

			current->i.linearMomentum -= gravityMomentum;
			// check for collisions with players during this move
			bool noCollisionDamage = false;
			if ( collision.fraction > 0.0f ) {
				if ( aorLOD < 2 ) {
					collided |= CheckForPlayerCollisions( collision.fraction * timeStep, collision, noCollisionDamage );
				} else if ( aorLOD < 3 ) {
					collided |= CheckForPlayerCollisions_Simple( collision.fraction * timeStep, collision, noCollisionDamage );
				}
			}
			current->i.linearMomentum += gravityMomentum;

			timeStepUsed = collision.fraction * timeStep;


	#ifdef RB_TIMINGS
			timer_collision.Stop();
	#endif

			// swap states for next simulation step

			Swap( current, next );

			if ( collided ) {
				// apply collision impulse
				lastCollision.trace = collision;
				lastCollision.time = gameLocal.time;
				if ( CollisionImpulse( collision, impulse, lastCollision.velocity, noCollisionDamage ) ) {
					current->atRest = gameLocal.time;
				}
			}
		}

		// update the position of the clip models
		for ( i = 0; i < bodies.Num(); i++ ) {
			bodies[i].Link( self, *current );
		}

		current->externalForce.Zero();
		current->externalTorque.Zero();

		if ( current->atRest < 0 ) {
			ActivateContactEntities();
		}

		if ( collided ) {
			// if the rigid body didn't come to rest or the other entity is not at rest
			ent = gameLocal.entities[collision.c.entityNum];
			if ( ent && ( !cameToRest || !ent->IsAtRest() ) && ent->IsCollisionPushable() ) {
				// apply impact to other entity
				ent->ApplyImpulse( self, collision.c.id, collision.c.point, -impulse );
			}

			// ensure that the collection will be reset next frame
			traceCollection.ForceNextUpdate();
		}

		current->lastTimeStep = timeStep;

		if ( IsOutsideWorld() ) {
//			gameLocal.Warning( "rigid body moved outside world bounds for entity '%s' type '%s' at (%s)",
//				self->name.c_str(), self->GetType()->classname, current->i.position.ToString(0) );
			Rest( gameLocal.time );
		}

#ifdef RB_TIMINGS
		timer_total.Stop();

		if ( rb_showTimings.GetInteger() == 1 ) {
			gameLocal.Printf( "%12s: t %1.4f cd %1.4f cnt %1.4f\n",
				self->name.c_str(),
				timer_total.Milliseconds(), timer_collision.Milliseconds(), timer_findcontacts.Milliseconds(), timer_contacts.Milliseconds() );
			lastTimerReset = 0;
		}
		else if ( rb_showTimings.GetInteger() == 2 ) {
			numRigidBodies++;
			if ( endTimeMSec > lastTimerReset ) {
				gameLocal.Printf( "rb %d: t %1.4f fc %1.4f cd %1.4f cnt %1.4f\n",
					numRigidBodies,
					timer_total.Milliseconds(), timer_collision.Milliseconds(), timer_findcontacts.Milliseconds(), timer_contacts.Milliseconds() );
			}
		}
		if ( endTimeMSec > lastTimerReset ) {
			lastTimerReset = endTimeMSec;
			numRigidBodies = 0;
			timer_total.Clear();
			timer_collision.Clear();
			timer_contacts.Clear();
			timer_findcontacts.Clear();
		}
#endif

		timeStep -= timeStepUsed;
		count++;
	} while ( count < maxRepetitions && timeStep >= minTimeStep ); //&& collision.fraction > 0.0001f );

	// HACK - remove the gravity contribution from momentum outside of evaluation
	current->i.linearMomentum -= gravityMomentum;

	anVec3 finalPosition = current->i.position;

	if ( !gameLocal.isClient ) {
		// HACK: see if a pusher is blocked somehow
		bool blocked = false;
		if ( !self->IsCollisionPushable() && lastCollision.time == gameLocal.time ) {
			arcEntity* lastCollideEnt = gameLocal.entities[ lastCollision.trace.c.entityNum ];
			if ( lastCollideEnt != nullptr && lastCollideEnt->fl.takedamage ) {
				anVec3 effectiveVelocity = ( finalPosition - initialPosition ) / MS2SEC( gameLocal.msec );
				float desiredSpeed = GetLinearVelocity().LengthSqr();
				float apparentSpeed = effectiveVelocity.LengthSqr();
				if ( desiredSpeed > anMath::FLT_EPSILON ) {
					float speedFraction = apparentSpeed / desiredSpeed;
					if ( speedFraction < 0.01f ) {
						if ( blockedTime > 200 ) {
							self->CollideFatal( lastCollideEnt );
						} else {
							blocked = true;
						}
					}
				}
			}
		}

		if ( blocked ) {
			blockedTime += gameLocal.msec;
		} else {
			blockedTime = 0;
		}
	}

	DebugDraw();

	if ( water ) {
		arcCollisionModel* model = water->GetCollisionModel( 0 );
		int numPlanes = model->GetNumBrushPlanes();
		if ( numPlanes ) {
			self->CheckWater( water->GetOrigin(), water->GetAxis(), model );
		}
	}

	return true;
}

/*
================
anPhysics_RigidBodyMultiple::UpdateTime
================
*/
void anPhysics_RigidBodyMultiple::UpdateTime( int endTimeMSec ) {
}

/*
================
anPhysics_RigidBodyMultiple::GetImpactInfo
================
*/
void anPhysics_RigidBodyMultiple::GetImpactInfo( const int id, const anVec3 &point, impactInfo_t *info ) const {
	anVec3 linearVelocity, angularVelocity;
	anMat3 inverseWorldInertiaTensor;

	linearVelocity = mainInverseMass * current->i.linearMomentum;
	inverseWorldInertiaTensor = current->i.orientation.Transpose() * mainInverseInertiaTensor * current->i.orientation;
	angularVelocity = inverseWorldInertiaTensor * current->i.angularMomentum;

	info->invMass = mainInverseMass;
	info->invInertiaTensor = inverseWorldInertiaTensor;
	info->position = point - ( current->i.position + mainCenterOfMass * current->i.orientation );
	info->velocity = linearVelocity + angularVelocity.Cross( info->position );
}

/*
================
anPhysics_RigidBodyMultiple::ApplyImpulse
================
*/
void anPhysics_RigidBodyMultiple::ApplyImpulse( const int id, const anVec3 &point, const anVec3 &impulse ) {
	if ( flags.noImpact ) {
		return;
	}
	if ( masterEntity ) {
		masterEntity->GetPhysics()->ApplyImpulse( 0, point, impulse );
		return;
	}

	if ( IsAtRest() ) {
		// don't wake up for small impulses (magic numbers FTW)
		if ( impulse.LengthFast() < IMPULSE_WAKE_STRENGTH ) {
			return;
		}
	}

	current->i.linearMomentum += impulse;
	current->i.angularMomentum += ( point - ( current->i.position + mainCenterOfMass * current->i.orientation ) ).Cross( impulse );
	Activate();
}

/*
================
anPhysics_RigidBodyMultiple::AddForce
================
*/
void anPhysics_RigidBodyMultiple::AddForce( const int id, const anVec3 &point, const anVec3 &force ) {
	if ( flags.noImpact ) {
		return;
	}
	if ( masterEntity ) {
		masterEntity->GetPhysics()->AddForce( 0, point, force );
		return;
	}
	current->externalForce += force;

	anVec3 comWorld = current->i.position + ( mainCenterOfMass * current->i.orientation );
	current->externalTorque += ( point - comWorld ).Cross( force );
	Activate();
}

/*
================
anPhysics_RigidBodyMultiple::AddForce
================
*/
void anPhysics_RigidBodyMultiple::AddForce( const anVec3& force ) {
	if ( flags.noImpact ) {
		return;
	}
	if ( masterEntity ) {
		masterEntity->GetPhysics()->AddForce( force );
		return;
	}

	current->externalForce += force;
	Activate();
}

/*
================
anPhysics_RigidBodyMultiple::AddLocalForce
================
*/
void anPhysics_RigidBodyMultiple::AddLocalForce( const int id, const anVec3 &point, const anVec3 &force ) {
	if ( flags.noImpact ) {
		return;
	}
	if ( masterEntity ) {
		masterEntity->GetPhysics()->AddForce( 0, ( point +  mainCenterOfMass ) * current->i.orientation, force );
		return;
	}

	current->externalForce += force*current->i.orientation;
	current->externalTorque += point.Cross( force )*current->i.orientation;
	Activate();
}

/*
================
anPhysics_RigidBodyMultiple::AddTorque
================
*/
void anPhysics_RigidBodyMultiple::AddTorque( const anVec3& torque ) {
	current->externalTorque += torque;
	Activate();
}

/*
================
anPhysics_RigidBodyMultiple::IsAtRest
================
*/
bool anPhysics_RigidBodyMultiple::IsAtRest( void ) const {
	return current->atRest >= 0;
}

/*
================
anPhysics_RigidBodyMultiple::GetRestStartTime
================
*/
int anPhysics_RigidBodyMultiple::GetRestStartTime( void ) const {
	return current->atRest;
}

/*
================
anPhysics_RigidBodyMultiple::Pushable
================
*/
bool anPhysics_RigidBodyMultiple::IsPushable( void ) const {
	return ( !flags.noImpact && !masterEntity );
}

/*
================
anPhysics_RigidBodyMultiple::GetVPushClipMask
================
*/
int anPhysics_RigidBodyMultiple::GetVPushClipMask() const {
	int vpushClipMask = CONTENTS_SLIDEMOVER;
	if ( !self->IsCollisionPushable() ) {
		vpushClipMask |= CONTENTS_MONSTER;
	}

	return vpushClipMask;
}

/*
================
anPhysics_RigidBodyMultiple::EvaluateContacts
================
*/
bool anPhysics_RigidBodyMultiple::EvaluateContacts( bool addEntityContacts ) {
	anVec3 dir;
	int num;

	ClearContacts();

	contacts.SetNum( RBM_MAX_CONTACTS, false );

	dir = current->i.linearMomentum + current->lastTimeStep * gravityVector * mainMass;
	dir.Normalize();

/*	dir = ( gravityVector * mainMass * current->lastTimeStep ); // + v;
	dir.Normalize();
	*/

	int count = 0;

	arcEntity* collisionEnt = nullptr;
	if ( lastCollision.time >= gameLocal.time - gameLocal.msec ) {
		collisionEnt = gameLocal.entities[ lastCollision.trace.c.entityNum ];
	}

	int vpushClipMask = GetVPushClipMask();

	for ( int i = 0; i < bodies.Num() && count < RBM_MAX_CONTACTS; i++ ) {
		arcRigidBodyMulti_Body& body = bodies[i];
		anClipModel* clipModel = body.GetClipModel();

		if ( body.GetClipMask() == MASK_HURTZONE ) {
			continue;
		}

		num = GetTraceCollection().Contacts( CLIP_DEBUG_PARMS_ENTINFO( self ) &contacts[ count ], RBM_MAX_CONTACTS - count, clipModel->GetOrigin(), nullptr, /*CONTACT_EPSILON*/ 1.0f, clipModel, clipModel->GetAxis(), body.GetClipMask() );

		count += num;

		// add contacts from any slidemovers we may be blocked by
		if ( collisionEnt != nullptr ) {
			int contents = collisionEnt->GetPhysics()->GetContents();
			if ( contents & vpushClipMask ) {
				anClipModel* otherClip = collisionEnt->GetPhysics()->GetClipModel();
				if ( otherClip != nullptr ) {
					num = gameLocal.clip.ContactsModel( CLIP_DEBUG_PARMS_ENTINFO( self ) &contacts[ count ], RBM_MAX_CONTACTS - count,
														clipModel->GetOrigin(), &dir, CONTACT_EPSILON, clipModel,
														clipModel->GetAxis(), body.GetClipMask(), otherClip,
														otherClip->GetOrigin(), otherClip->GetAxis() );

					for ( int j = count; j < count + num; j++ ) {
						contacts[ j ].entityNum = collisionEnt->entityNumber;
						contacts[ j ].id = clipModel->GetId();
						contacts[ j ].selfId = i;
					}
					count += num;
				}
			}
		}
	}


	// remove any contacts from entities if we're not to be pushed by them
	// otherwise the LCP solver will push us away from the entity a bit, slowing us down :(
	if ( !self->IsCollisionPushable() ) {
		for ( int i = 0; i < count; i++ ) {
			arcEntity* ent = gameLocal.entities[ contacts[i].entityNum ];
			if ( ent->GetPhysics()->IsPushable() && ent->IsCollisionPushable() ) {
				contacts.RemoveIndex( i );
				i--;
				count--;
			}
		}
	}

	if ( !flags.noGravity && count < RBM_MAX_CONTACTS && addEntityContacts ) {
		count += self->EvaluateContacts( &contacts[ count ], &contactInfoExt[ count ], RBM_MAX_CONTACTS - count );
	}

	contacts.SetNum( count, false );

	AddContactEntitiesForContacts();

	return contacts.Num() != 0;
}

/*
================
anPhysics_RigidBodyMultiple::SaveState
================
*/
void anPhysics_RigidBodyMultiple::SaveState( void ) {
	saved = *current;
}

/*
================
anPhysics_RigidBodyMultiple::RestoreState
================
*/
void anPhysics_RigidBodyMultiple::RestoreState( void ) {
	*current = saved;

	int i;
	for ( i = 0; i < bodies.Num(); i++ ) {
		bodies[i].Link( self, *current );
	}

	EvaluateContacts( true );
}

/*
================
anPhysics_RigidBodyMultiple::SetOrigin
================
*/
void anPhysics_RigidBodyMultiple::SetOrigin( const anVec3 &newOrigin, int id ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	if ( masterEntity != nullptr ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current->i.position = masterOrigin + newOrigin * masterAxis;
		localOrigin = newOrigin;
	} else {
		current->i.position = newOrigin;
	}

	int i;
	for ( i = 0; i < bodies.Num(); i++ ) {
		bodies[i].Link( self, *current );
	}

	Activate();
}

/*
================
anPhysics_RigidBodyMultiple::SetAxis
================
*/
void anPhysics_RigidBodyMultiple::SetAxis( const anMat3 &newAxis, int id ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	if ( masterEntity != nullptr ) {
		if ( flags.isOrientated ) {
			self->GetMasterPosition( masterOrigin, masterAxis );
			current->i.orientation = newAxis * masterAxis;
		}
		localAxis = newAxis;
	}
	else {
		current->i.orientation = newAxis;
	}

	int i;
	for ( i = 0; i < bodies.Num(); i++ ) {
		bodies[i].Link( self, *current );
	}

	Activate();
}

/*
================
anPhysics_RigidBodyMultiple::Move
================
*/
void anPhysics_RigidBodyMultiple::Translate( const anVec3 &translation, int id ) {

	if ( masterEntity != nullptr ) {
		localOrigin += translation;
	}
	current->i.position += translation;

	int i;
	for ( i = 0; i < bodies.Num(); i++ ) {
		bodies[i].Link( self, *current );
	}

	Activate();
}

/*
================
anPhysics_RigidBodyMultiple::Rotate
================
*/
void anPhysics_RigidBodyMultiple::Rotate( const anRotation &rotation, int id ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	current->i.orientation *= rotation.ToMat3();
	current->i.position *= rotation;

	if ( masterEntity != nullptr ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		localAxis *= rotation.ToMat3();
		localOrigin = ( current->i.position - masterOrigin ) * masterAxis.Transpose();
	}

	int i;
	for ( i = 0; i < bodies.Num(); i++ ) {
		bodies[i].Link( self, *current );
	}

	Activate();
}

/*
================
anPhysics_RigidBodyMultiple::GetBodyOffset
================
*/
const anVec3& anPhysics_RigidBodyMultiple::GetBodyOffset( int id ) const {
	assert( id >= 0 && id < bodies.Num() );

	if ( id < 0 || id >= bodies.Num() ) {
		return vec3_origin;
	}

	return bodies[ id ].GetOffset();
}

/*
================
anPhysics_RigidBodyMultiple::GetOrigin
================
*/
void anPhysics_RigidBodyMultiple::GetBodyOrigin( anVec3& org, int id ) const {
	assert( id >= 0 && id < bodies.Num() );

	if ( id < 0 || id >= bodies.Num() ) {
		org.Zero();
		return;
	}

	org = current->i.position + ( bodies[ id ].GetOffset() * current->i.orientation );
}

/*
================
anPhysics_RigidBodyMultiple::GetOrigin
================
*/
const anVec3& anPhysics_RigidBodyMultiple::GetOrigin( int id ) const {
	return current->i.position;
}

/*
================
anPhysics_RigidBodyMultiple::GetAxis
================
*/
const anMat3& anPhysics_RigidBodyMultiple::GetAxis( int id ) const {
	return current->i.orientation;
}

/*
================
anPhysics_RigidBodyMultiple::SetLinearVelocity
================
*/
void anPhysics_RigidBodyMultiple::SetLinearVelocity( const anVec3 &newLinearVelocity, int id ) {
	current->i.linearMomentum = newLinearVelocity * mainMass;
	Activate();
}

/*
================
anPhysics_RigidBodyMultiple::SetAngularVelocity
================
*/
void anPhysics_RigidBodyMultiple::SetAngularVelocity( const anVec3 &newAngularVelocity, int id ) {
	anMat3 worldInertiaTensor = 6.0f * ( current->i.orientation.Transpose() * mainInertiaTensor * current->i.orientation );
	current->i.angularMomentum = worldInertiaTensor * newAngularVelocity;
	Activate();
}

/*
================
anPhysics_RigidBodyMultiple::GetLinearVelocity
================
*/
const anVec3& anPhysics_RigidBodyMultiple::GetLinearVelocity( int id ) const {
	static anVec3 curLinearVelocity;
	curLinearVelocity = current->i.linearMomentum * mainInverseMass;
	return curLinearVelocity;
}

/*
================
anPhysics_RigidBodyMultiple::GetPointVelocity
================
*/
const anVec3& anPhysics_RigidBodyMultiple::GetPointVelocity( const anVec3& point, anVec3& velocity ) const {
	anVec3 comWorld = current->i.position + ( mainCenterOfMass * current->i.orientation );
	velocity = ( current->i.linearMomentum * mainInverseMass );
	velocity += GetAngularVelocity().Cross( point - comWorld );
	return velocity;
}

/*
================
anPhysics_RigidBodyMultiple::GetAngularVelocity
================
*/
const anVec3& anPhysics_RigidBodyMultiple::GetAngularVelocity( int id ) const {
	static anVec3 curAngularVelocity;
	anMat3 inverseWorldInertiaTensor;

	inverseWorldInertiaTensor = current->i.orientation.Transpose() * mainInverseInertiaTensor * current->i.orientation;
	curAngularVelocity = inverseWorldInertiaTensor * current->i.angularMomentum;
	return curAngularVelocity;
}

/*
================
anPhysics_RigidBodyMultiple::ClipTranslation
================
*/
void anPhysics_RigidBodyMultiple::ClipTranslation( trace_t &results, const anVec3 &translation, const anClipModel *model ) const {
	int i;
	trace_t tr;

	memset( &results, 0, sizeof( results ) );
	results.fraction = 1.f;

	for ( i = 0; i < bodies.Num(); i++ ) {
		anClipModel* clipModel = bodies[i].GetClipModel();
		if ( !clipModel ) {
			continue;
		}

		if ( model ) {
			gameLocal.clip.TranslationModel( CLIP_DEBUG_PARMS_ENTINFO( self ) tr, clipModel->GetOrigin(), clipModel->GetOrigin() + translation, clipModel, clipModel->GetAxis(), bodies[i].GetClipMask(), model, model->GetOrigin(), model->GetAxis() );
		} else {
			gameLocal.clip.Translation( CLIP_DEBUG_PARMS_ENTINFO( self ) tr, clipModel->GetOrigin(), clipModel->GetOrigin() + translation, clipModel, clipModel->GetAxis(), bodies[i].GetClipMask(), self );
		}

		if ( tr.fraction < results.fraction ) {
			results = tr;
		}

	}

	results.endpos = current->i.position + results.fraction * translation;
	results.endAxis = current->i.orientation;
}

/*
================
anPhysics_RigidBodyMultiple::ClipRotation
================
*/
void anPhysics_RigidBodyMultiple::ClipRotation( trace_t &results, const anRotation &rotation, const anClipModel *model ) const {
	int i;
	trace_t tr;
	anRotation partialRotation;

	memset( &results, 0, sizeof( results ) );
	results.fraction = 1.f;

	for ( i = 0; i < bodies.Num(); i++ ) {
		anClipModel* clipModel = bodies[i].GetClipModel();
		if ( !clipModel ) {
			continue;
		}

		if ( model ) {
			gameLocal.clip.RotationModel( CLIP_DEBUG_PARMS_ENTINFO( self ) tr, clipModel->GetOrigin(), rotation, clipModel, clipModel->GetAxis(), bodies[i].GetClipMask(), model, model->GetOrigin(), model->GetAxis() );
		} else {
			gameLocal.clip.Rotation( CLIP_DEBUG_PARMS_ENTINFO( self ) tr, clipModel->GetOrigin(), rotation, clipModel, clipModel->GetAxis(), bodies[i].GetClipMask(), self );
		}

		if ( tr.fraction < results.fraction ) {
			results = tr;
		}
	}

	partialRotation = rotation * results.fraction;
	results.endpos = current->i.position * partialRotation;
	results.endAxis = current->i.orientation * partialRotation.ToMat3();
}

/*
================
anPhysics_RigidBodyMultiple::ClipContents
================
*/
int anPhysics_RigidBodyMultiple::ClipContents( const anClipModel *model ) const {
	int contents = 0;
	int i;

	for ( i = 0; i < bodies.Num(); i++ ) {
		anClipModel* clipModel = bodies[i].GetClipModel();
		if ( !clipModel ) {
			continue;
		}

		if ( model ) {
			contents |= gameLocal.clip.ContentsModel( CLIP_DEBUG_PARMS_ENTINFO( self ) clipModel->GetOrigin(), clipModel, clipModel->GetAxis(), -1, model, model->GetOrigin(), model->GetAxis() );
		} else {
			contents |= gameLocal.clip.Contents( CLIP_DEBUG_PARMS_ENTINFO( self ) clipModel->GetOrigin(), clipModel, clipModel->GetAxis(), -1, nullptr );
		}
	}

	return contents;
}

/*
================
anPhysics_RigidBodyMultiple::SetMaster
================
*/
void anPhysics_RigidBodyMultiple::SetMaster( arcEntity *master, const bool orientated ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	if ( master ) {
		if ( !masterEntity ) {
			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			localOrigin = ( current->i.position - masterOrigin ) * masterAxis.Transpose();
			if ( orientated ) {
				localAxis = current->i.orientation * masterAxis.Transpose();
			} else {
				localAxis = current->i.orientation;
			}
			masterEntity = master;
			flags.isOrientated = orientated;
			ClearContacts();
		}
	} else {
		localOrigin = vec3_origin;
		localAxis = mat3_identity;

		if ( masterEntity ) {
			masterEntity = nullptr;
			Activate();
		}
	}
}

/*
================
anPhysics_RigidBodyMultiple::SetPushed
================
*/
void anPhysics_RigidBodyMultiple::SetPushed( int deltaTime ) {
	anRotation rotation;

	rotation = ( saved.i.orientation * current->i.orientation ).ToRotation();

	// velocity with which the af is pushed
	current->pushVelocity.SubVec3(0) += ( current->i.position - saved.i.position ) / ( MS2SEC( deltaTime ) );
	current->pushVelocity.SubVec3( 1 ) += rotation.GetVec() * -DEG2RAD( rotation.GetAngle() ) / ( MS2SEC( deltaTime ) );
	next->pushVelocity = current->pushVelocity;
}

/*
================
anPhysics_RigidBodyMultiple::GetPushedLinearVelocity
================
*/
const anVec3 &anPhysics_RigidBodyMultiple::GetPushedLinearVelocity( const int id ) const {
	return current->pushVelocity.SubVec3(0);
}

/*
================
anPhysics_RigidBodyMultiple::GetPushedAngularVelocity
================
*/
const anVec3 &anPhysics_RigidBodyMultiple::GetPushedAngularVelocity( const int id ) const {
	return current->pushVelocity.SubVec3( 1 );
}

/*
================
anPhysics_RigidBodyMultiple::SetClipMask
================
*/
void anPhysics_RigidBodyMultiple::SetClipMask( int mask, int id ) {
	if ( id >= 0 && id < bodies.Num() ) {
		bodies[ id ].SetClipMask( mask );
	} else {
		int i;
		for ( i = 0; i < bodies.Num(); i++ ) {
			bodies[i].SetClipMask( mask );
		}
	}

	mainClipMask = 0;

	int i;
	for ( i = 0; i < bodies.Num(); i++ ) {
		mainClipMask |= bodies[i].GetClipMask();
	}
}

/*
================
anPhysics_RigidBodyMultiple::GetClipMask
================
*/
int anPhysics_RigidBodyMultiple::GetClipMask( int id ) const {
	int contents = 0;

	if ( id >= 0 && id < bodies.Num() ) {
		contents = bodies[ id ].GetClipMask();
	} else {
		int i;
		for ( i = 0; i < bodies.Num(); i++ ) {
			contents |= bodies[i].GetClipMask();
		}
	}

	return contents;
}

/*
================
anPhysics_RigidBodyMultiple::GetBodyContacts
================
*/
int anPhysics_RigidBodyMultiple::GetBodyContacts( const int id, const contactInfo_t** _contacts, int maxContacts ) const {
	int i;
	int num = 0;

	assert( maxContacts >= 1 );
	if ( !( maxContacts >= 1 ) ) {
		return 0;
	}

	for ( i = 0; i < contacts.Num(); i++ ) {
		if ( id == -1 || contacts[i].selfId == id ) {
			_contacts[ num ] = &contacts[i];
			num++;
			if ( num >= maxContacts ) {
				return num;
			}
		}
	}

	return num;
}

/*
================
anPhysics_RigidBodyMultiple::GetBodyGroundContacts
================
*/
int anPhysics_RigidBodyMultiple::GetBodyGroundContacts( const int id, const contactInfo_t** _contacts, int maxContacts ) const {
	int i;
	int num = 0;

	assert( maxContacts >= 1 );
	if ( !( maxContacts >= 1 ) ) {
		return 0;
	}

	for ( i = 0; i < contacts.Num(); i++ ) {
		if ( id == -1 || contacts[i].selfId == id ) {
			if ( contacts[i].normal * -gravityNormal <= 0.0f ) {
				continue;
			}

			_contacts[ num ] = &contacts[i];
			num++;
			if ( num >= maxContacts ) {
				return num;
			}
		}
	}

	return num;
}

/*
================
anPhysics_RigidBodyMultiple::ApplyImpulse
================
*/
void anPhysics_RigidBodyMultiple::ApplyImpulse( const anVec3& linearImpulse, const anVec3& angularImpulse ) {
	current->i.linearMomentum += linearImpulse;
	current->i.angularMomentum += angularImpulse;
}

/*
================
anPhysics_RigidBodyMultiple::UnlinkClip
================
*/
void anPhysics_RigidBodyMultiple::UnlinkClip( void ) {
	int i;
	for ( i = 0; i < bodies.Num(); i++ ) {
		bodies[i].UnLink();
	}
}

/*
================
anPhysics_RigidBodyMultiple::LinkClip
================
*/
void anPhysics_RigidBodyMultiple::LinkClip( void ) {
	int i;
	for ( i = 0; i < bodies.Num(); i++ ) {
		bodies[i].Link( self, *current );
	}
}

/*
================
anPhysics_RigidBodyMultiple::EnableClip
================
*/
void anPhysics_RigidBodyMultiple::EnableClip( void ) {
	int i;
	for ( i = 0; i < bodies.Num(); i++ ) {
		bodies[i].GetClipModel()->Enable();
	}
}

/*
================
anPhysics_RigidBodyMultiple::DisableClip
================
*/
void anPhysics_RigidBodyMultiple::DisableClip( bool activateContacting ) {
	int i;
	for ( i = 0; i < bodies.Num(); i++ ) {
		if ( activateContacting ) {
			WakeEntitiesContacting( self, bodies[i].GetClipModel() );
		}
		bodies[i].GetClipModel()->Disable();
	}
}

/*
================
anPhysics_RigidBodyMultiple::CheckWater
================
*/

const anClipModel* anPhysics_RigidBodyMultiple::CheckWater( void ) {
	waterLevel = 0.0f;

	anBounds absBounds;
	absBounds.FromTransformedBounds( totalBounds, current->i.position, current->i.orientation );

	const anClipModel* clipModel;
	int count = gameLocal.clip.ClipModelsTouchingBounds( CLIP_DEBUG_PARMS_ENTINFO( self ) absBounds, CONTENTS_WATER, &clipModel, 1, nullptr );
	if ( !count ) {
		return nullptr;
	}

	if ( !clipModel->GetNumCollisionModels() ) {
		return nullptr;
	}

	anVec3 waterCurrent;
	clipModel->GetEntity()->GetWaterCurrent( waterCurrent );

	arcCollisionModel* model = clipModel->GetCollisionModel( 0 );
	int numPlanes = model->GetNumBrushPlanes();
	if ( !numPlanes ) {
		return nullptr;
	}
	const anBounds& modelBounds = model->GetBounds();

	//self->CheckWater( clipModel->GetOrigin(), clipModel->GetAxis(), model );

	{

		float scratch[ MAX_TRACEMODEL_WATER_POINTS ];
		float velocityScratch[ MAX_TRACEMODEL_WATER_POINTS ];
		float waterVolume = 0.0f;
		float totalVolume = 0.0f;
		anVec3 comWorld = mainCenterOfMass * current->i.orientation + current->i.position;

		for ( int j = 0; j < bodies.Num(); j++ ) {
			bool doBuoyancy = bodies[ j ].GetBuoyancy() > 0.0f;
			bool doDrag = bodies[ j ].GetWaterDrag() > 0.0f;
			if ( !doBuoyancy && !doDrag ) {
				continue;
			}

			anClipModel* bodyClip = bodies[ j ].GetClipModel();
			const traceModelWater_t* waterPoints = bodyClip->GetWaterPoints();
			float volume = bodyClip->GetTraceModelVolume();
			totalVolume += volume;

			const anMat3 transpose = bodyClip->GetAxis().Transpose();
			const anVec3 localAngularVelocity = GetAngularVelocity() * transpose;
			const anVec3 localLinearVelocity = GetLinearVelocity() * transpose;
			const anVec3 localGravity = gravityNormal * transpose;
			const anVec3 localCenterOfMass = ( comWorld - bodyClip->GetOrigin() ) * transpose;

			float bodyDrag = bodies[ j ].GetWaterDrag();

			for ( int i = 0; i < MAX_TRACEMODEL_WATER_POINTS; i++ ) {
				scratch[i] = waterPoints[i].weight * volume;
			}

			for ( int l = 0; l < numPlanes; l++ ) {
				anPlane plane = model->GetBrushPlane( l );
				plane.TranslateSelf( clipModel->GetOrigin() - bodyClip->GetOrigin() );
				plane.Normal() *= clipModel->GetAxis();
				plane.Normal() *= transpose;

				for ( int i = 0; i < MAX_TRACEMODEL_WATER_POINTS; i++ ) {
					if ( plane.Distance( waterPoints[i].xyz ) > 0.0f ) {
						scratch[i] = 0.0f;
						velocityScratch[i] = 0.0f;
					} else if ( doDrag ) {
						// this is calculated here as it is fairly expensive
						velocityScratch[i] = localGravity * ( localAngularVelocity.Cross( waterPoints[i].xyz - localCenterOfMass ) + localLinearVelocity );
					}
				}
			}

			float height = clipModel->GetOrigin().z - bodyClip->GetOrigin().z + modelBounds.GetMaxs().z;
			anPlane plane( transpose[ 2 ], height );

			for ( int i = 0; i < MAX_TRACEMODEL_WATER_POINTS; i++ ) {
				if ( scratch[i] == 0.0f || velocityScratch[i] == 0.0f ) {
					continue;
				}

				anVec3 org = bodyClip->GetOrigin() + ( bodyClip->GetAxis() * waterPoints[i].xyz );
				anVec3 mainSpaceOrg = org  - (current->i.position + mainCenterOfMass * current->i.orientation );

				if ( doBuoyancy ) {
					scratch[i] *= Min( -plane.Distance( waterPoints[i].xyz ) / 16.0f, 1.0f );

					anVec3 impulse = scratch[i] * ( ( -gravityNormal * bodies[ j ].GetBuoyancy() ) + ( waterCurrent * mainInverseMass ) );

					current->i.linearMomentum += impulse;
					current->i.angularMomentum += ( mainSpaceOrg ).Cross( impulse );

					waterVolume += scratch[i];
				}

				if ( doDrag ) {
					float drag = -waterPoints[i].weight * velocityScratch[i];
					anVec3 impulse = bodyDrag * drag * gravityNormal * mainMass;

					current->i.linearMomentum += impulse;
					current->i.angularMomentum += ( mainSpaceOrg ).Cross( impulse );
				}
			}
		}

		if ( totalVolume > anMath::FLT_EPSILON ) {
			waterLevel = waterVolume / totalVolume;
		}
	}

	return clipModel;
}

const float	RB_ORIGIN_MAX				= 32767;
const int	RB_ORIGIN_TOTAL_BITS		= 24;
const int	RB_ORIGIN_EXPONENT_BITS		= anMath::BitsForInteger( anMath::BitsForFloat( RB_ORIGIN_MAX ) ) + 1;
const int	RB_ORIGIN_MANTISSA_BITS		= RB_ORIGIN_TOTAL_BITS - 1 - RB_ORIGIN_EXPONENT_BITS;

const float	RB_LINEAR_VELOCITY_MIN				= 0.05f;
const float	RB_LINEAR_VELOCITY_MAX				= 8192.0f;
const int	RB_LINEAR_VELOCITY_TOTAL_BITS		= 20;
const int	RB_LINEAR_VELOCITY_EXPONENT_BITS	= anMath::BitsForInteger( anMath::BitsForFloat( RB_LINEAR_VELOCITY_MAX, RB_LINEAR_VELOCITY_MIN ) ) + 1;
const int	RB_LINEAR_VELOCITY_MANTISSA_BITS	= RB_LINEAR_VELOCITY_TOTAL_BITS - 1 - RB_LINEAR_VELOCITY_EXPONENT_BITS;

const float	RB_ANGULAR_VELOCITY_MIN				= 0.00001f;
const float	RB_ANGULAR_VELOCITY_MAX				= anMath::PI * 8.0f;
const int	RB_ANGULAR_VELOCITY_TOTAL_BITS		= 20;
const int	RB_ANGULAR_VELOCITY_EXPONENT_BITS	= anMath::BitsForInteger( anMath::BitsForFloat( RB_ANGULAR_VELOCITY_MAX, RB_ANGULAR_VELOCITY_MIN ) ) + 1;
const int	RB_ANGULAR_VELOCITY_MANTISSA_BITS	= RB_ANGULAR_VELOCITY_TOTAL_BITS - 1 - RB_ANGULAR_VELOCITY_EXPONENT_BITS;

/*
================
anPhysics_RigidBodyMultiple::CheckNetworkStateChanges
================
*/
bool anPhysics_RigidBodyMultiple::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_BASE( sdRBMultipleNetworkState );

		anVec3 angularVelocity = GetAngularVelocity();
		anVec3 linearVelocity = GetLinearVelocity();

		NET_CHECK_FIELD( angularVelocity, angularVelocity );
		NET_CHECK_FIELD( linearVelocity, linearVelocity );
		NET_CHECK_FIELD( orientation, current->i.orientation.ToCQuat() );
		NET_CHECK_FIELD( origin, current->i.position );

		return false;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_BASE( sdRBMultipleBroadcastState );

		NET_CHECK_FIELD( localOrigin, localOrigin );
		NET_CHECK_FIELD( localOrientation, localAxis.ToCQuat() );
		NET_CHECK_FIELD( atRest, current->atRest );

		return false;
	}

	return false;
}

/*
================
anPhysics_RigidBodyMultiple::WriteNetworkState
================
*/
void anPhysics_RigidBodyMultiple::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, anBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdRBMultipleNetworkState );

		newData.origin			= current->i.position;
		newData.orientation		= current->i.orientation.ToCQuat();
		newData.angularVelocity	= GetAngularVelocity();
		newData.linearVelocity	= GetLinearVelocity();

		msg.WriteDeltaVector( baseData.origin, newData.origin, RB_ORIGIN_EXPONENT_BITS, RB_ORIGIN_MANTISSA_BITS );
		msg.WriteDeltaCQuat( baseData.orientation, newData.orientation );
		msg.WriteDeltaVector( baseData.angularVelocity, newData.angularVelocity, RB_ANGULAR_VELOCITY_EXPONENT_BITS, RB_ANGULAR_VELOCITY_MANTISSA_BITS );
		msg.WriteDeltaVector( baseData.linearVelocity, newData.linearVelocity, RB_LINEAR_VELOCITY_EXPONENT_BITS, RB_LINEAR_VELOCITY_MANTISSA_BITS );

		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdRBMultipleBroadcastState );

		newData.localOrigin			= localOrigin;
		newData.localOrientation	= localAxis.ToCQuat();
		newData.atRest				= current->atRest;

		msg.WriteDeltaVector( baseData.localOrigin, newData.localOrigin, RB_ORIGIN_EXPONENT_BITS, RB_ORIGIN_MANTISSA_BITS );
		msg.WriteDeltaCQuat( baseData.localOrientation, newData.localOrientation );
		msg.WriteDeltaLong( baseData.atRest, newData.atRest );

		return;
	}
}

/*
================
anPhysics_RigidBodyMultiple::ApplyNetworkState
================
*/
void anPhysics_RigidBodyMultiple::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	traceCollection.ForceNextUpdate();
	if ( mode == NSM_VISIBLE ) {
		NET_GET_NEW( sdRBMultipleNetworkState );

		current->i.position			= newData.origin;
		current->i.orientation		= newData.orientation.ToMat3();
		SetAngularVelocity( newData.angularVelocity );
		SetLinearVelocity( newData.linearVelocity );

		self->UpdateVisuals();
		for ( int i = 0; i < bodies.Num(); i++ ) {
			bodies[i].Link( self, *current );
		}
		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdRBMultipleBroadcastState );

		localOrigin		= newData.localOrigin;
		localAxis			= newData.localOrientation.ToMat3();

		if ( current->atRest != newData.atRest ) {
			if ( newData.atRest == -1 ) {
				Activate();
			} else {
				Rest( newData.atRest );
			}
		}

		return;
	}
}

/*
================
anPhysics_RigidBodyMultiple::ReadNetworkState
================
*/
void anPhysics_RigidBodyMultiple::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const anBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdRBMultipleNetworkState );

		newData.origin			= msg.ReadDeltaVector( baseData.origin, RB_ORIGIN_EXPONENT_BITS, RB_ORIGIN_MANTISSA_BITS );
		newData.orientation		= msg.ReadDeltaCQuat( baseData.orientation );
		newData.angularVelocity	= msg.ReadDeltaVector( baseData.angularVelocity, RB_ANGULAR_VELOCITY_EXPONENT_BITS, RB_ANGULAR_VELOCITY_MANTISSA_BITS );
		newData.linearVelocity	= msg.ReadDeltaVector( baseData.linearVelocity, RB_LINEAR_VELOCITY_EXPONENT_BITS, RB_LINEAR_VELOCITY_MANTISSA_BITS );

		// denormals are bad!
		newData.origin.FixDenormals();
		newData.orientation.FixDenormals();
		newData.angularVelocity.FixDenormals();
		newData.linearVelocity.FixDenormals();

		self->OnNewOriginRead( newData.origin );
		self->OnNewAxesRead( newData.orientation.ToMat3() );
		return;
	}

	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdRBMultipleBroadcastState );

		newData.localOrigin			= msg.ReadDeltaVector( baseData.localOrigin, RB_ORIGIN_EXPONENT_BITS, RB_ORIGIN_MANTISSA_BITS );
		newData.localOrientation	= msg.ReadDeltaCQuat( baseData.localOrientation );
		newData.atRest				= msg.ReadDeltaLong( baseData.atRest );

		return;
	}
}

/*
================
anPhysics_RigidBodyMultiple::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* anPhysics_RigidBodyMultiple::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_VISIBLE ) {
		return new sdRBMultipleNetworkState();
	}
	if ( mode == NSM_BROADCAST ) {
		return new sdRBMultipleBroadcastState();
	}
	return nullptr;
}
