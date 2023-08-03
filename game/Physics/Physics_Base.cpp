
#include "../../idlib/Lib.h"
#pragma hdrstop

#include "../Game_local.h"

CLASS_DECLARATION( anPhysics, anPhysics_Base )
END_CLASS

/*
================
anPhysics_Base::anPhysics_Base
================
*/
anPhysics_Base::anPhysics_Base( void ) {
	self = nullptr;
	clipMask = 0;
	SetGravity( gameLocal.GetGravity() );
	ClearContacts();
}

/*
================
anPhysics_Base::~anPhysics_Base
================
*/
anPhysics_Base::~anPhysics_Base( void ) {
	if ( self && self->GetPhysics() == this ) {
		self->SetPhysics( nullptr );
	}
	anForce::DeletePhysics( this );
	ClearContacts();
}

/*
================
anPhysics_Base::Save
================
*/
void anPhysics_Base::Save( anSaveGame *savefile ) const {
	savefile->WriteObject( self );
	savefile->WriteInt( clipMask );
	savefile->WriteVec3( gravityVector );
	savefile->WriteVec3( gravityNormal );

	savefile->WriteInt( contacts.Num() );
	for ( int i = 0; i < contacts.Num(); i++ ) {
		savefile->WriteContactInfo( contacts[i] );
	}

	savefile->WriteInt( contactEntities.Num() );
	for ( int i = 0; i < contactEntities.Num(); i++ ) {
		contactEntities[i].Save( savefile );
	}
}

/*
================
anPhysics_Base::Restore
================
*/
void anPhysics_Base::Restore( anRestoreGame *savefile ) {
	int num;

	savefile->ReadObject( reinterpret_cast<anClass *&>( self ) );
	savefile->ReadInt( clipMask );
	savefile->ReadVec3( gravityVector );
	savefile->ReadVec3( gravityNormal );

	savefile->ReadInt( num );
	contacts.SetNum( num );
	for ( int i = 0; i < contacts.Num(); i++ ) {
		savefile->ReadContactInfo( contacts[i] );
	}

	savefile->ReadInt( num );
	contactEntities.SetNum( num );
	for ( int i = 0; i < contactEntities.Num(); i++ ) {
		contactEntities[i].Restore( savefile );
	}
}

/*
================
anPhysics_Base::SetSelf
================
*/
void anPhysics_Base::SetSelf( anEntity *e ) {
	assert( e );
	self = e;
}

/*
================
anPhysics_Base::SetClipModel
================
*/
void anPhysics_Base::SetClipModel( anClipModel *model, float density, int id, bool freeOld ) {
}

/*
================
anPhysics_Base::GetClipModel
================
*/
anClipModel *anPhysics_Base::GetClipModel( int id ) const {
	return nullptr;
}

/*
================
anPhysics_Base::GetNumClipModels
================
*/
int anPhysics_Base::GetNumClipModels( void ) const {
	return 0;
}

/*
================
anPhysics_Base::SetMass
================
*/
void anPhysics_Base::SetMass( float mass, int id ) {
}

/*
================
anPhysics_Base::GetMass
================
*/
float anPhysics_Base::GetMass( int id ) const {
	return 0.0f;
}


// bdube: Added center mass call
/*
================
anPhysics_Base::GetCenterMass

default center of mass is origin
================
*/
anVec3 anPhysics_Base::GetCenterMass( int id ) const {
	return GetOrigin();
}


/*
================
anPhysics_Base::SetContents
================
*/
void anPhysics_Base::SetContents( int contents, int id ) {
}

/*
================
anPhysics_Base::SetClipMask
================
*/
int anPhysics_Base::GetContents( int id ) const {
	return 0;
}

/*
================
anPhysics_Base::SetClipMask
================
*/
void anPhysics_Base::SetClipMask( int mask, int id ) {
	clipMask = mask;
}

/*
================
anPhysics_Base::GetClipMask
================
*/
int anPhysics_Base::GetClipMask( int id ) const {
	return clipMask;
}

/*
================
anPhysics_Base::GetBounds
================
*/
const anBounds &anPhysics_Base::GetBounds( int id ) const {
	return bounds_zero;
}

/*
================
anPhysics_Base::GetAbsBounds
================
*/
const anBounds &anPhysics_Base::GetAbsBounds( int id ) const {
	return bounds_zero;
}

/*
================
anPhysics_Base::Evaluate
================
*/
bool anPhysics_Base::Evaluate( int timeStepMSec, int endTimeMSec ) {
	return false;
}

/*
================
anPhysics_Base::UpdateTime
================
*/
void anPhysics_Base::UpdateTime( int endTimeMSec ) {
}

/*
================
anPhysics_Base::GetTime
================
*/
int anPhysics_Base::GetTime( void ) const {
	return 0;
}

/*
================
anPhysics_Base::GetImpactInfo
================
*/
void anPhysics_Base::GetImpactInfo( const int id, const anVec3 &point, impactInfo_t *info ) const {
	memset( info, 0, sizeof(* info) );
}

/*
================
anPhysics_Base::ApplyImpulse
================
*/
void anPhysics_Base::ApplyImpulse( const int id, const anVec3 &point, const anVec3 &impulse ) {
}

/*
================
anPhysics_Base::AddForce
================
*/
void anPhysics_Base::AddForce( const int id, const anVec3 &point, const anVec3 &force ) {
}

/*
================
anPhysics_Base::Activate
================
*/
void anPhysics_Base::Activate( void ) {
}

/*
================
anPhysics_Base::PutToRest
================
*/
void anPhysics_Base::PutToRest( void ) {
}

/*
================
anPhysics_Base::IsAtRest
================
*/
bool anPhysics_Base::IsAtRest( void ) const {
	return true;
}

/*
================
anPhysics_Base::GetRestStartTime
================
*/
int anPhysics_Base::GetRestStartTime( void ) const {
	return 0;
}

/*
================
anPhysics_Base::IsPushable
================
*/
bool anPhysics_Base::IsPushable( void ) const {
	return true;
}

/*
================
anPhysics_Base::IsInWater
================
*/
bool anPhysics_Base::IsInWater( void ) const {
	return false;
}

/*
================
anPhysics_Base::SaveState
================
*/
void anPhysics_Base::SaveState( void ) {
}

/*
================
anPhysics_Base::RestoreState
================
*/
void anPhysics_Base::RestoreState( void ) {
}

/*
================
anPhysics_Base::SetOrigin
================
*/
void anPhysics_Base::SetOrigin( const anVec3 &newOrigin, int id ) {
}

/*
================
anPhysics_Base::SetAxis
================
*/
void anPhysics_Base::SetAxis( const anMat3 &newAxis, int id ) {
}

/*
================
anPhysics_Base::Translate
================
*/
void anPhysics_Base::Translate( const anVec3 &translation, int id ) {
}

/*
================
anPhysics_Base::Rotate
================
*/
void anPhysics_Base::Rotate( const anRotation &rotation, int id ) {
}

/*
================
anPhysics_Base::GetOrigin
================
*/
const anVec3 &anPhysics_Base::GetOrigin( int id ) const {
	return vec3_origin;
}

/*
================
anPhysics_Base::GetAxis
================
*/
const anMat3 &anPhysics_Base::GetAxis( int id ) const {
	return mat3_identity;
}

/*
================
anPhysics_Base::SetLinearVelocity
================
*/
void anPhysics_Base::SetLinearVelocity( const anVec3 &newLinearVelocity, int id ) {
}

/*
================
anPhysics_Base::SetAngularVelocity
================
*/
void anPhysics_Base::SetAngularVelocity( const anVec3 &newAngularVelocity, int id ) {
}

/*
================
anPhysics_Base::GetLinearVelocity
================
*/
const anVec3 &anPhysics_Base::GetLinearVelocity( int id ) const {
	return vec3_origin;
}

/*
================
anPhysics_Base::GetAngularVelocity
================
*/
const anVec3 &anPhysics_Base::GetAngularVelocity( int id ) const {
	return vec3_origin;
}

/*
================
anPhysics_Base::SetGravity
================
*/
void anPhysics_Base::SetGravity( const anVec3 &newGravity ) {
	gravityVector = newGravity;
	gravityNormal = newGravity;
	gravityNormal.Normalize();
}

/*
================
anPhysics_Base::GetGravity
================
*/
const anVec3 &anPhysics_Base::GetGravity( void ) const {
	return gravityVector;
}

/*
================
anPhysics_Base::GetGravityNormal
================
*/
const anVec3 &anPhysics_Base::GetGravityNormal( void ) const {
	return gravityNormal;
}

/*
================
anPhysics_Base::ClipTranslation
================
*/
void anPhysics_Base::ClipTranslation( trace_t &results, const anVec3 &translation, const anClipModel *model ) const {
	memset( &results, 0, sizeof( trace_t ) );
}

/*
================
anPhysics_Base::ClipRotation
================
*/
void anPhysics_Base::ClipRotation( trace_t &results, const anRotation &rotation, const anClipModel *model ) const {
	memset( &results, 0, sizeof( trace_t ) );
}

/*
================
anPhysics_Base::ClipContents
================
*/
int anPhysics_Base::ClipContents( const anClipModel *model ) const {
	return 0;
}

/*
================
anPhysics_Base::DisableClip
================
*/
void anPhysics_Base::DisableClip( void ) {
}

/*
================
anPhysics_Base::EnableClip
================
*/
void anPhysics_Base::EnableClip( void ) {
}

/*
================
anPhysics_Base::UnlinkClip
================
*/
void anPhysics_Base::UnlinkClip( void ) {
}

/*
================
anPhysics_Base::LinkClip
================
*/
void anPhysics_Base::LinkClip( void ) {
}

/*
================
anPhysics_Base::EvaluateContacts
================
*/
bool anPhysics_Base::EvaluateContacts( void ) {
	return false;
}

/*
================
anPhysics_Base::GetNumContacts
================
*/
int anPhysics_Base::GetNumContacts( void ) const {
	return contacts.Num();
}

/*
================
anPhysics_Base::GetContact
================
*/
const contactInfo_t &anPhysics_Base::GetContact( int num ) const {
	return contacts[num];
}

/*
================
anPhysics_Base::ClearContacts
================
*/
void anPhysics_Base::ClearContacts( void ) {
	for ( int i = 0; i < contacts.Num(); i++ ) {
		anEntity *ent = gameLocal.entities[ contacts[i].entityNum ];
		if ( ent ) {
			ent->RemoveContactEntity( self );
		}
	}
	contacts.SetNum( 0, false );
}

/*
================
anPhysics_Base::AddContactEntity
================
*/
void anPhysics_Base::AddContactEntity( anEntity *e ) {
	bool found = false;

	for ( int i = 0; i < contactEntities.Num(); i++ ) {
		anEntity *ent = contactEntities[i].GetEntity();
		if ( ent == nullptr ) {
			contactEntities.RemoveIndex( i-- );
		}
		if ( ent == e ) {
			found = true;
		}
	}
	if ( !found ) {
		contactEntities.Alloc() = e;
	}
}

/*
================
anPhysics_Base::RemoveContactEntity
================
*/
void anPhysics_Base::RemoveContactEntity( anEntity *e ) {
	for ( int i = 0; i < contactEntities.Num(); i++ ) {
		anEntity *ent = contactEntities[i].GetEntity();
		if ( !ent ) {
			contactEntities.RemoveIndex( i-- );
			continue;
		}
		if ( ent == e ) {
			contactEntities.RemoveIndex( i-- );
			return;
		}
	}
}

/*
================
anPhysics_Base::GetContactNormal
================
*/
const anVec3 anPhysics_Base::GetContactNormal() const {
	anVec3 normal( vec3_zero );

	for ( int ix = 0; ix < GetNumContacts(); ++ix ) {
		normal += GetContact( ix ).normal;
	}

	return normal.ToNormal();
}

/*
================
anPhysics_Base::GetContactNormal
================
*/
const anVec3 anPhysics_Base::GetGroundContactNormal() const {
	anVec3 normal( vec3_zero );

	for ( int ix = 0; ix < GetNumContacts(); ++ix ) {
		if ( GetContact(ix).normal * -gravityNormal > 0.0f ) {
			normal += GetContact( ix ).normal;
		}
	}

	return normal.ToNormal();
}

/*
================
anPhysics_Base::HasGroundContacts
================
*/
bool anPhysics_Base::HasGroundContacts( void ) const {
	for ( int i = 0; i < contacts.Num(); i++ ) {
		if ( contacts[i].normal * -gravityNormal > 0.0f ) {
			return true;
		}
	}
	return false;
}

/*
================
anPhysics_Base::IsGroundEntity
================
*/
bool anPhysics_Base::IsGroundEntity( int entityNum ) const {
	for ( int i = 0; i < contacts.Num(); i++ ) {
		if ( contacts[i].entityNum == entityNum && ( contacts[i].normal * -gravityNormal > 0.0f ) ) {
			return true;
		}
	}
	return false;
}

/*
================
anPhysics_Base::IsGroundClipModel
================
*/
bool anPhysics_Base::IsGroundClipModel( int entityNum, int id ) const {
	for ( int i = 0; i < contacts.Num(); i++ ) {
		if ( contacts[i].entityNum == entityNum && contacts[i].id == id && ( contacts[i].normal * -gravityNormal > 0.0f ) ) {
			return true;
		}
	}
	return false;
}

/*
================
anPhysics_Base::SetPushed
================
*/
void anPhysics_Base::SetPushed( int deltaTime ) {
}

/*
================
anPhysics_Base::GetPushedLinearVelocity
================
*/
const anVec3 &anPhysics_Base::GetPushedLinearVelocity( const int id ) const {
	return vec3_origin;
}

/*
================
anPhysics_Base::GetPushedAngularVelocity
================
*/
const anVec3 &anPhysics_Base::GetPushedAngularVelocity( const int id ) const {
	return vec3_origin;
}

/*
================
anPhysics_Base::SetMaster
================
*/
void anPhysics_Base::SetMaster( anEntity *master, const bool orientated ) {
}

/*
================
anPhysics_Base::GetBlockingInfo
================
*/
const trace_t *anPhysics_Base::GetBlockingInfo( void ) const {
	return nullptr;
}

/*
================
anPhysics_Base::GetBlockingEntity
================
*/
anEntity *anPhysics_Base::GetBlockingEntity( void ) const {
	return nullptr;
}

/*
================
anPhysics_Base::GetLinearEndTime
================
*/
int anPhysics_Base::GetLinearEndTime( void ) const {
	return 0;
}

/*
================
anPhysics_Base::GetAngularEndTime
================
*/
int anPhysics_Base::GetAngularEndTime( void ) const {
	return 0;
}

/*
================
anPhysics_Base::AddGroundContacts
================
*/
void anPhysics_Base::AddGroundContacts( const anClipModel *clipModel ) {
	anVec6 dir;
	int index = contacts.Num();
	contacts.SetNum( index + 10, false );

	dir.SubVec3(0) = gravityNormal;
	dir.SubVec3( 1 ) = vec3_origin;


	int num = gameLocal.Contacts( self, &contacts[index], 10, clipModel->GetOrigin(),
					dir, CONTACT_EPSILON, clipModel, clipModel->GetAxis(), clipMask, self );

	contacts.SetNum( index + num, false );
}

/*
================
anPhysics_Base::AddContactEntitiesForContacts
================
*/
void anPhysics_Base::AddContactEntitiesForContacts( void ) {
	for ( int  i = 0; i < contacts.Num(); i++ ) {
		anEntity *ent = gameLocal.entities[ contacts[i].entityNum ];
		if ( ent && ent != self ) {
			ent->AddContactEntity( self );
		}
	}
}

/*
================
anPhysics_Base::ActivateContactEntities
================
*/
void anPhysics_Base::ActivateContactEntities( void ) {
	for ( int i = 0; i < contactEntities.Num(); i++ ) {
		anEntity *ent = contactEntities[i].GetEntity();
		if ( ent ) {
			ent->ActivatePhysics( self );
		} else {
			contactEntities.RemoveIndex( i-- );
		}
	}
}

/*
================
anPhysics_Base::IsOutsideWorld
================
*/
bool anPhysics_Base::IsOutsideWorld( void ) const {
	if ( !gameLocal.GetWorldBounds( self ).Expand( 128.0f ).IntersectsBounds( GetAbsBounds() ) ) {
		return true;
	}
	return false;
}

/*
================
anPhysics_Base::DrawVelocity

leaving here marked out but still alive til new code is tested
================
*/
void anPhysics_Base::DrawVelocityOld( int id, float linearScale, float angularScale ) const {
	anVec3 dir = GetLinearVelocity( id );
	dir *= linearScale;
	if ( dir.LengthSqr() > Square( 0.1f ) ) {
		dir.Truncate( 10.0f );
		anVec3 org = GetOrigin( id );
		gameRenderWorld->DebugArrow( colorRed, org, org + dir, 1 );
	}

	dir = GetAngularVelocity( id );
	float length = dir.Normalize();
	length *= angularScale;
	if ( length > 0.1f ) {
		if ( length < 60.0f ) {
			length = 60.0f;
		} else if ( length > 360.0f ) {
			length = 360.0f;
		}
		anMat3 axis = GetAxis( id );
		anVec3 vec = axis[2];
		if ( anMath::Fabs( dir * vec ) > 0.99f ) {
			vec = axis[0];
		}
		vec -= vec * dir * vec;
		vec.Normalize();
		vec *= 4.0f;
		anVec3 start = org + vec;
		for ( float a = 20.0f; a < length; a += 20.0f ) {
			anVec3 end = org + anRotation( vec3_origin, dir, -a ).ToMat3() * vec;
			gameRenderWorld->DebugLine( colorBlue, start, end, 1 );
			start = end;
		}
		end = org + anRotation( vec3_origin, dir, -length ).ToMat3() * vec;
		gameRenderWorld->DebugArrow( colorBlue, start, end, 1 );
	}
}

/*
================
anPhysics_Base::DrawVelocity
================
*/
void anPhysics_Base::DrawVelocity( int id, float linearScale, float angularScale ) const {
    anVec3 linearVelocity = GetLinearVelocity( id );
    anVec3 angularVelocity = GetAngularVelocity( id );
    anVec3 origin = GetOrigin( id );
    anMat3 axis = GetAxis( id );

		// Draw linear velocity
	float linearVelocityLength = linearVelocity.Length() * linearScale;

	if ( linearVelocityLength > 0.1f ) {
		linearVelocity.Truncate( 10.0f );
		gameRenderWorld->DebugArrow( colorRed, origin, origin + linearVelocity, 1 );
		// Draw angular velocity
		float angularVelocityLength = angularVelocity.Normalize() * angularScale;
		if ( angularVelocityLength > 0.1f ) {
			if ( angularVelocityLength < 60.0f )  {
				angularVelocityLength = 60.0f;
			} else if ( angularVelocityLength > 360.0f ) {
				angularVelocityLength = 360.0f;
			}

			anVec3 vec = axis[2];

			if ( anMath::Fabs( angularVelocity * vec ) > 0.99f ) {
				vec = axis[0];
			}

			vec -= vec * angularVelocity * vec;
			vec.Normalize();
			vec *= 4.0f;

			anVec3 start = origin + vec;
			anVec3 end;
			for ( float a = 20.0f; a < angularVelocityLength; a += 20.0f ) {
				end = origin + anRotation( vec3_origin, angularVelocity, -a ).ToMat3() * vec;
				gameRenderWorld->DebugLine( colorBlue, start, end, 1 );
				start = end;
			}
			end = origin + anRotation( vec3_origin, angularVelocity, -angularVelocityLength ).ToMat3() * vec;
			gameRenderWorld->DebugArrow( colorBlue, start, end, 1 );
		}
	}
}

/*
================
anPhysics_Base::WriteToSnapshot
================
*/
void anPhysics_Base::WriteToSnapshot( anBitMsgDelta &msg ) const {
}

/*
================
anPhysics_Base::ReadFromSnapshot
================
*/
void anPhysics_Base::ReadFromSnapshot( const anBitMsgDelta &msg ) {
}
