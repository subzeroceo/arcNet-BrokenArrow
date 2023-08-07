#pragma hdrstop
#include "../../idlib/Lib.h"

#include "../Game_local.h"

static const char *channelNames[ ANIM_NumAnimChannels ] = {
	"all", "torso", "legs", "head", "eyelids"
};

/***********************************************************************

	anAnim

***********************************************************************/

/*
=====================
anAnim::anAnim
=====================
*/
anAnim::anAnim() {
	modelDef = nullptr;
	numAnims = 0;
	memset( anims, 0, sizeof( anims ) );
	memset( &flags, 0, sizeof( flags ) );
}

/*
=====================
anAnim::anAnim
=====================
*/
anAnim::anAnim( const anDeclModelDef *modelDef, const anAnim *anim ) {
	this->modelDef = modelDef;
	numAnims = anim->numAnims;
	name = anim->name;
	realname = anim->realname;
	flags = anim->flags;

	memset( anims, 0, sizeof( anims ) );
	for ( i = 0; i < numAnims; i++ ) {
		anims[i] = anim->anims[i];
		anims[i]->IncreaseRefs();
	}

	frameLookup.SetNum( anim->frameLookup.Num() );
	memcpy( frameLookup.Ptr(), anim->frameLookup.Ptr(), frameLookup.MemoryUsed() );

	frameCommands.SetNum( anim->frameCommands.Num() );
	for ( int i = 0; i < frameCommands.Num(); i++ ) {
		frameCommands[i] = anim->frameCommands[i];
		if ( anim->frameCommands[i].string ) {
			frameCommands[i].string = new ( TAG_ANIM ) anStr( *anim->frameCommands[i].string );
		}
	}
}

/*
=====================
anAnim::~anAnim
=====================
*/
anAnim::~anAnim() {
	for ( int i = 0; i < numAnims; i++ ) {
		anims[i]->DecreaseRefs();
	}

	for ( int i = 0; i < frameCommands.Num(); i++ ) {
		delete frameCommands[i].string;
	}
}

/*
=====================
anAnim::SetAnim
=====================
*/
void anAnim::SetAnim( const anDeclModelDef *modelDef, const char *sourcename, const char *animname, int num, const anMD6Anim *md6anims[ ANIM_MaxSyncedAnims ] ) {
	this->modelDef = modelDef;

	for ( i = 0; i < numAnims; i++ ) {
		anims[i]->DecreaseRefs();
		anims[i] = nullptr;
	}

	assert( ( num > 0 ) && ( num <= ANIM_MaxSyncedAnims ) );
	numAnims	= num;
	realname	= sourcename;
	name		= animname;

	for ( int i = 0; i < num; i++ ) {
		anims[i] = md6anims[i];
		anims[i]->IncreaseRefs();
	}

	memset( &flags, 0, sizeof( flags ) );

	for ( int i = 0; i < frameCommands.Num(); i++ ) {
		delete frameCommands[i].string;
	}

	frameLookup.Clear();
	frameCommands.Clear();
}

/*
=====================
anAnim::Name
=====================
*/
const char *anAnim::Name() const {
	return name;
}

/*
=====================
anAnim::FullName
=====================
*/
const char *anAnim::FullName() const {
	return realname;
}

/*
=====================
anAnim::MD6Anim

index 0 will never be nullptr.  Any anim >= NumAnims will return nullptr.
=====================
*/
const anMD6Anim *anAnim::MD6Anim( int num ) const {
	if ( anims[0] == nullptr ) {
		return nullptr;
	}
	return anims[ num ];
}

/*
=====================
anAnim::ModelDef
=====================
*/
const anDeclModelDef *anAnim::ModelDef() const {
	return modelDef;
}

/*
=====================
anAnim::Length
=====================
*/
int anAnim::Length() const {
	if ( !anims[0] ) {
		return 0;
	}

	return anims[0]->Length();
}

/*
=====================
anAnim::NumFrames
=====================
*/
int	anAnim::NumFrames() const {
	if ( !anims[0] ) {
		return 0;
	}

	return anims[0]->NumFrames();
}

/*
=====================
anAnim::NumAnims
=====================
*/
int	anAnim::NumAnims() const {
	return numAnims;
}

/*
=====================
anAnim::TotalMovementDelta
=====================
*/
const anVec3 &anAnim::TotalMovementDelta() const {
	if ( !anims[0] ) {
		return vec3_zero;
	}

	return anims[0]->TotalMovementDelta();
}

/*
=====================
anAnim::GetOrigin
=====================
*/
bool anAnim::GetOrigin( anVec3 &offset, int animNum, int currentTime, int cyclecount ) const {
	if ( !anims[ animNum ] ) {
		offset.Zero();
		return false;
	}

	anims[ animNum ]->GetOrigin( offset, currentTime, cyclecount );
	return true;
}

/*
=====================
anAnim::GetOriginRotation
=====================
*/
bool anAnim::GetOriginRotation( anQuat &rotation, int animNum, int currentTime, int cyclecount ) const {
	if ( !anims[ animNum ] ) {
		rotation.Set( 0.0f, 0.0f, 0.0f, 1.0f );
		return false;
	}

	anims[ animNum ]->GetOriginRotation( rotation, currentTime, cyclecount );
	return true;
}

/*
=====================
anAnim::GetBounds
=====================
*/
inline bool anAnim::GetBounds( anBounds &bounds, int animNum, int currentTime, int cyclecount ) const {
	if ( !anims[ animNum ] ) {
		return false;
	}

	anims[ animNum ]->GetBounds( bounds, currentTime, cyclecount );
	return true;
}


/*
=====================
anAnim::AddFrameCommand

Returns nullptr if no error.
=====================
*/
const char *anAnim::AddFrameCommand( const anDeclModelDef *modelDef, int frameNum, anLexer &src, const anDict *def ) {
	int					i;
	int					index;
	anStr				text;
	anStr				funcname;
	frameCommand_t		fc;
	anToken				token;
	const jointInfo_t	*jointInfo;

	// make sure we're within bounds
	if ( ( frameNum < 1 ) || ( frameNum > anims[0]->NumFrames() ) ) {
		return va( "Frame %d out of range", frameNum );
	}

	// frame numbers are 1 based in .def files, but 0 based internally
	frameNum--;

	memset( &fc, 0, sizeof( fc ) );

	if ( !src.ReadTokenOnLine( &token ) ) {
		return "Unexpected end of line";
	}
	if ( token == "call" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SCRIPTFUNCTION;
		fc.function = gameLocal.program.FindFunction( token );
		if ( !fc.function ) {
			return va( "Function '%s' not found", token.c_str() );
		}
	} else if ( token == "object_call" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SCRIPTFUNCTIONOBJECT;
		fc.string = new ( TAG_ANIM ) anStr( token );
	} else if ( token == "event" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_EVENTFUNCTION;
		const anEventDef *ev = anEventDef::FindEvent( token );
		if ( !ev ) {
			return va( "Event '%s' not found", token.c_str() );
		}
		if ( ev->GetNumArgs() != 0 ) {
			return va( "Event '%s' has arguments", token.c_str() );
		}
		fc.string = new ( TAG_ANIM ) anStr( token );
	} else if ( token == "sound" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new ( TAG_ANIM ) anStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_voice" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_VOICE;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new ( TAG_ANIM ) anStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_voice2" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_VOICE2;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new ( TAG_ANIM ) anStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_body" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_BODY;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new ( TAG_ANIM ) anStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_body2" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_BODY2;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new ( TAG_ANIM ) anStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_body3" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_BODY3;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new ( TAG_ANIM ) anStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_weapon" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_WEAPON;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new ( TAG_ANIM ) anStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_global" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_GLOBAL;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new ( TAG_ANIM ) anStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_item" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_ITEM;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new ( TAG_ANIM ) anStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_chatter" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_CHATTER;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new ( TAG_ANIM ) anStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "skin" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SKIN;
		if ( token == "none" ) {
			fc.skin = nullptr;
		} else {
			fc.skin = declManager->FindSkin( token );
			if ( !fc.skin ) {
				return va( "Skin '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "fx" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_FX;
		if ( !declManager->FindType( DECL_FX, token.c_str() ) ) {
			return va( "fx '%s' not found", token.c_str() );
		}
		fc.string = new ( TAG_ANIM ) anStr( token );
	} else if ( token == "trigger" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_TRIGGER;
		fc.string = new ( TAG_ANIM ) anStr( token );
	} else if ( token == "triggerSmokeParticle" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_TRIGGER_SMOKE_PARTICLE;
		if ( !declManager->FindType( DECL_PARTICLE, token.c_str() ) ) {
			return va( "Particle '%s' not found", token.c_str() );
		}
		fc.string = new ( TAG_ANIM ) anStr( token );
	} else if ( token == "melee" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_MELEE;
		if ( !gameLocal.FindEntityDef( token.c_str(), false ) ) {
			return va( "Unknown entityDef '%s'", token.c_str() );
		}
		fc.string = new ( TAG_ANIM ) anStr( token );
	} else if ( token == "direct_damage" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_DIRECTDAMAGE;
		if ( !gameLocal.FindEntityDef( token.c_str(), false ) ) {
			return va( "Unknown entityDef '%s'", token.c_str() );
		}
		fc.string = new ( TAG_ANIM ) anStr( token );
	} else if ( token == "attack_begin" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_BEGINATTACK;
		if ( !gameLocal.FindEntityDef( token.c_str(), false ) ) {
			return va( "Unknown entityDef '%s'", token.c_str() );
		}
		fc.string = new ( TAG_ANIM ) anStr( token );
	} else if ( token == "attack_end" ) {
		fc.type = FC_ENDATTACK;
	} else if ( token == "muzzle_flash" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		if ( ( token != "" ) && !modelDef->FindJoint( token ) ) {
			return va( "Joint '%s' not found", token.c_str() );
		}
		fc.type = FC_MUZZLEFLASH;
		fc.string = new ( TAG_ANIM ) anStr( token );
	} else if ( token == "create_missile" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		if ( !modelDef->FindJoint( token ) ) {
			return va( "Joint '%s' not found", token.c_str() );
		}
		fc.type = FC_CREATEMISSILE;
		fc.string = new ( TAG_ANIM ) anStr( token );
	} else if ( token == "launch_missile" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		if ( !modelDef->FindJoint( token ) ) {
			return va( "Joint '%s' not found", token.c_str() );
		}
		fc.type = FC_LAUNCHMISSILE;
		fc.string = new ( TAG_ANIM ) anStr( token );
	} else if ( token == "fire_missile_at_target" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		jointInfo = modelDef->FindJoint( token );
		if ( !jointInfo ) {
			return va( "Joint '%s' not found", token.c_str() );
		}
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_FIREMISSILEATTARGET;
		fc.string = new ( TAG_ANIM ) anStr( token );
		fc.index = jointInfo->num;
	} else if ( token == "launch_projectile" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		if ( !declManager->FindDeclWithoutParsing( DECL_ENTITYDEF, token, false ) ) {
			return "Unknown projectile def";
		}
		fc.type = FC_LAUNCH_PROJECTILE;
		fc.string = new ( TAG_ANIM ) anStr( token );
	} else if ( token == "trigger_fx" ) {

		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		jointInfo = modelDef->FindJoint( token );
		if ( !jointInfo ) {
			return va( "Joint '%s' not found", token.c_str() );
		}
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		if ( !declManager->FindType( DECL_FX, token, false ) ) {
			return "Unknown FX def";
		}

		fc.type = FC_TRIGGER_FX;
		fc.string = new ( TAG_ANIM ) anStr( token );
		fc.index = jointInfo->num;

	} else if ( token == "start_emitter" ) {

		anStr str;
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		str = token + " ";

		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		jointInfo = modelDef->FindJoint( token );
		if ( !jointInfo ) {
			return va( "Joint '%s' not found", token.c_str() );
		}
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		if ( !declManager->FindType( DECL_PARTICLE, token.c_str() ) ) {
			return va( "Particle '%s' not found", token.c_str() );
		}
		str += token;
		fc.type = FC_START_EMITTER;
		fc.string = new ( TAG_ANIM ) anStr( str );
		fc.index = jointInfo->num;

	} else if ( token == "stop_emitter" ) {

		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_STOP_EMITTER;
		fc.string = new ( TAG_ANIM ) anStr( token );
	} else if ( token == "footstep" ) {
		fc.type = FC_FOOTSTEP;
	} else if ( token == "leftfoot" ) {
		fc.type = FC_LEFTFOOT;
	} else if ( token == "rightfoot" ) {
		fc.type = FC_RIGHTFOOT;
	} else if ( token == "enableEyeFocus" ) {
		fc.type = FC_ENABLE_EYE_FOCUS;
	} else if ( token == "disableEyeFocus" ) {
		fc.type = FC_DISABLE_EYE_FOCUS;
	} else if ( token == "disableGravity" ) {
		fc.type = FC_DISABLE_GRAVITY;
	} else if ( token == "enableGravity" ) {
		fc.type = FC_ENABLE_GRAVITY;
	} else if ( token == "jump" ) {
		fc.type = FC_JUMP;
	} else if ( token == "enableClip" ) {
		fc.type = FC_ENABLE_CLIP;
	} else if ( token == "disableClip" ) {
		fc.type = FC_DISABLE_CLIP;
	} else if ( token == "enableWalkIK" ) {
		fc.type = FC_ENABLE_WALK_IK;
	} else if ( token == "disableWalkIK" ) {
		fc.type = FC_DISABLE_WALK_IK;
	} else if ( token == "enableLegIK" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_ENABLE_LEG_IK;
		fc.index = atoi( token );
	} else if ( token == "disableLegIK" ) {
		if ( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_DISABLE_LEG_IK;
		fc.index = atoi( token );
	} else if ( token == "recordDemo" ) {
		fc.type = FC_RECORDDEMO;
		if ( src.ReadTokenOnLine( &token ) ) {
			fc.string = new ( TAG_ANIM ) anStr( token );
		}
	} else if ( token == "aviGame" ) {
		fc.type = FC_AVIGAME;
		if ( src.ReadTokenOnLine( &token ) ) {
			fc.string = new ( TAG_ANIM ) anStr( token );
		}
	} else {
		return va( "Unknown command '%s'", token.c_str() );
	}

	// check if we've initialized the frame loopup table
	if ( !frameLookup.Num() ) {
		// we haven't, so allocate the table and initialize it
		frameLookup.SetGranularity( 1 );
		frameLookup.SetNum( anims[0]->NumFrames() );
		for ( i = 0; i < frameLookup.Num(); i++ ) {
			frameLookup[i].num = 0;
			frameLookup[i].firstCommand = 0;
		}
	}

	// allocate space for a new command
	frameCommands.Alloc();

	// calculate the index of the new command
	index = frameLookup[ frameNum ].firstCommand + frameLookup[ frameNum ].num;

	// move all commands from our index onward up one to give us space for our new command
	for ( i = frameCommands.Num() - 1; i > index; i-- ) {
		frameCommands[i] = frameCommands[ i - 1 ];
	}

	// fix the indices of any later frames to account for the inserted command
	for ( i = frameNum + 1; i < frameLookup.Num(); i++ ) {
		frameLookup[i].firstCommand++;
	}

	// store the new command
	frameCommands[index] = fc;

	// increase the number of commands on this frame
	frameLookup[ frameNum ].num++;

	// return with no error
	return nullptr;
}

/*
=====================
anAnim::CallFrameCommands
=====================
*/
void anAnim::CallFrameCommands( anEntity *ent, int from, int to ) const {
	int index;
	int end;
	int frame;
	int numframes;

	numframes = anims[0]->NumFrames();

	frame = from;
	while( frame != to ) {
		frame++;
		if ( frame >= numframes ) {
			frame = 0;
		}

		index = frameLookup[ frame ].firstCommand;
		end = index + frameLookup[ frame ].num;
		while( index < end ) {
			const frameCommand_t &command = frameCommands[ index++ ];
			switch ( command.type ) {
				case FC_SCRIPTFUNCTION: {
					gameLocal.CallFrameCommand( ent, command.function );
					break;
				}
				case FC_SCRIPTFUNCTIONOBJECT: {
					gameLocal.CallObjectFrameCommand( ent, command.string->c_str() );
					break;
				}
				case FC_EVENTFUNCTION: {
					const anEventDef *ev = anEventDef::FindEvent( command.string->c_str() );
					ent->ProcessEvent( ev );
					break;
				}
				case FC_SOUND: {
					if ( !command.soundShader ) {
						if ( !ent->StartSound( command.string->c_str(), SND_CHANNEL_ANY, 0, false, nullptr ) ) {
							gameLocal.Warning( "Framecommand 'sound' on entity '%s', anim '%s', frame %d: Could not find sound '%s'",
								ent->name.c_str(), FullName(), frame + 1, command.string->c_str() );
						}
					} else {
						ent->StartSoundShader( command.soundShader, SND_CHANNEL_ANY, 0, false, nullptr );
					}
					break;
				}
				case FC_SOUND_VOICE: {
					if ( !command.soundShader ) {
						if ( !ent->StartSound( command.string->c_str(), SND_CHANNEL_VOICE, 0, false, nullptr ) ) {
							gameLocal.Warning( "Framecommand 'sound_voice' on entity '%s', anim '%s', frame %d: Could not find sound '%s'",
								ent->name.c_str(), FullName(), frame + 1, command.string->c_str() );
						}
					} else {
						ent->StartSoundShader( command.soundShader, SND_CHANNEL_VOICE, 0, false, nullptr );
					}
					break;
				}
				case FC_SOUND_VOICE2: {
					if ( !command.soundShader ) {
						if ( !ent->StartSound( command.string->c_str(), SND_CHANNEL_VOICE2, 0, false, nullptr ) ) {
							gameLocal.Warning( "Framecommand 'sound_voice2' on entity '%s', anim '%s', frame %d: Could not find sound '%s'",
								ent->name.c_str(), FullName(), frame + 1, command.string->c_str() );
						}
					} else {
						ent->StartSoundShader( command.soundShader, SND_CHANNEL_VOICE2, 0, false, nullptr );
					}
					break;
				}
				case FC_SOUND_BODY: {
					if ( !command.soundShader ) {
						if ( !ent->StartSound( command.string->c_str(), SND_CHANNEL_BODY, 0, false, nullptr ) ) {
							gameLocal.Warning( "Framecommand 'sound_body' on entity '%s', anim '%s', frame %d: Could not find sound '%s'",
								ent->name.c_str(), FullName(), frame + 1, command.string->c_str() );
						}
					} else {
						ent->StartSoundShader( command.soundShader, SND_CHANNEL_BODY, 0, false, nullptr );
					}
					break;
				}
				case FC_SOUND_BODY2: {
					if ( !command.soundShader ) {
						if ( !ent->StartSound( command.string->c_str(), SND_CHANNEL_BODY2, 0, false, nullptr ) ) {
							gameLocal.Warning( "Framecommand 'sound_body2' on entity '%s', anim '%s', frame %d: Could not find sound '%s'",
								ent->name.c_str(), FullName(), frame + 1, command.string->c_str() );
						}
					} else {
						ent->StartSoundShader( command.soundShader, SND_CHANNEL_BODY2, 0, false, nullptr );
					}
					break;
				}
				case FC_SOUND_BODY3: {
					if ( !command.soundShader ) {
						if ( !ent->StartSound( command.string->c_str(), SND_CHANNEL_BODY3, 0, false, nullptr ) ) {
							gameLocal.Warning( "Framecommand 'sound_body3' on entity '%s', anim '%s', frame %d: Could not find sound '%s'",
								ent->name.c_str(), FullName(), frame + 1, command.string->c_str() );
						}
					} else {
						ent->StartSoundShader( command.soundShader, SND_CHANNEL_BODY3, 0, false, nullptr );
					}
					break;
									 }
				case FC_SOUND_WEAPON: {
					if ( !command.soundShader ) {
						if ( !ent->StartSound( command.string->c_str(), SND_CHANNEL_WEAPON, 0, false, nullptr ) ) {
							gameLocal.Warning( "Framecommand 'sound_weapon' on entity '%s', anim '%s', frame %d: Could not find sound '%s'",
								ent->name.c_str(), FullName(), frame + 1, command.string->c_str() );
						}
					} else {
						ent->StartSoundShader( command.soundShader, SND_CHANNEL_WEAPON, 0, false, nullptr );
					}
					break;
				}
				case FC_SOUND_GLOBAL: {
					if ( !command.soundShader ) {
						if ( !ent->StartSound( command.string->c_str(), SND_CHANNEL_ANY, SSF_GLOBAL, false, nullptr ) ) {
							gameLocal.Warning( "Framecommand 'sound_global' on entity '%s', anim '%s', frame %d: Could not find sound '%s'",
								ent->name.c_str(), FullName(), frame + 1, command.string->c_str() );
						}
					} else {
						ent->StartSoundShader( command.soundShader, SND_CHANNEL_ANY, SSF_GLOBAL, false, nullptr );
					}
					break;
				}
				case FC_SOUND_ITEM: {
					if ( !command.soundShader ) {
						if ( !ent->StartSound( command.string->c_str(), SND_CHANNEL_ITEM, 0, false, nullptr ) ) {
							gameLocal.Warning( "Framecommand 'sound_item' on entity '%s', anim '%s', frame %d: Could not find sound '%s'",
								ent->name.c_str(), FullName(), frame + 1, command.string->c_str() );
						}
					} else {
						ent->StartSoundShader( command.soundShader, SND_CHANNEL_ITEM, 0, false, nullptr );
					}
					break;
				}
				case FC_SOUND_CHATTER: {
					if ( ent->CanPlayChatterSounds() ) {
						if ( !command.soundShader ) {
							if ( !ent->StartSound( command.string->c_str(), SND_CHANNEL_VOICE, 0, false, nullptr ) ) {
								gameLocal.Warning( "Framecommand 'sound_chatter' on entity '%s', anim '%s', frame %d: Could not find sound '%s'",
									ent->name.c_str(), FullName(), frame + 1, command.string->c_str() );
							}
						} else {
							ent->StartSoundShader( command.soundShader, SND_CHANNEL_VOICE, 0, false, nullptr );
						}
					}
					break;
				}
				case FC_FX: {
					arcEntityFx::StartFx( command.string->c_str(), nullptr, nullptr, ent, true );
					break;
				}
				case FC_SKIN: {
					ent->SetSkin( command.skin );
					break;
				}
				case FC_TRIGGER: {
					anEntity *target;

					target = gameLocal.FindEntity( command.string->c_str() );
					if ( target ) {
						SetTimeState ts(target->timeGroup);
						target->Signal( SIG_TRIGGER );
						target->ProcessEvent( &EV_Activate, ent );
						target->TriggerGuis();
					} else {
						gameLocal.Warning( "Framecommand 'trigger' on entity '%s', anim '%s', frame %d: Could not find entity '%s'",
							ent->name.c_str(), FullName(), frame + 1, command.string->c_str() );
					}
					break;
				}
				case FC_TRIGGER_SMOKE_PARTICLE: {
					ent->ProcessEvent( &AI_TriggerParticles, command.string->c_str() );
					break;
				}
				case FC_MELEE: {
					ent->ProcessEvent( &AI_AttackMelee, command.string->c_str() );
					break;
				}
				case FC_DIRECTDAMAGE: {
					ent->ProcessEvent( &AI_DirectDamage, command.string->c_str() );
					break;
				}
				case FC_BEGINATTACK: {
					ent->ProcessEvent( &AI_BeginAttack, command.string->c_str() );
					break;
				}
				case FC_ENDATTACK: {
					ent->ProcessEvent( &AI_EndAttack );
					break;
				}
				case FC_MUZZLEFLASH: {
					ent->ProcessEvent( &AI_MuzzleFlash, command.string->c_str() );
					break;
				}
				case FC_CREATEMISSILE: {
					ent->ProcessEvent( &AI_CreateMissile, command.string->c_str() );
					break;
				}
				case FC_LAUNCHMISSILE: {
					ent->ProcessEvent( &AI_AttackMissile, command.string->c_str() );
					break;
				}
				case FC_FIREMISSILEATTARGET: {
					ent->ProcessEvent( &AI_FireMissileAtTarget, modelDef->GetJointName( command.index ), command.string->c_str() );
					break;
				}
				case FC_LAUNCH_PROJECTILE: {
					ent->ProcessEvent( &AI_LaunchProjectile, command.string->c_str() );
					break;
				}
				case FC_TRIGGER_FX: {
					ent->ProcessEvent( &AI_TriggerFX, modelDef->GetJointName( command.index ), command.string->c_str() );
					break;
				}
				case FC_START_EMITTER: {
					int index = command.string->Find( " " );
					if (index >= 0) {
						anStr name = command.string->Left(index);
						anStr particle = command.string->Right(command.string->Length() - index - 1);
						ent->ProcessEvent( &AI_StartEmitter, name.c_str(), modelDef->GetJointName( command.index ), particle.c_str() );
					}
				}

				case FC_STOP_EMITTER: {
					ent->ProcessEvent( &AI_StopEmitter, command.string->c_str() );
				}
				case FC_FOOTSTEP : {
					ent->ProcessEvent( &EV_Footstep );
					break;
				}
				case FC_LEFTFOOT: {
					ent->ProcessEvent( &EV_FootstepLeft );
					break;
				}
				case FC_RIGHTFOOT: {
					ent->ProcessEvent( &EV_FootstepRight );
					break;
				}
				case FC_ENABLE_EYE_FOCUS: {
					ent->ProcessEvent( &AI_EnableEyeFocus );
					break;
				}
				case FC_DISABLE_EYE_FOCUS: {
					ent->ProcessEvent( &AI_DisableEyeFocus );
					break;
				}
				case FC_DISABLE_GRAVITY: {
					ent->ProcessEvent( &AI_DisableGravity );
					break;
				}
				case FC_ENABLE_GRAVITY: {
					ent->ProcessEvent( &AI_EnableGravity );
					break;
				}
				case FC_JUMP: {
					ent->ProcessEvent( &AI_JumpFrame );
					break;
				}
				case FC_ENABLE_CLIP: {
					ent->ProcessEvent( &AI_EnableClip );
					break;
				}
				case FC_DISABLE_CLIP: {
					ent->ProcessEvent( &AI_DisableClip );
					break;
				}
				case FC_ENABLE_WALK_IK: {
					ent->ProcessEvent( &EV_EnableWalkIK );
					break;
				}
				case FC_DISABLE_WALK_IK: {
					ent->ProcessEvent( &EV_DisableWalkIK );
					break;
				}
				case FC_ENABLE_LEG_IK: {
					ent->ProcessEvent( &EV_EnableLegIK, command.index );
					break;
				}
				case FC_DISABLE_LEG_IK: {
					ent->ProcessEvent( &EV_DisableLegIK, command.index );
					break;
				}
				case FC_RECORDDEMO: {
					if ( command.string ) {
						cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "recordDemo %s", command.string->c_str() ) );
					} else {
						cmdSystem->BufferCommandText( CMD_EXEC_NOW, "stoprecording" );
					}
					break;
				}
				case FC_AVIGAME: {
					if ( command.string ) {
						cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "aviGame %s", command.string->c_str() ) );
					} else {
						cmdSystem->BufferCommandText( CMD_EXEC_NOW, "aviGame" );
					}
					break;
				}
			}
		}
	}
}

/*
=====================
anAnim::FindFrameForFrameCommand
=====================
*/
int	anAnim::FindFrameForFrameCommand( frameCommandType_t framecommand, const frameCommand_t **command ) const {
	int frame;
	int index;
	int numframes;
	int end;

	if ( !frameCommands.Num() ) {
		return -1;
	}

	numframes = anims[0]->NumFrames();
	for ( frame = 0; frame < numframes; frame++ ) {
		end = frameLookup[ frame ].firstCommand + frameLookup[ frame ].num;
		for ( index = frameLookup[ frame ].firstCommand; index < end; index++ ) {
			if ( frameCommands[index].type == framecommand ) {
				if ( command ) {
					*command = &frameCommands[index];
				}
				return frame;
			}
		}
	}

	if ( command ) {
		*command = nullptr;
	}

	return -1;
}

/*
=====================
anAnim::HasFrameCommands
=====================
*/
bool anAnim::HasFrameCommands() const {
	if ( !frameCommands.Num() ) {
		return false;
	}
	return true;
}

/*
=====================
anAnim::SetAnimFlags
=====================
*/
void anAnim::SetAnimFlags( const animFlags_t &animflags ) {
	flags = animflags;
}

/*
=====================
anAnim::GetAnimFlags
=====================
*/
const animFlags_t &anAnim::GetAnimFlags() const {
	return flags;
}

/***********************************************************************

	anAnimBlend

***********************************************************************/

/*
=====================
anAnimBlend::anAnimBlend
=====================
*/
anAnimBlend::() {
	Reset( nuanAnimBlendllptr );
}

/*
=====================
anAnimBlend::Save

archives object for save game file
=====================
*/
void anAnimBlend::Save( anSaveGame *savefile ) const {
	savefile->WriteInt( starttime );
	savefile->WriteInt( endtime );
	savefile->WriteInt( timeOffset );
	savefile->WriteFloat( rate );

	savefile->WriteInt( blendStartTime );
	savefile->WriteInt( blendDuration );
	savefile->WriteFloat( blendStartValue );
	savefile->WriteFloat( blendEndValue );

	for ( int i = 0; i < ANIM_MaxSyncedAnims; i++ ) {
		savefile->WriteFloat( animWeights[i] );
	}
	savefile->WriteShort( cycle );
	savefile->WriteShort( frame );
	savefile->WriteShort( animNum );
	savefile->WriteBool( allowMove );
	savefile->WriteBool( allowFrameCommands );
}

/*
=====================
anAnimBlend::Restore

unarchives object from save game file
=====================
*/
void anAnimBlend::Restore( anRestoreGame *savefile, const anDeclModelDef *modelDef ) {
	this->modelDef = modelDef;

	savefile->ReadInt( starttime );
	savefile->ReadInt( endtime );
	savefile->ReadInt( timeOffset );
	savefile->ReadFloat( rate );

	savefile->ReadInt( blendStartTime );
	savefile->ReadInt( blendDuration );
	savefile->ReadFloat( blendStartValue );
	savefile->ReadFloat( blendEndValue );

	for ( int i = 0; i < ANIM_MaxSyncedAnims; i++ ) {
		savefile->ReadFloat( animWeights[i] );
	}
	savefile->ReadShort( cycle );
	savefile->ReadShort( frame );
	savefile->ReadShort( animNum );
	if ( !modelDef ) {
		animNum = 0;
	} else if ( ( animNum < 0 ) || ( animNum > modelDef->NumAnims() ) ) {
		gameLocal.Warning( "Anim number %d out of range for model '%s' during save game", animNum, modelDef->GetModelName() );
		animNum = 0;
	}
	savefile->ReadBool( allowMove );
	savefile->ReadBool( allowFrameCommands );
}

/*
=====================
anAnimBlend::Reset
=====================
*/
void anAnimBlend::Reset( const anDeclModelDef *_modelDef ) {
	modelDef	= _modelDef;
	cycle		= 1;
	starttime	= 0;
	endtime		= 0;
	timeOffset	= 0;
	rate		= 1.0f;
	frame		= 0;
	allowMove	= true;
	allowFrameCommands = true;
	animNum		= 0;

	memset( animWeights, 0, sizeof( animWeights ) );

	blendStartValue = 0.0f;
	blendEndValue	= 0.0f;
    blendStartTime	= 0;
	blendDuration	= 0;
}

/*
=====================
anAnimBlend::FullName
=====================
*/
const char *anAnimBlend::AnimFullName() const {
	const anAnim *anim = Anim();
	if ( !anim ) {
		return "";
	}

	return anim->FullName();
}

/*
=====================
anAnimBlend::AnimName
=====================
*/
const char *anAnimBlend::AnimName() const {
	const anAnim *anim = Anim();
	if ( !anim ) {
		return "";
	}

	return anim->Name();
}

/*
=====================
anAnimBlend::NumFrames
=====================
*/
int anAnimBlend::NumFrames() const {
	const anAnim *anim = Anim();
	if ( !anim ) {
		return 0;
	}

	return anim->NumFrames();
}

/*
=====================
anAnimBlend::Length
=====================
*/
int	anAnimBlend::Length() const {
	const anAnim *anim = Anim();
	if ( !anim ) {
		return 0;
	}

	return anim->Length();
}

/*
=====================
anAnimBlend::GetWeight
=====================
*/
float anAnimBlend::GetWeight( int currentTime ) const {
	float frac;

	int timeDelta = currentTime - blendStartTime;
	if ( timeDelta <= 0 ) {
		float w = blendStartValue;
	} else if ( timeDelta >= blendDuration ) {
		float w = blendEndValue;
	} else {
		frac = ( float )timeDelta / ( float )blendDuration;
		float w = blendStartValue + ( blendEndValue - blendStartValue ) * frac;
	}

	return w;
}

/*
=====================
anAnimBlend::GetFinalWeight
=====================
*/
float anAnimBlend::GetFinalWeight() const {
	return blendEndValue;
}

/*
=====================
anAnimBlend::SetWeight
=====================
*/
void anAnimBlend::SetWeight( float newweight, int currentTime, int blendTime ) {
	blendStartValue = GetWeight( currentTime );
	blendEndValue = newweight;
    blendStartTime = currentTime - 1;
	blendDuration = blendTime;

	if ( !newweight ) {
		endtime = currentTime + blendTime;
	}
}

/*
=====================
anAnimBlend::NumSyncedAnims
=====================
*/
int anAnimBlend::NumSyncedAnims() const {
	const anAnim *anim = Anim();
	if ( !anim ) {
		return 0;
	}

	return anim->NumAnims();
}

/*
=====================
anAnimBlend::SetSyncedAnimWeight
=====================
*/
bool anAnimBlend::SetSyncedAnimWeight( int num, float weight ) {
	const anAnim *anim = Anim();
	if ( !anim ) {
		return false;
	}

	if ( ( num < 0 ) || ( num > anim->NumAnims() ) ) {
		return false;
	}

	animWeights[ num ] = weight;
	return true;
}

/*
=====================
anAnimBlend::SetFrame
=====================
*/
void anAnimBlend::SetFrame( const anDeclModelDef *modelDef, int _animNum, int _frame, int currentTime, int blendTime ) {
	Reset( modelDef );
	if ( !modelDef ) {
		return;
	}

	const anAnim *_anim = modelDef->GetAnim( _animNum );
	if ( !_anim ) {
		return;
	}

	const anMD6Anim *md6anim = _anim->MD6Anim( 0 );
	if ( modelDef->Joints().Num() != md6anim->NumJoints() ) {
		gameLocal.Warning( "Model '%s' has different # of joints than anim '%s'", modelDef->GetModelName(), md6anim->Name() );
		return;
	}

	animNum				= _animNum;
	starttime			= currentTime;
	endtime				= -1;
	cycle				= -1;
	animWeights[0]	= 1.0f;
	frame				= _frame;

	// a frame of 0 means it's not a single frame blend, so we set it to frame + 1
	if ( frame <= 0 ) {
		frame = 1;
	} else if ( frame > _anim->NumFrames() ) {
		frame = _anim->NumFrames();
	}

	// set up blend
	blendEndValue		= 1.0f;
	blendStartTime		= currentTime - 1;
	blendDuration		= blendTime;
	blendStartValue		= 0.0f;
}

/*
=====================
anAnimBlend::CycleAnim
=====================
*/
void anAnimBlend::CycleAnim( const anDeclModelDef *modelDef, int _animNum, int currentTime, int blendTime ) {
	Reset( modelDef );
	if ( !modelDef ) {
		return;
	}

	const anAnim *_anim = modelDef->GetAnim( _animNum );
	if ( !_anim ) {
		return;
	}

	const anMD6Anim *md6anim = _anim->MD6Anim( 0 );
	if ( modelDef->Joints().Num() != md6anim->NumJoints() ) {
		gameLocal.Warning( "Model '%s' has different # of joints than anim '%s'", modelDef->GetModelName(), md6anim->Name() );
		return;
	}

	animNum				= _animNum;
	animWeights[0]	= 1.0f;
	endtime				= -1;
	cycle				= -1;
	if ( _anim->GetAnimFlags().random_cycle_start ) {
		// start the animation at a random time so that characters don't walk in sync
		starttime = currentTime - gameLocal.random.RandomFloat() * _anim->Length();
	} else {
		starttime = currentTime;
	}

	// set up blend
	blendEndValue		= 1.0f;
	blendStartTime		= currentTime - 1;
	blendDuration		= blendTime;
	blendStartValue		= 0.0f;
}

/*
=====================
anAnimBlend::PlayAnim
=====================
*/
void anAnimBlend::PlayAnim( const anDeclModelDef *modelDef, int _animNum, int currentTime, int blendTime ) {
	Reset( modelDef );
	if ( !modelDef ) {
		return;
	}

	const anAnim *_anim = modelDef->GetAnim( _animNum );
	if ( !_anim ) {
		return;
	}

	const anMD6Anim *md6anim = _anim->MD6Anim( 0 );
	if ( modelDef->Joints().Num() != md6anim->NumJoints() ) {
		gameLocal.Warning( "Model '%s' has different # of joints than anim '%s'", modelDef->GetModelName(), md6anim->Name() );
		return;
	}

	animNum				= _animNum;
	starttime			= currentTime;
	endtime				= starttime + _anim->Length();
	cycle				= 1;
	animWeights[0]	= 1.0f;

	// set up blend
	blendEndValue		= 1.0f;
	blendStartTime		= currentTime - 1;
	blendDuration		= blendTime;
	blendStartValue		= 0.0f;
}

/*
=====================
anAnimBlend::Clear
=====================
*/
void anAnimBlend::Clear( int currentTime, int clearTime ) {
	if ( !clearTime ) {
		Reset( modelDef );
	} else {
		SetWeight( 0.0f, currentTime, clearTime );
	}
}

/*
=====================
anAnimBlend::IsDone
=====================
*/
bool anAnimBlend::IsDone( int currentTime ) const {
	if ( !frame && ( endtime > 0 ) && ( currentTime >= endtime ) ) {
		return true;
	}

	if ( ( blendEndValue <= 0.0f ) && ( currentTime >= ( blendStartTime + blendDuration ) ) ) {
		return true;
	}

	return false;
}

/*
=====================
anAnimBlend::FrameHasChanged
=====================
*/
bool anAnimBlend::FrameHasChanged( int currentTime ) const {
	// if we don't have an anim, no change
	if ( !animNum ) {
		return false;
	}

	// if anim is done playing, no change
	if ( ( endtime > 0 ) && ( currentTime > endtime ) ) {
		return false;
	}

	// if our blend weight changes, we need to update
	if ( ( currentTime < ( blendStartTime + blendDuration ) && ( blendStartValue != blendEndValue ) ) ) {
		return true;
	}

	// if we're a single frame anim and this isn't the frame we started on, we don't need to update
	if ( ( frame || ( NumFrames() == 1 ) ) && ( currentTime != starttime ) ) {
		return false;
	}

	return true;
}

/*
=====================
anAnimBlend::GetCycleCount
=====================
*/
int anAnimBlend::GetCycleCount() const {
	return cycle;
}

/*
=====================
anAnimBlend::SetCycleCount
=====================
*/
void anAnimBlend::SetCycleCount( int count ) {
	const anAnim *anim = Anim();

	if ( !anim ) {
		cycle = -1;
		endtime = 0;
	} else {
		cycle = count;
		if ( cycle < 0 ) {
			cycle = -1;
			endtime	= -1;
		} else if ( cycle == 0 ) {
			cycle = 1;

			// most of the time we're running at the original frame rate, so avoid the int-to-float-to-int conversion
			if ( rate == 1.0f ) {
				endtime	= starttime - timeOffset + anim->Length();
			} else if ( rate != 0.0f ) {
				endtime	= starttime - timeOffset + anim->Length() / rate;
			} else {
				endtime = -1;
			}
		} else {
			// most of the time we're running at the original frame rate, so avoid the int-to-float-to-int conversion
			if ( rate == 1.0f ) {
				endtime	= starttime - timeOffset + anim->Length() * cycle;
			} else if ( rate != 0.0f ) {
				endtime	= starttime - timeOffset + ( anim->Length() * cycle ) / rate;
			} else {
				endtime = -1;
			}
		}
	}
}

/*
=====================
anAnimBlend::SetPlaybackRate
=====================
*/
void anAnimBlend::SetPlaybackRate( int currentTime, float newRate ) {
	int animTime;

	if ( rate == newRate ) {
		return;
	}

	animTime = AnimTime( currentTime );
	if ( newRate == 1.0f ) {
		timeOffset = animTime - ( currentTime - starttime );
	} else {
		timeOffset = animTime - ( currentTime - starttime ) * newRate;
	}

	rate = newRate;

	// update the anim endtime
	SetCycleCount( cycle );
}

/*
=====================
anAnimBlend::GetPlaybackRate
=====================
*/
float anAnimBlend::GetPlaybackRate() const {
	return rate;
}

/*
=====================
anAnimBlend::SetStartTime
=====================
*/
void anAnimBlend::SetStartTime( int _startTime ) {
	starttime = _startTime;

	// update the anim endtime
	SetCycleCount( cycle );
}

/*
=====================
anAnimBlend::GetStartTime
=====================
*/
int anAnimBlend::GetStartTime() const {
	if ( !animNum ) {
		return 0;
	}

	return starttime;
}

/*
=====================
anAnimBlend::GetEndTime
=====================
*/
int anAnimBlend::GetEndTime() const {
	if ( !animNum ) {
		return 0;
	}

	return endtime;
}

/*
=====================
anAnimBlend::PlayLength
=====================
*/
int anAnimBlend::PlayLength() const {
	if ( !animNum ) {
		return 0;
	}

	if ( endtime < 0 ) {
		return -1;
	}

	return endtime - starttime + timeOffset;
}

/*
=====================
anAnimBlend::AllowMovement
=====================
*/
void anAnimBlend::AllowMovement( bool allow ) {
	allowMove = allow;
}

/*
=====================
anAnimBlend::AllowFrameCommands
=====================
*/
void anAnimBlend::AllowFrameCommands( bool allow ) {
	allowFrameCommands = allow;
}


/*
=====================
anAnimBlend::Anim
=====================
*/
const anAnim *anAnimBlend::Anim() const {
	if ( !modelDef ) {
		return nullptr;
	}

	const anAnim *anim = modelDef->GetAnim( animNum );
	return anim;
}

/*
=====================
anAnimBlend::AnimNum
=====================
*/
int anAnimBlend::AnimNum() const {
	return animNum;
}

/*
=====================
anAnimBlend::AnimTime
=====================
*/
int anAnimBlend::AnimTime( int currentTime ) const {
	int time;
	int length;
	const anAnim *anim = Anim();

	if ( anim ) {
		if ( frame ) {
			return FRAME2MS( frame - 1 );
		}

		// most of the time we're running at the original frame rate, so avoid the int-to-float-to-int conversion
		if ( rate == 1.0f ) {
			time = currentTime - starttime + timeOffset;
		} else {
			time = static_cast<int>( ( currentTime - starttime ) * rate ) + timeOffset;
		}

		// given enough time, we can easily wrap time around in our frame calculations, so
		// keep cycling animations' time within the length of the anim.
		length = anim->Length();
		if ( ( cycle < 0 ) && ( length > 0 ) ) {
			time %= length;

			// time will wrap after 24 days (oh no!), resulting in negative results for the %.
			// adding the length gives us the proper result.
			if ( time < 0 ) {
				time += length;
			}
		}
		return time;
	} else {
		return 0;
	}
}

/*
=====================
anAnimBlend::GetFrameNumber
=====================
*/
int anAnimBlend::GetFrameNumber( int currentTime ) const {
	const anMD6Anim	*md6anim;
	frameBlend_t	frameinfo;
	int				animTime;

	const anAnim *anim = Anim();
	if ( !anim ) {
		return 1;
	}

	if ( frame ) {
		return frame;
	}

	md6anim = anim->MD6Anim( 0 );
	animTime = AnimTime( currentTime );
	md6anim->ConvertTimeToFrame( animTime, cycle, frameinfo );

	return frameinfo.frame1 + 1;
}

/*
=====================
anAnimBlend::CallFrameCommands
=====================
*/
void anAnimBlend::CallFrameCommands( anEntity *ent, int fromtime, int totime ) const {
	const anMD6Anim	*md6anim;
	frameBlend_t	frame1;
	frameBlend_t	frame2;
	int				fromFrameTime;
	int				toFrameTime;

	if ( !allowFrameCommands || !ent || frame || ( ( endtime > 0 ) && ( fromtime > endtime ) ) ) {
		return;
	}

	const anAnim *anim = Anim();
	if ( !anim || !anim->HasFrameCommands() ) {
		return;
	}

	if ( totime <= starttime ) {
		// don't play until next frame or we'll play commands twice.
		// this happens on the player sometimes.
		return;
	}

	fromFrameTime	= AnimTime( fromtime );
	toFrameTime		= AnimTime( totime );
	if ( toFrameTime < fromFrameTime ) {
		toFrameTime += anim->Length();
	}

	md6anim = anim->MD6Anim( 0 );
	md6anim->ConvertTimeToFrame( fromFrameTime, cycle, frame1 );
	md6anim->ConvertTimeToFrame( toFrameTime, cycle, frame2 );

	if ( fromFrameTime <= 0 ) {
		// make sure first frame is called
		anim->CallFrameCommands( ent, -1, frame2.frame1 );
	} else {
		anim->CallFrameCommands( ent, frame1.frame1, frame2.frame1 );
	}
}

/*
=====================
anAnimBlend::BlendAnim
=====================
*/
bool anAnimBlend::BlendAnim( int currentTime, int channel, int numJoints, anJointQuat *blendFrame, float &blendWeight, bool removeOriginOffset, bool overrideBlend, bool printInfo ) const {
	int				i;
	float			lerp;
	float			mixWeight;
	const anMD6Anim	*md6anim;
	anJointQuat		*ptr;
	frameBlend_t	frametime = { 0 };
	anJointQuat		*jointFrame;
	anJointQuat		*mixFrame;
	int				numAnims;
	int				time;

	const anAnim *anim = Anim();
	if ( !anim ) {
		return false;
	}

	float weight = GetWeight( currentTime );
	if ( blendWeight > 0.0f ) {
		if ( ( endtime >= 0 ) && ( currentTime >= endtime ) ) {
			return false;
		}
		if ( !weight ) {
			return false;
		}
		if ( overrideBlend ) {
			blendWeight = 1.0f - weight;
		}
	}

	if ( ( channel == ANIMCHANNEL_ALL ) && !blendWeight ) {
		// we don't need a temporary buffer, so just store it directly in the blend frame
		jointFrame = blendFrame;
	} else {
		// allocate a temporary buffer to copy the joints from
		jointFrame = ( anJointQuat * )_alloca16( numJoints * sizeof( *jointFrame ) );
	}

	time = AnimTime( currentTime );

	numAnims = anim->NumAnims();
	if ( numAnims == 1 ) {
		md6anim = anim->MD6Anim( 0 );
		if ( frame ) {
			md6anim->GetSingleFrame( frame - 1, jointFrame, modelDef->GetChannelJoints( channel ), modelDef->NumJointsOnChannel( channel ) );
		} else {
			md6anim->ConvertTimeToFrame( time, cycle, frametime );
			md6anim->GetInterpolatedFrame( frametime, jointFrame, modelDef->GetChannelJoints( channel ), modelDef->NumJointsOnChannel( channel ) );
		}
	} else {
		//
		// need to mix the multipoint anim together first
		//
		// allocate a temporary buffer to copy the joints to
		mixFrame = ( anJointQuat * )_alloca16( numJoints * sizeof( *jointFrame ) );

		if ( !frame ) {
			anim->MD6Anim( 0 )->ConvertTimeToFrame( time, cycle, frametime );
		}

		ptr = jointFrame;
		mixWeight = 0.0f;
		for ( i = 0; i < numAnims; i++ ) {
			if ( animWeights[i] > 0.0f ) {
				mixWeight += animWeights[i];
				lerp = animWeights[i] / mixWeight;
				md6anim = anim->MD6Anim( i );
				if ( frame ) {
					md6anim->GetSingleFrame( frame - 1, ptr, modelDef->GetChannelJoints( channel ), modelDef->NumJointsOnChannel( channel ) );
				} else {
					md6anim->GetInterpolatedFrame( frametime, ptr, modelDef->GetChannelJoints( channel ), modelDef->NumJointsOnChannel( channel ) );
				}

				// only blend after the first anim is mixed in
				if ( ptr != jointFrame ) {
					SIMDProcessor->BlendJoints( jointFrame, ptr, lerp, modelDef->GetChannelJoints( channel ), modelDef->NumJointsOnChannel( channel ) );
				}

				ptr = mixFrame;
			}
		}

		if ( !mixWeight ) {
			return false;
		}
	}

	if ( removeOriginOffset ) {
		if ( allowMove ) {
#ifdef VELOCITY_MOVE
			jointFrame[0].t.x = 0.0f;
#else
			jointFrame[0].t.Zero();
#endif
		}

		if ( anim->GetAnimFlags().anim_turn ) {
			jointFrame[0].q.Set( -0.70710677f, 0.0f, 0.0f, 0.70710677f );
		}
	}

	if ( !blendWeight ) {
		blendWeight = weight;
		if ( channel != ANIMCHANNEL_ALL ) {
			const int *index = modelDef->GetChannelJoints( channel );
			const int num = modelDef->NumJointsOnChannel( channel );
			for ( i = 0; i < num; i++ ) {
				int j = index[i];
				blendFrame[j].t = jointFrame[j].t;
				blendFrame[j].q = jointFrame[j].q;
			}
		}
    } else {
		blendWeight += weight;
		lerp = weight / blendWeight;
		SIMDProcessor->BlendJoints( blendFrame, jointFrame, lerp, modelDef->GetChannelJoints( channel ), modelDef->NumJointsOnChannel( channel ) );
	}

	if ( printInfo ) {
		if ( frame ) {
			gameLocal.Printf( "  %s: '%s', %d, %.2f%%\n", channelNames[ channel ], anim->FullName(), frame, weight * 100.0f );
		} else {
			gameLocal.Printf( "  %s: '%s', %.3f, %.2f%%\n", channelNames[ channel ], anim->FullName(), ( float )frametime.frame1 + frametime.backlerp, weight * 100.0f );
		}
	}

	return true;
}

/*
=====================
anAnimBlend::BlendOrigin
=====================
*/
void anAnimBlend::BlendOrigin( int currentTime, anVec3 &blendPos, float &blendWeight, bool removeOriginOffset ) const {
	float	lerp;
	anVec3	animpos;
	anVec3	pos;
	int		time;
	int		num;
	int		i;

	if ( frame || ( ( endtime > 0 ) && ( currentTime > endtime ) ) ) {
		return;
	}

	const anAnim *anim = Anim();
	if ( !anim ) {
		return;
	}

	if ( allowMove && removeOriginOffset ) {
		return;
	}

	float weight = GetWeight( currentTime );
	if ( !weight ) {
		return;
	}

	time = AnimTime( currentTime );

	pos.Zero();
	num = anim->NumAnims();
	for ( i = 0; i < num; i++ ) {
		anim->GetOrigin( animpos, i, time, cycle );
		pos += animpos * animWeights[i];
	}

	if ( !blendWeight ) {
		blendPos = pos;
		blendWeight = weight;
	} else {
		lerp = weight / ( blendWeight + weight );
		blendPos += lerp * ( pos - blendPos );
		blendWeight += weight;
	}
}

/*
=====================
anAnimBlend::BlendDelta
=====================
*/
void anAnimBlend::BlendDelta( int fromtime, int totime, anVec3 &blendDelta, float &blendWeight ) const {
	anVec3	animpos;

	if ( frame || !allowMove || ( ( endtime > 0 ) && ( fromtime > endtime ) ) ) {
		return;
	}

	const anAnim *anim = Anim();
	if ( !anim ) {
		return;
	}

	float weight = GetWeight( totime );
	if ( !weight ) {
		return;
	}

	int time1 = AnimTime( fromtime ), time2 = AnimTime( totime );
	if ( time2 < time1 ) {
		time2 += anim->Length();
	}

	int num = anim->NumAnims();

	anVec3 pos1.Zero();
	anVec3 pos2.Zero();
	for ( int i = 0; i < num; i++ ) {
		anim->GetOrigin( animpos, i, time1, cycle );
		pos1 += animpos * animWeights[i];

		anim->GetOrigin( animpos, i, time2, cycle );
		pos2 += animpos * animWeights[i];
	}

	anVec3 delta = pos2 - pos1;
	if ( !blendWeight ) {
		blendDelta = delta;
		blendWeight = weight;
	} else {
		float lerp = weight / ( blendWeight + weight );
		blendDelta += lerp * ( delta - blendDelta );
		blendWeight += weight;
	}
}

/*
=====================
anAnimBlend::BlendDeltaRotation
=====================
*/
void anAnimBlend::BlendDeltaRotation( int fromtime, int totime, anQuat &blendDelta, float &blendWeight ) const {
	anQuat	q1;
	anQuat	q2;
	anQuat	q3;

	if ( frame || !allowMove || ( ( endtime > 0 ) && ( fromtime > endtime ) ) ) {
		return;
	}

	const anAnim *anim = Anim();
	if ( !anim || !anim->GetAnimFlags().anim_turn ) {
		return;
	}

	float weight = GetWeight( totime );
	if ( !weight ) {
		return;
	}

	int time1 = AnimTime( fromtime );
	int time2 = AnimTime( totime );
	if ( time2 < time1 ) {
		time2 += anim->Length();
	}

	q1.Set( 0.0f, 0.0f, 0.0f, 1.0f );
	q2.Set( 0.0f, 0.0f, 0.0f, 1.0f );

	float mixWeight = 0.0f;
	int num = anim->NumAnims();
	for ( int i = 0; i < num; i++ ) {
		if ( animWeights[i] > 0.0f ) {
			mixWeight += animWeights[i];
			if ( animWeights[i] == mixWeight ) {
				anim->GetOriginRotation( q1, i, time1, cycle );
				anim->GetOriginRotation( q2, i, time2, cycle );
			} else {
				float lerp = animWeights[i] / mixWeight;
				anim->GetOriginRotation( q3, i, time1, cycle );
				q1.Slerp( q1, q3, lerp );

				anim->GetOriginRotation( q3, i, time2, cycle );
				q2.Slerp( q1, q3, lerp );
			}
		}
	}

	q3 = q1.Inverse() * q2;
	if ( !blendWeight ) {
		blendDelta = q3;
		blendWeight = weight;
	} else {
		float lerp = weight / ( blendWeight + weight );
		blendDelta.Slerp( blendDelta, q3, lerp );
		blendWeight += weight;
	}
}

/*
=====================
anAnimBlend::AddBounds
=====================
*/
bool anAnimBlend::AddBounds( int currentTime, anBounds &bounds, bool removeOriginOffset ) const {
	int			i;
	int			num;
	anBounds	b;
	int			time;
	anVec3		pos;
	bool		addorigin;

	if ( ( endtime > 0 ) && ( currentTime > endtime ) ) {
		return false;
	}

	const anAnim *anim = Anim();
	if ( !anim ) {
		return false;
	}

	float weight = GetWeight( currentTime );
	if ( !weight ) {
		return false;
	}

	time = AnimTime( currentTime );
	num = anim->NumAnims();

	addorigin = !allowMove || !removeOriginOffset;
	for ( i = 0; i < num; i++ ) {
		if ( anim->GetBounds( b, i, time, cycle ) ) {
			if ( addorigin ) {
				anim->GetOrigin( pos, i, time, cycle );
				b.TranslateSelf( pos );
			}
			bounds.AddBounds( b );
		}
	}

	return true;
}

/***********************************************************************

	anDeclModelDef

***********************************************************************/

/*
=====================
anDeclModelDef::anDeclModelDef
=====================
*/
anDeclModelDef::anDeclModelDef() {
	modelHandle	= nullptr;
	skin		= nullptr;
	offset.Zero();
	for ( int i = 0; i < ANIM_NumAnimChannels; i++ ) {
		channelJoints[i].Clear();
	}
}

/*
=====================
anDeclModelDef::~anDeclModelDef
=====================
*/
anDeclModelDef::~anDeclModelDef() {
	FreeData();
}

/*
=================
anDeclModelDef::Size
=================
*/
size_t anDeclModelDef::Size() const {
	return sizeof( anDeclModelDef );
}

/*
=====================
anDeclModelDef::CopyDecl
=====================
*/
void anDeclModelDef::CopyDecl( const anDeclModelDef *decl ) {
	FreeData();

	offset = decl->offset;
	modelHandle = decl->modelHandle;
	skin = decl->skin;

	anims.SetNum( decl->anims.Num() );
	for ( int i = 0; i < anims.Num(); i++ ) {
		anims[i] = new ( TAG_ANIM ) anAnim( this, decl->anims[i] );
	}

	joints.SetNum( decl->joints.Num() );
	memcpy( joints.Ptr(), decl->joints.Ptr(), decl->joints.Num() * sizeof( joints[0] ) );
	jointParents.SetNum( decl->jointParents.Num() );
	memcpy( jointParents.Ptr(), decl->jointParents.Ptr(), decl->jointParents.Num() * sizeof( jointParents[0] ) );
	for ( int i = 0; i < ANIM_NumAnimChannels; i++ ) {
		channelJoints[i] = decl->channelJoints[i];
	}
}

/*
=====================
anDeclModelDef::FreeData
=====================
*/
void anDeclModelDef::FreeData() {
	anims.DeleteContents( true );
	joints.Clear();
	jointParents.Clear();
	modelHandle	= nullptr;
	skin = nullptr;
	offset.Zero();
	for ( int i = 0; i < ANIM_NumAnimChannels; i++ ) {
		channelJoints[i].Clear();
	}
}

/*
================
anDeclModelDef::DefaultDefinition
================
*/
const char *anDeclModelDef::DefaultDefinition() const {
	return "{ }";
}

/*
====================
anDeclModelDef::FindJoint
====================
*/
const jointInfo_t *anDeclModelDef::FindJoint( const char *name ) const {
	if ( !modelHandle ) {
		return nullptr;
	}

	const anMD6Joint *jjoint = modelHandle->GetJoints();
	for ( int i = 0; i < joints.Num(); i++, joint++ ) {
		if ( !joint->name.Icmp( name ) ) {
			return &joints[i];
		}
	}

	return nullptr;
}

/*
=====================
anDeclModelDef::ModelHandle
=====================
*/
anRenderModel *anDeclModelDef::ModelHandle() const {
	return (anRenderModel *)modelHandle;
}

/*
=====================
anDeclModelDef::GetJointList
=====================
*/
void anDeclModelDef::GetJointList( const char *jointNames, anList<jointHandle_t> &jointList ) const {
	anStr				jointName;
	const jointInfo_t	*joint, *child;
	bool				getChildren, subtract;

	if ( !modelHandle ) {
		return;
	}

	jointList.Clear();

	int num = modelHandle->NumJoints();

	// scan through list of joints and add each to the joint list
	const char *pos = jointNames;
	while( *pos ) {
		// skip over whitespace
		while( ( *pos != 0 ) && isspace( (unsigned char)*pos ) ) {
			pos++;
		}

		if ( !*pos ) {
			// no more names
			break;
		}

		// copy joint name
		jointName = "";

		if ( *pos == '-' ) {
			subtract = true;
			pos++;
		} else {
			subtract = false;
		}

		if ( *pos == '*' ) {
			getChildren = true;
			pos++;
		} else {
			getChildren = false;
		}

		while( ( *pos != 0 ) && !isspace( (unsigned char)*pos ) ) {
			jointName += *pos;
			pos++;
		}

		joint = FindJoint( jointName );
		if ( !joint ) {
			gameLocal.Warning( "Unknown joint '%s' in '%s' for model '%s'", jointName.c_str(), jointNames, GetName() );
			continue;
		}

		if ( !subtract ) {
			jointList.AddUnique( joint->num );
		} else {
			jointList.Remove( joint->num );
		}

		if ( getChildren ) {
			// include all joint's children
			child = joint + 1;
			for ( int i = joint->num + 1; i < num; i++, child++ ) {
				// all children of the joint should follow it in the list.
				// once we reach a joint without a parent or with a parent
				// who is earlier in the list than the specified joint, then
				// we've gone through all it's children.
				if ( child->parentNum < joint->num ) {
					break;
				}

				if ( !subtract ) {
					jointList.AddUnique( child->num );
				} else {
					jointList.Remove( child->num );
				}
			}
		}
	}
}

/*
=====================
anDeclModelDef::Touch
=====================
*/
void anDeclModelDef::Touch() const {
	if ( modelHandle ) {
		renderModelManager->FindModel( modelHandle->Name() );
	}
}

/*
=====================
anDeclModelDef::GetDefaultSkin
=====================
*/
const anDeclSkin *anDeclModelDef::GetDefaultSkin() const {
	return skin;
}

/*
=====================
anDeclModelDef::GetDefaultPose
=====================
*/
const anJointQuat *anDeclModelDef::GetDefaultPose() const {
	return modelHandle->GetDefaultPose();
}

/*
=====================
anDeclModelDef::SetupJoints
=====================
*/
void anDeclModelDef::SetupJoints( int *numJoints, anJointMat **jointList, anBounds &frameBounds, bool removeOriginOffset ) const {
	if ( !modelHandle || modelHandle->IsDefaultModel() ) {
		Mem_Free16( (*jointList) );
		(* jointList) = nullptr;
		frameBounds.Clear();
		return;
	}

	// get the number of joints
	int num = modelHandle->NumJoints();

	if ( !num ) {
		gameLocal.Error( "model '%s' has no joints", modelHandle->Name() );
	}

	// set up initial pose for model (with no pose, model is just a jumbled mess)
	anJointMat * list = (anJointMat *) Mem_Alloc16( SIMD_ROUND_JOINTS( num ) * sizeof( list[0] ), TAG_JOINTMAT );
	const anJointQuat *pose = GetDefaultPose();

	// convert the joint quaternions to joint matrices
	SIMDProcessor->ConvertJointQuatsToJointMats( list, pose, joints.Num() );

	// check if we offset the model by the origin joint
	if ( removeOriginOffset ) {
#ifdef VELOCITY_MOVE
		list[0].SetTranslation( anVec3( offset.x, offset.y + pose[0].t.y, offset.z + pose[0].t.z ) );
#else
		list[0].SetTranslation( offset );
#endif
	} else {
		list[0].SetTranslation( pose[0].t + offset );
	}

	// transform the joint hierarchy
	SIMDProcessor->TransformJoints( list, jointParents.Ptr(), 1, joints.Num() - 1 );

	SIMD_INIT_LAST_JOINT( list, num );

	*numJoints = num;
	*jointList = list;

	// get the bounds of the default pose
	frameBounds = modelHandle->Bounds( nullptr );
}

/*
=====================
anDeclModelDef::ParseAnim
=====================
*/
bool anDeclModelDef::ParseAnim( anLexer &src, int numDefaultAnims ) {
	anAnim			*anim;
	const anMD6Anim	*md6anims[ ANIM_MaxSyncedAnims ], *md6anim;
	const anMD6Anim	*;
	anToken			token, realname;
	animFlags_t		flags;

	int numAnims = 0;
	memset( md6anims, 0, sizeof( md6anims ) );

	if ( !src.ReadToken( &realname ) ) {
		src.Warning( "Unexpected end of file" );
		MakeDefault();
		return false;
	}
	anStr alias = realname;

	for ( i = 0; i < anims.Num(); i++ ) {
		if ( !strcmp( anims[i]->FullName(), realname ) ) {
			break;
		}
	}

	if ( ( i < anims.Num() ) && ( i >= numDefaultAnims ) ) {
		src.Warning( "Duplicate anim '%s'", realname.c_str() );
		MakeDefault();
		return false;
	}

	if ( i < numDefaultAnims ) {
		anim = anims[i];
	} else {
		// create the alias associated with this animation
		anim = new ( TAG_ANIM ) anAnim();
		anims.Append( anim );
	}

	// random anims end with a number.  find the numeric suffix of the animation.
	int len = alias.Length();
	for ( i = len - 1; i > 0; i-- ) {
		if ( !isdigit( (unsigned char)alias[i] ) ) {
			break;
		}
	}

	// check for zero length name, or a purely numeric name
	if ( i <= 0 ) {
		src.Warning( "Invalid animation name '%s'", alias.c_str() );
		MakeDefault();
		return false;
	}

	// remove the numeric suffix
	alias.CapLength( i + 1 );

	// parse the anims from the string
	do {
		if ( !src.ReadToken( &token ) ) {
			src.Warning( "Unexpected end of file" );
			MakeDefault();
			return false;
		}

		// lookup the animation
		md6anim = animationLib.GetAnim( token );
		if ( !md6anim ) {
			src.Warning( "Couldn't load anim '%s'", token.c_str() );
			MakeDefault();
			return false;
		}

		md6anim->CheckModelHierarchy( modelHandle );

		if ( numAnims > 0 ) {
			// make sure it's the same length as the other anims
			if ( md6anim->Length() != md6anims[0]->Length() ) {
				src.Warning( "Anim '%s' does not match length of anim '%s'", md6anim->Name(), md6anims[0]->Name() );
				MakeDefault();
				return false;
			}
		}

		if ( numAnims >= ANIM_MaxSyncedAnims ) {
			src.Warning( "Exceeded max synced anims (%d)", ANIM_MaxSyncedAnims );
			MakeDefault();
			return false;
		}

		// add it to our list
		md6anims[ numAnims ] = md6anim;
		numAnims++;
	} while ( src.CheckTokenString( "," ) );

	if ( !numAnims ) {
		src.Warning( "No animation specified" );
		MakeDefault();
		return false;
	}

	anim->SetAnim( this, realname, alias, numAnims, md6anims );
	memset( &flags, 0, sizeof( flags ) );

	// parse any frame commands or animflags
	if ( src.CheckTokenString( "{" ) ) {
		while( 1 ) {
			if ( !src.ReadToken( &token ) ) {
				src.Warning( "Unexpected end of file" );
				MakeDefault();
				return false;
			}
			if ( token == "}" ) {
				break;
			}else if ( token == "prevent_idle_override" ) {
				flags.prevent_idle_override = true;
			} else if ( token == "random_cycle_start" ) {
				flags.random_cycle_start = true;
			} else if ( token == "ai_no_turn" ) {
				flags.ai_no_turn = true;
			} else if ( token == "anim_turn" ) {
				flags.anim_turn = true;
			} else if ( token == "frame" ) {
				// create a frame command
				int			frameNum;
				const char	*err;

				// make sure we don't have any line breaks while reading the frame command so the error line # will be correct
				if ( !src.ReadTokenOnLine( &token ) ) {
					src.Warning( "Missing frame # after 'frame'" );
					MakeDefault();
					return false;
				}
				if ( token.type == TT_PUNCTUATION && token == "-" ) {
					src.Warning( "Invalid frame # after 'frame'" );
					MakeDefault();
					return false;
				} else if ( token.type != TT_NUMBER || token.subtype == TT_FLOAT ) {
					src.Error( "expected integer value, found '%s'", token.c_str() );
				}

				// get the frame number
				frameNum = token.GetIntValue();

				// put the command on the specified frame of the animation
				err = anim->AddFrameCommand( this, frameNum, src, nullptr );
				if ( err ) {
					src.Warning( "%s", err );
					MakeDefault();
					return false;
				}
			} else {
				src.Warning( "Unknown command '%s'", token.c_str() );
				MakeDefault();
				return false;
			}
		}
	}

	// set the flags
	anim->SetAnimFlags( flags );
	return true;
}

/*
================
anDeclModelDef::Parse
================
*/
bool anDeclModelDef::Parse( const char *text, const int textLength, bool allowBinaryVersion ) {
	int					num;
	anStr				filename, jointNames, extension;
	const anMD6Joint	*md6joint, md6joints;
	anLexer				src;
	anToken				token, token2;
	anList<jointHandle_t> jointList;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	int numDefaultAnims = 0;
	while ( 1 ) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( !token.Icmp( "}" ) ) {
			break;
		}

		if ( token == "inherit" ) {
			if ( !src.ReadToken( &token2 ) ) {
				src.Warning( "Unexpected end of file" );
				MakeDefault();
				return false;
			}

			const anDeclModelDef *copy = static_cast<const anDeclModelDef *>( declManager->FindType( DECL_MODELDEF, token2, false ) );
			if ( !copy ) {
				common->Warning( "Unknown model definition '%s'", token2.c_str() );
			} else if ( copy->GetState() == DS_DEFAULTED ) {
				common->Warning( "inherited model definition '%s' defaulted", token2.c_str() );
				MakeDefault();
				return false;
			} else {
				CopyDecl( copy );
				numDefaultAnims = anims.Num();
			}
		} else if ( token == "skin" ) {
			if ( !src.ReadToken( &token2 ) ) {
				src.Warning( "Unexpected end of file" );
				MakeDefault();
				return false;
			}
			skin = declManager->FindSkin( token2 );
			if ( !skin ) {
				src.Warning( "Skin '%s' not found", token2.c_str() );
				MakeDefault();
				return false;
			}
		} else if ( token == "mesh" ) {
			if ( !src.ReadToken( &token2 ) ) {
				src.Warning( "Unexpected end of file" );
				MakeDefault();
				return false;
			}
			filename = token2;
			filename.ExtractFileExtension( extension );
			if ( extension != MD5_MESH_EXT ) {
				src.Warning( "Invalid model for MD6 mesh" );
				MakeDefault();
				return false;
			}
			modelHandle = renderModelManager->FindModel( filename );
			if ( !modelHandle ) {
				src.Warning( "Model '%s' not found", filename.c_str() );
				MakeDefault();
				return false;
			}

			if ( modelHandle->IsDefaultModel() ) {
				src.Warning( "Model '%s' defaulted", filename.c_str() );
				MakeDefault();
				return false;
			}

			// get the number of joints
			num = modelHandle->NumJoints();
			if ( !num ) {
				src.Warning( "Model '%s' has no joints", filename.c_str() );
			}

			// set up the joint hierarchy
			joints.SetGranularity( 1 );
			joints.SetNum( num );
			jointParents.SetNum( num );
			channelJoints[0].SetNum( num );
			md6joints = modelHandle->GetJoints();
			md6joint = md6joints;
			for ( int i = 0; i < num; i++, md6joint++ ) {
				joints[i].channel = ANIMCHANNEL_ALL;
				joints[i].num = static_cast<jointHandle_t>( i );
				if ( md6joint->parent ) {
					joints[i].parentNum = static_cast<jointHandle_t>( md6joint->parent - md6joints );
				} else {
					joints[i].parentNum = INVALID_JOINT;
				}
				jointParents[i] = joints[i].parentNum;
				channelJoints[0][i] = i;
			}
		} else if ( token == "remove" ) {
			// removes any anims whos name matches
			if ( !src.ReadToken( &token2 ) ) {
				src.Warning( "Unexpected end of file" );
				MakeDefault();
				return false;
			}
			num = 0;
			for ( int i = 0; i < anims.Num(); i++ ) {
				if ( ( token2 == anims[i]->Name() ) || ( token2 == anims[i]->FullName() ) ) {
					delete anims[i];
					anims.RemoveIndex( i );
					if ( i >= numDefaultAnims ) {
						src.Warning( "Anim '%s' was not inherited.  Anim should be removed from the model def.", token2.c_str() );
						MakeDefault();
						return false;
					}
					i--;
					numDefaultAnims--;
					num++;
					continue;
				}
			}
			if ( !num ) {
				src.Warning( "Couldn't find anim '%s' to remove", token2.c_str() );
				MakeDefault();
				return false;
			}
		} else if ( token == "anim" ) {
			if ( !modelHandle ) {
				src.Warning( "Must specify mesh before defining anims" );
				MakeDefault();
				return false;
			}
			if ( !ParseAnim( src, numDefaultAnims ) ) {
				MakeDefault();
				return false;
			}
		} else if ( token == "offset" ) {
			if ( !src.Parse1DMatrix( 3, offset.ToFloatPtr() ) ) {
				src.Warning( "Expected vector following 'offset'" );
				MakeDefault();
				return false;
			}
		} else if ( token == "channel" ) {
			if ( !modelHandle ) {
				src.Warning( "Must specify mesh before defining channels" );
				MakeDefault();
				return false;
			}

			// set the channel for a group of joints
			if ( !src.ReadToken( &token2 ) ) {
				src.Warning( "Unexpected end of file" );
				MakeDefault();
				return false;
			}
			if ( !src.CheckTokenString( "( " ) ) {
				src.Warning( "Expected { after '%s'\n", token2.c_str() );
				MakeDefault();
				return false;
			}

			for ( int i = ANIMCHANNEL_ALL + 1; i < ANIM_NumAnimChannels; i++ ) {
				if ( !anStr::Icmp( channelNames[i], token2 ) ) {
					break;
				}
			}

			if ( i >= ANIM_NumAnimChannels ) {
				src.Warning( "Unknown channel '%s'", token2.c_str() );
				MakeDefault();
				return false;
			}

			int channel = i;
			jointNames = "";

			while ( !src.CheckTokenString( " )" ) ) {
				if ( !src.ReadToken( &token2 ) ) {
					src.Warning( "Unexpected end of file" );
					MakeDefault();
					return false;
				}
				jointNames += token2;
				if ( ( token2 != "*" ) && ( token2 != "-" ) ) {
					jointNames += " ";
				}
			}

			GetJointList( jointNames, jointList );

			channelJoints[ channel ].SetNum( jointList.Num() );
			for ( int num = i = 0; i < jointList.Num(); i++ ) {
				jointHandle_t jointNum = jointList[i];
				if ( joints[ jointNum ].channel != ANIMCHANNEL_ALL ) {
					src.Warning( "Joint '%s' assigned to multiple channels", modelHandle->GetJointName( jointNum ) );
					continue;
				}
				joints[ jointNum ].channel = channel;
				channelJoints[ channel ][ num++ ] = jointNum;
			}
			channelJoints[ channel ].SetNum( num );
		} else {
			src.Warning( "unknown token '%s'", token.c_str() );
			MakeDefault();
			return false;
		}
	}

	// shrink the anim list down to save space
	anims.SetGranularity( 1 );
	anims.SetNum( anims.Num() );

	return true;
}

/*
=====================
anDeclModelDef::HasAnim
=====================
*/
bool anDeclModelDef::HasAnim( const char *name ) const {
	for ( int i = 0; i < anims.Num(); i++ ) {
		if ( !anStr::Icmp( name, anims[i]->Name() ) || !anStr::Icmp( name, anims[i]->FullName() ) ) {
			return true;
		}
	}

	// find any animations with same name
	for ( int i = 0; i < anims.Num(); i++ ) {
		if ( !strcmp( anims[i]->Name(), name ) ) {
			return true;
		}
	}

	// our failsafe our fallback mechanism!  and or more flex!
	if ( decl_animFallback.GetBool() ) {
		if ( fallbackCheckResult ) {
			commmon->Printf( "DeclModelDef::HasAnim '%s'", name );
			return true;
		}
	}
	return false;
}

/*
=====================
anDeclModelDef::NumAnims
=====================
*/
int anDeclModelDef::NumAnims() const {
	return anims.Num() + 1;
}

/*
=====================
anDeclModelDef::GetSpecificAnim

Gets the exact anim for the name, without randomization.
=====================
*/
int anDeclModelDef::GetSpecificAnim( const char *name ) const {
	// find a specific animation
	for ( int  i = 0; i < anims.Num(); i++ ) {
		if ( !strcmp( anims[i]->FullName(), name ) ) {
			return i + 1;
		}
	}

	// didn't find it
	return 0;
}

/*
=====================
anDeclModelDef::GetAnim
=====================
*/
const anAnim *anDeclModelDef::GetAnim( int index ) const {
	if ( ( index < 1 ) || ( index > anims.Num() ) ) {
		return nullptr;
	}

	return anims[ index - 1 ];
}

/*
=====================
anDeclModelDef::GetAnim
=====================
*/
int anDeclModelDef::GetAnim( const char *name ) const {
	int				i;
	int				which;
	const int		MAX_ANIMS = 64;
	int				animList[ MAX_ANIMS ];
	int				numAnims;
	int				len;

	len = strlen( name );
	if ( len && anStr::CharIsNumeric( name[ len - 1 ] ) ) {
		// find a specific animation
		return GetSpecificAnim( name );
	}

	// find all animations with same name
	numAnims = 0;
	for ( i = 0; i < anims.Num(); i++ ) {
		if ( !strcmp( anims[i]->Name(), name ) ) {
			animList[ numAnims++ ] = i;
			if ( numAnims >= MAX_ANIMS ) {
				break;
			}
		}
	}

	if ( !numAnims ) {
		return 0;
	}

	// get a random anim
	//FIXME: don't access gameLocal here?
	which = gameLocal.random.RandomInt( numAnims );
	return animList[ which ] + 1;
}

/*
=====================
anDeclModelDef::GetSkin
=====================
*/
const anDeclSkin *anDeclModelDef::GetSkin() const {
	return skin;
}

/*
=====================
anDeclModelDef::GetModelName
=====================
*/
const char *anDeclModelDef::GetModelName() const {
	if ( modelHandle ) {
		return modelHandle->Name();
	} else {
		return "";
	}
}

/*
=====================
anDeclModelDef::Joints
=====================
*/
const anList<jointInfo_t> &anDeclModelDef::Joints() const {
	return joints;
}

/*
=====================
anDeclModelDef::JointParents
=====================
*/
const int * anDeclModelDef::JointParents() const {
	return jointParents.Ptr();
}

/*
=====================
anDeclModelDef::NumJoints
=====================
*/
int anDeclModelDef::NumJoints() const {
	return joints.Num();
}

/*
=====================
anDeclModelDef::GetJoint
=====================
*/
const jointInfo_t *anDeclModelDef::GetJoint( int jointHandle ) const {
	if ( ( jointHandle < 0 ) || ( jointHandle > joints.Num() ) ) {
		gameLocal.Error( "DeclModelDef::GetJoint : joint handle out of range" );
	}
	return &joints[ jointHandle ];
}

/*
====================
anDeclModelDef::GetJointName
====================
*/
const char *anDeclModelDef::GetJointName( int jointHandle ) const {
	const anMD6Joint *joint;

	if ( !modelHandle ) {
		return nullptr;
	}

	if ( ( jointHandle < 0 ) || ( jointHandle > joints.Num() ) ) {
		gameLocal.Error( "DeclModelDef::GetJointName : joint handle out of range" );
	}

	joint = modelHandle->GetJoints();
	return joint[ jointHandle ].name.c_str();
}

/*
=====================
anDeclModelDef::NumJointsOnChannel
=====================
*/
int anDeclModelDef::NumJointsOnChannel( int channel ) const {
	if ( ( channel < 0 ) || ( channel >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "DeclModelDef::NumJointsOnChannel : channel out of range" );
		return 0;
	}
	return channelJoints[ channel ].Num();
}

/*
=====================
anDeclModelDef::GetChannelJoints
=====================
*/
const int * anDeclModelDef::GetChannelJoints( int channel ) const {
	if ( ( channel < 0 ) || ( channel >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "DeclModelDef::GetChannelJoints : channel out of range" );
		return nullptr;
	}
	return channelJoints[ channel ].Ptr();
}

/*
=====================
anDeclModelDef::GetVisualOffset
=====================
*/
const anVec3 &anDeclModelDef::GetVisualOffset() const {
	return offset;
}

/***********************************************************************

	anAnimator

***********************************************************************/

/*
=====================
anAnimator::anAnimator
=====================
*/
anAnimator::anAnimator() {
	int	i, j;

	modelDef				= nullptr;
	entity					= nullptr;
	numJoints				= 0;
	joints					= nullptr;
	lastTransformTime		= -1;
	stoppedAnimatingUpdate	= false;
	removeOriginOffset		= false;
	forceUpdate				= false;

	frameBounds.Clear();

	AFPoseJoints.SetGranularity( 1 );
	AFPoseJointMods.SetGranularity( 1 );
	AFPoseJointFrame.SetGranularity( 1 );

	ClearAFPose();

	for ( i = ANIMCHANNEL_ALL; i < ANIM_NumAnimChannels; i++ ) {
		for ( j = 0; j < ANIM_MaxAnimsPerChannel; j++ ) {
			channels[i][ j ].Reset( nullptr );
		}
	}
}

/*
=====================
anAnimator::~anAnimator
=====================
*/
anAnimator::~anAnimator() {
	FreeData();
}

/*
=====================
anAnimator::Allocated
=====================
*/
size_t anAnimator::Allocated() const {
	size_t	size;
	size = jointMods.Allocated() + numJoints * sizeof( joints[0] ) + jointMods.Num() * sizeof( jointMods[0] ) + AFPoseJointMods.Allocated() + AFPoseJointFrame.Allocated() + AFPoseJoints.Allocated();
	return size;
}

/*
=====================
anAnimator::Save

archives object for save game file
=====================
*/
void anAnimator::Save( anSaveGame *savefile ) const {
	savefile->WriteModelDef( modelDef );
	savefile->WriteObject( entity );

	savefile->WriteInt( jointMods.Num() );
	for ( int i = 0; i < jointMods.Num(); i++ ) {
		savefile->WriteInt( jointMods[i]->jointNum );
		savefile->WriteMat3( jointMods[i]->mat );
		savefile->WriteVec3( jointMods[i]->pos );
		savefile->WriteInt( (int&)jointMods[i]->transform_pos );
		savefile->WriteInt( (int&)jointMods[i]->transform_axis );
	}

	savefile->WriteInt( numJoints );
	for ( int i = 0; i < numJoints; i++ ) {
		float *data = joints[i].ToFloatPtr();
		for ( int j = 0; j < 12; j++ ) {
			savefile->WriteFloat( data[j] );
		}
	}

	savefile->WriteInt( lastTransformTime );
	savefile->WriteBool( stoppedAnimatingUpdate );
	savefile->WriteBool( forceUpdate );
	savefile->WriteBounds( frameBounds );

	savefile->WriteFloat( AFPoseBlendWeight );

	savefile->WriteInt( AFPoseJoints.Num() );
	for ( int i = 0; i < AFPoseJoints.Num(); i++ ) {
		savefile->WriteInt( AFPoseJoints[i] );
	}

	savefile->WriteInt( AFPoseJointMods.Num() );
	for ( int i = 0; i < AFPoseJointMods.Num(); i++ ) {
		savefile->WriteInt( (int&)AFPoseJointMods[i].mod );
		savefile->WriteMat3( AFPoseJointMods[i].axis );
		savefile->WriteVec3( AFPoseJointMods[i].origin );
	}

	savefile->WriteInt( AFPoseJointFrame.Num() );
	for ( int i = 0; i < AFPoseJointFrame.Num(); i++ ) {
		savefile->WriteFloat( AFPoseJointFrame[i].q.x );
		savefile->WriteFloat( AFPoseJointFrame[i].q.y );
		savefile->WriteFloat( AFPoseJointFrame[i].q.z );
		savefile->WriteFloat( AFPoseJointFrame[i].q.w );
		savefile->WriteVec3( AFPoseJointFrame[i].t );
	}

	savefile->WriteBounds( AFPoseBounds );
	savefile->WriteInt( AFPoseTime );

	savefile->WriteBool( removeOriginOffset );

	for ( int i = ANIMCHANNEL_ALL; i < ANIM_NumAnimChannels; i++ ) {
		for ( int j = 0; j < ANIM_MaxAnimsPerChannel; j++ ) {
			channels[i][ j ].Save( savefile );
		}
	}
}

/*
=====================
anAnimator::Restore

unarchives object from save game file
=====================
*/
void anAnimator::Restore( anRestoreGame *savefile ) {
	int num;

	savefile->ReadModelDef( modelDef );
	savefile->ReadObject( reinterpret_cast<anClass *&>( entity ) );

	savefile->ReadInt( num );
	jointMods.SetNum( num );
	for ( int i = 0; i < num; i++ ) {
		jointMods[i] = new ( TAG_ANIM ) jointMod_t;
		savefile->ReadInt( (int&)jointMods[i]->jointNum );
		savefile->ReadMat3( jointMods[i]->mat );
		savefile->ReadVec3( jointMods[i]->pos );
		savefile->ReadInt( (int&)jointMods[i]->transform_pos );
		savefile->ReadInt( (int&)jointMods[i]->transform_axis );
	}

	savefile->ReadInt( numJoints );
	joints = (anJointMat *) Mem_Alloc16( SIMD_ROUND_JOINTS( numJoints ) * sizeof( joints[0] ), TAG_JOINTMAT );
	for ( int i = 0; i < numJoints; i++ ) {
		float *data = joints[i].ToFloatPtr();
		for ( int j = 0; j < 12; j++ ) {
			savefile->ReadFloat( data[j] );
		}
	}
	SIMD_INIT_LAST_JOINT( joints, numJoints );

	savefile->ReadInt( lastTransformTime );
	savefile->ReadBool( stoppedAnimatingUpdate );
	savefile->ReadBool( forceUpdate );
	savefile->ReadBounds( frameBounds );

	savefile->ReadFloat( AFPoseBlendWeight );

	savefile->ReadInt( num );
	AFPoseJoints.SetGranularity( 1 );
	AFPoseJoints.SetNum( num );
	for ( int i = 0; i < AFPoseJoints.Num(); i++ ) {
		savefile->ReadInt( AFPoseJoints[i] );
	}

	savefile->ReadInt( num );
	AFPoseJointMods.SetGranularity( 1 );
	AFPoseJointMods.SetNum( num );
	for ( int i = 0; i < AFPoseJointMods.Num(); i++ ) {
		savefile->ReadInt( (int&)AFPoseJointMods[i].mod );
		savefile->ReadMat3( AFPoseJointMods[i].axis );
		savefile->ReadVec3( AFPoseJointMods[i].origin );
	}

	savefile->ReadInt( num );
	AFPoseJointFrame.SetGranularity( 1 );
	AFPoseJointFrame.SetNum( num );
	for ( int i = 0; i < AFPoseJointFrame.Num(); i++ ) {
		savefile->ReadFloat( AFPoseJointFrame[i].q.x );
		savefile->ReadFloat( AFPoseJointFrame[i].q.y );
		savefile->ReadFloat( AFPoseJointFrame[i].q.z );
		savefile->ReadFloat( AFPoseJointFrame[i].q.w );
		savefile->ReadVec3( AFPoseJointFrame[i].t );
	}

	savefile->ReadBounds( AFPoseBounds );
	savefile->ReadInt( AFPoseTime );

	savefile->ReadBool( removeOriginOffset );

	for ( int i = ANIMCHANNEL_ALL; i < ANIM_NumAnimChannels; i++ ) {
		for ( int j = 0; j < ANIM_MaxAnimsPerChannel; j++ ) {
			channels[i][ j ].Restore( savefile, modelDef );
		}
	}
}

/*
=====================
anAnimator::FreeData
=====================
*/
void anAnimator::FreeData() {
	if ( entity ) {
		entity->BecomeInactive( TH_ANIMATE );
	}

	for ( int i = ANIMCHANNEL_ALL; i < ANIM_NumAnimChannels; i++ ) {
		for ( int j = 0; j < ANIM_MaxAnimsPerChannel; j++ ) {
			channels[i][ j ].Reset( nullptr );
		}
	}

	jointMods.DeleteContents( true );
	Mem_Free16( joints );
	joints = nullptr;
	numJoints = 0;
	modelDef = nullptr;

	ForceUpdate();
}

/*
=====================
anAnimator::PushAnims
=====================
*/
void anAnimator::PushAnims( int channelNum, int currentTime, int blendTime ) {
	anAnimBlend *channel;

	channel = channels[ channelNum ];
	if ( !channel[0].GetWeight( currentTime ) || ( channel[0].starttime == currentTime ) ) {
		return;
	}

	for ( int i = ANIM_MaxAnimsPerChannel - 1; i > 0; i-- ) {
		channel[i] = channel[ i - 1 ];
	}

	channel[0].Reset( modelDef );
	channel[1].Clear( currentTime, blendTime );
	ForceUpdate();
}

/*
=====================
anAnimator::SetModel
=====================
*/
anRenderModel *anAnimator::SetModel( const char *modelname ) {
	FreeData();

	// check if we're just clearing the model
	if ( !modelname || !*modelname ) {
		return nullptr;
	}

	modelDef = static_cast<const anDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelname, false ) );
	if ( !modelDef ) {
		return nullptr;
	}

	anRenderModel *renderModel = modelDef->ModelHandle();
	if ( !renderModel ) {
		modelDef = nullptr;
		return nullptr;
	}

	// make sure model hasn't been purged
	modelDef->Touch();

	modelDef->SetupJoints( &numJoints, &joints, frameBounds, removeOriginOffset );
	modelDef->ModelHandle()->Reset();

	// set the modelDef on all channels
	for ( int i = ANIMCHANNEL_ALL; i < ANIM_NumAnimChannels; i++ ) {
		for ( int j = 0; j < ANIM_MaxAnimsPerChannel; j++ ) {
			channels[i][ j ].Reset( modelDef );
		}
	}

	return modelDef->ModelHandle();
}

/*
=====================
anAnimator::Size
=====================
*/
size_t anAnimator::Size() const {
	return sizeof(* this) + Allocated();
}

/*
=====================
anAnimator::SetEntity
=====================
*/
void anAnimator::SetEntity( anEntity *ent ) {
	entity = ent;
}

/*
=====================
anAnimator::GetEntity
=====================
*/
anEntity *anAnimator::GetEntity() const {
	return entity;
}

/*
=====================
anAnimator::RemoveOriginOffset
=====================
*/
void anAnimator::RemoveOriginOffset( bool remove ) {
	removeOriginOffset = remove;
}

/*
=====================
anAnimator::RemoveOrigin
=====================
*/
bool anAnimator::RemoveOrigin() const {
	return removeOriginOffset;
}

/*
=====================
anAnimator::GetJointList
=====================
*/
void anAnimator::GetJointList( const char *jointNames, anList<jointHandle_t> &jointList ) const {
	if ( modelDef ) {
		modelDef->GetJointList( jointNames, jointList );
	}
}

/*
=====================
anAnimator::NumAnims
=====================
*/
int	anAnimator::NumAnims() const {
	if ( !modelDef ) {
		return 0;
	}

	return modelDef->NumAnims();
}

/*
=====================
anAnimator::GetAnim
=====================
*/
const anAnim *anAnimator::GetAnim( int index ) const {
	if ( !modelDef ) {
		return nullptr;
	}

	return modelDef->GetAnim( index );
}

/*
=====================
anAnimator::GetAnim
=====================
*/
int anAnimator::GetAnim( const char *name ) const {
	if ( !modelDef ) {
		return 0;
	}

	return modelDef->GetAnim( name );
}

/*
=====================
anAnimator::HasAnim
=====================
*/
bool anAnimator::HasAnim( const char *name ) const {
	if ( !modelDef ) {
		return false;
	}

	return modelDef->HasAnim( name );
}

/*
=====================
anAnimator::NumJoints
=====================
*/
int	anAnimator::NumJoints() const {
	return numJoints;
}

/*
=====================
anAnimator::ModelHandle
=====================
*/
anRenderModel *anAnimator::ModelHandle() const {
	if ( !modelDef ) {
		return nullptr;
	}

	return modelDef->ModelHandle();
}

/*
=====================
anAnimator::ModelDef
=====================
*/
const anDeclModelDef *anAnimator::ModelDef() const {
	return modelDef;
}

/*
=====================
anAnimator::CurrentAnim
=====================
*/
anAnimBlend *anAnimator::CurrentAnim( int channelNum ) {
	if ( ( channelNum < 0 ) || ( channelNum >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "anAnimator::CurrentAnim : channel out of range" );
		return nullptr;
	}

	return &channels[ channelNum ][0];
}

/*
=====================
anAnimator::Clear
=====================
*/
void anAnimator::Clear( int channelNum, int currentTime, int cleartime ) {
	int			i;
	anAnimBlend	*blend;

	if ( ( channelNum < 0 ) || ( channelNum >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "anAnimator::Clear : channel out of range" );
		return;
	}

	blend = channels[ channelNum ];
	for ( i = 0; i < ANIM_MaxAnimsPerChannel; i++, blend++ ) {
		blend->Clear( currentTime, cleartime );
	}
	ForceUpdate();
}

/*
=====================
anAnimator::SetFrame
=====================
*/
void anAnimator::SetFrame( int channelNum, int animNum, int frame, int currentTime, int blendTime ) {
	if ( ( channelNum < 0 ) || ( channelNum >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "anAnimator::SetFrame : channel out of range" );
	}

	if ( !modelDef || !modelDef->GetAnim( animNum ) ) {
		return;
	}

	PushAnims( channelNum, currentTime, blendTime );
	channels[ channelNum ][0].SetFrame( modelDef, animNum, frame, currentTime, blendTime );
	if ( entity ) {
		entity->BecomeActive( TH_ANIMATE );
	}
}

/*
=====================
anAnimator::CycleAnim
=====================
*/
void anAnimator::CycleAnim( int channelNum, int animNum, int currentTime, int blendTime ) {
	if ( ( channelNum < 0 ) || ( channelNum >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "anAnimator::CycleAnim : channel out of range" );
	}

	if ( !modelDef || !modelDef->GetAnim( animNum ) ) {
		return;
	}

	PushAnims( channelNum, currentTime, blendTime );
	channels[ channelNum ][0].CycleAnim( modelDef, animNum, currentTime, blendTime );
	if ( entity ) {
		entity->BecomeActive( TH_ANIMATE );
	}
}

/*
=====================
anAnimator::PlayAnim
=====================
*/
void anAnimator::PlayAnim( int channelNum, int animNum, int currentTime, int blendTime ) {
	if ( ( channelNum < 0 ) || ( channelNum >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "anAnimator::PlayAnim : channel out of range" );
	}

	if ( !modelDef || !modelDef->GetAnim( animNum ) ) {
		return;
	}

	PushAnims( channelNum, currentTime, blendTime );
	channels[ channelNum ][0].PlayAnim( modelDef, animNum, currentTime, blendTime );
	if ( entity ) {
		entity->BecomeActive( TH_ANIMATE );
	}
}

/*
=====================
anAnimator::SyncAnimChannels
=====================
*/
void anAnimator::SyncAnimChannels( int channelNum, int fromChannelNum, int currentTime, int blendTime ) {
	if ( ( channelNum < 0 ) || ( channelNum >= ANIM_NumAnimChannels ) || ( fromChannelNum < 0 ) || ( fromChannelNum >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "anAnimator::SyncToChannel : channel out of range" );
		return;
	}

	anAnimBlend &fromBlend = channels[ fromChannelNum ][0];
	anAnimBlend &toBlend = channels[ channelNum ][0];

	float weight = fromBlend.blendEndValue;
	if ( ( fromBlend.Anim() != toBlend.Anim() ) || ( fromBlend.GetStartTime() != toBlend.GetStartTime() ) || ( fromBlend.GetEndTime() != toBlend.GetEndTime() ) ) {
		PushAnims( channelNum, currentTime, blendTime );
		toBlend = fromBlend;
		toBlend.blendStartValue = 0.0f;
		toBlend.blendEndValue = 0.0f;
	}
    toBlend.SetWeight( weight, currentTime - 1, blendTime );

	// disable framecommands on the current channel so that commands aren't called twice
	toBlend.AllowFrameCommands( false );

	if ( entity ) {
		entity->BecomeActive( TH_ANIMATE );
	}
}

/*
=====================
anAnimator::SetJointPos
=====================
*/
void anAnimator::SetJointPos( jointHandle_t jointNum, jointModTransform_t transform_type, const anVec3 &pos ) {
	if ( !modelDef || !modelDef->ModelHandle() || ( jointNum < 0 ) || ( jointNum >= numJoints ) ) {
		return;
	}

	jointMod_t *jointMod = nullptr;
	for ( int i = 0; i < jointMods.Num(); i++ ) {
		if ( jointMods[i]->jointNum == jointNum ) {
			jointMod = jointMods[i];
			break;
		} else if ( jointMods[i]->jointNum > jointNum ) {
			break;
		}
	}

	if ( !jointMod ) {
		jointMod = new ( TAG_ANIM ) jointMod_t;
		jointMod->jointNum = jointNum;
		jointMod->mat.Identity();
		jointMod->transform_axis = JOINTMOD_NONE;
		jointMods.Insert( jointMod, i );
	}

	jointMod->pos = pos;
	jointMod->transform_pos = transform_type;

	if ( entity ) {
		entity->BecomeActive( TH_ANIMATE );
	}
	ForceUpdate();
}

/*
=====================
anAnimator::SetJointAxis
=====================
*/
void anAnimator::SetJointAxis( jointHandle_t jointNum, jointModTransform_t transform_type, const anMat3 &mat ) {
	jointMod_t *jointMod;

	if ( !modelDef || !modelDef->ModelHandle() || ( jointNum < 0 ) || ( jointNum >= numJoints ) ) {
		return;
	}

	jointMod = nullptr;
	for ( int i = 0; i < jointMods.Num(); i++ ) {
		if ( jointMods[i]->jointNum == jointNum ) {
			jointMod = jointMods[i];
			break;
		} else if ( jointMods[i]->jointNum > jointNum ) {
			break;
		}
	}

	if ( !jointMod ) {
		jointMod = new ( TAG_ANIM ) jointMod_t;
		jointMod->jointNum = jointNum;
		jointMod->pos.Zero();
		jointMod->transform_pos = JOINTMOD_NONE;
		jointMods.Insert( jointMod, i );
	}

	jointMod->mat = mat;
	jointMod->transform_axis = transform_type;

	if ( entity ) {
		entity->BecomeActive( TH_ANIMATE );
	}
	ForceUpdate();
}

/*
=====================
anAnimator::ClearJoint
=====================
*/
void anAnimator::ClearJoint( jointHandle_t jointNum ) {
	int i;

	if ( !modelDef || !modelDef->ModelHandle() || ( jointNum < 0 ) || ( jointNum >= numJoints ) ) {
		return;
	}

	for ( int i = 0; i < jointMods.Num(); i++ ) {
		if ( jointMods[i]->jointNum == jointNum ) {
			delete jointMods[i];
			jointMods.RemoveIndex( i );
			ForceUpdate();
			break;
		} else if ( jointMods[i]->jointNum > jointNum ) {
			break;
		}
	}
}

/*
=====================
anAnimator::ClearAllJoints
=====================
*/
void anAnimator::ClearAllJoints() {
	if ( jointMods.Num() ) {
		ForceUpdate();
	}
	jointMods.DeleteContents( true );
}

/*
=====================
anAnimator::ClearAllAnims
=====================
*/
void anAnimator::ClearAllAnims( int currentTime, int cleartime ) {
	int	i;

	for ( int i = 0; i < ANIM_NumAnimChannels; i++ ) {
		Clear( i, currentTime, cleartime );
	}

	ClearAFPose();
	ForceUpdate();
}

/*
====================
anAnimator::GetDelta
====================
*/
void anAnimator::GetDelta( int fromtime, int totime, anVec3 &delta ) const {
	int					i;
	const anAnimBlend	*blend;
	float				blendWeight;

	if ( !modelDef || !modelDef->ModelHandle() || ( fromtime == totime ) ) {
		delta.Zero();
		return;
	}

	delta.Zero();
	blendWeight = 0.0f;

	blend = channels[ ANIMCHANNEL_ALL ];
	for ( int i = 0; i < ANIM_MaxAnimsPerChannel; i++, blend++ ) {
		blend->BlendDelta( fromtime, totime, delta, blendWeight );
	}

	if ( modelDef->Joints()[0].channel ) {
		blend = channels[ modelDef->Joints()[0].channel ];
		for ( int i = 0; i < ANIM_MaxAnimsPerChannel; i++, blend++ ) {
			blend->BlendDelta( fromtime, totime, delta, blendWeight );
		}
	}
}

/*
====================
anAnimator::GetDeltaRotation
====================
*/
bool anAnimator::GetDeltaRotation( int fromtime, int totime, anMat3 &delta ) const {
	int					i;
	const anAnimBlend	*blend;
	float				blendWeight;
	anQuat				q;

	if ( !modelDef || !modelDef->ModelHandle() || ( fromtime == totime ) ) {
		delta.Identity();
		return false;
	}

	q.Set( 0.0f, 0.0f, 0.0f, 1.0f );
	blendWeight = 0.0f;

	blend = channels[ ANIMCHANNEL_ALL ];
	for ( int i = 0; i < ANIM_MaxAnimsPerChannel; i++, blend++ ) {
		blend->BlendDeltaRotation( fromtime, totime, q, blendWeight );
	}

	if ( modelDef->Joints()[0].channel ) {
		blend = channels[ modelDef->Joints()[0].channel ];
		for ( int i = 0; i < ANIM_MaxAnimsPerChannel; i++, blend++ ) {
			blend->BlendDeltaRotation( fromtime, totime, q, blendWeight );
		}
	}

	if ( blendWeight > 0.0f ) {
		delta = q.ToMat3();
		return true;
	} else {
		delta.Identity();
		return false;
	}
}

/*
====================
anAnimator::GetOrigin
====================
*/
void anAnimator::GetOrigin( int currentTime, anVec3 &pos ) const {
	int					i;
	const anAnimBlend	*blend;
	float				blendWeight;

	if ( !modelDef || !modelDef->ModelHandle() ) {
		pos.Zero();
		return;
	}

	pos.Zero();
	blendWeight = 0.0f;

	blend = channels[ ANIMCHANNEL_ALL ];
	for ( int i = 0; i < ANIM_MaxAnimsPerChannel; i++, blend++ ) {
		blend->BlendOrigin( currentTime, pos, blendWeight, removeOriginOffset );
	}

	if ( modelDef->Joints()[0].channel ) {
		blend = channels[ modelDef->Joints()[0].channel ];
		for ( int i = 0; i < ANIM_MaxAnimsPerChannel; i++, blend++ ) {
			blend->BlendOrigin( currentTime, pos, blendWeight, removeOriginOffset );
		}
	}

	pos += modelDef->GetVisualOffset();
}

/*
====================
anAnimator::GetBounds
====================
*/
bool anAnimator::GetBounds( int currentTime, anBounds &bounds ) {
	int					i, j;
	const anAnimBlend	*blend;
	int					count;

	if ( !modelDef || !modelDef->ModelHandle() ) {
		return false;
	}

	if ( AFPoseJoints.Num() ) {
		bounds = AFPoseBounds;
		count = 1;
	} else {
		bounds.Clear();
		count = 0;
	}

	blend = channels[0];
	for ( int i = ANIMCHANNEL_ALL; i < ANIM_NumAnimChannels; i++ ) {
		for ( int j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
			if ( blend->AddBounds( currentTime, bounds, removeOriginOffset ) ) {
				count++;
			}
		}
	}

	if ( !count ) {
		if ( !frameBounds.IsCleared() ) {
			bounds = frameBounds;
			return true;
		} else {
			bounds.Zero();
			return false;
		}
	}

	bounds.TranslateSelf( modelDef->GetVisualOffset() );

	if ( g_debugBounds.GetBool() ) {
		if ( bounds[1][0] - bounds[0][0] > 2048 || bounds[1][1] - bounds[0][1] > 2048 ) {
			if ( entity ) {
				gameLocal.Warning( "big frameBounds on entity '%s' with model '%s': %f,%f", entity->name.c_str(), modelDef->ModelHandle()->Name(), bounds[1][0] - bounds[0][0], bounds[1][1] - bounds[0][1] );
			} else {
				gameLocal.Warning( "big frameBounds on model '%s': %f,%f", modelDef->ModelHandle()->Name(), bounds[1][0] - bounds[0][0], bounds[1][1] - bounds[0][1] );
			}
		}
	}

	frameBounds = bounds;

	return true;
}

/*
=====================
anAnimator::InitAFPose
=====================
*/
void anAnimator::InitAFPose() {
	if ( !modelDef ) {
		return;
	}

	AFPoseJoints.SetNum( modelDef->Joints().Num() );
	AFPoseJoints.SetNum( 0 );
	AFPoseJointMods.SetNum( modelDef->Joints().Num() );
	AFPoseJointFrame.SetNum( modelDef->Joints().Num() );
}

/*
=====================
anAnimator::SetAFPoseJointMod
=====================
*/
void anAnimator::SetAFPoseJointMod( const jointHandle_t jointNum, const AFJointModType_t mod, const anMat3 &axis, const anVec3 &origin ) {
	AFPoseJointMods[jointNum].mod = mod;
	AFPoseJointMods[jointNum].axis = axis;
	AFPoseJointMods[jointNum].origin = origin;
	int index = BinSearch_GreaterEqual<int>( AFPoseJoints.Ptr(), AFPoseJoints.Num(), jointNum );
	if ( index >= AFPoseJoints.Num() || jointNum != AFPoseJoints[index] ) {
		AFPoseJoints.Insert( jointNum, index );
	}
}

/*
=====================
anAnimator::FinishAFPose
=====================
*/
void anAnimator::FinishAFPose( int animNum, const anBounds &bounds, const int time ) {
	int					numJoints;
	int					parentNum;
	int					jointMod;
	int					jointNum;
	const int *			jointParent;

	if ( !modelDef ) {
		return;
	}

	const anAnim *anim = modelDef->GetAnim( animNum );
	if ( !anim ) {
		return;
	}

	numJoints = modelDef->Joints().Num();
	if ( !numJoints ) {
		return;
	}

	anRenderModel *md6 = modelDef->ModelHandle();
	const anMD6Anim *md6anim = anim->MD6Anim( 0 );

	if ( numJoints != md6anim->NumJoints() ) {
		gameLocal.Warning( "Model '%s' has different # of joints than anim '%s'", md6->Name(), md6anim->Name() );
		return;
	}

	anJointQuat *jointFrame = ( anJointQuat * )_alloca16( numJoints * sizeof( *jointFrame ) );
	md6anim->GetSingleFrame( 0, jointFrame, modelDef->GetChannelJoints( ANIMCHANNEL_ALL ), modelDef->NumJointsOnChannel( ANIMCHANNEL_ALL ) );

	if ( removeOriginOffset ) {
#ifdef VELOCITY_MOVE
		jointFrame[0].t.x = 0.0f;
#else
		jointFrame[0].t.Zero();
#endif
	}

	anJointMat *joints = ( anJointMat * )_alloca16( numJoints * sizeof( *joints ) );

	// convert the joint quaternions to joint matrices
	SIMDProcessor->ConvertJointQuatsToJointMats( joints, jointFrame, numJoints );

	// first joint is always root of entire hierarchy
	if ( AFPoseJoints.Num() && AFPoseJoints[0] == 0 ) {
		switch ( AFPoseJointMods[0].mod ) {
			case AF_JOINTMOD_AXIS: {
				joints[0].SetRotation( AFPoseJointMods[0].axis );
				break;
			}
			case AF_JOINTMOD_ORIGIN: {
				joints[0].SetTranslation( AFPoseJointMods[0].origin );
				break;
			}
			case AF_JOINTMOD_BOTH: {
				joints[0].SetRotation( AFPoseJointMods[0].axis );
				joints[0].SetTranslation( AFPoseJointMods[0].origin );
				break;
			}
		}
		j = 1;
	} else {
		j = 0;
	}

	// pointer to joint info
	jointParent = modelDef->JointParents();

	// transform the child joints
	for ( int i = 1; j < AFPoseJoints.Num(); j++, i++ ) {
		jointMod = AFPoseJoints[j];
		// transform any joints preceding the joint modifier
		SIMDProcessor->TransformJoints( joints, jointParent, i, jointMod - 1 );
		i = jointMod;
		parentNum = jointParent[i];

		switch ( AFPoseJointMods[jointMod].mod ) {
			case AF_JOINTMOD_AXIS: {
				joints[i].SetRotation( AFPoseJointMods[jointMod].axis );
				joints[i].SetTranslation( joints[parentNum].ToVec3() + joints[i].ToVec3() * joints[parentNum].ToMat3() );
				break;
			}
			case AF_JOINTMOD_ORIGIN: {
				joints[i].SetRotation( joints[i].ToMat3() * joints[parentNum].ToMat3() );
				joints[i].SetTranslation( AFPoseJointMods[jointMod].origin );
				break;
			}
			case AF_JOINTMOD_BOTH: {
				joints[i].SetRotation( AFPoseJointMods[jointMod].axis );
				joints[i].SetTranslation( AFPoseJointMods[jointMod].origin );
				break;
			}
		}
	}

	// transform the rest of the hierarchy
	SIMDProcessor->TransformJoints( joints, jointParent, i, numJoints - 1 );

	// untransform hierarchy
	SIMDProcessor->UntransformJoints( joints, jointParent, 1, numJoints - 1 );

	// convert joint matrices back to joint quaternions
	SIMDProcessor->ConvertJointMatsToJointQuats( AFPoseJointFrame.Ptr(), joints, numJoints );

	// find all modified joints and their parents
	bool *blendJoints = (bool *) _alloca16( numJoints * sizeof( bool ) );
	memset( blendJoints, 0, numJoints * sizeof( bool ) );

	// mark all modified joints and their parents
	for ( int i = 0; i < AFPoseJoints.Num(); i++ ) {
		for ( jointNum = AFPoseJoints[i]; jointNum != INVALID_JOINT; jointNum = jointParent[jointNum] ) {
			blendJoints[jointNum] = true;
		}
	}

	// lock all parents of modified joints
	AFPoseJoints.SetNum( 0 );
	for ( int i = 0; i < numJoints; i++ ) {
		if ( blendJoints[i] ) {
			AFPoseJoints.Append( i );
		}
	}

	AFPoseBounds = bounds;
	AFPoseTime = time;

	ForceUpdate();
}

/*
=====================
anAnimator::SetAFPoseBlendWeight
=====================
*/
void anAnimator::SetAFPoseBlendWeight( float blendWeight ) {
	AFPoseBlendWeight = blendWeight;
}

/*
=====================
anAnimator::BlendAFPose
=====================
*/
bool anAnimator::BlendAFPose( anJointQuat *blendFrame ) const {
	if ( !AFPoseJoints.Num() ) {
		return false;
	}

	SIMDProcessor->BlendJoints( blendFrame, AFPoseJointFrame.Ptr(), AFPoseBlendWeight, AFPoseJoints.Ptr(), AFPoseJoints.Num() );

	return true;
}

/*
=====================
anAnimator::ClearAFPose
=====================
*/
void anAnimator::ClearAFPose() {
	if ( AFPoseJoints.Num() ) {
		ForceUpdate();
	}
	AFPoseBlendWeight = 1.0f;
	AFPoseJoints.SetNum( 0 );
	AFPoseBounds.Clear();
	AFPoseTime = 0;
}

/*
=====================
anAnimator::ServiceAnims
=====================
*/
void anAnimator::ServiceAnims( int fromtime, int totime ) {
	if ( !modelDef ) {
		return;
	}

	if ( modelDef->ModelHandle() ) {
		anAnimBlend *blend = channels[0];
		for ( int i = 0; i < ANIM_NumAnimChannels; i++ ) {
			for ( int j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
				blend->CallFrameCommands( entity, fromtime, totime );
			}
		}
	}

	if ( !IsAnimating( totime ) ) {
		stoppedAnimatingUpdate = true;
		if ( entity ) {
			entity->BecomeInactive( TH_ANIMATE );

			// present one more time with stopped animations so the renderer can properly recreate interactions
			entity->BecomeActive( TH_UPDATEVISUALS );
		}
	}
}

/*
=====================
anAnimator::IsAnimating
=====================
*/
bool anAnimator::IsAnimating( int currentTime ) const {
	if ( !modelDef || !modelDef->ModelHandle() ) {
		return false;
	}

	// if animating with an articulated figure
	if ( AFPoseJoints.Num() && currentTime <= AFPoseTime ) {
		return true;
	}

	anAnimBlend *blend = channels[0];
	for ( int i = 0; i < ANIM_NumAnimChannels; i++ ) {
		for ( int j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
			if ( !blend->IsDone( currentTime ) ) {
				return true;
			}
		}
	}

	return false;
}

/*
=====================
anAnimator::FrameHasChanged
=====================
*/
bool anAnimator::FrameHasChanged( int currentTime ) const {
	if ( !modelDef || !modelDef->ModelHandle() ) {
		return false;
	}

	// if animating with an articulated figure
	if ( AFPoseJoints.Num() && currentTime <= AFPoseTime ) {
		return true;
	}

	anAnimBlend *blend = channels[0];
	for ( int i = 0; i < ANIM_NumAnimChannels; i++ ) {
		for ( int j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
			if ( blend->FrameHasChanged( currentTime ) ) {
				return true;
			}
		}
	}

	if ( forceUpdate && IsAnimating( currentTime ) ) {
		return true;
	}

	return false;
}

/*
=====================
anAnimator::CreateFrame
=====================
*/
bool anAnimator::CreateFrame( int currentTime, bool force ) {
	int					i, j;
	int					numJoints;
	int					parentNum;
	bool				hasAnim;
	bool				debugInfo;
	float				baseBlend;
	float				blendWeight;
	const anAnimBlend *	blend;
	const int *			jointParent;
	const jointMod_t *	jointMod;
	const anJointQuat *	defaultPose;

	static anCVar		r_showSkel( "r_showSkel", "0", CVAR_RENDERER | CVAR_INTEGER, "", 0, 2, idCmdSystem::ArgCompletion_Integer<0,2> );

	if ( !modelDef || !modelDef->ModelHandle() ) {
		return false;
	}

	if ( !force && !r_showSkel.GetInteger() ) {
		if ( lastTransformTime == currentTime ) {
			return false;
		}
		if ( lastTransformTime != -1 && !stoppedAnimatingUpdate && !IsAnimating( currentTime ) ) {
			return false;
		}
	}

	lastTransformTime = currentTime;
	stoppedAnimatingUpdate = false;

	if ( entity && ( ( g_debugAnim.GetInteger() == entity->entityNumber ) || ( g_debugAnim.GetInteger() == -2 ) ) ) {
		debugInfo = true;
		gameLocal.Printf( "---------------\n%d: entity '%s':\n", gameLocal.time, entity->GetName() );
 		gameLocal.Printf( "model '%s':\n", modelDef->GetModelName() );
	} else {
		debugInfo = false;
	}

	// init the joint buffer
	if ( AFPoseJoints.Num() ) {
		// initialize with AF pose anim for the case where there are no other animations and no AF pose joint modifications
		defaultPose = AFPoseJointFrame.Ptr();
	} else {
		defaultPose = modelDef->GetDefaultPose();
	}

	if ( !defaultPose ) {
		//gameLocal.Warning( "anAnimator::CreateFrame: no defaultPose on '%s'", modelDef->Name() );
		return false;
	}

	numJoints = modelDef->Joints().Num();
	anJointQuat *jointFrame = ( anJointQuat * )_alloca16( numJoints * sizeof( jointFrame[0] ) );
	SIMDProcessor->Memcpy( jointFrame, defaultPose, numJoints * sizeof( jointFrame[0] ) );

	hasAnim = false;

	// blend the all channel
	baseBlend = 0.0f;
	blend = channels[ ANIMCHANNEL_ALL ];
	for ( j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
		if ( blend->BlendAnim( currentTime, ANIMCHANNEL_ALL, numJoints, jointFrame, baseBlend, removeOriginOffset, false, debugInfo ) ) {
			hasAnim = true;
			if ( baseBlend >= 1.0f ) {
				break;
			}
		}
	}

	// only blend other channels if there's enough space to blend into
	if ( baseBlend < 1.0f ) {
		for ( i = ANIMCHANNEL_ALL + 1; i < ANIM_NumAnimChannels; i++ ) {
			if ( !modelDef->NumJointsOnChannel( i ) ) {
				continue;
			}
			if ( i == ANIMCHANNEL_EYELIDS ) {
				// eyelids blend over any previous anims, so skip it and blend it later
				continue;
			}
			blendWeight = baseBlend;
			blend = channels[i];
			for ( j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
				if ( blend->BlendAnim( currentTime, i, numJoints, jointFrame, blendWeight, removeOriginOffset, false, debugInfo ) ) {
					hasAnim = true;
					if ( blendWeight >= 1.0f ) {
						// fully blended
						break;
					}
				}
			}

			if ( debugInfo && !AFPoseJoints.Num() && !blendWeight ) {
				gameLocal.Printf( "%d: %s using default pose in model '%s'\n", gameLocal.time, channelNames[i], modelDef->GetModelName() );
			}
		}
	}

	// blend in the eyelids
	if ( modelDef->NumJointsOnChannel( ANIMCHANNEL_EYELIDS ) ) {
		blend = channels[ ANIMCHANNEL_EYELIDS ];
		blendWeight = baseBlend;
		for ( j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
			if ( blend->BlendAnim( currentTime, ANIMCHANNEL_EYELIDS, numJoints, jointFrame, blendWeight, removeOriginOffset, true, debugInfo ) ) {
				hasAnim = true;
				if ( blendWeight >= 1.0f ) {
					// fully blended
					break;
				}
			}
		}
	}

	// blend the articulated figure pose
	if ( BlendAFPose( jointFrame ) ) {
		hasAnim = true;
	}

	if ( !hasAnim && !jointMods.Num() ) {
		// no animations were updated
		return false;
	}

	// convert the joint quaternions to rotation matrices
	SIMDProcessor->ConvertJointQuatsToJointMats( joints, jointFrame, numJoints );

	// check if we need to modify the origin
	if ( jointMods.Num() && ( jointMods[0]->jointNum == 0 ) ) {
		jointMod = jointMods[0];
		switch ( jointMod->transform_axis ) {
			case JOINTMOD_NONE:
				break;

			case JOINTMOD_LOCAL:
				joints[0].SetRotation( jointMod->mat * joints[0].ToMat3() );
				break;

			case JOINTMOD_WORLD:
				joints[0].SetRotation( joints[0].ToMat3() * jointMod->mat );
				break;

			case JOINTMOD_LOCAL_OVERRIDE:
			case JOINTMOD_WORLD_OVERRIDE:
				joints[0].SetRotation( jointMod->mat );
				break;
		}

		switch ( jointMod->transform_pos ) {
			case JOINTMOD_NONE:
				break;

			case JOINTMOD_LOCAL:
				joints[0].SetTranslation( joints[0].ToVec3() + jointMod->pos );
				break;

			case JOINTMOD_LOCAL_OVERRIDE:
			case JOINTMOD_WORLD:
			case JOINTMOD_WORLD_OVERRIDE:
				joints[0].SetTranslation( jointMod->pos );
				break;
		}
		j = 1;
	} else {
		j = 0;
	}

	// add in the model offset
	joints[0].SetTranslation( joints[0].ToVec3() + modelDef->GetVisualOffset() );

	// pointer to joint info
	jointParent = modelDef->JointParents();

	// add in any joint modifications
	for ( i = 1; j < jointMods.Num(); j++, i++ ) {
		jointMod = jointMods[j];

		// transform any joints preceding the joint modifier
		SIMDProcessor->TransformJoints( joints, jointParent, i, jointMod->jointNum - 1 );
		i = jointMod->jointNum;

		parentNum = jointParent[i];

		// modify the axis
		switch ( jointMod->transform_axis ) {
			case JOINTMOD_NONE:
				joints[i].SetRotation( joints[i].ToMat3() * joints[ parentNum ].ToMat3() );
				break;

			case JOINTMOD_LOCAL:
				joints[i].SetRotation( jointMod->mat * ( joints[i].ToMat3() * joints[parentNum].ToMat3() ) );
				break;

			case JOINTMOD_LOCAL_OVERRIDE:
				joints[i].SetRotation( jointMod->mat * joints[parentNum].ToMat3() );
				break;

			case JOINTMOD_WORLD:
				joints[i].SetRotation( ( joints[i].ToMat3() * joints[parentNum].ToMat3() ) * jointMod->mat );
				break;

			case JOINTMOD_WORLD_OVERRIDE:
				joints[i].SetRotation( jointMod->mat );
				break;
		}

		// modify the position
		switch ( jointMod->transform_pos ) {
			case JOINTMOD_NONE:
				joints[i].SetTranslation( joints[parentNum].ToVec3() + joints[i].ToVec3() * joints[parentNum].ToMat3() );
				break;

			case JOINTMOD_LOCAL:
				joints[i].SetTranslation( joints[parentNum].ToVec3() + ( joints[i].ToVec3() + jointMod->pos ) * joints[parentNum].ToMat3() );
				break;

			case JOINTMOD_LOCAL_OVERRIDE:
				joints[i].SetTranslation( joints[parentNum].ToVec3() + jointMod->pos * joints[parentNum].ToMat3() );
				break;

			case JOINTMOD_WORLD:
				joints[i].SetTranslation( joints[parentNum].ToVec3() + joints[i].ToVec3() * joints[parentNum].ToMat3() + jointMod->pos );
				break;

			case JOINTMOD_WORLD_OVERRIDE:
				joints[i].SetTranslation( jointMod->pos );
				break;
		}
	}

	// transform the rest of the hierarchy
	SIMDProcessor->TransformJoints( joints, jointParent, i, numJoints - 1 );

	return true;
}

/*
=====================
anAnimator::ForceUpdate
=====================
*/
void anAnimator::ForceUpdate() {
	lastTransformTime = -1;
	forceUpdate = true;
}

/*
=====================
anAnimator::ClearForceUpdate
=====================
*/
void anAnimator::ClearForceUpdate() {
	forceUpdate = false;
}

/*
=====================
anAnimator::GetJointTransform>	gamex86.dll!anAnimator::ForceUpdate()  Line 4268	C++

=====================
*/
bool anAnimator::GetJointTransform( jointHandle_t jointHandle, int currentTime, anVec3 &offset, anMat3 &axis ) {
	if ( !modelDef || ( jointHandle < 0 ) || ( jointHandle >= modelDef->NumJoints() ) ) {
		return false;
	}

	CreateFrame( currentTime, false );

	offset = joints[ jointHandle ].ToVec3();
	axis = joints[ jointHandle ].ToMat3();

	return true;
}

/*
=====================
anAnimator::GetJointLocalTransform
=====================
*/
bool anAnimator::GetJointLocalTransform( jointHandle_t jointHandle, int currentTime, anVec3 &offset, anMat3 &axis ) {
	if ( !modelDef ) {
		return false;
	}

	const anList<jointInfo_t> &modelJoints = modelDef->Joints();

	if ( ( jointHandle < 0 ) || ( jointHandle >= modelJoints.Num() ) ) {
		return false;
	}

	// FIXME: overkill
	CreateFrame( currentTime, false );

	if ( jointHandle > 0 ) {
		anJointMat m = joints[ jointHandle ];
		m /= joints[ modelJoints[ jointHandle ].parentNum ];
		offset = m.ToVec3();
		axis = m.ToMat3();
	} else {
		offset = joints[ jointHandle ].ToVec3();
		axis = joints[ jointHandle ].ToMat3();
	}

	return true;
}

/*
=====================
anAnimator::GetJointHandle
=====================
*/
jointHandle_t anAnimator::GetJointHandle( const char *name ) const {
	if ( !modelDef || !modelDef->ModelHandle() ) {
		return INVALID_JOINT;
	}

	return modelDef->ModelHandle()->GetJointHandle( name );
}

/*
=====================
anAnimator::GetJointName
=====================
*/
const char *anAnimator::GetJointName( jointHandle_t handle ) const {
	if ( !modelDef || !modelDef->ModelHandle() ) {
		return "";
	}

	return modelDef->ModelHandle()->GetJointName( handle );
}

/*
=====================
anAnimator::GetChannelForJoint
=====================
*/
int anAnimator::GetChannelForJoint( jointHandle_t joint ) const {
	if ( !modelDef ) {
		gameLocal.Error( "anAnimator::GetChannelForJoint: nullptr model" );
		return -1;
	}

	if ( ( joint < 0 ) || ( joint >= numJoints ) ) {
		gameLocal.Error( "anAnimator::GetChannelForJoint: invalid joint num (%d)", joint );
		return -1;
	}

	return modelDef->GetJoint( joint )->channel;
}

/*
=====================
anAnimator::GetFirstChild
=====================
*/
jointHandle_t anAnimator::GetFirstChild( const char *name ) const {
	return GetFirstChild( GetJointHandle( name ) );
}

/*
=====================
anAnimator::GetFirstChild
=====================
*/
jointHandle_t anAnimator::GetFirstChild( jointHandle_t jointNum ) const {
	int					i;
	int					num;
	const jointInfo_t	*joint;

	if ( !modelDef ) {
		return INVALID_JOINT;
	}

	num = modelDef->NumJoints();
	if ( !num ) {
		return jointNum;
	}
	joint = modelDef->GetJoint( 0 );
	for ( i = 0; i < num; i++, joint++ ) {
		if ( joint->parentNum == jointNum ) {
			return ( jointHandle_t )joint->num;
		}
	}
	return jointNum;
}

/*
=====================
anAnimator::GetJoints
=====================
*/
void anAnimator::GetJoints( int *numJoints, anJointMat **jointsPtr ) {
	*numJoints = this->numJoints;
	*jointsPtr = this->joints;
}

/*
=====================
anAnimator::GetAnimFlags
=====================
*/
const animFlags_t anAnimator::GetAnimFlags( int animNum ) const {
	animFlags_t result;

	const anAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->GetAnimFlags();
	}

	memset( &result, 0, sizeof( result ) );
	return result;
}

/*
=====================
anAnimator::NumFrames
=====================
*/
int	anAnimator::NumFrames( int animNum ) const {
	const anAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->NumFrames();
	} else {
		return 0;
	}
}

/*
=====================
anAnimator::NumSyncedAnims
=====================
*/
int	anAnimator::NumSyncedAnims( int animNum ) const {
	const anAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->NumAnims();
	} else {
		return 0;
	}
}

/*
=====================
anAnimator::AnimName
=====================
*/
const char *anAnimator::AnimName( int animNum ) const {
	const anAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->Name();
	} else {
		return "";
	}
}

/*
=====================
anAnimator::AnimFullName
=====================
*/
const char *anAnimator::AnimFullName( int animNum ) const {
	const anAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->FullName();
	} else {
		return "";
	}
}

/*
=====================
anAnimator::AnimLength
=====================
*/
int	anAnimator::AnimLength( int animNum ) const {
	const anAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->Length();
	} else {
		return 0;
	}
}

/*
=====================
anAnimator::TotalMovementDelta
=====================
*/
const anVec3 &anAnimator::TotalMovementDelta( int animNum ) const {
	const anAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->TotalMovementDelta();
	} else {
		return vec3_origin;
	}
}

/*
===============================================================================

	anAnimated

===============================================================================
*/

const anEventDefInternal EV_Animated_Start( "internal_start" );
const anEventDefInternal EV_AnimDone( "internal_animDone", "d" );
const anEventDef EV_StartRagdoll( "startRagdoll", '\0', DOC_TEXT( "Switches the entity into ragdoll mode from its current animation pose." ), 0, "This will do nothing if the entity does not have an $decl:articulatedFigure$ set up." );

CLASS_DECLARATION( anAFEntity_Base, anAnimated )
	EVENT( EV_Activate,				anAnimated::Event_Activate )
	EVENT( EV_Animated_Start,		anAnimated::Event_Start )
	EVENT( EV_StartRagdoll,			anAnimated::Event_StartRagdoll )
	EVENT( EV_AnimDone,				anAnimated::Event_AnimDone )
END_CLASS

/*
===============
anAnimated::anAnimated
================
*/
anAnimated::anAnimated() {
	anim = 0;
	blendFrames = 0;
	soundJoint = INVALID_JOINT;
	activated = false;
	combatModel = nullptr;
	activator = nullptr;
	currentAnimIndex = 0;
	numAnims = 0;

}

/*
===============
anAnimated::anAnimated
================
*/
anAnimated::~anAnimated() {
	gameLocal.clip.DeleteClipModel( combatModel );
	combatModel = nullptr;
}

/*
===============
anAnimated::Spawn
================
*/
void anAnimated::Spawn( void ) {
	anStr		animname;
	int			anim2;
	float		wait;
	const char	*joint;

	joint = spawnArgs.GetString( "sound_bone", "origin" );
	soundJoint = animator.GetJointHandle( joint );
	if ( soundJoint == INVALID_JOINT ) {
		gameLocal.Warning( "anAnimated '%s' at (%s): cannot find joint '%s' for sound playback", name.c_str(), GetPhysics()->GetOrigin().ToString( 0 ), joint );
	}

	LoadAF();

	// allow bullets to collide with a combat model
	if ( spawnArgs.GetBool( "combatModel", "0" ) ) {
		combatModel = new anClipModel( modelDefHandle );
	}

	// allow the entity to take damage
	if ( spawnArgs.GetBool( "takeDamage", "0" ) ) {
		fl.takedamage = true;
	}

	blendFrames = 0;

	currentAnimIndex = 0;
	spawnArgs.GetInt( "numAnims", "0", numAnims );

	blendFrames = spawnArgs.GetInt( "blend_in" );

	animname = spawnArgs.GetString( numAnims ? "anim1" : "anim" );
	if ( !animname.Length() ) {
		anim = 0;
	} else {
		anim = animator.GetAnim( animname );
		if ( !anim ) {
			gameLocal.Error( "anAnimated '%s' at (%s): cannot find anim '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString( 0 ), animname.c_str() );
		}
	}

	if ( spawnArgs.GetBool( "hide" ) ) {
		Hide();

		if ( !numAnims ) {
			blendFrames = 0;
		}
	} else if ( spawnArgs.GetString( "start_anim", "", animname ) ) {
		anim2 = animator.GetAnim( animname );
		if ( !anim2 ) {
			gameLocal.Error( "anAnimated '%s' at (%s): cannot find anim '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString( 0 ), animname.c_str() );
		}
		animator.CycleAnim( ANIMCHANNEL_ALL, anim2, gameLocal.time, 0 );
	} else if ( anim ) {
		// init joints to the first frame of the animation
		animator.SetFrame( ANIMCHANNEL_ALL, anim, 1, gameLocal.time, 0 );

		if ( !numAnims ) {
			blendFrames = 0;
		}
	}

	spawnArgs.GetFloat( "wait", "-1", wait );

	if ( wait >= 0 ) {
		PostEventSec( &EV_Activate, wait, this );
	}
}

/*
===============
anAnimated::LoadAF
===============
*/
bool anAnimated::LoadAF( void ) {
	anStr fileName;

	if ( !spawnArgs.GetString( "ragdoll", "*unknown*", fileName ) ) {
		return false;
	}
	af.SetAnimator( GetAnimator() );
	return af.Load( this, fileName );
}

/*
===============
anAnimated::GetPhysicsToSoundTransform
===============
*/
bool anAnimated::GetPhysicsToSoundTransform( anVec3 &origin, anMat3 &axis ) {
	animator.GetJointTransform( soundJoint, gameLocal.time, origin, axis );
	axis = renderEntity.axis;
	return true;
}

/*
================
anAnimated::StartRagdoll
================
*/
bool anAnimated::StartRagdoll( void ) {
	// if no AF loaded
	if ( !af.IsLoaded() ) {
		return false;
	}

	// if the AF is already active
	if ( af.IsActive() ) {
		return true;
	}

	// disable any collision model used
	GetPhysics()->UnlinkClip();

	// start using the AF
	af.StartFromCurrentPose( spawnArgs.GetInt( "velocityTime", "0" ) );

	return true;
}

/*
=====================
anAnimated::PlayNextAnim
=====================
*/
void anAnimated::PlayNextAnim( void ) {
	const char *animName;
	int len;
	int cycle;

	if ( currentAnimIndex >= numAnims ) {
		Hide();
		if ( spawnArgs.GetBool( "remove" ) ) {
			PostEventMS( &EV_Remove, 0 );
		} else {
			currentAnimIndex = 0;
		}
		return;
	}

	Show();
	currentAnimIndex++;

	spawnArgs.GetString( va( "Animimation%d", currentAnimIndex ), nullptr, &animName );
	if ( !animName ) {
		anim = 0;
		animator.Clear( ANIMCHANNEL_ALL, gameLocal.time, FRAME2MS( blendFrames ) );
		return;
	}

	anim = animator.GetAnim( animName );
	if ( !anim ) {
		gameLocal.Warning( "Missing Animimation '%s' on %s", animName, name.c_str() );
		return;
	}

	if ( g_debugCinematic.GetBool() ) {
		gameLocal.Printf( "%d: '%s' Start Animimation '%s'\n", gameLocal.frameNum, GetName(), animName );
	}

	spawnArgs.GetInt( "cycle", "1", cycle );
	if ( ( currentAnimIndex == numAnims ) && spawnArgs.GetBool( "loop_last_anim" ) ) {
		cycle = -1;
	}

	animator.CycleAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, FRAME2MS( blendFrames ) );
	animator.CurrentAnim( ANIMCHANNEL_ALL )->SetCycleCount( cycle );

	len = animator.CurrentAnim( ANIMCHANNEL_ALL )->PlayLength();
	if ( len >= 0 ) {
		PostEventMS( &EV_AnimDone, len, currentAnimIndex );
	}

	// offset the start time of the shader to sync it to the game time
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );

	animator.ForceUpdate();
	UpdateAnimation();
	UpdateVisuals();
	Present();
}

/*
===============
anAnimated::Event_StartRagdoll
================
*/
void anAnimated::Event_StartRagdoll( void ) {
	StartRagdoll();
}

/*
===============
anAnimated::Event_AnimDone
================
*/
void anAnimated::Event_AnimDone( int animindex ) {
	if ( g_debugCinematic.GetBool() ) {
		const anAnim *animPtr = animator.GetAnim( anim );
		gameLocal.Printf( "%d: '%s' end anim '%s'\n", gameLocal.frameNum, GetName(), animPtr ? animPtr->Name() : "" );
	}

	if ( ( animindex >= numAnims ) && spawnArgs.GetBool( "remove" ) ) {
		Hide();
		PostEventMS( &EV_Remove, 0 );
	} else if ( spawnArgs.GetBool( "auto_advance" ) ) {
		PlayNextAnim();
	} else {
		activated = false;
	}
}

/*
===============
anAnimated::Event_Activate
================
*/
void anAnimated::Event_Activate( anEntity *_activator ) {
	if ( numAnims ) {
		PlayNextAnim();
		activator = _activator;
		return;
	}

	if ( activated ) {
		// already activated
		return;
	}

	activated = true;
	activator = _activator;
	ProcessEvent( &EV_Animated_Start );
}

/*
===============
anAnimated::Event_Start
================
*/
void anAnimated::Event_Start( void ) {
	int cycle;
	int len;

	Show();

	if ( numAnims ) {
		PlayNextAnim();
		return;
	}

	if ( anim ) {
		if ( g_debugCinematic.GetBool() ) {
			const anAnim *animPtr = animator.GetAnim( anim );
			gameLocal.Printf( "%d: '%s' start anim '%s'\n", gameLocal.frameNum, GetName(), animPtr ? animPtr->Name() : "" );
		}
		spawnArgs.GetInt( "cycle", "1", cycle );
		animator.CycleAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, FRAME2MS( blendFrames ) );
		animator.CurrentAnim( ANIMCHANNEL_ALL )->SetCycleCount( cycle );

		len = animator.CurrentAnim( ANIMCHANNEL_ALL )->PlayLength();
		if ( len >= 0 ) {
			PostEventMS( &EV_AnimDone, len, 1 );
		}
	}

	// offset the start time of the shader to sync it to the game time
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );

	animator.ForceUpdate();
	UpdateAnimation();
	UpdateVisuals();
	Present();
}

/***********************************************************************

	Util functions

***********************************************************************/

/*
=====================
ANIM_GetModelDefFromEntityDef
=====================
*/
const anDeclModelDef *ANIM_GetModelDefFromEntityDef( const anDict *args ) {
	const anDeclModelDef *modelDef;

	anStr name = args->GetString( "model" );
	modelDef = static_cast<const anDeclModelDef *>( declManager->FindType( DECL_MODELDEF, name, false ) );
	if ( modelDef != nullptr && modelDef->ModelHandle() ) {
		return modelDef;
	}

	return nullptr;
}

/*
=====================
anGameEdit::ANIM_GetModelFromEntityDef
=====================
*/
anRenderModel *anGameEdit::ANIM_GetModelFromEntityDef( const anDict *args ) {
	anRenderModel *model;
	const anDeclModelDef *modelDef;

	model = nullptr;

	anStr name = args->GetString( "model" );
	modelDef = static_cast<const anDeclModelDef *>( declManager->FindType( DECL_MODELDEF, name, false ) );
	if ( modelDef != nullptr ) {
		model = modelDef->ModelHandle();
	}

	if ( model == nullptr ) {
		model = renderModelManager->FindModel( name );
	}

	if ( model != nullptr && model->IsDefaultModel() ) {
		return nullptr;
	}

	return model;
}

/*
=====================
anGameEdit::ANIM_GetModelFromEntityDef
=====================
*/
anRenderModel *anGameEdit::ANIM_GetModelFromEntityDef( const char *classname ) {
	const anDict *args;

	args = gameLocal.FindEntityDefDict( classname, false );
	if ( !args ) {
		return nullptr;
	}

	return ANIM_GetModelFromEntityDef( args );
}

/*
=====================
anGameEdit::ANIM_GetModelOffsetFromEntityDef
=====================
*/
const anVec3 &anGameEdit::ANIM_GetModelOffsetFromEntityDef( const char *classname ) {
	const anDict *args;
	const anDeclModelDef *modelDef;

	args = gameLocal.FindEntityDefDict( classname, false );
	if ( !args ) {
		return vec3_origin;
	}

	modelDef = ANIM_GetModelDefFromEntityDef( args );
	if ( !modelDef ) {
		return vec3_origin;
	}

	return modelDef->GetVisualOffset();
}

/*
=====================
anGameEdit::ANIM_GetModelFromName
=====================
*/
anRenderModel *anGameEdit::ANIM_GetModelFromName( const char *modelName ) {
	const anDeclModelDef *modelDef;
	anRenderModel *model;

	model = nullptr;
	modelDef = static_cast<const anDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelName, false ) );
	if ( modelDef ) {
		model = modelDef->ModelHandle();
	}
	if ( !model ) {
		model = renderModelManager->FindModel( modelName );
	}
	return model;
}

/*
=====================
anGameEdit::ANIM_GetAnimFromEntityDef
=====================
*/
const anMD6Anim *anGameEdit::ANIM_GetAnimFromEntityDef( const char *classname, const char *animname ) {
	const anDict *args;
	const anMD6Anim *md6anim;
	const anAnim *anim;
	int	animNum;
	const char	*modelname;
	const anDeclModelDef *modelDef;

	args = gameLocal.FindEntityDefDict( classname, false );
	if ( !args ) {
		return nullptr;
	}

	md6anim = nullptr;
	modelname = args->GetString( "model" );
	modelDef = static_cast<const anDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelname, false ) );
	if ( modelDef ) {
		animNum = modelDef->GetAnim( animname );
		if ( animNum ) {
			anim = modelDef->GetAnim( animNum );
			if ( anim ) {
				md6anim = anim->MD6Anim( 0 );
			}
		}
	}
	return md6anim;
}

/*
=====================
anGameEdit::ANIM_GetNumAnimsFromEntityDef
=====================
*/
int anGameEdit::ANIM_GetNumAnimsFromEntityDef( const anDict *args ) {
	const char *modelname;
	const anDeclModelDef *modelDef;

	modelname = args->GetString( "model" );
	modelDef = static_cast<const anDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelname, false ) );
	if ( modelDef ) {
		return modelDef->NumAnims();
	}
	return 0;
}

/*
=====================
anGameEdit::ANIM_GetAnimNameFromEntityDef
=====================
*/
const char *anGameEdit::ANIM_GetAnimNameFromEntityDef( const anDict *args, int animNum ) {
	const char *modelname;
	const anDeclModelDef *modelDef;

	modelname = args->GetString( "model" );
	modelDef = static_cast<const anDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelname, false ) );
	if ( modelDef ) {
		const anAnim* anim = modelDef->GetAnim( animNum );
		if ( anim ) {
			return anim->FullName();
		}
	}
	return "";
}

/*
=====================
anGameEdit::ANIM_GetAnim
=====================
*/
const anMD6Anim *anGameEdit::ANIM_GetAnim( const char *fileName ) {
	return animationLib.GetAnim( fileName );
}

/*
=====================
anGameEdit::ANIM_GetLength
=====================
*/
int	anGameEdit::ANIM_GetLength( const anMD6Anim *anim ) {
	if ( !anim ) {
		return 0;
	}
	return anim->Length();
}

/*
=====================
anGameEdit::ANIM_GetNumFrames
=====================
*/
int anGameEdit::ANIM_GetNumFrames( const anMD6Anim *anim ) {
	if ( !anim ) {
		return 0;
	}
	return anim->NumFrames();
}

/*
=====================
anGameEdit::ANIM_CreateAnimFrame
=====================
*/
void anGameEdit::ANIM_CreateAnimFrame( const anRenderModel *model, const anMD6Anim *anim, int numJoints, anJointMat *joints, int time, const anVec3 &offset, bool remove_origin_offset ) {
	int					i;
	frameBlend_t		frame;
	const anMD6Joint	*md6joints;
	int					*index;

	if ( !model || model->IsDefaultModel() || !anim ) {
		return;
	}

	if ( numJoints != model->NumJoints() ) {
		gameLocal.Error( "ANIM_CreateAnimFrame: different # of joints in renderEntity_t than in model (%s)", model->Name() );
	}

	if ( !model->NumJoints() ) {
		// FIXME: Print out a warning?
		return;
	}

	if ( !joints ) {
		gameLocal.Error( "ANIM_CreateAnimFrame: nullptr joint frame pointer on model (%s)", model->Name() );
	}

	if ( numJoints != anim->NumJoints() ) {
		gameLocal.Warning( "Model '%s' has different # of joints than anim '%s'", model->Name(), anim->Name() );
		for ( i = 0; i < numJoints; i++ ) {
			joints[i].SetRotation( mat3_identity );
			joints[i].SetTranslation( offset );
		}
		return;
	}

	// create index for all joints
	index = (int *)_alloca16( numJoints * sizeof( int ) );
	for ( i = 0; i < numJoints; i++ ) {
		index[i] = i;
	}

	// create the frame
	anim->ConvertTimeToFrame( time, 1, frame );
	anJointQuat *jointFrame = ( anJointQuat * )_alloca16( numJoints * sizeof( *jointFrame ) );
	anim->GetInterpolatedFrame( frame, jointFrame, index, numJoints );

	// convert joint quaternions to joint matrices
	SIMDProcessor->ConvertJointQuatsToJointMats( joints, jointFrame, numJoints );

	// first joint is always root of entire hierarchy
	if ( remove_origin_offset ) {
		joints[0].SetTranslation( offset );
	} else {
		joints[0].SetTranslation( joints[0].ToVec3() + offset );
	}

	// transform the children
	md6joints = model->GetJoints();
	for ( i = 1; i < numJoints; i++ ) {
		joints[i] *= joints[ md6joints[i].parent - md6joints ];
	}
}

/*
=====================
anGameEdit::ANIM_CreateMeshForAnim
=====================
*/
anRenderModel *anGameEdit::ANIM_CreateMeshForAnim( anRenderModel *model, const char *classname, const char *animname, int frame, bool remove_origin_offset ) {
	renderEntity_t			ent;
	const anDict			*args;
	const char				*temp;
	anRenderModel			*newmodel;
	const anMD6Anim 		*md6anim;
	anStr					filename;
	anStr					extension;
	const anAnim			*anim;
	int						animNum;
	anVec3					offset;
	const anDeclModelDef	*modelDef;

	if ( !model || model->IsDefaultModel() ) {
		return nullptr;
	}

	args = gameLocal.FindEntityDefDict( classname, false );
	if ( !args ) {
		return nullptr;
	}

	memset( &ent, 0, sizeof( ent ) );

	ent.bounds.Clear();
	ent.suppressSurfaceInViewID = 0;

	modelDef = ANIM_GetModelDefFromEntityDef( args );
	if ( modelDef ) {
		animNum = modelDef->GetAnim( animname );
		if ( !animNum ) {
			return nullptr;
		}
		anim = modelDef->GetAnim( animNum );
		if ( !anim ) {
			return nullptr;
		}
		md6anim = anim->MD6Anim( 0 );
		ent.customSkin = modelDef->GetDefaultSkin();
		offset = modelDef->GetVisualOffset();
	} else {
		filename = animname;
		filename.ExtractFileExtension( extension );
		if ( !extension.Length() ) {
			animname = args->GetString( va( "anim %s", animname ) );
		}

		md6anim = animationLib.GetAnim( animname );
		offset.Zero();
	}

	if ( !md6anim ) {
		return nullptr;
	}

	temp = args->GetString( "skin", "" );
	if ( temp[0] ) {
		ent.customSkin = declManager->FindSkin( temp );
	}

	ent.numJoints = model->NumJoints();
	ent.joints = ( anJointMat * )Mem_Alloc16( SIMD_ROUND_JOINTS( ent.numJoints ) * sizeof( *ent.joints ), TAG_JOINTMAT );

	ANIM_CreateAnimFrame( model, md6anim, ent.numJoints, ent.joints, FRAME2MS( frame ), offset, remove_origin_offset );

	SIMD_INIT_LAST_JOINT( ent.joints, ent.numJoints );

	newmodel = model->InstantiateDynamicModel( &ent, nullptr, nullptr );

	Mem_Free16( ent.joints );
	ent.joints = nullptr;

	return newmodel;
}
