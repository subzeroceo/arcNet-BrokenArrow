
#include "../../idlib/Lib.h"
#pragma hdrstop

#include "../Game_local.h"

CLASS_DECLARATION( anPhysics_Base, anPhysics_Actor )
END_CLASS

/*
================
anPhysics_Actor::anPhysics_Actor
================
*/
anPhysics_Actor::anPhysics_Actor( void ) {
	clipModel = nullptr;
	SetClipModelAxis();
	mass = 100.0f;
	invMass = 1.0f / mass;
	masterEntity = nullptr;
	masterYaw = 0.0f;
	masterDeltaYaw = 0.0f;
	groundEntityPtr = nullptr;
}

/*
================
anPhysics_Actor::~anPhysics_Actor
================
*/
anPhysics_Actor::~anPhysics_Actor( void ) {
	if ( clipModel ) {
		delete clipModel;
		clipModel = nullptr;
	}
}

/*
================
anPhysics_Actor::Save
================
*/
void anPhysics_Actor::Save( anSaveGame *savefile ) const {
	savefile->WriteClipModel( clipModel );
	savefile->WriteMat3( clipModelAxis );

	savefile->WriteFloat( mass );
	savefile->WriteFloat( invMass );

	savefile->WriteObject( masterEntity );
	savefile->WriteFloat( masterYaw );
	savefile->WriteFloat( masterDeltaYaw );

	groundEntityPtr.Save( savefile );
}

/*
================
anPhysics_Actor::Restore
================
*/
void anPhysics_Actor::Restore( anRestoreGame *savefile ) {
	savefile->ReadClipModel( clipModel );
	savefile->ReadMat3( clipModelAxis );

	savefile->ReadFloat( mass );
	savefile->ReadFloat( invMass );

	savefile->ReadObject( reinterpret_cast<anClass *&>( masterEntity ) );
	savefile->ReadFloat( masterYaw );
	savefile->ReadFloat( masterDeltaYaw );

	groundEntityPtr.Restore( savefile );
}

/*
================
anPhysics_Actor::SetClipModelAxis
================
*/
void anPhysics_Actor::SetClipModelAxis( void ) {
	// align clip model to gravity direction
	if ( ( gravityNormal[2] == -1.0f ) || ( gravityNormal == vec3_zero ) ) {
		clipModelAxis.Identity();
	} else {
		clipModelAxis[2] = -gravityNormal;
		clipModelAxis[2].NormalVectors( clipModelAxis[0], clipModelAxis[1] );
		clipModelAxis[1] = -clipModelAxis[1];
	}

	if ( clipModel ) {
		clipModel->Link( self, 0, clipModel->GetOrigin(), clipModelAxis );
	}
}

/*
================
anPhysics_Actor::GetGravityAxis
================
*/
const anMat3 &anPhysics_Actor::GetGravityAxis( void ) const {
	return clipModelAxis;
}

/*
================
anPhysics_Actor::GetMasterDeltaYaw
================
*/
float anPhysics_Actor::GetMasterDeltaYaw( void ) const {
	return masterDeltaYaw;
}

/*
================
anPhysics_Actor::GetGroundEntity
================
*/
anEntity *anPhysics_Actor::GetGroundEntity( void ) const {
	return groundEntityPtr.GetEntity();
}

/*
================
anPhysics_Actor::SetClipModel
================
*/
void anPhysics_Actor::SetClipModel( anClipModel *model, const float density, int id, bool freeOld ) {
	assert( self );
	assert( model );					// a clip model is required
	assert( model->IsTraceModel() );	// and it should be a trace model
	assert( density > 0.0f );			// density should be valid

	if ( clipModel && clipModel != model && freeOld ) {
		delete clipModel;
	}
	clipModel = model;
	clipModel->Link( self, 0, clipModel->GetOrigin(), clipModelAxis );
}

/*
================
anPhysics_Actor::GetClipModel
================
*/
anClipModel *anPhysics_Actor::GetClipModel( int id ) const {
	return clipModel;
}

/*
================
anPhysics_Actor::GetNumClipModels
================
*/
int anPhysics_Actor::GetNumClipModels( void ) const {
	return 1;
}

/*
================
anPhysics_Actor::SetMass
================
*/
void anPhysics_Actor::SetMass( float _mass, int id ) {
	assert( _mass > 0.0f );
	mass = _mass;
	invMass = 1.0f / _mass;
}

/*
================
anPhysics_Actor::GetMass
================
*/
float anPhysics_Actor::GetMass( int id ) const {
	return mass;
}

/*
================
anPhysics_Actor::SetContents
================
*/
void anPhysics_Actor::SetContents( int contents, int id ) {
	clipModel->SetContents( contents );
}

/*
================
anPhysics_Actor::GetContents
================
*/
int anPhysics_Actor::GetContents( int id ) const {
	return clipModel->GetContents();
}

/*
================
anPhysics_Actor::GetBounds
================
*/
const anBounds &anPhysics_Actor::GetBounds( int id ) const {
	return clipModel->GetBounds();
}

/*
================
anPhysics_Actor::GetAbsBounds
================
*/
const anBounds &anPhysics_Actor::GetAbsBounds( int id ) const {
	return clipModel->GetAbsBounds();
}

/*
================
anPhysics_Actor::IsPushable
================
*/
bool anPhysics_Actor::IsPushable( void ) const {
	return ( masterEntity == nullptr );
}

/*
================
anPhysics_Actor::GetOrigin
================
*/
const anVec3 &anPhysics_Actor::GetOrigin( int id ) const {
	return clipModel->GetOrigin();
}

/*
================
anPhysics_Player::GetAxis
================
*/
const anMat3 &anPhysics_Actor::GetAxis( int id ) const {
	return clipModel->GetAxis();
}

/*
================
anPhysics_Actor::SetGravity
================
*/
void anPhysics_Actor::SetGravity( const anVec3 &newGravity ) {
	if ( newGravity != gravityVector ) {
		anPhysics_Base::SetGravity( newGravity );
		SetClipModelAxis();
	}
}

/*
================
anPhysics_Actor::ClipTranslation
================
*/
void anPhysics_Actor::ClipTranslation( trace_t &results, const anVec3 &translation, const anClipModel *model ) const {
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
anPhysics_Actor::ClipRotation
================
*/
void anPhysics_Actor::ClipRotation( trace_t &results, const anRotation &rotation, const anClipModel *model ) const {
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
anPhysics_Actor::ClipContents
================
*/
int anPhysics_Actor::ClipContents( const anClipModel *model ) const {
	if ( model ) {
		return gameLocal.ContentsModel( self, clipModel->GetOrigin(), clipModel, clipModel->GetAxis(), -1,
									model->GetCollisionModel(), model->GetOrigin(), model->GetAxis() );
	} else {
		return gameLocal.Contents( self, clipModel->GetOrigin(), clipModel, clipModel->GetAxis(), -1, nullptr );
	}
}

/*
================
anPhysics_Actor::DisableClip
================
*/
void anPhysics_Actor::DisableClip( void ) {
	clipModel->Disable();
}

/*
================
anPhysics_Actor::EnableClip
================
*/
void anPhysics_Actor::EnableClip( void ) {
	clipModel->Enable();
}

/*
================
anPhysics_Actor::UnlinkClip
================
*/
void anPhysics_Actor::UnlinkClip( void ) {
	clipModel->Unlink();
}

/*
================
anPhysics_Actor::LinkClip
================
*/
void anPhysics_Actor::LinkClip( void ) {
	clipModel->Link( self, 0, clipModel->GetOrigin(), clipModel->GetAxis() );
}

/*
================
anPhysics_Actor::EvaluateContacts
================
*/
bool anPhysics_Actor::EvaluateContacts( void ) {
	// get all the ground contacts
	ClearContacts();
	AddGroundContacts( clipModel );
	AddContactEntitiesForContacts();

	return ( contacts.Num() != 0 );
}
