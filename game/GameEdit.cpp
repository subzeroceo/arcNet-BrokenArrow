// Copyright (C) 2007 Id Software, Inc.
//

#include "Lib.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "GameEdit.h"
#include "Player.h"
#include "vehicles/Transport.h"
#include "WorldSpawn.h"
#include "physics/Physics_Monster.h"
#include "Light.h"
#include "Sound.h"
#include "Misc.h"
//#include "ai/AI.h"
#include "client/ClientEffect.h"

#include "script/Script_Interpreter.h"
#include "script/Script_Compiler.h"

/*
===============================================================================

	Ingame cursor.

===============================================================================
*/

CLASS_DECLARATION( arcEntity, idCursor3D )
END_CLASS

/*
===============
idCursor3D::idCursor3D
===============
*/
idCursor3D::idCursor3D( void ) {
	draggedPosition.Zero();
}

/*
===============
idCursor3D::~idCursor3D
===============
*/
idCursor3D::~idCursor3D( void ) {
}

/*
===============
idCursor3D::Spawn
===============
*/
void idCursor3D::Spawn( void ) {
}

/*
===============
idCursor3D::Present
===============
*/
void idCursor3D::Present( void ) {
	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}
	BecomeInactive( TH_UPDATEVISUALS );

	const anVec3 &origin = GetPhysics()->GetOrigin();
	const anMat3 &axis = GetPhysics()->GetAxis();

	if ( g_dragEntity.GetBool() ) {
		gameRenderWorld->DebugArrow( colorYellow, origin + axis[1] * -5.0f + axis[2] * 5.0f, origin, 2 );
		gameRenderWorld->DebugArrow( colorRed, origin, draggedPosition, 2 );
	}
}

/*
===============
idCursor3D::Think
===============
*/
void idCursor3D::Think( void ) {
	drag.Evaluate( gameLocal.time );
	Present();
}


/*
===============================================================================

	Allows entities to be dragged through the world with physics.

===============================================================================
*/

#define MAX_DRAG_TRACE_DISTANCE			2048.0f

/*
==============
idDragEntity::idDragEntity
==============
*/
idDragEntity::idDragEntity( void ) {
	cursor = nullptr;
	Clear();
}

/*
==============
idDragEntity::~idDragEntity
==============
*/
idDragEntity::~idDragEntity( void ) {
	StopDrag();
	selected = nullptr;
	delete cursor;
	cursor = nullptr;
}


/*
==============
idDragEntity::Clear
==============
*/
void idDragEntity::Clear() {
	StopDrag();

	dragEnt			= nullptr;
	joint			= INVALID_JOINT;
	id				= 0;
	localEntityPoint.Zero();
	localPlayerPoint.Zero();
	bodyName.Clear();
	selected		= nullptr;
}

/*
==============
idDragEntity::StopDrag
==============
*/
void idDragEntity::StopDrag( void ) {
	dragEnt = nullptr;
	if ( cursor ) {
		cursor->BecomeInactive( TH_THINK );
	}
}

/*
==============
idDragEntity::Update
==============
*/
void idDragEntity::Update( arcNetBasePlayer *player ) {
	anVec3 viewPoint, origin;
	anMat3 viewAxis, axis;
	trace_t trace;
	arcEntity *newEnt;
	anAngles angles;
	jointHandle_t newJoint;
	anString newBodyName;

	player->GetViewPos( viewPoint, viewAxis );

	// if no entity selected for dragging
    if ( !dragEnt.GetEntity() ) {

		if ( player->usercmd.buttons.btn.attack ) {

			gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, viewPoint, viewPoint + viewAxis[0] * MAX_DRAG_TRACE_DISTANCE, (CONTENTS_SOLID|CONTENTS_RENDERMODEL|CONTENTS_ACTORBODY|CONTENTS_SLIDEMOVER), player );
			if ( trace.fraction < 1.0f ) {

				newEnt = gameLocal.entities[ trace.c.entityNum ];
				if ( newEnt ) {

					if ( newEnt->GetBindMaster() ) {
						if ( newEnt->GetBindJoint() ) {
							trace.c.id = JOINT_HANDLE_TO_CLIPMODEL_ID( newEnt->GetBindJoint() );
						} else {
							trace.c.id = newEnt->GetBindBody();
						}
						newEnt = newEnt->GetBindMaster();
					}

					if ( newEnt->IsType( arcAFEntity_Base::Type ) && static_cast<arcAFEntity_Base *>(newEnt)->IsActiveAF() ) {
						arcAFEntity_Base *af = static_cast<arcAFEntity_Base *>(newEnt);

						// joint being dragged
						newJoint = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );
						// get the body id from the trace model id which might be a joint handle
						trace.c.id = af->BodyForClipModelId( trace.c.id );
						// get the name of the body being dragged
						newBodyName = af->GetAFPhysics()->GetBody( trace.c.id )->GetName();

					} else if ( newEnt->IsType( sdTransport_AF::Type ) ) {
						sdTransport_AF *af = static_cast<sdTransport_AF *>(newEnt);

						// joint being dragged
						newJoint = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );
						// get the body id from the trace model id which might be a joint handle
						trace.c.id = af->BodyForClipModelId( trace.c.id );
						// get the name of the body being dragged
						newBodyName = af->GetAFPhysics()->GetBody( trace.c.id )->GetName();
						// always drag the center of the body for stability
						trace.c.point = af->GetAFPhysics()->GetOrigin( trace.c.id );

					} else if ( newEnt->IsType( sdTransport::Type ) ) {

						// joint being dragged
						newJoint = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );
						// get the body id from the trace model id which might be a joint handle
						trace.c.id = 0;
						// get the name of the body being dragged
						newBodyName = "";

					} else if ( !newEnt->IsType( idWorldspawn::Type ) ) {

						if ( trace.c.id < 0 ) {
							newJoint = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );
						} else {
							newJoint = INVALID_JOINT;
						}
						newBodyName = "";

					} else {

						newJoint = INVALID_JOINT;
						newEnt = nullptr;
					}
				}
				if ( newEnt && !gameLocal.isClient ) {
					dragEnt = newEnt;
					selected = newEnt;
					joint = newJoint;
					id = trace.c.id;
					bodyName = newBodyName;

					if ( cursor == nullptr ) {
						cursor = static_cast< idCursor3D* >( gameLocal.SpawnEntityType( idCursor3D::Type, true ) );
					}

					anPhysics *phys = dragEnt.GetEntity()->GetPhysics();
					localPlayerPoint = ( trace.c.point - viewPoint ) * viewAxis.Transpose();
					origin = phys->GetOrigin( id );
					axis = phys->GetAxis( id );
					localEntityPoint = ( trace.c.point - origin ) * axis.Transpose();

					cursor->drag.Init( g_dragDamping.GetFloat(), g_dragMaxforce.GetFloat() );
					cursor->drag.SetPhysics( phys, id, localEntityPoint );
					cursor->Show();

					if ( phys->IsType( anPhysics_AF::Type ) ||
							phys->IsType( anPhysics_RigidBody::Type ) ||
								phys->IsType( anPhysics_Monster::Type ) ) {
						cursor->BecomeActive( TH_THINK );
					}
				}
			}
		}
	}

	// if there is an entity selected for dragging
	arcEntity *drag = dragEnt.GetEntity();
	if ( drag ) {

		if ( !player->usercmd.buttons.btn.attack ) {
			StopDrag();
			return;
		}

		if ( cursor ) {

			cursor->SetOrigin( viewPoint + localPlayerPoint * viewAxis );
			cursor->SetAxis( viewAxis );

			cursor->drag.SetDragPosition( cursor->GetPhysics()->GetOrigin() );

			renderEntity_t *renderEntity = drag->GetRenderEntity();
			arcAnimator *dragAnimator = drag->GetAnimator();

			if ( joint != INVALID_JOINT && renderEntity && dragAnimator ) {
				dragAnimator->GetJointTransform( joint, gameLocal.time, cursor->draggedPosition, axis );
				cursor->draggedPosition = renderEntity->origin + cursor->draggedPosition * renderEntity->axis;
				if ( g_dragEntity.GetBool() ) {
					gameRenderWorld->DrawText( va( "%s\n%s\n%s, %s", drag->GetName(), drag->GetType()->classname, dragAnimator->GetJointName( joint ), bodyName.c_str() ), cursor->GetPhysics()->GetOrigin(), 0.1f, colorWhite, viewAxis, 1 );
				}
			} else {
				cursor->draggedPosition = cursor->GetPhysics()->GetOrigin();
				if ( g_dragEntity.GetBool() ) {
					gameRenderWorld->DrawText( va( "%s\n%s\n%s", drag->GetName(), drag->GetType()->classname, bodyName.c_str() ), cursor->GetPhysics()->GetOrigin(), 0.1f, colorWhite, viewAxis, 1 );
				}
			}
		}
	}

	// if there is a selected entity
	if ( g_dragEntity.GetBool() && selected.GetEntity() && g_dragShowSelection.GetBool() ) {
		// draw the bbox of the selected entity
		renderEntity_t *renderEntity = selected.GetEntity()->GetRenderEntity();
		if ( renderEntity ) {
			gameRenderWorld->DebugBox( colorYellow, idBox( renderEntity->bounds, renderEntity->origin, renderEntity->axis ) );
		}
	}
}

/*
==============
idDragEntity::SetSelected
==============
*/
void idDragEntity::SetSelected( arcEntity *ent ) {
	selected = ent;
	StopDrag();
}

/*
==============
idDragEntity::DeleteSelected
==============
*/
void idDragEntity::DeleteSelected( void ) {
	delete selected.GetEntity();
	selected = nullptr;
	StopDrag();
}

/*
==============
idDragEntity::BindSelected
==============
*/
void idDragEntity::BindSelected( void ) {
	int num, largestNum;
	anLexer lexer;
	anToken type, bodyName;
	anString key, value, bindBodyName;
	const anKeyValue *kv;
	arcAFEntity_Base *af;

	af = static_cast<arcAFEntity_Base *>(dragEnt.GetEntity());

	if ( !af || !af->IsType( arcAFEntity_Base::Type ) || !af->IsActiveAF() ) {
		return;
	}

	bindBodyName = af->GetAFPhysics()->GetBody( id )->GetName();
	largestNum = 1;

	// parse all the bind constraints
	kv = af->spawnArgs.MatchPrefix( "bindConstraint ", nullptr );
	while ( kv ) {
		key = kv->GetKey();
		key.Strip( "bindConstraint " );
		if ( sscanf( key, "bind%d", &num ) ) {
			if ( num >= largestNum ) {
				largestNum = num + 1;
			}
		}

		lexer.LoadMemory( kv->GetValue(), kv->GetValue().Length(), kv->GetKey() );
		lexer.ReadToken( &type );
		lexer.ReadToken( &bodyName );
		lexer.FreeSource();

		// if there already exists a bind constraint for this body
		if ( bodyName.Icmp( bindBodyName ) == 0 ) {
			// delete the bind constraint
			af->spawnArgs.Delete( kv->GetKey() );
			kv = nullptr;
		}

		kv = af->spawnArgs.MatchPrefix( "bindConstraint ", kv );
	}

	sprintf( key, "bindConstraint bind%d", largestNum );
	sprintf( value, "ballAndSocket %s %s", bindBodyName.c_str(), af->GetAnimator()->GetJointName( joint ) );

	af->spawnArgs.Set( key, value );
	af->spawnArgs.Set( "bind", "worldspawn" );
	af->Bind( gameLocal.world, true );
}

/*
==============
idDragEntity::UnbindSelected
==============
*/
void idDragEntity::UnbindSelected( void ) {
	const anKeyValue *kv;
	arcAFEntity_Base *af;

	af = static_cast<arcAFEntity_Base *>( selected.GetEntity());

	if ( !af || !af->IsType( arcAFEntity_Base::Type ) || !af->IsActiveAF() ) {
		return;
	}

	// unbind the selected entity
	af->Unbind();

	// delete all the bind constraints
	kv = selected.GetEntity()->spawnArgs.MatchPrefix( "bindConstraint ", nullptr );
	while ( kv ) {
		selected.GetEntity()->spawnArgs.Delete( kv->GetKey() );
		kv = selected.GetEntity()->spawnArgs.MatchPrefix( "bindConstraint ", nullptr );
	}

	// delete any bind information
	af->spawnArgs.Delete( "bind" );
	af->spawnArgs.Delete( "bindToJoint" );
	af->spawnArgs.Delete( "bindToBody" );
}


/*
===============================================================================

	Handles ingame entity editing.

===============================================================================
*/

/*
==============
idEditEntities::idEditEntities
==============
*/
idEditEntities::idEditEntities( void ) {
	selectableEntityClasses.Clear();
	nextSelectTime = 0;
}

/*
=============
idEditEntities::SelectEntity
=============
*/
bool idEditEntities::SelectEntity( const anVec3 &origin, const anVec3 &dir, const arcEntity *skip ) {
	anVec3		end;
	arcEntity	*ent;

	if ( !g_editEntityMode.GetInteger() || selectableEntityClasses.Num() == 0 ) {
		return false;
	}

	if ( gameLocal.time < nextSelectTime ) {
		return true;
	}
	nextSelectTime = gameLocal.time + 300;

	end = origin + dir * 4096.0f;

	ent = nullptr;
	for ( int i = 0; i < selectableEntityClasses.Num(); i++ ) {
		ent = gameLocal.FindTraceEntity( origin, end, *selectableEntityClasses[i].typeInfo, skip );
		if ( ent ) {
			break;
		}
	}
	if ( ent ) {
		ClearSelectedEntities();
		if ( EntityIsSelectable( ent ) ) {
			AddSelectedEntity( ent );
			gameLocal.Printf( "entity #%d: %s '%s'\n", ent->entityNumber, ent->GetClassname(), ent->name.c_str() );
			ent->ShowEditingDialog();
			return true;
		}
	}
	return false;
}

/*
=============
idEditEntities::AddSelectedEntity
=============
*/
void idEditEntities::AddSelectedEntity(arcEntity *ent) {
	ent->fl.selected = true;
	selectedEntities.AddUnique(ent);
}

/*
==============
idEditEntities::RemoveSelectedEntity
==============
*/
void idEditEntities::RemoveSelectedEntity( arcEntity *ent ) {
    if ( selectedEntities.FindElement( ent ) ) {
		selectedEntities.Remove( ent );
	}
}

/*
=============
idEditEntities::ClearSelectedEntities
=============
*/
void idEditEntities::ClearSelectedEntities() {
	int i, count;

	count = selectedEntities.Num();
	for ( i = 0; i < count; i++ ) {
		selectedEntities[i]->fl.selected = false;
	}
	selectedEntities.Clear();
}

/*
=============
idEditEntities::GetSelectedEntities
=============
*/
anList<arcEntity *>& idEditEntities::GetSelectedEntities( void ) {
	return selectedEntities;
}

/*
=============
idEditEntities::EntityIsSelectable
=============
*/
bool idEditEntities::EntityIsSelectable( arcEntity *ent, anVec4 *color, anString *text ) {
	for ( int i = 0; i < selectableEntityClasses.Num(); i++ ) {
		if ( ent->GetType() == selectableEntityClasses[i].typeInfo ) {
			if ( text ) {
				*text = selectableEntityClasses[i].textKey;
			}
			if ( color ) {
				if ( ent->fl.selected ) {
					*color = colorRed;
				} else {
					switch ( i ) {
					case 1 :
						*color = colorYellow;
						break;
					case 2 :
						*color = colorBlue;
						break;
					default:
						*color = colorGreen;
					}
				}
			}
			return true;
		}
	}
	return false;
}

/*
=============
idEditEntities::DisplayEntities
=============
*/
void idEditEntities::DisplayEntities( void ) {
	arcEntity *ent;

	if ( !gameLocal.GetLocalPlayer() ) {
		return;
	}

	selectableEntityClasses.Clear();
	selectedTypeInfo_t sit;

	switch ( g_editEntityMode.GetInteger() ) {
		case 1:
			sit.typeInfo = &idLight::Type;
			sit.textKey = "texture";
			selectableEntityClasses.Append( sit );
			break;
		case 2:
			sit.typeInfo = &idSound::Type;
			sit.textKey = "s_shader";
			selectableEntityClasses.Append( sit );
			sit.typeInfo = &idLight::Type;
			sit.textKey = "texture";
			selectableEntityClasses.Append( sit );
			break;
		case 3:
			sit.typeInfo = &arcAFEntity_Base::Type;
			sit.textKey = "articulatedFigure";
			selectableEntityClasses.Append( sit );
			break;
		case 5:
			/*	jrad - anSAAI removal
			sit.typeInfo = &anSAAI::Type;
			sit.textKey = "name";
			selectableEntityClasses.Append( sit );
			*/
			break;
		case 6:
			sit.typeInfo = &arcEntity::Type;
			sit.textKey = "name";
			selectableEntityClasses.Append( sit );
			break;
		case 7:
			sit.typeInfo = &arcEntity::Type;
			sit.textKey = "model";
			selectableEntityClasses.Append( sit );
			break;
		default:
			return;
	}

	anBounds viewBounds( gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() );
	anBounds viewTextBounds( gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() );
	anMat3 axis = gameLocal.GetLocalPlayer()->viewAngles.ToMat3();

	viewTextBounds.ExpandSelf( 128 );
	viewBounds.ExpandSelf( g_maxShowDistance.GetFloat() );

	anString textKey;

	for ( ent = gameLocal.spawnedEntities.Next(); ent != nullptr; ent = ent->spawnNode.Next() ) {

		anVec4 color;

		textKey = "";
		if ( !EntityIsSelectable( ent, &color, &textKey ) ) {
			continue;
		}

		bool drawArrows = false;
		if ( ent->GetType() == &arcAFEntity_Base::Type ) {
			if ( !static_cast<arcAFEntity_Base *>(ent)->IsActiveAF() ) {
				continue;
			}
		} else if ( ent->GetType() == &idSound::Type ) {
			if ( ent->fl.selected ) {
				drawArrows = true;
			}
			const char* string = ent->spawnArgs.GetString( textKey );

			const idSoundShader *ss = nullptr;

			if ( anString::Length( string ) > 0 ) {
				ss = declHolder.declSoundShaderType.LocalFind( string );
			}
			if ( !ss || ss->HasDefaultSound() || ss->base->GetState() == DS_DEFAULTED ) {
				color.Set( 1.0f, 0.0f, 1.0f, 1.0f );
			}
		}

		if ( !viewBounds.ContainsPoint( ent->GetPhysics()->GetOrigin() ) ) {
			continue;
		}

		gameRenderWorld->DebugBounds( color, anBounds( ent->GetPhysics()->GetOrigin() ).Expand( 8 ) );
		if ( drawArrows ) {
			anVec3 start = ent->GetPhysics()->GetOrigin();
			anVec3 end = start + anVec3( 1, 0, 0 ) * 20.0f;
			gameRenderWorld->DebugArrow( colorWhite, start, end, 2 );
			gameRenderWorld->DrawText( "x+", end + anVec3( 4, 0, 0 ), 0.15f, colorWhite, axis );
			end = start + anVec3( 1, 0, 0 ) * -20.0f;
			gameRenderWorld->DebugArrow( colorWhite, start, end, 2 );
			gameRenderWorld->DrawText( "x-", end + anVec3( -4, 0, 0 ), 0.15f, colorWhite, axis );
			end = start + anVec3( 0, 1, 0 ) * +20.0f;
			gameRenderWorld->DebugArrow( colorGreen, start, end, 2 );
			gameRenderWorld->DrawText( "y+", end + anVec3( 0, 4, 0 ), 0.15f, colorWhite, axis );
			end = start + anVec3( 0, 1, 0 ) * -20.0f;
			gameRenderWorld->DebugArrow( colorGreen, start, end, 2 );
			gameRenderWorld->DrawText( "y-", end + anVec3( 0, -4, 0 ), 0.15f, colorWhite, axis );
			end = start + anVec3( 0, 0, 1 ) * +20.0f;
			gameRenderWorld->DebugArrow( colorBlue, start, end, 2 );
			gameRenderWorld->DrawText( "z+", end + anVec3( 0, 0, 4 ), 0.15f, colorWhite, axis );
			end = start + anVec3( 0, 0, 1 ) * -20.0f;
			gameRenderWorld->DebugArrow( colorBlue, start, end, 2 );
			gameRenderWorld->DrawText( "z-", end + anVec3( 0, 0, -4 ), 0.15f, colorWhite, axis );
		}

		if ( textKey.Length() ) {
			const char *text = ent->spawnArgs.GetString( textKey );
			if ( viewTextBounds.ContainsPoint( ent->GetPhysics()->GetOrigin() ) ) {
				gameRenderWorld->DrawText( text, ent->GetPhysics()->GetOrigin() + anVec3(0, 0, 12), 0.25, colorWhite, axis, 1 );
			}
		}
	}
}


/*
===============================================================================

	anGameEdit

===============================================================================
*/

anGameEdit			gameEditLocal;
anGameEdit *		gameEdit = &gameEditLocal;


/*
=============
anGameEdit::GetSelectedEntities
=============
*/
int anGameEdit::GetSelectedEntities( arcEntity *list[], int max ) {
	int num = 0;
	arcEntity *ent;

	for ( ent = gameLocal.spawnedEntities.Next(); ent != nullptr; ent = ent->spawnNode.Next() ) {
		if ( ent->fl.selected ) {
			list[num++] = ent;
			if ( num >= max ) {
				break;
			}
		}
	}
	return num;
}

/*
=============
anGameEdit::GetSelectedEntitiesByName
=============
*/
int anGameEdit::GetSelectedEntitiesByName( anString *list[], int max ) {
	int num = 0;
	arcEntity *ent;

	for ( ent = gameLocal.spawnedEntities.Next(); ent != nullptr; ent = ent->spawnNode.Next() ) {
		if ( ent->fl.selected ) {
			list[num++] = &ent->name;
			if ( num >= max ) {
				break;
			}
		}
	}
	return num;
}

/*
=============
anGameEdit::TriggerSelected
=============
*/
void anGameEdit::TriggerSelected() {
	arcEntity *ent;
	for ( ent = gameLocal.spawnedEntities.Next(); ent != nullptr; ent = ent->spawnNode.Next() ) {
		if ( ent->fl.selected ) {
			ent->ProcessEvent( &EV_Activate, gameLocal.GetLocalPlayer() );
		}
	}
}

/*
================
anGameEdit::ClearEntitySelection
================
*/
void anGameEdit::ClearEntitySelection() {
	if ( gameLocal.editEntities == nullptr ) {
		return;
	}
	arcEntity *ent;

	for ( ent = gameLocal.spawnedEntities.Next(); ent != nullptr; ent = ent->spawnNode.Next() ) {
		ent->fl.selected = false;
	}
	gameLocal.editEntities->ClearSelectedEntities();
}

/*
================
anGameEdit::AddSelectedEntity
================
*/
void anGameEdit::AddSelectedEntity( arcEntity *ent ) {
	if ( gameLocal.editEntities == nullptr ) {
		return;
	}
	if ( ent ) {
		gameLocal.editEntities->AddSelectedEntity( ent );
	}
}

/*
================
anGameEdit::RemoveSelectedEntity
================
*/
void anGameEdit::RemoveSelectedEntity( arcEntity *ent ) {
	if ( gameLocal.editEntities == nullptr ) {
		return;
	}
	if ( ent ) {
		gameLocal.editEntities->RemoveSelectedEntity( ent );
	}
}


/*
================
anGameEdit::FindEntityDefDict
================
*/
const anDict *anGameEdit::FindEntityDefDict( const char *name, bool makeDefault ) const {
	return gameLocal.FindEntityDefDict( name, makeDefault );
}

/*
================
anGameEdit::SpawnEntityDef
================
*/
void anGameEdit::SpawnEntityDef( const anDict &args, arcEntity **ent ) {
	gameLocal.SpawnEntityDef( args, true, ent );
}

/*
================
anGameEdit::FindEntity
================
*/
arcEntity *anGameEdit::FindEntity( const char *name ) const {
	return gameLocal.FindEntity( name );
}

/*
=============
anGameEdit::GetUniqueEntityName

generates a unique name for a given classname
=============
*/
const char *anGameEdit::GetUniqueEntityName( const char *classname ) const {
	int			id;
	static char	name[1024];

	// can only have MAX_GENTITIES, so if we have a spot available, we're guaranteed to find one
	for ( id = 0; id < MAX_GENTITIES; id++ ) {
		anString::snPrintf( name, sizeof( name ), "%s_%d", classname, id );
		if ( !gameLocal.FindEntity( name ) ) {
			return name;
		}
	}

	// id == MAX_GENTITIES + 1, which can't be in use if we get here
	anString::snPrintf( name, sizeof( name ), "%s_%d", classname, id );
	return name;
}

/*
================
anGameEdit::EntityGetOrigin
================
*/
void  anGameEdit::EntityGetOrigin( arcEntity *ent, anVec3 &org ) const {
	if ( ent ) {
		org = ent->GetPhysics()->GetOrigin();
	}
}

/*
================
anGameEdit::EntityGetAxis
================
*/
void anGameEdit::EntityGetAxis( arcEntity *ent, anMat3 &axis ) const {
	if ( ent ) {
		axis = ent->GetPhysics()->GetAxis();
	}
}

/*
================
anGameEdit::EntitySetOrigin
================
*/
void anGameEdit::EntitySetOrigin( arcEntity *ent, const anVec3 &org ) {
	if ( ent ) {
		ent->SetOrigin( org );
	}
}

/*
================
anGameEdit::EntitySetAxis
================
*/
void anGameEdit::EntitySetAxis( arcEntity *ent, const anMat3 &axis ) {
	if ( ent ) {
		ent->SetAxis( axis );
	}
}

/*
================
anGameEdit::EntitySetColor
================
*/
void anGameEdit::EntitySetColor( arcEntity *ent, const anVec3 color ) {
	if ( ent ) {
		ent->SetColor( color );
	}
}

/*
================
anGameEdit::EntityTranslate
================
*/
void anGameEdit::EntityTranslate( arcEntity *ent, const anVec3 &org ) {
	if ( ent ) {
		ent->GetPhysics()->Translate( org );
	}
}

/*
================
anGameEdit::EntityGetSpawnArgs
================
*/
const anDict *anGameEdit::EntityGetSpawnArgs( arcEntity *ent ) const {
	if ( ent ) {
		return &ent->spawnArgs;
	}
	return nullptr;
}

/*
================
anGameEdit::EntityUpdateChangeableSpawnArgs
================
*/
void anGameEdit::EntityUpdateChangeableSpawnArgs( arcEntity *ent, const anDict *dict ) {
	if ( ent ) {
		ent->UpdateChangeableSpawnArgs( dict );
	}
}

/*
================
anGameEdit::EntityChangeSpawnArgs
================
*/
void anGameEdit::EntityChangeSpawnArgs( arcEntity *ent, const anDict *newArgs ) {
	if ( ent ) {
		for ( int i = 0 ; i < newArgs->GetNumKeyVals () ; i ++ ) {
			const anKeyValue *kv = newArgs->GetKeyVal( i );

			if ( kv->GetValue().Length() > 0 ) {
				ent->spawnArgs.Set ( kv->GetKey(), kv->GetValue() );
			} else {
				ent->spawnArgs.Delete ( kv->GetKey() );
			}
		}
	}
}

/*
================
anGameEdit::EntityUpdateVisuals
================
*/
void anGameEdit::EntityUpdateVisuals( arcEntity *ent ) {
	if ( ent ) {
		ent->UpdateVisuals();
	}
}

/*
================
anGameEdit::EntitySetModel
================
*/
void anGameEdit::EntitySetModel( arcEntity *ent, const char *val ) {
	if ( ent ) {
		ent->spawnArgs.Set( "model", val );
		ent->SetModel( val );
	}
}

/*
================
anGameEdit::EntityStopSound
================
*/
void anGameEdit::EntityStopSound( arcEntity *ent ) {
	if ( ent ) {
		ent->StopSound( SND_ANY );
	}
}

/*
================
anGameEdit::EntityDelete
================
*/
void anGameEdit::EntityDelete( arcEntity *ent ) {
	ent->ProcessEvent( &EV_Remove );
}

/*
================
anGameEdit::EntityToSafeId
================
*/
int anGameEdit::EntityToSafeId ( arcEntity* ent ) const {
	return gameLocal.GetSpawnId( ent );
}

/*
================
anGameEdit::EntityFromSafeId
================
*/
arcEntity *anGameEdit::EntityFromSafeId( int safeID ) const {
	return gameLocal.EntityForSpawnId( safeID );
}

/*
================
anGameEdit::EntityFromIndex
================
*/
arcEntity *anGameEdit::EntityFromIndex( int index ) const {
	if ( index < 0 || index >= MAX_GENTITIES ) {
		return nullptr;
	}

	return gameLocal.entities[index];
}

/*
================
anGameEdit::PlayerIsValid
================
*/
bool anGameEdit::PlayerIsValid() const {
	return ( gameLocal.GetLocalPlayer() != nullptr );
}

/*
================
anGameEdit::PlayerGetOrigin
================
*/
void anGameEdit::PlayerGetOrigin( anVec3 &org ) const {
	if ( gameLocal.GetLocalPlayer() != nullptr )  {
		org = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();
	}
}

/*
================
anGameEdit::PlayerGetAxis
================
*/
void anGameEdit::PlayerGetAxis( anMat3 &axis ) const {
	if ( gameLocal.GetLocalPlayer() != nullptr )  {
		axis = gameLocal.GetLocalPlayer()->GetPhysics()->GetAxis();
	}
}

/*
================
anGameEdit::PlayerGetViewAngles
================
*/
void anGameEdit::PlayerGetViewAngles( anAngles &angles ) const {
	if ( gameLocal.GetLocalPlayer() != nullptr )  {
		angles = gameLocal.GetLocalPlayer()->viewAngles;
	}
}

/*
================
anGameEdit::PlayerGetEyePosition
================
*/
void anGameEdit::PlayerGetEyePosition( anVec3 &org ) const {
	if ( gameLocal.GetLocalPlayer() != nullptr )  {
		org = gameLocal.GetLocalPlayer()->GetEyePosition();
	}
}

/*
================
anGameEdit::MapGetEntityDict
================
*/
const anDict *anGameEdit::MapGetEntityDict( const char *name ) const {
	anMapFile *mapFile = gameLocal.GetLevelMap();
	if ( mapFile && name && *name ) {
		anMapEntity *mapent = mapFile->FindEntity( name );
		if ( mapent ) {
			return &mapent->epairs;
		}
	}
	return nullptr;
}

/*
================
anGameEdit::MapSave
================
*/
void anGameEdit::MapSave( const char *path ) const {
	anMapFile *mapFile = gameLocal.GetLevelMap();
	if ( mapFile != nullptr ) {
		mapFile->Write( ( path != nullptr && path[0] != '\0' ) ? path : mapFile->GetName(), ".entities" );
	}
}

/*
================
anGameEdit::MapSaveClass
================
*/
void anGameEdit::MapSaveClass( const char *path, const char* classname ) const {

	anMapFile *mapFile = gameLocal.GetLevelMap();
	if ( mapFile != nullptr ) {
		anFile* f = fileSystem->OpenFileWrite( ( path != nullptr && path[0] != '\0' ) ? path : va( "%s.entities", mapFile->GetName()) );
		if ( f != nullptr ) {
			int num = 0;
			for ( int i = 0; i < mapFile->GetNumEntities(); i++ ) {
				anMapEntity* ent = mapFile->GetEntity( i );
				if ( anString::Icmp( ent->epairs.GetString( "classname" ), classname ) == 0 ) {
					ent->Write( f, num );
					num++;
				}
			}
			fileSystem->CloseFile( f );
		}
	}
}

/*
================
anGameEdit::MapSetEntityKeyVal
================
*/
void anGameEdit::MapSetEntityKeyVal( const char *name, const char *key, const char *val ) const {
	anMapFile *mapFile = gameLocal.GetLevelMap();
	if ( mapFile && name && *name ) {
		anMapEntity *mapent = mapFile->FindEntity( name );
		if ( mapent ) {
			mapent->epairs.Set( key, val );
		}
	}
}

/*
================
anGameEdit::MapCopyDictToEntity
================
*/
void anGameEdit::MapCopyDictToEntity( const char *name, const anDict *dict ) const {
	anMapFile *mapFile = gameLocal.GetLevelMap();
	if ( mapFile && name && *name ) {
		anMapEntity *mapent = mapFile->FindEntity( name );
		if ( mapent ) {
			for ( int i = 0; i < dict->GetNumKeyVals(); i++ ) {
				const anKeyValue *kv = dict->GetKeyVal( i );
				const char *key = kv->GetKey();
				const char *val = kv->GetValue();
				mapent->epairs.Set( key, val );
			}
		}
	}
}

/*
================
anGameEdit::MapGetUniqueMatchingKeyVals
================
*/
int anGameEdit::MapGetUniqueMatchingKeyVals( const char *key, const char *list[], int max ) const {
	anMapFile *mapFile = gameLocal.GetLevelMap();
	int count = 0;
	if ( mapFile ) {
		for ( int i = 0; i < mapFile->GetNumEntities(); i++ ) {
			anMapEntity *ent = mapFile->GetEntity( i );
			if ( ent ) {
				const char *k = ent->epairs.GetString( key );
				if ( k && *k && count < max ) {
					list[count++] = k;
				}
			}
		}
	}
	return count;
}

/*
================
anGameEdit::MapAddEntity
================
*/
void anGameEdit::MapAddEntity( const anDict *dict ) const {
	anMapFile *mapFile = gameLocal.GetLevelMap();
	if ( mapFile ) {
		anMapEntity *ent = new anMapEntity();
		ent->epairs = *dict;
		mapFile->AddEntity( ent );
	}
}

/*
================
anGameEdit::MapRemoveEntity
================
*/
void anGameEdit::MapRemoveEntity( const char *name ) const {

	anMapFile *mapFile = gameLocal.GetLevelMap();
	if ( mapFile ) {
		anMapEntity *ent = mapFile->FindEntity( name );
		if ( ent ) {
			mapFile->RemoveEntity( ent );
		}
	}
}


/*
================
anGameEdit::MapGetEntitiesMatchignClassWithString
================
*/
int anGameEdit::MapGetEntitiesMatchingClassWithString( const char *classname, const char *list[], const int max, const char* matchKey, const char *matchValue ) const {
	anMapFile *mapFile = gameLocal.GetLevelMap();
	int count = 0;
	if ( mapFile ) {
		int entCount = mapFile->GetNumEntities();
		for ( int i = 0 ; i < entCount; i++ ) {
			anMapEntity *ent = mapFile->GetEntity( i );
			if ( ent != nullptr ) {
				anString work = ent->epairs.GetString( "classname" );
				if ( work.Icmp( classname ) == 0 ) {
					if ( matchKey && *matchKey && matchValue && *matchValue ) {
						work = ent->epairs.GetString( matchKey );
						if ( count < max && work.Icmp( matchValue ) == 0 ) {
							list[count++] = ent->epairs.GetString( "name" );
						}
					} else if ( count < max ) {
						list[count++] = ent->epairs.GetString( "name" );
					}
				}
			}
		}
	}
	return count;
}


// bdube: new game edit stuff
/*
================
anGameEdit::PlayerTraceFromEye
================
*/
bool anGameEdit::PlayerTraceFromEye ( trace_t &results, float length, int contentMask ) {
	anVec3		start;
	anVec3		end;
	anAngles	angles;

	PlayerGetEyePosition( start );
	PlayerGetEyePosition( end );
	PlayerGetViewAngles ( angles );

	end += angles.ToForward() * length;

	return gameLocal.clip.TracePoint ( CLIP_DEBUG_PARMS results, start, end, contentMask, gameLocal.GetLocalPlayer() );
}

/*
================
anGameEdit::EffectRefreshTemplate
================
*/
void anGameEdit::EffectRefreshTemplate ( int effectIndex ) const {
	rvClientEntity* cent;

	// Restart all effects
	for ( cent = gameLocal.clientSpawnedEntities.Next(); cent; cent = cent->spawnNode.Next() ) {
		rvClientEffect* effect = cent->Cast< rvClientEffect >();
		if ( effect && effect->GetEffectIndex() == effectIndex ) {
			effect->Restart();
		}
	}
}

/*
================
anGameEdit::GetGameTime
================
*/
int anGameEdit::GetGameTime ( int *previous ) const {
	if ( previous ) {
		*previous = gameLocal.previousTime;
	}

	return gameLocal.time;
}

/*
================
anGameEdit::SetGameTime
================
*/
void anGameEdit::SetGameTime ( int time ) const {
	gameLocal.time = time;
	gameLocal.previousTime = time;
}

/*
================
anGameEdit::TracePoint
================
*/
bool anGameEdit::TracePoint ( trace_t &results, const anVec3 &start, const anVec3 &end, int contentMask ) const {
	return gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS results, start, end, contentMask, nullptr );
}

/*
================
anGameEdit::MapEntityTranslate
================
*/
void anGameEdit::MapEntityTranslate( const char *name, const anVec3 &v ) const {
	anMapFile *mapFile = gameLocal.GetLevelMap();
	if ( mapFile && name && *name ) {
		anMapEntity *mapent = mapFile->FindEntity( name );
		if ( mapent ) {
			anVec3 origin;
			mapent->epairs.GetVector( "origin", "", origin );
			origin += v;
			mapent->epairs.SetVector( "origin", origin );
		}
	}
}

/*
================
anGameEdit::GetRenderWorld
================
*/
anRenderWorld* anGameEdit::GetRenderWorld() const {
	return gameRenderWorld;
}

/*
================
anGameEdit::GetRenderWorld
================
*/
const char* anGameEdit::MapGetName() const {
	return gameLocal.GetMapName();
}

/*
================
anGameEdit::KillClass
================
*/
void KillEntities( const anCommandArgs &args, const idTypeInfo &superClass, bool delayed );

void anGameEdit::KillClass( const char* classname ) {
	if ( classname == nullptr || classname[0] == '\0' ) {
		return;
	}

	idTypeInfo* info = anClass::GetClass( classname );
	if ( info == nullptr ) {
		gameLocal.Warning( "Unknown class '%s'", classname );
		return;
	}

	anCommandArgs dummyArgs;

	KillEntities( dummyArgs, *info, false );
}

/*
============
anGameEdit::NumUserInterfaceScopes
============
*/
int	anGameEdit::NumUserInterfaceScopes() const {
	return gameLocal.GetUIScopes().Num();
}

/*
============
anGameEdit::GetScope
============
*/
const guiScope_t& anGameEdit::GetScope( int index ) {
	return gameLocal.GetUIScopes()[index];
}
