#include "../idlib/Lib.h"
#pragma hdrstop

#include "Game_local.h"


/*
===============================================================================

  anMultiModelAF

===============================================================================
*/

CLASS_DECLARATION( anEntity, anMultiModelAF )
END_CLASS

/*
================
anMultiModelAF::Spawn
================
*/
void anMultiModelAF::Spawn( void ) {
	physicsObj.SetSelf( this );
}

/*
================
anMultiModelAF::~anMultiModelAF
================
*/
anMultiModelAF::~anMultiModelAF( void ) {
	int i;

	for ( i = 0; i < modelDefHandles.Num(); i++ ) {
		if ( modelDefHandles[i] != -1 ) {
			gameRenderWorld->FreeEntityDef( modelDefHandles[i] );
			modelDefHandles[i] = -1;
		}
	}
}

/*
================
anMultiModelAF::SetModelForId
================
*/
void anMultiModelAF::SetModelForId( int id, const anStr &modelName ) {
	modelHandles.AssureSize( id+1, nullptr );
	modelDefHandles.AssureSize( id+1, -1 );
	modelHandles[id] = renderModelManager->FindModel( modelName );
}

/*
================
anMultiModelAF::Present
================
*/
void anMultiModelAF::Present( void ) {
	int i;

	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}
	BecomeInactive( TH_UPDATEVISUALS );

	for ( i = 0; i < modelHandles.Num(); i++ ) {

		if ( !modelHandles[i] ) {
			continue;
		}

		renderEntity.origin = physicsObj.GetOrigin( i );
		renderEntity.axis = physicsObj.GetAxis( i );
		renderEntity.hModel = modelHandles[i];
		renderEntity.bodyId = i;

		// add to refresh list
		if ( modelDefHandles[i] == -1 ) {
			modelDefHandles[i] = gameRenderWorld->AddEntityDef( &renderEntity );
		} else {
			gameRenderWorld->UpdateEntityDef( modelDefHandles[i], &renderEntity );
		}
	}
}

/*
================
anMultiModelAF::Think
================
*/
void anMultiModelAF::Think( void ) {
	RunPhysics();
	Present();
}


/*
===============================================================================

  anAFChain

===============================================================================
*/

CLASS_DECLARATION( anMultiModelAF, anAFChain )
END_CLASS

/*
================
anAFChain::BuildChain

  builds a chain hanging down from the ceiling
  the highest link is a child of the link below it etc.
  this allows an object to be attached to multiple chains while keeping a single tree structure
================
*/
void anAFChain::BuildChain( const anStr &name, const anVec3 &origin, float linkLength, float linkWidth, float density, int numLinks, bool bindToWorld ) {
	int i;
	float halfLinkLength = linkLength * 0.5f;
	anTraceModel trm;
	anClipModel *clip;
	arcAFBody *body, *lastBody;
	arcAFConstraint_BallAndSocketJoint *bsj;
	arcAFConstraint_UniversalJoint *uj;
	anVec3 org;

	// create a trace model
	trm = anTraceModel( linkLength, linkWidth );
	trm.Translate( -trm.offset );

	org = origin - anVec3( 0, 0, halfLinkLength );

	lastBody = nullptr;
	for ( i = 0; i < numLinks; i++ ) {

		// add body
		clip = new anClipModel( trm );
		clip->SetContents( CONTENTS_SOLID );
		clip->Link( gameLocal.clip, this, 0, org, mat3_identity );
		body = new arcAFBody( name + anStr( i ), clip, density );
		physicsObj.AddBody( body );

		// visual model for body
		SetModelForId( physicsObj.GetBodyId( body ), spawnArgs.GetString( "model" ) );

		// add constraint
		if ( bindToWorld ) {
			if ( !lastBody ) {
				uj = new arcAFConstraint_UniversalJoint( name + anStr( i ), body, lastBody );
				uj->SetShafts( anVec3( 0, 0, -1 ), anVec3( 0, 0, 1 ) );
				//uj->SetConeLimit( anVec3( 0, 0, -1 ), 30.0f );
				//uj->SetPyramidLimit( anVec3( 0, 0, -1 ), anVec3( 1, 0, 0 ), 90.0f, 30.0f );
			}
			else {
				uj = new arcAFConstraint_UniversalJoint( name + anStr( i ), lastBody, body );
				uj->SetShafts( anVec3( 0, 0, 1 ), anVec3( 0, 0, -1 ) );
				//uj->SetConeLimit( anVec3( 0, 0, 1 ), 30.0f );
			}
			uj->SetAnchor( org + anVec3( 0, 0, halfLinkLength ) );
			uj->SetFriction( 0.9f );
			physicsObj.AddConstraint( uj );
		}
		else {
			if ( lastBody ) {
				bsj = new arcAFConstraint_BallAndSocketJoint( "joint" + anStr( i ), lastBody, body );
				bsj->SetAnchor( org + anVec3( 0, 0, halfLinkLength ) );
				bsj->SetConeLimit( anVec3( 0, 0, 1 ), 60.0f, anVec3( 0, 0, 1 ) );
				physicsObj.AddConstraint( bsj );
			}
		}

		org[2] -= linkLength;

		lastBody = body;
	}
}

/*
================
anAFChain::Spawn
================
*/
void anAFChain::Spawn( void ) {
	int numLinks;
	float length, linkLength, linkWidth, density;
	bool drop;
	anVec3 origin;

	spawnArgs.GetBool( "drop", "0", drop );
	spawnArgs.GetInt( "links", "3", numLinks );
	spawnArgs.GetFloat( "length", anStr( numLinks * 32.0f ), length );
	spawnArgs.GetFloat( "width", "8", linkWidth );
	spawnArgs.GetFloat( "density", "0.2", density );
	linkLength = length / numLinks;
	origin = GetPhysics()->GetOrigin();

	// initialize physics
	physicsObj.SetSelf( this );
	physicsObj.SetGravity( gameLocal.GetGravity() );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_ACTORBODY );
	SetPhysics( &physicsObj );

	BuildChain( "link", origin, linkLength, linkWidth, density, numLinks, !drop );
}

/*
===============================================================================

  anAFAttachment

===============================================================================
*/

CLASS_DECLARATION( anAnimatedEntity, anAFAttachment )
END_CLASS

/*
=====================
anAFAttachment::anAFAttachment
=====================
*/
anAFAttachment::anAFAttachment( void ) {
	body			= nullptr;
	combatModel		= nullptr;
	idleAnim		= 0;
	attachJoint		= INVALID_JOINT;
}

/*
=====================
anAFAttachment::~anAFAttachment
=====================
*/
anAFAttachment::~anAFAttachment( void ) {

	StopSound( SND_CHANNEL_ANY, false );

	delete combatModel;
	combatModel = nullptr;
}

/*
=====================
anAFAttachment::Spawn
=====================
*/
void anAFAttachment::Spawn( void ) {
	idleAnim = animator.GetAnim( "idle" );
}

/*
=====================
anAFAttachment::SetBody
=====================
*/
void anAFAttachment::SetBody( anEntity *bodyEnt, const char *model, jointHandle_t attachJoint ) {
	bool bleed;

	body = bodyEnt;
	this->attachJoint = attachJoint;
	SetModel( model );
	fl.takedamage = true;

	bleed = body->spawnArgs.GetBool( "bleed" );
	spawnArgs.SetBool( "bleed", bleed );
}

/*
=====================
anAFAttachment::ClearBody
=====================
*/
void anAFAttachment::ClearBody( void ) {
	body = nullptr;
	attachJoint = INVALID_JOINT;
	Hide();
}

/*
=====================
anAFAttachment::GetBody
=====================
*/
anEntity *anAFAttachment::GetBody( void ) const {
	return body;
}

/*
================
anAFAttachment::Save

archive object for savegame file
================
*/
void anAFAttachment::Save( anSaveGame *savefile ) const {
	savefile->WriteObject( body );
	savefile->WriteInt( idleAnim );
	savefile->WriteJoint( attachJoint );
}

/*
================
anAFAttachment::Restore

unarchives object from save game file
================
*/
void anAFAttachment::Restore( anRestoreGame *savefile ) {
	savefile->ReadObject( reinterpret_cast<anClass *&>( body ) );
	savefile->ReadInt( idleAnim );
	savefile->ReadJoint( attachJoint );

	SetCombatModel();
	LinkCombat();
}

/*
================
anAFAttachment::Hide
================
*/
void anAFAttachment::Hide( void ) {
	anEntity::Hide();
	UnlinkCombat();
}

/*
================
anAFAttachment::Show
================
*/
void anAFAttachment::Show( void ) {
	anEntity::Show();
	LinkCombat();
}

/*
============
anAFAttachment::Damage

Pass damage to body at the bindjoint
============
*/
void anAFAttachment::Damage( anEntity *inflictor, anEntity *attacker, const anVec3 &dir,
	const char *damageDefName, const float damageScale, const int location ) {

	if ( body ) {
		body->Damage( inflictor, attacker, dir, damageDefName, damageScale, attachJoint );
	}
}

/*
================
anAFAttachment::AddDamageEffect
================
*/
void anAFAttachment::AddDamageEffect( const trace_t &collision, const anVec3 &velocity, const char *damageDefName ) {
	if ( body ) {
		trace_t c = collision;
		c.c.id = JOINT_HANDLE_TO_CLIPMODEL_ID( attachJoint );
		body->AddDamageEffect( c, velocity, damageDefName );
	}
}

/*
================
anAFAttachment::GetImpactInfo
================
*/
void anAFAttachment::GetImpactInfo( anEntity *ent, int id, const anVec3 &point, impactInfo_t *info ) {
	if ( body ) {
		body->GetImpactInfo( ent, JOINT_HANDLE_TO_CLIPMODEL_ID( attachJoint ), point, info );
	} else {
		anEntity::GetImpactInfo( ent, id, point, info );
	}
}

/*
================
anAFAttachment::ApplyImpulse
================
*/
void anAFAttachment::ApplyImpulse( anEntity *ent, int id, const anVec3 &point, const anVec3 &impulse ) {
	if ( body ) {
		body->ApplyImpulse( ent, JOINT_HANDLE_TO_CLIPMODEL_ID( attachJoint ), point, impulse );
	} else {
		anEntity::ApplyImpulse( ent, id, point, impulse );
	}
}

/*
================
anAFAttachment::AddForce
================
*/
void anAFAttachment::AddForce( anEntity *ent, int id, const anVec3 &point, const anVec3 &force ) {
	if ( body ) {
		body->AddForce( ent, JOINT_HANDLE_TO_CLIPMODEL_ID( attachJoint ), point, force );
	} else {
		anEntity::AddForce( ent, id, point, force );
	}
}

/*
================
anAFAttachment::PlayIdleAnim
================
*/
void anAFAttachment::PlayIdleAnim( int blendTime ) {
	if ( idleAnim && ( idleAnim != animator.CurrentAnim( ANIMCHANNEL_ALL )->AnimNum() ) ) {
		animator.CycleAnim( ANIMCHANNEL_ALL, idleAnim, gameLocal.time, blendTime );
	}
}

/*
================
idAfAttachment::Think
================
*/
void anAFAttachment::Think( void ) {
	anAnimatedEntity::Think();
	if ( thinkFlags & TH_UPDATEPARTICLES ) {
		UpdateDamageEffects();
	}
}

/*
================
anAFAttachment::SetCombatModel
================
*/
void anAFAttachment::SetCombatModel( void ) {
	if ( combatModel ) {
		combatModel->Unlink();
		combatModel->LoadModel( modelDefHandle );
	} else {
		combatModel = new anClipModel( modelDefHandle );
	}
	combatModel->SetOwner( body );
}

/*
================
anAFAttachment::GetCombatModel
================
*/
anClipModel *anAFAttachment::GetCombatModel( void ) const {
	return combatModel;
}

/*
================
anAFAttachment::LinkCombat
================
*/
void anAFAttachment::LinkCombat( void ) {
	if ( fl.hidden ) {
		return;
	}

	if ( combatModel ) {
		combatModel->Link( gameLocal.clip, this, 0, renderEntity.origin, renderEntity.axis, modelDefHandle );
	}
}

/*
================
anAFAttachment::UnlinkCombat
================
*/
void anAFAttachment::UnlinkCombat( void ) {
	if ( combatModel ) {
		combatModel->Unlink();
	}
}


/*
===============================================================================

  anAFEntity_Base

===============================================================================
*/

const anEventDef EV_SetConstraintPosition( "SetConstraintPosition", "sv" );

CLASS_DECLARATION( anAnimatedEntity, anAFEntity_Base )
	EVENT( EV_SetConstraintPosition,	anAFEntity_Base::Event_SetConstraintPosition )
END_CLASS

static const float BOUNCE_SOUND_MIN_VELOCITY	= 80.0f;
static const float BOUNCE_SOUND_MAX_VELOCITY	= 200.0f;

/*
================
anAFEntity_Base::anAFEntity_Base
================
*/
anAFEntity_Base::anAFEntity_Base( void ) {
	combatModel = nullptr;
	combatModelContents = 0;
	nextSoundTime = 0;
	spawnOrigin.Zero();
	spawnAxis.Identity();
}

/*
================
anAFEntity_Base::~anAFEntity_Base
================
*/
anAFEntity_Base::~anAFEntity_Base( void ) {
	delete combatModel;
	combatModel = nullptr;
}

/*
================
anAFEntity_Base::Save
================
*/
void anAFEntity_Base::Save( anSaveGame *savefile ) const {
	savefile->WriteInt( combatModelContents );
	savefile->WriteClipModel( combatModel );
	savefile->WriteVec3( spawnOrigin );
	savefile->WriteMat3( spawnAxis );
	savefile->WriteInt( nextSoundTime );
	af.Save( savefile );
}

/*
================
anAFEntity_Base::Restore
================
*/
void anAFEntity_Base::Restore( anRestoreGame *savefile ) {
	savefile->ReadInt( combatModelContents );
	savefile->ReadClipModel( combatModel );
	savefile->ReadVec3( spawnOrigin );
	savefile->ReadMat3( spawnAxis );
	savefile->ReadInt( nextSoundTime );
	LinkCombat();

	af.Restore( savefile );
}

/*
================
anAFEntity_Base::Spawn
================
*/
void anAFEntity_Base::Spawn( void ) {
	spawnOrigin = GetPhysics()->GetOrigin();
	spawnAxis = GetPhysics()->GetAxis();
	nextSoundTime = 0;
}

/*
================
anAFEntity_Base::LoadAF
================
*/
bool anAFEntity_Base::LoadAF( void ) {
	anStr fileName;

	if ( !spawnArgs.GetString( "articulatedFigure", "*unknown*", fileName ) ) {
		return false;
	}

	af.SetAnimator( GetAnimator() );
	if ( !af.Load( this, fileName ) ) {
		gameLocal.Error( "anAFEntity_Base::LoadAF: Couldn't load af file '%s' on entity '%s'", fileName.c_str(), name.c_str() );
	}

	af.Start();

	af.GetPhysics()->Rotate( spawnAxis.ToRotation() );
	af.GetPhysics()->Translate( spawnOrigin );

	LoadState( spawnArgs );

	af.UpdateAnimation();
	animator.CreateFrame( gameLocal.time, true );
	UpdateVisuals();

	return true;
}

/*
================
anAFEntity_Base::Think
================
*/
void anAFEntity_Base::Think( void ) {
	RunPhysics();
	UpdateAnimation();
	if ( thinkFlags & TH_UPDATEVISUALS ) {
		Present();
		LinkCombat();
	}
}

/*
================
anAFEntity_Base::BodyForClipModelId
================
*/
int anAFEntity_Base::BodyForClipModelId( int id ) const {
	return af.BodyForClipModelId( id );
}

/*
================
anAFEntity_Base::SaveState
================
*/
void anAFEntity_Base::SaveState( anDict &args ) const {
	const anKeyValue *kv;

	// save the ragdoll pose
	af.SaveState( args );

	// save all the bind constraints
	kv = spawnArgs.MatchPrefix( "bindConstraint ", nullptr );
	while ( kv ) {
		args.Set( kv->GetKey(), kv->GetValue() );
		kv = spawnArgs.MatchPrefix( "bindConstraint ", kv );
	}

	// save the bind if it exists
	kv = spawnArgs.FindKey( "bind" );
	if ( kv ) {
		args.Set( kv->GetKey(), kv->GetValue() );
	}
	kv = spawnArgs.FindKey( "bindToJoint" );
	if ( kv ) {
		args.Set( kv->GetKey(), kv->GetValue() );
	}
	kv = spawnArgs.FindKey( "bindToBody" );
	if ( kv ) {
		args.Set( kv->GetKey(), kv->GetValue() );
	}
}

/*
================
anAFEntity_Base::LoadState
================
*/
void anAFEntity_Base::LoadState( const anDict &args ) {
	af.LoadState( args );
}

/*
================
anAFEntity_Base::AddBindConstraints
================
*/
void anAFEntity_Base::AddBindConstraints( void ) {
	af.AddBindConstraints();
}

/*
================
anAFEntity_Base::RemoveBindConstraints
================
*/
void anAFEntity_Base::RemoveBindConstraints( void ) {
	af.RemoveBindConstraints();
}

/*
================
anAFEntity_Base::GetImpactInfo
================
*/
void anAFEntity_Base::GetImpactInfo( anEntity *ent, int id, const anVec3 &point, impactInfo_t *info ) {
	if ( af.IsActive() ) {
		af.GetImpactInfo( ent, id, point, info );
	} else {
		anEntity::GetImpactInfo( ent, id, point, info );
	}
}

/*
================
anAFEntity_Base::ApplyImpulse
================
*/
void anAFEntity_Base::ApplyImpulse( anEntity *ent, int id, const anVec3 &point, const anVec3 &impulse ) {
	if ( af.IsLoaded() ) {
		af.ApplyImpulse( ent, id, point, impulse );
	}
	if ( !af.IsActive() ) {
		anEntity::ApplyImpulse( ent, id, point, impulse );
	}
}

/*
================
anAFEntity_Base::AddForce
================
*/
void anAFEntity_Base::AddForce( anEntity *ent, int id, const anVec3 &point, const anVec3 &force ) {
	if ( af.IsLoaded() ) {
		af.AddForce( ent, id, point, force );
	}
	if ( !af.IsActive() ) {
		anEntity::AddForce( ent, id, point, force );
	}
}

/*
================
anAFEntity_Base::Collide
================
*/
bool anAFEntity_Base::Collide( const trace_t &collision, const anVec3 &velocity ) {
	float v, f;

	if ( af.IsActive() ) {
		v = -( velocity * collision.c.normal );
		if ( v > BOUNCE_SOUND_MIN_VELOCITY && gameLocal.time > nextSoundTime ) {
			f = v > BOUNCE_SOUND_MAX_VELOCITY ? 1.0f : anMath::Sqrt( v - BOUNCE_SOUND_MIN_VELOCITY ) * ( 1.0f / anMath::Sqrt( BOUNCE_SOUND_MAX_VELOCITY - BOUNCE_SOUND_MIN_VELOCITY ) );
			if ( StartSound( "snd_bounce", SND_CHANNEL_ANY, 0, false, nullptr ) ) {
				// don't set the volume unless there is a bounce sound as it overrides the entire channel
				// which causes footsteps on ai's to not honor their shader parms
				SetSoundVolume( f );
			}
			nextSoundTime = gameLocal.time + 500;
		}
	}

	return false;
}

/*
================
anAFEntity_Base::GetPhysicsToVisualTransform
================
*/
bool anAFEntity_Base::GetPhysicsToVisualTransform( anVec3 &origin, anMat3 &axis ) {
	if ( af.IsActive() ) {
		af.GetPhysicsToVisualTransform( origin, axis );
		return true;
	}
	return anEntity::GetPhysicsToVisualTransform( origin, axis );
}

/*
================
anAFEntity_Base::UpdateAnimationControllers
================
*/
bool anAFEntity_Base::UpdateAnimationControllers( void ) {
	if ( af.IsActive() ) {
		if ( af.UpdateAnimation() ) {
			return true;
		}
	}
	return false;
}

/*
================
anAFEntity_Base::SetCombatModel
================
*/
void anAFEntity_Base::SetCombatModel( void ) {
	if ( combatModel ) {
		combatModel->Unlink();
		combatModel->LoadModel( modelDefHandle );
	} else {
		combatModel = new anClipModel( modelDefHandle );
	}
}

/*
================
anAFEntity_Base::GetCombatModel
================
*/
anClipModel *anAFEntity_Base::GetCombatModel( void ) const {
	return combatModel;
}

/*
================
anAFEntity_Base::SetCombatContents
================
*/
void anAFEntity_Base::SetCombatContents( bool enable ) {
	assert( combatModel );
	if ( enable && combatModelContents ) {
		assert( !combatModel->GetContents() );
		combatModel->SetContents( combatModelContents );
		combatModelContents = 0;
	} else if ( !enable && combatModel->GetContents() ) {
		assert( !combatModelContents );
		combatModelContents = combatModel->GetContents();
		combatModel->SetContents( 0 );
	}
}

/*
================
anAFEntity_Base::LinkCombat
================
*/
void anAFEntity_Base::LinkCombat( void ) {
	if ( fl.hidden ) {
		return;
	}
	if ( combatModel ) {
		combatModel->Link( gameLocal.clip, this, 0, renderEntity.origin, renderEntity.axis, modelDefHandle );
	}
}

/*
================
anAFEntity_Base::UnlinkCombat
================
*/
void anAFEntity_Base::UnlinkCombat( void ) {
	if ( combatModel ) {
		combatModel->Unlink();
	}
}

/*
================
anAFEntity_Base::FreeModelDef
================
*/
void anAFEntity_Base::FreeModelDef( void ) {
	UnlinkCombat();
	anEntity::FreeModelDef();
}

/*
===============
anAFEntity_Base::ShowEditingDialog
===============
*/
void anAFEntity_Base::ShowEditingDialog( void ) {
	common->InitTool( EDITOR_AF, &spawnArgs );
}

/*
================
anAFEntity_Base::DropAFs

  The entity should have the following key/value pairs set:
	"def_drop<type>AF"		"af def"
	"drop<type>Skin"		"skin name"
  To drop multiple articulated figures the following key/value pairs can be used:
	"def_drop<type>AF*"		"af def"
  where * is an aribtrary string.
================
*/
void anAFEntity_Base::DropAFs( anEntity *ent, const char *type, anList<anEntity *> *list ) {
	const anKeyValue *kv;
	const char *skinName;
	anEntity *newEnt;
	anAFEntity_Base *af;
	anDict args;
	const anDeclSkin *skin;

	// drop the articulated figures
	kv = ent->spawnArgs.MatchPrefix( va( "def_drop%sAF", type ), nullptr );
	while ( kv ) {

		args.Set( "classname", kv->GetValue() );
		gameLocal.SpawnEntityDef( args, &newEnt );

		if ( newEnt && newEnt->IsType( anAFEntity_Base::Type ) ) {
			af = static_cast<anAFEntity_Base *>(newEnt);
			af->GetPhysics()->SetOrigin( ent->GetPhysics()->GetOrigin() );
			af->GetPhysics()->SetAxis( ent->GetPhysics()->GetAxis() );
			af->af.SetupPose( ent, gameLocal.time );
			if ( list ) {
				list->Append( af );
			}
		}

		kv = ent->spawnArgs.MatchPrefix( va( "def_drop%sAF", type ), kv );
	}

	// change the skin to hide all the dropped articulated figures
	skinName = ent->spawnArgs.GetString( va( "skin_drop%s", type ) );
	if ( skinName[0] ) {
		skin = declManager->FindSkin( skinName );
		ent->SetSkin( skin );
	}
}

/*
================
anAFEntity_Base::Event_SetConstraintPosition
================
*/
void anAFEntity_Base::Event_SetConstraintPosition( const char *name, const anVec3 &pos ) {
	af.SetConstraintPosition( name, pos );
}

/*
===============================================================================

anAFEntity_Fragged

===============================================================================
*/

const anEventDef EV_Gib( "gib", "s" );
const anEventDef EV_Gibbed( "<gibbed>" );

CLASS_DECLARATION( anAFEntity_Base, anAFEntity_Fragged )
	EVENT( EV_Gib,		anAFEntity_Fragged::Event_Exploded )
	EVENT( EV_Gibbed,	anAFEntity_Base::Event_Remove )
END_CLASS


/*
================
anAFEntity_Fragged::anAFEntity_Fragged
================
*/
anAFEntity_Fragged::anAFEntity_Fragged( void ) {
	skeletonModel = nullptr;
	skeletonModelDefHandle = -1;
	gibbed = false;
}

/*
================
anAFEntity_Fragged::~anAFEntity_Fragged
================
*/
anAFEntity_Fragged::~anAFEntity_Fragged() {
	if ( skeletonModelDefHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( skeletonModelDefHandle );
		skeletonModelDefHandle = -1;
	}
}

/*
================
anAFEntity_Fragged::Save
================
*/
void anAFEntity_Fragged::Save( anSaveGame *savefile ) const {
	savefile->WriteBool( gibbed );
	savefile->WriteBool( combatModel != nullptr );
}

/*
================
anAFEntity_Fragged::Restore
================
*/
void anAFEntity_Fragged::Restore( anRestoreGame *savefile ) {
	bool hasCombatModel;

	savefile->ReadBool( gibbed );
	savefile->ReadBool( hasCombatModel );

	InitSkeletonModel();

	if ( hasCombatModel ) {
		SetCombatModel();
		LinkCombat();
	}
}

/*
================
anAFEntity_Fragged::Spawn
================
*/
void anAFEntity_Fragged::Spawn( void ) {
	InitSkeletonModel();

	gibbed = false;
}

/*
================
anAFEntity_Fragged::InitSkeletonModel
================
*/
void anAFEntity_Fragged::InitSkeletonModel( void ) {
	const char *modelName;
	const anDeclModelDef *modelDef;

	skeletonModel = nullptr;
	skeletonModelDefHandle = -1;

	modelName = spawnArgs.GetString( "model_gib" );

	modelDef = nullptr;
	if ( modelName[0] != '\0' ) {
		modelDef = static_cast<const anDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelName, false ) );
		if ( modelDef ) {
			skeletonModel = modelDef->ModelHandle();
		} else {
			skeletonModel = renderModelManager->FindModel( modelName );
		}
		if ( skeletonModel != nullptr && renderEntity.hModel != nullptr ) {
			if ( skeletonModel->NumJoints() != renderEntity.hModel->NumJoints() ) {
				gameLocal.Error( "gib model '%s' has different number of joints than model '%s'",
									skeletonModel->Name(), renderEntity.hModel->Name() );
			}
		}
	}
}

/*
================
anAFEntity_Fragged::Present
================
*/
void anAFEntity_Fragged::Present( void ) {
	renderEntity_t skeleton;

	if ( !gameLocal.isNewFrame ) {
		return;
	}

	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}

	// update skeleton model
	if ( gibbed && !IsHidden() && skeletonModel != nullptr ) {
		skeleton = renderEntity;
		skeleton.hModel = skeletonModel;
		// add to refresh list
		if ( skeletonModelDefHandle == -1 ) {
			skeletonModelDefHandle = gameRenderWorld->AddEntityDef( &skeleton );
		} else {
			gameRenderWorld->UpdateEntityDef( skeletonModelDefHandle, &skeleton );
		}
	}

	anEntity::Present();
}

/*
================
anAFEntity_Fragged::Damage
================
*/
void anAFEntity_Fragged::Damage( anEntity *inflictor, anEntity *attacker, const anVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	if ( !fl.takedamage ) {
		return;
	}
	anAFEntity_Base::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
	if ( health < -20 && spawnArgs.GetBool( "gib" ) ) {
		Gib( dir, damageDefName );
	}
}

/*
=====================
anAFEntity_Fragged::SpawnGibs
=====================
*/
void anAFEntity_Fragged::SpawnGibs( const anVec3 &dir, const char *damageDefName ) {
	int i;
	bool gibNonSolid;
	anVec3 entityCenter, velocity;
	anList<anEntity *> list;

	assert( !gameLocal.isClient );

	const anDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if ( !damageDef ) {
		gameLocal.Error( "Unknown damageDef '%s'", damageDefName );
	}

	// spawn gib articulated figures
	anAFEntity_Base::DropAFs( this, "gib", &list );

	// spawn gib items
	idMoveableItem::DropItems( this, "gib", &list );

	// blow out the gibs in the given direction away from the center of the entity
	entityCenter = GetPhysics()->GetAbsBounds().GetCenter();
	gibNonSolid = damageDef->GetBool( "gibNonSolid" );
	for ( i = 0; i < list.Num(); i++ ) {
		if ( gibNonSolid ) {
			list[i]->GetPhysics()->SetContents( 0 );
			list[i]->GetPhysics()->SetClipMask( 0 );
			list[i]->GetPhysics()->UnlinkClip();
			list[i]->GetPhysics()->PutToRest();
		} else {
			list[i]->GetPhysics()->SetContents( CONTENTS_CORPSE );
			list[i]->GetPhysics()->SetClipMask( CONTENTS_SOLID );
			velocity = list[i]->GetPhysics()->GetAbsBounds().GetCenter() - entityCenter;
			velocity.NormalizeFast();
			velocity += ( i & 1 ) ? dir : -dir;
			list[i]->GetPhysics()->SetLinearVelocity( velocity * 75.0f );
		}
		list[i]->GetRenderEntity()->noShadow = true;
		list[i]->GetRenderEntity()->shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f;
		list[i]->PostEventSec( &EV_Remove, 4.0f );
	}
}

/*
============
anAFEntity_Fragged::Gib
============
*/
void anAFEntity_Fragged::Gib( const anVec3 &dir, const char *damageDefName ) {
	// only gib once
	if ( gibbed ) {
		return;
	}

	const anDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if ( !damageDef ) {
		gameLocal.Error( "Unknown damageDef '%s'", damageDefName );
	}

	if ( damageDef->GetBool( "gibNonSolid" ) ) {
		GetAFPhysics()->SetContents( 0 );
		GetAFPhysics()->SetClipMask( 0 );
		GetAFPhysics()->UnlinkClip();
		GetAFPhysics()->PutToRest();
	} else {
		GetAFPhysics()->SetContents( CONTENTS_CORPSE );
		GetAFPhysics()->SetClipMask( CONTENTS_SOLID );
	}

	UnlinkCombat();

	if ( g_bloodEffects.GetBool() ) {
		if ( gameLocal.time > gameLocal.GetGibTime() ) {
			gameLocal.SetGibTime( gameLocal.time + GIB_DELAY );
			SpawnGibs( dir, damageDefName );
			renderEntity.noShadow = true;
			renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f;
			StartSound( "snd_gibbed", SND_CHANNEL_ANY, 0, false, nullptr );
			gibbed = true;
		}
	} else {
		gibbed = true;
	}


	PostEventSec( &EV_Gibbed, 4.0f );
}

/*
============
anAFEntity_Fragged::Event_Exploded
============
*/
void anAFEntity_Fragged::Event_Exploded( const char *damageDefName ) {
	Gib( anVec3( 0, 0, 1 ), damageDefName );
}

/*
===============================================================================

  anAFEntity_Generic

===============================================================================
*/

CLASS_DECLARATION( anAFEntity_Fragged, anAFEntity_Generic )
	EVENT( EV_Activate,			anAFEntity_Generic::Event_Activate )
END_CLASS

/*
================
anAFEntity_Generic::anAFEntity_Generic
================
*/
anAFEntity_Generic::anAFEntity_Generic( void ) {
	keepRunningPhysics = false;
}

/*
================
anAFEntity_Generic::~anAFEntity_Generic
================
*/
anAFEntity_Generic::~anAFEntity_Generic( void ) {
}

/*
================
anAFEntity_Generic::Save
================
*/
void anAFEntity_Generic::Save( anSaveGame *savefile ) const {
	savefile->WriteBool( keepRunningPhysics );
}

/*
================
anAFEntity_Generic::Restore
================
*/
void anAFEntity_Generic::Restore( anRestoreGame *savefile ) {
	savefile->ReadBool( keepRunningPhysics );
}

/*
================
anAFEntity_Generic::Think
================
*/
void anAFEntity_Generic::Think( void ) {
	anAFEntity_Base::Think();

	if ( keepRunningPhysics ) {
		BecomeActive( TH_PHYSICS );
	}
}

/*
================
anAFEntity_Generic::Spawn
================
*/
void anAFEntity_Generic::Spawn( void ) {
	if ( !LoadAF() ) {
		gameLocal.Error( "Couldn't load af file on entity '%s'", name.c_str() );
	}

	SetCombatModel();

	SetPhysics( af.GetPhysics() );

	af.GetPhysics()->PutToRest();
	if ( !spawnArgs.GetBool( "nodrop", "0" ) ) {
		af.GetPhysics()->Activate();
	}

	fl.takedamage = true;
}

/*
================
anAFEntity_Generic::Event_Activate
================
*/
void anAFEntity_Generic::Event_Activate( anEntity *activator ) {
	float delay;
	anVec3 init_velocity, init_avelocity;

	Show();

	af.GetPhysics()->EnableImpact();
	af.GetPhysics()->Activate();

	spawnArgs.GetVector( "init_velocity", "0 0 0", init_velocity );
	spawnArgs.GetVector( "init_avelocity", "0 0 0", init_avelocity );

	delay = spawnArgs.GetFloat( "init_velocityDelay", "0" );
	if ( delay == 0.0f ) {
		af.GetPhysics()->SetLinearVelocity( init_velocity );
	} else {
		PostEventSec( &EV_SetLinearVelocity, delay, init_velocity );
	}

	delay = spawnArgs.GetFloat( "init_avelocityDelay", "0" );
	if ( delay == 0.0f ) {
		af.GetPhysics()->SetAngularVelocity( init_avelocity );
	} else {
		PostEventSec( &EV_SetAngularVelocity, delay, init_avelocity );
	}
}


/*
===============================================================================

  anAFEntity_AttachedHead

===============================================================================
*/

CLASS_DECLARATION( anAFEntity_Fragged, anAFEntity_AttachedHead )
	EVENT( EV_Gib,				anAFEntity_AttachedHead::Event_Exploded )
	EVENT( EV_Activate,			anAFEntity_AttachedHead::Event_Activate )
END_CLASS

/*
================
anAFEntity_AttachedHead::anAFEntity_AttachedHead
================
*/
anAFEntity_AttachedHead::anAFEntity_AttachedHead() {
	head = nullptr;
}

/*
================
anAFEntity_AttachedHead::~anAFEntity_AttachedHead
================
*/
anAFEntity_AttachedHead::~anAFEntity_AttachedHead() {
	if ( head.GetEntity() ) {
		head.GetEntity()->ClearBody();
		head.GetEntity()->PostEventMS( &EV_Remove, 0 );
	}
}

/*
================
anAFEntity_AttachedHead::Spawn
================
*/
void anAFEntity_AttachedHead::Spawn( void ) {
	SetupHead();

	LoadAF();

	SetCombatModel();

	SetPhysics( af.GetPhysics() );

	af.GetPhysics()->PutToRest();
	if ( !spawnArgs.GetBool( "nodrop", "0" ) ) {
		af.GetPhysics()->Activate();
	}

	fl.takedamage = true;

	if ( head.GetEntity() ) {
		int anim = head.GetEntity()->GetAnimator()->GetAnim( "dead" );

		if ( anim ) {
			head.GetEntity()->GetAnimator()->SetFrame( ANIMCHANNEL_ALL, anim, 0, gameLocal.time, 0 );
		}
	}
}

/*
================
anAFEntity_AttachedHead::Save
================
*/
void anAFEntity_AttachedHead::Save( anSaveGame *savefile ) const {
	head.Save( savefile );
}

/*
================
anAFEntity_AttachedHead::Restore
================
*/
void anAFEntity_AttachedHead::Restore( anRestoreGame *savefile ) {
	head.Restore( savefile );
}

/*
================
anAFEntity_AttachedHead::SetupHead
================
*/
void anAFEntity_AttachedHead::SetupHead( void ) {
	anAFAttachment		*headEnt;
	anStr				jointName;
	const char			*headModel;
	jointHandle_t		joint;
	anVec3				origin;
	anMat3				axis;

	headModel = spawnArgs.GetString( "def_head", "" );
	if ( headModel[0] ) {
		jointName = spawnArgs.GetString( "head_joint" );
		joint = animator.GetJointHandle( jointName );
		if ( joint == INVALID_JOINT ) {
			gameLocal.Error( "Joint '%s' not found for 'head_joint' on '%s'", jointName.c_str(), name.c_str() );
		}

		headEnt = static_cast<anAFAttachment *>( gameLocal.SpawnEntityType( anAFAttachment::Type, nullptr ) );
		headEnt->SetName( va( "%s_head", name.c_str() ) );
		headEnt->SetBody( this, headModel, joint );
		headEnt->SetCombatModel();
		head = headEnt;

		animator.GetJointTransform( joint, gameLocal.time, origin, axis );
		origin = renderEntity.origin + origin * renderEntity.axis;
		headEnt->SetOrigin( origin );
		headEnt->SetAxis( renderEntity.axis );
		headEnt->BindToJoint( this, joint, true );
	}
}

/*
================
anAFEntity_AttachedHead::Think
================
*/
void anAFEntity_AttachedHead::Think( void ) {
	anAFEntity_Base::Think();
}

/*
================
anAFEntity_AttachedHead::LinkCombat
================
*/
void anAFEntity_AttachedHead::LinkCombat( void ) {
	anAFAttachment *headEnt;

	if ( fl.hidden ) {
		return;
	}

	if ( combatModel ) {
		combatModel->Link( gameLocal.clip, this, 0, renderEntity.origin, renderEntity.axis, modelDefHandle );
	}
	headEnt = head.GetEntity();
	if ( headEnt ) {
		headEnt->LinkCombat();
	}
}

/*
================
anAFEntity_AttachedHead::UnlinkCombat
================
*/
void anAFEntity_AttachedHead::UnlinkCombat( void ) {
	anAFAttachment *headEnt;

	if ( combatModel ) {
		combatModel->Unlink();
	}
	headEnt = head.GetEntity();
	if ( headEnt ) {
		headEnt->UnlinkCombat();
	}
}

/*
================
anAFEntity_AttachedHead::Hide
================
*/
void anAFEntity_AttachedHead::Hide( void ) {
	anAFEntity_Base::Hide();
	if ( head.GetEntity() ) {
		head.GetEntity()->Hide();
	}
	UnlinkCombat();
}

/*
================
anAFEntity_AttachedHead::Show
================
*/
void anAFEntity_AttachedHead::Show( void ) {
	anAFEntity_Base::Show();
	if ( head.GetEntity() ) {
		head.GetEntity()->Show();
	}
	LinkCombat();
}

/*
================
anAFEntity_AttachedHead::ProjectOverlay
================
*/
void anAFEntity_AttachedHead::ProjectOverlay( const anVec3 &origin, const anVec3 &dir, float size, const char *material ) {

	anEntity::ProjectOverlay( origin, dir, size, material );

	if ( head.GetEntity() ) {
		head.GetEntity()->ProjectOverlay( origin, dir, size, material );
	}
}

/*
============
anAFEntity_AttachedHead::Gib
============
*/
void anAFEntity_AttachedHead::Gib( const anVec3 &dir, const char *damageDefName ) {
	// only gib once
	if ( gibbed ) {
		return;
	}
	anAFEntity_Fragged::Gib( dir, damageDefName );
	if ( head.GetEntity() ) {
		head.GetEntity()->Hide();
	}
}

/*
============
anAFEntity_AttachedHead::Event_Exploded
============
*/
void anAFEntity_AttachedHead::Event_Exploded( const char *damageDefName ) {
	Gib( anVec3( 0, 0, 1 ), damageDefName );
}

/*
================
anAFEntity_AttachedHead::Event_Activate
================
*/
void anAFEntity_AttachedHead::Event_Activate( anEntity *activator ) {
	float delay;
	anVec3 init_velocity, init_avelocity;

	Show();

	af.GetPhysics()->EnableImpact();
	af.GetPhysics()->Activate();

	spawnArgs.GetVector( "init_velocity", "0 0 0", init_velocity );
	spawnArgs.GetVector( "init_avelocity", "0 0 0", init_avelocity );

	delay = spawnArgs.GetFloat( "init_velocityDelay", "0" );
	if ( delay == 0.0f ) {
		af.GetPhysics()->SetLinearVelocity( init_velocity );
	} else {
		PostEventSec( &EV_SetLinearVelocity, delay, init_velocity );
	}

	delay = spawnArgs.GetFloat( "init_avelocityDelay", "0" );
	if ( delay == 0.0f ) {
		af.GetPhysics()->SetAngularVelocity( init_avelocity );
	} else {
		PostEventSec( &EV_SetAngularVelocity, delay, init_avelocity );
	}
}


/*
===============================================================================

  anAFEntity_Vehicle

===============================================================================
*/

CLASS_DECLARATION( anAFEntity_Base, anAFEntity_Vehicle )
END_CLASS

/*
================
anAFEntity_Vehicle::anAFEntity_Vehicle
================
*/
anAFEntity_Vehicle::anAFEntity_Vehicle( void ) {
	player				= nullptr;
	eyesJoint			= INVALID_JOINT;
	steeringWheelJoint	= INVALID_JOINT;
	wheelRadius			= 0.0f;
	steerAngle			= 0.0f;
	steerSpeed			= 0.0f;
	dustSmoke			= nullptr;
}

/*
================
anAFEntity_Vehicle::Spawn
================
*/
void anAFEntity_Vehicle::Spawn( void ) {
	const char *eyesJointName = spawnArgs.GetString( "eyesJoint", "eyes" );
	const char *steeringWheelJointName = spawnArgs.GetString( "steeringWheelJoint", "steeringWheel" );

	LoadAF();

	SetCombatModel();

	SetPhysics( af.GetPhysics() );

	fl.takedamage = true;

	if ( !eyesJointName[0] ) {
		gameLocal.Error( "anAFEntity_Vehicle '%s' no eyes joint specified", name.c_str() );
	}
	eyesJoint = animator.GetJointHandle( eyesJointName );
	if ( !steeringWheelJointName[0] ) {
		gameLocal.Error( "anAFEntity_Vehicle '%s' no steering wheel joint specified", name.c_str() );
	}
	steeringWheelJoint = animator.GetJointHandle( steeringWheelJointName );

	spawnArgs.GetFloat( "wheelRadius", "20", wheelRadius );
	spawnArgs.GetFloat( "steerSpeed", "5", steerSpeed );

	player = nullptr;
	steerAngle = 0.0f;

	const char *smokeName = spawnArgs.GetString( "smokevhcle_dust", "muzzlesmoke" );
	if ( *smokeName != '\0' ) {
		dustSmoke = static_cast<const anDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
	}
}

/*
================
anAFEntity_Vehicle::Use
================
*/
void anAFEntity_Vehicle::Use( anBasePlayer *other ) {
	anVec3 origin;
	anMat3 axis;

	if ( player ) {
		if ( player == other ) {
			other->Unbind();
			player = nullptr;

			af.GetPhysics()->SetComeToRest( true );
		}
	}
	else {
		player = other;
		animator.GetJointTransform( eyesJoint, gameLocal.time, origin, axis );
		origin = renderEntity.origin + origin * renderEntity.axis;
		player->GetPhysics()->SetOrigin( origin );
		player->BindToBody( this, 0, true );

		af.GetPhysics()->SetComeToRest( false );
		af.GetPhysics()->Activate();
	}
}

/*
================
anAFEntity_Vehicle::GetSteerAngle
================
*/
float anAFEntity_Vehicle::GetSteerAngle( void ) {
	float idealSteerAngle, angleDelta;

	idealSteerAngle = player->usercmd.rightmove * ( 30.0f / 128.0f );
	angleDelta = idealSteerAngle - steerAngle;

	if ( angleDelta > steerSpeed ) {
		steerAngle += steerSpeed;
	} else if ( angleDelta < -steerSpeed ) {
		steerAngle -= steerSpeed;
	} else {
		steerAngle = idealSteerAngle;
	}

	return steerAngle;
}


/*
===============================================================================

  anAFEntity_VehicleSimple

===============================================================================
*/

CLASS_DECLARATION( anAFEntity_Vehicle, anAFEntity_VehicleSimple )
END_CLASS

/*
================
anAFEntity_VehicleSimple::anAFEntity_VehicleSimple
================
*/
anAFEntity_VehicleSimple::anAFEntity_VehicleSimple( void ) {
	int i;
	for ( i = 0; i < 4; i++ ) {
		suspension[i] = nullptr;
	}
}

/*
================
anAFEntity_VehicleSimple::~anAFEntity_VehicleSimple
================
*/
anAFEntity_VehicleSimple::~anAFEntity_VehicleSimple( void ) {
	delete wheelModel;
	wheelModel = nullptr;
}

/*
================
anAFEntity_VehicleSimple::Spawn
================
*/
void anAFEntity_VehicleSimple::Spawn( void ) {
	static const char *wheelJointKeys[] = {
		"wheelJointFrontLeft",
		"wheelJointFrontRight",
		"wheelJointRearLeft",
		"wheelJointRearRight"
	};
	static anVec3 wheelPoly[4] = { anVec3( 2, 2, 0 ), anVec3( 2, -2, 0 ), anVec3( -2, -2, 0 ), anVec3( -2, 2, 0 ) };

	int i;
	anVec3 origin;
	anMat3 axis;
	anTraceModel trm;

	trm.SetupPolygon( wheelPoly, 4 );
	trm.Translate( anVec3( 0, 0, -wheelRadius ) );
	wheelModel = new anClipModel( trm );

	for ( i = 0; i < 4; i++ ) {
		const char *wheelJointName = spawnArgs.GetString( wheelJointKeys[i], "" );
		if ( !wheelJointName[0] ) {
			gameLocal.Error( "anAFEntity_VehicleSimple '%s' no '%s' specified", name.c_str(), wheelJointKeys[i] );
		}
		wheelJoints[i] = animator.GetJointHandle( wheelJointName );
		if ( wheelJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "anAFEntity_VehicleSimple '%s' can't find wheel joint '%s'", name.c_str(), wheelJointName );
		}

		GetAnimator()->GetJointTransform( wheelJoints[i], 0, origin, axis );
		origin = renderEntity.origin + origin * renderEntity.axis;

		suspension[i] = new arcAFConstraint_Suspension();
		suspension[i]->Setup( va( "suspension%d", i ), af.GetPhysics()->GetBody( 0 ), origin, af.GetPhysics()->GetAxis( 0 ), wheelModel );
		suspension[i]->SetSuspension(	gvhcleSuspensionUp.GetFloat(),
										gvhcleSuspensionDown.GetFloat(),
										gvhcleSuspensionKCompress.GetFloat(),
										gvhcleSuspensionDamping.GetFloat(),
										gvhcleTireFriction.GetFloat() );

		af.GetPhysics()->AddConstraint( suspension[i] );
	}

	memset( wheelAngles, 0, sizeof( wheelAngles ) );
	BecomeActive( TH_THINK );
}

/*
================
anAFEntity_VehicleSimple::Think
================
*/
void anAFEntity_VehicleSimple::Think( void ) {
	int i;
	float force = 0.0f, velocity = 0.0f, steerAngle = 0.0f;
	anVec3 origin;
	anMat3 axis;
	anRotation wheelRotation, steerRotation;

	if ( thinkFlags & TH_THINK ) {

		if ( player ) {
			// capture the input from a player
			velocity = gvhcleVelocity.GetFloat();
			if ( player->usercmd.forwardmove < 0 ) {
				velocity = -velocity;
			}
			force = anMath::Fabs( player->usercmd.forwardmove * gvhcleForce.GetFloat() ) * (1.0f / 128.0f);
			steerAngle = GetSteerAngle();
		}

		// update the wheel motor force and steering
		for ( i = 0; i < 2; i++ ) {

			// front wheel drive
			if ( velocity != 0.0f ) {
				suspension[i]->EnableMotor( true );
			} else {
				suspension[i]->EnableMotor( false );
			}
			suspension[i]->SetMotorVelocity( velocity );
			suspension[i]->SetMotorForce( force );

			// update the wheel steering
			suspension[i]->SetSteerAngle( steerAngle );
		}

		// adjust wheel velocity for better steering because there are no differentials between the wheels
		if ( steerAngle < 0.0f ) {
			suspension[0]->SetMotorVelocity( velocity * 0.5f );
		} else if ( steerAngle > 0.0f ) {
			suspension[1]->SetMotorVelocity( velocity * 0.5f );
		}

		// update suspension with latest cvar settings
		for ( i = 0; i < 4; i++ ) {
			suspension[i]->SetSuspension(	gvhcleSuspensionUp.GetFloat(),
											gvhcleSuspensionDown.GetFloat(),
											gvhcleSuspensionKCompress.GetFloat(),
											gvhcleSuspensionDamping.GetFloat(),
											gvhcleTireFriction.GetFloat() );
		}

		// run the physics
		RunPhysics();

		// move and rotate the wheels visually
		for ( i = 0; i < 4; i++ ) {
			arcAFBody *body = af.GetPhysics()->GetBody( 0 );

			origin = suspension[i]->GetWheelOrigin();
			velocity = body->GetPointVelocity( origin ) * body->GetWorldAxis()[0];
			wheelAngles[i] += velocity * MS2SEC( gameLocal.msec ) / wheelRadius;

			// additional rotation about the wheel axis
			wheelRotation.SetAngle( RAD2DEG( wheelAngles[i] ) );
			wheelRotation.SetVec( 0, -1, 0 );

			if ( i < 2 ) {
				// rotate the wheel for steering
				steerRotation.SetAngle( steerAngle );
				steerRotation.SetVec( 0, 0, 1 );
				// set wheel rotation
				animator.SetJointAxis( wheelJoints[i], JOINTMOD_WORLD, wheelRotation.ToMat3() * steerRotation.ToMat3() );
			} else {
				// set wheel rotation
				animator.SetJointAxis( wheelJoints[i], JOINTMOD_WORLD, wheelRotation.ToMat3() );
			}

			// set wheel position for suspension
			origin = ( origin - renderEntity.origin ) * renderEntity.axis.Transpose();
			GetAnimator()->SetJointPos( wheelJoints[i], JOINTMOD_WORLD_OVERRIDE, origin );
		}
/*
		// spawn dust particle effects
		if ( force != 0.0f && !( gameLocal.frameNum & 7 ) ) {
			int numContacts;
			arcAFConstraint_Contact *contacts[2];
			for ( i = 0; i < 4; i++ ) {
				numContacts = af.GetPhysics()->GetBodyContactConstraints( wheels[i]->GetClipModel()->GetId(), contacts, 2 );
				for ( int j = 0; j < numContacts; j++ ) {
					gameLocal.smokeParticles->EmitSmoke( dustSmoke, gameLocal.time, gameLocal.random.RandomFloat(), contacts[j]->GetContact().point, contacts[j]->GetContact().normal.ToMat3() );
				}
			}
		}
*/
	}

	UpdateAnimation();
	if ( thinkFlags & TH_UPDATEVISUALS ) {
		Present();
		LinkCombat();
	}
}


/*
===============================================================================

  anAFEntity_VehicleFourWheels

===============================================================================
*/

CLASS_DECLARATION( anAFEntity_Vehicle, anAFEntity_VehicleFourWheels )
END_CLASS


/*
================
anAFEntity_VehicleFourWheels::anAFEntity_VehicleFourWheels
================
*/
anAFEntity_VehicleFourWheels::anAFEntity_VehicleFourWheels( void ) {
	int i;

	for ( i = 0; i < 4; i++ ) {
		wheels[i]		= nullptr;
		wheelJoints[i]	= INVALID_JOINT;
		wheelAngles[i]	= 0.0f;
	}
	steering[0]			= nullptr;
	steering[1]			= nullptr;
}

/*
================
anAFEntity_VehicleFourWheels::Spawn
================
*/
void anAFEntity_VehicleFourWheels::Spawn( void ) {
	int i;
	static const char *wheelBodyKeys[] = {
		"wheelBodyFrontLeft",
		"wheelBodyFrontRight",
		"wheelBodyRearLeft",
		"wheelBodyRearRight"
	};
	static const char *wheelJointKeys[] = {
		"wheelJointFrontLeft",
		"wheelJointFrontRight",
		"wheelJointRearLeft",
		"wheelJointRearRight"
	};
	static const char *steeringHingeKeys[] = {
		"steeringHingeFrontLeft",
		"steeringHingeFrontRight",
	};

	const char *wheelBodyName, *wheelJointName, *steeringHingeName;

	for ( i = 0; i < 4; i++ ) {
		wheelBodyName = spawnArgs.GetString( wheelBodyKeys[i], "" );
		if ( !wheelBodyName[0] ) {
			gameLocal.Error( "anAFEntity_VehicleFourWheels '%s' no '%s' specified", name.c_str(), wheelBodyKeys[i] );
		}
		wheels[i] = af.GetPhysics()->GetBody( wheelBodyName );
		if ( !wheels[i] ) {
			gameLocal.Error( "anAFEntity_VehicleFourWheels '%s' can't find wheel body '%s'", name.c_str(), wheelBodyName );
		}
		wheelJointName = spawnArgs.GetString( wheelJointKeys[i], "" );
		if ( !wheelJointName[0] ) {
			gameLocal.Error( "anAFEntity_VehicleFourWheels '%s' no '%s' specified", name.c_str(), wheelJointKeys[i] );
		}
		wheelJoints[i] = animator.GetJointHandle( wheelJointName );
		if ( wheelJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "anAFEntity_VehicleFourWheels '%s' can't find wheel joint '%s'", name.c_str(), wheelJointName );
		}
	}

	for ( i = 0; i < 2; i++ ) {
		steeringHingeName = spawnArgs.GetString( steeringHingeKeys[i], "" );
		if ( !steeringHingeName[0] ) {
			gameLocal.Error( "anAFEntity_VehicleFourWheels '%s' no '%s' specified", name.c_str(), steeringHingeKeys[i] );
		}
		steering[i] = static_cast<arcAFConstraint_Hinge *>(af.GetPhysics()->GetConstraint( steeringHingeName ) );
		if ( !steering[i] ) {
			gameLocal.Error( "anAFEntity_VehicleFourWheels '%s': can't find steering hinge '%s'", name.c_str(), steeringHingeName );
		}
	}

	memset( wheelAngles, 0, sizeof( wheelAngles ) );
	BecomeActive( TH_THINK );
}

/*
================
anAFEntity_VehicleFourWheels::Think
================
*/
void anAFEntity_VehicleFourWheels::Think( void ) {
	int i;
	float force = 0.0f, velocity = 0.0f, steerAngle = 0.0f;
	anVec3 origin;
	anMat3 axis;
	anRotation rotation;

	if ( thinkFlags & TH_THINK ) {

		if ( player ) {
			// capture the input from a player
			velocity = gvhcleVelocity.GetFloat();
			if ( player->usercmd.forwardmove < 0 ) {
				velocity = -velocity;
			}
			force = anMath::Fabs( player->usercmd.forwardmove * gvhcleForce.GetFloat() ) * (1.0f / 128.0f);
			steerAngle = GetSteerAngle();
		}

		// update the wheel motor force
		for ( i = 0; i < 2; i++ ) {
			wheels[2+i]->SetContactMotorVelocity( velocity );
			wheels[2+i]->SetContactMotorForce( force );
		}

		// adjust wheel velocity for better steering because there are no differentials between the wheels
		if ( steerAngle < 0.0f ) {
			wheels[2]->SetContactMotorVelocity( velocity * 0.5f );
		}
		else if ( steerAngle > 0.0f ) {
			wheels[3]->SetContactMotorVelocity( velocity * 0.5f );
		}

		// update the wheel steering
		steering[0]->SetSteerAngle( steerAngle );
		steering[1]->SetSteerAngle( steerAngle );
		for ( i = 0; i < 2; i++ ) {
			steering[i]->SetSteerSpeed( 3.0f );
		}

		// update the steering wheel
		animator.GetJointTransform( steeringWheelJoint, gameLocal.time, origin, axis );
		rotation.SetVec( axis[2] );
		rotation.SetAngle( -steerAngle );
		animator.SetJointAxis( steeringWheelJoint, JOINTMOD_WORLD, rotation.ToMat3() );

		// run the physics
		RunPhysics();

		// rotate the wheels visually
		for ( i = 0; i < 4; i++ ) {
			if ( force == 0.0f ) {
				velocity = wheels[i]->GetLinearVelocity() * wheels[i]->GetWorldAxis()[0];
			}
			wheelAngles[i] += velocity * MS2SEC( gameLocal.msec ) / wheelRadius;
			// give the wheel joint an additional rotation about the wheel axis
			rotation.SetAngle( RAD2DEG( wheelAngles[i] ) );
			axis = af.GetPhysics()->GetAxis( 0 );
			rotation.SetVec( (wheels[i]->GetWorldAxis() * axis.Transpose())[2] );
			animator.SetJointAxis( wheelJoints[i], JOINTMOD_WORLD, rotation.ToMat3() );
		}

		// spawn dust particle effects
		if ( force != 0.0f && !( gameLocal.frameNum & 7 ) ) {
			int numContacts;
			arcAFConstraint_Contact *contacts[2];
			for ( i = 0; i < 4; i++ ) {
				numContacts = af.GetPhysics()->GetBodyContactConstraints( wheels[i]->GetClipModel()->GetId(), contacts, 2 );
				for ( int j = 0; j < numContacts; j++ ) {
					gameLocal.smokeParticles->EmitSmoke( dustSmoke, gameLocal.time, gameLocal.random.RandomFloat(), contacts[j]->GetContact().point, contacts[j]->GetContact().normal.ToMat3() );
				}
			}
		}
	}

	UpdateAnimation();
	if ( thinkFlags & TH_UPDATEVISUALS ) {
		Present();
		LinkCombat();
	}
}


/*
===============================================================================

  anAFEntity_VehicleSixWheels

===============================================================================
*/

CLASS_DECLARATION( anAFEntity_Vehicle, anAFEntity_VehicleSixWheels )
END_CLASS

	/*
================
anAFEntity_VehicleSixWheels::anAFEntity_VehicleSixWheels
================
*/
anAFEntity_VehicleSixWheels::anAFEntity_VehicleSixWheels( void ) {
	int i;

	for ( i = 0; i < 6; i++ ) {
		wheels[i]		= nullptr;
		wheelJoints[i]	= INVALID_JOINT;
		wheelAngles[i]	= 0.0f;
	}
	steering[0]			= nullptr;
	steering[1]			= nullptr;
	steering[2]			= nullptr;
	steering[3]			= nullptr;
}

/*
================
anAFEntity_VehicleSixWheels::Spawn
================
*/
void anAFEntity_VehicleSixWheels::Spawn( void ) {
	int i;
	static const char *wheelBodyKeys[] = {
		"wheelBodyFrontLeft",
		"wheelBodyFrontRight",
		"wheelBodyMiddleLeft",
		"wheelBodyMiddleRight",
		"wheelBodyRearLeft",
		"wheelBodyRearRight"
	};
	static const char *wheelJointKeys[] = {
		"wheelJointFrontLeft",
		"wheelJointFrontRight",
		"wheelJointMiddleLeft",
		"wheelJointMiddleRight",
		"wheelJointRearLeft",
		"wheelJointRearRight"
	};
	static const char *steeringHingeKeys[] = {
		"steeringHingeFrontLeft",
		"steeringHingeFrontRight",
		"steeringHingeRearLeft",
		"steeringHingeRearRight"
	};

	const char *wheelBodyName, *wheelJointName, *steeringHingeName;

	for ( i = 0; i < 6; i++ ) {
		wheelBodyName = spawnArgs.GetString( wheelBodyKeys[i], "" );
		if ( !wheelBodyName[0] ) {
			gameLocal.Error( "anAFEntity_VehicleSixWheels '%s' no '%s' specified", name.c_str(), wheelBodyKeys[i] );
		}
		wheels[i] = af.GetPhysics()->GetBody( wheelBodyName );
		if ( !wheels[i] ) {
			gameLocal.Error( "anAFEntity_VehicleSixWheels '%s' can't find wheel body '%s'", name.c_str(), wheelBodyName );
		}
		wheelJointName = spawnArgs.GetString( wheelJointKeys[i], "" );
		if ( !wheelJointName[0] ) {
			gameLocal.Error( "anAFEntity_VehicleSixWheels '%s' no '%s' specified", name.c_str(), wheelJointKeys[i] );
		}
		wheelJoints[i] = animator.GetJointHandle( wheelJointName );
		if ( wheelJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "anAFEntity_VehicleSixWheels '%s' can't find wheel joint '%s'", name.c_str(), wheelJointName );
		}
	}

	for ( i = 0; i < 4; i++ ) {
		steeringHingeName = spawnArgs.GetString( steeringHingeKeys[i], "" );
		if ( !steeringHingeName[0] ) {
			gameLocal.Error( "anAFEntity_VehicleSixWheels '%s' no '%s' specified", name.c_str(), steeringHingeKeys[i] );
		}
		steering[i] = static_cast<arcAFConstraint_Hinge *>(af.GetPhysics()->GetConstraint( steeringHingeName ) );
		if ( !steering[i] ) {
			gameLocal.Error( "anAFEntity_VehicleSixWheels '%s': can't find steering hinge '%s'", name.c_str(), steeringHingeName );
		}
	}

	memset( wheelAngles, 0, sizeof( wheelAngles ) );
	BecomeActive( TH_THINK );
}

/*
================
anAFEntity_VehicleSixWheels::Think
================
*/
void anAFEntity_VehicleSixWheels::Think( void ) {
	int i;
	float force = 0.0f, velocity = 0.0f, steerAngle = 0.0f;
	anVec3 origin;
	anMat3 axis;
	anRotation rotation;

	if ( thinkFlags & TH_THINK ) {

		if ( player ) {
			// capture the input from a player
			velocity = gvhcleVelocity.GetFloat();
			if ( player->usercmd.forwardmove < 0 ) {
				velocity = -velocity;
			}
			force = anMath::Fabs( player->usercmd.forwardmove * gvhcleForce.GetFloat() ) * (1.0f / 128.0f);
			steerAngle = GetSteerAngle();
		}

		// update the wheel motor force
		for ( i = 0; i < 6; i++ ) {
			wheels[i]->SetContactMotorVelocity( velocity );
			wheels[i]->SetContactMotorForce( force );
		}

		// adjust wheel velocity for better steering because there are no differentials between the wheels
		if ( steerAngle < 0.0f ) {
			for ( i = 0; i < 3; i++ ) {
				wheels[(i<<1)]->SetContactMotorVelocity( velocity * 0.5f );
			}
		}
		else if ( steerAngle > 0.0f ) {
			for ( i = 0; i < 3; i++ ) {
				wheels[1+(i<<1)]->SetContactMotorVelocity( velocity * 0.5f );
			}
		}

		// update the wheel steering
		steering[0]->SetSteerAngle( steerAngle );
		steering[1]->SetSteerAngle( steerAngle );
		steering[2]->SetSteerAngle( -steerAngle );
		steering[3]->SetSteerAngle( -steerAngle );
		for ( i = 0; i < 4; i++ ) {
			steering[i]->SetSteerSpeed( 3.0f );
		}

		// update the steering wheel
		animator.GetJointTransform( steeringWheelJoint, gameLocal.time, origin, axis );
		rotation.SetVec( axis[2] );
		rotation.SetAngle( -steerAngle );
		animator.SetJointAxis( steeringWheelJoint, JOINTMOD_WORLD, rotation.ToMat3() );

		// run the physics
		RunPhysics();

		// rotate the wheels visually
		for ( i = 0; i < 6; i++ ) {
			if ( force == 0.0f ) {
				velocity = wheels[i]->GetLinearVelocity() * wheels[i]->GetWorldAxis()[0];
			}
			wheelAngles[i] += velocity * MS2SEC( gameLocal.msec ) / wheelRadius;
			// give the wheel joint an additional rotation about the wheel axis
			rotation.SetAngle( RAD2DEG( wheelAngles[i] ) );
			axis = af.GetPhysics()->GetAxis( 0 );
			rotation.SetVec( (wheels[i]->GetWorldAxis() * axis.Transpose())[2] );
			animator.SetJointAxis( wheelJoints[i], JOINTMOD_WORLD, rotation.ToMat3() );
		}

		// spawn dust particle effects
		if ( force != 0.0f && !( gameLocal.frameNum & 7 ) ) {
			int numContacts;
			arcAFConstraint_Contact *contacts[2];
			for ( i = 0; i < 6; i++ ) {
				numContacts = af.GetPhysics()->GetBodyContactConstraints( wheels[i]->GetClipModel()->GetId(), contacts, 2 );
				for ( int j = 0; j < numContacts; j++ ) {
					gameLocal.smokeParticles->EmitSmoke( dustSmoke, gameLocal.time, gameLocal.random.RandomFloat(), contacts[j]->GetContact().point, contacts[j]->GetContact().normal.ToMat3() );
				}
			}
		}
	}

	UpdateAnimation();
	if ( thinkFlags & TH_UPDATEVISUALS ) {
		Present();
		LinkCombat();
	}
}


/*
===============================================================================

  anAFEntity_SteamPipe

===============================================================================
*/

CLASS_DECLARATION( anAFEntity_Base, anAFEntity_SteamPipe )
END_CLASS


/*
================
anAFEntity_SteamPipe::anAFEntity_SteamPipe
================
*/
anAFEntity_SteamPipe::anAFEntity_SteamPipe( void ) {
	steamBody			= 0;
	steamForce			= 0.0f;
	steamUpForce		= 0.0f;
	steamModelDefHandle	= -1;
	memset( &steamRenderEntity, 0, sizeof( steamRenderEntity ) );
}

/*
================
anAFEntity_SteamPipe::~anAFEntity_SteamPipe
================
*/
anAFEntity_SteamPipe::~anAFEntity_SteamPipe( void ) {
	if ( steamModelDefHandle >= 0 ){
		gameRenderWorld->FreeEntityDef( steamModelDefHandle );
	}
}

/*
================
anAFEntity_SteamPipe::Save
================
*/
void anAFEntity_SteamPipe::Save( anSaveGame *savefile ) const {
}

/*
================
anAFEntity_SteamPipe::Restore
================
*/
void anAFEntity_SteamPipe::Restore( anRestoreGame *savefile ) {
	Spawn();
}

/*
================
anAFEntity_SteamPipe::Spawn
================
*/
void anAFEntity_SteamPipe::Spawn( void ) {
	anVec3 steamDir;
	const char *steamBodyName;

	LoadAF();

	SetCombatModel();

	SetPhysics( af.GetPhysics() );

	fl.takedamage = true;

	steamBodyName = spawnArgs.GetString( "steamBody", "" );
	steamForce = spawnArgs.GetFloat( "steamForce", "2000" );
	steamUpForce = spawnArgs.GetFloat( "steamUpForce", "10" );
	steamDir = af.GetPhysics()->GetAxis( steamBody )[2];
	steamBody = af.GetPhysics()->GetBodyId( steamBodyName );
	force.SetPosition( af.GetPhysics(), steamBody, af.GetPhysics()->GetOrigin( steamBody ) );
	force.SetForce( steamDir * -steamForce );

	InitSteamRenderEntity();

	BecomeActive( TH_THINK );
}

/*
================
anAFEntity_SteamPipe::InitSteamRenderEntity
================
*/
void anAFEntity_SteamPipe::InitSteamRenderEntity( void ) {
	const char	*temp;
	const anDeclModelDef *modelDef;

	memset( &steamRenderEntity, 0, sizeof( steamRenderEntity ) );
	steamRenderEntity.shaderParms[ SHADERPARM_RED ]		= 1.0f;
	steamRenderEntity.shaderParms[ SHADERPARM_GREEN ]	= 1.0f;
	steamRenderEntity.shaderParms[ SHADERPARM_BLUE ]	= 1.0f;
	modelDef = nullptr;
	temp = spawnArgs.GetString ( "model_steam" );
	if ( *temp != '\0' ) {
		if ( !strstr( temp, "." ) ) {
			modelDef = static_cast<const anDeclModelDef *>( declManager->FindType( DECL_MODELDEF, temp, false ) );
			if ( modelDef ) {
				steamRenderEntity.hModel = modelDef->ModelHandle();
			}
		}

		if ( !steamRenderEntity.hModel ) {
			steamRenderEntity.hModel = renderModelManager->FindModel( temp );
		}

		if ( steamRenderEntity.hModel ) {
			steamRenderEntity.bounds = steamRenderEntity.hModel->Bounds( &steamRenderEntity );
		} else {
			steamRenderEntity.bounds.Zero();
		}
		steamRenderEntity.origin = af.GetPhysics()->GetOrigin( steamBody );
		steamRenderEntity.axis = af.GetPhysics()->GetAxis( steamBody );
		steamModelDefHandle = gameRenderWorld->AddEntityDef( &steamRenderEntity );
	}
}

/*
================
anAFEntity_SteamPipe::Think
================
*/
void anAFEntity_SteamPipe::Think( void ) {
	anVec3 steamDir;

	if ( thinkFlags & TH_THINK ) {
		steamDir.x = gameLocal.random.CRandomFloat() * steamForce;
		steamDir.y = gameLocal.random.CRandomFloat() * steamForce;
		steamDir.z = steamUpForce;
		force.SetForce( steamDir );
		force.Evaluate( gameLocal.time );
		//gameRenderWorld->DebugArrow( colorWhite, af.GetPhysics()->GetOrigin( steamBody ), af.GetPhysics()->GetOrigin( steamBody ) - 10.0f * steamDir, 4 );
	}

	if ( steamModelDefHandle >= 0 ){
		steamRenderEntity.origin = af.GetPhysics()->GetOrigin( steamBody );
		steamRenderEntity.axis = af.GetPhysics()->GetAxis( steamBody );
		gameRenderWorld->UpdateEntityDef( steamModelDefHandle, &steamRenderEntity );
	}

	anAFEntity_Base::Think();
}


/*
===============================================================================

  anAFEntity_ClawFourFingers

===============================================================================
*/

const anEventDef EV_SetFingerAngle( "setFingerAngle", "f" );
const anEventDef EV_StopFingers( "stopFingers" );

CLASS_DECLARATION( anAFEntity_Base, anAFEntity_ClawFourFingers )
	EVENT( EV_SetFingerAngle,		anAFEntity_ClawFourFingers::Event_SetFingerAngle )
	EVENT( EV_StopFingers,			anAFEntity_ClawFourFingers::Event_StopFingers )
END_CLASS

static const char *clawConstraintNames[] = {
	"claw1", "claw2", "claw3", "claw4"
};

/*
================
anAFEntity_ClawFourFingers::anAFEntity_ClawFourFingers
================
*/
anAFEntity_ClawFourFingers::anAFEntity_ClawFourFingers( void ) {
	fingers[0]	= nullptr;
	fingers[1]	= nullptr;
	fingers[2]	= nullptr;
	fingers[3]	= nullptr;
}

/*
================
anAFEntity_ClawFourFingers::Save
================
*/
void anAFEntity_ClawFourFingers::Save( anSaveGame *savefile ) const {
	int i;

	for ( i = 0; i < 4; i++ ) {
		fingers[i]->Save( savefile );
	}
}

/*
================
anAFEntity_ClawFourFingers::Restore
================
*/
void anAFEntity_ClawFourFingers::Restore( anRestoreGame *savefile ) {
	int i;

	for ( i = 0; i < 4; i++ ) {
		fingers[i] = static_cast<arcAFConstraint_Hinge *>(af.GetPhysics()->GetConstraint( clawConstraintNames[i] ) );
		fingers[i]->Restore( savefile );
	}

	SetCombatModel();
	LinkCombat();
}

/*
================
anAFEntity_ClawFourFingers::Spawn
================
*/
void anAFEntity_ClawFourFingers::Spawn( void ) {
	int i;

	LoadAF();

	SetCombatModel();

	af.GetPhysics()->LockWorldConstraints( true );
	af.GetPhysics()->SetForcePushable( true );
	SetPhysics( af.GetPhysics() );

	fl.takedamage = true;

	for ( i = 0; i < 4; i++ ) {
		fingers[i] = static_cast<arcAFConstraint_Hinge *>(af.GetPhysics()->GetConstraint( clawConstraintNames[i] ) );
		if ( !fingers[i] ) {
			gameLocal.Error( "idClaw_FourFingers '%s': can't find claw constraint '%s'", name.c_str(), clawConstraintNames[i] );
		}
	}
}

/*
================
anAFEntity_ClawFourFingers::Event_SetFingerAngle
================
*/
void anAFEntity_ClawFourFingers::Event_SetFingerAngle( float angle ) {
	int i;

	for ( i = 0; i < 4; i++ ) {
		fingers[i]->SetSteerAngle( angle );
		fingers[i]->SetSteerSpeed( 0.5f );
	}
	af.GetPhysics()->Activate();
}

/*
================
anAFEntity_ClawFourFingers::Event_StopFingers
================
*/
void anAFEntity_ClawFourFingers::Event_StopFingers( void ) {
	int i;

	for ( i = 0; i < 4; i++ ) {
		fingers[i]->SetSteerAngle( fingers[i]->GetAngle() );
	}
}


/*
===============================================================================

  editor support routines

===============================================================================
*/


/*
================
anGameEdit::AF_SpawnEntity
================
*/
bool anGameEdit::AF_SpawnEntity( const char *fileName ) {
	anDict args;
	anBasePlayer *player;
	anAFEntity_Generic *ent;
	const anDeclAF *af;
	anVec3 org;
	float yaw;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk( false ) ) {
		return false;
	}

	af = static_cast<const anDeclAF *>( declManager->FindType( DECL_AF, fileName ) );
	if ( !af ) {
		return false;
	}

	yaw = player->viewAngles.yaw;
	args.Set( "angle", va( "%f", yaw + 180 ) );
	org = player->GetPhysics()->GetOrigin() + anAngles( 0, yaw, 0 ).ToForward() * 80 + anVec3( 0, 0, 1 );
	args.Set( "origin", org.ToString() );
	args.Set( "spawnclass", "anAFEntity_Generic" );
	if ( af->model[0] ) {
		args.Set( "model", af->model.c_str() );
	} else {
		args.Set( "model", fileName );
	}
	if ( af->skin[0] ) {
		args.Set( "skin", af->skin.c_str() );
	}
	args.Set( "articulatedFigure", fileName );
	args.Set( "nodrop", "1" );
	ent = static_cast<anAFEntity_Generic *>(gameLocal.SpawnEntityType( anAFEntity_Generic::Type, &args));

	// always update this entity
	ent->BecomeActive( TH_THINK );
	ent->KeepRunningPhysics();
	ent->fl.forcePhysicsUpdate = true;

	player->dragEntity.SetSelected( ent );

	return true;
}

/*
================
anGameEdit::AF_UpdateEntities
================
*/
void anGameEdit::AF_UpdateEntities( const char *fileName ) {
	anEntity *ent;
	anAFEntity_Base *af;
	anStr name;

	name = fileName;
	name.StripFileExtension();

	// reload any anAFEntity_Generic which uses the given articulated figure file
	for ( ent = gameLocal.spawnedEntities.Next(); ent != nullptr; ent = ent->spawnNode.Next() ) {
		if ( ent->IsType( anAFEntity_Base::Type ) ) {
			af = static_cast<anAFEntity_Base *>(ent);
			if ( name.Icmp( af->GetAFName() ) == 0 ) {
				af->LoadAF();
				af->GetAFPhysics()->PutToRest();
			}
		}
	}
}

/*
================
anGameEdit::AF_UndoChanges
================
*/
void anGameEdit::AF_UndoChanges( void ) {
	int i, c;
	anEntity *ent;
	anAFEntity_Base *af;
	anDeclAF *decl;

	c = declManager->GetNumDecls( DECL_AF );
	for ( i = 0; i < c; i++ ) {
		decl = static_cast<anDeclAF *>( const_cast<anDecl *>( declManager->DeclByIndex( DECL_AF, i, false ) ) );
		if ( !decl->modified ) {
			continue;
		}

		decl->Invalidate();
		declManager->FindType( DECL_AF, decl->GetName() );

		// reload all AF entities using the file
		for ( ent = gameLocal.spawnedEntities.Next(); ent != nullptr; ent = ent->spawnNode.Next() ) {
			if ( ent->IsType( anAFEntity_Base::Type ) ) {
				af = static_cast<anAFEntity_Base *>(ent);
				if ( anStr::Icmp( decl->GetName(), af->GetAFName() ) == 0 ) {
					af->LoadAF();
				}
			}
		}
	}
}

/*
================
GetJointTransform
================
*/
typedef struct {
	renderEntity_t *ent;
	const anMD5Joint *joints;
} jointTransformData_t;

static bool GetJointTransform( void *model, const anJointMat *frame, const char *jointName, anVec3 &origin, anMat3 &axis ) {
	int i;
	jointTransformData_t *data = reinterpret_cast<jointTransformData_t *>(model);

	for ( i = 0; i < data->ent->numJoints; i++ ) {
		if ( data->joints[i].name.Icmp( jointName ) == 0 ) {
			break;
		}
	}
	if ( i >= data->ent->numJoints ) {
		return false;
	}
	origin = frame[i].ToVec3();
	axis = frame[i].ToMat3();
	return true;
}

/*
================
GetArgString
================
*/
static const char *GetArgString( const anDict &args, const anDict *defArgs, const char *key ) {
	const char *s;

	s = args.GetString( key );
	if ( !s[0] && defArgs ) {
		s = defArgs->GetString( key );
	}
	return s;
}

/*
================
anGameEdit::AF_CreateMesh
================
*/
anRenderModel *anGameEdit::AF_CreateMesh( const anDict &args, anVec3 &meshOrigin, anMat3 &meshAxis, bool &poseIsSet ) {
	int i, jointNum;
	const anDeclAF *af;
	const anDeclAF_Body *fb;
	renderEntity_t ent;
	anVec3 origin, *bodyOrigin, *newBodyOrigin, *modifiedOrigin;
	anMat3 axis, *bodyAxis, *newBodyAxis, *modifiedAxis;
	declAFJointMod_t *jointMod;
	anAngles angles;
	const anDict *defArgs;
	const anKeyValue *arg;
	anStr name;
	jointTransformData_t data;
	const char *classname, *afName, *modelName;
	anRenderModel *md5;
	const anDeclModelDef *modelDef;
	const anMD6Anim *MD5anim;
	const anMD5Joint *MD5joint;
	const anMD5Joint *MD5joints;
	int numMD5joints;
	anJointMat *originalJoints;
	int parentNum;

	poseIsSet = false;
	meshOrigin.Zero();
	meshAxis.Identity();

	classname = args.GetString( "classname" );
	defArgs = gameLocal.FindEntityDefDict( classname );

	// get the articulated figure
	afName = GetArgString( args, defArgs, "articulatedFigure" );
	af = static_cast<const anDeclAF *>( declManager->FindType( DECL_AF, afName ) );
	if ( !af ) {
		return nullptr;
	}

	// get the md5 model
	modelName = GetArgString( args, defArgs, "model" );
	modelDef = static_cast< const anDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelName, false ) );
	if ( !modelDef ) {
		return nullptr;
	}

	// make sure model hasn't been purged
	if ( modelDef->ModelHandle() && !modelDef->ModelHandle()->IsLoaded() ) {
		modelDef->ModelHandle()->LoadModel();
	}

	// get the md5
	md5 = modelDef->ModelHandle();
	if ( !md5 || md5->IsDefaultModel() ) {
		return nullptr;
	}

	// get the articulated figure pose anim
	int animNum = modelDef->GetAnim( "af_pose" );
	if ( !animNum ) {
		return nullptr;
	}
	const anAnim *anim = modelDef->GetAnim( animNum );
	if ( !anim ) {
		return nullptr;
	}
	MD5anim = anim->MD5Anim( 0 );
	MD5joints = md5->GetJoints();
	numMD5joints = md5->NumJoints();

	// setup a render entity
	memset( &ent, 0, sizeof( ent ) );
	ent.customSkin = modelDef->GetSkin();
	ent.bounds.Clear();
	ent.numJoints = numMD5joints;
	ent.joints = ( anJointMat * )_alloca16( ent.numJoints * sizeof( *ent.joints ) );

	// create animation from of the af_pose
	ANIM_CreateAnimFrame( md5, MD5anim, ent.numJoints, ent.joints, 1, modelDef->GetVisualOffset(), false );

	// buffers to store the initial origin and axis for each body
	bodyOrigin = (anVec3 *) _alloca16( af->bodies.Num() * sizeof( anVec3 ) );
	bodyAxis = (anMat3 *) _alloca16( af->bodies.Num() * sizeof( anMat3 ) );
	newBodyOrigin = (anVec3 *) _alloca16( af->bodies.Num() * sizeof( anVec3 ) );
	newBodyAxis = (anMat3 *) _alloca16( af->bodies.Num() * sizeof( anMat3 ) );

	// finish the AF positions
	data.ent = &ent;
	data.joints = MD5joints;
	af->Finish( GetJointTransform, ent.joints, &data );

	// get the initial origin and axis for each AF body
	for ( i = 0; i < af->bodies.Num(); i++ ) {
		fb = af->bodies[i];

		if ( fb->modelType == TRM_BONE ) {
			// axis of bone trace model
			axis[2] = fb->v2.ToVec3() - fb->v1.ToVec3();
			axis[2].Normalize();
			axis[2].NormalVectors( axis[0], axis[1] );
			axis[1] = -axis[1];
		} else {
			axis = fb->angles.ToMat3();
		}

		newBodyOrigin[i] = bodyOrigin[i] = fb->origin.ToVec3();
		newBodyAxis[i] = bodyAxis[i] = axis;
	}

	// get any new body transforms stored in the key/value pairs
	for ( arg = args.MatchPrefix( "body ", nullptr ); arg; arg = args.MatchPrefix( "body ", arg ) ) {
		name = arg->GetKey();
		name.Strip( "body " );
		for ( i = 0; i < af->bodies.Num(); i++ ) {
			fb = af->bodies[i];
			if ( fb->name.Icmp( name ) == 0 ) {
				break;
			}
		}
		if ( i >= af->bodies.Num() ) {
			continue;
		}
		sscanf( arg->GetValue(), "%f %f %f %f %f %f", &origin.x, &origin.y, &origin.z, &angles.pitch, &angles.yaw, &angles.roll );

		if ( fb->jointName.Icmp( "origin" ) == 0 ) {
			meshAxis = bodyAxis[i].Transpose() * angles.ToMat3();
			meshOrigin = origin - bodyOrigin[i] * meshAxis;
			poseIsSet = true;
		} else {
			newBodyOrigin[i] = origin;
			newBodyAxis[i] = angles.ToMat3();
		}
	}

	// save the original joints
	originalJoints = ( anJointMat * )_alloca16( numMD5joints * sizeof( originalJoints[0] ) );
	memcpy( originalJoints, ent.joints, numMD5joints * sizeof( originalJoints[0] ) );

	// buffer to store the joint mods
	jointMod = (declAFJointMod_t *) _alloca16( numMD5joints * sizeof( declAFJointMod_t ) );
	memset( jointMod, -1, numMD5joints * sizeof( declAFJointMod_t ) );
	modifiedOrigin = (anVec3 *) _alloca16( numMD5joints * sizeof( anVec3 ) );
	memset( modifiedOrigin, 0, numMD5joints * sizeof( anVec3 ) );
	modifiedAxis = (anMat3 *) _alloca16( numMD5joints * sizeof( anMat3 ) );
	memset( modifiedAxis, 0, numMD5joints * sizeof( anMat3 ) );

	// get all the joint modifications
	for ( i = 0; i < af->bodies.Num(); i++ ) {
		fb = af->bodies[i];

		if ( fb->jointName.Icmp( "origin" ) == 0 ) {
			continue;
		}

		for ( jointNum = 0; jointNum < numMD5joints; jointNum++ ) {
			if ( MD5joints[jointNum].name.Icmp( fb->jointName ) == 0 ) {
				break;
			}
		}

		if ( jointNum >= 0 && jointNum < ent.numJoints ) {
			jointMod[ jointNum ] = fb->jointMod;
			modifiedAxis[ jointNum ] = ( bodyAxis[i] * originalJoints[jointNum].ToMat3().Transpose() ).Transpose() * ( newBodyAxis[i] * meshAxis.Transpose() );
			// FIXME: calculate correct modifiedOrigin
			modifiedOrigin[ jointNum ] = originalJoints[ jointNum ].ToVec3();
 		}
	}

	// apply joint modifications to the skeleton
	MD5joint = MD5joints + 1;
	for ( i = 1; i < numMD5joints; i++, MD5joint++ ) {

		parentNum = MD5joint->parent - MD5joints;
		anMat3 parentAxis = originalJoints[ parentNum ].ToMat3();
		anMat3 localm = originalJoints[i].ToMat3() * parentAxis.Transpose();
		anVec3 localt = ( originalJoints[i].ToVec3() - originalJoints[ parentNum ].ToVec3() ) * parentAxis.Transpose();

		switch ( jointMod[i] ) {
			case DECLAF_JOINTMOD_ORIGIN: {
				ent.joints[i].SetRotation( localm * ent.joints[ parentNum ].ToMat3() );
				ent.joints[i].SetTranslation( modifiedOrigin[i] );
				break;
			}
			case DECLAF_JOINTMOD_AXIS: {
				ent.joints[i].SetRotation( modifiedAxis[i] );
				ent.joints[i].SetTranslation( ent.joints[ parentNum ].ToVec3() + localt * ent.joints[ parentNum ].ToMat3() );
				break;
			}
			case DECLAF_JOINTMOD_BOTH: {
				ent.joints[i].SetRotation( modifiedAxis[i] );
				ent.joints[i].SetTranslation( modifiedOrigin[i] );
				break;
			}
			default: {
				ent.joints[i].SetRotation( localm * ent.joints[ parentNum ].ToMat3() );
				ent.joints[i].SetTranslation( ent.joints[ parentNum ].ToVec3() + localt * ent.joints[ parentNum ].ToMat3() );
				break;
			}
		}
	}

	// instantiate a mesh using the joint information from the render entity
	return md5->InstantiateDynamicModel( &ent, nullptr, nullptr );
}
