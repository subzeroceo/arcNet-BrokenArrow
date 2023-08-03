#include "../../idlib/Lib.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../Projectile.h"

CLASS_DECLARATION( anPhysics_Base, anPhysics_Particle )
END_CLASS

const float PRT_OVERCLIP	= 1.001f;
const float PRT_BOUNCESTOP	= 10.0f;

/*
================
anPhysics_Particle::DropToFloorAndRest

Drops the object straight down to the floor
================
*/
void anPhysics_Particle::DropToFloorAndRest( void ) {
	anVec3 down;
	trace_t tr;

	if ( testSolid ) {
		testSolid = false;
		if ( gameLocal.Contents( self, current.origin, clipModel, clipModel->GetAxis(), clipMask, self ) ) {
			gameLocal.Warning( "entity in solid '%s' type '%s' at (%s)",
								self->name.c_str(), self->GetType()->classname, current.origin.ToString(0) );
			PutToRest();
			dropToFloor = false;
			return;
		}
	}

	// put the body on the floor
	down = current.origin + gravityNormal * 128.0f;
	gameLocal.Translation( self, tr, current.origin, down, clipModel, clipModel->GetAxis(), clipMask, self );

	current.origin = tr.endpos;
	clipModel->Link( self, clipModel->GetId(), tr.endpos, clipModel->GetAxis() );

	// if on the floor already
	if ( tr.fraction == 0.0f ) {
		PutToRest();
		EvaluateContacts();// do a final contact check.  Items that drop to floor never do this check otherwise
		dropToFloor = false;
	} else if ( IsOutsideWorld() ) {
		gameLocal.Warning( "entity outside world bounds '%s' type '%s' at (%s)",
							self->name.c_str(), self->GetType()->classname, current.origin.ToString(0) );
		PutToRest();
		dropToFloor = false;
	}
}

/*
================
anPhysics_Particle::DebugDraw
================
*/
void anPhysics_Particle::DebugDraw( void ) {
	if ( rb_showBodies.GetBool() || ( rb_showActive.GetBool() && current.atRest < 0 ) ) {
		collisionModelManager->DrawModel( clipModel->GetCollisionModel(), clipModel->GetOrigin(), clipModel->GetAxis(), vec3_origin, mat3_identity, 0.0f );
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
anPhysics_Particle::anPhysics_Particle
================
*/
anPhysics_Particle::anPhysics_Particle( void ) {
	SetClipMask( MASK_SOLID );
	SetBouncyness( 0.6f, true );
	clipModel = nullptr;

	memset( &current, 0, sizeof( current ) );
	current.atRest = -1;
	current.origin.Zero();
	saved = current;

	dropToFloor		= false;
	testSolid		= false;
	hasMaster		= false;

	SetFriction( 0.6f, 0.6f, 0.0f );
	SetBouncyness ( 0.5f, true );

	gravityNormal.Zero();
}

/*
================
anPhysics_Particle::~anPhysics_Particle
================
*/
anPhysics_Particle::~anPhysics_Particle( void ) {
	if ( clipModel ) {
		delete clipModel;
		clipModel = nullptr;
	}
}

/*
================
anPhysics_Particle::Save
================
*/
void anPhysics_Particle::Save( anSaveGame *savefile ) const {
	savefile->WriteInt( current.atRest );
	savefile->WriteVec3( current.localOrigin );
	savefile->WriteMat3( current.localAxis );
	savefile->WriteVec3( current.pushVelocity );
	savefile->WriteVec3( current.origin );
	savefile->WriteVec3( current.velocity );
	savefile->WriteBool( current.onGround );
	savefile->WriteBool( current.inWater );

	savefile->WriteInt( saved.atRest );
	savefile->WriteVec3( saved.localOrigin );
	savefile->WriteMat3( saved.localAxis );
	savefile->WriteVec3( saved.pushVelocity );
	savefile->WriteVec3( saved.origin );
	savefile->WriteVec3( saved.velocity );
	savefile->WriteBool( saved.onGround );
	savefile->WriteBool( saved.inWater );

	savefile->WriteFloat( linearFriction );
	savefile->WriteFloat( angularFriction );
	savefile->WriteFloat( contactFriction );
	savefile->WriteFloat( bouncyness );
	savefile->WriteBool ( allowBounce );
	savefile->WriteBounds ( clipModel->GetBounds() );

	savefile->WriteBool( dropToFloor );
	savefile->WriteBool( testSolid );

	savefile->WriteBool( hasMaster );
	savefile->WriteBool( isOrientated );
	extraPassEntity.Save( savefile );

	savefile->WriteClipModel( clipModel );
}

/*
================
anPhysics_Particle::Restore
================
*/
void anPhysics_Particle::Restore( anRestoreGame *savefile ) {
	savefile->ReadInt( current.atRest );
	savefile->ReadVec3( current.localOrigin );
	savefile->ReadMat3( current.localAxis );
	savefile->ReadVec3( current.pushVelocity );
	savefile->ReadVec3( current.origin );
	savefile->ReadVec3( current.velocity );
	savefile->ReadBool( current.onGround );
	savefile->ReadBool( current.inWater );

	savefile->ReadInt( saved.atRest );
	savefile->ReadVec3( saved.localOrigin );
	savefile->ReadMat3( saved.localAxis );
	savefile->ReadVec3( saved.pushVelocity );
	savefile->ReadVec3( saved.origin );
	savefile->ReadVec3( saved.velocity );
	savefile->ReadBool( saved.onGround );
	savefile->ReadBool( saved.inWater );

	savefile->ReadFloat( linearFriction );
	savefile->ReadFloat( angularFriction );
	savefile->ReadFloat( contactFriction );
	savefile->ReadFloat( bouncyness );
	savefile->ReadBool ( allowBounce );

	anBounds bounds;					// Added unrestored var
	delete clipModel;
	savefile->ReadBounds ( bounds );
	clipModel = new anClipModel ( anTraceModel ( bounds ) );

	savefile->ReadBool( dropToFloor );
	savefile->ReadBool( testSolid );

	savefile->ReadBool( hasMaster );
	savefile->ReadBool( isOrientated );	// Added unrestored var
	extraPassEntity.Restore( savefile );

	savefile->ReadClipModel( clipModel );
}

/*
================
anPhysics_Particle::SetClipModel
================
*/
void anPhysics_Particle::SetClipModel( anClipModel *model, const float density, int id, bool freeOld ) {
	assert( self );
	assert( model );					// we need a clip model
	assert( model->IsTraceModel() );	// and it should be a trace model

	if ( clipModel && clipModel != model && freeOld ) {
		delete clipModel;
	}

	clipModel = model;
	clipModel->Link( self, 0, current.origin, clipModel->GetAxis() );
}

/*
================
anPhysics_Particle::GetClipModel
================
*/
anClipModel *anPhysics_Particle::GetClipModel( int id ) const {
	return clipModel;
}

/*
================
anPhysics_Particle::GetNumClipModels
================
*/
int anPhysics_Particle::GetNumClipModels( void ) const {
	return 1;
}

/*
================
anPhysics_Particle::SetBouncyness
================
*/
void anPhysics_Particle::SetBouncyness( const float b, bool _allowBounce ) {
	allowBounce = _allowBounce;
	if ( b < 0.0f || b > 1.0f ) {
		return;
	}
	bouncyness = b;
}

/*
================
anPhysics_Particle::SetFriction
================
*/
void anPhysics_Particle::SetFriction( const float linear, const float angular, const float contact ) {
	linearFriction = linear;
	angularFriction = angular;
	contactFriction = contact;
}


/*
================
anPhysics_Particle::PutToRest
================
*/
void anPhysics_Particle::PutToRest( void ) {
	current.atRest = gameLocal.time;
	current.velocity.Zero();
	self->BecomeInactive( TH_PHYSICS );
}

/*
================
anPhysics_Particle::DropToFloor
================
*/
void anPhysics_Particle::DropToFloor( void ) {
	dropToFloor = true;
	testSolid = true;
}

/*
================
anPhysics_Particle::Activate
================
*/
void anPhysics_Particle::Activate( void ) {
	current.atRest = -1;
	self->BecomeActive( TH_PHYSICS );
}

/*
================
anPhysics_Particle::EvaluateContacts
================
*/
bool anPhysics_Particle::EvaluateContacts( void ) {
	ClearContacts();
	AddGroundContacts( clipModel );
	AddContactEntitiesForContacts();
	return ( contacts.Num() != 0 );
}

/*
================
anPhysics_Particle::SetContents
================
*/
void anPhysics_Particle::SetContents( int contents, int id ) {
	clipModel->SetContents( contents );
}

/*
================
anPhysics_Particle::GetContents
================
*/
int anPhysics_Particle::GetContents( int id ) const {
	return clipModel->GetContents();
}

/*
================
anPhysics_Particle::GetBounds
================
*/
const anBounds &anPhysics_Particle::GetBounds( int id ) const {
	return clipModel->GetBounds();
}

/*
================
anPhysics_Particle::GetAbsBounds
================
*/
const anBounds &anPhysics_Particle::GetAbsBounds( int id ) const {
	return clipModel->GetAbsBounds();
}

/*
================
anPhysics_Particle::Evaluate

Evaluate the impulse based rigid body physics.
When a collision occurs an impulse is applied at the moment of impact but
the remaining time after the collision is ignored.
================
*/
bool anPhysics_Particle::Evaluate( int timeStepMSec, int endTimeMSec ) {
	particlePState_t next;
	float			 timeStep;
	float			 upspeed;

	timeStep = MS2SEC( timeStepMSec );

	// if bound to a master
	if ( hasMaster ) {
		anVec3	masterOrigin;
		anMat3	masterAxis;
		anVec3	oldOrigin;

		oldOrigin = current.origin;

		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + current.localOrigin * masterAxis;
		clipModel->Link( self, clipModel->GetId(), current.origin, current.localAxis * masterAxis );
		trace_t tr;
		gameLocal.Translation( self, tr, oldOrigin, current.origin, clipModel, clipModel->GetAxis(), clipMask, self );

		if ( tr.fraction < 1.0f ) {
			self->Collide ( tr, current.origin - oldOrigin );
		}

		DebugDraw();

		return true;
	}

	// if the body is at rest
	if ( current.atRest >= 0 || timeStep <= 0.0f ) {
		DebugDraw();
		return false;
	}

	// if putting the body to rest
	if ( dropToFloor ) {
		DropToFloorAndRest();
		return true;
	}

	clipModel->Unlink();

	// Determine if currently on the ground
	CheckGround();

	// Determine the current upward velocity
	if ( gravityNormal != vec3_zero ) {
		upspeed = -( current.velocity * gravityNormal );
	} else {
		upspeed = current.velocity.z;
	}

	// If not on the ground, or moving upwards, or bouncing and moving toward gravity then do a straight
	// forward slide move and gravity.
	if ( !current.onGround || upspeed > 1.0f || (bouncyness > 0.0f && upspeed < -PRT_BOUNCESTOP && !current.inWater) ) {
		// Force ground off when moving upward
		if ( upspeed > 0.0f ) {
			current.onGround = false;
		}
		SlideMove( current.origin, current.velocity, current.velocity * timeStep );
		if ( current.onGround && upspeed < PRT_BOUNCESTOP ) {
			current.velocity -= ( current.velocity * gravityNormal ) * gravityNormal;
		} else {
			current.velocity += (gravityVector * timeStep);
		}
	} else {
		anVec3 delta;

		// Slow down due to friction
		ApplyFriction ( timeStep );

		delta = current.velocity * timeStep;
		current.velocity -= ( current.velocity * gravityNormal ) * gravityNormal;
		if ( delta == vec3_origin ) {
			PutToRest( );
		} else {
			SlideMove( current.origin, current.velocity, delta );
		}
	}

	// update the position of the clip model
	clipModel->Link( self, clipModel->GetId(), current.origin, clipModel->GetAxis() );
	DebugDraw();

	// get all the ground contacts
	EvaluateContacts();

	current.pushVelocity.Zero();

	if ( IsOutsideWorld() ) {
		gameLocal.Warning( "clip model outside world bounds for entity '%s' at (%s)", self->name.c_str(), current.origin.ToString(0) );
		PutToRest();
	}

	return true;
}

/*
================
anPhysics_Particle::UpdateTime
================
*/
void anPhysics_Particle::UpdateTime( int endTimeMSec ) {
}

/*
================
anPhysics_Particle::GetTime
================
*/
int anPhysics_Particle::GetTime( void ) const {
	return gameLocal.time;
}

/*
================
anPhysics_Particle::IsAtRest
================
*/
bool anPhysics_Particle::IsAtRest( void ) const {
	return current.atRest >= 0;
}

/*
================
anPhysics_Particle::GetRestStartTime
================
*/
int anPhysics_Particle::GetRestStartTime( void ) const {
	return current.atRest;
}

/*
================
anPhysics_Particle::IsPushable
================
*/
bool anPhysics_Particle::IsPushable( void ) const {
	return ( !hasMaster );
}

/*
================
anPhysics_Particle::SaveState
================
*/
void anPhysics_Particle::SaveState( void ) {
	saved = current;
}

/*
================
anPhysics_Particle::RestoreState
================
*/
void anPhysics_Particle::RestoreState( void ) {
	current = saved;
	clipModel->Link( self, clipModel->GetId(), current.origin, clipModel->GetAxis() );
	EvaluateContacts();
}

/*
================
anPhysics::SetOrigin
================
*/
void anPhysics_Particle::SetOrigin( const anVec3 &newOrigin, int id ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	current.localOrigin = newOrigin;
	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + newOrigin * masterAxis;
	} else {
		current.origin = newOrigin;
	}
	clipModel->Link( self, clipModel->GetId(), current.origin, clipModel->GetAxis() );

	Activate();
}

/*
================
anPhysics::SetAxis
================
*/
void anPhysics_Particle::SetAxis( const anMat3 &newAxis, int id ) {
	current.localAxis = newAxis;
	clipModel->Link( self, 0, clipModel->GetOrigin(), newAxis );
	Activate();
}

/*
================
anPhysics_Particle::Translate
================
*/
void anPhysics_Particle::Translate( const anVec3 &translation, int id ) {
	current.localOrigin += translation;
	current.origin += translation;
	clipModel->Link( self, clipModel->GetId(), current.origin, clipModel->GetAxis() );
	Activate();
}

/*
================
anPhysics_Particle::Rotate(
================
*/
void anPhysics_Particle::Rotate( const anRotation &rotation, int id ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	current.origin *= rotation;
	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
	} else {
		current.localOrigin = current.origin;
	}
	clipModel->Link( self, 0, current.origin, clipModel->GetAxis() * rotation.ToMat3() );
	Activate();
}

/*
================
anPhysics_Particle::GetOrigin
================
*/
const anVec3 &anPhysics_Particle::GetOrigin( int id ) const {
	return clipModel->GetOrigin();
}

/*
================
anPhysics_Particle::GetAxis
================
*/
const anMat3 &anPhysics_Particle::GetAxis( int id ) const {
	if ( !clipModel ) {
		return anPhysics_Base::GetAxis ( id );
	}
	return clipModel->GetAxis();
}

/*
================
anPhysics_Particle::SetLinearVelocity
================
*/
void anPhysics_Particle::SetLinearVelocity( const anVec3 &velocity, int id ) {
	current.velocity = velocity;
	Activate();
}

/*
================
anPhysics_Particle::GetLinearVelocity
================
*/
const anVec3 &anPhysics_Particle::GetLinearVelocity( int id ) const {
	return current.velocity;
}

/*
================
anPhysics_Particle::ClipTranslation
================
*/
void anPhysics_Particle::ClipTranslation( trace_t &results, const anVec3 &translation, const anClipModel *model ) const {
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
anPhysics_Particle::ClipRotation
================
*/
void anPhysics_Particle::ClipRotation( trace_t &results, const anRotation &rotation, const anClipModel *model ) const {
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
anPhysics_Particle::ClipContents
================
*/
int anPhysics_Particle::ClipContents( const anClipModel *model ) const {
	if ( model ) {
		return gameLocal.ContentsModel( self, clipModel->GetOrigin(), clipModel, clipModel->GetAxis(), -1,
									model->GetCollisionModel(), model->GetOrigin(), model->GetAxis() );
	} else {
		return gameLocal.Contents( self, clipModel->GetOrigin(), clipModel, clipModel->GetAxis(), -1, nullptr );
	}
}

/*
================
anPhysics_Particle::DisableClip
================
*/
void anPhysics_Particle::DisableClip( void ) {
	clipModel->Disable();
}

/*
================
anPhysics_Particle::EnableClip
================
*/
void anPhysics_Particle::EnableClip( void ) {
	clipModel->Enable();
}

/*
================
anPhysics_Particle::UnlinkClip
================
*/
void anPhysics_Particle::UnlinkClip( void ) {
	clipModel->Unlink();
}

/*
================
anPhysics_Particle::LinkClip
================
*/
void anPhysics_Particle::LinkClip( void ) {
	clipModel->Link( self, clipModel->GetId(), current.origin, clipModel->GetAxis() );
}

/*
================
anPhysics_Particle::SetPushed
================
*/
void anPhysics_Particle::SetPushed( int deltaTime ) {
	// velocity with which the particle is pushed
	current.pushVelocity = ( current.origin - saved.origin ) / ( deltaTime * anMath::M_MS2SEC );
}

/*
================
anPhysics_Particle::GetPushedLinearVelocity
================
*/
const anVec3 &anPhysics_Particle::GetPushedLinearVelocity( const int id ) const {
	return current.pushVelocity;
}

/*
================
anPhysics_Particle::SetMaster
================
*/
void anPhysics_Particle::SetMaster( anEntity *master, const bool orientated ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	if ( master ) {
		if ( !hasMaster ) {
			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
			hasMaster = true;
		}
		ClearContacts();
	} else {
		if ( hasMaster ) {
			hasMaster = false;
			Activate();
		}
	}
}

/*
=====================
anPhysics_Particle::CheckGround
=====================
*/
void anPhysics_Particle::CheckGround( void ) {
	trace_t	groundTrace;
	anVec3	down;

	if ( gravityNormal == vec3_zero ) {
		current.onGround = false;
		return;
	}

	down = current.origin + gravityNormal * CONTACT_EPSILON;
	gameLocal.Translation( self, groundTrace, current.origin, down, clipModel, clipModel->GetAxis(), clipMask, self );
	if ( groundTrace.fraction == 1.0f ) {
		current.onGround = false;
		return;
	}

	if ( ( groundTrace.c.normal * -gravityNormal ) < 0.7f ) {
		current.onGround = false;
		return;
	}

	current.onGround = true;
}

/*
================
anPhysics_Particle::ApplyFriction
================
*/
void anPhysics_Particle::ApplyFriction( float timeStep ) {
	anVec3	vel;
	float	speed;
	float	newspeed;

	// ignore slope movement, remove all velocity in gravity direction
	vel = current.velocity + (current.velocity * gravityNormal) * gravityNormal;

	speed = vel.Length();
	if ( speed < 1.0f ) {
		// remove all movement orthogonal to gravity, allows for sinking underwater
		current.velocity = (current.velocity * gravityNormal) * gravityNormal;
		return;
	}

	// scale the velocity
	if ( current.onGround ) {
		newspeed = speed - ( ( speed * contactFriction ) * timeStep );
	} else {
		newspeed = speed - ( ( speed * linearFriction ) * timeStep );
	}

	if ( newspeed < 0 ) {
		newspeed = 0;
	}

	current.velocity *= ( newspeed / speed );
}

/*
================
anPhysics_Particle::SlideMove
================
*/
bool anPhysics_Particle::SlideMove( anVec3 &start, anVec3 &velocity, const anVec3 &delta ) {
	int		i;
	trace_t tr;
	anVec3	move;
	bool collide, rtnValue = false;

	move = delta;
	for ( i = 0; i < 3; i++ ) { // be sure if you change this upper value in the for () to update the exit condition below!!!!!
		gameLocal.Translation( self, tr, start, start + move, clipModel, clipModel->GetAxis(), clipMask, self, extraPassEntity );
		start = tr.endpos;
		if ( tr.fraction == 1.0f ) {
			if ( i > 0 ) {
				return false;
			}
			return true;
		}

		bool hitTeleporter = false;

		// let the entity know about the collision
		collide = self->Collide( tr, current.velocity, hitTeleporter );

		anEntity* ent;
		ent = gameLocal.entities[tr.c.entityNum];
		assert ( ent );

		// If we hit water just clip the move for now and keep on going
		if ( ent->GetPhysics()->GetContents() & CONTENTS_WATER ) {
			// Make sure we dont collide with water again
			clipMask &= ~CONTENTS_WATER;

			current.inWater = true;

			// Allow the loop to go one more round to push us through the water
			i--;

			velocity *= 0.4f;

			move.ProjectOntoPlane( tr.c.normal, PRT_OVERCLIP );
			continue;
		// bounce the projectile
		} else if ( !current.inWater && allowBounce && bouncyness ) {
			if ( !hitTeleporter ) {
				float dot;
				move = tr.endpos;
				dot = DotProduct( velocity, tr.c.normal );
				velocity  = ( velocity - ( 2.0f * dot * tr.c.normal ) ) * bouncyness;
			}
			return true;
// tr.c.material can (did) crash here if null
		} else if ( allowBounce && tr.c.material && (tr.c.material->GetSurfaceFlags() & SURF_BOUNCE) ) {
			float dot;
			move = tr.endpos;
			dot = DotProduct( velocity, tr.c.normal );
			velocity  = ( velocity - ( 2.0f * dot * tr.c.normal ) );
			return true;
		} else {
			i = 4;
			rtnValue = true;
		}

		// clip the movement delta and velocity
		if ( collide ) {
			move.ProjectOntoPlane( tr.c.normal, PRT_OVERCLIP );
			velocity.ProjectOntoPlane( tr.c.normal, PRT_OVERCLIP );
		}
	}

	return rtnValue;
}

const float	PRT_VELOCITY_MAX			= 16000;
const int	PRT_VELOCITY_TOTAL_BITS		= 16;
const int	PRT_VELOCITY_EXPONENT_BITS	= anMath::BitsForInteger( anMath::BitsForFloat( PRT_VELOCITY_MAX ) ) + 1;
const int	PRT_VELOCITY_MANTISSA_BITS	= PRT_VELOCITY_TOTAL_BITS - 1 - PRT_VELOCITY_EXPONENT_BITS;

/*
================
anPhysics_Particle::WriteToSnapshot
================
*/
void anPhysics_Particle::WriteToSnapshot( anBitMsgDelta &msg ) const {
	msg.WriteLong( current.atRest );
	msg.WriteBits ( current.onGround, 1 );
	msg.WriteFloat( current.origin[0] );
	msg.WriteFloat( current.origin[1] );
	msg.WriteFloat( current.origin[2] );
//	msg.WriteFloat( current.velocity[0], PRT_VELOCITY_EXPONENT_BITS, PRT_VELOCITY_MANTISSA_BITS );
//	msg.WriteFloat( current.velocity[1], PRT_VELOCITY_EXPONENT_BITS, PRT_VELOCITY_MANTISSA_BITS );
//	msg.WriteFloat( current.velocity[2], PRT_VELOCITY_EXPONENT_BITS, PRT_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.velocity[0] );
	msg.WriteDeltaFloat( 0.0f, current.velocity[1] );
	msg.WriteDeltaFloat( 0.0f, current.velocity[2] );
//	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[0], PRT_VELOCITY_EXPONENT_BITS, PRT_VELOCITY_MANTISSA_BITS );
//	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[1], PRT_VELOCITY_EXPONENT_BITS, PRT_VELOCITY_MANTISSA_BITS );
//	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[2], PRT_VELOCITY_EXPONENT_BITS, PRT_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[0] );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[1] );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[2] );

	// TODO: Check that this conditional write to delta message is OK
	if ( hasMaster ) {
		anCQuat localQuat;
		localQuat = current.localAxis.ToCQuat();

		msg.WriteBits ( 1, 1 );
		msg.WriteFloat( localQuat.x );
		msg.WriteFloat( localQuat.y );
		msg.WriteFloat( localQuat.z );
		msg.WriteDeltaFloat( current.origin[0], current.localOrigin[0] );
		msg.WriteDeltaFloat( current.origin[1], current.localOrigin[1] );
		msg.WriteDeltaFloat( current.origin[2], current.localOrigin[2] );
	} else {
		msg.WriteBits ( 0, 1 );
	}
}

/*
================
anPhysics_Particle::ReadFromSnapshot
================
*/
void anPhysics_Particle::ReadFromSnapshot( const anBitMsgDelta &msg ) {
	current.atRest = msg.ReadLong();
	current.onGround = ( msg.ReadBits( 1 ) != 0 );
	current.origin[0] = msg.ReadFloat();
	current.origin[1] = msg.ReadFloat();
	current.origin[2] = msg.ReadFloat();
//	current.velocity[0] = msg.ReadFloat( PRT_VELOCITY_EXPONENT_BITS, PRT_VELOCITY_MANTISSA_BITS );
//	current.velocity[1] = msg.ReadFloat( PRT_VELOCITY_EXPONENT_BITS, PRT_VELOCITY_MANTISSA_BITS );
//	current.velocity[2] = msg.ReadFloat( PRT_VELOCITY_EXPONENT_BITS, PRT_VELOCITY_MANTISSA_BITS );
	current.velocity[0] = msg.ReadDeltaFloat( 0.0f );
	current.velocity[1] = msg.ReadDeltaFloat( 0.0f );
	current.velocity[2] = msg.ReadDeltaFloat( 0.0f );
//	current.pushVelocity[0] = msg.ReadDeltaFloat( 0.0f, PRT_VELOCITY_EXPONENT_BITS, PRT_VELOCITY_MANTISSA_BITS );
//	current.pushVelocity[1] = msg.ReadDeltaFloat( 0.0f, PRT_VELOCITY_EXPONENT_BITS, PRT_VELOCITY_MANTISSA_BITS );
//	current.pushVelocity[2] = msg.ReadDeltaFloat( 0.0f, PRT_VELOCITY_EXPONENT_BITS, PRT_VELOCITY_MANTISSA_BITS );
	current.pushVelocity[0] = msg.ReadDeltaFloat( 0.0f );
	current.pushVelocity[1] = msg.ReadDeltaFloat( 0.0f );
	current.pushVelocity[2] = msg.ReadDeltaFloat( 0.0f );

	if ( msg.ReadBits ( 1 ) ) {
		anCQuat localQuat;
		localQuat.x = msg.ReadFloat( );
		localQuat.y = msg.ReadFloat( );
		localQuat.z = msg.ReadFloat( );
		current.localOrigin[0] = msg.ReadDeltaFloat( current.origin[0] );
		current.localOrigin[1] = msg.ReadDeltaFloat( current.origin[1] );
		current.localOrigin[2] = msg.ReadDeltaFloat( current.origin[2] );
		current.localAxis = localQuat.ToMat3();
	}

	if ( clipModel ) {
		clipModel->Link( self, clipModel->GetId(), current.origin, clipModel->GetAxis() );
	}
}
