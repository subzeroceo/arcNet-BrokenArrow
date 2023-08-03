
#include "../../idlib/Lib.h"
#pragma hdrstop

#include "../Game_local.h"

CLASS_DECLARATION( anPhysics, anPhysics_StaticMulti )
END_CLASS


// rjohnson: converted this from a struct to a class
idStaticPState defaultState;


/*
================
anPhysics_StaticMulti::anPhysics_StaticMulti
================
*/
anPhysics_StaticMulti::anPhysics_StaticMulti( void ) {
	self = nullptr;
	hasMaster = false;
	isOrientated = false;

	defaultState.origin.Zero();
	defaultState.axis.Identity();
	defaultState.localOrigin.Zero();
	defaultState.localAxis.Identity();

	current.SetNum( 1 );
	current[0] = defaultState;
	clipModels.SetNum( 1 );
	clipModels[0] = nullptr;
}

/*
================
anPhysics_StaticMulti::~anPhysics_StaticMulti
================
*/
anPhysics_StaticMulti::~anPhysics_StaticMulti( void ) {
	if ( self && self->GetPhysics() == this ) {
		self->SetPhysics( nullptr );
	}
	anForce::DeletePhysics( this );
	for ( int i = 0; i < clipModels.Num(); i++ ) {
		delete clipModels[i];
	}
}

/*
================
anPhysics_StaticMulti::Save
================
*/
void anPhysics_StaticMulti::Save( anSaveGame *savefile ) const {
	int i;

	savefile->WriteObject( self );

	savefile->WriteInt(current.Num());
	for  ( i = 0; i < current.Num(); i++ ) {
		savefile->WriteVec3( current[i].origin );
		savefile->WriteMat3( current[i].axis );
		savefile->WriteVec3( current[i].localOrigin );
		savefile->WriteMat3( current[i].localAxis );
	}

	savefile->WriteInt( clipModels.Num() );
	for ( i = 0; i < clipModels.Num(); i++ ) {
		savefile->WriteClipModel( clipModels[i] );
	}

	savefile->WriteBool(hasMaster);
	savefile->WriteBool(isOrientated);
}

/*
================
anPhysics_StaticMulti::Restore
================
*/
void anPhysics_StaticMulti::Restore( anRestoreGame *savefile ) {
	int i, num;

	savefile->ReadObject( reinterpret_cast<anClass *&>( self ) );

	savefile->ReadInt( num );
	current.AssureSize( num );
	for ( i = 0; i < num; i++ ) {
		savefile->ReadVec3( current[i].origin );
		savefile->ReadMat3( current[i].axis );
		savefile->ReadVec3( current[i].localOrigin );
		savefile->ReadMat3( current[i].localAxis );
	}

	savefile->ReadInt( num );
	clipModels.SetNum( num );
	for ( i = 0; i < num; i++ ) {
		savefile->ReadClipModel( clipModels[i] );
	}

	savefile->ReadBool(hasMaster);
	savefile->ReadBool(isOrientated);
}

/*
================
anPhysics_StaticMulti::SetSelf
================
*/
void anPhysics_StaticMulti::SetSelf( anEntity *e ) {
	assert( e );
	self = e;
}

/*
================
anPhysics_StaticMulti::RemoveIndex
================
*/
void anPhysics_StaticMulti::RemoveIndex( int id, bool freeClipModel ) {
	if ( id < 0 || id >= clipModels.Num() ) {
		return;
	}
	if ( clipModels[id] && freeClipModel ) {
		delete clipModels[id];
		clipModels[id] = nullptr;
	}
	clipModels.RemoveIndex( id );
	current.RemoveIndex( id );
}

/*
================
anPhysics_StaticMulti::SetClipModel
================
*/
void anPhysics_StaticMulti::SetClipModel( anClipModel *model, float density, int id, bool freeOld ) {
	int i;

	assert( self );

	if ( id >= clipModels.Num() ) {
		current.AssureSize( id+1, defaultState );
		clipModels.AssureSize( id+1, nullptr );
	}

	if ( clipModels[id] && clipModels[id] != model && freeOld ) {
		delete clipModels[id];
	}
	clipModels[id] = model;
	if ( clipModels[id] ) {


		clipModels[id]->Link( self, id, current[id].origin, current[id].axis );

	}

	for ( i = clipModels.Num() - 1; i >= 1; i-- ) {
		if ( clipModels[i] ) {
			break;
		}
	}
	current.SetNum( i+1, false );
	clipModels.SetNum( i+1, false );
}

/*
================
anPhysics_StaticMulti::GetClipModel
================
*/
anClipModel *anPhysics_StaticMulti::GetClipModel( int id ) const {
	if ( id >= 0 && id < clipModels.Num() && clipModels[id] ) {
		return clipModels[id];
	}


	return anClip::DefaultClipModel();

}

/*
================
anPhysics_StaticMulti::GetNumClipModels
================
*/
int anPhysics_StaticMulti::GetNumClipModels( void ) const {
	return clipModels.Num();
}

/*
================
anPhysics_StaticMulti::SetMass
================
*/
void anPhysics_StaticMulti::SetMass( float mass, int id ) {
}

/*
================
anPhysics_StaticMulti::GetMass
================
*/
float anPhysics_StaticMulti::GetMass( int id ) const {
	return 0.0f;
}


// bdube: Added center mass call
/*
================
anPhysics_StaticMulti::GetCenterMass

default center of mass is origin
================
*/
anVec3 anPhysics_StaticMulti::GetCenterMass ( int id ) const {
	return GetOrigin();
}


/*
================
anPhysics_StaticMulti::SetContents
================
*/
void anPhysics_StaticMulti::SetContents( int contents, int id ) {
	int i;

	if ( id >= 0 && id < clipModels.Num() ) {
		if ( clipModels[id] ) {
			clipModels[id]->SetContents( contents );
		}
	} else if ( id == -1 ) {
		for ( i = 0; i < clipModels.Num(); i++ ) {
			if ( clipModels[i] ) {
				clipModels[i]->SetContents( contents );
			}
		}
	}
}

/*
================
anPhysics_StaticMulti::GetContents
================
*/
int anPhysics_StaticMulti::GetContents( int id ) const {
	int i, contents = 0;

	if ( id >= 0 && id < clipModels.Num() ) {
		if ( clipModels[id] ) {
			contents = clipModels[id]->GetContents();
		}
	} else if ( id == -1 ) {
		for ( i = 0; i < clipModels.Num(); i++ ) {
			if ( clipModels[i] ) {
				contents |= clipModels[i]->GetContents();
			}
		}
	}
	return contents;
}

/*
================
anPhysics_StaticMulti::SetClipMask
================
*/
void anPhysics_StaticMulti::SetClipMask( int mask, int id ) {
}

/*
================
anPhysics_StaticMulti::GetClipMask
================
*/
int anPhysics_StaticMulti::GetClipMask( int id ) const {
	return 0;
}

/*
================
anPhysics_StaticMulti::GetBounds
================
*/
const anBounds &anPhysics_StaticMulti::GetBounds( int id ) const {
	int i;
	static anBounds bounds;

	if ( id >= 0 && id < clipModels.Num() ) {
		if ( clipModels[id] ) {
			return clipModels[id]->GetBounds();
		}
	}
	if ( id == -1 ) {
		bounds.Clear();
		for ( i = 0; i < clipModels.Num(); i++ ) {
			if ( clipModels[i] ) {
				bounds.AddBounds( clipModels[i]->GetAbsBounds() );
			}
		}
		for ( i = 0; i < clipModels.Num(); i++ ) {
			if ( clipModels[i] ) {
				bounds[0] -= clipModels[i]->GetOrigin();
				bounds[1] -= clipModels[i]->GetOrigin();
				break;
			}
		}
		return bounds;
	}
	return bounds_zero;
}

/*
================
anPhysics_StaticMulti::GetAbsBounds
================
*/
const anBounds &anPhysics_StaticMulti::GetAbsBounds( int id ) const {
	int i;
	static anBounds absBounds;

	if ( id >= 0 && id < clipModels.Num() ) {
		if ( clipModels[id] ) {
			return clipModels[id]->GetAbsBounds();
		}
	}
	if ( id == -1 ) {
		absBounds.Clear();
		for ( i = 0; i < clipModels.Num(); i++ ) {
			if ( clipModels[i] ) {
				absBounds.AddBounds( clipModels[i]->GetAbsBounds() );
			}
		}
		return absBounds;
	}
	return bounds_zero;
}

/*
================
anPhysics_StaticMulti::Evaluate
================
*/
bool anPhysics_StaticMulti::Evaluate( int timeStepMSec, int endTimeMSec ) {
	int i;
	anVec3 masterOrigin;
	anMat3 masterAxis;

	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		for ( i = 0; i < clipModels.Num(); i++ ) {
			current[i].origin = masterOrigin + current[i].localOrigin * masterAxis;
			if ( isOrientated ) {
				current[i].axis = current[i].localAxis * masterAxis;
			} else {
				current[i].axis = current[i].localAxis;
			}
			if ( clipModels[i] ) {


				clipModels[i]->Link( self, i, current[i].origin, current[i].axis );

			}
		}

		// FIXME: return false if master did not move
		return true;
	}
	return false;
}

/*
================
anPhysics_StaticMulti::UpdateTime
================
*/
void anPhysics_StaticMulti::UpdateTime( int endTimeMSec ) {
}

/*
================
anPhysics_StaticMulti::GetTime
================
*/
int anPhysics_StaticMulti::GetTime( void ) const {
	return 0;
}

/*
================
anPhysics_StaticMulti::GetImpactInfo
================
*/
void anPhysics_StaticMulti::GetImpactInfo( const int id, const anVec3 &point, impactInfo_t *info ) const {
	memset( info, 0, sizeof( *info ) );
}

/*
================
anPhysics_StaticMulti::ApplyImpulse
================
*/
void anPhysics_StaticMulti::ApplyImpulse( const int id, const anVec3 &point, const anVec3 &impulse ) {
}

/*
================
anPhysics_StaticMulti::AddForce
================
*/
void anPhysics_StaticMulti::AddForce( const int id, const anVec3 &point, const anVec3 &force ) {
}

/*
================
anPhysics_StaticMulti::Activate
================
*/
void anPhysics_StaticMulti::Activate( void ) {
}

/*
================
anPhysics_StaticMulti::PutToRest
================
*/
void anPhysics_StaticMulti::PutToRest( void ) {
}

/*
================
anPhysics_StaticMulti::IsAtRest
================
*/
bool anPhysics_StaticMulti::IsAtRest( void ) const {
	return true;
}

/*
================
anPhysics_StaticMulti::GetRestStartTime
================
*/
int anPhysics_StaticMulti::GetRestStartTime( void ) const {
	return 0;
}

/*
================
anPhysics_StaticMulti::IsPushable
================
*/
bool anPhysics_StaticMulti::IsPushable( void ) const {
	return false;
}


// bdube: water interraction
/*
================
anPhysics_StaticMulti::IsInWater
================
*/
bool anPhysics_StaticMulti::IsInWater ( void ) const {
	return false;
}


/*
================
anPhysics_StaticMulti::SaveState
================
*/
void anPhysics_StaticMulti::SaveState( void ) {
}

/*
================
anPhysics_StaticMulti::RestoreState
================
*/
void anPhysics_StaticMulti::RestoreState( void ) {
}

/*
================
anPhysics_StaticMulti::SetOrigin
================
*/
void anPhysics_StaticMulti::SetOrigin( const anVec3 &newOrigin, int id ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	if ( id >= 0 && id < clipModels.Num() ) {
		current[id].localOrigin = newOrigin;
		if ( hasMaster ) {
			self->GetMasterPosition( masterOrigin, masterAxis );
			current[id].origin = masterOrigin + newOrigin * masterAxis;
		} else {
			current[id].origin = newOrigin;
		}
		if ( clipModels[id] ) {


			clipModels[id]->Link( self, id, current[id].origin, current[id].axis );

		}
	} else if ( id == -1 ) {
		if ( hasMaster ) {
			self->GetMasterPosition( masterOrigin, masterAxis );
			Translate( masterOrigin + masterAxis * newOrigin - current[0].origin );
		} else {
			Translate( newOrigin - current[0].origin );
		}
	}
}

/*
================
anPhysics_StaticMulti::SetAxis
================
*/
void anPhysics_StaticMulti::SetAxis( const anMat3 &newAxis, int id ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	if ( id >= 0 && id < clipModels.Num() ) {
		current[id].localAxis = newAxis;
		if ( hasMaster && isOrientated ) {
			self->GetMasterPosition( masterOrigin, masterAxis );
			current[id].axis = newAxis * masterAxis;
		} else {
			current[id].axis = newAxis;
		}
		if ( clipModels[id] ) {


			clipModels[id]->Link( self, id, current[id].origin, current[id].axis );

		}
	} else if ( id == -1 ) {
		anMat3 axis;
		anRotation rotation;

		if ( hasMaster ) {
			self->GetMasterPosition( masterOrigin, masterAxis );
			axis = current[0].axis.Transpose() * ( newAxis * masterAxis );
		} else {
			axis = current[0].axis.Transpose() * newAxis;
		}
		rotation = axis.ToRotation();
		rotation.SetOrigin( current[0].origin );

		Rotate( rotation );
	}
}

/*
================
anPhysics_StaticMulti::Translate
================
*/
void anPhysics_StaticMulti::Translate( const anVec3 &translation, int id ) {
	int i;

	if ( id >= 0 && id < clipModels.Num() ) {
		current[id].localOrigin += translation;
		current[id].origin += translation;

		if ( clipModels[id] ) {


			clipModels[id]->Link( self, id, current[id].origin, current[id].axis );

		}
	} else if ( id == -1 ) {
		for ( i = 0; i < clipModels.Num(); i++ ) {
			current[i].localOrigin += translation;
			current[i].origin += translation;

			if ( clipModels[i] ) {


				clipModels[i]->Link( self, i, current[i].origin, current[i].axis );

			}
		}
	}
}

/*
================
anPhysics_StaticMulti::Rotate
================
*/
void anPhysics_StaticMulti::Rotate( const anRotation &rotation, int id ) {
	int i;
	anVec3 masterOrigin;
	anMat3 masterAxis;

	if ( id >= 0 && id < clipModels.Num() ) {
		current[id].origin *= rotation;
		current[id].axis *= rotation.ToMat3();

		if ( hasMaster ) {
			self->GetMasterPosition( masterOrigin, masterAxis );
			current[id].localAxis *= rotation.ToMat3();
			current[id].localOrigin = ( current[id].origin - masterOrigin ) * masterAxis.Transpose();
		} else {
			current[id].localAxis = current[id].axis;
			current[id].localOrigin = current[id].origin;
		}

		if ( clipModels[id] ) {


			clipModels[id]->Link( self, id, current[id].origin, current[id].axis );

		}
	} else if ( id == -1 ) {
		for ( i = 0; i < clipModels.Num(); i++ ) {
			current[i].origin *= rotation;
			current[i].axis *= rotation.ToMat3();

			if ( hasMaster ) {
				self->GetMasterPosition( masterOrigin, masterAxis );
				current[i].localAxis *= rotation.ToMat3();
				current[i].localOrigin = ( current[i].origin - masterOrigin ) * masterAxis.Transpose();
			} else {
				current[i].localAxis = current[i].axis;
				current[i].localOrigin = current[i].origin;
			}

			if ( clipModels[i] ) {


				clipModels[i]->Link( self, i, current[i].origin, current[i].axis );

			}
		}
	}
}

/*
================
anPhysics_StaticMulti::GetOrigin
================
*/
const anVec3 &anPhysics_StaticMulti::GetOrigin( int id ) const {
	if ( id >= 0 && id < clipModels.Num() ) {
		return current[id].origin;
	}
	if ( clipModels.Num() ) {
		return current[0].origin;
	} else {
		return vec3_origin;
	}
}

/*
================
anPhysics_StaticMulti::GetAxis
================
*/
const anMat3 &anPhysics_StaticMulti::GetAxis( int id ) const {
	if ( id >= 0 && id < clipModels.Num() ) {
		return current[id].axis;
	}
	if ( clipModels.Num() ) {
		return current[0].axis;
	} else {
		return mat3_identity;
	}
}

/*
================
anPhysics_StaticMulti::SetLinearVelocity
================
*/
void anPhysics_StaticMulti::SetLinearVelocity( const anVec3 &newLinearVelocity, int id ) {
}

/*
================
anPhysics_StaticMulti::SetAngularVelocity
================
*/
void anPhysics_StaticMulti::SetAngularVelocity( const anVec3 &newAngularVelocity, int id ) {
}

/*
================
anPhysics_StaticMulti::GetLinearVelocity
================
*/
const anVec3 &anPhysics_StaticMulti::GetLinearVelocity( int id ) const {
	return vec3_origin;
}

/*
================
anPhysics_StaticMulti::GetAngularVelocity
================
*/
const anVec3 &anPhysics_StaticMulti::GetAngularVelocity( int id ) const {
	return vec3_origin;
}

/*
================
anPhysics_StaticMulti::SetGravity
================
*/
void anPhysics_StaticMulti::SetGravity( const anVec3 &newGravity ) {
}

/*
================
anPhysics_StaticMulti::GetGravity
================
*/
const anVec3 &anPhysics_StaticMulti::GetGravity( void ) const {
	static anVec3 gravity( 0, 0, -g_gravity.GetFloat() );
	if ( gameLocal.isMultiplayer ) {
		gravity = anVec3( 0, 0, -g_mp_gravity.GetFloat() );
	}

	return gravity;
}

/*
================
anPhysics_StaticMulti::GetGravityNormal
================
*/
const anVec3 &anPhysics_StaticMulti::GetGravityNormal( void ) const {
	static anVec3 gravity( 0, 0, -1 );
	return gravity;
}

/*
================
anPhysics_StaticMulti::ClipTranslation
================
*/
void anPhysics_StaticMulti::ClipTranslation( trace_t &results, const anVec3 &translation, const anClipModel *model ) const {
	memset( &results, 0, sizeof( trace_t ) );
	gameLocal.Warning( "anPhysics_StaticMulti::ClipTranslation called" );
}

/*
================
anPhysics_StaticMulti::ClipRotation
================
*/
void anPhysics_StaticMulti::ClipRotation( trace_t &results, const anRotation &rotation, const anClipModel *model ) const {
	memset( &results, 0, sizeof( trace_t ) );
	gameLocal.Warning( "anPhysics_StaticMulti::ClipRotation called" );
}

/*
================
anPhysics_StaticMulti::ClipContents
================
*/
int anPhysics_StaticMulti::ClipContents( const anClipModel *model ) const {
	int i, contents;

	contents = 0;
	for ( i = 0; i < clipModels.Num(); i++ ) {
		if ( clipModels[i] ) {
			if ( model ) {

// ddynerman: multiple collision worlds
				contents |= gameLocal.ContentsModel( self, clipModels[i]->GetOrigin(), clipModels[i], clipModels[i]->GetAxis(), -1,
											model->GetCollisionModel(), model->GetOrigin(), model->GetAxis() );
			} else {
				contents |= gameLocal.Contents( self, clipModels[i]->GetOrigin(), clipModels[i], clipModels[i]->GetAxis(), -1, nullptr );

			}
		}
	}
	return contents;
}

/*
================
anPhysics_StaticMulti::DisableClip
================
*/
void anPhysics_StaticMulti::DisableClip( void ) {
	int i;

	for ( i = 0; i < clipModels.Num(); i++ ) {
        if ( clipModels[i] ) {
			clipModels[i]->Disable();
		}
	}
}

/*
================
anPhysics_StaticMulti::EnableClip
================
*/
void anPhysics_StaticMulti::EnableClip( void ) {
	int i;

	for ( i = 0; i < clipModels.Num(); i++ ) {
		if ( clipModels[i] ) {
			clipModels[i]->Enable();
		}
	}
}

/*
================
anPhysics_StaticMulti::UnlinkClip
================
*/
void anPhysics_StaticMulti::UnlinkClip( void ) {
	int i;

	for ( i = 0; i < clipModels.Num(); i++ ) {
        if ( clipModels[i] ) {
			clipModels[i]->Unlink();
		}
	}
}

/*
================
anPhysics_StaticMulti::LinkClip
================
*/
void anPhysics_StaticMulti::LinkClip( void ) {
	int i;

	for ( i = 0; i < clipModels.Num(); i++ ) {
		if ( clipModels[i] ) {


			clipModels[i]->Link( self, i, current[i].origin, current[i].axis );

		}
	}
}

/*
================
anPhysics_StaticMulti::EvaluateContacts
================
*/
bool anPhysics_StaticMulti::EvaluateContacts( void ) {
	return false;
}

/*
================
anPhysics_StaticMulti::GetNumContacts
================
*/
int anPhysics_StaticMulti::GetNumContacts( void ) const {
	return 0;
}

/*
================
anPhysics_StaticMulti::GetContact
================
*/
const contactInfo_t &anPhysics_StaticMulti::GetContact( int num ) const {
	static contactInfo_t info;
	memset( &info, 0, sizeof( info ) );
	return info;
}

/*
================
anPhysics_StaticMulti::ClearContacts
================
*/
void anPhysics_StaticMulti::ClearContacts( void ) {
}

/*
================
anPhysics_StaticMulti::AddContactEntity
================
*/
void anPhysics_StaticMulti::AddContactEntity( anEntity *e ) {
}

/*
================
anPhysics_StaticMulti::RemoveContactEntity
================
*/
void anPhysics_StaticMulti::RemoveContactEntity( anEntity *e ) {
}

/*
================
anPhysics_StaticMulti::HasGroundContacts
================
*/
bool anPhysics_StaticMulti::HasGroundContacts( void ) const {
	return false;
}

/*
================
anPhysics_StaticMulti::IsGroundEntity
================
*/
bool anPhysics_StaticMulti::IsGroundEntity( int entityNum ) const {
	return false;
}

/*
================
anPhysics_StaticMulti::IsGroundClipModel
================
*/
bool anPhysics_StaticMulti::IsGroundClipModel( int entityNum, int id ) const {
	return false;
}

/*
================
anPhysics_StaticMulti::SetPushed
================
*/
void anPhysics_StaticMulti::SetPushed( int deltaTime ) {
}

/*
================
anPhysics_StaticMulti::GetPushedLinearVelocity
================
*/
const anVec3 &anPhysics_StaticMulti::GetPushedLinearVelocity( const int id ) const {
	return vec3_origin;
}

/*
================
anPhysics_StaticMulti::GetPushedAngularVelocity
================
*/
const anVec3 &anPhysics_StaticMulti::GetPushedAngularVelocity( const int id ) const {
	return vec3_origin;
}

/*
================
anPhysics_StaticMulti::SetMaster
================
*/
void anPhysics_StaticMulti::SetMaster( anEntity *master, const bool orientated ) {
	int i;
	anVec3 masterOrigin;
	anMat3 masterAxis;

	if ( master ) {
		if ( !hasMaster ) {
			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			for ( i = 0; i < clipModels.Num(); i++ ) {
                current[i].localOrigin = ( current[i].origin - masterOrigin ) * masterAxis.Transpose();
				if ( orientated ) {
					current[i].localAxis = current[i].axis * masterAxis.Transpose();
				} else {
					current[i].localAxis = current[i].axis;
				}
			}
			hasMaster = true;
			isOrientated = orientated;
		}
	} else {
		if ( hasMaster ) {
			hasMaster = false;
		}
	}
}

/*
================
anPhysics_StaticMulti::GetBlockingInfo
================
*/
const trace_t *anPhysics_StaticMulti::GetBlockingInfo( void ) const {
	return nullptr;
}

/*
================
anPhysics_StaticMulti::GetBlockingEntity
================
*/
anEntity *anPhysics_StaticMulti::GetBlockingEntity( void ) const {
	return nullptr;
}

/*
================
anPhysics_StaticMulti::GetLinearEndTime
================
*/
int anPhysics_StaticMulti::GetLinearEndTime( void ) const {
	return 0;
}

/*
================
anPhysics_StaticMulti::GetAngularEndTime
================
*/
int anPhysics_StaticMulti::GetAngularEndTime( void ) const {
	return 0;
}

/*
================
anPhysics_StaticMulti::WriteToSnapshot
================
*/
void anPhysics_StaticMulti::WriteToSnapshot( anBitMsgDelta &msg ) const {
	int i;
	anCQuat quat, localQuat;

	// TODO: Check that this conditional write to delta message is OK
	msg.WriteByte( current.Num() );

	for ( i = 0; i < current.Num(); i++ ) {
		quat = current[i].axis.ToCQuat();
		localQuat = current[i].localAxis.ToCQuat();

		msg.WriteFloat( current[i].origin[0] );
		msg.WriteFloat( current[i].origin[1] );
		msg.WriteFloat( current[i].origin[2] );
		msg.WriteFloat( quat.x );
		msg.WriteFloat( quat.y );
		msg.WriteFloat( quat.z );
		msg.WriteDeltaFloat( current[i].origin[0], current[i].localOrigin[0] );
		msg.WriteDeltaFloat( current[i].origin[1], current[i].localOrigin[1] );
		msg.WriteDeltaFloat( current[i].origin[2], current[i].localOrigin[2] );
		msg.WriteDeltaFloat( quat.x, localQuat.x );
		msg.WriteDeltaFloat( quat.y, localQuat.y );
		msg.WriteDeltaFloat( quat.z, localQuat.z );
	}
}

/*
================
anPhysics_StaticMulti::ReadFromSnapshot
================
*/
void anPhysics_StaticMulti::ReadFromSnapshot( const anBitMsgDelta &msg ) {
	int i, num;
	anCQuat quat, localQuat;

	num = msg.ReadByte();
	assert( num == current.Num() );

	for ( i = 0; i < current.Num(); i++ ) {
		current[i].origin[0] = msg.ReadFloat();
		current[i].origin[1] = msg.ReadFloat();
		current[i].origin[2] = msg.ReadFloat();
		quat.x = msg.ReadFloat();
		quat.y = msg.ReadFloat();
		quat.z = msg.ReadFloat();
		current[i].localOrigin[0] = msg.ReadDeltaFloat( current[i].origin[0] );
		current[i].localOrigin[1] = msg.ReadDeltaFloat( current[i].origin[1] );
		current[i].localOrigin[2] = msg.ReadDeltaFloat( current[i].origin[2] );
		localQuat.x = msg.ReadDeltaFloat( quat.x );
		localQuat.y = msg.ReadDeltaFloat( quat.y );
		localQuat.z = msg.ReadDeltaFloat( quat.z );

		current[i].axis = quat.ToMat3();
		current[i].localAxis = localQuat.ToMat3();
	}
}
