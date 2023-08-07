#include "../idlib/Lib.h"
#pragma hdrstop

#include "Game_local.h"

/*
===============================================================================

  Articulated figure controller.

===============================================================================
*/
#define ARTICULATED_FIGURE_ANIM		"af_pose"
#define POSE_BOUNDS_EXPANSION		5.0f

/*
================
anAF::anAF
================
*/
anAF::anAF( void ) {
	self = nullptr;
	animator = nullptr;
	modifiedAnim = 0;
	baseOrigin.Zero();
	baseAxis.Identity();
	poseTime = -1;
	restStartTime = -1;
	isLoaded = false;
	isActive = false;
	hasBindConstraints = false;
}

/*
================
anAF::~anAF
================
*/
anAF::~anAF( void ) {
}

/*
================
anAF::Save
================
*/
void anAF::Save( anSaveGame *savefile ) const {
	savefile->WriteObject( self );
	savefile->WriteString( GetName() );
	savefile->WriteBool( hasBindConstraints );
	savefile->WriteVec3( baseOrigin );
	savefile->WriteMat3( baseAxis );
	savefile->WriteInt( poseTime );
	savefile->WriteInt( restStartTime );
	savefile->WriteBool( isLoaded );
	savefile->WriteBool( isActive );
	savefile->WriteStaticObject( physicsObj );
}

/*
================
anAF::Restore
================
*/
void anAF::Restore( anRestoreGame *savefile ) {
	savefile->ReadObject( reinterpret_cast<anClass *&>( self ) );
	savefile->ReadString( name );
	savefile->ReadBool( hasBindConstraints );
	savefile->ReadVec3( baseOrigin );
	savefile->ReadMat3( baseAxis );
	savefile->ReadInt( poseTime );
	savefile->ReadInt( restStartTime );
	savefile->ReadBool( isLoaded );
	savefile->ReadBool( isActive );

	animator = nullptr;
	modifiedAnim = 0;

	if ( self ) {
		SetAnimator( self->GetAnimator() );
		Load( self, name );
		if ( hasBindConstraints ) {
			AddBindConstraints();
		}
	}

	savefile->ReadStaticObject( physicsObj );

	if ( self ) {
		if ( isActive ) {
			// clear all animations
			animator->ClearAllAnims( gameLocal.time, 0 );
			animator->ClearAllJoints();

			// switch to articulated figure physics
			self->RestorePhysics( &physicsObj );
			physicsObj.EnableClip();
		}
		UpdateAnimation();
	}
}

/*
================
anAF::UpdateAnimation
================
*/
bool anAF::UpdateAnimation( void ) {
	anVec3 origin, renderOrigin, bodyOrigin;
	anMat3 axis, renderAxis, bodyAxis;
	renderEntity_t *renderEntity;

	if ( !IsLoaded() ) {
		return false;
	}

	if ( !IsActive() ) {
		return false;
	}

	renderEntity = self->GetRenderEntity();
	if ( !renderEntity ) {
		return false;
	}

	if ( physicsObj.IsAtRest() ) {
		if ( restStartTime == physicsObj.GetRestStartTime() ) {
			return false;
		}
		restStartTime = physicsObj.GetRestStartTime();
	}

	// get the render position
	origin = physicsObj.GetOrigin( 0 );
	axis = physicsObj.GetAxis( 0 );
	renderAxis = baseAxis.Transpose() * axis;
	renderOrigin = origin - baseOrigin * renderAxis;

	// create an animation frame which reflects the current pose of the articulated figure
	animator->InitAFPose();
	for ( int i = 0; i < jointMods.Num(); i++ ) {
		// check for the origin joint
		if ( jointMods[i].jointHandle == 0 ) {
			continue;
		}
		bodyOrigin = physicsObj.GetOrigin( jointMods[i].bodyId );
		bodyAxis = physicsObj.GetAxis( jointMods[i].bodyId );
		axis = jointMods[i].jointBodyAxis.Transpose() * ( bodyAxis * renderAxis.Transpose() );
		origin = ( bodyOrigin - jointMods[i].jointBodyOrigin * axis - renderOrigin ) * renderAxis.Transpose();
		animator->SetAFPoseJointMod( jointMods[i].jointHandle, jointMods[i].jointMod, axis, origin );
	}
	animator->FinishAFPose( modifiedAnim, GetBounds().Expand( POSE_BOUNDS_EXPANSION ), gameLocal.time );
	animator->SetAFPoseBlendWeight( 1.0f );

	return true;
}

/*
================
anAF::GetBounds

returns bounds for the current pose
================
*/
anBounds anAF::GetBounds( void ) const {
	anAFBody *body;
	anVec3 origin, entityOrigin;
	anMat3 axis, entityAxis;
	anBounds bounds, b;

	bounds.Clear();

	// get model base transform
	origin = physicsObj.GetOrigin( 0 );
	axis = physicsObj.GetAxis( 0 );

	entityAxis = baseAxis.Transpose() * axis;
	entityOrigin = origin - baseOrigin * entityAxis;

	// get bounds relative to base
	for ( int i = 0; i < jointMods.Num(); i++ ) {
		body = physicsObj.GetBody( jointMods[i].bodyId );
		origin = ( body->GetWorldOrigin() - entityOrigin ) * entityAxis.Transpose();
		axis = body->GetWorldAxis() * entityAxis.Transpose();
		b.FromTransformedBounds( body->GetClipModel()->GetBounds(), origin, axis );

		bounds += b;
	}

	return bounds;
}

/*
================
anAF::SetupPose

Transforms the articulated figure to match the current animation pose of the given entity.
================
*/
void anAF::SetupPose( anEntity *ent, int time ) {
	anAFBody *body;
	anVec3 origin;
	anMat3 axis;
	anAnimator *animatorPtr;
	renderEntity_t *renderEntity;

	if ( !IsLoaded() || !ent ) {
		return;
	}

	animatorPtr = ent->GetAnimator();
	if ( !animatorPtr ) {
		return;
	}

	renderEntity = ent->GetRenderEntity();
	if ( !renderEntity ) {
		return;
	}

	// if the animation is driven by the physics
	if ( self->GetPhysics() == &physicsObj ) {
		return;
	}

	// if the pose was already updated this frame
	if ( poseTime == time ) {
		return;
	}
	poseTime = time;

	for ( int i = 0; i < jointMods.Num(); i++ ) {
		body = physicsObj.GetBody( jointMods[i].bodyId );
		animatorPtr->GetJointTransform( jointMods[i].jointHandle, time, origin, axis );
		body->SetWorldOrigin( renderEntity->origin + ( origin + jointMods[i].jointBodyOrigin * axis ) * renderEntity->axis );
		body->SetWorldAxis( jointMods[i].jointBodyAxis * axis * renderEntity->axis );
	}

	if ( isActive ) {
		physicsObj.UpdateClipModels();
	}
}

/*
================
anAF::ChangePose

   Change the articulated figure to match the current animation pose of the given entity
   and set the velocity relative to the previous pose.
================
*/
void anAF::ChangePose( anEntity *ent, int time ) {
	float invDelta;
	anAFBody *body;
	anVec3 origin, lastOrigin;
	anMat3 axis;
	anAnimator *animatorPtr;
	renderEntity_t *renderEntity;

	if ( !IsLoaded() || !ent ) {
		return;
	}

	animatorPtr = ent->GetAnimator();
	if ( !animatorPtr ) {
		return;
	}

	renderEntity = ent->GetRenderEntity();
	if ( !renderEntity ) {
		return;
	}

	// if the animation is driven by the physics
	if ( self->GetPhysics() == &physicsObj ) {
		return;
	}

	// if the pose was already updated this frame
	if ( poseTime == time ) {
		return;
	}
	invDelta = 1.0f / MS2SEC( time - poseTime );
	poseTime = time;

	for ( int i = 0; i < jointMods.Num(); i++ ) {
		body = physicsObj.GetBody( jointMods[i].bodyId );
		animatorPtr->GetJointTransform( jointMods[i].jointHandle, time, origin, axis );
		lastOrigin = body->GetWorldOrigin();
		body->SetWorldOrigin( renderEntity->origin + ( origin + jointMods[i].jointBodyOrigin * axis ) * renderEntity->axis );
		body->SetWorldAxis( jointMods[i].jointBodyAxis * axis * renderEntity->axis );
		body->SetLinearVelocity( ( body->GetWorldOrigin() - lastOrigin ) * invDelta );
	}

	physicsObj.UpdateClipModels();
}

/*
================
anAF::EntitiesTouchingAF
================
*/
int anAF::EntitiesTouchingAF( afTouch_t touchList[ MAX_GENTITIES ] ) const {
	anAFBody *body;
	anClipModel *cm;
	anClipModel *clipModels[ MAX_GENTITIES ];

	if ( !IsLoaded() ) {
		return 0;
	}

	int numTouching = 0;
	int numClipModels = gameLocal.clip.ClipModelsTouchingBounds( physicsObj.GetAbsBounds(), -1, clipModels, MAX_GENTITIES );

	for ( int i = 0; i < jointMods.Num(); i++ ) {
		body = physicsObj.GetBody( jointMods[i].bodyId );
		for ( int j = 0; j < numClipModels; j++ ) {
			cm = clipModels[j];
			if ( !cm || cm->GetEntity() == self ) {
				continue;
			}

			if ( !cm->IsTraceModel() ) {
				continue;
			}

			if ( !body->GetClipModel()->GetAbsBounds().IntersectsBounds( cm->GetAbsBounds() ) ) {
				continue;
			}

			if ( gameLocal.clip.ContentsModel( body->GetWorldOrigin(), body->GetClipModel(), body->GetWorldAxis(), -1, cm->Handle(), cm->GetOrigin(), cm->GetAxis() ) ) {
				touchList[ numTouching ].touchedByBody = body;
				touchList[ numTouching ].touchedClipModel = cm;
				touchList[ numTouching ].touchedEnt  = cm->GetEntity();
				numTouching++;
				clipModels[j] = nullptr;
			}
		}
	}

	return numTouching;
}

/*
================
anAF::BodyForClipModelId
================
*/
int anAF::BodyForClipModelId( int id ) const {
	if ( id >= 0 ) {
		return id;
	} else {
		id = CLIPMODEL_ID_TO_JOINT_HANDLE( id );
		if ( id < jointBody.Num() ) {
			return jointBody[id];
		} else {
			return 0;
		}
	}
}

/*
================
anAF::GetPhysicsToVisualTransform
================
*/
void anAF::GetPhysicsToVisualTransform( anVec3 &origin, anMat3 &axis ) const {
	origin = - baseOrigin;
	axis = baseAxis.Transpose();
}

/*
================
anAF::GetImpactInfo
================
*/
void anAF::GetImpactInfo( anEntity *ent, int id, const anVec3 &point, impactInfo_t *info ) {
	SetupPose( self, gameLocal.time );
	physicsObj.GetImpactInfo( BodyForClipModelId( id ), point, info );
}

/*
================
anAF::ApplyImpulse
================
*/
void anAF::ApplyImpulse( anEntity *ent, int id, const anVec3 &point, const anVec3 &impulse ) {
	SetupPose( self, gameLocal.time );
	physicsObj.ApplyImpulse( BodyForClipModelId( id ), point, impulse );
}

/*
================
anAF::AddForce
================
*/
void anAF::AddForce( anEntity *ent, int id, const anVec3 &point, const anVec3 &force ) {
	SetupPose( self, gameLocal.time );
	physicsObj.AddForce( BodyForClipModelId( id ), point, force );
}

/*
================
anAF::AddBody

  Adds a body.
================
*/
void anAF::AddBody( anAFBody *body, const anJointMat *joints, const char *jointName, const AFJointModType_t mod ) {
	int index;
	jointHandle_t handle;
	anVec3 origin;
	anMat3 axis;

	handle = animator->GetJointHandle( jointName );
	if ( handle == INVALID_JOINT ) {
		gameLocal.Error( "anAF for entity '%s' at (%s) modifies unknown joint '%s'", self->name.c_str(), self->GetPhysics()->GetOrigin().ToString( 0 ), jointName );
	}

	assert( handle < animator->NumJoints() );
	origin = joints[ handle ].ToVec3();
	axis = joints[ handle ].ToMat3();

	index = jointMods.Num();
	jointMods.SetNum( index + 1, false );
	jointMods[index].bodyId = physicsObj.GetBodyId( body );
	jointMods[index].jointHandle = handle;
	jointMods[index].jointMod = mod;
	jointMods[index].jointBodyOrigin = ( body->GetWorldOrigin() - origin ) * axis.Transpose();
	jointMods[index].jointBodyAxis = body->GetWorldAxis() * axis.Transpose();
}

/*
================
anAF::SetBase

  Sets the base body.
================
*/
void anAF::SetBase( anAFBody *body, const anJointMat *joints ) {
	physicsObj.ForceBodyId( body, 0 );
	baseOrigin = body->GetWorldOrigin();
	baseAxis = body->GetWorldAxis();
	AddBody( body, joints, animator->GetJointName( animator->GetFirstChild( "origin" ) ), AF_JOINTMOD_AXIS );
}

/*
================
anAF::LoadBody
================
*/
bool anAF::LoadBody( const anDeclAF_Body *fb, const anJointMat *joints ) {
	int id;
	float length, mass;
	anTraceModel trm;
	anClipModel *clip;
	anAFBody *body;
	anMat3 axis, inertiaTensor;
	anVec3 centerOfMass, origin;
	anBounds bounds;
	anList<jointHandle_t> jointList;

	origin = fb->origin.ToVec3();
	axis = fb->angles.ToMat3();
	bounds[0] = fb->v1.ToVec3();
	bounds[1] = fb->v2.ToVec3();

	switch ( fb->modelType ) {
		case TRM_BOX: {
			trm.SetupBox( bounds );
			break;
		}
		case TRM_OCTAHEDRON: {
			trm.SetupOctahedron( bounds );
			break;
		}
		case TRM_DODECAHEDRON: {
			trm.SetupDodecahedron( bounds );
			break;
		}
		case TRM_CYLINDER: {
			trm.SetupCylinder( bounds, fb->numSides );
			break;
		}
		case TRM_CONE: {
			// place the apex at the origin
			bounds[0].z -= bounds[1].z;
			bounds[1].z = 0.0f;
			trm.SetupCone( bounds, fb->numSides );
			break;
		}
		case TRM_BONE: {
			// direction of bone
			axis[2] = fb->v2.ToVec3() - fb->v1.ToVec3();
			length = axis[2].Normalize();
			// axis of bone trace model
			axis[2].NormalVectors( axis[0], axis[1] );
			axis[1] = -axis[1];
			// create bone trace model
			trm.SetupBone( length, fb->width );
			break;
		}
		default:
			assert( 0 );
			break;
	}
	trm.GetMassProperties( 1.0f, mass, centerOfMass, inertiaTensor );
	trm.Translate( -centerOfMass );
	origin += centerOfMass * axis;

	body = physicsObj.GetBody( fb->name );
	if ( body ) {
		clip = body->GetClipModel();
		if ( !clip->IsEqual( trm ) ) {
			clip = new anClipModel( trm );
			clip->SetContents( fb->contents );
			clip->Link( gameLocal.clip, self, 0, origin, axis );
			body->SetClipModel( clip );
		}
		clip->SetContents( fb->contents );
		body->SetDensity( fb->density, fb->inertiaScale );
		body->SetWorldOrigin( origin );
		body->SetWorldAxis( axis );
		id = physicsObj.GetBodyId( body );
	} else {
		clip = new anClipModel( trm );
		clip->SetContents( fb->contents );
		clip->Link( gameLocal.clip, self, 0, origin, axis );
		body = new anAFBody( fb->name, clip, fb->density );
		if ( fb->inertiaScale != mat3_identity ) {
			body->SetDensity( fb->density, fb->inertiaScale );
		}
		id = physicsObj.AddBody( body );
	}
	if ( fb->linearFriction != -1.0f ) {
		body->SetFriction( fb->linearFriction, fb->angularFriction, fb->contactFriction );
	}
	body->SetClipMask( fb->clipMask );
	body->SetSelfCollision( fb->selfCollision );

	if ( fb->jointName == "origin" ) {
		SetBase( body, joints );
	} else {
		AFJointModType_t mod;
		if ( fb->jointMod == DECLAF_JOINTMOD_AXIS ) {
			mod = AF_JOINTMOD_AXIS;
		} else if ( fb->jointMod == DECLAF_JOINTMOD_ORIGIN ) {
			mod = AF_JOINTMOD_ORIGIN;
		} else if ( fb->jointMod == DECLAF_JOINTMOD_BOTH ) {
			mod = AF_JOINTMOD_BOTH;
		} else {
			mod = AF_JOINTMOD_AXIS;
		}
		AddBody( body, joints, fb->jointName, mod );
	}

	if ( fb->frictionDirection.ToVec3() != vec3_origin ) {
		body->SetFrictionDirection( fb->frictionDirection.ToVec3() );
	}
	if ( fb->contactMotorDirection.ToVec3() != vec3_origin ) {
		body->SetContactMotorDirection( fb->contactMotorDirection.ToVec3() );
	}

	// update table to find the nearest articulated figure body for a joint of the skeletal model
	animator->GetJointList( fb->containedJoints, jointList );
	for ( int i = 0; i < jointList.Num(); i++ ) {
		if ( jointBody[ jointList[i] ] != -1 ) {
			gameLocal.Warning( "%s: joint '%s' is already contained by body '%s'", name.c_str(), animator->GetJointName( (jointHandle_t)jointList[i] ), physicsObj.GetBody( jointBody[ jointList[i] ] )->GetName().c_str() );
		}
		jointBody[ jointList[i] ] = id;
	}

	return true;
}

/*
================
anAF::LoadConstraint
================
*/
bool anAF::LoadConstraint( const anDeclAF_Constraint *fc ) {
	anAngles angles;
	anMat3 axis;

	anAFBody *body1 = physicsObj.GetBody( fc->body1 );
	anAFBody *body2 = physicsObj.GetBody( fc->body2 );

	switch ( fc->type ) {
		case DECLAF_CONSTRAINT_FIXED: {
			anAFConstraint_Fixed *c;
			c = static_cast<anAFConstraint_Fixed *>( physicsObj.GetConstraint( fc->name ) );
			if ( c ) {
				c->SetBody1( body1 );
				c->SetBody2( body2 );
			} else {
				c = new anAFConstraint_Fixed( fc->name, body1, body2 );
				physicsObj.AddConstraint( c );
			}
			break;
		}
		case DECLAF_CONSTRAINT_BALLANDSOCKETJOINT: {
			anAFConstraint_BallAndSocketJoint *c;
			c = static_cast<anAFConstraint_BallAndSocketJoint *>( physicsObj.GetConstraint( fc->name ) );
			if ( c ) {
				c->SetBody1( body1 );
				c->SetBody2( body2 );
			} else {
				c = new anAFConstraint_BallAndSocketJoint( fc->name, body1, body2 );
				physicsObj.AddConstraint( c );
			}
			c->SetAnchor( fc->anchor.ToVec3() );
			c->SetFriction( fc->friction );
			switch ( fc->limit ) {
				case anDeclAF_Constraint::LIMIT_CONE: {
					c->SetConeLimit( fc->limitAxis.ToVec3(), fc->limitAngles[0], fc->shaft[0].ToVec3() );
					break;
				}
				case anDeclAF_Constraint::LIMIT_PYRAMID: {
					angles = fc->limitAxis.ToVec3().ToAngles();
					angles.roll = fc->limitAngles[2];
					axis = angles.ToMat3();
					c->SetPyramidLimit( axis[0], axis[1], fc->limitAngles[0], fc->limitAngles[1], fc->shaft[0].ToVec3() );
					break;
				}
				default: {
					c->SetNoLimit();
					break;
				}
			}
			break;
		}
		case DECLAF_CONSTRAINT_UNIVERSALJOINT: {
			anAFConstraint_UniversalJoint *c;
			c = static_cast<anAFConstraint_UniversalJoint *>( physicsObj.GetConstraint( fc->name ) );
			if ( c ) {
				c->SetBody1( body1 );
				c->SetBody2( body2 );
			} else {
				c = new anAFConstraint_UniversalJoint( fc->name, body1, body2 );
				physicsObj.AddConstraint( c );
			}
			c->SetAnchor( fc->anchor.ToVec3() );
			c->SetShafts( fc->shaft[0].ToVec3(), fc->shaft[1].ToVec3() );
			c->SetFriction( fc->friction );
			switch ( fc->limit ) {
				case anDeclAF_Constraint::LIMIT_CONE: {
					c->SetConeLimit( fc->limitAxis.ToVec3(), fc->limitAngles[0] );
					break;
				}
				case anDeclAF_Constraint::LIMIT_PYRAMID: {
					angles = fc->limitAxis.ToVec3().ToAngles();
					angles.roll = fc->limitAngles[2];
					axis = angles.ToMat3();
					c->SetPyramidLimit( axis[0], axis[1], fc->limitAngles[0], fc->limitAngles[1] );
					break;
				}
				default: {
					c->SetNoLimit();
					break;
				}
			}
			break;
		}
		case DECLAF_CONSTRAINT_HINGE: {
			anAFConstraint_Hinge *c;
			c = static_cast<anAFConstraint_Hinge *>( physicsObj.GetConstraint( fc->name ) );
			if ( c ) {
				c->SetBody1( body1 );
				c->SetBody2( body2 );
			} else {
				c = new anAFConstraint_Hinge( fc->name, body1, body2 );
				physicsObj.AddConstraint( c );
			}
			c->SetAnchor( fc->anchor.ToVec3() );
			c->SetAxis( fc->axis.ToVec3() );
			c->SetFriction( fc->friction );
			switch ( fc->limit ) {
				case anDeclAF_Constraint::LIMIT_CONE: {
					anVec3 left, up, axis, shaft;
					fc->axis.ToVec3().OrthogonalBasis( left, up );
					axis = left * anRotation( vec3_origin, fc->axis.ToVec3(), fc->limitAngles[0] );
					shaft = left * anRotation( vec3_origin, fc->axis.ToVec3(), fc->limitAngles[2] );
					c->SetLimit( axis, fc->limitAngles[1], shaft );
					break;
				}
				default: {
					c->SetNoLimit();
					break;
				}
			}
			break;
		}
		case DECLAF_CONSTRAINT_SLIDER: {
			anAFConstraint_Slider *c;
			c = static_cast<anAFConstraint_Slider *>( physicsObj.GetConstraint( fc->name ) );
			if ( c ) {
				c->SetBody1( body1 );
				c->SetBody2( body2 );
			} else {
				c = new anAFConstraint_Slider( fc->name, body1, body2 );
				physicsObj.AddConstraint( c );
			}
			c->SetAxis( fc->axis.ToVec3() );
			break;
		}
		case DECLAF_CONSTRAINT_SPRING: {
			anAFConstraint_Spring *c;
			c = static_cast<anAFConstraint_Spring *>( physicsObj.GetConstraint( fc->name ) );
			if ( c ) {
				c->SetBody1( body1 );
				c->SetBody2( body2 );
			} else {
				c = new anAFConstraint_Spring( fc->name, body1, body2 );
				physicsObj.AddConstraint( c );
			}
			c->SetAnchor( fc->anchor.ToVec3(), fc->anchor2.ToVec3() );
			c->SetSpring( fc->stretch, fc->compress, fc->damping, fc->restLength );
			c->SetLimit( fc->minLength, fc->maxLength );
			break;
		}
	}
	return true;
}

/*
================
GetJointTransform
================
*/
static bool GetJointTransform( void *model, const anJointMat *frame, const char *jointName, anVec3 &origin, anMat3 &axis ) {
	jointHandle_t joint = reinterpret_cast<anAnimator *>( model )->GetJointHandle( jointName );
	if ( ( joint >= 0 ) && ( joint < reinterpret_cast<anAnimator *>( model )->NumJoints() ) ) {
		origin = frame[ joint ].ToVec3();
		axis = frame[ joint ].ToMat3();
		return true;
	} else {
		return false;
	}
}

/*
================
anAF::Load
================
*/
bool anAF::Load( anEntity *ent, const char *fileName ) {
	const anDeclAF *file;
	const anDeclModelDef *modelDef;
	anRenderModel *model;
	int numJoints;
	anJointMat *joints;

	assert( ent );

	self = ent;
	physicsObj.SetSelf( self );

	if ( animator == nullptr ) {
		gameLocal.Warning( "Couldn't load af '%s' for entity '%s' at (%s): nullptr animator\n", name.c_str(), ent->name.c_str(), ent->GetPhysics()->GetOrigin().ToString( 0 ) );
		return false;
	}

	name = fileName;
	name.StripFileExtension();

	file = static_cast<const anDeclAF *>( declManager->FindType( DECL_AF, name ) );
	if ( !file ) {
		gameLocal.Warning( "Couldn't load af '%s' for entity '%s' at (%s)\n", name.c_str(), ent->name.c_str(), ent->GetPhysics()->GetOrigin().ToString( 0 ) );
		return false;
	}

	if ( file->bodies.Num() == 0 || file->bodies[0]->jointName != "origin" ) {
		gameLocal.Warning( "anAF::Load: articulated figure '%s' for entity '%s' at (%s) has no body which modifies the origin joint.",
							name.c_str(), ent->name.c_str(), ent->GetPhysics()->GetOrigin().ToString( 0 ) );
		return false;
	}

	modelDef = animator->ModelDef();
	if ( modelDef == nullptr || modelDef->GetState() == DS_DEFAULTED ) {
		gameLocal.Warning( "anAF::Load: articulated figure '%s' for entity '%s' at (%s) has no or defaulted modelDef '%s'",
							name.c_str(), ent->name.c_str(), ent->GetPhysics()->GetOrigin().ToString( 0 ), modelDef ? modelDef->GetName() : "" );
		return false;
	}

	model = animator->ModelHandle();
	if ( model == nullptr || model->IsDefaultModel() ) {
		gameLocal.Warning( "anAF::Load: articulated figure '%s' for entity '%s' at (%s) has no or defaulted model '%s'",
							name.c_str(), ent->name.c_str(), ent->GetPhysics()->GetOrigin().ToString( 0 ), model ? model->Name() : "" );
		return false;
	}

	// get the modified animation
	modifiedAnim = animator->GetAnim( ARTICULATED_FIGURE_ANIM );
	if ( !modifiedAnim ) {
		gameLocal.Warning( "anAF::Load: articulated figure '%s' for entity '%s' at (%s) has no modified animation '%s'",
							name.c_str(), ent->name.c_str(), ent->GetPhysics()->GetOrigin().ToString( 0 ), ARTICULATED_FIGURE_ANIM );
		return false;
	}

	// create the animation frame used to setup the articulated figure
	numJoints = animator->NumJoints();
	joints = ( anJointMat * )_alloca16( numJoints * sizeof( joints[0] ) );
	gameEdit->ANIM_CreateAnimFrame( model, animator->GetAnim( modifiedAnim )->MD5Anim( 0 ), numJoints, joints, 1, animator->ModelDef()->GetVisualOffset(), animator->RemoveOrigin() );

	// set all vector positions from model joints
	file->Finish( GetJointTransform, joints, animator );

	// initialize articulated figure physics
	physicsObj.SetGravity( gameLocal.GetGravity() );
	physicsObj.SetClipMask( file->clipMask );
	physicsObj.SetDefaultFriction( file->defaultLinearFriction, file->defaultAngularFriction, file->defaultContactFriction );
	physicsObj.SetSuspendSpeed( file->suspendVelocity, file->suspendAcceleration );
	physicsObj.SetSuspendTolerance( file->noMoveTime, file->noMoveTranslation, file->noMoveRotation );
	physicsObj.SetSuspendTime( file->minMoveTime, file->maxMoveTime );
	physicsObj.SetSelfCollision( file->selfCollision );

	// clear the list with transforms from joints to bodies
	jointMods.SetNum( 0, false );

	// clear the joint to body conversion list
	jointBody.AssureSize( animator->NumJoints() );
	for ( int i = 0; i < jointBody.Num(); i++ ) {
		jointBody[i] = -1;
	}

	// delete any bodies in the physicsObj that are no longer in the anDeclAF
	for ( int i = 0; i < physicsObj.GetNumBodies(); i++ ) {
		anAFBody *body = physicsObj.GetBody( i );
		for ( int j = 0; j < file->bodies.Num(); j++ ) {
			if ( file->bodies[j]->name.Icmp( body->GetName() ) == 0 ) {
				break;
			}
		}
		if ( j >= file->bodies.Num() ) {
			physicsObj.DeleteBody( i );
			i--;
		}
	}

	// delete any constraints in the physicsObj that are no longer in the anDeclAF
	for ( int i = 0; i < physicsObj.GetNumConstraints(); i++ ) {
		anAFConstraint *constraint = physicsObj.GetConstraint( i );
		for ( int j = 0; j < file->constraints.Num(); j++ ) {
			if ( file->constraints[j]->name.Icmp( constraint->GetName() ) == 0 &&
					file->constraints[j]->type == constraint->GetType() ) {
				break;
			}
		}
		if ( int j >= file->constraints.Num() ) {
			physicsObj.DeleteConstraint( i );
			i--;
		}
	}

	// load bodies from the file
	for ( int i = 0; i < file->bodies.Num(); i++ ) {
		LoadBody( file->bodies[i], joints );
	}

	// load constraints from the file
	for ( int i = 0; i < file->constraints.Num(); i++ ) {
		LoadConstraint( file->constraints[i] );
	}

	physicsObj.UpdateClipModels();

	// check if each joint is contained by a body
	for ( int i = 0; i < animator->NumJoints(); i++ ) {
		if ( jointBody[i] == -1 ) {
			gameLocal.Warning( "anAF::Load: articulated figure '%s' for entity '%s' at (%s) joint '%s' is not contained by a body",
				name.c_str(), self->name.c_str(), self->GetPhysics()->GetOrigin().ToString( 0 ), animator->GetJointName( (jointHandle_t)i ) );
		}
	}

	physicsObj.SetMass( file->totalMass );
	physicsObj.SetChanged();

	// disable the articulated figure for collision detection until activated
	physicsObj.DisableClip();

	isLoaded = true;

	return true;
}

/*
================
anAF::Start
================
*/
void anAF::Start( void ) {
	if ( !IsLoaded() ) {
		return;
	}
	// clear all animations
	animator->ClearAllAnims( gameLocal.time, 0 );
	animator->ClearAllJoints();
	// switch to articulated figure physics
	self->SetPhysics( &physicsObj );
	// start the articulated figure physics simulation
	physicsObj.EnableClip();
	physicsObj.Activate();
	isActive = true;
}

/*
================
anAF::TestSolid
================
*/
bool anAF::TestSolid( void ) const {
	anAFBody *body;
	trace_t trace;
	anStr str;
	bool solid;

	if ( !IsLoaded() ) {
		return false;
	}

	if ( !af_testSolid.GetBool() ) {
		return false;
	}

	solid = false;

	for ( int i = 0; i < physicsObj.GetNumBodies(); i++ ) {
		body = physicsObj.GetBody( i );
		if ( gameLocal.clip.Translation( trace, body->GetWorldOrigin(), body->GetWorldOrigin(), body->GetClipModel(), body->GetWorldAxis(), body->GetClipMask(), self ) ) {
			float depth = anMath::Fabs( trace.c.point * trace.c.normal - trace.c.dist );
			body->SetWorldOrigin( body->GetWorldOrigin() + trace.c.normal * ( depth + 8.0f ) );
			gameLocal.DWarning( "%s: body '%s' stuck in %d (normal = %.2f %.2f %.2f, depth = %.2f)", self->name.c_str(),
						body->GetName().c_str(), trace.c.contents, trace.c.normal.x, trace.c.normal.y, trace.c.normal.z, depth );
			solid = true;
		}
	}
	return solid;
}

/*
================
anAF::StartFromCurrentPose
================
*/
void anAF::StartFromCurrentPose( int inheritVelocityTime ) {
	if ( !IsLoaded() ) {
		return;
	}

	// if the ragdoll should inherit velocity from the animation
	if ( inheritVelocityTime > 0 ) {
		// make sure the ragdoll is at rest
		physicsObj.PutToRest();

		// set the pose for some time back
		SetupPose( self, gameLocal.time - inheritVelocityTime );

		// change the pose for the current time and set velocities
		ChangePose( self, gameLocal.time );
	} else {
		// transform the articulated figure to reflect the current animation pose
		SetupPose( self, gameLocal.time );
	}
	physicsObj.UpdateClipModels();

	TestSolid();
	Start();
	UpdateAnimation();

	// update the render entity origin and axis
	self->UpdateModel();

	// make sure the renderer gets the updated origin and axis
	self->Present();
}

/*
================
anAF::Stop
================
*/
void anAF::Stop( void ) {
	// disable the articulated figure for collision detection
	physicsObj.UnlinkClip();
	isActive = false;
}

/*
================
anAF::Rest
================
*/
void anAF::Rest( void ) {
	physicsObj.PutToRest();
}

/*
================
anAF::SetConstraintPosition

  Only moves constraints that bind the entity to another entity.
================
*/
void anAF::SetConstraintPosition( const char *name, const anVec3 &pos ) {
	anAFConstraint *constraint;
	constraint = GetPhysics()->GetConstraint( name );

	if ( !constraint ) {
		gameLocal.Warning( "can't find a constraint with the name '%s'", name );
		return;
	}

	if ( constraint->GetBody2() != nullptr ) {
		gameLocal.Warning( "constraint '%s' does not bind to another entity", name );
		return;
	}

	switch ( constraint->GetType() ) {
		case CONSTRAINT_BALLANDSOCKETJOINT: {
			anAFConstraint_BallAndSocketJoint *bs = static_cast<anAFConstraint_BallAndSocketJoint *>(constraint);
			bs->Translate( pos - bs->GetAnchor() );
			break;
		}
		case CONSTRAINT_UNIVERSALJOINT: {
			anAFConstraint_UniversalJoint *uj = static_cast<anAFConstraint_UniversalJoint *>(constraint);
			uj->Translate( pos - uj->GetAnchor() );
			break;
		}
		case CONSTRAINT_HINGE: {
			anAFConstraint_Hinge *hinge = static_cast<anAFConstraint_Hinge *>(constraint);
			hinge->Translate( pos - hinge->GetAnchor() );
			break;
		}
		default: {
			gameLocal.Warning( "cannot set the constraint position for '%s'", name );
			break;
		}
	}
}

/*
================
anAF::SaveState
================
*/
void anAF::SaveState( anDict &args ) const {
	for ( int i = 0; i < jointMods.Num(); i++ ) {
		anAFBody body = physicsObj.GetBody( jointMods[i].bodyId );
		anStr key = "body " + body->GetName();
		anStr value = body->GetWorldOrigin().ToString( 8 );
		value += " ";
		value += body->GetWorldAxis().ToAngles().ToString( 8 );
		args.Set( key, value );
	}
}

/*
================
anAF::LoadState
================
*/
void anAF::LoadState( const anDict &args ) {
	anStr name;
	anAFBody *body;
	anVec3 origin;
	anAngles angles;

	const anKeyValue *kv = args.MatchPrefix( "body ", nullptr );
	while ( kv ) {
		name = kv->GetKey();
		name.Strip( "body " );
		body = physicsObj.GetBody( name );
		if ( body ) {
			sscanf( kv->GetValue(), "%f %f %f %f %f %f", &origin.x, &origin.y, &origin.z, &angles.pitch, &angles.yaw, &angles.roll );
			body->SetWorldOrigin( origin );
			body->SetWorldAxis( angles.ToMat3() );
		} else {
			gameLocal.Warning( "Unknown body part %s in articulated figure %s", name.c_str(), this->name.c_str());
		}

		kv = args.MatchPrefix( "body ", kv );
	}

	physicsObj.UpdateClipModels();
}

/*
================
anAF::AddBindConstraints
================
*/
void anAF::AddBindConstraints( void ) {
	const anKeyValue *kv;
	anStr name;
	anAFBody *body;
	anLexer lexer;
	anToken type, bodyName, jointName;

	if ( !IsLoaded() ) {
		return;
	}

	const anDict &args = self->spawnArgs;

	// get the render position
	anVec3 origin = physicsObj.GetOrigin( 0 );
	anMat3 axis = physicsObj.GetAxis( 0 );
	anMat3 renderAxis = baseAxis.Transpose() * axis;
	anVec3 renderOrigin = origin - baseOrigin * renderAxis;

	// parse all the bind constraints
	for ( kv = args.MatchPrefix( "bindConstraint ", nullptr ); kv; kv = args.MatchPrefix( "bindConstraint ", kv ) ) {
		name = kv->GetKey();
		name.Strip( "bindConstraint " );

		lexer.LoadMemory( kv->GetValue(), kv->GetValue().Length(), kv->GetKey() );
		lexer.ReadToken( &type );

		lexer.ReadToken( &bodyName );
		body = physicsObj.GetBody( bodyName );
		if ( !body ) {
			gameLocal.Warning( "anAF::AddBindConstraints: body '%s' not found on entity '%s'", bodyName.c_str(), self->name.c_str() );
			lexer.FreeSource();
			continue;
		}

		if ( type.Icmp( "fixed" ) == 0 ) {
			anAFConstraint_Fixed *c = new anAFConstraint_Fixed( name, body, nullptr );
			physicsObj.AddConstraint( c );
		} else if ( type.Icmp( "ballAndSocket" ) == 0 ) {
			anAFConstraint_BallAndSocketJoint *c;

			c = new anAFConstraint_BallAndSocketJoint( name, body, nullptr );
			physicsObj.AddConstraint( c );
			lexer.ReadToken( &jointName );

			jointHandle_t joint = animator->GetJointHandle( jointName );
			if ( joint == INVALID_JOINT ) {
				gameLocal.Warning( "anAF::AddBindConstraints: joint '%s' not found", jointName.c_str() );
			}

			animator->GetJointTransform( joint, gameLocal.time, origin, axis );
			c->SetAnchor( renderOrigin + origin * renderAxis );
		} else if ( type.Icmp( "universal" ) == 0 ) {
			anAFConstraint_UniversalJoint *c = new anAFConstraint_UniversalJoint( name, body, nullptr );
			physicsObj.AddConstraint( c );
			lexer.ReadToken( &jointName );

			jointHandle_t joint = animator->GetJointHandle( jointName );
			if ( joint == INVALID_JOINT ) {
				gameLocal.Warning( "anAF::AddBindConstraints: joint '%s' not found", jointName.c_str() );
			}
			animator->GetJointTransform( joint, gameLocal.time, origin, axis );
			c->SetAnchor( renderOrigin + origin * renderAxis );
			c->SetShafts( anVec3( 0, 0, 1 ), anVec3( 0, 0, -1 ) );
		} else {
			gameLocal.Warning( "anAF::AddBindConstraints: unknown constraint type '%s' on entity '%s'", type.c_str(), self->name.c_str() );
		}

		lexer.FreeSource();
	}
	hasBindConstraints = true;
}

/*
================
anAF::RemoveBindConstraints
================
*/
void anAF::RemoveBindConstraints( void ) {
	if ( !IsLoaded() ) {
		return;
	}

	const anDict &args = self->spawnArgs;
	anStr name;

	const anKeyValue *kv = args.MatchPrefix( "bindConstraint ", nullptr );
	while ( kv ) {
		name = kv->GetKey();
		name.Strip( "bindConstraint " );

		if ( physicsObj.GetConstraint( name ) ) {
            physicsObj.DeleteConstraint( name );
		}

		kv = args.MatchPrefix( "bindConstraint ", kv );
	}

	hasBindConstraints = false;
}
