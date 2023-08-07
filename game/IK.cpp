// Copyright (C) 2007 Id Software, Inc.
//

#include "Lib.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ARC_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "IK.h"
#include "Entity.h"
#include "anim/Anim.h"
#include "Player.h"
#include "Mover.h"

/*
===============================================================================

  anIK

===============================================================================
*/

CLASS_DECLARATION( anClass, anIK )
END_CLASS

/*
================
anIK::anIK
================
*/
anIK::anIK( void ) {
	ik_activate = false;
	initialized = false;
	self = nullptr;
	animator = nullptr;
	modifiedAnim = 0;
	modelOffset.Zero();
}

/*
================
anIK::~anIK
================
*/
anIK::~anIK( void ) {
}

/*
================
anIK::IsInitialized
================
*/
bool anIK::IsInitialized( void ) const {
	return initialized && ik_enable.GetBool();
}

/*
================
anIK::IsInhibited
================
*/
bool anIK::IsInhibited( void ) const {
	return gameLocal.isClient && ( self->aorFlags & AOR_INHIBIT_IK );
}

/*
================
anIK::GetPhysics
================
*/
anPhysics* anIK::GetPhysics() {
	return self->GetPhysics();
}

/*
================
anIK::GetAnimator
================
*/
anAnimator *anIK::GetAnimator() {
	return animator;
}

/*
================
anIK::Init
================
*/
bool anIK::Init( anEntity *self, const char *anim, const anVec3 &modelOffset ) {
	anRenderModel *model;

	if ( self == nullptr ) {
		return false;
	}

	this->self = self;

	animator = self->GetAnimator();
	if ( animator == nullptr || animator->ModelDef() == nullptr ) {
		gameLocal.Warning( "anIK::Init: IK for entity '%s' at (%s) has no model set.",
							self->name.c_str(), self->GetPhysics()->GetOrigin().ToString( 0 ) );
		return false;
	}
	if ( animator->ModelDef()->ModelHandle() == nullptr ) {
		gameLocal.Warning( "anIK::Init: IK for entity '%s' at (%s) uses default model.",
							self->name.c_str(), self->GetPhysics()->GetOrigin().ToString( 0 ) );
		return false;
	}
	model = animator->ModelHandle();
	if ( model == nullptr ) {
		gameLocal.Warning( "anIK::Init: IK for entity '%s' at (%s) has no model set.",
							self->name.c_str(), self->GetPhysics()->GetOrigin().ToString( 0 ) );
		return false;
	}
	modifiedAnim = animator->GetAnim( anim );
	if ( modifiedAnim == 0 ) {
		gameLocal.Warning( "anIK::Init: IK for entity '%s' at (%s) has no modified animation.",
								self->name.c_str(), self->GetPhysics()->GetOrigin().ToString( 0 ) );
		return false;
	}

	this->modelOffset = modelOffset;

	return true;
}

/*
================
anIK::Evaluate
================
*/
bool anIK::Evaluate( void ) {
	return false;
}

/*
================
anIK::ClearJointMods
================
*/
void anIK::ClearJointMods( void ) {
	ik_activate = false;
}

/*
================
anIK::SolveTwoBones
================
*/
bool anIK::SolveTwoBones( const anVec3 &startPos, const anVec3 &endPos, const anVec3 &dir, float len0, float len1, anVec3 &jointPos ) {
	float length, lengthSqr, lengthInv, x, y;
	anVec3 vec0, vec1;

	vec0 = endPos - startPos;
	lengthSqr = vec0.LengthSqr();
	lengthInv = anMath::InvSqrt( lengthSqr );
	length = lengthInv * lengthSqr;

	// if the start and end position are too far out or too close to each other
	if ( length > len0 + len1 || length < anMath::Fabs( len0 - len1 ) ) {
		jointPos = startPos + 0.5f * vec0;
		return false;
	}

	vec0 *= lengthInv;
	vec1 = dir - vec0 * dir * vec0;
	vec1.Normalize();

	x = ( length * length + len0 * len0 - len1 * len1 ) * ( 0.5f * lengthInv );
	y = anMath::Sqrt( len0 * len0 - x * x );

	jointPos = startPos + ( x * vec0 ) + ( y * vec1 );

	return true;
}

/*
================
anIK::GetBoneAxis
================
*/
float anIK::GetBoneAxis( const anVec3 &startPos, const anVec3 &endPos, const anVec3 &dir, anMat3 &axis ) {
	axis[0] = endPos - startPos;
	float length = axis[0].Normalize();

	axis[1] = dir - ( axis[0] * ( dir * axis[0] ) );
	axis[1].Normalize();

	axis[2].Cross( axis[1], axis[0] );

	return length;
}

/*
===============================================================================

  anIK_Walk

===============================================================================
*/

CLASS_DECLARATION( anIK, anIK_Walk )
END_CLASS

/*
================
anIK_Walk::anIK_Walk
================
*/
anIK_Walk::anIK_Walk( void ) {
	initialized = false;
	footModel = nullptr;
	numLegs = 0;
	enabledLegs = 0;
	for ( int i = 0; i < MAX_LEGS; i++ ) {
		footJoints[i] = INVALID_JOINT;
		ankleJoints[i] = INVALID_JOINT;
		kneeJoints[i] = INVALID_JOINT;
		hipJoints[i] = INVALID_JOINT;
		dirJoints[i] = INVALID_JOINT;
		hipForward[i].Zero();
		kneeForward[i].Zero();
		upperLegLength[i] = 0.0f;
		lowerLegLength[i] = 0.0f;
		upperLegToHipJoint[i].Identity();
		lowerLegToKneeJoint[i].Identity();
		oldAnkleHeights[i] = 0.0f;
	}
	waistJoint = INVALID_JOINT;

	smoothing = 0.75f;
	waistSmoothing = 0.5f;
	footShift = 0.0f;
	waistShift = 0.0f;
	minWaistFloorDist = 0.0f;
	minWaistAnkleDist = 0.0f;
	footUpTrace = 32.0f;
	footDownTrace = 32.0f;
	tiltWaist = false;
	usePivot = false;

	pivotFoot = -1;
	pivotYaw = 0.0f;
	pivotPos.Zero();

	oldHeightsValid = false;
	oldWaistHeight = 0.0f;
	waistOffset.Zero();
}

/*
================
anIK_Walk::~anIK_Walk
================
*/
anIK_Walk::~anIK_Walk( void ) {
	gameLocal.clip.DeleteClipModel( footModel );
}

/*
================
anIK_Walk::Init
================
*/
bool anIK_Walk::Init( anEntity *self, const char *anim, const anVec3 &modelOffset ) {
	int i;
	float footSize;
	anVec3 verts[4];
	anTraceModel trm;
	const char *jointName;
	anVec3 dir, ankleOrigin, kneeOrigin, hipOrigin, dirOrigin;
	anMat3 axis, ankleAxis, kneeAxis, hipAxis;

	static anVec3 footWinding[4] = {
		anVec3(  1.0f,  1.0f, 0.0f ),
		anVec3( -1.0f,  1.0f, 0.0f ),
		anVec3( -1.0f, -1.0f, 0.0f ),
		anVec3(  1.0f, -1.0f, 0.0f )
	};

	if ( !self ) {
		return false;
	}

	numLegs = Min( self->spawnArgs.GetInt( "ik_numLegs", "0" ), MAX_LEGS );
	if ( numLegs == 0 ) {
		return true;
	}

	if ( !anIK::Init( self, anim, modelOffset ) ) {
		return false;
	}

	int numJoints = animator->NumJoints();
	anJointMat *joints = ( anJointMat * )_alloca16( numJoints * sizeof( joints[0] ) );

	// create the animation frame used to setup the IK
	gameEdit->ANIM_CreateAnimFrame( animator->ModelHandle(), animator->GetAnim( modifiedAnim )->MD5Anim( 0 ), numJoints, joints, 1, animator->ModelDef()->GetVisualOffset() + modelOffset, animator->RemoveOrigin() );

	enabledLegs = 0;

	// get all the joints
	for ( i = 0; i < numLegs; i++ ) {
		jointName = self->spawnArgs.GetString( va( "ik_foot%d", i+1 ) );
		footJoints[i] = animator->GetJointHandle( jointName );
		if ( footJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "anIK_Walk::Init: invalid foot joint '%s'", jointName );
		}

		jointName = self->spawnArgs.GetString( va( "ik_ankle%d", i+1 ) );
		ankleJoints[i] = animator->GetJointHandle( jointName );
		if ( ankleJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "anIK_Walk::Init: invalid ankle joint '%s'", jointName );
		}

		jointName = self->spawnArgs.GetString( va( "ik_knee%d", i+1 ) );
		kneeJoints[i] = animator->GetJointHandle( jointName );
		if ( kneeJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "anIK_Walk::Init: invalid knee joint '%s'", jointName );
		}

		jointName = self->spawnArgs.GetString( va( "ik_hip%d", i+1 ) );
		hipJoints[i] = animator->GetJointHandle( jointName );
		if ( hipJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "anIK_Walk::Init: invalid hip joint '%s'", jointName );
		}

		jointName = self->spawnArgs.GetString( va( "ik_dir%d", i+1 ) );
		dirJoints[i] = animator->GetJointHandle( jointName );

		enabledLegs |= 1 << i;
	}

	jointName = self->spawnArgs.GetString( "ik_waist" );
	waistJoint = animator->GetJointHandle( jointName );
	if ( waistJoint == INVALID_JOINT ) {
		gameLocal.Error( "anIK_Walk::Init: invalid waist joint '%s'", jointName );
	}

	// get the leg bone lengths and rotation matrices
	for ( i = 0; i < numLegs; i++ ) {
		oldAnkleHeights[i] = 0.0f;

		ankleAxis = joints[ ankleJoints[i] ].ToMat3();
		ankleOrigin = joints[ ankleJoints[i] ].ToVec3();

		kneeAxis = joints[ kneeJoints[i] ].ToMat3();
		kneeOrigin = joints[ kneeJoints[i] ].ToVec3();

		hipAxis = joints[ hipJoints[i] ].ToMat3();
		hipOrigin = joints[ hipJoints[i] ].ToVec3();

		// get the IK direction
		if ( dirJoints[i] != INVALID_JOINT ) {
			dirOrigin = joints[ dirJoints[i] ].ToVec3();
			dir = dirOrigin - kneeOrigin;
		} else {
			dir.Set( 1.0f, 0.0f, 0.0f );
		}

		hipForward[i] = dir * hipAxis.Transpose();
		kneeForward[i] = dir * kneeAxis.Transpose();

		// conversion from upper leg bone axis to hip joint axis
		upperLegLength[i] = GetBoneAxis( hipOrigin, kneeOrigin, dir, axis );
		upperLegToHipJoint[i] = hipAxis * axis.Transpose();

		// conversion from lower leg bone axis to knee joint axis
		lowerLegLength[i] = GetBoneAxis( kneeOrigin, ankleOrigin, dir, axis );
		lowerLegToKneeJoint[i] = kneeAxis * axis.Transpose();
	}

	smoothing = self->spawnArgs.GetFloat( "ik_smoothing", "0.75" );
	waistSmoothing = self->spawnArgs.GetFloat( "ik_waistSmoothing", "0.75" );
	footShift = self->spawnArgs.GetFloat( "ik_footShift", "0" );
	waistShift = self->spawnArgs.GetFloat( "ik_waistShift", "0" );
	minWaistFloorDist = self->spawnArgs.GetFloat( "ik_minWaistFloorDist", "0" );
	minWaistAnkleDist = self->spawnArgs.GetFloat( "ik_minWaistAnkleDist", "0" );
	footUpTrace = self->spawnArgs.GetFloat( "ik_footUpTrace", "32" );
	footDownTrace = self->spawnArgs.GetFloat( "ik_footDownTrace", "32" );
	tiltWaist = self->spawnArgs.GetBool( "ik_tiltWaist", "0" );
	usePivot = self->spawnArgs.GetBool( "ik_usePivot", "0" );

	// setup a clip model for the feet
	footSize = self->spawnArgs.GetFloat( "ik_footSize", "4" ) * 0.5f;
	if ( footSize > 0.0f ) {
		for ( i = 0; i < 4; i++ ) {
			verts[i] = footWinding[i] * footSize;
		}
		trm.SetupPolygon( verts, 4 );
		footModel = new anClipModel( trm, false );
	}

	initialized = true;

	return true;
}

/*
================
anIK_Walk::Evaluate
================
*/
bool anIK_Walk::Evaluate( void ) {
	int i, newPivotFoot;
	float modelHeight, jointHeight, lowestHeight, floorHeights[MAX_LEGS];
	float shift, smallestShift, newHeight, step, newPivotYaw, height, largestAnkleHeight;
	anVec3 modelOrigin, normal, hipDir, kneeDir, start, end, jointOrigins[MAX_LEGS];
	anVec3 footOrigin, ankleOrigin, kneeOrigin, hipOrigin, waistOrigin;
	anMat3 modelAxis, waistAxis, axis;
	anMat3 hipAxis[MAX_LEGS], kneeAxis[MAX_LEGS], ankleAxis[MAX_LEGS];
	trace_t results;

	if ( !self || !gameLocal.isNewFrame ) {
		return false;
	}

	// if no IK enabled on any legs
	if ( !enabledLegs ) {
		return false;
	}

	normal = - self->GetPhysics()->GetGravityNormal();
	modelOrigin = self->GetPhysics()->GetOrigin();
	modelAxis = self->GetRenderEntity()->axis;
	modelHeight = modelOrigin * normal;

	modelOrigin += modelOffset * modelAxis;

	// create frame without joint mods
	animator->CreateFrame( gameLocal.time, false );

	// get the joint positions for the feet
	lowestHeight = anMath::INFINITY;
	for ( i = 0; i < numLegs; i++ ) {
		animator->GetJointTransform( footJoints[i], gameLocal.time, footOrigin, axis );
		jointOrigins[i] = modelOrigin + footOrigin * modelAxis;
		jointHeight = jointOrigins[i] * normal;
		if ( jointHeight < lowestHeight ) {
			lowestHeight = jointHeight;
			newPivotFoot = i;
		}
	}

	if ( usePivot ) {
		newPivotYaw = modelAxis[0].ToYaw();
		// change pivot foot
		if ( newPivotFoot != pivotFoot || anMath::Fabs( anMath::AngleNormalize180( newPivotYaw - pivotYaw ) ) > 30.0f ) {
			pivotFoot = newPivotFoot;
			pivotYaw = newPivotYaw;
			animator->GetJointTransform( footJoints[pivotFoot], gameLocal.time, footOrigin, axis );
			pivotPos = modelOrigin + footOrigin * modelAxis;
		}

		// keep pivot foot in place
		jointOrigins[pivotFoot] = pivotPos;
	}

	// get the floor heights for the feet
	for ( i = 0; i < numLegs; i++ ) {
		if ( !( enabledLegs & ( 1 << i ) ) ) {
			continue;
		}

		start = jointOrigins[i] + normal * footUpTrace;
		end = jointOrigins[i] - normal * footDownTrace;
		gameLocal.clip.Translation( CLIP_DEBUG_PARMS results, start, end, footModel, mat3_identity, CONTENTS_SOLID|CONTENTS_IKCLIP, self );
		floorHeights[i] = results.endpos * normal;

		if ( ik_debug.GetBool() && footModel ) {
			anFixedWinding w;
			for ( int j = 0; j < footModel->GetTraceModel()->numVerts; j++ ) {
				w += footModel->GetTraceModel()->verts[j];
			}
			gameRenderWorld->DebugWinding( colorRed, w, results.endpos, results.endAxis );
		}
	}

	const anPhysics *phys = self->GetPhysics();

	// test whether or not the character standing on the ground
	bool onGround = phys->HasGroundContacts();

	// test whether or not the character is standing on a plat
	bool onPlat = false;
	for ( i = 0; i < phys->GetNumContacts(); i++ ) {
		anEntity *ent = gameLocal.entities[ phys->GetContact( i ).entityNum ];
		if ( ent != nullptr && ent->IsType( idPlat::Type ) ) {
			onPlat = true;
			break;
		}
	}

	// adjust heights of the ankles
	smallestShift = anMath::INFINITY;
	largestAnkleHeight = -anMath::INFINITY;
	for ( i = 0; i < numLegs; i++ ) {
		if ( onGround && ( enabledLegs & ( 1 << i ) ) ) {
			shift = floorHeights[i] - modelHeight + footShift;
		} else {
			shift = 0.0f;
		}

		if ( shift < smallestShift ) {
			smallestShift = shift;
		}

		animator->GetJointTransform( ankleJoints[i], gameLocal.time, ankleOrigin, ankleAxis[i] );
		jointOrigins[i] = modelOrigin + ankleOrigin * modelAxis;

		height = jointOrigins[i] * normal;

		if ( oldHeightsValid && !onPlat ) {
			step = height + shift - oldAnkleHeights[i];
			shift -= smoothing * step;
		}

		newHeight = height + shift;
		if ( newHeight > largestAnkleHeight ) {
			largestAnkleHeight = newHeight;
		}

		oldAnkleHeights[i] = newHeight;

		jointOrigins[i] += shift * normal;
	}

	animator->GetJointTransform( waistJoint, gameLocal.time, waistOrigin, waistAxis );
	waistOrigin = modelOrigin + waistOrigin * modelAxis;

	// adjust position of the waist
	waistOffset = ( smallestShift + waistShift ) * normal;

	// if the waist should be at least a certain distance above the floor
	if ( minWaistFloorDist > 0.0f && waistOffset * normal < 0.0f ) {
		start = waistOrigin;
		end = waistOrigin + waistOffset - normal * minWaistFloorDist;
		gameLocal.clip.Translation( CLIP_DEBUG_PARMS results, start, end, footModel, modelAxis, CONTENTS_SOLID|CONTENTS_IKCLIP, self );
		height = ( waistOrigin + waistOffset - results.endpos ) * normal;
		if ( height < minWaistFloorDist ) {
			waistOffset += ( minWaistFloorDist - height ) * normal;
		}
	}

	// if the waist should be at least a certain distance above the ankles
	if ( minWaistAnkleDist > 0.0f ) {
		height = ( waistOrigin + waistOffset ) * normal;
		if ( height - largestAnkleHeight < minWaistAnkleDist ) {
			waistOffset += ( minWaistAnkleDist - ( height - largestAnkleHeight ) ) * normal;
		}
	}

	if ( oldHeightsValid ) {
		// smoothly adjust height of waist
		newHeight = ( waistOrigin + waistOffset ) * normal;
		step = newHeight - oldWaistHeight;
		waistOffset -= waistSmoothing * step * normal;
	}

	// save height of waist for smoothing
	oldWaistHeight = ( waistOrigin + waistOffset ) * normal;

	if ( !oldHeightsValid ) {
		oldHeightsValid = true;
		return false;
	}

	// solve IK
	for ( i = 0; i < numLegs; i++ ) {
		// get the position of the hip in world space
		animator->GetJointTransform( hipJoints[i], gameLocal.time, hipOrigin, axis );
		hipOrigin = modelOrigin + waistOffset + hipOrigin * modelAxis;
		hipDir = hipForward[i] * axis * modelAxis;

		// get the IK bend direction
		animator->GetJointTransform( kneeJoints[i], gameLocal.time, kneeOrigin, axis );
		kneeDir = kneeForward[i] * axis * modelAxis;

		// solve IK and calculate knee position
		SolveTwoBones( hipOrigin, jointOrigins[i], kneeDir, upperLegLength[i], lowerLegLength[i], kneeOrigin );

		if ( ik_debug.GetBool() ) {
			gameRenderWorld->DebugLine( colorCyan, hipOrigin, kneeOrigin );
			gameRenderWorld->DebugLine( colorRed, kneeOrigin, jointOrigins[i] );
			gameRenderWorld->DebugLine( colorYellow, kneeOrigin, kneeOrigin + hipDir );
			gameRenderWorld->DebugLine( colorGreen, kneeOrigin, kneeOrigin + kneeDir );
		}

		// get the axis for the hip joint
		GetBoneAxis( hipOrigin, kneeOrigin, hipDir, axis );
		hipAxis[i] = upperLegToHipJoint[i] * ( axis * modelAxis.Transpose() );

		// get the axis for the knee joint
		GetBoneAxis( kneeOrigin, jointOrigins[i], kneeDir, axis );
		kneeAxis[i] = lowerLegToKneeJoint[i] * ( axis * modelAxis.Transpose() );
	}

	// set the joint mods
	animator->SetJointAxis( waistJoint, JOINTMOD_WORLD_OVERRIDE, waistAxis );
	animator->SetJointPos( waistJoint, JOINTMOD_WORLD_OVERRIDE, ( waistOrigin + waistOffset - modelOrigin ) * modelAxis.Transpose() );
	for ( i = 0; i < numLegs; i++ ) {
		animator->SetJointAxis( hipJoints[i], JOINTMOD_WORLD_OVERRIDE, hipAxis[i] );
		animator->SetJointAxis( kneeJoints[i], JOINTMOD_WORLD_OVERRIDE, kneeAxis[i] );
		animator->SetJointAxis( ankleJoints[i], JOINTMOD_WORLD_OVERRIDE, ankleAxis[i] );
	}

	ik_activate = true;

	return true;
}

/*
================
anIK_Walk::ClearJointMods
================
*/
void anIK_Walk::ClearJointMods( void ) {
	if ( !self || !ik_activate ) {
		return;
	}

	animator->SetJointAxis( waistJoint, JOINTMOD_NONE, mat3_identity );
	animator->SetJointPos( waistJoint, JOINTMOD_NONE, vec3_origin );
	for ( int i = 0; i < numLegs; i++ ) {
		animator->SetJointAxis( hipJoints[i], JOINTMOD_NONE, mat3_identity );
		animator->SetJointAxis( kneeJoints[i], JOINTMOD_NONE, mat3_identity );
		animator->SetJointAxis( ankleJoints[i], JOINTMOD_NONE, mat3_identity );
	}

	ik_activate = false;
}

/*
================
anIK_Walk::EnableAll
================
*/
void anIK_Walk::EnableAll( void ) {
	enabledLegs = ( 1 << numLegs ) - 1;
	oldHeightsValid = false;
}

/*
================
anIK_Walk::DisableAll
================
*/
void anIK_Walk::DisableAll( void ) {
	enabledLegs = 0;
	oldHeightsValid = false;
}

/*
================
anIK_Walk::EnableLeg
================
*/
void anIK_Walk::EnableLeg( int num ) {
	enabledLegs |= 1 << num;
}

/*
================
anIK_Walk::DisableLeg
================
*/
void anIK_Walk::DisableLeg( int num ) {
	enabledLegs &= ~( 1 << num );
}


/*
===============================================================================

  anIK_Reach

===============================================================================
*/

CLASS_DECLARATION( anIK, anIK_Reach )
END_CLASS

/*
================
anIK_Reach::anIK_Reach
================
*/
anIK_Reach::anIK_Reach() {
	int i;

	initialized = false;
	numArms = 0;
	enabledArms = 0;
	for ( i = 0; i < MAX_ARMS; i++ ) {
		handJoints[i] = INVALID_JOINT;
		elbowJoints[i] = INVALID_JOINT;
		shoulderJoints[i] = INVALID_JOINT;
		dirJoints[i] = INVALID_JOINT;
		shoulderForward[i].Zero();
		elbowForward[i].Zero();
		upperArmLength[i] = 0.0f;
		lowerArmLength[i] = 0.0f;
		upperArmToShoulderJoint[i].Identity();
		lowerArmToElbowJoint[i].Identity();
	}
}

/*
================
anIK_Reach::~anIK_Reach
================
*/
anIK_Reach::~anIK_Reach() {
}

/*
================
anIK_Reach::Init
================
*/
bool anIK_Reach::Init( anEntity *self, const char *anim, const anVec3 &modelOffset ) {
	int i;
	const char *jointName;
	anTraceModel trm;
	anVec3 dir, handOrigin, elbowOrigin, shoulderOrigin, dirOrigin;
	anMat3 axis, handAxis, elbowAxis, shoulderAxis;

	if ( !self ) {
		return false;
	}

	numArms = Min( self->spawnArgs.GetInt( "ik_numArms", "0" ), MAX_ARMS );
	if ( numArms == 0 ) {
		return true;
	}

	if ( !anIK::Init( self, anim, modelOffset ) ) {
		return false;
	}

	int numJoints = animator->NumJoints();
	anJointMat *joints = ( anJointMat * )_alloca16( numJoints * sizeof( joints[0] ) );

	// create the animation frame used to setup the IK
	gameEdit->ANIM_CreateAnimFrame( animator->ModelHandle(), animator->GetAnim( modifiedAnim )->MD5Anim( 0 ), numJoints, joints, 1, animator->ModelDef()->GetVisualOffset() + modelOffset, animator->RemoveOrigin() );

	enabledArms = 0;

	// get all the joints
	for ( i = 0; i < numArms; i++ ) {
		jointName = self->spawnArgs.GetString( va( "ik_hand%d", i+1 ) );
		handJoints[i] = animator->GetJointHandle( jointName );
		if ( handJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "anIK_Reach::Init: invalid hand joint '%s'", jointName );
		}

		jointName = self->spawnArgs.GetString( va( "ik_elbow%d", i+1 ) );
		elbowJoints[i] = animator->GetJointHandle( jointName );
		if ( elbowJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "anIK_Reach::Init: invalid elbow joint '%s'", jointName );
		}

		jointName = self->spawnArgs.GetString( va( "ik_shoulder%d", i+1 ) );
		shoulderJoints[i] = animator->GetJointHandle( jointName );
		if ( shoulderJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "anIK_Reach::Init: invalid shoulder joint '%s'", jointName );
		}

		jointName = self->spawnArgs.GetString( va( "ik_elbowDir%d", i+1 ) );
		dirJoints[i] = animator->GetJointHandle( jointName );

		enabledArms |= 1 << i;
	}

	// get the arm bone lengths and rotation matrices
	for ( i = 0; i < numArms; i++ ) {
		handAxis = joints[ handJoints[i] ].ToMat3();
		handOrigin = joints[ handJoints[i] ].ToVec3();

		elbowAxis = joints[ elbowJoints[i] ].ToMat3();
		elbowOrigin = joints[ elbowJoints[i] ].ToVec3();

		shoulderAxis = joints[ shoulderJoints[i] ].ToMat3();
		shoulderOrigin = joints[ shoulderJoints[i] ].ToVec3();

		// get the IK direction
		if ( dirJoints[i] != INVALID_JOINT ) {
			dirOrigin = joints[ dirJoints[i] ].ToVec3();
			dir = dirOrigin - elbowOrigin;
		} else {
			dir.Set( -1.0f, 0.0f, 0.0f );
		}

		shoulderForward[i] = dir * shoulderAxis.Transpose();
		elbowForward[i] = dir * elbowAxis.Transpose();

		// conversion from upper arm bone axis to should joint axis
		upperArmLength[i] = GetBoneAxis( shoulderOrigin, elbowOrigin, dir, axis );
		upperArmToShoulderJoint[i] = shoulderAxis * axis.Transpose();

		// conversion from lower arm bone axis to elbow joint axis
		lowerArmLength[i] = GetBoneAxis( elbowOrigin, handOrigin, dir, axis );
		lowerArmToElbowJoint[i] = elbowAxis * axis.Transpose();
	}

	initialized = true;

	return true;
}

/*
================
anIK_Reach::Evaluate
================
*/
bool anIK_Reach::Evaluate( void ) {
	return false;
	int i;
	anVec3 modelOrigin, shoulderOrigin, elbowOrigin, handOrigin, shoulderDir, elbowDir;
	anMat3 modelAxis, axis;
	anMat3 shoulderAxis[MAX_ARMS], elbowAxis[MAX_ARMS];
	trace_t trace;

	modelOrigin = self->GetRenderEntity()->origin;
	modelAxis = self->GetRenderEntity()->axis;

	// solve IK
	for ( i = 0; i < numArms; i++ ) {
		// get the position of the shoulder in world space
		animator->GetJointTransform( shoulderJoints[i], gameLocal.time, shoulderOrigin, axis );
		shoulderOrigin = modelOrigin + shoulderOrigin * modelAxis;
		shoulderDir = shoulderForward[i] * axis * modelAxis;

		// get the position of the hand in world space
		animator->GetJointTransform( handJoints[i], gameLocal.time, handOrigin, axis );
		handOrigin = modelOrigin + handOrigin * modelAxis;

		// get first collision going from shoulder to hand
		gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, shoulderOrigin, handOrigin, CONTENTS_SOLID, self );
		handOrigin = trace.endpos;

		// get the IK bend direction
		animator->GetJointTransform( elbowJoints[i], gameLocal.time, elbowOrigin, axis );
		elbowDir = elbowForward[i] * axis * modelAxis;

		// solve IK and calculate elbow position
		SolveTwoBones( shoulderOrigin, handOrigin, elbowDir, upperArmLength[i], lowerArmLength[i], elbowOrigin );

		if ( ik_debug.GetBool() ) {
			gameRenderWorld->DebugLine( colorCyan, shoulderOrigin, elbowOrigin );
			gameRenderWorld->DebugLine( colorRed, elbowOrigin, handOrigin );
			gameRenderWorld->DebugLine( colorYellow, elbowOrigin, elbowOrigin + elbowDir );
			gameRenderWorld->DebugLine( colorGreen, elbowOrigin, elbowOrigin + shoulderDir );
		}

		// get the axis for the shoulder joint
		GetBoneAxis( shoulderOrigin, elbowOrigin, shoulderDir, axis );
		shoulderAxis[i] = upperArmToShoulderJoint[i] * ( axis * modelAxis.Transpose() );

		// get the axis for the elbow joint
		GetBoneAxis( elbowOrigin, handOrigin, elbowDir, axis );
		elbowAxis[i] = lowerArmToElbowJoint[i] * ( axis * modelAxis.Transpose() );
	}

	for ( i = 0; i < numArms; i++ ) {
		animator->SetJointAxis( shoulderJoints[i], JOINTMOD_WORLD_OVERRIDE, shoulderAxis[i] );
		animator->SetJointAxis( elbowJoints[i], JOINTMOD_WORLD_OVERRIDE, elbowAxis[i] );
	}

	ik_activate = true;
}

/*
================
anIK_Reach::ClearJointMods
================
*/
void anIK_Reach::ClearJointMods( void ) {
	if ( !self || !ik_activate ) {
		return;
	}

	for ( int i = 0; i < numArms; i++ ) {
		animator->SetJointAxis( shoulderJoints[i], JOINTMOD_NONE, mat3_identity );
		animator->SetJointAxis( elbowJoints[i], JOINTMOD_NONE, mat3_identity );
		animator->SetJointAxis( handJoints[i], JOINTMOD_NONE, mat3_identity );
	}

	ik_activate = false;
}


/*
===============================================================================

	anIK_Aim

===============================================================================
*/

CLASS_DECLARATION( anIK, anIK_Aim )
END_CLASS

/*
================
anIK_Aim::anIK_Aim
================
*/
anIK_Aim::anIK_Aim( void ) {
}

/*
================
anIK_Aim::~anIK_Aim
================
*/
anIK_Aim::~anIK_Aim( void ) {
}

/*
================
anIK_Aim::Init
================
*/
bool anIK_Aim::Init( anEntity *self, const char *anim, const anVec3 &modelOffset ) {
	if ( !anIK::Init( self, anim, modelOffset ) ) {
		return false;
	}

	int count = self->spawnArgs.GetInt( "ik_numsets", "0" );

	int i;
	for ( i = 0; i < count; i++ ) {
		const char *jointname1		= self->spawnArgs.GetString( va( "ik_set%i_joint1", i ) );
		const char *jointname2		= self->spawnArgs.GetString( va( "ik_set%i_joint2", i ) );

		jointGroup_t& group = jointGroups.Alloc();

		group.joint1 = self->GetAnimator()->GetJointHandle( jointname1 );
		group.joint2 = self->GetAnimator()->GetJointHandle( jointname2 );
	}

	initialized = true;

	return true;
}

extern anCVar r_debugAxisLength;

/*
================
anIK_Aim::Evaluate
================
*/
bool anIK_Aim::Evaluate( void ) {
	for ( int i = 0; i < jointGroups.Num(); i++ ) {
		jointGroup_t &group = jointGroups[i];

		anVec3 org1, org2;

		self->GetAnimator()->GetJointTransform( group.joint1, gameLocal.time, org1 );
		self->GetAnimator()->GetJointTransform( group.joint2, gameLocal.time, org2 );

		anVec3 dir = org2 - org1;
		dir.Normalize();

		float dot = dir * group.lastDir;

		if ( dot < ( 1.f - 1e-4f ) ) {
			break;
		}
	}

	if ( i == jointGroups.Num() ) {
		return false;
	}

	ClearJointMods();

	for ( i = 0; i < jointGroups.Num(); i++ ) {
		jointGroup_t &group = jointGroups[i];

		anVec3 org1, org2;
		anMat3 axis1, axis2;

		self->GetAnimator()->GetJointTransform( group.joint1, gameLocal.time, org1, axis1 );
		self->GetAnimator()->GetJointTransform( group.joint2, gameLocal.time, org2, axis2 );

		anMat3 axes1, axes2;

		group.lastDir = org2 - org1;
		group.lastDir.Normalize();


		axes1[0] = org2 - org1;
		axes1[0] *= axis1.Transpose();
		axes1[0].Normalize();

		axes1[2].Set( 0.f, 0.f, 1.f );
		axes1[2] -= ( axes1[0] * axes1[2] ) * axes1[0];
		axes1[2].Normalize();

		axes1[1] = axes1[2].Cross( axes1[0] );
		axes2[0] = org1 - org2;
		axes2[0] *= axis2.Transpose();
		axes2[0].Normalize();

		axes2[2].Set( 0.f, 0.f, 1.f );
		axes2[2] -= ( axes2[0] * axes2[2] ) * axes2[0];
		axes2[2].Normalize();

		axes2[1] = axes2[2].Cross( axes2[0] );

		self->GetAnimator()->SetJointAxis( group.joint1, JOINTMOD_LOCAL, axes1 );
		self->GetAnimator()->SetJointAxis( group.joint2, JOINTMOD_LOCAL, axes2 );
	}

	return true;
}

/*
================
anIK_Aim::ClearJointMods
================
*/
void anIK_Aim::ClearJointMods( void ) {
	if ( !self ) {
		return;
	}

	for ( int i = 0; i < jointGroups.Num(); i++ ) {
		jointGroup_t& group = jointGroups[i];

		self->GetAnimator()->SetJointAxis( group.joint1, JOINTMOD_NONE, mat3_identity );
		self->GetAnimator()->SetJointAxis( group.joint2, JOINTMOD_NONE, mat3_identity );
	}
}

/*
===============================================================================

	anIK_WheeledVehicle

===============================================================================
*/

CLASS_DECLARATION( idIK, anIK_WheeledVehicle )
END_CLASS

/*
================
anIK_WheeledVehicle::anIK_WheeledVehicle
================
*/
anIK_WheeledVehicle::anIK_WheeledVehicle( void ) {
}

/*
================
anIK_WheeledVehicle::~anIK_WheeledVehicle
================
*/
anIK_WheeledVehicle::~anIK_WheeledVehicle( void ) {
}

/*
================
anIK_WheeledVehicle::AddWheel
================
*/
void anIK_WheeledVehicle::AddWheel( anVehicleRigidBodyWheel &wheel ) {
	wheels.Alloc() = &wheel;
}

/*
================
anIK_WheeledVehicle::ClearWheels
================
*/
void anIK_WheeledVehicle::ClearWheels( void ) {
	wheels.Clear();
}

/*
================
anIK_WheeledVehicle::ClearJointMods
================
*/
void anIK_WheeledVehicle::ClearJointMods( void ) {
	if ( !self ) {
		return;
	}

	for ( int i = 0; i < list.Num(); i++ ) {
		jointGroup_t &group = jointGroups[i];

		self->GetAnimator()->SetJointAxis( group.joint1, JOINTMOD_NONE, mat3_identity );
		self->GetAnimator()->SetJointAxis( group.joint2, JOINTMOD_NONE, mat3_identity );
	}
	/*vehicleDriveObjectList_t &list = rbParent->GetDriveObjects();
	for ( int i = 0; i < list.Num(); i++ ) {
		sdVehicleDriveObject *object = list[i];
		object->ClearSuspensionIK();
	}*/
}

/*
================
anIK_WheeledVehicle::Evaluate
================
*/
bool anIK_WheeledVehicle::Evaluate( void ) {
	anAnimator *animator = self->GetAnimator();
	bool changed = false;

	vehicleDriveObjectList_t &list = rbParent->GetDriveObjects();

	for ( int i = 0; i < list.Num(); i++ ) {
		sdVehicleDriveObject *object = list[ i ];
		changed |= object->UpdateSuspensionIK();
	}

	for ( int i = 0; i < wheels.Num(); i++ ) {
		anVehicleRigidBodyWheel *wheel = wheels[ i ];
		if ( !wheel->HasVisualStateChanged() ) {
			continue;
		}
		wheel->ResetVisualState();

		const anMat3 &frictionAxes = wheel->GetFrictionAxes();
		const anMat3 &baseAxes = wheel->GetBaseAxes();

		anRotation rotation;
		rotation.SetVec( wheel->GetRotationAxis() );
		rotation.SetAngle( wheel->GetWheelAngle() );

		animator->SetJointAxis( wheel->GetWheelJoint(), JOINTMOD_WORLD_OVERRIDE, baseAxes * rotation.ToMat3() * frictionAxes );

		changed = true;
	}

	return changed;
}

/*
================
anIK_WheeledVehicle::Init
================
*/
bool anIK_WheeledVehicle::Init( anTransport_RB *self, const char *anim, const anVec3 &modelOffset ) {
	rbParent = self;

	if ( !idIK::Init( self, anim, modelOffset ) ) {
		return false;
	}

	initialized = true;

	return true;
}

/*
===============================================================================

	anVehicleIKSystem

===============================================================================
*/

ABSTRACT_DECLARATION( idClass, anVehicleIKSystem )
END_CLASS

/*
================
anVehicleIKSystem::Setup
================
*/
bool anVehicleIKSystem::Setup( anTransport *vhcle, const angleClamp_t &yaw, const angleClamp_t &pitch, const anDict &ikParms ) {
	vehicle = vhcle;

	clampYaw = yaw;
	clampPitch = pitch;

	const char* weaponName = ikParms.GetString( "weapon" );
	if ( *weaponName ) {
		weapon = vhcle->GetWeapon( weaponName );
		if ( !weapon ) {
			gameLocal.Warning( "anVehicleIKSystem::Setup Invalid Weapon '%s'", weaponName );
			return false;
		}
	} else {
		weapon = nullptr;
	}
	return true;
}

/*
================
anVehicleIKSystem::GetPlayer
================
*/
anBasePlayer *anVehicleIKSystem::GetPlayer( void ) {	
	return weapon ? weapon->GetPlayer() : position->GetPlayer();
}

/*
===============================================================================

	anPlayerVehicleArmsIK

===============================================================================
*/

CLASS_DECLARATION( anVehicleIKSystem, anPlayerVehicleArmsIK )
END_CLASS

/*
================
anPlayerVehicleArmsIK::Setup
================
*/
bool anPlayerVehicleArmsIK::Setup( anTransport *vhcle, const angleClamp_t &yaw, const angleClamp_t &pitch, const anDict &ikParms ) {
	for ( int i = 0; i < ARM_JOINT_NUM_JOINTS; i++ ) {
		ikJoints[ i ] = INVALID_JOINT;
	}

	if ( !anVehicleIKSystem::Setup( vhcle, yaw, pitch, ikParms ) ) {
		return false;
	}

	yawSound = nullptr;
	if ( clampYaw.sound != nullptr ) {
		yawSound = vehicle->GetMotorSounds().Alloc();
		yawSound->Start( clampYaw.sound );
	}

	pitchSound = nullptr;
	if ( clampPitch.sound != nullptr ) {
		pitchSound = vehicle->GetMotorSounds().Alloc();
		pitchSound->Start( clampPitch.sound );
	}

	jointAngles.Zero();

	anAnimator *animator = vehicle->GetAnimator();
	const anDict &dict = ikParms;

	const char* joint;


	joint = dict.GetString( "jointWrist" );
	if ( *joint ) {
		ikJoints[ ARM_JOINT_INDEX_WRIST ] = animator->GetJointHandle( joint );
	}

	joint = dict.GetString( "jointMuzzle" );
	if ( *joint ) {
		ikJoints[ ARM_JOINT_INDEX_MUZZLE ] = animator->GetJointHandle( joint );
	}

	joint = dict.GetString( "jointElbow" );
	if ( *joint ) {
		ikJoints[ ARM_JOINT_INDEX_ELBOW ] = animator->GetJointHandle( joint );
	}

	ikJoints[ ARM_JOINT_INDEX_SHOULDER ] = animator->GetJointParent( ikJoints[ ARM_JOINT_INDEX_ELBOW ] );

	for ( int i = 0; i < ARM_JOINT_NUM_JOINTS; i++ ) {
		animator->GetJointTransform( ikJoints[ i ], gameLocal.time, baseJointPositions[ i ], baseJointAxes[ i ] );
	}

	pitchAxis = dict.GetInt( "pitchAxis", "2" );	
	requireTophat = dict.GetBool( "require_tophat" );

	oldParentAxis = mat3_identity;

	return true;
}

/*
================
anPlayerVehicleArmsIK::Update
================
*/
void anPlayerVehicleArmsIK::Update( void ) {
	anEntity *vehicleEnt = vehicle;
	if ( !vehicleEnt ) {
		return;
	}

	anVec3 shoulderPos;
	anMat3 temp;

	anAnimator *animator = vehicleEnt->GetAnimator();
	animator->GetJointTransform( ikJoints[ ARM_JOINT_INDEX_SHOULDER ], gameLocal.time, shoulderPos, temp );

	anVec3 shoulderToElbow	= baseJointPositions[ ARM_JOINT_INDEX_ELBOW ] - baseJointPositions[ ARM_JOINT_INDEX_SHOULDER ];
	anVec3 elbowToWrist		= baseJointPositions[ ARM_JOINT_INDEX_WRIST ] - baseJointPositions[ ARM_JOINT_INDEX_ELBOW ];
	anVec3 wristToMuzzle	= baseJointPositions[ ARM_JOINT_INDEX_MUZZLE ] - baseJointPositions[ ARM_JOINT_INDEX_WRIST ];
	anVec3 elbowToMuzzle	= baseJointPositions[ ARM_JOINT_INDEX_MUZZLE ] - baseJointPositions[ ARM_JOINT_INDEX_ELBOW ];

	anMat3 shoulderAxis;
	TransposeMultiply( baseJointAxes[ ARM_JOINT_INDEX_SHOULDER ], temp, shoulderAxis );
	anMat3 transposedShoulderAxis = shoulderAxis.Transpose();

	anBasePlayer* player = GetPlayer();
	if ( player && requireTophat ) {
		if ( !gameLocal.usercmds[ player->entityNumber ].buttons.btn.tophat ) {
			player = nullptr;
		}
	}

	bool changed = false;

	changed |= !oldParentAxis.Compare( temp, 0.005f );

	anAngles newAngles;

	renderView_t* view = player ? player->GetRenderView() : nullptr;
	renderEntity_t* renderEnt = vehicleEnt->GetRenderEntity();

	trace_t trace;
	anVec3 modelTarget;
	if ( view ) {
		anVec3 end = view->vieworg + ( 8192 * view->viewaxis[ 0 ] );
		gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, view->vieworg, end, CONTENTS_SOLID | CONTENTS_OPAQUE, player );

		modelTarget = trace.endpos;

		modelTarget -= renderEnt->origin;
		modelTarget *= renderEnt->axis.Transpose();

		modelTarget -= shoulderPos;
		modelTarget *= transposedShoulderAxis;

		modelTarget -= shoulderToElbow;
	}


	if ( view ) {
		anVec3 target = modelTarget;
		const anVec3 &dir = baseJointAxes[ ARM_JOINT_INDEX_MUZZLE ][ 0 ];
		
		target -= elbowToMuzzle - ( ( dir * elbowToMuzzle ) * dir );
		target *= baseJointAxes[ ARM_JOINT_INDEX_MUZZLE ].Transpose();

		newAngles.yaw = RAD2DEG( atan2( target[ 1 ], target[ 0 ] ) );
	} else {
		newAngles.yaw = 0;
	}

	bool yawChanged = !sdVehiclePosition::ClampAngle( newAngles, jointAngles, clampYaw, 1, 0.1f );
	if ( yawSound != nullptr ) {
		yawSound->Update( yawChanged );
	}
	changed |= yawChanged;

	anMat3 yawMat;
	anAngles::YawToMat3( newAngles.yaw, yawMat );

	if ( view ) {
		anVec3 target = modelTarget;

		anVec3 newElbowToWrist	= elbowToWrist * yawMat;
		anVec3 newWristToMuzzle	= wristToMuzzle * yawMat;

		anMat3 muzzleAxis = baseJointAxes[ ARM_JOINT_INDEX_MUZZLE ] * yawMat;

		target -= newElbowToWrist;
		target -= newWristToMuzzle - ( ( muzzleAxis[ 0 ] * newWristToMuzzle ) * muzzleAxis[ 0 ] );
		target *= muzzleAxis.Transpose();

		newAngles.pitch = -RAD2DEG( atan2( target[ 2 ], target[ 0 ] ) );
	} else {
		newAngles.pitch = 0;
	}

	bool pitchChanged = !sdVehiclePosition::ClampAngle( newAngles, jointAngles, clampPitch,	0, 0.1f );
	if ( pitchSound != nullptr ) {
		pitchSound->Update( pitchChanged );
	}
	changed |= pitchChanged;

	// configurable pitching axis - to support vertically oriented arms (eg badger, bumblebee)
	// as well as horizontally oriented arms (eg goliath)
	int truePitchAxis = pitchAxis;
	if ( truePitchAxis < 0 ) {
		newAngles.pitch = -newAngles.pitch;
		truePitchAxis = -truePitchAxis;
	}

	anAngles pitchAngles( 0.0f, 0.0f, 0.0f );
	if ( truePitchAxis == 1 ) {					// x-axis
		pitchAngles.roll = newAngles.pitch;
	} else if ( truePitchAxis == 3 ) {			// z-axis
		pitchAngles.yaw = newAngles.pitch;
	} else {									// y-axis
		pitchAngles.pitch = newAngles.pitch;
	}

	anMat3 pitchMat = pitchAngles.ToMat3();

	if ( changed ) {
		oldParentAxis = temp;
		jointAngles = newAngles;

		animator->SetJointAxis( ikJoints[ ARM_JOINT_INDEX_ELBOW ], JOINTMOD_WORLD_OVERRIDE, baseJointAxes[ ARM_JOINT_INDEX_ELBOW ] * yawMat * shoulderAxis );
		animator->SetJointAxis( ikJoints[ ARM_JOINT_INDEX_WRIST ], JOINTMOD_WORLD_OVERRIDE, pitchMat * baseJointAxes[ ARM_JOINT_INDEX_WRIST ] * yawMat * shoulderAxis );
	}
}

/*
===============================================================================

	anVehicleSwivel

===============================================================================
*/

CLASS_DECLARATION( anVehicleIKSystem, anVehicleSwivel )
END_CLASS

/*
================
anVehicleSwivel::Setup
================
*/
bool anVehicleSwivel::Setup( anTransport *vhcle, const angleClamp_t &yaw, const angleClamp_t &pitch, const anDict &ikParms ) {
	joint = INVALID_JOINT;

	if ( !anVehicleIKSystem::Setup( vhcle, yaw, pitch, ikParms ) ) {
		return false;
	}

	angles.Zero();

	anAnimator *animator = vehicle->GetAnimator();
	joint = animator->GetJointHandle( ikParms.GetString( "joint" ) );

	animator->GetJointTransform( joint, gameLocal.time, baseAxis );

	yawSound = nullptr;
	if ( clampYaw.sound != nullptr ) {
		yawSound = vehicle->GetMotorSounds().Alloc();
		yawSound->Start( clampYaw.sound );
	}

	return true;
}

/*
================
anVehicleSwivel::Update
================
*/
void anVehicleSwivel::Update( void ) {
	anEntity *vehicleEnt = vehicle;
	if ( !vehicleEnt ) {
		return;
	}

	anAnimator *animator = vehicleEnt->GetAnimator();

	anBasePlayer* player = GetPlayer();
	anAngles newAngles;

	if ( player ) {
		float diff = anMath::AngleDelta( player->clientViewAngles.yaw, angles.yaw );
		newAngles.yaw = angles.yaw + diff * (1.f-clampYaw.filter);//clampYaw.filter * angles.yaw + (1.f - clampYaw.filter) * player->clientViewAngles.yaw;
	} else {
		newAngles.yaw = 0;
	}

	bool changed = !sdVehiclePosition::ClampAngle( newAngles, angles, clampYaw,	1, 0.1f );
	if ( yawSound != nullptr ) {
		yawSound->Update( changed );
	}

	if ( changed ) {
		angles = newAngles;

		anMat3 yawAxis;
		anAngles::YawToMat3( angles.yaw, yawAxis );
		animator->SetJointAxis( joint, JOINTMOD_WORLD, yawAxis );
	}
}

/*
===============================================================================

	anVehicleJointAimer

===============================================================================
*/

CLASS_DECLARATION( anVehicleIKSystem, anVehicleJointAimer )
END_CLASS

/*
================
anVehicleJointAimer::Setup
================
*/
bool anVehicleJointAimer::Setup( anTransport *vhcle, const angleClamp_t &yaw, const angleClamp_t &pitch, const anDict &ikParms ) {
	joint = INVALID_JOINT;

	if ( !anVehicleIKSystem::Setup( vhcle, yaw, pitch, ikParms ) ) {
		return false;
	}

	yawSound = nullptr;
	if ( clampYaw.sound != nullptr ) {
		yawSound = vehicle->GetMotorSounds().Alloc();
		yawSound->Start( clampYaw.sound );
	}

	pitchSound = nullptr;
	if ( clampPitch.sound != nullptr ) {
		pitchSound = vehicle->GetMotorSounds().Alloc();
		pitchSound->Start( clampPitch.sound );
	}

	anAnimator *animator = vehicle->GetAnimator();
	joint = animator->GetJointHandle( ikParms.GetString( "joint" ) );

	if ( joint == INVALID_JOINT ) {
		return false;
	}

	animator->GetJointTransform( joint, gameLocal.time, baseAxis );
	angles = baseAxis.ToAngles();

	const char* weapon2Name = ikParms.GetString( "weapon2" );
	if ( *weapon2Name ) {
		weapon2 = vhcle->GetWeapon( weapon2Name );
		if ( !weapon2 ) {
			gameLocal.Warning( "anVehicleIKSystem::Setup Invalid Weapon '%s'", weapon2Name );
			return false;
		}
	} else {
		weapon2 = nullptr;
	}

	return true;
}

/*
================
anVehicleIKSystem::GetPlayer
================
*/
anBasePlayer *anVehicleJointAimer::GetPlayer( void ) {	
	anBasePlayer *player = weapon ? weapon->GetPlayer() : position->GetPlayer();
	if ( !player ) {
		return weapon2 ? weapon2->GetPlayer() : position->GetPlayer();
	}

	return player;
}

/*
================
anVehicleJointAimer::Update
================
*/
void anVehicleJointAimer::Update( void ) {
	anEntity *vehicleEnt = vehicle;
	if ( !vehicleEnt ) {
		return;
	}

	if ( joint == INVALID_JOINT ) {
		return;
	}

	anAnimator *animator = vehicleEnt->GetAnimator();

	anBasePlayer *player = GetPlayer();
	anMat3 tempJointAxis;
	anVec3 jointPos;
	animator->GetJointTransform( joint, gameLocal.time, jointPos, tempJointAxis );
	renderView_t *view = player ? player->GetRenderView() : nullptr;
	renderEntity_t *renderEnt = vehicleEnt->GetRenderEntity();

	trace_t trace;
	anVec3 modelTarget;
	if ( view ) {
		// find what is being aimed at
		anVec3 end = view->vieworg + ( 8192 * view->viewaxis[ 0 ] );
		gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, view->vieworg, end, CONTENTS_SOLID | CONTENTS_OPAQUE, player );
		modelTarget = trace.endpos;

		// transform modelTarget into entity space
		modelTarget -= renderEnt->origin;
		modelTarget *= renderEnt->axis.Transpose();

		// make target relative to joint
		modelTarget -= jointPos;

		// calculate the vector to the target
		anVec3 direction = modelTarget;
		direction.Normalize();
		anMat3 newAxis = direction.ToMat3();
		anAngles newAngles = newAxis.ToAngles();
		
		// clamp the angles
		
		bool yawChanged = !sdVehiclePosition::ClampAngle( newAngles, angles, clampYaw, 1, 0.1f );
		if ( yawSound != nullptr ) {
			yawSound->Update( yawChanged );
		}

		bool pitchChanged = !sdVehiclePosition::ClampAngle( newAngles, angles, clampPitch, 0, 0.1f );
		if ( pitchSound != nullptr ) {
			pitchSound->Update( pitchChanged );
		}

		if ( yawChanged || pitchChanged ) {
			// set the angles
			angles = newAngles;
			animator->SetJointAxis( joint, JOINTMOD_WORLD_OVERRIDE, newAngles.ToMat3() );
		}
	}
}

/*
===============================================================================

  anVehicleIK_Steering

===============================================================================
*/

CLASS_DECLARATION( anVehicleIKSystem, anVehicleIK_Steering )
END_CLASS

/*
================
anVehicleIK_Steering::anVehicleIK_Steering
================
*/
anVehicleIK_Steering::anVehicleIK_Steering( void ) {
}

/*
================
anVehicleIK_Steering::~anVehicleIK_Steering
================
*/
anVehicleIK_Steering::~anVehicleIK_Steering( void ) {
}

/*
================
anVehicleIK_Steering::Init
================
*/
bool anVehicleIK_Steering::Setup( anTransport *vhcle, const angleClamp_t &yaw, const angleClamp_t &pitch, const anDict &ikParms ) {
	if ( !anVehicleIKSystem::Setup( vhcle, yaw, pitch, ikParms ) ) {
		return false;
	}

	if ( !ik.Init( vehicle, ikParms ) ) {
		return false;
	}

	return true;
}

/*
================
anVehicleIK_Steering::Update
================
*/
void anVehicleIK_Steering::Update( void ) {
	anBasePlayer *player = GetPlayer();
	if ( player == nullptr ) {
		return;
	}

	ik.Update( player, vehicle );
}

/*
===============================================================================

  anVehicleWeaponAimer

===============================================================================
*/

CLASS_DECLARATION( anVehicleIKSystem, anVehicleWeaponAimer )
END_CLASS

/*
================
anVehicleWeaponAimer::Update
================
*/
void anVehicleWeaponAimer::Update( void ) {
	anBasePlayer* player = GetPlayer();
	if ( player ) {
		const renderView_t& renderView = player->renderView;

		trace_t trace;
		gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, renderView.vieworg, renderView.vieworg + ( renderView.viewaxis[ 0 ] * 4096 ), CONTENTS_SOLID | CONTENTS_OPAQUE, player );

		aimer.SetTarget( trace.endpos );
	} else {
		aimer.ClearTarget();
	}

	aimer.Update();
}

/*
================
anVehicleWeaponAimer::Setup
================
*/
bool anVehicleWeaponAimer::Setup( anTransport *vhcle, const angleClamp_t &yaw, const angleClamp_t &pitch, const anDict &ikParms ) {
	if ( !anVehicleIKSystem::Setup( vhcle, yaw, pitch, ikParms ) ) {
		return false;
	}

	anAnimator *animator = vehicle->GetAnimator();

	jointHandle_t pitchJoint	= animator->GetJointHandle( ikParms.GetString( "jointWrist" ) );
	jointHandle_t yawJoint		= animator->GetJointHandle( ikParms.GetString( "jointElbow" ) );
	jointHandle_t muzzleJoint	= animator->GetJointHandle( ikParms.GetString( "jointMuzzle" ) );
	jointHandle_t shoulderJoint = animator->GetJointHandle( ikParms.GetString( "jointShoulder" ) );
	if ( shoulderJoint == INVALID_JOINT ) {
		shoulderJoint = animator->GetJointParent( yawJoint );
	}

	int anim = animator->GetAnim( ikParms.GetString( "deployed_anim" ) );

	aimer.Init( ikParms.GetBool( "fix_barrel" ), ikParms.GetBool( "invert_pitch" ), vhcle, anim, yawJoint, pitchJoint, muzzleJoint, shoulderJoint, clampYaw, clampPitch );

	return true;
}
