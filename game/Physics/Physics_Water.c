#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

CLASS_DECLARATION( arcPhysics_Actor, arcPhysics_Player )
END_CLASS

// movement parameters
const float PM_STOPSPEED		= 100.0f;
const float PM_SWIMSCALE		= 0.5f;
const float PM_LADDERSPEED		= 100.0f;
const float PM_STEPSCALE		= 1.0f;

const float PM_ACCELERATE		= 10.0f;
const float PM_AIRACCELERATE	= 1.0f;
const float PM_WATERACCELERATE	= 4.0f;
const float PM_FLYACCELERATE	= 8.0f;

const float PM_FRICTION			= 6.0f;
const float PM_AIRFRICTION		= 0.0f;
const float PM_WATERFRICTION	= 1.0f;
const float PM_FLYFRICTION		= 3.0f;
const float PM_NOCLIPFRICTION	= 12.0f;

const float MIN_WALK_NORMAL		= 0.7f;		// can't walk on very steep slopes
const float OVERCLIP			= 1.001f;

// movementFlags
const int PMF_DUCKED			= 1;		// set when ducking
const int PMF_JUMPED			= 2;		// set when the player jumped this frame
const int PMF_STEPPED_UP		= 4;		// set when the player stepped up this frame
const int PMF_STEPPED_DOWN		= 8;		// set when the player stepped down this frame
const int PMF_JUMP_HELD			= 16;		// set when jump button is held down
const int PMF_TIME_LAND			= 32;		// movementTime is time before rejump
const int PMF_TIME_KNOCKBACK	= 64;		// movementTime is an air-accelerate only time
const int PMF_TIME_WATERJUMP	= 128;		// movementTime is waterjump
const int PMF_ALL_TIMES			= (PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK);

int c_pmove = 0;

/*
============
arcPhysics_Player::CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
float arcPhysics_Player::CmdScale( const usercmd_t &cmd ) const {
	int		max;
	float	total;
	float	scale;
	int		forwardmove;
	int		rightmove;
	int		upmove;

	forwardmove = cmd.forwardmove;
	rightmove = cmd.rightmove;

	// since the crouch key doubles as downward movement, ignore downward movement when we're on the ground
	// otherwise crouch speed will be lower than specified
	if ( walking ) {
		upmove = 0;
	} else {
		upmove = cmd.upmove;
	}

	max = abs( forwardmove );
	if ( abs( rightmove ) > max ) {
		max = abs( rightmove );
	}
	if ( abs( upmove ) > max ) {
		max = abs( upmove );
	}

	if ( !max ) {
		return 0.0f;
	}

	total = arcMath::Sqrt( (float) forwardmove * forwardmove + rightmove * rightmove + upmove * upmove );
	scale = (float) playerSpeed * max / ( 127.0f * total );

	return scale;
}

/*
==============
arcPhysics_Player::Accelerate

Handles user intended acceleration
==============
*/
void arcPhysics_Player::Accelerate( const arcVec3 &wishdir, const float wishspeed, const float accel ) {
#if 1
	// q2 style
	float addspeed, accelspeed, currentspeed;

	currentspeed = current.velocity * wishdir;
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) {
		return;
	}
	accelspeed = accel * frametime * wishspeed;
	if (accelspeed > addspeed) {
		accelspeed = addspeed;
	}

	current.velocity += accelspeed * wishdir;
#else
	// proper way (avoids strafe jump maxspeed bug), but feels bad
	arcVec3		wishVelocity;
	arcVec3		pushDir;
	float		pushLen;
	float		canPush;

	wishVelocity = wishdir * wishspeed;
	pushDir = wishVelocity - current.velocity;
	pushLen = pushDir.Normalize();

	canPush = accel * frametime * wishspeed;
	if (canPush > pushLen) {
		canPush = pushLen;
	}

	current.velocity += canPush * pushDir;
#endif
}

/*
==================
arcPhysics_Player::SlideMove

Returns true if the velocity was clipped in some way
==================
*/
#define	MAX_CLIP_PLANES	5

bool arcPhysics_Player::SlideMove( bool gravity, bool stepUp, bool stepDown, bool push ) {
	int			i, j, k, pushFlags;
	int			bumpcount, numbumps, numplanes;
	float		d, time_left, into, totalMass;
	arcVec3		dir, planes[MAX_CLIP_PLANES];
	arcVec3		end, stepEnd, primal_velocity, endVelocity, endClipVelocity, clipVelocity;
	trace_t		trace, stepTrace, downTrace;
	bool		nearGround, stepped, pushed;

	numbumps = 4;

	primal_velocity = current.velocity;

	if ( gravity ) {
		endVelocity = current.velocity + gravityVector * frametime;
		current.velocity = ( current.velocity + endVelocity ) * 0.5f;
		primal_velocity = endVelocity;
		if ( groundPlane ) {
			// slide along the ground plane
			current.velocity.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );
		}
	} else {
		endVelocity = current.velocity;
	}

	time_left = frametime;

	// never turn against the ground plane
	if ( groundPlane ) {
		numplanes = 1;
		planes[0] = groundTrace.c.normal;
	} else {
		numplanes = 0;
	}

	// never turn against original velocity
	planes[numplanes] = current.velocity;
	planes[numplanes].Normalize();
	numplanes++;

	for ( bumpcount = 0; bumpcount < numbumps; bumpcount++ ) {
		// calculate position we are trying to move to
		end = current.origin + time_left * current.velocity;

		// see if we can make it there
		gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );

		time_left -= time_left * trace.fraction;
		current.origin = trace.endpos;

		// if moved the entire distance
		if ( trace.fraction >= 1.0f ) {
			break;
		}

		stepped = pushed = false;

		// if we are allowed to step up
		if ( stepUp ) {
			nearGround = groundPlane | ladder;
			if ( !nearGround ) {
				// trace down to see if the player is near the ground
				// step checking when near the ground allows the player to move up stairs smoothly while jumping
				stepEnd = current.origin + maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );
				nearGround = ( downTrace.fraction < 1.0f && (downTrace.c.normal * -gravityNormal) > MIN_WALK_NORMAL );
			}

			// may only step up if near the ground or on a ladder
			if ( nearGround ) {
				// step up
				stepEnd = current.origin - maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				// trace along velocity
				stepEnd = downTrace.endpos + time_left * current.velocity;
				gameLocal.clip.Translation( stepTrace, downTrace.endpos, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				// step down
				stepEnd = stepTrace.endpos + maxStepHeight * gravityNormal;
				gameLocal.clip.Translation( downTrace, stepTrace.endpos, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );

				if ( downTrace.fraction >= 1.0f || (downTrace.c.normal * -gravityNormal) > MIN_WALK_NORMAL ) {
					// if moved the entire distance
					if ( stepTrace.fraction >= 1.0f ) {
						time_left = 0;
						current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
						current.origin = downTrace.endpos;
						current.movementFlags |= PMF_STEPPED_UP;
						current.velocity *= PM_STEPSCALE;
						break;
					}

					// if the move is further when stepping up
					if ( stepTrace.fraction > trace.fraction ) {
						time_left -= time_left * stepTrace.fraction;
						current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
						current.origin = downTrace.endpos;
						current.movementFlags |= PMF_STEPPED_UP;
						current.velocity *= PM_STEPSCALE;
						trace = stepTrace;
						stepped = true;
					}
				}
			}
		}

		// if we can push other entities and not blocked by the world
		if ( push && trace.c.entityNum != ENTITYNUM_WORLD ) {
			clipModel->SetPosition( current.origin, clipModel->GetAxis() );

			// clip movement, only push idMoveables, don't push entities the player is standing on
			// apply impact to pushed objects
			pushFlags = PUSHFL_CLIP|PUSHFL_ONLYMOVEABLE|PUSHFL_NOGROUNDENTITIES|PUSHFL_APPLYIMPULSE;

			// clip & push
			totalMass = gameLocal.push.ClipTranslationalPush( trace, self, pushFlags, end, end - current.origin );

			if ( totalMass > 0.0f ) {
				// decrease velocity based on the total mass of the objects being pushed ?
				current.velocity *= 1.0f - arcMath::ClampFloat( 0.0f, 1000.0f, totalMass - 20.0f ) * ( 1.0f / 950.0f );
				pushed = true;
			}

			current.origin = trace.endpos;
			time_left -= time_left * trace.fraction;

			// if moved the entire distance
			if ( trace.fraction >= 1.0f ) {
				break;
			}
		}

		if ( !stepped ) {
			// let the entity know about the collision
			self->Collide( trace, current.velocity );
		}

		if ( numplanes >= MAX_CLIP_PLANES ) {
			// MrElusive: I think we have some relatively high poly LWO models with a lot of slanted tris
			// where it may hit the max clip planes
			current.velocity = vec3_origin;
			return true;
		}

		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		for ( i = 0; i < numplanes; i++ ) {
			if ( ( trace.c.normal * planes[i] ) > 0.999f ) {
				current.velocity += trace.c.normal;
				break;
			}
		}
		if ( i < numplanes ) {
			continue;
		}
		planes[numplanes] = trace.c.normal;
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//

		// find a plane that it enters
		for ( i = 0; i < numplanes; i++ ) {
			into = current.velocity * planes[i];
			if ( into >= 0.1f ) {
				continue;		// move doesn't interact with the plane
			}

			// slide along the plane
			clipVelocity = current.velocity;
			clipVelocity.ProjectOntoPlane( planes[i], OVERCLIP );

			// slide along the plane
			endClipVelocity = endVelocity;
			endClipVelocity.ProjectOntoPlane( planes[i], OVERCLIP );

			// see if there is a second plane that the new move enters
			for ( j = 0; j < numplanes; j++ ) {
				if ( j == i ) {
					continue;
				}
				if ( ( clipVelocity * planes[j] ) >= 0.1f ) {
					continue;		// move doesn't interact with the plane
				}

				// try clipping the move to the plane
				clipVelocity.ProjectOntoPlane( planes[j], OVERCLIP );
				endClipVelocity.ProjectOntoPlane( planes[j], OVERCLIP );

				// see if it goes back into the first clip plane
				if ( ( clipVelocity * planes[i] ) >= 0 ) {
					continue;
				}

				// slide the original velocity along the crease
				dir = planes[i].Cross( planes[j] );
				dir.Normalize();
				d = dir * current.velocity;
				clipVelocity = d * dir;

				dir = planes[i].Cross( planes[j] );
				dir.Normalize();
				d = dir * endVelocity;
				endClipVelocity = d * dir;

				// see if there is a third plane the the new move enters
				for ( k = 0; k < numplanes; k++ ) {
					if ( k == i || k == j ) {
						continue;
					}
					if ( ( clipVelocity * planes[k] ) >= 0.1f ) {
						continue;		// move doesn't interact with the plane
					}

					// stop dead at a tripple plane interaction
					current.velocity = vec3_origin;
					return true;
				}
			}

			// if we have fixed all interactions, try another move
			current.velocity = clipVelocity;
			endVelocity = endClipVelocity;
			break;
		}
	}

	// step down
	if ( stepDown && groundPlane ) {
		stepEnd = current.origin + gravityNormal * maxStepHeight;
		gameLocal.clip.Translation( downTrace, current.origin, stepEnd, clipModel, clipModel->GetAxis(), clipMask, self );
		if ( downTrace.fraction > 1e-4f && downTrace.fraction < 1.0f ) {
			current.stepUp -= ( downTrace.endpos - current.origin ) * gravityNormal;
			current.origin = downTrace.endpos;
			current.movementFlags |= PMF_STEPPED_DOWN;
			current.velocity *= PM_STEPSCALE;
		}
	}

	if ( gravity ) {
		current.velocity = endVelocity;
	}

	// come to a dead stop when the velocity orthogonal to the gravity flipped
	clipVelocity = current.velocity - gravityNormal * current.velocity * gravityNormal;
	endClipVelocity = endVelocity - gravityNormal * endVelocity * gravityNormal;
	if ( clipVelocity * endClipVelocity < 0.0f ) {
		current.velocity = gravityNormal * current.velocity * gravityNormal;
	}

	return (bool)( bumpcount == 0 );
}

/*
==================
arcPhysics_Player::Friction

Handles both ground friction and water friction
==================
*/
void arcPhysics_Player::Friction( void ) {
	arcVec3	vel;
	float	speed, newspeed, control;
	float	drop;

	vel = current.velocity;
	if ( walking ) {
		// ignore slope movement, remove all velocity in gravity direction
		vel += (vel * gravityNormal) * gravityNormal;
	}

	speed = vel.Length();
	if ( speed < 1.0f ) {
		// remove all movement orthogonal to gravity, allows for sinking underwater
		if ( fabs( current.velocity * gravityNormal ) < 1e-5f ) {
			current.velocity.Zero();
		} else {
			current.velocity = (current.velocity * gravityNormal) * gravityNormal;
		}
		// FIXME: still have z friction underwater?
		return;
	}

	drop = 0;

	// spectator friction
	if ( current.movementType == PM_SPECTATOR ) {
		drop += speed * PM_FLYFRICTION * frametime;
	// apply ground friction
	} else if ( walking && waterLevel <= WATERLEVEL_FEET ) {
		// no friction on slick surfaces
		if ( !(groundMaterial && groundMaterial->GetSurfaceFlags() & SURF_SLICK) ) {
			// if getting knocked back, no friction
			if ( !(current.movementFlags & PMF_TIME_KNOCKBACK) ) {
				control = speed < PM_STOPSPEED ? PM_STOPSPEED : speed;
				drop += control * PM_FRICTION * frametime;
			}
		}
	// apply water friction even if just wading
	} else if ( waterLevel ) {
		drop += speed * PM_WATERFRICTION * waterLevel * frametime;
	// apply air friction
	} else {
		drop += speed * PM_AIRFRICTION * frametime;
	}

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0) {
		newspeed = 0;
	}
	current.velocity *= ( newspeed / speed );
}

/*
===================
arcPhysics_Player::WaterJumpMove

Flying out of the water
===================
*/
void arcPhysics_Player::WaterJumpMove( void ) {
	// waterjump has no control, but falls
	arcPhysics_Player::SlideMove( true, true, false, false );

	// add gravity
	current.velocity += gravityNormal * frametime;
	// if falling down
	if ( current.velocity * gravityNormal > 0.0f ) {
		// cancel as soon as we are falling down again
		current.movementFlags &= ~PMF_ALL_TIMES;
		current.movementTime = 0;
	}
}

/*
===================
arcPhysics_Player::WaterMove
===================
*/
void arcPhysics_Player::WaterMove( void ) {
	arcVec3	wishvel;
	float	wishspeed;
	arcVec3	wishdir;
	float	scale;
	float	vel;

	if ( arcPhysics_Player::CheckWaterJump() ) {
		arcPhysics_Player::WaterJumpMove();
		return;
	}

	arcPhysics_Player::Friction();

	scale = arcPhysics_Player::CmdScale( command );

	// user intentions
	if ( !scale ) {
		wishvel = gravityNormal * 60; // sink towards bottom
	} else {
		wishvel = scale * (viewForward * command.forwardmove + viewRight * command.rightmove);
		wishvel -= scale * gravityNormal * command.upmove;
	}

	wishdir = wishvel;
	wishspeed = wishdir.Normalize();

	if ( wishspeed > playerSpeed * PM_SWIMSCALE ) {
		wishspeed = playerSpeed * PM_SWIMSCALE;
	}

	arcPhysics_Player::Accelerate( wishdir, wishspeed, PM_WATERACCELERATE );

	// make sure we can go up slopes easily under water
	if ( groundPlane && ( current.velocity * groundTrace.c.normal ) < 0.0f ) {
		vel = current.velocity.Length();
		// slide along the ground plane
		current.velocity.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );

		current.velocity.Normalize();
		current.velocity *= vel;
	}

	arcPhysics_Player::SlideMove( false, true, false, false );
}

/*
===================
arcPhysics_Player::FlyMove
===================
*/
void arcPhysics_Player::FlyMove( void ) {
	arcVec3	wishvel;
	float	wishspeed;
	arcVec3	wishdir;
	float	scale;

	// normal slowdown
	arcPhysics_Player::Friction();

	scale = arcPhysics_Player::CmdScale( command );

	if ( !scale ) {
		wishvel = vec3_origin;
	} else {
		wishvel = scale * (viewForward * command.forwardmove + viewRight * command.rightmove);
		wishvel -= scale * gravityNormal * command.upmove;
	}

	wishdir = wishvel;
	wishspeed = wishdir.Normalize();

	arcPhysics_Player::Accelerate( wishdir, wishspeed, PM_FLYACCELERATE );

	arcPhysics_Player::SlideMove( false, false, false, false );
}

/*
===================
arcPhysics_Player::AirMove
===================
*/
void arcPhysics_Player::AirMove( void ) {
	arcVec3		wishvel;
	arcVec3		wishdir;
	float		wishspeed;
	float		scale;

	arcPhysics_Player::Friction();

	scale = arcPhysics_Player::CmdScale( command );

	// project moves down to flat plane
	viewForward -= (viewForward * gravityNormal) * gravityNormal;
	viewRight -= (viewRight * gravityNormal) * gravityNormal;
	viewForward.Normalize();
	viewRight.Normalize();

	wishvel = viewForward * command.forwardmove + viewRight * command.rightmove;
	wishvel -= (wishvel * gravityNormal) * gravityNormal;
	wishdir = wishvel;
	wishspeed = wishdir.Normalize();
	wishspeed *= scale;

	// not on ground, so little effect on velocity
	arcPhysics_Player::Accelerate( wishdir, wishspeed, PM_AIRACCELERATE );

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if ( groundPlane ) {
		current.velocity.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );
	}

	arcPhysics_Player::SlideMove( true, false, false, false );
}

/*
===================
arcPhysics_Player::WalkMove
===================
*/
void arcPhysics_Player::WalkMove( void ) {
	arcVec3		wishvel;
	arcVec3		wishdir;
	float		wishspeed;
	float		scale;
	float		accelerate;
	arcVec3		oldVelocity, vel;
	float		oldVel, newVel;

	if ( waterLevel > WATERLEVEL_WAIST && ( viewForward * groundTrace.c.normal ) > 0.0f ) {
		// begin swimming
		arcPhysics_Player::WaterMove();
		return;
	}

	if ( arcPhysics_Player::CheckJump() ) {
		// jumped away
		if ( waterLevel > WATERLEVEL_FEET ) {
			arcPhysics_Player::WaterMove();
		} else {
			arcPhysics_Player::AirMove();
		}
		return;
	}

	arcPhysics_Player::Friction();

	scale = arcPhysics_Player::CmdScale( command );

	// project moves down to flat plane
	viewForward -= (viewForward * gravityNormal) * gravityNormal;
	viewRight -= (viewRight * gravityNormal) * gravityNormal;

	// project the forward and right directions onto the ground plane
	viewForward.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );
	viewRight.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );
	//
	viewForward.Normalize();
	viewRight.Normalize();

	wishvel = viewForward * command.forwardmove + viewRight * command.rightmove;
	wishdir = wishvel;
	wishspeed = wishdir.Normalize();
	wishspeed *= scale;

	// clamp the speed lower if wading or walking on the bottom
	if ( waterLevel ) {
		float	waterScale;

		waterScale = waterLevel / 3.0f;
		waterScale = 1.0f - ( 1.0f - PM_SWIMSCALE ) * waterScale;
		if ( wishspeed > playerSpeed * waterScale ) {
			wishspeed = playerSpeed * waterScale;
		}
	}

	// when a player gets hit, they temporarily lose full control, which allows them to be moved a bit
	if ( ( groundMaterial && groundMaterial->GetSurfaceFlags() & SURF_SLICK ) || current.movementFlags & PMF_TIME_KNOCKBACK ) {
		accelerate = PM_AIRACCELERATE;
	} else {
		accelerate = PM_ACCELERATE;
	}

	arcPhysics_Player::Accelerate( wishdir, wishspeed, accelerate );

	if ( ( groundMaterial && groundMaterial->GetSurfaceFlags() & SURF_SLICK ) || current.movementFlags & PMF_TIME_KNOCKBACK ) {
		current.velocity += gravityVector * frametime;
	}

	oldVelocity = current.velocity;

	// slide along the ground plane
	current.velocity.ProjectOntoPlane( groundTrace.c.normal, OVERCLIP );

	// if not clipped into the opposite direction
	if ( oldVelocity * current.velocity > 0.0f ) {
		newVel = current.velocity.LengthSqr();
		if ( newVel > 1.0f ) {
			oldVel = oldVelocity.LengthSqr();
			if ( oldVel > 1.0f ) {
				// don't decrease velocity when going up or down a slope
				current.velocity *= arcMath::Sqrt( oldVel / newVel );
			}
		}
	}

	// don't do anything if standing still
	vel = current.velocity - (current.velocity * gravityNormal) * gravityNormal;
	if ( !vel.LengthSqr() ) {
		return;
	}

	gameLocal.push.InitSavingPushedEntityPositions();

	arcPhysics_Player::SlideMove( false, true, true, true );
}

/*
==============
arcPhysics_Player::DeadMove
==============
*/
void arcPhysics_Player::DeadMove( void ) {
	float	forward;

	if ( !walking ) {
		return;
	}

	// extra friction
	forward = current.velocity.Length();
	forward -= 20;
	if ( forward <= 0 ) {
		current.velocity = vec3_origin;
	} else {
		current.velocity.Normalize();
		current.velocity *= forward;
	}
}

/*
===============
arcPhysics_Player::NoclipMove
===============
*/
void arcPhysics_Player::NoclipMove( void ) {
	float		speed, drop, friction, newspeed, stopspeed;
	float		scale, wishspeed;
	arcVec3		wishdir;

	// friction
	speed = current.velocity.Length();
	if ( speed < 20.0f ) {
		current.velocity = vec3_origin;
	}
	else {
		stopspeed = playerSpeed * 0.3f;
		if ( speed < stopspeed ) {
			speed = stopspeed;
		}
		friction = PM_NOCLIPFRICTION;
		drop = speed * friction * frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0) {
			newspeed = 0;
		}

		current.velocity *= newspeed / speed;
	}

	// accelerate
	scale = arcPhysics_Player::CmdScale( command );

	wishdir = scale * (viewForward * command.forwardmove + viewRight * command.rightmove);
	wishdir -= scale * gravityNormal * command.upmove;
	wishspeed = wishdir.Normalize();
	wishspeed *= scale;

	arcPhysics_Player::Accelerate( wishdir, wishspeed, PM_ACCELERATE );

	// move
	current.origin += frametime * current.velocity;
}

/*
===============
arcPhysics_Player::SpectatorMove
===============
*/
void arcPhysics_Player::SpectatorMove( void ) {
	arcVec3	wishvel;
	float	wishspeed;
	arcVec3	wishdir;
	float	scale;

	trace_t	trace;
	arcVec3	end;

	// fly movement

	arcPhysics_Player::Friction();

	scale = arcPhysics_Player::CmdScale( command );

	if ( !scale ) {
		wishvel = vec3_origin;
	} else {
		wishvel = scale * (viewForward * command.forwardmove + viewRight * command.rightmove);
	}

	wishdir = wishvel;
	wishspeed = wishdir.Normalize();

	arcPhysics_Player::Accelerate( wishdir, wishspeed, PM_FLYACCELERATE );

	arcPhysics_Player::SlideMove( false, false, false, false );
}

/*
============
arcPhysics_Player::LadderMove
============
*/
void arcPhysics_Player::LadderMove( void ) {
	arcVec3	wishdir, wishvel, right;
	float	wishspeed, scale;
	float	upscale;

	// stick to the ladder
	wishvel = -100.0f * ladderNormal;
	current.velocity = (gravityNormal * current.velocity) * gravityNormal + wishvel;

	upscale = (-gravityNormal * viewForward + 0.5f) * 2.5f;
	if ( upscale > 1.0f ) {
		upscale = 1.0f;
	}
	else if ( upscale < -1.0f ) {
		upscale = -1.0f;
	}

	scale = arcPhysics_Player::CmdScale( command );
	wishvel = -0.9f * gravityNormal * upscale * scale * (float)command.forwardmove;

	// strafe
	if ( command.rightmove ) {
		// right vector orthogonal to gravity
		right = viewRight - (gravityNormal * viewRight) * gravityNormal;
		// project right vector into ladder plane
		right = right - (ladderNormal * right) * ladderNormal;
		right.Normalize();

		// if we are looking away from the ladder, reverse the right vector
		if ( ladderNormal * viewForward > 0.0f ) {
			right = -right;
		}
		wishvel += 2.0f * right * scale * (float) command.rightmove;
	}

	// up down movement
	if ( command.upmove ) {
		wishvel += -0.5f * gravityNormal * scale * (float) command.upmove;
	}

	// do strafe friction
	arcPhysics_Player::Friction();

	// accelerate
	wishspeed = wishvel.Normalize();
	arcPhysics_Player::Accelerate( wishvel, wishspeed, PM_ACCELERATE );

	// cap the vertical velocity
	upscale = current.velocity * -gravityNormal;
	if ( upscale < -PM_LADDERSPEED ) {
		current.velocity += gravityNormal * (upscale + PM_LADDERSPEED);
	} else if ( upscale > PM_LADDERSPEED ) {
		current.velocity += gravityNormal * (upscale - PM_LADDERSPEED);
	}

	if ( (wishvel * gravityNormal) == 0.0f ) {
		if ( current.velocity * gravityNormal < 0.0f ) {
			current.velocity += gravityVector * frametime;
			if ( current.velocity * gravityNormal > 0.0f ) {
				current.velocity -= (gravityNormal * current.velocity) * gravityNormal;
			}
		} else {
			current.velocity -= gravityVector * frametime;
			if ( current.velocity * gravityNormal < 0.0f ) {
				current.velocity -= (gravityNormal * current.velocity) * gravityNormal;
			}
		}
	}

	arcPhysics_Player::SlideMove( false, ( command.forwardmove > 0 ), false, false );
}

/*
=============
arcPhysics_Player::CorrectAllSolid
=============
*/
void arcPhysics_Player::CorrectAllSolid( trace_t &trace, int contents ) {
	if ( debugLevel ) {
		gameLocal.Printf( "%i:allsolid\n", c_pmove );
	}

	// FIXME: jitter around to find a free spot ?

	if ( trace.fraction >= 1.0f ) {
		memset( &trace, 0, sizeof( trace ) );
		trace.endpos = current.origin;
		trace.endAxis = clipModelAxis;
		trace.fraction = 0.0f;
		trace.c.dist = current.origin.z;
		trace.c.normal.Set( 0, 0, 1 );
		trace.c.point = current.origin;
		trace.c.entityNum = ENTITYNUM_WORLD;
		trace.c.id = 0;
		trace.c.type = CONTACT_TRMVERTEX;
		trace.c.material = NULL;
		trace.c.contents = contents;
	}
}

/*
=============
arcPhysics_Player::CheckGround
=============
*/
void arcPhysics_Player::CheckGround( void ) {
	int i, contents;
	arcVec3 point;
	bool hadGroundContacts;

	hadGroundContacts = HasGroundContacts();

	// set the clip model origin before getting the contacts
	clipModel->SetPosition( current.origin, clipModel->GetAxis() );

	EvaluateContacts();

	// setup a ground trace from the contacts
	groundTrace.endpos = current.origin;
	groundTrace.endAxis = clipModel->GetAxis();
	if ( contacts.Num() ) {
		groundTrace.fraction = 0.0f;
		groundTrace.c = contacts[0];
		for ( i = 1; i < contacts.Num(); i++ ) {
			groundTrace.c.normal += contacts[i].normal;
		}
		groundTrace.c.normal.Normalize();
	} else {
		groundTrace.fraction = 1.0f;
	}

	contents = gameLocal.clip.Contents( current.origin, clipModel, clipModel->GetAxis(), -1, self );
	if ( contents & MASK_SOLID ) {
		// do something corrective if stuck in solid
		arcPhysics_Player::CorrectAllSolid( groundTrace, contents );
	}

	// if the trace didn't hit anything, we are in free fall
	if ( groundTrace.fraction == 1.0f ) {
		groundPlane = false;
		walking = false;
		groundEntityPtr = NULL;
		return;
	}

	groundMaterial = groundTrace.c.material;
	groundEntityPtr = gameLocal.entities[ groundTrace.c.entityNum ];

	// check if getting thrown off the ground
	if ( (current.velocity * -gravityNormal) > 0.0f && ( current.velocity * groundTrace.c.normal ) > 10.0f ) {
		if ( debugLevel ) {
			gameLocal.Printf( "%i:kickoff\n", c_pmove );
		}

		groundPlane = false;
		walking = false;
		return;
	}

	// slopes that are too steep will not be considered onground
	if ( ( groundTrace.c.normal * -gravityNormal ) < MIN_WALK_NORMAL ) {
		if ( debugLevel ) {
			gameLocal.Printf( "%i:steep\n", c_pmove );
		}

		// FIXME: if they can't slide down the slope, let them walk (sharp crevices)

		// make sure we don't die from sliding down a steep slope
		if ( current.velocity * gravityNormal > 150.0f ) {
			current.velocity -= ( current.velocity * gravityNormal - 150.0f ) * gravityNormal;
		}

		groundPlane = true;
		walking = false;
		return;
	}

	groundPlane = true;
	walking = true;

	// hitting solid ground will end a waterjump
	if ( current.movementFlags & PMF_TIME_WATERJUMP ) {
		current.movementFlags &= ~( PMF_TIME_WATERJUMP | PMF_TIME_LAND );
		current.movementTime = 0;
	}

	// if the player didn't have ground contacts the previous frame
	if ( !hadGroundContacts ) {
		// don't do landing time if we were just going down a slope
		if ( (current.velocity * -gravityNormal) < -200.0f ) {
			// don't allow another jump for a little while
			current.movementFlags |= PMF_TIME_LAND;
			current.movementTime = 250;
		}
	}

	// let the entity know about the collision
	self->Collide( groundTrace, current.velocity );

	if ( groundEntityPtr.GetEntity() ) {
		impactInfo_t info;
		groundEntityPtr.GetEntity()->GetImpactInfo( self, groundTrace.c.id, groundTrace.c.point, &info );
		if ( info.invMass != 0.0f ) {
			groundEntityPtr.GetEntity()->ApplyImpulse( self, groundTrace.c.id, groundTrace.c.point, current.velocity / ( info.invMass * 10.0f ) );
		}
	}
}

/*
==============
arcPhysics_Player::CheckDuck

Sets clip model size
==============
*/
void arcPhysics_Player::CheckDuck( void ) {
	trace_t	trace;
	arcVec3 end;
	arcBounds bounds;
	float maxZ;

	if ( current.movementType == PM_DEAD ) {
		maxZ = pm_deadheight.GetFloat();
	} else {
		// stand up when up against a ladder
		if ( command.upmove < 0 && !ladder ) {
			// duck
			current.movementFlags |= PMF_DUCKED;
		} else {
			// stand up if possible
			if ( current.movementFlags & PMF_DUCKED ) {
				// try to stand up
				end = current.origin - ( pm_normalheight.GetFloat() - pm_crouchheight.GetFloat() ) * gravityNormal;
				gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );
				if ( trace.fraction >= 1.0f ) {
					current.movementFlags &= ~PMF_DUCKED;
				}
			}
		}

		if ( current.movementFlags & PMF_DUCKED ) {
			playerSpeed = crouchSpeed;
			maxZ = pm_crouchheight.GetFloat();
		} else {
			maxZ = pm_normalheight.GetFloat();
		}
	}
	// if the clipModel height should change
	if ( clipModel->GetBounds()[1][2] != maxZ ) {

		bounds = clipModel->GetBounds();
		bounds[1][2] = maxZ;
		if ( pm_usecylinder.GetBool() ) {
			clipModel->LoadModel( arcTraceModel( bounds, 8 ) );
		} else {
			clipModel->LoadModel( arcTraceModel( bounds ) );
		}
	}
}

/*
================
arcPhysics_Player::CheckLadder
================
*/
void arcPhysics_Player::CheckLadder( void ) {
	arcVec3		forward, start, end;
	trace_t		trace;
	float		tracedist;

	if ( current.movementTime ) {
		return;
	}

	// if on the ground moving backwards
	if ( walking && command.forwardmove <= 0 ) {
		return;
	}

	// forward vector orthogonal to gravity
	forward = viewForward - (gravityNormal * viewForward) * gravityNormal;
	forward.Normalize();

	if ( walking ) {
		// don't want to get sucked towards the ladder when still walking
		tracedist = 1.0f;
	} else {
		tracedist = 48.0f;
	}

	end = current.origin + tracedist * forward;
	gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );

	// if near a surface
	if ( trace.fraction < 1.0f ) {

		// if a ladder surface
		if ( trace.c.material && ( trace.c.material->GetSurfaceFlags() & SURF_LADDER ) ) {

			// check a step height higher
			end = current.origin - gravityNormal * ( maxStepHeight * 0.75f );
			gameLocal.clip.Translation( trace, current.origin, end, clipModel, clipModel->GetAxis(), clipMask, self );
			start = trace.endpos;
			end = start + tracedist * forward;
			gameLocal.clip.Translation( trace, start, end, clipModel, clipModel->GetAxis(), clipMask, self );

			// if also near a surface a step height higher
			if ( trace.fraction < 1.0f ) {
				// if it also is a ladder surface
				if ( trace.c.material && trace.c.material->GetSurfaceFlags() & SURF_LADDER ) {
					ladder = true;
					ladderNormal = trace.c.normal;
				}
			}
		}
	}
}

/*
=============
arcPhysics_Player::CheckJump
=============
*/
bool arcPhysics_Player::CheckJump( void ) {
	arcVec3 addVelocity;

	if ( command.upmove < 10 ) {
		// not holding jump
		return false;
	}

	// must wait for jump to be released
	if ( current.movementFlags & PMF_JUMP_HELD ) {
		return false;
	}

	// don't jump if we can't stand up
	if ( current.movementFlags & PMF_DUCKED ) {
		return false;
	}

	groundPlane = false;		// jumping away
	walking = false;
	current.movementFlags |= PMF_JUMP_HELD | PMF_JUMPED;

	addVelocity = 2.0f * maxJumpHeight * -gravityVector;
	addVelocity *= arcMath::Sqrt( addVelocity.Normalize() );
	current.velocity += addVelocity;

	return true;
}

/*
=============
arcPhysics_Player::CheckWaterJump
=============
*/
bool arcPhysics_Player::CheckWaterJump( void ) {
	arcVec3	spot;
	int		cont;
	arcVec3	flatforward;

	if ( current.movementTime ) {
		return false;
	}

	// check for water jump
	if ( waterLevel != WATERLEVEL_WAIST ) {
		return false;
	}

	flatforward = viewForward - (viewForward * gravityNormal) * gravityNormal;
	flatforward.Normalize();

	spot = current.origin + 30.0f * flatforward;
	spot -= 4.0f * gravityNormal;
	cont = gameLocal.clip.Contents( spot, NULL, mat3_identity, -1, self );
	if ( !(cont & CONTENTS_SOLID) ) {
		return false;
	}

	spot -= 16.0f * gravityNormal;
	cont = gameLocal.clip.Contents( spot, NULL, mat3_identity, -1, self );
	if ( cont ) {
		return false;
	}

	// jump out of water
	current.velocity = 200.0f * viewForward - 350.0f * gravityNormal;
	current.movementFlags |= PMF_TIME_WATERJUMP;
	current.movementTime = 2000;

	return true;
}

/*
=============
arcPhysics_Player::SetWaterLevel
=============
*/
void arcPhysics_Player::SetWaterLevel( void ) {
	arcVec3		point;
	arcBounds	bounds;
	int			contents;

	//
	// get waterlevel, accounting for ducking
	//
	waterLevel = WATERLEVEL_NONE;
	waterType = 0;

	bounds = clipModel->GetBounds();

	// check at feet level
	point = current.origin - ( bounds[0][2] + 1.0f ) * gravityNormal;
	contents = gameLocal.clip.Contents( point, NULL, mat3_identity, -1, self );
	if ( contents & MASK_WATER ) {
		waterType = contents;
		waterLevel = WATERLEVEL_FEET;

		// check at waist level
		point = current.origin - ( bounds[1][2] - bounds[0][2] ) * 0.5f * gravityNormal;
		contents = gameLocal.clip.Contents( point, NULL, mat3_identity, -1, self );
		if ( contents & MASK_WATER ) {
			waterLevel = WATERLEVEL_WAIST;

			// check at head level
			point = current.origin - ( bounds[1][2] - 1.0f ) * gravityNormal;
			contents = gameLocal.clip.Contents( point, NULL, mat3_identity, -1, self );
			if ( contents & MASK_WATER ) {
				waterLevel = WATERLEVEL_HEAD;
			}
		}
	}
}

/*
================
arcPhysics_Player::DropTimers
================
*/
void arcPhysics_Player::DropTimers( void ) {
	// drop misc timing counter
	if ( current.movementTime ) {
		if ( framemsec >= current.movementTime ) {
			current.movementFlags &= ~PMF_ALL_TIMES;
			current.movementTime = 0;
		} else {
			current.movementTime -= framemsec;
		}
	}
}

/*
================
arcPhysics_Player::MovePlayer
================
*/
void arcPhysics_Player::MovePlayer( int msec ) {
	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint for the previous frame
	c_pmove++;

	walking = false;
	groundPlane = false;
	ladder = false;

	// determine the time
	framemsec = msec;
	frametime = framemsec * 0.001f;

	// default speed
	playerSpeed = walkSpeed;

	// remove jumped and stepped up flag
	current.movementFlags &= ~(PMF_JUMPED|PMF_STEPPED_UP|PMF_STEPPED_DOWN);
	current.stepUp = 0.0f;

	if ( command.upmove < 10 ) {
		// not holding jump
		current.movementFlags &= ~PMF_JUMP_HELD;
	}

	// if no movement at all
	if ( current.movementType == PM_FREEZE ) {
		return;
	}

	// move the player velocity into the frame of a pusher
	current.velocity -= current.pushVelocity;

	// view vectors
	viewAngles.ToVectors( &viewForward, NULL, NULL );
	viewForward *= clipModelAxis;
	viewRight = gravityNormal.Cross( viewForward );
	viewRight.Normalize();

	// fly in spectator mode
	if ( current.movementType == PM_SPECTATOR ) {
		SpectatorMove();
		arcPhysics_Player::DropTimers();
		return;
	}

	// special no clip mode
	if ( current.movementType == PM_NOCLIP ) {
		arcPhysics_Player::NoclipMove();
		arcPhysics_Player::DropTimers();
		return;
	}

	// no control when dead
	if ( current.movementType == PM_DEAD ) {
		command.forwardmove = 0;
		command.rightmove = 0;
		command.upmove = 0;
	}

	// set watertype and waterlevel
	arcPhysics_Player::SetWaterLevel();

	// check for ground
	arcPhysics_Player::CheckGround();

	// check if up against a ladder
	arcPhysics_Player::CheckLadder();

	// set clip model size
	arcPhysics_Player::CheckDuck();

	// handle timers
	arcPhysics_Player::DropTimers();

	// move
	if ( current.movementType == PM_DEAD ) {
		// dead
		arcPhysics_Player::DeadMove();
	} else if ( ladder ) {
		// going up or down a ladder
		arcPhysics_Player::LadderMove();
	} else if ( current.movementFlags & PMF_TIME_WATERJUMP ) {
		// jumping out of water
		arcPhysics_Player::WaterJumpMove();
	} else if ( waterLevel > 1 ) {
		// swimming
		arcPhysics_Player::WaterMove();
	} else if ( walking ) {
		// walking on ground
		arcPhysics_Player::WalkMove();
	} else {
		// airborne
		arcPhysics_Player::AirMove();
	}

	// set watertype, waterlevel and groundentity
	arcPhysics_Player::SetWaterLevel();
	arcPhysics_Player::CheckGround();

	// move the player velocity back into the world frame
	current.velocity += current.pushVelocity;
	current.pushVelocity.Zero();
}

/*
================
arcPhysics_Player::GetWaterLevel
================
*/
waterLevel_t arcPhysics_Player::GetWaterLevel( void ) const {
	return waterLevel;
}

/*
================
arcPhysics_Player::GetWaterType
================
*/
int arcPhysics_Player::GetWaterType( void ) const {
	return waterType;
}

/*
================
arcPhysics_Player::HasJumped
================
*/
bool arcPhysics_Player::HasJumped( void ) const {
	return ( ( current.movementFlags & PMF_JUMPED ) != 0 );
}

/*
================
arcPhysics_Player::HasSteppedUp
================
*/
bool arcPhysics_Player::HasSteppedUp( void ) const {
	return ( ( current.movementFlags & ( PMF_STEPPED_UP | PMF_STEPPED_DOWN ) ) != 0 );
}

/*
================
arcPhysics_Player::GetStepUp
================
*/
float arcPhysics_Player::GetStepUp( void ) const {
	return current.stepUp;
}

/*
================
arcPhysics_Player::IsCrouching
================
*/
bool arcPhysics_Player::IsCrouching( void ) const {
	return ( ( current.movementFlags & PMF_DUCKED ) != 0 );
}

/*
================
arcPhysics_Player::OnLadder
================
*/
bool arcPhysics_Player::OnLadder( void ) const {
	return ladder;
}

/*
================
arcPhysics_Player::arcPhysics_Player
================
*/
arcPhysics_Player::arcPhysics_Player( void ) {
	debugLevel = false;
	clipModel = NULL;
	clipMask = 0;
	memset( &current, 0, sizeof( current ) );
	saved = current;
	walkSpeed = 0;
	crouchSpeed = 0;
	maxStepHeight = 0;
	maxJumpHeight = 0;
	memset( &command, 0, sizeof( command ) );
	viewAngles.Zero();
	framemsec = 0;
	frametime = 0;
	playerSpeed = 0;
	viewForward.Zero();
	viewRight.Zero();
	walking = false;
	groundPlane = false;
	memset( &groundTrace, 0, sizeof( groundTrace ) );
	groundMaterial = NULL;
	ladder = false;
	ladderNormal.Zero();
	waterLevel = WATERLEVEL_NONE;
	waterType = 0;
}

/*
================
arcPhysics_Player_SavePState
================
*/
void arcPhysics_Player_SavePState( arcSaveGame *savefile, const playerPState_t &state ) {
	savefile->WriteVec3( state.origin );
	savefile->WriteVec3( state.velocity );
	savefile->WriteVec3( state.localOrigin );
	savefile->WriteVec3( state.pushVelocity );
	savefile->WriteFloat( state.stepUp );
	savefile->WriteInt( state.movementType );
	savefile->WriteInt( state.movementFlags );
	savefile->WriteInt( state.movementTime );
}

/*
================
arcPhysics_Player_RestorePState
================
*/
void arcPhysics_Player_RestorePState( arcRestoreGame *savefile, playerPState_t &state ) {
	savefile->ReadVec3( state.origin );
	savefile->ReadVec3( state.velocity );
	savefile->ReadVec3( state.localOrigin );
	savefile->ReadVec3( state.pushVelocity );
	savefile->ReadFloat( state.stepUp );
	savefile->ReadInt( state.movementType );
	savefile->ReadInt( state.movementFlags );
	savefile->ReadInt( state.movementTime );
}

/*
================
arcPhysics_Player::Save
================
*/
void arcPhysics_Player::Save( arcSaveGame *savefile ) const {
	arcPhysics_Player_SavePState( savefile, current );
	arcPhysics_Player_SavePState( savefile, saved );

	savefile->WriteFloat( walkSpeed );
	savefile->WriteFloat( crouchSpeed );
	savefile->WriteFloat( maxStepHeight );
	savefile->WriteFloat( maxJumpHeight );
	savefile->WriteInt( debugLevel );

	savefile->WriteUsercmd( command );
	savefile->WriteAngles( viewAngles );

	savefile->WriteInt( framemsec );
	savefile->WriteFloat( frametime );
	savefile->WriteFloat( playerSpeed );
	savefile->WriteVec3( viewForward );
	savefile->WriteVec3( viewRight );

	savefile->WriteBool( walking );
	savefile->WriteBool( groundPlane );
	savefile->WriteTrace( groundTrace );
	savefile->WriteMaterial( groundMaterial );

	savefile->WriteBool( ladder );
	savefile->WriteVec3( ladderNormal );

	savefile->WriteInt( (int)waterLevel );
	savefile->WriteInt( waterType );
}

/*
================
arcPhysics_Player::Restore
================
*/
void arcPhysics_Player::Restore( arcRestoreGame *savefile ) {
	arcPhysics_Player_RestorePState( savefile, current );
	arcPhysics_Player_RestorePState( savefile, saved );

	savefile->ReadFloat( walkSpeed );
	savefile->ReadFloat( crouchSpeed );
	savefile->ReadFloat( maxStepHeight );
	savefile->ReadFloat( maxJumpHeight );
	savefile->ReadInt( debugLevel );

	savefile->ReadUsercmd( command );
	savefile->ReadAngles( viewAngles );

	savefile->ReadInt( framemsec );
	savefile->ReadFloat( frametime );
	savefile->ReadFloat( playerSpeed );
	savefile->ReadVec3( viewForward );
	savefile->ReadVec3( viewRight );

	savefile->ReadBool( walking );
	savefile->ReadBool( groundPlane );
	savefile->ReadTrace( groundTrace );
	savefile->ReadMaterial( groundMaterial );

	savefile->ReadBool( ladder );
	savefile->ReadVec3( ladderNormal );

	savefile->ReadInt( (int &)waterLevel );
	savefile->ReadInt( waterType );
}

/*
================
arcPhysics_Player::SetPlayerInput
================
*/
void arcPhysics_Player::SetPlayerInput( const usercmd_t &cmd, const arcAngles &newViewAngles ) {
	command = cmd;
	viewAngles = newViewAngles;		// can't use cmd.angles cause of the delta_angles
}

/*
================
arcPhysics_Player::SetSpeed
================
*/
void arcPhysics_Player::SetSpeed( const float newWalkSpeed, const float newCrouchSpeed ) {
	walkSpeed = newWalkSpeed;
	crouchSpeed = newCrouchSpeed;
}

/*
================
arcPhysics_Player::SetMaxStepHeight
================
*/
void arcPhysics_Player::SetMaxStepHeight( const float newMaxStepHeight ) {
	maxStepHeight = newMaxStepHeight;
}

/*
================
arcPhysics_Player::GetMaxStepHeight
================
*/
float arcPhysics_Player::GetMaxStepHeight( void ) const {
	return maxStepHeight;
}

/*
================
arcPhysics_Player::SetMaxJumpHeight
================
*/
void arcPhysics_Player::SetMaxJumpHeight( const float newMaxJumpHeight ) {
	maxJumpHeight = newMaxJumpHeight;
}

/*
================
arcPhysics_Player::SetMovementType
================
*/
void arcPhysics_Player::SetMovementType( const pmtype_t type ) {
	current.movementType = type;
}

/*
================
arcPhysics_Player::SetKnockBack
================
*/
void arcPhysics_Player::SetKnockBack( const int knockBackTime ) {
	if ( current.movementTime ) {
		return;
	}
	current.movementFlags |= PMF_TIME_KNOCKBACK;
	current.movementTime = knockBackTime;
}

/*
================
arcPhysics_Player::SetDebugLevel
================
*/
void arcPhysics_Player::SetDebugLevel( bool set ) {
	debugLevel = set;
}

/*
================
arcPhysics_Player::Evaluate
================
*/
bool arcPhysics_Player::Evaluate( int timeStepMSec, int endTimeMSec ) {
	arcVec3 masterOrigin, oldOrigin;
	arcMat3 masterAxis;

	waterLevel = WATERLEVEL_NONE;
	waterType = 0;
	oldOrigin = current.origin;

	clipModel->Unlink();

	// if bound to a master
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + current.localOrigin * masterAxis;
		clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
		current.velocity = ( current.origin - oldOrigin ) / ( timeStepMSec * 0.001f );
		masterDeltaYaw = masterYaw;
		masterYaw = masterAxis[0].ToYaw();
		masterDeltaYaw = masterYaw - masterDeltaYaw;
		return true;
	}

	ActivateContactEntities();

	arcPhysics_Player::MovePlayer( timeStepMSec );

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );

	if ( IsOutsideWorld() ) {
		gameLocal.Warning( "clip model outside world bounds for entity '%s' at (%s)", self->name.c_str(), current.origin.ToString(0) );
	}

	return true; //( current.origin != oldOrigin );
}

/*
================
arcPhysics_Player::UpdateTime
================
*/
void arcPhysics_Player::UpdateTime( int endTimeMSec ) {
}

/*
================
arcPhysics_Player::GetTime
================
*/
int arcPhysics_Player::GetTime( void ) const {
	return gameLocal.time;
}

/*
================
arcPhysics_Player::GetImpactInfo
================
*/
void arcPhysics_Player::GetImpactInfo( const int id, const arcVec3 &point, impactInfo_t *info ) const {
	info->invMass = invMass;
	info->invInertiaTensor.Zero();
	info->position.Zero();
	info->velocity = current.velocity;
}

/*
================
arcPhysics_Player::ApplyImpulse
================
*/
void arcPhysics_Player::ApplyImpulse( const int id, const arcVec3 &point, const arcVec3 &impulse ) {
	if ( current.movementType != PM_NOCLIP ) {
		current.velocity += impulse * invMass;
	}
}

/*
================
arcPhysics_Player::IsAtRest
================
*/
bool arcPhysics_Player::IsAtRest( void ) const {
	return false;
}

/*
================
arcPhysics_Player::GetRestStartTime
================
*/
int arcPhysics_Player::GetRestStartTime( void ) const {
	return -1;
}

/*
================
arcPhysics_Player::SaveState
================
*/
void arcPhysics_Player::SaveState( void ) {
	saved = current;
}

/*
================
arcPhysics_Player::RestoreState
================
*/
void arcPhysics_Player::RestoreState( void ) {
	current = saved;
	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
	EvaluateContacts();
}

/*
================
arcPhysics_Player::SetOrigin
================
*/
void arcPhysics_Player::SetOrigin( const arcVec3 &newOrigin, int id ) {
	arcVec3 masterOrigin;
	arcMat3 masterAxis;

	current.localOrigin = newOrigin;
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + newOrigin * masterAxis;
	} else {
		current.origin = newOrigin;
	}

	clipModel->Link( gameLocal.clip, self, 0, newOrigin, clipModel->GetAxis() );
}

/*
================
arcPhysics_Player::GetOrigin
================
*/
const arcVec3 & arcPhysics_Player::PlayerGetOrigin( void ) const {
	return current.origin;
}

/*
================
arcPhysics_Player::SetAxis
================
*/
void arcPhysics_Player::SetAxis( const arcMat3 &newAxis, int id ) {
	clipModel->Link( gameLocal.clip, self, 0, clipModel->GetOrigin(), newAxis );
}

/*
================
arcPhysics_Player::Translate
================
*/
void arcPhysics_Player::Translate( const arcVec3 &translation, int id ) {

	current.localOrigin += translation;
	current.origin += translation;

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
}

/*
================
arcPhysics_Player::Rotate
================
*/
void arcPhysics_Player::Rotate( const idRotation &rotation, int id ) {
	arcVec3 masterOrigin;
	arcMat3 masterAxis;

	current.origin *= rotation;
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
	} else {
		current.localOrigin = current.origin;
	}

	clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() * rotation.ToMat3() );
}

/*
================
arcPhysics_Player::SetLinearVelocity
================
*/
void arcPhysics_Player::SetLinearVelocity( const arcVec3 &newLinearVelocity, int id ) {
	current.velocity = newLinearVelocity;
}

/*
================
arcPhysics_Player::GetLinearVelocity
================
*/
const arcVec3 &arcPhysics_Player::GetLinearVelocity( int id ) const {
	return current.velocity;
}

/*
================
arcPhysics_Player::SetPushed
================
*/
void arcPhysics_Player::SetPushed( int deltaTime ) {
	arcVec3 velocity;
	float d;

	// velocity with which the player is pushed
	velocity = ( current.origin - saved.origin ) / ( deltaTime * arcMath::M_MS2SEC );

	// remove any downward push velocity
	d = velocity * gravityNormal;
	if ( d > 0.0f ) {
		velocity -= d * gravityNormal;
	}

	current.pushVelocity += velocity;
}

/*
================
arcPhysics_Player::GetPushedLinearVelocity
================
*/
const arcVec3 &arcPhysics_Player::GetPushedLinearVelocity( const int id ) const {
	return current.pushVelocity;
}

/*
================
arcPhysics_Player::ClearPushedVelocity
================
*/
void arcPhysics_Player::ClearPushedVelocity( void ) {
	current.pushVelocity.Zero();
}

/*
================
arcPhysics_Player::SetMaster

  the binding is never orientated
================
*/
void arcPhysics_Player::SetMaster( arcEntity *master, const bool orientated ) {
	arcVec3 masterOrigin;
	arcMat3 masterAxis;

	if ( master ) {
		if ( !masterEntity ) {
			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
			masterEntity = master;
			masterYaw = masterAxis[0].ToYaw();
		}
		ClearContacts();
	} else {
		if ( masterEntity ) {
			masterEntity = NULL;
		}
	}
}

const float	PLAYER_VELOCITY_MAX				= 4000;
const int	PLAYER_VELOCITY_TOTAL_BITS		= 16;
const int	PLAYER_VELOCITY_EXPONENT_BITS	= arcMath::BitsForInteger( arcMath::BitsForFloat( PLAYER_VELOCITY_MAX ) ) + 1;
const int	PLAYER_VELOCITY_MANTISSA_BITS	= PLAYER_VELOCITY_TOTAL_BITS - 1 - PLAYER_VELOCITY_EXPONENT_BITS;
const int	PLAYER_MOVEMENT_TYPE_BITS		= 3;
const int	PLAYER_MOVEMENT_FLAGS_BITS		= 8;
