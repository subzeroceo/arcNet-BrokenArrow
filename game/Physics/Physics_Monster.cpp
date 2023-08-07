
#include "../../idlib/Lib.h"
#pragma hdrstop

#include "../Game_local.h"

CLASS_DECLARATION( anPhysics_Actor, anPhysics_Monster )
END_CLASS

const float OVERCLIP = 1.001f;

/*
=====================
anPhysics_Monster::CheckGround
=====================
*/
void anPhysics_Monster::CheckGround( monsterPState_t &state ) {
	trace_t groundTrace;
	anVec3 down;

	if ( gravityNormal == vec3_zero ) {
		state.onGround = false;
		groundEntityPtr = nullptr;
		return;
	}

	down = state.origin + gravityNormal * CONTACT_EPSILON;


	gameLocal.Translation( self, groundTrace, state.origin, down, clipModel, clipModel->GetAxis(), clipMask, self );


	if ( groundTrace.fraction == 1.0f ) {
		state.onGround = false;
		groundEntityPtr = nullptr;
		return;
	}

	groundEntityPtr = gameLocal.entities[ groundTrace.c.entityNum ];

	if ( ( groundTrace.c.normal * -gravityNormal ) < minFloorCosine ) {
		state.onGround = false;
		return;
	}

	state.onGround = true;

	// let the entity know about the collision
	self->Collide( groundTrace, state.velocity );

	// apply impact to a non world floor entity
	if ( groundTrace.c.entityNum != ENTITYNUM_WORLD && groundEntityPtr.GetEntity() ) {
		impactInfo_t info;
		groundEntityPtr.GetEntity()->GetImpactInfo( self, groundTrace.c.id, groundTrace.c.point, &info );
		if ( info.invMass != 0.0f ) {
			groundEntityPtr.GetEntity()->ApplyImpulse( self, 0, groundTrace.c.point, state.velocity  / ( info.invMass * 10.0f ) );
		}
	}
}

/*
=====================
anPhysics_Monster::SlideMove
=====================
*/
monsterMoveResult_t anPhysics_Monster::SlideMove( anVec3 &start, anVec3 &velocity, const anVec3 &delta ) {
	int i;
	trace_t tr;
	anVec3 move;

	blockingEntity = nullptr;
	move = delta;
	for ( i = 0; i < 3; i++ ) {

// ddynerman: multiple collision worlds
		gameLocal.Translation( self, tr, start, start + move, clipModel, clipModel->GetAxis(), clipMask, self );


		start = tr.endpos;

		if ( tr.fraction == 1.0f ) {
			if ( i > 0 ) {
				return MM_SLIDING;
			}
			return MM_OK;
		}

		if ( tr.c.entityNum != ENTITYNUM_NONE ) {
			blockingEntity = gameLocal.entities[ tr.c.entityNum ];
		}

		// clip the movement delta and velocity
		move.ProjectOntoPlane( tr.c.normal, OVERCLIP );
		velocity.ProjectOntoPlane( tr.c.normal, OVERCLIP );
	}

	return MM_BLOCKED;
}

/*
=====================
anPhysics_Monster::StepMove

  move start into the delta direction
  the velocity is clipped conform any collisions
=====================
*/
monsterMoveResult_t anPhysics_Monster::StepMove( anVec3 &start, anVec3 &velocity, const anVec3 &delta ) {
	trace_t tr;
	anVec3 up, down, noStepPos, noStepVel, stepPos, stepVel;
	monsterMoveResult_t result1, result2;
	float	stepdist;
	float	nostepdist;

	if ( delta == vec3_origin ) {
		return MM_OK;
	}

	// try to move without stepping up
	noStepPos = start;
	noStepVel = velocity;
	result1 = SlideMove( noStepPos, noStepVel, delta );
	if ( result1 == MM_OK ) {
		velocity = noStepVel;

// bdube: dont step when there is no gravity
		if ( gravityNormal == vec3_zero || forceDeltaMove ) {

			start = noStepPos;
			return MM_OK;
		}

		// try to step down so that we walk down slopes and stairs at a normal rate
		down = noStepPos + gravityNormal * maxStepHeight;


		gameLocal.Translation( self, tr, noStepPos, down, clipModel, clipModel->GetAxis(), clipMask, self );

		if ( tr.fraction < 1.0f ) {
			start = tr.endpos;
			return MM_STEPPED;
		} else {
			start = noStepPos;
			return MM_OK;
		}
	}



	if ( blockingEntity && blockingEntity->IsType( anActor::GetClassType() ) ) {

		// try to step down in case walking into an actor while going down steps
		down = noStepPos + gravityNormal * maxStepHeight;


		gameLocal.Translation( self, tr, noStepPos, down, clipModel, clipModel->GetAxis(), clipMask, self );

		start = tr.endpos;
		velocity = noStepVel;
		return MM_BLOCKED;
	}

	if ( gravityNormal == vec3_zero ) {
		return result1;
	}

	// try to step up
	up = start - gravityNormal * maxStepHeight;


	gameLocal.Translation( self, tr, start, up, clipModel, clipModel->GetAxis(), clipMask, self );

	if ( tr.fraction == 0.0f ) {
		start = noStepPos;
		velocity = noStepVel;
		return result1;
	}

	// try to move at the stepped up position
	stepPos = tr.endpos;
	stepVel = velocity;
	result2 = SlideMove( stepPos, stepVel, delta );
	if ( result2 == MM_BLOCKED ) {
		start = noStepPos;
		velocity = noStepVel;
		return result1;
	}

	// step down again
	down = stepPos + gravityNormal * maxStepHeight;


	gameLocal.Translation( self, tr, stepPos, down, clipModel, clipModel->GetAxis(), clipMask, self );

	stepPos = tr.endpos;

	// if the move is further without stepping up, or the slope is too steap, don't step up
	nostepdist = ( noStepPos - start ).LengthSqr();
	stepdist = ( stepPos - start ).LengthSqr();
	if ( ( nostepdist >= stepdist ) || ( ( tr.c.normal * -gravityNormal ) < minFloorCosine ) ) {
		start = noStepPos;
		velocity = noStepVel;
		return MM_SLIDING;
	}

	start = stepPos;
	velocity = stepVel;

	return MM_STEPPED;
}

/*
================
anPhysics_Monster::Activate
================
*/
void anPhysics_Monster::Activate( void ) {
	current.atRest = -1;
	self->BecomeActive( TH_PHYSICS );
}

/*
================
anPhysics_Monster::Rest
================
*/
void anPhysics_Monster::Rest( void ) {
	current.atRest = gameLocal.time;
	current.velocity.Zero();
	self->BecomeInactive( TH_PHYSICS );
}

/*
================
anPhysics_Monster::PutToRest
================
*/
void anPhysics_Monster::PutToRest( void ) {
	Rest();
}

/*
================
anPhysics_Monster::anPhysics_Monster
================
*/
anPhysics_Monster::anPhysics_Monster( void ) {

	memset( &current, 0, sizeof( current ) );
	current.atRest = -1;
	saved = current;

	delta.Zero();
	maxStepHeight = 18.0f;
	minFloorCosine = 0.7f;
	moveResult = MM_OK;
	forceDeltaMove = false;
	fly = false;
	useVelocityMove = false;
	noImpact = false;
	blockingEntity = nullptr;
}

/*
================
anPhysics_Monster_SavePState
================
*/
void anPhysics_Monster_SavePState( anSaveGame *savefile, const monsterPState_t &state ) {
	savefile->WriteInt( state.atRest );
	savefile->WriteBool( state.onGround );
	savefile->WriteVec3( state.origin );
	savefile->WriteVec3( state.velocity );
	savefile->WriteVec3( state.localOrigin );
	savefile->WriteVec3( state.pushVelocity );

	savefile->WriteVec3( state.lastPushVelocity );	// cnicholson: Added unsaved var
}

/*
================
anPhysics_Monster_RestorePState
================
*/
void anPhysics_Monster_RestorePState( anRestoreGame *savefile, monsterPState_t &state ) {
	savefile->ReadInt( state.atRest );
	savefile->ReadBool( state.onGround );
	savefile->ReadVec3( state.origin );
	savefile->ReadVec3( state.velocity );
	savefile->ReadVec3( state.localOrigin );
	savefile->ReadVec3( state.pushVelocity );

	savefile->ReadVec3( state.lastPushVelocity );
}

/*
================
anPhysics_Monster::Save
================
*/
void anPhysics_Monster::Save( anSaveGame *savefile ) const {

	anPhysics_Monster_SavePState( savefile, current );
	anPhysics_Monster_SavePState( savefile, saved );

	savefile->WriteFloat( maxStepHeight );
	savefile->WriteFloat( minFloorCosine );
	savefile->WriteVec3( delta );

	savefile->WriteBool( forceDeltaMove );
	savefile->WriteBool( fly );
	savefile->WriteBool( useVelocityMove );
	savefile->WriteBool( noImpact );

	savefile->WriteInt( (int)moveResult );
	savefile->WriteObject( blockingEntity );
}

/*
================
anPhysics_Monster::Restore
================
*/
void anPhysics_Monster::Restore( anRestoreGame *savefile ) {

	anPhysics_Monster_RestorePState( savefile, current );
	anPhysics_Monster_RestorePState( savefile, saved );

	savefile->ReadFloat( maxStepHeight );
	savefile->ReadFloat( minFloorCosine );
	savefile->ReadVec3( delta );

	savefile->ReadBool( forceDeltaMove );
	savefile->ReadBool( fly );
	savefile->ReadBool( useVelocityMove );
	savefile->ReadBool( noImpact );

	savefile->ReadInt( ( int&)moveResult );
	savefile->ReadObject( reinterpret_cast<anClass *&>( blockingEntity ) );
}

/*
================
anPhysics_Monster::SetDelta
================
*/
void anPhysics_Monster::SetDelta( const anVec3 &d ) {
	delta = d;
	if ( delta != vec3_origin ) {
		Activate();
	}
}

/*
================
anPhysics_Monster::SetMaxStepHeight
================
*/
void anPhysics_Monster::SetMaxStepHeight( const float newMaxStepHeight ) {
	maxStepHeight = newMaxStepHeight;
}

/*
================
anPhysics_Monster::GetMaxStepHeight
================
*/
float anPhysics_Monster::GetMaxStepHeight( void ) const {
	return maxStepHeight;
}

/*
================
anPhysics_Monster::OnGround
================
*/
bool anPhysics_Monster::OnGround( void ) const {
	return current.onGround;
}

/*
================
anPhysics_Monster::GetSlideMoveEntity
================
*/
anEntity *anPhysics_Monster::GetSlideMoveEntity( void ) const {
	return blockingEntity;
}

/*
================
anPhysics_Monster::GetMoveResult
================
*/
monsterMoveResult_t anPhysics_Monster::GetMoveResult( void ) const {
	return moveResult;
}

/*
================
anPhysics_Monster::ForceDeltaMove
================
*/
void anPhysics_Monster::ForceDeltaMove( bool force ) {
	forceDeltaMove = force;
}

/*
================
anPhysics_Monster::UseFlyMove
================
*/
void anPhysics_Monster::UseFlyMove( bool force ) {
	fly = force;
}

/*
================
anPhysics_Monster::UseVelocityMove
================
*/
void anPhysics_Monster::UseVelocityMove( bool force ) {
	useVelocityMove = force;
}

/*
================
anPhysics_Monster::EnableImpact
================
*/
void anPhysics_Monster::EnableImpact( void ) {
	noImpact = false;
}

/*
================
anPhysics_Monster::DisableImpact
================
*/
void anPhysics_Monster::DisableImpact( void ) {
	noImpact = true;
}

/*
================
anPhysics_Monster::Evaluate
================
*/
bool anPhysics_Monster::Evaluate( int timeStepMSec, int endTimeMSec ) {
	anVec3 masterOrigin, oldOrigin;
	anMat3 masterAxis;
	float timeStep;

	timeStep = MS2SEC( timeStepMSec );

	moveResult = MM_OK;
	blockingEntity = nullptr;
	oldOrigin = current.origin;

	// if bound to a master
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + current.localOrigin * masterAxis;


		clipModel->Link( self, 0, current.origin, clipModel->GetAxis() );

		current.velocity = ( current.origin - oldOrigin ) / timeStep;
		masterDeltaYaw = masterYaw;
		masterYaw = masterAxis[0].ToYaw();
		masterDeltaYaw = masterYaw - masterDeltaYaw;
		return true;
	}

	// if the monster is at rest
	if ( current.atRest >= 0 ) {
		return true;
	}

	ActivateContactEntities();

	// move the monster velocity into the frame of a pusher
	current.velocity -= current.pushVelocity;

	clipModel->Unlink();

	// check if on the ground
	anPhysics_Monster::CheckGround( current );

	// if not on the ground or moving upwards
	float upspeed;
	if ( gravityNormal != vec3_zero ) {
		upspeed = -( current.velocity * gravityNormal );
	} else {
		upspeed = current.velocity.z;
	}
	if ( fly || ( !forceDeltaMove && ( !current.onGround || upspeed > 1.0f ) ) ) {
		if ( upspeed < 0.0f ) {
			moveResult = MM_FALLING;
		}
		else {
			current.onGround = false;
			moveResult = MM_OK;
		}
		delta = current.velocity * timeStep;
		if ( delta != vec3_origin ) {
			moveResult = anPhysics_Monster::SlideMove( current.origin, current.velocity, delta );
            delta.Zero();
		}

		if ( !fly ) {
			current.velocity += gravityVector * timeStep;
		}
	} else {
		if ( useVelocityMove ) {
			delta = current.velocity * timeStep;
		} else {
			current.velocity = delta / timeStep;
		}

		current.velocity -= ( current.velocity * gravityNormal ) * gravityNormal;

		if ( delta == vec3_origin ) {
			Rest();
		} else {
			// try moving into the desired direction

// jshepard: flying creatures, even if not using fly move, shouldn't use step move
			if ( self->IsType( anSAAI::GetClassType() ) && ((anSAAI*)self)->move.moveType == MOVETYPE_FLY ) {
				moveResult = anPhysics_Monster::SlideMove( current.origin, current.velocity, delta );
			} else {
				moveResult = anPhysics_Monster::StepMove( current.origin, current.velocity, delta );
			}
			delta.Zero();

			current.velocity -= ( current.velocity * gravityNormal ) * gravityNormal;
		}
	}


	clipModel->Link( self, 0, current.origin, clipModel->GetAxis() );


	// get all the ground contacts
	EvaluateContacts();

	// move the monster velocity back into the world frame
	current.velocity += current.pushVelocity;
	current.lastPushVelocity = current.pushVelocity;
	current.pushVelocity.Zero();

	if ( IsOutsideWorld() ) {
		gameLocal.Warning( "clip model outside world bounds for entity '%s' at (%s)", self->name.c_str(), current.origin.ToString( 0 ) );
		Rest();
	}


// bdube: determine if we moved this frame
	return true;

}

/*
================
anPhysics_Monster::UpdateTime
================
*/
void anPhysics_Monster::UpdateTime( int endTimeMSec ) {
}

/*
================
anPhysics_Monster::GetTime
================
*/
int anPhysics_Monster::GetTime( void ) const {
	return gameLocal.time;
}

/*
================
anPhysics_Monster::GetImpactInfo
================
*/
void anPhysics_Monster::GetImpactInfo( const int id, const anVec3 &point, impactInfo_t *info ) const {
	info->invMass = invMass;
	info->invInertiaTensor.Zero();
	info->position.Zero();
	info->velocity = current.velocity;
}

/*
================
anPhysics_Monster::ApplyImpulse
================
*/
void anPhysics_Monster::ApplyImpulse( const int id, const anVec3 &point, const anVec3 &impulse ) {
	if ( noImpact ) {
		return;
	}
	current.velocity += impulse * invMass;
	Activate();
}

/*
================
anPhysics_Monster::IsAtRest
================
*/
bool anPhysics_Monster::IsAtRest( void ) const {
	return current.atRest >= 0;
}

/*
================
anPhysics_Monster::GetRestStartTime
================
*/
int anPhysics_Monster::GetRestStartTime( void ) const {
	return current.atRest;
}

/*
================
anPhysics_Monster::SaveState
================
*/
void anPhysics_Monster::SaveState( void ) {
	saved = current;
}

/*
================
anPhysics_Monster::RestoreState
================
*/
void anPhysics_Monster::RestoreState( void ) {
	current = saved;


	clipModel->Link( self, 0, current.origin, clipModel->GetAxis() );


	EvaluateContacts();
}

/*
================
anPhysics_Player::SetOrigin
================
*/
void anPhysics_Monster::SetOrigin( const anVec3 &newOrigin, int id ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	current.localOrigin = newOrigin;
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + newOrigin * masterAxis;
	}
	else {
		current.origin = newOrigin;
	}


	clipModel->Link( self, 0, newOrigin, clipModel->GetAxis() );

	Activate();
}

/*
================
anPhysics_Player::SetAxis
================
*/
void anPhysics_Monster::SetAxis( const anMat3 &newAxis, int id ) {


	clipModel->Link( self, 0, clipModel->GetOrigin(), newAxis );

	Activate();
}

/*
================
anPhysics_Monster::Translate
================
*/
void anPhysics_Monster::Translate( const anVec3 &translation, int id ) {

	current.localOrigin += translation;
	current.origin += translation;


	clipModel->Link( self, 0, current.origin, clipModel->GetAxis() );

	Activate();
}

/*
================
anPhysics_Monster::Rotate
================
*/
void anPhysics_Monster::Rotate( const anRotation &rotation, int id ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	current.origin *= rotation;
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
	}
	else {
		current.localOrigin = current.origin;
	}


	clipModel->Link( self, 0, current.origin, clipModel->GetAxis() * rotation.ToMat3() );

	Activate();
}

/*
================
anPhysics_Monster::SetLinearVelocity
================
*/
void anPhysics_Monster::SetLinearVelocity( const anVec3 &newLinearVelocity, int id ) {
	current.velocity = newLinearVelocity;
	Activate();
}

/*
================
anPhysics_Monster::GetLinearVelocity
================
*/
const anVec3 &anPhysics_Monster::GetLinearVelocity( int id ) const {
	return current.velocity;
}

/*
================
anPhysics_Monster::SetPushed
================
*/
void anPhysics_Monster::SetPushed( int deltaTime ) {
	// velocity with which the monster is pushed
	current.pushVelocity += ( current.origin - saved.origin ) / ( deltaTime * anMath::M_MS2SEC );
}

/*
================
anPhysics_Monster::GetPushedLinearVelocity
================
*/
const anVec3 &anPhysics_Monster::GetPushedLinearVelocity( const int id ) const {
	return current.lastPushVelocity;
}

/*
================
anPhysics_Monster::SetMaster

  the binding is never orientated
================
*/
void anPhysics_Monster::SetMaster( anEntity *master, const bool orientated ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	if ( master ) {
		if ( !masterEntity ) {
			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
			masterEntity = master;
			masterYaw = masterAxis[0].ToYaw();
		}
		ClearContacts();
	}
	else {
		if ( masterEntity ) {
			masterEntity = nullptr;
			Activate();
		}
	}
}

const float	MONSTER_VELOCITY_MAX			= 4000;
const int	MONSTER_VELOCITY_TOTAL_BITS		= 16;
const int	MONSTER_VELOCITY_EXPONENT_BITS	= anMath::BitsForInteger( anMath::BitsForFloat( MONSTER_VELOCITY_MAX ) ) + 1;
const int	MONSTER_VELOCITY_MANTISSA_BITS	= MONSTER_VELOCITY_TOTAL_BITS - 1 - MONSTER_VELOCITY_EXPONENT_BITS;

/*
================
anPhysics_Monster::WriteToSnapshot
================
*/
void anPhysics_Monster::WriteToSnapshot( anBitMsgDelta &msg ) const {
	msg.WriteFloat( current.origin[0] );
	msg.WriteFloat( current.origin[1] );
	msg.WriteFloat( current.origin[2] );
	msg.WriteFloat( current.velocity[0], MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	msg.WriteFloat( current.velocity[1], MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	msg.WriteFloat( current.velocity[2], MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( current.origin[0], current.localOrigin[0] );
	msg.WriteDeltaFloat( current.origin[1], current.localOrigin[1] );
	msg.WriteDeltaFloat( current.origin[2], current.localOrigin[2] );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[0], MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[1], MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	msg.WriteDeltaFloat( 0.0f, current.pushVelocity[2], MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	msg.WriteLong( current.atRest );
	msg.WriteBits( current.onGround, 1 );
}

/*
================
anPhysics_Monster::ReadFromSnapshot
================
*/
void anPhysics_Monster::ReadFromSnapshot( const anBitMsgDelta &msg ) {
	current.origin[0] = msg.ReadFloat();
	current.origin[1] = msg.ReadFloat();
	current.origin[2] = msg.ReadFloat();
	current.velocity[0] = msg.ReadFloat( MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	current.velocity[1] = msg.ReadFloat( MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	current.velocity[2] = msg.ReadFloat( MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	current.localOrigin[0] = msg.ReadDeltaFloat( current.origin[0] );
	current.localOrigin[1] = msg.ReadDeltaFloat( current.origin[1] );
	current.localOrigin[2] = msg.ReadDeltaFloat( current.origin[2] );
	current.pushVelocity[0] = msg.ReadDeltaFloat( 0.0f, MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	current.pushVelocity[1] = msg.ReadDeltaFloat( 0.0f, MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	current.pushVelocity[2] = msg.ReadDeltaFloat( 0.0f, MONSTER_VELOCITY_EXPONENT_BITS, MONSTER_VELOCITY_MANTISSA_BITS );
	current.atRest = msg.ReadLong();
	current.onGround = msg.ReadBits( 1 ) != 0;
}
