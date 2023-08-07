// Copyright (C) 2007 Id Software, Inc.
//


#include "../Lib.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Vehicle_RigidBody.h"
//#include "../../DeclSurfaceType.h"


/*
===============================================================================

	anVehicle_RigidBody

===============================================================================
*/
const idEventDef EV_setDamageDealtScale( "setDamageDealtScale", '\0', DOC_TEXT( "Sets the scale factor applied to damage applied when this vehicle collides into something." ), 1, NULL, "f", "scale", "Scale factor to apply." );

CLASS_DECLARATION( anTransport_RB, anVehicle_RigidBody )
	EVENT( EV_setDamageDealtScale,			anVehicle_RigidBody::Event_SetDamageDealtScale )
END_CLASS

/*
================
anVehicle_RigidBody::anVehicle_RigidBody
================
*/
anVehicle_RigidBody::anVehicle_RigidBody( void ) {
	collideDamage						= NULL;
	collideFatalDamage					= NULL;
	onCollisionFunc						= NULL;
	onCollisionSideScrapeFunc			= NULL;
	nextCollisionTime					= 0;
	collideDamageDealtScale				= 1.0f;
}

/*
================
anVehicle_RigidBody::~anVehicle_RigidBody
================
*/
anVehicle_RigidBody::~anVehicle_RigidBody( void ) {
}

/*
================
anVehicle_RigidBody::DoLoadVehicleScript
================
*/
void anVehicle_RigidBody::DoLoadVehicleScript( void ) {
	if ( !spawnArgs.GetBool( "disableIK" ) ) {
		ik.Init( this, IK_ANIM, vec3_origin );
	}
	ik.ClearWheels();

	physicsObj.ClearClipModels();

	physicsObj.SetFriction( spawnArgs.GetFloat( "linear_friction" ), spawnArgs.GetFloat( "angular_friction" ) );
	physicsObj.SetWaterFriction( spawnArgs.GetFloat( "linear_friction_water" ), spawnArgs.GetFloat( "angular_friction_water" ) );
	physicsObj.SetBouncyness( spawnArgs.GetFloat( "bouncyness" ) );
	physicsObj.SetWaterRestThreshold( spawnArgs.GetFloat( "water_rest_threshold", "1" ) );

	LoadParts( VPT_PART | VPT_WHEEL | VPT_HOVER | VPT_SIMPLE_PART | VPT_SCRIPTED_PART 
			| VPT_MASS | VPT_TRACK | VPT_ROTOR | VPT_THRUSTER | VPT_SUSPENSION | VPT_VTOL 
			| VPT_ANTIGRAV | VPT_PSEUDO_HOVER | VPT_DRAGPLANE | VPT_RUDDER 
			| VPT_AIRBRAKE | VPT_HURTZONE | VPT_ANTIROLL | VPT_ANTIPITCH );

	physicsObj.CalculateMassProperties();
}

/*
================
anVehicle_RigidBody::Spawn
================
*/
void anVehicle_RigidBody::Spawn( void ) {
	const char *damagename = spawnArgs.GetString( "dmg_collide" );
	if ( *damagename ) {
		collideDamage = gameLocal.declDamageType.LocalFind( damagename, false );
		if( !collideDamage ) {
			gameLocal.Warning( "sdVehicle::Spawn Invalid Damage Type '%s'", damagename );
		}
	} else {
		collideDamage = NULL;
	}

	damagename = spawnArgs.GetString( "dmg_collide_fatal" );
	if ( *damagename ) {
		collideFatalDamage = gameLocal.declDamageType.LocalFind( damagename, false );
		if( !collideFatalDamage ) {
			gameLocal.Warning( "sdVehicle::Spawn Invalid Damage Type '%s'", damagename );
		}
	}
	if ( collideFatalDamage == NULL ) {
		collideFatalDamage = collideDamage;
	}


	onCollisionFunc = scriptObject->GetFunction( "OnCollision" );
	onCollisionSideScrapeFunc = scriptObject->GetFunction( "OnCollisionSideScrape" );

	collideDotLimit = spawnArgs.GetFloat( "collide_dot_limit", "-0.5" );

	physicsObj.SetSelf( this );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetFastPath( true );

	SetPhysics( &physicsObj );

	LoadVehicleScript();

	BecomeActive( TH_THINK );

	if( !health ) {
		health = 2000;
	}

	float gravityScale;
	if ( spawnArgs.GetFloat( "gravity", DEFAULT_GRAVITY_STRING, gravityScale ) ) {
		physicsObj.SetGravity( physicsObj.GetGravityNormal() * gravityScale );
	}

	BecomeActive( TH_UPDATEVISUALS );
	Present();

	nextSelfCollisionTime	= gameLocal.time + SEC2MS( 5 ); // 5s spawn invulnerability to allow it to drop to the ground
	nextCollisionTime		= 0;
	nextCollisionSound		= 0;
	nextJumpSound			= 0;

	// NOTE: USE THIS ONLY TO LOCK DOWN THE HANDLING OF A VEHICLE!
	//		 Tweak the mass distrubtion in the vscript, and ONLY use this if you
	//		 need it to prevent collision model modifications upsetting the handling
	anVec3 IDiagonal;
	anVec3 IOther;
	bool hasIDiagonal = spawnArgs.GetVector( "do_not_modify_itd", "1 1 1", IDiagonal );
	bool hasIOther = spawnArgs.GetVector( "do_not_modify_ito", "0 0 0", IOther );

	if ( hasIDiagonal || hasIOther ) {
		if ( !hasIDiagonal ) {
			anMat3 originalITT = physicsObj.GetInertiaTensor();
			IDiagonal[0] = originalITT[0][0];
			IDiagonal[1] = originalITT[1][1];
			IDiagonal[2] = originalITT[2][2];
		}
		if ( !hasIOther ) {
			anMat3 originalITT = physicsObj.GetInertiaTensor();
			IOther[0] = originalITT[0][1];
			IOther[1] = originalITT[0][2];
			IOther[2] = originalITT[1][2];
		}

		anMat3 itt;
		itt[0][0] = IDiagonal[0];
		itt[1][1] = IDiagonal[1];
		itt[2][2] = IDiagonal[2];
		itt[0][1] = itt[1][0] = IOther[0];
		itt[0][2] = itt[2][0] = IOther[1];
		itt[1][2] = itt[2][1] = IOther[2];
		physicsObj.SetInertiaTensor( itt );
	}
}

/*
================
anVehicle_RigidBody::Collide
================
*/
bool anVehicle_RigidBody::Collide( const trace_t &collision, const anVec3 &velocity, int bodyId ) {
	// FIXME: This stuff should be done on collisions against us too.
	anVec3 normal = -collision.c.normal;
	float length = ( velocity * normal );
	anVec3 v = length * normal;

	if ( collideDamage ) {
		anEntity *other = gameLocal.entities[ collision.c.entityNum ];
		anTransport* otherTransport = other->Cast< anTransport >();
		const sdVehicleControlBase* otherControl = NULL;
		if ( otherTransport != NULL ) {
			otherControl = otherTransport->GetVehicleControl();
		}

		partDamageInfo_t damageInfo;	

		sdVehicleDriveObject *object = PartForCollisionById( collision, PFC_SELF_COLLISION );
		if ( object ) {
			damageInfo = *object->GetDamageInfo();
		} else {
			damageInfo.damageScale			= 1.f;
			damageInfo.collisionScale		= 1.f;
			damageInfo.collisionMinSpeed	= 256.f;
			damageInfo.collisionMaxSpeed	= 1024.f;
		}

		if( ( gameLocal.time > nextCollisionTime ) && ( length > damageInfo.collisionMinSpeed ) ) {
			float damage = ( length - damageInfo.collisionMinSpeed ) / ( damageInfo.collisionMaxSpeed - damageInfo.collisionMinSpeed );
			if ( damage > 1.f ) {
				damage = 1.f;
			}

			bool playerCollide = false;
			if ( other->IsType( anBasePlayer ::Type ) ) {
				playerCollide = true;
				if ( collision.c.normal.z > 0.9f ) {
					// landed on their head, should crush them
					damage *= 4.f;
				}
			}

			bool otherIgnoreDamage = otherControl != NULL && otherControl->IgnoreCollisionDamage( -normal );
			if ( other->fl.takedamage && !otherIgnoreDamage ) {
				anBasePlayer *driver = positionManager.FindDriver();
				int oldHealth = other->GetHealth();
				other->Damage( this, driver != NULL ? ( anEntity *)driver : this, v, collideDamage, damage * collideDamageDealtScale, &collision );
				if ( driver != NULL ) {
					if ( oldHealth > 0 && other->GetHealth() <= 0 ) {
						anBasePlayer *otherPlayer = other->Cast<anBasePlayer>();
						}
					}
				}
			}

			bool ignoreDamage = GetVehicleControl() != NULL && GetVehicleControl()->IgnoreCollisionDamage( normal );
			if ( !ignoreDamage && !playerCollide && gameLocal.time > nextSelfCollisionTime ) {
				// HACK: take no collision damage from below if the other thing doesn't take any damage
				//		 this basically makes it so that you don't take annoying damage from scraping terrain
				float underneathNess = normal * GetPhysics()->GetAxis()[ 2 ];
				if ( other->fl.takedamage || underneathNess > collideDotLimit ) {
					if ( vehicleControl && IsCareening() ) {
						damage *= vehicleControl->GetCareeningCollideScale();
					}
					Damage( this, this, -v, collideDamage, damage * damageInfo.collisionScale, &collision );
			}
		}

/*		if( g_debugDamage.GetInteger() ) {
			gameRenderWorld->DebugBox( colorGreen, idBox( collision.endpos, anVec3( 8, 8, 8 ), mat3_identity ), 3000 );
			gameRenderWorld->DebugLine( colorBlue, collision.endpos, collision.endpos + collision.c.normal * length, 3000 );

			gameRenderWorld->DebugBox( colorYellow, idBox( collision.c.point, anVec3( 8, 8, 8 ), mat3_identity ), 3000 );
			gameRenderWorld->DebugLine( colorBlue, collision.c.point, collision.c.point + collision.c.normal * length, 3000 );
		}*/
	}

	HandleCollision( collision, length );

	return anTransport::Collide( collision, velocity, bodyId );
}

/*
================
anVehicle_RigidBody::CollideFatal
================
*/
void anVehicle_RigidBody::CollideFatal( anEntity *other ) {
	anEntity *driver = positionManager.FindDriver();
	anBasePlayer *playerDriver = NULL;
	
	if ( driver != NULL ) {
		playerDriver = driver->Cast< anBasePlayer  >();
		assert( playerDriver != NULL );
	}

	if ( collideFatalDamage != NULL ) {
		if ( other->fl.takedamage ) {
			int oldHealth = other->GetHealth();
			anVec3 dir = GetPhysics()->GetOrigin() - other->GetPhysics()->GetOrigin();
			dir.Normalize();
			other->Damage( this, driver ? driver : this, dir, collideFatalDamage, -1.0f, NULL, true );
			if ( playerDriver != NULL ) {
				anBasePlayer *otherPlayer = other->Cast< anBasePlayer  >();
				if ( otherPlayer != NULL && otherPlayer->GetEntityAllegiance( playerDriver ) == TA_ENEMY ) {
					if ( oldHealth > 0 && otherPlayer->GetHealth() <= 0 ) {
						IncRoadKillStats( playerDriver );
					}
				}
			}
		}
	}
}

/*
================
anVehicle_RigidBody::UpdateAnimationControllers
================
*/
bool anVehicle_RigidBody::UpdateAnimationControllers( void ) {
	if ( !gameLocal.isNewFrame ) {
		return false;
	}

	if ( ik.IsInitialized() ) {
		if ( !ik.IsInhibited() ) {
			return ik.Evaluate();
		}
		return false;
	} else {
		ik.ClearJointMods();
	}

	return false;
}

/*
================
anVehicle_RigidBody::HandleCollision
================
*/
void anVehicle_RigidBody::HandleCollision( const trace_t &collision, const float velocity ) {
	anVec3 bodyOrigin;
	anVec3 localCollisionPoint;

	physicsObj.GetBodyOrigin( bodyOrigin, collision.c.selfId );

	localCollisionPoint = ( collision.c.point - bodyOrigin ) * physicsObj.GetAxis().Transpose();

	// get dot with forward facing direction
	anVec3 localCollisionNormal = collision.c.normal * physicsObj.GetAxis().Transpose();
	anVec3 v( 1, 0, 0 );
	float dot = v * localCollisionNormal;

	const anBounds &bodyBounds = physicsObj.GetBounds( collision.c.selfId );
	const anVec3 bodySize = bodyBounds.Size();

#if 0
	bool foundCollisionPoint = false;

	// sides
	if ( localCollisionPoint.x > 0 && localCollisionPoint.y > 0 ) {
		if ( anMath::Fabs( dot ) < .15f && localCollisionPoint.x > ( .5f * bodySize.x ) - 2.f ) {
			gameLocal.Printf( "Collision with front left side. (dot = %f)\n", anMath::Fabs( dot ) );
/*			if ( g_debugDamage.GetInteger() ) {
				anVec3 collisionCenter;
				collisionCenter.y = ( .5f * bodySize.y ) - 2.f;
				collisionCenter.z = 0.f;

				anVec3 collisionExtents;
				collisionExtents.y = 2.f;
				collisionExtents.z = .5f * bodySize.z;

				if ( anMath::Fabs( dot ) > .06f ) {
					float fraction = 1.f - ( ( anMath::Fabs( dot ) - .06f ) / .09f );

					collisionExtents.x = .0625f * bodySize.x + fraction * .1875f * bodySize.x;
					collisionCenter.x = .5f * bodySize.x - collisionExtents.x;
				} else {
					collisionCenter.x = .25f * bodySize.x;
					collisionExtents.x = .25f * bodySize.x;
				}

				idBox collisionBox( collisionCenter, collisionExtents, mat3_identity );

				// move into world
				collisionBox.RotateSelf( physicsObj.GetAxis() );
				collisionBox.TranslateSelf( bodyOrigin );

				gameRenderWorld->DebugBox( colorRed, collisionBox, 3000 );
			}*/

			anVec3 mins, maxs;
			mins.y = .5f * bodySize.y;
			mins.z = -.5f * bodySize.z;

			maxs.x = .5f * bodySize.x;
			maxs.y = .5f * bodySize.y;
			maxs.z = .5f * bodySize.z;

			if ( anMath::Fabs( dot ) > .06f ) {
				float fraction = ( anMath::Fabs( dot ) - .06f ) / .09f;
				mins.x = fraction * ( .4375f * bodySize.x );
			} else {
				mins.x = 0.f;
			}

			foundCollisionPoint = true;
		} else {
			// just generate a small box around the collision point
/*			if ( g_debugDamage.GetInteger() ) {
				anVec3 collisionCenter;
				collisionCenter.x = localCollisionPoint.x;
				collisionCenter.y = localCollisionPoint.y;
				collisionCenter.z = 0.f;

				anVec3 collisionExtents;
				collisionExtents.x = 2.f;
				collisionExtents.y = 2.f;
				collisionExtents.z = .5f * bodySize.z;

				idBox collisionBox( collisionCenter, collisionExtents, mat3_identity );

				// move into world
				collisionBox.RotateSelf( physicsObj.GetAxis() );
				collisionBox.TranslateSelf( bodyOrigin );

				gameRenderWorld->DebugBox( colorCyan, collisionBox, 3000 );
			}*/

			anVec3 mins, maxs;
			mins.y = -.5f * bodySize.y;
			mins.z = -.5f * bodySize.z;

			maxs.x = .5f * bodySize.x;
			maxs.y = -.5f * bodySize.y;
			maxs.z = .5f * bodySize.z;

			if ( anMath::Fabs( dot ) > .06f ) {
				float fraction = ( anMath::Fabs( dot ) - .06f ) / .09f;
				mins.x = fraction * ( .4375f * bodySize.x );
			} else {
				mins.x = 0.f;
			}

			foundCollisionPoint = true;
		}
	}
	if ( localCollisionPoint.x > 0 && localCollisionPoint.y < 0 ) {
		if ( anMath::Fabs( dot ) < .15f && localCollisionPoint.x > ( .5f * bodySize.x ) - 2.f ) {
			gameLocal.Printf( "Collision with front right side. (dot = %f)\n", anMath::Fabs( dot ) );
/*			if ( g_debugDamage.GetInteger() ) {
				anVec3 collisionCenter;
				collisionCenter.y = ( -.5f * bodySize.y ) + 2.f;
				collisionCenter.z = 0.f;

				anVec3 collisionExtents;
				collisionExtents.y = 2.f;
				collisionExtents.z = .5f * bodySize.z;

				if ( anMath::Fabs( dot ) > .06f ) {
					float fraction = 1.f - ( ( anMath::Fabs( dot ) - .06f ) / .09f );

					collisionExtents.x = .0625f * bodySize.x + fraction * .1875f * bodySize.x;
					collisionCenter.x = .5f * bodySize.x - collisionExtents.x;
				} else {
					collisionCenter.x = .25f * bodySize.x;
					collisionExtents.x = .25f * bodySize.x;
				}

				idBox collisionBox( collisionCenter, collisionExtents, mat3_identity );

				// move into world
				collisionBox.RotateSelf( physicsObj.GetAxis() );
				collisionBox.TranslateSelf( bodyOrigin );

				gameRenderWorld->DebugBox( colorRed, collisionBox, 3000 );
			}*/

			anVec3 mins, maxs;
			mins.x = -.5f * bodySize.x;
			mins.y = .5f * bodySize.y;
			mins.z = -.5f * bodySize.z;

			maxs.y = .5f * bodySize.y;
			maxs.z = .5f * bodySize.z;

			if ( anMath::Fabs( dot ) > .06f ) {
				float fraction = ( anMath::Fabs( dot ) - .06f ) / .09f;
				maxs.x = -fraction * ( .4375f * bodySize.x );
			} else {
				maxs.x = 0.f;
			}

			foundCollisionPoint = true;
		} else {
			// just generate a small box around the collision point
/*			if ( g_debugDamage.GetInteger() ) {
				anVec3 collisionCenter;
				collisionCenter.x = localCollisionPoint.x;
				collisionCenter.y = localCollisionPoint.y;
				collisionCenter.z = 0.f;

				anVec3 collisionExtents;
				collisionExtents.x = 2.f;
				collisionExtents.y = 2.f;
				collisionExtents.z = .5f * bodySize.z;

				idBox collisionBox( collisionCenter, collisionExtents, mat3_identity );

				// move into world
				collisionBox.RotateSelf( physicsObj.GetAxis() );
				collisionBox.TranslateSelf( bodyOrigin );

				gameRenderWorld->DebugBox( colorCyan, collisionBox, 3000 );
			}*/

			foundCollisionPoint = true;
		}
	}
	if ( localCollisionPoint.x < 0 && localCollisionPoint.y > 0 ) {
		if ( anMath::Fabs( dot ) < .15f && localCollisionPoint.x < ( -.5f * bodySize.x ) + 2.f ) {
			gameLocal.Printf( "Collision with rear left side. (dot = %f)\n", anMath::Fabs( dot ) );
/*			if ( g_debugDamage.GetInteger() ) {
				anVec3 collisionCenter;
				collisionCenter.y = ( .5f * bodySize.y ) - 2.f;
				collisionCenter.z = 0.f;

				anVec3 collisionExtents;
				collisionExtents.y = 2.f;
				collisionExtents.z = .5f * bodySize.z;

				if ( anMath::Fabs( dot ) > .06f ) {
					float fraction = 1.f - ( ( anMath::Fabs( dot ) - .06f ) / .09f );

					collisionExtents.x = .0625f * bodySize.x + fraction * .1875f * bodySize.x;
					collisionCenter.x = -.5f * bodySize.x + collisionExtents.x;
				} else {
					collisionCenter.x = -.25f * bodySize.x;
					collisionExtents.x = .25f * bodySize.x;
				}

				idBox collisionBox( collisionCenter, collisionExtents, mat3_identity );

				// move into world
				collisionBox.RotateSelf( physicsObj.GetAxis() );
				collisionBox.TranslateSelf( bodyOrigin );

				gameRenderWorld->DebugBox( colorRed, collisionBox, 3000 );
			}*/

			anVec3 mins, maxs;
			mins.x = -.5f * bodySize.x;
			mins.y = -.5f * bodySize.y;
			mins.z = -.5f * bodySize.z;

			maxs.y = -.5f * bodySize.y;
			maxs.z = .5f * bodySize.z;

			if ( anMath::Fabs( dot ) > .06f ) {
				float fraction = ( anMath::Fabs( dot ) - .06f ) / .09f;
				maxs.x = -fraction * ( .4375f * bodySize.x );
			} else {
				maxs.x = 0.f;
			}

			foundCollisionPoint = true;
		} else {
			// just generate a small box around the collision point
/*			if ( g_debugDamage.GetInteger() ) {
				anVec3 collisionCenter;
				collisionCenter.x = localCollisionPoint.x;
				collisionCenter.y = localCollisionPoint.y;
				collisionCenter.z = 0.f;

				anVec3 collisionExtents;
				collisionExtents.x = 2.f;
				collisionExtents.y = 2.f;
				collisionExtents.z = .5f * bodySize.z;

				idBox collisionBox( collisionCenter, collisionExtents, mat3_identity );

				// move into world
				collisionBox.RotateSelf( physicsObj.GetAxis() );
				collisionBox.TranslateSelf( bodyOrigin );

				gameRenderWorld->DebugBox( colorCyan, collisionBox, 3000 );
			}*/

			foundCollisionPoint = true;
		}
	}
	if ( localCollisionPoint.x < 0 && localCollisionPoint.y < 0 ) {
		if ( anMath::Fabs( dot ) < .15f && localCollisionPoint.x < ( -.5f * bodySize.x ) + 2.f ) {
			gameLocal.Printf( "Collision with rear right side. (dot = %f)\n", anMath::Fabs( dot ) );
/*			if ( g_debugDamage.GetInteger() ) {
				anVec3 collisionCenter;
				collisionCenter.y = ( -.5f * bodySize.y ) + 2.f;
				collisionCenter.z = 0.f;

				anVec3 collisionExtents;
				collisionExtents.y = 2.f;
				collisionExtents.z = .5f * bodySize.z;

				if ( anMath::Fabs( dot ) > .06f ) {
					float fraction = 1.f - ( ( anMath::Fabs( dot ) - .06f ) / .09f );

					collisionExtents.x = .0625f * bodySize.x + fraction * .1875f * bodySize.x;
					collisionCenter.x = -.5f * bodySize.x + collisionExtents.x;
				} else {
					collisionCenter.x = -.25f * bodySize.x;
					collisionExtents.x = .25f * bodySize.x;
				}

				idBox collisionBox( collisionCenter, collisionExtents, mat3_identity );

				// move into world
				collisionBox.RotateSelf( physicsObj.GetAxis() );
				collisionBox.TranslateSelf( bodyOrigin );

				gameRenderWorld->DebugBox( colorRed, collisionBox, 3000 );
			}*/
			foundCollisionPoint = true;
		} else {
			// just generate a small box around the collision point
/*			if ( g_debugDamage.GetInteger() ) {
				anVec3 collisionCenter;
				collisionCenter.x = localCollisionPoint.x;
				collisionCenter.y = localCollisionPoint.y;
				collisionCenter.z = 0.f;

				anVec3 collisionExtents;
				collisionExtents.x = 2.f;
				collisionExtents.y = 2.f;
				collisionExtents.z = .5f * bodySize.z;

				idBox collisionBox( collisionCenter, collisionExtents, mat3_identity );

				// move into world
				collisionBox.RotateSelf( physicsObj.GetAxis() );
				collisionBox.TranslateSelf( bodyOrigin );

				gameRenderWorld->DebugBox( colorCyan, collisionBox, 3000 );
			}*/
			foundCollisionPoint = true;
		}
	}
	// top
	if ( !foundCollisionPoint && localCollisionPoint.z > ( .5f * bodySize.z ) - 2.f ) {
		// just generate a small box around the collision point
/*		if ( g_debugDamage.GetInteger() ) {
			anVec3 collisionCenter;
			collisionCenter.x = localCollisionPoint.x;
			collisionCenter.y = localCollisionPoint.y;
			collisionCenter.z = localCollisionPoint.z;

			anVec3 collisionExtents;
			collisionExtents.x = 2.f;
			collisionExtents.y = 2.f;
			collisionExtents.z = localCollisionPoint.z;

			idBox collisionBox( collisionCenter, collisionExtents, mat3_identity );

			// move into world
			collisionBox.RotateSelf( physicsObj.GetAxis() );
			collisionBox.TranslateSelf( bodyOrigin );

			gameRenderWorld->DebugBox( colorCyan, collisionBox, 3000 );
		}*/

		foundCollisionPoint = true;
	}

	// bottom
	if ( !foundCollisionPoint && localCollisionPoint.z < ( -.5f * bodySize.z ) + 2.f ) {
		// just generate a small box around the collision point
/*		if ( g_debugDamage.GetInteger() ) {
			anVec3 collisionCenter;
			collisionCenter.x = localCollisionPoint.x;
			collisionCenter.y = localCollisionPoint.y;
			collisionCenter.z = localCollisionPoint.z;

			anVec3 collisionExtents;
			collisionExtents.x = 2.f;
			collisionExtents.y = 2.f;
			collisionExtents.z = localCollisionPoint.z;

			idBox collisionBox( collisionCenter, collisionExtents, mat3_identity );

			// move into world
			collisionBox.RotateSelf( physicsObj.GetAxis() );
			collisionBox.TranslateSelf( bodyOrigin );

			gameRenderWorld->DebugBox( colorCyan, collisionBox, 3000 );
		}*/

		foundCollisionPoint = true;
	}
#endif

	bool scrape = false;
	anVec3 mins, maxs;

	// scraping
	bool front;

	if ( localCollisionPoint.x > 0 ) {
		front = true;

		if ( anMath::Fabs( dot ) < .15f && localCollisionPoint.x > ( .5f * bodySize.x ) - 2.f ) {
			scrape = true;
		}
	} else {
		front = false;
		if ( anMath::Fabs( dot ) < .15f && localCollisionPoint.x < ( -.5f * bodySize.x ) + 2.f ) {
			scrape = true;
		}
	}

	if ( scrape ) {
		mins.y = maxs.y = ( localCollisionPoint.y > 0 ? 1 : -1 ) * .5f * bodySize.y;
		if ( front ) {
			if ( anMath::Fabs( dot ) > .06f ) {
				float fraction = ( anMath::Fabs( dot ) - .06f ) / .09f;
				mins.x = fraction * ( .4375f * bodySize.x );
			} else {
				mins.x = 0.f;
			}

			maxs.x = .5f * bodySize.x;
		} else {
			mins.x = -.5f * bodySize.x;

			if ( anMath::Fabs( dot ) > .06f ) {
				float fraction = ( anMath::Fabs( dot ) - .06f ) / .09f;
				maxs.x = -fraction * ( .4375f * bodySize.x );
			} else {
				maxs.x = 0.f;
			}
		}

		mins.z = -.5f * bodySize.z;
		maxs.z = .5f * bodySize.z;
	} else {
		// not scraping
        mins.x = maxs.x = localCollisionPoint.x;
		mins.y = maxs.y = localCollisionPoint.y;

		if ( localCollisionPoint.x > ( .5f * bodySize.x ) - 2.f ||
			 localCollisionPoint.x < ( -.5f * bodySize.x ) + 2.f ||
			 localCollisionPoint.y > ( .5f * bodySize.y ) - 2.f ||
			 localCollisionPoint.x < ( -.5f * bodySize.y ) + 2.f ) {
			 // sides
			mins.z = -.5f * bodySize.z;
			maxs.z = .5f * bodySize.z;
		} else {
		//if ( ( localCollisionPoint.z > ( .5f * bodySize.z ) - 2.f ) || ( localCollisionPoint.z < ( -.5f * bodySize.z ) + 2.f ) ) {
			// top/bottom
			mins.z = maxs.z = localCollisionPoint.z;
		}
	}

	// transform to world
	mins *= physicsObj.GetAxis();
	mins += bodyOrigin;

	maxs *= physicsObj.GetAxis();
	maxs += bodyOrigin;

	if ( onCollisionFunc ) {
		sdScriptHelper helper;
		helper.Push( loggedTrace ? loggedTrace->GetScriptObject() : NULL );
		helper.Push( velocity );
		helper.Push( mins );
		helper.Push( maxs );
		CallNonBlockingScriptEvent( onCollisionFunc, helper );
	}

	if ( scrape ) {
		if ( onCollisionSideScrapeFunc ) {
			sdScriptHelper helper;
			helper.Push( loggedTrace ? loggedTrace->GetScriptObject() : NULL );
			helper.Push( velocity );
			helper.Push( mins );
			helper.Push( maxs );
			CallNonBlockingScriptEvent( onCollisionSideScrapeFunc, helper );
		}
	}

	gameLocal.FreeLoggedTrace( loggedTrace );
}

/*
================
anVehicle_RigidBody::FreezePhysics
================
*/
void anVehicle_RigidBody::FreezePhysics( bool freeze ) {
	physicsObj.SetFrozen( freeze );
	physicsObj.SetLinearVelocity( vec3_origin );
	physicsObj.SetAngularVelocity( vec3_origin );
}

/*
================
anVehicle_RigidBody::Event_SetDamageDealtScale
================
*/
void anVehicle_RigidBody::Event_SetDamageDealtScale( float scale ) {
	SetDamageDealtScale( scale );
}

/*
================
anVehicle_RigidBody::SetDamageDealtScale
================
*/
void anVehicle_RigidBody::SetDamageDealtScale( float scale ) {
	collideDamageDealtScale = scale;
}
