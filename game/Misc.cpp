#include "/idlib/Lib.h"
#include "game_local.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ARC_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Misc.h"
#include "Player.h"
#include "Camera.h"
#include "ai/AI.h"
#include "Projectile.h"
#include "WorldSpawn.h"
#include "ContentMask.h"
#include "Moveable.h"
/*

Various utility objects and functions.

*/

/*
===============================================================================

idSpawnableEntity

A simple, spawnable entity with a model and no functionable ability of it's own.
For example, it can be used as a placeholder during development, for marking
locations on maps for script, or for simple placed models without any behavior
that can be bound to other entities.  Should not be subclassed.
===============================================================================
*/

CLASS_DECLARATION( anEntity, idSpawnableEntity )
END_CLASS

/*
======================
idSpawnableEntity::Spawn
======================
*/
void idSpawnableEntity::Spawn() {
	// this just holds dict information
}

/*
===============================================================================

sdModelStatic

A simple, spawnable entity with a collision model and no functionable ability of its own.

	NOTE: this entity is really a hack, and can go as soon as anEntity is cleaned up
	( that is, when it has no renderEntity anymore )

===============================================================================
*/

CLASS_DECLARATION( anEntity, sdModelStatic )
END_CLASS

/*
======================
sdModelStatic::Spawn
======================
*/
void sdModelStatic::Spawn( void ) {
	Hide();
	memset( &renderEntity, 0, sizeof( renderEntity ) );
}

/*
===============
sdModelStatic::PostMapSpawn
===============
*/
void sdModelStatic::PostMapSpawn( void ) {
	anEntity::PostMapSpawn();

	sdInstanceCollector< sdLODEntity > lodEntity( false );
	if ( lodEntity.Num() < 1 ) {
		return;
	}
	sdLODEntity* lodEnt = lodEntity[0];

	lodEnt->AddClipModel( new anClipModel( GetPhysics()->GetClipModel() ), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );

	PostEventMS( &EV_Remove, 0 );
}

/*
===============
sdModelStatic::InhibitSpawn
================
*/
bool sdModelStatic::InhibitSpawn( const anDict &args ) {
	if ( args.GetBool( "noClipModel" ) ) {
		return true;
	}

	if ( args.GetBool( "mergecm" ) ) {
		assert( 0 );	// the map compiler isn't doing its job if we ever get here
		return true;
	}

	if ( args.GetBool( "inlineCollisionModel" ) ) {
		assert( 0 );	// the map compiler isn't doing its job if we ever get here
		return true;
	}

	return false;
}

/*
===============================================================================

	sdDynamicSpawnPoint

===============================================================================
*/

CLASS_DECLARATION( sdScriptEntity, sdDynamicSpawnPoint )
END_CLASS

/*
===============
sdDynamicSpawnPoint::sdDynamicSpawnPoint
================
*/
sdDynamicSpawnPoint::sdDynamicSpawnPoint( void ) : spawnPoint( nullptr ) {
}

/*
===============
sdDynamicSpawnPoint::~sdDynamicSpawnPoint
================
*/
sdDynamicSpawnPoint::~sdDynamicSpawnPoint( void ) {
	gameLocal.UnRegisterSpawnPoint( spawnPoint );
}

/*
===============
sdDynamicSpawnPoint::Spawn
================
*/
void sdDynamicSpawnPoint::Spawn( void ) {
	spawnPoint = &gameLocal.RegisterSpawnPoint( this, vec3_origin, ang_zero );
	spawnPoint->GetRequirements().Load( spawnArgs, "require" );
}

/*
==============
sdDynamicSpawnPoint::CanCollide
==============
*/
bool sdDynamicSpawnPoint::CanCollide( const anEntity *other, int traceId ) const {
	if ( traceId == TM_THIRDPERSON_OFFSET ) {
		return false;
	}
	return anEntity::CanCollide( other, traceId );
}

/*
===============================================================================

	anBasePlayerStart

===============================================================================
*/

CLASS_DECLARATION( anEntity, anBasePlayerStart )
END_CLASS

/*
===============
anBasePlayerStart::anBasePlayerStart
================
*/
anBasePlayerStart::anBasePlayerStart( void ) {
}

/*
===============
anBasePlayerStart::InhibitSpawn
================
*/
bool anBasePlayerStart::InhibitSpawn( const anDict &args ) {
	return gameLocal.isClient && !gameLocal.serverIsRepeater;
}

/*
===============
anBasePlayerStart::PostMapSpawn
================
*/
void anBasePlayerStart::PostMapSpawn( void ) {
	idSpawnPoint *spot;

	anAngles angles;

	const char *targetName = spawnArgs.GetString( "target" );
	anEntity *target = gameLocal.FindEntity( targetName );

	const char *ownerName = spawnArgs.GetString( "owner" );

	bool parachute = spawnArgs.GetBool( "parachute" );
	anVec3 origin = GetPhysics()->GetOrigin();
	if ( parachute ) {
		origin.z += spawnArgs.GetFloat( "parachute_height", "2048" );
	}

	if ( *ownerName ) {
		anEntity *owner = gameLocal.FindEntity( ownerName );
		if ( !owner ) {
			gameLocal.Error( "anBasePlayerStart::PostMapSpawn Could not find owner '%s'", ownerName );
		}

		anVec3 org = ( origin - owner->GetPhysics()->GetOrigin() ) * owner->GetPhysics()->GetAxis().Transpose();
		if ( target ) {
			anVec3 vec = target->GetPhysics()->GetOrigin() - org;
			vec.Normalize();
			angles = vec.ToMat3().ToAngles();
		} else {
			angles = GetPhysics()->GetAxis().ToAngles();
		}
		spot = &gameLocal.RegisterSpawnPoint( owner, org, angles );

	} else {
		if ( target ) {
			anVec3 vec = target->GetPhysics()->GetOrigin() - origin;
			vec.Normalize();
			angles = vec.ToMat3().ToAngles();
		} else {
			angles = GetPhysics()->GetAxis().ToAngles();
		}
		spot = &gameLocal.RegisterSpawnPoint( nullptr, origin, angles );
	}

	spot->GetRequirements().Load( spawnArgs, "require" );
	spot->SetParachute( parachute );

	PostEventMS( &EV_Remove, 0 );
}

/*
===============================================================================

  anForceField

===============================================================================
*/

const anEventDef EV_Toggle( "toggle", '\0', DOC_TEXT( "Toggles the state of the force field." ), 0, "Calling $event:activate$ on a force field will toggle the state, then reset it after a set wait period, using $event:toggle$ will not reset." );

CLASS_DECLARATION( anEntity, anForceField )
	EVENT( EV_Activate,		anForceField::Event_Activate )
	EVENT( EV_Toggle,		anForceField::Event_Toggle )
	EVENT( EV_FindTargets,	anForceField::Event_FindTargets )
	EVENT( EV_GetMins,		anForceField::Event_GetMins )
	EVENT( EV_GetMaxs,		anForceField::Event_GetMaxs )
END_CLASS

anCVar g_debugForceFields( "g_debugForceFields", "0", CVAR_GAME | CVAR_BOOL, "" );

/*
===============
anForceField::Toggle
================
*/
void anForceField::Toggle( void ) {
	active = !active;
}

/*
================
anForceField::Think
================
*/
void anForceField::Think( void ) {
	if ( active ) {
		// evaluate force
		forceField.Evaluate( gameLocal.time );
	}
	Present();

	if ( g_debugForceFields.GetBool() ) {
		gameRenderWorld->DebugBounds( colorBlue, forceField.GetClipModel()->GetBounds(), forceField.GetClipModel()->GetOrigin() );
	}
}

/*
================
anForceField::Spawn
================
*/
void anForceField::Spawn( void ) {
	anVec3 uniform;
	float explosion, implosion, randomTorque;

	if ( spawnArgs.GetVector( "uniform", "0 0 0", uniform ) ) {
		forceField.Uniform( uniform );
	} else if ( spawnArgs.GetFloat( "explosion", "0", explosion ) ) {
		forceField.Explosion( explosion );
	} else if ( spawnArgs.GetFloat( "implosion", "0", implosion ) ) {
		forceField.Implosion( implosion );
	}

	if ( spawnArgs.GetFloat( "randomTorque", "0", randomTorque ) ) {
		forceField.RandomTorque( randomTorque );
	}

	if ( spawnArgs.GetBool( "applyForce", "0" ) ) {
		forceField.SetApplyType( FORCEFIELD_APPLY_FORCE );
	} else if ( spawnArgs.GetBool( "applyImpulse", "0" ) ) {
		forceField.SetApplyType( FORCEFIELD_APPLY_IMPULSE );
	} else {
		forceField.SetApplyType( FORCEFIELD_APPLY_VELOCITY );
	}

	forceField.SetPlayerOnly( spawnArgs.GetBool( "playerOnly", "0" ) );
	forceField.SetMonsterOnly( spawnArgs.GetBool( "monsterOnly", "0" ) );

	// set the collision model on the force field
	forceField.SetClipModel( new anClipModel( GetPhysics()->GetClipModel() ) );

	// remove the collision model from the physics object
	GetPhysics()->SetClipModel( nullptr, 1.0f );

	active = spawnArgs.GetBool( "start_on" );

	PostEventMS( &EV_FindTargets, 0 );

	BecomeActive( TH_THINK );
}

/*
===============
anForceField::Event_Toggle
================
*/
void anForceField::Event_Toggle( void ) {
	Toggle();
}

/*
================
anForceField::Event_Activate
================
*/
void anForceField::Event_Activate( anEntity *activator ) {
	float wait;

	Toggle();
	if ( spawnArgs.GetFloat( "wait", "0.01", wait ) ) {
		PostEventSec( &EV_Toggle, wait );
	}
}

/*
================
anForceField::Event_FindTargets
================
*/
void anForceField::Event_FindTargets( void ) {
	FindTargets();
	RemoveNullTargets();
	if ( targets.Num() ) {
		forceField.Uniform( targets[0].GetEntity()->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin() );
	}
}

/*
================
anForceField::Event_GetMins
================
*/
void anForceField::Event_GetMins( void ) {
	idProgram::ReturnVector( forceField.GetClipModel()->GetBounds()[0] );
}

/*
================
anForceField::Event_GetMaxs
================
*/
void anForceField::Event_GetMaxs( void ) {
	idProgram::ReturnVector( forceField.GetClipModel()->GetBounds()[1] );
}

/*
===============
sdStaticEntityNetworkData::~sdStaticEntityNetworkData
===============
*/
sdStaticEntityNetworkData::~sdStaticEntityNetworkData( void ) {
	delete physicsData;
}

/*
===============
sdStaticEntityNetworkData::MakeDefault
===============
*/
void sdStaticEntityNetworkData::MakeDefault( void ) {
	if ( physicsData != nullptr ) {
		physicsData->MakeDefault();
	}
}

/*
===============
sdStaticEntityNetworkData::Write
===============
*/
void sdStaticEntityNetworkData::Write( anFile *file ) const {
	if ( physicsData != nullptr ) {
		physicsData->Write( file );
	}
}

/*
===============
sdStaticEntityNetworkData::Read
===============
*/
void sdStaticEntityNetworkData::Read( anFile *file ) {
	if ( physicsData != nullptr ) {
		physicsData->Read( file );
	}
}

/*
===============
sdStaticEntityNetworkData::MakeDefault
===============
*/
void sdStaticEntityBroadcastData::MakeDefault( void ) {
	if ( physicsData != nullptr ) {
		physicsData->MakeDefault();
	}
	hidden = -1;
	forceDisableClip = -1;
}

/*
===============
sdStaticEntityBroadcastData::~sdStaticEntityBroadcastData
===============
*/
sdStaticEntityBroadcastData::~sdStaticEntityBroadcastData( void ) {
	delete physicsData;
}

/*
===============
sdStaticEntityBroadcastData::Write
===============
*/
void sdStaticEntityBroadcastData::Write( anFile *file ) const {
	if ( physicsData != nullptr ) {
		physicsData->Write( file );
	}

	file->WriteBool( hidden > 0 );
	file->WriteBool( forceDisableClip > 0 );
}

/*
===============
sdStaticEntityBroadcastData::Read
===============
*/
void sdStaticEntityBroadcastData::Read( anFile *file ) {
	if ( physicsData != nullptr ) {
		physicsData->Read( file );
	}

	bool temp;
	file->ReadBool( temp );
	hidden = temp ? 1 : 0;
	file->ReadBool( temp );
	forceDisableClip = temp ? 1 : 0;
}

/*
===============================================================================

	idStaticEntity

	Some static entities may be optimized into inline geometry by dmap

===============================================================================
*/

CLASS_DECLARATION( anEntity, idStaticEntity )
END_CLASS

/*
===============
idStaticEntity::idStaticEntity
===============
*/
idStaticEntity::idStaticEntity( void ) {
}

/*
================
idStaticEntity::~idStaticEntity
================
*/
idStaticEntity::~idStaticEntity( void ) {
	delete []renderEntity.areas;
}

/*
===============
idStaticEntity::Spawn
===============
*/
void idStaticEntity::Spawn( void ) {
	bool solid	= spawnArgs.GetBool( "solid" );
	bool hidden = spawnArgs.GetBool( "hide" );
	bool disableClip = spawnArgs.GetBool( "disableClip" );

	const char *areas;
	if ( spawnArgs.GetString( "areas", "", &areas ) ) {
		anStringList areaList;
		idSplitStringIntoList( areaList, areas, " " );
		if ( areaList.Num() ) {
			renderEntity.numAreas = areaList.Num();
			renderEntity.areas = new int[ areaList.Num() ];
			for ( int i=0; i<areaList.Num(); i++ ) {
				renderEntity.areas[i] = atoi( areaList[i].c_str() );
			}
		}
	}

	if ( solid ) {
		GetPhysics()->SetContents( CONTENTS_SOLID );
	} else {
		GetPhysics()->SetContents( 0 );
	}

	if ( hidden ) {
		Hide();
	}
	if ( disableClip ) {
		ForceDisableClip();
	}
}

/*
===============
idStaticEntity::PostMapSpawn
===============
*/
void idStaticEntity::PostMapSpawn( void ) {
	anEntity::PostMapSpawn();

	if ( !IsNetSynced() && !spawnArgs.GetBool( "dynamic" ) ) {
		sdInstanceCollector< sdLODEntity > lodEntity( false );
		if ( lodEntity.Num() < 1 ) {
			return;
		}
		sdLODEntity* lodEnt = lodEntity[0];

		if ( GetPhysics()->GetNumClipModels() > 0 ) {
			lodEnt->AddClipModel( new anClipModel( GetPhysics()->GetClipModel() ), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
		}

		if ( !IsHidden() && renderEntity.hModel != nullptr ) {
			lodEnt->AddRenderEntity( renderEntity, -1 );
		}

		PostEventMS( &EV_Remove, 0 );
	}
}

/*
================
idStaticEntity::InhibitSpawn
================
*/
bool idStaticEntity::InhibitSpawn( const anDict &args ) {
	assert( gameLocal.world != nullptr );

	// an inline static model will not do anything at all
	if ( args.GetBool( "inline" ) || gameLocal.world->spawnArgs.GetBool( "inlineAllStatics" ) ) {
		return true;
	}

	return false;
}

/*
================
idStaticEntity::Think
================
*/
void idStaticEntity::Think( void ) {
	anEntity::Think();
}

/*
================
idStaticEntity::Hide
================
*/
void idStaticEntity::Hide( void ) {
	anEntity::Hide();
	fl.forceDisableClip = true;
	DisableClip();
}

/*
================
idStaticEntity::Show
================
*/
void idStaticEntity::Show( void ) {
	anEntity::Show();
	fl.forceDisableClip = false;
	EnableClip();
}

/*
===============================================================================

	sdEnvDefinition

===============================================================================
*/

CLASS_DECLARATION( anEntity, sdEnvDefinitionEntity )
END_CLASS

/*
================
sdEnvDefinition::Spawn
================
*/
void sdEnvDefinitionEntity::Spawn( void ) {
	sdEnvDefinition env;
	spawnArgs.GetVector( "origin", "0 0 0", env.origin );
	spawnArgs.GetString( "env_name", "", env.name );
	spawnArgs.GetInt( "env_size", "128", env.size );

	if ( env.name.Length() ) {
		gameLocal.AddEnvDefinition( env );
	} else {
		gameLocal.Warning( "No env_name field specified on environment definition, skipped" );
	}
	PostEventMS( &EV_Remove, 0 );

}

/*
===============================================================================

	idSpawnController

===============================================================================
*/

CLASS_DECLARATION( sdScriptEntity, idSpawnController )
END_CLASS

/*
================
idSpawnController::Spawn
================
*/
void idSpawnController::Spawn( void ) {
	spawnRequirements.Load( spawnArgs, "require_spawn" );
}

/*
===============================================================================

	sdLODEntity

===============================================================================
*/

CLASS_DECLARATION( anEntity, sdLODEntity )
END_CLASS

/*
================
sdLODEntity::sdLODEntity
================
*/
sdLODEntity::sdLODEntity() {
}

/*
================
sdLODEntity::~sdLODEntity
================
*/
sdLODEntity::~sdLODEntity() {
	FreeModelDefs();
}

/*
================
sdLODEntity::FreeModelDefs
================
*/
void sdLODEntity::FreeModelDefs() {
	for ( int i = 0; i < modelDefHandles.Num(); i++ ) {
		int &modelDefHandle = modelDefHandles[i];
		if ( modelDefHandle != -1 ) {
			renderEntity_t *re = gameRenderWorld->GetRenderEntity( modelDefHandle );
			delete []re->dummies;
			gameRenderWorld->FreeEntityDef( modelDefHandle );
			modelDefHandle = -1;
		}
	}
}

/*
================
sdLODEntity::AddRenderEntity
================
*/
void sdLODEntity::AddRenderEntity( const renderEntity_t& entity, int ID ) {
	modelDefHandles.Alloc() = gameRenderWorld->AddEntityDef( &entity );
	modelID.Alloc() = ID;
}

/*
================
sdLODEntity::AddClipModel
================
*/
void sdLODEntity::AddClipModel( anClipModel* clipModel, const anVec3 &origin, const anMat3 &axes ) {
	int numClipModels = physicsObj.GetNumClipModels();
	physicsObj.SetClipModel( clipModel, 1.0f, numClipModels );
	physicsObj.SetOrigin( origin, numClipModels );
	physicsObj.SetAxis( axes, numClipModels );
}

/*
================
sdLODEntity::Spawn
================
*/
void sdLODEntity::Spawn() {
	physicsObj.SetSelf( this );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin(), 0 );
	physicsObj.SetAxis( GetPhysics()->GetAxis(), 0 );

	// add models
	const anKeyValue*	arg;
	renderEntity_t		renderEntity;

	memset( &renderEntity, 0, sizeof( renderEntity ) );
	renderEntity.spawnID = gameLocal.GetSpawnId( this );//renderEntity.entityNum = entityNumber;
	renderEntity.axis.Identity();
	renderEntity.shaderParms[ SHADERPARM_RED ] = 1.0f;
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = 1.0f;
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = 1.0f;
	renderEntity.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;

	const char *temp = nullptr;

	for ( int i = 0; i < spawnArgs.GetNumKeyVals(); i++ ) {
		arg = spawnArgs.GetKeyVal( i );
		if ( !arg->GetKey().Icmpn( "model", anStr::Length( "model" ) ) ) {
			const char *model = arg->GetValue();
			if ( *model != '\0' ) {
				renderEntity.hModel = renderModelManager->FindModel( model );
			}

			anStr modelID = ( arg->GetKey().c_str() + anStr::Length( "model" ) );

			if ( renderEntity.hModel != nullptr && !renderEntity.hModel->IsDefaultModel() ) {
				renderEntity.bounds		= renderEntity.hModel->Bounds();
				renderEntity.origin		= spawnArgs.GetVector( "origin" + modelID );
				renderEntity.shadowVisDistMult = spawnArgs.GetFloat( "shadowVisDistMult" + modelID, "0" );
				renderEntity.maxVisDist = spawnArgs.GetInt( "maxvisdist" + modelID );
				renderEntity.minVisDist = spawnArgs.GetInt( "minvisdist" + modelID );
				renderEntity.visDistFalloff = spawnArgs.GetFloat( "visDistFalloff" + modelID, "0.25" );
				renderEntity.mapId		= spawnArgs.GetInt( "mapid" + modelID );
				renderEntity.flags.pushByCenter = spawnArgs.GetBool( "pushByOrigin" + modelID );
				renderEntity.flags.occlusionTest = spawnArgs.GetBool( "occlusionTest" + modelID );
				renderEntity.flags.noShadow = spawnArgs.GetBool( "noShadows" + modelID );
				renderEntity.flags.noSelfShadow = spawnArgs.GetBool( "noSelfShadows" + modelID );
				renderEntity.flags.dontCastFromAtmosLight = spawnArgs.GetBool( "dontCastFromAtmosLight" + modelID );
				renderEntity.dummies = nullptr;
				renderEntity.numVisDummies = 0;

				anStr gpuSpecParam = spawnArgs.GetString( "drawSpec" + modelID, "low" );
				if ( gpuSpecParam.Icmp( "high" ) == 0 ) {
					renderEntity.drawSpec = 2;
				} else if ( gpuSpecParam.Icmp( "med" ) == 0 || gpuSpecParam.Icmp( "medium" ) == 0 ) {
					renderEntity.drawSpec = 1;
				} else if ( gpuSpecParam.Icmp( "low" ) == 0 ) {
					renderEntity.drawSpec = 0;
				} else {
					renderEntity.drawSpec = 0;
				}

				anStr shadowSpec = spawnArgs.GetString( "shadowSpec" + modelID , "low" );
				if ( shadowSpec.Icmp( "high" ) == 0 ) {
					renderEntity.shadowSpec = 2;
				} else if ( shadowSpec.Icmp( "med" ) == 0 || shadowSpec.Icmp( "medium" ) == 0 ) {
					renderEntity.shadowSpec = 1;
				} else if ( shadowSpec.Icmp( "low" ) == 0 ) {
					renderEntity.shadowSpec = 0;
				} else {
					renderEntity.shadowSpec = 0;
				}

				temp = spawnArgs.GetString( "ambientCubeMap" + modelID );
				if ( *temp ) {
					renderEntity.ambientCubeMap = declHolder.FindAmbientCubeMap( temp );
				} else {
					renderEntity.ambientCubeMap = nullptr;
				}

				// add to renderer
				AddRenderEntity( renderEntity, atoi( modelID ) );

				// find inlineCollisionModel value
				if ( !spawnArgs.GetBool( "inlineCollisionModel" + modelID, "1" ) ) {
					// hook up collision model
					AddClipModel( new anClipModel( model ), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
				}

				// find instanced collision models
				int id = 0;
				const anKeyValue* kv = spawnArgs.FindKey( "cm_model" + modelID + "_0" );

				while( kv != nullptr ) {
					anVec3 origin = spawnArgs.GetVector( va( "cmodel%s_%d_origin", modelID.c_str(), id ), "0 0 0" );
					anMat3 axis = spawnArgs.GetMatrix( va( "cmodel%s_%d_axis", modelID.c_str(), id ), "1 0 0 0 1 0 0 0 1" );
					AddClipModel( new anClipModel( kv->GetValue().c_str() ), origin, axis );
					id++;
					kv = spawnArgs.FindKey( va( "cm_model%s_%d", modelID.c_str(), id ) );
				}
			}
		}
	}

	physicsObj.SetContents( CONTENTS_SOLID );
	SetPhysics( &physicsObj );
}

void sdLODEntity::PostMapSpawn() {
	anEntity::PostMapSpawn();
	for ( int i = 0; i < modelID.Num(); i++ ) {
		int ID = modelID[i];
		if ( ID != -1 ) {
			renderEntity_t *re = gameRenderWorld->GetRenderEntity( modelDefHandles[i] );
			anStr value = spawnArgs.GetString( va( "visDummies%d", ID ) );
			if ( !value.IsEmpty() ) {
				anStringList strlist;
				idSplitStringIntoList( strlist, value, "," );
				anList< anEntityPtr<anEntity> > dummies;
				anList< int > areas;
				for ( intj=0; j<strlist.Num(); j++ ) {
					anEntity *ent = gameLocal.FindEntity( strlist[j] );
					if ( ent ) {
						anVec3 org = ent->GetPhysics()->GetOrigin();
						int areaNum = gameRenderWorld->PointInArea( org );
						if ( areaNum >= 0 ) {
							bool found = areas.FindIndex( areaNum ) != -1;

							if ( !found ) {
								anEntityPtr<anEntity> &entityPtr = dummies.Alloc();
								entityPtr = ent;
								areas.Alloc() = areaNum;
							}
						}
					}
				}

				re->numVisDummies = dummies.Num();

				if ( re->numVisDummies ) {
					int validcount = 0;
					int c = 0;
					re->dummies = new anVec3[ re->numVisDummies ];
					for ( intj=0; j<re->numVisDummies; j++ ) {
						if ( dummies[j].IsValid() ) {
							re->dummies[c++] = dummies[j].GetEntity()->GetPhysics()->GetOrigin();
						}
					}
					gameRenderWorld->UpdateEntityDef( modelDefHandles[i], re );
				}
			}
		}
	}
}

/*
===============================================================================

	sdEnvBounds

===============================================================================
*/

CLASS_DECLARATION( anEntity, sdEnvBoundsEntity )
END_CLASS

/*
================
sdEnvDefinition::Spawn
================
*/
void sdEnvBoundsEntity::Spawn( void ) {

	anVec3 origin, size;
	anStr name;
	spawnArgs.GetVector( "origin", "0 0 0", origin );
	spawnArgs.GetVector( "size", "8 8 8", size );
	spawnArgs.GetString( "env_name", "", name );

	if ( name.Length() ) {
		gameRenderWorld->AddEnvBounds( origin, size, name );
	} else {
		gameLocal.Warning( "No env_name field specified on environment definition, skipped" );
	}
	PostEventMS( &EV_Remove, 0 );

}

/*
===============================================================================

	anLadderEntity

===============================================================================
*/

CLASS_DECLARATION( anEntity, anLadderEntity )
END_CLASS

/*
================
anLadderEntity::Spawn
================
*/
void anLadderEntity::Spawn( void ) {
	ladderModel = nullptr;

	anClipModel* model = GetPhysics()->GetClipModel();
	if ( model == nullptr ) {
		gameLocal.Error( "anLadderEntity::Spawn No Collision Model" );
	}

	bool surfaceFound = false;
	for ( int i = 0; i < model->GetNumCollisionModels(); i++ ) {
		anCollisionModel* cm = model->GetCollisionModel( i );
		for ( int j = 0; j < cm->GetNumPolygons(); j++ ) {
			const anMaterial* material = cm->GetPolygonMaterial( j );
			if ( material == nullptr ) {
				continue;
			}

			if ( !( material->GetSurfaceFlags() & SURF_LADDER ) ) {
				continue;
			}

			if ( surfaceFound ) {
				gameLocal.Error( "anLadderEntity::Spawn Multiple Ladder Surfaces" );
			}
			surfaceFound = true;

			ladderNormal = cm->GetPolygonPlane( j ).Normal();

			anFixedWinding ladderWinding;
			cm->GetPolygon( j, ladderWinding );

			anTraceModel trm;
			trm.SetupPolygonPrism( ladderWinding, 16.f );

			ladderModel = new anClipModel( trm, true );
		}
	}
	if ( !surfaceFound ) {
		gameLocal.Error( "anLadderEntity::Spawn No Ladder Surface Found" );
	}

//	BecomeActive( TH_THINK );
}

/*
================
anLadderEntity::~anLadderEntity
================
*/
anLadderEntity::~anLadderEntity( void ) {
	gameLocal.clip.DeleteClipModel( ladderModel );
}

/*
================
anLadderEntity::Think
================
*/
void anLadderEntity::Think( void ) {
	anEntity::Think();
//	ladderModel->Draw( GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
}

/*
================
anLadderEntity::GetLadderNormal
================
*/
anVec3 anLadderEntity::GetLadderNormal( void ) const {
	return GetPhysics()->GetAxis() * ladderNormal;
}
