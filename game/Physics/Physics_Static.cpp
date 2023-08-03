
#include "../../idlib/Lib.h"
#pragma hdrstop

#include "../Game_local.h"

CLASS_DECLARATION( anPhysics, anPhysics_Static )
END_CLASS

/*
================
anPhysics_Static::anPhysics_Static
================
*/
anPhysics_Static::anPhysics_Static( void ) {
	self = nullptr;
	clipModel = nullptr;
	current.origin.Zero();
	current.axis.Identity();
	current.localOrigin.Zero();
	current.localAxis.Identity();
	hasMaster = false;
	isOrientated = false;
}

/*
================
anPhysics_Static::~anPhysics_Static
================
*/
anPhysics_Static::~anPhysics_Static( void ) {
	if ( self && self->GetPhysics() == this ) {
		self->SetPhysics( nullptr );
	}
	anForce::DeletePhysics( this );
	if ( clipModel ) {
		delete clipModel;
	}
}

/*
================
anPhysics_Static::Save
================
*/
void anPhysics_Static::Save( anSaveGame *savefile ) const {
	savefile->WriteObject( self );

	savefile->WriteVec3( current.origin );
	savefile->WriteMat3( current.axis );
	savefile->WriteVec3( current.localOrigin );
	savefile->WriteMat3( current.localAxis );
	savefile->WriteClipModel( clipModel );

	savefile->WriteBool( hasMaster );
	savefile->WriteBool( isOrientated );
}

/*
================
anPhysics_Static::Restore
================
*/
void anPhysics_Static::Restore( anRestoreGame *savefile ) {
	savefile->ReadObject( reinterpret_cast<anClass *&>( self ) );

	savefile->ReadVec3( current.origin );
	savefile->ReadMat3( current.axis );
	savefile->ReadVec3( current.localOrigin );
	savefile->ReadMat3( current.localAxis );
	savefile->ReadClipModel( clipModel );

	savefile->ReadBool( hasMaster );
	savefile->ReadBool( isOrientated );
}

/*
================
anPhysics_Static::SetSelf
================
*/
void anPhysics_Static::SetSelf( anEntity *e ) {
	assert( e );
	self = e;
}

/*
================
anPhysics_Static::SetClipModel
================
*/
void anPhysics_Static::SetClipModel( anClipModel *model, float density, int id, bool freeOld ) {
	assert( self );

	if ( clipModel && clipModel != model && freeOld ) {
		delete clipModel;
	}
	clipModel = model;
	if ( clipModel ) {


		clipModel->Link( self, 0, current.origin, current.axis );

	}
}

/*
================
anPhysics_Static::GetClipModel
================
*/
anClipModel *anPhysics_Static::GetClipModel( int id ) const {
	if ( clipModel ) {
		return clipModel;
	}


	return anClip::DefaultClipModel();

}

/*
================
anPhysics_Static::GetNumClipModels
================
*/
int anPhysics_Static::GetNumClipModels( void ) const {
	return ( clipModel != nullptr );
}

/*
================
anPhysics_Static::SetMass
================
*/
void anPhysics_Static::SetMass( float mass, int id ) {
}

/*
================
anPhysics_Static::GetMass
================
*/
float anPhysics_Static::GetMass( int id ) const {
	return 0.0f;
}


// bdube: Added center mass call
/*
================
anPhysics_Static::GetCenterMass

default center of mass is origin
================
*/
anVec3 anPhysics_Static::GetCenterMass ( int id ) const {
	return GetOrigin();
}


/*
================
anPhysics_Static::SetContents
================
*/
void anPhysics_Static::SetContents( int contents, int id ) {
	if ( clipModel ) {
		clipModel->SetContents( contents );
	}
}

/*
================
anPhysics_Static::GetContents
================
*/
int anPhysics_Static::GetContents( int id ) const {
	if ( clipModel ) {
		return clipModel->GetContents();
	}
	return 0;
}

/*
================
anPhysics_Static::SetClipMask
================
*/
void anPhysics_Static::SetClipMask( int mask, int id ) {
}

/*
================
anPhysics_Static::GetClipMask
================
*/
int anPhysics_Static::GetClipMask( int id ) const {
	return 0;
}

/*
================
anPhysics_Static::GetBounds
================
*/
const anBounds &anPhysics_Static::GetBounds( int id ) const {
	if ( clipModel ) {
		return clipModel->GetBounds();
	}
	return bounds_zero;
}

/*
================
anPhysics_Static::GetAbsBounds
================
*/
const anBounds &anPhysics_Static::GetAbsBounds( int id ) const {
	static anBounds absBounds;

	if ( clipModel ) {
		return clipModel->GetAbsBounds();
	}
	absBounds[0] = absBounds[1] = current.origin;
	return absBounds;
}

/*
================
anPhysics_Static::Evaluate
================
*/
bool anPhysics_Static::Evaluate( int timeStepMSec, int endTimeMSec ) {

// bdube: draw bbox
	if ( hasMaster ) {
		anVec3 masterOrigin;
		anVec3 oldOrigin;
		anMat3 masterAxis;
		anMat3 oldAxis;


		oldOrigin = current.origin;
		oldAxis = current.axis;

		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + current.localOrigin * masterAxis;
		if ( isOrientated ) {
			current.axis = current.localAxis * masterAxis;
		} else {
			current.axis = current.localAxis;
		}
		if ( clipModel ) {


			clipModel->Link( self, 0, current.origin, current.axis );

		}

		return ( current.origin != oldOrigin || current.axis != oldAxis );
	}
	return false;
}

/*
================
anPhysics_Static::UpdateTime
================
*/
void anPhysics_Static::UpdateTime( int endTimeMSec ) {
}

/*
================
anPhysics_Static::GetTime
================
*/
int anPhysics_Static::GetTime( void ) const {
	return 0;
}

/*
================
anPhysics_Static::GetImpactInfo
================
*/
void anPhysics_Static::GetImpactInfo( const int id, const anVec3 &point, impactInfo_t *info ) const {
	memset( info, 0, sizeof( *info ) );
}

/*
================
anPhysics_Static::ApplyImpulse
================
*/
void anPhysics_Static::ApplyImpulse( const int id, const anVec3 &point, const anVec3 &impulse ) {
}

/*
================
anPhysics_Static::AddForce
================
*/
void anPhysics_Static::AddForce( const int id, const anVec3 &point, const anVec3 &force ) {
}

/*
================
anPhysics_Static::Activate
================
*/
void anPhysics_Static::Activate( void ) {
}

/*
================
anPhysics_Static::PutToRest
================
*/
void anPhysics_Static::PutToRest( void ) {
}

/*
================
anPhysics_Static::IsAtRest
================
*/
bool anPhysics_Static::IsAtRest( void ) const {
	return true;
}

/*
================
anPhysics_Static::GetRestStartTime
================
*/
int anPhysics_Static::GetRestStartTime( void ) const {
	return 0;
}

/*
================
anPhysics_Static::IsPushable
================
*/
bool anPhysics_Static::IsPushable( void ) const {
	return false;
}


// bdube: water interraction
/*
================
anPhysics_Static::IsInWater
================
*/
bool anPhysics_Static::IsInWater ( void ) const {
	return false;
}


/*
================
anPhysics_Static::SaveState
================
*/
void anPhysics_Static::SaveState( void ) {
}

/*
================
anPhysics_Static::RestoreState
================
*/
void anPhysics_Static::RestoreState( void ) {
}

/*
================
anPhysics_Static::SetOrigin
================
*/
void anPhysics_Static::SetOrigin( const anVec3 &newOrigin, int id ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	current.localOrigin = newOrigin;

	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + newOrigin * masterAxis;
	} else {
		current.origin = newOrigin;
	}

	if ( clipModel ) {


		clipModel->Link( self, 0, current.origin, current.axis );

	}
}

/*
================
anPhysics_Static::SetAxis
================
*/
void anPhysics_Static::SetAxis( const anMat3 &newAxis, int id ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	current.localAxis = newAxis;

	if ( hasMaster && isOrientated ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.axis = newAxis * masterAxis;
	} else {
		current.axis = newAxis;
	}

	if ( clipModel ) {


		clipModel->Link( self, 0, current.origin, current.axis );

	}
}

/*
================
anPhysics_Static::Translate
================
*/
void anPhysics_Static::Translate( const anVec3 &translation, int id ) {
	current.localOrigin += translation;
	current.origin += translation;

	if ( clipModel ) {


		clipModel->Link( self, 0, current.origin, current.axis );

	}
}

/*
================
anPhysics_Static::Rotate
================
*/
void anPhysics_Static::Rotate( const anRotation &rotation, int id ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	current.origin *= rotation;
	current.axis *= rotation.ToMat3();

	if ( hasMaster ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.localAxis *= rotation.ToMat3();
		current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
	} else {
		current.localAxis = current.axis;
		current.localOrigin = current.origin;
	}

	if ( clipModel ) {


		clipModel->Link( self, 0, current.origin, current.axis );

	}
}

/*
================
anPhysics_Static::GetOrigin
================
*/
const anVec3 &anPhysics_Static::GetOrigin( int id ) const {
	return current.origin;
}

/*
================
anPhysics_Static::GetAxis
================
*/
const anMat3 &anPhysics_Static::GetAxis( int id ) const {
	return current.axis;
}

/*
================
anPhysics_Static::SetLinearVelocity
================
*/
void anPhysics_Static::SetLinearVelocity( const anVec3 &newLinearVelocity, int id ) {
}

/*
================
anPhysics_Static::SetAngularVelocity
================
*/
void anPhysics_Static::SetAngularVelocity( const anVec3 &newAngularVelocity, int id ) {
}

/*
================
anPhysics_Static::GetLinearVelocity
================
*/
const anVec3 &anPhysics_Static::GetLinearVelocity( int id ) const {
	return vec3_origin;
}

/*
================
anPhysics_Static::GetAngularVelocity
================
*/
const anVec3 &anPhysics_Static::GetAngularVelocity( int id ) const {
	return vec3_origin;
}

/*
================
anPhysics_Static::SetGravity
================
*/
void anPhysics_Static::SetGravity( const anVec3 &newGravity ) {
}

/*
================
anPhysics_Static::GetGravity
================
*/
const anVec3 &anPhysics_Static::GetGravity( void ) const {
	static anVec3 gravity( 0, 0, -g_gravity.GetFloat() );
	if ( gameLocal.isMultiplayer ) {
		gravity = anVec3( 0, 0, -g_mp_gravity.GetFloat() );
	}

	return gravity;
}

/*
================
anPhysics_Static::GetGravityNormal
================
*/
const anVec3 &anPhysics_Static::GetGravityNormal( void ) const {
	static anVec3 gravity( 0, 0, -1 );
	return gravity;
}

/*
================
anPhysics_Static::ClipTranslation
================
*/
void anPhysics_Static::ClipTranslation( trace_t &results, const anVec3 &translation, const anClipModel *model ) const {
	if ( model ) {


		gameLocal.TranslationModel( self, results, current.origin, current.origin + translation,
			clipModel, current.axis, MASK_SOLID, model->GetCollisionModel(), model->GetOrigin(), model->GetAxis() );
	} else {
		gameLocal.Translation( self, results, current.origin, current.origin + translation,
			clipModel, current.axis, MASK_SOLID, self );

	}
}

/*
================
anPhysics_Static::ClipRotation
================
*/
void anPhysics_Static::ClipRotation( trace_t &results, const anRotation &rotation, const anClipModel *model ) const {
	if ( model ) {


		gameLocal.RotationModel( self, results, current.origin, rotation,
			clipModel, current.axis, MASK_SOLID, model->GetCollisionModel(), model->GetOrigin(), model->GetAxis() );
	} else {
		gameLocal.Rotation( self, results, current.origin, rotation, clipModel, current.axis, MASK_SOLID, self );
	}

}

/*
================
anPhysics_Static::ClipContents
================
*/
int anPhysics_Static::ClipContents( const anClipModel *model ) const {
	if ( clipModel ) {
		if ( model ) {

// ddynerman: multiple collision worlds
			return gameLocal.ContentsModel( self, clipModel->GetOrigin(), clipModel, clipModel->GetAxis(), -1,
				model->GetCollisionModel(), model->GetOrigin(), model->GetAxis() );
		} else {
			return gameLocal.Contents( self, clipModel->GetOrigin(), clipModel, clipModel->GetAxis(), -1, nullptr );

		}
	}
	return 0;
}

/*
================
anPhysics_Static::DisableClip
================
*/
void anPhysics_Static::DisableClip( void ) {
	if ( clipModel ) {
		clipModel->Disable();
	}
}

/*
================
anPhysics_Static::EnableClip
================
*/
void anPhysics_Static::EnableClip( void ) {
	if ( clipModel ) {
		clipModel->Enable();
	}
}

/*
================
anPhysics_Static::UnlinkClip
================
*/
void anPhysics_Static::UnlinkClip( void ) {
	if ( clipModel ) {
		clipModel->Unlink();
	}
}

/*
================
anPhysics_Static::LinkClip
================
*/
void anPhysics_Static::LinkClip( void ) {
	if ( clipModel ) {


		clipModel->Link( self, 0, current.origin, current.axis );

	}
}

/*
================
anPhysics_Static::EvaluateContacts
================
*/
bool anPhysics_Static::EvaluateContacts( void ) {
	return false;
}

/*
================
anPhysics_Static::GetNumContacts
================
*/
int anPhysics_Static::GetNumContacts( void ) const {
	return 0;
}

/*
================
anPhysics_Static::GetContact
================
*/
const contactInfo_t &anPhysics_Static::GetContact( int num ) const {
	static contactInfo_t info;
	memset( &info, 0, sizeof( info ) );
	return info;
}

/*
================
anPhysics_Static::ClearContacts
================
*/
void anPhysics_Static::ClearContacts( void ) {
}

/*
================
anPhysics_Static::AddContactEntity
================
*/
void anPhysics_Static::AddContactEntity( anEntity *e ) {
}

/*
================
anPhysics_Static::RemoveContactEntity
================
*/
void anPhysics_Static::RemoveContactEntity( anEntity *e ) {
}

/*
================
anPhysics_Static::HasGroundContacts
================
*/
bool anPhysics_Static::HasGroundContacts( void ) const {
	return false;
}

/*
================
anPhysics_Static::IsGroundEntity
================
*/
bool anPhysics_Static::IsGroundEntity( int entityNum ) const {
	return false;
}

/*
================
anPhysics_Static::IsGroundClipModel
================
*/
bool anPhysics_Static::IsGroundClipModel( int entityNum, int id ) const {
	return false;
}

/*
================
anPhysics_Static::SetPushed
================
*/
void anPhysics_Static::SetPushed( int deltaTime ) {
}

/*
================
anPhysics_Static::GetPushedLinearVelocity
================
*/
const anVec3 &anPhysics_Static::GetPushedLinearVelocity( const int id ) const {
	return vec3_origin;
}

/*
================
anPhysics_Static::GetPushedAngularVelocity
================
*/
const anVec3 &anPhysics_Static::GetPushedAngularVelocity( const int id ) const {
	return vec3_origin;
}

/*
================
anPhysics_Static::SetMaster
================
*/
void anPhysics_Static::SetMaster( anEntity *master, const bool orientated ) {
	anVec3 masterOrigin;
	anMat3 masterAxis;

	if ( master ) {
		if ( !hasMaster ) {
			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
			if ( orientated ) {
				current.localAxis = current.axis * masterAxis.Transpose();
			} else {
				current.localAxis = current.axis;
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
anPhysics_Static::GetBlockingInfo
================
*/
const trace_t *anPhysics_Static::GetBlockingInfo( void ) const {
	return nullptr;
}

/*
================
anPhysics_Static::GetBlockingEntity
================
*/
anEntity *anPhysics_Static::GetBlockingEntity( void ) const {
	return nullptr;
}

/*
================
anPhysics_Static::GetLinearEndTime
================
*/
int anPhysics_Static::GetLinearEndTime( void ) const {
	return 0;
}

/*
================
anPhysics_Static::GetAngularEndTime
================
*/
int anPhysics_Static::GetAngularEndTime( void ) const {
	return 0;
}

/*
================
anPhysics_Static::WriteToSnapshot
================
*/
void anPhysics_Static::WriteToSnapshot( anBitMsgDelta &msg ) const {
	anCQuat quat, localQuat;

	quat = current.axis.ToCQuat();
	localQuat = current.localAxis.ToCQuat();

	msg.WriteFloat( current.origin[0] );
	msg.WriteFloat( current.origin[1] );
	msg.WriteFloat( current.origin[2] );
	msg.WriteFloat( quat.x );
	msg.WriteFloat( quat.y );
	msg.WriteFloat( quat.z );
	msg.WriteDeltaFloat( current.origin[0], current.localOrigin[0] );
	msg.WriteDeltaFloat( current.origin[1], current.localOrigin[1] );
	msg.WriteDeltaFloat( current.origin[2], current.localOrigin[2] );
	msg.WriteDeltaFloat( quat.x, localQuat.x );
	msg.WriteDeltaFloat( quat.y, localQuat.y );
	msg.WriteDeltaFloat( quat.z, localQuat.z );
}

/*
================
anPhysics_Base::ReadFromSnapshot
================
*/
void anPhysics_Static::ReadFromSnapshot( const anBitMsgDelta &msg ) {
	anCQuat quat, localQuat;

	current.origin[0] = msg.ReadFloat();
	current.origin[1] = msg.ReadFloat();
	current.origin[2] = msg.ReadFloat();
	quat.x = msg.ReadFloat();
	quat.y = msg.ReadFloat();
	quat.z = msg.ReadFloat();
	current.localOrigin[0] = msg.ReadDeltaFloat( current.origin[0] );
	current.localOrigin[1] = msg.ReadDeltaFloat( current.origin[1] );
	current.localOrigin[2] = msg.ReadDeltaFloat( current.origin[2] );
	localQuat.x = msg.ReadDeltaFloat( quat.x );
	localQuat.y = msg.ReadDeltaFloat( quat.y );
	localQuat.z = msg.ReadDeltaFloat( quat.z );

	current.axis = quat.ToMat3();
	current.localAxis = localQuat.ToMat3();
}
