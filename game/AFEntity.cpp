#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"


/*
===============================================================================

  arcMultiModelAF

===============================================================================
*/

CLASS_DECLARATION( arcEntity, arcMultiModelAF )
END_CLASS

/*
================
arcMultiModelAF::Spawn
================
*/
void arcMultiModelAF::Spawn( void ) {
	physicsObj.SetSelf( this );
}

/*
================
arcMultiModelAF::~arcMultiModelAF
================
*/
arcMultiModelAF::~arcMultiModelAF( void ) {
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
arcMultiModelAF::SetModelForId
================
*/
void arcMultiModelAF::SetModelForId( int id, const idStr &modelName ) {
	modelHandles.AssureSize( id+1, NULL );
	modelDefHandles.AssureSize( id+1, -1 );
	modelHandles[id] = renderModelManager->FindModel( modelName );
}

/*
================
arcMultiModelAF::Present
================
*/
void arcMultiModelAF::Present( void ) {
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
arcMultiModelAF::Think
================
*/
void arcMultiModelAF::Think( void ) {
	RunPhysics();
	Present();
}


/*
===============================================================================

  arcNetChain

===============================================================================
*/

CLASS_DECLARATION( arcMultiModelAF, arcNetChain )
END_CLASS

/*
================
arcNetChain::BuildChain

  builds a chain hanging down from the ceiling
  the highest link is a child of the link below it etc.
  this allows an object to be attached to multiple chains while keeping a single tree structure
================
*/
void arcNetChain::BuildChain( const idStr &name, const arcVec3 &origin, float linkLength, float linkWidth, float density, int numLinks, bool bindToWorld ) {
	int i;
	float halfLinkLength = linkLength * 0.5f;
	arcTraceModel trm;
	arcClipModel *clip;
	arcAFBody *body, *lastBody;
	arcAFConstraint_BallAndSocketJoint *bsj;
	arcAFConstraint_UniversalJoint *uj;
	arcVec3 org;

	// create a trace model
	trm = arcTraceModel( linkLength, linkWidth );
	trm.Translate( -trm.offset );

	org = origin - arcVec3( 0, 0, halfLinkLength );

	lastBody = NULL;
	for ( i = 0; i < numLinks; i++ ) {

		// add body
		clip = new arcClipModel( trm );
		clip->SetContents( CONTENTS_SOLID );
		clip->Link( gameLocal.clip, this, 0, org, mat3_identity );
		body = new arcAFBody( name + idStr(i), clip, density );
		physicsObj.AddBody( body );

		// visual model for body
		SetModelForId( physicsObj.GetBodyId( body ), spawnArgs.GetString( "model" ) );

		// add constraint
		if ( bindToWorld ) {
			if ( !lastBody ) {
				uj = new arcAFConstraint_UniversalJoint( name + idStr(i), body, lastBody );
				uj->SetShafts( arcVec3( 0, 0, -1 ), arcVec3( 0, 0, 1 ) );
				//uj->SetConeLimit( arcVec3( 0, 0, -1 ), 30.0f );
				//uj->SetPyramidLimit( arcVec3( 0, 0, -1 ), arcVec3( 1, 0, 0 ), 90.0f, 30.0f );
			}
			else {
				uj = new arcAFConstraint_UniversalJoint( name + idStr(i), lastBody, body );
				uj->SetShafts( arcVec3( 0, 0, 1 ), arcVec3( 0, 0, -1 ) );
				//uj->SetConeLimit( arcVec3( 0, 0, 1 ), 30.0f );
			}
			uj->SetAnchor( org + arcVec3( 0, 0, halfLinkLength ) );
			uj->SetFriction( 0.9f );
			physicsObj.AddConstraint( uj );
		}
		else {
			if ( lastBody ) {
				bsj = new arcAFConstraint_BallAndSocketJoint( "joint" + idStr(i), lastBody, body );
				bsj->SetAnchor( org + arcVec3( 0, 0, halfLinkLength ) );
				bsj->SetConeLimit( arcVec3( 0, 0, 1 ), 60.0f, arcVec3( 0, 0, 1 ) );
				physicsObj.AddConstraint( bsj );
			}
		}

		org[2] -= linkLength;

		lastBody = body;
	}
}

/*
================
arcNetChain::Spawn
================
*/
void arcNetChain::Spawn( void ) {
	int numLinks;
	float length, linkLength, linkWidth, density;
	bool drop;
	arcVec3 origin;

	spawnArgs.GetBool( "drop", "0", drop );
	spawnArgs.GetInt( "links", "3", numLinks );
	spawnArgs.GetFloat( "length", idStr( numLinks * 32.0f ), length );
	spawnArgs.GetFloat( "width", "8", linkWidth );
	spawnArgs.GetFloat( "density", "0.2", density );
	linkLength = length / numLinks;
	origin = GetPhysics()->GetOrigin();

	// initialize physics
	physicsObj.SetSelf( this );
	physicsObj.SetGravity( gameLocal.GetGravity() );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_BODY );
	SetPhysics( &physicsObj );

	BuildChain( "link", origin, linkLength, linkWidth, density, numLinks, !drop );
}

/*
===============================================================================

  arcAFAttachment

===============================================================================
*/

CLASS_DECLARATION( arcAnimatedEntity, arcAFAttachment )
END_CLASS

/*
=====================
arcAFAttachment::arcAFAttachment
=====================
*/
arcAFAttachment::arcAFAttachment( void ) {
	body			= NULL;
	combatModel		= NULL;
	idleAnim		= 0;
	attachJoint		= INVALID_JOINT;
}

/*
=====================
arcAFAttachment::~arcAFAttachment
=====================
*/
arcAFAttachment::~arcAFAttachment( void ) {

	StopSound( SND_CHANNEL_ANY, false );

	delete combatModel;
	combatModel = NULL;
}

/*
=====================
arcAFAttachment::Spawn
=====================
*/
void arcAFAttachment::Spawn( void ) {
	idleAnim = animator.GetAnim( "idle" );
}

/*
=====================
arcAFAttachment::SetBody
=====================
*/
void arcAFAttachment::SetBody( arcEntity *bodyEnt, const char *model, jointHandle_t attachJoint ) {
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
arcAFAttachment::ClearBody
=====================
*/
void arcAFAttachment::ClearBody( void ) {
	body = NULL;
	attachJoint = INVALID_JOINT;
	Hide();
}

/*
=====================
arcAFAttachment::GetBody
=====================
*/
arcEntity *arcAFAttachment::GetBody( void ) const {
	return body;
}

/*
================
arcAFAttachment::Save

archive object for savegame file
================
*/
void arcAFAttachment::Save( arcSaveGame *savefile ) const {
	savefile->WriteObject( body );
	savefile->WriteInt( idleAnim );
	savefile->WriteJoint( attachJoint );
}

/*
================
arcAFAttachment::Restore

unarchives object from save game file
================
*/
void arcAFAttachment::Restore( arcRestoreGame *savefile ) {
	savefile->ReadObject( reinterpret_cast<idClass *&>( body ) );
	savefile->ReadInt( idleAnim );
	savefile->ReadJoint( attachJoint );

	SetCombatModel();
	LinkCombat();
}

/*
================
arcAFAttachment::Hide
================
*/
void arcAFAttachment::Hide( void ) {
	arcEntity::Hide();
	UnlinkCombat();
}

/*
================
arcAFAttachment::Show
================
*/
void arcAFAttachment::Show( void ) {
	arcEntity::Show();
	LinkCombat();
}

/*
============
arcAFAttachment::Damage

Pass damage to body at the bindjoint
============
*/
void arcAFAttachment::Damage( arcEntity *inflictor, arcEntity *attacker, const arcVec3 &dir,
	const char *damageDefName, const float damageScale, const int location ) {

	if ( body ) {
		body->Damage( inflictor, attacker, dir, damageDefName, damageScale, attachJoint );
	}
}

/*
================
arcAFAttachment::AddDamageEffect
================
*/
void arcAFAttachment::AddDamageEffect( const trace_t &collision, const arcVec3 &velocity, const char *damageDefName ) {
	if ( body ) {
		trace_t c = collision;
		c.c.id = JOINT_HANDLE_TO_CLIPMODEL_ID( attachJoint );
		body->AddDamageEffect( c, velocity, damageDefName );
	}
}

/*
================
arcAFAttachment::GetImpactInfo
================
*/
void arcAFAttachment::GetImpactInfo( arcEntity *ent, int id, const arcVec3 &point, impactInfo_t *info ) {
	if ( body ) {
		body->GetImpactInfo( ent, JOINT_HANDLE_TO_CLIPMODEL_ID( attachJoint ), point, info );
	} else {
		arcEntity::GetImpactInfo( ent, id, point, info );
	}
}

/*
================
arcAFAttachment::ApplyImpulse
================
*/
void arcAFAttachment::ApplyImpulse( arcEntity *ent, int id, const arcVec3 &point, const arcVec3 &impulse ) {
	if ( body ) {
		body->ApplyImpulse( ent, JOINT_HANDLE_TO_CLIPMODEL_ID( attachJoint ), point, impulse );
	} else {
		arcEntity::ApplyImpulse( ent, id, point, impulse );
	}
}

/*
================
arcAFAttachment::AddForce
================
*/
void arcAFAttachment::AddForce( arcEntity *ent, int id, const arcVec3 &point, const arcVec3 &force ) {
	if ( body ) {
		body->AddForce( ent, JOINT_HANDLE_TO_CLIPMODEL_ID( attachJoint ), point, force );
	} else {
		arcEntity::AddForce( ent, id, point, force );
	}
}

/*
================
arcAFAttachment::PlayIdleAnim
================
*/
void arcAFAttachment::PlayIdleAnim( int blendTime ) {
	if ( idleAnim && ( idleAnim != animator.CurrentAnim( ANIMCHANNEL_ALL )->AnimNum() ) ) {
		animator.CycleAnim( ANIMCHANNEL_ALL, idleAnim, gameLocal.time, blendTime );
	}
}

/*
================
idAfAttachment::Think
================
*/
void arcAFAttachment::Think( void ) {
	arcAnimatedEntity::Think();
	if ( thinkFlags & TH_UPDATEPARTICLES ) {
		UpdateDamageEffects();
	}
}

/*
================
arcAFAttachment::SetCombatModel
================
*/
void arcAFAttachment::SetCombatModel( void ) {
	if ( combatModel ) {
		combatModel->Unlink();
		combatModel->LoadModel( modelDefHandle );
	} else {
		combatModel = new arcClipModel( modelDefHandle );
	}
	combatModel->SetOwner( body );
}

/*
================
arcAFAttachment::GetCombatModel
================
*/
arcClipModel *arcAFAttachment::GetCombatModel( void ) const {
	return combatModel;
}

/*
================
arcAFAttachment::LinkCombat
================
*/
void arcAFAttachment::LinkCombat( void ) {
	if ( fl.hidden ) {
		return;
	}

	if ( combatModel ) {
		combatModel->Link( gameLocal.clip, this, 0, renderEntity.origin, renderEntity.axis, modelDefHandle );
	}
}

/*
================
arcAFAttachment::UnlinkCombat
================
*/
void arcAFAttachment::UnlinkCombat( void ) {
	if ( combatModel ) {
		combatModel->Unlink();
	}
}


/*
===============================================================================

  arcAFEntity_Base

===============================================================================
*/

const arcEventDef EV_SetConstraintPosition( "SetConstraintPosition", "sv" );

CLASS_DECLARATION( arcAnimatedEntity, arcAFEntity_Base )
	EVENT( EV_SetConstraintPosition,	arcAFEntity_Base::Event_SetConstraintPosition )
END_CLASS

static const float BOUNCE_SOUND_MIN_VELOCITY	= 80.0f;
static const float BOUNCE_SOUND_MAX_VELOCITY	= 200.0f;

/*
================
arcAFEntity_Base::arcAFEntity_Base
================
*/
arcAFEntity_Base::arcAFEntity_Base( void ) {
	combatModel = NULL;
	combatModelContents = 0;
	nextSoundTime = 0;
	spawnOrigin.Zero();
	spawnAxis.Identity();
}

/*
================
arcAFEntity_Base::~arcAFEntity_Base
================
*/
arcAFEntity_Base::~arcAFEntity_Base( void ) {
	delete combatModel;
	combatModel = NULL;
}

/*
================
arcAFEntity_Base::Save
================
*/
void arcAFEntity_Base::Save( arcSaveGame *savefile ) const {
	savefile->WriteInt( combatModelContents );
	savefile->WriteClipModel( combatModel );
	savefile->WriteVec3( spawnOrigin );
	savefile->WriteMat3( spawnAxis );
	savefile->WriteInt( nextSoundTime );
	af.Save( savefile );
}

/*
================
arcAFEntity_Base::Restore
================
*/
void arcAFEntity_Base::Restore( arcRestoreGame *savefile ) {
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
arcAFEntity_Base::Spawn
================
*/
void arcAFEntity_Base::Spawn( void ) {
	spawnOrigin = GetPhysics()->GetOrigin();
	spawnAxis = GetPhysics()->GetAxis();
	nextSoundTime = 0;
}

/*
================
arcAFEntity_Base::LoadAF
================
*/
bool arcAFEntity_Base::LoadAF( void ) {
	idStr fileName;

	if ( !spawnArgs.GetString( "articulatedFigure", "*unknown*", fileName ) ) {
		return false;
	}

	af.SetAnimator( GetAnimator() );
	if ( !af.Load( this, fileName ) ) {
		gameLocal.Error( "arcAFEntity_Base::LoadAF: Couldn't load af file '%s' on entity '%s'", fileName.c_str(), name.c_str() );
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
arcAFEntity_Base::Think
================
*/
void arcAFEntity_Base::Think( void ) {
	RunPhysics();
	UpdateAnimation();
	if ( thinkFlags & TH_UPDATEVISUALS ) {
		Present();
		LinkCombat();
	}
}

/*
================
arcAFEntity_Base::BodyForClipModelId
================
*/
int arcAFEntity_Base::BodyForClipModelId( int id ) const {
	return af.BodyForClipModelId( id );
}

/*
================
arcAFEntity_Base::SaveState
================
*/
void arcAFEntity_Base::SaveState( arcDict &args ) const {
	const idKeyValue *kv;

	// save the ragdoll pose
	af.SaveState( args );

	// save all the bind constraints
	kv = spawnArgs.MatchPrefix( "bindConstraint ", NULL );
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
arcAFEntity_Base::LoadState
================
*/
void arcAFEntity_Base::LoadState( const arcDict &args ) {
	af.LoadState( args );
}

/*
================
arcAFEntity_Base::AddBindConstraints
================
*/
void arcAFEntity_Base::AddBindConstraints( void ) {
	af.AddBindConstraints();
}

/*
================
arcAFEntity_Base::RemoveBindConstraints
================
*/
void arcAFEntity_Base::RemoveBindConstraints( void ) {
	af.RemoveBindConstraints();
}

/*
================
arcAFEntity_Base::GetImpactInfo
================
*/
void arcAFEntity_Base::GetImpactInfo( arcEntity *ent, int id, const arcVec3 &point, impactInfo_t *info ) {
	if ( af.IsActive() ) {
		af.GetImpactInfo( ent, id, point, info );
	} else {
		arcEntity::GetImpactInfo( ent, id, point, info );
	}
}

/*
================
arcAFEntity_Base::ApplyImpulse
================
*/
void arcAFEntity_Base::ApplyImpulse( arcEntity *ent, int id, const arcVec3 &point, const arcVec3 &impulse ) {
	if ( af.IsLoaded() ) {
		af.ApplyImpulse( ent, id, point, impulse );
	}
	if ( !af.IsActive() ) {
		arcEntity::ApplyImpulse( ent, id, point, impulse );
	}
}

/*
================
arcAFEntity_Base::AddForce
================
*/
void arcAFEntity_Base::AddForce( arcEntity *ent, int id, const arcVec3 &point, const arcVec3 &force ) {
	if ( af.IsLoaded() ) {
		af.AddForce( ent, id, point, force );
	}
	if ( !af.IsActive() ) {
		arcEntity::AddForce( ent, id, point, force );
	}
}

/*
================
arcAFEntity_Base::Collide
================
*/
bool arcAFEntity_Base::Collide( const trace_t &collision, const arcVec3 &velocity ) {
	float v, f;

	if ( af.IsActive() ) {
		v = -( velocity * collision.c.normal );
		if ( v > BOUNCE_SOUND_MIN_VELOCITY && gameLocal.time > nextSoundTime ) {
			f = v > BOUNCE_SOUND_MAX_VELOCITY ? 1.0f : arcMath::Sqrt( v - BOUNCE_SOUND_MIN_VELOCITY ) * ( 1.0f / arcMath::Sqrt( BOUNCE_SOUND_MAX_VELOCITY - BOUNCE_SOUND_MIN_VELOCITY ) );
			if ( StartSound( "snd_bounce", SND_CHANNEL_ANY, 0, false, NULL ) ) {
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
arcAFEntity_Base::GetPhysicsToVisualTransform
================
*/
bool arcAFEntity_Base::GetPhysicsToVisualTransform( arcVec3 &origin, arcMat3 &axis ) {
	if ( af.IsActive() ) {
		af.GetPhysicsToVisualTransform( origin, axis );
		return true;
	}
	return arcEntity::GetPhysicsToVisualTransform( origin, axis );
}

/*
================
arcAFEntity_Base::UpdateAnimationControllers
================
*/
bool arcAFEntity_Base::UpdateAnimationControllers( void ) {
	if ( af.IsActive() ) {
		if ( af.UpdateAnimation() ) {
			return true;
		}
	}
	return false;
}

/*
================
arcAFEntity_Base::SetCombatModel
================
*/
void arcAFEntity_Base::SetCombatModel( void ) {
	if ( combatModel ) {
		combatModel->Unlink();
		combatModel->LoadModel( modelDefHandle );
	} else {
		combatModel = new arcClipModel( modelDefHandle );
	}
}

/*
================
arcAFEntity_Base::GetCombatModel
================
*/
arcClipModel *arcAFEntity_Base::GetCombatModel( void ) const {
	return combatModel;
}

/*
================
arcAFEntity_Base::SetCombatContents
================
*/
void arcAFEntity_Base::SetCombatContents( bool enable ) {
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
arcAFEntity_Base::LinkCombat
================
*/
void arcAFEntity_Base::LinkCombat( void ) {
	if ( fl.hidden ) {
		return;
	}
	if ( combatModel ) {
		combatModel->Link( gameLocal.clip, this, 0, renderEntity.origin, renderEntity.axis, modelDefHandle );
	}
}

/*
================
arcAFEntity_Base::UnlinkCombat
================
*/
void arcAFEntity_Base::UnlinkCombat( void ) {
	if ( combatModel ) {
		combatModel->Unlink();
	}
}

/*
================
arcAFEntity_Base::FreeModelDef
================
*/
void arcAFEntity_Base::FreeModelDef( void ) {
	UnlinkCombat();
	arcEntity::FreeModelDef();
}

/*
===============
arcAFEntity_Base::ShowEditingDialog
===============
*/
void arcAFEntity_Base::ShowEditingDialog( void ) {
	common->InitTool( EDITOR_AF, &spawnArgs );
}

/*
================
arcAFEntity_Base::DropAFs

  The entity should have the following key/value pairs set:
	"def_drop<type>AF"		"af def"
	"drop<type>Skin"		"skin name"
  To drop multiple articulated figures the following key/value pairs can be used:
	"def_drop<type>AF*"		"af def"
  where * is an aribtrary string.
================
*/
void arcAFEntity_Base::DropAFs( arcEntity *ent, const char *type, arcList<arcEntity *> *list ) {
	const idKeyValue *kv;
	const char *skinName;
	arcEntity *newEnt;
	arcAFEntity_Base *af;
	arcDict args;
	const arcDeclSkin *skin;

	// drop the articulated figures
	kv = ent->spawnArgs.MatchPrefix( va( "def_drop%sAF", type ), NULL );
	while ( kv ) {

		args.Set( "classname", kv->GetValue() );
		gameLocal.SpawnEntityDef( args, &newEnt );

		if ( newEnt && newEnt->IsType( arcAFEntity_Base::Type ) ) {
			af = static_cast<arcAFEntity_Base *>(newEnt);
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
arcAFEntity_Base::Event_SetConstraintPosition
================
*/
void arcAFEntity_Base::Event_SetConstraintPosition( const char *name, const arcVec3 &pos ) {
	af.SetConstraintPosition( name, pos );
}

/*
===============================================================================

arcAFEntity_Gibbable

===============================================================================
*/

const arcEventDef EV_Gib( "gib", "s" );
const arcEventDef EV_Gibbed( "<gibbed>" );

CLASS_DECLARATION( arcAFEntity_Base, arcAFEntity_Gibbable )
	EVENT( EV_Gib,		arcAFEntity_Gibbable::Event_Gib )
	EVENT( EV_Gibbed,	arcAFEntity_Base::Event_Remove )
END_CLASS


/*
================
arcAFEntity_Gibbable::arcAFEntity_Gibbable
================
*/
arcAFEntity_Gibbable::arcAFEntity_Gibbable( void ) {
	skeletonModel = NULL;
	skeletonModelDefHandle = -1;
	gibbed = false;
}

/*
================
arcAFEntity_Gibbable::~arcAFEntity_Gibbable
================
*/
arcAFEntity_Gibbable::~arcAFEntity_Gibbable() {
	if ( skeletonModelDefHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( skeletonModelDefHandle );
		skeletonModelDefHandle = -1;
	}
}

/*
================
arcAFEntity_Gibbable::Save
================
*/
void arcAFEntity_Gibbable::Save( arcSaveGame *savefile ) const {
	savefile->WriteBool( gibbed );
	savefile->WriteBool( combatModel != NULL );
}

/*
================
arcAFEntity_Gibbable::Restore
================
*/
void arcAFEntity_Gibbable::Restore( arcRestoreGame *savefile ) {
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
arcAFEntity_Gibbable::Spawn
================
*/
void arcAFEntity_Gibbable::Spawn( void ) {
	InitSkeletonModel();

	gibbed = false;
}

/*
================
arcAFEntity_Gibbable::InitSkeletonModel
================
*/
void arcAFEntity_Gibbable::InitSkeletonModel( void ) {
	const char *modelName;
	const arcDeclModelDef *modelDef;

	skeletonModel = NULL;
	skeletonModelDefHandle = -1;

	modelName = spawnArgs.GetString( "model_gib" );

	modelDef = NULL;
	if ( modelName[0] != '\0' ) {
		modelDef = static_cast<const arcDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelName, false ) );
		if ( modelDef ) {
			skeletonModel = modelDef->ModelHandle();
		} else {
			skeletonModel = renderModelManager->FindModel( modelName );
		}
		if ( skeletonModel != NULL && renderEntity.hModel != NULL ) {
			if ( skeletonModel->NumJoints() != renderEntity.hModel->NumJoints() ) {
				gameLocal.Error( "gib model '%s' has different number of joints than model '%s'",
									skeletonModel->Name(), renderEntity.hModel->Name() );
			}
		}
	}
}

/*
================
arcAFEntity_Gibbable::Present
================
*/
void arcAFEntity_Gibbable::Present( void ) {
	renderEntity_t skeleton;

	if ( !gameLocal.isNewFrame ) {
		return;
	}

	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}

	// update skeleton model
	if ( gibbed && !IsHidden() && skeletonModel != NULL ) {
		skeleton = renderEntity;
		skeleton.hModel = skeletonModel;
		// add to refresh list
		if ( skeletonModelDefHandle == -1 ) {
			skeletonModelDefHandle = gameRenderWorld->AddEntityDef( &skeleton );
		} else {
			gameRenderWorld->UpdateEntityDef( skeletonModelDefHandle, &skeleton );
		}
	}

	arcEntity::Present();
}

/*
================
arcAFEntity_Gibbable::Damage
================
*/
void arcAFEntity_Gibbable::Damage( arcEntity *inflictor, arcEntity *attacker, const arcVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	if ( !fl.takedamage ) {
		return;
	}
	arcAFEntity_Base::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
	if ( health < -20 && spawnArgs.GetBool( "gib" ) ) {
		Gib( dir, damageDefName );
	}
}

/*
=====================
arcAFEntity_Gibbable::SpawnGibs
=====================
*/
void arcAFEntity_Gibbable::SpawnGibs( const arcVec3 &dir, const char *damageDefName ) {
	int i;
	bool gibNonSolid;
	arcVec3 entityCenter, velocity;
	arcList<arcEntity *> list;

	assert( !gameLocal.isClient );

	const arcDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if ( !damageDef ) {
		gameLocal.Error( "Unknown damageDef '%s'", damageDefName );
	}

	// spawn gib articulated figures
	arcAFEntity_Base::DropAFs( this, "gib", &list );

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
arcAFEntity_Gibbable::Gib
============
*/
void arcAFEntity_Gibbable::Gib( const arcVec3 &dir, const char *damageDefName ) {
	// only gib once
	if ( gibbed ) {
		return;
	}

	const arcDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
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
			StartSound( "snd_gibbed", SND_CHANNEL_ANY, 0, false, NULL );
			gibbed = true;
		}
	} else {
		gibbed = true;
	}


	PostEventSec( &EV_Gibbed, 4.0f );
}

/*
============
arcAFEntity_Gibbable::Event_Gib
============
*/
void arcAFEntity_Gibbable::Event_Gib( const char *damageDefName ) {
	Gib( arcVec3( 0, 0, 1 ), damageDefName );
}

/*
===============================================================================

  arcAFEntity_Generic

===============================================================================
*/

CLASS_DECLARATION( arcAFEntity_Gibbable, arcAFEntity_Generic )
	EVENT( EV_Activate,			arcAFEntity_Generic::Event_Activate )
END_CLASS

/*
================
arcAFEntity_Generic::arcAFEntity_Generic
================
*/
arcAFEntity_Generic::arcAFEntity_Generic( void ) {
	keepRunningPhysics = false;
}

/*
================
arcAFEntity_Generic::~arcAFEntity_Generic
================
*/
arcAFEntity_Generic::~arcAFEntity_Generic( void ) {
}

/*
================
arcAFEntity_Generic::Save
================
*/
void arcAFEntity_Generic::Save( arcSaveGame *savefile ) const {
	savefile->WriteBool( keepRunningPhysics );
}

/*
================
arcAFEntity_Generic::Restore
================
*/
void arcAFEntity_Generic::Restore( arcRestoreGame *savefile ) {
	savefile->ReadBool( keepRunningPhysics );
}

/*
================
arcAFEntity_Generic::Think
================
*/
void arcAFEntity_Generic::Think( void ) {
	arcAFEntity_Base::Think();

	if ( keepRunningPhysics ) {
		BecomeActive( TH_PHYSICS );
	}
}

/*
================
arcAFEntity_Generic::Spawn
================
*/
void arcAFEntity_Generic::Spawn( void ) {
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
arcAFEntity_Generic::Event_Activate
================
*/
void arcAFEntity_Generic::Event_Activate( arcEntity *activator ) {
	float delay;
	arcVec3 init_velocity, init_avelocity;

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

  arcAFEntity_WithAttachedHead

===============================================================================
*/

CLASS_DECLARATION( arcAFEntity_Gibbable, arcAFEntity_WithAttachedHead )
	EVENT( EV_Gib,				arcAFEntity_WithAttachedHead::Event_Gib )
	EVENT( EV_Activate,			arcAFEntity_WithAttachedHead::Event_Activate )
END_CLASS

/*
================
arcAFEntity_WithAttachedHead::arcAFEntity_WithAttachedHead
================
*/
arcAFEntity_WithAttachedHead::arcAFEntity_WithAttachedHead() {
	head = NULL;
}

/*
================
arcAFEntity_WithAttachedHead::~arcAFEntity_WithAttachedHead
================
*/
arcAFEntity_WithAttachedHead::~arcAFEntity_WithAttachedHead() {
	if ( head.GetEntity() ) {
		head.GetEntity()->ClearBody();
		head.GetEntity()->PostEventMS( &EV_Remove, 0 );
	}
}

/*
================
arcAFEntity_WithAttachedHead::Spawn
================
*/
void arcAFEntity_WithAttachedHead::Spawn( void ) {
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
arcAFEntity_WithAttachedHead::Save
================
*/
void arcAFEntity_WithAttachedHead::Save( arcSaveGame *savefile ) const {
	head.Save( savefile );
}

/*
================
arcAFEntity_WithAttachedHead::Restore
================
*/
void arcAFEntity_WithAttachedHead::Restore( arcRestoreGame *savefile ) {
	head.Restore( savefile );
}

/*
================
arcAFEntity_WithAttachedHead::SetupHead
================
*/
void arcAFEntity_WithAttachedHead::SetupHead( void ) {
	arcAFAttachment		*headEnt;
	idStr				jointName;
	const char			*headModel;
	jointHandle_t		joint;
	arcVec3				origin;
	arcMat3				axis;

	headModel = spawnArgs.GetString( "def_head", "" );
	if ( headModel[ 0 ] ) {
		jointName = spawnArgs.GetString( "head_joint" );
		joint = animator.GetJointHandle( jointName );
		if ( joint == INVALID_JOINT ) {
			gameLocal.Error( "Joint '%s' not found for 'head_joint' on '%s'", jointName.c_str(), name.c_str() );
		}

		headEnt = static_cast<arcAFAttachment *>( gameLocal.SpawnEntityType( arcAFAttachment::Type, NULL ) );
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
arcAFEntity_WithAttachedHead::Think
================
*/
void arcAFEntity_WithAttachedHead::Think( void ) {
	arcAFEntity_Base::Think();
}

/*
================
arcAFEntity_WithAttachedHead::LinkCombat
================
*/
void arcAFEntity_WithAttachedHead::LinkCombat( void ) {
	arcAFAttachment *headEnt;

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
arcAFEntity_WithAttachedHead::UnlinkCombat
================
*/
void arcAFEntity_WithAttachedHead::UnlinkCombat( void ) {
	arcAFAttachment *headEnt;

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
arcAFEntity_WithAttachedHead::Hide
================
*/
void arcAFEntity_WithAttachedHead::Hide( void ) {
	arcAFEntity_Base::Hide();
	if ( head.GetEntity() ) {
		head.GetEntity()->Hide();
	}
	UnlinkCombat();
}

/*
================
arcAFEntity_WithAttachedHead::Show
================
*/
void arcAFEntity_WithAttachedHead::Show( void ) {
	arcAFEntity_Base::Show();
	if ( head.GetEntity() ) {
		head.GetEntity()->Show();
	}
	LinkCombat();
}

/*
================
arcAFEntity_WithAttachedHead::ProjectOverlay
================
*/
void arcAFEntity_WithAttachedHead::ProjectOverlay( const arcVec3 &origin, const arcVec3 &dir, float size, const char *material ) {

	arcEntity::ProjectOverlay( origin, dir, size, material );

	if ( head.GetEntity() ) {
		head.GetEntity()->ProjectOverlay( origin, dir, size, material );
	}
}

/*
============
arcAFEntity_WithAttachedHead::Gib
============
*/
void arcAFEntity_WithAttachedHead::Gib( const arcVec3 &dir, const char *damageDefName ) {
	// only gib once
	if ( gibbed ) {
		return;
	}
	arcAFEntity_Gibbable::Gib( dir, damageDefName );
	if ( head.GetEntity() ) {
		head.GetEntity()->Hide();
	}
}

/*
============
arcAFEntity_WithAttachedHead::Event_Gib
============
*/
void arcAFEntity_WithAttachedHead::Event_Gib( const char *damageDefName ) {
	Gib( arcVec3( 0, 0, 1 ), damageDefName );
}

/*
================
arcAFEntity_WithAttachedHead::Event_Activate
================
*/
void arcAFEntity_WithAttachedHead::Event_Activate( arcEntity *activator ) {
	float delay;
	arcVec3 init_velocity, init_avelocity;

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

  arcAFEntity_Vehicle

===============================================================================
*/

CLASS_DECLARATION( arcAFEntity_Base, arcAFEntity_Vehicle )
END_CLASS

/*
================
arcAFEntity_Vehicle::arcAFEntity_Vehicle
================
*/
arcAFEntity_Vehicle::arcAFEntity_Vehicle( void ) {
	player				= NULL;
	eyesJoint			= INVALID_JOINT;
	steeringWheelJoint	= INVALID_JOINT;
	wheelRadius			= 0.0f;
	steerAngle			= 0.0f;
	steerSpeed			= 0.0f;
	dustSmoke			= NULL;
}

/*
================
arcAFEntity_Vehicle::Spawn
================
*/
void arcAFEntity_Vehicle::Spawn( void ) {
	const char *eyesJointName = spawnArgs.GetString( "eyesJoint", "eyes" );
	const char *steeringWheelJointName = spawnArgs.GetString( "steeringWheelJoint", "steeringWheel" );

	LoadAF();

	SetCombatModel();

	SetPhysics( af.GetPhysics() );

	fl.takedamage = true;

	if ( !eyesJointName[0] ) {
		gameLocal.Error( "arcAFEntity_Vehicle '%s' no eyes joint specified", name.c_str() );
	}
	eyesJoint = animator.GetJointHandle( eyesJointName );
	if ( !steeringWheelJointName[0] ) {
		gameLocal.Error( "arcAFEntity_Vehicle '%s' no steering wheel joint specified", name.c_str() );
	}
	steeringWheelJoint = animator.GetJointHandle( steeringWheelJointName );

	spawnArgs.GetFloat( "wheelRadius", "20", wheelRadius );
	spawnArgs.GetFloat( "steerSpeed", "5", steerSpeed );

	player = NULL;
	steerAngle = 0.0f;

	const char *smokeName = spawnArgs.GetString( "smoke_vehicle_dust", "muzzlesmoke" );
	if ( *smokeName != '\0' ) {
		dustSmoke = static_cast<const arcDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
	}
}

/*
================
arcAFEntity_Vehicle::Use
================
*/
void arcAFEntity_Vehicle::Use( arcNetBasePlayer *other ) {
	arcVec3 origin;
	arcMat3 axis;

	if ( player ) {
		if ( player == other ) {
			other->Unbind();
			player = NULL;

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
arcAFEntity_Vehicle::GetSteerAngle
================
*/
float arcAFEntity_Vehicle::GetSteerAngle( void ) {
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

  arcAFEntity_VehicleSimple

===============================================================================
*/

CLASS_DECLARATION( arcAFEntity_Vehicle, arcAFEntity_VehicleSimple )
END_CLASS

/*
================
arcAFEntity_VehicleSimple::arcAFEntity_VehicleSimple
================
*/
arcAFEntity_VehicleSimple::arcAFEntity_VehicleSimple( void ) {
	int i;
	for ( i = 0; i < 4; i++ ) {
		suspension[i] = NULL;
	}
}

/*
================
arcAFEntity_VehicleSimple::~arcAFEntity_VehicleSimple
================
*/
arcAFEntity_VehicleSimple::~arcAFEntity_VehicleSimple( void ) {
	delete wheelModel;
	wheelModel = NULL;
}

/*
================
arcAFEntity_VehicleSimple::Spawn
================
*/
void arcAFEntity_VehicleSimple::Spawn( void ) {
	static const char *wheelJointKeys[] = {
		"wheelJointFrontLeft",
		"wheelJointFrontRight",
		"wheelJointRearLeft",
		"wheelJointRearRight"
	};
	static arcVec3 wheelPoly[4] = { arcVec3( 2, 2, 0 ), arcVec3( 2, -2, 0 ), arcVec3( -2, -2, 0 ), arcVec3( -2, 2, 0 ) };

	int i;
	arcVec3 origin;
	arcMat3 axis;
	arcTraceModel trm;

	trm.SetupPolygon( wheelPoly, 4 );
	trm.Translate( arcVec3( 0, 0, -wheelRadius ) );
	wheelModel = new arcClipModel( trm );

	for ( i = 0; i < 4; i++ ) {
		const char *wheelJointName = spawnArgs.GetString( wheelJointKeys[i], "" );
		if ( !wheelJointName[0] ) {
			gameLocal.Error( "arcAFEntity_VehicleSimple '%s' no '%s' specified", name.c_str(), wheelJointKeys[i] );
		}
		wheelJoints[i] = animator.GetJointHandle( wheelJointName );
		if ( wheelJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "arcAFEntity_VehicleSimple '%s' can't find wheel joint '%s'", name.c_str(), wheelJointName );
		}

		GetAnimator()->GetJointTransform( wheelJoints[i], 0, origin, axis );
		origin = renderEntity.origin + origin * renderEntity.axis;

		suspension[i] = new arcAFConstraint_Suspension();
		suspension[i]->Setup( va( "suspension%d", i ), af.GetPhysics()->GetBody( 0 ), origin, af.GetPhysics()->GetAxis( 0 ), wheelModel );
		suspension[i]->SetSuspension(	g_vehicleSuspensionUp.GetFloat(),
										g_vehicleSuspensionDown.GetFloat(),
										g_vehicleSuspensionKCompress.GetFloat(),
										g_vehicleSuspensionDamping.GetFloat(),
										g_vehicleTireFriction.GetFloat() );

		af.GetPhysics()->AddConstraint( suspension[i] );
	}

	memset( wheelAngles, 0, sizeof( wheelAngles ) );
	BecomeActive( TH_THINK );
}

/*
================
arcAFEntity_VehicleSimple::Think
================
*/
void arcAFEntity_VehicleSimple::Think( void ) {
	int i;
	float force = 0.0f, velocity = 0.0f, steerAngle = 0.0f;
	arcVec3 origin;
	arcMat3 axis;
	idRotation wheelRotation, steerRotation;

	if ( thinkFlags & TH_THINK ) {

		if ( player ) {
			// capture the input from a player
			velocity = g_vehicleVelocity.GetFloat();
			if ( player->usercmd.forwardmove < 0 ) {
				velocity = -velocity;
			}
			force = arcMath::Fabs( player->usercmd.forwardmove * g_vehicleForce.GetFloat() ) * (1.0f / 128.0f);
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
			suspension[i]->SetSuspension(	g_vehicleSuspensionUp.GetFloat(),
											g_vehicleSuspensionDown.GetFloat(),
											g_vehicleSuspensionKCompress.GetFloat(),
											g_vehicleSuspensionDamping.GetFloat(),
											g_vehicleTireFriction.GetFloat() );
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
		if ( force != 0.0f && !( gameLocal.framenum & 7 ) ) {
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

  arcAFEntity_VehicleFourWheels

===============================================================================
*/

CLASS_DECLARATION( arcAFEntity_Vehicle, arcAFEntity_VehicleFourWheels )
END_CLASS


/*
================
arcAFEntity_VehicleFourWheels::arcAFEntity_VehicleFourWheels
================
*/
arcAFEntity_VehicleFourWheels::arcAFEntity_VehicleFourWheels( void ) {
	int i;

	for ( i = 0; i < 4; i++ ) {
		wheels[i]		= NULL;
		wheelJoints[i]	= INVALID_JOINT;
		wheelAngles[i]	= 0.0f;
	}
	steering[0]			= NULL;
	steering[1]			= NULL;
}

/*
================
arcAFEntity_VehicleFourWheels::Spawn
================
*/
void arcAFEntity_VehicleFourWheels::Spawn( void ) {
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
			gameLocal.Error( "arcAFEntity_VehicleFourWheels '%s' no '%s' specified", name.c_str(), wheelBodyKeys[i] );
		}
		wheels[i] = af.GetPhysics()->GetBody( wheelBodyName );
		if ( !wheels[i] ) {
			gameLocal.Error( "arcAFEntity_VehicleFourWheels '%s' can't find wheel body '%s'", name.c_str(), wheelBodyName );
		}
		wheelJointName = spawnArgs.GetString( wheelJointKeys[i], "" );
		if ( !wheelJointName[0] ) {
			gameLocal.Error( "arcAFEntity_VehicleFourWheels '%s' no '%s' specified", name.c_str(), wheelJointKeys[i] );
		}
		wheelJoints[i] = animator.GetJointHandle( wheelJointName );
		if ( wheelJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "arcAFEntity_VehicleFourWheels '%s' can't find wheel joint '%s'", name.c_str(), wheelJointName );
		}
	}

	for ( i = 0; i < 2; i++ ) {
		steeringHingeName = spawnArgs.GetString( steeringHingeKeys[i], "" );
		if ( !steeringHingeName[0] ) {
			gameLocal.Error( "arcAFEntity_VehicleFourWheels '%s' no '%s' specified", name.c_str(), steeringHingeKeys[i] );
		}
		steering[i] = static_cast<arcAFConstraint_Hinge *>(af.GetPhysics()->GetConstraint( steeringHingeName ));
		if ( !steering[i] ) {
			gameLocal.Error( "arcAFEntity_VehicleFourWheels '%s': can't find steering hinge '%s'", name.c_str(), steeringHingeName );
		}
	}

	memset( wheelAngles, 0, sizeof( wheelAngles ) );
	BecomeActive( TH_THINK );
}

/*
================
arcAFEntity_VehicleFourWheels::Think
================
*/
void arcAFEntity_VehicleFourWheels::Think( void ) {
	int i;
	float force = 0.0f, velocity = 0.0f, steerAngle = 0.0f;
	arcVec3 origin;
	arcMat3 axis;
	idRotation rotation;

	if ( thinkFlags & TH_THINK ) {

		if ( player ) {
			// capture the input from a player
			velocity = g_vehicleVelocity.GetFloat();
			if ( player->usercmd.forwardmove < 0 ) {
				velocity = -velocity;
			}
			force = arcMath::Fabs( player->usercmd.forwardmove * g_vehicleForce.GetFloat() ) * (1.0f / 128.0f);
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
		if ( force != 0.0f && !( gameLocal.framenum & 7 ) ) {
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

  arcAFEntity_VehicleSixWheels

===============================================================================
*/

CLASS_DECLARATION( arcAFEntity_Vehicle, arcAFEntity_VehicleSixWheels )
END_CLASS

	/*
================
arcAFEntity_VehicleSixWheels::arcAFEntity_VehicleSixWheels
================
*/
arcAFEntity_VehicleSixWheels::arcAFEntity_VehicleSixWheels( void ) {
	int i;

	for ( i = 0; i < 6; i++ ) {
		wheels[i]		= NULL;
		wheelJoints[i]	= INVALID_JOINT;
		wheelAngles[i]	= 0.0f;
	}
	steering[0]			= NULL;
	steering[1]			= NULL;
	steering[2]			= NULL;
	steering[3]			= NULL;
}

/*
================
arcAFEntity_VehicleSixWheels::Spawn
================
*/
void arcAFEntity_VehicleSixWheels::Spawn( void ) {
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
			gameLocal.Error( "arcAFEntity_VehicleSixWheels '%s' no '%s' specified", name.c_str(), wheelBodyKeys[i] );
		}
		wheels[i] = af.GetPhysics()->GetBody( wheelBodyName );
		if ( !wheels[i] ) {
			gameLocal.Error( "arcAFEntity_VehicleSixWheels '%s' can't find wheel body '%s'", name.c_str(), wheelBodyName );
		}
		wheelJointName = spawnArgs.GetString( wheelJointKeys[i], "" );
		if ( !wheelJointName[0] ) {
			gameLocal.Error( "arcAFEntity_VehicleSixWheels '%s' no '%s' specified", name.c_str(), wheelJointKeys[i] );
		}
		wheelJoints[i] = animator.GetJointHandle( wheelJointName );
		if ( wheelJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "arcAFEntity_VehicleSixWheels '%s' can't find wheel joint '%s'", name.c_str(), wheelJointName );
		}
	}

	for ( i = 0; i < 4; i++ ) {
		steeringHingeName = spawnArgs.GetString( steeringHingeKeys[i], "" );
		if ( !steeringHingeName[0] ) {
			gameLocal.Error( "arcAFEntity_VehicleSixWheels '%s' no '%s' specified", name.c_str(), steeringHingeKeys[i] );
		}
		steering[i] = static_cast<arcAFConstraint_Hinge *>(af.GetPhysics()->GetConstraint( steeringHingeName ));
		if ( !steering[i] ) {
			gameLocal.Error( "arcAFEntity_VehicleSixWheels '%s': can't find steering hinge '%s'", name.c_str(), steeringHingeName );
		}
	}

	memset( wheelAngles, 0, sizeof( wheelAngles ) );
	BecomeActive( TH_THINK );
}

/*
================
arcAFEntity_VehicleSixWheels::Think
================
*/
void arcAFEntity_VehicleSixWheels::Think( void ) {
	int i;
	float force = 0.0f, velocity = 0.0f, steerAngle = 0.0f;
	arcVec3 origin;
	arcMat3 axis;
	idRotation rotation;

	if ( thinkFlags & TH_THINK ) {

		if ( player ) {
			// capture the input from a player
			velocity = g_vehicleVelocity.GetFloat();
			if ( player->usercmd.forwardmove < 0 ) {
				velocity = -velocity;
			}
			force = arcMath::Fabs( player->usercmd.forwardmove * g_vehicleForce.GetFloat() ) * (1.0f / 128.0f);
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
		if ( force != 0.0f && !( gameLocal.framenum & 7 ) ) {
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

  arcAFEntity_SteamPipe

===============================================================================
*/

CLASS_DECLARATION( arcAFEntity_Base, arcAFEntity_SteamPipe )
END_CLASS


/*
================
arcAFEntity_SteamPipe::arcAFEntity_SteamPipe
================
*/
arcAFEntity_SteamPipe::arcAFEntity_SteamPipe( void ) {
	steamBody			= 0;
	steamForce			= 0.0f;
	steamUpForce		= 0.0f;
	steamModelDefHandle	= -1;
	memset( &steamRenderEntity, 0, sizeof( steamRenderEntity ) );
}

/*
================
arcAFEntity_SteamPipe::~arcAFEntity_SteamPipe
================
*/
arcAFEntity_SteamPipe::~arcAFEntity_SteamPipe( void ) {
	if ( steamModelDefHandle >= 0 ){
		gameRenderWorld->FreeEntityDef( steamModelDefHandle );
	}
}

/*
================
arcAFEntity_SteamPipe::Save
================
*/
void arcAFEntity_SteamPipe::Save( arcSaveGame *savefile ) const {
}

/*
================
arcAFEntity_SteamPipe::Restore
================
*/
void arcAFEntity_SteamPipe::Restore( arcRestoreGame *savefile ) {
	Spawn();
}

/*
================
arcAFEntity_SteamPipe::Spawn
================
*/
void arcAFEntity_SteamPipe::Spawn( void ) {
	arcVec3 steamDir;
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
arcAFEntity_SteamPipe::InitSteamRenderEntity
================
*/
void arcAFEntity_SteamPipe::InitSteamRenderEntity( void ) {
	const char	*temp;
	const arcDeclModelDef *modelDef;

	memset( &steamRenderEntity, 0, sizeof( steamRenderEntity ) );
	steamRenderEntity.shaderParms[ SHADERPARM_RED ]		= 1.0f;
	steamRenderEntity.shaderParms[ SHADERPARM_GREEN ]	= 1.0f;
	steamRenderEntity.shaderParms[ SHADERPARM_BLUE ]	= 1.0f;
	modelDef = NULL;
	temp = spawnArgs.GetString ( "model_steam" );
	if ( *temp != '\0' ) {
		if ( !strstr( temp, "." ) ) {
			modelDef = static_cast<const arcDeclModelDef *>( declManager->FindType( DECL_MODELDEF, temp, false ) );
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
arcAFEntity_SteamPipe::Think
================
*/
void arcAFEntity_SteamPipe::Think( void ) {
	arcVec3 steamDir;

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

	arcAFEntity_Base::Think();
}


/*
===============================================================================

  arcAFEntity_ClawFourFingers

===============================================================================
*/

const arcEventDef EV_SetFingerAngle( "setFingerAngle", "f" );
const arcEventDef EV_StopFingers( "stopFingers" );

CLASS_DECLARATION( arcAFEntity_Base, arcAFEntity_ClawFourFingers )
	EVENT( EV_SetFingerAngle,		arcAFEntity_ClawFourFingers::Event_SetFingerAngle )
	EVENT( EV_StopFingers,			arcAFEntity_ClawFourFingers::Event_StopFingers )
END_CLASS

static const char *clawConstraintNames[] = {
	"claw1", "claw2", "claw3", "claw4"
};

/*
================
arcAFEntity_ClawFourFingers::arcAFEntity_ClawFourFingers
================
*/
arcAFEntity_ClawFourFingers::arcAFEntity_ClawFourFingers( void ) {
	fingers[0]	= NULL;
	fingers[1]	= NULL;
	fingers[2]	= NULL;
	fingers[3]	= NULL;
}

/*
================
arcAFEntity_ClawFourFingers::Save
================
*/
void arcAFEntity_ClawFourFingers::Save( arcSaveGame *savefile ) const {
	int i;

	for ( i = 0; i < 4; i++ ) {
		fingers[i]->Save( savefile );
	}
}

/*
================
arcAFEntity_ClawFourFingers::Restore
================
*/
void arcAFEntity_ClawFourFingers::Restore( arcRestoreGame *savefile ) {
	int i;

	for ( i = 0; i < 4; i++ ) {
		fingers[i] = static_cast<arcAFConstraint_Hinge *>(af.GetPhysics()->GetConstraint( clawConstraintNames[i] ));
		fingers[i]->Restore( savefile );
	}

	SetCombatModel();
	LinkCombat();
}

/*
================
arcAFEntity_ClawFourFingers::Spawn
================
*/
void arcAFEntity_ClawFourFingers::Spawn( void ) {
	int i;

	LoadAF();

	SetCombatModel();

	af.GetPhysics()->LockWorldConstraints( true );
	af.GetPhysics()->SetForcePushable( true );
	SetPhysics( af.GetPhysics() );

	fl.takedamage = true;

	for ( i = 0; i < 4; i++ ) {
		fingers[i] = static_cast<arcAFConstraint_Hinge *>(af.GetPhysics()->GetConstraint( clawConstraintNames[i] ));
		if ( !fingers[i] ) {
			gameLocal.Error( "idClaw_FourFingers '%s': can't find claw constraint '%s'", name.c_str(), clawConstraintNames[i] );
		}
	}
}

/*
================
arcAFEntity_ClawFourFingers::Event_SetFingerAngle
================
*/
void arcAFEntity_ClawFourFingers::Event_SetFingerAngle( float angle ) {
	int i;

	for ( i = 0; i < 4; i++ ) {
		fingers[i]->SetSteerAngle( angle );
		fingers[i]->SetSteerSpeed( 0.5f );
	}
	af.GetPhysics()->Activate();
}

/*
================
arcAFEntity_ClawFourFingers::Event_StopFingers
================
*/
void arcAFEntity_ClawFourFingers::Event_StopFingers( void ) {
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
idGameEdit::AF_SpawnEntity
================
*/
bool idGameEdit::AF_SpawnEntity( const char *fileName ) {
	arcDict args;
	arcNetBasePlayer *player;
	arcAFEntity_Generic *ent;
	const arcDeclAF *af;
	arcVec3 org;
	float yaw;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk( false ) ) {
		return false;
	}

	af = static_cast<const arcDeclAF *>( declManager->FindType( DECL_AF, fileName ) );
	if ( !af ) {
		return false;
	}

	yaw = player->viewAngles.yaw;
	args.Set( "angle", va( "%f", yaw + 180 ) );
	org = player->GetPhysics()->GetOrigin() + arcAngles( 0, yaw, 0 ).ToForward() * 80 + arcVec3( 0, 0, 1 );
	args.Set( "origin", org.ToString() );
	args.Set( "spawnclass", "arcAFEntity_Generic" );
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
	ent = static_cast<arcAFEntity_Generic *>(gameLocal.SpawnEntityType( arcAFEntity_Generic::Type, &args));

	// always update this entity
	ent->BecomeActive( TH_THINK );
	ent->KeepRunningPhysics();
	ent->fl.forcePhysicsUpdate = true;

	player->dragEntity.SetSelected( ent );

	return true;
}

/*
================
idGameEdit::AF_UpdateEntities
================
*/
void idGameEdit::AF_UpdateEntities( const char *fileName ) {
	arcEntity *ent;
	arcAFEntity_Base *af;
	idStr name;

	name = fileName;
	name.StripFileExtension();

	// reload any arcAFEntity_Generic which uses the given articulated figure file
	for ( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->IsType( arcAFEntity_Base::Type ) ) {
			af = static_cast<arcAFEntity_Base *>(ent);
			if ( name.Icmp( af->GetAFName() ) == 0 ) {
				af->LoadAF();
				af->GetAFPhysics()->PutToRest();
			}
		}
	}
}

/*
================
idGameEdit::AF_UndoChanges
================
*/
void idGameEdit::AF_UndoChanges( void ) {
	int i, c;
	arcEntity *ent;
	arcAFEntity_Base *af;
	arcDeclAF *decl;

	c = declManager->GetNumDecls( DECL_AF );
	for ( i = 0; i < c; i++ ) {
		decl = static_cast<arcDeclAF *>( const_cast<arcDecl *>( declManager->DeclByIndex( DECL_AF, i, false ) ) );
		if ( !decl->modified ) {
			continue;
		}

		decl->Invalidate();
		declManager->FindType( DECL_AF, decl->GetName() );

		// reload all AF entities using the file
		for ( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
			if ( ent->IsType( arcAFEntity_Base::Type ) ) {
				af = static_cast<arcAFEntity_Base *>(ent);
				if ( idStr::Icmp( decl->GetName(), af->GetAFName() ) == 0 ) {
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
	const idMD5Joint *joints;
} jointTransformData_t;

static bool GetJointTransform( void *model, const idJointMat *frame, const char *jointName, arcVec3 &origin, arcMat3 &axis ) {
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
static const char *GetArgString( const arcDict &args, const arcDict *defArgs, const char *key ) {
	const char *s;

	s = args.GetString( key );
	if ( !s[0] && defArgs ) {
		s = defArgs->GetString( key );
	}
	return s;
}

/*
================
idGameEdit::AF_CreateMesh
================
*/
idRenderModel *idGameEdit::AF_CreateMesh( const arcDict &args, arcVec3 &meshOrigin, arcMat3 &meshAxis, bool &poseIsSet ) {
	int i, jointNum;
	const arcDeclAF *af;
	const arcDeclAF_Body *fb;
	renderEntity_t ent;
	arcVec3 origin, *bodyOrigin, *newBodyOrigin, *modifiedOrigin;
	arcMat3 axis, *bodyAxis, *newBodyAxis, *modifiedAxis;
	declAFJointMod_t *jointMod;
	arcAngles angles;
	const arcDict *defArgs;
	const idKeyValue *arg;
	idStr name;
	jointTransformData_t data;
	const char *classname, *afName, *modelName;
	idRenderModel *md5;
	const arcDeclModelDef *modelDef;
	const idMD5Anim *MD5anim;
	const idMD5Joint *MD5joint;
	const idMD5Joint *MD5joints;
	int numMD5joints;
	idJointMat *originalJoints;
	int parentNum;

	poseIsSet = false;
	meshOrigin.Zero();
	meshAxis.Identity();

	classname = args.GetString( "classname" );
	defArgs = gameLocal.FindEntityDefDict( classname );

	// get the articulated figure
	afName = GetArgString( args, defArgs, "articulatedFigure" );
	af = static_cast<const arcDeclAF *>( declManager->FindType( DECL_AF, afName ) );
	if ( !af ) {
		return NULL;
	}

	// get the md5 model
	modelName = GetArgString( args, defArgs, "model" );
	modelDef = static_cast< const arcDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelName, false ) );
	if ( !modelDef ) {
		return NULL;
	}

	// make sure model hasn't been purged
	if ( modelDef->ModelHandle() && !modelDef->ModelHandle()->IsLoaded() ) {
		modelDef->ModelHandle()->LoadModel();
	}

	// get the md5
	md5 = modelDef->ModelHandle();
	if ( !md5 || md5->IsDefaultModel() ) {
		return NULL;
	}

	// get the articulated figure pose anim
	int animNum = modelDef->GetAnim( "af_pose" );
	if ( !animNum ) {
		return NULL;
	}
	const arcAnim *anim = modelDef->GetAnim( animNum );
	if ( !anim ) {
		return NULL;
	}
	MD5anim = anim->MD5Anim( 0 );
	MD5joints = md5->GetJoints();
	numMD5joints = md5->NumJoints();

	// setup a render entity
	memset( &ent, 0, sizeof( ent ) );
	ent.customSkin = modelDef->GetSkin();
	ent.bounds.Clear();
	ent.numJoints = numMD5joints;
	ent.joints = ( idJointMat * )_alloca16( ent.numJoints * sizeof( *ent.joints ) );

	// create animation from of the af_pose
	ANIM_CreateAnimFrame( md5, MD5anim, ent.numJoints, ent.joints, 1, modelDef->GetVisualOffset(), false );

	// buffers to store the initial origin and axis for each body
	bodyOrigin = (arcVec3 *) _alloca16( af->bodies.Num() * sizeof( arcVec3 ) );
	bodyAxis = (arcMat3 *) _alloca16( af->bodies.Num() * sizeof( arcMat3 ) );
	newBodyOrigin = (arcVec3 *) _alloca16( af->bodies.Num() * sizeof( arcVec3 ) );
	newBodyAxis = (arcMat3 *) _alloca16( af->bodies.Num() * sizeof( arcMat3 ) );

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
	for ( arg = args.MatchPrefix( "body ", NULL ); arg; arg = args.MatchPrefix( "body ", arg ) ) {
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
	originalJoints = ( idJointMat * )_alloca16( numMD5joints * sizeof( originalJoints[0] ) );
	memcpy( originalJoints, ent.joints, numMD5joints * sizeof( originalJoints[0] ) );

	// buffer to store the joint mods
	jointMod = (declAFJointMod_t *) _alloca16( numMD5joints * sizeof( declAFJointMod_t ) );
	memset( jointMod, -1, numMD5joints * sizeof( declAFJointMod_t ) );
	modifiedOrigin = (arcVec3 *) _alloca16( numMD5joints * sizeof( arcVec3 ) );
	memset( modifiedOrigin, 0, numMD5joints * sizeof( arcVec3 ) );
	modifiedAxis = (arcMat3 *) _alloca16( numMD5joints * sizeof( arcMat3 ) );
	memset( modifiedAxis, 0, numMD5joints * sizeof( arcMat3 ) );

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
		arcMat3 parentAxis = originalJoints[ parentNum ].ToMat3();
		arcMat3 localm = originalJoints[i].ToMat3() * parentAxis.Transpose();
		arcVec3 localt = ( originalJoints[i].ToVec3() - originalJoints[ parentNum ].ToVec3() ) * parentAxis.Transpose();

		switch( jointMod[i] ) {
			case DECLAF_JOINTMOD_ORIGIN: {
				ent.joints[ i ].SetRotation( localm * ent.joints[ parentNum ].ToMat3() );
				ent.joints[ i ].SetTranslation( modifiedOrigin[ i ] );
				break;
			}
			case DECLAF_JOINTMOD_AXIS: {
				ent.joints[ i ].SetRotation( modifiedAxis[ i ] );
				ent.joints[ i ].SetTranslation( ent.joints[ parentNum ].ToVec3() + localt * ent.joints[ parentNum ].ToMat3() );
				break;
			}
			case DECLAF_JOINTMOD_BOTH: {
				ent.joints[ i ].SetRotation( modifiedAxis[ i ] );
				ent.joints[ i ].SetTranslation( modifiedOrigin[ i ] );
				break;
			}
			default: {
				ent.joints[ i ].SetRotation( localm * ent.joints[ parentNum ].ToMat3() );
				ent.joints[ i ].SetTranslation( ent.joints[ parentNum ].ToVec3() + localt * ent.joints[ parentNum ].ToMat3() );
				break;
			}
		}
	}

	// instantiate a mesh using the joint information from the render entity
	return md5->InstantiateDynamicModel( &ent, NULL, NULL );
}
