
#include "../../idlib/Lib.h"
#pragma hdrstop

#include "../Game_local.h"

CLASS_DECLARATION( anForce, anForce_Field )
END_CLASS

/*
================
anForce_Field::anForce_Field
================
*/
anForce_Field::anForce_Field( void ) {
	type			= FORCEFIELD_UNIFORM;
	applyType		= FORCEFIELD_APPLY_FORCE;
	magnitude		= 0.0f;
	dir.Set( 0, 0, 1 );
	randomTorque	= 0.0f;
	playerOnly		= false;
	monsterOnly		= false;
	clipModel		= nullptr;
	lastApplyTime   = -1;
	owner			= nullptr;
}

/*
================
anForce_Field::~anForce_Field
================
*/
anForce_Field::~anForce_Field( void ) {
	if ( this->clipModel ) {
		delete this->clipModel;
	}
}

/*
================
anForce_Field::Save
================
*/
void anForce_Field::Save( anSaveGame *savefile ) const {
	savefile->WriteInt( type );
	savefile->WriteInt( applyType);
	savefile->WriteFloat( magnitude );
	savefile->WriteVec3( dir );
	savefile->WriteFloat( randomTorque );
	savefile->WriteBool( playerOnly );
	savefile->WriteBool( monsterOnly );
	savefile->WriteClipModel( clipModel );

	savefile->WriteInt ( lastApplyTime );
	// TOSAVE: anEntity*			owner;
}

/*
================
anForce_Field::Restore
================
*/
void anForce_Field::Restore( anRestoreGame *savefile ) {
	savefile->ReadInt( ( int&)type );
	savefile->ReadInt( ( int&)applyType);
	savefile->ReadFloat( magnitude );
	savefile->ReadVec3( dir );
	savefile->ReadFloat( randomTorque );
	savefile->ReadBool( playerOnly );
	savefile->ReadBool( monsterOnly );
	savefile->ReadClipModel( clipModel );

	savefile->ReadInt ( lastApplyTime );
}

/*
================
anForce_Field::SetClipModel
================
*/
void anForce_Field::SetClipModel( anClipModel *clipModel ) {
	if ( this->clipModel && clipModel != this->clipModel ) {
		delete this->clipModel;
	}
	this->clipModel = clipModel;
}

/*
================
anForce_Field::Uniform
================
*/
void anForce_Field::Uniform( const anVec3 &force ) {
	dir = force;
	magnitude = dir.Normalize();
	type = FORCEFIELD_UNIFORM;
}

/*
================
anForce_Field::Explosion
================
*/
void anForce_Field::Explosion( float force ) {
	magnitude = force;
	type = FORCEFIELD_EXPLOSION;
}

/*
================
anForce_Field::Implosion
================
*/
void anForce_Field::Implosion( float force ) {
	magnitude = force;
	type = FORCEFIELD_IMPLOSION;
}

/*
================
anForce_Field::RandomTorque
================
*/
void anForce_Field::RandomTorque( float force ) {
	randomTorque = force;
}

/*
================
anForce_Field::Evaluate
================
*/
void anForce_Field::Evaluate( int time ) {
	int numClipModels, i;
	anBounds bounds;
	anVec3 force, torque, angularVelocity;
	anClipModel *cm, *clipModelList[ MAX_GENTITIES ];

	assert( clipModel );
	bounds.FromTransformedBounds( clipModel->GetBounds(), clipModel->GetOrigin(), clipModel->GetAxis() );
	numClipModels = gameLocal.ClipModelsTouchingBounds( owner, bounds, -1, clipModelList, MAX_GENTITIES );

	for ( i = 0; i < numClipModels; i++ ) {
		cm = clipModelList[i];
		if ( !cm->IsTraceModel() ) {
			continue;
		}

		anEntity *entity = cm->GetEntity();

		if ( !entity ) {
			continue;
		}

		anPhysics *physics = entity->GetPhysics();

		if ( playerOnly ) {
			if ( !physics->IsType( anPhysics_Player::GetClassType() ) ) {
				continue;
			}
		} else if ( monsterOnly ) {
			if ( !physics->IsType( anPhysics_Monster::GetClassType() ) ) {
				continue;
			}
		}
		if ( entity->IsType( idItem::GetClassType() ) ) {
			continue;
		}

		if ( entity->IsType( anBasePlayer::GetClassType() ) ) {
			if ( ( (anBasePlayer *)entity )->health <= 0 ) {
				continue;
			}

		}

		if ( !gameLocal.ContentsModel( owner, cm->GetOrigin(), cm, cm->GetAxis(), -1,
						clipModel->GetCollisionModel(), clipModel->GetOrigin(), clipModel->GetAxis() ) ) {
			continue;
		}

		switch ( type ) {
			case FORCEFIELD_UNIFORM: {
				force = dir;
				break;
			}
			case FORCEFIELD_EXPLOSION: {
				force = cm->GetOrigin() - clipModel->GetOrigin();
				force.Normalize();
				break;
			}
			case FORCEFIELD_IMPLOSION: {
				force = clipModel->GetOrigin() - cm->GetOrigin();
				force.Normalize();
				break;
			}
			default: {
				gameLocal.Error( "anForce_Field: invalid type" );
				break;
			}
		}

		if ( randomTorque != 0.0f ) {
			torque[0] = gameLocal.random.CRandomFloat();
			torque[1] = gameLocal.random.CRandomFloat();
			torque[2] = gameLocal.random.CRandomFloat();
			if ( torque.Normalize() == 0.0f ) {
				torque[2] = 1.0f;
			}
		}

		switch ( applyType ) {
			case FORCEFIELD_APPLY_FORCE: {
				if ( randomTorque != 0.0f ) {
					entity->AddForce( gameLocal.world, cm->GetId(), cm->GetOrigin() + torque.Cross( dir ) * randomTorque, dir * magnitude );
				} else {
					entity->AddForce( gameLocal.world, cm->GetId(), cm->GetOrigin(), force * magnitude );
				}
				break;
			}
			case FORCEFIELD_APPLY_VELOCITY: {
				physics->SetLinearVelocity( force * magnitude, cm->GetId() );
				if ( randomTorque != 0.0f ) {
					angularVelocity = physics->GetAngularVelocity( cm->GetId() );
					physics->SetAngularVelocity( 0.5f * (angularVelocity + torque * randomTorque), cm->GetId() );
				}
				break;
			}
			case FORCEFIELD_APPLY_IMPULSE: {
				if ( randomTorque != 0.0f ) {
					entity->ApplyImpulse( gameLocal.world, cm->GetId(), cm->GetOrigin() + torque.Cross( dir ) * randomTorque, dir * magnitude );
				} else {
					entity->ApplyImpulse( gameLocal.world, cm->GetId(), cm->GetOrigin(), force * magnitude );
				}
				break;
			}
			default: {
				gameLocal.Error( "anForce_Field: invalid apply type" );
				break;
			}
		}

		lastApplyTime = time;
	}
}
